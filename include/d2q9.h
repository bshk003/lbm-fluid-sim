#ifndef D2Q9_H
#define D2Q9_H

#include <vector>
#include <array>
#include <map>
#include <cmath>
#include "lbm.h"

class D2Q9: public LBM<2>
{
    public:
        using CellState = std::array<double, 9>;
        using VelocityVec = std::array<double, 2>;

        struct InitialConditions {
            std::vector<CellType> cell_type;
            std::vector<double> initial_rho;
            std::vector<VelocityVec> initial_u;
        };
    
        D2Q9(size_t width, size_t height, double tau);
        D2Q9(size_t width, size_t height, 
             const std::array<bool, 2> is_periodic,
             double tau, 
             const InitialConditions& ic);
        
        const std::vector<double>& get_density() const override;
        const std::vector<VelocityVec>& get_velocity() const override;

    private:
        // D2Q9 population arrays
        std::vector<CellState> m_f;
        std::vector<CellState> m_f_new;

        // Lists of special cells for boundary conditions
        std::vector<std::pair<size_t, size_t>> m_solid_cells;
        std::vector<std::pair<size_t, size_t>> m_inflow_cells;
        std::vector<std::pair<size_t, size_t>> m_outflow_cells;
        
        // The velocity and density for the inflow cells
        std::map<std::pair<size_t, size_t>, std::pair<VelocityVec, double>> m_inflow_conditions;
        
        std::array<bool, 2> m_is_periodic {false, false}; 

        void collide() override;
        void stream() override;
        void compute_macroscopic() override;
        void apply_boundary() override;
        
        // The equilibrium state for a single cell given macroscopic variables
        CellState compute_equilibrium(double rho, const VelocityVec& u);

        // A helper: coords to index
        size_t coords_to_index(int x, int y) const { return static_cast<size_t>(y * m_dimensions[0] + x); }

        // Constant parameters for D2Q9
        // The order: the center, 4 cardinals, 4 diagonals 
        static constexpr std::array<std::array<int, 2>, 9> m_directions 
            = {{{0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, 1}, {-1, -1}, {1, -1}}};    
        static constexpr CellState m_weights 
            = {4.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/9.0, 1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0};
        static constexpr std::array<int, 9> m_bounce_back_indices 
            = {0, 3, 4, 1, 2, 7, 8, 5, 6}; 
        static constexpr double m_csq = 1.0 / 3.0;

        static constexpr double MIN_DENSITY_THRESHOLD = 1e-7;
};

#endif


