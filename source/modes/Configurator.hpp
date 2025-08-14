#pragma once
#include <tesla.hpp>
#include "../Utils.hpp"

#include <unordered_set>

// External variables for navigation (add these to your globals)
extern std::string jumpItemName;
extern std::string jumpItemValue;
extern std::atomic<bool> jumpItemExactMatch;
//std::atomic<bool> skipJumpReset;
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
    
public:
    TogglesConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Toggles"));
        
        // Get section name for INI
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        
        if (isFullMode) {
            // Full mode specific toggles
            
            // Show Real Freqs
            auto* realFreqs = new tsl::elm::ToggleListItem("Show Real Freqs", getCurrentShowRealFreqs());
            realFreqs->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_real_freqs", state ? "true" : "false");
            });
            list->addItem(realFreqs);
            
            // Show Deltas
            auto* showDeltas = new tsl::elm::ToggleListItem("Show Deltas", getCurrentShowDeltas());
            showDeltas->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_deltas", state ? "true" : "false");
            });
            list->addItem(showDeltas);
            
            // Show Target Freqs
            auto* targetFreqs = new tsl::elm::ToggleListItem("Show Target Freqs", getCurrentShowTargetFreqs());
            targetFreqs->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_target_freqs", state ? "true" : "false");
            });
            list->addItem(targetFreqs);
            
            // Show FPS
            auto* showFPS = new tsl::elm::ToggleListItem("Show FPS", getCurrentShowFPS());
            showFPS->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_fps", state ? "true" : "false");
            });
            list->addItem(showFPS);
            
            // Show RES
            auto* showRES = new tsl::elm::ToggleListItem("Show RES", getCurrentShowRES());
            showRES->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_res", state ? "true" : "false");
            });
            list->addItem(showRES);
            
            // Show Read Speed
            auto* showRDSD = new tsl::elm::ToggleListItem("Show Read Speed", getCurrentShowRDSD());
            showRDSD->setStateChangedListener([this](bool state) {
                ult::setIniFileValue(configIniPath, "full", "show_read_speed", state ? "true" : "false");
            });
            list->addItem(showRDSD);
            
        } else {
            // Mini/Micro mode toggles (existing functionality)
            
            // Real Frequencies
            auto* realFreqs = new tsl::elm::ToggleListItem("Real Frequencies", getCurrentRealFreqs());
            realFreqs->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "real_freqs", state ? "true" : "false");
            });
            list->addItem(realFreqs);
            
            // Real Voltages
            auto* realVolts = new tsl::elm::ToggleListItem("Real Voltages", getCurrentRealVolts());
            realVolts->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "real_volts", state ? "true" : "false");
            });
            list->addItem(realVolts);
            
            // Show Full CPU
            auto* showFullCPU = new tsl::elm::ToggleListItem("Show Full CPU", getCurrentShowFullCPU());
            showFullCPU->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_full_cpu", state ? "true" : "false");
            });
            list->addItem(showFullCPU);
            
            // Show Full Resolution
            auto* showFullRes = new tsl::elm::ToggleListItem("Show Full Resolution", getCurrentShowFullRes());
            showFullRes->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_full_res", state ? "true" : "false");
            });
            list->addItem(showFullRes);
            
            // SOC Voltage
            auto* socVoltage = new tsl::elm::ToggleListItem("Show SOC Voltage", getCurrentShowSOCVoltage());
            socVoltage->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "show_soc_voltage", state ? "true" : "false");
            });
            list->addItem(socVoltage);
            
            // Use DTC Symbol
            auto* dtcSymbol = new tsl::elm::ToggleListItem("Use DTC Symbol", getCurrentUseDTCSymbol());
            dtcSymbol->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "use_dtc_symbol", state ? "true" : "false");
            });
            list->addItem(dtcSymbol);

            // Dynamic Colors
            auto* dynamicColors = new tsl::elm::ToggleListItem("Use Dynamic Colors", getCurrentUseDynamicColors());
            dynamicColors->setStateChangedListener([this, section](bool state) {
                ult::setIniFileValue(configIniPath, section, "use_dynamic_colors", state ? "true" : "false");
            });
            list->addItem(dynamicColors);
            
            // Mode-specific toggles
            //if (isMicroMode) {
            //    // Layer Position for Micro
            //    auto* layerPos = new tsl::elm::ToggleListItem("Layer Position Bottom", getCurrentLayerPosBottom());
            //    layerPos->setStateChangedListener([this](bool state) {
            //        ult::setIniFileValue(configIniPath, "micro", "layer_height_align", state ? "bottom" : "top");
            //    });
            //    list->addItem(layerPos);
            //}
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
    // Helper methods to get current values for Mini/Micro modes
    bool getCurrentRealFreqs() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "real_freqs");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentRealVolts() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "real_volts");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value == "TRUE";
    }
    
    bool getCurrentShowFullCPU() {
        const std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "show_full_cpu");
        if (value.empty()) return false; // Default is false
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
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value == "TRUE";
    }

    bool getCurrentUseDynamicColors() {
        std::string section = isMiniMode ? "mini" : "micro";
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "use_dynamic_colors");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value == "TRUE";
    }
    
    
    // Helper methods for Full mode toggles
    bool getCurrentShowRealFreqs() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_real_freqs");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowDeltas() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_deltas");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowTargetFreqs() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_target_freqs");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowFPS() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_fps");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowRES() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_res");
        if (value.empty()) return true; // Default is true
        convertToUpper(value);
        return value != "FALSE";
    }
    
    bool getCurrentShowRDSD() {
        std::string value = ult::parseValueFromIniSection(configIniPath, "full", "show_read_speed");
        if (value.empty()) return true; // Default is true
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
    int currentRate;
    
public:
    RefreshRateConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
        
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        const std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        currentRate = value.empty() ? 1 : std::clamp(atoi(value.c_str()), 1, 60);
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        
        list->addItem(new tsl::elm::CategoryHeader("Refresh Rate"));

        // Predefined values
        static const std::vector<int> rates = {1, 2, 3, 5, 10, 15, 30, 60};
        for (int rate : rates) {
            auto* rateItem = new tsl::elm::ListItem(std::to_string(rate) + " Hz");
            if (rate == currentRate) {
                rateItem->setValue(ult::CHECKMARK_SYMBOL);
                lastSelectedListItem = rateItem;
            }
            rateItem->setClickListener([this, rateItem, rate](uint64_t keys) {
                if (keys & KEY_A) {
                    const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
                    ult::setIniFileValue(configIniPath, section, "refresh_rate", std::to_string(rate));
                    rateItem->setValue(ult::CHECKMARK_SYMBOL);
                    if (lastSelectedListItem)
                        lastSelectedListItem->setValue("");
                    lastSelectedListItem = rateItem;
                    //tsl::goBack();
                    return true;
                }
                return false;
            });
            list->addItem(rateItem);
        }
        
        // Set jump target for return navigation
        //skipJumpReset.store(true, std::memory_order_release);
        list->jumpToItem("", ult::CHECKMARK_SYMBOL, false);

        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", "Configuration");
        rootFrame->setContent(list);
        return rootFrame;
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_B) {
            // Set jump target for return navigation
            jumpItemName = "Refresh Rate";
            jumpItemValue = "";
            jumpItemExactMatch = false;
            //skipJumpReset.store(true, std::memory_order_release);
            
            //tsl::goBack();
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
    std::string fontType; // "handheld" or "docked"
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    std::string title;
    
public:
    FontSizeSelector(const std::string& mode, const std::string& type) 
        : modeName(mode), fontType(type) {
            isMiniMode = (mode == "Mini");
            isMicroMode = (mode == "Micro");
            isFullMode = (mode == "Full");
            title = fontType;
            title[0] = std::toupper(title[0]); // Capitalize first letter
            title += " Font Size";
        }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader(title));

        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        const std::string keyName = fontType + "_font_size";
        const std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, keyName);
        const int currentSize = currentValue.empty() ? 15 : atoi(currentValue.c_str());
        
        // Font size range depends on mode
        int minSize = 8;
        const int maxSize = isMiniMode ? 22 : 18;
        
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
                    // Set jump target for return navigation
                    //jumpItemName = "Font Sizes";
                    //jumpItemValue = "";
                    //jumpItemExactMatch = false;
                    ////skipJumpReset.store(true, std::memory_order_release);
                    
                    //tsl::goBack();
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
            // Set jump target for return navigation
            jumpItemName = title;
            jumpItemValue = "";
            jumpItemExactMatch = false;
            //skipJumpReset.store(true, std::memory_order_release);
            
            //tsl::goBack();
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
    bool isFullMode;
    
public:
    FontSizeConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Font Sizes"));
        
        // Get current values
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        const std::string handheldValue = ult::parseValueFromIniSection(configIniPath, section, "handheld_font_size");
        const std::string dockedValue = ult::parseValueFromIniSection(configIniPath, section, "docked_font_size");
        
        const int handheldSize = handheldValue.empty() ? 15 : atoi(handheldValue.c_str());
        const int dockedSize = dockedValue.empty() ? 15 : atoi(dockedValue.c_str());
        
        // Handheld font size
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
        
        // Docked font size
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
    
public:
    ColorSelector(const std::string& mode, const std::string& title, const std::string& key, const std::string& def) 
        : modeName(mode), modeTitle(title), colorKey(key), defaultValue(def) {
            isMiniMode = (mode == "Mini");
            isMicroMode = (mode == "Micro");
            isFullMode = (mode == "Full");
        }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        
        list->addItem(new tsl::elm::CategoryHeader(modeTitle));

        // Get current value
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        std::string currentValue = ult::parseValueFromIniSection(configIniPath, section, colorKey);
        if (currentValue.empty()) currentValue = defaultValue;
        
        // Predefined colors with proper alpha values
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
                    // Set jump target for return navigation
                    //jumpItemName = "Colors";
                    //jumpItemValue = "";
                    //jumpItemExactMatch = false;
                    //skipJumpReset.store(true, std::memory_order_release);
                    
                    //tsl::goBack();
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
    
public:
    ColorConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        list->addItem(new tsl::elm::CategoryHeader("Colors"));
        
        // Helper function to get current color value
        auto getCurrentColor = [this](const std::string& key, const std::string& def) {
            std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
            std::string value = ult::parseValueFromIniSection(configIniPath, section, key);
            return value.empty() ? def : value;
        };
        
        // Background Color
        auto* bgColor = new tsl::elm::ListItem("Background Color");
        bgColor->setValue(getCurrentColor("background_color", "#0009"));
        bgColor->setClickListener([this](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::changeTo<ColorSelector>(modeName, "Background Color", "background_color", "#0009");
                return true;
            }
            return false;
        });
        list->addItem(bgColor);
        
        // Text Color
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
        
        // Separator Color
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
        
        // Category Color
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
        
        // Mini-specific colors
        if (isMiniMode) {
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
            // Set jump target for return navigation
            //jumpItemName = "Colors";
            //jumpItemValue = "";
            //jumpItemExactMatch = false;
            //skipJumpReset.store(true, std::memory_order_release);
            
            tsl::goBack();
            return true;
        }
        return false;
    }
};

// Show Configuration
class ShowConfig : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    std::vector<std::string> elementOrder;
    std::unordered_set<std::string> enabledElements;
    
public:
    ShowConfig(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
    }
    
    virtual tsl::elm::Element* createUI() override {
        auto* list = new tsl::elm::List();
        
        list->addItem(new tsl::elm::CategoryHeader("Elements " + ult::DIVIDER_SYMBOL + "\uE0E3 Move Up / \uE0E2 Move Down"));

        // Get current show string
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        std::string showValue = ult::parseValueFromIniSection(configIniPath, section, "show");
        
        // Get current order string (separate from show string to maintain positions)
        std::string orderValue = ult::parseValueFromIniSection(configIniPath, section, "element_order");
        
        if (showValue.empty()) {
            showValue = isMiniMode ? "DTC+BAT+CPU+GPU+RAM+TMP+FPS+RES" : "FPS+CPU+GPU+RAM+SOC+BAT+DTC";
        }
        convertToUpper(showValue);
        
        // Parse enabled elements
        enabledElements.clear();
        ult::StringStream ss(showValue);
        std::string item;
        while (ss.getline(item, '+')) {
            if (!item.empty()) {
                enabledElements.insert(item);
            }
        }
        
        // Parse element order (if exists) or use show string as initial order
        elementOrder.clear();
        if (!orderValue.empty()) {
            convertToUpper(orderValue);
            ult::StringStream orderSS(orderValue);  // Use your custom StringStream
            std::string orderItem;
            while (orderSS.getline(orderItem, '+')) {  // Call getline as a member function
                if (!orderItem.empty()) {
                    elementOrder.push_back(orderItem);
                }
            }
        } else {
            // First time setup - use show string as order
            ult::StringStream ss2(showValue);
            while (ss2.getline(item, '+')) {
                if (!item.empty()) {
                    elementOrder.push_back(item);
                }
            }
        }
        
        // All possible elements
        static const std::vector<std::string> allElements = {"DTC", "BAT", "CPU", "GPU", "RAM", "TMP", "FPS", "RES", "SOC"};
        
        // Add any missing elements to the end (only if they're not already in the order)
        for (const std::string& element : allElements) {
            if (std::find(elementOrder.begin(), elementOrder.end(), element) == elementOrder.end()) {
                elementOrder.push_back(element);
            }
        }
        
        // Create list items based on current order
        for (size_t i = 0; i < elementOrder.size(); i++) {
            const std::string& element = elementOrder[i];
            const bool isEnabled = enabledElements.find(element) != enabledElements.end();
            
            auto* elementItem = new tsl::elm::ListItem(element);
            elementItem->setValue(isEnabled ? "On" : "Off", !isEnabled ? true : false);
            
            elementItem->setClickListener([this, elementItem, element](uint64_t keys) {
                static bool hasNotTriggeredAnimation = false;

                if (hasNotTriggeredAnimation) { // needed because of swapTo
                    elementItem->triggerClickAnimation();
                    hasNotTriggeredAnimation = false;
                }

                if (keys & KEY_A) {
                    // Toggle on/off state
                    const bool currentlyEnabled = enabledElements.find(element) != enabledElements.end();
                    
                    if (currentlyEnabled) {
                        enabledElements.erase(element);
                    } else {
                        enabledElements.insert(element);
                    }
                    
                    // Update both show string and order
                    updateShowAndOrder();
                    jumpItemName = element;
                    jumpItemValue = "";
                    jumpItemExactMatch = true;
                    hasNotTriggeredAnimation = true;
                    
                    // Refresh the page to show updated state
                    tsl::swapTo<ShowConfig>(SwapDepth(1), modeName);
                    return true;
                }
                else if (keys & KEY_Y || keys & KEY_X) {
                    // Find current position of this element
                    size_t currentPos = 0;
                    for (size_t j = 0; j < elementOrder.size(); j++) {
                        if (elementOrder[j] == element) {
                            currentPos = j;
                            break;
                        }
                    }
                    
                    if (keys & KEY_Y) {
                        // Move Up
                        if (currentPos > 0) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos - 1]);
                        } else {
                            // Move from top to bottom
                            const std::string temp = elementOrder[0];
                            for (size_t j = 0; j < elementOrder.size() - 1; j++) {
                                elementOrder[j] = elementOrder[j + 1];
                            }
                            elementOrder[elementOrder.size() - 1] = temp;
                        }
                    } else if (keys & KEY_X) {
                        // Move Down
                        if (currentPos < elementOrder.size() - 1) {
                            std::swap(elementOrder[currentPos], elementOrder[currentPos + 1]);
                        } else {
                            // Move from bottom to top
                            const std::string temp = elementOrder[elementOrder.size() - 1];
                            for (size_t j = elementOrder.size() - 1; j > 0; j--) {
                                elementOrder[j] = elementOrder[j - 1];
                            }
                            elementOrder[0] = temp;
                        }
                    }
                    
                    // Update both show string and order
                    updateShowAndOrder();
                    jumpItemName = element;
                    jumpItemValue = "";
                    jumpItemExactMatch = true;
                    
                    // Refresh the page to show new order
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
        
        // Build show string from enabled elements in current order
        for (const std::string& element : elementOrder) {
            // Add to order string (all elements)
            if (!orderFirst) {
                newOrderValue += "+";
            }
            newOrderValue += element;
            orderFirst = false;
            
            // Add to show string (only enabled elements)
            if (enabledElements.find(element) != enabledElements.end()) {
                if (!showFirst) {
                    newShowValue += "+";
                }
                newShowValue += element;
                showFirst = false;
            }
        }
        
        // Save to ini
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        ult::setIniFileValue(configIniPath, section, "show", newShowValue);
        ult::setIniFileValue(configIniPath, section, "element_order", newOrderValue);
    }
};


// Base configurator class
class ConfiguratorOverlay : public tsl::Gui {
private:
    std::string modeName;
    bool isMiniMode;
    bool isMicroMode;
    bool isFullMode;
    
public:
    ConfiguratorOverlay(const std::string& mode) : modeName(mode) {
        isMiniMode = (mode == "Mini");
        isMicroMode = (mode == "Micro");
        isFullMode = (mode == "Full");
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
        
        // 2. Toggles dropdown (all modes)
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
        
        // 3. Font Sizes (Mini/Micro/Full modes have fonts)
        if (!isFullMode) {  // Only show if font configuration is relevant
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
        
        // 4. Colors (Mini/Micro modes only)
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
        
        // 5. Elements (Mini/Micro modes only)
        if (!isFullMode) {
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
        
        // 6. Mode-specific settings (non-toggle items only)
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

            // Layer Position for Micro
            //auto* layerPos = new tsl::elm::ToggleListItem("Layer Position Bottom", getCurrentLayerPosBottom());
            //layerPos->setStateChangedListener([this](bool state) {
            //    ult::setIniFileValue(configIniPath, "micro", "layer_height_align", state ? "bottom" : "top");
            //});
            //list->addItem(layerPos);

            // Layer Position for Full mode (setPosRight = layer_width_align)
            auto* layerPos = new tsl::elm::ListItem("Layer Position");
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
            // Layer Position for Full mode (setPosRight = layer_width_align)
            auto* layerPos = new tsl::elm::ListItem("Layer Position");
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
        }
        
        // Apply jumpToItem to land on the correct item when returning
        list->jumpToItem(jumpItemName, jumpItemValue, jumpItemExactMatch.load(std::memory_order_acquire));
        
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", modeName+" Mode");
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
    // Helper methods to get current values
    int getCurrentRefreshRate() {
        const std::string section = isMiniMode ? "mini" : (isMicroMode ? "micro" : "full");
        std::string value = ult::parseValueFromIniSection(configIniPath, section, "refresh_rate");
        return value.empty() ? 1 : atoi(value.c_str());
    }
    
    std::string getCurrentTextAlign() {
        if (isMicroMode) {
            std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "text_align");
            convertToUpper(value);
            if (value == "LEFT") return "Left";
            if (value == "RIGHT") return "Right";
            return "Center"; // Default
        }
        return "";
    }
    
    std::string getCurrentLayerPosRight() {
        if (isFullMode) {
            std::string value = ult::parseValueFromIniSection(configIniPath, "full", "layer_width_align");
            convertToUpper(value);
            if (value == "RIGHT") return "Right";
            return "Left"; // Default
        }
        return "";
    }

    std::string getCurrentLayerPosBottom() {
        if (isMicroMode) {
            std::string value = ult::parseValueFromIniSection(configIniPath, "micro", "layer_height_align");
            convertToUpper(value);
            if (value == "BOTTOM") return "Bottom";
            return "Top";
        }
        return "";
    }
    
    // Helper methods to change values
    std::string cycleTextAlign() {
        std::string next;
        if (isMicroMode) {
            const std::string current = getCurrentTextAlign();
            next = "Center";
            if (current == "Left") next = "Center";
            else if (current == "Center") next = "Right";
            else if (current == "Right") next = "Left";
            
            ult::setIniFileValue(configIniPath, "micro", "text_align", next);
        }
        return next;
    }
    
    std::string cycleLayerPosRight() {
        std::string next;
        if (isFullMode) {
            const std::string current = getCurrentLayerPosRight();
            next = (current == "Left") ? "Right" : "Left";
            
            const std::string value = (next == "Right") ? "right" : "left";
            ult::setIniFileValue(configIniPath, "full", "layer_width_align", value);
        }
        return next;
    }

    std::string cycleLayerPosBottom() {
        std::string next;
        if (isMicroMode) {
            const std::string current = getCurrentLayerPosBottom();
            next = (current == "Bottom") ? "Top" : "Bottom";
            
            const std::string value = (next == "Bottom") ? "bottom" : "top";
            ult::setIniFileValue(configIniPath, "micro", "layer_height_align", value);
        }
        return next;
    }
};
