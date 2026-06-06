#include "desktop.h"
#include "utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <dirent.h>
#include <cstring>
#include <cstdint>

static bool edid_is_valid(const std::vector<uint8_t>& edid) {
	if (edid.size() < 128 || edid.size() % 128 != 0) return false;
	if (edid[0] != 0x00 || edid[1] != 0xFF || edid[2] != 0xFF || edid[3] != 0xFF ||
		edid[4] != 0xFF || edid[5] != 0xFF || edid[6] != 0xFF || edid[7] != 0x00) return false;
	uint8_t checksum = 0;
	for (int i = 0; i < 128; i++) checksum += edid[i];
	return checksum == 0;
}

static std::string edid_get_name(const std::vector<uint8_t>& edid) {
	if (edid.size() < 128) return "";
	for (int d = 0; d < 4; d++) {
		int offset = 54 + d * 18;
		if (offset + 18 > (int)edid.size()) break;
		if (edid[offset] != 0x00 || edid[offset + 1] != 0x00 || edid[offset + 2] != 0xFC) continue;
		if (edid[offset + 3] != 0x00) continue;
		char name[14] = {};
		for (int i = 0; i < 13; i++) {
			char c = (char)edid[offset + 5 + i];
			if (c == '\n') c = ' ';
			name[i] = c;
		}
		std::string result(name);
		size_t end = result.find_last_not_of(" \n\r\t");
		if (end != std::string::npos) result = result.substr(0, end + 1);
		return result;
	}
	return "";
}

static bool edid_get_dtd(const std::vector<uint8_t>& edid, int& h_res, int& v_res, double& refresh) {
	if (edid.size() < 128) return false;
	for (int d = 0; d < 4; d++) {
		int offset = 54 + d * 18;
		if (offset + 18 > (int)edid.size()) break;
		if (edid[offset] == 0x00 && edid[offset + 1] == 0x00 && edid[offset + 2] == 0x00) continue;

		uint16_t pc_raw = (uint16_t)edid[offset + 1] << 8 | edid[offset];
		if (pc_raw == 0) continue;

		int h_active = edid[offset + 2] | ((edid[offset + 4] >> 4) << 8);
		int h_blank  = edid[offset + 3] | ((edid[offset + 4] & 0x0F) << 8);
		int v_active = edid[offset + 5] | ((edid[offset + 7] >> 4) << 8);
		int v_blank  = edid[offset + 6] | ((edid[offset + 7] & 0x0F) << 8);

		double pixel_clock_mhz = pc_raw * 0.01;
		int h_total = h_active + h_blank;
		int v_total = v_active + v_blank;

		h_res = h_active;
		v_res = v_active;

		if (h_total > 0 && v_total > 0 && pixel_clock_mhz > 0) {
			refresh = (pixel_clock_mhz * 1000.0 * 1000.0) / (h_total * v_total);
		} else {
			refresh = 0;
		}
		return true;
	}
	return false;
}

static bool read_sysfs_file(const std::string& path, std::string& out) {
	std::ifstream f(path);
	if (!f.is_open()) return false;
	std::getline(f, out);
	return true;
}

static bool read_sysfs_edid(const std::string& path, std::vector<uint8_t>& edid) {
	std::ifstream f(path, std::ios::binary);
	if (!f.is_open()) return false;
	edid.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	return !edid.empty();
}

static bool drm_connector_is_connected(const std::string& sysfs_path) {
	std::string status;
	if (read_sysfs_file(sysfs_path + "/status", status)) {
		status = trim(status);
		return status == "connected";
	}
	std::string enabled;
	if (read_sysfs_file(sysfs_path + "/enabled", enabled)) {
		enabled = trim(enabled);
		return enabled == "enabled";
	}
	return false;
}

static std::string clean_connector_name(const std::string& sysfs_name) {
	size_t dash = sysfs_name.find('-');
	if (dash == std::string::npos) return sysfs_name;
	return sysfs_name.substr(dash + 1);
}

static std::vector<MonitorInfo> get_xrandr_monitors() {
	std::string raw = exec("xrandr --current 2>/dev/null");
	std::vector<MonitorInfo> monitors;
	std::istringstream iss(raw);
	std::string line;

	while (std::getline(iss, line)) {
		if (line.find('*') == std::string::npos) continue;
		if (line.find('x') == std::string::npos) continue;

		std::istringstream ls(line);
		std::string name;
		ls >> name;

		std::string token;
		bool found_mode = false;
		std::string resolution, refresh;

		while (ls >> token) {
			if (!found_mode && token.find('x') != std::string::npos) {
				resolution = token;
				found_mode = true;
				continue;
			}
			if (found_mode && token.find('*') != std::string::npos) {
				std::string hz_str;
				for (char c : token) {
					if (c == '*') break;
					if ((c >= '0' && c <= '9') || c == '.') hz_str += c;
				}
				if (!hz_str.empty()) {
					double hz = 0;
					try { hz = std::stod(hz_str); } catch (...) {}
					refresh = hz > 0 ? std::to_string((int)(hz + 0.5)) : "?";
				}
				break;
			}
			if (found_mode && token.find('+') != std::string::npos) break;
		}

		if (!resolution.empty()) {
			MonitorInfo m;
			m.name = name;
			m.resolution = resolution;
			m.refresh = refresh.empty() ? "?" : refresh;
			monitors.push_back(m);
		}
	}

	return monitors;
}

static std::vector<MonitorInfo> get_drm_sysfs_monitors() {
	std::vector<MonitorInfo> monitors;
	DIR* dir = opendir("/sys/class/drm");
	if (!dir) return monitors;

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		std::string name(entry->d_name);
		if (name == "." || name == "..") continue;

		std::string sysfs_path = "/sys/class/drm/" + name;
		if (!drm_connector_is_connected(sysfs_path)) continue;

		std::vector<uint8_t> edid;
		bool has_edid = read_sysfs_edid(sysfs_path + "/edid", edid) && edid_is_valid(edid);

		MonitorInfo m;
		m.name = clean_connector_name(name);

		if (has_edid) {
			std::string mon_name = edid_get_name(edid);
			if (!mon_name.empty()) m.name = mon_name;

			int h_res = 0, v_res = 0;
			double refresh = 0;
			if (edid_get_dtd(edid, h_res, v_res, refresh)) {
				m.resolution = std::to_string(h_res) + "x" + std::to_string(v_res);
				m.refresh = refresh > 0 ? std::to_string((int)(refresh + 0.5)) : "?";
			}
		}

		if (m.resolution.empty()) {
			std::string modes;
			if (read_sysfs_file(sysfs_path + "/modes", modes)) {
				std::istringstream iss(modes);
				std::string first_mode;
				if (std::getline(iss, first_mode)) {
					first_mode = trim(first_mode);
					if (!first_mode.empty()) {
						m.resolution = first_mode;
						if (m.refresh.empty()) m.refresh = "?";
					}
				}
			}
		}

		if (!m.resolution.empty()) {
			if (m.refresh.empty()) m.refresh = "?";
			monitors.push_back(m);
		}
	}

	closedir(dir);
	return monitors;
}

static std::vector<MonitorInfo> get_hyprctl_monitors() {
	std::string raw = exec("hyprctl monitors -j 2>/dev/null");
	if (raw.empty()) return {};

	std::vector<MonitorInfo> monitors;
	std::istringstream iss(raw);
	std::string line;

	std::string name, res, refresh;
	while (std::getline(iss, line)) {
		if (line.find("\"name\":") != std::string::npos) {
			size_t q1 = line.find('"', line.find(':'));
			size_t q2 = line.find('"', q1 + 1);
			if (q1 != std::string::npos && q2 != std::string::npos)
				name = line.substr(q1 + 1, q2 - q1 - 1);
		}
		if (line.find("\"res\":") != std::string::npos) {
			size_t after = line.find(':') + 1;
			std::string val = trim(line.substr(after));
			val.erase(std::remove_if(val.begin(), val.end(), [](char c){ return c == '"' || c == ','; }), val.end());
			res = trim(val);
		}
		if (line.find("\"refreshRate\":") != std::string::npos) {
			size_t after = line.find(':') + 1;
			std::string val = trim(line.substr(after));
			val.erase(std::remove_if(val.begin(), val.end(), [](char c){ return c == ','; }), val.end());
			double hz = 0;
			try { hz = std::stod(trim(val)); } catch (...) {}
			refresh = std::to_string((int)(hz + 0.5));
		}
		if (!name.empty() && !res.empty()) {
			MonitorInfo m;
			m.name = name;
			m.resolution = res;
			m.refresh = refresh.empty() ? "?" : refresh;
			monitors.push_back(m);
			name.clear(); res.clear(); refresh.clear();
		}
	}
	return monitors;
}

static std::vector<MonitorInfo> get_wlr_randr_monitors() {
	std::string raw = exec("wlr-randr 2>/dev/null");
	if (raw.empty()) return {};

	std::vector<MonitorInfo> monitors;
	std::istringstream iss(raw);
	std::string line;
	MonitorInfo current;

	while (std::getline(iss, line)) {
		if (line.find("Enabled") != std::string::npos) {
			if (!current.name.empty() && !current.resolution.empty()) {
				current.refresh = current.refresh.empty() ? "?" : current.refresh;
				monitors.push_back(current);
			}
			current = MonitorInfo();
			continue;
		}
		auto px = line.find("px");
		if (px != std::string::npos) {
			std::istringstream ls(line);
			std::string mode, at, res_str;
			ls >> mode >> at >> res_str;
			if (at == "at") {
				current.resolution = res_str;
				std::string hz_str;
				ls >> hz_str;
				double hz = 0;
				try { hz = std::stod(hz_str); } catch (...) {}
				current.refresh = hz > 0 ? std::to_string((int)(hz + 0.5)) : "?";
			}
		} else if (line.find(':') != std::string::npos && current.name.empty()) {
			current.name = trim(line.substr(0, line.find(':')));
		}
	}
	if (!current.name.empty() && !current.resolution.empty()) {
		current.refresh = current.refresh.empty() ? "?" : current.refresh;
		monitors.push_back(current);
	}
	return monitors;
}

static std::string strip_ansi(const std::string& s) {
	std::string result;
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
			i += 2;
			while (i < s.size() && s[i] != 'm') i++;
			continue;
		}
		result += s[i];
	}
	return result;
}

static std::vector<MonitorInfo> get_kscreen_monitors() {
	std::string raw = exec("kscreen-doctor --outputs 2>/dev/null");
	if (raw.empty()) return {};

	std::vector<MonitorInfo> monitors;
	std::string current_name, current_res, current_refresh;

	std::istringstream iss(raw);
	std::string line;
	while (std::getline(iss, line)) {
		std::string clean = strip_ansi(line);

		auto out_pos = clean.find("Output:");
		if (out_pos != std::string::npos) {
			if (!current_name.empty() && !current_res.empty()) {
				MonitorInfo m;
				m.name = current_name;
				m.resolution = current_res;
				m.refresh = current_refresh.empty() ? "?" : current_refresh;
				monitors.push_back(m);
			}
			current_name.clear(); current_res.clear(); current_refresh.clear();

			std::istringstream tokens(clean.substr(out_pos + 7));
			std::string id, name;
			tokens >> id >> name;
			current_name = name;
			continue;
		}

		auto modes_pos = clean.find("Modes:");
		if (modes_pos != std::string::npos) {
			std::string modes_str = clean.substr(modes_pos + 6);
			size_t star = modes_str.rfind('*');
			if (star != std::string::npos) {
				size_t colon_before = modes_str.rfind(':', star);
				size_t entry_start = 0;
				if (colon_before != std::string::npos && colon_before < star) {
					size_t sp = modes_str.rfind(' ', colon_before);
					entry_start = (sp != std::string::npos) ? sp + 1 : 0;
				}
				std::string entry = modes_str.substr(entry_start, star - entry_start);
				size_t at = entry.find('@');
				if (at != std::string::npos) {
					size_t colon = entry.find(':');
					if (colon != std::string::npos && colon < at) {
						current_res = entry.substr(colon + 1, at - colon - 1);
					} else {
						current_res = entry.substr(0, at);
					}
					std::string hz_str = entry.substr(at + 1);
					double hz = 0;
					try { hz = std::stod(hz_str); } catch (...) {}
					current_refresh = hz > 0 ? std::to_string((int)(hz + 0.5)) : "?";
				}
			}
		}
	}

	if (!current_name.empty() && !current_res.empty()) {
		MonitorInfo m;
		m.name = current_name;
		m.resolution = current_res;
		m.refresh = current_refresh.empty() ? "?" : current_refresh;
		monitors.push_back(m);
	}
	return monitors;
}

std::vector<MonitorInfo> get_monitors() {
	const char* xdg_current = getenv("XDG_CURRENT_DESKTOP");
	bool is_kde = xdg_current && std::string(xdg_current).find("KDE") != std::string::npos;

	if (is_kde) {
		auto ks = get_kscreen_monitors();
		if (!ks.empty()) return ks;
	}

	const char* x_display = getenv("DISPLAY");
	if (x_display) {
		auto xr = get_xrandr_monitors();
		if (!xr.empty()) return xr;
	}

	auto hypr = get_hyprctl_monitors();
	if (!hypr.empty()) return hypr;

	auto wlr = get_wlr_randr_monitors();
	if (!wlr.empty()) return wlr;

	auto drm = get_drm_sysfs_monitors();
	if (!drm.empty()) return drm;

	return {};
}

std::string get_desktop_environment() {
	const char* desktop = getenv("XDG_CURRENT_DESKTOP");
	if (desktop && desktop[0]) return desktop;

	const char* session = getenv("DESKTOP_SESSION");
	if (session && session[0]) return session;

	const char* session_desktop = getenv("XDG_SESSION_DESKTOP");
	if (session_desktop && session_desktop[0]) return session_desktop;

	return "Unknown";
}

std::string get_display_server() {
	const char* wayland_display = getenv("WAYLAND_DISPLAY");
	const char* x_display = getenv("DISPLAY");

	if (wayland_display) return "Wayland";
	if (x_display) return "X11";
	return "Unknown";
}

std::string get_compositor() {
	std::string result = trim(exec("loginctl show-session $(loginctl | awk -v u=$(whoami) '$3==u{print $1; exit}') -p Compositor 2>/dev/null | cut -d= -f2"));
	if (!result.empty() && result != "") return result;

	const char* xdg_current = getenv("XDG_CURRENT_DESKTOP");
	if (xdg_current && xdg_current[0]) {
		std::string de = xdg_current;
		if (de.find("KDE") != std::string::npos) return "kwin_wayland";
		if (de.find("GNOME") != std::string::npos) return "mutter";
		if (de.find("SWAY") != std::string::npos || de.find("sway") != std::string::npos) return "sway";
		if (de.find("Hyprland") != std::string::npos) return "Hyprland";
		if (de.find("wlroots") != std::string::npos) return "wlroots";
	}

	std::string hypr = trim(exec("hyprctl version 2>/dev/null | head -1"));
	if (!hypr.empty()) return "Hyprland";

	std::string sway_pid = trim(exec("pidof sway 2>/dev/null"));
	if (!sway_pid.empty()) return "sway";

	std::string kwin_pid = trim(exec("pidof kwin_wayland 2>/dev/null"));
	if (!kwin_pid.empty()) return "kwin_wayland";

	std::string mutter_pid = trim(exec("pidof mutter 2>/dev/null"));
	if (!mutter_pid.empty()) return "mutter";

	return "Unknown";
}
