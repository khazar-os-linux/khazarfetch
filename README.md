# KhazarFetch

**KhazarFetch** is a lightweight, modular system information tool written in C++17.  
It displays hardware, desktop, terminal, disk, USB, and package info with Nerd Font icons and terminal-native colors.

**Version:** Alpha 0.2.5

## Features

- **System Info** — User, Hostname, OS, Kernel, Uptime
- **Hardware Info** — CPU, GPU (integrated/discrete), RAM, Swap
- **Desktop Info** — DE, Resolution (X11 & Wayland), Display Server, Compositor
- **Terminal Info** — Terminal emulator & running shell with version
- **Disk Info** — Block devices with partition mount points
- **USB Devices** — via `lsusb` or sysfs fallback
- **Package Info** — Package count (pacman, dpkg, rpm, xbps, apk, portage) & repositories
- **Nerd Font icons** for disk/USB sections
- **Terminal-native color palette** — respects your terminal theme
- **Parallel execution** — all info getters run concurrently via `std::async`
- **Safe process execution** — `fork`/`exec`/`pipe` with 5s timeout and SIGKILL cleanup
- **PPID tree walking** — terminal & shell detection via `/proc` (no external deps)

## Requirements

### Build
- **C++17 compiler** (g++ or clang++)
- **make**

### Runtime (optional)
| Package | Purpose |
|---------|---------|
| `nerd-fonts` | Icon glyphs in disk/USB output |
| `xorg-xrandr` | X11 resolution detection |
| `wlr-randr` | Wayland resolution detection (generic) |
| `kscreen-doctor` | KDE Wayland resolution detection |
| `hyprctl` | Hyprland resolution/compositor detection |
| `usbutils` | USB device listing (`lsusb`) |
| `inetutils` | Hostname fallback |

## Installation

### Build from Source
```bash
git clone https://github.com/khazar-os-linux/khazarfetch.git
cd khazarfetch
make
sudo make install
```

### Arch Linux (PKGBUILD)
```bash
git clone https://github.com/khazar-os-linux/khazarfetch.git
cd khazarfetch
makepkg -si
```

## Uninstall

- Installed with `make`: `sudo make uninstall`
- Installed with PKGBUILD: `sudo pacman -R khazarfetch`

## Project Structure

```
src/
  main.cpp         — entry point, parallel execution, output formatting
  utils.cpp/h      — trim(), exec(), PPID tree walk, is_shell(), map_terminal_name()
  system_info.cpp/h— username, hostname, OS, kernel, uptime
  hardware.cpp/h   — CPU, GPU, RAM, Swap (via /proc/meminfo)
  desktop.cpp/h    — DE, resolution, display server, compositor
  terminal.cpp/h   — terminal emulator & shell detection
  disk.cpp/h       — disk usage (lsblk), USB devices (lsusb/sysfs)
  packages.cpp/h   — package count, repository list
  colors.h         — terminal-native color macros
```

## License

GPL-3.0
