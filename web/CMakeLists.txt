add_executable(chip8_web bindings.cpp)
target_link_libraries(chip8_web PRIVATE Chip8)
target_compile_options(chip8_web PRIVATE ${PROJECT_FLAGS})
target_link_options(chip8_web PRIVATE
    --no-entry
    -lembind
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sALLOW_MEMORY_GROWTH=1
)
set_target_properties(chip8_web PROPERTIES
    OUTPUT_NAME chip8
    SUFFIX ".js"
)

# Keep a complete, ready-to-serve page next to the generated module.
foreach(asset index.html main.js games.js style.css)
    configure_file(${asset} ${asset} COPYONLY)
endforeach()
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
