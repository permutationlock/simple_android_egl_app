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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android/log.h>

#include <jni.h>

#include "android_native_app_glue.h"

#define SEGL_ANDROID_LOG_ID "SEGL"

#define PERIOD (int64_t)(2.0f * M_PI * 1e9f)

#define countof(x) (sizeof(x) / (sizeof((x)[0])))

struct egl_ctx {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
};

static void egl_ctx_load(struct egl_ctx *egl_ctx, struct android_app *app) {
    if (egl_ctx->display != EGL_NO_DISPLAY) {
        return;
    }

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
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_CONFORMANT,
        EGL_OPENGL_ES2_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_COLOR_BUFFER_TYPE,
        EGL_RGB_BUFFER,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
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
        EGL_CONTEXT_MAJOR_VERSION,
        2,
        EGL_CONTEXT_MINOR_VERSION,
        0,
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

static char *path_join(const char *p1, const char *p2) {
    size_t p1_len = strlen(p1);
    size_t p2_len = strlen(p2);

    char *p3 = malloc(p1_len + p2_len + 2);
    size_t p3_len = 0;

    memcpy(&p3[p3_len], p1, p1_len);
    p3_len += p1_len;

    p3[p3_len++] = '/';

    memcpy(&p3[p3_len], p2, p2_len);
    p3_len += p2_len;

    p3[p3_len] = '\0';

    return p3;
}

struct state {
    int64_t elapsed;
};

static void load_state(struct state *state, struct android_app *app) {
    char *fname = path_join(app->activity->internalDataPath, "state");
    FILE *file = fopen(fname, "r");
    if (file != NULL) {
        struct state last_state;
        size_t len = fread(&last_state, sizeof(last_state), 1, file);
        if (len) {
            *state = last_state;
        }
        fclose(file);
    }
    free(fname);
}

static void save_state(struct state *state, struct android_app *app) {
    char *fname = path_join(app->activity->internalDataPath, "state");
    FILE *file = fopen(fname, "w");
    if (file != NULL) {
        fwrite(state, sizeof(*state), 1, file);
        fclose(file);
    }
    free(fname);
}

static struct state state;
static int running;
static struct egl_ctx egl_ctx = {
    .display = EGL_NO_DISPLAY,
    .context = EGL_NO_CONTEXT,
    .surface = EGL_NO_SURFACE,
};

static void handle_cmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_INIT_WINDOW"
            );
            egl_ctx_load(&egl_ctx, app);
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_TERM_WINDOW"
            );
            egl_ctx_unload(&egl_ctx);
            break;
        }
        case APP_CMD_RESUME: {
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_RESUME"
            );
            running = 1;
            load_state(&state, app);
            break;
        }
        case APP_CMD_PAUSE: {
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_PAUSE"
            );
            running = 0;
            save_state(&state, app);
            break;
        }
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

    {
        /* call the Android Java API to enter fullscreen */

        /* NOTE: JavaVM and JNIEnv are pointer types */
        JavaVM jvm = *app->activity->vm;
        JNIEnv *envptr = NULL;
        jvm->AttachCurrentThread(app->activity->vm, &envptr, NULL);

        JNIEnv env = *envptr;

        jclass av_NativeActivity = env->FindClass(
            envptr,
            "android/app/NativeActivity"
        );
        jclass av_Window = env->FindClass(envptr, "android/view/Window");
        jclass av_View = env->FindClass(envptr, "android/view/View");
        jclass avwm_LayoutParams = env->FindClass(
            envptr,
            "android/view/WindowManager$LayoutParams"
        );

        jmethodID av_NativeActivity_getWindow = env->GetMethodID(
            envptr,
            av_NativeActivity,
            "getWindow",
            "()Landroid/view/Window;"
        );
        jmethodID av_Window_getDecorView = env->GetMethodID(
            envptr,
            av_Window,
            "getDecorView",
            "()Landroid/view/View;"
        );
        jmethodID av_Window_addFlags = env->GetMethodID(
            envptr,
            av_Window,
            "addFlags",
            "(I)V"
        );
        jmethodID av_View_setSystemUiVisibility = env->GetMethodID(
            envptr,
            av_View,
            "setSystemUiVisibility",
            "(I)V"
        );

        jfieldID av_View_SYSTEM_UI_FLAG_LOW_PROFILE = env->GetStaticFieldID(
            envptr,
            av_View,
            "SYSTEM_UI_FLAG_LOW_PROFILE",
            "I"
        );
        jfieldID av_View_SYSTEM_UI_FLAG_HIDE_NAVIGATION = env->GetStaticFieldID(
            envptr,
            av_View,
            "SYSTEM_UI_FLAG_HIDE_NAVIGATION",
            "I"
        );
        jfieldID av_View_SYSTEM_UI_FLAG_FULLSCREEN = env->GetStaticFieldID(
            envptr,
            av_View,
            "SYSTEM_UI_FLAG_FULLSCREEN",
            "I"
        );
        jfieldID av_View_SYSTEM_UI_FLAG_IMMERSIVE_STICKY = env->GetStaticFieldID(
            envptr,
            av_View,
            "SYSTEM_UI_FLAG_IMMERSIVE_STICKY",
            "I"
        );
        jfieldID av_View_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = env
            ->GetStaticFieldID(
                envptr,
                av_View,
                "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN",
                "I"
            );
        jfieldID av_View_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = env
            ->GetStaticFieldID(
                envptr,
                av_View,
                "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION",
                "I"
            );
        jfieldID av_View_SYSTEM_UI_FLAG_LAYOUT_STABLE = env->GetStaticFieldID(
            envptr,
            av_View,
            "SYSTEM_UI_FLAG_LAYOUT_STABLE",
            "I"
        );
        jfieldID avwm_LayoutParams_FLAG_FULLSCREEN = env->GetStaticFieldID(
            envptr,
            avwm_LayoutParams,
            "FLAG_FULLSCREEN",
            "I"
        );
        jfieldID avwm_LayoutParams_FLAG_KEEP_SCREEN_ON = env->GetStaticFieldID(
            envptr,
            avwm_LayoutParams,
            "FLAG_KEEP_SCREEN_ON",
            "I"
        );
        jfieldID avwm_LayoutParams_FLAG_HARDWARE_ACCELERATED = env
            ->GetStaticFieldID(
                envptr,
                avwm_LayoutParams,
                "FLAG_HARDWARE_ACCELERATED",
                "I"
            );

        /* Window window = getWindow(); */
        jobject window = env->CallObjectMethod(
            envptr,
            app->activity->clazz,
            av_NativeActivity_getWindow
        );

        /* View decorView = window.getDecorView(); */
        jobject decorView = env->CallObjectMethod(
            envptr,
            window,
            av_Window_getDecorView
        );

        /*
        * decorView.setSystemUiVisibility(
        *     View.SYSTEM_UI_FLAG_LOW_PROFILE |
        *     View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
        *     View.SYSTEM_UI_FLAG_FULLSCREEN |
        *     View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
        *     View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
        *     View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
        *     View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        * );
        */
        env->CallVoidMethod(
            envptr,
            decorView,
            av_View_setSystemUiVisibility,
            env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_LOW_PROFILE
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_HIDE_NAVIGATION
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_FULLSCREEN
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            ) | env->GetStaticIntField(
                envptr,
                av_View,
                av_View_SYSTEM_UI_FLAG_LAYOUT_STABLE
            )
        );

        /*
        * window.addFlags(
        *     WindowManager.LayoutParams.FLAG_FULLSCREEN |
        *     WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |
        *     WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED
        * );
        */
        env->CallVoidMethod(
            envptr,
            window,
            av_Window_addFlags,
            env->GetStaticIntField(
                envptr,
                avwm_LayoutParams,
                avwm_LayoutParams_FLAG_FULLSCREEN
            ) | env->GetStaticIntField(
                envptr,
                avwm_LayoutParams,
                avwm_LayoutParams_FLAG_KEEP_SCREEN_ON
            ) | env->GetStaticIntField(
                envptr,
                avwm_LayoutParams,
                avwm_LayoutParams_FLAG_HARDWARE_ACCELERATED
            )
        );

        jvm->DetachCurrentThread(app->activity->vm);
    }

    {
        /* load a text file asset */

        AAsset *file = AAssetManager_open(
            app->activity->assetManager,
            "name.txt",
            AASSET_MODE_BUFFER
        );
        if (file != NULL) {
            off_t file_len = AAsset_getLength(file);
            const char *file_buffer = AAsset_getBuffer(file);

            char *str = malloc((size_t)file_len + 1);
            memcpy(str, file_buffer, (size_t)file_len);
            str[file_len] = 0;

            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "name asset: %s",
                str
            );

            free(str);
            AAsset_close(file);
        }
    }

    /* main app loop */

    struct timespec last;
    clock_gettime(CLOCK_MONOTONIC, &last);

    for (;;) {
        int events;
        struct android_poll_source *source;

        while (!running) {
            /* while paused, freeze clock and wait for next event */
            ALooper_pollOnce(-1, 0, &events, (void **)&source);
            if (source != NULL) {
                source->process(app, source);
            }
            clock_gettime(CLOCK_MONOTONIC, &last);
        }

        while (ALooper_pollOnce(0, 0, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        state.elapsed += time_since(now, last);
        last = now;

        while (state.elapsed >= PERIOD) {
            state.elapsed -= PERIOD;
        }

        if (egl_ctx.display == EGL_NO_DISPLAY) {
            /* if EGL display not loaded, wait 16ms (1 frame @ 60FPS) */
            const struct timespec duration = { .tv_nsec = 16L * 1000L * 1000L };
            nanosleep(&duration, NULL);
        } else {
            int width = ANativeWindow_getWidth(app->window);
            int height = ANativeWindow_getHeight(app->window);

            float t = (float)state.elapsed / 1e9f;

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
