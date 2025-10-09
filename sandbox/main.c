#include <stdio.h>

#define PICO_IMPLEMENTATION
#include "pico/picoCanvas.h"

void logger(const char* message, picoCanvas canvas) {
    (void)canvas; // unused
    printf("Logger: %s\n", message);
}

int main() {
    printf("Hello, Pico!\n");

    picoCanvas canvas = picoCanvasCreate("PicoCanvas Example", 800, 600, logger);

    while (true) {
        picoCanvasUpdate(canvas);
    }

    picoCanvasDestroy(canvas);
    printf("Hello, Pico!\n");
    return 0;
}