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

void write_to_ram(uint32_t ram_id, uint32_t rel_address, uint8_t value) {
    ram_ram[ram_id][rel_address] = value;
}

