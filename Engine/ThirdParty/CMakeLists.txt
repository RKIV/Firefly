add_library(ThirdParty INTERFACE)

add_subdirectory(SDL)
add_subdirectory(glm)

find_package(Vulkan REQUIRED)

target_link_libraries(ThirdParty INTERFACE Vulkan::Vulkan SDL3-shared glm-header-only)
target_link_libraries(ThirdParty INTERFACE "${Vulkan_LIBRARY}")
target_include_directories(ThirdParty INTERFACE "${Vulkan_INCLUDE_DIR}")
target_include_directories(ThirdParty INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")