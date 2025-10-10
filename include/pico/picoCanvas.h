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

/**
 * @file picoCanvas.h
 * @brief A lightweight, single-header library for creating cross-platform pixel canvas windows
 *
 * picoCanvas provides a simple API for creating windows with direct pixel buffer access,
 * making it ideal for software rendering, pixel art, game development, and visualization.
 *
 * Usage:
 * @code
 * #define PICO_CANVAS_IMPLEMENTATION
 * #include "picoCanvas.h"
 *
 * int main() {
 *     picoCanvas canvas = picoCanvasCreate("My Window", 800, 600, NULL);
 *     while (picoCanvasIsOpen(canvas)) {
 *         picoCanvasUpdate(canvas);
 *         picoCanvasClear(canvas, picoCanvasRgba2Color(0, 0, 0, 255));
 *         // Draw your pixels here
 *         picoCanvasSwapBuffers(canvas);
 *     }
 *     picoCanvasDestroy(canvas);
 *     return 0;
 * }
 * @endcode
 *
 * @author Jaysmito Mukherjee
 * @date 2025
 */

#ifndef PICO_CANVAS_H
#define PICO_CANVAS_H

#include <stdbool.h>
#include <stdint.h>

// Platform Detection

/**
 * @def PICO_CANVAS_WIN32
 * @brief Defined when targeting Windows platform
 *
 * This macro is automatically defined on Windows (_WIN32 or _WIN64) systems.
 * It enables the Win32 API backend for window creation and rendering.
 */

/**
 * @def PICO_CANVAS_X11
 * @brief Defined when targeting Linux X11 platform
 *
 * This macro is automatically defined on Linux/Unix systems when X11 is selected
 * as the windowing backend. X11 is the default on Linux unless overridden.
 */

/**
 * @def PICO_CANVAS_WAYLAND
 * @brief Defined when targeting Linux Wayland platform
 *
 * This macro is defined on Linux/Unix systems when Wayland is explicitly selected
 * by defining PICO_CANVAS_PREFER_WAYLAND before including this header.
 */

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

// Type Definitions

/**
 * @typedef picoCanvas_t
 * @brief Opaque structure representing a canvas window
 *
 * This structure contains all the internal state for a canvas window.
 * Users should only interact with it through the provided API functions.
 */
typedef struct picoCanvas_t picoCanvas_t;

/**
 * @typedef picoCanvas
 * @brief Handle to a canvas window
 *
 * This is a pointer to the opaque picoCanvas_t structure. All API functions
 * take this handle as their first parameter.
 */
typedef picoCanvas_t *picoCanvas;

/**
 * @typedef picoCanvasColor
 * @brief Represents a 32-bit RGBA color value
 *
 * Color is packed as RGBA with 8 bits per channel:
 * - Bits 24-31: Red channel
 * - Bits 16-23: Green channel
 * - Bits 8-15: Blue channel
 * - Bits 0-7: Alpha channel
 *
 * Use picoCanvasRgba2Color() to create colors and picoCanvasColor2Rgba() to extract components.
 */
typedef uint32_t picoCanvasColor;

// ============================================
// Callback Types
// ============================================

/**
 * @typedef picoCanvasLoggerCallback
 * @brief Callback function type for logging messages from picoCanvas
 *
 * @param message The log message string (null-terminated)
 * @param canvas The canvas instance that generated the log message
 *
 * @note The message pointer is only valid during the callback invocation.
 *       Copy the string if you need to store it.
 */
typedef void (*picoCanvasLoggerCallback)(const char *message, picoCanvas canvas);

/**
 * @typedef picoCanvasResizeCallback
 * @brief Callback function type for handling canvas resize events
 *
 * This callback is invoked whenever the canvas window is resized, either by
 * the user dragging the window borders or programmatically via picoCanvasSetSize().
 *
 * @param width The new width of the canvas in pixels
 * @param height The new height of the canvas in pixels
 * @param canvas The canvas instance that was resized
 *
 * @note The buffers are automatically recreated before this callback is invoked.
 */
typedef void (*picoCanvasResizeCallback)(int32_t width, int32_t height, picoCanvas canvas);

// Canvas Lifecycle Functions

/**
 * @brief Creates a new picoCanvas window with the specified dimensions
 *
 * This function allocates and initializes a new canvas window with double-buffered
 * rendering. The window is immediately shown on screen.
 *
 * @param name The title of the window (can be NULL for default title "PicoCanvas")
 * @param width The initial width of the canvas in pixels (must be > 0)
 * @param height The initial height of the canvas in pixels (must be > 0)
 * @param logger Optional callback function for logging messages (can be NULL)
 *
 * @return A new picoCanvas handle, or NULL if creation failed
 *
 * @note The caller is responsible for destroying the canvas with picoCanvasDestroy()
 *       when done to avoid memory leaks.
 *
 * @see picoCanvasDestroy()
 */
picoCanvas picoCanvasCreate(const char *name, int32_t width, int32_t height, picoCanvasLoggerCallback logger);

/**
 * @brief Destroys a picoCanvas instance and frees all associated resources
 *
 * This function closes the window, destroys all graphics buffers, and frees
 * all memory associated with the canvas.
 *
 * @param canvas The canvas instance to destroy
 *
 * @note After calling this function, the canvas handle is invalid and must not be used.
 * @warning Do not call this function twice on the same canvas.
 *
 * @see picoCanvasCreate()
 */
void picoCanvasDestroy(picoCanvas canvas);

// Canvas Update and Rendering Functions

/**
 * @brief Processes window events and updates the canvas state
 *
 * This function pumps and processes all pending window messages.
 *
 * @param canvas The canvas instance to update
 *
 * @note This function should be called regularly (typically once per frame in your
 *       main loop) to keep the window responsive. If not called frequently enough,
 *       the window may appear frozen to the operating system.
 *
 * @warning Always check picoCanvasIsOpen() after calling this function, as the
 *          user may have closed the window.
 *
 * @see picoCanvasIsOpen()
 */
void picoCanvasUpdate(picoCanvas canvas);

/**
 * @brief Swaps the front and back buffers, displaying the rendered content
 *
 * This function copies the contents of the back buffer (where you draw) to the
 * front buffer (what's displayed on screen) and triggers a window repaint.
 *
 * @param canvas The canvas instance to swap buffers for
 *
 * @note This implements double-buffering to prevent tearing and flickering.
 *       Always draw to the back buffer and call this function when you're done
 *       drawing a frame.
 *
 * @note The back buffer contents are preserved after swapping, so you can
 *       perform incremental updates without clearing.
 *
 * @see picoCanvasClear(), picoCanvasDrawPixel()
 */
void picoCanvasSwapBuffers(picoCanvas canvas);

// ============================================
// Canvas Property Getters and Setters
// ============================================

/**
 * @brief Sets user-defined data associated with the canvas
 *
 * This function allows you to attach arbitrary data to a canvas instance,
 * which can be useful for storing application state that needs to be accessed
 * in callbacks or other parts of your application.
 *
 * @param canvas The canvas instance
 * @param userData Pointer to user-defined data (can be any type, including NULL)
 *
 * @note The library does not manage this memory. You are responsible for
 *       allocating and freeing any memory pointed to by userData.
 *
 * @see picoCanvasGetUserData()
 */
void picoCanvasSetUserData(picoCanvas canvas, void *userData);

/**
 * @brief Retrieves the user-defined data associated with the canvas
 *
 * Returns the pointer that was previously set with picoCanvasSetUserData().
 *
 * @param canvas The canvas instance
 *
 * @return Pointer to the user-defined data, or NULL if none was set
 *
 * @see picoCanvasSetUserData()
 */
void *picoCanvasGetUserData(picoCanvas canvas);

/**
 * @brief Sets a callback function to be called when the canvas is resized
 *
 * The callback will be invoked whenever the window is resized, either by the
 * user or programmatically. The graphics buffers are automatically recreated
 * before the callback is invoked.
 *
 * @param canvas The canvas instance
 * @param callback The callback function to be called on resize events, or NULL to remove
 *
 * @note Only one resize callback can be set at a time. Setting a new callback
 *       replaces the previous one.
 *
 * @see picoCanvasResizeCallback, picoCanvasSetSize()
 */
void picoCanvasSetResizeCallback(picoCanvas canvas, picoCanvasResizeCallback callback);

/**
 * @brief Checks if the canvas window is still open
 *
 * Returns false if the user has closed the window or if the window was destroyed.
 *
 * @param canvas The canvas instance to check
 *
 * @return true if the window is open and active, false otherwise
 *
 * @note This is typically used as the condition in your main loop:
 * @code
 * while (picoCanvasIsOpen(canvas)) {
 *     // Update and render
 * }
 * @endcode
 *
 * @see picoCanvasUpdate()
 */
bool picoCanvasIsOpen(picoCanvas canvas);

/**
 * @brief Sets the title of the canvas window
 *
 * Changes the text displayed in the window's title bar.
 *
 * @param canvas The canvas instance
 * @param title The new title string (must be null-terminated)
 *
 * @note The string is copied internally, so you don't need to keep the pointer valid.
 */
void picoCanvasSetTitle(picoCanvas canvas, const char *title);

/**
 * @brief Resizes the canvas window to the specified dimensions
 *
 * This function requests the operating system to resize the window. The actual
 * resize will trigger a WM_SIZE(on windows for example) event which recreates
 * the buffers and invokes the resize callback if one is set.
 *
 * @param canvas The canvas instance
 * @param width The new width in pixels (must be > 0)
 * @param height The new height in pixels (must be > 0)
 *
 * @note The resize may not happen immediately. The resize callback will be
 *       invoked once the resize is complete.
 *
 * @see picoCanvasSetResizeCallback()
 */
void picoCanvasSetSize(picoCanvas canvas, int32_t width, int32_t height);

// Drawing Functions

/**
 * @brief Clears the entire back buffer to a solid color
 *
 * Fills every pixel in the back buffer with the specified color. This is typically
 * called at the beginning of each frame before drawing.
 *
 * @param canvas The canvas instance
 * @param color The color to fill the canvas with
 *
 * @note This function modifies the back buffer only. Call picoCanvasSwapBuffers()
 *       to display the cleared canvas on screen.
 *
 * @see picoCanvasSwapBuffers(), picoCanvasRgba2Color()
 */
void picoCanvasClear(picoCanvas canvas, picoCanvasColor color);

/**
 * @brief Draws a single pixel at the specified coordinates
 *
 * Sets the color of a single pixel in the back buffer. The coordinate system
 * has (0,0) at the top-left corner, with X increasing to the right and Y
 * increasing downward.
 *
 * @param canvas The canvas instance
 * @param x The x-coordinate of the pixel (0 is left edge)
 * @param y The y-coordinate of the pixel (0 is top edge)
 * @param color The color to set the pixel to
 *
 * @note Coordinates outside the canvas bounds [0, width) x [0, height) are
 *       safely ignored (no bounds checking error).
 *
 * @note Changes are made to the back buffer only. Call picoCanvasSwapBuffers()
 *       to display your drawing on screen.
 *
 * @see picoCanvasSwapBuffers(), picoCanvasRgba2Color()
 */
void picoCanvasDrawPixel(picoCanvas canvas, int32_t x, int32_t y, picoCanvasColor color);

// Color Utility Functions

/**
 * @brief Converts RGBA color components to a picoCanvasColor value
 *
 * Packs four 8-bit color channels into a single 32-bit color value that can
 * be used with drawing functions.
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255, where 0 is transparent and 255 is fully opaque)
 *
 * @return A packed picoCanvasColor value
 *
 * @note The color format is RGBA with 8 bits per channel, packed as:
 *       (R << 24) | (G << 16) | (B << 8) | A
 *
 * @see picoCanvasColor2Rgba()
 */
picoCanvasColor picoCanvasRgba2Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @brief Extracts RGBA components from a picoCanvasColor value
 *
 * Unpacks a 32-bit color value into its individual 8-bit RGBA components.
 *
 * @param color The packed color value to decompose
 * @param r Pointer to store the red component (can be NULL to skip)
 * @param g Pointer to store the green component (can be NULL to skip)
 * @param b Pointer to store the blue component (can be NULL to skip)
 * @param a Pointer to store the alpha component (can be NULL to skip)
 *
 * @note Any parameter can be NULL if you don't need that particular component.
 *       This is useful when you only need to extract specific channels.
 *
 * @see picoCanvasRgba2Color()
 */
void picoCanvasColor2Rgba(picoCanvasColor color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_CANVAS_IMPLEMENTATION)
#define PICO_CANVAS_IMPLEMENTATION
#endif

#ifdef PICO_CANVAS_IMPLEMENTATION

// Common Implementation (Platform Independent)

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

// Platform Specific Implementation

#if defined(PICO_CANVAS_WIN32)
// --------------------------------------------
// Win32 Implementation
// --------------------------------------------

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

// --------------------------------------------
// Internal Helper Functions (Win32)
// --------------------------------------------

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

// --------------------------------------------
// Public API Implementation (Win32)
// --------------------------------------------

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
// --------------------------------------------
// X11 Implementation (Linux)
// --------------------------------------------

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    XImage *image;
    uint32_t *buffer;
} __picoCanvasGraphicsBuffer_t;
typedef __picoCanvasGraphicsBuffer_t *__picoCanvasGraphicsBuffer;

struct picoCanvas_t {
    Display *display;
    int screen;
    Window rootWindow;
    Window windowHandle;
    GC gc;
    Pixmap pixmap;
    bool isOpen;
    int32_t width;
    int32_t height;
    __picoCanvasGraphicsBuffer frontBuffer;
    __picoCanvasGraphicsBuffer backBuffer;
    picoCanvasLoggerCallback logger;
    picoCanvasResizeCallback resizeCallback;
    void *userData;
    Atom wmDeleteWindow;
};

// --------------------------------------------
// Internal Helper Functions (X11)
// --------------------------------------------

static __picoCanvasGraphicsBuffer __picoCanvasGraphicsBufferCreate(Display *display, int screen, int32_t width, int32_t height)
{
    __picoCanvasGraphicsBuffer buffer = (__picoCanvasGraphicsBuffer)PICO_MALLOC(sizeof(__picoCanvasGraphicsBuffer_t));
    if (!buffer)
        return NULL;
    memset(buffer, 0, sizeof(__picoCanvasGraphicsBuffer_t));

    buffer->buffer = (uint32_t *)PICO_MALLOC(width * height * sizeof(uint32_t));
    if (!buffer->buffer) {
        PICO_FREE(buffer);
        return NULL;
    }

    buffer->image = XCreateImage(
        display,
        DefaultVisual(display, screen),
        DefaultDepth(display, screen),
        ZPixmap,
        0,
        (char *)buffer->buffer,
        width,
        height,
        32,
        0);

    if (!buffer->image) {
        PICO_FREE(buffer->buffer);
        PICO_FREE(buffer);
        return NULL;
    }

    return buffer;
}

static void __picoCanvasGraphicsBufferDestroy(__picoCanvasGraphicsBuffer buffer)
{
    if (buffer->image) {
        buffer->image->data = NULL; // Prevent XDestroyImage from freeing our buffer
        XDestroyImage(buffer->image);
    }
    if (buffer->buffer)
        PICO_FREE(buffer->buffer);
    PICO_FREE(buffer);
}

static bool __picoCanvasGraphicsBufferRecreate(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);

    if (canvas->pixmap)
        XFreePixmap(canvas->display, canvas->pixmap);

    canvas->frontBuffer = __picoCanvasGraphicsBufferCreate(canvas->display, canvas->screen, canvas->width, canvas->height);
    canvas->backBuffer  = __picoCanvasGraphicsBufferCreate(canvas->display, canvas->screen, canvas->width, canvas->height);

    if (!canvas->frontBuffer || !canvas->backBuffer)
        return false;

    canvas->pixmap = XCreatePixmap(
        canvas->display,
        canvas->windowHandle,
        canvas->width,
        canvas->height,
        DefaultDepth(canvas->display, canvas->screen));

    return canvas->pixmap != 0;
}

// --------------------------------------------
// Public API Implementation (X11)
// --------------------------------------------

picoCanvas picoCanvasCreate(const char *name, int32_t width, int32_t height, picoCanvasLoggerCallback logger)
{
    picoCanvas canvas = (picoCanvas)PICO_MALLOC(sizeof(picoCanvas_t));
    if (!canvas) {
        if (logger)
            logger("Failed to allocate memory for picoCanvas", canvas);
        return NULL;
    }
    memset(canvas, 0, sizeof(picoCanvas_t));

    canvas->width    = width;
    canvas->height   = height;
    canvas->isOpen   = true;
    canvas->logger   = logger;
    canvas->userData = NULL;

    canvas->display = XOpenDisplay(NULL);
    if (!canvas->display) {
        if (canvas->logger)
            canvas->logger("Failed to open X display", canvas);
        PICO_FREE(canvas);
        return NULL;
    }

    canvas->screen     = DefaultScreen(canvas->display);
    canvas->rootWindow = RootWindow(canvas->display, canvas->screen);

    XSetWindowAttributes windowAttributes = {0};
    windowAttributes.background_pixel     = WhitePixel(canvas->display, canvas->screen);
    windowAttributes.border_pixel         = BlackPixel(canvas->display, canvas->screen);
    windowAttributes.event_mask           = ExposureMask | KeyPressMask | ButtonPressMask |
                                  PointerMotionMask | StructureNotifyMask;

    canvas->windowHandle = XCreateWindow(
        canvas->display,
        canvas->rootWindow,
        100, 100,
        width, height,
        1,
        DefaultDepth(canvas->display, canvas->screen),
        InputOutput,
        DefaultVisual(canvas->display, canvas->screen),
        CWBackPixel | CWBorderPixel | CWEventMask,
        &windowAttributes);

    if (!canvas->windowHandle) {
        if (canvas->logger)
            canvas->logger("Failed to create X window", canvas);
        XCloseDisplay(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    XStoreName(canvas->display, canvas->windowHandle, name ? name : "PicoCanvas");

    canvas->wmDeleteWindow = XInternAtom(canvas->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(canvas->display, canvas->windowHandle, &canvas->wmDeleteWindow, 1);

    XGCValues gcValues  = {0};
    gcValues.foreground = BlackPixel(canvas->display, canvas->screen);
    gcValues.background = WhitePixel(canvas->display, canvas->screen);
    gcValues.line_style = LineSolid;
    gcValues.line_width = 1;
    gcValues.cap_style  = CapButt;
    gcValues.join_style = JoinMiter;
    gcValues.fill_style = FillSolid;

    unsigned long gcMask = GCBackground | GCForeground | GCLineStyle |
                           GCLineWidth | GCCapStyle | GCJoinStyle | GCFillStyle;
    canvas->gc = XCreateGC(canvas->display, canvas->windowHandle, gcMask, &gcValues);

    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
        if (canvas->logger)
            canvas->logger("Failed to create graphics buffers", canvas);
        XFreeGC(canvas->display, canvas->gc);
        XDestroyWindow(canvas->display, canvas->windowHandle);
        XCloseDisplay(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    XMapWindow(canvas->display, canvas->windowHandle);
    XFlush(canvas->display);

    return canvas;
}

void picoCanvasDestroy(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    if (canvas->pixmap)
        XFreePixmap(canvas->display, canvas->pixmap);
    if (canvas->gc)
        XFreeGC(canvas->display, canvas->gc);
    if (canvas->windowHandle)
        XDestroyWindow(canvas->display, canvas->windowHandle);
    if (canvas->display)
        XCloseDisplay(canvas->display);
    PICO_FREE(canvas);
}

void picoCanvasUpdate(picoCanvas canvas)
{
    XEvent event = {0};

    while (XPending(canvas->display)) {
        XNextEvent(canvas->display, &event);
        switch (event.type) {
            case Expose:
                break;
            case ConfigureNotify:
                if (event.xconfigure.width != canvas->width ||
                    event.xconfigure.height != canvas->height) {
                    canvas->width  = event.xconfigure.width;
                    canvas->height = event.xconfigure.height;
                    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
                        if (canvas->logger)
                            canvas->logger("Failed to recreate graphics buffers on resize", canvas);
                    }
                    if (canvas->resizeCallback)
                        canvas->resizeCallback(canvas->width, canvas->height, canvas);
                }
                break;
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == canvas->wmDeleteWindow) {
                    canvas->isOpen = false;
                }
                break;
            case DestroyNotify:
                canvas->isOpen = false;
                break;
            case KeyPress:
            case ButtonPress:
            case MotionNotify:
                break;
            default:
                break;
        }
    }
}

void picoCanvasSwapBuffers(picoCanvas canvas)
{
    memcpy(canvas->frontBuffer->buffer, canvas->backBuffer->buffer, canvas->width * canvas->height * sizeof(uint32_t));

    XPutImage(
        canvas->display,
        canvas->pixmap,
        canvas->gc,
        canvas->frontBuffer->image,
        0, 0, 0, 0,
        canvas->width,
        canvas->height);

    XCopyArea(
        canvas->display,
        canvas->pixmap,
        canvas->windowHandle,
        canvas->gc,
        0, 0,
        canvas->width,
        canvas->height,
        0, 0);

    XFlush(canvas->display);
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
    XStoreName(canvas->display, canvas->windowHandle, title);
    XFlush(canvas->display);
}

void picoCanvasSetSize(picoCanvas canvas, int32_t width, int32_t height)
{
    XResizeWindow(canvas->display, canvas->windowHandle, width, height);
    XFlush(canvas->display);
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

#elif defined(PICO_CANVAS_WAYLAND)
// --------------------------------------------
// Wayland Implementation (Linux)
// --------------------------------------------

// TODO: Wayland implementation

#else
#error "No backend platform selected for picoCanvas"
#endif // platform selection

#endif // PICO_CANVAS_IMPLEMENTATION

#endif // PICO_CANVAS_H