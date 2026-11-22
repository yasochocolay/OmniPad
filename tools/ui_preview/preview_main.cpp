// OmniPad Companion - desktop UI preview.
// Renders each screen of the real UI code (app/src/ui.cpp) to PNG files, without a PS4.
// Usage: preview <output_dir>
#include "ui.h"
#include "config_manager.h"
#include "hid_profile.h"
#include "companion_features.h"
#include "i18n.h"
#include <stdio.h>
#include <string.h>
#include <string>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

static void save_png(Scene2D& scene, const std::string& path) {
    const uint32_t* fb = scene.GetBackBuffer();
    int w = scene.GetWidth(), h = scene.GetHeight();
    std::string rgb((size_t)w * h * 3, '\0');
    for (int i = 0; i < w * h; i++) {
        rgb[i * 3 + 0] = (char)((fb[i] >> 16) & 0xFF);
        rgb[i * 3 + 1] = (char)((fb[i] >> 8) & 0xFF);
        rgb[i * 3 + 2] = (char)(fb[i] & 0xFF);
    }
    stbi_write_png(path.c_str(), w, h, 3, rgb.data(), w * 3);
    printf("wrote %s\n", path.c_str());
}

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : ".";
    Scene2D scene(1920, 1080, 4);
    if (!scene.Init(0xC000000, 2)) { fprintf(stderr, "scene init failed\n"); return 1; }
    UI ui;
    if (!ui.init(&scene)) { fprintf(stderr, "ui init failed\n"); return 1; }

    AppConfig cfg;
    OrbisPadData pad; memset(&pad, 0, sizeof(pad));
    pad.leftStick.x = pad.leftStick.y = pad.rightStick.x = pad.rightStick.y = 128;
    TesterState tester;
    tester.slotConnected[0] = true;

    HidStatus ds; ds.connected = true; ds.type = "dualsense_bt"; ds.vid = 0x054C; ds.pid = 0x0CE6;
    ds.name = "DualSense Wireless"; ds.device_count = 1;
    DualSenseDeviceInfo d; d.slot = 0; d.player_slot = 1; d.user_name = "Player 1"; d.battery_pct = 75;
    ds.devices.push_back(d);

    HidStatus hid; hid.connected = true; hid.type = "generic_hid"; hid.vid = 0x2DC8; hid.pid = 0x3001;
    hid.name = "8BitDo SF30"; hid.nbuttons = 16;
    HidProfile profile; profile.reset_defaults(16);

    // Sample data for the OmniPad 1.0 screens (files live under $PREVIEW_DATA_DIR/data/omnipad)
    BluetoothInfo bt;
    Bluetooth::refresh(bt);
    HidStatus btStatus;
    HidProfile::read_status(btStatus);
    BtRemapProfile remap;
    BtRemap::load(remap, BT_REMAP_SWITCH_PRO);
    LogView log;
    DaemonLog::load(log);

    struct Shot { const char* name; int tab; bool hidConn; bool dsConn; const char* toast; int card; };
    const Shot shots[] = {
        { "01_settings",    TAB_SETTINGS,  false, false, NULL, 5 },
        { "02_bluetooth",   TAB_BLUETOOTH, false, false, NULL, 0 },
        { "03_tester",      TAB_TESTER,    false, false, NULL, 0 },
        { "04_dualsense",   TAB_DUALSENSE, false, true,  NULL, 0 },
        { "05_remap_hid",   TAB_REMAP_HID, true,  false, NULL, 0 },
        { "06_bt_remap",    TAB_BT_REMAP,  false, false, NULL, 0 },
        { "07_log",         TAB_LOG,       false, false, "Log copied to /mnt/usb0/omnipad.log", 0 },
        { "08_info",        TAB_INFO,      false, false, NULL, 0 },
    };
    const UiLanguage langs[2] = { LANG_ENGLISH, LANG_FRENCH };
    const char* prefixes[2] = { "en_", "fr_" };
    for (int l = 0; l < 2; l++) {
        i18n_set_language(langs[l]);
        cfg.language_index = langs[l] == LANG_FRENCH ? 2 : 1;
        for (const Shot& s : shots) {
            UIExtra extra;
            extra.bt = &bt;
            extra.btStatus = &btStatus;
            extra.remap = &remap;
            extra.remapRow = 1;
            extra.log = &log;
            const HidStatus* status = s.hidConn ? &hid : (s.dsConn ? &ds : &btStatus);
            ui.render(&scene, cfg, s.tab, s.card, s.toast ? 90 : 0, pad, tester, s.toast,
                      s.hidConn, status, s.hidConn ? &profile : NULL, 0, false,
                      NULL, s.dsConn, 0, &extra);
            save_png(scene, out + "/" + prefixes[l] + s.name + ".png");
        }
    }
    return 0;
}
