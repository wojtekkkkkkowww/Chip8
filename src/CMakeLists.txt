add_library(Chip8 STATIC
    Chip8.cc
    MonoScreen.cc
    Instructions.cc
    DrawSprite.cc
    AssemblyCommand.cc
    Decoder.cc
)
target_include_directories(Chip8 PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_compile_features(Chip8 PUBLIC cxx_std_20)
target_compile_options(Chip8 PRIVATE ${PROJECT_FLAGS})

add_executable(runChip8 emulator.cc)
target_compile_options(runChip8 PRIVATE ${PROJECT_FLAGS})
target_link_libraries(runChip8 PRIVATE Chip8)
