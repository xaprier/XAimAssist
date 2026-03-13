/**
 * @file Application.hpp
 * @brief Top-level entry point that wires and runs the application.
 */

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

namespace xaimassist::app {

/**
 * @class Application
 * @brief Constructs the Qt application, composition root, main window
 *        and enters the event loop.
 */
class Application {
  public:
    /// Construct all subsystems and enter the Qt event loop; returns exit code.
    int Run(int argc, char* argv[]);
};
}  // namespace xaimassist::app

#endif  // APPLICATION_HPP
