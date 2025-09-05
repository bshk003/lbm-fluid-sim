#ifndef RENDERER_H
#define RENDERER_H  

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Renderer 
{  
public:
    Renderer (int width, int height, int grid_width, int grid_height);
    ~Renderer();

    
    void render(std::vector<float>& density);

    const bool should_close() const { return glfwWindowShouldClose(m_window); }
    void poll_events() { glfwPollEvents(); }
    void wait_events() { glfwWaitEvents(); }

private:
    int m_width, m_height, m_grid_width, m_grid_height;
    GLFWwindow* m_window;
    GLuint m_shader_program, m_textureID;
    GLuint m_vao, m_ebo, m_vbo;

    void init();
    GLuint compile_shader(const char* shader_src, GLenum type);
    GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
};

#endif // RENDERER_H