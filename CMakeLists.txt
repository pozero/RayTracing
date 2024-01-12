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

function(COMPILATION_CONFIG TARGET)
    if (IPO)
        set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    target_compile_definitions(${TARGET}
        PRIVATE
            ROOT_PATH="${PROJECT_SOURCE_DIR}"
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${TARGET} 
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
                $<$<CONFIG:Debug>:-fstandalone-debug>
                $<$<CONFIG:RelWithDebInfo>:-fstandalone-debug>
        )
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

add_executable(Raytracing)

target_sources(Raytracing
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/main.cpp
        ${PROJECT_SOURCE_DIR}/src/glad_impl.cpp
        ${PROJECT_SOURCE_DIR}/src/shader.cpp
        ${PROJECT_SOURCE_DIR}/src/shader.h
)

target_include_directories(Raytracing
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/third_party/glfw/include
        ${PROJECT_SOURCE_DIR}/third_party/glad/include
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/glfw)

target_link_libraries(Raytracing
    PRIVATE
        glfw
)

compilation_config(Raytracing)

ADD_CUSTOM_TARGET(link_compile_database 
    ALL
        COMMAND 
            ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json
)

ADD_CUSTOM_TARGET(link_executable 
    ALL
        COMMAND 
            ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/Raytracing ${PROJECT_SOURCE_DIR}/Raytracing
)
