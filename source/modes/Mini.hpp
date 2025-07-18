class MiniOverlay : public tsl::Gui {
private:
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_var_compressed_c[128] = "";
    char SoCPCB_temperature_c[64] = "";
    char soc_temperature_c[64] = "";
    char skin_temperature_c[64] = "";

    uint32_t rectangleWidth;
    char Variables[512];
    size_t fontsize;
    MiniSettings settings;
    bool Initialized = false;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
public:
    MiniOverlay() { 
        disableJumpTo = true;
        //tsl::initializeUltrahandSettings();
        PowerConsumption = 0.0f;
        batTimeEstimate = -1;
        strcpy(SoCPCB_temperature_c, "-.-- W-.-% [--:--]"); // Default display

        GetConfigSettings(&settings);
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else fontsize = settings.dockedFontSize;
        switch(settings.setPos) {
            case 1:
            case 4:
            case 7:
                tsl::gfx::Renderer::get().setLayerPos(624, 0);
                break;
            case 2:
            case 5:
            case 8:
                tsl::gfx::Renderer::get().setLayerPos(1248, 0);
                break;
        }
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        //alphabackground = 0x0;
        tsl::hlp::requestForeground(false);
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        deactivateOriginalFooter = true;
        StartThreads();
    }
    ~MiniOverlay() {

        CloseThreads();
        FullMode = true;
        tsl::hlp::requestForeground(true);
        //alphabackground = 0xD;
        deactivateOriginalFooter = false;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;
    bool resolutionShow = false;

    virtual tsl::elm::Element* createUI() override {
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("", "");
    
        auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            const u16 frameWidth = 448;
            
            // Cache parsed settings and calculations
            static std::vector<std::string> showKeys;
            static std::string lastShowSetting;
            static bool needsRecalc = true;
            static std::vector<std::string> labelLines; // Store individual label lines
            static std::string lastVariables; // Track changes in Variables content
            static size_t entryCount = 0;
            static uint32_t cachedHeight = 0;
            static int cachedBaseX = 0, cachedBaseY = 0;
            static bool lastGameRunning = false; // Track game state changes
            
            // Check if we need to recalculate due to content changes
            bool contentChanged = (std::string(Variables) != lastVariables) || 
                                (GameRunning != lastGameRunning);
            
            // Only recalculate if settings changed or content changed
            if (settings.show != lastShowSetting || !Initialized || contentChanged) {
                showKeys.clear();
                
                // Parse once and cache
                size_t start = 0, end = 0;
                const std::string& show = settings.show;
                while ((end = show.find('+', start)) != std::string::npos) {
                    showKeys.emplace_back(show.substr(start, end - start));
                    start = end + 1;
                }
                if (start < show.length()) {
                    showKeys.emplace_back(show.substr(start));
                }
                
                lastShowSetting = settings.show;
                lastVariables = Variables;
                lastGameRunning = GameRunning;
                needsRecalc = true;
            }
            
            // Initial width calculation (only once)
            if (!Initialized) {
                
                rectangleWidth = 0;
                //std::pair<u32, u32> dimensions;
                u32 width;
                
                for (const auto& key : showKeys) {
                    if (key == "CPU") {
                        //dimensions = renderer->drawString("[100%,100%,100%,100%]@4444.4", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (!settings.realVolts) {
                            if (settings.showFullCPU)
                                width = renderer->getTextDimensions("[100%,100%,100%,100%]@4444.4", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("100%@4444.4", false, fontsize).first;
                        } else {
                            if (settings.showFullCPU)
                                width = renderer->getTextDimensions("[100%,100%,100%,100%]@4444.4444 mV", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("100%@4444.4444 mV", false, fontsize).first;
                        }
                    } else if (key == "GPU" || (key == "RAM" && settings.showRAMLoad && R_SUCCEEDED(sysclkCheck))) {
                        //dimensions = renderer->drawString("100.0%@4444.4", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (!settings.realVolts) {
                            width = renderer->getTextDimensions("100%@4444.4", false, fontsize).first;
                        } else {
                            width = renderer->getTextDimensions("100%@4444.4444 mV", false, fontsize).first;
                        }
                    } else if (key == "RAM" && (!settings.showRAMLoad || R_FAILED(sysclkCheck))) {
                        //dimensions = renderer->drawString("44444444MB@4444.4", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (!settings.realVolts) {
                            width = renderer->getTextDimensions("100%@4444.4", false, fontsize).first;
                        } else {
                            width = renderer->getTextDimensions("100%@4444.44444.4 mV", false, fontsize).first;
                        }
                    } else if (key == "SOC") {                // new block
                        if (!settings.realVolts)
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88°C (100%)", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88°C", false, fontsize).first;
                        else
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88°C (100%)444 mV", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88°C444 mV", false, fontsize).first;
                    } else if (key == "TMP") {
                        //dimensions = renderer->drawString("88.8\u00B0C88.8\u00B0C88.8\u00B0C (100%)", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (!settings.realVolts) {
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88\u00B0C88\u00B0C88\u00B0C (100%)", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88\u00B0C88\u00B0C88\u00B0C", false, fontsize).first;
                        } else {
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88\u00B0C88\u00B0C88\u00B0C4444.4 mV", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88\u00B0C88\u00B0C88\u00B0C4444.4 mV", false, fontsize).first;
                        }
                    } else if (key == "BAT") {
                        //dimensions = renderer->drawString("-44.44 W100.0% [44:44]", false, 0, 0, fontsize, renderer->a(0x0000));
                        width = renderer->getTextDimensions("-44.44 W100.0% [44:44]", false, fontsize).first;
                    } else if (key == "FPS") {
                        //dimensions = renderer->drawString("444.4", false, 0, 0, fontsize, renderer->a(0x0000));
                        width = renderer->getTextDimensions("444.4", false, fontsize).first;
                    } else if (key == "RES") {
                        //dimensions = renderer->drawString("3840x21603840x2160", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (settings.showFullResolution)
                            width = renderer->getTextDimensions("3840x21603840x2160", false, fontsize).first;
                        else
                            width = renderer->getTextDimensions("2160p2160p", false, fontsize).first;
                    } else {
                        continue;
                    }
                    
                    if (rectangleWidth < width) {
                        rectangleWidth = width;
                    }
                }
                Initialized = true;
                needsRecalc = true;
            }
            
            // Recalculate layout when needed (including content changes)
            if (needsRecalc) {
                // Build label lines array for individual centering
                labelLines.clear();
                entryCount = 0;
                uint8_t flags = 0;
                
                bool shouldAdd;
                std::string labelText;
                for (const auto& key : showKeys) {
                    shouldAdd = false;
                    labelText = "";
                    
                    if (key == "CPU" && !(flags & 1)) {
                        shouldAdd = true;
                        labelText = "CPU";
                        flags |= 1;
                    } else if (key == "GPU" && !(flags & 2)) {
                        shouldAdd = true;
                        labelText = "GPU";
                        flags |= 2;
                    } else if (key == "RAM" && !(flags & 4)) {
                        shouldAdd = true;
                        labelText = "RAM";
                        flags |= 4;
                    } else if (key == "SOC" && !(flags & 8)) {
                        shouldAdd = true;
                        labelText = "SOC";
                        flags |= 8;
                    } else if (key == "TMP" && !(flags & 16)) {
                        shouldAdd = true;
                        labelText = "TMP";
                        flags |= 16;
                    } else if ((key == "BAT" || key == "DRAW") && !(flags & 32)) {
                        shouldAdd = true;
                        labelText = "BAT";
                        flags |= 32;
                    } else if (key == "FPS" && !(flags & 64) && GameRunning) {
                        shouldAdd = true;
                        labelText = "FPS";
                        flags |= 64;
                    } else if (key == "RES" && !(flags & 128) && GameRunning) {
                        shouldAdd = true;
                        labelText = "RES";
                        flags |= 128;
                        resolutionShow = true;
                    }
                    
                    if (shouldAdd) {
                        labelLines.push_back(labelText);
                        entryCount++;
                        
                        //if (settings.realVolts && key != "BAT" && key != "DRAW" && key != "FPS" && key != "RES") {
                        //    labelLines.push_back(""); // Empty line for voltage info
                        //    entryCount++;
                        //}
                    }
                }
                
                // Calculate actual entry count from Variables string
                size_t actualEntryCount = 1; // Start with 1 for the first line
                for (size_t i = 0; Variables[i] != '\0'; i++) {
                    if (Variables[i] == '\n') {
                        actualEntryCount++;
                    }
                }
                
                // Use the actual entry count for height calculation
                cachedHeight = ((fontsize + settings.spacing) * actualEntryCount) + (fontsize / 3) + settings.spacing;
                uint32_t margin = (fontsize * 4);
                
                cachedBaseX = 0;
                cachedBaseY = 0;
                
                if (ult::useRightAlignment) {
                    cachedBaseX = frameWidth - (margin + rectangleWidth + (fontsize / 3));
                } else {
                    switch (settings.setPos) {
                        case 1:
                            cachedBaseX = 224 - ((margin + rectangleWidth + (fontsize / 3)) / 2);
                            break;
                        case 4:
                            cachedBaseX = 224 - ((margin + rectangleWidth + (fontsize / 3)) / 2);
                            cachedBaseY = 360 - cachedHeight / 2;
                            break;
                        case 7:
                            cachedBaseX = 224 - ((margin + rectangleWidth + (fontsize / 3)) / 2);
                            cachedBaseY = 720 - cachedHeight;
                            break;
                        case 2:
                            cachedBaseX = 448 - (margin + rectangleWidth + (fontsize / 3));
                            break;
                        case 5:
                            cachedBaseX = 448 - (margin + rectangleWidth + (fontsize / 3));
                            cachedBaseY = 360 - cachedHeight / 2;
                            break;
                        case 8:
                            cachedBaseX = 448 - (margin + rectangleWidth + (fontsize / 3));
                            cachedBaseY = 720 - cachedHeight;
                            break;
                    }
                }
                needsRecalc = false;
            }
            
            // Fast rendering using cached values
            uint32_t margin = (fontsize * 4);
            
            // Draw background
            renderer->drawRect(cachedBaseX, cachedBaseY, margin + rectangleWidth + (fontsize / 3) + settings.spacing, cachedHeight, renderer->a(settings.backgroundColor));
            
            // Split Variables into lines for individual positioning
            std::vector<std::string> variableLines;
            std::string variablesStr(Variables);
            size_t start = 0;
            size_t pos = 0;
            while ((pos = variablesStr.find('\n', start)) != std::string::npos) {
                variableLines.push_back(variablesStr.substr(start, pos - start));
                start = pos + 1;
            }
            if (start < variablesStr.length()) {
                variableLines.push_back(variablesStr.substr(start));
            }
            
            // Draw each label and variable line individually
            uint32_t currentY = cachedBaseY + fontsize + settings.spacing;
            size_t labelIndex = 0;
            
            uint32_t labelWidth, labelCenterX;
            for (size_t i = 0; i < variableLines.size() && labelIndex < labelLines.size(); i++) {
                // Draw label (centered in label region)
                if (!labelLines[labelIndex].empty()) {
                    //std::pair<u32, u32> labelDimensions = renderer->drawString(labelLines[labelIndex].c_str(), false, 0, 0, fontsize, renderer->a(0x0000));
                    //u32 width = renderer->getTextDimensions(labelLines[labelIndex].c_str(), fontsize);
                    labelWidth = renderer->getTextDimensions(labelLines[labelIndex].c_str(), false, fontsize).first;
                    labelCenterX = cachedBaseX + (margin / 2) - (labelWidth / 2);
                    renderer->drawString(labelLines[labelIndex].c_str(), false, labelCenterX, currentY, fontsize, settings.catColor);
                }
                
                // Draw variable data
                //renderer->drawString(variableLines[i].c_str(), false, cachedBaseX + margin, currentY, fontsize, renderer->a(settings.textColor));
                //renderer->drawStringWith(variableLines[i].c_str(), false, cachedBaseX + margin, currentY, fontsize, renderer->a(settings.textColor));
                renderer->drawStringWithColoredSections(variableLines[i].c_str(), false, {""}, cachedBaseX + margin, currentY, fontsize, settings.textColor, a(settings.separatorColor));

                currentY += fontsize + settings.spacing;   // previously += fontsize
                ++labelIndex;
            }
        });
    
        rootFrame->setContent(Status);
        return rootFrame;
    }

    virtual void update() override {
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            if (fontsize != settings.handheldFontSize) {
                Initialized = false;
                fontsize = settings.handheldFontSize;
            }
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            if (fontsize != settings.dockedFontSize) {
                Initialized = false;
                fontsize = settings.dockedFontSize;
            }
        }

        //Make stuff ready to print
        ///CPU

        char MINI_CPU_Usage0[7];
        char MINI_CPU_Usage1[7];
        char MINI_CPU_Usage2[7];
        char MINI_CPU_Usage3[7];

        if (idletick0 > systemtickfrequency_impl)
            strcpy(MINI_CPU_Usage0, "0%");
        else snprintf(MINI_CPU_Usage0, sizeof(MINI_CPU_Usage0), "%.0f%%", (1.d - ((double)idletick0 / systemtickfrequency_impl)) * 100);
        if (idletick1 > systemtickfrequency_impl)
            strcpy(MINI_CPU_Usage1, "0%");
        else snprintf(MINI_CPU_Usage1, sizeof(MINI_CPU_Usage1), "%.0f%%", (1.d - ((double)idletick1 / systemtickfrequency_impl)) * 100);
        if (idletick2 > systemtickfrequency_impl)
            strcpy(MINI_CPU_Usage2, "0%");
        else snprintf(MINI_CPU_Usage2, sizeof(MINI_CPU_Usage2), "%.0f%%", (1.d - ((double)idletick2 / systemtickfrequency_impl)) * 100);
        if (idletick3 > systemtickfrequency_impl)
            strcpy(MINI_CPU_Usage3, "0%");
        else snprintf(MINI_CPU_Usage3, sizeof(MINI_CPU_Usage3), "%.0f%%", (1.d - ((double)idletick3 / systemtickfrequency_impl)) * 100);

        mutexLock(&mutex_Misc);
        
        char MINI_CPU_compressed_c[42];
        char MINI_CPU_volt_c[16];
        if (settings.showFullCPU) {
            // Show all cores
            if (settings.realFrequencies && realCPU_Hz) {
                snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                    "[%s,%s,%s,%s]@%hu.%hhu", 
                    MINI_CPU_Usage0, MINI_CPU_Usage1, MINI_CPU_Usage2, MINI_CPU_Usage3, 
                    realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
            }
            else {
                snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                    "[%s,%s,%s,%s]@%hu.%hhu", 
                    MINI_CPU_Usage0, MINI_CPU_Usage1, MINI_CPU_Usage2, MINI_CPU_Usage3, 
                    CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
            }
        } else {
            // Show only max CPU usage
            // Extract numeric values from percentage strings
            double usage0 = 0, usage1 = 0, usage2 = 0, usage3 = 0;
            sscanf(MINI_CPU_Usage0, "%lf%%", &usage0);
            sscanf(MINI_CPU_Usage1, "%lf%%", &usage1);
            sscanf(MINI_CPU_Usage2, "%lf%%", &usage2);
            sscanf(MINI_CPU_Usage3, "%lf%%", &usage3);
            
            // Find max usage
            double maxUsage = usage0;
            if (usage1 > maxUsage) maxUsage = usage1;
            if (usage2 > maxUsage) maxUsage = usage2;
            if (usage3 > maxUsage) maxUsage = usage3;
            
            if (settings.realFrequencies && realCPU_Hz) {
                snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                    "%.0f%%@%hu.%hhu", 
                    maxUsage,
                    realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
            }
            else {
                snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                    "%.0f%%@%hu.%hhu", 
                    maxUsage,
                    CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
            }
        }


        //if (settings.realVolts) { 
        //    if (isMariko) {
        //        snprintf(MINI_CPU_volt_c, sizeof(MINI_CPU_volt_c), "%u.%u mV", realCPU_mV/1000, (realCPU_mV/100)%10);
        //    }
        //    else {
        //        snprintf(MINI_CPU_volt_c, sizeof(MINI_CPU_volt_c), "%u.%u mV", realCPU_mV/1000, (realCPU_mV/10)%100);
        //    }
        //} 
        /* ─── CPU ───────────────────────────────────────────── */
        if (settings.realVolts) {
            uint32_t mv = realCPU_mV / 1000;
            snprintf(MINI_CPU_volt_c, sizeof(MINI_CPU_volt_c), "%u mV", mv);
        }


        char MINI_GPU_Load_c[14];
        char MINI_GPU_volt_c[16];
        if (settings.realFrequencies && realGPU_Hz) {
            snprintf(MINI_GPU_Load_c, sizeof(MINI_GPU_Load_c),
                     "%hu%%%s%hu.%hhu",
                     GPU_Load_u / 10,               // integer %
                     "@",                           // keep diff char as before
                     realGPU_Hz / 1000000,
                     (realGPU_Hz / 100000) % 10);
        } else {
            snprintf(MINI_GPU_Load_c, sizeof(MINI_GPU_Load_c),
                     "%hu%%%s%hu.%hhu",
                     GPU_Load_u / 10,               // integer %
                     "@",
                     GPU_Hz / 1000000,
                     (GPU_Hz / 100000) % 10);
        }
        
        //if (settings.realVolts) { 
        //    if (isMariko) {
        //        snprintf(MINI_GPU_volt_c, sizeof(MINI_GPU_volt_c), "%u.%u mV", realGPU_mV/1000, (realGPU_mV/100)%10);
        //    }
        //    else {
        //        snprintf(MINI_GPU_volt_c, sizeof(MINI_GPU_volt_c), "%u.%u mV", realGPU_mV/1000, (realGPU_mV/10)%100);
        //    } 
        //} 
        /* ─── GPU ───────────────────────────────────────────── */
        if (settings.realVolts) {
            uint32_t mv = realGPU_mV / 1000;
            snprintf(MINI_GPU_volt_c, sizeof(MINI_GPU_volt_c), "%u mV", mv);
        }

        ///RAM
        char MINI_RAM_var_compressed_c[24];   // 19 → 24 bytes for headroom
        char MINI_RAM_volt_c[32];
        
        if (R_FAILED(sysclkCheck) || !settings.showRAMLoad) {
            /* ── “used / total MB” branch ────────────────────────────────────────── */
            float ramTotalGiB = (RAM_Total_application_u + RAM_Total_applet_u +
                                 RAM_Total_system_u + RAM_Total_systemunsafe_u) /
                                (1024.0f * 1024.0f);           // MiB → GiB
            float ramUsedGiB  = (RAM_Used_application_u + RAM_Used_applet_u +
                                 RAM_Used_system_u + RAM_Used_systemunsafe_u) /
                                (1024.0f * 1024.0f);
        
            if (settings.realFrequencies && realRAM_Hz) {
                snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                         "%.0f/%.0fMB@%hu.%hhu",
                         ramUsedGiB, ramTotalGiB,
                         realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
            } else {
                snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                         "%.0f/%.0fMB@%hu.%hhu",
                         ramUsedGiB, ramTotalGiB,
                         RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
            }
        
        } else {
            /* ── “percentage” branch (integer %) ─────────────────────────────────── */
            unsigned ramLoadInt = ramLoad[SysClkRamLoad_All] / 10;  // drop decimal
        
            if (settings.realFrequencies && realRAM_Hz) {
                snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                         "%u%%@%hu.%hhu",
                         ramLoadInt,
                         realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
            } else {
                snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                         "%u%%@%hu.%hhu",
                         ramLoadInt,
                         RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
            }
        }

        //if (settings.realVolts) { 
        //    uint32_t vdd2 = realRAM_mV / 10000;
        //    uint32_t vddq = realRAM_mV % 10000;
        //    if (isMariko) {
        //        snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%u.%u%u.%u mV", vdd2/10, vdd2%10, vddq/10, vddq%10);
        //    }
        //    else {
        //        snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%u.%u mV", vdd2/10, vdd2%10);
        //    } 
        //} 
        /* ─── RAM ───────────────────────────────────────────── */
        if (settings.realVolts) {
            float mv_vdd2 = (realRAM_mV / 10000) / 10.0f;   // VDD2 in µV → mV
            uint32_t mv_vddq = (realRAM_mV % 10000) / 10;   // VDDQ in µV → mV
            if (isMariko)
                snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c),
                         "%.1f mV%u mV", mv_vdd2, mv_vddq);
            else
                snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c),
                         "%.1f mV", mv_vdd2);
        }

        
        int duty = safeFanDuty((int)Rotation_Duty);

        ///Thermal
        // ── SoC temperature line ───────────────────────────────
        if (settings.showFanPercentage) {
            snprintf(soc_temperature_c, sizeof soc_temperature_c,
                     "%d°C (%d%%)",
                     (int)SOC_temperatureF,
                     (int)duty);          // or any percentage you prefer
        } else {
            snprintf(soc_temperature_c, sizeof soc_temperature_c,
                     "%d°C",
                     (int)SOC_temperatureF);
        }

        if (settings.showFanPercentage) {
            snprintf(skin_temperature_c, sizeof skin_temperature_c,
                "%d\u00B0C%d\u00B0C%hu\u00B0C (%d%%)",
                (int)SOC_temperatureF, (int)PCB_temperatureF,
                skin_temperaturemiliC / 1000,
                (int)duty);
        } else {
            snprintf(skin_temperature_c, sizeof skin_temperature_c,
                "%d\u00B0C%d\u00B0C%hu\u00B0C",
                (int)SOC_temperatureF, (int)PCB_temperatureF,
                skin_temperaturemiliC / 1000);
        }
        //snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%2.1f%%", Rotation_Duty);

        char MINI_SOC_volt_c[16] = ""; 
        //if (settings.realVolts) { 
        //    snprintf(MINI_SOC_volt_c, sizeof(MINI_SOC_volt_c), "%u.%u mV", realSOC_mV/1000, (realSOC_mV/100)%10);
        //} 
        /* ─── SoC ───────────────────────────────────────────── */
        if (settings.realVolts) {
            uint32_t mv = realSOC_mV / 1000;
            snprintf(MINI_SOC_volt_c, sizeof(MINI_SOC_volt_c), "%u mV", mv);
        }

        if (GameRunning && NxFps && resolutionShow) {
            if (!resolutionLookup) {
                (NxFps -> renderCalls[0].calls) = 0xFFFF;
                resolutionLookup = 1;
            }
            else if (resolutionLookup == 1) {
                if ((NxFps -> renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
            }
            else {
                memcpy(&m_resolutionRenderCalls, &(NxFps -> renderCalls), sizeof(m_resolutionRenderCalls));
                memcpy(&m_resolutionViewportCalls, &(NxFps -> viewportCalls), sizeof(m_resolutionViewportCalls));
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
                    size_t out_iter_s = out_iter;
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

            }
        }
        else if (!GameRunning && resolutionLookup != 0) {
            resolutionLookup = 0;
        }
        
        ///FPS
        char Temp[256];
        uint8_t flags = 0;
        // Pre-computed hash map (initialize once, reuse many times)
        static const std::unordered_map<std::string, std::function<void()>> key_handlers = {
            {"CPU", [&]() {
                if (!(flags & 1)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, MINI_CPU_compressed_c);
                    if (settings.realVolts) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_CPU_volt_c);
                    }
                    flags |= 1;
                }
            }},
            {"GPU", [&]() {
                if (!(flags & 2)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, MINI_GPU_Load_c);
                    if (settings.realVolts) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_GPU_volt_c);
                    }
                    flags |= 2;
                }
            }},
            {"RAM", [&]() {
                if (!(flags & 4)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, MINI_RAM_var_compressed_c);
                    if (settings.realVolts) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_RAM_volt_c);
                    }
                    flags |= 4;
                }
            }},
            {"SOC", [&]() {
                if (!(flags & 8)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, soc_temperature_c); // <- use appropriate SOC string here
                    if (settings.realVolts) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_SOC_volt_c);
                    }
                    flags |= 8;
                }
            }},
            {"TMP", [&]() {
                if (!(flags & 16)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, skin_temperature_c);
                    if (settings.realVolts) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_SOC_volt_c);
                    }
                    flags |= 16;
                }
            }},
            {"BAT", [&]() {
                if (!(flags & 32)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, SoCPCB_temperature_c);
                    flags |= 32;
                }
            }},
            {"DRAW", [&]() {
                if (!(flags & 32)) {
                    if (Temp[0]) strcat(Temp, "\n");
                    strcat(Temp, SoCPCB_temperature_c);
                    flags |= 32;
                }
            }},
            {"FPS", [&]() {
                if (!(flags & 64) && GameRunning) {
                    if (Temp[0]) strcat(Temp, "\n");
                    char Temp_s[24];
                    snprintf(Temp_s, sizeof(Temp_s), "%2.1f [%2.1f - %2.1f]", FPSavg, FPSmin, FPSmax);
                    strcat(Temp, Temp_s);
                    flags |= 64;
                }
            }},
            {"RES", [&]() {
                if (!(flags & 128) && GameRunning && m_resolutionOutput[0].width) {
                    if (Temp[0]) strcat(Temp, "\n");
                    char Temp_s[32];
                    if (settings.showFullResolution) {
                        if (!m_resolutionOutput[1].width)
                            snprintf(Temp_s, sizeof(Temp_s), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                        else 
                            snprintf(Temp_s, sizeof(Temp_s), "%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                    } else {
                        if (!m_resolutionOutput[1].width)
                            snprintf(Temp_s, sizeof(Temp_s), "%dp", m_resolutionOutput[0].height);
                        else 
                            snprintf(Temp_s, sizeof(Temp_s), "%d%d", m_resolutionOutput[0].height, m_resolutionOutput[1].height);
                    }
                    strcat(Temp, Temp_s);
                    flags |= 128;
                }
            }}
        };
        
        // Optimized loop
        for (const std::string& key : ult::split(settings.show, '+')) {
            auto it = key_handlers.find(key);
            if (it != key_handlers.end()) {
                it->second();
            }
        }
        mutexUnlock(&mutex_Misc);
        strcpy(Variables, Temp);
        
        /* ── Battery / power draw ───────────────────────────────────── */
        char remainingBatteryLife[8];
        
        /* Normalise “-0.00” → “0.00” W */
        float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f
                                                         : PowerConsumption;
        
        mutexLock(&mutex_BatteryChecker);
        
        /* keep “--:--” whenever estimate is negative */
        if (batTimeEstimate >= 0 && (drawW >= 0.01f || drawW <= -0.01f)) {
            snprintf(remainingBatteryLife, sizeof remainingBatteryLife,
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }
        
        snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c,
                 "%.2f W%.1f%% [%s]",
                 drawW,
                 (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f,
                 remainingBatteryLife);
        
        mutexUnlock(&mutex_BatteryChecker);

        static bool skipOnce = true;

        if (!skipOnce) {
            static bool runOnce = true;
            if (runOnce)
                isRendering = true;
        } else {
            skipOnce = false;
        }

    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (isKeyComboPressed(keysHeld, keysDown)) {
            isRendering = false;
            TeslaFPS = 60;
            if (skipMain)
                tsl::goBack();
            else {
                tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem Mini");
                tsl::Overlay::get()->close();
            }
            return true;
        }
        else if (((keysDown & KEY_L) && (keysDown & KEY_ZL)) || ((keysDown & KEY_L) && (keysHeld & KEY_ZL)) || ((keysHeld & KEY_L) && (keysDown & KEY_ZL))) { 
            FPSmin = 254; 
            FPSmax = 0; 
        } 
        return false;
    }
};