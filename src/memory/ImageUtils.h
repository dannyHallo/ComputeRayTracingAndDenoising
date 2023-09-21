#pragma once

#include "utils/vulkan.h"

namespace ImageUtils {
VkImageMemoryBarrier readOnlyToGeneralBarrier(const VkImage &image);
VkImageMemoryBarrier undefinedToTransferDstBarrier(const VkImage &image);
VkImageMemoryBarrier generalToReadOnlyBarrier(const VkImage &image);
VkImageMemoryBarrier generalToTransferDstBarrier(const VkImage &image);
VkImageMemoryBarrier generalToTransferSrcBarrier(const VkImage &image);
VkImageMemoryBarrier transferSrcToGeneralBarrier(const VkImage &image);
VkImageMemoryBarrier transferSrcToReadOnlyBarrier(const VkImage &image);
VkImageMemoryBarrier transferDstToGeneralBarrier(const VkImage &image);
VkImageMemoryBarrier transferDstToTransferSrcBarrier(const VkImage &image);
VkImageMemoryBarrier transferDstToReadOnlyBarrier(const VkImage &image);
VkImageMemoryBarrier transferDstToColorAttachmentBarrier(const VkImage &image);
VkImageCopy imageCopyRegion(uint32_t width, uint32_t height);
} // namespace ImageUtils