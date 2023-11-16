#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "rom.h"

//#define SLOW
#define DEBUG

#define CLK_PIN 21
#define RESET_PIN 26
#define BT_U5_OE_PIN 8
#define BT_U6_OE_PIN 9
#define BT_U7_OE_PIN 10
#define RW_PIN 11

#define CLK_PIN_MASK (1 << CLK_PIN)
#define RESET_PIN_MASK (1 << RESET_PIN)
#define BT_U5_OE_PIN_MASK (1 << BT_U5_OE_PIN)
#define BT_U6_OE_PIN_MASK (1 << BT_U6_OE_PIN)
#define BT_U7_OE_PIN_MASK (1 << BT_U7_OE_PIN)
#define RW_PIN_MASK (1 << RW_PIN)

#define GPIO_PINS_MASK 0xFF

#ifdef SLOW
#define TIME_DELTA sleep_ms(250);
#define TIME_DELTA_SMALL sleep_us(1);
#define TIME_DELTA_LARGE sleep_ms(250);
#else
#define TIME_DELTA asm volatile("nop \nnop \nnop \nnop \nnop \nnop \nnop \nnop \n");
#define TIME_DELTA_SMALL asm volatile("nop \nnop \nnop \nnop \n");
#define TIME_DELTA_LARGE sleep_us(1);
#endif

uint8_t ram[64 * 1024];
uint64_t clks; 
uint64_t start;


void set_clock(const int val){
    gpio_put(CLK_PIN, val);
}


uint32_t sample_address()
{
    uint32_t raw_data;
    uint32_t gpio_vals;

    gpio_set_mask(BT_U7_OE_PIN_MASK | BT_U5_OE_PIN_MASK);

    gpio_put(BT_U6_OE_PIN, 0);

    // stabilize and sample GPIOs
    // do set clock vs nops
    TIME_DELTA
    gpio_vals = gpio_get_all();

    raw_data = ((gpio_vals & 0xff) << 8 );


    // select the correct bus tranceiver
    gpio_put(BT_U5_OE_PIN, 0);
    gpio_put(BT_U6_OE_PIN, 1);
    gpio_put(BT_U7_OE_PIN, 1);
    // stabilize and sample GPIOs
    
    TIME_DELTA
    gpio_vals = gpio_get_all();
    raw_data = raw_data | ((gpio_vals & 0xff) );

    return raw_data;
}

void wait(){
    while(true) {
        sleep_ms(1000);
    }
}


#define PROC_PORT_TOP   0x1
#define LOWER_RAM_TOP   0x9fff
#define VIC_BASE        0xd000    
#define VIC_TOP         0xd3ff

bool kernal_rom_visible = 1;
bool basic_rom_visible = 1;
bool io_visible = 1;

#define PROC_REG 0x1
#define PROC_DIR 0x0

char * cia_names[] = {"test"};

#define CIA_1_BASE 0xdc00
#define CIA_1_TOP 0xdcff
#define CIA_2_BASE 0xdd00
#define CIA_2_TOP 0xddff

uint32_t write_to_cia(uint32_t cia_id, uint32_t rel_address, uint32_t val) {
    // registers are repeated in data range
    rel_address = rel_address & 0x0f;

    switch (rel_address) {
        case 0x0:
            printf("name %s\n", cia_names[0]);
            break;
        default:
            printf("ignore write to cia %d, register 0x%x with value 0x%02x\n", cia_id, rel_address, val);
            break;
    }

    return val;

}

#define CID_BASE 0xd400
#define CID_TOP  0xd7ff

uint32_t write_to_cid(uint32_t rel_address, uint32_t val) {
    // registers are repeated in data range
    rel_address = rel_address & 0x1f;

    switch (rel_address) {
        case 0x0:
            printf("name %s\n", cia_names[0]);
            break;
        default:
            printf("ignore write to cid, register 0x%x with value 0x%02x\n", rel_address, val);
            break;
    }

    return val;

}

uint32_t read_from_proc(uint32_t address)
{
    gpio_put(BT_U5_OE_PIN, 1);
    gpio_put(BT_U6_OE_PIN, 1);
    gpio_put(BT_U7_OE_PIN, 0);

    // stabilize and sample GPIOs
    TIME_DELTA

    uint32_t gpio_vals = gpio_get_all() & 0xff;

    if (address > PROC_PORT_TOP && address <= LOWER_RAM_TOP) {
        // processor port registers
        // NOTE no range check (all addressing are backed by RAM!)
        ram[address] = gpio_vals;
        return gpio_vals;
    }
    else if (address >= VIC_BASE && address <= VIC_TOP) {
        printf("write to VIC\n");
        ram[address] = gpio_vals;
        return gpio_vals;
    }
    else if (address >= CIA_1_BASE && address <= CIA_1_TOP) {
        ram[address] = gpio_vals;
        return write_to_cia(1, address - CIA_1_BASE, gpio_vals);
    }
    else if (address >= CIA_2_BASE && address <= CIA_2_TOP) {
        ram[address] = gpio_vals;
        return write_to_cia(2, address - CIA_2_BASE, gpio_vals);
    }
    else if (address >= CID_BASE && address <= CID_TOP) {
        ram[address] = gpio_vals;
        return write_to_cid(address - CID_BASE, gpio_vals);
    }

    switch (address) {
        case PROC_DIR: 
            printf("write to proc reg\n");
            if ((gpio_vals & 0xff) != 0x2f) {
                printf("this proc write not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
                wait();
            }
            else {
                //
            }

            return 0;
        case PROC_REG:  
            printf("write to proc reg\n");
            if ((gpio_vals & 0x7) != 0x07) {
                printf("this proc write not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
                wait();
            }
            else {
                kernal_rom_visible = 1;
                basic_rom_visible = 1;
                io_visible = 1;
            }

            // ignore writes to datasette 
            return 0;
        
    }
}

uint32_t bus_transaction(uint32_t address, uint32_t write)
{
    gpio_put(BT_U5_OE_PIN, 1);
    gpio_put(BT_U6_OE_PIN, 1);
    gpio_put(BT_U7_OE_PIN, 0);

    // stabilize and sample GPIOs
    TIME_DELTA

    uint32_t gpio_vals = gpio_get_all() & 0xff;

    if (address > PROC_PORT_TOP && address <= LOWER_RAM_TOP) {
        // processor port registers
        // NOTE no range check (all addressing are backed by RAM!)
        ram[address] = gpio_vals;
        return gpio_vals;
    }
    else if (address >= VIC_BASE && address <= VIC_TOP) {
        printf("write to VIC\n");
        ram[address] = gpio_vals;
        return gpio_vals;
    }
    else if (address >= CIA_1_BASE && address <= CIA_1_TOP) {
        ram[address] = gpio_vals;
        return write_to_cia(1, address - CIA_1_BASE, gpio_vals);
    }
    else if (address >= CIA_2_BASE && address <= CIA_2_TOP) {
        ram[address] = gpio_vals;
        return write_to_cia(2, address - CIA_2_BASE, gpio_vals);
    }
    else if (address >= CID_BASE && address <= CID_TOP) {
        ram[address] = gpio_vals;
        return write_to_cid(address - CID_BASE, gpio_vals);
    }

    switch (address) {
        case PROC_DIR: 
            printf("write to proc reg\n");
            if ((gpio_vals & 0xff) != 0x2f) {
                printf("this proc write not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
                wait();
            }
            else {
                //
            }

            return 0;
        case PROC_REG:  
            printf("write to proc reg\n");
            if ((gpio_vals & 0x7) != 0x07) {
                printf("this proc write not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
                wait();
            }
            else {
                kernal_rom_visible = 1;
                basic_rom_visible = 1;
                io_visible = 1;
            }

            // ignore writes to datasette 
            return 0;
        
    }
    
    
    printf("writes not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
    wait();

    return gpio_vals;
}


#define BASE 0x600

#define KERNAL_BASE 0xE000
#define KERNAL_END  0xFFFF




uint8_t read_from_mem(uint32_t address){
    
    if (address >= KERNAL_BASE && address <= KERNAL_END)
    {
        return kernal_rom[address - KERNAL_BASE];
    }

    if (address > 0x1 && address <= 0x9fff)
    {
        return ram[address];
    }
    printf("address not supported for reading: 0x%4x\n", address);
    
    wait();

    return 0;
}

uint8_t read_control() {
    return gpio_get(RW_PIN);
}


void simulate_mem_read(uint32_t data){
    // set data on databus and switch tranceivers with one read
    gpio_set_dir_out_masked(0x7FF);
    gpio_put_masked(0x7FF, data | 0x300);
    // 0x300 -> u5 and u6 are set
}

int main() {
    stdio_init_all();

    printf("\nStart over USB and uart\n");

    // init memory
    for (int i=0; i<sizeof(ram); i++) {
        ram[i] = 0xff;
    }

    gpio_init_mask(CLK_PIN_MASK | RESET_PIN_MASK | BT_U5_OE_PIN_MASK | BT_U6_OE_PIN_MASK |
        BT_U7_OE_PIN_MASK | RW_PIN_MASK | GPIO_PINS_MASK);

    gpio_set_dir_out_masked(CLK_PIN_MASK | RESET_PIN_MASK | BT_U5_OE_PIN_MASK | 
        BT_U6_OE_PIN_MASK | BT_U7_OE_PIN_MASK);

    gpio_set_mask(BT_U5_OE_PIN_MASK | BT_U6_OE_PIN_MASK | BT_U7_OE_PIN_MASK);

    gpio_put(RESET_PIN, 0);

    for (int i=0; i<2; i++) {
        TIME_DELTA_LARGE
        set_clock(0);
        TIME_DELTA_LARGE
        set_clock(1);
    }

    TIME_DELTA_LARGE
    gpio_put(RESET_PIN, 1);
    TIME_DELTA_LARGE

    while (true) {
        // CLOCK FALL
        set_clock(0);
        clks++;

        gpio_set_dir_in_masked(GPIO_PINS_MASK);

        uint32_t sampled_address = sample_address();
        uint8_t control = read_control();

        //  CLOCK RISE
        set_clock(1);
        
        uint8_t mem_output;

        if (control == 1) {
            mem_output = read_from_mem(sampled_address);
            simulate_mem_read(mem_output);
        }
        else {
            mem_output = read_from_proc(sampled_address);
        }

#ifdef DEBUG
        TIME_DELTA_LARGE
        printf("CLK: %6lld A: %04x RW:%c D: %02x\n", 
                clks,  sampled_address, control == 1 ? 'r': 'w', mem_output);
            
#endif
    }
}
