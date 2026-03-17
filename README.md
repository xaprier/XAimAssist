# XAimAssist - A 3D FPS Aim Training Application

<p align="center">
  <a href="https://github.com/xaprier/XAimAssist/blob/main/LICENSE" target="blank">
    <img src="https://img.shields.io/github/license/xaprier/XAimAssist" alt="license" />
  </a>
  <a href="https://github.com/xaprier/XAimAssist/actions" target="blank">
    <img src="https://img.shields.io/github/actions/workflow/status/xaprier/XAimAssist/release.yml?branch=main" alt="build status" />
  </a>
  <a href="https://xaprier.github.io/XAimAssist/" target="blank">
    <img src="https://img.shields.io/badge/docs-doxygen-blue.svg" alt="documentation" />
  </a>
  <a href="https://github.com/xaprier/XAimAssist/releases" target="blank">
    <img src="https://img.shields.io/github/v/release/xaprier/XAimAssist" alt="latest release" />
  </a>
</p>

An open-source 3D FPS aim training application built with modern C++ architecture, Qt6, and VTK rendering.

<p align="center">
    <a href="https://xaprier.github.io/XAimAssist/">
        View API Documentation
    </a>
    |
    <a href="https://github.com/xaprier/XAimAssist/blob/main/BUILD.md">
        Build Instructions
    </a>
</p>

## Overview

XAimAssist provides a modular, extensible framework for first-person shooter aim training with real-time performance tracking, customizable training modes, and comprehensive statistics analysis. The application features clean separation of concerns across multiple architectural layers, enabling maintainability and testability.

## Key Features

### Training System

- Multiple built-in modes: Static Sphere, GridShot, Next Shot, Tracking, Strafing
- Configurable session parameters (duration, distance, target properties)
- Real-time performance metrics and scoring
- Session pause/resume capability
- Customizable target appearance (color, size, resolution)

### Performance Tracking

- Real-time statistics: accuracy, reaction time, shots per second
- Session history with per-mode performance aggregation
- Performance tier classification (best, above average, average, below average, worst)
- Frame-time monitoring and diagnostics

### Persistence & Profiles

- SQLite-backed profile management with per-profile settings isolation
- Session history storage with queryable metrics
- Automatic schema versioning and migrations
- Legacy INI configuration import (one-time)

### User Interface

- Modern QML-based responsive UI with five screens
- Real-time metric visualization during training
- Dynamic light/dark theme system harmonized with target color
- Multi-language support (English, Turkish)

### Input System

- Physical sensitivity model (cm/360 + DPI)
- Raw input support for unaccelerated mouse movement
- Configurable yaw/pitch multipliers and Y-axis inversion
- FPS-style mouse capture

## Quick Start

### Installation

Download the latest release for your platform from [GitHub Releases](https://github.com/xaprier/XAimAssist/releases).

### Building from Source

See [BUILD.md](BUILD.md) for detailed build instructions for Linux, Windows, and macOS.

### Project Structure

```
XAimAssist/
├── src/
│ ├── CMakeLists.txt
│ ├── app/ # Application composition and lifecycle
│ ├── core/ # EventBus, Logger, FrameClock
│ ├── engine/ # VTK rendering abstraction
│ ├── world/ # Entity-Component-System
│ ├── gameplay/ # Game modes and target system
│ ├── input/ # Input handling and sensitivity
│ ├── stats/ # Statistics tracking
│ ├── persistence/ # SQLite persistence layer
│ └── ui/ # QML interface and view models
├── CMakeLists.txt
├── BUILD.md # Build instructions
└── README.md # This file
```

### Module Responsibilities

**app**: Application lifecycle, dependency injection, Qt/VTK integration.  
**core**: EventBus (pub/sub), Logger, FrameClock (fixed timestep).  
**engine**: VTK abstraction (Engine, RenderContext, Scene, CameraBackend).  
**world**: ECS (World, Transform/Render/Collider components, render sync).  
**gameplay**: Game mode framework, TargetSystem, built-in modes.  
**input**: InputManager, SensitivityModel (cm/360 calculations).  
**stats**: StatTracker, performance tier classification.  
**persistence**: SQLite database, ProfileManager, SessionHistory, SettingsManager.  
**ui**: QML views, UiViewModel bridge, theme/localization.

## Documentation

Full API documentation is available at [xaprier.github.io/XAimAssist](https://xaprier.github.io/XAimAssist).

Generate documentation locally:

```
doxygen Doxyfile
xdg-open docs/html/index.html
```

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please:

1. Fork the repository and create a feature branch
2. Follow existing code style
3. Add Doxygen documentation for public APIs
4. Test in both Debug and Release builds
5. Submit a pull request with clear description

## Build

See [BUILD.md](BUILD.md) file for build instructions and requirements.
