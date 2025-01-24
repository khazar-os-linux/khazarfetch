# Khazarfetch

**Khazarfetch** is a lightweight and minimalistic tool designed to display system information in a clean and elegant way.

## Features
- Displays CPU, RAM, and system details.
- Minimal and user-friendly design.
- Fast and resource-efficient.

## Requirements
- **C++ Compiler** (e.g., g++, clang).
- Linux or any Unix-based operating system.

## Installation

### Build from Source
1. Clone this repository:
   ```bash
   git clone https://github.com/khazar-os-linux/khazarfetch.git
   cd khazarfetch
   make

2. Build with PKGBUILD. (Arch Only)
   ```bash
   git clone https://github.com/khazar-os-linux/khazarfetch.git
   cd khazarfetch
   makepkg -si

## Uninstall

1. Installed With `make`:
   Just run `make uninstall`.

2. Installed With PKGBUILD:
   `sudo pacman -R khazarfetch`
