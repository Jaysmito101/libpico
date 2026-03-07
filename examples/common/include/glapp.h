#ifndef PICO_COMMON_GL_APP_H
#define PICO_COMMON_GL_APP_H

typedef struct picoCommonGlApp_t picoCommonGlApp_t;
typedef picoCommonGlApp_t *picoCommonGlApp;


picoCommonGlApp picoCommonGlAppCreate(const char *title, int width, int height);
void picoCommonGlAppDestroy(picoCommonGlApp app);
void picoCommonGlAppSwapBuffers(picoCommonGlApp app);
void picoCommonGlAppSetUserData(picoCommonGlApp app, void *userData);
void *picoCommonGlAppGetUserData(picoCommonGlApp app);

int picoCommonGlAppGetWidth(picoCommonGlApp app);
int picoCommonGlAppGetHeight(picoCommonGlApp app);
int picoCommonGlAppGetFramebufferWidth(picoCommonGlApp app);
int picoCommonGlAppGetFramebufferHeight(picoCommonGlApp app);

int picoCommonGlAppShouldClose(picoCommonGlApp app);
void picoCommonGlAppPollEvents(picoCommonGlApp app);

double picoCommonGlAppGetTime(picoCommonGlApp app);

#endif
