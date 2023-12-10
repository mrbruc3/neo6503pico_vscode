#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "c64_def.h"

#include "chip_ram.h"

uint8_t ram_ram[RAM_COUNT][0x2000]; // 8 KB

uint8_t read_from_ram(uint32_t ram_id, uint32_t rel_address) {
    return ram_ram[ram_id][rel_address];
}


uint8_t screen_code_map[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ..... !........*.....01234567890.......................................................................abcdefghijklmnopqrstuvwxyz......................................................................................................";


bool start_output = false;

void print_screen_code(uint8_t value) {
    //printf("%c");
}



// 64K RAM SYSTEM  38911 BASIC BYTES FREE 
//                                      RE


void write_to_ram(uint32_t ram_id, uint32_t rel_address, uint8_t value) {

    uint32_t screen_ram_start = 0x400 - 2;

    if (ram_id == 0 && rel_address >= screen_ram_start && rel_address <= screen_ram_start + 1000) {

    
        if (value == 42) {
            start_output = true;
        }

        if (start_output) {
            printf("write to screen ram %d = %d, %c\n", rel_address - screen_ram_start, value, screen_code_map[value]);
            printf("----------------------------------------\n");

            for (int y=0; y<25; y++) {
                for (int x=0; x<40; x++) {
                    printf("%c", screen_code_map[ram_ram[0][screen_ram_start + y*40 + x]]);
                    //printf("%d ",  1024 + y*40 + x);
                }
                printf("\n");
            }
            printf("----------------------------------------\n");
        }
    }

    ram_ram[ram_id][rel_address] = value;
}

