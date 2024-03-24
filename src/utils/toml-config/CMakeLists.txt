add_library(src-utils-toml-config STATIC TomlConfigReader.cpp)
target_include_directories(src-utils-toml-config PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-utils-toml-config PRIVATE src-utils-logger)
