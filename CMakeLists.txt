cmake_minimum_required(VERSION 3.31)

project(Firefly
            VERSION 0.1
            LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Engine)
add_subdirectory(Game)