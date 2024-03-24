add_library(src-utils-logger STATIC Logger.cpp)
target_include_directories(src-utils-logger PRIVATE ${vcpkg_INCLUDE_DIR})
target_link_libraries(src-utils-logger PRIVATE spdlog::spdlog)
