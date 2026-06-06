#pragma once

#include <string>
#include <vector>

struct MonitorInfo {
    std::string name;
    std::string resolution;
    std::string refresh;
};

std::string get_desktop_environment();
std::vector<MonitorInfo> get_monitors();
std::string get_display_server();
std::string get_compositor();
