#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "chip_cia.h"
#include "c64_def.h"

struct cia_state {
    uint8_t mem[0xff];
};

struct cia_state cia_state_list[CIA_COUNT];


void cia_init(){
    for(int i=0; i<CIA_COUNT; i++){
        for (int j=0; j<sizeof(struct cia_state); j++) {
            //cia_state_list[i]
            memset(& cia_state_list[i], 0, sizeof(struct cia_state));
        }
    }
}

void write_to_cia(uint32_t cia_id, uint32_t rel_address, uint8_t val) {
    // registers are repeated in data range
    rel_address = rel_address & 0x0f;

    switch (rel_address) {
        default:
            cia_state_list[cia_id].mem[rel_address] = val;
            printf("emulate write to cia %d with ram, register 0x%x with value 0x%02x\n", cia_id, rel_address, val);
            break;
    }
}

uint8_t read_from_cia(uint32_t cia_id, uint32_t rel_address) {
    // registers are repeated in data range
    rel_address = rel_address & 0x0f;
    switch (rel_address) {
        default:
            {
                uint8_t val = cia_state_list[cia_id].mem[rel_address];
                printf("read from cia %d, register 0x%x with value 0x%02x\n", cia_id, rel_address, val);
                return val;
                break;
            }
    }
}
