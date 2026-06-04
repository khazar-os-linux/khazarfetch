#include "terminal.h"
#include "utils.h"

#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>

static std::string get_shell_version(const std::string& shell_name) {
    if (shell_name == "bash") {
        const char* v = getenv("BASH_VERSION");
        if (v && v[0]) return v;
    } else if (shell_name == "zsh") {
        const char* v = getenv("ZSH_VERSION");
        if (v && v[0]) return v;
    } else if (shell_name == "fish") {
        std::string ver = trim(exec("fish --version 2>/dev/null | head -1"));
        if (!ver.empty()) return ver;
    }

    std::string ver = trim(exec((shell_name + " --version 2>/dev/null | head -1").c_str()));
    if (!ver.empty()) return ver;

    return "";
}

static std::string get_terminal_from_env() {
    const char* v;

    v = getenv("SSH_CONNECTION");
    if (v && v[0]) {
        v = getenv("SSH_TTY");
        if (v && v[0]) return v;
    }

    v = getenv("KITTY_PID");
    if (v && v[0]) return "Kitty";
    v = getenv("KITTY_INSTALLATION_DIR");
    if (v && v[0]) return "Kitty";

    v = getenv("WT_SESSION");
    if (v && v[0]) return "Windows Terminal";
    v = getenv("WT_PROFILE_ID");
    if (v && v[0]) return "Windows Terminal";

    v = getenv("ALACRITTY_SOCKET");
    if (v && v[0]) return "Alacritty";
    v = getenv("ALACRITTY_LOG");
    if (v && v[0]) return "Alacritty";
    v = getenv("ALACRITTY_WINDOW_ID");
    if (v && v[0]) return "Alacritty";

    v = getenv("KONSOLE_VERSION");
    if (v && v[0]) return "Konsole";

    v = getenv("GNOME_TERMINAL_SCREEN");
    if (v && v[0]) return "GNOME Terminal";
    v = getenv("GNOME_TERMINAL_SERVICE");
    if (v && v[0]) return "GNOME Terminal";

    v = getenv("TERM_PROGRAM");
    if (v && v[0]) {
        std::string tp = v;
        if (tp == "iTerm.app") return "iTerm2";
        if (tp == "Terminal.app") return "Apple Terminal";
        if (tp == "Hyper") return "Hyper";
        if (tp == "WarpTerminal") return "Warp";
        if (tp.find(".app") != std::string::npos)
            return tp.substr(0, tp.find(".app"));
        return tp;
    }

    v = getenv("LC_TERMINAL");
    if (v && v[0]) return v;

    v = getenv("TERMUX_VERSION");
    if (v && v[0]) return "Termux";

    v = getenv("ConEmuPID");
    if (v && v[0]) return "ConEmu";

    v = getenv("TERMINAL_VERSION_STRING");
    if (v && v[0]) return "Terminal";

    return "";
}

std::string get_terminal() {
    std::string env_result = get_terminal_from_env();
    if (!env_result.empty()) return env_result;

    std::string result;
    walk_ppid_tree([&](const PidEntry& e) {
        if (!is_shell(e.name)) {
            result = map_terminal_name(e.name);
            return false;
        }
        return true;
    });

    if (!result.empty()) return result;

    const char* term = getenv("TERM");
    if (term && term[0]) {
        std::string t = term;
        if (t == "xterm-kitty") return "Kitty";
        if (t == "rxvt-unicode-256color") return "URxvt";
        if (t == "tw52" || t == "tw100") return "TosWin2";
        if (t == "xterm-256color") return "xterm";
        return t;
    }

    return "Unknown Terminal";
}

static std::string detect_running_shell() {
    std::string result;
    walk_ppid_tree([&](const PidEntry& e) {
        if (is_shell(e.name)) {
            std::string mapped;
            if (e.name == "bash" || e.name == "zsh" || e.name == "fish" ||
                e.name == "sh" || e.name == "dash" || e.name == "ksh" ||
                e.name == "mksh" || e.name == "oksh" || e.name == "csh" ||
                e.name == "tcsh" || e.name == "ash" || e.name == "pwsh" ||
                e.name == "nu" || e.name == "xonsh" || e.name == "elvish") {
                mapped = e.name;
            } else {
                mapped = e.name;
            }
            if (!mapped.empty()) {
                std::string ver = get_shell_version(mapped);
                if (!ver.empty()) result = mapped + " " + ver;
                else result = mapped;
                return false;
            }
        }
        return true;
    });

    return result;
}

std::string get_shell() {
    std::string running = detect_running_shell();
    if (!running.empty()) return running;

    const char* shell = getenv("SHELL");
    if (shell && shell[0]) {
        std::string s = shell;
        size_t slash = s.rfind('/');
        if (slash != std::string::npos) s = s.substr(slash + 1);
        std::string ver = get_shell_version(s);
        if (!ver.empty()) return s + " " + ver;
        return s;
    }

    return "Unknown";
}
