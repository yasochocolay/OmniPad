// Screens added in OmniPad 1.0: Bluetooth, Bluetooth button remap, daemon log.
// Plus the shared widgets of the 1.0 visual style (value pills, footer hints).
#include "ui.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

// ------------------------------------------------------------------ shared widgets

void UI::draw_value_pill(Scene2D* scene, int centerX, int y, int w, const std::string& value, bool selected) {
    const int h = 44;
    const int x = centerX - w / 2;
    draw_rounded_rect(scene, x, y, w, h, h / 2, selected ? COLOR_SIDEBAR_BLUE : COLOR_PILL_BG);
    draw_rounded_outline(scene, x, y, w, h, h / 2, selected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, selected ? 2 : 1);
    draw_text_centered(scene, value, fontValue, centerX, y + 30, selected ? COLOR_WHITE : Color{ 215, 212, 240 });
    if (selected) {
        draw_text(scene, "<", fontValue, x + 16, y + 30, COLOR_BLUE_BRIGHT);
        draw_text(scene, ">", fontValue, x + w - 28, y + 30, COLOR_BLUE_BRIGHT);
    }
}

void UI::draw_hints(Scene2D* scene, const Hint* hints, int count) {
    const int GAP = 34;
    const int ICON_GAP = 10;
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += (hints[i].icon ? hints[i].icon->width + ICON_GAP : 0) +
                 scene->GetTextWidth(tr(hints[i].label), fontFooter) + (i + 1 < count ? GAP : 0);
    }
    int x = 1860 - total;
    for (int i = 0; i < count; i++) {
        if (hints[i].icon && hints[i].icon->pixels) {
            draw_image(scene, *hints[i].icon, x, 916 - hints[i].icon->height / 2);
            x += hints[i].icon->width + ICON_GAP;
        }
        draw_text(scene, hints[i].label, fontFooter, x, 922, hints[i].color);
        x += scene->GetTextWidth(tr(hints[i].label), fontFooter) + GAP;
    }
}

void UI::draw_footer_title(Scene2D* scene, const ImageRGBA& icon, const char* title, const std::string& subtitle) {
    scene->DrawRectangle(60, FOOTER_Y, 1800, 2, COLOR_DIVIDER);
    if (icon.pixels) draw_image(scene, icon, 60, 890);
    else draw_log_icon(scene, 68, 900, COLOR_BLUE_BRIGHT);
    draw_text(scene, title, fontCardTitle, 120, 912, COLOR_WHITE);
    draw_text(scene, subtitle, fontFooter, 120, 938, COLOR_TEXT_DIM);
}

void UI::draw_log_icon(Scene2D* scene, int x, int y, Color color) {
    // three text lines, the middle one shorter
    draw_rounded_rect(scene, x, y, 28, 5, 2, color);
    draw_rounded_rect(scene, x, y + 11, 20, 5, 2, color);
    draw_rounded_rect(scene, x, y + 22, 28, 5, 2, color);
}

void UI::draw_status_dot(Scene2D* scene, int x, int y, Color color) {
    draw_circle(scene, x, y, 7, color);
}

std::string UI::fit_text(Scene2D* scene, const std::string& text, FT_Face face, int maxWidth) {
    std::string shown = tr(text.c_str());
    if (scene->GetTextWidth(shown.c_str(), face) <= maxWidth) return shown;
    const int ellipsis = scene->GetTextWidth("...", face);
    while (!shown.empty()) {
        // drop one UTF-8 character from the end
        size_t cut = shown.size() - 1;
        while (cut > 0 && (((unsigned char)shown[cut]) & 0xC0) == 0x80) cut--;
        shown.erase(cut);
        if (scene->GetTextWidth(shown.c_str(), face) + ellipsis <= maxWidth) break;
    }
    return shown + "...";
}

// Small right-pointing arrow used in the remap grid.
static void draw_arrow(Scene2D* scene, int x, int cy, Color color) {
    for (int i = 0; i < 8; i++) {
        scene->DrawRectangle(x + i, cy - (7 - i), 1, (7 - i) * 2 + 1, color);
    }
    scene->DrawRectangle(x - 16, cy - 1, 16, 3, color);
}

// ------------------------------------------------------------------ Bluetooth

void UI::render_bluetooth(Scene2D* scene, const UIExtra& extra) {
    BluetoothInfo empty;
    const BluetoothInfo& bt = extra.bt ? *extra.bt : empty;
    const int colW = 830;
    const int panelX = CARD_X + colW + 30;
    const int panelW = 1850 - panelX;
    const bool switchOn = bt.switch_requested;

    // Card 0: Switch Pro mode (the only interactive card)
    {
        const int y = CARD_Y_START;
        const bool sel = (extra.btSelectedCard == 0);
        draw_rounded_rect(scene, CARD_X, y, colW, CARD_H, 12, COLOR_CARD_BG);
        if (sel) {
            draw_rounded_outline(scene, CARD_X - 1, y - 1, colW + 2, CARD_H + 2, 12, COLOR_BLUE_GLOW, 2);
            draw_rounded_outline(scene, CARD_X, y, colW, CARD_H, 12, COLOR_BLUE_BRIGHT, 2);
        } else {
            draw_rounded_outline(scene, CARD_X, y, colW, CARD_H, 12, COLOR_CARD_BORDER, 1);
        }
        draw_image(scene, iconRumble, CARD_X + 20, y + 22);
        draw_text(scene, "Switch Pro Bluetooth Mode", fontCardTitle, CARD_X + 85, y + 38, COLOR_WHITE);
        draw_text(scene, fit_text(scene, "Pair a Switch Pro or a pad in Switch mode.", fontCardDesc, colW - 380),
                  fontCardDesc, CARD_X + 85, y + 68, COLOR_TEXT_DIM);
        draw_value_pill(scene, CARD_X + colW - 150, y + 24, 240, switchOn ? "On" : "Off", sel);
    }

    // Card 1: status
    {
        const int y = CARD_Y_START + 108;
        const int h = 196;
        draw_rounded_rect(scene, CARD_X, y, colW, h, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, y, colW, h, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconInfo, CARD_X + 20, y + 20);
        draw_text(scene, "Status", fontCardTitle, CARD_X + 85, y + 38, COLOR_WHITE);

        int lineY = y + 80;
        const int textX = CARD_X + 110;
        // firmware
        std::string fwLine = std::string(tr("Firmware")) + " " + bt.fw_text + "  -  ";
        Color fwColor = COLOR_OK;
        if (bt.fw_switch_ok) {
            fwLine += tr("compatible with the Switch Pro mode");
        } else if (bt.fw_bt_ok) {
            fwLine += tr("DualSense only (Switch Pro mode needs 12.50 - 13.52)");
            fwColor = COLOR_WARN;
        } else {
            fwLine += tr("Bluetooth controllers need firmware 9.00 or newer");
            fwColor = COLOR_ERROR;
        }
        draw_status_dot(scene, CARD_X + 92, lineY - 7, fwColor);
        draw_text(scene, fit_text(scene, fwLine, fontCardDesc, colW - 130), fontCardDesc, textX, lineY, COLOR_WHITE);
        lineY += 38;

        // mode reported by the daemon
        std::string mode;
        Color modeColor = COLOR_OK;
        if (bt.daemon_mode == "switch_pro") mode = "Switch Pro";
        else if (bt.daemon_mode == "dualsense") mode = "DualSense";
        else if (bt.daemon_mode == "off") { mode = tr("Off"); modeColor = COLOR_TEXT_DIM; }
        else { mode = tr("unknown (OmniPad daemon not running)"); modeColor = COLOR_WARN; }
        draw_status_dot(scene, CARD_X + 92, lineY - 7, modeColor);
        draw_text(scene, std::string(tr("Active Bluetooth mode:")) + " " + mode, fontCardDesc, textX, lineY, COLOR_WHITE);
        lineY += 38;

        // what happens next
        if (bt.switch_requested && !bt.fw_switch_ok) {
            draw_status_dot(scene, CARD_X + 92, lineY - 7, COLOR_ERROR);
            draw_text(scene, "Switch Pro mode is ignored on this firmware.", fontCardDesc, textX, lineY, COLOR_ERROR);
        } else if (bt.restart_needed()) {
            draw_status_dot(scene, CARD_X + 92, lineY - 7, COLOR_WARN);
            draw_text(scene, "Restart the PS4 (and GoldHEN) to apply the new mode.", fontCardDesc, textX, lineY, COLOR_WARN);
        } else {
            draw_status_dot(scene, CARD_X + 92, lineY - 7, COLOR_TEXT_DIM);
            draw_text(scene, switchOn ? "DualSense Bluetooth is off while the Switch Pro mode is on."
                                      : "Up to 4 DualSense controllers can be paired.",
                      fontCardDesc, textX, lineY, COLOR_TEXT_DIM);
        }
    }

    // Card 2: pairing guide
    {
        const int y = CARD_Y_START + 320;
        const int h = 248;
        draw_rounded_rect(scene, CARD_X, y, colW, h, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, y, colW, h, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconGuide, CARD_X + 20, y + 20);
        draw_text(scene, "How to pair a Switch Pro controller", fontCardTitle, CARD_X + 85, y + 38, COLOR_WHITE);
        const char* steps[4] = {
            "1. Turn the Switch Pro mode on, then restart the PS4 and GoldHEN.",
            "2. On the PS4, open Settings > Devices > Bluetooth Devices.",
            "3. Hold the small sync button on top of the controller.",
            "4. Select \"Pro Controller\". Later, press Home to reconnect it."
        };
        for (int i = 0; i < 4; i++) {
            draw_text(scene, fit_text(scene, steps[i], fontCardDesc, colW - 110), fontCardDesc, CARD_X + 85, y + 84 + i * 40,
                      i == 0 ? COLOR_BLUE_BRIGHT : COLOR_WHITE);
        }
    }

    // Right panel: connected Bluetooth controllers
    {
        const int y = CARD_Y_START;
        const int h = 568;
        draw_rounded_rect(scene, panelX, y, panelW, h, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, panelX, y, panelW, h, 12, COLOR_CARD_BORDER, 1);
        draw_text(scene, "Bluetooth Controllers", fontCardTitle, panelX + 24, y + 40, COLOR_WHITE);
        draw_text(scene, "Live status from the OmniPad daemon", fontCardDesc, panelX + 24, y + 68, COLOR_BLUE_BRIGHT);
        scene->DrawRectangle(panelX + 24, y + 88, panelW - 48, 1, COLOR_DIVIDER);

        const HidStatus* st = extra.btStatus;
        const bool isBt = st && st->connected && (st->type == "dualsense_bt" || st->type == "switch_pro_bt");
        int rowY = y + 120;
        if (isBt && !st->devices.empty()) {
            for (size_t i = 0; i < st->devices.size() && i < 4; i++) {
                const DualSenseDeviceInfo& d = st->devices[i];
                draw_rounded_rect(scene, panelX + 24, rowY, panelW - 48, 96, 10, COLOR_PILL_BG);
                draw_status_dot(scene, panelX + 48, rowY + 30, COLOR_OK);
                draw_text(scene, st->type == "switch_pro_bt" ? "Switch Pro" : "DualSense", fontCardTitle, panelX + 66, rowY + 38, COLOR_WHITE);
                char line[96];
                if (st->type == "dualsense_bt") {
                    snprintf(line, sizeof(line), "%s %d  -  %s %d%%%s", tr("Player"), d.player_slot, tr("Battery"),
                             d.battery_pct, d.battery_charging ? " +" : "");
                } else {
                    snprintf(line, sizeof(line), "%s %d", tr("Player"), d.player_slot);
                }
                draw_text(scene, line, fontCardDesc, panelX + 66, rowY + 72, COLOR_TEXT_DIM);
                rowY += 108;
            }
        } else if (isBt) {
            draw_status_dot(scene, panelX + 48, rowY + 2, COLOR_OK);
            draw_text(scene, st->name, fontCardTitle, panelX + 66, rowY + 10, COLOR_WHITE);
        } else {
            draw_text(scene, "No Bluetooth controller connected.", fontCardDesc, panelX + 24, rowY + 4, COLOR_WHITE);
            draw_text(scene, fit_text(scene, "Paired controllers appear here a few seconds", fontCardDesc, panelW - 48),
                      fontCardDesc, panelX + 24, rowY + 36, COLOR_TEXT_DIM);
            draw_text(scene, fit_text(scene, "after you press a button on them.", fontCardDesc, panelW - 48),
                      fontCardDesc, panelX + 24, rowY + 62, COLOR_TEXT_DIM);
        }
        draw_text(scene, fit_text(scene, "Mode file: " SWITCH_BT_FLAG_PATH, fontFooter, panelW - 48), fontFooter,
                  panelX + 24, y + h - 24, Color{ 120, 114, 170 });
    }

    // Footer
    draw_footer_title(scene, iconRumble, "Bluetooth", "Bluetooth mode changes apply after a PS4 restart.");
    const Hint hints[] = {
        { &btnArrows, "Change", COLOR_TEXT_DIM },
        { &btnTriangle, "Refresh", COLOR_TEXT_DIM },
        { &btnOptions, "Exit", COLOR_TEXT_DIM },
    };
    draw_hints(scene, hints, 3);
}

// ------------------------------------------------------------------ Bluetooth remap

void UI::render_bt_remap(Scene2D* scene, const UIExtra& extra) {
    BtRemapProfile empty;
    const BtRemapProfile& p = extra.remap ? *extra.remap : empty;
    const bool isSwitch = (p.target == BT_REMAP_SWITCH_PRO);

    // Header card
    {
        const int y = CARD_Y_START;
        const int h = 80;
        draw_rounded_rect(scene, CARD_X, y, 1440, h, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, y, 1440, h, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconRemap, CARD_X + 20, y + 16);
        draw_text(scene, std::string(tr("Controller:")) + " " + BtRemap::target_name(p.target), fontCardTitle,
                  CARD_X + 85, y + 34, COLOR_WHITE);
        char info[160];
        int rules = BtRemap::rule_count(p);
        snprintf(info, sizeof(info), "%d %s  -  %s", rules, tr(rules == 1 ? "custom rule" : "custom rules"),
                 BtRemap::profile_path(p.target).c_str());
        draw_text(scene, info, fontCardDesc, CARD_X + 85, y + 62, COLOR_TEXT_DIM);
        if (p.dirty) {
            draw_value_pill(scene, CARD_X + 1440 - 170, y + 18, 280, "Unsaved changes", false);
        }
    }

    // Grid: 2 columns x 9 rows
    const int rowsPerCol = (BT_REMAP_ROWS + 1) / 2;
    const int gridY = CARD_Y_START + 96;
    const int rowH = 50;
    const int colW = 712;
    for (int row = 0; row < BT_REMAP_ROWS; row++) {
        const int col = row / rowsPerCol;
        const int x = CARD_X + col * (colW + 16);
        const int y = gridY + (row % rowsPerCol) * rowH;
        const bool sel = (row == extra.remapRow);
        draw_rounded_rect(scene, x, y, colW, rowH - 6, 10, sel ? COLOR_SIDEBAR_BLUE : COLOR_CARD_BG);
        draw_rounded_outline(scene, x, y, colW, rowH - 6, 10, sel ? COLOR_BLUE_BRIGHT : COLOR_CARD_BORDER, sel ? 2 : 1);

        // source button (+ the physical Switch Pro button that produces it)
        std::string label = tr(BtRemap::row_label(row));
        draw_text(scene, label, fontCardTitle, x + 20, y + 31, COLOR_WHITE);
        if (isSwitch && BtRemap::switch_label(row)[0]) {
            int lw = scene->GetTextWidth(label.c_str(), fontCardTitle);
            draw_text(scene, std::string("(") + BtRemap::switch_label(row) + ")", fontCardDesc, x + 28 + lw, y + 30,
                      Color{ 140, 136, 190 });
        }

        const int c = p.choice[row];
        draw_arrow(scene, x + 372, y + 22, c == 0 ? COLOR_BORDER_DIM : COLOR_BLUE_BRIGHT);

        // destination pill
        const int pillW = 270;
        const int pillCX = x + colW - pillW / 2 - 12;
        const int pillY = y + 3;
        const int pillH = 38;
        draw_rounded_rect(scene, pillCX - pillW / 2, pillY, pillW, pillH, pillH / 2, sel ? COLOR_SIDEBAR_BLUE : COLOR_PILL_BG);
        if (sel) draw_rounded_outline(scene, pillCX - pillW / 2, pillY, pillW, pillH, pillH / 2, COLOR_BLUE_BRIGHT, 1);
        Color vc = (c == 0) ? Color{ 140, 136, 190 } : (c == BT_REMAP_CHOICES - 1 ? COLOR_ERROR : COLOR_BLUE_BRIGHT);
        draw_text_centered(scene, BtRemap::choice_label(c), fontCardDesc, pillCX, pillY + 26, vc);
        if (sel) {
            draw_text(scene, "<", fontCardDesc, pillCX - pillW / 2 + 14, pillY + 26, COLOR_BLUE_BRIGHT);
            draw_text(scene, ">", fontCardDesc, pillCX + pillW / 2 - 24, pillY + 26, COLOR_BLUE_BRIGHT);
        }
    }

    // Footer
    draw_footer_title(scene, iconRemap, "Remap Bluetooth",
                      isSwitch ? "Buttons as sent to the PS4. Letters in brackets are the Switch Pro buttons."
                               : "Buttons as sent to the PS4 by the DualSense.");
    const Hint hints[] = {
        { &btnArrows, "Change", COLOR_TEXT_DIM },
        { &btnCross, "Save", COLOR_BLUE_BRIGHT },
        { &btnSquare, "Reset", COLOR_TEXT_DIM },
        { &btnTriangle, isSwitch ? "DualSense" : "Switch Pro", COLOR_TEXT_DIM },
        { &btnOptions, "Exit", COLOR_TEXT_DIM },
    };
    draw_hints(scene, hints, 5);
}

// ------------------------------------------------------------------ Log

void UI::render_log(Scene2D* scene, const UIExtra& extra) {
    LogView empty;
    const LogView& log = extra.log ? *extra.log : empty;
    const int x = CARD_X;
    const int y = CARD_Y_START;
    const int w = 1440;
    const int h = 568;
    draw_rounded_rect(scene, x, y, w, h, 12, Color{ 14, 12, 36 });
    draw_rounded_outline(scene, x, y, w, h, 12, COLOR_CARD_BORDER, 1);

    draw_text(scene, "OmniPad Daemon Log", fontCardTitle, x + 24, y + 40, COLOR_WHITE);
    const int total = (int)log.lines.size();
    if (total > 0) {
        char range[96];
        int last = log.scroll + DaemonLog::VISIBLE_LINES;
        if (last > total) last = total;
        snprintf(range, sizeof(range), "%s %d-%d / %d", tr("Lines"), log.scroll + 1, last, total);
        int rw = scene->GetTextWidth(range, fontCardDesc);
        draw_text(scene, range, fontCardDesc, x + w - 24 - rw, y + 40, COLOR_BLUE_BRIGHT);
    }
    scene->DrawRectangle(x + 24, y + 58, w - 48, 1, COLOR_DIVIDER);

    if (!log.found) {
        draw_text_centered(scene, "No log file yet.", fontCardTitle, x + w / 2, y + 260, COLOR_WHITE);
        draw_text_centered(scene, "The OmniPad daemon writes /data/omnipad/omnipad.log after the PS4 starts.",
                           fontCardDesc, x + w / 2, y + 298, COLOR_TEXT_DIM);
    } else {
        const int lineH = 24;
        const int firstY = y + 90;
        for (int i = 0; i < DaemonLog::VISIBLE_LINES; i++) {
            int idx = log.scroll + i;
            if (idx >= total) break;
            const std::string& line = log.lines[idx];
            Color c = Color{ 215, 212, 240 };
            if (line.find("[ERROR]") != std::string::npos) c = COLOR_ERROR;
            else if (line.find("[WARN]") != std::string::npos) c = COLOR_WARN;
            else if (line.find("Switch Pro") != std::string::npos) c = COLOR_BLUE_BRIGHT;
            // log text is never translated: draw it directly
            Color bg = { 0, 0, 0 };
            std::string shown = fit_text(scene, line, fontCardDesc, w - 80);
            scene->DrawText((char*)shown.c_str(), fontCardDesc, x + 24, firstY + i * lineH, bg, c);
        }
        // scrollbar
        if (total > DaemonLog::VISIBLE_LINES) {
            const int trackX = x + w - 16;
            const int trackY = y + 72;
            const int trackH = DaemonLog::VISIBLE_LINES * lineH;
            draw_rounded_rect(scene, trackX, trackY, 6, trackH, 3, COLOR_CARD_BORDER);
            int thumbH = trackH * DaemonLog::VISIBLE_LINES / total;
            if (thumbH < 24) thumbH = 24;
            int maxScroll = total - DaemonLog::VISIBLE_LINES;
            int thumbY = trackY + (trackH - thumbH) * log.scroll / maxScroll;
            draw_rounded_rect(scene, trackX, thumbY, 6, thumbH, 3, COLOR_BLUE_MAIN);
        }
    }

    ImageRGBA none;
    draw_footer_title(scene, none, "Log", "Copy this file to a USB drive and attach it when you report a bug.");
    const Hint hints[] = {
        { &btnArrows, "Scroll", COLOR_TEXT_DIM },
        { &btnL2, "Page", COLOR_TEXT_DIM },
        { &btnSquare, "Copy to USB", COLOR_BLUE_BRIGHT },
        { &btnTriangle, "Refresh", COLOR_TEXT_DIM },
        { &btnOptions, "Exit", COLOR_TEXT_DIM },
    };
    draw_hints(scene, hints, 5);
}
