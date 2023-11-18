#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "rom.h"

#include "chip_cia.h"

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

#define CIA_1_BASE 0xdc00
#define CIA_1_TOP 0xdcff
#define CIA_2_BASE 0xdd00
#define CIA_2_TOP 0xddff

#define CIA_RANGE 0xff

struct bus_device {
    uint32_t address_start;
    uint32_t address_range;
    uint32_t index;
    char * label;
    void (*write_to_device)(uint32_t cia_id, uint32_t rel_address, uint8_t val);
    uint8_t (*read_from_device)(uint32_t cia_id, uint32_t rel_address);
};


struct bus_device bus_device_list[] = {
    {CIA_1_BASE, CIA_RANGE, 0, "CIA1", &write_to_cia, &read_from_cia},
    {CIA_2_BASE, CIA_RANGE, 1, "CIA2", &write_to_cia, &read_from_cia},
};

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

#define CIA_1_BASE 0xdc00
#define CIA_1_TOP 0xdcff
#define CIA_2_BASE 0xdd00
#define CIA_2_TOP 0xddff

#define CID_BASE 0xd400
#define CID_TOP  0xd7ff

char * cia_names[] = {"cia_1", "cia_2"};

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

#define BASE 0x600

#define KERNAL_BASE 0xE000
#define KERNAL_END  0xFFFF


uint8_t bus_transaction(uint32_t address, uint32_t read)
{
    gpio_put(BT_U5_OE_PIN, 1);
    gpio_put(BT_U6_OE_PIN, 1);
    gpio_put(BT_U7_OE_PIN, 0);

    // stabilize and sample GPIOs
    TIME_DELTA

    uint32_t gpio_vals;

    if (!read) {
         gpio_vals = gpio_get_all() & 0xff;
    }

    if (address > PROC_PORT_TOP && address <= LOWER_RAM_TOP) {
        if (!read) {
            // processor port registers
            // NOTE no range check (all addressing are backed by RAM!)
            ram[address] = gpio_vals;
            return gpio_vals;
        }
        else {
            return ram[address];
        }
    }
    else if (address >= VIC_BASE && address <= VIC_TOP) {
        if (!read) {
            printf("write to VIC\n");
            ram[address] = gpio_vals;
            return gpio_vals;
        }
        else {
            printf("not supported address read: 0x%04xx\n", address);
            wait();
            return 0;
        }
    }
    else if (address >= CID_BASE && address <= CID_TOP) {
        if (!read) {
            ram[address] = gpio_vals;
            return write_to_cid(address - CID_BASE, gpio_vals);
        }
        else {
            printf("not supported address read: 0x%04xx\n", address);
            wait();
            return 0;
        }
    }
    else if (address >= KERNAL_BASE && address <= KERNAL_END)
    {
        if (!read) {
            printf("not supported address write in KERNAL: 0x%04x\n", address);
            wait();
        }
        else {
            return kernal_rom[address - KERNAL_BASE];
        }
    }

    for (int i=0; i<sizeof(bus_device_list)/sizeof(bus_device_list[0]); i++) {
        // continue of this is not the device
        if (!(address >= bus_device_list[i].address_start 
            && address < bus_device_list[i].address_start + bus_device_list[i].address_range)) {
            continue;
        }

        printf("found transaction to device %s on address 0x%04x %d\n", 
            bus_device_list[i].label, address, i);

        if (!read) {
            bus_device_list[i].write_to_device(
                bus_device_list[i].index, 
                address - bus_device_list[i].address_start,
                gpio_vals);

            return gpio_vals;
        }
        else {
            return bus_device_list[i].read_from_device(
                bus_device_list[i].index, 
                address - bus_device_list[i].address_start);
            wait();
        }

        
    }


    switch (address) {
        case PROC_DIR: 
            if (!read) {
                printf("write to proc reg\n");
                if ((gpio_vals & 0xff) != 0x2f) {
                    printf("this proc write not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
                    wait();
                }
                else {
                    //
                }
            }
            else {
                printf("not supported address read: 0x%04xx\n", address);
                wait();
                return 0;
            }
            return 0;
        case PROC_REG:  
            if (!read) {
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
            else {
                printf("not supported address read: 0x%04xx\n", address);
                wait();
                return 0;
            }
    }
    
    if (!read) {
        printf("writes not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
    }
    else {
        printf("read not supported to address 0x%04x (value: 0x%02x)\n", address, gpio_vals);
    }
    wait();

    return gpio_vals;
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

    cia_init();

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



        if (control) {
            mem_output = bus_transaction(sampled_address, control);
            simulate_mem_read(mem_output);
        }
        else {
            mem_output = bus_transaction(sampled_address, control);
        }


#ifdef DEBUG
        TIME_DELTA_LARGE
        printf("CLK: %6lld A: %04x RW:%c ", 
                clks,  sampled_address, control ? 'r': 'w');
            
#endif

#ifdef DEBUG
        TIME_DELTA_LARGE
        //printf("CLK: %6lld A: %04x RW:%c D: %02x\n", 
        //            clks,  sampled_address, control == 1 ? 'r': 'w', mem_output);

        printf("%02x\n", mem_output);
        

#endif
    }
}
