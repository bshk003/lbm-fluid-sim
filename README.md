## LBM-D2Q9 Simulator

This is a simple 2D fluid dynamics simulator based on the *Lattice Boltzmann Method (LBM)* with a D2Q9 lattice and an OpenGL renderer.

---

###  Features
- D2Q9 lattice Boltzmann implementation (BGK collision model).
- Flexible domain setup:
  - Geometry and initial conditions defined via *bitmap + YAML*.
  - Supports solid, inflow, outflow, and fluid cells.
  - Periodic and non-periodic boundaries.
- Real-time OpenGL renderer:
  - Scalar field visualization (e.g. density, vorticity).
  - Tracers with configurable size, color, and emission.
- Video recording using FFmpeg.
- Configurable via YAML: simulation parameters, visualization, tracers, etc.

![](examples/boltzmann.gif)

![](examples/boltzmann_2.gif)

![](examples/chamber.gif)

More demos are in `examples/`.

---

### Usage

Use `scripts/prepare_simulation.py` to pack the color-coded simulation domain (stored as an image file) and simulation parameters (in a yaml) to an input binary file.

The CLI usage: `lbm-fluid-sim --input <input_file> --output <output_file.mp4>`.

### References
[1] Wolf-Gladrow, Dieter (2000). Lattice-Gas Cellular Automata and Lattice Boltzmann Models.

[2] Krüger, T.; Kusumaatmaja, H.; Kuzmin, A.; Shardt, O.; Silva, G.; Viggen, E. M. (2017). The Lattice Boltzmann Method: Principles and Practice.

[3] A very elegant minimalist D2Q9 implementation by Prof. Dan Schroeder: https://physics.weber.edu/schroeder/fluids/ 
