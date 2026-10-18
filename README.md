# CHIP-8

A CHIP-8 emulator written in C++.

## Compatibility target

The original CHIP-8 interpreter for the COSMAC VIP is the baseline model.

## Native library and tests

```sh
cmake -S . -B build-native
cmake --build build-native
ctest --test-dir build-native --output-on-failure
```

Use `-DBUILD_TESTING=OFF` to build only the native library, without GoogleTest.
There is no native application entry point.

## WebAssembly game

With the Emscripten SDK activated:

```sh
emcmake cmake -S . -B build-web
cmake --build build-web
python3 -m http.server 8000 --bind 127.0.0.1 --directory build-web/web
```

Open http://localhost:8000 on the machine running the server. To use another
machine, copy the complete contents of `build-web/web` and serve that directory
there.
