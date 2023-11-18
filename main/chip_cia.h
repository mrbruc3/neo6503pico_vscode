#ifndef CHIP_CIA_H
#define CHIP_CIA_H

void cia_init();

void write_to_cia(uint32_t cia_id, uint32_t rel_address, uint8_t val);

uint8_t read_from_cia(uint32_t cia_id, uint32_t rel_address);


#endif