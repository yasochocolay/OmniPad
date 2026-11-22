#pragma once
// Companion App features that only touch files (no drawing):
//  - Bluetooth: firmware check, Switch Pro mode switch (flag file), mode reported by the daemon
//  - Daemon log viewer and copy to a USB drive
//  - Bluetooth button remap profiles (/data/omnipad/controllers/VVVV_PPPP.ini, "src = dst" rules)

#include <stdint.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------- Bluetooth
#define SWITCH_BT_FLAG_PATH       "/data/omnipad/switchpro_bt.enable"
#define SWITCH_BT_FLAG_PATH_USER  "/user/data/omnipad/switchpro_bt.enable"
#define BT_MODE_FILE_PATH         "/data/omnipad/omnipad.btmode"
#define BT_MODE_FILE_PATH_USER    "/user/data/omnipad/omnipad.btmode"

struct BluetoothInfo {
    uint32_t fw_bcd;          // e.g. 0x1250, 0 when unknown
    std::string fw_text;      // "12.50"
    bool fw_bt_ok;            // FW >= 9.00: DualSense Bluetooth supported
    bool fw_switch_ok;        // FW in the Switch Pro safe list (12.50, 13.02, 13.04, 13.50, 13.52)
    bool switch_requested;    // flag file present
    std::string daemon_mode;  // as reported by the running daemon: "dualsense", "switch_pro", "off" or "" (unknown)

    BluetoothInfo() : fw_bcd(0), fw_bt_ok(false), fw_switch_ok(false), switch_requested(false) {}
    bool restart_needed() const;   // requested mode differs from the mode the daemon is running
};

namespace Bluetooth {
    void refresh(BluetoothInfo& out);
    bool set_switch_mode(bool enable);   // create / delete the flag file
}

// ---------------------------------------------------------------- Log viewer
#define DAEMON_LOG_PATH       "/data/omnipad/omnipad.log"
#define DAEMON_LOG_PATH_USER  "/user/data/omnipad/omnipad.log"

struct LogView {
    std::vector<std::string> lines;
    int scroll;               // first visible line
    bool loaded;
    bool found;
    LogView() : scroll(0), loaded(false), found(false) {}
};

namespace DaemonLog {
    static const int VISIBLE_LINES = 20;
    void load(LogView& view);                  // reads the tail of the log, scrolls to the end
    bool copy_to_usb(std::string& outPath);    // copies the whole log to /mnt/usbN/omnipad.log
}

// ---------------------------------------------------------------- Bluetooth remap
enum BtRemapTarget { BT_REMAP_SWITCH_PRO = 0, BT_REMAP_DUALSENSE = 1, BT_REMAP_TARGET_COUNT };

static const int BT_REMAP_ROWS = 18;        // source buttons
static const int BT_REMAP_CHOICES = 20;     // 0 = default (no rule), 1..18 = button, 19 = disabled

struct BtRemapProfile {
    int target;                  // BtRemapTarget
    int choice[BT_REMAP_ROWS];   // see BT_REMAP_CHOICES
    bool dirty;
    BtRemapProfile() : target(BT_REMAP_SWITCH_PRO), dirty(false) { for (int i = 0; i < BT_REMAP_ROWS; i++) choice[i] = 0; }
};

namespace BtRemap {
    const char* row_key(int row);              // "cross", "circle", ... (daemon names)
    const char* row_label(int row);            // "Cross", "Circle", ...
    const char* switch_label(int row);         // physical Switch Pro button for that row ("B", "A", ...), or ""
    const char* choice_label(int choice);      // "Default", "Cross", ..., "Disabled"
    const char* target_name(int target);       // "Switch Pro", "DualSense"
    std::string profile_path(int target);
    void load(BtRemapProfile& p, int target);
    bool save(const BtRemapProfile& p);
    void reset(BtRemapProfile& p);
    int  rule_count(const BtRemapProfile& p);
}
