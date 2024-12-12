add_library(FireflyCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Private/Firefly.cpp")
target_include_directories(FireflyCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_link_libraries(FireflyCore PUBLIC ThirdParty)