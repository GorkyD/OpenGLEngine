cmake_minimum_required(VERSION 3.16)
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

function(_ogle_detect_wwise_sdk out_var)
    if(DEFINED ENV{WWISESDK} AND EXISTS "$ENV{WWISESDK}/include/AK/AkPlatforms.h")
        set(${out_var} "$ENV{WWISESDK}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{WWISEROOT} AND EXISTS "$ENV{WWISEROOT}/SDK/include/AK/AkPlatforms.h")
        set(${out_var} "$ENV{WWISEROOT}/SDK" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{WWISE_SDK_DIR} AND EXISTS "$ENV{WWISE_SDK_DIR}/include/AK/AkPlatforms.h")
        set(${out_var} "$ENV{WWISE_SDK_DIR}" PARENT_SCOPE)
        return()
    endif()

    set(_known_installs_json "")
    if(WIN32)
        set(_known_installs_json "$ENV{APPDATA}/WwiseLauncher/Json/knownInstalls.json")
    elseif(APPLE)
        set(_known_installs_json "$ENV{HOME}/Library/Application Support/WwiseLauncher/Json/knownInstalls.json")
    else()
        set(_known_installs_json "$ENV{HOME}/.config/WwiseLauncher/Json/knownInstalls.json")
    endif()

    if(EXISTS "${_known_installs_json}")
        file(READ "${_known_installs_json}" _known_installs_contents)
        string(REGEX MATCHALL "\"[^\"]*\"" _known_install_entries "${_known_installs_contents}")
        set(_best "")
        foreach(_entry ${_known_install_entries})
            string(REGEX REPLACE "^\"|\"$" "" _install_path "${_entry}")
            string(REPLACE "\\\\" "/" _install_path "${_install_path}")
            if(EXISTS "${_install_path}/SDK/include/AK/AkPlatforms.h")
                set(_best "${_install_path}/SDK")
            endif()
        endforeach()
        if(_best)
            set(${out_var} "${_best}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

_ogle_detect_wwise_sdk(WWISE_SDK_DIR_AUTODETECTED)

set(WWISE_SDK_DIR "${WWISE_SDK_DIR_AUTODETECTED}" CACHE PATH "Path to Wwise SDK root (auto-detected if available)")

set(OPENGLENGINE_ENABLE_WWISE_DEFAULT OFF)
if(WWISE_SDK_DIR_AUTODETECTED)
    set(OPENGLENGINE_ENABLE_WWISE_DEFAULT ON)
endif()
option(OPENGLENGINE_ENABLE_WWISE "Enable Wwise integration" ${OPENGLENGINE_ENABLE_WWISE_DEFAULT})

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

if(OPENGLENGINE_ENABLE_WWISE AND WWISE_SDK_ROOT)
    message(STATUS "Wwise SDK found: ${WWISE_SDK_ROOT}")
elseif(OPENGLENGINE_ENABLE_WWISE)
    message(WARNING "OPENGLENGINE_ENABLE_WWISE is ON but no valid Wwise SDK was found (set WWISE_SDK_DIR or the WWISESDK env var)")
endif()

function(_ogle_detect_wwise_project out_var)
    if(DEFINED ENV{WWISE_PROJECT_DIR} AND EXISTS "$ENV{WWISE_PROJECT_DIR}")
        set(${out_var} "$ENV{WWISE_PROJECT_DIR}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{WWISEROOT})
        set(_guess "$ENV{WWISEROOT}/../Projects/${PROJECT_NAME}")
        if(EXISTS "${_guess}")
            set(${out_var} "${_guess}" PARENT_SCOPE)
            return()
        endif()
    endif()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

_ogle_detect_wwise_project(WWISE_PROJECT_DIR_AUTODETECTED)
set(WWISE_PROJECT_DIR "${WWISE_PROJECT_DIR_AUTODETECTED}" CACHE PATH
    "Path to the Wwise project folder that contains GeneratedSoundBanks/ (auto-detected if available)")

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
    OpenGLEngine/Engine/source/Render/Font.cpp
    OpenGLEngine/Engine/source/Window/Window.cpp
    OpenGLEngine/Engine/source/Input/InputSystem.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsSystems.cpp
    OpenGLEngine/Engine/source/Ecs/Core/EcsWorld.cpp
    OpenGLEngine/Engine/source/Resource/ModelLoader.cpp
    OpenGLEngine/Engine/source/Resource/MeshFactory.cpp
    OpenGLEngine/Engine/source/Resource/EntityFactory.cpp
    OpenGLEngine/Engine/source/Save/SaveService.cpp
    OpenGLEngine/Engine/source/Scene/DemoScene.cpp
)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/OpenGLEngine/Game/*.cpp"
)

file(GLOB GAME_ENTRIES CONFIGURE_DEPENDS LIST_DIRECTORIES true
    "${CMAKE_SOURCE_DIR}/OpenGLEngine/Game/*"
)
set(GAME_INCLUDE_DIRS "")
foreach(_entry ${GAME_ENTRIES})
    if(IS_DIRECTORY "${_entry}")
        list(APPEND GAME_INCLUDE_DIRS "${_entry}")
    endif()
endforeach()

add_executable(OpenGLEngine ${ENGINE_SOURCES} ${GAME_SOURCES})

target_include_directories(OpenGLEngine PRIVATE
    OpenGLEngine/Engine/include
    OpenGLEngine/Engine
    OpenGLEngine/Game
    ${GAME_INCLUDE_DIRS}
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
    if(WIN32)
        target_link_libraries(OpenGLEngine PRIVATE ws2_32 winmm)
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

foreach(_entry ${GAME_ENTRIES})
    if(IS_DIRECTORY "${_entry}" AND EXISTS "${_entry}/Assets")
        add_custom_command(TARGET OpenGLEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_entry}/Assets"
                $<TARGET_FILE_DIR:OpenGLEngine>/Assets
        )
    endif()
endforeach()
