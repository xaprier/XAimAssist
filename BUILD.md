# Building XAimAssist

This document provides detailed instructions for building XAimAssist from source on Linux, Windows, and macOS.

## Table of Contents

- [Prerequisites](#prerequisites)
    - [Required Tools](#required-tools)
    - [Required Dependencies](#required-dependencies)
- [Platform-Specific Instructions](#platform-specific-instructions)
    - [Linux](#linux)
        - [Debian/Ubuntu](#debianubuntu)
        - [Arch Linux](#arch-linux)
        - [Fedora/RHEL](#fedorarhel)
    - [Windows](#windows)
    - [macOS](#macos)
- [Build Configurations](#build-configurations)
    - [Debug Build](#debug-build)
    - [Release Build](#release-build)
    - [Custom Build](#custom-build)
- [CMake Presets](#cmake-presets)
    - [Configure Presets](#configure-presets)
    - [Build Presets](#build-presets)
    - [Usage](#usage)
- [Installation](#installation)
    - [Linux System-Wide Installation](#linux-system-wide-installation)
    - [Local Installation](#local-installation)

## Prerequisites

### Required Tools

**All Platforms:**

- CMake 3.24 or newer
- Git
- C++17 compatible compiler
- Ninja build system (recommended) or Make/MSBuild

**Compiler Requirements:**

- Linux: GCC 9.0+ or Clang 10.0+
- Windows: MSVC 2019+ (Visual Studio 2019 or newer)
- macOS: Apple Clang 12.0+ (Xcode 12+)

### Required Dependencies

**Qt 6.5 or newer** with the following modules:

- Qt6Core
- Qt6Gui
- Qt6Widgets
- Qt6OpenGL
- Qt6OpenGLWidgets
- Qt6Qml
- Qt6Quick
- Qt6QuickWidgets
- Qt6QuickControls2
- Qt6Sql

**VTK 9.2 or newer** with the following modules:

- VTK::CommonCore
- VTK::CommonColor
- VTK::CommonDataModel
- VTK::CommonExecutionModel
- VTK::CommonTransforms
- VTK::FiltersSources
- VTK::FiltersGeneral
- VTK::RenderingCore
- VTK::RenderingOpenGL2
- VTK::InteractionStyle
- VTK::GUISupportQt

**Graphics:**

- OpenGL 3.3+ capable graphics driver

## Platform-Specific Instructions

### Linux

#### Debian/Ubuntu

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-translations-l10n \
    libqt6sql6-sqlite \
    libvtk9-dev \
    libvtk9-qt-dev

# Clone and build
git clone https://github.com/xaprier/XAimAssist.git
cd XAimAssist
cmake --preset release
cmake --build --preset release
./build-release/src/app/XAimAssist
```

#### Arch Linux

```bash
# Install dependencies
sudo pacman -S --needed \
 base-devel \
 cmake \
 ninja \
 git \
 qt6-base \
 qt6-declarative \
 qt6-translations \
 vtk \
 openmp

# Clone and build
git clone https://github.com/xaprier/XAimAssist.git
cd XAimAssist
cmake --preset release
cmake --build --preset release
./build-release/src/app/XAimAssist
```

#### Fedora/RHEL

```bash
# Install dependencies
sudo dnf install -y \
 gcc-c++ \
 cmake \
 ninja-build \
 git \
 qt6-qtbase-devel \
 qt6-qtdeclarative-devel \
 qt6-qttranslations \
 vtk-devel \
 vtk-qt

# Clone and build
git clone https://github.com/xaprier/XAimAssist.git
cd XAimAssist
cmake --preset release
cmake --build --preset release
./build-release/src/app/XAimAssist
```

## Build Configurations

### Debug Build

Debug builds include symbols, assertions, and verbose logging:

```bash
cmake --preset default
cmake --build --preset default
./build/src/app/XAimAssist
```

Enable performance profiling:

```bash
PERF_EVENT=1 ./build/src/app/XAimAssist
```

### Release Build

Release builds optimize for performance and minimize binary size:

```bash
cmake --preset release
cmake --build --preset release
./build-release/src/app/XAimAssist
```

### Custom Build

Custom builds allow you to specify additional options:

```bash
cmake -B build-custom \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_TESTING=ON

cmake --build build-custom
```

## CMake Presets

Available presets (see [CMakePresets.json](CMakePresets.json)):

### Configure Presets:

- `default` - Debug build with Ninja
- `release` - Release build with Ninja

### Build Presets:

- `default` - Build default configuration
- `release` - Build release configuration

### Usage

```bash
# List available presets
cmake --list-presets

# Use preset
cmake --preset release
cmake --build --preset release
```

## Installation

### Linux System-Wide Installation

To install XAimAssist system-wide on Linux, run:

```bash
cmake --preset release
cmake --build --preset release
sudo cmake --install build-release --prefix /usr/local
```

Run installed application:

```bash
/usr/local/bin/XAimAssist
```

### Local Installation

To install XAimAssist locally, run:

```bash
cmake --preset release
cmake --build --preset release
cmake --install build-release --prefix ~/.local
```

Run installed application:

```bash
~/.local/bin/XAimAssist
```
