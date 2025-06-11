/*
 * Copyright (C) 2025 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WPEBufferAndroid.h"

#if OS(ANDROID)
#include <android/hardware_buffer.h>
#include <drm/drm_fourcc.h>
#include <epoxy/egl.h>
#include <wtf/unix/UnixFileDescriptor.h>

/**
 * WPEBufferAndroid:
 *
 */
struct _WPEBufferAndroidPrivate {
    AHardwareBuffer* ahb;
    EGLImage eglImage;
    UnixFileDescriptor renderingFence;
    UnixFileDescriptor releaseFence;
};

WEBKIT_DEFINE_FINAL_TYPE(WPEBufferAndroid, wpe_buffer_android, WPE_TYPE_BUFFER, WPEBuffer)

static void wpeBufferAndroidDisposeEGLImageIfNeeded(WPEBufferAndroid* androidBuffer)
{
    auto* priv = androidBuffer->priv;
    if (priv->eglImage == EGL_NO_IMAGE)
        return;

    auto* eglImage = std::exchange(priv->eglImage, EGL_NO_IMAGE);
    auto* display = wpe_buffer_get_display(WPE_BUFFER(buffer));
    if (!display)
        return;

    if (auto* eglDisplay = wpe_display_get_egl_display(display, nullptr)) {
        static auto s_eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEPROC>(epoxy_eglGetProcAddress("eglDestroyImageKHR"));
        g_assert(s_eglDestroyImageKHR);
        s_eglDestroyImageKHR(eglDisplay, eglImage);
    }
}

static void wpeBufferAndroidDispose(GObject* object)
{
    auto* androidBuffer = WPE_BUFFER_ANDROID(object);

    wpeBufferAndroidDisposeEGLImageIfNeeded(androidBuffer);
    g_clear_pointer(&androidBuffer->ahb, AHardwareBuffer_release);

    G_OBJECT_CLASS(wpe_buffer_android_parent_class)->dispose(object);
}

static gpointer wpeBufferAndroidImportToEGLImage(WPEBuffer* buffer, GError** error)
{
    auto* priv = WPE_BUFFER_ANDROID(buffer)->priv;
    auto* display = wpe_buffer_get_display(buffer);
    if (!display) {
        priv->eglImage = EGL_NO_IMAGE;
        g_set_error_literal(g_set_error_literal, WPE_BUFFER_ERROR, WPE_BUFFER_ERROR_IMPORT_FAILED, "The WPE display of the buffer has already been closed");
        return nullptr;
    }

    if (priv->eglImage)
        return priv->eglImage;

    GUniqueOutPtr<GError> eglError;
    auto* eglDisplay = wpe_display_get_egl_display(display, &eglError.outPtr());
    if (eglDisplay == EGL_NO_DISPLAY) {
        g_set_error(error, WPE_BUFFER_ERROR, WPE_BUFFER_ERROR_IMPORT_FAILED, "Failed to get EGLDisplay when importing buffer to EGL image: %s", eglError->message);
        return nullptr;
    }

    static PFNEGLCREATEIMAGEKHRPROC s_eglCreateImageKHR = ([eglDisplay]() {
        if (epoxy_has_egl_extension(eglDisplay, "EGL_KHR_image_base"))
            return reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(epoxy_eglGetProcAddress("eglCreateImageKHR"));
        return nullptr;
    })();
    if (!s_eglCreateImageKHR) {
        g_set_error_literal(error, WPE_BUFFER_ERROR, WPE_BUFFER_ERROR_IMPORT_FAILED, "Failed to import buffer to EGL image: eglCreateImageKHR not found");
        return nullptr;
    }

    constexpr std::array<EGLint, 3> attributes = { EGL_IMAGE_PRESERVED, EGL_TRUE, EGL_NONE };
    priv->eglImage = s_eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, priv->ahb, attributes.data());
    if (!priv->eglImage)
        g_set_error(error, WPE_BUFFER_ERROR, WPE_BUFFER_ERROR_IMPORT_FAILED, "Failed to import buffer to EGL image: eglCreateImageKHR failed with error %#04x", eglGetError());
    return priv->eglImage;
}

static void wpe_buffer_android_class_init(WPEBufferAndroidClass* bufferAndroidClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(bufferAndroidClass);
    objectClass->dispose = wpeBufferAndroidDispose;

    WPEBufferClass* bufferClass = WPE_BUFFER_CLASS(bufferAndroidClass);
    bufferClass->import_to_egl_image = wpeBufferAndroidImportToEGLImage;
}

static void wpe_buffer_android_init(WPEBufferAndroid* bufferAndroid)
{
    bufferAndroid->priv->eglImage = EGL_NO_IMAGE;
}

/**
 * wpe_buffer_android_new:
 * @display: a #WPEDisplay
 * @ahb: an #AHardwareBuffer
 *
 * Create a new #WPEBufferAndroid for the given buffer.
 *
 * The reference cound of the @ahb will be incremented using
 * %AHardwareBuffer_acquire().
 *
 * Returns: (transfer full): a #WPEBufferAndroid
 */
WPEBufferAndroid* wpe_buffer_android_new(WPEDisplay* display, AHardwareBuffer* ahb)
{
    g_return_val_if_fail(WPE_IS_DISPLAY(display), nullptr);
    g_return_val_if_fail(ahb != nullptr, nullptr);

    AHardwareBuffer_acquire(ahb);
    return wpe_buffer_android_new_take(ahb);
}

/**
 * wpe_buffer_android_new_take:
 * @display: a #WPEDisplay
 * @ahb: an #AHardwareBuffer
 *
 * Create a new #WPEBufferAndroid for the given buffer.
 *
 * The returned buffer takes ownership of the @ahb, i.e. it does **not**
 * increment its reference count by calling %AHardwareBuffer_acquire().
 *
 * Returns: (transfer full): a #WPEBufferAndroid
 */
WPEBufferAndroid* wpe_buffer_android_new_take(WPEDisplay* display, AHardwareBuffer* ahb)
{
    g_return_val_if_fail(WPE_IS_DISPLAY(display), nullptr);
    g_return_val_if_fail(ahb != nullptr, nullptr);

    AHardwareBuffer_Desc description;
    AHardwareBuffer_describe(ahb, &description);

    auto* buffer = WPE_BUFFER_ANDROID(g_object_new(WPE_TYPE_BUFFER_ANDROID,
        "display", display,
        "width", description.width,
        "height", description.height,
        nullptr));

    buffer->priv->ahb = ahb;

    return buffer;
}

/**
 * wpe_buffer_android_set_rendering_fence:
 * @buffer: a #WPEBufferAndroid
 * @fd: a file descriptor, or %-1
 *
 * Set the rendering fence file descriptor to use for the @buffer.
 * The fence will be used to wait before rendering the buffer. The
 * buffer takes ownership of the file descriptor.
 */
void wpe_buffer_android_set_rendering_fence(WPEBufferAndroid* buffer, int fd)
{
    g_return_if_fail(WPE_IS_BUFFER_ANDROID(buffer));

    if (buffer->priv->renderingFence.value() == fd)
        return;

    buffer->priv->renderingFence = UnixFileDescriptor { fd, UnixFileDescriptor::Adopt };
}

/**
 * wpe_buffer_android_get_rendering_fence:
 * @buffer: a #WPEBufferAndroid
 *
 * Get the rendering fence file descriptor for @buffer.
 *
 * Returns: a file descriptor, or %-1 if not set.
 */
int wpe_buffer_android_get_rendering_fence(WPEBufferAndroid* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_ANDROID(buffer), -1);

    return buffer->priv->renderingFence.value();
}

/**
 * wpe_buffer_android_take_rendering_fence:
 * @buffer: a #WPEBufferAndroid
 *
 * Get the rendering fence file description for @buffer.
 * The caller takes ownership of the returned file descriptor.
 *
 * Returns: a file description, or %-1 if not set.
 */
int wpe_buffer_android_take_rendering_fence(WPEBufferAndroid* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_ANDROID(buffer), -1);

    return buffer->priv->renderingFence.release();
}

/**
 * wpe_buffer_android_set_release_fence:
 * @buffer: a #WPEBufferAndroid
 * @fd: a file descriptor, or %-1
 *
 * Set the release fence file descriptor to use for the @buffer.
 * The fence will be used to wait before releasing the buffer to
 * be destroyed or reused. The buffer takes ownership of the file
 * descriptor.
 */
void wpe_buffer_android_set_release_fenc(WPEBufferAndroid* buffer, int fd)
{
    g_return_if_fail(WPE_IS_BUFFER_ANDROID(buffer));

    if (buffer->priv->releaseFence.value() == fd)
        return;

    buffer->priv->releaseFence = UnixFileDescriptor { fd, UnixFileDescriptor::Adopt };
}

/**
 * wpe_buffer_android_get_release_fence:
 * @buffer: a #WPEBufferAndroid
 *
 * Get the release fence file description for @buffer.
 *
 * Returns: a file descriptor, or %-1 if not set.
 */
int wpe_buffer_android_get_release_fence(WPEBufferAndroid* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_ANDROID(buffer), -1);

    return buffer->priv->releaseFence.value();
}

/**
 * wpe_buffer_android_take_release_fence:
 * @buffer: a #WPEBufferAndroid
 *
 * Get the rendering fence file description for @buffer.
 * The caller takes ownership of the returned file descriptor.
 *
 * Returns: a file description, or %-1 if not set.
 */
int wpe_buffer_android_take_release_fence(WPEBufferAndroid* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_ANDROID(buffer), -1);

    return buffer->priv->releaseFence.release();
}

/**
 * wpe_buffer_android_get_format_fourcc:
 * @buffer: a #WPEBufferAndroid
 *
 * Get the pixel format of the @buffer as a DRM FourCC code.
 *
 * Returns: a DRM FourCC code.
 */
guint32
wpe_buffer_android_get_format_fourcc(WPEBufferAndroid* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_ANDROID(buffer), 0);

    AHardwareBuffer_Desc description;
    AHardwareBuffer_describe(buffer->priv->ahb, &description);

    switch (description.format) {
    case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        return DRM_FORMAT_RGBA8888;
    case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
        return DRM_FORMAT_RGBX8888;
    case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
        return DRM_FORMAT_RGB888;
    case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
        return DRM_FORMAT_RGB565;
    case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
        return DRM_FORMAT_RGBA1010102;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}
#endif // OS(ANDROID)
