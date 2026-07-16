/*
 * Copyright (C) 2026 Igalia S.L.
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

#if USE(VULKAN)

// The Volk header can leave device functions undefined to prevent accidental
// usage. Instead, use the per-device functions table to avoid the dispatch
// overhead (up to 7%), see https://github.com/zeux/volk#optimizing-device-calls
//
// Note that the Volk header needs to be included first, to let it define
// needed preprocessor macros. This way they are already defined correctly
// before WPEBufferVulkan.h gets to include vulkan_core.h.
#define VOLK_NO_DEVICE_PROTOTYPES
#include <volk.h>

#include "WPEBufferVulkan.h"
#include <drm/drm_fourcc.h>
#include <wtf/glib/WTFGType.h>

/**
 * WPEBufferVulkan:
 *
 * A #WPEBuffer backend by a Vulkan image in device memory.
 */
struct _WPEBufferVulkanPrivate {
    VolkDeviceTable deviceTable;
    VkDevice device;
    VkImage image;
    VkFormat format;
    VkDeviceMemory memory;
};
WEBKIT_DEFINE_FINAL_TYPE(WPEBufferVulkan, wpe_buffer_vulkan, WPE_TYPE_BUFFER, WPEBuffer)

static void wpeBufferVulkanDispose(GObject* object)
{
    auto* priv = WPE_BUFFER_VULKAN(object)->priv;

    if (priv->image != VK_NULL_HANDLE) {
        priv->deviceTable.vkDestroyImage(priv->device, priv->image, NULL);
        priv->image = VK_NULL_HANDLE;
    }

    G_OBJECT_CLASS(wpe_buffer_vulkan_parent_class)->dispose(object);
}

static void wpe_buffer_vulkan_class_init(WPEBufferVulkanClass* bufferVulkanClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(bufferVulkanClass);
    objectClass->dispose = wpeBufferVulkanDispose;
}

WPEBufferVulkan* wpe_buffer_vulkan_new(WPEDisplay* display, VkDevice device, VkImage image, VkFormat format, gint width, gint height, VkDeviceMemory memory)
{
    g_return_val_if_fail(WPE_IS_DISPLAY(display), nullptr);
    g_return_val_if_fail(device == VK_NULL_HANDLE, nullptr);
    g_return_val_if_fail(image == VK_NULL_HANDLE, nullptr);
    g_return_val_if_fail(format == VK_FORMAT_UNDEFINED, nullptr);
    g_return_val_if_fail(memory == VK_NULL_HANDLE, nullptr);

    auto* buffer = WPE_BUFFER_VULKAN(g_object_new(WPE_TYPE_BUFFER_VULKAN,
        "display", display,
        "width", width,
        "height", height,
        nullptr));

    buffer->priv->device = device;
    buffer->priv->image = image;
    buffer->priv->format = format;
    buffer->priv->memory = memory;

    volkLoadDeviceTable(&buffer->priv->deviceTable, buffer->priv->device);

    return buffer;
}

VkDevice wpe_buffer_vulkan_get_device(WPEBufferVulkan* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_VULKAN(buffer), VK_NULL_HANDLE);

    return buffer->priv->device;
}

VkImage wpe_buffer_vulkan_get_image(WPEBufferVulkan* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_VULKAN(buffer), VK_NULL_HANDLE);

    return buffer->priv->image;
}

guint32 wpe_buffer_vulkan_get_format(WPEBufferVulkan* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_VULKAN(buffer), 0);

    // Keep in sync with Vulkan::toVulkanFormat() in WebCore's VulkanUtilities.cpp
    switch (buffer->priv->format) {
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        return DRM_FORMAT_RGBA8888;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return DRM_FORMAT_ABGR8888;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return DRM_FORMAT_ARGB8888;
    case VK_FORMAT_R8G8B8_UNORM:
        return DRM_FORMAT_BGR888;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        return DRM_FORMAT_RGB565;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return DRM_FORMAT_ABGR16161616F;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return DRM_FORMAT_ABGR2101010;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

VkDeviceMemory wpe_buffer_vulkan_get_memory(WPEBufferVulkan* buffer)
{
    g_return_val_if_fail(WPE_IS_BUFFER_VULKAN(buffer), VK_NULL_HANDLE);

    return buffer->priv->memory;
}

#endif // USE(VULKAN)
