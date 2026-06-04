#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <algorithm>

#include "colors.h"
#include "system_info.h"
#include "hardware.h"
#include "desktop.h"
#include "terminal.h"
#include "disk.h"
#include "packages.h"

#define KHAZARFETCH_VERSION "Beta 0.1.5"

static int label_width = 14;

static std::string row(const std::string& lbl, const std::string& val) {
    return std::string(label_width - (int)lbl.size(), ' ') + lbl + " │ " + val + "\n";
}

static std::string progress_bar(int percent, int width = 20) {
    int filled = (percent * width + 50) / 100;
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;

    std::string bar;
    for (int i = 0; i < filled; i++) bar += "█";
    for (int i = filled; i < width; i++) bar += "░";
    return bar;
}

static std::string pad_left(const std::string& s, int width) {
int len = 0;
for (size_t i = 0; i < s.size(); i++) len++;
return std::string(width - len, ' ') + s;
}

static std::string mem_swap_line(const std::string& lbl, unsigned long used_kb, unsigned long total_kb) {
if (total_kb == 0) return row(lbl, "N/A");

int pct = (int)((used_kb * 100) / total_kb);
if (pct > 100) pct = 100;

std::string val = pad_left(format_size(used_kb), 7) + " / " + format_size(total_kb) + " " + progress_bar(pct);
return row(lbl, val);
}

int main() {
    try {
        auto f_cpu = std::async(std::launch::async, get_cpu_info);
        auto f_gpu = std::async(std::launch::async, get_gpu_info);
        auto f_mem = std::async(std::launch::async, get_memory_stats);
        auto f_mon = std::async(std::launch::async, get_monitors);
        auto f_de = std::async(std::launch::async, get_desktop_environment);
        auto f_ds = std::async(std::launch::async, get_display_server);
        auto f_comp = std::async(std::launch::async, get_compositor);
        auto f_term = std::async(std::launch::async, get_terminal);
        auto f_shell = std::async(std::launch::async, get_shell);
        auto f_disk = std::async(std::launch::async, get_disk_info);
        auto f_pkgs = std::async(std::launch::async, get_package_count);
        auto f_repos = std::async(std::launch::async, get_repositories);

        std::string user = get_username();
        std::string host = get_hostname();
        std::string os = get_os_info();
        std::string kernel = get_kernel();
        std::string uptime = get_uptime();

        auto cpu = f_cpu.get();
        auto gpu = f_gpu.get();
        auto mem = f_mem.get();
        auto monitors = f_mon.get();
        auto de = f_de.get();
        auto ds = f_ds.get();
        auto comp = f_comp.get();
        auto term = f_term.get();
        auto shell = f_shell.get();
        auto disks = f_disk.get();
        auto pkgs = f_pkgs.get();
        auto repos = f_repos.get();

        std::cout << "\n";
        std::cout << " ── " << user << "@" << host << " ──────────────────────\n";
        std::cout << row("OS", os);
        std::cout << row("Kernel", kernel);
        std::cout << row("Uptime", uptime);
        std::cout << "\n";

        std::cout << " ── Hardware ────────────────────\n";
        std::cout << row("CPU", cpu);
        std::cout << row("GPU", gpu);
        std::cout << mem_swap_line("RAM", mem.mem_used_kb, mem.mem_total_kb);
        std::cout << mem_swap_line("Swap", mem.swap_used_kb, mem.swap_total_kb);
        std::cout << "\n";

        std::cout << " ── Desktop ─────────────────────\n";
        std::cout << row("DE", de);
        std::cout << row("Server", ds);
        std::cout << row("Compositor", comp);
        if (monitors.empty()) {
            std::cout << row("Display", "Unknown");
        } else if (monitors.size() == 1) {
            std::cout << row("Display", monitors[0].resolution + " @" + monitors[0].refresh + "Hz");
        } else {
            for (size_t i = 0; i < monitors.size(); i++) {
                std::string lbl = (i == 0) ? "Display" : "";
                std::string val = monitors[i].name + " " + monitors[i].resolution + " @" + monitors[i].refresh + "Hz";
                std::cout << row(lbl, val);
            }
        }
        std::cout << "\n";

        std::cout << " ── Terminal ────────────────────\n";
        std::cout << row("Terminal", term);
        std::cout << row("Shell", shell);
        std::cout << "\n";

        if (!disks.empty()) {
            std::cout << " ── Disk ────────────────────────\n";
            for (const auto& d : disks) {
                std::string val = pad_left(d.size, 7) + " " + progress_bar(d.use_percent) + " " + d.mount;
                std::cout << row(d.name, val);
            }
            std::cout << "\n";
        }

        std::cout << " ── Packages ────────────────────\n";
        std::cout << row("Packages", pkgs);
        std::cout << row("Repos", repos);
        std::cout << "\n";

        std::cout << " KhazarFetch version " KHAZARFETCH_VERSION "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }

    return 0;
}
