#include "d2q9.h"
#include <stdexcept>
#include <numeric>
#include <utility>
#include <iostream>

D2Q9::D2Q9(size_t width, size_t height, double tau): LBM<2>({width, height}, tau)
{    
    // A static, uniform density state for testing purposes
    m_f.resize(m_total_size);
    m_f_new.resize(m_total_size);

    m_cell_type.resize(m_total_size);
    std::fill(m_cell_type.begin(), m_cell_type.end(), CellType::FLUID);

    m_rho.resize(m_total_size);
    std::fill(m_rho.begin(), m_rho.end(), 1.0);
    std::fill(m_u.begin(), m_u.end(), VelocityVec{0.0, 0.0});

    for (size_t i = 0; i < m_total_size; i++)
    {
        m_f[i] = compute_equilibrium(m_rho[i], m_u[i]);
    }
}

D2Q9::D2Q9(size_t width, size_t height, 
           const std::array<bool, 2> is_periodic,
           double tau, 
           const InitialConditions& ic): LBM<2>({width, height}, tau)
{    
    if (m_total_size != ic.cell_type.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: cell type");
    }

    if (m_total_size != ic.initial_rho.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: density");
    }

    if (m_total_size != ic.initial_u.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: velocity");
    }

    m_is_periodic = is_periodic;
    m_cell_type = ic.cell_type;
    m_rho = ic.initial_rho;
    m_u = ic.initial_u;

    m_f.resize(m_total_size);
    m_f_new.resize(m_total_size);
    
    size_t idx;
    for (size_t y = 0; y < m_dimensions[1]; y++)
    {
        for (size_t x = 0; x < m_dimensions[0]; x++)
        {
            idx = coords_to_index(x, y);
            switch (m_cell_type[idx])
            {
                case CellType::SOLID:
                    m_solid_cells.push_back({x, y});
                    m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                    // m_f[idx] remains zero-initialized
                    break;
                case CellType::INFLOW:
                    m_inflow_cells.push_back({x, y});
                    m_inflow_conditions[{x, y}] = {m_u[idx], m_rho[idx]};
                    m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                    break;
                case CellType::OUTFLOW:
                    m_outflow_cells.push_back({x, y});
                    //m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                    break;
                default:
                    m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                    break;
            }
            //m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
        }
    }   
}

D2Q9::CellState D2Q9::compute_equilibrium(double rho, const VelocityVec& u)
{
    CellState f_eq;
    const double usq = u[0] * u[0] + u[1] * u[1];

    for (size_t dir = 0; dir < 9; dir++)
    {
        const double eu = m_directions[dir][0] * u[0] + m_directions[dir][1] * u[1];
        f_eq[dir] = m_weights[dir] * rho * (1.0 + (eu / m_csq) + ((eu * eu) / (2.0 * m_csq * m_csq)) - (usq / (2.0 * m_csq)));
    }
    return f_eq; 
}


void D2Q9::collide() 
{
    // The Bhatnagar-Gross-Krook (BGK) collision
    for (size_t idx = 0; idx < m_total_size; idx++)
    {
        if (m_cell_type[idx] == CellType::FLUID) {

        CellState f_eq = compute_equilibrium(m_rho[idx], m_u[idx]);
        for (size_t dir = 0; dir < 9; dir++)
            m_f[idx][dir] -= m_inv_tau * (m_f[idx][dir] - f_eq[dir]);
    }}
}


void D2Q9::stream() 
{
    size_t dest_idx, src_idx, opposite_dir;

    // The pull scheme with bounce-back for solids
    for (size_t y = 0; y < m_dimensions[1]; y++)
    {
        for (size_t x = 0; x < m_dimensions[0]; x++)
        {
            dest_idx = coords_to_index(x, y);
            for (size_t dir = 0; dir < 9; dir++)
            {
                int src_x = static_cast<int>(x) - m_directions[dir][0];
                int src_y = static_cast<int>(y) - m_directions[dir][1];

                if (m_is_periodic[0])
                    src_x = (src_x + m_dimensions[0]) % m_dimensions[0];

                if (m_is_periodic[1])
                    src_y = (src_y + m_dimensions[1]) % m_dimensions[1];
                
                if (0 <= src_x && src_x < m_dimensions[0] && 0 <= src_y && src_y < m_dimensions[1])
                {
                    src_idx = coords_to_index(src_x, src_y);                    
                    // Bounce off solid cells
                    if (m_cell_type[src_idx] == CellType::SOLID)                                        
                    {
                        opposite_dir = m_bounce_back_indices[dir];
                        m_f_new[dest_idx][dir] = m_f[dest_idx][opposite_dir];
                    }
                    else                    
                    {
                        m_f_new[dest_idx][dir] = m_f[src_idx][dir];
                    }
                    
                }
                else
                {
                    // Out of bounds: do nothing for now
                    // Could implement specific boundary conditions here
                }
                    
            }
        }
    }
    std::swap(m_f, m_f_new);
}



void D2Q9::compute_macroscopic() 
{   
    for (size_t i = 0; i < m_total_size; i++)
    {
        if (m_cell_type[i] == CellType::SOLID) continue;

        m_rho[i] = std::accumulate(m_f[i].begin(), m_f[i].end(), 0.0);
        
        m_u[i] = {0.0, 0.0};
        for (size_t dir = 0; dir < 9; dir++)
        {
            m_u[i][0] += m_f[i][dir] * m_directions[dir][0];
            m_u[i][1] += m_f[i][dir] * m_directions[dir][1];
        }

        // if (m_rho[i] > MIN_DENSITY_THRESHOLD)
        // {
            m_u[i][0] /= m_rho[i];
            m_u[i][1] /= m_rho[i];
        // }
        // else m_u[i] = {0.0, 0.0};
    }
}

void D2Q9::apply_boundary()
{
    // Inflow: set incoming populations at left inflow cells
    for (const auto& [x,y] : m_inflow_cells) {
        size_t idx = coords_to_index(x, y);
        auto [u_in, rho_in] = m_inflow_conditions[{x,y}];   // assume stored

        // m_u[idx] = u_in;
        // m_rho[idx] = rho_in;

        auto feq = compute_equilibrium(rho_in, u_in);

        // For left boundary (x==0) incoming populations are indices 1(E),5(NE),8(SE)
      
        m_f[idx][1] = feq[1];
        m_f[idx][5] = feq[5];
        m_f[idx][8] = feq[8];
        m_f[idx][3] = feq[3];
        m_f[idx][7] = feq[7];
        m_f[idx][6] = feq[6];
        // leave outgoing populations untouched (they were pulled in stream)
    }

    // Outflow: copy from neighbor to the left (zero-gradient)
    for (const auto& [x,y] : m_outflow_cells) {
        size_t idx = coords_to_index(x,y);
        int xL = static_cast<int>(x) - 1;
        if (xL < 0) xL = 0; // guard (shouldn't happen)
        size_t idxL = coords_to_index(xL, y);
        m_f[idx] = m_f[idxL];
        m_rho[idx] = m_rho[idxL];
        m_u[idx] = m_u[idxL];
    }

    // Note: SOLID bounce-back already handled in stream() for pull-scheme,
    // so do not do additional swapping here for SOLIDs.
}

// void D2Q9::apply_boundary()
// {
//     // Inflow
//     for (const auto& [x,y] : m_inflow_cells) {
//         size_t idx = coords_to_index(x,y);
//         auto [u_in, rho_in] = m_inflow_conditions[{x,y}];

//         // m_u[idx]   = u_in;
//         // m_rho[idx] = rho_in;

//         m_u[idx]   = {0.1f, 0.0f}; // Prescribe only the velocity, let density adjust
   

//         auto feq = compute_equilibrium(m_rho[idx], m_u[idx]);

//         // Only incoming directions for left wall
//         m_f[idx][1] = feq[1];
//         m_f[idx][5] = feq[5];
//         m_f[idx][8] = feq[8];

//         m_f[idx]= feq;
//     }

//     // Outflow (zero-gradient: copy from neighbor left)
//     for (const auto& [x,y] : m_outflow_cells) {
//         size_t idx  = coords_to_index(x,y);
//         size_t idxL = coords_to_index(x-1,y);
//         m_f[idx] = m_f[idxL];
//         m_rho[idx] = m_rho[idxL];
//         m_u[idx]   = m_u[idxL];
//     }
// }


const std::vector<double>& D2Q9::get_density() const { return m_rho; }
const std::vector<D2Q9::VelocityVec>& D2Q9::get_velocity() const { return m_u; }