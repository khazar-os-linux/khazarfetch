#include "disk.h"
#include "utils.h"

#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

static std::string format_disk_line(const std::string& name, const std::string& size,
                                     const std::string& type, const std::string& mount,
                                     bool is_usb) {
    if (type == "disk") {
        return (is_usb ? "|- 󰊴 " : "|- 󰋊 ") + name + " (" + size + ")";
    }
    if (!mount.empty()) {
        return " 󰉋 " + name + " (" + size + ") at " + mount;
    }
    return "";
}

static bool parse_lsblk_line(const std::string& line,
                              std::string& name, std::string& size,
                              std::string& type, std::string& mount,
                              std::string& tran) {
    std::istringstream parts(line);
    std::string fields[5];
    int count = 0;
    while (count < 5 && parts >> fields[count]) count++;

    if (count < 4) return false;

    name = fields[0];
    size = fields[1];
    type = fields[2];

    if (count == 5) {
        mount = fields[3];
        tran = fields[4];
    } else {
        mount = "";
        tran = fields[3];
    }

    return true;
}

std::string get_disk_usage() {
    std::string raw = exec("lsblk -nro NAME,SIZE,TYPE,MOUNTPOINT,TRAN 2>/dev/null");
    if (raw.empty()) return "Unknown";

    std::string usb_section;
    std::string main_section;
    std::istringstream iss(raw);
    std::string line;

    while (std::getline(iss, line)) {
        std::string name, size, type, mount, tran;
        if (!parse_lsblk_line(line, name, size, type, mount, tran)) continue;

        if (type != "disk" && type != "part") continue;

        bool is_usb = (tran == "usb");
        std::string formatted = format_disk_line(name, size, type, mount, is_usb);
        if (formatted.empty()) continue;

        if (is_usb) {
            if (!usb_section.empty()) usb_section += "\n";
            usb_section += formatted;
        } else {
            if (!main_section.empty()) main_section += "\n";
            main_section += formatted;
        }
    }

    std::string result;
    if (!usb_section.empty()) result += usb_section + "\n";
    if (!main_section.empty()) result += main_section;
    return trim(result);
}

static std::string read_sysfs_usb_string(const std::string& devpath, const std::string& attr) {
    try {
        std::ifstream f(devpath + "/" + attr);
        if (f.is_open()) {
            std::string val;
            std::getline(f, val);
            return trim(val);
        }
    } catch (...) {}
    return "";
}

static std::string get_usb_from_sysfs() {
    std::string result;
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/sys/bus/usb/devices/")) {
            if (!entry.is_symlink() && !entry.is_directory()) continue;
            std::string path = entry.path().string();

            std::string id_vendor = read_sysfs_usb_string(path, "idVendor");
            if (id_vendor.empty()) continue;

            std::string manufacturer = read_sysfs_usb_string(path, "manufacturer");
            std::string product = read_sysfs_usb_string(path, "product");

            std::string desc;
            if (!product.empty()) {
                desc = product;
            } else if (!manufacturer.empty()) {
                desc = manufacturer + " " + id_vendor;
            } else {
                desc = "USB Device " + id_vendor;
            }

            if (!result.empty()) result += "\n";
            result += " |- " + desc;
        }
    } catch (...) {}
    return result;
}

std::string get_usb_devices() {
    std::string cmd = "lsusb | awk '{$1=$2=\"\"; print $0}' | sed 's/^[ \\t]*/ |- /'";
    std::string usb_devices = exec(cmd.c_str());

    if (!usb_devices.empty()) {
        return trim(usb_devices);
    }

    std::string sysfs_result = get_usb_from_sysfs();
    if (!sysfs_result.empty()) return sysfs_result;

    return " |- No USB devices found";
}
