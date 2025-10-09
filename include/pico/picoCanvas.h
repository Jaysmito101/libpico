/*
MIT License

Copyright (c) 2025 Jaysmito Mukherjee (jaysmito101@gmail.com)

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

#ifndef PICO_CANVAS_H
#define PICO_CANVAS_H

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64)
#define PICO_CANVAS_WIN32
#elif defined(__linux__) || defined(__unix__)
#ifdef PICO_CANVAS_PREFER_WAYLAND
#define PICO_CANVAS_WAYLAND
#elif defined(PICO_CANVAS_PREFER_X11)
#define PICO_CANVAS_X11
#else
#define PICO_CANVAS_X11 // default to X11
#endif
#else
#error "Unsupported platform for picoCanvas"
#endif

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE free
#endif

typedef struct picoCanvas_t picoCanvas_t;
typedef picoCanvas_t* picoCanvas;

typedef void (*picoCanvasLoggerCallback)(const char* message, picoCanvas canvas);

picoCanvas picoCanvasCreate(const char* name, int32_t width, int32_t height, picoCanvasLoggerCallback logger);
void picoCanvasDestroy(picoCanvas canvas);
void picoCanvasUpdate(picoCanvas canvas);



#ifdef PICO_IMPLEMENTATION
#define PICO_CANVAS_IMPLEMENTATION
#endif // PICO_IMPLEMENTATION

#ifdef PICO_CANVAS_IMPLEMENTATION

#if defined(PICO_CANVAS_WIN32)

#include <Windows.h>
#include <stdlib.h>

typedef struct{
    HBITMAP bitmap;
    uint32_t* buffer;
}  __picoCanvasGraphicsBuffer_t;
typedef __picoCanvasGraphicsBuffer_t* __picoCanvasGraphicsBuffer;

struct picoCanvas_t {
    HWND windowHandle;
    HINSTANCE moduleHandle;
    bool isOpen;
    int32_t width;
    int32_t height;
    __picoCanvasGraphicsBuffer frontBuffer;
    __picoCanvasGraphicsBuffer backBuffer;
    picoCanvasLoggerCallback logger;
    void* userData;
};

static __picoCanvasGraphicsBuffer __picoCanvasGraphicsBufferCreate(int32_t width, int32_t height, bool useBitmap) {
    __picoCanvasGraphicsBuffer buffer = (__picoCanvasGraphicsBuffer)PICO_MALLOC(sizeof(__picoCanvasGraphicsBuffer_t));
    if (!buffer) return NULL;
    memset(buffer, 0, sizeof(__picoCanvasGraphicsBuffer_t));

    if (useBitmap) {
        HDC hdcScreen = GetDC(NULL);
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        buffer->bitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)(&buffer->buffer), INVALID_HANDLE_VALUE, (DWORD)0);
        ReleaseDC(NULL, hdcScreen);
    } else {
        buffer->bitmap = NULL;
        buffer->buffer = (uint32_t*)PICO_MALLOC(width * height * sizeof(uint32_t));
        if (!buffer->buffer) {
            PICO_FREE(buffer);
            return NULL;
        }
    }
    return buffer;
}

static void __picoCanvasGraphicsBufferDestroy(__picoCanvasGraphicsBuffer buffer) {
    if (buffer->bitmap) {
        DeleteObject(buffer->bitmap);
    } else {
        if (buffer->buffer) PICO_FREE(buffer->buffer);
    }
    PICO_FREE(buffer);
}

static bool __picoCanvasGraphicsBufferRecreate(picoCanvas canvas) {
    if (canvas->frontBuffer) __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer) __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    canvas->frontBuffer = __picoCanvasGraphicsBufferCreate(canvas->width, canvas->height, true);
    canvas->backBuffer = __picoCanvasGraphicsBufferCreate(canvas->width, canvas->height, false);
    return canvas->frontBuffer && canvas->backBuffer;
}

picoCanvas picoCanvasCreate(const char* name, int32_t width, int32_t height, picoCanvasLoggerCallback logger) {
    picoCanvas canvas = (picoCanvas)PICO_MALLOC(sizeof(picoCanvas_t));
    if (!canvas) {
        if (logger) logger("Failed to allocate memory for picoCanvas", canvas);
        return NULL;
    }
    memset(canvas, 0, sizeof(picoCanvas_t));

    canvas->width = width;
    canvas->height = height;
    canvas->isOpen = true;
    canvas->logger = logger;
    canvas->userData = NULL;
    canvas->moduleHandle = GetModuleHandle(NULL);

    WNDCLASSEX wincl = {0};
    wincl.hInstance = canvas->moduleHandle;
    wincl.lpszClassName = "PicoCanvasWindowClass";
    wincl.lpfnWndProc = DefWindowProc;
    wincl.style = CS_DBLCLKS;
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    if (!RegisterClassEx(&wincl)) {
        if (canvas->logger) canvas->logger("Failed to register window class", canvas);
        return NULL;
    }

    canvas->windowHandle = CreateWindowEx(
        0,
        wincl.lpszClassName,
        name ? name : "PicoCanvas",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        HWND_DESKTOP, NULL, canvas->moduleHandle, NULL
    );
    if (!canvas->windowHandle) {
        if (canvas->logger) canvas->logger("Failed to create window", canvas);
        return NULL;
    }

    ShowWindow(canvas->windowHandle, SW_SHOW);

    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
        if (canvas->logger) canvas->logger("Failed to create graphics buffers", canvas);
        return NULL;
    }

    return canvas;
}

void picoCanvasDestroy(picoCanvas canvas) {
    if (canvas->frontBuffer) __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer) __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    PICO_FREE(canvas);
}

void picoCanvasUpdate(picoCanvas canvas) {
    MSG msg = {0};
    while (PeekMessage(&msg, canvas->windowHandle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


#elif defined(PICO_CANVAS_X11)


#elif defined(PICO_CANVAS_WAYLAND)


#else
#error "No backend platform selected for picoCanvas"
#endif // platform selection


#endif // PICO_CANVAS_IMPLEMENTATION


#endif // PICO_CANVAS_H