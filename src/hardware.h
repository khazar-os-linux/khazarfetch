#pragma once

#include <string>

struct MemoryStats {
    unsigned long mem_total_kb;
    unsigned long mem_used_kb;
    unsigned long swap_total_kb;
    unsigned long swap_used_kb;
    bool valid;
};

std::string get_cpu_info();
std::string get_gpu_info();
MemoryStats get_memory_stats();
std::string format_size(unsigned long kb);
