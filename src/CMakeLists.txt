add_library(Chip8 INTERFACE)
target_include_directories(Chip8 INTERFACE
    ${PROJECT_SOURCE_DIR}/include
)
target_compile_options(Chip8 INTERFACE
    ${PROJECT_FLAGS}
)

add_executable(runChip8
   emulator.cc
)

target_sources(runChip8
  PRIVATE
    emulator.cc
)

target_link_libraries(runChip8 
    PRIVATE
        Chip8
)
