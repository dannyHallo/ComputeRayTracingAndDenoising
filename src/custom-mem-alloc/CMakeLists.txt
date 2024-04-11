add_library(src-custom-mem-alloc STATIC CustomMemoryAllocator.cpp)
target_include_directories(src-custom-mem-alloc PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-custom-mem-alloc PRIVATE
    src-utils-logger
)
