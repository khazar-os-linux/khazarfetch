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
            os_name = line.substr(5);
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
    std::string result = exec("xdpyinfo | grep 'dimensions:' | awk '{print $2}'");
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
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
    std::string cmd = "df -h --output=source,size,used,pcent | grep '^/'";
    return exec(cmd.c_str());
}

// Function to get swap usage
std::string get_swap_usage() {
    std::string cmd = "free -h --si | grep 'Swap:' | awk '{print $3 \"/\" $2}'";
    return exec(cmd.c_str());
}

// Function to get installed repositories
std::string get_repositories() {
    std::string repositories;

    // Check Pacman repositories
    std::ifstream pacman_conf("/etc/pacman.conf");
    std::string line;
    std::regex repo_regex("^\\[(.+)\\]");
    std::smatch match;

    while (std::getline(pacman_conf, line)) {
        if (std::regex_search(line, match, repo_regex)) {
            repositories += match[1].str() + " ";
        }
    }

    return repositories.empty() ? "Unknown" : repositories;
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
    std::string cmd = "lspci | grep -i 'vga\\|3d' | cut -d ':' -f3";
    std::string result = exec(cmd.c_str());
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
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

int main() {
    std::cout << "\n";
    
    // Sistem Bilgileri
    std::cout << GREEN << "  💻 SYSTEM INFO" << RESET << "\n";
    std::cout << "  ├─ 👤 User: " << RESET << get_username() << "\n";
    std::cout << "  ├─ 🖨️  OS: " << RESET << get_os_info() << "\n";
    std::cout << "  ├─ 📟 Kernel: " << RESET << get_kernel() << "\n";
    std::cout << "  └─ ⏰ Uptime: " << RESET << get_uptime() << "\n";
    
    std::cout << "\n";
    
    // Donanım Bilgileri
    std::cout << GREEN << "  🔧 HARDWARE INFO" << RESET << "\n";
    std::cout << "  ├─ 🖥️  CPU: " << RESET << get_cpu_info() << "\n";
    std::cout << "  ├─ 📼 GPU: " << RESET << get_gpu_info() << "\n";
    std::cout << "  ├─ 🗂  RAM: " << RESET << get_memory_info() << "\n";
    std::cout << "  └─ 💿 Swap: " << RESET << get_swap_usage() << "\n";
    
    std::cout << "\n";
    
    // Masaüstü Bilgileri
    std::cout << GREEN << "  🖥️ DESKTOP INFO" << RESET << "\n";
    std::cout << "  ├─ 🗔 DE: " << RESET << get_desktop_environment() << "\n";
    std::cout << "  ├─ 📺 Resolution: " << RESET << get_resolution() << "\n";
    std::cout << "  └─ 💾 Display Server: " << RESET << get_display_server() << "\n";
    
    std::cout << "\n";
    
    // Terminal Bilgileri
    std::cout << GREEN << "  📱 TERMINAL INFO" << RESET << "\n";
    std::cout << "  ├─ 🖫 Terminal: " << RESET << get_terminal() << "\n";
    std::cout << "  └─ 🐚 Shell: " << RESET << get_shell() << "\n";
    
    std::cout << "\n";
    
    // Disk Bilgileri
    std::cout << GREEN << "  💽 DISK INFO" << RESET << "\n";
    std::cout << "  └─ " << RESET << get_disk_usage() << "\n";
    
    std::cout << "\n";
    
    // Paket Bilgileri
    std::cout << GREEN << "  📦 PACKAGE INFO" << RESET << "\n";
    std::cout << "  └─ Repos: " << RESET << get_repositories() << "\n";
    
    std::cout << "\n";

    return 0;
}