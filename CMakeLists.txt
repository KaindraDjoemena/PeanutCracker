cmake_minimum_required(VERSION 3.15)
project(PeanutCracker LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PeanutCracker/dependencies/assimp/lib")

add_subdirectory(PeanutCracker/dependencies/glfw)

set(IMGUI_SOURCES 
    "PeanutCracker/dependencies/imgui/imgui.cpp"
    "PeanutCracker/dependencies/imgui/imgui_demo.cpp"
    "PeanutCracker/dependencies/imgui/imgui_draw.cpp"
    "PeanutCracker/dependencies/imgui/imgui_tables.cpp"
    "PeanutCracker/dependencies/imgui/imgui_widgets.cpp"
    "PeanutCracker/dependencies/imgui/backends/imgui_impl_glfw.cpp"
    "PeanutCracker/dependencies/imgui/backends/imgui_impl_opengl3.cpp"
)

add_executable(PeanutCracker 
    "PeanutCracker/src/stb.cpp"
    "PeanutCracker/dependencies/glad/src/glad.c"
    "PeanutCracker/dependencies/imguizmo/ImGuizmo.cpp"
    ${IMGUI_SOURCES}
    "PeanutCracker/src/assetManager.cpp"
    "PeanutCracker/src/camera.cpp"
    "PeanutCracker/src/cubemap.cpp"
    "PeanutCracker/src/frustum.cpp"
    "PeanutCracker/src/gui.cpp"
    "PeanutCracker/src/light.cpp"
    "PeanutCracker/src/main.cpp"
    "PeanutCracker/src/mesh.cpp"
    "PeanutCracker/src/model.cpp"
    "PeanutCracker/src/object.cpp"
    "PeanutCracker/src/ray.cpp"
    "PeanutCracker/src/renderer.cpp"
    "PeanutCracker/src/scene.cpp"
    "PeanutCracker/src/sceneNode.cpp"
    "PeanutCracker/src/shader.cpp"
    "PeanutCracker/src/shadowCasterComponent.cpp"
    "PeanutCracker/src/sphereColliderComponent.cpp"
)

target_link_libraries(PeanutCracker PRIVATE 
    glfw
    opengl32
    gdi32 winmm user32 shell32
    debug     "${ASSIMP_DIR}/zlibstaticd.lib"
    optimized "${ASSIMP_DIR}/zlibstatic.lib"
    debug     "${ASSIMP_DIR}/assimp-vc143-mtd.lib"
    optimized "${ASSIMP_DIR}/assimp-vc143-mt.lib"
)

target_include_directories(PeanutCracker PRIVATE
    "PeanutCracker/src"
    "PeanutCracker/dependencies/glfw/include"
    "PeanutCracker/dependencies/glm"
    "PeanutCracker/dependencies/glad/include"
    "PeanutCracker/dependencies/stb_image"
    "PeanutCracker/dependencies/imgui"
    "PeanutCracker/dependencies/imgui/backends"
    "PeanutCracker/dependencies/imguizmo"
    "PeanutCracker/dependencies/assimp/include"
)

# Asset copying
add_custom_command(TARGET PeanutCracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_SOURCE_DIR}/PeanutCracker/defaultshaders" 
        "$<TARGET_FILE_DIR:PeanutCracker>/defaultshaders"
    COMMENT "Copying defaultshaders assets"
)