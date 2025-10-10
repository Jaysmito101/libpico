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

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

// ---------------------------------------------------------------------------------------------------------------

typedef struct picoCanvas_t picoCanvas_t;
typedef picoCanvas_t *picoCanvas;
typedef uint32_t picoCanvasColor;
typedef void (*picoCanvasLoggerCallback)(const char *message, picoCanvas canvas);
typedef void (*picoCanvasResizeCallback)(int32_t width, int32_t height, picoCanvas canvas);

// ---------------------------------------------------------------------------------------------------------------

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
void picoCanvasGetSize(picoCanvas canvas, int32_t *width, int32_t *height);
void picoCanvasClear(picoCanvas canvas, picoCanvasColor color);
void picoCanvasDrawPixel(picoCanvas canvas, int32_t x, int32_t y, picoCanvasColor color);
picoCanvasColor picoCanvasRgba2Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void picoCanvasColor2Rgba(picoCanvasColor color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

// ---------------------------------------------------------------------------------------------------------------

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_CANVAS_IMPLEMENTATION)
#define PICO_CANVAS_IMPLEMENTATION
#endif

#if defined(_WIN32) || defined(_WIN64)
#define PICO_CANVAS_WIN32
#elif defined(__linux__) || defined(__unix__)
#if defined(PICO_CANVAS_PREFER_WAYLAND)
#define PICO_CANVAS_WAYLAND
#elif defined(PICO_CANVAS_PREFER_X11)
#define PICO_CANVAS_X11
#else
#define PICO_CANVAS_X11
#endif
#else
#error "Unsupported platform for picoCanvas"
#endif

// ---------------------------------------------------------------------------------------------------------------

#ifdef PICO_CANVAS_IMPLEMENTATION

// ---------------------------------------------------------------------------------------------------------------
#if 1 // Common function implemntations

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

#endif
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
#ifdef PICO_CANVAS_WIN32

#include <Windows.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------

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
        bmi.bmiHeader.biHeight      = -height;
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

// ---------------------------------------------------------------------------------------------------------------

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

void picoCanvasGetSize(picoCanvas canvas, int32_t *width, int32_t *height)
{
    if (width)
        *width = canvas->width;
    if (height)
        *height = canvas->height;
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

#endif // PICO_CANVAS_WIN32
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
#ifdef PICO_CANVAS_X11

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------

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
        buffer->image->data = NULL;
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

// ---------------------------------------------------------------------------------------------------------------

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

void picoCanvasGetSize(picoCanvas canvas, int32_t *width, int32_t *height)
{
    if (width)
        *width = canvas->width;
    if (height)
        *height = canvas->height;
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

#endif // PICO_CANVAS_X11
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
#if defined(PICO_CANVAS_WAYLAND)

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

// ---------------------------------------------------------------------------------------------------------------

typedef struct {
    struct wl_buffer *buffer;
    uint32_t *data;
    int32_t width;
    int32_t height;
} __picoCanvasGraphicsBuffer_t;
typedef __picoCanvasGraphicsBuffer_t *__picoCanvasGraphicsBuffer;

struct picoCanvas_t {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct wl_shell *shell;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    bool isOpen;
    int32_t width;
    int32_t height;
    __picoCanvasGraphicsBuffer frontBuffer;
    __picoCanvasGraphicsBuffer backBuffer;
    picoCanvasLoggerCallback logger;
    picoCanvasResizeCallback resizeCallback;
    void *userData;
};

// ---------------------------------------------------------------------------------------------------------------

static bool __picoCanvasGraphicsBufferRecreate(picoCanvas canvas);

static void __picoCanvasRegistryHandler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    picoCanvas canvas = (picoCanvas)data;

    if (strcmp(interface, "wl_compositor") == 0) {
        canvas->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        canvas->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        canvas->shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    }
}

static void __picoCanvasRegistryRemover(void *data, struct wl_registry *registry, uint32_t id)
{
}

static const struct wl_registry_listener __picoCanvasRegistryListener = {
    __picoCanvasRegistryHandler,
    __picoCanvasRegistryRemover};

static void __picoCanvasShellSurfacePing(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void __picoCanvasShellSurfaceConfigure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
    picoCanvas canvas = (picoCanvas)data;
    if (width > 0 && height > 0 && (width != canvas->width || height != canvas->height)) {
        canvas->width  = width;
        canvas->height = height;
        if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
            if (canvas->logger)
                canvas->logger("Failed to recreate graphics buffers on resize", canvas);
        }
        if (canvas->resizeCallback)
            canvas->resizeCallback(width, height, canvas);
    }
}

static void __picoCanvasShellSurfacePopupDone(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener __picoCanvasShellSurfaceListener = {
    __picoCanvasShellSurfacePing,
    __picoCanvasShellSurfaceConfigure,
    __picoCanvasShellSurfacePopupDone};

static int __picoCanvasCreateSharedMemoryFile(off_t size)
{
    static const char template[] = "/picocanvas-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = PICO_MALLOC(strlen(path) + sizeof(template));
    if (!name)
        return -1;

    strcpy(name, path);
    strcat(name, template);

    fd = mkstemp(name);
    if (fd >= 0) {
        unlink(name);
        ret = ftruncate(fd, size);
        if (ret < 0) {
            close(fd);
            fd = -1;
        }
    }

    PICO_FREE(name);
    return fd;
}

static __picoCanvasGraphicsBuffer __picoCanvasGraphicsBufferCreate(picoCanvas canvas, int32_t width, int32_t height, bool createBuffer)
{
    __picoCanvasGraphicsBuffer buffer = (__picoCanvasGraphicsBuffer)PICO_MALLOC(sizeof(__picoCanvasGraphicsBuffer_t));
    if (!buffer)
        return NULL;
    memset(buffer, 0, sizeof(__picoCanvasGraphicsBuffer_t));

    buffer->width  = width;
    buffer->height = height;

    if (createBuffer) {
        int32_t stride = width * 4;
        int32_t size   = stride * height;

        int fd = __picoCanvasCreateSharedMemoryFile(size);
        if (fd < 0) {
            PICO_FREE(buffer);
            return NULL;
        }

        buffer->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (buffer->data == MAP_FAILED) {
            close(fd);
            PICO_FREE(buffer);
            return NULL;
        }

        struct wl_shm_pool *pool = wl_shm_create_pool(canvas->shm, fd, size);
        buffer->buffer           = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        if (!buffer->buffer) {
            munmap(buffer->data, size);
            PICO_FREE(buffer);
            return NULL;
        }
    } else {
        buffer->data = (uint32_t *)PICO_MALLOC(width * height * sizeof(uint32_t));
        if (!buffer->data) {
            PICO_FREE(buffer);
            return NULL;
        }
        buffer->buffer = NULL;
    }

    return buffer;
}

static void __picoCanvasGraphicsBufferDestroy(__picoCanvasGraphicsBuffer buffer)
{
    if (buffer->buffer) {
        wl_buffer_destroy(buffer->buffer);
        munmap(buffer->data, buffer->width * buffer->height * 4);
    } else {
        if (buffer->data)
            PICO_FREE(buffer->data);
    }
    PICO_FREE(buffer);
}

static bool __picoCanvasGraphicsBufferRecreate(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);

    canvas->frontBuffer = __picoCanvasGraphicsBufferCreate(canvas, canvas->width, canvas->height, true);
    canvas->backBuffer  = __picoCanvasGraphicsBufferCreate(canvas, canvas->width, canvas->height, false);

    return canvas->frontBuffer && canvas->backBuffer;
}

// ---------------------------------------------------------------------------------------------------------------

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

    canvas->display = wl_display_connect(NULL);
    if (!canvas->display) {
        if (canvas->logger)
            canvas->logger("Failed to connect to Wayland display", canvas);
        PICO_FREE(canvas);
        return NULL;
    }

    canvas->registry = wl_display_get_registry(canvas->display);
    wl_registry_add_listener(canvas->registry, &__picoCanvasRegistryListener, canvas);
    wl_display_dispatch(canvas->display);
    wl_display_roundtrip(canvas->display);

    if (!canvas->compositor || !canvas->shm || !canvas->shell) {
        if (canvas->logger)
            canvas->logger("Failed to bind required Wayland interfaces", canvas);
        wl_display_disconnect(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    canvas->surface = wl_compositor_create_surface(canvas->compositor);
    if (!canvas->surface) {
        if (canvas->logger)
            canvas->logger("Failed to create Wayland surface", canvas);
        wl_display_disconnect(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    canvas->shell_surface = wl_shell_get_shell_surface(canvas->shell, canvas->surface);
    if (!canvas->shell_surface) {
        if (canvas->logger)
            canvas->logger("Failed to create shell surface", canvas);
        wl_surface_destroy(canvas->surface);
        wl_display_disconnect(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    wl_shell_surface_add_listener(canvas->shell_surface, &__picoCanvasShellSurfaceListener, canvas);
    wl_shell_surface_set_toplevel(canvas->shell_surface);
    wl_shell_surface_set_title(canvas->shell_surface, name ? name : "PicoCanvas");

    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
        if (canvas->logger)
            canvas->logger("Failed to create graphics buffers", canvas);
        wl_shell_surface_destroy(canvas->shell_surface);
        wl_surface_destroy(canvas->surface);
        wl_display_disconnect(canvas->display);
        PICO_FREE(canvas);
        return NULL;
    }

    wl_display_flush(canvas->display);

    return canvas;
}

void picoCanvasDestroy(picoCanvas canvas)
{
    if (canvas->frontBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->frontBuffer);
    if (canvas->backBuffer)
        __picoCanvasGraphicsBufferDestroy(canvas->backBuffer);
    if (canvas->shell_surface)
        wl_shell_surface_destroy(canvas->shell_surface);
    if (canvas->surface)
        wl_surface_destroy(canvas->surface);
    if (canvas->shell)
        wl_shell_destroy(canvas->shell);
    if (canvas->compositor)
        wl_compositor_destroy(canvas->compositor);
    if (canvas->shm)
        wl_shm_destroy(canvas->shm);
    if (canvas->registry)
        wl_registry_destroy(canvas->registry);
    if (canvas->display)
        wl_display_disconnect(canvas->display);
    PICO_FREE(canvas);
}

void picoCanvasUpdate(picoCanvas canvas)
{
    if (wl_display_dispatch_pending(canvas->display) == -1) {
        canvas->isOpen = false;
    }
}

void picoCanvasSwapBuffers(picoCanvas canvas)
{

    memcpy(canvas->frontBuffer->data, canvas->backBuffer->data, canvas->width * canvas->height * sizeof(uint32_t));

    wl_surface_attach(canvas->surface, canvas->frontBuffer->buffer, 0, 0);
    wl_surface_damage(canvas->surface, 0, 0, canvas->width, canvas->height);
    wl_surface_commit(canvas->surface);
    wl_display_flush(canvas->display);
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
    wl_shell_surface_set_title(canvas->shell_surface, title ? title : "PicoCanvas");
    wl_display_flush(canvas->display);
}

void picoCanvasSetSize(picoCanvas canvas, int32_t width, int32_t height)
{

    canvas->width  = width;
    canvas->height = height;
    if (!__picoCanvasGraphicsBufferRecreate(canvas)) {
        if (canvas->logger)
            canvas->logger("Failed to recreate graphics buffers on size change", canvas);
    }
    wl_display_flush(canvas->display);
}

void picoCanvasGetSize(picoCanvas canvas, int32_t *width, int32_t *height)
{
    if (width)
        *width = canvas->width;
    if (height)
        *height = canvas->height;
}

void picoCanvasClear(picoCanvas canvas, picoCanvasColor color)
{
    for (int i = 0; i < canvas->width * canvas->height; ++i)
        canvas->backBuffer->data[i] = color;
}

void picoCanvasDrawPixel(picoCanvas canvas, int32_t x, int32_t y, picoCanvasColor color)
{
    if (x < 0 || x >= canvas->width || y < 0 || y >= canvas->height)
        return;
    canvas->backBuffer->data[y * canvas->width + x] = color;
}

// ---------------------------------------------------------------------------------------------------------------

#endif // PICO_CANVAS_WAYLAND
// ---------------------------------------------------------------------------------------------------------------

#endif // PICO_CANVAS_IMPLEMENTATION

#endif // PICO_CANVAS_H