#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "Utils.hpp"
#include <cstdlib>
#include <ctime>

//static tsl::elm::HeaderOverlayFrame* rootFrame = nullptr;
static bool skipMain = false;
static std::string lastSelectedItem;

#include "modes/FPS_Counter.hpp"
#include "modes/FPS_Graph.hpp"
#include "modes/Full.hpp"
#include "modes/Mini.hpp"
#include "modes/Micro.hpp"
#include "modes/Battery.hpp"
#include "modes/Misc.hpp"
#include "modes/Resolutions.hpp"
#include "modes/Configurator.hpp" 



//Graphs
//class GraphsMenu : public tsl::Gui {
//public:
//    GraphsMenu() {}
//
//    virtual tsl::elm::Element* createUI() override {
//        
//        auto* list = new tsl::elm::List();
//
//        list->addItem(new tsl::elm::CategoryHeader("FPS"));
//
//        auto* comFPSGraph = new tsl::elm::ListItem("Graph");
//        comFPSGraph->setClickListener([](uint64_t keys) {
//            if (keys & KEY_A) {
//                tsl::swapTo<com_FPSGraph>();
//                return true;
//            }
//            return false;
//        });
//        list->addItem(comFPSGraph);
//
//        auto* comFPSCounter = new tsl::elm::ListItem("Counter");
//        comFPSCounter->setClickListener([](uint64_t keys) {
//            if (keys & KEY_A) {
//                tsl::swapTo<com_FPS>();
//                return true;
//            }
//            return false;
//        });
//        list->addItem(comFPSCounter);
//
//        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("Status Monitor", "Modes");
//        rootFrame->setContent(list);
//
//        return rootFrame;
//    }
//
//    virtual void update() override {
//        if (fixForeground) {
//            fixForeground = false;
//            tsl::hlp::requestForeground(true);
//        }
//    }
//
//    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
//        if (disableJumpTo)
//            disableJumpTo = false;
//        if (fixHiding) {
//            if (isKeyComboPressed2(keysDown, keysHeld)) {
//                tsl::Overlay::get()->hide();
//                fixHiding = false;
//                return true;
//            }
//        }
//
//        if (keysDown & KEY_B) {
//            tsl::goBack();
//            return true;
//        }
//        return false;
//    }
//};

//Other
class OtherMenu : public tsl::Gui {
public:
    OtherMenu() { }

    virtual tsl::elm::Element* createUI() override {
        
        auto* list = new tsl::elm::List();

        list->addItem(new tsl::elm::CategoryHeader("Other"));

        auto* Battery = new tsl::elm::ListItem("Battery/Charger");
        Battery->disableClickAnimation();
        Battery->setClickListener([](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::swapTo<BatteryOverlay>();
                return true;
            }
            return false;
        });
        list->addItem(Battery);

        auto* Misc = new tsl::elm::ListItem("Miscellaneous");
        Misc->disableClickAnimation();
        Misc->setClickListener([](uint64_t keys) {
            if (keys & KEY_A) {
                tsl::swapTo<MiscOverlay>();
                return true;
            }
            return false;
        });
        list->addItem(Misc);

        //if (SaltySD) {
        //    auto* Res = new tsl::elm::ListItem("Game Resolutions");
        //    Res->setClickListener([](uint64_t keys) {
        //        if (keys & KEY_A) {
        //            tsl::swapTo<ResolutionsOverlay>();
        //            return true;
        //        }
        //        return false;
        //    });
        //    list->addItem(Res);
        //}
        //tsl::elm::g_disableMenuCacheOnReturn.store(true, std::memory_order_release);
        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("Status Monitor", "Modes");
        if (!lastSelectedItem.empty())
            list->jumpToItem(lastSelectedItem);

        rootFrame->setContent(list);

        return rootFrame;
    }

    virtual void update() override {
        if (fixForeground) {
            fixForeground = false;
            tsl::hlp::requestForeground(true);
        }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (disableJumpTo)
            disableJumpTo = false;
        if (fixHiding) {
            if (isKeyComboPressed2(keysDown, keysHeld)) {
                tsl::Overlay::get()->hide();
                fixHiding = false;
                return true;
            }
        }

        if (keysDown & KEY_B) {
            lastSelectedItem = "Other";
            tsl::swapTo<MainMenu>();
            triggerRumbleDoubleClick.store(true, std::memory_order_release);
            triggerExitSound.store(true, std::memory_order_release);
            return true;
        }
        return false;
    }
};

//Main Menu
class MainMenu : public tsl::Gui {
public:
    MainMenu() {
        if (lastMode != "returning")
            lastMode = "";
    }

    virtual tsl::elm::Element* createUI() override {
        
        auto* list = new tsl::elm::List();
        
        //list->addItem(new tsl::elm::CategoryHeader("Modes " + ult::DIVIDER_SYMBOL + " \uE0E0 Enter " + ult::DIVIDER_SYMBOL + " \uE0E3 Configure"));
        list->addItem(new tsl::elm::CategoryHeader("Modes " + ult::DIVIDER_SYMBOL + " \uE0E3 Configure"));

        auto* Full = new tsl::elm::ListItem("Full");
        Full->disableClickAnimation();
        Full->setClickListener([](uint64_t keys) {
            if (keys & KEY_A) {
                lastMode = "full";
                tsl::swapTo<FullOverlay>();
                return true;
            }
            if (keys & KEY_Y) {
                triggerRumbleClick.store(true, std::memory_order_release);
                triggerSettingsSound.store(true, std::memory_order_release);
                // Launch configurator for Mini mode
                tsl::swapTo<ConfiguratorOverlay>("Full");
                return true;
            }
            return false;
        });
        list->addItem(Full);
        //auto* Mini = new tsl::elm::ListItem("Mini");
        //Mini->setClickListener([](uint64_t keys) {
        //    if (keys & KEY_A) {
        //        tsl::swapTo<MiniOverlay>();
        //        return true;
        //    }
        //    return false;
        //});
        //list->addItem(Mini);

        bool fileExist = false;
        FILE* test = fopen(std::string(folderpath + filename).c_str(), "rb");
        if (test) {
            fclose(test);
            fileExist = true;
            filepath = folderpath + filename;
        }
        else {
            test = fopen(std::string(folderpath + "Status-Monitor-Overlay.ovl").c_str(), "rb");
            if (test) {
                fclose(test);
                fileExist = true;
                filepath = folderpath + "Status-Monitor-Overlay.ovl";
            }
        }
        if (fileExist) {
            auto* Mini = new tsl::elm::ListItem("Mini");
            Mini->disableClickAnimation();
            Mini->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::setNextOverlay(filepath, "--miniOverlay");
                    tsl::Overlay::get()->close();
                    return true;
                }
                if (keys & KEY_Y) {
                    triggerRumbleClick.store(true, std::memory_order_release);
                    triggerSettingsSound.store(true, std::memory_order_release);
                    // Launch configurator for Mini mode
                    tsl::swapTo<ConfiguratorOverlay>("Mini");
                    return true;
                }
                return false;
            });
            list->addItem(Mini);

            auto* Micro = new tsl::elm::ListItem("Micro");
            Micro->disableClickAnimation();
            Micro->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::setNextOverlay(filepath, "--microOverlay");
                    tsl::Overlay::get()->close();
                    return true;
                }
                if (keys & KEY_Y) {
                    triggerRumbleClick.store(true, std::memory_order_release);
                    triggerSettingsSound.store(true, std::memory_order_release);
                    // Launch configurator for Micro mode
                    tsl::swapTo<ConfiguratorOverlay>("Micro");
                    return true;
                }
                return false;
            });
            list->addItem(Micro);
        }
        if (SaltySD) {
            //auto* Graphs = new tsl::elm::ListItem("FPS");
            //Graphs->setValue(ult::DROPDOWN_SYMBOL);
            //Graphs->setClickListener([](uint64_t keys) {
            //    if (keys & KEY_A) {
            //        tsl::swapTo<GraphsMenu>();
            //        return true;
            //    }
            //    return false;
            //});
            //list->addItem(Graphs);

            auto* comFPSGraph = new tsl::elm::ListItem("FPS Graph");
            comFPSGraph->disableClickAnimation();
            comFPSGraph->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::elm::g_disableMenuCacheOnReturn.store(true, std::memory_order_release);
                    lastMode = "fps_graph";
                    tsl::swapTo<com_FPSGraph>();
                    return true;
                }
                if (keys & KEY_Y) {
                    triggerRumbleClick.store(true, std::memory_order_release);
                    triggerSettingsSound.store(true, std::memory_order_release);
                    // Launch configurator for Micro mode
                    tsl::swapTo<ConfiguratorOverlay>("FPS Graph");
                    return true;
                }
                return false;
            });
            list->addItem(comFPSGraph);

            auto* comFPSCounter = new tsl::elm::ListItem("FPS Counter");
            comFPSCounter->disableClickAnimation();
            comFPSCounter->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::elm::g_disableMenuCacheOnReturn.store(true, std::memory_order_release);
                    lastMode = "fps_counter";
                    tsl::swapTo<com_FPS>();
                    return true;
                }
                if (keys & KEY_Y) {
                    triggerRumbleClick.store(true, std::memory_order_release);
                    triggerSettingsSound.store(true, std::memory_order_release);
                    // Launch configurator for Micro mode
                    tsl::swapTo<ConfiguratorOverlay>("FPS Counter");
                    return true;
                }
                return false;
            });
            list->addItem(comFPSCounter);

            auto* Res = new tsl::elm::ListItem("Game Resolutions");
            Res->disableClickAnimation();
            Res->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    tsl::elm::g_disableMenuCacheOnReturn.store(true, std::memory_order_release);
                    lastMode = "game_resolutions";
                    tsl::swapTo<ResolutionsOverlay>();
                    return true;
                }
                if (keys & KEY_Y) {
                    triggerRumbleClick.store(true, std::memory_order_release);
                    triggerSettingsSound.store(true, std::memory_order_release);
                    // Launch configurator for Micro mode
                    tsl::swapTo<ConfiguratorOverlay>("Game Resolutions");
                    return true;
                }
                return false;
            });
            list->addItem(Res);

        }
        auto* Other = new tsl::elm::ListItem("Other");
        Other->disableClickAnimation();
        Other->setValue(ult::DROPDOWN_SYMBOL);
        Other->setClickListener([](uint64_t keys) {
            if (keys & KEY_A) {
                triggerRumbleClick.store(true, std::memory_order_release);
                triggerEnterSound.store(true, std::memory_order_release);
                tsl::swapTo<OtherMenu>();
                return true;
            }
            return false;
        });
        list->addItem(Other);

        if (!lastSelectedItem.empty())
            list->jumpToItem(lastSelectedItem);

        //list->disableCaching();
        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("Status Monitor", APP_VERSION);
        rootFrame->setContent(list);

        return rootFrame;
    }

    virtual void update() override {
        if (!ult::useRightAlignment) {
            //if ((tsl::cfg::LayerPosX || tsl::cfg::LayerPosY)) {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
            //}
        } else {
            const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
            tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
        }
        if (fixForeground) {
            fixForeground = false;
            tsl::hlp::requestForeground(true);
        }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (disableJumpTo)
            disableJumpTo = false;
        if (fixHiding) {
            if (isKeyComboPressed2(keysDown, keysHeld)) {
                tsl::Overlay::get()->hide();
                fixHiding = false;
                return true;
            }
        }

        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

class MonitorOverlay : public tsl::Overlay {
public:

    virtual void initServices() override {
        //Initialize services
        tsl::hlp::doWithSmSession([this]{

            apmInitialize();
            if (hosversionAtLeast(8,0,0)) clkrstCheck = clkrstInitialize();
            else pcvCheck = pcvInitialize();

            if (hosversionAtLeast(5,0,0)) tcCheck = tcInitialize();

            if (hosversionAtLeast(6,0,0) && R_SUCCEEDED(pwmInitialize())) {
                pwmCheck = pwmOpenSession2(&g_ICon, 0x3D000001);
            }

            if (R_SUCCEEDED(nvInitialize())) nvCheck = nvOpen(&fd, "/dev/nvhost-ctrl-gpu");

            psmCheck = psmInitialize();
            if (R_SUCCEEDED(psmCheck)) {
                psmService = psmGetServiceSession();
            }
            i2cCheck = i2cInitialize();

            SaltySD = CheckPort();

            if (SaltySD) {
                LoadSharedMemoryAndRefreshRate();
            }
            if (sysclkIpcRunning() && R_SUCCEEDED(sysclkIpcInitialize())) {
                uint32_t sysClkApiVer = 0;
                sysclkIpcGetAPIVersion(&sysClkApiVer);
                if (sysClkApiVer < 4) {
                    sysclkIpcExit();
                }
                else sysclkCheck = 0;
            }
            if (R_SUCCEEDED(splInitialize())) {
                u64 sku = 0;
                splGetConfig(SplConfigItem_HardwareType, &sku);
                switch(sku) {
                    case 2 ... 5:
                        isMariko = true;
                        break;
                    default:
                        isMariko = false;
                }
            }
            splExit();

        });
        Hinted = envIsSyscallHinted(0x6F);
    }

    virtual void exitServices() override {
        CloseThreads();
        if (R_SUCCEEDED(sysclkCheck)) {
            sysclkIpcExit();
        }
        shmemClose(&_sharedmemory);
        //Exit services
        clkrstExit();
        pcvExit();
        tsExit();
        tcExit();
        pwmChannelSessionClose(&g_ICon);
        pwmExit();
        nvClose(fd);
        nvExit();
        psmExit();
        i2cExit();
        apmExit();
    }

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainMenu>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

class MicroMode : public tsl::Overlay {
public:

    virtual void initServices() override {
        //tsl::hlp::requestForeground(false);
        //Initialize services
        tsl::hlp::doWithSmSession([this]{
            apmInitialize();
            if (hosversionAtLeast(8,0,0)) clkrstCheck = clkrstInitialize();
            else pcvCheck = pcvInitialize();

            if (R_SUCCEEDED(nvInitialize())) nvCheck = nvOpen(&fd, "/dev/nvhost-ctrl-gpu");

            if (hosversionAtLeast(5,0,0)) tcCheck = tcInitialize();

            if (hosversionAtLeast(6,0,0) && R_SUCCEEDED(pwmInitialize())) {
                pwmCheck = pwmOpenSession2(&g_ICon, 0x3D000001);
            }

            i2cCheck = i2cInitialize();

            psmCheck = psmInitialize();
            if (R_SUCCEEDED(psmCheck)) {
                psmService = psmGetServiceSession();
            }

            SaltySD = CheckPort();

            if (SaltySD) {
                LoadSharedMemory();
            }
            if (sysclkIpcRunning() && R_SUCCEEDED(sysclkIpcInitialize())) {
                uint32_t sysClkApiVer = 0;
                sysclkIpcGetAPIVersion(&sysClkApiVer);
                if (sysClkApiVer < 4) {
                    sysclkIpcExit();
                }
                else sysclkCheck = 0;
            }
            if (R_SUCCEEDED(splInitialize())) {
                u64 sku = 0;
                splGetConfig(SplConfigItem_HardwareType, &sku);
                switch(sku) {
                    case 2 ... 5:
                        isMariko = true;
                        break;
                    default:
                        isMariko = false;
                }
            }
            splExit();
        });
        Hinted = envIsSyscallHinted(0x6F);
    }

    virtual void exitServices() override {
        CloseThreads();
        shmemClose(&_sharedmemory);
        if (R_SUCCEEDED(sysclkCheck)) {
            sysclkIpcExit();
        }
        //Exit services
        clkrstExit();
        pcvExit();
        tsExit();
        tcExit();
        pwmChannelSessionClose(&g_ICon);
        pwmExit();
        i2cExit();
        psmExit();
        nvClose(fd);
        nvExit();
        apmExit();
    }

    virtual void onShow() override { // Called before overlay wants to change from invisible to visible state
        tsl::hlp::requestForeground(false);
    }    
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MicroOverlay>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

class MiniEntryOverlay : public tsl::Overlay {
public:
    MiniEntryOverlay() {}

    virtual void initServices() override {

        //tsl::hlp::requestForeground(false);
        // Same service‐init as before
        tsl::hlp::doWithSmSession([this]{
            apmInitialize();
            if (hosversionAtLeast(8,0,0)) clkrstCheck = clkrstInitialize();
            else pcvCheck = pcvInitialize();

            if (R_SUCCEEDED(nvInitialize())) nvCheck = nvOpen(&fd, "/dev/nvhost-ctrl-gpu");

            if (hosversionAtLeast(5,0,0)) tcCheck = tcInitialize();

            if (hosversionAtLeast(6,0,0) && R_SUCCEEDED(pwmInitialize())) {
                pwmCheck = pwmOpenSession2(&g_ICon, 0x3D000001);
            }

            i2cCheck = i2cInitialize();

            psmCheck = psmInitialize();
            if (R_SUCCEEDED(psmCheck)) {
                psmService = psmGetServiceSession();
            }

            SaltySD = CheckPort();

            if (SaltySD) {
                LoadSharedMemory();
            }
            if (sysclkIpcRunning() && R_SUCCEEDED(sysclkIpcInitialize())) {
                uint32_t sysClkApiVer = 0;
                sysclkIpcGetAPIVersion(&sysClkApiVer);
                if (sysClkApiVer < 4) {
                    sysclkIpcExit();
                }
                else sysclkCheck = 0;
            }
            if (R_SUCCEEDED(splInitialize())) {
                u64 sku = 0;
                splGetConfig(SplConfigItem_HardwareType, &sku);
                switch(sku) {
                    case 2 ... 5:
                        isMariko = true;
                        break;
                    default:
                        isMariko = false;
                }
            }
            splExit();
        });
        Hinted = envIsSyscallHinted(0x6F);

    }

    virtual void exitServices() override {
        CloseThreads();
        shmemClose(&_sharedmemory);
        if (R_SUCCEEDED(sysclkCheck)) {
            sysclkIpcExit();
        }
        // Exit services
        clkrstExit();
        pcvExit();
        tsExit();
        tcExit();
        pwmChannelSessionClose(&g_ICon);
        pwmExit();
        i2cExit();
        psmExit();
        nvClose(fd);
        nvExit();
        apmExit();
    }

    // **Override onShow** so that as soon as this Overlay appears, we let input pass through.
    virtual void onShow() override {
        // Request that Tesla stop grabbing all buttons/touches
        tsl::hlp::requestForeground(false);

        // (Optional) hide Tesla’s footer if you don’t want it
        deactivateOriginalFooter = true;
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        // Immediately show your MiniOverlay page
        return initially<MiniOverlay>();
    }
};

// Helper function to check if overlay file exists
bool checkOverlayFile(const std::string& filename) {
    struct stat buffer;
    return stat(filename.c_str(), &buffer) == 0;
}

// Helper function to setup micro mode paths
void setupMiniMode() {
    ult::DefaultFramebufferWidth = 1280;
    ult::DefaultFramebufferHeight = 720;

    // Try user-specified filename first, then fallback to default
    const std::string primaryPath = folderpath + filename;
    
    if (checkOverlayFile(primaryPath)) {
        filepath = primaryPath;
    } else {
        const std::string fallbackPath = folderpath + "Status-Monitor-Overlay.ovl";
        if (checkOverlayFile(fallbackPath)) {
            filepath = fallbackPath;
        }
    }
}

void setupMicroMode() {
    ult::DefaultFramebufferWidth = 1280;
    //ult::DefaultFramebufferHeight = 28;
    ult::DefaultFramebufferHeight = 720;
    
    // Try user-specified filename first, then fallback to default
    const std::string primaryPath = folderpath + filename;
    
    if (checkOverlayFile(primaryPath)) {
        filepath = primaryPath;
    } else {
        const std::string fallbackPath = folderpath + "Status-Monitor-Overlay.ovl";
        if (checkOverlayFile(fallbackPath)) {
            filepath = fallbackPath;
        }
    }
}

// This function gets called on startup to create a new Overlay object
int main(int argc, char **argv) {
    systemtickfrequency = armGetSystemTickFreq();
    ParseIniFile(); // parse INI from file
    
    if (argc > 0) {
        filename = argv[0]; // set global
        
        // Read the entire INI file once
        auto iniData = ult::getParsedDataFromIniFile(ult::OVERLAYS_INI_FILEPATH);
        
        // Check if mode_args exists in memory
        bool usingModeArgs = false;
        auto sectionIt = iniData.find(filename);
        if (sectionIt != iniData.end()) {
            auto keyIt = sectionIt->second.find("mode_args");
            usingModeArgs = (keyIt != sectionIt->second.end() && !keyIt->second.empty());
        }
        
        if (!usingModeArgs) {
            // Modify in memory (no file I/O)
            iniData[filename]["mode_args"] = "(-mini, -micro)";
            iniData[filename]["mode_labels"] = "(Mini, Micro)";
            
            // Write once with all changes
            ult::saveIniFileData(ult::OVERLAYS_INI_FILEPATH, iniData);
        }
    
    
        //ult::useRightAlignment = (ult::parseValueFromIniSection(ult::ULTRAHAND_CONFIG_INI_PATH, ult::ULTRAHAND_PROJECT_NAME, "right_alignment") == ult::TRUE_STR);
        // Check command line arguments
        for (u8 arg = 0; arg < argc; arg++) {
            if (argv[arg][0] != '-') continue;  // Check first character
            
            if (strcasecmp(argv[arg], "--microOverlay") == 0) {
                FullMode = false;
                lastMode = "micro";
                setupMicroMode();
                return tsl::loop<MicroMode>(argc, argv);
            } 
            else if (strcasecmp(argv[arg], "--miniOverlay") == 0) {
                FullMode = false;
                lastMode = "mini";
                setupMiniMode();
                //ult::useRightAlignment = ult::useRightAlignment || (ult::parseValueFromIniSection(configIniPath, "mini", "right_alignment") == ult::TRUE_STR);
                return tsl::loop<MiniEntryOverlay>(argc, argv);
            } 
            else if (strcasecmp(argv[arg], "-micro") == 0) {
                FullMode = false;
                skipMain = true;
                lastMode = "micro";
                ult::DefaultFramebufferWidth = 1280;
                //ult::DefaultFramebufferHeight = 28;
                ult::DefaultFramebufferHeight = 720;
                return tsl::loop<MicroMode>(argc, argv);
            } 
            else if (strcasecmp(argv[arg], "-mini") == 0) {
                FullMode = false;
                skipMain = true;
                lastMode = "mini";
                ult::DefaultFramebufferWidth = 1280;
                ult::DefaultFramebufferHeight = 720;
                //ult::useRightAlignment = (ult::parseValueFromIniSection(configIniPath, "mini", "right_alignment") == ult::TRUE_STR);
                return tsl::loop<MiniEntryOverlay>(argc, argv);
            }
            else if (strcasecmp(argv[arg], "--lastSelectedItem") == 0) {
                // Check if there's a next argument for the item name
                if (arg + 1 < argc) {
                    lastSelectedItem = argv[arg + 1];
                    arg++; // Skip the next argument since we've consumed it
                }
                // Don't return here, continue processing other arguments
            }
        }
    }
    
    // Default case
    return tsl::loop<MonitorOverlay>(argc, argv);
}
