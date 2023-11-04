##
## Work in progress solution
##

add_executable(mark6502 main.c)


# pull in common dependencies
target_link_libraries(mark6502 pico_stdlib )

# create map/bin/hex file etc.
pico_add_extra_outputs(mark6502)

pico_enable_stdio_usb(mark6502 1)
pico_enable_stdio_uart(mark6502 1)


target_compile_definitions(mark6502 PRIVATE
        PICO_DEFAULT_UART=0
        PICO_DEFAULT_UART_TX_PIN=28
        PICO_DEFAULT_UART_RX_PIN=29
        PICO_DEFAULT_UART_BAUD_RATE=115200
) 


##
## Reference solution
##

add_executable(reference reference.c)

target_link_libraries(reference pico_stdlib )

pico_add_extra_outputs(reference)

pico_enable_stdio_usb(reference 1)
pico_enable_stdio_uart(reference 1)

target_compile_definitions(reference PRIVATE
        PICO_DEFAULT_UART=0
        PICO_DEFAULT_UART_TX_PIN=28
        PICO_DEFAULT_UART_RX_PIN=29
        PICO_DEFAULT_UART_BAUD_RATE=115200
) 



