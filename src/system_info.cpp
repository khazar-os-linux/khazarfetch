#include "system_info.h"
#include "utils.h"

#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>

static std::string dequote(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::string get_username() {
    const char* user = getenv("USER");
    if (user && user[0]) return user;
    std::string result = exec("whoami");
    return trim(result);
}

std::string get_hostname() {
    std::ifstream f("/proc/sys/kernel/hostname");
    if (f.is_open()) {
        std::string hostname;
        std::getline(f, hostname);
        return trim(hostname);
    }
    std::string result = exec("hostname");
    return trim(result);
}

std::string get_os_info() {
    std::ifstream os_release("/etc/os-release");
    std::string line;
    std::string os_name;
    std::string os_version;

    while (std::getline(os_release, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            return dequote(line.substr(12));
        }
        if (line.find("NAME=") == 0) {
            os_name = dequote(line.substr(5));
        }
        if (line.find("VERSION=") == 0) {
            os_version = dequote(line.substr(8));
        }
    }

    if (!os_name.empty()) {
        return os_version.empty() ? os_name : os_name + " " + os_version;
    }

    return "Unknown";
}

std::string get_kernel() {
    std::ifstream f("/proc/sys/kernel/osrelease");
    if (f.is_open()) {
        std::string kernel;
        std::getline(f, kernel);
        return trim(kernel);
    }
    std::string result = exec("uname -r");
    return trim(result);
}

std::string get_uptime() {
    std::ifstream uptime_file("/proc/uptime");
    if (!uptime_file.is_open()) return "Unknown";

    double uptime_seconds = 0;
    if (!(uptime_file >> uptime_seconds)) return "Unknown";

    int days = static_cast<int>(uptime_seconds / 86400);
    int hours = static_cast<int>((uptime_seconds - days * 86400) / 3600);
    int minutes = static_cast<int>((uptime_seconds - days * 86400 - hours * 3600) / 60);

    std::string result;
    if (days > 0) result += std::to_string(days) + " day ";
    result += std::to_string(hours) + " hour " + std::to_string(minutes) + " minute";
    return result;
}
