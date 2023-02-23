#pragma once

#include "VulkanApplicationContext.h"
#include "VulkanSwapchain.h"

namespace VulkanGlobal {
// Application context - manages device, surface, queues and command pool.
const VulkanApplicationContext context{};
const VulkanSwapchain swapchainContext{};
} // namespace VulkanGlobal