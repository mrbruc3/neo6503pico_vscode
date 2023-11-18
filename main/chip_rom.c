#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "chip_cia.h"
#include "c64_def.h"

#include "rom.h"

uint8_t read_from_rom(uint32_t rom_id, uint32_t rel_address) {
    switch(rom_id) {
        case 0: 
            return kernal_rom[rel_address];
        default:
            printf("trying to read from rom %s - not implemented\n", rom_id);
            while(true){}
            return 0;
    }
}
