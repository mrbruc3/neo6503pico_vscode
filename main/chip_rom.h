#ifndef CHIP_ROM_H
#define CHIP_ROM_H

uint8_t read_from_rom(uint32_t rom_id, uint32_t rel_address);

void write_to_rom(uint32_t rom_id, uint32_t rel_address, uint8_t value);


#endif