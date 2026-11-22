#pragma once
// Minimal UI translation: strings are written in English in the code and looked up in a
// per-language table at draw time (UI::draw_text). Unknown strings are shown as-is.

enum UiLanguage { LANG_ENGLISH = 0, LANG_FRENCH = 1 };

void i18n_set_language(UiLanguage lang);
UiLanguage i18n_language();

// Returns the translation of `english` for the current language, or `english` itself.
const char* tr(const char* english);
