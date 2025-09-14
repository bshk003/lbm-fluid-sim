#ifndef D2Q9_OBSERVABLE_H
#define D2Q9_OBSERVABLE_H  

#include "d2q9.h"
#include <vector>
#include <cmath>
#include <map>
#include <functional>
#include <execution>
#include <algorithm>


// A scalar observable f is returned by filling in the out_field. 
// The out_field values are supposed to be in the [0,1] range.
// For normalization, zero_ref == where the zero value of f gets mapped to in [0,1]
// amplitude == the expected amplitude of |f|

void D2Q9_compute_speed(const D2Q9& lbm, 
                        std::vector<float>& out_field,
                        const float zero_ref,
                        const float amplitude);

void D2Q9_compute_density(const D2Q9& lbm, 
                          std::vector<float>& out_field,
                          const float zero_ref,
                          const float amplitude);

void D2Q9_compute_vorticity(const D2Q9& lbm, 
                            std::vector<float>& out_field,
                            const float zero_ref,
                            const float amplitude);

void D2Q9_compute_zero(const D2Q9& lbm, 
                            std::vector<float>& out_field,
                            const float zero_ref,
                            const float amplitude);


using ComputeFunc = std::function<void(const D2Q9&, std::vector<float>&, const float, const float)>;

extern const std::map<std::string, ComputeFunc> compute_functions;
const std::map<std::string, ComputeFunc>& get_compute_functions();

#endif