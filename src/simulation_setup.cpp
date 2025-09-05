#include "d2q9.h"
#include "simulation_setup.h"

static inline size_t idx2d(size_t x, size_t y, size_t width) { 
    return y * width + x; 
}

const double u0 = 0.1; // Inflow velocity

const D2Q9::InitialConditions create_initial_conditions(size_t GRID_WIDTH, size_t GRID_HEIGHT) {
    const size_t TOTAL_CELLS = GRID_WIDTH * GRID_HEIGHT;
    D2Q9::InitialConditions ic;
    ic.cell_type.resize(TOTAL_CELLS, CellType::FLUID);
    ic.initial_rho.resize(TOTAL_CELLS, 1.0);
    ic.initial_u.resize(TOTAL_CELLS, D2Q9::VelocityVec{u0, 0.0});

    // Top and bottom solid walls (no-slip)
    // for (size_t x = 0; x < GRID_WIDTH; ++x) {
    //     ic.cell_type[idx2d(x, 0, GRID_WIDTH)] = CellType::SOLID;
    //     ic.cell_type[idx2d(x, GRID_HEIGHT - 1, GRID_WIDTH)] = CellType::SOLID;
    // }

    // A cylinder in the middle
    // size_t cx = GRID_WIDTH / 4;
    // size_t cy = GRID_HEIGHT / 2;
    // size_t radius = 20;
    // for (size_t y = 1; y + 1 < GRID_HEIGHT; ++y) {
    //     for (size_t x = 1; x + 1 < GRID_WIDTH; ++x) {
    //         if (std::hypot(static_cast<float>(x) - cx, static_cast<float>(y) - cy) < radius) {
    //             ic.cell_type[idx2d(x, y, GRID_WIDTH)] = CellType::SOLID;
    //         }
    //     }
    // }

    for(size_t y=30; y<40; ++y) {
        for(size_t x=25; x<27; ++x) {
            ic.cell_type[idx2d(x + y, y, GRID_WIDTH)] = CellType::SOLID;
        }
    }

     for (size_t y = 0; y < GRID_HEIGHT; ++y) {
    //     // Inflow boundary
        if (ic.cell_type[idx2d(0, y, GRID_WIDTH)] == CellType::FLUID) {
            ic.cell_type[idx2d(0, y, GRID_WIDTH)] = CellType::INFLOW;
            ic.initial_u[idx2d(0, y, GRID_WIDTH)] = D2Q9::VelocityVec{u0, 0.0f}; // Set a horizontal inflow velocity
        }}
    //    // Outflow boundary
    //     if (ic.cell_type[idx2d(GRID_WIDTH-1, y, GRID_WIDTH)] == CellType::FLUID) {
    //         ic.cell_type[idx2d(GRID_WIDTH-1, y, GRID_WIDTH)] = CellType::OUTFLOW;
    //     }
    // }

    return ic;
}
