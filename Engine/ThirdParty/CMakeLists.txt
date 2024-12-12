add_library(ThirdParty INTERFACE)

add_subdirectory(Vulkan-Headers)
add_subdirectory(SDL)
add_subdirectory(glm)

target_link_libraries(ThirdParty INTERFACE Vulkan-Headers SDL3-shared glm-header-only)
target_include_directories(ThirdParty INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")