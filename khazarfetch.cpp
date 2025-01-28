#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <array>
#include <vector>
#include <regex>
#include <filesystem>
#include <locale>
#include <unistd.h>
#include <future>
#include <chrono>
#include <thread>
#include <map>

#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"
#define RESET   "\033[0m"

// Khazar OS logo ASCII art

using namespace std;

// Function to execute a command
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto timeout = std::chrono::seconds(5); // 5 seconds timeout
    std::future<std::string> future_result = std::async(std::launch::async, [cmd]() -> std::string {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    });

    if (future_result.wait_for(timeout) == std::future_status::timeout) {
        return "Timeout or Error";
    }

    return future_result.get();
}

// Function to get the username
std::string get_username() {
    std::string result = exec("whoami");
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

// Function to get the hostname
std::string get_hostname() {
    std::string result = exec("hostname");
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

// Function to get OS information
std::string get_os_info() {
    std::ifstream os_release("/etc/os-release");
    std::string line;
    std::string os_name;
    
    while (std::getline(os_release, line)) {
        if (line.find("NAME=") == 0) {
            os_name = line.substr(6);
            if (os_name.back() == '\"') {
                os_name.pop_back(); // Remove trailing quote
            }
            break;
        }
    }

    return os_name;
}

// Function to get the kernel version
std::string get_kernel() {
    std::string result = exec("uname -r");
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

// Function to get system uptime
std::string get_uptime() {
    std::ifstream uptime_file("/proc/uptime");
    double uptime_seconds;
    uptime_file >> uptime_seconds;
    int hours = static_cast<int>(uptime_seconds / 3600);
    int minutes = static_cast<int>((uptime_seconds - (hours * 3600)) / 60);
    return std::to_string(hours) + " hour " + std::to_string(minutes) + " minute";
}

// Function to get screen resolution
std::string get_resolution() {
    // Wayland için kontrol
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    if (wayland_display) {
        // Wayland için çözünürlük alma komutu
        std::string wayland_cmd = "echo $WAYLAND_DISPLAY >/dev/null && wlr-randr 2>/dev/null | grep -B1 \"Enabled\" | grep -v \"Enabled\" | awk '{print $2}'";
        std::string wayland_result = exec(wayland_cmd.c_str());
        if (!wayland_result.empty() && wayland_result != "Timeout or Error") {
            wayland_result.erase(wayland_result.find_last_not_of(" \n\r\t") + 1);
            return wayland_result;
        }
    }

    // X11 için xrandr'ı dene
    std::string xrandr_result = exec("xrandr --current 2>/dev/null | grep '*' | awk '{print $1}'");
    if (!xrandr_result.empty() && xrandr_result != "Timeout or Error") {
        xrandr_result.erase(xrandr_result.find_last_not_of(" \n\r\t") + 1);
        return xrandr_result;
    }

    // Son çare olarak /sys/class/drm'den oku
    std::string cmd = "for p in /sys/class/drm/*/modes; do if [ -f \"$p\" ]; then head -1 \"$p\"; fi; done | head -1";
    std::string result = exec(cmd.c_str());
    
    if (!result.empty() && result != "Timeout or Error") {
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
        return result;
    }

    return "Unknown";
}

// Function to get CPU information
std::string get_cpu_info() {
    std::string cmd = "lscpu | grep 'Model name' | awk -F: '{print $2}'";
    std::string cpu_info = exec(cmd.c_str());
    
    // Remove leading and trailing whitespace
    cpu_info.erase(0, cpu_info.find_first_not_of(" \n\r\t"));
    cpu_info.erase(cpu_info.find_last_not_of(" \n\r\t") + 1);

    return cpu_info;
}

// Function to get memory information
std::string get_memory_info() {
    std::string total_cmd = "free -h --si | grep 'Mem:' | awk '{print $2}'";
    std::string used_cmd = "free -h --si | grep 'Mem:' | awk '{print $3}'";
    
    std::string total_mem = exec(total_cmd.c_str());
    std::string used_mem = exec(used_cmd.c_str());

    // Remove newline characters and extra spaces
    total_mem.erase(total_mem.find_last_not_of(" \n\r\t") + 1);
    used_mem.erase(used_mem.find_last_not_of(" \n\r\t") + 1);

    return "Used: " + used_mem + " / Total: " + total_mem;
}

// Function to get terminal emulator
std::string get_terminal() {
    // Önce TERM_PROGRAM değişkenini kontrol et
    const char* term_program = getenv("TERM_PROGRAM");
    if (term_program) {
        return term_program;
    }

    // Process parent'ını kontrol et
    std::string ppid_cmd = "ps -p $PPID -o comm=";
    std::string parent_process = exec(ppid_cmd.c_str());
    
    // Boşlukları temizle
    parent_process.erase(0, parent_process.find_first_not_of(" \n\r\t"));
    parent_process.erase(parent_process.find_last_not_of(" \n\r\t") + 1);

    // Yaygın terminal emülatörlerini kontrol et
    if (parent_process.find("konsole") != std::string::npos) {
        return "Konsole";
    } else if (parent_process.find("gnome-terminal") != std::string::npos) {
        return "GNOME Terminal";
    } else if (parent_process.find("xfce4-terminal") != std::string::npos) {
        return "XFCE Terminal";
    } else if (parent_process.find("mate-terminal") != std::string::npos) {
        return "MATE Terminal";
    } else if (parent_process.find("kitty") != std::string::npos) {
        return "Kitty";
    } else if (parent_process.find("alacritty") != std::string::npos) {
        return "Alacritty";
    } else if (parent_process.find("terminator") != std::string::npos) {
        return "Terminator";
    } else if (parent_process.find("tilix") != std::string::npos) {
        return "Tilix";
    } else if (parent_process.find("urxvt") != std::string::npos) {
        return "URxvt";
    } else if (parent_process.find("terminology") != std::string::npos) {
        return "Terminology";
    }

    // Son çare olarak TERM değişkenini kontrol et
    const char* term = getenv("TERM");
    if (term) {
        if (std::string(term) == "xterm-kitty") {
            return "Kitty";
        } else if (std::string(term) == "rxvt-unicode-256color") {
            return "URxvt";
        }
    }

    return "Unknown Terminal";
}

// Function to get shell information
std::string get_shell() {
    char* shell = getenv("SHELL");
    return shell ? std::string(shell) : "Unknown";
}

// Function to get disk usage
std::string get_disk_usage() {
    std::string result;

    // USB disklerini ve bölümlerini önce listele
    std::string usb_cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,TRAN | "
                         "awk '($5==\"usb\") && ($3==\"disk\" || $3==\"part\") {"
                         "if ($3==\"disk\") {"
                         "  printf \"|- 󰊴 %s (%s)\\n\", $1, $2"
                         "} else if ($4!=\"\") {"
                         "  printf \"    󰉋 %s (%s) at %s\\n\", $1, $2, $4"
                         "}}'";
    
    std::string usb_disks = exec(usb_cmd.c_str());

    if (!usb_disks.empty()) {
        result += usb_disks + "\n";
    }

    // Ana diskleri ve bölümlerini sonra listele (USB hariç)
    std::string main_cmd = "lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,TRAN | "
                           "awk '($3==\"disk\" || $3==\"part\") && $5!=\"usb\" {"
                           "if ($3==\"disk\") {"
                           "  printf \"|- 󰋊 %s (%s)\\n\", $1, $2"
                           "} else if ($4!=\"\") {"
                           "  printf \"    󰉋 %s (%s) at %s\\n\", $1, $2, $4"
                           "}}'";
    
    std::string main_disks = exec(main_cmd.c_str());

    if (!main_disks.empty()) {
        result += main_disks;
    }

    // Boşlukları temizle
    if (!result.empty()) {
        result.erase(0, result.find_first_not_of(" \n\r\t"));
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
    }

    return result;
}

// Function to get swap usage
std::string get_swap_usage() {
    std::string cmd = "free -h --si | grep 'Swap:' | awk '{print $3 \"/\" $2}'";
    return exec(cmd.c_str());
}

// Function to get installed repositories
std::string get_repositories() {
    std::string repositories;

    // Pacman (Arch Linux)
    if (std::filesystem::exists("/etc/pacman.conf")) {
        std::ifstream pacman_conf("/etc/pacman.conf");
        std::string line;
        std::regex repo_regex("^\\[(.+)\\]");
        std::smatch match;

        while (std::getline(pacman_conf, line)) {
            if (std::regex_search(line, match, repo_regex)) {
                std::string repo = match[1].str();
                if (repo != "options") { // options bölümünü atlayalım
                    repositories += repo + " ";
                }
            }
        }
        if (!repositories.empty()) {
            return repositories;
        }
    }

    // APT (Debian/Ubuntu)
    if (std::filesystem::exists("/etc/apt/sources.list")) {
        std::string cmd = "grep -h '^deb ' /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null | awk '{print $2}' | sort -u";
        std::string apt_repos = exec(cmd.c_str());
        if (!apt_repos.empty() && apt_repos != "Timeout or Error") {
            return apt_repos;
        }
    }

    // DNF/YUM (Fedora/RHEL)
    if (std::filesystem::exists("/etc/yum.repos.d/")) {
        std::string cmd = "find /etc/yum.repos.d/ -type f -name '*.repo' -exec grep -h '\\[.*\\]' {} \\; | tr -d '[]'";
        std::string dnf_repos = exec(cmd.c_str());
        if (!dnf_repos.empty() && dnf_repos != "Timeout or Error") {
            return dnf_repos;
        }
    }

    // Zypper (openSUSE)
    if (std::filesystem::exists("/etc/zypp/repos.d/")) {
        std::string cmd = "find /etc/zypp/repos.d/ -type f -name '*.repo' -exec grep -h '\\[.*\\]' {} \\; | tr -d '[]'";
        std::string zypper_repos = exec(cmd.c_str());
        if (!zypper_repos.empty() && zypper_repos != "Timeout or Error") {
            return zypper_repos;
        }
    }

    // Portage (Gentoo)
    if (std::filesystem::exists("/etc/portage/repos.conf/")) {
        std::string cmd = "find /etc/portage/repos.conf/ -type f -exec grep -h '\\[.*\\]' {} \\; | tr -d '[]'";
        std::string portage_repos = exec(cmd.c_str());
        if (!portage_repos.empty() && portage_repos != "Timeout or Error") {
            return portage_repos;
        }
    }

    // XBPS (Void Linux)
    if (std::filesystem::exists("/usr/share/xbps.d/")) {
        std::string cmd = "grep -h '^repository=' /usr/share/xbps.d/*.conf /etc/xbps.d/*.conf 2>/dev/null | cut -d'=' -f2";
        std::string xbps_repos = exec(cmd.c_str());
        if (!xbps_repos.empty() && xbps_repos != "Timeout or Error") {
            return xbps_repos;
        }
    }

    // Alpine Linux
    if (std::filesystem::exists("/etc/apk/repositories")) {
        std::string cmd = "cat /etc/apk/repositories | grep -v '^#' | awk '{print $1}'";
        std::string alpine_repos = exec(cmd.c_str());
        if (!alpine_repos.empty() && alpine_repos != "Timeout or Error") {
            return alpine_repos;
        }
    }

    return "Unknown";
}

// Function to get display server
std::string get_display_server() {
    const char* wayland_display = getenv("WAYLAND_DISPLAY");
    const char* x_display = getenv("DISPLAY");
    
    if (wayland_display) {
        return "Wayland";
    } else if (x_display) {
        return "X11";
    }
    return "Unknown";
}

// Function to get GPU information
std::string get_gpu_info() {
    std::string result = "|- GPU\n";
    
    // Integrated GPUs (Intel, AMD)
    std::string integrated_cmd = "lspci | grep -i 'vga\\|3d\\|display' | grep -i '\\(intel\\|amd\\).*\\(graphics\\|igp\\|integrated\\)' | cut -d ':' -f3";
    std::string integrated_gpu = exec(integrated_cmd.c_str());
    
    // Dedicated/Discrete GPUs (NVIDIA, AMD)
    std::string discrete_cmd = "lspci | grep -i 'vga\\|3d\\|display' | grep -i 'nvidia\\|amd\\|radeon' | grep -vi 'integrated' | cut -d ':' -f3";
    std::string discrete_gpu = exec(discrete_cmd.c_str());
    
    // Clean up integrated GPU result
    if (!integrated_gpu.empty()) {
        integrated_gpu.erase(0, integrated_gpu.find_first_not_of(" \n\r\t"));
        integrated_gpu.erase(integrated_gpu.find_last_not_of(" \n\r\t") + 1);
    }
    
    // Clean up discrete GPU result
    if (!discrete_gpu.empty()) {
        discrete_gpu.erase(0, discrete_gpu.find_first_not_of(" \n\r\t"));
        discrete_gpu.erase(discrete_gpu.find_last_not_of(" \n\r\t") + 1);
    }
    
    // Add results to output
    result += "    |- Integrated: " + (integrated_gpu.empty() ? "None" : integrated_gpu) + "\n";
    result += "    |- Discrete: " + (discrete_gpu.empty() ? "None" : discrete_gpu);
    
    return result;
}

// Function to get desktop environment
std::string get_desktop_environment() {
    const char* desktop = getenv("XDG_CURRENT_DESKTOP");
    const char* session = getenv("DESKTOP_SESSION");
    
    if (desktop) {
        return desktop;
    } else if (session) {
        return session;
    }
    return "Unknown";
}

// Function to get USB devices
std::string get_usb_devices() {
    std::string result;
    
    // USB cihazlarını listele
    std::string cmd = "lsusb | awk '{$1=$2=\"\"; print $0}' | sed 's/^[ \\t]*/    |- /'";
    std::string usb_devices = exec(cmd.c_str());
    
    if (!usb_devices.empty() && usb_devices != "Timeout or Error") {
        // Boşlukları temizle
        usb_devices.erase(usb_devices.find_last_not_of(" \n\r\t") + 1);
        result = usb_devices;
    } else {
        result = "    |- No USB devices found";
    }
    
    return result;
}

int main() {
    std::cout << "\n";
    
    // System Information
    std::cout << GREEN << "╭─" << RESET << " SYSTEM INFO\n";
    std::cout << GREEN << "├" << RESET << " User: " << RESET << get_username() << "\n";
    std::cout << GREEN << "├" << RESET << " Hostname: " << RESET << get_hostname() << "\n";
    std::cout << GREEN << "├" << RESET << " OS: " << RESET << get_os_info() << "\n";
    std::cout << GREEN << "├" << RESET << " Kernel: " << RESET << get_kernel() << "\n";
    std::cout << GREEN << "╰" << RESET << " Uptime: " << RESET << get_uptime() << "\n";
    
    std::cout << "\n";
    
    // Hardware Information
    std::cout << GREEN << "╭─" << RESET << " HARDWARE INFO\n";
    std::cout << GREEN << "├" << RESET << " CPU: " << RESET << get_cpu_info() << "\n";
    std::string gpu_info = get_gpu_info();
    gpu_info = std::regex_replace(gpu_info, std::regex("\\|-"), "├");
    gpu_info = std::regex_replace(gpu_info, std::regex("    \\|-"), "│  ├");
    std::cout << GREEN << gpu_info << RESET << "\n";
    std::cout << GREEN << "╰" << RESET << " RAM: " << RESET << get_memory_info() << "\n";
    
    std::cout << "\n";
    
    // Desktop Information
    std::cout << GREEN << "╭─" << RESET << " DESKTOP INFO\n";
    std::cout << GREEN << "├" << RESET << " DE: " << RESET << get_desktop_environment() << "\n";
    std::cout << GREEN << "├" << RESET << " Resolution: " << RESET << get_resolution() << "\n";
    std::cout << GREEN << "╰" << RESET << " Display Server: " << RESET << get_display_server() << "\n";
    
    std::cout << "\n";
    
    // Terminal Information
    std::cout << GREEN << "╭─" << RESET << " TERMINAL INFO\n";
    std::cout << GREEN << "├" << RESET << " Terminal: " << RESET << get_terminal() << "\n";
    std::cout << GREEN << "╰" << RESET << " Shell: " << RESET << get_shell() << "\n";
    
    std::cout << "\n";
    
    // Disk Information
    std::cout << GREEN << "╭─" << RESET << " DISK INFO\n";
    std::string disk_info = get_disk_usage();
    disk_info = std::regex_replace(disk_info, std::regex("\\|-"), "├");
    disk_info = std::regex_replace(disk_info, std::regex("    \\|-"), "│  ├");
    std::cout << GREEN << disk_info << RESET << "\n";
    std::cout << GREEN << "╰" << RESET << "\n";
    
    std::cout << "\n";
    
    // USB Devices
    std::cout << GREEN << "╭─" << RESET << " USB DEVICES\n";
    std::string usb_info = get_usb_devices();
    usb_info = std::regex_replace(usb_info, std::regex("\\|-"), "├");
    std::cout << GREEN << usb_info << RESET << "\n";
    std::cout << GREEN << "╰" << RESET << "\n";
    
    std::cout << "\n";
    
    // Package Information
    std::cout << GREEN << "╭─" << RESET << " PACKAGE INFO\n";
    std::cout << GREEN << "╰" << RESET << " Repos: " << RESET << get_repositories() << "\n";
    
    std::cout << "\n";

    return 0;
}