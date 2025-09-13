cmake_minimum_required(VERSION 3.13)

project(rbfx-test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_C_COMPILER_LAUNCHER "ccache")
# set(CMAKE_CXX_COMPILER_LAUNCHER "ccache")

#list(APPEND CMAKE_PREFIX_PATH "/home/USERNAME/apps/rbfx/share")
find_package(Urho3D REQUIRED)
find_package(sdl2 REQUIRED)
find_package(assimp REQUIRED)

add_compile_definitions(USING_RBFX)

add_library(rbfx_test_pch INTERFACE)
add_library(rbfx_test::pch ALIAS rbfx_test_pch)
target_precompile_headers(rbfx_test_pch INTERFACE
    <Urho3D/Core/CoreEvents.h>
    <Urho3D/Engine/Application.h>
    <Urho3D/Engine/EngineDefs.h>
    <Urho3D/Graphics/Camera.h>
    <Urho3D/Graphics/CustomGeometry.h>
    <Urho3D/Graphics/DebugRenderer.h>
    <Urho3D/Graphics/Geometry.h>
    <Urho3D/Graphics/Graphics.h>
    <Urho3D/Graphics/GraphicsEvents.h>
    <Urho3D/Graphics/IndexBuffer.h>
    <Urho3D/Graphics/Light.h>
    <Urho3D/Graphics/Material.h>
    <Urho3D/Graphics/Model.h>
    <Urho3D/Graphics/Octree.h>
    <Urho3D/Graphics/Renderer.h>
    <Urho3D/Graphics/StaticModel.h>
    <Urho3D/Graphics/Technique.h>
    <Urho3D/Graphics/VertexBuffer.h>
    <Urho3D/Graphics/Viewport.h>
    <Urho3D/Graphics/Zone.h>
    <Urho3D/Physics/CollisionShape.h>
    <Urho3D/Physics/PhysicsEvents.h>
    <Urho3D/Physics/PhysicsWorld.h>
    <Urho3D/Physics/RigidBody.h>
    <Urho3D/Input/Input.h>
    <Urho3D/Input/InputEvents.h>
    <Urho3D/Resource/ResourceCache.h>
    <Urho3D/RenderPipeline/RenderPipeline.h>
    <Urho3D/RenderPipeline/ShaderConsts.h>
    <Urho3D/Scene/Scene.h>
    <Urho3D/SystemUI/DebugHud.h>
    <Urho3D/UI/Font.h>
    <Urho3D/UI/UI.h>
    <Urho3D/UI/UIEvents.h>
)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    src/SceneLoader.cpp
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Urho3D
    ${ASSIMP_LIBRARIES}
    rbfx_test::pch
)
