add_library(src-utils-event-dispatcher STATIC GlobalEventDispatcher.cpp)
target_include_directories(src-utils-event-dispatcher PRIVATE ${vcpkg_INCLUDE_DIR})
target_link_libraries(src-utils-event-dispatcher PRIVATE EnTT::EnTT)
