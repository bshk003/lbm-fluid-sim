#ifndef D2Q9_H
#define D2Q9_H

#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <algorithm>
#include "lbm.h"

class D2Q9: public LBM<2>
{
    public:
        using CellState = std::array<double, 9>;
        using VelocityVec = std::array<double, 2>;

        struct InitialConditions 
        {
            std::vector<CellType> cell_type;
            std::vector<double> initial_rho;
            std::vector<VelocityVec> initial_u;
        };
    
        D2Q9(size_t width, size_t height, double tau);
        D2Q9(const LBMParams& lbm_params, 
             const InitialConditions& initials);
        
        const std::vector<double>& get_density() const override;
        const std::vector<VelocityVec>& get_velocity() const override;
        const std::vector<float>& get_obstacle_mask() const { return m_obstacle_mask; }
        const std::vector<size_t>& get_fluid_cells() const { return m_fluid_cells; }
        const std::vector<size_t>& get_inflow_cells() const { return m_inflow_cells; }
        
        // A helper: coords to index
        size_t coords_to_index(int x, int y) const 
        { 
            x = m_is_periodic[0] ? (x + m_dimensions[0]) % m_dimensions[0] 
                                 : std::clamp(x, 0, static_cast<int>(m_dimensions[0]) - 1);
            y = m_is_periodic[1] ? (y + m_dimensions[1]) % m_dimensions[1]
                                 : std::clamp(y, 0, static_cast<int>(m_dimensions[1]) - 1);

            return static_cast<size_t>(y * m_dimensions[0] + x); 
        }

        static size_t coords_to_index(int x, int y, const LBM<2>::LBMParams& params) 
        { 
            x = params.is_periodic[0] ? (x + params.dimensions[0]) % params.dimensions[0] 
                                 : std::clamp(x, 0, static_cast<int>(params.dimensions[0]) - 1);
            y = params.is_periodic[1] ? (y + params.dimensions[1]) % params.dimensions[1]
                                 : std::clamp(y, 0, static_cast<int>(params.dimensions[1]) - 1);

            return static_cast<size_t>(y * params.dimensions[0] + x);
        }
        
        // A helper: index to coords
        std::array<size_t, 2> index_to_coords(size_t idx) const
        {
            return {idx % m_dimensions[0], idx / m_dimensions[0]};
        }

        size_t get_neighbor_index(size_t dest_idx, const std::array<int, 2>& direction) const
        {
            // Convert linear index to (x, y)
            int nx = m_dimensions[0];
            int ny = m_dimensions[1];
            int x = dest_idx % nx;
            int y = dest_idx / nx;

            int src_x = x - direction[0];
            int src_y = y - direction[1];

            // Impose zero-gradient in non-periodic directions
            if (m_is_periodic[0])
                src_x = (src_x + nx) % nx;
            else
                src_x = std::clamp(src_x, 0, nx - 1);

            if (m_is_periodic[1])
                src_y = (src_y + ny) % ny;
            else
                src_y = std::clamp(src_y, 0, ny - 1);

            return src_y * nx + src_x;
        }

    private:
        // D2Q9 cell states
        std::vector<CellState> m_f;
        std::vector<CellState> m_f_new;

        // Lists of special cells for boundary conditions
        std::vector<size_t> m_fluid_cells;
        std::vector<size_t> m_solid_cells;
        std::vector<size_t> m_inflow_cells;
        std::vector<size_t> m_outflow_cells;
        
        // The velocity and density for the inflow cells
        std::map<size_t, std::pair<VelocityVec, double>> m_inflow_conditions;   

        // The density for the outflow cells
        std::map<size_t, double> m_outflow_conditions;
        
        // The obstacle bitmask for rendering
        std::vector<float> m_obstacle_mask;

        // The (x,y)-coordinates are to be flattened to indices
        std::vector<size_t> m_indices;

        void collide() override;
        void stream() override;
        void compute_macroscopic() override;
        void apply_cell_conditions() override;
        
        // The equilibrium state for a single cell given macroscopic variables
        CellState compute_equilibrium(double rho, const VelocityVec& u);

        // Constant parameters for D2Q9
        // The order: the center, 4 cardinals, 4 diagonals 
        static constexpr std::array<std::array<int, 2>, 9> m_directions 
            = {{{0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, 1}, {-1, -1}, {1, -1}}};    
        static constexpr CellState m_weights 
            = {4.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0};
        static constexpr std::array<size_t, 9> m_bounce_back_indices 
            = {0, 3, 4, 1, 2, 7, 8, 5, 6}; 
        static constexpr double m_csq = 1.0 / 3.0;
        static constexpr double m_inv_csq = 3.0; // The inverse of the speed of sound squared

        static constexpr double MIN_DENSITY_THRESHOLD = 1e-7;
};

D2Q9::InitialConditions sample_d2q9(const LBM<2>::LBMParams& params);

#endif


