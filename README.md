## LBM-D2Q9 Simulator

This is a simple 2D fluid dynamics simulator based on the **Lattice Boltzmann Method (LBM)** with a D2Q9 lattice and an OpenGL renderer.

---

###  Features
- D2Q9 lattice Boltzmann implementation (BGK collision model).
- Flexible domain setup:
  - Geometry and initial conditions defined via **bitmap + YAML**.
  - Supports solid, inflow, outflow, and fluid cells.
  - Periodic and non-periodic boundaries.
- Real-time OpenGL renderer:
  - Scalar field visualization (e.g. density, vorticity).
  - Tracers with configurable size, color, and emission.
- Video recording using **FFmpeg**.
- Configurable via YAML: simulation parameters, visualization, tracers, etc.

---

### Some demos

  
<video src="examples/block.mp4" width="600" controls></video>


<video src="examples/chamber.mp4" width="600" controls></video>

<video src="examples/segment.mp4" width="600" controls></video>

<video src="examples/boltzmann.mp4" width="600" controls></video>

<video src="examples/boltzmann_2.mp4" width="600" controls></video>
---

### Usage

Build with CMake:
```bash
mkdir build && cd build
cmake ..
make
