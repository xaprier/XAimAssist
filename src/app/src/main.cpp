/// @file main.cpp — application entry point.
#include "app/Application.hpp"

int main(int argc, char* argv[]) {
    xaimassist::app::Application application;
    return application.Run(argc, argv);
}
