#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "chip_cia.h"




void write_to_cia(uint32_t cia_id, uint32_t rel_address, uint32_t val) {
    // registers are repeated in data range
    rel_address = rel_address & 0x0f;

    switch (rel_address) {
        case 0x0:
            printf("name %s\n", cia_id);
            break;
        default:
            printf("ignore write to cia %d, register 0x%x with value 0x%02x\n", cia_id, rel_address, val);
            break;
    }
}
