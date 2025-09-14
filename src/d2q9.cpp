#include "d2q9.h"
#include <stdexcept>
#include <numeric>
#include <execution>
#include <iterator>
#include <utility>
#include <iostream>

D2Q9::D2Q9(size_t width, size_t height, double tau): 
    LBM<2>(LBMParams{ {width, height}, {false, false}, tau })
{    
    // Create a static, uniform density state for testing purposes
    m_f.resize(m_total_size);
    m_f_new.resize(m_total_size);

    m_cell_type.resize(m_total_size);
    std::fill(m_cell_type.begin(), m_cell_type.end(), CellType::FLUID);

    m_rho.resize(m_total_size);
    std::fill(m_rho.begin(), m_rho.end(), 1.0);

    m_u.resize(m_total_size);
    std::fill(m_u.begin(), m_u.end(), VelocityVec{0.0, 0.0});

    for (size_t idx = 0; idx < m_total_size; idx++)
        m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
}

D2Q9::D2Q9(const LBMParams& lbm_params, 
           const InitialConditions& initials): LBM<2>(lbm_params)
{  
    if (m_total_size != initials.cell_type.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: cell type");
    }

    if (m_total_size != initials.initial_rho.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: density");
    }

    if (m_total_size != initials.initial_u.size())
    {
        throw std::runtime_error("Wrong size of the initial conditions data: velocity");
    }

    m_cell_type = initials.cell_type;
    m_rho = initials.initial_rho;
    m_u = initials.initial_u;

    m_f.resize(m_total_size);
    m_f_new.resize(m_total_size);

    m_obstacle_mask.resize(m_total_size, 0.0f);

    m_indices.resize(m_total_size);
    std::iota(m_indices.begin(), m_indices.end(), 0);

    // A simplification that helps to handle boundaries: 
    // if both directions are non-periodic, mark the corners as solid.
    if (!m_is_periodic[0] && !m_is_periodic[1])
    {
        m_cell_type[coords_to_index(0, 0)] = CellType::SOLID;
        m_cell_type[coords_to_index(m_dimensions[0] - 1, 0)] = CellType::SOLID;
        m_cell_type[coords_to_index(0, m_dimensions[1] - 1)] = CellType::SOLID;
        m_cell_type[coords_to_index(m_dimensions[0] - 1, m_dimensions[1] - 1)] = CellType::SOLID;
    }

    std::for_each(m_indices.begin(), m_indices.end(),
                  [this](size_t idx)
                  {
                        switch(m_cell_type[idx])
                        {
                            case CellType::FLUID:
                                m_fluid_cells.push_back(idx);
                                m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                                break;
                            case CellType::SOLID:
                                m_solid_cells.push_back(idx);
                                m_rho[idx] = 1.0; // A reference density value
                                m_u[idx] = {0.0, 0.0};
                                m_obstacle_mask[idx] = 1.0f;
                                break;
                            case CellType::INFLOW:
                                m_inflow_cells.push_back(idx);
                                m_inflow_conditions[idx] = {m_u[idx], m_rho[idx]};
                                m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                                break;
                            case CellType::OUTFLOW:
                                m_outflow_cells.push_back(idx);
                                m_outflow_conditions[idx] = m_rho[idx];
                                m_f[idx] = compute_equilibrium(m_rho[idx], m_u[idx]);
                        }
                  });
    
}

D2Q9::CellState D2Q9::compute_equilibrium(double rho, const VelocityVec& u)
{
    CellState f_eq;
    const double usq = u[0] * u[0] + u[1] * u[1];

    for (size_t dir = 0; dir < 9; dir++)
    {
        const double eu = m_directions[dir][0] * u[0] + m_directions[dir][1] * u[1];
        f_eq[dir] = m_weights[dir] * rho * (1.0 + eu * m_inv_csq 
                                                + (m_inv_csq * m_inv_csq * (eu * eu) / 2.0) 
                                                - (m_inv_csq * usq / 2.0));
    }
    return f_eq; 
}

void D2Q9::collide()
{
    std::for_each(std::execution::par,
                  m_fluid_cells.begin(), m_fluid_cells.end(),
                  [this](size_t idx)
                  {
                        CellState f_eq = compute_equilibrium(m_rho[idx], m_u[idx]);
                        for (size_t dir = 0; dir < 9; dir++) 
                             m_f[idx][dir] -= m_inv_tau * (m_f[idx][dir] - f_eq[dir]);
                  });
}

void D2Q9::stream()
{
    auto process_cell = [this](size_t dest_idx) 
    {
        if (m_cell_type[dest_idx] == CellType::SOLID) return;
        size_t src_idx, opposite_dir;

        for (size_t dir = 0; dir < 9; dir++) 
        {
            src_idx = get_neighbor_index(dest_idx, m_directions[dir]);
            //Bounce off solid cells
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
       // std::cout << m_f_new[dest_idx][2] << std::endl;
    };

    std::for_each(std::execution::par,
                  m_indices.begin(), m_indices.end(),
                  process_cell);

    std::swap(m_f, m_f_new);
}


void D2Q9::compute_macroscopic()
{
    auto process_cell = [this](size_t idx)
    {
        if (m_cell_type[idx] == CellType::FLUID) 
        {
            m_rho[idx] = std::accumulate(m_f[idx].begin(), m_f[idx].end(), 0.0);

            m_u[idx] = {0.0, 0.0};
            for (size_t dir = 0; dir < 9; dir++)
            {
                m_u[idx][0] += m_f[idx][dir] * m_directions[dir][0];
                m_u[idx][1] += m_f[idx][dir] * m_directions[dir][1];
            }

            if (m_rho[idx] > MIN_DENSITY_THRESHOLD)
            {
                m_u[idx][0] /= m_rho[idx];
                m_u[idx][1] /= m_rho[idx];
            }
        }
    };

    std::for_each(std::execution::par, 
                  m_indices.begin(), m_indices.end(), 
                  process_cell);
}

void D2Q9::apply_cell_conditions()
{
    // Use Zou-He conditions for the inflow/outflow cells on edges.
    // For inner inflow/outflow cells in the domain, renew the cell state to the equilibrium.

    // Handling inflow cells
    for (size_t idx : m_inflow_cells)
    {
        auto [x, y] = index_to_coords(idx);
        auto [u_in, rho_in] = m_inflow_conditions[idx];

        if (!m_is_periodic[0] && x == 0) 
        {
            // West boundary
            m_f[idx][1] = m_f[idx][3] + (2.0/3.0) * rho_in * u_in[0];
            m_f[idx][5] = m_f[idx][7] + (1.0/6.0) * rho_in * u_in[0] + 0.5 * rho_in * u_in[1];
            m_f[idx][8] = m_f[idx][6] + (1.0/6.0) * rho_in * u_in[0] - 0.5 * rho_in * u_in[1];
        }
        else if (!m_is_periodic[0] && x == m_dimensions[0] - 1) 
        {
            // East boundary
            m_f[idx][3] = m_f[idx][1] - (2.0/3.0) * rho_in * u_in[0];
            m_f[idx][6] = m_f[idx][8] - (1.0/6.0) * rho_in * u_in[0] + 0.5 * rho_in * u_in[1];
            m_f[idx][7] = m_f[idx][5] - (1.0/6.0) * rho_in * u_in[0] - 0.5 * rho_in * u_in[1];
        }
        else if (!m_is_periodic[1] && y == 0) 
        {
            // South boundary
            m_f[idx][2] = m_f[idx][4] + (2.0/3.0) * rho_in * u_in[1];
            m_f[idx][5] = m_f[idx][7] + 0.5 * rho_in * u_in[0] + (1.0/6.0) * rho_in * u_in[1];
            m_f[idx][6] = m_f[idx][8] - 0.5 * rho_in * u_in[0] + (1.0/6.0) * rho_in * u_in[1];
        }
        else if (!m_is_periodic[1] && y == m_dimensions[1] - 1) 
        {
            // North boundary
            m_f[idx][4] = m_f[idx][2] - (2.0/3.0) * rho_in * u_in[1];
            m_f[idx][7] = m_f[idx][5] - 0.5 * rho_in * u_in[0] - (1.0/6.0) * rho_in * u_in[1];
            m_f[idx][8] = m_f[idx][6] + 0.5 * rho_in * u_in[0] - (1.0/6.0) * rho_in * u_in[1];
        }
        else
        {
            // Internal or periodic inflow
            m_f[idx] = compute_equilibrium(rho_in, u_in);
        }
    }

    // Handling outflow cells
    for (size_t idx : m_outflow_cells)
    {
        auto [x, y] = index_to_coords(idx);
        double rho_out = m_outflow_conditions[idx];
        VelocityVec u_out = {0.0, 0.0};

        if (!m_is_periodic[0] && x == 0) 
        {
            // West boundary
            m_f[idx][1] = m_f[idx][3] + (2.0/3.0) * rho_out * u_out[0];
            m_f[idx][5] = m_f[idx][7] + (1.0/6.0) * rho_out * u_out[0] + 0.5 * rho_out * u_out[1];
            m_f[idx][8] = m_f[idx][6] + (1.0/6.0) * rho_out * u_out[0] - 0.5 * rho_out * u_out[1];
        }
        else if (!m_is_periodic[0] && x == m_dimensions[0] - 1) 
        {
            // East boundary
            m_f[idx][3] = m_f[idx][1] - (2.0/3.0) * rho_out * u_out[0];
            m_f[idx][6] = m_f[idx][8] - (1.0/6.0) * rho_out * u_out[0] + 0.5 * rho_out * u_out[1];
            m_f[idx][7] = m_f[idx][5] - (1.0/6.0) * rho_out * u_out[0] - 0.5 * rho_out * u_out[1];
        }
        else if (!m_is_periodic[1] && y == 0) 
        {
            // South boundary
            m_f[idx][2] = m_f[idx][4] + (2.0/3.0) * rho_out * u_out[1];
            m_f[idx][5] = m_f[idx][7] + 0.5 * rho_out * u_out[0] + (1.0/6.0) * rho_out * u_out[1];
            m_f[idx][6] = m_f[idx][8] - 0.5 * rho_out * u_out[0] + (1.0/6.0) * rho_out * u_out[1];
        }
        else if (!m_is_periodic[1] && y == m_dimensions[1] - 1)
        {
            // North boundary
            m_f[idx][4] = m_f[idx][2] - (2.0/3.0) * rho_out * u_out[1];
            m_f[idx][7] = m_f[idx][5] - 0.5 * rho_out * u_out[0] - (1.0/6.0) * rho_out * u_out[1];
            m_f[idx][8] = m_f[idx][6] + 0.5 * rho_out * u_out[0] - (1.0/6.0) * rho_out * u_out[1];
        }
        else 
        {
            // Internal or periodic inflow
            m_f[idx] = compute_equilibrium(rho_out, u_out);
        }
    }
}


const std::vector<double>& D2Q9::get_density() const { return m_rho; }
const std::vector<D2Q9::VelocityVec>& D2Q9::get_velocity() const { return m_u; }