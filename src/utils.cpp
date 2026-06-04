#include "utils.h"

#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_set>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <thread>

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \n\r\t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \n\r\t");
    return s.substr(start, end - start + 1);
}

std::string exec(const char* cmd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return "";

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return "";
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        setpgid(0, 0);
        execl("/bin/sh", "sh", "-c", cmd, nullptr);
        _exit(127);
    }

    close(pipefd[1]);
    setpgid(pid, pid);

    std::string result;
    char buffer[256];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        result.append(buffer, n);
    }
    close(pipefd[0]);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool killed = false;
    while (true) {
        int status;
        pid_t ret = waitpid(pid, &status, WNOHANG);
        if (ret == pid) break;
        if (ret == -1 && errno != EINTR) break;
        if (!killed && std::chrono::steady_clock::now() >= deadline) {
            kill(-pid, SIGKILL);
            killed = true;
        }
        if (killed) {
            waitpid(pid, &status, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (killed) return "";
    return result;
}

bool read_proc_comm(pid_t pid, std::string& name) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/comm");
    if (!f.is_open()) return false;
    std::getline(f, name);
    return true;
}

pid_t read_proc_ppid(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) return 0;
    std::string line;
    std::getline(f, line);
    size_t rp = line.rfind(')');
    if (rp == std::string::npos) return 0;
    std::istringstream iss(line.substr(rp + 2));
    char state;
    pid_t ppid;
    iss >> state >> ppid;
    return ppid;
}

bool is_shell(const std::string& name) {
    static const std::unordered_set<std::string> shells = {
        "sh", "ash", "bash", "zsh", "ksh", "mksh", "oksh",
        "csh", "tcsh", "fish", "dash", "pwsh", "nu",
        "xonsh", "elvish", "git-shell", "oil.ovm",
        "login", "su", "sudo", "strace", "gdb", "lldb",
        "ltrace", "perf", "time", "valgrind", "proot",
        "script", "run-parts", "clifm", "chezmoi",
        "kilo", "fastfetch", "flashfetch", "neofetch"
    };
    if (shells.count(name)) return true;
    if (name.size() > 3 && name.substr(name.size() - 3) == ".sh") return true;
    if (name.find("command-not-") != std::string::npos) return true;
    if (name.find("debug") != std::string::npos) return true;
    return false;
}

void walk_ppid_tree(const std::function<bool(const PidEntry&)>& callback, int max_depth) {
    static const std::vector<std::string> boundary = {
        "systemd", "init", "(init)", "sshd",
        "tmux", "screen", "bwrap"
    };

    pid_t pid = getppid();
    while (pid > 1 && max_depth-- > 0) {
        std::string name;
        if (!read_proc_comm(pid, name)) break;
        name = trim(name);
        if (name.empty()) break;

        bool is_boundary = false;
        for (const auto& b : boundary) {
            if (name == b) { is_boundary = true; break; }
        }
        if (!is_boundary) {
            if (name.find("tmux") == 0 || name.find("screen") == 0 ||
                name.find("flatpak-") == 0 || name.find("Relay(") == 0) {
                is_boundary = true;
            }
        }
        if (is_boundary) break;

        PidEntry entry{pid, name};
        if (!callback(entry)) break;

        pid = read_proc_ppid(pid);
        if (pid <= 1) break;
    }
}

std::string map_terminal_name(const std::string& proc) {
    static const std::map<std::string, std::string> mapping = {
        {"gnome-terminal-", "GNOME Terminal"},
        {"gnome-terminal", "GNOME Terminal"},
        {"kgx", "GNOME Console"},
        {"konsole", "Konsole"},
        {"xfce4-terminal", "XFCE Terminal"},
        {"mate-terminal", "MATE Terminal"},
        {"kitty", "Kitty"},
        {"alacritty", "Alacritty"},
        {"terminator", "Terminator"},
        {"tilix", "Tilix"},
        {"urxvtd", "URxvt"},
        {"urxvt", "URxvt"},
        {"rxvt", "URxvt"},
        {"terminology", "Terminology"},
        {"wezterm-gui", "WezTerm"},
        {"foot", "Foot"},
        {"footclient", "Foot"},
        {"weston-terminal", "Weston Terminal"},
        {"lxterminal", "LXTerminal"},
        {"sakura", "Sakura"},
        {"termite", "Termite"},
        {"st", "st"},
        {"xterm", "xterm"},
        {"eterm", "Eterm"},
        {"roxterm", "ROXTerm"},
        {"qterminal", "QTerminal"},
        {"yakuake", "Yakuake"},
        {"guake", "Guake"},
        {"tilda", "Tilda"},
        {"hyper", "Hyper"},
        {"deepin-terminal", "Deepin Terminal"},
        {"io.elementary.terminal", "Elementary Terminal"},
        {"ptyxis-agent", "Ptyxis"},
        {"ghostty", "Ghostty"},
        {"tabby", "Tabby"},
        {"contour", "Contour"},
        {"blackbox", "Black Box"},
        {"iTerm.app", "iTerm2"},
        {"Apple Terminal", "Apple Terminal"},
        {"WarpTerminal", "Warp"},
    };
    for (const auto& [key, val] : mapping) {
        if (proc == key || proc.find(key) == 0) return val;
    }
    return proc;
}
