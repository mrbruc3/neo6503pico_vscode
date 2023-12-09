#ifndef CHIP_RAM_H
#define CHIP_RAM_H

uint8_t read_from_ram(uint32_t ram_id, uint32_t rel_address);

void write_to_ram(uint32_t ram_id, uint32_t rel_address, uint8_t value);


#endif