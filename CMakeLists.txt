cmake_minimum_required(VERSION 3.16)
project(OpenGLEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)

set(WWISE_SDK_DIR "" CACHE PATH "Path to Wwise SDK root")
option(OPENGLENGINE_ENABLE_WWISE "Enable Wwise integration" OFF)
if(NOT WWISE_SDK_DIR AND DEFINED ENV{WWISE_SDK_DIR})
    set(WWISE_SDK_DIR "$ENV{WWISE_SDK_DIR}")
endif()

set(WWISE_SDK_ROOT "")
if(WWISE_SDK_DIR)
    if(EXISTS "${WWISE_SDK_DIR}/include/AK/AkPlatforms.h")
        set(WWISE_SDK_ROOT "${WWISE_SDK_DIR}")
    elseif(EXISTS "${WWISE_SDK_DIR}/SDK/include/AK/AkPlatforms.h")
        set(WWISE_SDK_ROOT "${WWISE_SDK_DIR}/SDK")
    endif()
endif()

# --- GLAD (bundled) ---
add_library(glad STATIC
    OpenGLEngine/Engine/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    OpenGLEngine/Engine/glad/include
)

# --- Engine + Game sources ---
set(ENGINE_SOURCES
    OpenGLEngine/Engine/source/Audio/AudioSystem.cpp
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

if(OPENGLENGINE_ENABLE_WWISE AND WWISE_SDK_ROOT)
    target_include_directories(OpenGLEngine PRIVATE
        ${WWISE_SDK_ROOT}
        ${WWISE_SDK_ROOT}/include
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Win32
    )
    target_compile_definitions(OpenGLEngine PRIVATE OPENGLENGINE_USE_WWISE=1 UNICODE _UNICODE)
    target_sources(OpenGLEngine PRIVATE
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Win32/AkDefaultIOHookDeferred.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkFileLocationBase.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkMultipleFileLocation.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkGeneratedSoundBanksResolver.cpp
    )

    set(WWISE_PLATFORM_DIR "")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WWISE_PLATFORM_DIR "x64_vc170")
    else()
        set(WWISE_PLATFORM_DIR "Win32_vc170")
    endif()

    set(WWISE_CONFIG_DIR "Release")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(WWISE_CONFIG_DIR "Debug")
    endif()

    set(WWISE_LIB_DIR "${WWISE_SDK_ROOT}/${WWISE_PLATFORM_DIR}/${WWISE_CONFIG_DIR}/lib")
    if(NOT EXISTS "${WWISE_LIB_DIR}")
        message(FATAL_ERROR "Wwise libs not found: ${WWISE_LIB_DIR}")
    endif()

    target_link_directories(OpenGLEngine PRIVATE ${WWISE_LIB_DIR})
    target_link_libraries(OpenGLEngine PRIVATE
        AkSoundEngine
        AkMemoryMgr
        AkStreamMgr
        AkAutobahn
        CommunicationCentral
    )
else()
    target_compile_definitions(OpenGLEngine PRIVATE OPENGLENGINE_USE_WWISE=0)
endif()

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
