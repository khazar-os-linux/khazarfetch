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

static std::string format_size(unsigned long kb) {
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

static std::string get_memory_info() {
    MemInfo mi = read_meminfo();
    if (mi.valid) {
        unsigned long used = mi.mem_total - mi.mem_available;
        return "Used: " + format_size(used) + " / Total: " + format_size(mi.mem_total);
    }

    std::string total_cmd = "free -h --si | grep 'Mem:' | awk '{print $2}'";
    std::string used_cmd = "free -h --si | grep 'Mem:' | awk '{print $3}'";
    std::string total_mem = trim(exec(total_cmd.c_str()));
    std::string used_mem = trim(exec(used_cmd.c_str()));
    return "Used: " + used_mem + " / Total: " + total_mem;
}

static std::string get_swap_usage() {
    MemInfo mi = read_meminfo();
    if (mi.valid) {
        unsigned long swap_used = mi.swap_total - mi.swap_free;
        return format_size(swap_used) + " / " + format_size(mi.swap_total);
    }

    std::string cmd = "free -h --si | grep 'Swap:' | awk '{print $3 \"/\" $2}'";
    return trim(exec(cmd.c_str()));
}

void get_memory_and_swap(std::string& mem, std::string& swap) {
    MemInfo mi = read_meminfo();
    if (mi.valid) {
        unsigned long used = mi.mem_total - mi.mem_available;
        unsigned long swap_used = mi.swap_total - mi.swap_free;
        mem = "Used: " + format_size(used) + " / Total: " + format_size(mi.mem_total);
        swap = format_size(swap_used) + " / " + format_size(mi.swap_total);
    } else {
        mem = get_memory_info();
        swap = get_swap_usage();
    }
}

std::string get_gpu_info() {
    std::string all_gpus = trim(exec("lspci | grep -i 'vga\\|3d\\|display' | cut -d ':' -f3"));

    std::string integrated;
    std::string discrete;
    std::istringstream iss(all_gpus);
    std::string line;
    while (std::getline(iss, line)) {
        line = trim(line);
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

    std::string result = "|- GPU\n";
    result += " |- Integrated: " + (integrated.empty() ? "None" : integrated) + "\n";
    result += " |- Discrete: " + (discrete.empty() ? "None" : discrete);

    return result;
}
