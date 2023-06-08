add_executable(freewire)

pico_generate_pio_header(freewire ${CMAKE_CURRENT_LIST_DIR}/freewire_write.pio)
pico_generate_pio_header(freewire ${CMAKE_CURRENT_LIST_DIR}/freewire_read.pio)

target_sources(freewire PRIVATE freewire.c freewire_word.c)

target_link_libraries(freewire PRIVATE pico_stdlib hardware_pio pico_multicore)
pico_add_extra_outputs(freewire)

# add url via pico_set_program_url
example_auto_set_url(freewire)
