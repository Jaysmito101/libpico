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

#include <stdbool.h>
#include <stdint.h>

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
#define PICO_FREE   free
#endif

typedef struct picoCanvas_t picoCanvas_t;
typedef picoCanvas_t *picoCanvas;

typedef uint32_t picoCanvasColor;

typedef void (*picoCanvasLoggerCallback)(const char *message, picoCanvas canvas);
typedef void (*picoCanvasResizeCallback)(int32_t width, int32_t height, picoCanvas canvas);

picoCanvas picoCanvasCreate(const char *name, int32_t width, int32_t height, picoCanvasLoggerCallback logger);
void picoCanvasDestroy(picoCanvas canvas);
void picoCanvasUpdate(picoCanvas canvas);
void picoCanvasSwapBuffers(picoCanvas canvas);
void picoCanvasSetUserData(picoCanvas canvas, void *userData);
void *picoCanvasGetUserData(picoCanvas canvas);
void picoCanvasSetResizeCallback(picoCanvas canvas, picoCanvasResizeCallback callback);
bool picoCanvasIsOpen(picoCanvas canvas);
void picoCanvasSetTitle(picoCanvas canvas, const char *title);
void picoCanvasSetSize(picoCanvas canvas, int32_t width, int32_t height);
void picoCanvasClear(picoCanvas canvas, picoCanvasColor color);
void picoCanvasDrawPixel(picoCanvas canvas, int32_t x, int32_t y, picoCanvasColor color);

picoCanvasColor picoCanvasRgba2Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void picoCanvasColor2Rgba(picoCanvasColor color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

#ifdef PICO_IMPLEMENTATION
#define PICO_CANVAS_IMPLEMENTATION
#endif // PICO_IMPLEMENTATION

#ifdef PICO_CANVAS_IMPLEMENTATION

// common implementation

picoCanvasColor picoCanvasRgba2Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((picoCanvasColor)r << 24) | ((picoCanvasColor)g << 16) | ((picoCanvasColor)b << 8) | (picoCanvasColor)a;
}

void picoCanvasColor2Rgba(picoCanvasColor color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
    if (r)
        *r = (color >> 24) & 0xFF;
    if (g)
        *g = (color >> 16) & 0xFF;
    if (b)
        *b = (color >> 8) & 0xFF;
    if (a)
        *a = color & 0xFF;
}

// platform specific implementation

#if defined(PICO_CANVAS_WIN32)

#include <Windows.h>
#include <stdlib.h>

typedef struct {
    HBITMAP bitmap;
    uint32_t *buffer;
} __picoCanvasGraphicsBuffer_t;
typedef __picoCanvasGraphicsBuffer_t *__picoCanvasGraphicsBuffer;

struct picoCanvas_t {
    HWND windowHandle;
    HINSTANCE moduleHandle;
    bool isOpen;
    int32_t width;
    int32_t height;
    __picoCanvasGraphicsBuffer frontBuffer;
    __picoCanvasGraphicsBuffer backBuffer;
    picoCanvasLoggerCallback logger;
    picoCanvasResizeCallback resizeCallback;
    void *userData;
};

static __picoCanvasGraphicsBuffer __picoCanvasGraphicsBufferCreate(int32_t width, int32_t height, bool useBitmap)
{
    __picoCanvasGraphicsBuffer buffer = (__picoCanvasGraphicsBuffer)PICO_MALLOC(sizeof(__picoCanvasGraphicsBuffer_t));
    if (!buffer)
        return NULL;
    memset(buffer, 0, sizeof(__picoCanvasGraphicsBuffer_t));

    if (useBitmap) {
        HDC hdcScreen               = GetDC(NULL);
        BITMAPINFO bmi              = {0};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = width;
        bmi.bmiHeader.biHeight      = -height; // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        buffer->bitmap              = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void **)(&buffer->buffer), NULL, (DWORD)0);
        ReleaseDC(NULL, hdcScreen);
    } else {
        buffer->bitmap = NULL;
        buffer->buffer = (uint32_t *)PICO_MALLOC(width * height * sizeof(uint32_t));
        if (!buffer->buffer) {
            PICO_FREE(buffer);
            return NULL;
        }
    }
    return buffer;
}

static void __picoCanvasGraphicsBufferDestroy(__picoCanvasGraphicsBuffer buffer)
{
    if (buffer->bitmap) {
        DeleteObject(buffer->bitmap);
    } else {
        if (buffer->buffer)
            PICO_FREE(buffer->buffer);
    }
    PICO_FREE(buffer);
}

static bool __picoCanvasGraphicsBufferRecreate(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    canvas->frontBuffer = __picoCanvasGraphicsBufferCreate(canvas->width, canvas->height, true);
    canvas->backBuffer  = __picoCanvasGraphicsBufferCreate(canvas->width, canvas->height, false);
    return canvas->frontBuffer && canvas->backBuffer;
}

LRESULT CALLBACK __picoCanvasWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    picoCanvas canvas = (picoCanvas)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!canvas)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc    = BeginPaint(hwnd, &ps);
            HDC hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, canvas->frontBuffer->bitmap);
            BitBlt(hdc, 0, 0, canvas->width, canvas->height, hdcMem, 0, 0, SRCCOPY);
            DeleteDC(hdcMem);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SIZE: {
            int32_t newWidth  = LOWORD(lParam);
            int32_t newHeight = HIWORD(lParam);
            if (newWidth != canvas->width || newHeight != canvas->height) {
                canvas->width  = newWidth;
                canvas->height = newHeight;
                if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
                    if (canvas->logger)
                        canvas->logger("Failed to recreate graphics buffers on resize", canvas);
                }
                if (canvas->resizeCallback)
                    canvas->resizeCallback(newWidth, newHeight, canvas);
            }
            break;
        }
        case WM_DESTROY: {
            canvas->isOpen = false;
            break;
        }
        case WM_CLOSE: {
            canvas->isOpen = false;
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

picoCanvas picoCanvasCreate(const char *name, int32_t width, int32_t height, picoCanvasLoggerCallback logger)
{
    picoCanvas canvas = (picoCanvas)PICO_MALLOC(sizeof(picoCanvas_t));
    if (!canvas) {
        if (logger)
            logger("Failed to allocate memory for picoCanvas", canvas);
        return NULL;
    }
    memset(canvas, 0, sizeof(picoCanvas_t));

    canvas->width        = width;
    canvas->height       = height;
    canvas->isOpen       = true;
    canvas->logger       = logger;
    canvas->userData     = NULL;
    canvas->moduleHandle = GetModuleHandle(NULL);

    WNDCLASSEX wincl    = {0};
    wincl.hInstance     = canvas->moduleHandle;
    wincl.lpszClassName = "PicoCanvasWindowClass";
    wincl.lpfnWndProc   = __picoCanvasWindowProc;
    wincl.style         = CS_DBLCLKS;
    wincl.cbSize        = sizeof(WNDCLASSEX);
    wincl.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    if (!RegisterClassEx(&wincl)) {
        if (canvas->logger)
            canvas->logger("Failed to register window class", canvas);
        return NULL;
    }

    canvas->windowHandle = CreateWindowEx(
        0,
        wincl.lpszClassName,
        name ? name : "PicoCanvas",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        HWND_DESKTOP, NULL, canvas->moduleHandle, NULL);
    if (!canvas->windowHandle) {
        if (canvas->logger)
            canvas->logger("Failed to create window", canvas);
        return NULL;
    }

    ShowWindow(canvas->windowHandle, SW_SHOW);
    SetWindowLongPtr(canvas->windowHandle, GWLP_USERDATA, (LONG_PTR)canvas);

    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
        if (canvas->logger)
            canvas->logger("Failed to create graphics buffers", canvas);
        return NULL;
    }


    return canvas;
}

void picoCanvasDestroy(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    PICO_FREE(canvas);
}

void picoCanvasUpdate(picoCanvas canvas)
{
    MSG msg = {0};
    while (PeekMessage(&msg, canvas->windowHandle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void picoCanvasSwapBuffers(picoCanvas canvas)
{
    memcpy(canvas->frontBuffer->buffer, canvas->backBuffer->buffer, canvas->width * canvas->height * sizeof(uint32_t));
    InvalidateRect(canvas->windowHandle, NULL, FALSE);
}

void picoCanvasSetUserData(picoCanvas canvas, void *userData)
{
    canvas->userData = userData;
}

void *picoCanvasGetUserData(picoCanvas canvas)
{
    return canvas->userData;
}

void picoCanvasSetResizeCallback(picoCanvas canvas, picoCanvasResizeCallback callback)
{
    canvas->resizeCallback = callback;
}

bool picoCanvasIsOpen(picoCanvas canvas)
{
    return canvas->isOpen;
}

void picoCanvasSetTitle(picoCanvas canvas, const char *title)
{
    SetWindowText(canvas->windowHandle, title);
}

void picoCanvasSetSize(picoCanvas canvas, int32_t width, int32_t height)
{
    SetWindowPos(canvas->windowHandle, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void picoCanvasClear(picoCanvas canvas, picoCanvasColor color)
{
    for (int i = 0; i < canvas->width * canvas->height; ++i)
        canvas->backBuffer->buffer[i] = color;
}

void picoCanvasDrawPixel(picoCanvas canvas, int32_t x, int32_t y, picoCanvasColor color)
{
    if (x < 0 || x >= canvas->width || y < 0 || y >= canvas->height)
        return;
    canvas->backBuffer->buffer[y * canvas->width + x] = color;
}

#elif defined(PICO_CANVAS_X11)

#elif defined(PICO_CANVAS_WAYLAND)

#else
#error "No backend platform selected for picoCanvas"
#endif // platform selection

#endif // PICO_CANVAS_IMPLEMENTATION

#endif // PICO_CANVAS_H