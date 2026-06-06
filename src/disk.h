#pragma once

#include <string>
#include <vector>

struct DiskInfo {
    std::string name;
    std::string size;
    std::string mount;
    int use_percent;
};

std::vector<DiskInfo> get_disk_info();
