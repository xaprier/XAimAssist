/// @file main.cpp — application entry point.
#include "app/Application.hpp"

// Force discrete GPU on hybrid systems (NVIDIA Optimus / AMD PowerXpress).
// These symbols are read by the GPU driver before the process enters main().
// Without them, hybrid-GPU laptops default to the integrated GPU, halving
// rendering throughput and increasing input latency significantly.
#ifdef _WIN32
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char* argv[]) {
    xaimassist::app::Application application;
    return application.Run(argc, argv);
}
