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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "VulkanGraphicsBuffer.h"

#if USE(VULKAN)
#include "FourCC.h"
#include "IntSize.h"
#include "Logging.h"
#include "VulkanUtilities.h"

namespace WebCore {
namespace Vulkan {

Result<GraphicsBuffer> GraphicsBuffer::create(const IntSize& size, const FourCC& fourcc, const std::span<const uint64_t> modifiers)
{
    if (size.width() <= 0 || size.height() <= 0) {
        RELEASE_LOG_DEBUG(Vulkan, "Vulkan::GraphicsBuffer::create: cannot use size %dx%d", size.width(), size.height());
        return makeUnexpected(VK_INCOMPLETE);
    }

    auto format = toVulkanFormat(fourcc);
    if (format == VK_FORMAT_UNDEFINED) {
        RELEASE_LOG_DEBUG(Vulkan, "Vulkan::GraphicsBuffer::create: cannot map '%s' to a VkFormat", fourcc.string().data());
        return makeUnexpected(VK_INCOMPLETE);
    }

    ImageCreateInfo createInfo;
    createInfo->format = format;
    createInfo->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo->imageType = VK_IMAGE_TYPE_2D;
    createInfo->mipLevels = 1;
    createInfo->arrayLayers = 1;
    createInfo->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo->tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo->samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo->extent = {
        .width = static_cast<uint32_t>(size.width()),
        .height = static_cast<uint32_t>(size.height()),
        .depth = 1,
    };

    // Needed for interop with OpenGL implementations that support ARB_texture_view,
    // OES_texture_view, EXT_texture_view, or are support OpenGL 4.3+.
    createInfo->flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

    static constexpr auto externalHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    // TODO: Use VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID on Android.
    auto externalMemoryInfo [[maybe_unused]] = createInfo.next<ExternalMemoryImageCreateInfo>(externalHandleType);

    auto& device = Device::sharedDevice();
    auto image = device.createImage(createInfo);
    if (!image)
        return makeUnexpected(image.error());

    auto memRequirements = MemoryRequirements();
    auto memDedicatedRequirements = memRequirements.next<MemoryDedicatedRequirements>();
    image->fillMemoryRequirements(memRequirements);

    auto deviceMemory = device.allocateExternalMemory(memRequirements, *image, externalHandleType);
    if (!deviceMemory)
        return makeUnexpected(deviceMemory.error());

    if (auto result = device.bindImageMemory(*image, *deviceMemory); result != VK_SUCCESS)
        return makeUnexpected(result);

    return GraphicsBuffer(WTF::move(*image), WTF::move(*deviceMemory), format, size, memRequirements->size, memDedicatedRequirements->requiresDedicatedAllocation);
}

} // namespace Vulkan
} // namespace WebCore

#endif // USE(VULKAN)
