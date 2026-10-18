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

if(EMSCRIPTEN)
    target_compile_options(Chip8 PUBLIC -fexceptions)
    target_link_options(Chip8 INTERFACE -fexceptions)
endif()
