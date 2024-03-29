# We might need to support modle here, so always use the latest possible stable version
cmake_minimum_required(VERSION 3.16)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(Raytracing
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_program(CCACH_PATH ccache)
if(CCACH_PATH)
    message(STATUS "Using ccache at ${CCACH_PATH}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACH_PATH})
else()
    message(STATUS "Can't find ccache")
endif()

# link time optimization / interprocedual optimization
option(IPO "Enable linke time optimization (might significantly increase build time)" ON)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
    message(STATUS "IPO: ${IPO}")
else()
    set(IPO OFF)
    message(STATUS "IPO isn't supported: <${IPO_ERROR}>")
endif()

add_executable(Raytracing)

if (IPO)
    set_property(TARGET Raytracing PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_compile_definitions(Raytracing
    PRIVATE
        ROOT_PATH="${PROJECT_SOURCE_DIR}/"
        BINARY_PATH="${PROJECT_BINARY_DIR}/"
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Raytracing 
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
            -Wno-padded
            -ffast-math 
            $<$<CONFIG:Debug>:-fstandalone-debug>
            $<$<CONFIG:RelWithDebInfo>:-fstandalone-debug>
    )
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

file(GLOB_RECURSE 
   CPP_SOURCE_FILE
       "${PROJECT_SOURCE_DIR}/src/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/*.h"
       "${PROJECT_SOURCE_DIR}/src/asset/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/asset/*.h"
       "${PROJECT_SOURCE_DIR}/src/raytracer/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/raytracer/*.h"
       "${PROJECT_SOURCE_DIR}/src/renderer/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/renderer/*.h"
       "${PROJECT_SOURCE_DIR}/src/utils/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/utils/*.h"
       "${PROJECT_SOURCE_DIR}/src/vulkan/*.cpp"
       "${PROJECT_SOURCE_DIR}/src/vulkan/*.h"
)

target_sources(Raytracing
   PRIVATE
       ${CPP_SOURCE_FILE}
)

target_include_directories(Raytracing
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/third_party/glfw/include
        ${PROJECT_SOURCE_DIR}/third_party/glm
        ${PROJECT_SOURCE_DIR}/third_party/stb
        ${PROJECT_SOURCE_DIR}/third_party/fmt/include
        ${PROJECT_SOURCE_DIR}/third_party/vma/include
        ${PROJECT_SOURCE_DIR}/third_party/tinyobj
)

set(BUILD_SHARED_LIBS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(GLFW_VULKAN_STATIC OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glfw)

set(FMT_PEDANTIC OFF)
set(FMT_WERROR OFF)
set(FMT_DOC OFF)
set(FMT_INSTALL OFF)
set(FMT_TEST OFF)
set(FMT_FUZZ OFF)
set(FMT_CUDA_TEST OFF)
set(FMT_OS ON)
set(FMT_MODULE OFF)
set(FMT_SYSTEM_HEADERS OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/fmt)

add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glm)

set(JSON_BuildTests OFF)
set(JSON_Instal OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/json)

target_link_libraries(Raytracing
    PRIVATE
        glfw
        glm::glm
        fmt::fmt
        nlohmann_json::nlohmann_json
)

file(GLOB_RECURSE 
    GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

file(GLOB_RECURSE
    GLSL_INCLUDE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.glsl"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND glslc --target-env=vulkan -g ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(compile_shaders
    DEPENDS ${SPIRV_BINARY_FILES}
    DEPENDS ${GLSL_INCLUDE_FILES}
)

add_dependencies(Raytracing compile_shaders)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    target_include_directories(Raytracing
        PRIVATE
            $ENV{VULKAN_SDK}/Include
    )
else()
    add_custom_target(link_compile_database 
        ALL
            COMMAND 
                ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json
    )

    add_custom_target(link_executable 
        ALL
            COMMAND 
                ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/Raytracing ${PROJECT_SOURCE_DIR}/Raytracing
    )
endif()

