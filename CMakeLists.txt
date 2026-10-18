cmake_minimum_required(VERSION 3.32)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PROJECT_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Werror
)

project(Chip8)

add_subdirectory(src)

if(EMSCRIPTEN)
    add_subdirectory(web)
else()
    include(CTest)
    if(BUILD_TESTING)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        )
        FetchContent_MakeAvailable(googletest)
        add_subdirectory(tests)
    endif()
endif()
