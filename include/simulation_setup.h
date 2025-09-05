#ifndef SIMULATION_SETUP_H
#define SIMULATION_SETUP_H

#include <vector>
#include <array>
#include <cmath>
#include "d2q9.h"

// Function to create and define initial conditions and domain geometry
const D2Q9::InitialConditions create_initial_conditions(size_t GRID_WIDTH, size_t GRID_HEIGHT);

#endif // SIMULATION_SETUP_H