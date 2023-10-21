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

int main() {
    stdio_usb_init();

    // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);


    const uint LED_PIN = DEF_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(CLK_PIN);
    gpio_set_dir(CLK_PIN, GPIO_OUT);

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

    uint gpio_mask = 0xff;

    gpio_init_mask(gpio_mask);
    gpio_set_dir_masked(gpio_mask, GPIO_IN);



    gpio_put(RESET_PIN, 0);
    sleep_ms(1000);
    gpio_put(RESET_PIN, 1);


    uart_puts(UART_ID, "\r\n");
    while (true) {
        // time before clock fall
        sleep_ms(1);
        
        gpio_put(CLK_PIN, 0);
        gpio_put(LED_PIN, 0);

        // CLOCK FALL
        // - sample address bus

        uint32_t raw_data = 0;

        for (int j=0; j<3; j++){
            // select the correct bus tranceiver
            gpio_put(BT_U5_OE, j == 0 ? 0 : 1);
            gpio_put(BT_U6_OE, j == 1 ? 0 : 1);
            gpio_put(BT_U7_OE, j == 2 ? 0 : 1);

            // stabilize and sample GPIOs
            sleep_ms(1);
            uint32_t gpio_vals = gpio_get_all();

/*
            uart_putc_raw(UART_ID, j == 0 ? '0' : '1');
            uart_putc_raw(UART_ID, j == 1 ? '0' : '1');
            uart_putc_raw(UART_ID, j == 2 ? '0' : '1');
            uart_putc_raw(UART_ID, ' ');
            uart_putc_raw(UART_ID, '0' + j);
            uart_putc_raw(UART_ID, ' ');
*/
            for (int i=0; i<8; i++){
                if (i == 8 || i == 16 || i == 24) {
                    uart_putc_raw(UART_ID, ' ');
                }
                uart_putc_raw(UART_ID, (gpio_vals >> i) & 0x1 ? '1' : '0');
            }
            uart_putc_raw(UART_ID, ' ');
            
        }
        uart_puts(UART_ID, "\r\n");



        sleep_ms(125);



        gpio_set_dir_masked(gpio_mask, GPIO_OUT);
        sleep_ms(1);

        // not something in particular
        gpio_put_masked(gpio_mask, 0xFF);
        
        sleep_ms(1);

                sleep_ms(125);



        gpio_put(CLK_PIN, 1);
        gpio_put(LED_PIN, 1);

        sleep_ms(125);
        gpio_set_dir_masked(gpio_mask, GPIO_IN);




    }

 

}
