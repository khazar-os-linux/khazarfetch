#include <iostream>
#include <string>
#include <memory>
#include <array>
#include <regex>
#include <filesystem>
#include <windows.h>
#include <wmi.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <sstream>

#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"
#define RESET   "\033[0m"

#pragma comment(lib, "wbemuuid.lib")

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    
    if (!pipe) {
        return "Error";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }
    
    return result;
}

std::string get_username() {
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserName(username, &username_len);
    return std::string(username);
}

std::string get_hostname() {
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostname_len = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerName(hostname, &hostname_len);
    return std::string(hostname);
}

std::string get_os_info() {
    std::string os_info = exec("wmic os get Caption /value");
    size_t pos = os_info.find("=");
    if (pos != std::string::npos) {
        return os_info.substr(pos + 1);
    }
    return "Windows";
}

std::string get_kernel() {
    std::string kernel = exec("wmic os get Version /value");
    size_t pos = kernel.find("=");
    if (pos != std::string::npos) {
        return kernel.substr(pos + 1);
    }
    return "Unknown";
}

std::string get_uptime() {
    DWORD tickCount = GetTickCount64() / 1000; // Convert to seconds
    int days = tickCount / (24 * 3600);
    int hours = (tickCount % (24 * 3600)) / 3600;
    int minutes = (tickCount % 3600) / 60;
    
    std::stringstream ss;
    if (days > 0) ss << days << " days, ";
    ss << hours << " hours, " << minutes << " minutes";
    return ss.str();
}

std::string get_cpu_info() {
    std::string cpu = exec("wmic cpu get name /value");
    size_t pos = cpu.find("=");
    if (pos != std::string::npos) {
        return cpu.substr(pos + 1);
    }
    return "Unknown CPU";
}

std::string get_gpu_info() {
    std::string result;
    std::string gpu_list = exec("wmic path win32_VideoController get name /value");
    std::istringstream stream(gpu_list);
    std::string line;
    bool first = true;
    
    while (std::getline(stream, line)) {
        if (line.find("=") != std::string::npos) {
            size_t pos = line.find("=");
            std::string gpu_name = line.substr(pos + 1);
            if (!gpu_name.empty()) {
                if (first) {
                    result += "    |- Discrete: " + gpu_name + "\n";
                    first = false;
                } else {
                    result += "    |- Integrated: " + gpu_name + "\n";
                }
            }
        }
    }
    
    return result;
}

std::string get_memory_info() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG usedPhysMem = totalPhysMem - memInfo.ullAvailPhys;
    
    double totalGB = totalPhysMem / (1024.0 * 1024 * 1024);
    double usedGB = usedPhysMem / (1024.0 * 1024 * 1024);
    
    char buffer[100];
    sprintf(buffer, "%.1fG / %.1fG", usedGB, totalGB);
    return std::string(buffer);
}

std::string get_disk_info() {
    std::string result;
    std::string cmd = "wmic logicaldisk get caption,size,freespace /format:list";
    std::string output = exec(cmd.c_str());
    std::istringstream stream(output);
    std::string line;
    
    std::map<std::string, std::pair<double, double>> disks; // drive letter -> {free, total}
    std::string current_drive;
    
    while (std::getline(stream, line)) {
        if (line.find("Caption=") != std::string::npos) {
            current_drive = line.substr(8);
        } else if (line.find("FreeSpace=") != std::string::npos && !current_drive.empty()) {
            double free_space = std::stoll(line.substr(10)) / (1024.0 * 1024 * 1024);
            disks[current_drive].first = free_space;
        } else if (line.find("Size=") != std::string::npos && !current_drive.empty()) {
            double total_size = std::stoll(line.substr(5)) / (1024.0 * 1024 * 1024);
            disks[current_drive].second = total_size;
        }
    }
    
    for (const auto& disk : disks) {
        result += "    |- " + disk.first + " (" + 
                 std::to_string(static_cast<int>(disk.second.second)) + "GB total, " +
                 std::to_string(static_cast<int>(disk.second.first)) + "GB free)\n";
    }
    
    return result;
}

std::string get_terminal_info() {
    return "Windows Terminal";
}

std::string get_resolution() {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    return std::to_string(desktop.right) + "x" + std::to_string(desktop.bottom);
}

std::string get_windows_version() {
    return exec("ver");
}

int main() {
    // Enable ANSI escape sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    std::cout << "\n";
    
    // System Information
    std::cout << GREEN << "╭─" << RESET << " SYSTEM INFO\n";
    std::cout << GREEN << "├" << RESET << " User: " << get_username() << "\n";
    std::cout << GREEN << "├" << RESET << " Hostname: " << get_hostname() << "\n";
    std::cout << GREEN << "├" << RESET << " OS: " << get_os_info() << "\n";
    std::cout << GREEN << "├" << RESET << " Kernel: " << get_kernel() << "\n";
    std::cout << GREEN << "╰" << RESET << " Uptime: " << get_uptime() << "\n";
    
    std::cout << "\n";
    
    // Hardware Information
    std::cout << GREEN << "╭─" << RESET << " HARDWARE INFO\n";
    std::cout << GREEN << "├" << RESET << " CPU: " << get_cpu_info() << "\n";
    std::cout << GREEN << "├" << RESET << " GPU:\n" << get_gpu_info();
    std::cout << GREEN << "├" << RESET << " RAM: " << get_memory_info() << "\n";
    std::cout << GREEN << "╰" << RESET << " Resolution: " << get_resolution() << "\n";
    
    std::cout << "\n";
    
    // Disk Information
    std::cout << GREEN << "╭─" << RESET << " DISK INFO\n";
    std::cout << get_disk_info();
    std::cout << GREEN << "╰" << RESET << "\n";
    
    std::cout << "\n";
    
    // Terminal Information
    std::cout << GREEN << "╭─" << RESET << " TERMINAL INFO\n";
    std::cout << GREEN << "╰" << RESET << " Terminal: " << get_terminal_info() << "\n";
    
    std::cout << "\n";
    
    return 0;
}