#include "companion_features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <orbis/libkernel.h>

// ---------------------------------------------------------------- small file helpers
static bool file_exists(const char* path) {
    int fd = sceKernelOpen(path, O_RDONLY, 0);
    if (fd < 0) return false;
    sceKernelClose(fd);
    return true;
}

// Reads at most maxBytes from the END of the file (whole file when it is smaller).
static bool read_file_tail(const char* path, size_t maxBytes, std::string& out, bool& truncated) {
    out.clear();
    truncated = false;
    int fd = sceKernelOpen(path, O_RDONLY, 0);
    if (fd < 0) return false;
    off_t size = sceKernelLseek(fd, 0, SEEK_END);
    if (size < 0) size = 0;
    off_t start = 0;
    if ((size_t)size > maxBytes) {
        start = size - (off_t)maxBytes;
        truncated = true;
    }
    sceKernelLseek(fd, start, SEEK_SET);
    out.resize((size_t)(size - start));
    size_t got = 0;
    while (got < out.size()) {
        ssize_t n = sceKernelRead(fd, &out[got], out.size() - got);
        if (n <= 0) break;
        got += (size_t)n;
    }
    out.resize(got);
    sceKernelClose(fd);
    return true;
}

static bool write_file(const char* path, const std::string& data) {
    int fd = sceKernelOpen(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return false;
    size_t done = 0;
    while (done < data.size()) {
        ssize_t n = sceKernelWrite(fd, data.data() + done, data.size() - done);
        if (n <= 0) break;
        done += (size_t)n;
    }
    sceKernelClose(fd);
    return done == data.size();
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string lower(std::string s) {
    for (size_t i = 0; i < s.size(); i++) if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] + 32);
    return s;
}

// ---------------------------------------------------------------- Bluetooth
bool BluetoothInfo::restart_needed() const {
    if (daemon_mode.empty()) return false;   // daemon not reporting yet: nothing to compare
    if (switch_requested && fw_switch_ok) return daemon_mode != "switch_pro";
    return daemon_mode == "switch_pro";
}

void Bluetooth::refresh(BluetoothInfo& out) {
    OrbisKernelSwVersion ver;
    memset(&ver, 0, sizeof(ver));
    ver.Size = sizeof(ver);
    out.fw_bcd = 0;
    out.fw_text = "?";
    if (sceKernelGetSystemSwVersion(&ver) == 0) {
        out.fw_bcd = ver.Version >> 16;
        char buf[16];
        snprintf(buf, sizeof(buf), "%X.%02X", (unsigned)(out.fw_bcd >> 8), (unsigned)(out.fw_bcd & 0xFF));
        out.fw_text = buf;
    }
    const uint32_t fw = out.fw_bcd;
    out.fw_bt_ok = fw >= 0x0900;
    out.fw_switch_ok = (fw == 0x1250 || fw == 0x1302 || fw == 0x1304 || fw == 0x1350 || fw == 0x1352);
    out.switch_requested = file_exists(SWITCH_BT_FLAG_PATH) || file_exists(SWITCH_BT_FLAG_PATH_USER);

    out.daemon_mode.clear();
    std::string data;
    bool truncated = false;
    if (read_file_tail(BT_MODE_FILE_PATH, 1024, data, truncated) ||
        read_file_tail(BT_MODE_FILE_PATH_USER, 1024, data, truncated)) {
        size_t pos = 0;
        while (pos < data.size()) {
            size_t nl = data.find('\n', pos);
            std::string line = data.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
            pos = (nl == std::string::npos) ? data.size() : nl + 1;
            size_t eq = line.find('=');
            if (eq != std::string::npos && trim(line.substr(0, eq)) == "mode") {
                out.daemon_mode = trim(line.substr(eq + 1));
            }
        }
    }
}

bool Bluetooth::set_switch_mode(bool enable) {
    if (enable) {
        static const char* body = "Created by OmniPad Companion. Delete this file to turn the Switch Pro Bluetooth mode off.\n";
        sceKernelMkdir("/data/omnipad", 0777);
        if (write_file(SWITCH_BT_FLAG_PATH, body)) return true;
        sceKernelMkdir("/user/data/omnipad", 0777);
        return write_file(SWITCH_BT_FLAG_PATH_USER, body);
    }
    sceKernelUnlink(SWITCH_BT_FLAG_PATH);
    sceKernelUnlink(SWITCH_BT_FLAG_PATH_USER);
    return !file_exists(SWITCH_BT_FLAG_PATH) && !file_exists(SWITCH_BT_FLAG_PATH_USER);
}

// ---------------------------------------------------------------- Log viewer
void DaemonLog::load(LogView& view) {
    view.lines.clear();
    view.loaded = true;
    std::string data;
    bool truncated = false;
    view.found = read_file_tail(DAEMON_LOG_PATH, 64 * 1024, data, truncated) ||
                 read_file_tail(DAEMON_LOG_PATH_USER, 64 * 1024, data, truncated);
    if (!view.found) {
        view.scroll = 0;
        return;
    }
    size_t pos = 0;
    if (truncated) {                       // drop the partial first line
        size_t nl = data.find('\n');
        pos = (nl == std::string::npos) ? data.size() : nl + 1;
    }
    while (pos < data.size()) {
        size_t nl = data.find('\n', pos);
        std::string line = data.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        for (size_t i = 0; i < line.size(); i++) if (line[i] == '\t') line[i] = ' ';
        view.lines.push_back(line);
        pos = (nl == std::string::npos) ? data.size() : nl + 1;
    }
    const size_t MAX_LINES = 2000;
    if (view.lines.size() > MAX_LINES) view.lines.erase(view.lines.begin(), view.lines.end() - MAX_LINES);
    int maxScroll = (int)view.lines.size() - VISIBLE_LINES;
    view.scroll = maxScroll > 0 ? maxScroll : 0;
}

bool DaemonLog::copy_to_usb(std::string& outPath) {
    std::string data;
    bool truncated = false;
    if (!read_file_tail(DAEMON_LOG_PATH, 8 * 1024 * 1024, data, truncated) &&
        !read_file_tail(DAEMON_LOG_PATH_USER, 8 * 1024 * 1024, data, truncated)) {
        return false;
    }
    for (int n = 0; n < 8; n++) {
        char path[64];
        snprintf(path, sizeof(path), "/mnt/usb%d/omnipad.log", n);
        if (write_file(path, data)) {
            outPath = path;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------- Bluetooth remap
struct RemapRowDef { const char* key; const char* label; const char* switchLabel; };

static const RemapRowDef ROWS[BT_REMAP_ROWS] = {
    { "cross",    "Cross",          "B" },
    { "circle",   "Circle",         "A" },
    { "square",   "Square",         "Y" },
    { "triangle", "Triangle",       "X" },
    { "l1",       "L1",             "L" },
    { "r1",       "R1",             "R" },
    { "l2",       "L2",             "ZL" },
    { "r2",       "R2",             "ZR" },
    { "l3",       "L3",             "L-stick" },
    { "r3",       "R3",             "R-stick" },
    { "share",    "Share",          "-" },
    { "options",  "Options",        "+" },
    { "ps",       "PS",             "Home" },
    { "touchpad", "Touchpad",       "Capture" },
    { "up",       "D-Pad Up",       "" },
    { "down",     "D-Pad Down",     "" },
    { "left",     "D-Pad Left",     "" },
    { "right",    "D-Pad Right",    "" },
};

const char* BtRemap::row_key(int row) { return (row >= 0 && row < BT_REMAP_ROWS) ? ROWS[row].key : ""; }
const char* BtRemap::row_label(int row) { return (row >= 0 && row < BT_REMAP_ROWS) ? ROWS[row].label : ""; }
const char* BtRemap::switch_label(int row) { return (row >= 0 && row < BT_REMAP_ROWS) ? ROWS[row].switchLabel : ""; }

const char* BtRemap::choice_label(int choice) {
    if (choice <= 0) return "Default";
    if (choice >= BT_REMAP_CHOICES - 1) return "Disabled";
    return ROWS[choice - 1].label;
}

const char* BtRemap::target_name(int target) {
    return target == BT_REMAP_DUALSENSE ? "DualSense" : "Switch Pro";
}

static const char* target_file(int target) {
    return target == BT_REMAP_DUALSENSE ? "054C_0CE6.ini" : "057E_2009.ini";
}

std::string BtRemap::profile_path(int target) {
    return std::string("/data/omnipad/controllers/") + target_file(target);
}

static int row_index(const std::string& name) {
    std::string n = lower(name);
    if (n == "create") n = "share";
    for (int i = 0; i < BT_REMAP_ROWS; i++) if (n == ROWS[i].key) return i;
    return -1;
}

void BtRemap::reset(BtRemapProfile& p) {
    for (int i = 0; i < BT_REMAP_ROWS; i++) p.choice[i] = 0;
    p.dirty = true;
}

int BtRemap::rule_count(const BtRemapProfile& p) {
    int n = 0;
    for (int i = 0; i < BT_REMAP_ROWS; i++) if (p.choice[i] != 0) n++;
    return n;
}

void BtRemap::load(BtRemapProfile& p, int target) {
    p.target = target;
    p.dirty = false;
    for (int i = 0; i < BT_REMAP_ROWS; i++) p.choice[i] = 0;

    std::string data;
    bool truncated = false;
    std::string path = profile_path(target);
    std::string userPath = std::string("/user/data/omnipad/controllers/") + target_file(target);
    if (!read_file_tail(path.c_str(), 16 * 1024, data, truncated) &&
        !read_file_tail(userPath.c_str(), 16 * 1024, data, truncated)) {
        return;
    }
    size_t pos = 0;
    while (pos < data.size()) {
        size_t nl = data.find('\n', pos);
        std::string line = trim(data.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
        pos = (nl == std::string::npos) ? data.size() : nl + 1;
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = lower(trim(line.substr(0, eq)));
        std::string val = lower(trim(line.substr(eq + 1)));
        if (key.compare(0, 6, "remap_") == 0) key = key.substr(6);
        int row = row_index(key);
        if (row < 0) continue;
        if (val == "none" || val == "disabled") {
            p.choice[row] = BT_REMAP_CHOICES - 1;
        } else {
            int dst = row_index(val);
            if (dst >= 0 && dst != row) p.choice[row] = dst + 1;
        }
    }
}

bool BtRemap::save(const BtRemapProfile& p) {
    std::string out;
    out += "# OmniPad Bluetooth button remap for ";
    out += target_name(p.target);
    out += "\n# Written by OmniPad Companion. Format: <button pressed> = <button sent to the PS4>\n[Remap]\n";
    for (int i = 0; i < BT_REMAP_ROWS; i++) {
        int c = p.choice[i];
        if (c == 0) continue;
        out += ROWS[i].key;
        out += " = ";
        out += (c >= BT_REMAP_CHOICES - 1) ? "none" : ROWS[c - 1].key;
        out += "\n";
    }
    sceKernelMkdir("/data/omnipad", 0777);
    sceKernelMkdir("/data/omnipad/controllers", 0777);
    if (write_file(profile_path(p.target).c_str(), out)) return true;
    sceKernelMkdir("/user/data/omnipad", 0777);
    sceKernelMkdir("/user/data/omnipad/controllers", 0777);
    std::string userPath = std::string("/user/data/omnipad/controllers/") + target_file(p.target);
    return write_file(userPath.c_str(), out);
}
