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

#pragma once

#if USE(VULKAN)
#include "VulkanTypes.h"
#include <wtf/ArgumentCoder.h>
#include <wtf/unix/UnixFileDescriptor.h>

namespace WebCore {

class VulkanGraphicsBuffer
{
    WTF_MAKE_NONCOPYABLE(VulkanGraphicsBuffer);

public:
    [[nodiscard]] static Vulkan::Result<VulkanGraphicsBuffer> create(const IntSize&, const FourCC&, const std::span<const uint64_t> modifiers);

    VulkanGraphicsBuffer(VulkanGraphicsBuffer&&) = default;
    VulkanGraphicsBuffer& operator=(VulkanGraphicsBuffer&&) = default;

    [[nodiscard]] WTF::UnixFileDescriptor fileDescriptor() const;

    const Vulkan::Image& image() const LIFETIME_BOUND { return m_image; }
    Vulkan::Image& image() LIFETIME_BOUND { return m_image; }

    const Vulkan::DeviceMemory& memory() const LIFETIME_BOUND { return m_memory; }
    Vulkan::DeviceMemory& memory() LIFETIME_BOUND { return m_memory; }

    VkFormat format() const { return m_format; }
    const IntSize& size() const { return m_size; }
    size_t allocatedSize() const { return m_allocatedSize; }
    bool dedicatedAllocation() const { return m_dedicatedAllocation; }

private:
    VulkanGraphicsBuffer(Vulkan::Image&& image, Vulkan::DeviceMemory&& memory, VkFormat format, const IntSize& size, size_t allocatedSize, bool dedicatedAllocation)
        : m_image(WTF::move(image))
        , m_memory(WTF::move(memory))
        , m_format(format)
        , m_size(size)
        , m_allocatedSize(allocatedSize)
        , m_dedicatedAllocation(dedicatedAllocation)
    {
    }

    VulkanGraphicsBuffer(UnixFileDescriptor&&, VkFormat, const IntSize&, size_t allocatedSize, bool dedicatedAllocation);

    Vulkan::Image m_image;
    Vulkan::DeviceMemory m_memory;
    VkFormat m_format;
    IntSize m_size;
    size_t m_allocatedSize;
    bool m_dedicatedAllocation;

    friend struct IPC::ArgumentCoder<VulkanGraphicsBuffer>;
};

namespace Vulkan {
using GraphicsBuffer = VulkanGraphicsBuffer;
} // namespace Vulkan

} // namespace WebCore

#endif // USE(VULKAN)
