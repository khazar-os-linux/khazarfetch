# Changelog

All notable changes to KhazarFetch are documented here.

---

## [Alpha 0.2.5] — 2026-06-01

### Added

- **Modular architecture** — refactored from single 644-line file into 8 source modules under `src/`
- **Compositor detection** (`get_compositor`) — loginctl, XDG_CURRENT_DESKTOP mapping (KDE→kwin_wayland, GNOME→mutter, Sway, Hyprland, wlroots), pidof fallbacks
- **Package count** (`get_package_count`) — pacman (directory count), dpkg, rpm, xbps, apk, portage (`/var/db/pkg` iteration)
- **Shell version detection** — `BASH_VERSION`/`ZSH_VERSION` env vars, `fish --version`, generic `<shell> --version`
- **Wayland compositor-specific resolution** — kscreen-doctor, hyprctl monitors JSON, niri msg outputs, wayland-info, wlr-randr fallback
- **Sysfs USB fallback** — reads `/sys/bus/usb/devices/*/` idVendor/manufacturer/product when `lsusb` unavailable
- **Nerd Font icons** — disk and USB sections use Material Design Icons (󰋊, 󰊴, 󰉋)
- **Terminal-native color palette** — uses 8-bit color index (`\033[38;5;2m`) instead of hardcoded RGB, respects terminal theme
- **Parallel execution** — all info getters launched via `std::async(std::launch::async, ...)`
- **Exception handling** — `try/catch` in `main()` for graceful error reporting
- **PKGBUILD optdepends** — nerd-fonts, kscreen-doctor, hyprctl, inetutils
- **Makefile RECOMMENDS** — nerd-fonts listed

### Changed

- **`exec()` rewritten** — replaced `popen`/`pclose`+`std::async` with `fork`/`exec`/`pipe`; 5-second timeout with SIGKILL process group cleanup; `setpgid` in both parent and child to prevent race; 10ms sleep in WNOHANG poll loop to avoid busy-wait
- **Terminal detection** — PPID tree walking via `/proc/<pid>/comm` + `/proc/<pid>/stat`, 15+ env var checks, 35+ pretty name mappings, shell/intermediary skip list
- **Shell detection** — PPID tree walking first, `$SHELL` as fallback, all `is_shell()` names handled
- **Memory info** — `/proc/meminfo` direct read with `MemAvailable`; `get_memory_and_swap()` reads once for both RAM and Swap
- **Disk parsing** — `parse_lsblk_line()` handles variable field counts (4 vs 5 fields when MOUNTPOINT empty); merged duplicate `lsblk` calls into single `lsblk -nro`
- **Portage repo detection** — `find_repo_sections("/etc/portage/repos.conf/", ".conf")` instead of empty glob
- **PKGBUILD source** — uses `${url}` variable reference
- **`.SRCINFO`** — synced with PKGBUILD (added nerd-fonts, makedepends)

### Fixed

- **`hyprctl monitors -j` grep pattern** — `"res":\s*\K[0-9x]+` matches JSON integer format (was looking for quoted string)
- **`loginctl` compositor lookup** — `awk -v u=$(whoami) '$3==u{print $1; exit}'` instead of fragile `grep $(whoami)` which matched username anywhere in line
- **`detect_running_shell()` whitelist drift** — now maps all shells recognized by `is_shell()`, not just a hardcoded subset
- **`fix_tree_chars()` regex recompilation** — regex objects made `static const` to avoid recompilation on every call from multiple async tasks
- **`is_shell()` O(n) lookup** — converted from `std::vector` linear scan to `std::unordered_set` O(1) lookup
- **Uptime null check** — `if (!(uptime_file >> uptime_seconds)) return "Unknown"`
- **Disk parsing** — variable lsblk field count when MOUNTPOINT is empty
- **Missing `<thread>` include** — `std::this_thread::sleep_for` in `exec()` WNOHANG loop
- **Duplicate `#include <cstring>`** in utils.cpp

### Removed

- **6 unused color macros** — RED, YELLOW, BLUE, MAGENTA, CYAN, WHITE
- **6 unused color functions** — `color_256()`, `color_rgb()`, `color_bold()`, `color_dim()`, `color_italic()`, `color_underline()`
- **`get_memory_info()` / `get_swap_usage()`** from public API — made static, removed from `hardware.h`
- **Monolithic `khazarfetch.cpp`** — replaced by modular `src/` structure
- **Duplicate `MAKEFILE`** file
- **ASCII logo** — removed from output
- **Hardcoded RGB green** — replaced with terminal palette index

### Internal

- **`walk_ppid_tree()`** — shared PPID tree walk extracted to `utils.cpp/h` with `PidEntry` struct; eliminates duplicated walk in `terminal.cpp`
- **`trim()` utility** — DRY fix, used in 23+ places across codebase
- **`dequote()` helper** — extracted in `system_info.cpp`, replaced 3 inline dequote blocks
- **`format_disk_line()`** — helper in `disk.cpp` for consistent disk output formatting
- **`read_meminfo()` / `MemInfo` struct** — single `/proc/meminfo` read for both RAM and Swap
- **`count_dir_entries()`** — zero-fork directory count for pacman/portage package counting
- **`format_size()`** — shared KB/MB/GB formatting for memory values
