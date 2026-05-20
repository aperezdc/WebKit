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
#include "FourCC.h"
#include "VulkanHandle.h"
#include "VulkanStructure.h"
#include <expected>
#include <wtf/text/WTFString.h>

#ifdef VK_KHR_external_memory_fd
#include <wtf/unix/UnixFileDescriptor.h>
#endif

namespace WebCore {

class IntSize;
class PlatformDisplay;

namespace Vulkan {

template <typename Type>
using Result = std::expected<Type, VkResult>;

struct ApplicationInfo : Structure<VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO> {
    ApplicationInfo(const String& applicationName, uint32_t apiVersion = VK_API_VERSION_1_3)
        : ApplicationInfo(applicationName.utf8().data(), apiVersion)
    {
    }

private:
    ApplicationInfo(const char* applicationName, uint32_t apiVersion);
};

struct MemoryRequirements : Structure<VkMemoryRequirements2, VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2> {
    const VkMemoryRequirements* LIFETIME_BOUND operator->() const { return &ptr()->memoryRequirements; }
    VkMemoryRequirements* LIFETIME_BOUND operator->() { return &ptr()->memoryRequirements; }
};

struct Image;

struct MemoryAllocateInfo : Structure<VkMemoryAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO> {
    MemoryAllocateInfo(VkDeviceSize allocationSize, uint32_t memoryTypeIndex)
    {
        ptr()->allocationSize = allocationSize;
        ptr()->memoryTypeIndex = memoryTypeIndex;
    }
};

struct ExportMemoryAllocateInfo : Structure<VkExportMemoryAllocateInfo, VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO> {
    ExportMemoryAllocateInfo(VkExternalMemoryHandleTypeFlags handleTypes)
    {
        ptr()->handleTypes = handleTypes;
    }
};

struct MemoryDedicatedAllocateInfo : Structure<VkMemoryDedicatedAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO> {
    MemoryDedicatedAllocateInfo() : MemoryDedicatedAllocateInfo(VK_NULL_HANDLE, VK_NULL_HANDLE) { }
    MemoryDedicatedAllocateInfo(const Image&);

private:
    MemoryDedicatedAllocateInfo(VkImage, VkBuffer);
};

struct MemoryDedicatedRequirements : Structure<VkMemoryDedicatedRequirements, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS> {
};

struct DeviceMemory : Handle<VkDeviceMemory>
{
    ~DeviceMemory();

#ifdef VK_KHR_external_memory_fd
    [[nodiscard]] Result<UnixFileDescriptor> getFileDescriptor(VkExternalMemoryHandleTypeFlagBits) const;
#endif

    VULKAN_DEFINE_HANDLE_METHODS(DeviceMemory);
    friend struct Device;
};

#ifdef VK_KHR_external_memory_fd
struct MemoryGetFdInfo : Structure<VkMemoryGetFdInfoKHR, VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR> {
    MemoryGetFdInfo(const DeviceMemory& memory, VkExternalMemoryHandleTypeFlagBits handleType)
    {
        ptr()->memory = memory.ptr();
        ptr()->handleType = handleType;
    }
};
#endif // VK_KHR_external_memory_fd

struct ImageMemoryRequirementsInfo : Structure<VkImageMemoryRequirementsInfo2, VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2> {
    ImageMemoryRequirementsInfo(const Image&);
};

struct ImageCreateInfo : Structure<VkImageCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO> {
};

struct ImageFormatProperties : Structure<VkImageFormatProperties2, VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2> {
};

struct ExternalMemoryImageCreateInfo : Structure<VkExternalMemoryImageCreateInfo, VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO> {
    ExternalMemoryImageCreateInfo(VkExternalMemoryHandleTypeFlags handleTypes)
    {
        ptr()->handleTypes = handleTypes;
    }
};

#ifdef VK_EXT_image_drm_format_modifier
struct ImageDrmFormatModifierListCreateInfo : Structure<VkImageDrmFormatModifierListCreateInfoEXT, VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT> {
    ImageDrmFormatModifierListCreateInfo() = default;

    ImageDrmFormatModifierListCreateInfo(const uint64_t& modifier)
    {
        ptr()->drmFormatModifierCount = 1;
        ptr()->pDrmFormatModifiers = std::addressof(modifier);
    }

    ImageDrmFormatModifierListCreateInfo(const std::span<const uint64_t> modifiers)
    {
        ptr()->drmFormatModifierCount = modifiers.size();
        ptr()->pDrmFormatModifiers = modifiers.data();
    }
};

struct PhysicalDeviceImageDrmFormatModifierInfo : Structure<VkPhysicalDeviceImageDrmFormatModifierInfoEXT, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT> {
};
#endif

struct Image : Handle<VkImage>
{
    [[nodiscard]] static Result<Image> create(const ImageCreateInfo&);
    ~Image();

    void fillMemoryRequirements(MemoryRequirements& memRequirements);

    VULKAN_DEFINE_HANDLE_METHODS(Image);
    friend struct Instance;
};

struct InstanceCreateInfo : Structure<VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO> {
    InstanceCreateInfo(const ApplicationInfo& applicationInfo LIFETIME_BOUND, std::span<const char* const> enabledLayers LIFETIME_BOUND = { }, std::span<const char* const> enabledExtensions LIFETIME_BOUND = { });
};

using QueueFamilyProperties = VkQueueFamilyProperties;

struct DeviceQueueCreateInfo : Structure<VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO> {
    DeviceQueueCreateInfo(uint32_t familyIndex, std::span<const float> queuePriorities);
};

struct PhysicalDeviceProperties : Structure<VkPhysicalDeviceProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2> {
    const VkPhysicalDeviceProperties* LIFETIME_BOUND operator->() const { return &ptr()->properties; }
    VkPhysicalDeviceProperties* LIFETIME_BOUND operator->() { return &ptr()->properties; }
};

struct PhysicalDeviceDRMProperties : Structure<VkPhysicalDeviceDrmPropertiesEXT, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT> {
};

struct PhysicalDeviceIDProperties : Structure<VkPhysicalDeviceIDProperties, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES> {
    std::span<const uint8_t> deviceUUID() const;
    std::span<const uint8_t> driverUUID() const;
};

struct PhysicalDeviceImageFormatInfo : Structure<VkPhysicalDeviceImageFormatInfo2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2> {
};

struct PhysicalDeviceMemoryProperties : Structure<VkPhysicalDeviceMemoryProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2> {
    const VkPhysicalDeviceMemoryProperties* LIFETIME_BOUND operator->() const { return &ptr()->memoryProperties; }
    VkPhysicalDeviceMemoryProperties* LIFETIME_BOUND operator->() { return &ptr()->memoryProperties; }

    std::span<const VkMemoryType> memoryTypes() const LIFETIME_BOUND
    {
        return unsafeMakeSpan(ptr()->memoryProperties.memoryTypes, ptr()->memoryProperties.memoryTypeCount);
    }

    std::span<const VkMemoryHeap> memoryHeaps() const LIFETIME_BOUND
    {
        return unsafeMakeSpan(ptr()->memoryProperties.memoryHeaps, ptr()->memoryProperties.memoryHeapCount);
    }
};

struct PhysicalDevice : BorrowedHandle<VkPhysicalDevice> {
    void fillProperties(PhysicalDeviceProperties& properties) const
    {
        vkGetPhysicalDeviceProperties2(ptr(), properties.ptr());
    }

    void fillMemoryProperties(PhysicalDeviceMemoryProperties& memoryProperties) const
    {
        vkGetPhysicalDeviceMemoryProperties2(ptr(), memoryProperties.ptr());
    }

    Vector<QueueFamilyProperties> queueFamilies() const;
};

struct DeviceCreateInfo : Structure<VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO> {
    DeviceCreateInfo(const DeviceQueueCreateInfo&, std::span<const char* const> enabledExtensions = { });
};

struct Device : Handle<VkDevice> {
    [[nodiscard]] static Result<Device> create(const PhysicalDevice&, const DeviceCreateInfo&);
    ~Device();

    void swap(Device& other)
    {
        Base::swap(other);
        std::swap(m_physicalDevice, other.m_physicalDevice);
        std::swap(m_table, other.m_table);
    }

    static void setSharedDevice(Device&&);
    [[nodiscard]] static Device* sharedDeviceIfExists();
    [[nodiscard]] static Device& sharedDevice();

    [[nodiscard]] std::optional<uint32_t> findMemoryTypeIndex(const MemoryRequirements&, VkMemoryPropertyFlags) const;

    [[nodiscard]] Result<DeviceMemory> allocateExternalMemory(const MemoryRequirements&, const Image&, VkExternalMemoryHandleTypeFlags) const;

    [[nodiscard]] Result<DeviceMemory> allocateMemory(const MemoryAllocateInfo& memAllocateInfo) const
    {
        VkDeviceMemory memory;
        if (auto result = m_table.vkAllocateMemory(ptr(), memAllocateInfo.ptr(), nullptr, &memory); result != VK_SUCCESS)
            return makeUnexpected(result);

        return DeviceMemory(memory);
    }

    void freeMemory(DeviceMemory& handle)
    {
        if (VkDeviceMemory memory = handle.leakPtr())
            m_table.vkFreeMemory(ptr(), memory, nullptr);
    }

#ifdef VK_KHR_external_memory_fd
    [[nodiscard]] Result<UnixFileDescriptor> getMemoryFileDescriptor(const DeviceMemory&, VkExternalMemoryHandleTypeFlagBits) const;
#endif

    [[nodiscard]] Result<Image> createImage(const ImageCreateInfo& creationInfo) const
    {
        VkImage image;
        if (auto result = m_table.vkCreateImage(ptr(), creationInfo.ptr(), nullptr, &image); result != VK_SUCCESS)
            return makeUnexpected(result);

        return Image(image);
    }

    ALWAYS_INLINE void destroyImage(Image& handle)
    {
        if (VkImage image = handle.leakPtr())
            m_table.vkDestroyImage(ptr(), image, nullptr);
    }

    ALWAYS_INLINE void getImageMemoryRequirements(const ImageMemoryRequirementsInfo& imageMemRequirements, MemoryRequirements& memRequirements)
    {
        m_table.vkGetImageMemoryRequirements2(ptr(), imageMemRequirements.ptr(), memRequirements.ptr());
    }

    ALWAYS_INLINE VkResult bindImageMemory(const Image& image, const DeviceMemory& memory, VkDeviceSize offset = 0)
    {
        return m_table.vkBindImageMemory(ptr(), image.ptr(), memory.ptr(), offset);
    }

private:
    explicit Device(VkDevice, const PhysicalDevice&);

    VolkDeviceTable m_table;
    PhysicalDevice m_physicalDevice;

    static Device s_sharedDevice;

    VULKAN_DEFINE_HANDLE_METHODS(Device);
};

struct Instance : Handle<VkInstance> {
    [[nodiscard]] static const Vector<VkLayerProperties>& availableLayers();
    [[nodiscard]] static bool hasLayers(std::span<const char* const> layerNames);
    [[nodiscard]] static bool hasLayer(const char* const layerName);

    [[nodiscard]] static Result<Vector<VkExtensionProperties>> availableExtensions(const char* layerName = nullptr);
    [[nodiscard]] static bool hasExtensions(const Vector<VkExtensionProperties>&, std::span<const char* const> extensionNames);
    [[nodiscard]] static bool hasExtension(const Vector<VkExtensionProperties>&, const char* const extensionName);
    [[nodiscard]] static bool hasExtensions(std::span<const char* const> extensionNames, const char* layerName = nullptr);
    [[nodiscard]] static bool hasExtension(const char* const extensionName, const char* layerName = nullptr);

    static void setSharedInstance(Instance&&);
    [[nodiscard]] static Instance* sharedInstanceIfExists();
    [[nodiscard]] static Instance& sharedInstance();

    [[nodiscard]] static Result<Instance> create(const InstanceCreateInfo&);
    ~Instance();

#ifdef VK_EXT_debug_utils
    void swap(Instance& other)
    {
        Base::swap(other);
        std::swap(m_debugMessenger, other.m_debugMessenger);
    }
#endif // VK_EXT_debug_utils

    [[nodiscard]] Result<Vector<PhysicalDevice>> availableDevices() const;
    [[nodiscard]] Result<PhysicalDevice> deviceForDisplay(PlatformDisplay&);
    [[nodiscard]] VkResult installDebugMessenger();

private:
    explicit Instance(VkInstance);

    static Instance s_sharedInstance;

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT m_debugMessenger { nullptr };
#endif

    VULKAN_DEFINE_HANDLE_METHODS(Instance);
};

} // namespace Vulkan
} // namespace WebCore

#endif // USE(VULKAN)
