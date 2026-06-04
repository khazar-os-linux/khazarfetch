#include "hardware.h"
#include "utils.h"

#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

struct MemInfo {
    unsigned long mem_total = 0;
    unsigned long mem_available = 0;
    unsigned long swap_total = 0;
    unsigned long swap_free = 0;
    bool valid = false;
};

static MemInfo read_meminfo() {
    MemInfo mi;
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) return mi;

    std::string line;
    while (std::getline(f, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu", &mi.mem_total);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lu", &mi.mem_available);
        } else if (line.find("SwapTotal:") == 0) {
            sscanf(line.c_str(), "SwapTotal: %lu", &mi.swap_total);
        } else if (line.find("SwapFree:") == 0) {
            sscanf(line.c_str(), "SwapFree: %lu", &mi.swap_free);
        }
    }
    mi.valid = (mi.mem_total > 0);
    return mi;
}

std::string format_size(unsigned long kb) {
    if (kb >= 1048576) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fG", kb / 1048576.0);
        return buf;
    } else if (kb >= 1024) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fM", kb / 1024.0);
        return buf;
    }
    return std::to_string(kb) + "K";
}

std::string get_cpu_info() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") == 0) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    return trim(line.substr(colon + 1));
                }
            }
        }
    }

    std::string cmd = "lscpu | grep 'Model name' | awk -F: '{print $2}'";
    return trim(exec(cmd.c_str()));
}

MemoryStats get_memory_stats() {
    MemoryStats ms{};
    MemInfo mi = read_meminfo();
    if (mi.valid) {
        ms.mem_total_kb = mi.mem_total;
        ms.mem_used_kb = mi.mem_total - mi.mem_available;
        ms.swap_total_kb = mi.swap_total;
        ms.swap_used_kb = mi.swap_total - mi.swap_free;
        ms.valid = true;
    }
    return ms;
}

static std::string clean_gpu_name(std::string name) {
static const std::vector<std::string> vendor_prefixes = {
"NVIDIA Corporation ",
"Advanced Micro Devices, Inc. ",
"AMD/ATI ",
"Intel Corporation ",
"Intel ",
};
for (const auto& prefix : vendor_prefixes) {
if (name.find(prefix) == 0) {
name = name.substr(prefix.size());
break;
}
}
size_t rev = name.rfind(" (rev ");
if (rev != std::string::npos) name = name.substr(0, rev);
return trim(name);
}

std::string get_gpu_info() {
std::string all_gpus = trim(exec("lspci | grep -i 'vga\\|3d\\|display' | cut -d ':' -f3"));

std::string integrated;
std::string discrete;
std::istringstream iss(all_gpus);
std::string line;
while (std::getline(iss, line)) {
line = clean_gpu_name(trim(line));
if (line.empty()) continue;
if (line.find("NVIDIA") != std::string::npos ||
(line.find("Radeon") != std::string::npos && line.find("Integrated") == std::string::npos)) {
if (!discrete.empty()) discrete += "\n";
discrete += line;
} else {
if (!integrated.empty()) integrated += "\n";
integrated += line;
}
}

std::string result;
if (!integrated.empty()) result += integrated;
if (!discrete.empty()) {
if (!result.empty()) result += " / ";
result += discrete;
}
if (result.empty()) result = "None";
return result;
}
