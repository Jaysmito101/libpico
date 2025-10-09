# libpico
A collection of useful single-header stb-like C libraries for various things.

All the individual libraries are in the `include/pico` folder, you can just copy the ones you need into your project.
Building is extremely simple, just add the headers to your include path and in any one C file that includes all the 
headers you wanna use define `PICO_IMPLEMENTATION` before including the headers.

## Libraries:

| Library | File | Description |
|---------|------|-------------|
| picoCanvas | `include/pico/pico_window.h` | A simple cross-platform library to create a window and draw pixels/shapes to it. |