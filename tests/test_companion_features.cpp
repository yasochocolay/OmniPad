// Host tests for companion_features.cpp (file formats shared with the daemon).
#include "companion_features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
static int fails = 0;
#define CHECK(c, m) do { if (c) printf("ok   %s\n", m); else { printf("FAIL %s (line %d)\n", m, __LINE__); fails++; } } while (0)
static std::string slurp(const std::string& p) { FILE* f = fopen(p.c_str(), "rb"); if (!f) return ""; std::string s; char b[4096]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) s.append(b, n); fclose(f); return s; }
int main() {
    std::string root = getenv("PREVIEW_DATA_DIR");
    system(("rm -rf '" + root + "' && mkdir -p '" + root + "/data/omnipad' '" + root + "/usb0'").c_str());

    // Bluetooth: flag file round trip + firmware gate + restart detection
    BluetoothInfo bt;
    setenv("PREVIEW_FW", "1250", 1); Bluetooth::refresh(bt);
    CHECK(bt.fw_bcd == 0x1250 && bt.fw_text == "12.50" && bt.fw_switch_ok && bt.fw_bt_ok, "FW 12.50 detected and allowed");
    CHECK(!bt.switch_requested && bt.daemon_mode.empty(), "no flag, no daemon mode file");
    CHECK(Bluetooth::set_switch_mode(true), "flag file created");
    Bluetooth::refresh(bt);
    CHECK(bt.switch_requested, "flag detected");
    FILE* f = fopen((root + "/data/omnipad/omnipad.btmode").c_str(), "w"); fputs("mode = dualsense\n", f); fclose(f);
    Bluetooth::refresh(bt);
    CHECK(bt.daemon_mode == "dualsense" && bt.restart_needed(), "restart needed when daemon still runs DualSense");
    f = fopen((root + "/data/omnipad/omnipad.btmode").c_str(), "w"); fputs("mode = switch_pro\n", f); fclose(f);
    Bluetooth::refresh(bt);
    CHECK(!bt.restart_needed(), "no restart needed once daemon runs Switch Pro");
    CHECK(Bluetooth::set_switch_mode(false), "flag file deleted");
    Bluetooth::refresh(bt);
    CHECK(!bt.switch_requested && bt.restart_needed(), "turning off asks for restart");
    setenv("PREVIEW_FW", "1200", 1); Bluetooth::refresh(bt);
    CHECK(!bt.fw_switch_ok && bt.fw_bt_ok, "FW 12.00: DualSense only");
    setenv("PREVIEW_FW", "0505", 1); Bluetooth::refresh(bt);
    CHECK(!bt.fw_bt_ok, "FW 5.05: no Bluetooth");

    // Remap: save -> file content readable by the daemon parser -> load back
    BtRemapProfile p; BtRemap::load(p, BT_REMAP_SWITCH_PRO);
    CHECK(BtRemap::rule_count(p) == 0, "no profile yet");
    p.choice[0] = 2;                        // cross -> circle
    p.choice[1] = 1;                        // circle -> cross
    p.choice[13] = BT_REMAP_CHOICES - 1;    // touchpad -> none
    CHECK(BtRemap::save(p), "profile saved");
    std::string ini = slurp(root + "/data/omnipad/controllers/057E_2009.ini");
    CHECK(ini.find("cross = circle\n") != std::string::npos && ini.find("circle = cross\n") != std::string::npos &&
          ini.find("touchpad = none\n") != std::string::npos, "daemon format: src = dst");
    CHECK(ini.find("[Remap]") != std::string::npos && ini.find("square") == std::string::npos, "only changed rows written");
    BtRemapProfile q; BtRemap::load(q, BT_REMAP_SWITCH_PRO);
    CHECK(q.choice[0] == 2 && q.choice[1] == 1 && q.choice[13] == BT_REMAP_CHOICES - 1 && BtRemap::rule_count(q) == 3, "loaded back identically");
    BtRemapProfile d; BtRemap::load(d, BT_REMAP_DUALSENSE);
    CHECK(BtRemap::rule_count(d) == 0 && BtRemap::profile_path(1).find("054C_0CE6.ini") != std::string::npos, "DualSense profile is separate");
    f = fopen((root + "/data/omnipad/controllers/054C_0CE6.ini").c_str(), "w"); fputs("# hand written\nremap_CREATE = PS\nl1 = disabled\nbogus = cross\n", f); fclose(f);
    BtRemap::load(d, BT_REMAP_DUALSENSE);
    CHECK(d.choice[10] == 13 && d.choice[4] == BT_REMAP_CHOICES - 1 && BtRemap::rule_count(d) == 2, "accepts remap_ prefix, create alias, case, disabled; ignores unknown");

    // Log: tail, partial-line drop, scroll to end, USB copy
    LogView v; DaemonLog::load(v);
    CHECK(!v.found && v.lines.empty(), "missing log handled");
    f = fopen((root + "/data/omnipad/omnipad.log").c_str(), "w");
    for (int i = 0; i < 5000; i++) fprintf(f, "[INFO] line %d\r\n", i);
    fclose(f);
    DaemonLog::load(v);
    CHECK(v.found && v.lines.size() <= 2000 && v.lines.back() == "[INFO] line 4999", "tail of big log, CRLF stripped");
    CHECK(v.lines.front().rfind("[INFO] line ", 0) == 0, "first kept line is complete");
    CHECK(v.scroll == (int)v.lines.size() - DaemonLog::VISIBLE_LINES, "scrolled to the end");
    std::string dest;
    CHECK(DaemonLog::copy_to_usb(dest) && dest == "/mnt/usb0/omnipad.log", "copied to first USB drive");
    CHECK(slurp(root + "/usb0/omnipad.log") == slurp(root + "/data/omnipad/omnipad.log"), "USB copy is identical");
    system(("rm -rf '" + root + "/usb0'").c_str());
    CHECK(!DaemonLog::copy_to_usb(dest), "no USB drive -> failure reported");

    printf("\n%s (%d failures)\n", fails ? "TESTS FAILED" : "ALL TESTS PASSED", fails);
    return fails != 0;
}
