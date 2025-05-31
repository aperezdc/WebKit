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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if USE(COORDINATED_GRAPHICS) && OS(ANDROID)
#include <optional>
#include <wtf/ThreadSafeRefCounted.h>

typedef struct AHardwareBuffer AHardwareBuffer;

namespace WebCore {

class AndroidBuffer final : public ThreadSafeRefCounted<AndroidBuffer> {
public:
    static Ref<AndroidBuffer> create(std::unique_ptr<AHardwareBuffer>&& buffer)
    {
        return adoptRef(*new AndroidBuffer(WTFMove(buffer)));
    }

    uint64_t id() const { return m_id; }

    enum class ColorSpace : uint8_t { BT601, BT709, BT2020, SMPTE240M };
    std::optional<ColorSpace> colorSpace() const { return m_colorSpace; }
    void setColorSpace(ColorSpace colorSpace) { m_colorSpace = colorSpace; }

    ~AndroidBuffer() = default;

private:
    explicit AndroidBuffer(std::unique_ptr<AHardwareBuffer>&&);

    // TODO(android): Use AHardwareBuffer_getId on API 31+
    uint64_t m_id { 0 };

    std::optional<ColorSpace> m_colorSpace;
    std::unique_ptr<AHardwareBuffer> m_buffer;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS) && OS(ANDROID)
