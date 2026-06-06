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

static int east_asian_width(uint32_t cp) {
    if (cp < 0x1100) return 1;
    if (cp < 0x1160) return 2;
    if (cp >= 0x231A && cp <= 0x231B) return 2;
    if (cp >= 0x2329 && cp <= 0x232A) return 2;
    if (cp >= 0x23E9 && cp <= 0x23F3) return 2;
    if (cp >= 0x23F8 && cp <= 0x23FA) return 2;
    if (cp >= 0x25FD && cp <= 0x25FE) return 2;
    if (cp >= 0x2614 && cp <= 0x2615) return 2;
    if (cp >= 0x2648 && cp <= 0x2653) return 2;
    if (cp >= 0x267F && cp <= 0x267F) return 2;
    if (cp >= 0x2693 && cp <= 0x2693) return 2;
    if (cp >= 0x26A1 && cp <= 0x26A1) return 2;
    if (cp >= 0x26AA && cp <= 0x26AB) return 2;
    if (cp >= 0x26BD && cp <= 0x26BF) return 2;
    if (cp >= 0x26C4 && cp <= 0x26CD) return 2;
    if (cp >= 0x26D3 && cp <= 0x26E1) return 2;
    if (cp >= 0x26E8 && cp <= 0x26F2) return 2;
    if (cp >= 0x26F4 && cp <= 0x26F5) return 2;
    if (cp >= 0x26FA && cp <= 0x26FD) return 2;
    if (cp >= 0x2702 && cp <= 0x2705) return 2;
    if (cp >= 0x2708 && cp <= 0x2712) return 2;
    if (cp >= 0x2714 && cp <= 0x2714) return 2;
    if (cp >= 0x2716 && cp <= 0x2716) return 2;
    if (cp >= 0x271D && cp <= 0x271D) return 2;
    if (cp >= 0x2721 && cp <= 0x2721) return 2;
    if (cp >= 0x2728 && cp <= 0x2728) return 2;
    if (cp >= 0x2733 && cp <= 0x2734) return 2;
    if (cp >= 0x2744 && cp <= 0x2744) return 2;
    if (cp >= 0x2747 && cp <= 0x2747) return 2;
    if (cp >= 0x274C && cp <= 0x274C) return 2;
    if (cp >= 0x274E && cp <= 0x274E) return 2;
    if (cp >= 0x2753 && cp <= 0x2755) return 2;
    if (cp >= 0x2757 && cp <= 0x2757) return 2;
    if (cp >= 0x2763 && cp <= 0x2767) return 2;
    if (cp >= 0x2795 && cp <= 0x2797) return 2;
    if (cp >= 0x27A1 && cp <= 0x27A1) return 2;
    if (cp >= 0x27B0 && cp <= 0x27B0) return 2;
    if (cp >= 0x27BF && cp <= 0x27BF) return 2;
    if (cp >= 0x2B1B && cp <= 0x2B1C) return 2;
    if (cp >= 0x2B50 && cp <= 0x2B50) return 2;
    if (cp >= 0x2B55 && cp <= 0x2B55) return 2;
    if (cp >= 0x3000 && cp <= 0x303E) return 2;
    if (cp >= 0x3041 && cp <= 0x3247) return 2;
    if (cp >= 0x3250 && cp <= 0x4DBF) return 2;
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;
    if (cp >= 0xA000 && cp <= 0xA4CF) return 2;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return 2;
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;
    if (cp >= 0xFE10 && cp <= 0xFE19) return 2;
    if (cp >= 0xFE30 && cp <= 0xFE6F) return 2;
    if (cp >= 0xFF01 && cp <= 0xFF60) return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;
    if (cp >= 0x1F000 && cp <= 0x1F9FF) return 2;
    if (cp >= 0x1FA00 && cp <= 0x1FA6F) return 2;
    if (cp >= 0x1FA70 && cp <= 0x1FAFF) return 2;
    if (cp >= 0x20000 && cp <= 0x2FFFD) return 2;
    if (cp >= 0x30000 && cp <= 0x3FFFD) return 2;
    return 1;
}

int unicode_display_width(const std::string& s) {
    int width = 0;
    size_t i = 0;
    while (i < s.size()) {
        uint32_t cp = 0;
        unsigned char c = s[i];
        int bytes = 0;
        if (c < 0x80) { cp = c; bytes = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; bytes = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; bytes = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; bytes = 4; }
        else { i++; continue; }
        if (i + bytes > s.size()) break;
        for (int j = 1; j < bytes; j++) {
            if ((s[i + j] & 0xC0) != 0x80) { i++; goto next_char; }
            cp = (cp << 6) | (s[i + j] & 0x3F);
        }
        i += bytes;
        if (cp >= 0xFE00 && cp <= 0xFE0F) continue;
        if (cp >= 0x1F3FB && cp <= 0x1F3FF) continue;
        if (cp >= 0xE0020 && cp <= 0xE007F) continue;
        if (cp == 0x200D) continue;
        width += east_asian_width(cp);
        next_char:;
    }
    return width;
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
