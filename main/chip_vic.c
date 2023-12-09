#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "chip_vic.h"
#include "c64_def.h"

uint8_t vic_ram[0x40]; // 0x40 B



#define VIC2_RASTER_LINE_REG 0x12

uint8_t read_from_vic(uint32_t vic_id, uint32_t rel_address) {
    switch(vic_id) {
        case 0x12:
            printf("read from raster line reg\n");
            return 0;

        default:
            printf("trying to read from vic %d (0x%04x) - not implemented\n", vic_id, rel_address);
            while(true){}
            return 0;
    }
}

void write_to_vic(uint32_t vic_id, uint32_t rel_address, uint8_t value) {

    // memory range loops 
    rel_address = rel_address & 0x3f;

    switch(rel_address) {
        case 0x00: printf(" sprite $0 x coord "); break;
        case 0x11: printf(" screen control register #1 "); break;
        case 0x12: printf(" raster line register "); break;
        case 0x2e: printf(" sprint 7 color "); break;
    }

    printf("... vic 2 write ...\n");

    vic_ram[rel_address] = value;
}

