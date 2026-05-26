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
class FontSizeConfig;
class FontSizeSelector;
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
        {"#000","Black"},      {"#222","Charcoal"},    {"#444","Dark Gray"},
        {"#666","Gray"},       {"#999","Light Gray"},   {"#CCC","Silver"},
        {"#EEE","Off-White"},  {"#FFF","White"},

        {"#200","Dark Red"},   {"#700","Maroon"},       {"#B22","Crimson"},
        {"#F00","Red"},        {"#F66","Light Red"},    {"#FAA","Salmon"},

        {"#520","Dark Orange"},{"#A40","Burnt Orange"}, {"#F80","Orange"},
        {"#FB6","Light Orange"},{"#FC8","Peach"},

        {"#220","Dark Yellow"},{"#CA0","Gold"},         {"#FF0","Yellow"},
        {"#FF6","Light Yellow"},{"#FFC","Cream"},

        {"#020","Dark Green"}, {"#063","Forest Green"}, {"#080","Green"},
        {"#0C0","Lime Green"}, {"#0F0","Bright Green"}, {"#8F8","Light Green"},
        {"#CFA","Mint"},

        {"#022","Dark Teal"},  {"#066","Teal"},         {"#0AA","Aqua"},
        {"#0FF","Cyan"},       {"#8FF","Light Cyan"},

        {"#002","Midnight Blue"},{"#003","Dark Blue"},  {"#04A","Navy"},
        {"#06F","Royal Blue"}, {"#00F","Blue"},         {"#2DF","Light Blue"},
        {"#8CF","Sky Blue"},   {"#ACE","Powder Blue"},

        {"#202","Dark Purple"},{"#404","Eggplant"},     {"#608","Indigo"},
        {"#808","Purple"},     {"#A0F","Violet"},       {"#C8F","Lavender"},
        {"#D9F","Light Lavender"},

        {"#606","Dark Magenta"},{"#F0F","Magenta"},     {"#F4A","Hot Pink"},
        {"#F8A","Pink"},       {"#FCE","Light Pink"},

        {"#321","Dark Brown"}, {"#642","Brown"},        {"#A75","Light Brown"},
        {"#DB8","Tan"},
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
inline tsl::elm::Element* makeFrame(const std::string& subtitle, tsl::elm::Element* content) {
    auto* f = new tsl::elm::OverlayFrame("Status Monitor", subtitle);
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
        const std::string full = this->m_value;

        // x = getX() + getWidth() - textWidth(full,20) - 19
        // (equivalent to drawValue's getX() + m_maxWidth + 47 with full value)
        const s32 fullValueWidth = renderer->getTextDimensions(full, false, 20).first;
        const s32 swatchX = this->getX() + this->getWidth() - fullValueWidth - 19;

        // Strip the swatch so the parent never renders it.
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

        // Draw the swatch once in m_swatchColor at the correct position.
        static constexpr s32 fontSize   = 20;
        static constexpr u16 itemHeight = isMini ? tsl::style::MiniListItemDefaultHeight
                                                 : tsl::style::ListItemDefaultHeight;
        static constexpr s32 yOffset    = (tsl::style::ListItemDefaultHeight - itemHeight) / 2 + 1;
        const s32 swatchY = this->getY() + 45 - yOffset - 1;
        renderer->drawStringWithColoredSections(
            COLOR_SWATCH, false, COLOR_SWATCH_SPECIAL,
            swatchX, swatchY, fontSize, tsl::Color(0, 0, 0, 0), m_swatchColor);
    }

private:
    tsl::Color m_swatchColor;
};

// ColorConfig navigation rows (full height); ColorSelector palette rows (mini height).
using ColorSwatchListItem     = ColorSwatchListItemT<false>;
using MiniColorSwatchListItem = ColorSwatchListItemT<true>;

// Shared color palette used by ColorSelector.
static const std::vector<std::pair<std::string, std::string>> g_colorPalette = {
    {"Black","#000"},        {"Charcoal","#222"},     {"Dark Gray","#444"},
    {"Gray","#666"},         {"Light Gray","#999"},   {"Silver","#CCC"},
    {"Off-White","#EEE"},    {"White","#FFF"},

    {"Dark Red","#200"},     {"Maroon","#700"},       {"Crimson","#B22"},
    {"Red","#F00"},          {"Light Red","#F66"},    {"Salmon","#FAA"},

    {"Dark Orange","#520"},  {"Burnt Orange","#A40"}, {"Orange","#F80"},
    {"Light Orange","#FB6"}, {"Peach","#FC8"},

    {"Dark Yellow","#220"},  {"Gold","#CA0"},         {"Yellow","#FF0"},
    {"Light Yellow","#FF6"}, {"Cream","#FFC"},

    {"Dark Green","#020"},   {"Forest Green","#063"}, {"Green","#080"},
    {"Lime Green","#0C0"},   {"Bright Green","#0F0"}, {"Light Green","#8F8"},
    {"Mint","#CFA"},

    {"Dark Teal","#022"},    {"Teal","#066"},         {"Aqua","#0AA"},
    {"Cyan","#0FF"},         {"Light Cyan","#8FF"},

    {"Midnight Blue","#002"},{"Dark Blue","#003"},    {"Navy","#04A"},
    {"Royal Blue","#06F"},   {"Blue","#00F"},         {"Light Blue","#2DF"},
    {"Sky Blue","#8CF"},     {"Powder Blue","#ACE"},

    {"Dark Purple","#202"},  {"Eggplant","#404"},     {"Indigo","#608"},
    {"Purple","#808"},       {"Violet","#A0F"},       {"Lavender","#C8F"},
    {"Light Lavender","#D9F"},

    {"Dark Magenta","#606"}, {"Magenta","#F0F"},      {"Hot Pink","#F4A"},
    {"Pink","#F8A"},         {"Light Pink","#FCE"},

    {"Dark Brown","#321"},   {"Brown","#642"},        {"Light Brown","#A75"},
    {"Tan","#DB8"},
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
        if (currentColor.empty()) currentColor = "#0009";
        const std::string currentAlpha = extractAlphaFromColor(currentColor);

        static const std::pair<std::string, char> alphaOptions[16] = {
            {"0%",'0'},   {"7%",'1'},   {"13%",'2'},  {"20%",'3'},
            {"27%",'4'},  {"33%",'5'},  {"40%",'6'},  {"47%",'7'},
            {"53%",'8'},  {"60%",'9'},  {"67%",'A'},  {"73%",'B'},
            {"80%",'C'},  {"87%",'D'},  {"93%",'E'},  {"100%",'F'}
        };

        for (const auto& option : alphaOptions) {
            auto* alphaItem = new tsl::elm::MiniListItem(option.first);
            if (!currentAlpha.empty() && currentAlpha[0] == option.second)
                selectItem(lastSelectedListItem, alphaItem, ult::CHECKMARK_SYMBOL);
            alphaItem->setClickListener([this, alphaItem, option, section](uint64_t keys) {
                if (keys & KEY_A) {
                    std::string color = ult::parseValueFromIniSection(configIniPath, section, colorKey);
                    if (color.empty()) color = "#0009";
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
        const bool isMiniMode     = (modeName == "Mini");

        std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, iniKey);
        if (currentValue.empty()) {
            if (slot == 1) currentValue = std::string("%a, %b %d");
            else           currentValue = std::string("%l:%M:%S %p");
        }

        if (slot == 2) {
            list->addItem(new tsl::elm::CategoryHeader("None"));
            auto* noneItem = new tsl::elm::MiniListItem(ult::OPTION_SYMBOL);
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
            addToggle(list, "Dynamic Temp Colors", "use_dynamic_colors",  true);
            addToggle(list, "Integer FPS",         "integer_fps",         true);

        } else if (flags.isFull) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Real Frequencies",    "show_real_freqs",     true);
            addToggle(list, "Target Frequencies",  "show_target_freqs",   true);
            addToggle(list, "Frequency Deltas",    "show_deltas",         true);
            addToggle(list, "FPS",                 "show_fps",            true);
            addToggle(list, "RES",                 "show_res",            true);
            addToggle(list, "Read Speed",          "show_read_speed",     true);
            addToggle(list, "Dynamic Temp Colors", "use_dynamic_colors",  true);

        } else if (flags.isMini || flags.isMicro) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "1080p Docked",   "use_1080p_docked",   true);
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);

            if (flags.isMini)
                addToggle(list, "Labels", "show_labels", true);

            addToggle(list, "Real Frequencies",    "real_freqs",          true);
            addToggle(list, "Real Voltages",       "real_volts",          true);
            addToggle(list, "Dynamic Temp Colors", "use_dynamic_colors",  true);

            list->addItem(new tsl::elm::CategoryHeader("CPU"));
            addToggle(list, "Full CPU",              "show_full_cpu",              true);
            addToggle(list, "Full CPU Max Core 0-2", "show_full_cpu_max_core_012", flags.isMini);
            addToggle(list, "Stacked Full CPU",      "show_stacked_full_cpu",      flags.isMicro);
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

            list->addItem(new tsl::elm::CategoryHeader("BAT"));
            addToggle(list, "Invert Battery Display", "invert_battery_display", true);
            addToggle(list, "Stacked", "show_stacked_bat", flags.isMicro);

            list->addItem(new tsl::elm::CategoryHeader("DTC"));
            addToggle(list, "Use DTC Symbol", "use_dtc_symbol",   true);
            addToggle(list, "Stacked",        "show_stacked_dtc", flags.isMicro);

        } else if (flags.isGameRes) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);

        } else if (flags.isFPSCounter) {
            list->addItem(new tsl::elm::CategoryHeader("Global"));
            addToggle(list, "1080p Docked",   "use_1080p_docked",   true);
            addToggle(list, "Disable Screenshots", "disable_screenshots", false);
            addToggle(list, "Integer FPS",         "integer_fps",         true);
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
        const int defaultRate = (flags.isFPSCounter || flags.isFPSGraph) ? 5 : 3;
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
        currentPadding = value.empty() ? 10 : std::clamp(atoi(value.c_str()), 0, 14);
    }
    ~FramePaddingConfig() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Frame Padding"));

        for (int padding = 0; padding <= 14; ++padding) {
            auto* paddingItem = new tsl::elm::MiniListItem(std::to_string(padding) + " px");
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
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// =============================================================================
// Micro Padding Configs (Horizontal / Vertical / Label)
// =============================================================================

// Shared implementation for all three Micro padding screens.
template<int DEFAULT, int MIN_P, int MAX_P>
class MicroPaddingConfigBase : public tsl::Gui {
protected:
    int         currentPadding;
    std::string iniKey;
    std::string headerLabel;
    std::string jumpLabel;

    virtual std::string formatLabel(int p) const { return std::to_string(p) + " px"; }

public:
    MicroPaddingConfigBase(const std::string& key, const std::string& header, const std::string& jump)
        : iniKey(key), headerLabel(header), jumpLabel(jump) {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", iniKey);
        currentPadding = value.empty() ? DEFAULT : std::clamp(atoi(value.c_str()), MIN_P, MAX_P);
    }
    ~MicroPaddingConfigBase() { lastSelectedListItem = nullptr; }

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(headerLabel));
        for (int p = MIN_P; p <= MAX_P; ++p) {
            auto* item = new tsl::elm::MiniListItem(formatLabel(p));
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
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), "Micro");
            return true;
        }
        return false;
    }
};

class MicroHPaddingConfig : public MicroPaddingConfigBase<8, 0, 20> {
public:
    MicroHPaddingConfig() : MicroPaddingConfigBase("horizontal_padding", "Horizontal Padding", "Horizontal Padding") {}
};

class MicroVPaddingConfig : public MicroPaddingConfigBase<2, 0, 20> {
public:
    MicroVPaddingConfig() : MicroPaddingConfigBase("vertical_padding", "Vertical Padding", "Vertical Padding") {}
};

class MicroLabelPaddingConfig : public MicroPaddingConfigBase<0, 4, 12> {
    int autoDefault;
protected:
    std::string formatLabel(int p) const override {
        std::string s = std::to_string(p) + " px";
        if (p == autoDefault) s += " (default)";
        return s;
    }
public:
    MicroLabelPaddingConfig() : MicroPaddingConfigBase("label_padding", "Label Padding", "Label Padding") {
        const std::string fsVal = ult::parseValueFromIniSection(configIniPath, "micro", "handheld_font_size");
        const int fs = fsVal.empty() ? 15 : atoi(fsVal.c_str());
        autoDefault = (fs <= 16) ? 6 : (fs <= 20 ? 8 : 10);
        // currentPadding==0 means "auto"; snap display to autoDefault
        if (currentPadding == 0) currentPadding = autoDefault;
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
            defaultSize = flags.isFPSCounter ? 60 : 22;
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
        const int default1080pSize = flags.isFPSCounter ? 60 : 22;

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
                            (flags.isFPSGraph && (key == "fps_counter_color" || key == "dashed_line_color")));
        isTextBasedColor  = (key == "text_color" || key == "separator_color" || key == "cat_color" ||
                            key == "cat_color_1" || key == "cat_color_2" ||
                            (flags.isFPSGraph && (key == "border_color" || key == "max_fps_text_color" ||
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

public:
    ColorConfig(const std::string& mode) : modeName(mode), flags(mode) {}

    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Colors"));

        if (!flags.isFull) {
            addColorWithAlpha(list, "Background Color", "background_color",       "#0009", "Background Alpha");
            if (flags.isMini || flags.isMicro || flags.isFPSCounter || flags.isFPSGraph || flags.isGameRes)
                addColorWithAlpha(list, "Focus Color",  "focus_background_color", "#000F", "Focus Alpha");
        } else {
            addColorWithAlpha(list, "Background Color", "background_color",       "#0009", "Background Alpha");
        }

        addColorItem(list, "Text Color", "text_color", "#FFFF");

        if (flags.isFPSGraph) {
            addColorItem(list, "Category Color", "cat_color", "#0F0F");

            struct FPSGraphColorSetting {
                const char* name; const char* key; const char* def; bool hasAlpha;
            };
            static const FPSGraphColorSetting fpsGraphColors[] = {
                {"FPS Counter",  "fps_counter_color",  "#888C", true},
                {"Border",       "border_color",        "#2DFF", false},
                {"Dashed Line",  "dashed_line_color",   "#8888", true},
                {"Max FPS Text", "max_fps_text_color",  "#FFFF", false},
                {"Min FPS Text", "min_fps_text_color",  "#FFFF", false},
                {"Main Line",    "main_line_color",     "#FFFF", false},
                {"Rounded Line", "rounded_line_color",  "#F0FF", false},
                {"Perfect Line", "perfect_line_color",  "#0C0F", false},
            };
            for (const auto& c : fpsGraphColors) {
                if (c.hasAlpha)
                    addColorWithAlpha(list,
                        std::string(c.name) + " Color", c.key, c.def,
                        std::string(c.name) + " Alpha");
                else
                    addColorItem(list, std::string(c.name) + " Color", c.key, c.def);
            }

        } else if (flags.isFull) {
            addColorItem(list, "Category Color 1", "cat_color_1",    "#8FFF");
            addColorItem(list, "Category Color 2", "cat_color_2",    "#2DFF");
            addColorItem(list, "Separator Color",  "separator_color","#888F");

        } else if (flags.isMini || flags.isMicro) {
            addColorItem(list, "Category Color", "cat_color",       "#2DFF");
            addColorItem(list, "Separator Color", "separator_color", "#888F");

        } else if (flags.isGameRes) {
            addColorItem(list, "Category Color", "cat_color", "#2DFF");
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
            showValue = isMiniMode ? "DTC+BAT+CPU+GPU+RAM+TMP+FPS+RES" : "FPS+CPU+GPU+RAM+TMP+BAT+DTC";
        convertToUpper(showValue);

        enabledElements.clear();
        {
            ult::StringStream ss(showValue);
            std::string item;
            while (ss.getline(item, '+'))
                if (!item.empty()) enabledElements.insert(item);
        }

        static constexpr std::string_view miniElements[]  = {"DTC","BAT","CPU","GPU","RAM","MEM","READ","SOC","TMP","FPS","RES"};
        static constexpr std::string_view microElements[] = {"FPS","CPU","GPU","RAM","READ","SOC","TMP","RES","BAT","DTC"};
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

        for (size_t i = 0; i < elementOrder.size(); i++) {
            const std::string& element = elementOrder[i];
            const bool isEnabled = enabledElements.count(element) > 0;

            auto* elementItem = new tsl::elm::MiniListItem(element);
            elementItem->enableShortHoldKey();
            elementItem->enableLongHoldKey();
            elementItem->setValue(isEnabled ? ult::ON : ult::OFF, !isEnabled);

            elementItem->setClickListener([this, elementItem, element](uint64_t keys) {
                static bool hasNotTriggeredAnimation = false;
                if (hasNotTriggeredAnimation) {
                    elementItem->triggerClickAnimation();
                    hasNotTriggeredAnimation = false;
                }
                if (keys & KEY_A) {
                    // Dynamic elements (FPS, RES, READ) only render when a game is running,
                    // so they don't count as "always visible". Block turning off an always-showing
                    // element if it would leave no always-showing elements enabled.
                    static const std::unordered_set<std::string> dynamicElements = {"FPS", "RES", "READ"};
                    if (enabledElements.count(element)) {
                        // Turning OFF — guard against leaving zero always-showing elements
                        if (dynamicElements.count(element) == 0) {
                            // Count always-showing elements that would remain enabled after removal
                            int alwaysOnAfter = 0;
                            for (const auto& e : enabledElements) {
                                if (e != element && dynamicElements.count(e) == 0)
                                    alwaysOnAfter++;
                            }
                            if (alwaysOnAfter == 0) return true; // Block: last always-showing element
                        }
                        enabledElements.erase(element);
                    } else {
                        enabledElements.insert(element);
                    }
                    updateShowAndOrder();
                    jumpItemName = element; jumpItemValue = ""; jumpItemExactMatch = true;
                    hasNotTriggeredAnimation = true;
                    tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
                    return true;
                }
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
                    tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
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
        const int defaultRate = (flags.isFPSCounter || flags.isFPSGraph) ? 5 : 3;
        return value.empty() ? defaultRate : atoi(value.c_str());
    }

    int getCurrentFramePadding() const {
        const std::string section = modeToSection(modeName);
        if (section.empty()) return 10;
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "frame_padding");
        return value.empty() ? 10 : atoi(value.c_str());
    }

    int getCurrentMicroHPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "horizontal_padding");
        return value.empty() ? 8 : std::clamp(atoi(value.c_str()), 0, 20);
    }

    int getCurrentMicroVPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "vertical_padding");
        return value.empty() ? 2 : std::clamp(atoi(value.c_str()), 0, 20);
    }

    int getCurrentMicroLabelPadding() const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "label_padding");
        if (value.empty()) {
            const std::string fsVal = ult::parseValueFromIniSection(configIniPath, "micro", "handheld_font_size");
            const int fs = fsVal.empty() ? 15 : atoi(fsVal.c_str());
            return (fs <= 16) ? 6 : 8;
        }
        return std::clamp(atoi(value.c_str()), 4, 12);
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

        // Frame Padding (Mini/Game Resolutions/FPS Counter/FPS Graph)
        if (flags.isMini || flags.isGameRes || flags.isFPSCounter || flags.isFPSGraph) {
            auto* framePadding = new tsl::elm::ListItem("Frame Padding");
            framePadding->setValue(std::to_string(getCurrentFramePadding()) + " px");
            framePadding->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<FramePaddingConfig>(modeName); return true; }
                return false;
            });
            list->addItem(framePadding);
        }

        // Mode-specific positioning
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

            auto* hPadding = new tsl::elm::ListItem("Horizontal Padding");
            hPadding->setValue(std::to_string(getCurrentMicroHPadding()) + " px");
            hPadding->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroHPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(hPadding);

            auto* vPadding = new tsl::elm::ListItem("Vertical Padding");
            vPadding->setValue(std::to_string(getCurrentMicroVPadding()) + " px");
            vPadding->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroVPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(vPadding);

            auto* lPadding = new tsl::elm::ListItem("Label Padding");
            lPadding->setValue(std::to_string(getCurrentMicroLabelPadding()) + " px");
            lPadding->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) { tsl::changeTo<MicroLabelPaddingConfig>(); return true; }
                return false;
            });
            list->addItem(lPadding);

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

        auto* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", modeName);
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
