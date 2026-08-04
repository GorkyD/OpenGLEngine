cmake_minimum_required(VERSION 3.23)
project(OpenGLEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND MSVC)
    find_program(MSVC_LIB_TOOL lib.exe)
    if(MSVC_LIB_TOOL)
        set(CMAKE_AR "${MSVC_LIB_TOOL}")
    endif()
endif()

# --- Dependencies ---
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(imgui CONFIG REQUIRED)

include(cmake/WwiseSetup.cmake)


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
    OpenGLEngine/Engine/source/Render/ShadowMap.cpp
    OpenGLEngine/Engine/source/Render/PostProcessPipeline.cpp
    OpenGLEngine/Engine/source/Render/Font.cpp
    OpenGLEngine/Engine/source/Window/Window.cpp
    OpenGLEngine/Engine/source/Input/InputSystem.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsSystems.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsWorld.cpp
    OpenGLEngine/Engine/source/Resource/ModelLoader.cpp
    OpenGLEngine/Engine/source/Resource/SkinnedModelLoader.cpp
    OpenGLEngine/Engine/source/Resource/MeshFactory.cpp
    OpenGLEngine/Engine/source/Resource/EntityFactory.cpp
    OpenGLEngine/Engine/source/Resource/ModelCache.cpp
    OpenGLEngine/Engine/source/Resource/SceneSerializer.cpp
    OpenGLEngine/Engine/source/Physics/Physics.cpp
    OpenGLEngine/Engine/source/Platform/HighResolutionTimer.cpp
    OpenGLEngine/Engine/source/Platform/ProcessLauncher.cpp
    OpenGLEngine/Engine/source/Editor/AssetLibrary.cpp
    OpenGLEngine/Engine/source/Editor/EditorCommands.cpp
    OpenGLEngine/Engine/source/Save/SaveService.cpp
    OpenGLEngine/Engine/source/Scene/DemoScene.cpp
)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/OpenGLEngine/Game/*.cpp"
)

set(GAME_MAIN_SOURCES "")
foreach(_source ${GAME_SOURCES})
    get_filename_component(_name "${_source}" NAME)
    if(_name STREQUAL "GameMain.cpp")
        list(APPEND GAME_MAIN_SOURCES "${_source}")
    endif()
endforeach()
if(GAME_MAIN_SOURCES)
    list(REMOVE_ITEM GAME_SOURCES ${GAME_MAIN_SOURCES})
endif()

file(GLOB _gameEntries CONFIGURE_DEPENDS LIST_DIRECTORIES true
    "${CMAKE_SOURCE_DIR}/OpenGLEngine/Game/*"
)

set(GAME_DIRS "")
foreach(_entry ${_gameEntries})
    if(IS_DIRECTORY "${_entry}")
        list(APPEND GAME_DIRS "${_entry}")
    endif()
endforeach()

add_library(OpenGLEngineCore STATIC ${ENGINE_SOURCES})

target_include_directories(OpenGLEngineCore PUBLIC
    OpenGLEngine/Engine/include
    OpenGLEngine/Engine
)

add_executable(OpenGLEngine ${GAME_SOURCES})
target_link_libraries(OpenGLEngine PRIVATE OpenGLEngineCore)
target_include_directories(OpenGLEngine PRIVATE
    OpenGLEngine/Game
    ${GAME_DIRS}
)

if(OPENGLENGINE_ENABLE_WWISE AND WWISE_SDK_ROOT AND NOT WIN32)
    message(WARNING "Wwise integration currently supports Windows only, building without audio")
    set(OPENGLENGINE_ENABLE_WWISE OFF)
endif()

if(OPENGLENGINE_ENABLE_WWISE AND WWISE_SDK_ROOT)
    target_include_directories(OpenGLEngineCore PUBLIC
        ${WWISE_SDK_ROOT}
        ${WWISE_SDK_ROOT}/include
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Win32
    )
    target_compile_definitions(OpenGLEngineCore PUBLIC OPENGLENGINE_USE_WWISE=1 UNICODE _UNICODE)
    target_sources(OpenGLEngineCore PRIVATE
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Win32/AkDefaultIOHookDeferred.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkFileLocationBase.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkMultipleFileLocation.cpp
        ${WWISE_SDK_ROOT}/samples/SoundEngine/Common/AkGeneratedSoundBanksResolver.cpp
    )

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

    target_link_directories(OpenGLEngineCore PUBLIC ${WWISE_LIB_DIR})
    target_link_libraries(OpenGLEngineCore PUBLIC
        AkSoundEngine
        AkMemoryMgr
        AkStreamMgr
        AkAutobahn
        CommunicationCentral
    )
    if(WIN32)
        target_link_libraries(OpenGLEngineCore PUBLIC ws2_32 winmm)
    endif()

    if(WWISE_PROJECT_DIR)
        set(WWISE_GENERATED_BANKS_PLATFORM "Windows")
        if(APPLE)
            set(WWISE_GENERATED_BANKS_PLATFORM "Mac")
        elseif(UNIX)
            set(WWISE_GENERATED_BANKS_PLATFORM "Linux")
        endif()
        set(WWISE_GENERATED_BANKS_DIR "${WWISE_PROJECT_DIR}/GeneratedSoundBanks/${WWISE_GENERATED_BANKS_PLATFORM}")

        add_custom_command(TARGET OpenGLEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DSRC_DIR=${WWISE_GENERATED_BANKS_DIR}
                -DDST_DIR=${CMAKE_SOURCE_DIR}/OpenGLEngine/Assets/Wwise
                -P ${CMAKE_SOURCE_DIR}/cmake/CopyWwiseBanks.cmake
            COMMENT "Syncing Wwise SoundBanks from ${WWISE_GENERATED_BANKS_DIR}"
        )
    endif()
else()
    target_compile_definitions(OpenGLEngineCore PUBLIC OPENGLENGINE_USE_WWISE=0)
endif()

target_link_libraries(OpenGLEngineCore PUBLIC
    glad
    glfw
    OpenGL::GL
    assimp::assimp
    imgui::imgui
)

if(WIN32)
    target_link_libraries(OpenGLEngineCore PUBLIC winmm)
endif()

target_compile_definitions(OpenGLEngineCore PUBLIC GL_SILENCE_DEPRECATION)
target_compile_definitions(OpenGLEngineCore PUBLIC OPENGLENGINE_SOURCE_ASSETS="${CMAKE_SOURCE_DIR}/OpenGLEngine/Assets")

set(ASSET_COPY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/OpenGLEngine/Assets
        ${CMAKE_BINARY_DIR}/Assets
)

set(GAME_REGISTRY_ENTRIES "")

foreach(_gameDir ${GAME_DIRS})
    get_filename_component(_dirName "${_gameDir}" NAME)
    string(MAKE_C_IDENTIFIER "${_dirName}" _gameId)
    string(APPEND GAME_REGISTRY_ENTRIES "    {\"${_gameId}\", \"${_gameDir}/Assets\"},\n")

    if(EXISTS "${_gameDir}/Assets")
        list(APPEND ASSET_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_gameDir}/Assets"
                ${CMAKE_BINARY_DIR}/Assets
        )
    endif()
endforeach()

add_custom_target(OpenGLEngineAssets ALL ${ASSET_COPY_COMMANDS}
    COMMENT "Copying engine and game assets"
)

add_dependencies(OpenGLEngine OpenGLEngineAssets)

foreach(_gameMain ${GAME_MAIN_SOURCES})
    get_filename_component(_gameDir "${_gameMain}" DIRECTORY)
    get_filename_component(_gameName "${_gameDir}" NAME)
    string(MAKE_C_IDENTIFIER "${_gameName}" _gameTarget)

    file(GLOB_RECURSE _gameSources CONFIGURE_DEPENDS "${_gameDir}/*.cpp")

    add_executable(${_gameTarget} ${_gameSources})
    target_link_libraries(${_gameTarget} PRIVATE OpenGLEngineCore)
    target_include_directories(${_gameTarget} PRIVATE "${_gameDir}")
    target_compile_definitions(${_gameTarget} PRIVATE
        OPENGLENGINE_GAME_NAME="${_gameTarget}"
        OPENGLENGINE_GAME_ASSETS="${_gameDir}/Assets"
    )

    add_dependencies(${_gameTarget} OpenGLEngineAssets)

    message(STATUS "Game executable: ${_gameTarget} (${_gameName})")
endforeach()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/GameRegistry.h.in"
    "${CMAKE_BINARY_DIR}/generated/GameRegistry.h"
    @ONLY
)
target_include_directories(OpenGLEngineCore PUBLIC "${CMAKE_BINARY_DIR}/generated")
