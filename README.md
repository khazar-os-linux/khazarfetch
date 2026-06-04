# KhazarFetch

**KhazarFetch** is a lightweight, modular system information tool written in C++17.
It displays hardware, desktop, terminal, disk, and package info in a clean tabular layout with no-color output.

**Version:** Beta 0.1.5

## Features

- **System Info** — User, Hostname, OS, Kernel, Uptime
- **Hardware Info** — CPU, GPU (abbreviated, integrated/discrete), RAM, Swap with progress bars
- **Desktop Info** — DE, Display Server, Compositor, Monitor resolution + refresh rate
- **Terminal Info** — Terminal emulator & running shell with version
- **Disk Info** — Disk usage with progress bars and mount points (df -h)
- **Package Info** — Package count (pacman, dpkg, rpm, xbps, apk, portage) & repositories
- **No-color output** — plain text, respects terminal theme
- **Parallel execution** — all info getters run concurrently via `std::async`
- **Safe process execution** — `fork`/`exec`/`pipe` with 5s timeout and SIGKILL cleanup
- **PPID tree walking** — terminal & shell detection via `/proc` (no external deps)
- **DRM sysfs EDID parsing** — monitor name and resolution from EDID DTD when no tool available

## Monitor Detection

Priority chain (first success returns):

1. **xrandr** — X11 (actual runtime refresh rate)
2. **kscreen-doctor** — KDE Wayland only (actual runtime refresh rate)
3. **hyprctl** — Hyprland (JSON, actual runtime refresh rate)
4. **wlr-randr** — wlroots-based compositors
5. **DRM sysfs EDID** — universal fallback; parses `/sys/class/drm/*/edid` for monitor name + DTD resolution/refresh

## Requirements

### Build
- **C++17 compiler** (g++ or clang++)
- **make**

### Runtime (optional)
| Package | Purpose |
|---------|---------|
| `xorg-xrandr` | X11 resolution + refresh rate detection |
| `kscreen-doctor` | KDE Wayland resolution + refresh rate |
| `hyprctl` | Hyprland resolution/compositor detection |
| `wlr-randr` | wlroots Wayland resolution detection |
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
  main.cpp        — entry point, tabular layout, progress bars, parallel execution
  utils.cpp/h     — trim(), exec(), PPID tree walk, is_shell(), map_terminal_name()
  system_info.cpp/h — username, hostname, OS, kernel, uptime
  hardware.cpp/h  — CPU, GPU (abbreviated), RAM, Swap (via /proc/meminfo)
  desktop.cpp/h   — DE, display server, compositor, monitor detection (EDID/xrandr/hyprctl/wlr-randr)
  terminal.cpp/h  — terminal emulator & shell detection
  disk.cpp/h      — disk usage (df -h), dedup by source device
  packages.cpp/h  — package count, repository list
  colors.h        — empty (no-color output)
```

## License

GPL-3.0
