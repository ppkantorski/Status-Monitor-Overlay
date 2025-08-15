/*
 * Mode-Specific Configuration Settings
 * 
 * Based on actual settings structures, each mode only shows applicable settings:
 * 
 * Mini Mode: Refresh Rate, Colors (background, focus_background, separator, category, text), 
 *           Toggles, Font Sizes, Elements
 * 
 * Micro Mode: Refresh Rate, Colors (background, separator, category, text), Toggles, 
 *            Font Sizes, Elements, Text Alignment, Vertical Position (Top/Bottom only)
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
class ShowConfig;
class TogglesConfig;

// Toggles Configuration
class TogglesConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    bool isFPSGraphMode;
    
public:
    TogglesConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
        isFPSGraphMode = (mode == "FPS Graph");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Toggles"));
        
        if (isFPSGraphMode) {
            // FPS Graph: only show_info
            auto* showInfo = new tsl::elm::ToggleListItem("Show Info", getCurrentShowInfo());
            showInfo->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "fps-graph", "show_info", state ? "true" : "false");
            });
            list->addItem(showInfo);
            
        } else if (isFullMode) {
            // Full mode: specific full toggles
            auto* realFreqs = new tsl::elm::ToggleListItem("Show Real Freqs", getCurrentShowRealFreqs());
            realFreqs->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_real_freqs", state ? "true" : "false");
            });
            list->addItem(realFreqs);
            
            auto* showDeltas = new tsl::elm::ToggleListItem("Show Deltas", getCurrentShowDeltas());
            showDeltas->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_deltas", state ? "true" : "false");
            });
            list->addItem(showDeltas);
            
            auto* targetFreqs = new tsl::elm::ToggleListItem("Show Target Freqs", getCurrentShowTargetFreqs());
            targetFreqs->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_target_freqs", state ? "true" : "false");
            });
            list->addItem(targetFreqs);
            
            auto* showFPS = new tsl::elm::ToggleListItem("Show FPS", getCurrentShowFPS());
            showFPS->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_fps", state ? "true" : "false");
            });
            list->addItem(showFPS);
            
            auto* showRES = new tsl::elm::ToggleListItem("Show RES", getCurrentShowRES());
            showRES->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_res", state ? "true" : "false");
            });
            list->addItem(showRES);
            
            auto* showRDSD = new tsl::elm::ToggleListItem("Show Read Speed", getCurrentShowRDSD());
            showRDSD->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_read_speed", state ? "true" : "false");
            });
            list->addItem(showRDSD);
            
        } else if (isMiniMode || isMicroMode) {
            // Mini/Micro modes: shared toggles
            const std::string section = isMiniMode ? "mini" : "micro";
            
            auto* realFreqs = new tsl::elm::ToggleListItem("Real Frequencies", getCurrentRealFreqs());
            realFreqs->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "real_freqs", state ? "true" : "false");
            });
            list->addItem(realFreqs);
            
            auto* realVolts = new tsl::elm::ToggleListItem("Real Voltages", getCurrentRealVolts());
            realVolts->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "real_volts", state ? "true" : "false");
            });
            list->addItem(realVolts);
            
            auto* showFullCPU = new tsl::elm::ToggleListItem("Show Full CPU", getCurrentShowFullCPU());
            showFullCPU->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_full_cpu", state ? "true" : "false");
            });
            list->addItem(showFullCPU);
            
            auto* showFullRes = new tsl::elm::ToggleListItem("Show Full Resolution", getCurrentShowFullRes());
            showFullRes->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_full_res", state ? "true" : "false");
            });
            list->addItem(showFullRes);
            
            auto* socVoltage = new tsl::elm::ToggleListItem("Show SOC Voltage", getCurrentShowSOCVoltage());
            socVoltage->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_soc_voltage", state ? "true" : "false");
            });
            list->addItem(socVoltage);
            
            auto* dtcSymbol = new tsl::elm::ToggleListItem("Use DTC Symbol", getCurrentUseDTCSymbol());
            dtcSymbol->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "use_dtc_symbol", state ? "true" : "false");
            });
            list->addItem(dtcSymbol);

            auto* dynamicColors = new tsl::elm::ToggleListItem("Use Dynamic Colors", getCurrentUseDynamicColors());
            dynamicColors->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "use_dynamic_colors", state ? "true" : "false");
            });
            list->addItem(dynamicColors);
        }
        
        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        {
            jumpItemName = "";
            jumpItemValue = "";
            jumpItemExactMatch = false;
        }

        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
    
private:
    // Helper methods for getting current toggle states
    bool getCurrentShowInfo() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "fps-graph", "show_info");
        if (value.empty()) return false;
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentRealFreqs() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "real_freqs");
        if (value.empty()) return true;
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentRealVolts() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "real_volts");
        if (value.empty()) return true;
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentShowFullCPU() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "show_full_cpu");
        if (value.empty()) return false;
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentShowFullRes() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "show_full_res");
        if (value.empty()) return isMiniMode; // Default: true for mini, false for micro
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowSOCVoltage() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "show_soc_voltage");
        if (value.empty()) return !isMiniMode; // Default: false for mini, true for micro
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentUseDTCSymbol() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "use_dtc_symbol");
        if (value.empty()) return true;
        convertToUpper(value);
        return value == "TRUE";
    }

    bool getCurrentUseDynamicColors() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "use_dynamic_colors");
        if (value.empty()) return true;
        convertToUpper(value);
        return value == "TRUE";
    }
    
    // Full mode toggle helpers
    bool getCurrentShowRealFreqs() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_real_freqs");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowDeltas() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_deltas");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowTargetFreqs() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_target_freqs");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowFPS() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_fps");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowRES() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_res");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowRDSD() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_read_speed");
        if (value.empty()) return true;
        convertToUpper(value);
        return value != "FALSE";
    }
};

// Refresh Rate Configuration
class RefreshRateConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    bool isGameResolutionsMode;
    bool isFPSCounterMode;
    bool isFPSGraphMode;
    int currentRate;
    
public:
    RefreshRateConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
        isGameResolutionsMode = (mode == "Game Resolutions");
        isFPSCounterMode = (mode == "FPS Counter");
        isFPSGraphMode = (mode == "FPS Graph");
        
        std::string section;
        if (isMiniMode) section = "mini";
        else if (isMicroMode) section = "micro";
        else if (isFullMode) section = "full";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        int defaultRate = (isGameResolutionsMode) ? 10 : ((isFPSCounterMode || isFPSGraphMode) ? 30 : 1);
        currentRate = value.empty() ? defaultRate : std::clamp(atoi(value.c_str()), 1, 60);
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Refresh Rate"));

        static const std::vector<int> rates = {1, 2, 3, 5, 10, 15, 30, 60};
        for (int rate : rates) {
            auto* rateItem = new tsl::elm::ListItem(std::to_string(rate) + " Hz");
            if (rate == currentRate) {
                rateItem->setValue(ult::CHECKMARK_SYMBOL);
                lastSelectedListItem = rateItem;
            }
            rateItem->setClickListener([this, rateItem, rate](uint64_t keys) {
                if (keys & KEY_A) {
                    std::string section;
                    if (isMiniMode) section = "mini";
                    else if (isMicroMode) section = "micro";
                    else if (isFullMode) section = "full";
                    else if (isGameResolutionsMode) section = "game_resolutions";
                    else if (isFPSCounterMode) section = "fps-counter";
                    else if (isFPSGraphMode) section = "fps-graph";
                    
                    ult::setIniFileValue(configIniPath, section, "refresh_rate", std::to_string(rate));
                    rateItem->setValue(ult::CHECKMARK_SYMBOL);
                    if (lastSelectedListItem)
                        lastSelectedListItem->setValue("");
                    lastSelectedListItem = rateItem;
                    return true;
                }
                return false;
            });
            list->addItem(rateItem);
        }
        
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);

        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            jumpItemName = "Refresh Rate";
            jumpItemValue = "";
            jumpItemExactMatch = false;
            
            tsl::swapTo<ConfiguratorOverlay>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// Font Size Selector
class FontSizeSelector : public tsl::Gui {
private:
    std::string modeName;
    std::string fontType;
    bool isMiniMode;
    bool isMicroMode;
    bool isFPSCounterMode;
    std::string title;
    
public:
    FontSizeSelector(const std::string& mode, const std::string& type) 
        : modeName(mode), fontType(type) {
            isMiniMode = (mode == "Mini");
            isMicroMode = (mode == "Micro");
            isFPSCounterMode = (mode == "FPS Counter");
            title = fontType;
            title[0] = std::toupper(title[0]);
            title += " Font Size";
        }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(title));

        std::string section;
        if (isMiniMode) section = "mini";
        else if (isMicroMode) section = "micro";
        else if (isFPSCounterMode) section = "fps-counter";
        
        const std::string keyName = fontType + "_font_size";
        const std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, keyName);
        int defaultSize = isFPSCounterMode ? 40 : 15;
        const int currentSize = currentValue.empty() ? defaultSize : atoi(currentValue.c_str());
        
        // Font size range depends on mode
        int minSize = 8;
        int maxSize;
        if (isFPSCounterMode) maxSize = 150;
        else if (isMiniMode) maxSize = 22;
        else maxSize = 18; // Micro mode
        
        for (int size = minSize; size <= maxSize; size++) {
            auto* sizeItem = new tsl::elm::ListItem(std::to_string(size) + " pt");
            if (size == currentSize) {
                sizeItem->setValue(ult::CHECKMARK_SYMBOL);
                lastSelectedListItem = sizeItem;
            }
            sizeItem->setClickListener([this, sizeItem, size, keyName, section](uint64_t keys) {
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, keyName, std::to_string(size));
                    sizeItem->setValue(ult::CHECKMARK_SYMBOL);
                    if (lastSelectedListItem)
                        lastSelectedListItem->setValue("");
                    lastSelectedListItem = sizeItem;
                    return true;
                }
                return false;
            });
            list->addItem(sizeItem);
        }
        
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Font Sizes");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            jumpItemName = title;
            jumpItemValue = "";
            jumpItemExactMatch = false;
            
            tsl::swapTo<FontSizeConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// Font Size Configuration
class FontSizeConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFPSCounterMode;
    
public:
    FontSizeConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFPSCounterMode = (mode == "FPS Counter");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Font Sizes"));
        
        std::string section;
        if (isMiniMode) section = "mini";
        else if (isMicroMode) section = "micro";
        else if (isFPSCounterMode) section = "fps-counter";
        
        const std::string handheldValue = ult::parseValueFromIniSection(configIniPath, section, "handheld_font_size");
        const std::string dockedValue = ult::parseValueFromIniSection(configIniPath, section, "docked_font_size");
        
        int defaultSize = isFPSCounterMode ? 40 : 15;
        const int handheldSize = handheldValue.empty() ? defaultSize : atoi(handheldValue.c_str());
        const int dockedSize = dockedValue.empty() ? defaultSize : atoi(dockedValue.c_str());
        
        auto* handheldItem = new tsl::elm::ListItem("Handheld Font Size");
        handheldItem->setValue(std::to_string(handheldSize) + " pt");
        handheldItem->setClickListener([this](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<FontSizeSelector>(modeName, "handheld");
                return true;
            }
            return false;
        });
        list->addItem(handheldItem);
        
        auto* dockedItem = new tsl::elm::ListItem("Docked Font Size");
        dockedItem->setValue(std::to_string(dockedSize) + " pt");
        dockedItem->setClickListener([this](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<FontSizeSelector>(modeName, "docked");
                return true;
            }
            return false;
        });
        list->addItem(dockedItem);
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        {
            jumpItemName = "";
            jumpItemValue = "";
            jumpItemExactMatch = false;
        }
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// Color Selector
class ColorSelector : public tsl::Gui {
private:
    std::string modeName;
    std::string modeTitle;
    std::string colorKey;
    std::string defaultValue;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    bool isGameResolutionsMode;
    bool isFPSCounterMode;
    bool isFPSGraphMode;
    
public:
    ColorSelector(const std::string& mode, const std::string& title, const std::string& key, const std::string& def) 
        : modeName(mode), modeTitle(title), colorKey(key), defaultValue(def) {
            isMiniMode = (mode == "Mini");
            isMicroMode = (mode == "Micro");
            isFullMode = (mode == "Full");
            isGameResolutionsMode = (mode == "Game Resolutions");
            isFPSCounterMode = (mode == "FPS Counter");
            isFPSGraphMode = (mode == "FPS Graph");
        }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(modeTitle));

        std::string section;
        if (isMiniMode) section = "mini";
        else if (isMicroMode) section = "micro";
        else if (isFullMode) section = "full";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, colorKey);
        if (currentValue.empty()) currentValue = defaultValue;
        
        static const std::vector<std::pair<std::string, std::string>> colors = {
            {"Transparent", "#0000"},
            {"Semi-transparent Black", "#0009"},
            {"Black", "#000F"},
            {"White", "#FFFF"},
            {"Red", "#F00F"},
            {"Green", "#0F0F"},
            {"Blue", "#00FF"},
            {"Yellow", "#FF0F"},
            {"Cyan", "#0FFF"},
            {"Magenta", "#F0FF"},
            {"Light Blue", "#2DFF"},
            {"Dark Blue", "#003F"}
        };
        
        std::string _jumpItemValue;
        std::string lastSelectedColor;
        for (const auto& color : colors) {
            auto* colorItem = new tsl::elm::ListItem(color.first);
            colorItem->setValue(color.second);
            if (color.second == currentValue) {
                colorItem->setValue(color.second + " " + ult::CHECKMARK_SYMBOL);
                lastSelectedListItem = colorItem;
                _jumpItemValue = color.second + " " + ult::CHECKMARK_SYMBOL;
                lastSelectedColor = color.second;
            }
            colorItem->setClickListener([this, lastSelectedColor, colorItem, color, section](uint64_t keys) {
                static auto _lastSelectedColor = lastSelectedColor;
                if (keys & KEY_A) {
                    ult::setIniFileValue(configIniPath, section, colorKey, color.second);
                    
                    colorItem->setValue(color.second + " " + ult::CHECKMARK_SYMBOL);
                    if (lastSelectedListItem)
                        lastSelectedListItem->setValue(_lastSelectedColor);

                    lastSelectedListItem = colorItem;
                    _lastSelectedColor = color.second;
                    return true;
                }
                return false;
            });
            list->addItem(colorItem);
        }
        list->jumpToItem("", _jumpItemValue, false);
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Colors");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            jumpItemName = modeTitle;
            jumpItemValue = "";
            jumpItemExactMatch = false;

            tsl::swapTo<ColorConfig>(SwapDepth(2), modeName);
            return true;
        }
        return false;
    }
};

// Color Configuration
class ColorConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    bool isGameResolutionsMode;
    bool isFPSCounterMode;
    bool isFPSGraphMode;
    
public:
    ColorConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
        isGameResolutionsMode = (mode == "Game Resolutions");
        isFPSCounterMode = (mode == "FPS Counter");
        isFPSGraphMode = (mode == "FPS Graph");
        
        // Full mode should never access color configuration
        if (isFullMode) {
            // This should not happen, but if it does, go back
            tsl::goBack();
        }
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Colors"));
        
        auto getCurrentColor = [this](const std::string& key, const std::string& def) {
            std::string section;
            if (isMiniMode) section = "mini";
            else if (isMicroMode) section = "micro";
            else if (isFullMode) section = "full";
            else if (isGameResolutionsMode) section = "game_resolutions";
            else if (isFPSCounterMode) section = "fps-counter";
            else if (isFPSGraphMode) section = "fps-graph";
            
            std::string value = ult::parseValueFromIniSection(configIniPath, section, key);
            return value.empty() ? def : value;
        };
        
        // Background Color (all modes)
        auto* bgColor = new tsl::elm::ListItem("Background Color");
        std::string bgDefault = "#0009";
        bgColor->setValue(getCurrentColor("background_color", bgDefault));
        bgColor->setClickListener([this, bgDefault](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<ColorSelector>(modeName, "Background Color", "background_color", bgDefault);
                return true;
            }
            return false;
        });
        list->addItem(bgColor);
        
        // Text Color (all modes)
        auto* textColor = new tsl::elm::ListItem("Text Color");
        textColor->setValue(getCurrentColor("text_color", "#FFFF"));
        textColor->setClickListener([this](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<ColorSelector>(modeName, "Text Color", "text_color", "#FFFF");
                return true;
            }
            return false;
        });
        list->addItem(textColor);
        
        if (isFPSGraphMode) {
            // FPS Graph specific colors
            struct ColorSetting {
                std::string name;
                std::string key;
                std::string defaultVal;
            };
            
            std::vector<ColorSetting> fpsGraphColors = {
                {"FPS Counter Color", "fps_counter_color", "#4444"},
                {"Border Color", "border_color", "#F00F"},
                {"Dashed Line Color", "dashed_line_color", "#8888"},
                {"Max FPS Text Color", "max_fps_text_color", "#FFFF"},
                {"Min FPS Text Color", "min_fps_text_color", "#FFFF"},
                {"Main Line Color", "main_line_color", "#FFFF"},
                {"Rounded Line Color", "rounded_line_color", "#F0FF"},
                {"Perfect Line Color", "perfect_line_color", "#0C0F"}
            };
            
            for (const auto& color : fpsGraphColors) {
                auto* colorItem = new tsl::elm::ListItem(color.name);
                colorItem->setValue(getCurrentColor(color.key, color.defaultVal));
                colorItem->setClickListener([this, color](uint64_t keys) {
                    if (keys & KEY_A) {
                        tsl::changeTo<ColorSelector>(modeName, color.name, color.key, color.defaultVal);
                        return true;
                    }
                    return false;
                });
                list->addItem(colorItem);
            }
            
        } else if (isMiniMode) {
            // Mini mode: has all colors including focus background
            auto* focusBgColor = new tsl::elm::ListItem("Focus Background Color");
            focusBgColor->setValue(getCurrentColor("focus_background_color", "#000F"));
            focusBgColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Focus Background Color", "focus_background_color", "#000F");
                    return true;
                }
                return false;
            });
            list->addItem(focusBgColor);
            
            auto* sepColor = new tsl::elm::ListItem("Separator Color");
            sepColor->setValue(getCurrentColor("separator_color", "#2DFF"));
            sepColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Separator Color", "separator_color", "#2DFF");
                    return true;
                }
                return false;
            });
            list->addItem(sepColor);
            
            auto* catColor = new tsl::elm::ListItem("Category Color");
            catColor->setValue(getCurrentColor("cat_color", "#2DFF"));
            catColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Category Color", "cat_color", "#2DFF");
                    return true;
                }
                return false;
            });
            list->addItem(catColor);
            
        } else if (isMicroMode) {
            // Micro mode: separator and category colors (no focus background like Mini)
            auto* sepColor = new tsl::elm::ListItem("Separator Color");
            sepColor->setValue(getCurrentColor("separator_color", "#2DFF"));
            sepColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Separator Color", "separator_color", "#2DFF");
                    return true;
                }
                return false;
            });
            list->addItem(sepColor);
            
            auto* catColor = new tsl::elm::ListItem("Category Color");
            catColor->setValue(getCurrentColor("cat_color", "#2DFF"));
            catColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Category Color", "cat_color", "#2DFF");
                    return true;
                }
                return false;
            });
            list->addItem(catColor);
            
        } else if (isGameResolutionsMode) {
            // Game Resolutions: only category color (no separator)
            auto* catColor = new tsl::elm::ListItem("Category Color");
            catColor->setValue(getCurrentColor("cat_color", "#FFFF"));
            catColor->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorSelector>(modeName, "Category Color", "cat_color", "#FFFF");
                    return true;
                }
                return false;
            });
            list->addItem(catColor);
        }
        // FPS Counter mode: only background and text colors (already added above)
        // Full mode: NO color settings at all (excluded from this function)
        
        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        {
            jumpItemName = "";
            jumpItemValue = "";
            jumpItemExactMatch = false;
        }

        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// Show Configuration (Mini/Micro only)
class ShowConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    std::vector<std::string> elementOrder;
    std::unordered_set<std::string> enabledElements;
    
public:
    ShowConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Elements " + ult::DIVIDER_SYMBOL + "\uE0E3 Move Up / \uE0E2 Move Down"));

        const std::string section = isMiniMode ? "mini" : "micro";
        std::string showValue = ult::parseValueFromIniSection(configIniPath, section, "show");
        std::string orderValue = ult::parseValueFromIniSection(configIniPath, section, "element_order");
        
        if (showValue.empty()) {
            showValue = isMiniMode ? "DTC+BAT+CPU+GPU+RAM+TMP+FPS+RES" : "FPS+CPU+GPU+RAM+SOC+BAT+DTC";
        }
        convertToUpper(showValue);
        
        enabledElements.clear();
        ult::StringStream ss(showValue);
        std::string item;
        while (ss.getline(item, '+')) {
            if (!item.empty()) {
                enabledElements.insert(item);
            }
        }
        
        elementOrder.clear();
        if (!orderValue.empty()) {
            convertToUpper(orderValue);
            ult::StringStream orderSS(orderValue);
            std::string orderItem;
            while (orderSS.getline(orderItem, '+')) {
                if (!orderItem.empty()) {
                    elementOrder.push_back(orderItem);
                }
            }
        } else {
            ult::StringStream ss2(showValue);
            while (ss2.getline(item, '+')) {
                if (!item.empty()) {
                    elementOrder.push_back(item);
                }
            }
        }
        
        static const std::vector<std::string> allElements = {"DTC", "BAT", "CPU", "GPU", "RAM", "TMP", "FPS", "RES", "SOC", "READ"};
        
        for (const std::string& element : allElements) {
            if (std::find(elementOrder.begin(), elementOrder.end(), element) == elementOrder.end()) {
                elementOrder.push_back(element);
            }
        }
        
        for (size_t i = 0; i < elementOrder.size(); i++) {
            const std::string& element = elementOrder[i];
            const bool isEnabled = enabledElements.find(element) != enabledElements.end();
            
            auto* elementItem = new tsl::elm::ListItem(element);
            elementItem->setValue(isEnabled ? "On" : "Off", !isEnabled ? true : false);
            
            elementItem->setClickListener([this, elementItem, element](uint64_t keys) {
                static bool hasNotTriggeredAnimation = false;

                if (hasNotTriggeredAnimation) {
                    elementItem->triggerClickAnimation();
                    hasNotTriggeredAnimation = false;
                }

                if (keys & KEY_A) {
                    const bool currentlyEnabled = enabledElements.find(element) != enabledElements.end();
                    
                    if (currentlyEnabled) {
                        enabledElements.erase(element);
                    } else {
                        enabledElements.insert(element);
                    }
                    
                    updateShowAndOrder();
                    jumpItemName = element;
                    jumpItemValue = "";
                    jumpItemExactMatch = true;
                    hasNotTriggeredAnimation = true;
                    
                    tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
                    return true;
                }
                else if (keys & KEY_Y || keys & KEY_X) {
                    size_t currentPos = 0;
                    for (size_t j = 0; j < elementOrder.size(); j++) {
                        if (elementOrder[j] == element) {
                            currentPos = j;
                            break;
                        }
                    }
                    
                    if (keys & KEY_Y) {
                        if (currentPos > 0) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos - 1]);
                        } else {
                            const std::string temp = elementOrder[0];
                            for (size_t j = 0; j < elementOrder.size() - 1; j++) {
                                elementOrder[j] = elementOrder[j + 1];
                            }
                            elementOrder[elementOrder.size() - 1] = temp;
                        }
                    } else if (keys & KEY_X) {
                        if (currentPos < elementOrder.size() - 1) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos + 1]);
                        } else {
                            const std::string temp = elementOrder[elementOrder.size() - 1];
                            for (size_t j = elementOrder.size() - 1; j > 0; j--) {
                                elementOrder[j] = elementOrder[j - 1];
                            }
                            elementOrder[0] = temp;
                        }
                    }
                    
                    updateShowAndOrder();
                    jumpItemName = element;
                    jumpItemValue = "";
                    jumpItemExactMatch = true;
                    
                    tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
                    return true;
                }
                return false;
            });
            
            list->addItem(elementItem);
        }

        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch);
        {
            jumpItemName = "";
            jumpItemValue = "";
            jumpItemExactMatch = false;
        }
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
    
private:
    void updateShowAndOrder() {
        std::string newShowValue;
        std::string newOrderValue;
        bool showFirst = true;
        bool orderFirst = true;
        
        for (const std::string& element : elementOrder) {
            if (!orderFirst) {
                newOrderValue += "+";
            }
            newOrderValue += element;
            orderFirst = false;
            
            if (enabledElements.find(element) != enabledElements.end()) {
                if (!showFirst) {
                    newShowValue += "+";
                }
                newShowValue += element;
                showFirst = false;
            }
        }
        
        const std::string section = isMiniMode ? "mini" : "micro";
        ult::setIniFileValue(configIniPath, section, "show", newShowValue);
        ult::setIniFileValue(configIniPath, section, "element_order", newOrderValue);
    }
};

// Main Configurator
class ConfiguratorOverlay : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    bool isGameResolutionsMode;
    bool isFPSCounterMode;
    bool isFPSGraphMode;
    
public:
    ConfiguratorOverlay(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
        isGameResolutionsMode = (mode == "Game Resolutions");
        isFPSCounterMode = (mode == "FPS Counter");
        isFPSGraphMode = (mode == "FPS Graph");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Configuration"));

        // 1. Refresh Rate (all modes)
        auto* refreshRate = new tsl::elm::ListItem("Refresh Rate");
        refreshRate->setValue(std::to_string(getCurrentRefreshRate()) + " Hz");
        refreshRate->setClickListener([this](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<RefreshRateConfig>(modeName);
                return true;
            }
            return false;
        });
        list->addItem(refreshRate);
        
        // 2. Colors (not Full mode - it has no color settings)
        if (!isFullMode) {
            auto* colors = new tsl::elm::ListItem("Colors");
            colors->setValue(ult::DROPDOWN_SYMBOL);
            colors->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ColorConfig>(modeName);
                    return true;
                }
                return false;
            });
            list->addItem(colors);
        }
        
        // 3. Toggles (Mini/Micro/Full/FPS Graph only - Game Resolutions and FPS Counter have no toggles)
        if (isMiniMode || isMicroMode || isFullMode || isFPSGraphMode) {
            auto* toggles = new tsl::elm::ListItem("Toggles");
            toggles->setValue(ult::DROPDOWN_SYMBOL);
            toggles->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<TogglesConfig>(modeName);
                    return true;
                }
                return false;
            });
            list->addItem(toggles);
        }
        
        // 4. Font Sizes (Mini/Micro/FPS Counter only)
        if (isMiniMode || isMicroMode || isFPSCounterMode) {
            auto* fontSizes = new tsl::elm::ListItem("Font Sizes");
            fontSizes->setValue(ult::DROPDOWN_SYMBOL);
            fontSizes->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<FontSizeConfig>(modeName);
                    return true;
                }
                return false;
            });
            list->addItem(fontSizes);
        }
        
        // 5. Elements (Mini/Micro only)
        if (isMiniMode || isMicroMode) {
            auto* showSettings = new tsl::elm::ListItem("Elements");
            showSettings->setValue(ult::DROPDOWN_SYMBOL);
            showSettings->setClickListener([this](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::changeTo<ShowConfig>(modeName);
                    return true;
                }
                return false;
            });
            list->addItem(showSettings);
        }
        
        // 6. Mode-specific positioning settings
        if (isMicroMode) {
            // Text Alignment for Micro
            auto* textAlign = new tsl::elm::ListItem("Text Alignment");
            textAlign->setValue(getCurrentTextAlign());
            textAlign->setClickListener([this, textAlign](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string next = cycleTextAlign();
                    textAlign->setValue(next);
                    return true;
                }
                return false;
            });
            list->addItem(textAlign);

            // Vertical Position for Micro (Top/Bottom only)
            auto* layerPos = new tsl::elm::ListItem("Vertical Position");
            layerPos->setValue(getCurrentLayerPosBottom());
            layerPos->setClickListener([this, layerPos](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string next = cycleLayerPosBottom();
                    layerPos->setValue(next);
                    return true;
                }
                return false;
            });
            list->addItem(layerPos);
            
        } else if (isFullMode) {
            // Horizontal Position for Full (Left/Right only)
            auto* layerPos = new tsl::elm::ListItem("Horizontal Position");
            layerPos->setValue(getCurrentLayerPosRight());
            layerPos->setClickListener([this, layerPos](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string next = cycleLayerPosRight();
                    layerPos->setValue(next);
                    return true;
                }
                return false;
            });
            list->addItem(layerPos);
            
        } else if (isGameResolutionsMode || isFPSCounterMode || isFPSGraphMode) {
            // Both horizontal and vertical positioning
            auto* layerPosH = new tsl::elm::ListItem("Horizontal Position");
            layerPosH->setValue(getCurrentLayerPosRight());
            layerPosH->setClickListener([this, layerPosH](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string next = cycleLayerPosRight();
                    layerPosH->setValue(next);
                    return true;
                }
                return false;
            });
            list->addItem(layerPosH);
            
            auto* layerPosV = new tsl::elm::ListItem("Vertical Position");
            layerPosV->setValue(getCurrentLayerPosBottom());
            layerPosV->setClickListener([this, layerPosV](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string next = cycleLayerPosBottom();
                    layerPosV->setValue(next);
                    return true;
                }
                return false;
            });
            list->addItem(layerPosV);
        }
        
        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch.load(std::memory_order_acquire));
        {
            jumpItemName = "";
            jumpItemValue = "";
            jumpItemExactMatch = false;
        }
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", modeName);
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
    
private:
    int getCurrentRefreshRate() {
        std::string section;
        if (isMiniMode) section = "mini";
        else if (isMicroMode) section = "micro";
        else if (isFullMode) section = "full";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        int defaultRate = (isGameResolutionsMode) ? 10 : ((isFPSCounterMode || isFPSGraphMode) ? 30 : 1);
        return value.empty() ? defaultRate : atoi(value.c_str());
    }
    
    std::string getCurrentTextAlign() {
        if (isMicroMode) {
            std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "text_align");
            convertToUpper(value);
            if (value == "LEFT") return "Left";
            if (value == "RIGHT") return "Right";
            return "Center";
        }
        return "";
    }
    
    std::string getCurrentLayerPosRight() {
        std::string section;
        if (isFullMode) section = "full";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "layer_width_align");
        convertToUpper(value);
        
        if (isFullMode) {
            // Full mode: only Left and Right allowed
            if (value == "RIGHT") return "Right";
            return "Left";
        } else {
            // Other modes: allow Center
            if (value == "RIGHT") return "Right";
            if (value == "CENTER") return "Center";
            return "Left";
        }
    }

    std::string getCurrentLayerPosBottom() {
        std::string section;
        if (isMicroMode) section = "micro";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "layer_height_align");
        convertToUpper(value);
        
        if (isMicroMode) {
            // Micro mode: only Top and Bottom allowed
            if (value == "BOTTOM") return "Bottom";
            return "Top";
        } else {
            // Other modes: allow Center
            if (value == "BOTTOM") return "Bottom";
            if (value == "CENTER") return "Center";
            return "Top";
        }
    }
    
    std::string cycleTextAlign() {
        if (isMicroMode) {
            const std::string current = getCurrentTextAlign();
            std::string next;
            if (current == "Left") next = "Center";
            else if (current == "Center") next = "Right";
            else if (current == "Right") next = "Left";
            else next = "Center";
            
            ult::setIniFileValue(configIniPath, "micro", "text_align", next);
            return next;
        }
        return "";
    }
    
    std::string cycleLayerPosRight() {
        std::string section;
        if (isFullMode) section = "full";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        const std::string current = getCurrentLayerPosRight();
        std::string next;
        
        if (isFullMode) {
            // Full mode: only Left and Right
            if (current == "Left") next = "Right";
            else next = "Left";
        } else {
            // Other modes: Left -> Center -> Right -> Left
            if (current == "Left") next = "Center";
            else if (current == "Center") next = "Right";
            else next = "Left";
        }
        
        const std::string value = (next == "Right") ? "right" : (next == "Center" ? "center" : "left");
        ult::setIniFileValue(configIniPath, section, "layer_width_align", value);
        return next;
    }

    std::string cycleLayerPosBottom() {
        std::string section;
        if (isMicroMode) section = "micro";
        else if (isGameResolutionsMode) section = "game_resolutions";
        else if (isFPSCounterMode) section = "fps-counter";
        else if (isFPSGraphMode) section = "fps-graph";
        
        const std::string current = getCurrentLayerPosBottom();
        std::string next;
        
        if (isMicroMode) {
            // Micro mode: only Top and Bottom
            if (current == "Top") next = "Bottom";
            else next = "Top";
        } else {
            // Other modes: Top -> Center -> Bottom -> Top
            if (current == "Top") next = "Center";
            else if (current == "Center") next = "Bottom";
            else next = "Top";
        }
        
        const std::string value = (next == "Bottom") ? "bottom" : (next == "Center" ? "center" : "top");
        ult::setIniFileValue(configIniPath, section, "layer_height_align", value);
        return next;
    }
};