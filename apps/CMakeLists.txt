add_executable(${CMAKE_PROJECT_NAME} main.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    src-utils-logger
    src-application
)
