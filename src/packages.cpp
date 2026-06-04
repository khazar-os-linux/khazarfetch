#include "packages.h"
#include "utils.h"

#include <fstream>
#include <string>
#include <regex>
#include <filesystem>

static std::string find_repo_sections(const std::string& dir, const std::string& ext = ".repo") {
    std::string cmd = "find " + dir + " -type f -name '*" + ext + "' -exec grep -h '\\[.*\\]' {} \\; | tr -d '[]'";
    return trim(exec(cmd.c_str()));
}

static int count_dir_entries(const std::string& path) {
    int count = 0;
    try {
        for (const auto& e : std::filesystem::directory_iterator(path)) {
            if (e.is_directory()) count++;
        }
    } catch (...) {}
    return count;
}

std::string get_repositories() {
    std::string repositories;

    if (std::filesystem::exists("/etc/pacman.conf")) {
        std::ifstream pacman_conf("/etc/pacman.conf");
        std::string line;
        std::regex repo_regex("^\\[(.+)\\]");
        std::smatch match;

        while (std::getline(pacman_conf, line)) {
            if (std::regex_search(line, match, repo_regex)) {
                std::string repo = match[1].str();
                if (repo != "options") {
                    repositories += repo + " ";
                }
            }
        }
        if (!repositories.empty()) return repositories;
    }

    if (std::filesystem::exists("/etc/apt/sources.list")) {
        std::string cmd = "grep -h '^deb ' /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null | awk '{print $2}' | sort -u";
        std::string apt_repos = exec(cmd.c_str());
        if (!apt_repos.empty()) return trim(apt_repos);
    }

    if (std::filesystem::exists("/etc/yum.repos.d/")) {
        std::string repos = find_repo_sections("/etc/yum.repos.d/");
        if (!repos.empty()) return repos;
    }

    if (std::filesystem::exists("/etc/zypp/repos.d/")) {
        std::string repos = find_repo_sections("/etc/zypp/repos.d/");
        if (!repos.empty()) return repos;
    }

    if (std::filesystem::exists("/etc/portage/repos.conf/")) {
        std::string repos = find_repo_sections("/etc/portage/repos.conf/", ".conf");
        if (!repos.empty()) return repos;
    }

    if (std::filesystem::exists("/usr/share/xbps.d/")) {
        std::string cmd = "grep -h '^repository=' /usr/share/xbps.d/*.conf /etc/xbps.d/*.conf 2>/dev/null | cut -d'=' -f2";
        std::string xbps_repos = exec(cmd.c_str());
        if (!xbps_repos.empty()) return trim(xbps_repos);
    }

    if (std::filesystem::exists("/etc/apk/repositories")) {
        std::string cmd = "cat /etc/apk/repositories | grep -v '^#' | awk '{print $1}'";
        std::string alpine_repos = exec(cmd.c_str());
        if (!alpine_repos.empty()) return trim(alpine_repos);
    }

    return "Unknown";
}

std::string get_package_count() {
    if (std::filesystem::exists("/var/lib/pacman/local")) {
        return std::to_string(count_dir_entries("/var/lib/pacman/local"));
    }
    if (std::filesystem::exists("/usr/lib/dpkg")) {
        return trim(exec("dpkg -l 2>/dev/null | grep '^ii' | wc -l"));
    }
    if (std::filesystem::exists("/var/lib/rpm")) {
        return trim(exec("rpm -qa 2>/dev/null | wc -l"));
    }
    if (std::filesystem::exists("/var/lib/xbps")) {
        return trim(exec("xbps-query -l 2>/dev/null | wc -l"));
    }
    if (std::filesystem::exists("/var/lib/apk")) {
        return trim(exec("apk info 2>/dev/null | wc -l"));
    }
    if (std::filesystem::exists("/var/db/pkg")) {
        int count = 0;
        try {
            for (const auto& cat : std::filesystem::directory_iterator("/var/db/pkg")) {
                if (cat.is_directory()) {
                    for (const auto& pkg : std::filesystem::directory_iterator(cat.path())) {
                        if (pkg.is_directory()) count++;
                    }
                }
            }
        } catch (...) {}
        return std::to_string(count);
    }
    return "Unknown";
}
