/*
MIT License

Copyright (c) 2026 Jaysmito Mukherjee (jaysmito101@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * This has been purely implemented based of the ITU-T H.264 (V15) (08/2024) specification.
 * - Jaysmito Mukherjee (jaysmito101@gmail.com)
 */

// NOTE: It is important to keep in mind that this is NOT A DECODER. This library is only meant to parse H.264 bitstreams
//       and extract NAL units, slices, headers and other metadata from them. Actual decoding of video frames is outside the scope of this library.
//       This library can be used as a pre-processing step before feeding the data to an actual H.264 decoder.
//       The primary goal of this library is to provide a simple and lightweight way to parse H.264 streams to be used
//       together with decoder applications like Vulkan Video, DirectX Video, etc.

#ifndef PICO_H264_H
#define PICO_H264_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

#ifndef PICO_REALLOC
#define PICO_REALLOC realloc
#endif

#ifndef PICO_ASSERT
#include <assert.h>
#define PICO_ASSERT(expr) assert(expr)
#endif

#ifndef PICO_H264_LOG
#define PICO_H264_LOG(...) \
    do {                   \
        (void)0;           \
    } while (0)
#endif

#endif // PICO_H264_H