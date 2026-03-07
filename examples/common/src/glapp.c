#include "glapp.h"
#include <stdio.h>
#include <stdlib.h>

struct picoCommonGlApp_t {
    GLFWwindow *window;
    void *userData;
};

picoCommonGlApp picoCommonGlAppCreate(const char *title, int width, int height)
{
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return NULL;
    }

    glfwMakeContextCurrent(window);

    picoCommonGlApp app = (picoCommonGlApp)malloc(sizeof(struct picoCommonGlApp_t));
    if (!app) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return NULL;
    }

    app->window   = window;
    app->userData = NULL;

    glfwMakeContextCurrent(app->window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        picoCommonGlAppDestroy(app);
        return NULL;
    }

    return app;
}

void picoCommonGlAppDestroy(picoCommonGlApp app)
{
    if (app) {
        if (app->window) {
            glfwDestroyWindow(app->window);
        }
        free(app);
    }
    glfwTerminate();
}

void picoCommonGlAppSwapBuffers(picoCommonGlApp app)
{
    if (app && app->window) {
        glfwSwapBuffers(app->window);
    }
}

void picoCommonGlAppSetUserData(picoCommonGlApp app, void *userData)
{
    if (app) {
        app->userData = userData;
    }
}

void *picoCommonGlAppGetUserData(picoCommonGlApp app)
{
    if (app) {
        return app->userData;
    }
    return NULL;
}

int picoCommonGlAppGetWidth(picoCommonGlApp app)
{
    if (app && app->window) {
        int w, h;
        glfwGetWindowSize(app->window, &w, &h);
        return w;
    }
    return 0;
}

int picoCommonGlAppGetHeight(picoCommonGlApp app)
{
    if (app && app->window) {
        int w, h;
        glfwGetWindowSize(app->window, &w, &h);
        return h;
    }
    return 0;
}

int picoCommonGlAppGetFramebufferWidth(picoCommonGlApp app)
{
    if (app && app->window) {
        int w, h;
        glfwGetFramebufferSize(app->window, &w, &h);
        return w;
    }
    return 0;
}

int picoCommonGlAppGetFramebufferHeight(picoCommonGlApp app)
{
    if (app && app->window) {
        int w, h;
        glfwGetFramebufferSize(app->window, &w, &h);
        return h;
    }
    return 0;
}

int picoCommonGlAppShouldClose(picoCommonGlApp app)
{
    if (app && app->window) {
        return glfwWindowShouldClose(app->window);
    }
    return 1;
}

void picoCommonGlAppPollEvents(picoCommonGlApp app)
{
    (void)app;
    glfwPollEvents();
}

double picoCommonGlAppGetTime(picoCommonGlApp app)
{
    (void)app;
    return glfwGetTime();
}
