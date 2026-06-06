#include "disk.h"
#include "utils.h"

#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

std::vector<DiskInfo> get_disk_info() {
    std::string raw = exec("df -h --output=source,size,avail,pcent,target 2>/dev/null | tail -n +2");
    if (raw.empty()) return {};

    std::vector<DiskInfo> disks;
    std::istringstream iss(raw);
    std::string line;

    while (std::getline(iss, line)) {
        std::istringstream ls(line);
        std::string source, size, avail, pcent, target;
        if (!(ls >> source >> size >> avail >> pcent >> target)) continue;

        if (source.find("/dev/") != 0) continue;
        if (target == "/snap") continue;
        if (target.find("/snap/") == 0) continue;
        if (target.find("/boot") == 0) continue;
        if (target.find("/efi") == 0) continue;

        bool dup = false;
        for (const auto& d : disks) {
            if (d.name == source.substr(5)) { dup = true; break; }
        }
        if (dup) continue;

        DiskInfo d;
        d.name = source.substr(5);
        d.size = size;
        d.mount = target;

        pcent.erase(std::remove_if(pcent.begin(), pcent.end(), [](char c){ return c == '%'; }), pcent.end());
        try { d.use_percent = std::stoi(pcent); } catch (...) { d.use_percent = 0; }

        disks.push_back(d);
    }

    return disks;
}
