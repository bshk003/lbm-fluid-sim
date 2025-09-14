#include "d2q9_observables.h"

const std::map<std::string, ComputeFunc> compute_functions = 
{
    {"speed", D2Q9_compute_speed},
    {"vorticity", D2Q9_compute_vorticity},
    {"density", D2Q9_compute_density},
    {"zero", D2Q9_compute_zero}
};

const std::map<std::string, ComputeFunc>& get_compute_functions() 
{
    return compute_functions;
}

// Observable computation functions
void D2Q9_compute_speed(const D2Q9& lbm, 
                        std::vector<float>& out_field,
                        const float zero_ref,
                        const float amplitude)
{
    const float scale = std::max(1 - zero_ref, zero_ref) / amplitude;
    std::transform(std::execution::par,
                   lbm.get_velocity().begin(), 
                   lbm.get_velocity().end(), 
                   out_field.begin(), 
                   [scale, zero_ref](D2Q9::VelocityVec val)
                   {
                        float value = static_cast<float>(hypot(val[0], val[1]));
                        return scale * value + zero_ref;
                   });
}

void D2Q9_compute_density(const D2Q9& lbm, 
                          std::vector<float>& out_field,
                          const float zero_ref,
                          const float amplitude)
{
    const float scale = std::max(1 - zero_ref, zero_ref) / amplitude;
    std::transform(std::execution::par,
                   lbm.get_density().begin(), 
                   lbm.get_density().end(), 
                   out_field.begin(), 
                   [scale, zero_ref](double val)
                   {
                        float value = static_cast<float>(val);
                        return scale * value + zero_ref;
                   });
}

void D2Q9_compute_vorticity(const D2Q9& lbm, 
                            std::vector<float>& out_field,
                            const float zero_ref,
                            const float amplitude)
{
    const auto& u = lbm.get_velocity();
    const auto& dims = lbm.get_dimensions();
    const float scale = std::max(1 - zero_ref, zero_ref) / amplitude;

    size_t width = dims[0];
    size_t height = dims[1];
    double dudy, dudx, curl;

    for (int y=0; y < height; ++y) 
    {
        for (int x=0; x < width; ++x)         
        {
            dudy = (u[lbm.coords_to_index(x+1, y)][1] - u[lbm.coords_to_index(x-1, y)][1]) * 0.5;
            dudx = (u[lbm.coords_to_index(x, y+1)][0] - u[lbm.coords_to_index(x, y-1)][0]) * 0.5;
            curl = dudx - dudy;

            out_field[lbm.coords_to_index(x,y)] = scale * static_cast<float>(curl) + zero_ref;
        }
    }
}

void D2Q9_compute_zero(const D2Q9& lbm, 
                            std::vector<float>& out_field,
                            const float zero_ref,
                            const float amplitude)
{
    std::fill(out_field.begin(), out_field.end(), zero_ref);
}