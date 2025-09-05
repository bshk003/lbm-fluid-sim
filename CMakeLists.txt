cmake_minimum_required(VERSION 3.10)
project(lbm-fluid-sim)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(lbm-fluid-sim src/main.cpp src/renderer.cpp src/d2q9.cpp src/simulation_setup.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(lbm-fluid-sim PRIVATE OpenGL::GL glfw GLEW::GLEW)
