/*
 * Mode-Specific Configuration Settings
 * 
 * Based on actual settings structures, each mode only shows applicable settings:
 * 
 * Mini Mode: Refresh Rate, Colors (background, focus_background, separator, category, text), 
 *           Toggles, Font Sizes, Elements, DTC Format
 * 
 * Micro Mode: Refresh Rate, Colors (background, separator, category, text), Toggles, 
 *            Font Sizes, Elements, Text Alignment, Vertical Position (Top/Bottom only), DTC Format
 * 
 * Full Mode: Refresh Rate, Toggles (show_real_freqs, show_deltas, etc.), 
 *           Horizontal Position (Left/Right only) - NO colors, fonts, or elements
 * 
 * FPS Counter: Refresh Rate, Colors (background, text only), Font Sizes, 
 *             Horizontal/Vertical Position
 * 
 * FPS Graph: Refresh Rate, Colors (8 graph-specific colors), Toggles (show_info only), 
 *           Horizontal/Vertical Position - NO fonts
 * 
 * Game Resolutions: Refresh Rate, Colors (background, category, text only), 
 *                  Horizontal/Vertical Position - NO toggles, fonts, or elements
 */

#pragma once
#include <tesla.hpp>
#include "../Utils.hpp"

#include <unordered_set>

// External variables for navigation
extern std::string jumpItemName;
extern std::string jumpItemValue;
extern std::atomic<bool> jumpItemExactMatch;
static tsl::elm::ListItem* lastSelectedListItem;

// Forward declarations
class ConfiguratorOverlay;
class RefreshRateConfig;
class SampleRateConfig;
class FontSizeConfig;
class FontSizeSelector;
class PaddingsConfig;
class ColorConfig;
class ColorSelector;
class AlphaSelector;
class ShowConfig;
class TogglesConfig;
class DTCFormatConfig;
class ModeComboConfig;

// =============================================================================
// Shared helpers
// =============================================================================

// Map a mode name to its INI section string.
inline std::string modeToSection(const std::string& mode) {
    if (mode == "Mini")             return "mini";
    if (mode == "Micro")            return "micro";
    if (mode == "Full")             return "full";
    if (mode == "FPS Counter")      return "fps-counter";
    if (mode == "FPS Graph")        return "fps-graph";
    if (mode == "Game Resolutions") return "game_resolutions";
    return "";
}

// Read a boolean INI value, returning defaultVal when the key is absent.
inline bool readBool(const std::string& section, const std::string& key, bool defaultVal = true) {
    std::string value = ult::parseValueFromIniSection(configIniPath, section, key);
    if (value.empty()) return defaultVal;
    convertToUpper(value);
    return value != "FALSE";
}

// Read a boolean that is stored inverted (key is "show_stacked_*" where
// true-in-file means NOT stacked). Returns the logical "stacked" state.
inline bool readInvertedBool(const std::string& section, const std::string& key, bool defaultVal = true) {
    std::string value = ult::parseValueFromIniSection(configIniPath, section, key);
    if (value.empty()) return defaultVal;
    convertToUpper(value);
    return value == "FALSE"; // inverted: file "false" => stacked true
}

// Color helpers
inline std::string extractColorWithoutAlpha(const std::string& rgba) {
    if (rgba.length() >= 5 && rgba[0] == '#') return rgba.substr(0, 4);
    return rgba;
}

inline std::string extractAlphaFromColor(const std::string& rgba) {
    if (rgba.length() == 5 && rgba[0] == '#') return std::string(1, rgba[4]);
    return "9";
}

inline std::string setAlphaInColor(const std::string& rgba, char alpha) {
    if (rgba.length() >= 4 && rgba[0] == '#') return rgba.substr(0, 4) + alpha;
    return rgba;
}

// Shared color name lookup. Used by both ColorSelector and ColorConfig.
// Returns the color name for a #RGB string, or the hex itself if unknown.
inline std::string getColorName(const std::string& hexColor) {
    std::string rgb = (hexColor.length() == 5 && hexColor[0] == '#')
                      ? hexColor.substr(0, 4) : hexColor;

    static const std::map<std::string, std::string> colorNames = {
        // Neutrals
        {"#000","Black"},        {"#222","Charcoal"},     {"#444","Dark Gray"},
        {"#666","Gray"},         {"#789","Slate"},         {"#899","Cool Gray"},
        {"#999","Light Gray"},   {"#CCC","Silver"},        {"#EEE","Off-White"},
        {"#FEE","Warm White"},   {"#FFF","White"},

        // Reds
        {"#200","Dark Red"},     {"#802","Burgundy"},      {"#700","Maroon"},
        {"#B22","Crimson"},      {"#F20","Scarlet"},       {"#F00","Red"},
        {"#F33","Tomato"},       {"#F66","Light Red"},     {"#F84","Coral"},
        {"#FAA","Salmon"},

        // Oranges
        {"#520","Dark Orange"},  {"#A40","Burnt Orange"},  {"#B41","Rust"},
        {"#F40","Vermilion"},    {"#F80","Orange"},        {"#FA6","Apricot"},
        {"#FB6","Light Orange"}, {"#FC8","Peach"},

        // Yellows
        {"#220","Dark Yellow"},  {"#880","Olive"},         {"#CA0","Gold"},
        {"#DB0","Mustard"},      {"#ECA","Wheat"},         {"#FC0","Amber"},
        {"#FF0","Yellow"},       {"#FF6","Light Yellow"},  {"#FFC","Cream"},

        // Greens
        {"#020","Dark Green"},   {"#063","Forest Green"},  {"#080","Green"},
        {"#0C5","Emerald"},      {"#484","Fern"},          {"#682","Olive Green"},
        {"#0C0","Lime Green"},   {"#8F0","Chartreuse"},    {"#0F0","Bright Green"},
        {"#8F8","Light Green"},  {"#9B9","Sage"},          {"#AEC","Seafoam"},
        {"#BEB","Pale Green"},   {"#CFA","Mint"},

        // Teals & Cyans
        {"#022","Dark Teal"},    {"#055","Dark Cyan"},     {"#066","Teal"},
        {"#08A","Cerulean"},     {"#0AA","Aqua"},          {"#0FF","Cyan"},
        {"#799","Steel Cyan"},   {"#8FF","Light Cyan"},    {"#9EC","Seafoam Green"},

        // Blues
        {"#003","Dark Blue"},    {"#008","Navy"},          {"#04A","Cobalt"},
        {"#359","Denim"},        {"#657","Slate Navy"},    {"#48B","Steel Blue"},
        {"#06F","Royal Blue"},   {"#00F","Blue"},          {"#0AF","Azure"},
        {"#69E","Cornflower"},   {"#2DF","Light Blue"},    {"#8CF","Sky Blue"},
        {"#ACE","Powder Blue"},  {"#CEF","Ice Blue"},

        // Purples
        {"#202","Dark Purple"},  {"#404","Deep Purple"},   {"#608","Indigo"},
        {"#64F","Indigo Blue"},  {"#75F","Violet Blue"},   {"#808","Purple"},
        {"#66C","Periwinkle"},    {"#93C","Amethyst"},      {"#A0F","Violet"},
        {"#969","Mauve"},         {"#C8F","Lavender"},      {"#CCF","Light Periwinkle"},
        {"#D7D","Orchid"},        {"#DAD","Plum"},          {"#D9F","Thistle"},

        // Magentas & Pinks
        {"#606","Dark Magenta"}, {"#F0F","Magenta"},       {"#F09","Fuchsia"},
        {"#F4A","Hot Pink"},     {"#F69","Rose"},          {"#F8A","Pink"},
        {"#C89","Dusty Rose"},   {"#F9C","Petal"},         {"#FBD","Baby Pink"},
        {"#FCE","Light Pink"},   {"#FDE","Blush"},

        // Browns
        {"#321","Dark Brown"},   {"#642","Brown"},         {"#755","Muted Mauve"},
        {"#B73","Caramel"},      {"#A53","Sienna"},        {"#A75","Light Brown"},
        {"#A98","Taupe"},        {"#CB8","Sand"},          {"#DB8","Tan"},
        {"#FE9","Khaki"},
    };

    auto it = colorNames.find(rgb);
    if (it != colorNames.end()) {
        if (rgb == "#000" && hexColor.length() == 5 && hexColor[4] == '0')
            return "Transparent";
        return it->second;
    }
    return rgb;
}

// Convert a 4-bit alpha nibble character to a percentage string.
inline std::string alphaToPercent(char alpha) {
    static const char* const table[16] = {
        "0%","7%","13%","20%","27%","33%","40%","47%",
        "53%","60%","67%","73%","80%","87%","93%","100%"
    };
    const int idx = (alpha >= '0' && alpha <= '9') ? (alpha - '0')
                  : (alpha >= 'A' && alpha <= 'F') ? (alpha - 'A' + 10)
                  : (alpha >= 'a' && alpha <= 'f') ? (alpha - 'a' + 10)
                  : 9;
    return table[idx];
}

inline std::string getAlphaPercentage(const std::string& color) {
    if (color.length() == 5 && color[0] == '#') return alphaToPercent(color[4]);
    return "60%";
}

// Compact mode-flag bundle. Construct once per class from the mode string.
struct ModeFlags {
    bool isMini, isMicro, isFull, isGameRes, isFPSCounter, isFPSGraph;
    explicit ModeFlags(const std::string& mode)
        : isMini      (mode == "Mini")
        , isMicro     (mode == "Micro")
        , isFull      (mode == "Full")
        , isGameRes   (mode == "Game Resolutions")
        , isFPSCounter(mode == "FPS Counter")
        , isFPSGraph  (mode == "FPS Graph") {}
};

// Clear all three jump-state globals at once.
inline void clearJump() {
    jumpItemName = ""; jumpItemValue = ""; jumpItemExactMatch = false;
}

// Apply checkmark to newItem, clear it from the previous selection.
inline void selectItem(tsl::elm::ListItem*& lastSelected,
                       tsl::elm::ListItem* newItem,
                       const std::string& checkmark,
                       const std::string& clearValue = "") {
    if (lastSelected && lastSelected != newItem)
        lastSelected->setValue(clearValue);
    newItem->setValue(checkmark);
    lastSelected = newItem;
}

// Build a standard OverlayFrame, attach content, and return it.
// Localize the OverlayFrame subtitle before it is handed to the frame.
// OverlayFrame draws the subtitle with drawStringWithColoredSections (which only
// colors the DIVIDER_SYMBOL segments and does NOT consult the translation cache),
// so unless we translate here the subtitle would always render in English.
// Mirrors tsl::elm::CategoryHeader's set-time logic: try a full-string cache hit
// first (covers plain subtitles like "Alpha" / "DTC Format 1"), otherwise translate
// each divider-separated segment individually (covers composites like
// "Full <DIV> Configuration <DIV> Paddings"). Divider glyphs are preserved verbatim.
inline std::string localizeSubtitle(const std::string& subtitle) {
    std::string sub = subtitle;
    bool fullHit = false;
    {
        std::shared_lock<std::shared_mutex> readLock(tsl::gfx::s_translationCacheMutex);
        auto it = ult::translationCache.find(sub);
        if (it != ult::translationCache.end() && it->second != sub) {
            sub = it->second;
            fullHit = true;
        }
    }
    if (!fullHit)
        tsl::gfx::translateStringSegments(sub, tsl::s_dividerSpecialChars);
    return sub;
}

inline tsl::elm::Element* makeFrame(const std::string& subtitle, tsl::elm::Element* content) {
    auto* f = new tsl::elm::OverlayFrame("Status Monitor", localizeSubtitle(subtitle));
    f->setContent(content);
    return f;
}

// Convert a #RGB or #RGBA string to a fully-opaque tsl::Color for a color swatch.
// Each hex nibble maps directly to the 4-bit RGBA4444 channel value.
inline tsl::Color hexToSwatchColor(const std::string& hex) {
    if (hex.size() < 4 || hex[0] != '#') return tsl::Color(0xF, 0xF, 0xF, 0xF);
    auto nibble = [](char c) -> u8 {
        if (c >= '0' && c <= '9') return static_cast<u8>(c - '0');
        if (c >= 'A' && c <= 'F') return static_cast<u8>(c - 'A' + 10);
        if (c >= 'a' && c <= 'f') return static_cast<u8>(c - 'a' + 10);
        return 0u;
    };
    return tsl::Color(nibble(hex[1]), nibble(hex[2]), nibble(hex[3]), 0xF);
}

// Unicode filled square used as a color swatch in list item values.
static const std::string COLOR_SWATCH = "■";

// Special-chars vector for drawStringWithColoredSections — routes ■ to the swatch color.
static const std::vector<std::string> COLOR_SWATCH_SPECIAL = { COLOR_SWATCH };

// Template: isMini=true uses MiniListItem height, isMini=false uses full ListItem height.
// Renders the swatch in its true color by stripping it from m_value before the parent
// draws (no double-composite / anti-alias fringe), then drawing it once via
// drawStringWithColoredSections with the stored swatch color. The checkmark and focus
// highlight draw normally through the parent.
template<bool isMini>
class ColorSwatchListItemT : public std::conditional_t<isMini, tsl::elm::MiniListItem, tsl::elm::ListItem> {
    using Base = std::conditional_t<isMini, tsl::elm::MiniListItem, tsl::elm::ListItem>;
public:
    explicit ColorSwatchListItemT(const std::string& text)
        : Base(text), m_swatchColor(tsl::Color(0xF, 0xF, 0xF, 0xF)) {}

    void setSwatchColor(tsl::Color color) { m_swatchColor = color; }

    virtual void draw(tsl::gfx::Renderer* renderer) override {
        static constexpr s32 fontSize   = 20;
        static constexpr u16 itemHeight = isMini ? tsl::style::MiniListItemDefaultHeight
                                                 : tsl::style::ListItemDefaultHeight;
        static constexpr s32 yOffset    = (tsl::style::ListItemDefaultHeight - itemHeight) / 2 + 1;
        const s32 swatchY = this->getY() + 45 - yOffset - 1;

        if (this->m_flags.m_radioSelector && ult::useSwitch2Style) {
            // Switch 2 radio mode: base draws the radio circle with COLOR_SWATCH as
            // the label text (left of circle). We overdraw the label in the true
            // swatch color so it appears colored rather than plain white/grey.
            this->m_maxWidth = 0;
            Base::draw(renderer);
            // groupLeft mirrors drawRadioSelector's anchor: getX() + m_maxWidth + 47.
            // Read m_maxWidth (now freshly recalculated by Base::draw) BEFORE zeroing it.
            const s32 groupLeft = this->getX() + this->m_maxWidth + 47;
            this->m_maxWidth = 0;
            renderer->drawStringWithColoredSections(
                COLOR_SWATCH, false, COLOR_SWATCH_SPECIAL,
                groupLeft, swatchY, fontSize, tsl::Color(0, 0, 0, 0), m_swatchColor);
        } else {
            // Switch 1 / non-radio mode: original swatch-in-value behavior.
            const std::string full = this->m_value;

            const s32 fullValueWidth = renderer->getTextDimensions(full, false, 20).first;
            const s32 swatchX = this->getX() + this->getWidth() - fullValueWidth - 19;

            std::string withoutSwatch = full;
            const auto pos = withoutSwatch.find(COLOR_SWATCH);
            if (pos != std::string::npos) {
                withoutSwatch.erase(pos, COLOR_SWATCH.size());
                while (!withoutSwatch.empty() && withoutSwatch.front() == ' ')
                    withoutSwatch.erase(withoutSwatch.begin());
            }
            this->m_value    = withoutSwatch;
            this->m_maxWidth = 0;
            Base::draw(renderer);
            this->m_value    = full;
            this->m_maxWidth = 0;

            renderer->drawStringWithColoredSections(
                COLOR_SWATCH, false, COLOR_SWATCH_SPECIAL,
                swatchX, swatchY, fontSize, tsl::Color(0, 0, 0, 0), m_swatchColor);
        }
    }
private:
    tsl::Color m_swatchColor;
};

// ColorConfig navigation rows (full height); ColorSelector palette rows (mini height).
using ColorSwatchListItem     = ColorSwatchListItemT<false>;
using MiniColorSwatchListItem = ColorSwatchListItemT<true>;

// Shared color palette used by ColorSelector.
static const std::vector<std::pair<std::string, std::string>> g_colorPalette = {
    // Neutrals
    {"Black","#000"},        {"Charcoal","#222"},      {"Dark Gray","#444"},
    {"Gray","#666"},         {"Slate","#789"},          {"Cool Gray","#899"},
    {"Light Gray","#999"},   {"Silver","#CCC"},         {"Off-White","#EEE"},
    {"Warm White","#FEE"},   {"White","#FFF"},

    // Reds
    {"Dark Red","#200"},     {"Burgundy","#802"},       {"Maroon","#700"},
    {"Crimson","#B22"},      {"Scarlet","#F20"},        {"Red","#F00"},
    {"Tomato","#F33"},       {"Light Red","#F66"},      {"Coral","#F84"},
    {"Salmon","#FAA"},

    // Oranges
    {"Dark Orange","#520"},  {"Burnt Orange","#A40"},   {"Rust","#B41"},
    {"Vermilion","#F40"},    {"Orange","#F80"},         {"Apricot","#FA6"},
    {"Light Orange","#FB6"}, {"Peach","#FC8"},

    // Yellows
    {"Dark Yellow","#220"},  {"Olive","#880"},          {"Gold","#CA0"},
    {"Mustard","#DB0"},      {"Wheat","#ECA"},          {"Amber","#FC0"},
    {"Yellow","#FF0"},       {"Light Yellow","#FF6"},   {"Cream","#FFC"},

    // Greens
    {"Dark Green","#020"},   {"Forest Green","#063"},   {"Green","#080"},
    {"Emerald","#0C5"},      {"Fern","#484"},           {"Olive Green","#682"},
    {"Lime Green","#0C0"},   {"Chartreuse","#8F0"},     {"Bright Green","#0F0"},
    {"Light Green","#8F8"},  {"Sage","#9B9"},           {"Seafoam","#AEC"},
    {"Pale Green","#BEB"},   {"Mint","#CFA"},

    // Teals & Cyans
    {"Dark Teal","#022"},    {"Dark Cyan","#055"},      {"Teal","#066"},
    {"Cerulean","#08A"},     {"Aqua","#0AA"},           {"Cyan","#0FF"},
    {"Steel Cyan","#799"},   {"Light Cyan","#8FF"},     {"Seafoam Green","#9EC"},

    // Blues
    {"Dark Blue","#003"},    {"Navy","#008"},           {"Cobalt","#04A"},
    {"Denim","#359"},        {"Slate Navy","#657"},     {"Steel Blue","#48B"},
    {"Royal Blue","#06F"},   {"Blue","#00F"},           {"Azure","#0AF"},
    {"Cornflower","#69E"},   {"Light Blue","#2DF"},     {"Sky Blue","#8CF"},
    {"Powder Blue","#ACE"},  {"Ice Blue","#CEF"},

    // Purples
    {"Dark Purple","#202"},  {"Deep Purple","#404"},    {"Indigo","#608"},
    {"Indigo Blue","#64F"},  {"Violet Blue","#75F"},    {"Purple","#808"},
    {"Periwinkle","#66C"},     {"Amethyst","#93C"},     {"Violet","#A0F"},
    {"Mauve","#969"},          {"Lavender","#C8F"},     {"Light Periwinkle","#CCF"},
    {"Orchid","#D7D"},         {"Plum","#DAD"},         {"Thistle","#D9F"},

    // Magentas & Pinks
    {"Dark Magenta","#606"}, {"Magenta","#F0F"},        {"Fuchsia","#F09"},
    {"Hot Pink","#F4A"},     {"Rose","#F69"},           {"Pink","#F8A"},
    {"Dusty Rose","#C89"},   {"Petal","#F9C"},          {"Baby Pink","#FBD"},
    {"Light Pink","#FCE"},   {"Blush","#FDE"},

    // Browns
    {"Dark Brown","#321"},   {"Brown","#642"},          {"Muted Mauve","#755"},
    {"Caramel","#B73"},      {"Sienna","#A53"},         {"Light Brown","#A75"},
    {"Taupe","#A98"},        {"Sand","#CB8"},           {"Tan","#DB8"},
    {"Khaki","#FE9"},
};

// =============================================================================
// Mode Combo helpers
// =============================================================================

static constexpr const char* const g_defaultModeCombos[] = {
    "ZL+ZR+DDOWN",  "ZL+ZR+DRIGHT", "ZL+ZR+DUP",    "ZL+ZR+DLEFT",
    "L+R+DDOWN",    "L+R+DRIGHT",   "L+R+DUP",       "L+R+DLEFT",
    "L+DDOWN",      "R+DDOWN",
    "ZL+ZR+PLUS",   "L+R+PLUS",     "ZL+ZR+MINUS",   "L+R+MINUS",
    "ZL+MINUS",     "ZR+MINUS",     "ZL+PLUS",        "ZR+PLUS",    "MINUS+PLUS",
    "LS+RS",        "L+DDOWN+RS",   "L+R+LS",         "L+R+RS",
    "ZL+ZR+LS",     "ZL+ZR+RS",     "ZL+ZR+L",        "ZL+ZR+R",    "ZL+ZR+LS+RS"
};

static constexpr size_t kModeComboSlotCount = 5;

inline int modeComboIndexFor(const std::string& modeName) {
    if (modeName == "Full")             return 0;
    if (modeName == "Mini")             return 1;
    if (modeName == "Micro")            return 2;
    if (modeName == "FPS Graph")        return 3;
    if (modeName == "FPS Counter")      return 4;
    if (modeName == "Game Resolutions") return 5;
    return -1;
}

inline std::string readModeCombo(int modeIdx) {
    if (modeIdx < 0 || modeIdx >= static_cast<int>(kModeComboSlotCount)) return "";
    if (filename.empty() || !ult::isFile(ult::OVERLAYS_INI_FILEPATH)) return "";
    const std::string mc = ult::parseValueFromIniSection(
        ult::OVERLAYS_INI_FILEPATH, filename, "mode_combos");
    if (mc.empty()) return "";
    auto slots = ult::splitIniList(mc);
    if (modeIdx >= static_cast<int>(slots.size())) return "";
    return slots[modeIdx];
}

inline void removeModeComboFromOthers(const std::string& keyCombo) {
    if (keyCombo.empty()) return;
    const u64 targetKeys = tsl::hlp::comboStringToKeys(keyCombo);
    if (targetKeys == 0) return;

    if (ult::isFile(ult::OVERLAYS_INI_FILEPATH)) {
        auto data = ult::getParsedDataFromIniFile(ult::OVERLAYS_INI_FILEPATH);
        bool dirty = false;
        for (auto& [name, section] : data) {
            auto kcIt = section.find("key_combo");
            if (kcIt != section.end() && !kcIt->second.empty() &&
                tsl::hlp::comboStringToKeys(kcIt->second) == targetKeys) {
                kcIt->second = "";
                dirty = true;
            }
            auto mcIt = section.find("mode_combos");
            if (mcIt != section.end() && !mcIt->second.empty()) {
                auto slots = ult::splitIniList(mcIt->second);
                bool changed = false;
                for (auto& c : slots) {
                    if (!c.empty() && tsl::hlp::comboStringToKeys(c) == targetKeys) {
                        c = "";
                        changed = true;
                    }
                }
                if (changed) {
                    mcIt->second = "(" + ult::joinIniList(slots) + ")";
                    dirty = true;
                }
            }
        }
        if (dirty) ult::saveIniFileData(ult::OVERLAYS_INI_FILEPATH, data);
    }

    if (ult::isFile(ult::PACKAGES_INI_FILEPATH)) {
        auto pkgData = ult::getParsedDataFromIniFile(ult::PACKAGES_INI_FILEPATH);
        bool pkgDirty = false;
        for (auto& [name, section] : pkgData) {
            auto kcIt = section.find("key_combo");
            if (kcIt != section.end() && !kcIt->second.empty() &&
                tsl::hlp::comboStringToKeys(kcIt->second) == targetKeys) {
                kcIt->second = "";
                pkgDirty = true;
            }
        }
        if (pkgDirty) ult::saveIniFileData(ult::PACKAGES_INI_FILEPATH, pkgData);
    }
}

inline void writeModeCombo(int modeIdx, const std::string& combo) {
    if (modeIdx < 0 || modeIdx >= static_cast<int>(kModeComboSlotCount)) return;
    if (filename.empty()) return;
    auto iniData = ult::getParsedDataFromIniFile(ult::OVERLAYS_INI_FILEPATH);
    auto& section = iniData[filename];
    auto slots = ult::splitIniList(section["mode_combos"]);
    if (slots.size() < kModeComboSlotCount) slots.resize(kModeComboSlotCount);
    slots[modeIdx] = combo;
    section["mode_combos"] = "(" + ult::joinIniList(slots) + ")";
    ult::saveIniFileData(ult::OVERLAYS_INI_FILEPATH, iniData);
    tsl::hlp::loadEntryKeyCombos();
}

// =============================================================================
// Alpha Selector
// =============================================================================
class AlphaSelector : public tsl::Gui {
private:
    std::string modeName;
    std::string colorKey;
    std::string title;

public:
    AlphaSelector(const std::string& mode, const std::string& key, const std::string& displayTitle)
        : modeName(mode), colorKey(key), title(displayTitle) {}

    ~AlphaSelector() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(title));

        const std::string section = modeToSection(modeName);
        std::string currentColor = ult::parseValueFromIniSection(configIniPath, section, colorKey);
        if (currentColor.empty()) currentColor = "#000A";
        const std::string currentAlpha = extractAlphaFromColor(currentColor);

        static const std::pair<std::string, char> alphaOptions[16] = {
            {"0%",'0'},   {"7%",'1'},   {"13%",'2'},  {"20%",'3'},
            {"27%",'4'},  {"33%",'5'},  {"40%",'6'},  {"47%",'7'},
            {"53%",'8'},  {"60%",'9'},  {"67%",'A'},  {"73%",'B'},
            {"80%",'C'},  {"87%",'D'},  {"93%",'E'},  {"100%",'F'}
        };

        for (const auto& option : alphaOptions) {
            auto* alphaItem = new tsl::elm::MiniListItem(option.first);
            alphaItem->setRadioSelector();
            if (!currentAlpha.empty() && currentAlpha[0] == option.second)
                selectItem(lastSelectedListItem, alphaItem, ult::CHECKMARK_SYMBOL);
            alphaItem->setClickListener([this, alphaItem, option, section](uint64_t keys) {
                if (keys & KEY_A) {
                    std::string color = ult::parseValueFromIniSection(configIniPath, section, colorKey);
                    if (color.empty()) color = "#000A";
                    ult::setIniFileValue(configIniPath, section, colorKey, setAlphaInColor(color, option.second));
                    selectItem(lastSelectedListItem, alphaItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(alphaItem);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame("Alpha", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = title;
            jumpItemValue = "";
            jumpItemExactMatch = false;
            tsl::swapTo<ColorConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// DTC Format Configuration
// =============================================================================
struct DTCFormatEntry {
    std::string label;
    std::string fmt;
};
struct DTCFormatCategory {
    std::string header;
    std::vector<DTCFormatEntry> entries;
};

static const std::vector<DTCFormatCategory> dtcFormatCategories = {
    {"Time", {
        {"Time 24h",    "%H:%M"},
        {"Time 24h(s)", "%H:%M:%S"},
        {"Time 12h",    "%l:%M %p"},
        {"Time 12h(s)", "%l:%M:%S %p"}
    }},
    {"Date", {
        {"Date US Dash",   "%m-%d-%Y"},
        {"Date US Slash",  "%m/%d/%Y"},
        {"Date EU Dash",   "%d-%m-%Y"},
        {"Date EU Slash",  "%d/%m/%Y"},
        {"Date ISO",       "%Y-%m-%d"},
        {"Date ISO Slash", "%Y/%m/%d"},
        {"Date Short US",  "%m/%d/%y"},
        {"Date Short EU",  "%d/%m/%y"},
        {"Date Compact",   "%Y%m%d"}
    }},
    {"Day & Month", {
        {"Day + Date Short", "%a, %b %d"},
        {"Day + Date Full",  "%A, %B %d"},
        {"Weekday Short",    "%a"},
        {"Weekday Full",     "%A"},
        {"Month Short",      "%b"},
        {"Month Full",       "%B"},
        {"Month + Year",     "%b %Y"},
        {"Day of Year",      "%j"},
    }},
};

static const std::vector<std::pair<std::string, std::string>>& getDTCFormatsFlat() {
    static const std::vector<std::pair<std::string, std::string>> flat = []() {
        std::vector<std::pair<std::string, std::string>> out;
        for (const auto& cat : dtcFormatCategories)
            for (const auto& e : cat.entries)
                out.push_back({e.label, e.fmt});
        return out;
    }();
    return flat;
}

class DTCFormatConfig : public tsl::Gui {
private:
    std::string modeName;
    int slot;

public:
    DTCFormatConfig(const std::string& mode, int slotIndex)
        : modeName(mode), slot(slotIndex) {}
    ~DTCFormatConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        const std::string section = modeToSection(modeName);
        const std::string iniKey  = (slot == 2) ? "dtc_format_2" : "dtc_format_1";
        const std::string title   = (slot == 2) ? "DTC Format 2" : "DTC Format 1";
        //const bool isMiniMode     = (modeName == "Mini");

        std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, iniKey);
        if (currentValue.empty()) {
            if (slot == 1) currentValue = std::string("%a, %b %d");
            else           currentValue = std::string("%H:%M:%S");
        }

        if (slot == 2) {
            list->addItem(new tsl::elm::CategoryHeader("None"));
            auto* noneItem = new tsl::elm::MiniListItem(ult::OPTION_SYMBOL);
            noneItem->setRadioSelector();
            if (currentValue == ult::OPTION_SYMBOL)
                selectItem(lastSelectedListItem, noneItem, ult::CHECKMARK_SYMBOL);
            noneItem->setClickListener([this, noneItem, section](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "dtc_format_2", ult::OPTION_SYMBOL);
                    selectItem(lastSelectedListItem, noneItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(noneItem);
        }

        for (const auto& cat : dtcFormatCategories) {
            list->addItem(new tsl::elm::CategoryHeader(cat.header));
            for (const auto& entry : cat.entries) {
                auto* formatItem = new tsl::elm::MiniListItem(entry.label);
                formatItem->setRadioSelector();
                if (entry.fmt == currentValue)
                    selectItem(lastSelectedListItem, formatItem, ult::CHECKMARK_SYMBOL);
                const std::string capturedFmt = entry.fmt;
                const std::string capturedKey = iniKey;
                formatItem->setClickListener([this, formatItem, capturedFmt, capturedKey, section](uint64_t keys) {
                    if (keys & KEY_A) {
                        ult::setIniFileValue(configIniPath, section, capturedKey, capturedFmt);
                        selectItem(lastSelectedListItem, formatItem, ult::CHECKMARK_SYMBOL);
                        return true;
                    }
                    return false;
                });
                list->addItem(formatItem);
            }
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(title, list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = (slot == 2) ? "DTC Format 2" : "DTC Format 1";
            jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Toggles Configuration
// =============================================================================
class TogglesConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;
    std::string section; // cached once

    void addToggle(tsl::elm::List* list, const std::string& label,
                   const std::string& key, bool defaultVal,
                   const std::string& overrideSection = "") {
        const std::string& sec = overrideSection.empty() ? section : overrideSection;
        auto* item = new tsl::elm::MiniToggleListItem(label, readBool(sec, key, defaultVal));
        item->setStateChangedListener([sec, key](bool state) {
            ult::setIniFileValue(configIniPath, sec, key, state ? "true" : "false");
        });
        list->addItem(item);
    }



public:
    TogglesConfig(const std::string& mode) : modeName(mode), flags(mode) {
        section = modeToSection(mode);
    }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();

        if (flags.isFPSGraph) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Info",                "show_info",           true);
            addToggle(list, "Dynamic Temps", "use_dynamic_colors",  true);
            addToggle(list, "Integer FPS",         "integer_fps",         true);
            addToggle(list, "Border",         "use_border",       true);
            addToggle(list, "Dynamic Border", "dynamic_border",   true);
            addToggle(list, "CW Border Flow", "cw_border_flow",   true);
            addToggle(list, "Graph Border",   "use_graph_border", false);
            addToggle(list, "Graph Background", "graph_background", false);

        } else if (flags.isFull) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Real Frequencies",    "show_real_freqs",     true);
            addToggle(list, "Target Frequencies",  "show_target_freqs",   true);
            addToggle(list, "Frequency Deltas",    "show_deltas",         true);
            addToggle(list, "FPS",                 "show_fps",            true);
            addToggle(list, "RES",                 "show_res",            true);
            addToggle(list, "Read Speed",          "show_read_speed",     true);
            addToggle(list, "Dynamic Temps", "use_dynamic_colors",  true);

        } else if (flags.isMini || flags.isMicro) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "1080p Docked",   "use_1080p_docked",   true);
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);

            if (flags.isMini)
                addToggle(list, "Labels", "show_labels", true);

            addToggle(list, "Real Frequencies",    "real_freqs",          true);
            addToggle(list, "Real Voltages",       "real_volts",          true);
            addToggle(list, "Dynamic Temps", "use_dynamic_colors",  true);
            if (flags.isMini) {
                addToggle(list, "Border",         "use_border",      true);
                addToggle(list, "Dynamic Border", "dynamic_border",  true);
                addToggle(list, "CW Border Flow", "cw_border_flow",  true);
            }

            list->addItem(new tsl::elm::CategoryHeader("CPU"));
            addToggle(list, "Full CPU",              "show_full_cpu",              true);
            addToggle(list, "Full CPU Max Core 0-2", "show_full_cpu_max_core_012", flags.isMini);
            addToggle(list, "Stacked Full CPU",      "show_stacked_full_cpu",      true);
            addToggle(list, "CPU Temp",              "show_cpu_temp",              flags.isMicro);
            addToggle(list, "Stacked CPU Temp",      "show_stacked_cpu_temp",      true);
            addToggle(list, "Voltage At End",        "voltage_at_end_cpu",         flags.isMicro);

            list->addItem(new tsl::elm::CategoryHeader("GPU"));
            addToggle(list, "GPU Temp",         "show_gpu_temp",         flags.isMicro);
            addToggle(list, "Stacked GPU Temp", "show_stacked_gpu_temp", true);
            addToggle(list, "Voltage At End",   "voltage_at_end_gpu",    true);

            list->addItem(new tsl::elm::CategoryHeader("RAM"));
            addToggle(list, "RAM Bandwidth",         "show_ram_bandwidth",               true);
            addToggle(list, "Stacked RAM Bandwidth", "show_stacked_ram_bandwidth",       true);
            addToggle(list, "RAM Load CPU/GPU", "show_RAM_load_CPU_GPU",                 false);
            addToggle(list, "Stacked RAM Load CPU/GPU", "show_stacked_ram_load_cpu_gpu", true);

            if (isMariko)
                addToggle(list, "VDD2", "show_vdd2", true);

            addToggle(list, "VDDQ", "show_vddq", flags.isMini);

            if (isMariko)
                addToggle(list, "Stacked VDD2/VDDQ", "show_stacked_vddq", true);

            addToggle(list, "RAM Temp",         "show_ram_temp",         flags.isMicro);
            addToggle(list, "Stacked RAM Temp", "show_stacked_ram_temp", true);
            addToggle(list, "Voltage At End",   "voltage_at_end_ram",    flags.isMicro);

            list->addItem(new tsl::elm::CategoryHeader("TMP"));
            {
                // Mutual-exclusivity pair
                auto* compTemps   = new tsl::elm::MiniToggleListItem("CPU/GPU/RAM Temps",
                    readBool(section, "show_component_temps", true));
                auto* socPcbTemps = new tsl::elm::MiniToggleListItem("SOC/PCB/Skin Temps",
                    readBool(section, "show_soc_pcb_skin_temps", true));

                compTemps->setStateChangedListener([this, compTemps, socPcbTemps](bool state) {
                    if (!state && !socPcbTemps->getState()) {
                        compTemps->setState(true);
                        ult::setIniFileValue(configIniPath, section, "show_component_temps", "true");
                    } else {
                        ult::setIniFileValue(configIniPath, section, "show_component_temps",
                                            state ? "true" : "false");
                    }
                });
                socPcbTemps->setStateChangedListener([this, compTemps, socPcbTemps](bool state) {
                    if (!state && !compTemps->getState()) {
                        socPcbTemps->setState(true);
                        ult::setIniFileValue(configIniPath, section, "show_soc_pcb_skin_temps", "true");
                    } else {
                        ult::setIniFileValue(configIniPath, section, "show_soc_pcb_skin_temps",
                                            state ? "true" : "false");
                    }
                });
                list->addItem(compTemps);
                list->addItem(socPcbTemps);

                if (flags.isMicro)
                    addToggle(list, "Stacked Temps", "show_stacked_temps", true);
            }

            addToggle(list, "SOC Voltage",     "show_soc_voltage",     true);
            addToggle(list, "Stacked Fan/SOC", "show_stacked_fan_soc", true);
            addToggle(list, "Voltage At End",  "voltage_at_end_tmp",   true);

            list->addItem(new tsl::elm::CategoryHeader("RES"));
            addToggle(list, "Full Resolution", "show_full_res", true);
            addToggle(list, "Primary Only",    "show_primary_res", flags.isMicro);

            list->addItem(new tsl::elm::CategoryHeader("FPS"));
            addToggle(list, "Integer FPS", "integer_fps", true);
            addToggle(list, "Use FPS Graph", "use_fps_graph", false);

            list->addItem(new tsl::elm::CategoryHeader("BAT"));
            addToggle(list, "Invert Battery Display", "invert_battery_display", true);
            addToggle(list, "Stacked", "show_stacked_bat", flags.isMicro);

            list->addItem(new tsl::elm::CategoryHeader("DTC"));
            addToggle(list, "Stop Watch",     "show_stopwatch",   true);
            addToggle(list, "Use DTC Symbol", "use_dtc_symbol",   true);
            addToggle(list, "Stacked",        "show_stacked_dtc", flags.isMicro);

        } else if (flags.isGameRes) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Border",         "use_border",      true);
            addToggle(list, "Dynamic Border", "dynamic_border",  true);
            addToggle(list, "CW Border Flow", "cw_border_flow",  true);

        } else if (flags.isFPSCounter) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "1080p Docked",   "use_1080p_docked",   true);
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Integer FPS",         "integer_fps",         true);
            addToggle(list, "Border",         "use_border",      true);
            addToggle(list, "Dynamic Border", "dynamic_border",  true);
            addToggle(list, "CW Border Flow", "cw_border_flow",  true);
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        clearJump();

        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// =============================================================================
// Sample Rate Configuration (Mini / FPS Graph)
// =============================================================================
class SampleRateConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;
    int currentRate;
    int maxRate;  // capped at the current refresh_rate

public:
    SampleRateConfig(const std::string& mode) : modeName(mode), flags(mode) {
        const std::string section = modeToSection(mode);
        const std::string rrVal = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        const int defaultRate       = flags.isFPSGraph ? 30 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 30 : (flags.isFull ? 2 : 3)));
        const int defaultSampleRate = (flags.isFPSGraph ? 3 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 2 : defaultRate)));
        maxRate = rrVal.empty() ? defaultRate : std::clamp(atoi(rrVal.c_str()), 1, 60);
        const std::string srVal = ult::parseValueFromIniSection(configIniPath, section, "sample_rate");
        currentRate = srVal.empty() ? std::min(defaultSampleRate, maxRate) : std::clamp(atoi(srVal.c_str()), 1, maxRate);
    }
    ~SampleRateConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Sample Rate"));

        const std::string section = modeToSection(modeName);
        static const int rates[] = {1, 2, 3, 5, 10, 15, 30, 60};
        for (int rate : rates) {
            if (rate > maxRate) break;
            auto* rateItem = new tsl::elm::MiniListItem(std::to_string(rate) + " Hz");
            rateItem->setRadioSelector();
            if (rate == currentRate)
                selectItem(lastSelectedListItem, rateItem, ult::CHECKMARK_SYMBOL);
            rateItem->setClickListener([this, rateItem, rate, section](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "sample_rate", std::to_string(rate));
                    selectItem(lastSelectedListItem, rateItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(rateItem);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + std::string(ult::DIVIDER_SYMBOL) + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Sample Rate"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Refresh Rate Configuration
// =============================================================================
class RefreshRateConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;
    int         currentRate;

public:
    RefreshRateConfig(const std::string& mode) : modeName(mode), flags(mode) {
        const std::string section = modeToSection(mode);
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        const int defaultRate = flags.isFPSGraph ? 30 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 30 : (flags.isFull ? 2 : 3)));
        currentRate = value.empty() ? defaultRate : std::clamp(atoi(value.c_str()), 1, 60);
    }
    ~RefreshRateConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Refresh Rate"));

        const std::string section = modeToSection(modeName);
        static const int rates[] = {1, 2, 3, 5, 10, 15, 30, 60};
        for (int rate : rates) {
            auto* rateItem = new tsl::elm::MiniListItem(std::to_string(rate) + " Hz");
            rateItem->setRadioSelector();
            if (rate == currentRate)
                selectItem(lastSelectedListItem, rateItem, ult::CHECKMARK_SYMBOL);
            rateItem->setClickListener([this, rateItem, rate, section](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "refresh_rate", std::to_string(rate));
                    selectItem(lastSelectedListItem, rateItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(rateItem);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Refresh Rate"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Mode Combo Configuration
// =============================================================================
class ModeComboConfig : public tsl::Gui {
private:
    std::string modeName;
    int         modeIdx;
    std::string currentCombo;

public:
    ModeComboConfig(const std::string& mode)
        : modeName(mode)
        , modeIdx(modeComboIndexFor(mode))
        , currentCombo(readModeCombo(modeIdx)) {}

    ~ModeComboConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Mode Combo"));

        {
            auto* noneItem = new tsl::elm::ListItem(ult::OPTION_SYMBOL);
            noneItem->setRadioSelector();
            if (currentCombo.empty())
                selectItem(lastSelectedListItem, noneItem, ult::CHECKMARK_SYMBOL);
            noneItem->setClickListener([this, noneItem](uint64_t keys) -> bool {
                if (!(keys & KEY_A)) return false;
                writeModeCombo(modeIdx, "");
                currentCombo.clear();
                selectItem(lastSelectedListItem, noneItem, ult::CHECKMARK_SYMBOL);
                return true;
            });
            list->addItem(noneItem);
        }

        const u64 launchKeys  = tsl::cfg::launchCombo;
        const u64 currentKeys = currentCombo.empty() ? 0
                              : tsl::hlp::comboStringToKeys(currentCombo);

        for (const auto& combo : g_defaultModeCombos) {
            const u64 comboKeys = tsl::hlp::comboStringToKeys(combo);
            if (comboKeys == launchKeys) continue;

            std::string display = combo;
            ult::convertComboToUnicode(display);

            auto* item = new tsl::elm::ListItem(display);
            item->setRadioSelector();
            if (currentKeys != 0 && comboKeys == currentKeys)
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);

            const std::string comboStr(combo);
            item->setClickListener([this, item, comboStr](uint64_t keys) -> bool {
                if (!(keys & KEY_A)) return false;
                removeModeComboFromOthers(comboStr);
                writeModeCombo(modeIdx, comboStr);
                currentCombo = comboStr;
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
                return true;
            });
            list->addItem(item);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Mode Combo"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Frame Padding Configuration
// =============================================================================
class FramePaddingConfig : public tsl::Gui {
private:
    std::string modeName;
    std::string section;
    int currentPadding;

public:
    FramePaddingConfig(const std::string& mode) : modeName(mode) {
        section = modeToSection(mode);
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "frame_padding");
        currentPadding = value.empty() ? 0 : std::clamp(atoi(value.c_str()), 0, 14);
    }
    ~FramePaddingConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Frame Padding"));

        for (int padding = 0; padding <= 14; ++padding) {
            auto* paddingItem = new tsl::elm::MiniListItem(std::to_string(padding) + " px");
            paddingItem->setRadioSelector();
            if (padding == currentPadding)
                selectItem(lastSelectedListItem, paddingItem, ult::CHECKMARK_SYMBOL);
            paddingItem->setClickListener([this, paddingItem, padding](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "frame_padding", std::to_string(padding));
                    selectItem(lastSelectedListItem, paddingItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(paddingItem);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Frame Padding"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<PaddingsConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// Returns the per-mode default border thickness in tenths of a space (sp),
// used as the fallback when the ini key is absent.
// All modes: 8 (0.8 sp)
inline int defaultBorderThickness(const std::string& mode) {
    (void)mode;
    return 8; // 0.8 sp for all modes
}

// Border thickness for the configurable Switch 2 frame border. Stored in tenths
// of a space (sp); a radio-list dropdown. Range 1..30 tenths (0.1–3.0 sp).
class BorderThicknessConfig : public tsl::Gui {
private:
    std::string modeName;
    std::string section;
    int currentThickness;

public:
    BorderThicknessConfig(const std::string& mode) : modeName(mode) {
        section = modeToSection(mode);
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "border_thickness");
        currentThickness = value.empty() ? defaultBorderThickness(mode) : std::clamp(atoi(value.c_str()), 1, 30);
    }
    ~BorderThicknessConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Border Thickness"));

        for (int tenths = 1; tenths <= 30; ++tenths) {
            const std::string label = std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + " sp";
            auto* item = new tsl::elm::MiniListItem(label);
            item->setRadioSelector();
            if (tenths == currentThickness)
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
            item->setClickListener([this, item, tenths](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "border_thickness", std::to_string(tenths));
                    selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Border Thickness"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<PaddingsConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Micro Padding Configs (Horizontal / Vertical / Label / Element)
// =============================================================================

// Micro paddings are stored in tenths of a space.
// Format a tenths value as "X.Y sp" (used for horizontal / vertical / label).
inline std::string formatSpTenths(int tenths) {
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + " sp";
}
// Format a tenths value that is a whole number of spaces as "N sp" (element padding).
inline std::string formatSpWhole(int tenths) {
    return std::to_string(tenths / 10) + " sp";
}

// Shared implementation for all Micro padding screens. Range/step come from the
// template so each screen can differ; only the default, range and ini key vary.
//   Horizontal/Vertical/Label: 2..30 (0.2..3.0 sp, step 1 = 0.1 sp)
//   Element:                   10..100 (1..10 sp, step 10 = 1 sp)
template<int DEFAULT, int MIN_T, int MAX_T, int STEP_T>
class MicroPaddingConfigBase : public tsl::Gui {
protected:
    static constexpr int MIN_P  = MIN_T;
    static constexpr int MAX_P  = MAX_T;
    static constexpr int STEP_P = STEP_T;

    int         currentPadding;
    std::string iniKey;
    std::string headerLabel;
    std::string jumpLabel;
    std::string modeName = "Micro";

    virtual std::string formatLabel(int tenths) const { return formatSpTenths(tenths); }

public:
    MicroPaddingConfigBase(const std::string& key, const std::string& header, const std::string& jump)
        : iniKey(key), headerLabel(header), jumpLabel(jump) {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", iniKey);
        currentPadding = value.empty() ? DEFAULT : std::clamp(atoi(value.c_str()), MIN_P, MAX_P);
        // Snap to the nearest valid step so a stray value still shows a checkmark.
        currentPadding = MIN_P + ((currentPadding - MIN_P + (STEP_P / 2)) / STEP_P) * STEP_P;
        currentPadding = std::clamp(currentPadding, MIN_P, MAX_P);
    }
    ~MicroPaddingConfigBase() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(headerLabel));
        for (int p = MIN_P; p <= MAX_P; p += STEP_P) {
            auto* item = new tsl::elm::MiniListItem(formatLabel(p));
            item->setRadioSelector();
            if (p == currentPadding)
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
            item->setClickListener([this, item, p](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, "micro", iniKey, std::to_string(p));
                    selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame("Micro " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = jumpLabel; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<PaddingsConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

class MicroHPaddingConfig : public MicroPaddingConfigBase<14, 2, 30, 1> {
public:
    MicroHPaddingConfig() : MicroPaddingConfigBase("horizontal_padding", "Horizontal Padding", "Horizontal Padding") {}
};

class MicroVPaddingConfig : public MicroPaddingConfigBase<8, 2, 30, 1> {
public:
    MicroVPaddingConfig() : MicroPaddingConfigBase("vertical_padding", "Vertical Padding", "Vertical Padding") {}
};

class MicroStackedSpacingConfig : public MicroPaddingConfigBase<4, 0, 30, 1> {
public:
    MicroStackedSpacingConfig() : MicroPaddingConfigBase("stacked_spacing", "Stacked Spacing", "Stacked Spacing") {}
};

class MicroLabelPaddingConfig : public MicroPaddingConfigBase<14, 2, 30, 1> {
public:
    MicroLabelPaddingConfig() : MicroPaddingConfigBase("label_padding", "Label Padding", "Label Padding") {}
};

class MicroElementPaddingConfig : public MicroPaddingConfigBase<50, 10, 100, 10> {
protected:
    std::string formatLabel(int tenths) const override { return formatSpWhole(tenths); }
public:
    MicroElementPaddingConfig() : MicroPaddingConfigBase("element_padding", "Element Padding", "Element Padding") {}
};

// =============================================================================
// Mini Padding Configs (Horizontal / Vertical / Spacing / Corner Radius)
// =============================================================================
// Mirrors the Micro padding screens but writes to the [mini] ini section and
// swaps back to the Mini configurator. All values are tenths of a space ("sp").
//   Horizontal/Vertical/Spacing: 2..30 (0.2..3.0 sp, step 1 = 0.1 sp)
//   Corner Radius:               0..80 (0..8.0 sp, step 2 = 0.2 sp)
template<int DEFAULT, int MIN_T, int MAX_T, int STEP_T>
class MiniPaddingConfigBase : public tsl::Gui {
protected:
    static constexpr int MIN_P  = MIN_T;
    static constexpr int MAX_P  = MAX_T;
    static constexpr int STEP_P = STEP_T;

    int         currentPadding;
    std::string iniKey;
    std::string headerLabel;
    std::string jumpLabel;
    std::string modeName = "Mini";

    virtual std::string formatLabel(int tenths) const { return formatSpTenths(tenths); }

public:
    MiniPaddingConfigBase(const std::string& key, const std::string& header, const std::string& jump)
        : iniKey(key), headerLabel(header), jumpLabel(jump) {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", iniKey);
        currentPadding = value.empty() ? DEFAULT : std::clamp(atoi(value.c_str()), MIN_P, MAX_P);
        // Snap to the nearest valid step so a stray value still shows a checkmark.
        currentPadding = MIN_P + ((currentPadding - MIN_P + (STEP_P / 2)) / STEP_P) * STEP_P;
        currentPadding = std::clamp(currentPadding, MIN_P, MAX_P);
    }
    ~MiniPaddingConfigBase() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(headerLabel));
        for (int p = MIN_P; p <= MAX_P; p += STEP_P) {
            auto* item = new tsl::elm::MiniListItem(formatLabel(p));
            item->setRadioSelector();
            if (p == currentPadding)
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
            item->setClickListener([this, item, p](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, "mini", iniKey, std::to_string(p));
                    selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame("Mini " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = jumpLabel; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<PaddingsConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

class MiniHPaddingConfig : public MiniPaddingConfigBase<34, 2, 60, 1> {
public:
    MiniHPaddingConfig() : MiniPaddingConfigBase("horizontal_padding", "Horizontal Padding", "Horizontal Padding") {}
};

class MiniVPaddingConfig : public MiniPaddingConfigBase<34, 2, 60, 1> {
public:
    MiniVPaddingConfig() : MiniPaddingConfigBase("vertical_padding", "Vertical Padding", "Vertical Padding") {}
};

class MiniSpacingConfig : public MiniPaddingConfigBase<15, 2, 30, 1> {
public:
    MiniSpacingConfig() : MiniPaddingConfigBase("spacing", "Spacing", "Spacing") {}
};

class MiniStackedSpacingConfig : public MiniPaddingConfigBase<4, 0, 30, 1> {
public:
    MiniStackedSpacingConfig() : MiniPaddingConfigBase("stacked_spacing", "Stacked Spacing", "Stacked Spacing") {}
};

class MiniCornerRadiusConfig : public MiniPaddingConfigBase<36, 0, 80, 2> {
public:
    MiniCornerRadiusConfig() : MiniPaddingConfigBase("corner_radius", "Corner Radius", "Corner Radius") {}
};

// Mode-generic Corner Radius (sp) config for modes that don't use the Mini-only
// MiniCornerRadiusConfig (FPS Graph / FPS Counter / Game Resolutions). Writes
// corner_radius (tenths of a space, 0..80 step 2) to the mode's own section.
class CornerRadiusConfig : public tsl::Gui {
private:
    std::string modeName;
    std::string section;
    int currentRadius;

public:
    CornerRadiusConfig(const std::string& mode) : modeName(mode) {
        section = modeToSection(mode);
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "corner_radius");
        currentRadius = value.empty() ? 36 : std::clamp(atoi(value.c_str()), 0, 80);
        currentRadius = ((currentRadius + 1) / 2) * 2;   // snap to step 2
        currentRadius = std::clamp(currentRadius, 0, 80);
    }
    ~CornerRadiusConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Corner Radius"));
        for (int p = 0; p <= 80; p += 2) {
            auto* item = new tsl::elm::MiniListItem(formatSpTenths(p));
            item->setRadioSelector();
            if (p == currentRadius)
                selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
            item->setClickListener([this, item, p](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, "corner_radius", std::to_string(p));
                    currentRadius = p;
                    selectItem(lastSelectedListItem, item, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Corner Radius"; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<PaddingsConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Paddings Configuration (hub — one dropdown for all per-mode padding options)
// =============================================================================
class PaddingsConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;

    // ---- value helpers (mirrors ConfiguratorOverlay getters) ----------------
    int getFramePadding() const {
        const std::string section = modeToSection(modeName);
        const std::string v = ult::parseValueFromIniSection(configIniPath, section, "frame_padding");
        return v.empty() ? 0 : std::clamp(atoi(v.c_str()), 0, 14);
    }
    int getBorderThickness() const {
        const std::string section = modeToSection(modeName);
        const std::string v = ult::parseValueFromIniSection(configIniPath, section, "border_thickness");
        return v.empty() ? defaultBorderThickness(modeName) : std::clamp(atoi(v.c_str()), 0, 30);
    }
    int getCornerRadius() const {
        const std::string section = modeToSection(modeName);
        const std::string v = ult::parseValueFromIniSection(configIniPath, section, "corner_radius");
        return v.empty() ? 36 : std::clamp(atoi(v.c_str()), 0, 80);
    }
    int getMicroHPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "micro", "horizontal_padding");
        return v.empty() ? 14 : std::clamp(atoi(v.c_str()), 2, 30);
    }
    int getMicroVPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "micro", "vertical_padding");
        return v.empty() ? 8 : std::clamp(atoi(v.c_str()), 2, 30);
    }
    int getMicroStackedSpacing() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "micro", "stacked_spacing");
        int val = v.empty() ? 4 : std::clamp(atoi(v.c_str()), 0, 30);
        val = 0 + ((val - 0 + (1 / 2)) / 1) * 1;
        return std::clamp(val, 0, 30);
    }
    int getMicroLabelPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "micro", "label_padding");
        return v.empty() ? 14 : std::clamp(atoi(v.c_str()), 2, 30);
    }
    int getMicroElementPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "micro", "element_padding");
        int val = v.empty() ? 50 : std::clamp(atoi(v.c_str()), 10, 100);
        val = 10 + ((val - 10 + (10 / 2)) / 10) * 10;
        return std::clamp(val, 10, 100);
    }
    int getMiniHPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "mini", "horizontal_padding");
        return v.empty() ? 34 : std::clamp(atoi(v.c_str()), 2, 60);
    }
    int getMiniVPadding() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "mini", "vertical_padding");
        return v.empty() ? 34 : std::clamp(atoi(v.c_str()), 2, 60);
    }
    int getMiniSpacing() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "mini", "spacing");
        return v.empty() ? 15 : std::clamp(atoi(v.c_str()), 2, 30);
    }
    int getMiniStackedSpacing() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "mini", "stacked_spacing");
        int val = v.empty() ? 4 : std::clamp(atoi(v.c_str()), 0, 30);
        val = 0 + ((val - 0 + (1 / 2)) / 1) * 1;
        return std::clamp(val, 0, 30);
    }
    int getMiniCornerRadius() const {
        const std::string v = ult::parseValueFromIniSection(configIniPath, "mini", "corner_radius");
        int val = v.empty() ? 36 : std::clamp(atoi(v.c_str()), 0, 80);
        val = 0 + ((val - 0 + (2 / 2)) / 2) * 2;
        return std::clamp(val, 0, 80);
    }

public:
    PaddingsConfig(const std::string& mode) : modeName(mode), flags(mode) {}
    ~PaddingsConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Paddings"));

        // Frame Padding — Mini / Game Resolutions / FPS Counter / FPS Graph
        if (flags.isMini || flags.isGameRes || flags.isFPSCounter || flags.isFPSGraph) {
            auto* item = new tsl::elm::ListItem("Frame Padding");
            item->setValue(std::to_string(getFramePadding()) + " px");
            item->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<FramePaddingConfig>(modeName); return true; }
                return false;
            });
            list->addItem(item);

            // Border Thickness - same modes carry the configurable frame border.
            auto* btItem = new tsl::elm::ListItem("Border Thickness");
            {
                const int bt = getBorderThickness();
                btItem->setValue(std::to_string(bt / 10) + "." + std::to_string(bt % 10) + " sp");
            }
            btItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<BorderThicknessConfig>(modeName); return true; }
                return false;
            });
            list->addItem(btItem);

            // Corner Radius (sp) for the non-Mini border modes; Mini has its own
            // entry below via MiniCornerRadiusConfig.
            if (flags.isGameRes || flags.isFPSCounter || flags.isFPSGraph) {
                auto* crItem = new tsl::elm::ListItem("Corner Radius");
                crItem->setValue(formatSpTenths(getCornerRadius()));
                crItem->setClickListener([this](uint64_t keys) {
                    if (keys & KEY_A) { tsl::changeTo<CornerRadiusConfig>(modeName); return true; }
                    return false;
                });
                list->addItem(crItem);
            }
        }

        // Mini paddings
        if (flags.isMini) {
            auto* hItem = new tsl::elm::ListItem("Horizontal Padding");
            hItem->setValue(formatSpTenths(getMiniHPadding()));
            hItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MiniHPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(hItem);

            auto* vItem = new tsl::elm::ListItem("Vertical Padding");
            vItem->setValue(formatSpTenths(getMiniVPadding()));
            vItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MiniVPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(vItem);

            auto* spItem = new tsl::elm::ListItem("Spacing");
            spItem->setValue(formatSpTenths(getMiniSpacing()));
            spItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MiniSpacingConfig>(); return true; }
                return false;
            });
            list->addItem(spItem);

            auto* ssItem = new tsl::elm::ListItem("Stacked Spacing");
            ssItem->setValue(formatSpTenths(getMiniStackedSpacing()));
            ssItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MiniStackedSpacingConfig>(); return true; }
                return false;
            });
            list->addItem(ssItem);

            auto* crItem = new tsl::elm::ListItem("Corner Radius");
            crItem->setValue(formatSpTenths(getMiniCornerRadius()));
            crItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MiniCornerRadiusConfig>(); return true; }
                return false;
            });
            list->addItem(crItem);
        }

        // Micro paddings
        if (flags.isMicro) {
            auto* hItem = new tsl::elm::ListItem("Horizontal Padding");
            hItem->setValue(formatSpTenths(getMicroHPadding()));
            hItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroHPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(hItem);

            auto* vItem = new tsl::elm::ListItem("Vertical Padding");
            vItem->setValue(formatSpTenths(getMicroVPadding()));
            vItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroVPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(vItem);

            auto* ssItem = new tsl::elm::ListItem("Stacked Spacing");
            ssItem->setValue(formatSpTenths(getMicroStackedSpacing()));
            ssItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroStackedSpacingConfig>(); return true; }
                return false;
            });
            list->addItem(ssItem);

            auto* lItem = new tsl::elm::ListItem("Label Padding");
            lItem->setValue(formatSpTenths(getMicroLabelPadding()));
            lItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroLabelPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(lItem);

            auto* eItem = new tsl::elm::ListItem("Element Padding");
            eItem->setValue(formatSpWhole(getMicroElementPadding()));
            eItem->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroElementPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(eItem);
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration " + ult::DIVIDER_SYMBOL + " Paddings", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = "Paddings"; jumpItemValue = ""; jumpItemExactMatch = true;
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Font Size Selector
// =============================================================================
class FontSizeSelector : public tsl::Gui {
private:
    std::string modeName;
    std::string fontType;
    ModeFlags   flags;
    std::string title;

public:
    FontSizeSelector(const std::string& mode, const std::string& type)
        : modeName(mode), fontType(type), flags(mode) {
        // Build a human-readable title for the header and back-navigation jump.
        if (fontType == "handheld")
            title = "Handheld Font Size";
        else if (fontType == "docked")
            title = "Docked Font Size";
        else if (fontType == "docked_1080p")
            title = "1080p Docked Font Size";
        else {
            title = fontType;
            title[0] = std::toupper(title[0]);
            title += " Font Size";
        }
    }
    ~FontSizeSelector() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(title));

        const std::string section = modeToSection(modeName);
        const std::string keyName = fontType + "_font_size";
        const std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, keyName);

        // Default sizes per type; 1080p defaults are ~1.5× the 720p docked defaults.
        int defaultSize;
        if (fontType == "docked_1080p")
            defaultSize = flags.isFPSCounter ? 68 : 21;
        else
            defaultSize = flags.isFPSCounter ? 40 : 15;

        const int currentSize = currentValue.empty() ? defaultSize : atoi(currentValue.c_str());

        const int minSize = 8;
        // 1080p allows larger values since 1px = 1px (no 1.5× VI scale).
        int maxSize;
        if (fontType == "docked_1080p")
            maxSize = flags.isFPSCounter ? 225 : (flags.isMini ? 33 : 27);
        else
            maxSize = flags.isFPSCounter ? 150 : (flags.isMini ? 22 : 18);

        for (int size = minSize; size <= maxSize; size++) {
            auto* sizeItem = new tsl::elm::MiniListItem(std::to_string(size) + " pt");
            sizeItem->setRadioSelector();
            if (size == currentSize)
                selectItem(lastSelectedListItem, sizeItem, ult::CHECKMARK_SYMBOL);
            sizeItem->setClickListener([this, sizeItem, size, keyName, section](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, keyName, std::to_string(size));
                    selectItem(lastSelectedListItem, sizeItem, ult::CHECKMARK_SYMBOL);
                    return true;
                }
                return false;
            });
            list->addItem(sizeItem);
        }

        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration " + ult::DIVIDER_SYMBOL + " Font Sizes", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = title; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<FontSizeConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Font Size Configuration
// =============================================================================
class FontSizeConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;

public:
    FontSizeConfig(const std::string& mode) : modeName(mode), flags(mode) {}

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Font Sizes"));

        const std::string section = modeToSection(modeName);
        const int defaultSize      = flags.isFPSCounter ? 40 : 15;
        const int default1080pSize = flags.isFPSCounter ? 68 : 21;

        auto makeItem = [&](const std::string& label, const std::string& key,
                            const std::string& type, int defSz) {
            const std::string val = ult::parseValueFromIniSection(configIniPath, section, key);
            const int sz = val.empty() ? defSz : atoi(val.c_str());
            auto* item = new tsl::elm::ListItem(label);
            item->setValue(std::to_string(sz) + " pt");
            item->setClickListener([this, type](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<FontSizeSelector>(modeName, type);
                    return true;
                }
                return false;
            });
            list->addItem(item);
        };

        makeItem("Handheld",     "handheld_font_size",     "handheld",    defaultSize);
        makeItem("Docked",       "docked_font_size",       "docked",      defaultSize);
        makeItem("1080p Docked", "docked_1080p_font_size", "docked_1080p", default1080pSize);

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        clearJump();

        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// =============================================================================
// Color Selector
// =============================================================================
class ColorSelector : public tsl::Gui {
private:
    std::string modeName;
    std::string modeTitle;
    std::string colorKey;
    std::string defaultValue;
    ModeFlags   flags;
    bool isBackgroundColor;
    bool isTextBasedColor;

public:
    ColorSelector(const std::string& mode, const std::string& title,
                  const std::string& key, const std::string& def)
        : modeName(mode), modeTitle(title), colorKey(key), defaultValue(def), flags(mode) {
        isBackgroundColor = (key == "background_color" || key == "focus_background_color" ||
                            (flags.isFPSGraph && (key == "fps_counter_color" || key == "dashed_line_color" ||
                             key == "plot_background_color")));
        isTextBasedColor  = (key == "text_color" || key == "separator_color" || key == "cat_color" ||
                            key == "cat_color_1" || key == "cat_color_2" ||
                            key == "border_color" ||
                            key == "border_wheel_color_1" || key == "border_wheel_color_2" ||
                            key == "border_wheel_color_3" || key == "border_wheel_color_3_deep" ||
                            key == "border_wheel_color_4" || key == "border_wheel_color_4_deep" ||
                            (flags.isFPSGraph && (key == "max_fps_text_color" ||
                             key == "min_fps_text_color" || key == "main_line_color" ||
                             key == "rounded_line_color" || key == "perfect_line_color")));
    }
    ~ColorSelector() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(modeTitle));

        const std::string section = modeToSection(modeName);
        std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, colorKey);
        if (currentValue.empty()) currentValue = defaultValue;

        const std::string currentColorWithoutAlpha = extractColorWithoutAlpha(currentValue);

        std::string _jumpItemValue;
        for (const auto& color : g_colorPalette) {
            auto* colorItem = new MiniColorSwatchListItem(color.first);
            const std::string hexRgb = extractColorWithoutAlpha(color.second); // #RGB
            const tsl::Color swatchColor = hexToSwatchColor(hexRgb);
            colorItem->setSwatchColor(swatchColor);
            colorItem->setRadioLabelSelector();
            colorItem->setValue(COLOR_SWATCH);

            const bool isSelected = (hexRgb == currentColorWithoutAlpha);
            if (isSelected) {
                colorItem->setValue(COLOR_SWATCH + " " + ult::CHECKMARK_SYMBOL);
                lastSelectedListItem = colorItem;
                _jumpItemValue = COLOR_SWATCH + " " + ult::CHECKMARK_SYMBOL;
            }

            colorItem->setClickListener([this, colorItem, color, section, hexRgb, swatchColor](uint64_t keys) {
                if (keys & KEY_A) {
                    std::string valueToSave = color.second;
                    if (isBackgroundColor) {
                        std::string existing = ult::parseValueFromIniSection(configIniPath, section, colorKey);
                        char alpha = (existing.length() == 5) ? existing[4]
                                   : (defaultValue.length() == 5) ? defaultValue[4]
                                   : '9';
                        valueToSave = setAlphaInColor(color.second, alpha);
                    } else if (isTextBasedColor) {
                        valueToSave = setAlphaInColor(color.second, 'F');
                    }
                    ult::setIniFileValue(configIniPath, section, colorKey, valueToSave);

                    if (lastSelectedListItem && lastSelectedListItem != colorItem) {
                        for (const auto& c : g_colorPalette) {
                            if (lastSelectedListItem->getText() == c.first) {
                                lastSelectedListItem->setValue(COLOR_SWATCH);
                                break;
                            }
                        }
                    }
                    colorItem->setValue(COLOR_SWATCH + " " + ult::CHECKMARK_SYMBOL);
                    lastSelectedListItem = colorItem;
                    return true;
                }
                return false;
            });
            list->addItem(colorItem);
        }

        list->jumpToItem("", _jumpItemValue, false);
        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration " + ult::DIVIDER_SYMBOL + " Colors", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            jumpItemName = modeTitle; jumpItemValue = ""; jumpItemExactMatch = false;
            tsl::swapTo<ColorConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Color Configuration
// =============================================================================
class ColorConfig : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;

    std::string getCurrentColor(const std::string& key, const std::string& def) const {
        const std::string section = modeToSection(modeName);
        std::string value = ult::parseValueFromIniSection(configIniPath, section, key);
        return value.empty() ? def : value;
    }

    void addColorItem(tsl::elm::List* list, const std::string& label,
                      const std::string& key, const std::string& def) {
        const std::string currentColor = getCurrentColor(key, def);
        auto* item = new ColorSwatchListItem(label);
        item->setValue(COLOR_SWATCH);
        item->setSwatchColor(hexToSwatchColor(extractColorWithoutAlpha(currentColor)));
        item->setClickListener([this, label, key, def](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<ColorSelector>(modeName, label, key, def);
                return true;
            }
            return false;
        });
        list->addItem(item);
    }

    void addColorWithAlpha(tsl::elm::List* list, const std::string& colorLabel,
                           const std::string& key, const std::string& def,
                           const std::string& alphaLabel) {
        const std::string currentColor = getCurrentColor(key, def);
        auto* colorItem = new ColorSwatchListItem(colorLabel);
        colorItem->setValue(COLOR_SWATCH);
        colorItem->setSwatchColor(hexToSwatchColor(extractColorWithoutAlpha(currentColor)));
        colorItem->setClickListener([this, colorLabel, key, def](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<ColorSelector>(modeName, colorLabel, key, def);
                return true;
            }
            return false;
        });
        list->addItem(colorItem);

        auto* alphaItem = new tsl::elm::ListItem(alphaLabel);
        alphaItem->setValue(getAlphaPercentage(currentColor));
        alphaItem->setClickListener([this, key, alphaLabel](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<AlphaSelector>(modeName, key, alphaLabel);
                return true;
            }
            return false;
        });
        list->addItem(alphaItem);
    }

    // The six Switch 2 frame-border wheel anchor colours (shared by every overlay
    // that carries the configurable border). Defaults are the muted slate palette.
    void addBorderWheelColors(tsl::elm::List* list) {
        addColorItem(list, "Border Wheel 1",      "border_wheel_color_1",      "#0C0F");
        addColorItem(list, "Border Wheel 2",      "border_wheel_color_2",      "#64FF");
        addColorItem(list, "Border Wheel 3",      "border_wheel_color_3",      "#08AF");
        addColorItem(list, "Border Wheel 3 Deep", "border_wheel_color_3_deep", "#657F");
        addColorItem(list, "Border Wheel 4",      "border_wheel_color_4",      "#A98F");
        addColorItem(list, "Border Wheel 4 Deep", "border_wheel_color_4_deep", "#C8FF");
    }

public:
    ColorConfig(const std::string& mode) : modeName(mode), flags(mode) {}

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Colors"));

        addColorWithAlpha(list, "Background Color", "background_color",       "#000A", "Background Alpha");
        if (flags.isMini || flags.isMicro || flags.isFPSCounter || flags.isFPSGraph || flags.isGameRes || flags.isFull)
            addColorWithAlpha(list, "Focus Color",  "focus_background_color", "#000F", "Focus Alpha");

        addColorItem(list, "Text Color", "text_color", "#FFFF");

        if (flags.isFPSGraph) {
            addColorItem(list, "Category Color", "cat_color", "#2DFF");

            struct FPSGraphColorSetting {
                const char* name; const char* key; const char* def; bool hasAlpha;
            };
            static const FPSGraphColorSetting fpsGraphColors[] = {
                {"FPS Counter",  "fps_counter_color",  "#2DFF", false},
                {"Graph",        "plot_background_color", "#0007", true},
                {"Border",       "border_color",        "#04AF", false},
                {"Dashed Line",  "dashed_line_color",   "#0AAF", true},
                {"Max FPS Text", "max_fps_text_color",  "#FFFF", false},
                {"Min FPS Text", "min_fps_text_color",  "#FFFF", false},
                {"Main Line",    "main_line_color",     "#0F0F", false},
                {"Rounded Line", "rounded_line_color",  "#0C0F", false},
                {"Perfect Line", "perfect_line_color",  "#A0FF", false},
            };
            for (const auto& c : fpsGraphColors) {
                if (c.hasAlpha)
                    addColorWithAlpha(list,
                        std::string(c.name) + " Color", c.key, c.def,
                        std::string(c.name) + " Alpha");
                else
                    addColorItem(list, std::string(c.name) + " Color", c.key, c.def);
            }
            // Switch 2 frame-border wheel colours (the flat "Border" colour is in
            // the table above and is reused for both the outer frame and the plot).
            addBorderWheelColors(list);

        } else if (flags.isFull) {
            addColorItem(list, "Category Color 1", "cat_color_1",    "#8FFF");
            addColorItem(list, "Category Color 2", "cat_color_2",    "#2DFF");
            addColorItem(list, "Separator Color",  "separator_color","#2DFF");

        } else if (flags.isMini || flags.isMicro) {
            addColorItem(list, "Category Color", "cat_color",       "#2DFF");
            addColorItem(list, "Separator Color", "separator_color", "#2DFF");
            if (flags.isMini) {
                addColorItem(list, "Border Color", "border_color", "#04AF");
                addBorderWheelColors(list);
            }

        } else if (flags.isFPSCounter) {
            addColorItem(list, "Border Color", "border_color", "#04AF");
            addBorderWheelColors(list);

        } else if (flags.isGameRes) {
            addColorItem(list, "Category Color", "cat_color", "#2DFF");
            addColorItem(list, "Border Color", "border_color", "#04AF");
            addBorderWheelColors(list);
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        clearJump();

        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// =============================================================================
// Show Configuration (Mini/Micro only)
// =============================================================================
class ShowConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    std::vector<std::string> elementOrder;
    std::unordered_set<std::string> enabledElements;
    // Rebuilding the UI (tsl::swapTo) must never happen synchronously from
    // inside an element's own onClick (which is what setStateChangedListener /
    // setClickListener fire from). ToggleListItem::onClick keeps touching
    // `this` after invoking the state-changed listener, so if that listener
    // destroys the element tree mid-call, the toggle item becomes a dangling
    // pointer the moment the listener returns -> crash. Instead we just flag
    // that a rebuild is needed here, and perform the actual swapTo from
    // handleInput(), which the framework only calls *after* the onClick call
    // chain has fully unwound for the frame -- the same safe pattern every
    // other Config screen in this file already uses for KEY_B.
    bool pendingRebuild = false;

public:
    ShowConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
    }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(
            "Elements " + ult::DIVIDER_SYMBOL + " \uE0E3 Move Down " + ult::DIVIDER_SYMBOL + " \uE0E2 Move Up"));

        const std::string section = isMiniMode ? "mini" : "micro";
        std::string showValue  = ult::parseValueFromIniSection(configIniPath, section, "show");
        std::string orderValue = ult::parseValueFromIniSection(configIniPath, section, "element_order");

        if (showValue.empty())
            showValue = isMiniMode ? "DTC+BAT+CPU+GPU+RAM+TMP+RES+FPS" : "FPS+CPU+GPU+RAM+TMP+BAT+DTC";
        convertToUpper(showValue);

        enabledElements.clear();
        {
            ult::StringStream ss(showValue);
            std::string item;
            while (ss.getline(item, '+'))
                if (!item.empty()) enabledElements.insert(item);
        }

        // Element order arrays: order matches the ini element_order defaults so the
        // fallback (no element_order key) produces the same sequence as the ini.
        static constexpr std::string_view miniElements[]  = {"DTC","BAT","CPU","GPU","RAM","MEM","SOC","TMP","READ","RES","FPS"};
        static constexpr std::string_view microElements[] = {"FPS","RES","CPU","GPU","RAM","READ","SOC","TMP","BAT","DTC"};
        const auto* allElements    = isMiniMode ? miniElements : microElements;
        const size_t allElementsSize = isMiniMode ? std::size(miniElements) : std::size(microElements);

        elementOrder.clear();
        if (!orderValue.empty()) {
            convertToUpper(orderValue);
            ult::StringStream orderSS(orderValue);
            std::string orderItem;
            while (orderSS.getline(orderItem, '+'))
                if (!orderItem.empty()) elementOrder.push_back(orderItem);
        } else {
            for (size_t i = 0; i < allElementsSize; i++) {
                auto elem = allElements[i];
                if (!isMiniMode && elem == "MEM") continue;
                elementOrder.emplace_back(elem);
            }
        }

        static const std::unordered_set<std::string> dynamicElements = {"FPS", "RES", "READ"};

        for (size_t i = 0; i < elementOrder.size(); i++) {
            const std::string& element = elementOrder[i];
            const bool isEnabled = enabledElements.count(element) > 0;

            auto* elementItem = new tsl::elm::MiniToggleListItem(element, isEnabled);
            elementItem->enableShortHoldKey();
            elementItem->enableLongHoldKey();

            // KEY_A: ToggleListItem handles the visual flip; we guard the last
            // always-showing element and persist the change.
            elementItem->setStateChangedListener([this, elementItem, element](bool newState) {
                if (!newState) {
                    // Turning OFF — block if this would leave no always-showing elements
                    if (dynamicElements.count(element) == 0) {
                        int alwaysOnAfter = 0;
                        for (const auto& e : enabledElements) {
                            if (e != element && dynamicElements.count(e) == 0)
                                alwaysOnAfter++;
                        }
                        if (alwaysOnAfter == 0) {
                            // Revert the visual state and bail
                            elementItem->setState(true);
                            return;
                        }
                    }
                    enabledElements.erase(element);
                } else {
                    enabledElements.insert(element);
                }
                updateShowAndOrder();
                jumpItemName = element; jumpItemValue = ""; jumpItemExactMatch = true;
                // Don't swapTo here -- this listener is invoked from inside
                // ToggleListItem::onClick, which still touches `this` (the
                // toggle item being destroyed by the swap) after we return.
                // Defer to handleInput() instead.
                pendingRebuild = true;
            });

            // KEY_X / KEY_Y: reorder (ToggleListItem passes non-KEY_A keys through
            // to the click listener after handling its own KEY_A toggle).
            elementItem->setClickListener([this, element](uint64_t keys) {
                if (keys & KEY_Y || keys & KEY_X) {
                    size_t currentPos = 0;
                    for (size_t j = 0; j < elementOrder.size(); j++) {
                        if (elementOrder[j] == element) { currentPos = j; break; }
                    }
                    if (keys & KEY_X) {
                        if (currentPos > 0) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos - 1]);
                        } else {
                            const std::string temp = elementOrder[0];
                            for (size_t j = 0; j < elementOrder.size() - 1; j++)
                                elementOrder[j] = elementOrder[j + 1];
                            elementOrder.back() = temp;
                        }
                        triggerMoveFeedback();
                    } else if (keys & KEY_Y) {
                        if (currentPos < elementOrder.size() - 1) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos + 1]);
                        } else {
                            const std::string temp = elementOrder.back();
                            for (size_t j = elementOrder.size() - 1; j > 0; j--)
                                elementOrder[j] = elementOrder[j - 1];
                            elementOrder[0] = temp;
                        }
                        triggerMoveFeedback();
                    }
                    updateShowAndOrder();
                    jumpItemName = element; jumpItemValue = ""; jumpItemExactMatch = true;
                    // Same reasoning as the toggle listener above: defer the
                    // rebuild to handleInput() rather than swapping here.
                    pendingRebuild = true;
                    return true;
                }
                return false;
            });
            list->addItem(elementItem);
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        clearJump();

        return makeFrame(modeName + " " + ult::DIVIDER_SYMBOL + " Configuration", list);
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        // Performed here (not inside the toggle/click listeners) so the swap
        // only happens once the element's own onClick call has fully returned
        // -- see the pendingRebuild comment above for why this matters.
        if (pendingRebuild) {
            pendingRebuild = false;
            tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
            return true;
        }
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            tsl::goBack();
            return true;
        }
        return false;
    }

private:
    void updateShowAndOrder() {
        std::string newShowValue, newOrderValue;
        bool showFirst = true, orderFirst = true;
        for (const std::string& element : elementOrder) {
            if (!orderFirst) newOrderValue += "+";
            newOrderValue += element;
            orderFirst = false;
            if (enabledElements.count(element)) {
                if (!showFirst) newShowValue += "+";
                newShowValue += element;
                showFirst = false;
            }
        }
        const std::string section = isMiniMode ? "mini" : "micro";
        ult::setIniFileValue(configIniPath, section, "show",          newShowValue);
        ult::setIniFileValue(configIniPath, section, "element_order", newOrderValue);
    }
};

// =============================================================================
// Main Configurator Overlay
// =============================================================================
class ConfiguratorOverlay : public tsl::Gui {
private:
    std::string modeName;
    ModeFlags   flags;

    int getCurrentRefreshRate() const {
        const std::string section = modeToSection(modeName);
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        const int defaultRate = flags.isFPSGraph ? 30 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 30 : (flags.isFull ? 2 : 3)));
        return value.empty() ? defaultRate : atoi(value.c_str());
    }

    int getCurrentSampleRate() const {
        const std::string section = modeToSection(modeName);
        const std::string rrVal = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        const int defaultRate       = flags.isFPSGraph ? 30 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 30 : (flags.isFull ? 2 : 3)));
        const int defaultSampleRate = (flags.isFPSGraph ? 3 : (flags.isFPSCounter ? 30 : ((flags.isMini || flags.isMicro || flags.isGameRes) ? 2 : defaultRate)));
        const int maxRate = rrVal.empty() ? defaultRate : std::clamp(atoi(rrVal.c_str()), 1, 60);
        const std::string srVal = ult::parseValueFromIniSection(configIniPath, section, "sample_rate");
        return srVal.empty() ? std::min(defaultSampleRate, maxRate) : std::clamp(atoi(srVal.c_str()), 1, maxRate);
    }

    int getCurrentFramePadding() const {
        const std::string section = modeToSection(modeName);
        if (section.empty()) return 0;
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "frame_padding");
        return value.empty() ? 0 : atoi(value.c_str());
    }

    int getCurrentMicroHPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "horizontal_padding");
        return value.empty() ? 14 : std::clamp(atoi(value.c_str()), 2, 30);
    }

    int getCurrentMicroVPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "vertical_padding");
        return value.empty() ? 8 : std::clamp(atoi(value.c_str()), 2, 30);
    }

    int getCurrentMicroStackedSpacing() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "stacked_spacing");
        return value.empty() ? 4 : std::clamp(atoi(value.c_str()), 0, 30);
    }

    int getCurrentMicroLabelPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "label_padding");
        return value.empty() ? 14 : std::clamp(atoi(value.c_str()), 2, 30);
    }

    // Element padding is whole spaces (stored as tenths: 10..100). Snap to the
    // nearest whole space so the menu value matches what the engine applies.
    int getCurrentMicroElementPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "element_padding");
        const int ep = value.empty() ? 50 : std::clamp(atoi(value.c_str()), 10, 100);
        return std::clamp(((ep + 5) / 10) * 10, 10, 100);
    }

    // Mini space-unit paddings (tenths of a space). Defaults mirror MiniSettings.
    int getCurrentMiniHPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", "horizontal_padding");
        return value.empty() ? 34 : std::clamp(atoi(value.c_str()), 2, 60);
    }

    int getCurrentMiniVPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", "vertical_padding");
        return value.empty() ? 34 : std::clamp(atoi(value.c_str()), 2, 60);
    }

    int getCurrentMiniSpacing() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", "spacing");
        return value.empty() ? 15 : std::clamp(atoi(value.c_str()), 2, 30);
    }

    int getCurrentMiniStackedSpacing() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", "stacked_spacing");
        return value.empty() ? 4 : std::clamp(atoi(value.c_str()), 0, 30);
    }

    int getCurrentMiniCornerRadius() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "mini", "corner_radius");
        return value.empty() ? 36 : std::clamp(atoi(value.c_str()), 0, 80);
    }

    std::string getDTCFormatName(const std::string& formatStr) const {
        if (formatStr == ult::OPTION_SYMBOL) return ult::OPTION_SYMBOL;
        for (const auto& format : getDTCFormatsFlat())
            if (format.second == formatStr) return format.first;
        return formatStr;
    }

    std::string getCurrentDTCFormatLabel(int slot) const {
        if (!(flags.isMini || flags.isMicro)) return "";
        const std::string section = modeToSection(modeName);
        const std::string iniKey  = (slot == 2) ? "dtc_format_2" : "dtc_format_1";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, iniKey);
        if (value.empty()) {
            const std::string legacy = ult::parseValueFromIniSection(configIniPath, section, "dtc_format");
            if (!legacy.empty()) {
                const size_t divPos = legacy.find(ult::DIVIDER_SYMBOL);
                if (divPos != std::string::npos)
                    value = (slot == 1) ? legacy.substr(0, divPos)
                                        : legacy.substr(divPos + ult::DIVIDER_SYMBOL.length());
                else
                    value = (slot == 1) ? legacy : ult::OPTION_SYMBOL;
            } else {
                if (slot == 1) value = std::string("%a, %b %d");
                else           value = std::string("%l:%M:%S %p");
            }
        }
        return getDTCFormatName(value);
    }

    std::string getCurrentTextAlign() const {
        std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "text_align");
        convertToUpper(value);
        if (value == "LEFT")  return "Left";
        if (value == "RIGHT") return "Right";
        return "Center";
    }

    std::string getCurrentLayerPosRight() const {
        const std::string section = modeToSection(modeName);
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "layer_width_align");
        convertToUpper(value);
        if (value == "RIGHT")  return "Right";
        if (!flags.isFull && value == "CENTER") return "Center";
        return "Left";
    }

    std::string getCurrentLayerPosBottom() const {
        const std::string section = modeToSection(modeName);
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "layer_height_align");
        convertToUpper(value);
        if (value == "BOTTOM") return "Bottom";
        if (!flags.isMicro && value == "CENTER") return "Center";
        return "Top";
    }

    std::string cycleTextAlign() {
        const std::string current = getCurrentTextAlign();
        const std::string next = (current == "Left") ? "Center" : (current == "Center") ? "Right" : "Left";
        ult::setIniFileValue(configIniPath, "micro", "text_align", next);
        return next;
    }

    std::string cycleLayerPosRight() {
        const std::string section = modeToSection(modeName);
        const std::string current = getCurrentLayerPosRight();
        std::string next;
        if (flags.isFull)
            next = (current == "Left") ? "Right" : "Left";
        else
            next = (current == "Left") ? "Center" : (current == "Center") ? "Right" : "Left";
        const std::string value = (next == "Right") ? "right" : (next == "Center" ? "center" : "left");
        ult::setIniFileValue(configIniPath, section, "layer_width_align", value);
        return next;
    }

    std::string cycleLayerPosBottom() {
        const std::string section = modeToSection(modeName);
        const std::string current = getCurrentLayerPosBottom();
        std::string next;
        if (flags.isMicro)
            next = (current == "Top") ? "Bottom" : "Top";
        else
            next = (current == "Top") ? "Center" : (current == "Center") ? "Bottom" : "Top";
        const std::string value = (next == "Bottom") ? "bottom" : (next == "Center" ? "center" : "top");
        ult::setIniFileValue(configIniPath, section, "layer_height_align", value);
        return next;
    }

public:
    ConfiguratorOverlay(const std::string& mode) : modeName(mode), flags(mode) {}

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Configuration"));

        // Elements (Mini/Micro only)
        if (flags.isMini || flags.isMicro) {
            auto* showSettings = new tsl::elm::ListItem("Elements");
            showSettings->setValue(ult::DROPDOWN_SYMBOL);
            showSettings->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<ShowConfig>(modeName); return true; }
                return false;
            });
            list->addItem(showSettings);
        }

        // Toggles (all modes)
        {
            auto* toggles = new tsl::elm::ListItem("Toggles");
            toggles->setValue(ult::DROPDOWN_SYMBOL);
            toggles->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<TogglesConfig>(modeName); return true; }
                return false;
            });
            list->addItem(toggles);
        }

        // Colors (all modes)
        {
            auto* colors = new tsl::elm::ListItem("Colors");
            colors->setValue(ult::DROPDOWN_SYMBOL);
            colors->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<ColorConfig>(modeName); return true; }
                return false;
            });
            list->addItem(colors);
        }

        // Font Sizes (Mini/Micro/FPS Counter only)
        if (flags.isMini || flags.isMicro || flags.isFPSCounter) {
            auto* fontSizes = new tsl::elm::ListItem("Font Sizes");
            fontSizes->setValue(ult::DROPDOWN_SYMBOL);
            fontSizes->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<FontSizeConfig>(modeName); return true; }
                return false;
            });
            list->addItem(fontSizes);
        }

        // Paddings — after Font Sizes (or after Colors for modes without Font Sizes),
        // before Sample Rate / Refresh Rate.
        if (flags.isMini || flags.isGameRes || flags.isFPSCounter || flags.isFPSGraph || flags.isMicro) {
            auto* paddings = new tsl::elm::ListItem("Paddings");
            paddings->setValue(ult::DROPDOWN_SYMBOL);
            paddings->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<PaddingsConfig>(modeName); return true; }
                return false;
            });
            list->addItem(paddings);
        }

        // Sample Rate (Mini / Micro / FPS Counter / FPS Graph / Game Resolutions / Full) — above Refresh Rate
        if (flags.isMini || flags.isMicro || flags.isFPSCounter || flags.isFPSGraph || flags.isGameRes || flags.isFull) {
            auto* sampleRate = new tsl::elm::ListItem("Sample Rate");
            sampleRate->setValue(std::to_string(getCurrentSampleRate()) + " Hz");
            sampleRate->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<SampleRateConfig>(modeName); return true; }
                return false;
            });
            list->addItem(sampleRate);
        }

        // Refresh Rate (all modes)
        {
            auto* refreshRate = new tsl::elm::ListItem("Refresh Rate");
            refreshRate->setValue(std::to_string(getCurrentRefreshRate()) + " Hz");
            refreshRate->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<RefreshRateConfig>(modeName); return true; }
                return false;
            });
            list->addItem(refreshRate);
        }

        // DTC Format (Mini/Micro only)
        if (flags.isMini || flags.isMicro) {
            auto* dtcFormat1 = new tsl::elm::ListItem("DTC Format 1");
            dtcFormat1->setValue(getCurrentDTCFormatLabel(1));
            dtcFormat1->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<DTCFormatConfig>(modeName, 1); return true; }
                return false;
            });
            list->addItem(dtcFormat1);

            auto* dtcFormat2 = new tsl::elm::ListItem("DTC Format 2");
            dtcFormat2->setValue(getCurrentDTCFormatLabel(2));
            dtcFormat2->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<DTCFormatConfig>(modeName, 2); return true; }
                return false;
            });
            list->addItem(dtcFormat2);
        }

        // Mode-specific positioning (kept in main menu — these are positional, not padding)
        if (flags.isMicro) {
            auto* textAlign = new tsl::elm::ListItem("Text Alignment");
            textAlign->setValue(getCurrentTextAlign());
            textAlign->setClickListener([this, textAlign](uint64_t keys) {
                if (keys & KEY_A) { textAlign->setValue(cycleTextAlign()); return true; }
                return false;
            });
            list->addItem(textAlign);

            auto* layerPos = new tsl::elm::ListItem("Vertical Position");
            layerPos->setValue(getCurrentLayerPosBottom());
            layerPos->setClickListener([this, layerPos](uint64_t keys) {
                if (keys & KEY_A) { layerPos->setValue(cycleLayerPosBottom()); return true; }
                return false;
            });
            list->addItem(layerPos);

        } else if (flags.isFull) {
            auto* layerPos = new tsl::elm::ListItem("Horizontal Position");
            layerPos->setValue(getCurrentLayerPosRight());
            layerPos->setClickListener([this, layerPos](uint64_t keys) {
                if (keys & KEY_A) { layerPos->setValue(cycleLayerPosRight()); return true; }
                return false;
            });
            list->addItem(layerPos);
        }

        // Mode Combo
        {
            const int slotIdx = modeComboIndexFor(modeName);
            if (slotIdx >= 0) {
                std::string comboDisplay = readModeCombo(slotIdx);
                if (comboDisplay.empty()) comboDisplay = ult::OPTION_SYMBOL;
                else ult::convertComboToUnicode(comboDisplay);

                auto* modeCombo = new tsl::elm::ListItem("Mode Combo", comboDisplay);
                modeCombo->setClickListener([this](uint64_t keys) {
                    if (keys & KEY_A) { tsl::changeTo<ModeComboConfig>(modeName); return true; }
                    return false;
                });
                list->addItem(modeCombo);
            }
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch.load(std::memory_order_acquire));
        clearJump();

        auto* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", localizeSubtitle(modeName));
        rootFrame->setContent(list);
        return rootFrame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos,
                             HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            triggerExitFeedback();
            lastSelectedItem = modeName;
            tsl::swapTo<MainMenu>();
            return true;
        }
        return false;
    }
};