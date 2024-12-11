# VK_HEADER_VERSION 296
add_library(Vulkan INTERFACE)
target_include_directories(Vulkan INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_directories(Vulkan INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Lib")