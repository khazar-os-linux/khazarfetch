#pragma once

#include <string>

std::string get_cpu_info();
std::string get_gpu_info();
void get_memory_and_swap(std::string& mem, std::string& swap);
