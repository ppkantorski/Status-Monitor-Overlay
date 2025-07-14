class FullOverlay : public tsl::Gui {
private:
    char RealCPU_Hz_c[64] = "";
    char RealGPU_Hz_c[64] = "";
    char RealRAM_Hz_c[64] = "";
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_compressed_c[64] = "";
    char RAM_var_compressed_c[128] = "";
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

    uint8_t COMMON_MARGIN = 20;
    FullSettings settings;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    std::string formattedKeyCombo = keyCombo;
    std::string message = "Press to Exit";
    const std::vector<std::string> KEY_SYMBOLS = {
        "\uE0E4", "\uE0E5", "\uE0E6", "\uE0E7",
        "\uE0E8", "\uE0E9", "\uE0ED", "\uE0EB",
        "\uE0EE", "\uE0EC", "\uE0E0", "\uE0E1",
        "\uE0E2", "\uE0E3", "\uE08A", "\uE08B",
        "\uE0B6", "\uE0B5"
    };
public:
    FullOverlay() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        tsl::hlp::requestForeground(false);
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        if (settings.setPosRight) {
            tsl::gfx::Renderer::get().setLayerPos(1248, 0);
        }
        deactivateOriginalFooter = true;
        formatButtonCombination(formattedKeyCombo);
        message = "Press " + formattedKeyCombo + " to Exit";
        StartThreads();
    }
    ~FullOverlay() {
        CloseThreads();
        //FullMode = true;
        tsl::hlp::requestForeground(true);
        //alphabackground = 0xD;
        if (settings.setPosRight) {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
        }
        deactivateOriginalFooter = false;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;

    virtual tsl::elm::Element* createUI() override {
        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", APP_VERSION, true);

        auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            
            //Print strings
            ///CPU
            uint32_t height_offset = 162;

            renderer->drawString("CPU Usage:", false, COMMON_MARGIN, height_offset - 42, 20, 0xFFFF);
            renderer->drawString(CPU_Hz_c, false, COMMON_MARGIN, height_offset - 15, 15, 0xFFFF);
            renderer->drawString(RealCPU_Hz_c, false, COMMON_MARGIN, height_offset, 15, 0xFFFF);
            renderer->drawString(CPU_compressed_c, false, COMMON_MARGIN, height_offset + 15, 15, 0xFFFF);

            ///GPU
            height_offset = 252;

            renderer->drawString("GPU Usage:", false, COMMON_MARGIN, height_offset - 42, 20, 0xFFFF);
            renderer->drawString(GPU_Hz_c, false, COMMON_MARGIN, height_offset - 15, 15, 0xFFFF);
            renderer->drawString(RealGPU_Hz_c, false, COMMON_MARGIN, height_offset, 15, 0xFFFF);
            renderer->drawString(GPU_Load_c, false, COMMON_MARGIN, height_offset + 15, 15, 0xFFFF);

            ///RAM
            height_offset = 342;

            renderer->drawString("RAM Usage:", false, COMMON_MARGIN, height_offset - 42, 20, 0xFFFF);
            renderer->drawString(RAM_Hz_c, false, COMMON_MARGIN, height_offset - 15, 15, 0xFFFF);
            renderer->drawString(RealRAM_Hz_c, false, COMMON_MARGIN, height_offset, 15, 0xFFFF);
            renderer->drawString(RAM_load_c, false, COMMON_MARGIN, height_offset + 15, 15, 0xFFFF);
    
            if (R_SUCCEEDED(Hinted)) {
                static auto dimensions = renderer->drawString("Total: \nApplication: \nApplet: \nSystem: \nSystem Unsafe: ", false, 0, height_offset + 40, 15, 0x0000);
                renderer->drawString("Total: \nApplication: \nApplet: \nSystem: \nSystem Unsafe: ", false, COMMON_MARGIN, height_offset + 40, 15, 0xFFFF);
                renderer->drawString(RAM_var_compressed_c, false, COMMON_MARGIN + dimensions.first, height_offset + 40, 15, 0xFFFF);
            }
            
            ///Thermal
            height_offset = 522;

            renderer->drawString("Thermal:", false, COMMON_MARGIN, height_offset - 42, 20, 0xFFFF);
            static auto dimensions1 = renderer->drawString("Temperatures: ", false, 0, height_offset - 15, 15, 0x0000);
            renderer->drawString("Temperatures:", false, COMMON_MARGIN, height_offset - 15, 15, 0xFFFF);
            renderer->drawString(SoCPCB_temperature_c, false, COMMON_MARGIN + dimensions1.first, height_offset - 15, 15, 0xFFFF);
            
            renderer->drawString(Rotation_SpeedLevel_c, false, COMMON_MARGIN, height_offset, 15, 0xFFFF);

            //renderer->drawString(BatteryDraw_c, false, COMMON_MARGIN, 575, 15, 0xFFFF);
            
            ///FPS
            height_offset = 605;

            renderer->drawString("Game:", false, COMMON_MARGIN, height_offset - 47, 20, 0xFFFF);
            renderer->drawString(FPS_var_compressed_c, false, COMMON_MARGIN, height_offset - 20, 15, 0xFFFF);
        
            renderer->drawString(Resolutions_c, false, COMMON_MARGIN, height_offset, 15, 0xFFFF);
            
            renderer->drawStringWithColoredSections(message.c_str(), false, KEY_SYMBOLS, COMMON_MARGIN, 693, 23,  a(tsl::bottomTextColor), a(tsl::buttonColor));
            
        });

        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        //Make stuff ready to print
        ///CPU
        snprintf(CPU_compressed_c, sizeof(CPU_compressed_c), "Load #0: %.2f%%#1: %.2f%%#2: %.2f%%#3: %.2f%%", 
            (idletick0 > systemtickfrequency_impl) ? 0.0f : (1.d - ((double)idletick0 / systemtickfrequency_impl)) * 100,
            (idletick1 > systemtickfrequency_impl) ? 0.0f : (1.d - ((double)idletick1 / systemtickfrequency_impl)) * 100,
            (idletick2 > systemtickfrequency_impl) ? 0.0f : (1.d - ((double)idletick2 / systemtickfrequency_impl)) * 100,
            (idletick3 > systemtickfrequency_impl) ? 0.0f : (1.d - ((double)idletick3 / systemtickfrequency_impl)) * 100);

        mutexLock(&mutex_Misc);
        snprintf(CPU_Hz_c, sizeof(CPU_Hz_c), "Target Frequency: %u.%u MHz", CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
        snprintf(RealCPU_Hz_c, sizeof(RealCPU_Hz_c), "Real Frequency:     %u.%u MHz", realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);

        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "Target Frequency: %u.%u MHz", GPU_Hz / 1000000, (GPU_Hz / 100000) % 10);
        snprintf(RealGPU_Hz_c, sizeof(RealGPU_Hz_c), "Real Frequency:     %u.%u MHz", realGPU_Hz / 1000000, (realGPU_Hz / 100000) % 10);
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "Load: %u.%u%%", GPU_Load_u / 10, GPU_Load_u % 10);
        
        ///RAM
        snprintf(RAM_Hz_c, sizeof RAM_Hz_c, "Target Frequency: %u.%u MHz", RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
        snprintf(RealRAM_Hz_c, sizeof(RealRAM_Hz_c), "Real Frequency:     %u.%u MHz", realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
        
        float RAM_Total_application_f = (float)RAM_Total_application_u / 1024 / 1024;
        float RAM_Total_applet_f = (float)RAM_Total_applet_u / 1024 / 1024;
        float RAM_Total_system_f = (float)RAM_Total_system_u / 1024 / 1024;
        float RAM_Total_systemunsafe_f = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        float RAM_Used_application_f = (float)RAM_Used_application_u / 1024 / 1024;
        float RAM_Used_applet_f = (float)RAM_Used_applet_u / 1024 / 1024;
        float RAM_Used_system_f = (float)RAM_Used_system_u / 1024 / 1024;
        float RAM_Used_systemunsafe_f = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c), "%4.2f / %4.2f MB\n%4.2f / %4.2f MB\n%4.2f / %4.2f MB\n%4.2f / %4.2f MB\n%4.2f / %4.2f MB", 
            RAM_Used_all_f, RAM_Total_all_f,
            RAM_Used_application_f, RAM_Total_application_f,
            RAM_Used_applet_f, RAM_Total_applet_f,
            RAM_Used_system_f, RAM_Total_system_f,
            RAM_Used_systemunsafe_f, RAM_Total_systemunsafe_f);
        
        int RAM_GPU_Load = ramLoad[SysClkRamLoad_All] - ramLoad[SysClkRamLoad_Cpu];
        snprintf(RAM_load_c, sizeof RAM_load_c, 
            "Load: %u.%u%% (CPU %u.%uGPU %u.%u)",
            ramLoad[SysClkRamLoad_All] / 10, ramLoad[SysClkRamLoad_All] % 10,
            ramLoad[SysClkRamLoad_Cpu] / 10, ramLoad[SysClkRamLoad_Cpu] % 10,
            RAM_GPU_Load / 10, RAM_GPU_Load % 10);
        
        ///Thermal
        snprintf(SoCPCB_temperature_c, sizeof SoCPCB_temperature_c, 
            "SOC %2.1f\u00B0CPCB %2.1f\u00B0CSkin %2d.%d\u00B0C", 
            SOC_temperatureF, PCB_temperatureF, skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);
        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "Fan Rotation Level: %2.1f%%", Rotation_Duty);
        
        ///FPS
        snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "PFPS: %u  FPS: %2.1f", FPS, FPSavg);

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
                if (!m_resolutionOutput[1].width)
                    snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                else snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
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
        else snprintf(remainingBatteryLife, sizeof remainingBatteryLife, "--:--");
        snprintf(BatteryDraw_c, sizeof BatteryDraw_c, "Battery Power Flow: %+.2fW[%s]", PowerConsumption, remainingBatteryLife);
        mutexUnlock(&mutex_BatteryChecker);

        static bool runOnce = true;
        if (runOnce)
            isRendering = true;
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (isKeyComboPressed(keysHeld, keysDown)) {
            isRendering = false;
            TeslaFPS = 60;
            tsl::goBack();
            return true;
        }
        return false;
    }
};
