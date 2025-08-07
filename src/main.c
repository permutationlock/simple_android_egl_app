// Copyright (c) 2025 Daniel Aven Bross

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/native_window.h>
#include <android/log.h>

#include "android_native_app_glue.h"

#define SEGL_ANDROID_LOG_ID "SEGLAPP"

#define PERIOD (int64_t)(2.0f * M_PI * 1e9f)

#define countof(x) (sizeof(x) / (sizeof((x)[0])))

struct egl_ctx {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
};

static void egl_ctx_load(struct egl_ctx *egl_ctx, struct android_app *app) {
    egl_ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_ctx->display == EGL_NO_DISPLAY) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to find EGL display"
        );
        exit(1);
    }

    EGLint major;
    EGLint minor;
    if (!eglInitialize(egl_ctx->display, &major, &minor)) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to initialize EGL display"
        );
        exit(1);
    }

    // NOTE: may wish to require an 8 bit alpha channel as well
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE,
    };
    EGLConfig configs[64];
    EGLint nconfigs;
    if (
        !eglChooseConfig(
            egl_ctx->display,
            attribs,
            configs,
            (EGLint)countof(configs),
            &nconfigs
        ) ||
        nconfigs == 0
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to find EGL config"
        );
        exit(1);
    }

    // NOTE: we just select the config with the most MSAA samples from first 64
    EGLint best_i = 0;
    EGLint max_samples = 0;
    for (EGLint i = 0; i < nconfigs; i += 1) {
        EGLint samples;
        eglGetConfigAttrib(egl_ctx->display, configs[i], EGL_SAMPLES, &samples);
        if (samples > max_samples) {
            best_i = i;
            max_samples = samples;
        }
    }

    egl_ctx->config = configs[best_i];
    const EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 2,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE,
    };
    egl_ctx->context = eglCreateContext(
        egl_ctx->display,
        egl_ctx->config,
        EGL_NO_CONTEXT,
        context_attribs
    );
    if (egl_ctx->context == EGL_NO_CONTEXT) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to create EGL context"
        );
        exit(1);
    }
    egl_ctx->surface = eglCreateWindowSurface(
        egl_ctx->display,
        egl_ctx->config,
        app->window,
        NULL
    );
    if (egl_ctx->surface == EGL_NO_SURFACE) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to create EGL surface"
        );
        exit(1);
    }

    if (
        !eglMakeCurrent(
            egl_ctx->display,
            egl_ctx->surface,
            egl_ctx->surface,
            egl_ctx->context
        )
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to set EGL surface and context"
        );
        exit(1);
    }
}

static void egl_ctx_unload(struct egl_ctx *egl_ctx) {
    if (egl_ctx->display == EGL_NO_DISPLAY) {
        return;
    }

    eglMakeCurrent(
        egl_ctx->display,
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT
    );

    if (egl_ctx->context != EGL_NO_CONTEXT) {
        eglDestroyContext(egl_ctx->display, egl_ctx->context);
    }

    if (egl_ctx->surface != EGL_NO_SURFACE) {
        eglDestroySurface(egl_ctx->display, egl_ctx->surface);
    }

    eglTerminate(egl_ctx->display);

    egl_ctx->display = EGL_NO_DISPLAY;
    egl_ctx->context = EGL_NO_CONTEXT;
    egl_ctx->surface = EGL_NO_SURFACE;
}

static struct egl_ctx egl_ctx = {
    .display = EGL_NO_DISPLAY,
    .context = EGL_NO_CONTEXT,
    .surface = EGL_NO_SURFACE,
};

static void handle_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_INIT_WINDOW"
            );

            if (egl_ctx.display == EGL_NO_DISPLAY) {
                egl_ctx_load(&egl_ctx, app);
            }

            break;
        case APP_CMD_TERM_WINDOW:
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_TERM_WINDOW"
            );

            if (egl_ctx.display != EGL_NO_DISPLAY) {
                egl_ctx_unload(&egl_ctx);
            }

            break;
        case APP_CMD_DESTROY:
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_DESTROY"
            );
            break;
        default:
            break;
    }
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    return 0;
}

static inline int64_t time_since(struct timespec end, struct timespec start) {
    int64_t seconds = (int64_t)end.tv_sec - (int64_t)start.tv_sec;
    int64_t sec_diff = seconds * 1000L * 1000L * 1000L;
    int64_t nsec_diff = (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;

    return sec_diff + nsec_diff;
}

void android_main(struct android_app *app) {
    __android_log_print(ANDROID_LOG_INFO, SEGL_ANDROID_LOG_ID, "android_main");

    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    int64_t elapsed = 0;
    struct timespec last;
    clock_gettime(CLOCK_MONOTONIC, &last);

    for (;;) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed += time_since(now, last);
        last = now;

        while (elapsed >= PERIOD) {
            elapsed -= PERIOD;
        }

        int events;
        struct android_poll_source *source;
        while (ALooper_pollOnce(0, 0, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }
        }

        if (egl_ctx.display == EGL_NO_DISPLAY) {
            const struct timespec duration = { .tv_nsec = 32L * 1000L * 1000L };
            nanosleep(&duration, NULL);
        } else {
            int width = ANativeWindow_getWidth(app->window);
            int height = ANativeWindow_getHeight(app->window);

            float t = (float)elapsed / 1e9f;

            glViewport(0, 0, width, height);
            glClearColor(
                0.5f * (1.0f + cosf(t)),
                0.5f * (1.0f + sinf(t)),
                0.5f * (1.0f - cosf(t)),
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT);

            eglSwapBuffers(egl_ctx.display, egl_ctx.surface);
        }
    }
}
