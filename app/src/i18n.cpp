#include "i18n.h"
#include <string>
#include <unordered_map>

namespace {

struct Entry { const char* en; const char* fr; };

// French translations, keyed by the exact English text drawn by the UI.
const Entry FRENCH[] = {
#include "i18n_fr.inc"
};

UiLanguage g_lang = LANG_ENGLISH;

const std::unordered_map<std::string, const char*>& french_map() {
    static std::unordered_map<std::string, const char*> map;
    if (map.empty()) {
        for (size_t i = 0; i < sizeof(FRENCH) / sizeof(FRENCH[0]); i++) map[FRENCH[i].en] = FRENCH[i].fr;
    }
    return map;
}

}  // namespace

void i18n_set_language(UiLanguage lang) { g_lang = lang; }
UiLanguage i18n_language() { return g_lang; }

const char* tr(const char* english) {
    if (!english || g_lang == LANG_ENGLISH) return english;
    const std::unordered_map<std::string, const char*>& map = french_map();
    std::unordered_map<std::string, const char*>::const_iterator it = map.find(english);
    return it != map.end() ? it->second : english;
}
