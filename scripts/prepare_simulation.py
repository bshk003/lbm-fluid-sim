import argparse
import yaml
import struct
import PIL
from PIL import Image
from pathlib import Path

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(bytes.fromhex(hex_color))

def main(config_file):
    try:
        config_dir = Path(config_file).parent
        with open(config_file, 'r') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"Error: '{config_file}' not found.")
        return
        
    sim_params = config['simulation_params']
    periodicity = config['periodicity']
    color_map = config['color_map']
    render = config['render']
    tracers_params = config['tracers']
    
    viscosity = sim_params['viscosity']
    is_periodic_x = periodicity['x']
    is_periodic_y = periodicity['y']

    steps_per_frame = render['steps_per_frame']
    render_window_size = render['render_window_size']

    # The relaxation parameter
    tau = 3.0 * viscosity + 0.5
    
    map_filename = color_map['map_filename']
    try:
        img = Image.open(config_dir/map_filename).convert('RGB')
        width, height = img.size
    except Exception as e:
        print(f"Could not process the image file '{map_filename}': {e}")
        raise

    color_data = {hex_to_rgb(item['color']): item for item in color_map['colors']}
    
    total_size = width * height
    cell_type = [0] * total_size
    initial_rho = [0.0] * total_size
    initial_u_x = [0.0] * total_size
    initial_u_y = [0.0] * total_size
    tracers = []

    # Make sure the order matches the one used in lbm.h
    cell_type_map = {'FLUID': 0, 'SOLID': 1, 'INFLOW': 2, 'OUTFLOW': 3}

    # Process a domain bitmap
    for y in range(height):
        for x in range(width):
            pixel_rgb = img.getpixel((x, y))
            cell_info = color_data.get(pixel_rgb)
            y = height - y - 1
            idx = y * width + x

            if not cell_info or cell_info['type'] == 'SOLID':
                if not cell_info:
                    print(f"Warning: Unknown color {pixel_rgb} at ({x}, {y}). Treating as SOLID.")
                cell_type[idx] = cell_type_map['SOLID']
                initial_rho[idx] = 1.0
                initial_u_x[idx] = 0.0
                initial_u_y[idx] = 0.0
            else:
                cell_type[idx] = cell_type_map[cell_info['type']]
                initial_rho[idx] = cell_info['initial_rho']
                if cell_info['type'] == 'INFLOW' or cell_info['type'] == 'FLUID':
                    initial_u_x[idx] = cell_info['initial_u'][0]
                    initial_u_y[idx] = cell_info['initial_u'][1]
                if cell_info['type'] == 'FLUID' and cell_info.get('tracer', False):
                    tracers.append(idx)

    if config_file.endswith('.yaml'):
        output_file = config_file[:-5] + '.dat'
    else:
        output_file = config_file + '.dat'

    # Make sure the format matches the one used in d2q9_setup.cpp
    with open(output_file, 'wb') as f:
        # LBM parameters (grid dimensions, periodicity, tau) 
        f.write(struct.pack('<QQ', width, height))
        f.write(struct.pack('<bb', is_periodic_x, is_periodic_y))
        f.write(struct.pack('<d', tau))

        # Visualization parameters (renderer window dimensions, steps per frame)
        f.write(struct.pack('<QQ', *render_window_size))
        f.write(struct.pack('<Q', steps_per_frame))

        # Quantities to render
        n_quant = len(render['render_quantities'])
        f.write(struct.pack('<B', n_quant))
        for quant in render['render_quantities']:
            quant_id = quant['quantity']
            f.write(struct.pack('<B', len(quant_id)))
            f.write(quant_id.encode('utf-8'))
            f.write(struct.pack('<ff', float(quant['offset']), float(quant['amplitude'])))

        # Tracers parameters
        tracers_color = [float(val) / 255 for val in hex_to_rgb(tracers_params.get('color', '#FF00FF').lstrip('#'))]
        tracers_color.append(1.0) # The alpha channel
        f.write(struct.pack('<ffff', *tracers_color))
        f.write(struct.pack('<f', float(tracers_params.get('size', 3.0))))
        f.write(struct.pack('<f', float(tracers_params.get('emission_rate', 0))))
        f.write(struct.pack('<Q', tracers_params.get('random_initial', 0)))
    
        # Data arrays
        f.write(struct.pack(f'<{total_size}B', *cell_type))
        f.write(struct.pack(f'<{total_size}d', *initial_rho))
        f.write(struct.pack(f'<{total_size}d', *initial_u_x))
        f.write(struct.pack(f'<{total_size}d', *initial_u_y))

        # Initial tracers
        f.write(struct.pack('<Q', len(tracers)))
        f.write(struct.pack(f'<{len(tracers)}Q', *tracers))        

        
    print(f"Simulation setup data saved as {output_file}.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Prepare LBM simulation data from a YAML config file.")
    parser.add_argument('config_file', type=str, help='Path to the YAML configuration file.')
    args = parser.parse_args()
    main(args.config_file)
