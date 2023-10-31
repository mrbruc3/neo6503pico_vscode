/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"


#define UART_ID uart0
#define BAUD_RATE 115200

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 28
#define UART_RX_PIN 29


#define DEF_LED_PIN 23

#define CLK_PIN 21

#define RESET_PIN 26

#define BT_U5_OE 8
#define BT_U6_OE 9
#define BT_U7_OE 10

#define RW_PIN 11


void print_raw_values(uint32_t vals, char * pre, int bits, char * post)
{
    uart_puts(UART_ID, pre);
    
    for (int i=0; i<bits; i++){
        if (i == 8 || i == 16 || i == 24) {
            uart_putc_raw(UART_ID, ' ');
        }
        uart_putc_raw(UART_ID, (vals >> i) & 0x1 ? '1' : '0');
    }
    uart_puts(UART_ID, post);
}

#define GPIO_MASK 0xFF

uint32_t sample_address()
{
    uint32_t raw_data = 0;

    gpio_set_dir_in_masked(GPIO_MASK);

    for (int j=0; j<2; j++){
        // select the correct bus tranceiver
        gpio_put(BT_U5_OE, j == 0 ? 0 : 1);
        gpio_put(BT_U6_OE, j == 1 ? 0 : 1);
        gpio_put(BT_U7_OE, j == 2 ? 0 : 1);

        // stabilize and sample GPIOs
        sleep_ms(1);
        uint32_t gpio_vals = gpio_get_all() & 0xff;
        raw_data = raw_data | (gpio_vals << 8 * j);
    }
    
    return raw_data;
}

uint32_t read_from_proc()
{
    gpio_set_dir_in_masked(GPIO_MASK);

    gpio_put(BT_U5_OE, 1);
    gpio_put(BT_U6_OE, 1);
    gpio_put(BT_U7_OE, 0);

    // stabilize and sample GPIOs
    sleep_ms(1);
    uint32_t gpio_vals = gpio_get_all() & 0xff;

    return gpio_vals;
}

uint8_t read_from_mem(uint32_t address){
    
    //uarts_puts

    printf("R: %x\n", address);

    uint8_t mem[] = {
        0xa2, 0x01, // 0x0600   ldx #$01  
        0x86, 0x00, //          stx $00
    };
    
    uint16_t base = 0x6000;

    switch (address) {
        case 0xfffc:    return base & 0xff;
        case 0xfffd:    return (base >> 8) & 0xff;
    }



    if (address >= base && address < base + sizeof(mem)) {
        printf("from mem\n");
        return mem[address - base];
    }

    return 0xbb;
}

uint8_t read_control() {
    uint8_t control = 0;

    control = control | gpio_get(RW_PIN);

    return control;
}


void set_clock(int val){
    gpio_put(CLK_PIN, val);
    gpio_put(DEF_LED_PIN, val);
}


void simulate_mem_read(uint32_t data){
    gpio_set_dir_out_masked(GPIO_MASK);
    sleep_ms(1);
    gpio_put_masked(GPIO_MASK, data);

    // select the cU7 bus tranceiver
    gpio_put(BT_U5_OE, 1); // 1 = disconnect
    gpio_put(BT_U6_OE, 1);
    gpio_put(BT_U7_OE, 0); // 0 = connect
    sleep_ms(1);
}

int main() {
    stdio_usb_init();

    // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);

    while (!stdio_usb_connected()){
       uart_puts(UART_ID, ".\r\n");     
    }
    uart_puts(UART_ID, "\r\n");     

    printf("Start over USB\n");

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    gpio_init(DEF_LED_PIN);
    gpio_set_dir(DEF_LED_PIN, GPIO_OUT);

    gpio_init(CLK_PIN);
    gpio_set_dir(CLK_PIN, GPIO_OUT);

    gpio_init(RW_PIN);
    gpio_set_dir(RW_PIN, GPIO_IN);

    gpio_init(22);
    gpio_set_dir(22, GPIO_IN);
    gpio_pull_up(22);

    gpio_init(RESET_PIN);
    gpio_set_dir(26, GPIO_OUT);

    gpio_init(BT_U5_OE);
    gpio_init(BT_U6_OE);
    gpio_init(BT_U7_OE);

    gpio_set_dir(BT_U5_OE, GPIO_OUT);
    gpio_set_dir(BT_U6_OE, GPIO_OUT);
    gpio_set_dir(BT_U7_OE, GPIO_OUT);

    gpio_put(BT_U5_OE, 1);
    gpio_put(BT_U6_OE, 1);
    gpio_put(BT_U7_OE, 1);

    gpio_init_mask(GPIO_MASK);
    gpio_set_dir_in_masked(GPIO_MASK);



    gpio_put(RESET_PIN, 0);

    for (int i=0; i<10; i++) {
        sleep_ms(1);
        set_clock(0);
        sleep_ms(1);
        set_clock(1);
    }

    sleep_ms(1);
    gpio_put(RESET_PIN, 1);
    // time before clock fall
    sleep_ms(1);

    uart_puts(UART_ID, "\r\nSTART \r\n");
    while (true) {
        sleep_ms(125);
        
        // CLOCK FALL
        // - when reading: data lines are sampled
        // - new addr will be generated (after tADS)        
        set_clock(0);

        // forward to when address lines are stable  (min: tADS)
        sleep_ms(1);

        uint32_t sampled_address = sample_address();
        uint8_t control = read_control();
        uint8_t mem_output;

        //  CLOCK RISE
        // -- doesn't really do anything
        sleep_ms(125);
        set_clock(1);

        if (control == 1) {
            mem_output = read_from_mem(sampled_address);
            simulate_mem_read(mem_output);
        }
        else {
            mem_output = read_from_proc();
        }


        print_raw_values(sampled_address, "A: ", 16, "");
        print_raw_values(control, " C: ", 1, "");
        print_raw_values(mem_output, " D: ", 8, "\r\n");



    }

 

}
