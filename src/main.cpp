#include "renderer.h"
#include "d2q9.h"
#include "simulation_setup.h"
#include <iostream>
#include <vector>
#include <array>
#include <cmath>

int main() {
    const size_t GRID_WIDTH = 300;
    const size_t GRID_HEIGHT = 100;
    double viscosity = 0.02;
    const double TAU = 3.0 * viscosity + 0.5; // Relaxation time
    const size_t TOTAL_CELLS = GRID_WIDTH * GRID_HEIGHT;

    D2Q9::InitialConditions ic = create_initial_conditions(GRID_WIDTH, GRID_HEIGHT);
    try {
        D2Q9 lbm(GRID_WIDTH, GRID_HEIGHT, {true, true}, TAU, ic);
        Renderer renderer(800, 400, GRID_WIDTH, GRID_HEIGHT);
        std::vector<float> render_field(TOTAL_CELLS);
        
        
        std::cout << "Starting LBM simulation..." << std::endl;
        std::cout << "Press ESC or close window to exit" << std::endl;

        // Main LBM simulation loop
        
        double curl = 0.0;
        while (!renderer.should_close()) {
            for (int i = 0; i < 3; ++i) // LBM steps per frame
                lbm.step();
            
            // Prepare the velocity magnitude field for rendering
            const auto& current_velocity = lbm.get_velocity();
            const auto& current_density = lbm.get_density();
            
            for (size_t i = 0; i < TOTAL_CELLS; ++i) {
                //curl = std::abs(current_velocity[i][0] - current_velocity[i][1]);
                double speed = std::hypot(current_velocity[i][0], current_velocity[i][1]);
                render_field[i] = speed / 0.1; //std::min(speed / max_speed_for_render, 1.0f);
                //render_field[i] = (current_density[i]-0.7); // Visualize x-velocity component
                //std::cout << current_density[i] << std::endl;
                //render_field[i] = curl;
            }

            renderer.render(render_field);
        }
        
        std::cout << "Simulation completed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
