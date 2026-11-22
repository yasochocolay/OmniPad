#pragma once
// OmniPad Companion visual theme: layout grid and colour palette shared by every screen.
#include "common/graphics.h"

// Layout
static const int SIDEBAR_X = 60;
static const int SIDEBAR_W = 310;
static const int CARD_X = 410;
static const int CARD_W = 1140;
static const int CARD_H = 92;
static const int CARD_Y_START = 280;
static const int CARD_Y_STEP = 104;
static const int FOOTER_Y = 865;

// Palette (dark violet surfaces, cyan / violet / pink accents).
// The legacy names are kept so older screens pick up the new colours.
static const Color COLOR_BLUE_GLOW   = { 58, 42, 154 };   // soft outer glow of selected items
static const Color COLOR_BLUE_MAIN   = { 123, 108, 255 }; // violet accent (borders, bars)
static const Color COLOR_BLUE_BRIGHT = { 34, 225, 255 };  // cyan accent (highlighted text, values)
static const Color COLOR_PINK        = { 255, 79, 216 };
static const Color COLOR_WHITE       = { 255, 255, 255 };
static const Color COLOR_TEXT_DIM    = { 172, 168, 214 };
static const Color COLOR_CARD_BG     = { 22, 19, 54 };
static const Color COLOR_CARD_BORDER = { 48, 42, 104 };
static const Color COLOR_DIVIDER     = { 44, 38, 96 };
static const Color COLOR_TOAST_BG    = { 30, 24, 80 };
static const Color COLOR_TOAST_BORDER= { 34, 225, 255 };
static const Color COLOR_SIDEBAR_BLUE= { 44, 34, 120 };   // selected menu item
static const Color COLOR_SIDEBAR_DARK= { 20, 17, 50 };    // menu item
static const Color COLOR_BORDER_DIM  = { 48, 42, 104 };
static const Color COLOR_PILL_BG     = { 36, 30, 88 };
static const Color COLOR_OK          = { 0, 230, 140 };
static const Color COLOR_WARN        = { 255, 196, 60 };
static const Color COLOR_ERROR       = { 255, 90, 90 };
