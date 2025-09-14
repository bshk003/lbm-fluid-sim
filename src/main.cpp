#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <optional>
#include <cstdlib>
#include <cstdio>

#include "renderer.h"
#include "d2q9.h"
#include "d2q9_setup.h"
#include "d2q9_observables.h"
#include "tracers_collection.h"

struct Args
{
    std::optional<std::string> input_file;
    std::optional<std::string> output_file; 
};

struct QuantParamsStatus
{
    int current_quant;
    const std::vector<QuantityParams>* quants;
};

Args parse_args(int argc, char** argv)
{
    Args args;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        std::cout << arg << std::endl;
        if (arg == "--input" && i + 1 < argc)
            args.input_file = argv[++i];
        else if (arg == "--output" && i + 1 < argc)
            args.output_file = argv[++i];
        else
            std::cerr << "Unsupported command line argument: " << arg << std::endl; 
    }
    return args;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // ESC to close the window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) 
    {
        QuantParamsStatus* current_status = static_cast<QuantParamsStatus*>(glfwGetWindowUserPointer(window));
        current_status->current_quant = (current_status->current_quant + 1) % (current_status->quants)->size(); 

        std::cout << "Currently rendering: " << (*current_status->quants)[current_status->current_quant].quant_id << std::endl;
    }
}

int main(int argc, char** argv)
{
    LBM<2>::LBMParams lbm_params;
    D2Q9::InitialConditions initials;
    VisualizationParams visual_params;
    std::vector<QuantityParams> quants_params;  
    TracersParams tracers_params;

    GLFWwindow* renderer_window;

    Args args = parse_args(argc, argv);
    //Args args{ std::make_optional<std::string>("../examples/boltzmann.dat"), std::nullopt };

    if (args.input_file)
    {
        std::cout << "Loading setup from " << *args.input_file << std::endl;
        load_from_binary(*args.input_file, 
                          lbm_params, 
                          initials, 
                          visual_params, 
                          quants_params, 
                          tracers_params);
    }
    else
    {
        std::cout << "No input file provided. Using a sample setup." << std::endl;

        lbm_params = {{200, 80}, {false, true}, 0.6}; // Grid dimensions, periodicity, tau
        initials = sample_d2q9(lbm_params);
        visual_params = {800, 320, 1};
        quants_params = { {"speed", 0.0f, 0.2f}, {"vorticity", 0.5f, 0.05f} };
    }

    // Bring up an ffmpeg pipe if requested
    FILE* ffmpeg = nullptr;
    if (args.output_file) 
    { 
        std::string cmd =   "ffmpeg -y "
                            "-f rawvideo -pix_fmt rgb24 "
                            "-s " + std::to_string(visual_params.width) + "x" + std::to_string(visual_params.height) + " "
                            "-r 60 "
                            "-i - "
                            "-vf vflip "
                            "-an -c:v libx264 -pix_fmt yuv420p "
                            + *args.output_file;

        ffmpeg = popen(cmd.c_str(), "w");
        if (!ffmpeg) 
        {
            std::cerr << "Failed to open ffmpeg pipe\n";
            return -1;
        }
    }
    
    D2Q9 lbm(lbm_params,  initials);
    Renderer renderer(visual_params.width, 
                      visual_params.height, 
                      lbm_params.dimensions[0], 
                      lbm_params.dimensions[1]);    
    TracersCollection tracers(lbm, tracers_params);

    renderer_window = renderer.get_window();

    QuantParamsStatus quants_status = {0, &quants_params};
    if (!quants_params.size())
    {
        std::cout << "No quantities to render. Exiting the simulation." << std::endl;
        return 0;
    } 

    // Register the keyboard callback
    glfwSetKeyCallback(renderer_window, key_callback);
    
    glfwSetWindowUserPointer(renderer_window, &quants_status);    

    std::vector<float> render_field(lbm.get_total_size()); 
    std::vector<unsigned char> pixels(3 * visual_params.width * visual_params.height);
    const auto& compute_functions = get_compute_functions();

    std::cout << "Starting LBM simulation..." << std::endl;
    std::cout << "Press ESC or close window to exit." << std::endl;
    std::cout << "Press SPACE to switch between quantities to render." << std::endl;
    std::cout << "Currently rendering: " << quants_params[quants_status.current_quant].quant_id << std::endl;

    try 
    {   
        // Main loop
        while (!renderer.should_close()) 
        {
            for (int step_cnt = 0; step_cnt < visual_params.steps_per_frame; step_cnt++)
                lbm.step();       

            // Render an observable
            const QuantityParams& current_quant = quants_params[quants_status.current_quant];
            
            auto it = compute_functions.find(current_quant.quant_id);
            if (it != compute_functions.end())
                it->second(lbm, render_field, current_quant.offset, current_quant.amplitude);
            else
                std::cerr << "Error: unknown quantity '" << current_quant.quant_id << "' to render" << std::endl;
            renderer.render(render_field, lbm.get_obstacle_mask());
            
            // Process tracers
            tracers.update_positions();
            tracers.emit_tracers();
            tracers.render_tracers();
            
            renderer.poll_events();
            glfwSwapBuffers(renderer_window);

            if (ffmpeg)
            {
                glReadPixels(0, 0, visual_params.width, visual_params.height,
                             GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
                fwrite(pixels.data(), 1, pixels.size(), ffmpeg);
            }
        }

        if (ffmpeg) pclose(ffmpeg);

        std::cout << "Simulation completed." << std::endl;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
