cmake_minimum_required(VERSION 3.13)

project(rbfx-test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#list(APPEND CMAKE_PREFIX_PATH "/home/USERNAME/apps/rbfx/share")
find_package(Urho3D REQUIRED)
find_package(sdl2 REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Urho3D
)
