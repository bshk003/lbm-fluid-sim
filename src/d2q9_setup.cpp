#include "d2q9_setup.h"
#include <vector>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <array>
#include <numeric>
#include <algorithm>


// Load domain geometry and simulation parameters
void load_from_binary(const std::string& filename, 
                      LBM<2>::LBMParams& lbm_params, 
                      D2Q9::InitialConditions& initials, 
                      VisualizationParams& visual_params,
                      std::vector<QuantityParams>& render_quant_params,
                      TracersParams& tracers_params) 
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) 
        throw std::runtime_error("Failed to open input file " + filename);

    uint64_t width_int, height_int, 
             render_window_width, render_window_height, steps_per_frame;
    int8_t periodic_x_int, periodic_y_int, n_quant;
    std::array<float, 4> tracers_color;
    double tau_double;

    // LBM params
    file.read(reinterpret_cast<char*>(&width_int), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&height_int), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&periodic_x_int), sizeof(int8_t));
    file.read(reinterpret_cast<char*>(&periodic_y_int), sizeof(int8_t));
    file.read(reinterpret_cast<char*>(&tau_double), sizeof(double));
    lbm_params.dimensions = {static_cast<size_t>(width_int), static_cast<size_t>(height_int)};
    lbm_params.is_periodic = {static_cast<bool>(periodic_x_int), static_cast<bool>(periodic_y_int)};
    lbm_params.tau = static_cast<double>(tau_double);

    // Visualization params
    file.read(reinterpret_cast<char*>(&render_window_width), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&render_window_height), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&steps_per_frame), sizeof(uint64_t));
    visual_params.width = static_cast<size_t>(render_window_width);
    visual_params.height = static_cast<size_t>(render_window_height);
    visual_params.steps_per_frame = static_cast<size_t>(steps_per_frame);

    // Quantities rendering params
    file.read(reinterpret_cast<char*>(&n_quant), sizeof(uint8_t));
    for (uint8_t i = 0; i < n_quant; i++) 
    {
        uint8_t quant_len;
        file.read(reinterpret_cast<char*>(&quant_len), sizeof(uint8_t));
        std::string quant_id(quant_len, '\0');
        file.read(&quant_id[0], quant_len);
        
        float offset, amplitude;
        file.read(reinterpret_cast<char*>(&offset), sizeof(float));
        file.read(reinterpret_cast<char*>(&amplitude), sizeof(float));

        render_quant_params.push_back({quant_id, offset, amplitude});
    }
    // Tracers params
    file.read(reinterpret_cast<char*>(&tracers_color), 4 * sizeof(float));
    tracers_params.color = tracers_color;
    file.read(reinterpret_cast<char*>(&tracers_params.size), sizeof(float));
    file.read(reinterpret_cast<char*>(&tracers_params.emission_rate), sizeof(float));
    file.read(reinterpret_cast<char*>(&tracers_params.random_initial), sizeof(uint64_t));

    // Load the initial conditions
    size_t total_size = lbm_params.dimensions[0] * lbm_params.dimensions[1];
    initials.cell_type.resize(total_size);
    initials.initial_rho.resize(total_size);
    initials.initial_u.resize(total_size);

    // Cell types
    std::vector<uint8_t> cell_type_raw(total_size);
    file.read(reinterpret_cast<char*>(cell_type_raw.data()), total_size * sizeof(uint8_t));
    for (size_t i = 0; i < total_size; ++i) 
        initials.cell_type[i] = static_cast<CellType>(cell_type_raw[i]);

    // Density
    file.read(reinterpret_cast<char*>(initials.initial_rho.data()), total_size * sizeof(double));

    // Velocity
    std::vector<double> u_x_raw(total_size);
    std::vector<double> u_y_raw(total_size);
    file.read(reinterpret_cast<char*>(u_x_raw.data()), total_size * sizeof(double));
    file.read(reinterpret_cast<char*>(u_y_raw.data()), total_size * sizeof(double));

    for (size_t i = 0; i < total_size; ++i) {
        initials.initial_u[i] = {u_x_raw[i], u_y_raw[i]};
    }

    // Initial tracers
    size_t num_initial_tracers;
    file.read(reinterpret_cast<char*>(&num_initial_tracers), sizeof(uint64_t));

    tracers_params.initial_tracers.resize(num_initial_tracers);
    file.read(reinterpret_cast<char*>(tracers_params.initial_tracers.data()), sizeof(uint64_t) * num_initial_tracers);
}


// Sample initial conditions
D2Q9::InitialConditions sample_d2q9(const LBM<2>::LBMParams& params)
{
    const D2Q9::VelocityVec u0 = {0.1, 0.0}; // Inflow velocity    
    D2Q9::InitialConditions initials;

    size_t total_cells = params.dimensions[0] * params.dimensions[1];

    initials.cell_type.resize(total_cells, CellType::FLUID);
    initials.initial_rho.resize(total_cells, 1.0);
    initials.initial_u.resize(total_cells, u0);

    //Inflow on the left boundary
    for (size_t y = 0; y < params.dimensions[1]; y++) 
        initials.cell_type[D2Q9::coords_to_index(0, y, params)] = CellType::INFLOW;

    // An obstacle
    for(size_t x = 35; x < 55; x++) 
        initials.cell_type[D2Q9::coords_to_index(x, 85-x, params)] = CellType::SOLID;    

    return initials;
}