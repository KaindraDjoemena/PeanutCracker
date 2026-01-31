
# PeanutCracker

**A low-level 3D renderer built from scratch using C++17 and OpenGL 3.3 (Core Profile)**

This project was built to understand the low level graphics pipeline by designing and implementing
the elements of a real-time 3D rendering engine. Prioritizing code structure and performance, while generating quality images

## Installation
#### Prerequisites
- `Windows 10/11`
- `Visual Studio 2019/2022` (or compatible C++ compiler)
- `CMake 3.15+`

#### Build & Run
```bash
# clone
git clone --recursive https://github.com/KaindraDjoemena/PeanutCracker.git

# build
cd PeanutCracker\
cmake -B build
cmake --build build --config Release --parallel

# run
cd build\Release
.\PeanutCracker.exe
```
    
## Tools & Libraries
| Library | Usage |
| :--- | :--- |
| **[GLFW](https://github.com/glfw/glfw)** | Window creation, OpenGL context & input handling |
| **[GLAD](https://github.com/Dav1dde/glad)** | OpenGL function loader |
| **[GLM](https://github.com/g-truc/glm)** | Matrix and vector operations |
| **[Dear ImGui](https://github.com/ocornut/imgui)** | Debug tools & editor interface |
| **[ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)** | 3D gizmos |
| **[Assimp](https://github.com/assimp/assimp)** | Asset import pipeline (.obj, .fbx) |
| **[stb_image](https://github.com/nothings/stb)** | Image/texture decoding |

## License
[MIT](https://choosealicense.com/licenses/mit/)

