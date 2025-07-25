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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <dlfcn.h>

#include <android/native_window.h>
#include <android/log.h>

#include "android_native_app_glue.h"

#define SEGL_ANDROID_LOG_ID "SEGLAPP"

#define TIMESTEP 16L * 1000L * 1000L

#define countof(x) (sizeof(x) / (sizeof((x)[0])))

typedef struct android_app AndroidApp;
typedef struct android_poll_source AndroidPollSource;

typedef struct timespec TimeSpec;

typedef struct {
    PFNEGLCHOOSECONFIGPROC ChooseConfig;
    PFNEGLCOPYBUFFERSPROC CopyBuffers;
    PFNEGLCREATECONTEXTPROC CreateContext;
    PFNEGLCREATEPBUFFERSURFACEPROC CreatePbufferSurface;
    PFNEGLCREATEPIXMAPSURFACEPROC CreatePixmapSurface;
    PFNEGLCREATEWINDOWSURFACEPROC CreateWindowSurface;
    PFNEGLDESTROYCONTEXTPROC DestroyContext;
    PFNEGLDESTROYSURFACEPROC DestroySurface;
    PFNEGLGETCONFIGATTRIBPROC GetConfigAttrib;
    PFNEGLGETCONFIGSPROC GetConfigs;
    PFNEGLGETCURRENTDISPLAYPROC GetCurrentDisplay;
    PFNEGLGETCURRENTSURFACEPROC GetCurrentSurface;
    PFNEGLGETDISPLAYPROC GetDisplay;
    PFNEGLGETERRORPROC GetError;
    PFNEGLGETPROCADDRESSPROC GetProcAddress;
    PFNEGLINITIALIZEPROC Initialize;
    PFNEGLMAKECURRENTPROC MakeCurrent;
    PFNEGLQUERYCONTEXTPROC QueryContext;
    PFNEGLQUERYSTRINGPROC QueryString;
    PFNEGLQUERYSURFACEPROC QuerySurface;
    PFNEGLSWAPBUFFERSPROC SwapBuffers;
    PFNEGLTERMINATEPROC Terminate;
    PFNEGLWAITGLPROC WaitGL;
    PFNEGLWAITNATIVEPROC WaitNative;
} SEglVtable;

static SEglVtable segl_vtable_load(void) {
    void *so_handle = dlopen("libEGL.so", RTLD_LAZY | RTLD_LOCAL);
    if (so_handle == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            "SEGLAP",
            "failed to load libEGL.so: %s",
            dlerror()
        );
        exit(1);
    }

    SEglVtable vtable = { 0 };

    vtable.ChooseConfig = dlsym(so_handle, "eglChooseConfig");
    if (vtable.ChooseConfig == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglChooseConfig: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.CopyBuffers = dlsym(so_handle, "eglCopyBuffers");
    if (vtable.CopyBuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglCopyBuffers: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.CreateContext = dlsym(so_handle, "eglCreateContext");
    if (vtable.CreateContext == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglCreateContext: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.CreatePbufferSurface = dlsym(so_handle, "eglCreatePbufferSurface");
    if (vtable.CreatePbufferSurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglCreatePbufferSu: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.CreatePixmapSurface = dlsym(so_handle, "eglCreatePixmapSurface");
    if (vtable.CreatePixmapSurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglCreatePixmapSurf: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.CreateWindowSurface = dlsym(so_handle, "eglCreateWindowSurface");
    if (vtable.CreateWindowSurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglCreateWindowSurf: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.DestroyContext = dlsym(so_handle, "eglDestroyContext");
    if (vtable.DestroyContext == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglDestroyContext: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.DestroySurface = dlsym(so_handle, "eglDestroySurface");
    if (vtable.DestroySurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglDestroySurface: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetConfigAttrib = dlsym(so_handle, "eglGetConfigAttrib");
    if (vtable.GetConfigAttrib == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetConfigAttrib: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetConfigs = dlsym(so_handle, "eglGetConfigs");
    if (vtable.GetConfigs == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetConfigs: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetCurrentDisplay = dlsym(so_handle, "eglGetCurrentDisplay");
    if (vtable.GetCurrentDisplay == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetCurrentDisplay: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetCurrentSurface = dlsym(so_handle, "eglGetCurrentSurface");
    if (vtable.GetCurrentSurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetCurrentSurface: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetDisplay = dlsym(so_handle, "eglGetDisplay");
    if (vtable.GetDisplay == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetDisplay: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetError = dlsym(so_handle, "eglGetError");
    if (vtable.GetError == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetError: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.GetProcAddress = dlsym(so_handle, "eglGetProcAddress");
    if (vtable.GetProcAddress == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglGetProcAddress: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.Initialize = dlsym(so_handle, "eglInitialize");
    if (vtable.Initialize == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglInitialize: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.MakeCurrent = dlsym(so_handle, "eglMakeCurrent");
    if (vtable.MakeCurrent == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglMakeCurrent: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.QueryContext = dlsym(so_handle, "eglQueryContext");
    if (vtable.QueryContext == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglQueryContext: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.QueryString = dlsym(so_handle, "eglQueryString");
    if (vtable.QueryString == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglQueryString: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.QuerySurface = dlsym(so_handle, "eglQuerySurface");
    if (vtable.QuerySurface == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglQuerySurface: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.SwapBuffers = dlsym(so_handle, "eglSwapBuffers");
    if (vtable.SwapBuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglSwapBuffers: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.Terminate = dlsym(so_handle, "eglTerminate");
    if (vtable.Terminate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglTerminate: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.WaitGL = dlsym(so_handle, "eglWaitGL");
    if (vtable.WaitGL == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglWaitGL: %s",
            dlerror()
        );
        exit(1);
    }
    vtable.WaitNative = dlsym(so_handle, "eglWaitNative");
    if (vtable.WaitNative == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load eglWaitNative: %s",
            dlerror()
        );
        exit(1);
    }

    return vtable;
}

typedef struct {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
} SEglCtx;

static SEglCtx segl_ctx_load(AndroidApp *app, SEglVtable *segl_vtable) {
    SEglCtx segl_ctx;

    segl_ctx.display = segl_vtable->GetDisplay(EGL_DEFAULT_DISPLAY);
    if (segl_ctx.display == EGL_NO_DISPLAY) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to find EGL display"
        );
        exit(1);
    }

    EGLint major;
    EGLint minor;
    if (!segl_vtable->Initialize(segl_ctx.display, &major, &minor)) {
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
    EGLConfig configs[32];
    EGLint nconfigs;
    if (
        !segl_vtable->ChooseConfig(
            segl_ctx.display,
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

    // NOTE: we just select the config with the most MSAA samples from first 32
    EGLint best_i = 0;
    EGLint max_samples = 0;
    for (EGLint i = 0; i < nconfigs; i += 1) {
        EGLint samples;
        assert(
            segl_vtable->GetConfigAttrib(
                segl_ctx.display,
                configs[i],
                EGL_SAMPLES,
                &samples
            )
        );
        if (samples > max_samples) {
            best_i = i;
            max_samples = samples;
        }
    }

    segl_ctx.config = configs[best_i];
    const EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION,
        2,
        EGL_CONTEXT_MINOR_VERSION,
        0,
        EGL_NONE,
    };
    segl_ctx.context = segl_vtable->CreateContext(
        segl_ctx.display,
        segl_ctx.config,
        EGL_NO_CONTEXT,
        context_attribs
    );
    if (segl_ctx.context == EGL_NO_CONTEXT) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to create EGL context"
        );
        exit(1);
    }
    segl_ctx.surface = segl_vtable->CreateWindowSurface(
        segl_ctx.display,
        segl_ctx.config,
        app->window,
        NULL
    );
    if (segl_ctx.surface == EGL_NO_SURFACE) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to create EGL surface"
        );
        exit(1);
    }

    if (
        !segl_vtable->MakeCurrent(
            segl_ctx.display,
            segl_ctx.surface,
            segl_ctx.surface,
            segl_ctx.context
        )
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to set EGL surface and context"
        );
        exit(1);
    }

    return segl_ctx;
}

static void segl_ctx_unload(SEglCtx *segl_ctx, SEglVtable *segl_vtable) {
    if (segl_ctx->display == EGL_NO_DISPLAY) {
        return;
    }

    segl_vtable->MakeCurrent(
        segl_ctx->display,
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT
    );

    if (segl_ctx->context != EGL_NO_CONTEXT) {
        segl_vtable->DestroyContext(segl_ctx->display, segl_ctx->context);
    }

    if (segl_ctx->surface != EGL_NO_SURFACE) {
        segl_vtable->DestroySurface(segl_ctx->display, segl_ctx->surface);
    }

    segl_vtable->Terminate(segl_ctx->display);

    segl_ctx->display = EGL_NO_DISPLAY;
    segl_ctx->context = EGL_NO_CONTEXT;
    segl_ctx->surface = EGL_NO_SURFACE;
}

typedef struct {
    PFNGLACTIVETEXTUREPROC ActiveTexture;
    PFNGLATTACHSHADERPROC AttachShader;
    PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
    PFNGLBINDBUFFERPROC BindBuffer;
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer;
    PFNGLBINDRENDERBUFFERPROC BindRenderbuffer;
    PFNGLBINDTEXTUREPROC BindTexture;
    PFNGLBLENDCOLORPROC BlendColor;
    PFNGLBLENDEQUATIONPROC BlendEquation;
    PFNGLBLENDEQUATIONSEPARATEPROC BlendEquationSeparate;
    PFNGLBLENDFUNCPROC BlendFunc;
    PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
    PFNGLBUFFERDATAPROC BufferData;
    PFNGLBUFFERSUBDATAPROC BufferSubData;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
    PFNGLCLEARPROC Clear;
    PFNGLCLEARCOLORPROC ClearColor;
    PFNGLCLEARDEPTHFPROC ClearDepthf;
    PFNGLCLEARSTENCILPROC ClearStencil;
    PFNGLCOLORMASKPROC ColorMask;
    PFNGLCOMPILESHADERPROC CompileShader;
    PFNGLCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D;
    PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D;
    PFNGLCOPYTEXIMAGE2DPROC CopyTexImage2D;
    PFNGLCOPYTEXSUBIMAGE2DPROC CopyTexSubImage2D;
    PFNGLCREATEPROGRAMPROC CreateProgram;
    PFNGLCREATESHADERPROC CreateShader;
    PFNGLCULLFACEPROC CullFace;
    PFNGLDELETEBUFFERSPROC DeleteBuffers;
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers;
    PFNGLDELETEPROGRAMPROC DeleteProgram;
    PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers;
    PFNGLDELETESHADERPROC DeleteShader;
    PFNGLDELETETEXTURESPROC DeleteTextures;
    PFNGLDEPTHFUNCPROC DepthFunc;
    PFNGLDEPTHMASKPROC DepthMask;
    PFNGLDEPTHRANGEFPROC DepthRangef;
    PFNGLDETACHSHADERPROC DetachShader;
    PFNGLDISABLEPROC Disable;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;
    PFNGLDRAWARRAYSPROC DrawArrays;
    PFNGLDRAWELEMENTSPROC DrawElements;
    PFNGLENABLEPROC Enable;
    PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
    PFNGLFINISHPROC Finish;
    PFNGLFLUSHPROC Flush;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer;
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
    PFNGLFRONTFACEPROC FrontFace;
    PFNGLGENBUFFERSPROC GenBuffers;
    PFNGLGENERATEMIPMAPPROC GenerateMipmap;
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers;
    PFNGLGENRENDERBUFFERSPROC GenRenderbuffers;
    PFNGLGENTEXTURESPROC GenTextures;
    PFNGLGETACTIVEATTRIBPROC GetActiveAttrib;
    PFNGLGETACTIVEUNIFORMPROC GetActiveUniform;
    PFNGLGETATTACHEDSHADERSPROC GetAttachedShaders;
    PFNGLGETATTRIBLOCATIONPROC GetAttribLocation;
    PFNGLGETBOOLEANVPROC GetBooleanv;
    PFNGLGETBUFFERPARAMETERIVPROC GetBufferParameteriv;
    PFNGLGETERRORPROC GetError;
    PFNGLGETFLOATVPROC GetFloatv;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
        GetFramebufferAttachmentParameteriv;
    PFNGLGETINTEGERVPROC GetIntegerv;
    PFNGLGETPROGRAMIVPROC GetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
    PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv;
    PFNGLGETSHADERIVPROC GetShaderiv;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
    PFNGLGETSHADERPRECISIONFORMATPROC GetShaderPrecisionFormat;
    PFNGLGETSHADERSOURCEPROC GetShaderSource;
    PFNGLGETSTRINGPROC GetString;
    PFNGLGETTEXPARAMETERFVPROC GetTexParameterfv;
    PFNGLGETTEXPARAMETERIVPROC GetTexParameteriv;
    PFNGLGETUNIFORMFVPROC GetUniformfv;
    PFNGLGETUNIFORMIVPROC GetUniformiv;
    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
    PFNGLGETVERTEXATTRIBFVPROC GetVertexAttribfv;
    PFNGLGETVERTEXATTRIBIVPROC GetVertexAttribiv;
    PFNGLGETVERTEXATTRIBPOINTERVPROC GetVertexAttribPointerv;
    PFNGLHINTPROC Hint;
    PFNGLISBUFFERPROC IsBuffer;
    PFNGLISENABLEDPROC IsEnabled;
    PFNGLISFRAMEBUFFERPROC IsFramebuffer;
    PFNGLISPROGRAMPROC IsProgram;
    PFNGLISRENDERBUFFERPROC IsRenderbuffer;
    PFNGLISSHADERPROC IsShader;
    PFNGLISTEXTUREPROC IsTexture;
    PFNGLLINEWIDTHPROC LineWidth;
    PFNGLLINKPROGRAMPROC LinkProgram;
    PFNGLPIXELSTOREIPROC PixelStorei;
    PFNGLPOLYGONOFFSETPROC PolygonOffset;
    PFNGLREADPIXELSPROC ReadPixels;
    PFNGLRELEASESHADERCOMPILERPROC ReleaseShaderCompiler;
    PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage;
    PFNGLSAMPLECOVERAGEPROC SampleCoverage;
    PFNGLSCISSORPROC Scissor;
    PFNGLSHADERBINARYPROC ShaderBinary;
    PFNGLSHADERSOURCEPROC ShaderSource;
    PFNGLSTENCILFUNCPROC StencilFunc;
    PFNGLSTENCILFUNCSEPARATEPROC StencilFuncSeparate;
    PFNGLSTENCILMASKPROC StencilMask;
    PFNGLSTENCILMASKSEPARATEPROC StencilMaskSeparate;
    PFNGLSTENCILOPPROC StencilOp;
    PFNGLSTENCILOPSEPARATEPROC StencilOpSeparate;
    PFNGLTEXIMAGE2DPROC TexImage2D;
    PFNGLTEXPARAMETERFPROC TexParameterf;
    PFNGLTEXPARAMETERFVPROC TexParameterfv;
    PFNGLTEXPARAMETERIPROC TexParameteri;
    PFNGLTEXPARAMETERIVPROC TexParameteriv;
    PFNGLTEXSUBIMAGE2DPROC TexSubImage2D;
    PFNGLUNIFORM1FPROC Uniform1f;
    PFNGLUNIFORM1FVPROC Uniform1fv;
    PFNGLUNIFORM1IPROC Uniform1i;
    PFNGLUNIFORM1IVPROC Uniform1iv;
    PFNGLUNIFORM2FPROC Uniform2f;
    PFNGLUNIFORM2FVPROC Uniform2fv;
    PFNGLUNIFORM2IPROC Uniform2i;
    PFNGLUNIFORM2IVPROC Uniform2iv;
    PFNGLUNIFORM3FPROC Uniform3f;
    PFNGLUNIFORM3FVPROC Uniform3fv;
    PFNGLUNIFORM3IPROC Uniform3i;
    PFNGLUNIFORM3IVPROC Uniform3iv;
    PFNGLUNIFORM4FPROC Uniform4f;
    PFNGLUNIFORM4FVPROC Uniform4fv;
    PFNGLUNIFORM4IPROC Uniform4i;
    PFNGLUNIFORM4IVPROC Uniform4iv;
    PFNGLUNIFORMMATRIX2FVPROC UniformMatrix2fv;
    PFNGLUNIFORMMATRIX3FVPROC UniformMatrix3fv;
    PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
    PFNGLUSEPROGRAMPROC UseProgram;
    PFNGLVALIDATEPROGRAMPROC ValidateProgram;
    PFNGLVERTEXATTRIB1FPROC VertexAttrib1f;
    PFNGLVERTEXATTRIB1FVPROC VertexAttrib1fv;
    PFNGLVERTEXATTRIB2FPROC VertexAttrib2f;
    PFNGLVERTEXATTRIB2FVPROC VertexAttrib2fv;
    PFNGLVERTEXATTRIB3FPROC VertexAttrib3f;
    PFNGLVERTEXATTRIB3FVPROC VertexAttrib3fv;
    PFNGLVERTEXATTRIB4FPROC VertexAttrib4f;
    PFNGLVERTEXATTRIB4FVPROC VertexAttrib4fv;
    PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
    PFNGLVIEWPORTPROC Viewport;
} SGlVtable;

static SGlVtable sgl_vtable_load(SEglVtable *segl_vtable) {
    SGlVtable vtable = { 0 };

    vtable.ActiveTexture = (PFNGLACTIVETEXTUREPROC)segl_vtable->GetProcAddress(
        "glActiveTexture"
    );
    if (vtable.ActiveTexture == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glActiveTexture"
        );
        exit(1);
    }
    vtable.AttachShader = (PFNGLATTACHSHADERPROC)segl_vtable->GetProcAddress(
        "glAttachShader"
    );
    if (vtable.AttachShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glAttachShader"
        );
        exit(1);
    }
    vtable.BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)segl_vtable
        ->GetProcAddress("glBindAttribLocation");
    if (vtable.BindAttribLocation == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBindAttribLocation"
        );
        exit(1);
    }
    vtable.BindBuffer = (PFNGLBINDBUFFERPROC)segl_vtable->GetProcAddress(
        "glBindBuffer"
    );
    if (vtable.BindBuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBindBuffer"
        );
        exit(1);
    }
    vtable.BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)segl_vtable
        ->GetProcAddress("glBindFramebuffer");
    if (vtable.BindFramebuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBindFramebuffer"
        );
        exit(1);
    }
    vtable.BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)segl_vtable
        ->GetProcAddress("glBindRenderbuffer");
    if (vtable.BindRenderbuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBindRenderbuffer"
        );
        exit(1);
    }
    vtable.BindTexture = (PFNGLBINDTEXTUREPROC)segl_vtable->GetProcAddress(
        "glBindTexture"
    );
    if (vtable.BindTexture == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBindTexture"
        );
        exit(1);
    }
    vtable.BlendColor = (PFNGLBLENDCOLORPROC)segl_vtable->GetProcAddress(
        "glBlendColor"
    );
    if (vtable.BlendColor == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBlendColor"
        );
        exit(1);
    }
    vtable.BlendEquation = (PFNGLBLENDEQUATIONPROC)segl_vtable->GetProcAddress(
        "glBlendEquation"
    );
    if (vtable.BlendEquation == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBlendEquation"
        );
        exit(1);
    }
    vtable.BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)segl_vtable
        ->GetProcAddress("glBlendEquationSeparate");
    if (vtable.BlendEquationSeparate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBlendEquationSeparate"
        );
        exit(1);
    }
    vtable.BlendFunc = (PFNGLBLENDFUNCPROC)segl_vtable->GetProcAddress(
        "glBlendFunc"
    );
    if (vtable.BlendFunc == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBlendFunc"
        );
        exit(1);
    }
    vtable.BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)segl_vtable
        ->GetProcAddress("glBlendFuncSeparate");
    if (vtable.BlendFuncSeparate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBlendFuncSeparate"
        );
        exit(1);
    }
    vtable.BufferData = (PFNGLBUFFERDATAPROC)segl_vtable->GetProcAddress(
        "glBufferData"
    );
    if (vtable.BufferData == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBufferData"
        );
        exit(1);
    }
    vtable.BufferSubData = (PFNGLBUFFERSUBDATAPROC)segl_vtable->GetProcAddress(
        "glBufferSubData"
    );
    if (vtable.BufferSubData == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glBufferSubData"
        );
        exit(1);
    }
    vtable.CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)segl_vtable
        ->GetProcAddress("glCheckFramebufferStatus");
    if (vtable.CheckFramebufferStatus == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCheckFramebufferStatus"
        );
        exit(1);
    }
    vtable.Clear = (PFNGLCLEARPROC)segl_vtable->GetProcAddress("glClear");
    if (vtable.Clear == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glClear"
        );
        exit(1);
    }
    vtable.ClearColor = (PFNGLCLEARCOLORPROC)segl_vtable->GetProcAddress(
        "glClearColor"
    );
    if (vtable.ClearColor == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glClearColor"
        );
        exit(1);
    }
    vtable.ClearDepthf = (PFNGLCLEARDEPTHFPROC)segl_vtable->GetProcAddress(
        "glClearDepthf"
    );
    if (vtable.ClearDepthf == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glClearDepthf"
        );
        exit(1);
    }
    vtable.ClearStencil = (PFNGLCLEARSTENCILPROC)segl_vtable->GetProcAddress(
        "glClearStencil"
    );
    if (vtable.ClearStencil == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glClearStencil"
        );
        exit(1);
    }
    vtable.ColorMask = (PFNGLCOLORMASKPROC)segl_vtable->GetProcAddress(
        "glColorMask"
    );
    if (vtable.ColorMask == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glColorMask"
        );
        exit(1);
    }
    vtable.CompileShader = (PFNGLCOMPILESHADERPROC)segl_vtable->GetProcAddress(
        "glCompileShader"
    );
    if (vtable.CompileShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCompileShader"
        );
        exit(1);
    }
    vtable.CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)segl_vtable
        ->GetProcAddress("glCompressedTexImage2D");
    if (vtable.CompressedTexImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCompressedTexImage2D"
        );
        exit(1);
    }
    vtable.CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)
        segl_vtable->GetProcAddress("glCompressedTexSubImage2D");
    if (vtable.CompressedTexSubImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCompressedTexSubImage2D"
        );
        exit(1);
    }
    vtable.CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)segl_vtable->GetProcAddress(
        "glCopyTexImage2D"
    );
    if (vtable.CopyTexImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCopyTexImage2D"
        );
        exit(1);
    }
    vtable.CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)segl_vtable
        ->GetProcAddress("glCopyTexSubImage2D");
    if (vtable.CopyTexSubImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCopyTexSubImage2D"
        );
        exit(1);
    }
    vtable.CreateProgram = (PFNGLCREATEPROGRAMPROC)segl_vtable->GetProcAddress(
        "glCreateProgram"
    );
    if (vtable.CreateProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCreateProgram"
        );
        exit(1);
    }
    vtable.CreateShader = (PFNGLCREATESHADERPROC)segl_vtable->GetProcAddress(
        "glCreateShader"
    );
    if (vtable.CreateShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCreateShader"
        );
        exit(1);
    }
    vtable.CullFace = (PFNGLCULLFACEPROC)segl_vtable->GetProcAddress(
        "glCullFace"
    );
    if (vtable.CullFace == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glCullFace"
        );
        exit(1);
    }
    vtable.DeleteBuffers = (PFNGLDELETEBUFFERSPROC)segl_vtable->GetProcAddress(
        "glDeleteBuffers"
    );
    if (vtable.DeleteBuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteBuffers"
        );
        exit(1);
    }
    vtable.DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)segl_vtable
        ->GetProcAddress("glDeleteFramebuffers");
    if (vtable.DeleteFramebuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteFramebuffers"
        );
        exit(1);
    }
    vtable.DeleteProgram = (PFNGLDELETEPROGRAMPROC)segl_vtable->GetProcAddress(
        "glDeleteProgram"
    );
    if (vtable.DeleteProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteProgram"
        );
        exit(1);
    }
    vtable.DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)segl_vtable
        ->GetProcAddress("glDeleteRenderbuffers");
    if (vtable.DeleteRenderbuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteRenderbuffers"
        );
        exit(1);
    }
    vtable.DeleteShader = (PFNGLDELETESHADERPROC)segl_vtable->GetProcAddress(
        "glDeleteShader"
    );
    if (vtable.DeleteShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteShader"
        );
        exit(1);
    }
    vtable.DeleteTextures = (PFNGLDELETETEXTURESPROC)segl_vtable->GetProcAddress(
        "glDeleteTextures"
    );
    if (vtable.DeleteTextures == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDeleteTextures"
        );
        exit(1);
    }
    vtable.DepthFunc = (PFNGLDEPTHFUNCPROC)segl_vtable->GetProcAddress(
        "glDepthFunc"
    );
    if (vtable.DepthFunc == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDepthFunc"
        );
        exit(1);
    }
    vtable.DepthMask = (PFNGLDEPTHMASKPROC)segl_vtable->GetProcAddress(
        "glDepthMask"
    );
    if (vtable.DepthMask == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDepthMask"
        );
        exit(1);
    }
    vtable.DepthRangef = (PFNGLDEPTHRANGEFPROC)segl_vtable->GetProcAddress(
        "glDepthRangef"
    );
    if (vtable.DepthRangef == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDepthRangef"
        );
        exit(1);
    }
    vtable.DetachShader = (PFNGLDETACHSHADERPROC)segl_vtable->GetProcAddress(
        "glDetachShader"
    );
    if (vtable.DetachShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDetachShader"
        );
        exit(1);
    }
    vtable.Disable = (PFNGLDISABLEPROC)segl_vtable->GetProcAddress("glDisable");
    if (vtable.Disable == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDisable"
        );
        exit(1);
    }
    vtable.DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)
        segl_vtable->GetProcAddress("glDisableVertexAttribArray");
    if (vtable.DisableVertexAttribArray == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDisableVertexAttribArray"
        );
        exit(1);
    }
    vtable.DrawArrays = (PFNGLDRAWARRAYSPROC)segl_vtable->GetProcAddress(
        "glDrawArrays"
    );
    if (vtable.DrawArrays == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDrawArrays"
        );
        exit(1);
    }
    vtable.DrawElements = (PFNGLDRAWELEMENTSPROC)segl_vtable->GetProcAddress(
        "glDrawElements"
    );
    if (vtable.DrawElements == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glDrawElements"
        );
        exit(1);
    }
    vtable.Enable = (PFNGLENABLEPROC)segl_vtable->GetProcAddress("glEnable");
    if (vtable.Enable == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glEnable"
        );
        exit(1);
    }
    vtable.EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)
        segl_vtable->GetProcAddress("glEnableVertexAttribArray");
    if (vtable.EnableVertexAttribArray == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glEnableVertexAttribArray"
        );
        exit(1);
    }
    vtable.Finish = (PFNGLFINISHPROC)segl_vtable->GetProcAddress("glFinish");
    if (vtable.Finish == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glFinish"
        );
        exit(1);
    }
    vtable.Flush = (PFNGLFLUSHPROC)segl_vtable->GetProcAddress("glFlush");
    if (vtable.Flush == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glFlush"
        );
        exit(1);
    }
    vtable.FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)
        segl_vtable->GetProcAddress("glFramebufferRenderbuffer");
    if (vtable.FramebufferRenderbuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glFramebufferRenderbuffer"
        );
        exit(1);
    }
    vtable.FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)segl_vtable
        ->GetProcAddress("glFramebufferTexture2D");
    if (vtable.FramebufferTexture2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glFramebufferTexture2D"
        );
        exit(1);
    }
    vtable.FrontFace = (PFNGLFRONTFACEPROC)segl_vtable->GetProcAddress(
        "glFrontFace"
    );
    if (vtable.FrontFace == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glFrontFace"
        );
        exit(1);
    }
    vtable.GenBuffers = (PFNGLGENBUFFERSPROC)segl_vtable->GetProcAddress(
        "glGenBuffers"
    );
    if (vtable.GenBuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGenBuffers"
        );
        exit(1);
    }
    vtable.GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)segl_vtable->GetProcAddress(
        "glGenerateMipmap"
    );
    if (vtable.GenerateMipmap == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGenerateMipmap"
        );
        exit(1);
    }
    vtable.GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)segl_vtable
        ->GetProcAddress("glGenFramebuffers");
    if (vtable.GenFramebuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGenFramebuffers"
        );
        exit(1);
    }
    vtable.GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)segl_vtable
        ->GetProcAddress("glGenRenderbuffers");
    if (vtable.GenRenderbuffers == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGenRenderbuffers"
        );
        exit(1);
    }
    vtable.GenTextures = (PFNGLGENTEXTURESPROC)segl_vtable->GetProcAddress(
        "glGenTextures"
    );
    if (vtable.GenTextures == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGenTextures"
        );
        exit(1);
    }
    vtable.GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)segl_vtable
        ->GetProcAddress("glGetActiveAttrib");
    if (vtable.GetActiveAttrib == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetActiveAttrib"
        );
        exit(1);
    }
    vtable.GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)segl_vtable
        ->GetProcAddress("glGetActiveUniform");
    if (vtable.GetActiveUniform == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetActiveUniform"
        );
        exit(1);
    }
    vtable.GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)segl_vtable
        ->GetProcAddress("glGetAttachedShaders");
    if (vtable.GetAttachedShaders == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetAttachedShaders"
        );
        exit(1);
    }
    vtable.GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)segl_vtable
        ->GetProcAddress("glGetAttribLocation");
    if (vtable.GetAttribLocation == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetAttribLocation"
        );
        exit(1);
    }
    vtable.GetBooleanv = (PFNGLGETBOOLEANVPROC)segl_vtable->GetProcAddress(
        "glGetBooleanv"
    );
    if (vtable.GetBooleanv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetBooleanv"
        );
        exit(1);
    }
    vtable.GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)segl_vtable
        ->GetProcAddress("glGetBufferParameteriv");
    if (vtable.GetBufferParameteriv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetBufferParameteriv"
        );
        exit(1);
    }
    vtable.GetError = (PFNGLGETERRORPROC)segl_vtable->GetProcAddress(
        "glGetError"
    );
    if (vtable.GetError == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetError"
        );
        exit(1);
    }
    vtable.GetFloatv = (PFNGLGETFLOATVPROC)segl_vtable->GetProcAddress(
        "glGetFloatv"
    );
    if (vtable.GetFloatv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetFloatv"
        );
        exit(1);
    }
    vtable.GetFramebufferAttachmentParameteriv = (
        PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
    )segl_vtable->GetProcAddress("glGetFramebufferAttachmentParameteriv");
    if (vtable.GetFramebufferAttachmentParameteriv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetFramebufferAttachmentParameteriv"
        );
        exit(1);
    }
    vtable.GetIntegerv = (PFNGLGETINTEGERVPROC)segl_vtable->GetProcAddress(
        "glGetIntegerv"
    );
    if (vtable.GetIntegerv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetIntegerv"
        );
        exit(1);
    }
    vtable.GetProgramiv = (PFNGLGETPROGRAMIVPROC)segl_vtable->GetProcAddress(
        "glGetProgramiv"
    );
    if (vtable.GetProgramiv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetProgramiv"
        );
        exit(1);
    }
    vtable.GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)segl_vtable
        ->GetProcAddress("glGetProgramInfoLog");
    if (vtable.GetProgramInfoLog == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetProgramInfoLog"
        );
        exit(1);
    }
    vtable.GetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)
        segl_vtable->GetProcAddress("glGetRenderbufferParameteriv");
    if (vtable.GetRenderbufferParameteriv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetRenderbufferParameteriv"
        );
        exit(1);
    }
    vtable.GetShaderiv = (PFNGLGETSHADERIVPROC)segl_vtable->GetProcAddress(
        "glGetShaderiv"
    );
    if (vtable.GetShaderiv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetShaderiv"
        );
        exit(1);
    }
    vtable.GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)segl_vtable
        ->GetProcAddress("glGetShaderInfoLog");
    if (vtable.GetShaderInfoLog == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetShaderInfoLog"
        );
        exit(1);
    }
    vtable.GetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)
        segl_vtable->GetProcAddress("glGetShaderPrecisionFormat");
    if (vtable.GetShaderPrecisionFormat == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetShaderPrecisionFormat"
        );
        exit(1);
    }
    vtable.GetShaderSource = (PFNGLGETSHADERSOURCEPROC)segl_vtable
        ->GetProcAddress("glGetShaderSource");
    if (vtable.GetShaderSource == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetShaderSource"
        );
        exit(1);
    }
    vtable.GetString = (PFNGLGETSTRINGPROC)segl_vtable->GetProcAddress(
        "glGetString"
    );
    if (vtable.GetString == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetString"
        );
        exit(1);
    }
    vtable.GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC)segl_vtable
        ->GetProcAddress("glGetTexParameterfv");
    if (vtable.GetTexParameterfv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetTexParameterfv"
        );
        exit(1);
    }
    vtable.GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC)segl_vtable
        ->GetProcAddress("glGetTexParameteriv");
    if (vtable.GetTexParameteriv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetTexParameteriv"
        );
        exit(1);
    }
    vtable.GetUniformfv = (PFNGLGETUNIFORMFVPROC)segl_vtable->GetProcAddress(
        "glGetUniformfv"
    );
    if (vtable.GetUniformfv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetUniformfv"
        );
        exit(1);
    }
    vtable.GetUniformiv = (PFNGLGETUNIFORMIVPROC)segl_vtable->GetProcAddress(
        "glGetUniformiv"
    );
    if (vtable.GetUniformiv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetUniformiv"
        );
        exit(1);
    }
    vtable.GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)segl_vtable
        ->GetProcAddress("glGetUniformLocation");
    if (vtable.GetUniformLocation == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetUniformLocation"
        );
        exit(1);
    }
    vtable.GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)segl_vtable
        ->GetProcAddress("glGetVertexAttribfv");
    if (vtable.GetVertexAttribfv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetVertexAttribfv"
        );
        exit(1);
    }
    vtable.GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)segl_vtable
        ->GetProcAddress("glGetVertexAttribiv");
    if (vtable.GetVertexAttribiv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetVertexAttribiv"
        );
        exit(1);
    }
    vtable.GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)
        segl_vtable->GetProcAddress("glGetVertexAttribPointerv");
    if (vtable.GetVertexAttribPointerv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glGetVertexAttribPointerv"
        );
        exit(1);
    }
    vtable.Hint = (PFNGLHINTPROC)segl_vtable->GetProcAddress("glHint");
    if (vtable.Hint == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glHint"
        );
        exit(1);
    }
    vtable.IsBuffer = (PFNGLISBUFFERPROC)segl_vtable->GetProcAddress(
        "glIsBuffer"
    );
    if (vtable.IsBuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsBuffer"
        );
        exit(1);
    }
    vtable.IsEnabled = (PFNGLISENABLEDPROC)segl_vtable->GetProcAddress(
        "glIsEnabled"
    );
    if (vtable.IsEnabled == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsEnabled"
        );
        exit(1);
    }
    vtable.IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)segl_vtable->GetProcAddress(
        "glIsFramebuffer"
    );
    if (vtable.IsFramebuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsFramebuffer"
        );
        exit(1);
    }
    vtable.IsProgram = (PFNGLISPROGRAMPROC)segl_vtable->GetProcAddress(
        "glIsProgram"
    );
    if (vtable.IsProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsProgram"
        );
        exit(1);
    }
    vtable.IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)segl_vtable->GetProcAddress(
        "glIsRenderbuffer"
    );
    if (vtable.IsRenderbuffer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsRenderbuffer"
        );
        exit(1);
    }
    vtable.IsShader = (PFNGLISSHADERPROC)segl_vtable->GetProcAddress(
        "glIsShader"
    );
    if (vtable.IsShader == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsShader"
        );
        exit(1);
    }
    vtable.IsTexture = (PFNGLISTEXTUREPROC)segl_vtable->GetProcAddress(
        "glIsTexture"
    );
    if (vtable.IsTexture == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glIsTexture"
        );
        exit(1);
    }
    vtable.LineWidth = (PFNGLLINEWIDTHPROC)segl_vtable->GetProcAddress(
        "glLineWidth"
    );
    if (vtable.LineWidth == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glLineWidth"
        );
        exit(1);
    }
    vtable.LinkProgram = (PFNGLLINKPROGRAMPROC)segl_vtable->GetProcAddress(
        "glLinkProgram"
    );
    if (vtable.LinkProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glLinkProgram"
        );
        exit(1);
    }
    vtable.PixelStorei = (PFNGLPIXELSTOREIPROC)segl_vtable->GetProcAddress(
        "glPixelStorei"
    );
    if (vtable.PixelStorei == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glPixelStorei"
        );
        exit(1);
    }
    vtable.PolygonOffset = (PFNGLPOLYGONOFFSETPROC)segl_vtable->GetProcAddress(
        "glPolygonOffset"
    );
    if (vtable.PolygonOffset == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glPolygonOffset"
        );
        exit(1);
    }
    vtable.ReadPixels = (PFNGLREADPIXELSPROC)segl_vtable->GetProcAddress(
        "glReadPixels"
    );
    if (vtable.ReadPixels == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glReadPixels"
        );
        exit(1);
    }
    vtable.ReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)segl_vtable
        ->GetProcAddress("glReleaseShaderCompiler");
    if (vtable.ReleaseShaderCompiler == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glReleaseShaderCompiler"
        );
        exit(1);
    }
    vtable.RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)segl_vtable
        ->GetProcAddress("glRenderbufferStorage");
    if (vtable.RenderbufferStorage == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glRenderbufferStorage"
        );
        exit(1);
    }
    vtable.SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)segl_vtable->GetProcAddress(
        "glSampleCoverage"
    );
    if (vtable.SampleCoverage == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glSampleCoverage"
        );
        exit(1);
    }
    vtable.Scissor = (PFNGLSCISSORPROC)segl_vtable->GetProcAddress("glScissor");
    if (vtable.Scissor == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glScissor"
        );
        exit(1);
    }
    vtable.ShaderBinary = (PFNGLSHADERBINARYPROC)segl_vtable->GetProcAddress(
        "glShaderBinary"
    );
    if (vtable.ShaderBinary == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glShaderBinary"
        );
        exit(1);
    }
    vtable.ShaderSource = (PFNGLSHADERSOURCEPROC)segl_vtable->GetProcAddress(
        "glShaderSource"
    );
    if (vtable.ShaderSource == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glShaderSource"
        );
        exit(1);
    }
    vtable.StencilFunc = (PFNGLSTENCILFUNCPROC)segl_vtable->GetProcAddress(
        "glStencilFunc"
    );
    if (vtable.StencilFunc == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilFunc"
        );
        exit(1);
    }
    vtable.StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)segl_vtable
        ->GetProcAddress("glStencilFuncSeparate");
    if (vtable.StencilFuncSeparate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilFuncSeparate"
        );
        exit(1);
    }
    vtable.StencilMask = (PFNGLSTENCILMASKPROC)segl_vtable->GetProcAddress(
        "glStencilMask"
    );
    if (vtable.StencilMask == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilMask"
        );
        exit(1);
    }
    vtable.StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)segl_vtable
        ->GetProcAddress("glStencilMaskSeparate");
    if (vtable.StencilMaskSeparate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilMaskSeparate"
        );
        exit(1);
    }
    vtable.StencilOp = (PFNGLSTENCILOPPROC)segl_vtable->GetProcAddress(
        "glStencilOp"
    );
    if (vtable.StencilOp == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilOp"
        );
        exit(1);
    }
    vtable.StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)segl_vtable
        ->GetProcAddress("glStencilOpSeparate");
    if (vtable.StencilOpSeparate == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glStencilOpSeparate"
        );
        exit(1);
    }
    vtable.TexImage2D = (PFNGLTEXIMAGE2DPROC)segl_vtable->GetProcAddress(
        "glTexImage2D"
    );
    if (vtable.TexImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexImage2D"
        );
        exit(1);
    }
    vtable.TexParameterf = (PFNGLTEXPARAMETERFPROC)segl_vtable->GetProcAddress(
        "glTexParameterf"
    );
    if (vtable.TexParameterf == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexParameterf"
        );
        exit(1);
    }
    vtable.TexParameterfv = (PFNGLTEXPARAMETERFVPROC)segl_vtable->GetProcAddress(
        "glTexParameterfv"
    );
    if (vtable.TexParameterfv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexParameterfv"
        );
        exit(1);
    }
    vtable.TexParameteri = (PFNGLTEXPARAMETERIPROC)segl_vtable->GetProcAddress(
        "glTexParameteri"
    );
    if (vtable.TexParameteri == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexParameteri"
        );
        exit(1);
    }
    vtable.TexParameteriv = (PFNGLTEXPARAMETERIVPROC)segl_vtable->GetProcAddress(
        "glTexParameteriv"
    );
    if (vtable.TexParameteriv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexParameteriv"
        );
        exit(1);
    }
    vtable.TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)segl_vtable->GetProcAddress(
        "glTexSubImage2D"
    );
    if (vtable.TexSubImage2D == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glTexSubImage2D"
        );
        exit(1);
    }
    vtable.Uniform1f = (PFNGLUNIFORM1FPROC)segl_vtable->GetProcAddress(
        "glUniform1f"
    );
    if (vtable.Uniform1f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform1f"
        );
        exit(1);
    }
    vtable.Uniform1fv = (PFNGLUNIFORM1FVPROC)segl_vtable->GetProcAddress(
        "glUniform1fv"
    );
    if (vtable.Uniform1fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform1fv"
        );
        exit(1);
    }
    vtable.Uniform1i = (PFNGLUNIFORM1IPROC)segl_vtable->GetProcAddress(
        "glUniform1i"
    );
    if (vtable.Uniform1i == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform1i"
        );
        exit(1);
    }
    vtable.Uniform1iv = (PFNGLUNIFORM1IVPROC)segl_vtable->GetProcAddress(
        "glUniform1iv"
    );
    if (vtable.Uniform1iv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform1iv"
        );
        exit(1);
    }
    vtable.Uniform2f = (PFNGLUNIFORM2FPROC)segl_vtable->GetProcAddress(
        "glUniform2f"
    );
    if (vtable.Uniform2f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform2f"
        );
        exit(1);
    }
    vtable.Uniform2fv = (PFNGLUNIFORM2FVPROC)segl_vtable->GetProcAddress(
        "glUniform2fv"
    );
    if (vtable.Uniform2fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform2fv"
        );
        exit(1);
    }
    vtable.Uniform2i = (PFNGLUNIFORM2IPROC)segl_vtable->GetProcAddress(
        "glUniform2i"
    );
    if (vtable.Uniform2i == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform2i"
        );
        exit(1);
    }
    vtable.Uniform2iv = (PFNGLUNIFORM2IVPROC)segl_vtable->GetProcAddress(
        "glUniform2iv"
    );
    if (vtable.Uniform2iv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform2iv"
        );
        exit(1);
    }
    vtable.Uniform3f = (PFNGLUNIFORM3FPROC)segl_vtable->GetProcAddress(
        "glUniform3f"
    );
    if (vtable.Uniform3f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform3f"
        );
        exit(1);
    }
    vtable.Uniform3fv = (PFNGLUNIFORM3FVPROC)segl_vtable->GetProcAddress(
        "glUniform3fv"
    );
    if (vtable.Uniform3fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform3fv"
        );
        exit(1);
    }
    vtable.Uniform3i = (PFNGLUNIFORM3IPROC)segl_vtable->GetProcAddress(
        "glUniform3i"
    );
    if (vtable.Uniform3i == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform3i"
        );
        exit(1);
    }
    vtable.Uniform3iv = (PFNGLUNIFORM3IVPROC)segl_vtable->GetProcAddress(
        "glUniform3iv"
    );
    if (vtable.Uniform3iv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform3iv"
        );
        exit(1);
    }
    vtable.Uniform4f = (PFNGLUNIFORM4FPROC)segl_vtable->GetProcAddress(
        "glUniform4f"
    );
    if (vtable.Uniform4f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform4f"
        );
        exit(1);
    }
    vtable.Uniform4fv = (PFNGLUNIFORM4FVPROC)segl_vtable->GetProcAddress(
        "glUniform4fv"
    );
    if (vtable.Uniform4fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform4fv"
        );
        exit(1);
    }
    vtable.Uniform4i = (PFNGLUNIFORM4IPROC)segl_vtable->GetProcAddress(
        "glUniform4i"
    );
    if (vtable.Uniform4i == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform4i"
        );
        exit(1);
    }
    vtable.Uniform4iv = (PFNGLUNIFORM4IVPROC)segl_vtable->GetProcAddress(
        "glUniform4iv"
    );
    if (vtable.Uniform4iv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniform4iv"
        );
        exit(1);
    }
    vtable.UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)segl_vtable
        ->GetProcAddress("glUniformMatrix2fv");
    if (vtable.UniformMatrix2fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniformMatrix2fv"
        );
        exit(1);
    }
    vtable.UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)segl_vtable
        ->GetProcAddress("glUniformMatrix3fv");
    if (vtable.UniformMatrix3fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniformMatrix3fv"
        );
        exit(1);
    }
    vtable.UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)segl_vtable
        ->GetProcAddress("glUniformMatrix4fv");
    if (vtable.UniformMatrix4fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUniformMatrix4fv"
        );
        exit(1);
    }
    vtable.UseProgram = (PFNGLUSEPROGRAMPROC)segl_vtable->GetProcAddress(
        "glUseProgram"
    );
    if (vtable.UseProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glUseProgram"
        );
        exit(1);
    }
    vtable.ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)segl_vtable
        ->GetProcAddress("glValidateProgram");
    if (vtable.ValidateProgram == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glValidateProgram"
        );
        exit(1);
    }
    vtable.VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)segl_vtable->GetProcAddress(
        "glVertexAttrib1f"
    );
    if (vtable.VertexAttrib1f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib1f"
        );
        exit(1);
    }
    vtable.VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)segl_vtable
        ->GetProcAddress("glVertexAttrib1fv");
    if (vtable.VertexAttrib1fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib1fv"
        );
        exit(1);
    }
    vtable.VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)segl_vtable->GetProcAddress(
        "glVertexAttrib2f"
    );
    if (vtable.VertexAttrib2f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib2f"
        );
        exit(1);
    }
    vtable.VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)segl_vtable
        ->GetProcAddress("glVertexAttrib2fv");
    if (vtable.VertexAttrib2fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib2fv"
        );
        exit(1);
    }
    vtable.VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)segl_vtable->GetProcAddress(
        "glVertexAttrib3f"
    );
    if (vtable.VertexAttrib3f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib3f"
        );
        exit(1);
    }
    vtable.VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)segl_vtable
        ->GetProcAddress("glVertexAttrib3fv");
    if (vtable.VertexAttrib3fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib3fv"
        );
        exit(1);
    }
    vtable.VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)segl_vtable->GetProcAddress(
        "glVertexAttrib4f"
    );
    if (vtable.VertexAttrib4f == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib4f"
        );
        exit(1);
    }
    vtable.VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)segl_vtable
        ->GetProcAddress("glVertexAttrib4fv");
    if (vtable.VertexAttrib4fv == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttrib4fv"
        );
        exit(1);
    }
    vtable.VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)segl_vtable
        ->GetProcAddress("glVertexAttribPointer");
    if (vtable.VertexAttribPointer == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glVertexAttribPointer"
        );
        exit(1);
    }
    vtable.Viewport = (PFNGLVIEWPORTPROC)segl_vtable->GetProcAddress(
        "glViewport"
    );
    if (vtable.Viewport == NULL) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            SEGL_ANDROID_LOG_ID,
            "failed to load glViewport"
        );
        exit(1);
    }

    return vtable;
}

static SEglVtable egl;
static SEglCtx egl_ctx;
static SGlVtable gl;

static void handle_cmd(AndroidApp *app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_INIT_WINDOW"
            );
            if (egl_ctx.display != EGL_NO_DISPLAY) {
                break;
            }
            egl_ctx = segl_ctx_load(app, &egl);
            break;
        case APP_CMD_TERM_WINDOW:
            __android_log_print(
                ANDROID_LOG_INFO,
                SEGL_ANDROID_LOG_ID,
                "APP_CMD_TERM_WINDOW"
            );
            if (egl_ctx.display == EGL_NO_DISPLAY) {
                break;
            }
            segl_ctx_unload(&egl_ctx, &egl);
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

static int32_t handle_input(AndroidApp *app, AInputEvent *event) {
    return 0;
}

static inline int64_t time_since(TimeSpec end, TimeSpec start) {
    int64_t seconds = (int64_t)end.tv_sec - (int64_t)start.tv_sec;
    int64_t sec_diff = seconds * 1000L * 1000L * 1000L;
    int64_t nsec_diff = (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;
    return sec_diff + nsec_diff;
}

void android_main(AndroidApp *app) {
    __android_log_print(ANDROID_LOG_INFO, SEGL_ANDROID_LOG_ID, "android_main");
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    __android_log_print(
        ANDROID_LOG_INFO,
        SEGL_ANDROID_LOG_ID,
        "egl_vtable_load"
    );
    egl = segl_vtable_load();

    __android_log_print(ANDROID_LOG_INFO, SEGL_ANDROID_LOG_ID, "gl_vtable_load");
    gl = sgl_vtable_load(&egl);

    egl_ctx = (SEglCtx){
        .display = EGL_NO_DISPLAY,
        .context = EGL_NO_CONTEXT,
        .surface = EGL_NO_SURFACE,
    };

    float red = 0.66f;
    bool red_flip = false;
    float green = 0.33f;
    bool green_flip = false;
    float blue = 0.0f;
    bool blue_flip = false;

    int64_t elapsed = 0;
    TimeSpec last;
    clock_gettime(CLOCK_MONOTONIC, &last);
    for (;;) {
        TimeSpec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed += time_since(now, last);
        last = now;

        while (elapsed >= TIMESTEP) {
            red += 0.005f;
            green += 0.006f;
            blue += 0.007;
            if (red >= 1.0f) {
                red -= 1.0f;
                red_flip = !red_flip;
            }
            if (green >= 1.0f) {
                green -= 1.0f;
                green_flip = !green_flip;
            }
            if (blue >= 1.0f) {
                blue -= 1.0f;
                blue_flip = !blue_flip;
            }
            elapsed -= TIMESTEP;
        }

        int events;
        AndroidPollSource *source;
        while (ALooper_pollOnce(0, 0, &events, (void **)&source) >= 0) {
            if (source != NULL) {
                source->process(app, source);
            }
        }

        if (egl_ctx.display == EGL_NO_DISPLAY) {
            const struct timespec duration = { .tv_nsec = TIMESTEP };
            nanosleep(&duration, NULL);
            continue;
        }

        int width = ANativeWindow_getWidth(app->window);
        int height = ANativeWindow_getHeight(app->window);

        gl.Viewport(0, 0, width, height);
        gl.ClearColor(
            red_flip ? 1.0f - red : red,
            green_flip ? 1.0f - green : green,
            blue_flip ? 1.0f - blue : blue,
            1.0f
        );
        gl.Clear(GL_COLOR_BUFFER_BIT);

        egl.SwapBuffers(egl_ctx.display, egl_ctx.surface);
    }
}
