An attempt at implementing http://www.niessnerlab.org/papers/2016/4subdiv/brainerd2016efficient.pdf. It isn't finished, and the subdivision runs on the CPU instead of the GPU which was the goal of the paper. It was for a university project and I was under a lot of time pressure, so don't judge me too hard for the code.

This project uses [glm](https://github.com/g-truc/glm) and [tinyobjloader](https://github.com/syoyo/tinyobjloader).

Build Instructions
==================

Dependencies: glfw3, glew.
Has only been tested with GCC 6.2 and Clang 3.9.1.

From source directory:

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

Usage Instructions
==================

Use WASD to strafe the camera horizontally and vertically. Click and drag on the viewport to look around with the camera.
