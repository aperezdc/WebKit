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
#include "WPEBufferVulkan.h"

#if USE(VULKAN)
#include <wtf/glib/WTFGType.h>

// The Volk header can leave device functions undefined to prevent accidental
// usage. Instead, use the per-device functions table to avoid the dispatch
// overhead (up to 7%), see https://github.com/zeux/volk#optimizing-device-calls
#define VOLK_NO_DEVICE_PROTOTYPES
#include <volk.h>

/**
 * WPEBufferVulkan:
 *
 * A #WPEBuffer backend by a Vulkan image in device memory.
 */
struct _WPEBufferVulkanPrivate {
    VolkDeviceTable deviceTable;
    VkDevice device;
    VkImage image;
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

#endif // USE(VULKAN)
