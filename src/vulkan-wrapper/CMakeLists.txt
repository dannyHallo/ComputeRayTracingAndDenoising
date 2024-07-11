add_library(src-vulkan-wrapper STATIC
    descriptor-set/DescriptorSetBundle.cpp
    sampler/Sampler.cpp
    memory/Buffer.cpp
    memory/BufferBundle.cpp
    memory/Image.cpp
    pipeline/ComputePipeline.cpp
    pipeline/Pipeline.cpp
    utils/SimpleCommands.cpp
)

target_include_directories(src-vulkan-wrapper PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(src-vulkan-wrapper PRIVATE
    src-app-context
    src-utils-logger
    src-utils-io
    src-utils-shader-compiler
    volk::volk
    volk::volk_headers
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
)
