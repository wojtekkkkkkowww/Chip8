cmake_minimum_required(VERSION 3.32)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(-std=c++20)

if ( DEBUG )
    add_compile_options(-g)
    message("compiling in debug mode")
else() 
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

project(SlayBoy)

## Adding the GoogleTest library to the project
## https://google.github.io/googletest/quickstart-cmake.html

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

add_subdirectory(src)
add_subdirectory(tests)
enable_testing()
