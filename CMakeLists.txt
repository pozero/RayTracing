cmake_minimum_required(VERSION 3.10)

project(RayTracing)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RayTracing
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/VulkanHeader.h
    ${PROJECT_SOURCE_DIR}/src/VulkanLoad.cpp
    ${PROJECT_SOURCE_DIR}/src/VulkanDevice.h
    ${PROJECT_SOURCE_DIR}/src/VulkanDevice.cpp
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_program(SCCACH_PATH sccache)
if(SCCACH_PATH)
    message(STATUS "Using sccache at ${SCCACH_PATH}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACH_PATH})
else()
    message(STATUS "Can't find sccache")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if (IPO_SUPPORTED)
    set_property(TARGET RayTracing PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(RayTracing
        PRIVATE 
            -Werror
            -Weverything
            -Weffc++
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++11-compat
            -Wno-c++14-compat
            -Wno-c++17-compat
            -Wno-c++20-compat
            -Wno-c++20-extensions
            -Wno-old-style-cast
            -Wno-newline-eof
            -ffast-math 
            -fno-rtti
            -fmsc-version=0
            $<$<CONFIG:Debug>:-fstandalone-debug>
            $<$<CONFIG:RelWithDebInfo>:-fstandalone-debug>
    )
endif()

option(VK_DBG "Enable vulkan debug utility" OFF)

target_compile_definitions(RayTracing
    PRIVATE
        ROOT_PATH="${PROJECT_SOURCE_DIR}"
        $<$<BOOL:${VK_DBG}>:VK_DBG>
)

if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
    if (EXISTS "${PROJECT_SOURCE_DIR}/compile_commands.json")
        file(REMOVE ${PROJECT_SOURCE_DIR}/compile_commands.json)
    endif()
    file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json)
endif()

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_VULKAN_STATIC OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glfw)

option(FMT_PEDANTIC OFF)
option(FMT_WERROR OFF)
option(FMT_DOC OFF)
option(FMT_INSTALL OFF)
option(FMT_TEST OFF)
option(FMT_FUZZ OFF)
option(FMT_CUDA_TEST OFF)
option(FMT_OS ON)
option(FMT_MODULE OFF)
option(FMT_SYSTEM_HEADERS OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/fmt)

target_link_libraries(RayTracing
    PRIVATE
        glfw
        fmt::fmt
        $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib
)

target_include_directories(RayTracing
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/glfw/include
        ${PROJECT_SOURCE_DIR}/third_party/glad/include
        ${PROJECT_SOURCE_DIR}/third_party/fmt/include
        $ENV{VULKAN_SDK}/Include
)

