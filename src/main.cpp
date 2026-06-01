#include <iostream>
#include <string>
#include <regex>
#include <future>

#include "colors.h"
#include "system_info.h"
#include "hardware.h"
#include "desktop.h"
#include "terminal.h"
#include "disk.h"
#include "packages.h"


static std::string fix_tree_chars(const std::string& input) {
    static const std::regex re1("\\|-");
    static const std::regex re2(" \\|-");
    std::string result = std::regex_replace(input, re1, "├");
    result = std::regex_replace(result, re2, "│ ├");
    return result;
}

int main() {
    try {

        auto f_cpu = std::async(std::launch::async, get_cpu_info);
        auto f_gpu = std::async(std::launch::async, []{ return fix_tree_chars(get_gpu_info()); });
        auto f_mem_swap = std::async(std::launch::async, []{
            std::string m, s;
            get_memory_and_swap(m, s);
            return std::make_pair(m, s);
        });
        auto f_disk = std::async(std::launch::async, []{ return fix_tree_chars(get_disk_usage()); });
        auto f_usb = std::async(std::launch::async, []{ return fix_tree_chars(get_usb_devices()); });
        auto f_repos = std::async(std::launch::async, get_repositories);
        auto f_pkgs = std::async(std::launch::async, get_package_count);
        auto f_term = std::async(std::launch::async, get_terminal);
        auto f_shell = std::async(std::launch::async, get_shell);
        auto f_res = std::async(std::launch::async, get_resolution);
        auto f_de = std::async(std::launch::async, get_desktop_environment);
        auto f_ds = std::async(std::launch::async, get_display_server);
        auto f_comp = std::async(std::launch::async, get_compositor);

        std::cout << COLOR << "╭─" << RESET << " SYSTEM INFO\n";
        std::cout << COLOR << "├" << RESET << " User: " << RESET << get_username() << "\n";
        std::cout << COLOR << "├" << RESET << " Hostname: " << RESET << get_hostname() << "\n";
        std::cout << COLOR << "├" << RESET << " OS: " << RESET << get_os_info() << "\n";
        std::cout << COLOR << "├" << RESET << " Kernel: " << RESET << get_kernel() << "\n";
        std::cout << COLOR << "╰" << RESET << " Uptime: " << RESET << get_uptime() << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " HARDWARE INFO\n";
        std::cout << COLOR << "├" << RESET << " CPU: " << RESET << f_cpu.get() << "\n";
        std::cout << COLOR << f_gpu.get() << RESET << "\n";
        auto ms = f_mem_swap.get();
        std::cout << COLOR << "╰" << RESET << " RAM: " << RESET << ms.first << "\n";
        std::cout << COLOR << "╰" << RESET << " Swap: " << RESET << ms.second << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " DESKTOP INFO\n";
        std::cout << COLOR << "├" << RESET << " DE: " << RESET << f_de.get() << "\n";
        std::cout << COLOR << "├" << RESET << " Resolution: " << RESET << f_res.get() << "\n";
        std::cout << COLOR << "├" << RESET << " Display Server: " << RESET << f_ds.get() << "\n";
        std::cout << COLOR << "╰" << RESET << " Compositor: " << RESET << f_comp.get() << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " TERMINAL INFO\n";
        std::cout << COLOR << "├" << RESET << " Terminal: " << RESET << f_term.get() << "\n";
        std::cout << COLOR << "╰" << RESET << " Shell: " << RESET << f_shell.get() << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " DISK INFO\n";
        std::cout << COLOR << f_disk.get() << RESET << "\n";
        std::cout << COLOR << "╰" << RESET << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " USB DEVICES\n";
        std::cout << COLOR << f_usb.get() << RESET << "\n";
        std::cout << COLOR << "╰" << RESET << "\n";

        std::cout << "\n";

        std::cout << COLOR << "╭─" << RESET << " PACKAGE INFO\n";
        std::cout << COLOR << "├" << RESET << " Packages: " << RESET << f_pkgs.get() << "\n";
        std::cout << COLOR << "╰" << RESET << " Repos: " << RESET << f_repos.get() << "\n";

        std::cout << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }

    return 0;
}
