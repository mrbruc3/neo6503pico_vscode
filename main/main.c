#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"
//#include "hardware/uart.h"

//#define SLOW
//#define DEBUG


#define UART_ID uart0
#define BAUD_RATE 115200
/*
// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 28
#define UART_RX_PIN 29
*/

#define DEF_LED_PIN 23

#define CLK_PIN 21

#define RESET_PIN 26

#define BT_U5_OE 8
#define BT_U6_OE 9
#define BT_U7_OE 10

#define RW_PIN 11



#ifdef SLOW
#define TIME_DELTA sleep_ms(250);
#define TIME_DELTA_SMALL sleep_us(1);
#else
#define TIME_DELTA asm volatile("nop \nnop \nnop \nnop \nnop \nnop \nnop \nnop \n");
#define TIME_DELTA_SMALL asm volatile("nop \nnop \nnop \nnop \n");
#endif

#define TIME_DELTA_LARGE sleep_ms(1);

uint8_t ram[64 * 1024];

#define GPIO_MASK 0xFF

uint64_t clks;
uint64_t start;

uint8_t mem[] = {
0xae, 0x00, 0x04,
0x18 ,     
0xd8      ,
0xa2, 0x00   ,
0x86 , 0x00   ,
0x86 , 0x01   ,
0x86 , 0x06   ,
0x86 , 0x07   ,
0x86 , 0x08   ,
0xa2 , 0x01   ,
0x86 , 0x02   ,
0xa2 , 0x00   ,
0x86 , 0x03   ,
0xa2 , 0x03   ,
0x86 , 0x04   ,
0xa2 , 0x05   ,
0x86 , 0x05   ,
0x18      ,
0xa5 , 0x00   ,
0x65 , 0x02   ,
0x85 , 0x00   ,
0xa5 , 0x01   ,
0x65 , 0x03   ,
0x85 , 0x01   ,
0xa0 , 0x00   ,
0xa6 , 0x04   ,
0xca      ,
0xd0 , 0x04   ,
0xa0 , 0x01   ,
0xa2 , 0x03   ,
0x86 , 0x04   ,
0xa6 , 0x05   ,
0xca      ,
0xd0 , 0x04   ,
0xa0 , 0x01   ,
0xa2 , 0x05   ,
0x86 , 0x05   ,
0x18      ,
0x88      ,
0xd0 , 0x15   ,
0x18      ,
0xa5 , 0x08   ,
0x65 , 0x00   ,
0x85 , 0x08   ,
0xa5 , 0x07   ,
0x65 , 0x01   ,
0x85 , 0x07   ,
0xa5 , 0x06   ,
0x65 , 0x03   ,
0xb0 , 0x11   ,
0x85 , 0x06   ,
0xa5 , 0x00   ,
0xc9 , 0xe7   ,
0xd0 , 0xbc   ,
0xa5 , 0x01   ,
0xc9 , 0x03   ,
0xd0 , 0xb6   ,
0x4c , 0x73, 0x06,
0xa2 , 0x00   ,
0x86 , 0x06   ,
0x86 , 0x07   ,
0x86 , 0x08   ,
0xa0 , 0xaa   ,
0xae, 0x01, 0x04,
0x4c, 0x76, 0x06 
};



uint32_t sample_address()
{
    uint32_t raw_data;
    uint32_t gpio_vals;
    //gpio_set_dir_in_masked(GPIO_MASK);
    gpio_put(BT_U7_OE, 1);
   
    // select the correct bus tranceiver
    gpio_put(BT_U5_OE, 1);
    gpio_put(BT_U6_OE, 0);

    // stabilize and sample GPIOs
    TIME_DELTA_SMALL;
    gpio_vals = gpio_get_all();
       
    // select the correct bus tranceiver
    gpio_put(BT_U5_OE, 0);
    gpio_put(BT_U6_OE, 1);
    raw_data = ((gpio_vals & 0xff) << 8 );
    // stabilize and sample GPIOs
    // only two nops because we also do some calculations after the last gpio_put
    asm volatile("nop \n");
    gpio_vals = gpio_get_all();
    raw_data = raw_data | ((gpio_vals & 0xff) );

    
    return raw_data;
}

uint32_t read_from_proc(uint32_t address)
{
  
    gpio_put(BT_U5_OE, 1);
    //gpio_put(BT_U6_OE, 1); // already done in sample_address
    gpio_put(BT_U7_OE, 0);

    // stabilize and sample GPIOs
    TIME_DELTA_SMALL;
    
    uint32_t gpio_vals = gpio_get_all() & 0xff;

    // NOTE no range check (all addressing are backed by RAM!)
    ram[address] = gpio_vals;
    


    return gpio_vals;
}


#define BASE 0x600

uint8_t read_from_mem(uint32_t address){
    if (address == 0x0400)
        {
            printf("start\n");
            clks = 0;
            start = time_us_64();
            return 0;
        }
        else if (address == 0x0401)
        {
            clks = 74602;


            uint64_t delta = time_us_64() - start;
            float mhz = ((float)clks / (float)delta);
            uint32_t it = ram[0] + (ram[1] << 8);
            uint32_t ans = (ram[6] << 16) + (ram[7] << 8) + ram[8];

            printf("delta us:     %6lld\n", delta);
            printf("clock cycles: %6lld\n",clks);
            printf("mhz:          %3f\n",mhz);
            printf("iterations:   %3d\n", it);
            printf("answer:       %6d\n", ans);    
                                    
            if (ans != 233168) {
                printf("ERROR\n");
            }
            else {
                printf("SUCCESS\n");
                gpio_put(DEF_LED_PIN, 1);

                while(true) {
                    sleep_ms(1000);
                }

            }
            return 0;
        }
    
    return ram[address];
}

uint8_t read_control() {
    return gpio_get(RW_PIN);
}


void set_clock(const int val){
    gpio_put(CLK_PIN, val);
}

void simulate_mem_read(uint32_t data){
    // set data on databus and switch tranceivers with one read
    gpio_set_dir_out_masked(0x7FF);
    gpio_put_masked(0x7FF, data | 0x300);
}

int main() {
    //set_sys_clock_khz(125000, true);
    set_sys_clock_khz(250000, true);

    stdio_init_all();

    printf("\nStart over USB and uart\n");

    // init memory
    for (int i=0; i<sizeof(ram); i++) {
        ram[i] = 0xff;
    }

    // copy program into mem
    for (int i=0; i<sizeof(mem); i++) {
        ram[i + BASE] = mem[i];
    }
 
    // init reset vector
    ram[0xfffc] = BASE & 0xff;
    ram[0xfffd] = (BASE >> 8) & 0xff;


    gpio_init(DEF_LED_PIN);
    gpio_set_dir(DEF_LED_PIN, GPIO_OUT);

    gpio_init(CLK_PIN);
    gpio_set_dir(CLK_PIN, GPIO_OUT);

    gpio_init(RW_PIN);
    gpio_set_dir(RW_PIN, GPIO_IN);

    gpio_init(22);
    gpio_set_dir(22, GPIO_IN);
    gpio_pull_up(22);

    gpio_init(RESET_PIN);
    gpio_set_dir(26, GPIO_OUT);

    gpio_init(BT_U5_OE);
    gpio_init(BT_U6_OE);
    gpio_init(BT_U7_OE);

    gpio_set_dir(BT_U5_OE, GPIO_OUT);
    gpio_set_dir(BT_U6_OE, GPIO_OUT);
    gpio_set_dir(BT_U7_OE, GPIO_OUT);

    gpio_put(BT_U5_OE, 1);
    gpio_put(BT_U6_OE, 1);
    gpio_put(BT_U7_OE, 1);

    gpio_init_mask(GPIO_MASK);
    gpio_set_dir_in_masked(GPIO_MASK);

    gpio_put(RESET_PIN, 0);

    for (int i=0; i<10; i++) {
        TIME_DELTA_LARGE
        set_clock(0);
        TIME_DELTA_LARGE
        set_clock(1);
    }

    TIME_DELTA_SMALL
    gpio_put(RESET_PIN, 1);
    // time before clock fall
    TIME_DELTA_SMALL

    while (true) {
        // CLOCK FALL
        // - when reading: data lines are sampled
        // - new addr will be generated (after tADS)        
        set_clock(0);
        //clks++;

        // forward to when address lines are stable  (min: tADS)

        gpio_set_dir_in_masked(GPIO_MASK);
    
        uint32_t sampled_address = sample_address();
        uint8_t control = read_control();
        uint8_t mem_output;

        //  CLOCK RISE
        // -- doesn't really do anything
        set_clock(1);

        if (control == 1) {
            mem_output = read_from_mem(sampled_address);
            simulate_mem_read(mem_output);
        }
        else {
            mem_output = read_from_proc(sampled_address);
        }


#ifdef DEBUG
        printf("CLK: %6lld A: %04x RW:%c D: %04x\n", 
                clks,  sampled_address, control == 1 ? 'r': 'w', mem_output);
            
#endif
    }
}
