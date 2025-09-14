#include "renderer.h"
#include <iostream>
#include <stdexcept>
#include "shaders.h"

Renderer::Renderer(size_t width, size_t height, size_t grid_width, size_t grid_height) 
    : m_width(width), m_height(height), m_grid_width(grid_width), m_grid_height(grid_height)
{
    if (!glfwInit()) { throw std::runtime_error("Failed to initialize GLFW"); }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "LBM Renderer", nullptr, nullptr);
    if (!m_window) 
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create a GLFW window");
    }

    init();
}

Renderer::~Renderer()
{
    glDeleteTextures(1, &m_fieldTex);
    glDeleteTextures(1, &m_obstacleTex);
    glDeleteProgram(m_shader_program);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}


void Renderer::init()
{    
    glfwMakeContextCurrent(m_window);
    glewInit();
    glViewport(0, 0, m_width, m_height);
    
    // Compile and link shaders
    GLuint vertex_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(fragment_shader_src, GL_FRAGMENT_SHADER);
    m_shader_program = create_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // A full-screen quad. The scalar field texture will be mapped onto this
    float vertices[] = {
        -1.0f,  1.0f,        0.0f, 1.0f, // top left
        -1.0f, -1.0f,        0.0f, 0.0f, // bottom left
         1.0f, -1.0f,        1.0f, 0.0f, // bottom right
         1.0f,  1.0f,        1.0f, 1.0f  // top right
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // Allocate memory for vertices and send data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Allocate memory for indices and send data   
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    int stride = 4 * sizeof(float);

    // How to interpret the vertex data: position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // How to interpret the vertex data: texcoords (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Scalar field texture
    glGenTextures(1, &m_fieldTex);
    glBindTexture(GL_TEXTURE_2D, m_fieldTex);
    // Minimization/magnification filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Allocate texture memory; "GL_R32F" -- use a single float channel per pixel
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_grid_width, m_grid_height, 0, GL_RED, GL_FLOAT, nullptr);

    // Obstacle mask texture
    glGenTextures(1, &m_obstacleTex);
    glBindTexture(GL_TEXTURE_2D, m_obstacleTex);
    // Minimization/magnification filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Allocate texture memory; "GL_R32F" -- use a single float channel per pixel
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_grid_width, m_grid_height, 0, GL_RED, GL_FLOAT, nullptr);
    
}

void Renderer::render(const std::vector<float>& scalar_field, 
                      const std::vector<float>& obstacle_mask)
{
    if (scalar_field.size() != m_grid_width * m_grid_height) 
        throw std::runtime_error("Grid dimensions do not match the scalar field size");

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shader_program);

    // Update scalar field texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fieldTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_grid_width, m_grid_height,
                    GL_RED, GL_FLOAT, scalar_field.data());
    glUniform1i(glGetUniformLocation(m_shader_program, "scalarTex"), 0);

    // Update obstacle mask texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_obstacleTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_grid_width, m_grid_height,
                    GL_RED, GL_FLOAT, obstacle_mask.data());
    glUniform1i(glGetUniformLocation(m_shader_program, "obstacleTex"), 1);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    //glfwSwapBuffers(m_window);
    glfwPollEvents();
}

// Helpers: shader compilation, program linking

GLuint compile_shader(const char* shader_src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shader_src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        throw std::runtime_error(std::string("Shader compilation error: ") + infoLog);
    }
    return shader;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) 
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        throw std::runtime_error(std::string("Program linking error: ") + infoLog);
    }
    return program;
}