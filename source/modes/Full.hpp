class MainMenu;

class FullOverlay : public tsl::Gui {
private:
    char DeltaCPU_c[12] = "";
    char DeltaGPU_c[12] = "";
    char DeltaRAM_c[12] = "";
    char RealCPU_Hz_c[64] = "";
    char RealGPU_Hz_c[64] = "";
    char RealRAM_Hz_c[64] = "";
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_compressed_c[64] = "";
    char RAM_var_compressed_c[128] = "";
    char RAM_percentage_var_compressed_c[128] = "";
    char CPU_Hz_c[64] = "";
    char GPU_Hz_c[64] = "";
    char RAM_Hz_c[64] = "";
    char CPU_compressed_c[160] = "";
    char SoCPCB_temperature_c[64] = "";
    char skin_temperature_c[32] = "";
    char BatteryDraw_c[64] = "";
    char FPS_var_compressed_c[64] = "";
    char RAM_load_c[64] = "";
    char Resolutions_c[64] = "";
    char readSpeed_c[32] = "";
    
    // New separated value buffers for CPU cores
    char CPU_Core0_c[16] = "";
    char CPU_Core1_c[16] = "";
    char CPU_Core2_c[16] = "";
    char CPU_Core3_c[16] = "";
    
    // New separated value buffers for FPS
    char PFPS_value_c[16] = "";
    char FPS_value_c[16] = "";

    static constexpr uint8_t COMMON_MARGIN = 20;
    FullSettings settings;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    std::string formattedKeyCombo = keyCombo;
    std::string message;
    const std::vector<std::string> KEY_SYMBOLS = {
        "\uE0E4", "\uE0E5", "\uE0E6", "\uE0E7",
        "\uE0E8", "\uE0E9", "\uE0ED", "\uE0EB",
        "\uE0EE", "\uE0EC", "\uE0E0", "\uE0E1",
        "\uE0E2", "\uE0E3", "\uE08A", "\uE08B",
        "\uE0B6", "\uE0B5"
    };

    bool skipOnce = true;
    bool runOnce = true;

    bool originalUseRightAlignment = ult::useRightAlignment;
public:
    FullOverlay() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        tsl::hlp::requestForeground(false);
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        idletick0.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick1.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick2.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick3.store(systemtickfrequency_impl, std::memory_order_relaxed);
        if (settings.setPosRight) {
            const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
            tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
            ult::useRightAlignment = true;
        } else {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
            ult::useRightAlignment = false;
        }
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        formatButtonCombination(formattedKeyCombo);
        //message = "Press " + formattedKeyCombo + " to Exit";

        realVoltsPolling = false;
        StartThreads();
    }
    ~FullOverlay() {
        CloseThreads();
        fixForeground = true;
        ult::useRightAlignment = originalUseRightAlignment;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;

    virtual tsl::elm::Element* createUI() override {
        

        auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            //static auto targetFreqWidth = renderer->getTextDimensions("Target Frequency", false, 15).first;
            //static auto realFreqWidth = renderer->getTextDimensions("Real Frequency", false, 15).first;
            //static auto freqWidth = std::max(targetFreqWidth, realFreqWidth);

            //static auto batteryLabelWidth = renderer->getTextDimensions("Battery Power Flow", false, 15).first;
            //static auto fanLabelWidth = renderer->getTextDimensions("Fan Rotation Level", false, 15).first;
            //static auto boardWidth = std::max(batteryLabelWidth, fanLabelWidth);

            static constexpr size_t valueOffset = 150;
            static constexpr size_t deltaOffset = 246;
            static constexpr size_t ramPercentageOffset = 350;

            //Print strings
            ///CPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {

                uint32_t height_offset = 155;
                if (realCPU_Hz && settings.showRealFreqs) {
                    height_offset = 162;
                }
                renderer->drawString("CPU Usage", false, COMMON_MARGIN, 120, 20, (settings.catColor1));
                if (settings.showTargetFreqs) {
                    //static auto targetFreqWidth = renderer->getTextDimensions("Target Frequency: ", false, 15).first;
                    renderer->drawString("Target Frequency", false, COMMON_MARGIN, height_offset, 15, (settings.catColor2));
                    renderer->drawString(CPU_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset, 15, (settings.textColor));
                }
                if (realCPU_Hz && settings.showRealFreqs) {
                    //static auto realFreqWidth = renderer->getTextDimensions("Real Frequency: ", false, 15).first;
                    renderer->drawString("Real Frequency", false, COMMON_MARGIN, height_offset - 15, 15, (settings.catColor2));
                    renderer->drawString(RealCPU_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset - 15, 15, (settings.textColor));
                    if (settings.showDeltas && settings.showTargetFreqs) {
                        renderer->drawString(DeltaCPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 7, 15, (settings.textColor));
                    }
                    else if (settings.showDeltas && !settings.showTargetFreqs) {
                        renderer->drawString(DeltaCPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 15, 15, (settings.textColor));
                    }
                }
                else if (realCPU_Hz && settings.showDeltas && (settings.showRealFreqs || settings.showTargetFreqs)) {
                    renderer->drawString(DeltaCPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset, 15, (settings.textColor));
                }
                
                // CPU Core labels and values
                static auto core0Width = renderer->getTextDimensions("Core 0  ", false, 15).first;
                static auto core1Width = renderer->getTextDimensions("Core 1  ", false, 15).first;
                static auto core2Width = renderer->getTextDimensions("Core 2  ", false, 15).first;
                static auto core3Width = renderer->getTextDimensions("Core 3  ", false, 15).first;
                
                const uint32_t core_height = height_offset + 30;
                renderer->drawString("Core 0  ", false, COMMON_MARGIN, core_height, 15, (settings.catColor2));
                renderer->drawString(CPU_Core0_c, false, COMMON_MARGIN + core0Width, core_height, 15, (settings.textColor));
                
                renderer->drawString("Core 1  ", false, COMMON_MARGIN, core_height + 15, 15, (settings.catColor2));
                renderer->drawString(CPU_Core1_c, false, COMMON_MARGIN + core1Width, core_height + 15, 15, (settings.textColor));
                
                renderer->drawString("Core 2  ", false, COMMON_MARGIN, core_height + 30, 15, (settings.catColor2));
                renderer->drawString(CPU_Core2_c, false, COMMON_MARGIN + core2Width, core_height + 30, 15, (settings.textColor));
                
                renderer->drawString("Core 3  ", false, COMMON_MARGIN, core_height + 45, 15, (settings.catColor2));
                renderer->drawString(CPU_Core3_c, false, COMMON_MARGIN + core3Width, core_height + 45, 15, (settings.textColor));
            }
            
            ///GPU
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(nvCheck)) {
                
                uint32_t height_offset = 320-8;
                if (realGPU_Hz && settings.showRealFreqs) {
                    height_offset = 327-8;
                }

                renderer->drawString("GPU Usage", false, COMMON_MARGIN, 285-8, 20, (settings.catColor1));
                if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                    if (settings.showTargetFreqs) { 
                        //static auto targetFreqWidth = renderer->getTextDimensions("Target Frequency: ", false, 15).first;
                        renderer->drawString("Target Frequency", false, COMMON_MARGIN, height_offset, 15, (settings.catColor2));
                        renderer->drawString(GPU_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset, 15, (settings.textColor));
                    }
                    if (realCPU_Hz && settings.showRealFreqs) {
                        //static auto realFreqWidth = renderer->getTextDimensions("Real Frequency: ", false, 15).first;
                        renderer->drawString("Real Frequency", false, COMMON_MARGIN, height_offset - 15, 15, (settings.catColor2));
                        renderer->drawString(RealGPU_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset - 15, 15, (settings.textColor));
                        if (settings.showDeltas && settings.showTargetFreqs) {
                            renderer->drawString(DeltaGPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 7, 15, (settings.textColor));
                        }
                        else if (settings.showDeltas && !settings.showTargetFreqs) {
                            renderer->drawString(DeltaGPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 15, 15, (settings.textColor));
                        }
                    }
                    else if (realGPU_Hz && settings.showDeltas && (settings.showRealFreqs || settings.showTargetFreqs)) {
                        renderer->drawString(DeltaGPU_c, false, COMMON_MARGIN +  deltaOffset, height_offset, 15, (settings.textColor));
                    }
                }
                if (R_SUCCEEDED(nvCheck)) {
                    //static auto loadWidth = renderer->getTextDimensions("Load: ", false, 15).first;
                    renderer->drawString("Load", false, COMMON_MARGIN, height_offset + 15, 15, (settings.catColor2));
                    renderer->drawString(GPU_Load_c, false, COMMON_MARGIN + valueOffset, height_offset + 15, 15, (settings.textColor));
                }
                
            }

            static std::vector<std::string> specialChars = {""};
            
            ///RAM
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(Hinted)) {
                
                uint32_t height_offset = 410;
                if (realRAM_Hz && settings.showRealFreqs) {
                    height_offset += 7;
                }

                renderer->drawString("RAM Usage", false, COMMON_MARGIN, 375, 20, (settings.catColor1));
                if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                    if (settings.showTargetFreqs) {
                        //static auto targetFreqWidth = renderer->getTextDimensions("Target Frequency: ", false, 15).first;
                        renderer->drawString("Target Frequency", false, COMMON_MARGIN, height_offset, 15, (settings.catColor2));
                        renderer->drawString(RAM_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset, 15, (settings.textColor));
                    }
                    if (realRAM_Hz && settings.showRealFreqs) {
                        //static auto realFreqWidth = renderer->getTextDimensions("Real Frequency: ", false, 15).first;
                        renderer->drawString("Real Frequency", false, COMMON_MARGIN, height_offset - 15, 15, (settings.catColor2));
                        renderer->drawString(RealRAM_Hz_c, false, COMMON_MARGIN + valueOffset, height_offset - 15, 15, (settings.textColor));
                        if (settings.showDeltas && settings.showTargetFreqs) {
                            renderer->drawString(DeltaRAM_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 7, 15, (settings.textColor));
                        }
                        else if (settings.showDeltas && !settings.showTargetFreqs) {
                            renderer->drawString(DeltaRAM_c, false, COMMON_MARGIN +  deltaOffset, height_offset - 15, 15, (settings.textColor));
                        }
                    }
                    else if (realRAM_Hz && settings.showDeltas && (settings.showRealFreqs || settings.showTargetFreqs)) {
                        renderer->drawString(DeltaRAM_c, false, COMMON_MARGIN +  deltaOffset, height_offset, 15, (settings.textColor));
                    }
                    if (R_SUCCEEDED(sysclkCheck)) {
                        //static auto loadLabelWidth = renderer->getTextDimensions("Load: ", false, 15).first;
                        renderer->drawString("Load", false, COMMON_MARGIN, height_offset+15, 15, (settings.catColor2));
                        renderer->drawStringWithColoredSections(RAM_load_c, false, specialChars, COMMON_MARGIN + valueOffset, height_offset+15, 15, (settings.textColor), settings.separatorColor);
                    }
                }
                if (R_SUCCEEDED(Hinted)) {
                    //static auto textWidth = renderer->getTextDimensions("Total \nApplication \nApplet \nSystem \nSystem Unsafe ", false, 15).first;
                    renderer->drawString("Total\nApplication\nApplet\nSystem\nSystem Unsafe", false, COMMON_MARGIN, height_offset + 40, 15, (settings.catColor2));
                    renderer->drawString(RAM_var_compressed_c, false, COMMON_MARGIN + valueOffset, height_offset + 40, 15, (settings.textColor));
                    renderer->drawString(RAM_percentage_var_compressed_c, false, ramPercentageOffset, height_offset + 40, 15, (settings.textColor));
                }
            }
            
            ///Thermal
            if (R_SUCCEEDED(i2cCheck) || R_SUCCEEDED(tcCheck) || R_SUCCEEDED(pwmCheck)) {
                renderer->drawString("Board", false, 20, 550, 20, (settings.catColor1));
                if (R_SUCCEEDED(i2cCheck)) {
                    //static auto batteryLabelWidth = renderer->getTextDimensions("Battery Power Flow: ", false, 15).first;
                    renderer->drawString("Battery Power Flow", false, COMMON_MARGIN, 575, 15, (settings.catColor2));
                    renderer->drawString(BatteryDraw_c, false, COMMON_MARGIN + valueOffset, 575, 15, (settings.textColor));
                }
                if (R_SUCCEEDED(i2cCheck) || R_SUCCEEDED(tcCheck)) {
                    //static auto textWidth1 = renderer->getTextDimensions("Temperatures ", false, 15).first;
                    static auto textWidth2 = renderer->getTextDimensions("SoC\nPCB\nSkin", false, 15).first;
                    renderer->drawString("\nTemperatures", false, COMMON_MARGIN, 590, 15, (settings.catColor2));
                    renderer->drawString("SoC\nPCB\nSkin", false, COMMON_MARGIN + valueOffset-textWidth2 - 10, 590, 15, (settings.catColor2));
                    renderer->drawString(SoCPCB_temperature_c, false, COMMON_MARGIN + valueOffset, 590, 15, (settings.textColor));
                }
                if (R_SUCCEEDED(pwmCheck)) {
                    //static auto fanLabelWidth = renderer->getTextDimensions("Fan Rotation Level ", false, 15).first;
                    renderer->drawString("Fan Rotation Level", false, COMMON_MARGIN, 635, 15, (settings.catColor2));
                    renderer->drawString(Rotation_SpeedLevel_c, false, COMMON_MARGIN + valueOffset, 635, 15, (settings.textColor));
                }
            }
            
            ///FPS
            if (GameRunning) {
                const uint32_t width_offset = 150;
                if (settings.showFPS || settings.showRES || settings.showRDSD) {
                    renderer->drawString("Game", false, COMMON_MARGIN + width_offset, 185+12, 20, (settings.catColor1));
                }
                uint32_t height = 210+12;
                if (settings.showFPS == true) {
                    static auto pfpsWidth = renderer->getTextDimensions("PFPS  ", false, 15).first;
                    static auto fpsWidth = renderer->getTextDimensions("FPS  ", false, 15).first;
                    
                    renderer->drawString("PFPS  ", false, COMMON_MARGIN + width_offset, height, 15, (settings.catColor2));
                    renderer->drawString(PFPS_value_c, false, COMMON_MARGIN + width_offset + pfpsWidth, height, 15, (settings.textColor));
                    
                    // Calculate position for "FPS " label - add some spacing
                    const uint32_t fps_x_offset = COMMON_MARGIN + width_offset + pfpsWidth + renderer->getTextDimensions(PFPS_value_c, false, 15).first + 15;
                    renderer->drawString("FPS  ", false, fps_x_offset, height, 15, (settings.catColor2));
                    renderer->drawString(FPS_value_c, false, fps_x_offset + fpsWidth, height, 15, (settings.textColor));
                    
                    height += 15;
                }
                if ((settings.showRES == true) && NxFps && SharedMemoryUsed && (NxFps -> API >= 1)) {
                    static auto resLabelWidth = renderer->getTextDimensions("Resolutions  ", false, 15).first;
                    renderer->drawString("Resolutions  ", false, COMMON_MARGIN + width_offset, height, 15, (settings.catColor2));
                    
                    renderer->drawStringWithColoredSections(Resolutions_c, false, specialChars, COMMON_MARGIN + width_offset + resLabelWidth, height, 15, (settings.textColor), settings.separatorColor);
                    height += 15;
                }
                if (settings.showRDSD == true) {
                    static auto readLabelWidth = renderer->getTextDimensions("Read Speed  ", false, 15).first;
                    renderer->drawString("Read Speed  ", false, COMMON_MARGIN + width_offset, height, 15, (settings.catColor2));
                    renderer->drawString(readSpeed_c, false, COMMON_MARGIN + width_offset + readLabelWidth, height, 15, (settings.textColor));
                }
            }
            
            //renderer->drawStringWithColoredSections(message, false, KEY_SYMBOLS, 30, 693, 23,  a(tsl::bottomTextColor), a(tsl::buttonColor));
            

            static const auto pressWidth = renderer->getTextDimensions("Press ", false, 23).first;
            static const auto keyComboWidth = renderer->getTextDimensions(formattedKeyCombo.c_str(), false, 23).first;
            
            static constexpr u16 baseX = 30;
            static constexpr u16 baseY = 693;
            static constexpr u8 fontSize = 23;

            // Draw "Press "
            renderer->drawString("Press ", false, baseX, baseY, fontSize, (tsl::bottomTextColor));
            
            // Draw formatted key combo with colored sections
            renderer->drawStringWithColoredSections(formattedKeyCombo, false, KEY_SYMBOLS, baseX + pressWidth, baseY, fontSize, (tsl::bottomTextColor), (tsl::buttonColor));
            
            // Draw " to Exit"
            renderer->drawString(" to Exit", false, baseX + pressWidth + keyComboWidth, baseY, fontSize, (tsl::bottomTextColor));
        });
        
        auto rootFrame = new tsl::elm::HeaderOverlayFrame("Status Monitor", APP_VERSION);
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        //Make stuff ready to print
        ///CPU
        if (systemtickfrequency_impl > 0) {
           const uint64_t idle0_val = std::min(idletick0.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle1_val = std::min(idletick1.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle2_val = std::min(idletick2.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle3_val = std::min(idletick3.load(std::memory_order_acquire), systemtickfrequency_impl);
           
           const float usage0 = std::clamp(100.0f * (1.0f - float(idle0_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage1 = std::clamp(100.0f * (1.0f - float(idle1_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage2 = std::clamp(100.0f * (1.0f - float(idle2_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage3 = std::clamp(100.0f * (1.0f - float(idle3_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           
           // Format individual core values
           snprintf(CPU_Core0_c, sizeof(CPU_Core0_c), "%.2f%%", usage0);
           snprintf(CPU_Core1_c, sizeof(CPU_Core1_c), "%.2f%%", usage1);
           snprintf(CPU_Core2_c, sizeof(CPU_Core2_c), "%.2f%%", usage2);
           snprintf(CPU_Core3_c, sizeof(CPU_Core3_c), "%.2f%%", usage3);
        }

        mutexLock(&mutex_Misc);
        snprintf(CPU_Hz_c, sizeof(CPU_Hz_c), "%u.%u MHz", CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
        if (realCPU_Hz) {
            snprintf(RealCPU_Hz_c, sizeof(RealCPU_Hz_c), "%u.%u MHz", realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
            const int32_t deltaCPU = (int32_t)(realCPU_Hz / 1000) - (CPU_Hz / 1000);
            snprintf(DeltaCPU_c, sizeof(DeltaCPU_c), "Δ %d.%u", deltaCPU / 1000, abs(deltaCPU / 100) % 10);
        }
        
        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "%u.%u MHz", GPU_Hz / 1000000, (GPU_Hz / 100000) % 10);
        if (realGPU_Hz) {
            snprintf(RealGPU_Hz_c, sizeof(RealGPU_Hz_c), "%u.%u MHz", realGPU_Hz / 1000000, (realGPU_Hz / 100000) % 10);
            const int32_t deltaGPU = (int32_t)(realGPU_Hz / 1000) - (GPU_Hz / 1000);
            snprintf(DeltaGPU_c, sizeof(DeltaGPU_c), "Δ %d.%u", deltaGPU / 1000, abs(deltaGPU / 100) % 10);
        }
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "%u.%u%%", GPU_Load_u / 10, GPU_Load_u % 10);
        
        ///RAM
        snprintf(RAM_Hz_c, sizeof RAM_Hz_c, "%u.%u MHz", RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
        if (realRAM_Hz) {
            snprintf(RealRAM_Hz_c, sizeof(RealRAM_Hz_c), "%u.%u MHz", realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
            const int32_t deltaRAM = (int32_t)(realRAM_Hz / 1000) - (RAM_Hz / 1000);
            snprintf(DeltaRAM_c, sizeof(DeltaRAM_c), "Δ %d.%u", deltaRAM / 1000, abs(deltaRAM / 100) % 10);
        }

        const float RAM_Total_application_f    = (float)RAM_Total_application_u / 1024 / 1024;
        const float RAM_Total_applet_f         = (float)RAM_Total_applet_u / 1024 / 1024;
        const float RAM_Total_system_f         = (float)RAM_Total_system_u / 1024 / 1024;
        const float RAM_Total_systemunsafe_f   = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        const float RAM_Total_all_f            = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        
        const float RAM_Used_application_f     = (float)RAM_Used_application_u / 1024 / 1024;
        const float RAM_Used_applet_f          = (float)RAM_Used_applet_u / 1024 / 1024;
        const float RAM_Used_system_f          = (float)RAM_Used_system_u / 1024 / 1024;
        const float RAM_Used_systemunsafe_f    = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        const float RAM_Used_all_f             = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        
        // Compute percentages
        const int RAMPct_all          = (int)((RAM_Used_all_f          / RAM_Total_all_f)        * 100.0f );
        const int RAMPct_app          = (int)((RAM_Used_application_f / RAM_Total_application_f) * 100.0f );
        const int RAMPct_applet       = (int)((RAM_Used_applet_f      / RAM_Total_applet_f)      * 100.0f );
        const int RAMPct_system       = (int)((RAM_Used_system_f      / RAM_Total_system_f)      * 100.0f );
        const int RAMPct_systemunsafe = (int)((RAM_Used_systemunsafe_f/ RAM_Total_systemunsafe_f)* 100.0f );
        
        snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c),
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB",
            RAM_Used_all_f,          RAM_Total_all_f,
            RAM_Used_application_f,  RAM_Total_application_f,
            RAM_Used_applet_f,       RAM_Total_applet_f,
            RAM_Used_system_f,       RAM_Total_system_f,
            RAM_Used_systemunsafe_f, RAM_Total_systemunsafe_f
        );
        
        // 2. Percentages only (newlines preserved)
        snprintf(RAM_percentage_var_compressed_c, sizeof(RAM_percentage_var_compressed_c),
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)",
            RAMPct_all,
            RAMPct_app,
            RAMPct_applet,
            RAMPct_system,
            RAMPct_systemunsafe
        );
        
        if (R_SUCCEEDED(sysclkCheck)) {
            const int RAM_GPU_Load = ramLoad[SysClkRamLoad_All] - ramLoad[SysClkRamLoad_Cpu];
            snprintf(RAM_load_c, sizeof RAM_load_c, 
                "%u.%u%% (CPU %u.%u%%GPU %u.%u%%)",
                ramLoad[SysClkRamLoad_All] / 10, ramLoad[SysClkRamLoad_All] % 10,
                ramLoad[SysClkRamLoad_Cpu] / 10, ramLoad[SysClkRamLoad_Cpu] % 10,
                RAM_GPU_Load / 10, RAM_GPU_Load % 10);
        }
        ///Thermal
        snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c, 
            "%.1f\u00B0C\n%.1f\u00B0C\n%d.%d\u00B0C", 
            SOC_temperatureF, PCB_temperatureF, skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%.1f%%", Rotation_Duty);
        
        ///FPS
        if (settings.showFPS == true) {
            snprintf(PFPS_value_c, sizeof PFPS_value_c, "%1u", FPS);
            snprintf(FPS_value_c, sizeof FPS_value_c, "%.1f", useOldFPSavg ? FPSavg_old : FPSavg);
        }

        //Resolutions
        if ((settings.showRES == true) && GameRunning && NxFps) {
            if (!resolutionLookup) {
                (NxFps -> renderCalls[0].calls) = 0xFFFF;
                resolutionLookup = 1;
            }
            else if (resolutionLookup == 1) {
                if ((NxFps -> renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
            }
            else {
                if (NxFps && SharedMemoryUsed) {
                    memcpy(&m_resolutionRenderCalls, &(NxFps -> renderCalls), sizeof(m_resolutionRenderCalls));
                    memcpy(&m_resolutionViewportCalls, &(NxFps -> viewportCalls), sizeof(m_resolutionViewportCalls));
                } else {
                    memset(&m_resolutionRenderCalls, 0, sizeof(m_resolutionRenderCalls));
                    memset(&m_resolutionViewportCalls, 0, sizeof(m_resolutionViewportCalls));
                }
                qsort(m_resolutionRenderCalls, 8, sizeof(resolutionCalls), compare);
                qsort(m_resolutionViewportCalls, 8, sizeof(resolutionCalls), compare);
                memset(&m_resolutionOutput, 0, sizeof(m_resolutionOutput));
                size_t out_iter = 0;
                bool found = false;
                for (size_t i = 0; i < 8; i++) {
                    for (size_t x = 0; x < 8; x++) {
                        if (m_resolutionRenderCalls[i].width == 0) {
                            break;
                        }
                        if ((m_resolutionRenderCalls[i].width == m_resolutionViewportCalls[x].width) && (m_resolutionRenderCalls[i].height == m_resolutionViewportCalls[x].height)) {
                            m_resolutionOutput[out_iter].width = m_resolutionRenderCalls[i].width;
                            m_resolutionOutput[out_iter].height = m_resolutionRenderCalls[i].height;
                            m_resolutionOutput[out_iter].calls = (m_resolutionRenderCalls[i].calls > m_resolutionViewportCalls[x].calls) ? m_resolutionRenderCalls[i].calls : m_resolutionViewportCalls[x].calls;
                            out_iter++;
                            found = true;
                            break;
                        }
                    }
                    if (!found && m_resolutionRenderCalls[i].width != 0) {
                        m_resolutionOutput[out_iter].width = m_resolutionRenderCalls[i].width;
                        m_resolutionOutput[out_iter].height = m_resolutionRenderCalls[i].height;
                        m_resolutionOutput[out_iter].calls = m_resolutionRenderCalls[i].calls;
                        out_iter++;
                    }
                    found = false;
                    if (out_iter == 8) break;
                }
                if (out_iter < 8) {
                    const size_t out_iter_s = out_iter;
                    for (size_t x = 0; x < 8; x++) {
                        for (size_t y = 0; y < out_iter_s; y++) {
                            if (m_resolutionViewportCalls[x].width == 0) {
                                break;
                            }
                            if ((m_resolutionViewportCalls[x].width == m_resolutionOutput[y].width) && (m_resolutionViewportCalls[x].height == m_resolutionOutput[y].height)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found && m_resolutionViewportCalls[x].width != 0) {
                            m_resolutionOutput[out_iter].width = m_resolutionViewportCalls[x].width;
                            m_resolutionOutput[out_iter].height = m_resolutionViewportCalls[x].height;
                            m_resolutionOutput[out_iter].calls = m_resolutionViewportCalls[x].calls;
                            out_iter++;         
                        }
                        found = false;
                        if (out_iter == 8) break;
                    }
                }
                qsort(m_resolutionOutput, 8, sizeof(resolutionCalls), compare);
                static std::pair<uint16_t, uint16_t> old_res[2];
                
                // Only swap if BOTH resolutions exist (prevent swapping with empty slot)
                if (m_resolutionOutput[0].width && m_resolutionOutput[1].width) {
                    if ((m_resolutionOutput[0].width == old_res[1].first && m_resolutionOutput[0].height == old_res[1].second) || 
                        (m_resolutionOutput[1].width == old_res[0].first && m_resolutionOutput[1].height == old_res[0].second)) {
                        const uint16_t swap_width = m_resolutionOutput[0].width;
                        const uint16_t swap_height = m_resolutionOutput[0].height;
                        m_resolutionOutput[0].width = m_resolutionOutput[1].width;
                        m_resolutionOutput[0].height = m_resolutionOutput[1].height;
                        m_resolutionOutput[1].width = swap_width;
                        m_resolutionOutput[1].height = swap_height;
                    }
                }
                
                //if (!m_resolutionOutput[1].width) {
                //    snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                //}
                //else {
                //    snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                //}

                if (!m_resolutionOutput[1].width || !m_resolutionOutput[0].width) {
                    if (!m_resolutionOutput[1].width)
                        snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                    else snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                }
                else snprintf(Resolutions_c, sizeof(Resolutions_c),"%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                
                old_res[0] = std::make_pair(m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                old_res[1] = std::make_pair(m_resolutionOutput[1].width, m_resolutionOutput[1].height);
            }
            if (settings.showRDSD == true && GameRunning && NxFps) {
                if ((NxFps -> readSpeedPerSecond) != 0.f) snprintf(readSpeed_c, sizeof(readSpeed_c), "%.2f MiB/s", (NxFps -> readSpeedPerSecond) / 1048576.f);
                else snprintf(readSpeed_c, sizeof(readSpeed_c), "n/d");
            }
        }
        else if (!GameRunning && resolutionLookup != 0) {
            resolutionLookup = 0;
        }

        mutexUnlock(&mutex_Misc);

        //Battery Power Flow
        char remainingBatteryLife[8];
        mutexLock(&mutex_BatteryChecker);
        if (batTimeEstimate >= 0) {
            snprintf(remainingBatteryLife, sizeof remainingBatteryLife, "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        }
        else snprintf(remainingBatteryLife, sizeof remainingBatteryLife, "-:--");
        snprintf(BatteryDraw_c, sizeof BatteryDraw_c, "%+.2f W [%s]", PowerConsumption, remainingBatteryLife);
        mutexUnlock(&mutex_BatteryChecker);
        
        if (!skipOnce) {
            if (runOnce) {
                isRendering = true;
                leventClear(&renderingStopEvent);
                runOnce = false;
            }
        } else {
            skipOnce = false;
        }
    }


    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (isKeyComboPressed(keysHeld, keysDown)) {
            isRendering = false;
            leventSignal(&renderingStopEvent);
            triggerRumbleDoubleClick.store(true, std::memory_order_release);
            triggerExitSound.store(true, std::memory_order_release);
            skipOnce = true;
            runOnce = true;
            TeslaFPS = 60;
            lastSelectedItem = "Full";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            return true;
        }
        return false;
    }
};