#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define PICO_IMPLEMENTATION
#include "pico/picoCanvas.h"

typedef struct {
    picoCanvas canvas;
} AppState;

void drawCircle(picoCanvas canvas, int32_t cx, int32_t cy, int32_t r, picoCanvasColor color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                picoCanvasDrawPixel(canvas, cx + x, cy + y, color);
            }
        }
    }
}

void drawRect(picoCanvas canvas, int32_t x, int32_t y, int32_t w, int32_t h, picoCanvasColor color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            picoCanvasDrawPixel(canvas, x + j, y + i, color);
        }
    }
}

static void draw(AppState* state) {
    int32_t w, h;
    picoCanvasGetSize(state->canvas, &w, &h);
    
    
    picoCanvasClear(state->canvas, picoCanvasRgba2Color(25, 25, 25, 255));
    
    float time = picoCanvasGetTime(state->canvas) * 0.01f;

    for (int i = 0; i < 5; i++) {
        float phase = time + i * 1.2f;
        int32_t cx = w/2 + (int32_t)(cos(phase * 0.5f) * (w * 0.3f));
        int32_t cy = h/2 + (int32_t)(sin(phase * 0.3f) * (h * 0.3f));
        int32_t r = 20 + (int32_t)(sin(phase) * 10);
        uint8_t shade = 60 + (uint8_t)i * 30;
        drawCircle(state->canvas, cx, cy, r, picoCanvasRgba2Color(shade, shade, shade + 10, 200));
    }
    
    for (int i = 0; i < 4; i++) {
        float angle = time * 0.8f + i * 3.14159f / 2;
        int32_t x = (i % 2 == 0 ? 50 : w - 100) + (int32_t)(cos(angle) * 30);
        int32_t y = (i < 2 ? 50 : h - 100) + (int32_t)(sin(angle) * 30);
        uint8_t grey = 100 + (uint8_t)i * 30;
        drawRect(state->canvas, x, y, 50, 50, picoCanvasRgba2Color(grey, grey, grey + 20, 180));
    }
    
    for (int x = 0; x < w; x += 4) {
        int32_t y = h/2 + (int32_t)(sin(x * 0.05f + time * 2) * 40);
        drawCircle(state->canvas, x, y, 3, picoCanvasRgba2Color(140, 140, 150, 255));
    }
}

void logger(const char *message, picoCanvas canvas) {
    (void)canvas;
    printf("Logger: %s\n", message);
}

void resizeCallback(int32_t width, int32_t height, picoCanvas canvas) {
    (void)width, (void)height;
    AppState* state = (AppState*)picoCanvasGetUserData(canvas);
    draw(state);
    picoCanvasSwapBuffers(state->canvas);
}

int main(void) {
    printf("Hello, Pico!\n");
    AppState state = {0};
    state.canvas = picoCanvasCreate("picoCanvas Example - Jaysmito Mukherjee", 800, 600, logger);
    picoCanvasSetUserData(state.canvas, &state);
    picoCanvasSetResizeCallback(state.canvas, resizeCallback);
    
    while (picoCanvasIsOpen(state.canvas)) {
        draw(&state);
        picoCanvasSwapBuffers(state.canvas);
        picoCanvasUpdate(state.canvas);
    }
    
    picoCanvasDestroy(state.canvas);
    printf("Goodbye, Pico!\n");
    return 0;
}
