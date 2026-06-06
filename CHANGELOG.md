# Changelog

All notable changes to KhazarFetch are documented here.

---

## [Beta 0.1.6] — 2026-06-06

### Added

- **Unicode display width calculation** — `unicode_display_width()` in `utils.cpp/h` with full UTF-8 decoding and East Asian Width lookup table; correctly handles multi-byte characters, variation selectors (FE00–FE0F), skin tone modifiers (1F3FB–1F3FF), tag characters (E0020–E007F), and zero-width joiner (200D)
- **UTF-8 continuation byte validation** — `unicode_display_width()` validates `10xxxxxx` pattern on continuation bytes; malformed sequences are skipped instead of producing incorrect codepoints

### Changed

- **`row()` label alignment** — uses `unicode_display_width(lbl)` instead of `lbl.size()` (byte count) for correct padding with multi-byte labels
- **`pad_left()` value alignment** — uses `unicode_display_width(s)` instead of `s.size()`; adds negative padding guard (`padding < 0 → 0`) to prevent `std::string` overflow on platforms where `size_t` is unsigned
- **`east_asian_width()` table** — comprehensive Unicode 15.0 East Asian Width ranges; PUA (0xE000–0xF8FF, Nerd Fonts) correctly returns width 1; CJK blocks properly bounded (0x4E00–0x9FFF, Hangul 0xAC00–0xD7AF, CJK Compat 0xF900–0xFAFF, Halfwidth/Fullwidth 0xFF01–0xFF60, etc.)

### Fixed

- **Cross-terminal alignment corruption** — `row()` and `pad_left()` used byte length (`s.size()`) instead of display width, causing misalignment when values contained multi-byte UTF-8 characters (e.g., shell version strings, GPU names with special chars)
- **`east_asian_width()` overlapping ranges** — removed dead code: `cp < 0x1160+1` (line 153), `0x2B1B–0x2B55` superset (line 193) that subsumed 3 prior ranges, `0x4E00–0x10FFFF` superset (line 197) that subsumed 5 emoji/supplementary plane ranges
- **`east_asian_width()` incorrect ranges** — `0x2B1B–0x2B55` incorrectly marked ~51 single-width codepoints (⬅⬆⬇ etc.) as width 2; `0x4E00–0x10FFFF` incorrectly marked PUA (0xE000–0xF8FF, used by Nerd Fonts) as width 2, causing over-padding

---

## [Beta 0.1.5] — 2026-06-04

### Added

- **DRM sysfs EDID parsing** — reads `/sys/class/drm/*/edid` directly; parses monitor name from descriptor tag 0xFC, resolution and refresh rate from Detailed Timing Descriptor (DTD); validates EDID header (00 FF FF FF FF FF FF 00) and checksum; fallback to `/sys/class/drm/*/modes` when EDID unavailable
- **GPU name abbreviation** — `clean_gpu_name()` strips vendor prefixes (NVIDIA Corporation, AMD/ATI, Intel Corporation, Advanced Micro Devices) and revision suffixes (`(rev XX)`) from `lspci` output
- **Tabular section headers** — `user@host` displayed as `── user@host ──` section header instead of standalone line
- **Progress bar alignment** — RAM, Swap, and Disk size values left-padded to fixed width (7 chars) so all progress bars start at the same column

### Changed

- **Monitor detection priority chain** — xrandr (X11) → kscreen-doctor (KDE Wayland only) → hyprctl (Hyprland) → wlr-randr (wlroots) → DRM sysfs EDID (universal fallback); kscreen-doctor is now tried only when `XDG_CURRENT_DESKTOP` contains "KDE", not as a generic Wayland method
- **No-color output** — all ANSI color codes removed; `colors.h` is now empty
- **USB section removed entirely** — `get_usb_devices()` deleted from disk.cpp/h, main.cpp, PKGBUILD, .SRCINFO
- **Disk info** — switched from `lsblk` to `df -h` for percentage data; deduplicates Btrfs subvolumes by source device name; filters /snap and /boot
- **Memory/swap structs** — `MemoryStats` public struct replaces `get_memory_and_swap()`; `format_size()` now public in `hardware.h`
- **Display section** — multi-monitor support with `MonitorInfo` struct (name, resolution, refresh); single monitor shows `resolution @Hz`, multiple show name per line

### Fixed

- **EDID DTD byte layout** — correct VESA EDID 1.4 nibble packing: `ha = byte2 | ((byte4 >> 4) << 8)`, `hb = byte3 | ((byte4 & 0x0F) << 8)` for horizontal; same pattern for vertical
- **`std::remove` ADL conflict** — `<cstdio>` `remove()` conflicted with `std::remove`; replaced with `std::remove_if` + lambda
- **Disk dedup comparison** — comparison used `d.name == source` but `d.name` was `source.substr(5)` — fixed to `d.name == source.substr(5)`
- **Main.cpp extra `}`** — was closing try block early
- **GPU name** — was showing full `lspci` output (e.g. `NVIDIA Corporation TU117M [GeForce GTX 1650 Mobile / Max-Q] (rev a1)`); now abbreviated to `TU117M [GeForce GTX 1650 Mobile / Max-Q]`

### Removed

- **kscreen-doctor as generic Wayland method** — only used when KDE is detected via `XDG_CURRENT_DESKTOP`
- **ASCII logo/frame** — `print_logo()` deleted
- **Color macros** — `COLOR`, `LABEL`, `LOW`, `MED`, `HIGH`, `DIM`, `RESET` removed; `colors.h` is empty

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
