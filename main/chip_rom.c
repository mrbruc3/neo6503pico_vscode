#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "chip_cia.h"
#include "c64_def.h"

#include "rom.h"

uint8_t rom_ram[ROM_COUNT][0x2000]; // 8 KB

uint8_t read_from_rom(uint32_t rom_id, uint32_t rel_address) {
    switch(rom_id) {
        case 0: 
            return kernal_rom[rel_address];
        case 1: 
            printf("read from basic rom 0x%04x\n", rel_address);
            return basic_rom[rel_address];
        default:
            printf("trying to read from rom %d - not implemented\n", rom_id);
            while(true){}
            return 0;
    }
}

// when basic rom is selected, reads are from rom, writes still go to ram
void write_to_rom(uint32_t rom_id, uint32_t rel_address, uint8_t value) {
    rom_ram[rom_id][rel_address] = value;
}

