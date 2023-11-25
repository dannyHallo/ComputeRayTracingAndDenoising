#define SPDLOG_HEADER_ONLY
#include "Logger.hpp"

// this is required to define format functions that are used by spdlog
#define FMT_HEADER_ONLY
#include "spdlog/fmt/bundled/format.h"

#include "spdlog/spdlog.h" // spdlog/include/spdlog/details/windows_include.h defines APIENTRY!

const std::unordered_map<int, std::string> resultCodeNamePair = {
    {VK_SUCCESS, "Command successfully completed"},
    {VK_NOT_READY, "A fence or query has not yet completed"},
    {VK_TIMEOUT, "A wait operation has not completed in the specified time"},
    {VK_EVENT_SET, "An event is signaled"},
    {VK_EVENT_RESET, "An event is unsignaled"},
    {VK_INCOMPLETE, "A return array was too small for the result"},
    {VK_ERROR_OUT_OF_HOST_MEMORY, "A host memory allocation has failed"},
    {VK_ERROR_OUT_OF_DEVICE_MEMORY, "A device memory allocation has failed"},
    {VK_ERROR_INITIALIZATION_FAILED, "Initialization of an object could not be completed for "
                                     "implementation-specific reasons"},
    {VK_ERROR_DEVICE_LOST, "The logical or physical device has been lost"},
    {VK_ERROR_MEMORY_MAP_FAILED, "Mapping of a memory object has failed"},
    {VK_ERROR_LAYER_NOT_PRESENT, "A requested layer is not present or could not be loaded"},
    {VK_ERROR_EXTENSION_NOT_PRESENT, "A requested extension is not supported"},
    {VK_ERROR_FEATURE_NOT_PRESENT, "A requested feature is not supported"},
    {VK_ERROR_INCOMPATIBLE_DRIVER,
     "The requested version of Vulkan is not supported by the driver or is "
     "otherwise "
     "incompatible for implementation-specific reasons"},
    {VK_ERROR_TOO_MANY_OBJECTS, "Too many objects of the type have already been created"},
    {VK_ERROR_FORMAT_NOT_SUPPORTED, "A requested format is not supported on this device"},
    {VK_ERROR_FRAGMENTED_POOL,
     "A pool allocation has failed due to fragmentation of the pool’s memory. "
     "This must only be returned if no attempt to "
     "allocate host or device memory was made to accommodate the new "
     "allocation. If the failure was definitely due to "
     "fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned. "
     "If the allocation could have failed due to "
     "either fragmentation of the pool or an attempt to allocate host or "
     "device memory failing (due to heap exhaustion or "
     "platform limits), VK_ERROR_OUT_OF_POOL_MEMORY should be returned "
     "instead."},
    {VK_ERROR_UNKNOWN, "An unknown error has occurred; either the application "
                       "has provided invalid input, or an implementation "
                       "failure has occurred"},
    {VK_ERROR_OUT_OF_POOL_MEMORY,
     "A pool memory allocation has failed. This must only be returned if no "
     "attempt to allocate host or device memory was made "
     "to accommodate the new allocation. This should be returned in preference "
     "to VK_ERROR_OUT_OF_HOST_MEMORY or "
     "VK_ERROR_OUT_OF_DEVICE_MEMORY, but only if the implementation is certain "
     "of the cause."},
    {VK_ERROR_INVALID_EXTERNAL_HANDLE,
     "An external handle is not a valid handle of the specified type"},
    {VK_ERROR_FRAGMENTATION, "A descriptor pool creation has failed due to fragmentation."},
    {VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
     "A buffer creation or memory allocation failed because the requested "
     "address is not available. A shader group handle "
     "assignment failed because the requested shader group handle information "
     "is no longer valid."},
    {VK_PIPELINE_COMPILE_REQUIRED,
     "Indicates that a pipeline creation would have required compilation if "
     "the compiled state "
     "was not already available in cache."},
    {VK_ERROR_SURFACE_LOST_KHR, "A surface is no longer available."},
    {VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,
     "The requested window is already in use by Vulkan or another API in a "
     "manner which prevents it from being used again."},
    {VK_SUBOPTIMAL_KHR, "A swapchain no longer matches the surface properties "
                        "exactly, but can still be used to present to the "
                        "surface successfully."},
    {VK_ERROR_OUT_OF_DATE_KHR,
     "A surface has changed in such a way that it is no longer compatible with "
     "the swapchain, and further presentation "
     "requests using the swapchain will fail. Applications must query the new "
     "surface properties and recreate their swapchain "
     "if they wish to continue presenting to the surface."},
    {VK_ERROR_INCOMPATIBLE_DISPLAY_KHR,
     "The display used by a swapchain does not use the same presentable image "
     "layout, or is "
     "incompatible in a way that prevents sharing an image."},
    {VK_ERROR_VALIDATION_FAILED_EXT, "A validation layer found an error."},
    {VK_ERROR_INVALID_SHADER_NV,
     "One or more shaders failed to compile or link. More details are reported "
     "back to the "
     "application via VK_EXT_debug_report if enabled."},
#ifdef VK_ENABLE_BETA_EXTENSIONS
    {VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR,
     "The image usage flags specified in VkImageCreateInfo are not supported "
     "for this image format on this platform."}
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    ,
    {VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR,
     "The video picture layout specified in "
     "VkVideoPictureResourceKHR::pictureFormat is not supported for this video "
     "codec on "
     "this platform."}
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    ,
    {VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR,
     "The video profile operation specified in "
     "VkVideoProfileKHR::videoCodecOperation is not supported for this video "
     "codec on "
     "this platform."}
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    ,
    {VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR,
     "The video profile format specified in VkVideoProfileKHR::videoFormat is "
     "not supported for this video codec on this platform."}
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    ,
    {VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR,
     "The video profile codec specified in VkVideoProfileKHR::videoCodec is "
     "not supported on this platform."}
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    ,
    {VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR,
     "The video standard version specified in "
     "VkVideoProfileKHR::videoStdVersion "
     "is not supported for this video codec on this platform."},
#endif
    {VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
     "The DRM format modifier plane layout specified in "
     "VkImageDrmFormatModifierExplicitCreateInfoEXT is not supported by the "
     "device."},
    {VK_ERROR_NOT_PERMITTED_KHR, "The operation is not permitted."},
    {VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,
     "An operation on a swapchain created with "
     "VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not "
     "have "
     "exlusive full-screen access. This may occur due to "
     "implementation-dependent reasons, outside of the application’s "
     "control."},
    {VK_THREAD_IDLE_KHR, "A deferred operation is not complete but there is currently no work for "
                         "this thread to do at the time of this call."},
    {VK_THREAD_DONE_KHR, "A deferred operation is not complete but there is no "
                         "work remaining to assign to additional threads."},
    {VK_OPERATION_DEFERRED_KHR, "A deferred operation was requested and at "
                                "least some of the work was deferred."},
    {VK_OPERATION_NOT_DEFERRED_KHR,
     "A deferred operation was requested and no operations were deferred."},
    {VK_ERROR_COMPRESSION_EXHAUSTED_EXT,
     "The compressed data provided to the API was exhausted before the "
     "operation completed."},
    {VK_ERROR_OUT_OF_POOL_MEMORY_KHR,
     "A pool memory allocation has failed. This must only be returned if no "
     "attempt to allocate host or device memory was made "
     "to accommodate the new allocation. This should be returned in preference "
     "to VK_ERROR_OUT_OF_HOST_MEMORY or "
     "VK_ERROR_OUT_OF_DEVICE_MEMORY, but only if the implementation is certain "
     "of the cause."},
    {VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR,
     "An external handle is not a valid handle of the specified type"},
    {VK_ERROR_FRAGMENTATION_EXT, "A descriptor pool creation has failed due to fragmentation."},
    {VK_ERROR_NOT_PERMITTED_EXT, "The operation is not permitted."},
    {VK_ERROR_INVALID_DEVICE_ADDRESS_EXT,
     "A buffer creation or memory allocation failed because the requested "
     "address is not available. A shader group handle "
     "assignment failed because the requested shader group handle information "
     "is no longer valid."},
    {VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR,
     "A buffer creation or memory allocation failed because the requested "
     "address is not available. A shader group handle "
     "assignment failed because the requested shader group handle information "
     "is no longer valid."},
    {VK_PIPELINE_COMPILE_REQUIRED_EXT,
     "Indicates that a pipeline creation would have required compilation if "
     "the compiled "
     "state was not already available in cache."},
    {VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT,
     "Indicates that a pipeline creation would have required compilation if "
     "the "
     "compiled state was not already available in cache."},
    {VK_RESULT_MAX_ENUM, "Reserved for internal use by the Vulkan specification"}};

void Logger::checkStep(const std::string stepName, const int resultCode) {
  if (resultCode == VK_SUCCESS) return;

  if (resultCodeNamePair.find(resultCode) == resultCodeNamePair.end()) {
    throwError("Error code not found: " + std::to_string(resultCode));
  }

  std::string errorMsg = resultCodeNamePair.at(resultCode);
  throwError("Error code: " + std::to_string(resultCode) + " " + errorMsg);
}
