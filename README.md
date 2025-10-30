# libpico
A collection of useful single-header stb-like C libraries for various things.

All the individual libraries are in the `include/pico` folder, you can just copy the ones you need into your project.
Building is extremely simple, just add the headers to your include path and in any one C file that includes all the 
headers you wanna use define `PICO_IMPLEMENTATION` before including the headers.

## Libraries:

| Library | File | Description | Example |
|---------|------|-------------|---------|
| picoCanvas | `include/pico/picoCanvas.h` | A simple cross-platform library to create a window and draw pixels to it. | `examples/picoCanvas` |
| picoLog | `include/pico/picoLog.h` | A simple logging library. | `examples/picoLog` |
| picoThreads | `include/pico/picoThreads.h` | A simple cross-platform threading + thread pool library. | `examples/picoThreads` |
| picoStream | `include/pico/picoStream.h` | A simple file and memory stream library. | `examples/picoStream` |
| picoPerf | `include/pico/picoPerf.h` | A simple performance measurement library. | `examples/picoPerf` |
| picoM3U8 | `include/pico/picoM3u8.h` | A library for parsing M3U8 playlists (HLS). (Full spec implementation) | `examples/picoM3U8` |