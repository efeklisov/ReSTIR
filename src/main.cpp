#include <stdexcept>
#include <iostream>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include "app.hpp"

int main(int argc, char* argv[]) {
    CLI::App parser{"Wanna-be pathtracer"};

    params_t params;

    parser.add_option("-N,--N", params.N, "Number of samples per pixel");
    parser.add_option("-m,--method", params.method, "Which shader to use");
    parser.add_flag("-c,--capture", params.capture, "Capture screenshot");
    parser.add_flag("-o,--offline", params.pseudoOffline, "Quit after rendering the screenshot");
    parser.add_flag("-a,--accumulate", params.accumulate, "Stitch frames together");
    parser.add_option("-f,--frames", params.frames, "Number of frames to concatenate");
    parser.add_option("-t,--tolerance", params.tolerance, "Number of frames before capturing");
    parser.add_option("-M,--M", params.M, "M value for RIS");
    parser.add_flag("-i,--immediate", params.immediate, "Unlock FPS");
    parser.add_flag("--100", params.multiply, "Multiply geometry");

    try {
        parser.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return parser.exit(e);
    }

    App app(params);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
