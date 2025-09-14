#ifndef D2Q9_SETUP_H
#define D2Q9_SETUP_H

#include "d2q9.h"       
#include "lbm.h"       
#include "tracers_collection.h" 

struct VisualizationParams
{
    size_t width;
    size_t height;
    size_t steps_per_frame;
};

struct QuantityParams
{
    std::string quant_id;
    float offset;
    float amplitude;
};


// Loads simulation data from a binary file and populates existing structs
void load_from_binary(const std::string& filename, 
                      LBM<2>::LBMParams& lbm_params, 
                      D2Q9::InitialConditions& initials, 
                      VisualizationParams& visual_params,
                      std::vector<QuantityParams>& render_quant_params,
                      TracersParams& tracers_params);

void sample_d2q9(LBM<2>::LBMParams& lbm_params, 
                 D2Q9::InitialConditions& initials, 
                 VisualizationParams& visual_params,
                 std::vector<QuantityParams>& quant_params,
                 TracersParams& tracers_params);

#endif // D2Q9_SETUP_H