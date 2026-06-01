#pragma once

#include <string>
#include <functional>

std::string trim(const std::string& s);
std::string exec(const char* cmd);
bool read_proc_comm(pid_t pid, std::string& name);
pid_t read_proc_ppid(pid_t pid);
bool is_shell(const std::string& name);
std::string map_terminal_name(const std::string& proc);

struct PidEntry {
    pid_t pid;
    std::string name;
};

void walk_ppid_tree(const std::function<bool(const PidEntry&)>& callback, int max_depth = 20);
