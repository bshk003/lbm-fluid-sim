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

    LBM(const std::vector<size_t>& dimensions, double tau) : m_dimensions(dimensions), m_tau(tau), m_inv_tau(1.0f / m_tau)
    {
        if (dimensions.size() != N_DIM)
            throw std::runtime_error("Wrong dimensions count.");

        m_total_size = std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<size_t>());
    };    
    virtual ~LBM() = default;

    // A single LBM evolution step
    void step()
    {   
        // The pull scheme order
        compute_macroscopic();
        collide();
        stream();  
        apply_boundary();
 
    }

    virtual const std::vector<double>& get_density() const = 0;
    virtual const std::vector<VelocityVec>& get_velocity() const = 0;

protected:
    // Constant parameters
    std::vector<size_t> m_dimensions;
    size_t m_total_size;
    double m_tau;
    double m_inv_tau;

    // Domain geometry
    std::vector<CellType> m_cell_type;

    // Macroscopic variables
    std::vector<double> m_rho; // density
    std::vector<VelocityVec> m_u; // velocity    
    
    virtual void collide() = 0;
    virtual void stream() = 0;
    virtual void compute_macroscopic() = 0;
    virtual void apply_boundary() = 0;
};

#endif