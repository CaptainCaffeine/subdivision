# Specify includes relative to the src directory
include_directories(.)

set(RENDERER_SOURCES
    renderer/Init.cpp
    renderer/Render.cpp
    renderer/Debug.cpp
    renderer/Shader.cpp
    renderer/Camera.cpp
    renderer/Input.cpp
    renderer/Mesh.cpp)

set(RENDERER_HEADERS
    renderer/Init.h
    renderer/Render.h
    renderer/Shader.h
    renderer/Camera.h
    renderer/Input.h
    renderer/Mesh.h)

#set(SUBDIVISION_SOURCES
#    subdivision/XX.cpp)
#
#set(SUBDIVISION_HEADERS
#    subdivision/XX.h)

add_executable(subdivision main.cpp ${RENDERER_SOURCES}
                                    ${RENDERER_HEADERS})

target_link_libraries(subdivision glfw ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARIES})
