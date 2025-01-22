add_executable(Game "${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp")
target_link_libraries(Game PUBLIC FireflyCore)
target_include_directories(Game INTERFACE "${Vulkan_INCLUDE_DIR}")
install(FILES $<TARGET_RUNTIME_DLLS:Game> TYPE BIN)