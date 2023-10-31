add_executable(mark6502
        main.c
        )



# pull in common dependencies
target_link_libraries(mark6502 pico_stdlib pico_stdio_usb pico_stdio_uart)

# create map/bin/hex file etc.
pico_add_extra_outputs(mark6502)


