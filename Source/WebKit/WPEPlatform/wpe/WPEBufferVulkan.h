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

#ifndef WPEBufferVulkan_h
#define WPEBufferVulkan_h

#if !defined(__WPE_PLATFORM_H_INSIDE__) && !defined(BUILDING_WEBKIT)
#error "Only <wpe/wpe-platform.h> can be included directly."
#endif

#ifndef __GI_SCANNER__

#include <wpe/WPEDefines.h>
#include <wpe/WPEBuffer.h>

G_BEGIN_DECLS

typedef struct VkDevice_T* VkDevice;
typedef struct VkImage_T* VkImage;

#define WPE_TYPE_BUFFER_VULKAN (wpe_buffer_vulkan_get_type())
WPE_API G_DECLARE_FINAL_TYPE (WPEBufferVulkan, wpe_buffer_vulkan, WPE, BUFFER_VULKAN, WPEBuffer)

WPE_API WPEBufferVulkan *wpe_buffer_vulkan_new        (WPEDisplay      *display,
                                                       VkDevice         device,
                                                       VkImage          image);
WPE_API VkDevice         wpe_buffer_vulkan_get_device (WPEBufferVulkan *buffer);
WPE_API VkImage          wpe_buffer_vulkan_get_image  (WPEBufferVulkan *buffer);
WPE_API guint32          wpe_buffer_vulkan_get_format (WPEBufferVulkan *buffer);

G_END_DECLS

#endif /* !__GI_SCANNER__ */

#endif /* WPEBufferVulkan_h */
