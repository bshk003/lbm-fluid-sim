#include "tracers_collection.h"
#include "renderer.h"
#include "shaders.h"

TracersCollection::TracersCollection(const D2Q9& lbm, const TracersParams& tracers_params)
    : m_lbm(&lbm),
      m_grid_width(lbm.get_dimensions()[0]),
      m_grid_height(lbm.get_dimensions()[1]),
      m_emission_rate(tracers_params.emission_rate),
      m_num_tracers(tracers_params.random_initial),
      m_rng(std::random_device{}())
{
    std::vector<size_t> fluid_cell_indices = m_lbm -> get_fluid_cells();

    std::shuffle(fluid_cell_indices.begin(), fluid_cell_indices.end(), m_rng);
    size_t tracers_counter = std::min(m_num_tracers, fluid_cell_indices.size());
    
    // Add randomly placed tracers
    for (size_t i = 0; i < tracers_counter; i++)
    {
        auto coords = m_lbm -> index_to_coords(fluid_cell_indices[i]);
        m_positions.push_back({static_cast<float>(coords[0]), 
                               static_cast<float>(coords[1])});
    }
    // Add custom tracers from the initial data
    for (size_t idx: tracers_params.initial_tracers)
    {
        auto coords = m_lbm -> index_to_coords(idx);
        m_positions.push_back({static_cast<float>(coords[0]), 
                               static_cast<float>(coords[1])});
    }

    // Initialize the GL data
    init(tracers_params);
}

TracersCollection::~TracersCollection()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void TracersCollection::init(const TracersParams& tracers_params)
{
    GLuint vshader = compile_shader(tracer_vertex_shader_src, GL_VERTEX_SHADER);
    GLuint fshader = compile_shader(tracer_fragment_shader_src, GL_FRAGMENT_SHADER);
    m_shader_program = create_program(vshader, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER, 
                 m_positions.size() * sizeof(float) * 2,
                 m_positions.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); // location=0 in shader
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    
    // Set up the constant parameters for the tracers shader program
    glUseProgram(m_shader_program);

    GLint gridSizeLoc = glGetUniformLocation(m_shader_program, "uGridSize");
    glUniform2f(gridSizeLoc, (float)m_grid_width, (float)m_grid_height);

    GLint screenSizeLoc = glGetUniformLocation(m_shader_program, "uScreenSize");
    glUniform2f(screenSizeLoc, (float)m_grid_width, (float)m_grid_height);

    // Tracer size + color
    GLint pointSizeLoc = glGetUniformLocation(m_shader_program, "uPointSize");
    glUniform1f(pointSizeLoc, tracers_params.size);

    GLint colorLoc = glGetUniformLocation(m_shader_program, "uTracerColor");
    glUniform4f(colorLoc,
                tracers_params.color[0],
                tracers_params.color[1],
                tracers_params.color[2],
                tracers_params.color[3]);
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(0); // unbind for safety
}

void TracersCollection::update_positions() 
{
    for (size_t i = 0; i < m_positions.size(); ) 
        {
            // Get the integer coordinates of the cell the tracer is in
            int x_int = static_cast<int>(m_positions[i][0]);
            int y_int = static_cast<int>(m_positions[i][1]); 
            size_t idx = m_lbm -> coords_to_index(x_int, y_int);       

            // Get the velocity at that cell
            const auto& velocity = m_lbm -> get_velocity()[idx];

            // Update the tracer's position
            m_positions[i][0] += static_cast<float>(velocity[0] * 5);
            m_positions[i][1] += static_cast<float>(velocity[1] * 5);

            if (m_lbm -> is_periodic(0))
            {
                if (m_positions[i][0] < 0)
                    m_positions[i][0] += m_grid_width;
                else if (m_positions[i][0] >= m_grid_width)
                    m_positions[i][0] -= m_grid_width;
            }
            if (m_lbm -> is_periodic(1))
            {
                if (m_positions[i][1] < 0)
                    m_positions[i][1] += m_grid_height;
                else if (m_positions[i][1] >= m_grid_height)
                    m_positions[i][1] -= m_grid_height;
            }

            // Delete a tracer if it gets out of the grid
            if (!(0 <= m_positions[i][0] && m_positions[i][0] < m_grid_width &&
                  0 <= m_positions[i][1] && m_positions[i][1] < m_grid_height) ||
                  m_lbm -> get_cell_type(idx) == CellType::OUTFLOW)
            {
                m_positions[i] = m_positions.back();
                m_positions.pop_back();
            }
            else
            {
                // Regular update. Move on to the next tracer
                i++;
            }
        }
}

void TracersCollection::emit_tracers()
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r;

    for (size_t inflow_idx: m_lbm -> get_inflow_cells())
    {
        r = dist(m_rng);
        if (r < m_emission_rate)
        {
            auto coords = m_lbm -> index_to_coords(inflow_idx);
            m_positions.push_back({static_cast<float>(coords[0]), 
                                   static_cast<float>(coords[1])});
        }
    }
}


void TracersCollection::render_tracers() 
{
    glUseProgram(m_shader_program);

    // Bind VBO and update its data
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 
                 m_positions.size() * sizeof(float) * 2,
                 m_positions.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, m_positions.size());
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}