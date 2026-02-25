cmake_minimum_required(VERSION 3.16)
project(OpenGLEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)

# --- GLAD (bundled) ---
add_library(glad STATIC
    OpenGLEngine/Engine/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    OpenGLEngine/Engine/glad/include
)

# --- Engine + Game sources ---
set(ENGINE_SOURCES
    OpenGLEngine/Engine/source/Engine/Engine.cpp
    OpenGLEngine/Engine/source/Render/RenderEngine.cpp
    OpenGLEngine/Engine/source/Render/ShaderProgram.cpp
    OpenGLEngine/Engine/source/Render/VertexArrayObject.cpp
    OpenGLEngine/Engine/source/Render/UniformBuffer.cpp
    OpenGLEngine/Engine/source/Render/Texture.cpp
    OpenGLEngine/Engine/source/Window/Window.cpp
    OpenGLEngine/Engine/source/Input/InputSystem.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsSystems.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsWorld.cpp
    OpenGLEngine/Engine/source/Resource/ModelLoader.cpp
)

set(GAME_SOURCES
    OpenGLEngine/Game/main.cpp
    OpenGLEngine/Game/ExampleGame.cpp
)

add_executable(OpenGLEngine ${ENGINE_SOURCES} ${GAME_SOURCES})

target_include_directories(OpenGLEngine PRIVATE
    OpenGLEngine/Engine/include
    OpenGLEngine/Engine
    OpenGLEngine/Game
)

target_link_libraries(OpenGLEngine PRIVATE
    glad
    glfw
    OpenGL::GL
    assimp::assimp
)

target_compile_definitions(OpenGLEngine PRIVATE GL_SILENCE_DEPRECATION)

# Copy assets to build directory
add_custom_command(TARGET OpenGLEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/OpenGLEngine/Assets
        $<TARGET_FILE_DIR:OpenGLEngine>/Assets
)
