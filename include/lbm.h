#ifndef LBM_H
#define LBM_H

#include <vector>
#include <array>
#include <functional>
#include <numeric>
#include <stdexcept>

enum CellType {FLUID, SOLID, INFLOW, OUTFLOW};

// An abstract class for an N_DIM-ensional LBM automaton 
template <size_t N_DIM>
class LBM
{
public:
    using VelocityVec = std::array<double, N_DIM>;   

    struct LBMParams 
    {
        std::vector<size_t> dimensions;
        std::array<bool, N_DIM> is_periodic; 
        double tau;
    };

    LBM(const LBMParams& params) : m_dimensions(params.dimensions), 
                                   m_is_periodic(params.is_periodic), 
                                   m_tau(params.tau),
                                   m_inv_tau(1.0 / m_tau)
    {
        if (m_dimensions.size() != N_DIM)
            throw std::runtime_error("Dimension count mismatch in LBMParams.");

        m_total_size = std::accumulate(m_dimensions.begin(), m_dimensions.end(), 1, std::multiplies<size_t>());
    }   

    virtual ~LBM() = default;

    // A single LBM evolution step
    void step()
    {   
        // The pull scheme order
        collide();
        stream();  
        apply_cell_conditions();
        compute_macroscopic(); 
    }

    virtual const std::vector<double>& get_density() const = 0;
    virtual const std::vector<VelocityVec>& get_velocity() const = 0;
    size_t get_total_size() const { return m_total_size; }
    const std::vector<size_t>& get_dimensions() const { return m_dimensions; }
    bool is_periodic(size_t dim) const { return (dim < N_DIM)? m_is_periodic[dim]: false; }
    CellType get_cell_type(size_t idx) const { return m_cell_type[idx]; }

protected:
    // The relaxation time and its inverse
    const double m_tau;
    const double m_inv_tau;

    // Domain geometry and cell types distribution
    const std::vector<size_t> m_dimensions;
    const std::array<bool, N_DIM> m_is_periodic;
    size_t m_total_size;
    std::vector<CellType> m_cell_type;

    // Macroscopic variables
    std::vector<double> m_rho; // density
    std::vector<VelocityVec> m_u; // velocity    
    
    virtual void collide() = 0;
    virtual void stream() = 0;
    virtual void apply_cell_conditions() = 0;
    virtual void compute_macroscopic() = 0;
};

#endif