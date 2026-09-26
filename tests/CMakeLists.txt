add_executable(chip8_tests
    screen_test.cc
    decoder_test.cc
    basic_instructions_test.cc
    arithmetic_instructions_test.cc
    skip_instructions_test.cc
    memory_instructions_test.cc
    draw_sprite_test.cc
)
target_compile_options(chip8_tests PRIVATE ${PROJECT_FLAGS})
target_link_libraries(chip8_tests PRIVATE GTest::gtest_main Chip8)

include(GoogleTest)
gtest_discover_tests(chip8_tests)
