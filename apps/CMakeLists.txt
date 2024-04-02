add_executable(run main.cpp)

target_include_directories(run PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(run PRIVATE
    src-utils-logger
    src-application
)
