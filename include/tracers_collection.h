#ifndef TRACERS_COLLECTION_H
#define TRACERS_COLLECTION_H

#include <algorithm>
#include <random>
#include <cstdlib>
#include <array>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "d2q9.h"

struct TracersParams
{
    std::array<float, 4> color;
    float size;
    float emission_rate;
    size_t random_initial;
    std::vector<size_t> initial_tracers;
};

class TracersCollection
{
    public:
        TracersCollection(const D2Q9& lbm, const TracersParams& tracers_params);
        ~TracersCollection();

        void update_positions();
        void emit_tracers();
        void render_tracers();    
    
    private:
        GLuint m_vao, m_vbo, m_shader_program;
        size_t m_num_tracers;
        const D2Q9* m_lbm;
        size_t m_grid_width, m_grid_height;
        float m_emission_rate;
        std::vector<std::array<float, 2>> m_positions;
        std::mt19937 m_rng;

        void init(const TracersParams& tracers_params);
};

#endif