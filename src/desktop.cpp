#include "desktop.h"
#include "utils.h"

#include <fstream>
#include <string>
#include <cstdlib>

static std::string get_wayland_resolution() {
    std::string result;

    result = trim(exec("kscreen-doctor --outputs 2>/dev/null | grep -oP '\\d+x\\d+' | head -1"));
    if (!result.empty()) return result;

    result = trim(exec("hyprctl monitors -j 2>/dev/null | grep -oP '\"res\":\\s*\\K[0-9x]+' | head -1"));
    if (!result.empty()) return result;

    result = trim(exec("niri msg outputs 2>/dev/null | grep -oP '\\d+x\\d+' | head -1"));
    if (!result.empty()) return result;

    result = trim(exec("wayland-info 2>/dev/null | grep -oP '\\d+x\\d+' | head -1"));
    if (!result.empty()) return result;

    result = trim(exec("wlr-randr 2>/dev/null | grep -B1 \"Enabled\" | grep -v \"Enabled\" | awk '{print $2}'"));
    if (!result.empty()) return result;

    return "";
}

std::string get_desktop_environment() {
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    if (desktop && desktop[0]) return desktop;

    const char* session = getenv("DESKTOP_SESSION");
    if (session && session[0]) return session;

    const char* session_desktop = getenv("XDG_SESSION_DESKTOP");
    if (session_desktop && session_desktop[0]) return session_desktop;

    return "Unknown";
}

std::string get_resolution() {
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    if (wayland_display) {
        std::string wl_result = get_wayland_resolution();
        if (!wl_result.empty()) return wl_result;
    }

    std::string xrandr_result = trim(exec("xrandr --current 2>/dev/null | grep '*' | awk '{print $1}'"));
    if (!xrandr_result.empty()) return xrandr_result;

    std::string cmd = "for p in /sys/class/drm/*/modes; do if [ -f \"$p\" ]; then head -1 \"$p\"; fi; done | head -1";
    std::string result = trim(exec(cmd.c_str()));
    if (!result.empty()) return result;

    return "Unknown";
}

std::string get_display_server() {
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    const char* x_display = getenv("DISPLAY");

    if (wayland_display) return "Wayland";
    if (x_display) return "X11";
    return "Unknown";
}

std::string get_compositor() {
    std::string result = trim(exec("loginctl show-session $(loginctl | awk -v u=$(whoami) '$3==u{print $1; exit}') -p Compositor 2>/dev/null | cut -d= -f2"));
    if (!result.empty() && result != "") return result;

    const char* xdg_current = getenv("XDG_CURRENT_DESKTOP");
    if (xdg_current && xdg_current[0]) {
        std::string de = xdg_current;
        if (de.find("KDE") != std::string::npos) return "kwin_wayland";
        if (de.find("GNOME") != std::string::npos) return "mutter";
        if (de.find("SWAY") != std::string::npos || de.find("sway") != std::string::npos) return "sway";
        if (de.find("Hyprland") != std::string::npos) return "Hyprland";
        if (de.find("wlroots") != std::string::npos) return "wlroots";
    }

    std::string hypr = trim(exec("hyprctl version 2>/dev/null | head -1"));
    if (!hypr.empty()) return "Hyprland";

    std::string sway_pid = trim(exec("pidof sway 2>/dev/null"));
    if (!sway_pid.empty()) return "sway";

    std::string kwin_pid = trim(exec("pidof kwin_wayland 2>/dev/null"));
    if (!kwin_pid.empty()) return "kwin_wayland";

    std::string mutter_pid = trim(exec("pidof mutter 2>/dev/null"));
    if (!mutter_pid.empty()) return "mutter";

    return "Unknown";
}
