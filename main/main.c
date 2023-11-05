#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"

//#define SLOW
//#define DEBUG
//#define COUNT_CLOCK

#define PROGRAM_BASE 0x600

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
0xae, 0x00, 0x05,
0x4c, 0x76, 0x06 
};


void set_clock(const int val){
    gpio_put(CLK_PIN, val);
}

void simulate_mem_read(uint32_t data){
    // set data on databus and switch tranceivers with one read
    
    // 0x300 -> u5 and u6 are set
}

int main() {
    set_sys_clock_khz(250000, true);

    stdio_init_all();

    printf("\nStart over USB and uart\n");

    uint8_t ram[64 * 1024];

    // init memory
    for (int i=0; i<sizeof(ram); i++) {
        ram[i] = 0xff;
    }

    // copy program into mem
    for (int i=0; i<sizeof(mem); i++) {
        ram[i + PROGRAM_BASE] = mem[i];
    }
 
    // init reset vector
    ram[0xfffc] = PROGRAM_BASE & 0xff;
    ram[0xfffd] = (PROGRAM_BASE >> 8) & 0xff;

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

     uint32_t control = 1;

    while (true) {
        // CLOCK FALL
#ifdef COUNT_CLOCK             
        clks++;
#endif        
        uint32_t sampled_address;
        uint32_t gpio_vals;
        
        gpio_set_mask(BT_U7_OE_PIN_MASK | BT_U5_OE_PIN_MASK);
        gpio_clr_mask(BT_U6_OE_PIN_MASK | CLK_PIN_MASK);
        gpio_set_dir_in_masked(GPIO_PINS_MASK);

        // stabilize and sample GPIOs
        // do set clock vs nops
        asm volatile("nop \nnop \nnop \n");
        asm volatile("nop \nnop \n");
        gpio_vals = gpio_get_all();

        // select the correct bus tranceiver
        gpio_set_mask(BT_U7_OE_PIN_MASK | BT_U6_OE_PIN_MASK);
        gpio_clr_mask(BT_U5_OE_PIN_MASK);

        sampled_address = ((gpio_vals & 0xff) << 8 );
        
        // stabilize and sample GPIOs
        // only two nops because we also do some calculations after the last gpio_put
        
        asm volatile("nop \nnop \nnop \nnop \n");

        gpio_vals = gpio_get_all();
        sampled_address = sampled_address | ((gpio_vals & 0xff) );

        control = (gpio_vals & RW_PIN_MASK);

        gpio_set_mask(BT_U6_OE_PIN_MASK | BT_U5_OE_PIN_MASK | CLK_PIN_MASK);
        gpio_clr_mask(BT_U7_OE_PIN_MASK);        
        //  CLOCK RISE

        uint8_t mem_output;

        if (control) {
            //mem_output = read_from_mem(sampled_address);
            if (sampled_address == 0x0400)
            {
                printf("start measurement\n");
                clks = 0;
                start = time_us_64();
            }
            else if (sampled_address == 0x0500)
            {
#ifdef COUNT_CLOCK            
                if (clks != 74602) {
                    printf("Clock count is wrong (%d != 74601)\n", clks);
                    while(true) {sleep_ms(1000);}
                }
#else         
                clks = 74602;
#endif 

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
                }
                while(true) { sleep_ms(1000); }
            }
                
            mem_output =  ram[sampled_address];

            gpio_set_dir_out_masked(GPIO_PINS_MASK);
            gpio_put_masked(0x0FF, mem_output);
        }
        else {
            // stabilize and sample GPIOs
            asm volatile("nop \nnop \nnop \n");
            
            uint32_t gpio_vals = (uint8_t)gpio_get_all();

            // NOTE no range check (all addressing are backed by RAM!)
            ram[sampled_address] = gpio_vals;
            mem_output = gpio_vals;
        }

#ifdef DEBUG
        TIME_DELTA_LARGE
        printf("CLK: %6lld A: %04x RW:%c D: %04x\n", 
                clks,  sampled_address, control == 1 ? 'r': 'w', mem_output);
            
#endif
    }
}
