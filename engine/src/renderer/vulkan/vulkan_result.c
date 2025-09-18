#include "renderer/vulkan/vulkan_result.h"

bool vulkan_result_is_success(VkResult result)
{
    switch(result)
    {
        // Коды успеха.
        case VK_SUCCESS:
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
        case VK_SUBOPTIMAL_KHR:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return true;

        // Коды ошибок.
        default:
            return false;
    }
}

const char* vulkan_result_get_string(VkResult result)
{
    switch(result)
    {
        // Коды успеха.
        case VK_SUCCESS:
            return "VK_SUCCESS: Command successfully completed";
        case VK_NOT_READY:
            return "VK_NOT_READY: A fence or query has not yet completed";
        case VK_TIMEOUT:
            return "VK_TIMEOUT: A wait operation has not completed in the specified time";
        case VK_EVENT_SET:
            return "VK_EVENT_SET: An event is signaled";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET: An event is unsignaled";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE: A return array was too small for the result";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR: A swapchain no longer matches the surface properties exactly";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR: Deferred operation is not complete but no work for this thread";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR: Deferred operation is not complete but no work remaining";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR: A deferred operation was requested and work was deferred";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR: A deferred operation was requested but no operations were deferred";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return "VK_PIPELINE_COMPILE_REQUIRED_EXT: Pipeline creation would have required compilation";

        // Коды ошибок.
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY: A host memory allocation has failed";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY: A device memory allocation has failed";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED: Initialization of an object could not be completed";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST: The logical or physical device has been lost";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED: Mapping of a memory object has failed";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT: A requested layer is not present or could not be loaded";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT: A requested extension is not supported";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT: A requested feature is not supported";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER: The requested version of Vulkan is not supported";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS: Too many objects of the type have already been created";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED: A requested format is not supported on this device";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL: A pool allocation has failed due to fragmentation";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY: A pool memory allocation has failed";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE: An external handle is not a valid handle";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION: A descriptor pool creation has failed due to fragmentation";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: A buffer creation failed because address is not available";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR: A surface is no longer available";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: The requested window is already in use";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR: A surface has changed and is no longer compatible with swapchain";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: The display is incompatible for sharing an image";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT: Invalid usage was detected";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT: Internal resources for compression are exhausted";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: The requested image usage flags are not supported";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: The requested video picture layout is not supported";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR: The driver has denied a request due to insufficient privileges";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
            return "VK_ERROR_NOT_ENOUGH_SPACE_KHR: Not enough space to return all required data";
        case VK_ERROR_UNKNOWN: default:
            return "VK_ERROR_UNKNOWN: An unknown error has occurred";
    }
}
