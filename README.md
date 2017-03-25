Matthew Murray
V00800801

Build Instructions
==================

    sudo apt-get install cmake libglfw3-dev libglew-dev

From source directory:

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

Usage Instructions
==================
From build directory:

    ./src/subdivision

Use WASD to strafe the camera horizontally and vertically. Click and drag on the viewport to look around with the camera.
