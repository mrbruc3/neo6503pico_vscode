#ifndef CHIP_VIC_H
#define CHIP_VIC_H

uint8_t read_from_vic(uint32_t rom_id, uint32_t rel_address);

void write_to_vic(uint32_t rom_id, uint32_t rel_address, uint8_t value);


#endif