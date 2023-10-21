add_executable(mark6502
        main.c
        )

# pull in common dependencies
target_link_libraries(mark6502 pico_stdlib pico_stdio_usb)



# create map/bin/hex file etc.
pico_add_extra_outputs(mark6502)

# add url via pico_set_program_url
#example_auto_set_url(blink)
