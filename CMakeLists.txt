cmake_minimum_required(VERSION 3.15)
project(PeanutCracker LANGUAGES CXX C)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PeanutCracker/dependencies/assimp/lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

file(GLOB_RECURSE IMGUI_SOURCES 
    "PeanutCracker/dependencies/imgui/*.cpp"
)

add_executable(PeanutCracker 
    "PeanutCracker/src/main.cpp"
    "PeanutCracker/src/stb.cpp"
    "PeanutCracker/dependencies/glad/src/glad.c"
    ${IMGUI_SOURCES}
    "PeanutCracker/src/headers/assetManager.h"
    "PeanutCracker/src/headers/ray.h"
    "PeanutCracker/src/ImGuizmo.cpp"
)

target_include_directories(PeanutCracker PRIVATE
    "PeanutCracker/src"
    "PeanutCracker/dependencies/glm"
    "PeanutCracker/dependencies/glad/include"
    "PeanutCracker/dependencies/stb_image"
    "PeanutCracker/dependencies/imgui"
    "PeanutCracker/dependencies/imgui/backends"
    #"PeanutCracker/dependencies/imguizmo"
    "PeanutCracker/dependencies/assimp/include"
)

target_link_libraries(PeanutCracker PRIVATE 
    glfw 
    opengl32
    
    # Handle ZLib (Debug vs Release)
    debug     "${ASSIMP_DIR}/zlibstaticd.lib"
    optimized "${ASSIMP_DIR}/zlibstatic.lib"
    
    # Handle Assimp (Debug vs Release)
    debug     "${ASSIMP_DIR}/assimp-vc143-mtd.lib"
    optimized "${ASSIMP_DIR}/assimp-vc143-mt.lib"
)

set(EXECUTABLE_OUTPUT_DIR "$<TARGET_FILE_DIR:PeanutCracker>")

# Define the source folder for the assets
set(SHADERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/PeanutCracker/defaultshaders")

# Add a command to execute AFTER the 'PeanutCracker' target is built.
add_custom_command(
    TARGET PeanutCracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${SHADERS_SOURCE_DIR}" 
        "${EXECUTABLE_OUTPUT_DIR}/defaultshaders"
    COMMENT "Copying defaultshaders assets to build directory"
)