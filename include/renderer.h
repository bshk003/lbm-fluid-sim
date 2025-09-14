#ifndef RENDERER_H
#define RENDERER_H  

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Renderer 
{  
public:
    Renderer (size_t width, size_t height, size_t grid_width, size_t grid_height);
    ~Renderer();
    
    void render(const std::vector<float>& scalar_field, 
                const std::vector<float>& obstacle_mask);

    const bool should_close() const { return glfwWindowShouldClose(m_window); }
    void poll_events() { glfwPollEvents(); }
    void wait_events() { glfwWaitEvents(); }
    GLFWwindow* get_window() { return m_window; }

private:
    size_t m_width, m_height, m_grid_width, m_grid_height;
    GLFWwindow* m_window;
    GLuint m_shader_program, m_fieldTex, m_obstacleTex;
    GLuint m_vao, m_ebo, m_vbo;

    void init();
};

// Helper functions

GLuint compile_shader(const char* shader_src, GLenum type);
GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);

#endif 