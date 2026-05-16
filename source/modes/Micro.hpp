class MainMenu;

class MicroOverlay : public tsl::Gui {
private:
    char GPU_Load_c[32];
    char RAM_var_compressed_c[128];
    char CPU_compressed_c[160];
    char CPU_Usage0[32];
    char CPU_Usage1[32];
    char CPU_Usage2[32];
    char CPU_Usage3[32];
    char CPU_UsageM[32];
    char soc_temperature_c[32];
    char skin_temperature_c[64];  // expanded: holds combined SBS string too
    char componentTemps_c[64];    // HOC: CPU/GPU/RAM die temps
    char splitBotTemps_c[64];     // split mode: bottom row temps (SOC/PCB/Skin for HOC, empty for single-group)
    char cpu_temp_c[16];          // CPU die temp string e.g. "52°C"
    char gpu_temp_c[16];          // GPU die temp string e.g. "48°C"
    char ram_temp_c[16];          // RAM die temp string e.g. "45°C"
    char FPS_var_compressed_c[64];
    char Battery_c[32];
    char CPU_volt_c[16];
    char GPU_volt_c[16];
    char RAM_volt_c[32];
    char vdd2_only_c[16];   // split RAM mode: VDD2 string alone
    char vddq_only_c[16];  // split RAM mode: VDDQ string alone
    char SOC_volt_c[16];
    char RES_var_compressed_c[32];
    char READ_var_compressed_c[32];
    char DTC_c[32];

    static constexpr uint32_t margin = 4;
    static constexpr int32_t gridGap = 4;  // pixels between grid rows in non-SBS mode

    // Performance optimization members
    bool Initialized = false;
    MicroSettings settings;
    //size_t text_width = 0;
    //size_t fps_width = 0;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    size_t fontsize = 0;
    bool showFPS = false;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    bool tmpIsGrid = false;  // true when TMP shown as 2-row grid (both groups, no SBS, no split)
    bool tmpIsSplit = false; // true when fan on row 1, SOC volt on row 2 (showSideBySideFanSOC=false)
    bool ramIsSplit = false; // true when VDD2 on row 1, VDDQ on row 2 (showSideBySideVDDQ=false, Mariko only)
    bool cpuIsSplit = false; // true when CPU volt on row 1, CPU temp on row 2
    bool gpuIsSplit = false; // true when GPU volt on row 1, GPU temp on row 2
    bool ramTempSplit = false; // true when VDDQ on row 1, RAM temp on row 2 (VDDQ-only Mariko)
    
    // Pre-compiled render data structures
    struct RenderItem {
        uint8_t type;
        const char* label;
        const char* data_ptr;
        const char* volt_ptr;
        bool has_voltage;
    };
    
    // Resolution tracking
    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;
    bool resolutionShow = false;

    std::vector<RenderItem> renderItems;
    uint32_t cachedMargin = 0;
    int32_t  cachedAscent = 0;      // pixels above baseline
    int32_t  cachedDescentAbs = 0;  // pixels below baseline (positive value)
    tsl::Color catColorA = 0;
    tsl::Color textColorA = 0;
    int32_t base_y = 0;             // signed: text-baseline = base_y + cachedMargin
    bool renderDataDirty = true;

    bool skipOnce = true;
    bool runOnce = true;
    
    // Fixed spacing system - calculate actual widths at render time
    struct LayoutMetrics {
        uint32_t label_data_gap = 8;      // Fixed gap between label and data
        uint32_t volt_separator_gap = 0;   // Fixed gap before voltage separator
        uint32_t volt_data_gap = 0;        // Fixed gap after voltage separator
        uint32_t item_spacing = 16;        // Minimum spacing between complete items
        uint32_t side_margin = 8;          // Left/right edge padding — set to label_data_gap in calculateLayoutMetrics
        bool calculated = false;
    } layout;
    
    inline const char* getDifferenceSymbol(int32_t /*delta*/) {
        return "@";
    }
    
    void calculateLayoutMetrics(tsl::gfx::Renderer *renderer) {
        if (layout.calculated) return;
        
        // Use font size to determine appropriate spacing
        if (fontsize <= 16) {
            layout.label_data_gap = 6;
            layout.volt_separator_gap = 0;
            layout.volt_data_gap = 0;
            layout.item_spacing = 12;
        } else if (fontsize <= 20) {
            layout.label_data_gap = 8;
            layout.volt_separator_gap = 0;
            layout.volt_data_gap = 0;
            layout.item_spacing = 16;
        } else {
            layout.label_data_gap = 10;
            layout.volt_separator_gap = 0;
            layout.volt_data_gap = 0;
            layout.item_spacing = 20;
        }
        // Horizontal padding setting controls left/right gap from screen edge to text.
        // The bar rectangle always spans full screen width; only the text is inset.
        layout.side_margin = settings.horizontalPadding;
        
        layout.calculated = true;
    }

public:
    MicroOverlay() { 
        tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        //tsl::initializeUltrahandSettings();
        GetConfigSettings(&settings);
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else fontsize = settings.dockedFontSize;

        if (ult::limitedMemory && settings.setPosBottom) {
            const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
            // VI space is always 1080 units tall. At handheld 720p (1.5x scale):
            //   layerY = 1080 - FramebufferHeight * 1.5  (bottom-aligns the layer)
            const float bottomVI = 1080.0f - static_cast<float>(tsl::cfg::FramebufferHeight) * 1.5f;
            // Clamp to 0 before casting: bottomVI is only 540 for a 360-row FB, so
            // large underscan values can make the result negative — casting a negative
            // float to uint32_t is UB and will crash on AArch64.
            tsl::gfx::Renderer::get().setLayerPos(0u,
                static_cast<uint32_t>(std::max(0.0f, bottomVI - verticalUnderscanPixels)));
        }

        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        FullMode = false;
        //alphabackground = 0x0;
        deactivateOriginalFooter = true;
        StartThreads();
        
        // Pre-allocate render items vector
        //renderItems.reserve(8);
        realVoltsPolling = settings.realVolts;

        // Get initial battery readings BEFORE starting threads
        if (R_SUCCEEDED(psmCheck) && R_SUCCEEDED(i2cCheck)) {
            uint16_t data = 0;
            float tempA = 0.0;
            
            // Get initial power consumption
            Max17050ReadReg(MAX17050_AvgCurrent, &data);
            tempA = (1.5625 / (max17050SenseResistor * max17050CGain)) * (s16)data;
            PowerConsumption = tempA * batVoltageAvg / 1000000.0; // Rough initial estimate
            
            // Get initial battery info
            psmGetBatteryChargeInfoFields(psmService, &_batteryChargeInfoFields);
            
            // Get initial time estimate
            if (tempA >= 0) {
                batTimeEstimate = -1;
            } else {
                Max17050ReadReg(MAX17050_TTE, &data);
                const float batteryTimeEstimateInMinutes = (5.625 * data) / 60;
                if (batteryTimeEstimateInMinutes > (99.0*60.0)+59.0) {
                    batTimeEstimate = (99*60)+59;
                } else {
                    batTimeEstimate = (int16_t)batteryTimeEstimateInMinutes;
                }
            }
        } else {
            // Fallback if checks failed
            PowerConsumption = 0.0f;
            batTimeEstimate = -1;
            _batteryChargeInfoFields = {0};
        }
        
        // Now format the initial Battery_c string
        char remainingBatteryLife[8];
        const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f : PowerConsumption;
        
        if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
            snprintf(remainingBatteryLife, sizeof(remainingBatteryLife),
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }
        
        if (!settings.invertBatteryDisplay) {
            snprintf(Battery_c, sizeof(Battery_c),
                     "%.2f W%.1f%% [%s]",
                     drawW,
                     (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f,
                     remainingBatteryLife);
        } else {
            snprintf(Battery_c, sizeof(Battery_c),
                     "%.1f%% [%s]%.2f W",
                     (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f,
                     remainingBatteryLife,
                     drawW);
        }

        
    }
    
    ~MicroOverlay() {
        CloseThreads();
        fixForeground = true;
        FullMode = true;
    }
    
    // Fast parsing and render item preparation
    void prepareRenderItems() {
        if (!renderDataDirty) return;
        
        renderItems.clear();
        
        // Fast manual parsing of settings.show
        const std::string& show = settings.show;
        size_t start = 0, end = 0;
        uint8_t seen_flags = 0;
        
        static size_t len;
        static uint32_t key3;
        while (start < show.length()) {
            end = show.find('+', start);
            if (end == std::string::npos) end = show.length();
            
            len = end - start;
            if (len >= 3) {
                const char* key = &show[start];
                
                // Use first 3 chars for fast comparison
                key3 = (key[0] << 16) | (key[1] << 8) | key[2];
                
                switch (key3) {
                    case 0x435055: // "CPU"
                        if (!(seen_flags & 1)) {
                            const bool cpuSplit    = settings.showCPUTemp && !settings.showSideBySideCPUTemp && settings.realVolts;
                            const bool cpuSBSDefer = settings.voltageAtEndCPU && settings.showCPUTemp && settings.showSideBySideCPUTemp && settings.realVolts;
                            renderItems.push_back({0, "CPU", CPU_compressed_c, CPU_volt_c, settings.realVolts && !cpuSplit && !cpuSBSDefer});
                            seen_flags |= 1;
                        }
                        break;
                    case 0x475055: // "GPU"
                        if (!(seen_flags & 2)) {
                            const bool gpuSplit    = settings.showGPUTemp && !settings.showSideBySideGPUTemp && settings.realVolts;
                            const bool gpuSBSDefer = settings.voltageAtEndGPU && settings.showGPUTemp && settings.showSideBySideGPUTemp && settings.realVolts;
                            renderItems.push_back({1, "GPU", GPU_Load_c, GPU_volt_c, settings.realVolts && !gpuSplit && !gpuSBSDefer});
                            seen_flags |= 2;
                        }
                        break;
                    case 0x52414D: // "RAM"
                        if (!(seen_flags & 4)) {
                            // In split VDD2/VDDQ mode, voltage is drawn by the renderer
                            const bool splitRAMVolt = !settings.showSideBySideVDDQ &&
                                                      settings.realVolts && settings.showVDD2 &&
                                                      settings.showVDDQ && isMariko;
                            // ramTempSplit: VDDQ-only Mariko with RAM temp split (volt drawn by renderer)
                            const bool ramTempSplitCond = settings.showRAMTemp && !settings.showSideBySideRAMTemp &&
                                                          !splitRAMVolt && settings.realVolts &&
                                                          ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                                           (settings.showVDD2 && !settings.showVDDQ));
                            const bool ramInlineDefer = settings.voltageAtEndRAM && settings.realVolts
                                && !splitRAMVolt && !ramTempSplitCond && settings.showRAMTemp;
                            renderItems.push_back({2, "RAM", RAM_var_compressed_c, RAM_volt_c,
                                settings.realVolts && !splitRAMVolt && !ramTempSplitCond && !ramInlineDefer});
                            seen_flags |= 4;
                        }
                        break;
                    case 0x534F43: // "SOC"
                        if (!(seen_flags & 8)) {
                            renderItems.push_back({3, "SOC", soc_temperature_c, SOC_volt_c, settings.realVolts && settings.showSOCVoltage});
                            seen_flags |= 8;
                        }
                        break;
                    case 0x544D50: // "TMP"
                        if (!(seen_flags & 16)) {
                            // In split mode volt is drawn by the renderer, not via has_voltage
                            const bool splitFanSOC = !settings.showSideBySideFanSOC &&
                                                     settings.realVolts && settings.showSOCVoltage;
                            renderItems.push_back({4, "TMP", skin_temperature_c, SOC_volt_c,
                                settings.realVolts && settings.showSOCVoltage && !splitFanSOC});
                            seen_flags |= 16;
                        }
                        break;
                    case 0x524553: // "RES"
                        if (!(seen_flags & 128)) {
                            renderItems.push_back({7, "RES", RES_var_compressed_c, nullptr, false}); // We'll reuse FPS buffer temporarily
                            seen_flags |= 128;
                            resolutionShow = true;
                        }
                        break;
                    case 0x465053: // "FPS"
                        if (!(seen_flags & 32)) {
                            renderItems.push_back({5, "FPS", FPS_var_compressed_c, nullptr, false});
                            seen_flags |= 32;
                        }
                        break;
                    case 0x424154: // "BAT"
                        if (!(seen_flags & 64)) {
                            renderItems.push_back({6, "BAT", Battery_c, nullptr, false});
                            seen_flags |= 64;
                        }
                        break;
                    case 0x524541: // "REA" (for READ)
                        if (len >= 4 && key[3] == 'D' && !(seen_flags & 512)) {
                            renderItems.push_back({9, "READ", READ_var_compressed_c, nullptr, false});
                            seen_flags |= 512;
                        }
                        break;
                    case 0x445443: // "DTC" 
                        if (!(seen_flags & 256) && settings.showDTC) {
                            renderItems.push_back({8, settings.useDTCSymbol ? "\uE007" : "DTC", DTC_c, nullptr, false});
                            seen_flags |= 256;
                        }
                        break;
                }
            }
            start = end + 1;
        }
        
        renderDataDirty = false;

        // Determine if TMP should render as a 2-row grid
        bool hasTmp = false;
        bool hasCpu = false;
        bool hasGpu = false;
        bool hasRam = false;
        for (const auto& item : renderItems) {
            if (item.type == 4) hasTmp = true;
            if (item.type == 0) hasCpu = true;
            if (item.type == 1) hasGpu = true;
            if (item.type == 2) hasRam = true;
        }
        tmpIsSplit = hasTmp &&
                     !settings.showSideBySideFanSOC &&
                     settings.realVolts && settings.showSOCVoltage;
        tmpIsGrid = hasTmp &&
                    settings.showComponentTemps &&
                    settings.showSocPcbSkinTemps &&
                    !settings.showSideBySideTemps &&
                    !tmpIsSplit;  // split takes priority over grid

        ramIsSplit = hasRam &&
                     !settings.showSideBySideVDDQ &&
                     settings.realVolts && settings.showVDD2 &&
                     settings.showVDDQ && isMariko;

        // CPU die temp split: volt on top row, CPU temp on bottom row
        cpuIsSplit = hasCpu && settings.showCPUTemp && !settings.showSideBySideCPUTemp && settings.realVolts;
        // GPU die temp split: volt on top row, GPU temp on bottom row
        gpuIsSplit = hasGpu && settings.showGPUTemp && !settings.showSideBySideGPUTemp && settings.realVolts;
        // RAM temp split: VDDQ-only Mariko → VDDQ top row, RAM temp bottom row
        ramTempSplit = hasRam && settings.showRAMTemp && !settings.showSideBySideRAMTemp && !ramIsSplit &&
                       settings.realVolts &&
                       ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                        (settings.showVDD2 && !settings.showVDDQ));
    }
    
    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            cachedMargin = renderer->getTextDimensions("CPUGPURAMSOCBAT[]", false, fontsize).second;
            if (!Initialized) {
                //cachedMargin = renderer->drawString(" ", false, 0, 0, fontsize, renderer->a(0x0000)).first;
                
                catColorA = settings.catColor;
                textColorA = settings.textColor;
                {
                    // Get true ascent / descent from the font so we can center text
                    // properly inside the bar rectangle.
                    const auto fm = tsl::gfx::FontManager::getFontMetricsForCharacter('A', fontsize);
                    cachedAscent    = fm.ascent;           // positive: pixels above baseline
                    cachedDescentAbs = -fm.descent;        // fm.descent is negative; make positive
                    const int32_t vPad = (int32_t)settings.verticalPadding;
                    if (settings.setPosBottom) {
                        // Bar bottom is at FramebufferHeight; text centered with vPad on each side.
                        // baseline = FramebufferHeight - vPad - cachedDescentAbs
                        // base_y   = baseline - cachedMargin
                        base_y = (int32_t)tsl::cfg::FramebufferHeight
                                 - cachedDescentAbs - vPad
                                 - (int32_t)cachedMargin;
                    } else {
                        // Bar top is at 0; text centered with vPad on each side.
                        // baseline = vPad + cachedAscent
                        // base_y   = baseline - cachedMargin  (may be negative, that's fine)
                        base_y = vPad + cachedAscent - (int32_t)cachedMargin;
                    }
                }
                Initialized = true;
                renderDataDirty = true;
                layout.calculated = false; // Force recalculation
                tsl::hlp::requestForeground(false);
            }
            
            //renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth, cachedMargin + 4, a(settings.backgroundColor));
            // Update render items so tmpIsGrid is correct before bar draw
            prepareRenderItems();
            calculateLayoutMetrics(renderer);
            {
                const int32_t gridExtraHeight = (tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit) ? ((int32_t)cachedMargin + gridGap) : 0;
                const int32_t vPad = (int32_t)settings.verticalPadding;
                // Visual text height = ascent + |descent|  (excludes lineGap)
                const int32_t textVisualH = cachedAscent + cachedDescentAbs;
                // barH: textVisualH + N above + N below, plus any grid expansion
                const int32_t barH = textVisualH + 2 * vPad + gridExtraHeight;
                // barY: top position → flush at Y=0; bottom → bar bottom at FramebufferHeight
                const int32_t barY = settings.setPosBottom
                    ? (int32_t)tsl::cfg::FramebufferHeight - barH
                    : 0;
                renderer->drawRect(0, barY, tsl::cfg::FramebufferWidth, barH, a(settings.backgroundColor));
            }

            
            // Separate battery from other items
            std::vector<RenderItem> main_items;
            //RenderItem* battery_item = nullptr;
            
            for (auto& item : renderItems) {
                //if (item.type == 6) { // BAT
                //    battery_item = &item;
                if (item.type == 5 && (!GameRunning || (strcmp(FPS_var_compressed_c, "254.0") == 0))) {
                    // Skip FPS if no game running
                    continue;
                } else if (item.type == 7 && (!GameRunning || !m_resolutionOutput[0].width)) {
                    // Skip RES if no game running or no resolution data yet
                    continue;
                } else if (item.type == 8 && !settings.showDTC) {
                    // Skip DTC if disabled in settings
                    continue;
                } else if (item.type == 9 && (!GameRunning || !NxFps)) {
                    // Skip READ if no game running or no NxFps available
                    continue;
                } else {
                    main_items.push_back(item);
                }
            }
            
            // Calculate actual widths for all main items
            struct ItemLayout {
                uint32_t label_width;
                uint32_t data_width;
                uint32_t volt_width;
                uint32_t total_width;
            };
            
            std::vector<ItemLayout> item_layouts;
            uint32_t total_main_width = 0;

            static const auto sep_width = renderer->getTextDimensions("", false, fontsize).first;

            static ItemLayout item_layout;
            for (const auto& item : main_items) {
                item_layout = {};
                
                // Calculate actual label width
                //auto label_dim = renderer->drawString(item.label, false, 0, 0, fontsize, renderer->a(0x0000));
                //auto label_dim = renderer->getTextDimensions(item.label, fontsize);
                item_layout.label_width = renderer->getTextDimensions(item.label, false, fontsize).first;
                
                // Calculate actual data width
                //auto data_dim = renderer->drawString(item.data_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
                //auto data_dim = renderer->getTextDimensions(item.data_ptr, fontsize);
                item_layout.data_width = renderer->getTextDimensions(item.data_ptr, false, fontsize).first;
                
                // Calculate voltage width if present
                if (item.has_voltage && item.volt_ptr) {
                    //uto volt_dim = renderer->drawString(item.volt_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
                    //auto volt_dim = renderer->getTextDimensions(item.volt_ptr, fontsize);
                    item_layout.volt_width = renderer->getTextDimensions(item.volt_ptr, false, fontsize).first;
                    
                    // Total: label + gap + data + gap + "|" + gap + voltage
                    //auto sep_width = renderer->drawString("", false, 0, 0, fontsize, renderer->a(0x0000));
                    item_layout.total_width = item_layout.label_width + layout.label_data_gap + 
                                            item_layout.data_width + layout.volt_separator_gap + 
                                            sep_width + layout.volt_data_gap + item_layout.volt_width;
                } else {
                    // Total: label + gap + data
                    item_layout.total_width = item_layout.label_width + layout.label_data_gap + item_layout.data_width;

                    // In split TMP mode the bottom row renders [temp_part][DIVIDER][volt],
                    // which may be wider than the top row [temp_part][DIVIDER][fan].
                    // Parse the data string to find the fan-divider split point, then
                    // compute max(top_row_w, temp_part_w + sep_width + volt_w).
                    if (tmpIsSplit && item.type == 4 && item.volt_ptr && item.volt_ptr[0]) {
                        const uint32_t volt_w = renderer->getTextDimensions(item.volt_ptr, false, fontsize).first;
                        uint32_t temp_part_w = item_layout.data_width; // fallback: whole data
                        const std::string dataStr(item.data_ptr);
                        const size_t divPos = dataStr.find(ult::DIVIDER_SYMBOL);
                        if (divPos != std::string::npos) {
                            const std::string tempPart = dataStr.substr(0, divPos);
                            temp_part_w = renderer->getTextDimensions(tempPart.c_str(), false, fontsize).first;
                        }
                        const uint32_t bottom_w = temp_part_w + sep_width + volt_w;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  std::max(item_layout.data_width, bottom_w);
                    }

                    // In split RAM mode both rows share the volt column: take max(vdd2_w, vddq_w).
                    // When SBS RAM temp is ON, temp draws at the center row (singleItemY) — use max of all 3.
                    // When SBS RAM temp is OFF, temp appends to VDDQ row — widen vddq_w accordingly.
                    if (ramIsSplit && item.type == 2 && (vdd2_only_c[0] || vddq_only_c[0])) {
                        const uint32_t vdd2_w = vdd2_only_c[0] ? renderer->getTextDimensions(vdd2_only_c, false, fontsize).first : 0;
                        const uint32_t vddq_w_base = vddq_only_c[0] ? renderer->getTextDimensions(vddq_only_c, false, fontsize).first : 0;
                        if (settings.showRAMTemp && ram_temp_c[0]) {
                            const uint32_t div_w = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const uint32_t temp_w = renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                            if (settings.showSideBySideRAMTemp) {
                                if (settings.voltageAtEndRAM) {
                                    // VoltAtEnd+SBS: temp first, then voltages to the right
                                    // Width from voltColX = div_w + temp_w + div_w + max(vdd2_w, vddq_w_base)
                                    item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                              item_layout.data_width +
                                                              div_w + temp_w + div_w + std::max(vdd2_w, vddq_w_base);
                                } else {
                                    // Normal SBS: temp to the right of the widest voltage
                                    // Width from voltColX = div_w + max(vdd2_w, vddq_w_base) + div_w + temp_w
                                    item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                              item_layout.data_width +
                                                              div_w + std::max(vdd2_w, vddq_w_base) + div_w + temp_w;
                                }
                            } else {
                                // non-SBS: temp appended inline after VDDQ at bottom row
                                const uint32_t vddq_w = vddq_w_base + div_w + temp_w;
                                item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                          item_layout.data_width + sep_width + std::max(vdd2_w, vddq_w);
                            }
                        } else {
                            item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                      item_layout.data_width + sep_width + std::max(vdd2_w, vddq_w_base);
                        }
                    }

                    // CPU split: volt top row, CPU temp bottom row
                    if (cpuIsSplit && item.type == 0) {
                        const uint32_t volt_w = CPU_volt_c[0] ? renderer->getTextDimensions(CPU_volt_c, false, fontsize).first : 0;
                        const uint32_t temp_w = cpu_temp_c[0] ? renderer->getTextDimensions(cpu_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + sep_width + std::max(volt_w, temp_w);
                    }

                    // GPU split: volt top row, GPU temp bottom row
                    if (gpuIsSplit && item.type == 1) {
                        const uint32_t volt_w = GPU_volt_c[0] ? renderer->getTextDimensions(GPU_volt_c, false, fontsize).first : 0;
                        const uint32_t temp_w = gpu_temp_c[0] ? renderer->getTextDimensions(gpu_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + sep_width + std::max(volt_w, temp_w);
                    }

                    // RAM temp split (single volt rail): volt top, RAM temp bottom
                    if (ramTempSplit && item.type == 2) {
                        const char* topVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                        const uint32_t volt_w = topVolt[0] ? renderer->getTextDimensions(topVolt, false, fontsize).first : 0;
                        const uint32_t temp_w = ram_temp_c[0] ? renderer->getTextDimensions(ram_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + sep_width + std::max(volt_w, temp_w);
                    }
                }

                // For SBS temp modes, add the extra temp width.
                // When voltageAtEnd ON, has_voltage=false so we must also include the full sep+volt.
                {
                    const uint32_t div_w = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    if (!cpuIsSplit && item.type == 0 && settings.showCPUTemp && settings.showSideBySideCPUTemp && cpu_temp_c[0]) {
                        const uint32_t temp_w = renderer->getTextDimensions(cpu_temp_c, false, fontsize).first;
                        if (settings.voltageAtEndCPU && settings.realVolts) {
                            const uint32_t volt_w = CPU_volt_c[0] ? renderer->getTextDimensions(CPU_volt_c, false, fontsize).first : 0;
                            item_layout.total_width += layout.volt_separator_gap + sep_width + layout.volt_data_gap + div_w + temp_w + volt_w;
                        } else {
                            item_layout.total_width += div_w + temp_w;
                        }
                    }
                    if (!gpuIsSplit && item.type == 1 && settings.showGPUTemp && settings.showSideBySideGPUTemp && gpu_temp_c[0]) {
                        const uint32_t temp_w = renderer->getTextDimensions(gpu_temp_c, false, fontsize).first;
                        if (settings.voltageAtEndGPU && settings.realVolts) {
                            const uint32_t volt_w = GPU_volt_c[0] ? renderer->getTextDimensions(GPU_volt_c, false, fontsize).first : 0;
                            item_layout.total_width += layout.volt_separator_gap + sep_width + layout.volt_data_gap + div_w + temp_w + volt_w;
                        } else {
                            item_layout.total_width += div_w + temp_w;
                        }
                    }
                    if (!ramIsSplit && !ramTempSplit && item.type == 2 && settings.showRAMTemp && ram_temp_c[0]) {
                        const uint32_t temp_w = renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                        if (settings.voltageAtEndRAM && settings.realVolts) {
                            const uint32_t volt_w = RAM_volt_c[0] ? renderer->getTextDimensions(RAM_volt_c, false, fontsize).first : 0;
                            item_layout.total_width += layout.volt_separator_gap + sep_width + layout.volt_data_gap + div_w + temp_w + volt_w;
                        } else {
                            item_layout.total_width += div_w + temp_w;
                        }
                    }
                }
                
                item_layouts.push_back(item_layout);
                total_main_width += item_layout.total_width;
            }
            

            
            // Determine if we have battery and handle it as the rightmost item
            std::vector<RenderItem> all_items_ordered;
            std::vector<ItemLayout> all_layouts_ordered;
            
            // Add main items first
            for (size_t i = 0; i < main_items.size(); i++) {
                all_items_ordered.push_back(main_items[i]);
                all_layouts_ordered.push_back(item_layouts[i]);
            }
            
            // Add battery as the last item if present
            //if (battery_item) {
            //    //auto bat_label_dim = renderer->drawString("BAT", false, 0, 0, fontsize, renderer->a(0x0000));
            //    //auto bat_label_dim = renderer->getTextDimensions("BAT", fontsize);
            //    //auto bat_data_dim = renderer->drawString(battery_item->data_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
            //    //auto bat_data_dim = renderer->getTextDimensions(battery_item->data_ptr, fontsize);
            //    
            //    ItemLayout battery_layout = {};
            //    battery_layout.label_width = renderer->getTextDimensions("BAT", false, fontsize).first;
            //    battery_layout.data_width = renderer->getTextDimensions(battery_item->data_ptr, false, fontsize).first;
            //    battery_layout.volt_width = 0;
            //    battery_layout.total_width = battery_layout.label_width + layout.label_data_gap + battery_layout.data_width;
            //    
            //    all_items_ordered.push_back(*battery_item);
            //    all_layouts_ordered.push_back(battery_layout);
            //}
            
            // Calculate total width of all items
            uint32_t total_all_width = 0;
            for (const auto& item_layout : all_layouts_ordered) {
                total_all_width += item_layout.total_width;
            }
            
            // Calculate available space for distribution
            //uint32_t available_width = tsl::cfg::FramebufferWidth - (2 * layout.side_margin);
            //uint32_t remaining_space = available_width - total_all_width;
                        
            // Calculate positions based on alignment mode
            std::vector<uint32_t> item_positions;
            const size_t N = all_items_ordered.size();
            if (N == 0) return;
            
            if (N == 1) {
                // Single item positioning based on alignment
                if (settings.alignTo == 2) { // RIGHT
                    const uint32_t total_width = all_layouts_ordered[0].total_width;
                    item_positions.push_back(tsl::cfg::FramebufferWidth - layout.side_margin - total_width);
                } else { // LEFT or CENTER
                    item_positions.push_back(layout.side_margin);
                }
            } else {
                uint32_t total_widths = 0;
                for (const auto& layout : all_layouts_ordered) {
                    total_widths += layout.total_width;
                }
                
                if (settings.alignTo == 0) { // LEFT alignment
                    // All items except last positioned from left with small gaps
                    // Last item (battery if present) positioned at far right
                    const uint32_t small_gap = layout.item_spacing;
                    
                    // Position items from left
                    uint32_t current_x = layout.side_margin;
                    for (size_t i = 0; i < N - 1; ++i) {
                        item_positions.push_back(current_x);
                        current_x += all_layouts_ordered[i].total_width + small_gap;
                    }
                    
                    // Position last item at far right
                    const uint32_t last_width = all_layouts_ordered[N-1].total_width;
                    item_positions.push_back(tsl::cfg::FramebufferWidth - layout.side_margin - last_width);
                    
                } else if (settings.alignTo == 2) { // RIGHT alignment
                    // First item at far left, remaining items packed at right
                    const uint32_t small_gap = layout.item_spacing;
                    
                    // Resize vector to hold all positions
                    item_positions.resize(N);
                    
                    // Position first item at far left
                    item_positions[0] = layout.side_margin;
                    
                    // Calculate total width of items 1 to N-1 plus gaps between them
                    uint32_t right_group_width = 0;
                    for (size_t i = 1; i < N; ++i) {
                        right_group_width += all_layouts_ordered[i].total_width;
                        if (i < N - 1) right_group_width += small_gap; // Gap after each item except the last
                    }
                    
                    // Start positioning from right margin minus total width of right group
                    uint32_t current_x = tsl::cfg::FramebufferWidth - layout.side_margin - right_group_width;
                    
                    // Position items 1 to N-1 sequentially from left to right within the right group
                    for (size_t i = 1; i < N; ++i) {
                        item_positions[i] = current_x;
                        current_x += all_layouts_ordered[i].total_width + small_gap;
                    }
                } else { // CENTER alignment (default behavior)
                    // Total available width for spacing = framebuffer width minus total item widths minus margins
                    const int32_t total_spacing = (int32_t)tsl::cfg::FramebufferWidth - (2 * (int32_t)layout.side_margin) - (int32_t)total_widths;
                    
                    // Number of gaps between items is N-1
                    const uint32_t gap = total_spacing > 0 ? (uint32_t)(total_spacing / (N - 1)) : 0;
                    
                    // Position first item flush left
                    item_positions.push_back(layout.side_margin);
                    static uint32_t prev_pos, prev_width;
                    // Position subsequent items
                    for (size_t i = 1; i < N; ++i) {
                        prev_pos = item_positions[i - 1];
                        prev_width = all_layouts_ordered[i - 1].total_width;
                        item_positions.push_back(prev_pos + prev_width + gap);
                    }
            
                    // Fix rounding error by distributing remainder evenly across gaps
                    const int32_t last_item_end = item_positions.back() + all_layouts_ordered.back().total_width;
                    const int32_t overflow = (int32_t)tsl::cfg::FramebufferWidth - layout.side_margin - last_item_end;
                    
                    if (overflow != 0 && N > 1) {
                        for (size_t i = 1; i < item_positions.size(); ++i) {
                            item_positions[i] += (overflow * (int32_t)i) / (int32_t)(N - 1);
                        }
                    }
                }
            }


            uint32_t current_x;
            static std::vector<std::string> specialChars = {""};

            // \u2500\u2500 Grid-mode Y coordinates \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
            // When tmpIsGrid: bar is taller; other items centered in expanded bar;
            // TMP renders as two rows (componentTemps_c top, skin_temperature_c bottom).
            const int32_t gridExtraH  = (tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit) ? ((int32_t)cachedMargin + gridGap) : 0;
            const int32_t halfExtra   = gridExtraH / 2;
            // Y for non-TMP items — centered in expanded bar (grid or split)
            const int32_t singleItemY = (int32_t)base_y + (int32_t)cachedMargin +
                                        ((tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit) ? (settings.setPosBottom ? -halfExtra : halfExtra) : 0);
            // Y for the two TMP rows (used by both grid mode and split fan/volt mode)
            const int32_t gridTopY = (int32_t)base_y + (int32_t)cachedMargin +
                                     (settings.setPosBottom ? -(int32_t)gridExtraH : 0);
            const int32_t gridBotY = (int32_t)base_y + (int32_t)cachedMargin +
                                     (settings.setPosBottom ? 0 : (int32_t)gridExtraH);

            // Render all items at calculated positions
            for (size_t i = 0; i < all_items_ordered.size(); i++) {
                const auto& item = all_items_ordered[i];
                const auto& item_layout = all_layouts_ordered[i];
                current_x = item_positions[i];

                // Draw label — for TMP grid mode, label is centered between the two rows
                {
                    const int32_t labelY = (tmpIsGrid && item.type == 4) ? (int32_t)singleItemY : (int32_t)singleItemY;
                    renderer->drawString(item.label, false, current_x, labelY, fontsize, catColorA);
                }
                current_x += item_layout.label_width + layout.label_data_gap;

                // Draw data
                if (settings.useDynamicColors) {
                    if (item.type == 3) { // SOC temperature
                        std::string dataStr(item.data_ptr);
                        const size_t degreesPos = dataStr.find("°");
                        if (degreesPos != std::string::npos) {
                            const size_t cPos = dataStr.find("C", degreesPos);
                            if (cPos != std::string::npos) {
                                const std::string tempPart = dataStr.substr(0, cPos + 1);
                                const std::string restPart = dataStr.substr(cPos + 1);
                                const int temp = atoi(item.data_ptr);
                                renderer->drawString(tempPart, false, current_x, singleItemY, fontsize, tsl::GradientColor((float)temp));
                                if (!restPart.empty()) {
                                    const uint32_t tpw = renderer->getTextDimensions(tempPart, false, fontsize).first;
                                    uint32_t ax = current_x + tpw;
                                    const size_t dv = restPart.find(ult::DIVIDER_SYMBOL);
                                    if (dv != std::string::npos) {
                                        const size_t dl = ult::DIVIDER_SYMBOL.length();
                                        ax += renderer->drawString(restPart.substr(0, dv + dl), false, ax, singleItemY, fontsize, (settings.separatorColor)).first;
                                        const std::string ad = restPart.substr(dv + dl);
                                        if (!ad.empty()) {
                                            static const std::vector<std::string> fic = {""};
                                            renderer->drawStringWithColoredSections(ad, false, fic, ax, singleItemY, fontsize, textColorA, catColorA);
                                        }
                                    } else {
                                        renderer->drawStringWithColoredSections(restPart, false, specialChars, ax, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                    }
                                }
                            } else {
                                renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                            }
                        } else {
                            renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                        }

                    } else if (item.type == 4) { // TMP
                        if (tmpIsSplit) {
                            // SPLIT MODE: fan on top row, SOC voltage on bottom row.
                            // HOC split (splitBotTemps_c set): temps fill both rows, fan top, volt bottom.
                            // Single-group (!splitBotTemps_c[0]): temps at singleItemY (centered),
                            //   fan at tmpFanY and volt at tmpVoltY, both starting at the fan column X.
                            const bool splitIsHoc = splitBotTemps_c[0] != 0;
                            // Temp groups never swap — component temps always top, SOC temps always bottom.
                            const int32_t tmpFanY  = gridTopY;  // component temps + fan column row
                            const int32_t tmpVoltY = gridBotY;  // SOC/PCB/skin temps + volt column row
                            // Only fan and voltage swap when voltageAtEndTMP is OFF
                            const int32_t fanDrawY  = settings.voltageAtEndTMP ? gridTopY : gridBotY;
                            const int32_t voltDrawY = settings.voltageAtEndTMP ? gridBotY : gridTopY;
                            uint32_t fanColX = current_x; // X where fan/volt column starts (set after temps)
                            // Top row: parse temps + fan
                            {
                                std::string topStr(skin_temperature_c);
                                uint32_t rx = current_x;
                                size_t rpos = 0;
                                bool rOK = true;
                                const bool topIsComp = settings.showComponentTemps;
                                bool pastTopDiv = false;
                                // Temps draw at tmpFanY (HOC) or singleItemY (single-group)
                                const int32_t tempDrawY = splitIsHoc ? tmpFanY : singleItemY;
                                for (int tc = 0; tc < 6 && rOK && rpos < topStr.length(); tc++) {
                                    while (rpos < topStr.length() && topStr[rpos] == ' ') {
                                        renderer->drawString(" ", false, rx, tempDrawY, fontsize, textColorA);
                                        rx += renderer->getTextDimensions(" ", false, fontsize).first;
                                        rpos++;
                                    }
                                    if (rpos >= topStr.length()) break;
                                    const size_t nxtDiv = topStr.find(ult::DIVIDER_SYMBOL, rpos);
                                    const size_t nxtDeg = topStr.find("°", rpos);
                                    if (nxtDiv != std::string::npos && (nxtDeg == std::string::npos || nxtDiv < nxtDeg)) {
                                        const size_t dl = ult::DIVIDER_SYMBOL.length();
                                        const size_t afterDiv = nxtDiv + dl;
                                        const size_t degAfterDiv = topStr.find("°", afterDiv);
                                        if (degAfterDiv != std::string::npos) {
                                            rx += renderer->drawString(topStr.substr(nxtDiv, dl), false, rx, tempDrawY, fontsize, (settings.separatorColor)).first;
                                            rpos = afterDiv;
                                            pastTopDiv = true;
                                            tc--;
                                            continue;
                                        } else {
                                            break; // fan divider: stop temp parsing
                                        }
                                    }
                                    if (nxtDeg == std::string::npos) break;
                                    const size_t cp = topStr.find("C", nxtDeg);
                                    if (cp == std::string::npos) { rOK = false; break; }
                                    const std::string tp = topStr.substr(rpos, cp + 1 - rpos);
                                    const int tv = atoi(tp.c_str());
                                    const bool useHigh = topIsComp && (!settings.showSideBySideTemps || !pastTopDiv);
                                    renderer->drawStringWithColoredSections(tp, false, specialChars, rx, tempDrawY, fontsize,
                                        useHigh ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                                : tsl::GradientColor((float)tv),
                                        (settings.separatorColor));
                                    rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                    rpos = cp + 1;
                                }
                                fanColX = rx; // mark where fan column starts
                                // Draw remaining text (DIVIDER + fan icon + duty%) at fanDrawY
                                if (rpos < topStr.length()) {
                                    const std::string rest = topStr.substr(rpos);
                                    const size_t dv = rest.find(ult::DIVIDER_SYMBOL);
                                    if (dv != std::string::npos) {
                                        const size_t dl = ult::DIVIDER_SYMBOL.length();
                                        rx += renderer->drawString(rest.substr(0, dv + dl), false, rx, fanDrawY, fontsize, (settings.separatorColor)).first;
                                        const std::string afterFanDiv = rest.substr(dv + dl);
                                        if (!afterFanDiv.empty()) {
                                            static const std::vector<std::string> splitFic = {""};
                                            renderer->drawStringWithColoredSections(afterFanDiv, false, splitFic,
                                                rx, fanDrawY, fontsize, textColorA, catColorA);
                                        }
                                    }
                                }
                                if (!rOK) {
                                    renderer->drawStringWithColoredSections(skin_temperature_c, false, specialChars, current_x, tempDrawY, fontsize, textColorA, (settings.separatorColor));
                                    fanColX = current_x + renderer->getTextDimensions(skin_temperature_c, false, fontsize).first;
                                }
                            }
                            // Center connector divider (between fan and volt column)
                            if (SOC_volt_c[0])
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, fanColX, singleItemY, fontsize, (settings.separatorColor));
                            // Bottom row: optional HOC temps + SOC voltage
                            {
                                // Single-group: volt starts at fanColX; HOC: volt starts after bot temps
                                uint32_t rx = splitIsHoc ? current_x : fanColX;
                                if (splitIsHoc) {
                                    std::string botStr(splitBotTemps_c);
                                    size_t rpos = 0;
                                    bool rOK = true;
                                    for (int tc = 0; tc < 3 && rOK && rpos < botStr.length(); tc++) {
                                        while (rpos < botStr.length() && botStr[rpos] == ' ') {
                                            renderer->drawString(" ", false, rx, tmpVoltY, fontsize, textColorA);
                                            rx += renderer->getTextDimensions(" ", false, fontsize).first;
                                            rpos++;
                                        }
                                        if (rpos >= botStr.length()) break;
                                        const size_t dp = botStr.find("°", rpos);
                                        if (dp == std::string::npos) { rOK = false; break; }
                                        const size_t cp = botStr.find("C", dp);
                                        if (cp == std::string::npos) { rOK = false; break; }
                                        const std::string tp = botStr.substr(rpos, cp + 1 - rpos);
                                        const int tv = atoi(tp.c_str());
                                        renderer->drawStringWithColoredSections(tp, false, specialChars, rx, tmpVoltY, fontsize,
                                            tsl::GradientColor((float)tv), (settings.separatorColor));
                                        rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                        rpos = cp + 1;
                                    }
                                    if (!rOK) {
                                        renderer->drawStringWithColoredSections(splitBotTemps_c, false, specialChars, current_x, tmpVoltY, fontsize, textColorA, (settings.separatorColor));
                                        rx = current_x + renderer->getTextDimensions(splitBotTemps_c, false, fontsize).first;
                                    }
                                }
                                // Draw SOC voltage
                                if (SOC_volt_c[0]) {
                                    rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, voltDrawY, fontsize, (settings.separatorColor)).first;
                                    renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars,
                                        rx, voltDrawY, fontsize, textColorA, (settings.separatorColor));
                                }
                            }
                        } else if (tmpIsGrid) {
                            // —— GRID MODE: two rows ——
                            // Top row: CPU/GPU/RAM die temps with HIGH gradient
                            {
                                std::string rowStr(componentTemps_c);
                                uint32_t rx = current_x;
                                size_t rpos = 0;
                                bool rOK = true;
                                for (int tc = 0; tc < 3 && rOK && rpos < rowStr.length(); tc++) {
                                    while (rpos < rowStr.length() && rowStr[rpos] == ' ') {
                                        renderer->drawString(" ", false, rx, gridTopY, fontsize, textColorA);
                                        rx += renderer->getTextDimensions(" ", false, fontsize).first;
                                        rpos++;
                                    }
                                    if (rpos >= rowStr.length()) break;
                                    const size_t dp = rowStr.find("°", rpos);
                                    if (dp == std::string::npos) { rOK = false; break; }
                                    const size_t cp = rowStr.find("C", dp);
                                    if (cp == std::string::npos) { rOK = false; break; }
                                    const std::string tp = rowStr.substr(rpos, cp + 1 - rpos);
                                    const int tv = atoi(tp.c_str());
                                    renderer->drawStringWithColoredSections(tp, false, specialChars, rx, gridTopY, fontsize,
                                        tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH), (settings.separatorColor));
                                    rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                    rpos = cp + 1;
                                }
                                if (!rOK) renderer->drawStringWithColoredSections(componentTemps_c, false, specialChars, current_x, gridTopY, fontsize, textColorA, (settings.separatorColor));
                            }
                            // Bottom row: SOC/PCB/Skin + fan with standard gradient
                            {
                                std::string rowStr(skin_temperature_c);
                                uint32_t rx = current_x;
                                size_t rpos = 0;
                                bool rOK = true;
                                for (int tc = 0; tc < 3 && rOK && rpos < rowStr.length(); tc++) {
                                    while (rpos < rowStr.length() && rowStr[rpos] == ' ') {
                                        renderer->drawString(" ", false, rx, gridBotY, fontsize, textColorA);
                                        rx += renderer->getTextDimensions(" ", false, fontsize).first;
                                        rpos++;
                                    }
                                    if (rpos >= rowStr.length()) break;
                                    const size_t dp = rowStr.find("°", rpos);
                                    if (dp == std::string::npos) { rOK = false; break; }
                                    const size_t cp = rowStr.find("C", dp);
                                    if (cp == std::string::npos) { rOK = false; break; }
                                    const std::string tp = rowStr.substr(rpos, cp + 1 - rpos);
                                    const int tv = atoi(tp.c_str());
                                    renderer->drawStringWithColoredSections(tp, false, specialChars, rx, gridBotY, fontsize,
                                        tsl::GradientColor((float)tv), (settings.separatorColor));
                                    rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                    rpos = cp + 1;
                                }
                                if (rpos < rowStr.length()) {
                                    const std::string rest = rowStr.substr(rpos);
                                    const size_t dv = rest.find(ult::DIVIDER_SYMBOL);
                                    if (dv != std::string::npos) {
                                        const size_t dl = ult::DIVIDER_SYMBOL.length();
                                        rx += renderer->drawString(rest.substr(0, dv + dl), false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                                        const std::string ad = rest.substr(dv + dl);
                                        if (!ad.empty()) {
                                            static const std::vector<std::string> fic2 = {""};
                                            renderer->drawStringWithColoredSections(ad, false, fic2, rx, singleItemY, fontsize, textColorA, catColorA);
                                        }
                                    } else {
                                        renderer->drawStringWithColoredSections(rest, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                    }
                                }
                                if (!rOK) renderer->drawStringWithColoredSections(skin_temperature_c, false, specialChars, current_x, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        } else {
                            // —— SINGLE-ROW MODE (SBS, comp-only, or soc-only) ——
                            // When only component temps: use HIGH gradient; otherwise standard
                            const bool compOnly = settings.showComponentTemps && !settings.showSocPcbSkinTemps;
                            std::string dataStr(item.data_ptr);
                            uint32_t renderX = current_x;
                            size_t pos = 0;
                            bool parseSuccess = true;
                            // For SBS mode (divider present) parse up to 6 temps, switching gradient at divider
                            bool pastDivider = false;
                            for (int tempCount = 0; tempCount < 6 && parseSuccess && pos < dataStr.length(); tempCount++) {
                                while (pos < dataStr.length() && dataStr[pos] == ' ') {
                                    renderer->drawString(" ", false, renderX, singleItemY, fontsize, textColorA);
                                    renderX += renderer->getTextDimensions(" ", false, fontsize).first;
                                    pos++;
                                }
                                if (pos >= dataStr.length()) break;
                                // Check for divider before a temp (SBS mode)
                                const size_t nextDivPos = dataStr.find(ult::DIVIDER_SYMBOL, pos);
                                const size_t nextDegPos = dataStr.find("°", pos);
                                if (nextDivPos != std::string::npos && nextDegPos != std::string::npos && nextDivPos < nextDegPos) {
                                    // Draw divider and switch gradient
                                    const size_t dl = ult::DIVIDER_SYMBOL.length();
                                    renderX += renderer->drawString(dataStr.substr(nextDivPos, dl), false, renderX, singleItemY, fontsize, (settings.separatorColor)).first;
                                    pos = nextDivPos + dl;
                                    pastDivider = true;
                                    continue;
                                }
                                const size_t degreesPos = nextDegPos;
                                if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                                const size_t cPos = dataStr.find("C", degreesPos);
                                if (cPos == std::string::npos) { parseSuccess = false; break; }
                                const std::string tempPart = dataStr.substr(pos, cPos + 1 - pos);
                                const int temp = atoi(tempPart.c_str());
                                // HIGH gradient for component temps (before divider in SBS, or compOnly)
                                const bool useHigh = compOnly || (settings.showSideBySideTemps && !pastDivider && settings.showComponentTemps);
                                const tsl::Color tempColor = useHigh ?
                                    tsl::GradientColor((float)temp, tsl::DEFAULT_TEMP_RANGE_HIGH) :
                                    tsl::GradientColor((float)temp);
                                renderer->drawStringWithColoredSections(tempPart, false, specialChars, renderX, singleItemY, fontsize, tempColor, (settings.separatorColor));
                                renderX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                                pos = cPos + 1;
                            }
                            // Render any remaining text (fan%, etc.) - only if we drew at least one temp
                            if (pos < dataStr.length() && (parseSuccess || renderX > current_x)) {
                                const std::string restPart = dataStr.substr(pos);
                                const size_t divPos2 = restPart.find(ult::DIVIDER_SYMBOL);
                                if (divPos2 != std::string::npos) {
                                    const size_t dl = ult::DIVIDER_SYMBOL.length();
                                    renderX += renderer->drawString(restPart.substr(0, divPos2 + dl), false, renderX, singleItemY, fontsize, (settings.separatorColor)).first;
                                    const std::string afterDiv = restPart.substr(divPos2 + dl);
                                    if (!afterDiv.empty()) {
                                        static const std::vector<std::string> fic3 = {""};
                                        renderer->drawStringWithColoredSections(afterDiv, false, fic3, renderX, singleItemY, fontsize, textColorA, catColorA);
                                    }
                                } else {
                                    renderer->drawStringWithColoredSections(restPart, false, specialChars, renderX, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                }
                            }
                            if (!parseSuccess && renderX == current_x) {
                                // Full fallback only if nothing was successfully drawn
                                renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                            }
                        }

                    } else {
                        // Normal rendering for all other item types
                        renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    }
                } else {
                    // Dynamic colors off: normal rendering
                    renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                }
                current_x += item_layout.data_width;

                // Draw voltage if present -- inline at singleItemY for all modes including TMP grid
                if (item.has_voltage && item.volt_ptr) {
                    current_x += layout.volt_separator_gap;
                    renderer->drawString("", false, current_x, singleItemY, fontsize, (settings.separatorColor));
                    current_x += sep_width + layout.volt_data_gap;
                    renderer->drawStringWithColoredSections(item.volt_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    current_x += item_layout.volt_width; // advance past volt for SBS temp positioning
                }

                // RAM split: VDD2/VDDQ on top/bottom rows.
                // VoltAtEnd OFF: normal (VDD2 top, VDDQ+temp bottom, SBS temp centered).
                // VoltAtEnd ON + SBS off: temp+DIV+VDD2 on top, VDDQ on bottom.
                // VoltAtEnd ON + SBS on:  temp immediately at voltColX+div (before volt columns).
                if (ramIsSplit && item.type == 2) {
                    const uint32_t voltColX = current_x;
                    const uint32_t rdiv_w = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, singleItemY, fontsize, (settings.separatorColor));
                    if (!settings.voltageAtEndRAM) {
                        // ── Normal ──
                        if (vdd2_only_c[0]) {
                            uint32_t rx = voltColX;
                            rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                            renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                        }
                        if (vddq_only_c[0]) {
                            uint32_t rx = voltColX;
                            rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                            renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            if (settings.showRAMTemp && !settings.showSideBySideRAMTemp && ram_temp_c[0]) {
                                rx += renderer->getTextDimensions(vddq_only_c, false, fontsize).first;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, rx, gridBotY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                            }
                        }
                        if (settings.showRAMTemp && settings.showSideBySideRAMTemp && ram_temp_c[0]) {
                            const uint32_t vdd2_rw = vdd2_only_c[0] ? renderer->getTextDimensions(vdd2_only_c, false, fontsize).first : 0;
                            const uint32_t vddq_rw = vddq_only_c[0] ? renderer->getTextDimensions(vddq_only_c, false, fontsize).first : 0;
                            uint32_t cx = voltColX + rdiv_w + std::max(vdd2_rw, vddq_rw);
                            cx += renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, singleItemY, fontsize, (settings.separatorColor)).first;
                            const int rtv = atoi(ram_temp_c);
                            renderer->drawString(ram_temp_c, false, cx, singleItemY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    } else {
                        // ── VoltAtEnd ON ──
                        if (!settings.showSideBySideRAMTemp && ram_temp_c[0]) {
                            // SBS off: temp+DIV+VDD2 on top row, VDDQ on bottom row
                            if (vdd2_only_c[0]) {
                                uint32_t rx = voltColX;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, rx, gridTopY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                                rx += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                            }
                            if (vddq_only_c[0]) {
                                uint32_t rx = voltColX;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        } else {
                            // SBS on + VoltAtEnd: temp at center first (left-connector IS the divider before temp),
                            // then a right-fork connector, then VDD2/VDDQ to the right.
                            // Layout from voltColX:  ╸[temp]╸[VDD2 top]
                            //                        ╸[temp]╸  (right connector at center)
                            //                        ╸[temp]╸[VDDQ bot]
                            uint32_t voltStartX = voltColX + rdiv_w; // right after left connector; temp starts here
                            if (settings.showRAMTemp && settings.showSideBySideRAMTemp && ram_temp_c[0]) {
                                // Temp at center row — the left connector already drawn IS the ╸ before temp
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, voltStartX, singleItemY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                                voltStartX += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                                // Draw right-fork connector at singleItemY before the volt column
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, voltStartX, singleItemY, fontsize, (settings.separatorColor));
                            }
                            if (vdd2_only_c[0]) {
                                uint32_t rx = voltStartX;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                            }
                            if (vddq_only_c[0]) {
                                uint32_t rx = voltStartX;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        }
                    }
                }

                // CPU split: volt/temp on top/bottom rows; swapped when voltageAtEndCPU
                if (cpuIsSplit && item.type == 0) {
                    const uint32_t voltColX = current_x;
                    const int32_t cpuVoltY = settings.voltageAtEndCPU ? gridBotY : gridTopY;
                    const int32_t cpuTempY = settings.voltageAtEndCPU ? gridTopY : gridBotY;
                    renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, singleItemY, fontsize, (settings.separatorColor));
                    if (CPU_volt_c[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuVoltY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars, rx, cpuVoltY, fontsize, textColorA, (settings.separatorColor));
                    }
                    if (cpu_temp_c[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuTempY, fontsize, (settings.separatorColor)).first;
                        const int tv = atoi(cpu_temp_c);
                        renderer->drawString(cpu_temp_c, false, rx, cpuTempY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }

                // GPU split: volt/temp on top/bottom rows; swapped when voltageAtEndGPU
                if (gpuIsSplit && item.type == 1) {
                    const uint32_t voltColX = current_x;
                    const int32_t gpuVoltY = settings.voltageAtEndGPU ? gridBotY : gridTopY;
                    const int32_t gpuTempY = settings.voltageAtEndGPU ? gridTopY : gridBotY;
                    renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, singleItemY, fontsize, (settings.separatorColor));
                    if (GPU_volt_c[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gpuVoltY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(GPU_volt_c, false, specialChars, rx, gpuVoltY, fontsize, textColorA, (settings.separatorColor));
                    }
                    if (gpu_temp_c[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gpuTempY, fontsize, (settings.separatorColor)).first;
                        const int tv = atoi(gpu_temp_c);
                        renderer->drawString(gpu_temp_c, false, rx, gpuTempY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }

                // RAM temp split (single volt rail): volt/temp on top/bottom; swapped when voltageAtEndRAM
                if (ramTempSplit && item.type == 2) {
                    const uint32_t voltColX = current_x;
                    const int32_t rtsVoltY = settings.voltageAtEndRAM ? gridBotY : gridTopY;
                    const int32_t rtsTempY = settings.voltageAtEndRAM ? gridTopY : gridBotY;
                    renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, singleItemY, fontsize, (settings.separatorColor));
                    const char* topVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                    if (topVolt[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, rtsVoltY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(topVolt, false, specialChars, rx, rtsVoltY, fontsize, textColorA, (settings.separatorColor));
                    }
                    if (ram_temp_c[0]) {
                        uint32_t rx = voltColX;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, rtsTempY, fontsize, (settings.separatorColor)).first;
                        const int tv = atoi(ram_temp_c);
                        renderer->drawString(ram_temp_c, false, rx, rtsTempY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }

                // SBS CPU temp: normal = DIV+temp after volt; voltAtEnd = sep+DIV+temp+volt (volt drawn last)
                if (!cpuIsSplit && item.type == 0 && settings.showCPUTemp && settings.showSideBySideCPUTemp && cpu_temp_c[0]) {
                    const int tv = atoi(cpu_temp_c);
                    if (settings.voltageAtEndCPU && settings.realVolts && CPU_volt_c[0]) {
                        // has_voltage=false; reordered: DIV + temp + DIV + volt
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(cpu_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        rx += renderer->getTextDimensions(cpu_temp_c, false, fontsize).first;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    } else {
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(cpu_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }

                // SBS GPU temp: normal = DIV+temp after volt; voltAtEnd = sep+DIV+temp+volt
                if (!gpuIsSplit && item.type == 1 && settings.showGPUTemp && settings.showSideBySideGPUTemp && gpu_temp_c[0]) {
                    const int tv = atoi(gpu_temp_c);
                    if (settings.voltageAtEndGPU && settings.realVolts && GPU_volt_c[0]) {
                        // reordered: DIV + temp + DIV + volt
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(gpu_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        rx += renderer->getTextDimensions(gpu_temp_c, false, fontsize).first;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(GPU_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    } else {
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(gpu_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }

                // RAM temp inline (non-split, non-ramTempSplit): DIV+temp; voltAtEnd = sep+DIV+temp+volt
                if (!ramIsSplit && !ramTempSplit && item.type == 2 && settings.showRAMTemp && ram_temp_c[0]) {
                    const int tv = atoi(ram_temp_c);
                    if (settings.voltageAtEndRAM && settings.realVolts && RAM_volt_c[0]) {
                        // reordered: DIV + temp + DIV + volt
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(ram_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        rx += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(RAM_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    } else {
                        uint32_t rx = current_x;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawString(ram_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                    }
                }
                        }
        });
        
        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);
        return rootFrame;
    }

    virtual void update() override {
        if (triggerExitNow)
            return;

        if (!SaltySD) {
            SaltySD = CheckPort();

            if (SaltySD) {
                LoadSharedMemory();
                //Assign NX-FPS to default core
                threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
                threadStart(&t6);
            }
        }

        //static bool triggerExit = false;
        //if (triggerExit) {
        //    ult::setIniFileValue(
        //        ult::ULTRAHAND_CONFIG_INI_PATH,
        //        ult::ULTRAHAND_PROJECT_NAME,
        //        ult::IN_OVERLAY_STR,
        //        ult::FALSE_STR
        //    );
        //    tsl::setNextOverlay(
        //        ult::OVERLAY_PATH + "ovlmenu.ovl"
        //    );
        //    tsl::Overlay::get()->close();
        //    return;
        //}

        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            if (fontsize != settings.handheldFontSize) {
                Initialized = false;
                layout.calculated = false; // Recalculate layout for new font size
                fontsize = settings.handheldFontSize;
            }
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            if (fontsize != settings.dockedFontSize) {
                Initialized = false;
                layout.calculated = false; // Recalculate layout for new font size
                fontsize = settings.dockedFontSize;
            }
        }

        // CPU usage calculations - optimized with fewer conditionals
        const double inv_freq = 1.0 / systemtickfrequency_impl;
        
        // Capture systemtickfrequency_impl and inv_freq safely
        const auto formatUsage = [this](char* buf, size_t size, uint64_t idletick, double inv_freq) {
            if (idletick > systemtickfrequency_impl) {
                strcpy(buf, "0%");
            } else {
                snprintf(buf, size, "%.0f%%", (1.0 - (idletick * inv_freq)) * 100.0);
            }
        };
        
        // Atomically load idle ticks before using them
        const uint64_t idle0 = idletick0.load(std::memory_order_acquire);
        const uint64_t idle1 = idletick1.load(std::memory_order_acquire);
        const uint64_t idle2 = idletick2.load(std::memory_order_acquire);
        const uint64_t idle3 = idletick3.load(std::memory_order_acquire);
        
        formatUsage(CPU_Usage0, sizeof(CPU_Usage0), idle0, inv_freq);
        formatUsage(CPU_Usage1, sizeof(CPU_Usage1), idle1, inv_freq);
        formatUsage(CPU_Usage2, sizeof(CPU_Usage2), idle2, inv_freq);
        formatUsage(CPU_Usage3, sizeof(CPU_Usage3), idle3, inv_freq);

        mutexLock(&mutex_Misc);
        
        // CPU frequency and voltage
        const char* cpuDiff = "@";
        if (realCPU_Hz) {
            const int32_t deltaCPU = (int32_t)(realCPU_Hz / 1000) - (CPU_Hz / 1000);
            cpuDiff = getDifferenceSymbol(deltaCPU);
        }
        
        const uint32_t cpuFreq = settings.realFrequencies && realCPU_Hz ? realCPU_Hz : CPU_Hz;
        
        if (settings.showFullCPU) {
            snprintf(CPU_compressed_c, sizeof(CPU_compressed_c), 
                "[%s,%s,%s,%s]%s%u.%u", 
                CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3, 
                cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
        } else {
            // Find max CPU usage across all cores
            const auto extractUsage = [](const char* usage_str) -> double {
                return strtod(usage_str, nullptr);
            };
            
            const double usage0 = extractUsage(CPU_Usage0);
            const double usage1 = extractUsage(CPU_Usage1);
            const double usage2 = extractUsage(CPU_Usage2);
            const double usage3 = extractUsage(CPU_Usage3);
            
            const double maxUsage = std::max({usage0, usage1, usage2, usage3});
            
            snprintf(CPU_compressed_c, sizeof(CPU_compressed_c), 
                "%.0f%%%s%u.%u", 
                maxUsage, cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
        }
            
        //if (settings.realVolts) {
        //    snprintf(CPU_volt_c, sizeof(CPU_volt_c), "%u.%u mV", 
        //        realCPU_mV/1000, (isMariko ? (realCPU_mV/100)%10 : (realCPU_mV/10)%100));
        //}

        /* ── CPU voltage ───────────────────────────── */
        if (settings.realVolts) {
            const uint32_t mv = realCPU_mV / 1000;                 // µV → mV
            snprintf(CPU_volt_c, sizeof(CPU_volt_c), "%u mV", mv);
        }
        
        // GPU frequency and voltage
        const char* gpuDiff = "@";
        if (realGPU_Hz) {
            const int32_t deltaGPU = (int32_t)(realGPU_Hz / 1000) - (GPU_Hz / 1000);
            gpuDiff = getDifferenceSymbol(deltaGPU);
        }
        
        const uint32_t gpuFreq = settings.realFrequencies && realGPU_Hz ? realGPU_Hz : GPU_Hz;
        snprintf(GPU_Load_c, sizeof(GPU_Load_c),
                 "%u%%%s%u.%u",
                 GPU_Load_u / 10,
                 gpuDiff, gpuFreq / 1000000, (gpuFreq / 100000) % 10);
            
        //if (settings.realVolts) {
        //    snprintf(GPU_volt_c, sizeof(GPU_volt_c), "%u.%u mV", 
        //        realGPU_mV/1000, (isMariko ? (realGPU_mV/100)%10 : (realGPU_mV/10)%100));
        //}

        

        /* ── GPU voltage ───────────────────────────── */
        if (settings.realVolts) {
            const uint32_t mv = realGPU_mV / 1000;
            snprintf(GPU_volt_c, sizeof(GPU_volt_c), "%u mV", mv);
        }
        
        // RAM usage and frequency
        char MICRO_RAM_all_c[16];
        if (!settings.showRAMLoad) {
            // User wants GB display
            const float RAM_Total_all_f = (RAM_Total_application_u + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u) / (1024.0f * 1024.0f * 1024.0f);
            const float RAM_Used_all_f = (RAM_Used_application_u + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u) / (1024.0f * 1024.0f * 1024.0f);
            snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%.0f%.0fGB", RAM_Used_all_f, RAM_Total_all_f);
        } else {
            // User wants percentage display
            if (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck)) {
                // Use IPC RAM load if available (sys-clk-hoc or hoc-clk)
                snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%hu%%",
                         ramLoad[SysClkRamLoad_All] / 10);
            } else {
                // Calculate percentage manually when sys-clk isn't available
                const uint64_t RAM_Total_all = RAM_Total_application_u + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u;
                const uint64_t RAM_Used_all = RAM_Used_application_u + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u;
                const unsigned ramLoadPercent = (RAM_Total_all > 0) ? (unsigned)((RAM_Used_all * 100) / RAM_Total_all) : 0;
                snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%u%%", ramLoadPercent);
            }
        }

        const char* ramDiff = "@";
        if (realRAM_Hz) {
            const int32_t deltaRAM = (int32_t)(realRAM_Hz / 1000) - (RAM_Hz / 1000);
            ramDiff = getDifferenceSymbol(deltaRAM);
        }
        
        const uint32_t ramFreq = settings.realFrequencies && realRAM_Hz ? realRAM_Hz : RAM_Hz;
        snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c), 
            "%s%s%u.%u", MICRO_RAM_all_c, ramDiff, 
            ramFreq / 1000000, (ramFreq / 100000) % 10);
            
        //if (settings.realVolts) {
        //    uint32_t vdd2 = realRAM_mV / 10000;
        //    uint32_t vddq = realRAM_mV % 10000;
        //    if (isMariko) {
        //        snprintf(RAM_volt_c, sizeof(RAM_volt_c), "%u.%u%u.%u mV", 
        //            vdd2/10, vdd2%10, vddq/10, vddq%10);
        //    } else {
        //        snprintf(RAM_volt_c, sizeof(RAM_volt_c), "%u.%u mV", vdd2/10, vdd2%10);
        //    }
        //}

        /* ── RAM voltage ───────────────────────────── */
        if (settings.realVolts && (settings.showVDD2 || settings.showVDDQ)) {
            /* realRAM_mV packs VDD2 | VDDQ in 10-µV units        *
             * → split, convert to mV                           */
            const float mv_vdd2 = (realRAM_mV / 10000) / 10.0f;   // VDD2
            const uint32_t mv_vddq = (realRAM_mV % 10000) / 10;   // VDDQ

            // Build separate single-volt strings (used by split VDD2/VDDQ mode)
            char temp_buffer[16];
            vdd2_only_c[0] = '\0';
            vddq_only_c[0] = '\0';
            if (settings.showVDD2) {
                if (settings.decimalVDD2)
                    snprintf(vdd2_only_c, sizeof(vdd2_only_c), "%.1f mV", mv_vdd2);
                else
                    snprintf(vdd2_only_c, sizeof(vdd2_only_c), "%u mV", (uint32_t)mv_vdd2);
            }
            if (settings.showVDDQ && isMariko)
                snprintf(vddq_only_c, sizeof(vddq_only_c), "%u mV", mv_vddq);

            // Build combined voltage string (side-by-side mode)
            RAM_volt_c[0] = '\0';
            if (settings.showVDD2) {
                if (settings.decimalVDD2) {
                    snprintf(temp_buffer, sizeof(temp_buffer), "%.1f mV", mv_vdd2);
                } else {
                    snprintf(temp_buffer, sizeof(temp_buffer), "%u mV", (uint32_t)mv_vdd2);
                }
                strcat(RAM_volt_c, temp_buffer);
            }
            if (settings.showVDDQ && isMariko) {
                if (RAM_volt_c[0] != '\0') {
                    strcat(RAM_volt_c, "");
                }
                snprintf(temp_buffer, sizeof(temp_buffer), "%u mV", mv_vddq);
                strcat(RAM_volt_c, temp_buffer);
            }
        } else {
            RAM_volt_c[0] = '\0'; // Empty if voltages disabled
        }
        
        /* ── Battery / power draw ───────────────────────────── */
        char remainingBatteryLife[8];

        /* Normalise “-0.00” → “0.00” W */
        const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f
                                                         : PowerConsumption;

        mutexLock(&mutex_BatteryChecker);

        /* show a time only when the estimate is valid **and** draw ≥ 0.01 W */
        if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
            snprintf(remainingBatteryLife, sizeof remainingBatteryLife,
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }

        if (!settings.invertBatteryDisplay) {
            snprintf(Battery_c, sizeof Battery_c,
                     "%.2f W%.1f%% [%s]",
                     drawW,
                     (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f,
                     remainingBatteryLife);
        } else {
            snprintf(Battery_c, sizeof Battery_c,
                     "%.1f%% [%s]%.2f W",
                     (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f,
                     remainingBatteryLife,
                     drawW);
        }

        mutexUnlock(&mutex_BatteryChecker);


        // Format current datetime for DTC
        if (settings.showDTC) {
            time_t rawtime = time(NULL);
            struct tm *timeinfo = localtime(&rawtime);
            strftime(DTC_c, sizeof(DTC_c), settings.dtcFormat.c_str(), timeinfo);
        }

        // Thermal info
        const int duty = safeFanDuty((int)Rotation_Duty);

        /* Integer SoC temperature + duty */
        snprintf(soc_temperature_c, sizeof soc_temperature_c,
                 "%d°C %d%%",
                 (int)SOC_temperatureF,          // SoC °C, no decimals
                 duty);                          // fan %
        
        /* Build temperature buffers based on active toggle settings */

        // Component die temps (CPU/GPU/RAM) — populated when showComponentTemps is on
        if (settings.showComponentTemps) {
            snprintf(componentTemps_c, sizeof componentTemps_c,
                     "%d°C %d°C %d°C",
                     (int)(componentCPU_mC / 1000),
                     (int)(componentGPU_mC / 1000),
                     (int)(componentRAM_mC / 1000));
        } else {
            componentTemps_c[0] = '\0';
        }

        // Individual die temps for per-component temp display
        if (settings.showCPUTemp)
            snprintf(cpu_temp_c, sizeof cpu_temp_c, "%d\u00B0C", (int)(componentCPU_mC / 1000));
        else
            cpu_temp_c[0] = '\0';

        if (settings.showGPUTemp)
            snprintf(gpu_temp_c, sizeof gpu_temp_c, "%d\u00B0C", (int)(componentGPU_mC / 1000));
        else
            gpu_temp_c[0] = '\0';

        if (settings.showRAMTemp)
            snprintf(ram_temp_c, sizeof ram_temp_c, "%d\u00B0C", (int)(componentRAM_mC / 1000));
        else
            ram_temp_c[0] = '\0';

        // SOC/PCB/Skin row string for item.data_ptr (non-split modes):
        //  - Split         (!showSideBySideFanSOC): top=temps+fan, bot=temps only
        //  - Side-By-Side  (both on, SBS on):       compTemps +  + SOC/PCB/Skin+fan
        //  - Grid          (both on, SBS off):       SOC/PCB/Skin+fan  (compTemps read from member)
        //  - Comp-only     (comp on, soc off):       CPU/GPU/RAM+fan
        //  - Soc-only      (default):                SOC/PCB/Skin+fan
        const bool splitFanSOC = !settings.showSideBySideFanSOC &&
                                  settings.realVolts && settings.showSOCVoltage;
        if (splitFanSOC) {
            // Split mode: top row = temps + DIVIDER + fan%, bottom row = temps only (or empty)
            // skin_temperature_c = top row data; splitBotTemps_c = bottom row temps
            if (settings.showComponentTemps) {
                // Top row: comp temps + fan
                std::string topStr = std::string(componentTemps_c) +
                                     " " + std::to_string(duty) + "%";
                strncpy(skin_temperature_c, topStr.c_str(), sizeof(skin_temperature_c) - 1);
                skin_temperature_c[sizeof(skin_temperature_c) - 1] = '\0';
                if (settings.showSocPcbSkinTemps) {
                    // Bottom row: SOC/PCB/Skin temps (volt drawn by renderer)
                    snprintf(splitBotTemps_c, sizeof splitBotTemps_c,
                             "%d°C %d°C %hu°C",
                             (int)SOC_temperatureF, (int)PCB_temperatureF,
                             (uint16_t)(skin_temperaturemiliC / 1000));
                } else {
                    splitBotTemps_c[0] = '\0'; // empty bottom row
                }
            } else {
                // Top row: SOC/PCB/Skin temps + fan
                char socStr[48];
                snprintf(socStr, sizeof socStr, "%d°C %d°C %hu°C",
                         (int)SOC_temperatureF, (int)PCB_temperatureF,
                         (uint16_t)(skin_temperaturemiliC / 1000));
                std::string topStr = std::string(socStr) + " " + std::to_string(duty) + "%";
                strncpy(skin_temperature_c, topStr.c_str(), sizeof(skin_temperature_c) - 1);
                skin_temperature_c[sizeof(skin_temperature_c) - 1] = '\0';
                splitBotTemps_c[0] = '\0'; // empty bottom row
            }
        } else if (settings.showComponentTemps && settings.showSocPcbSkinTemps && settings.showSideBySideTemps) {
            // Side-by-side: "CPU GPU RAM  SOC PCB Skin fan"
            char socPcbSkin_c[48];
            snprintf(socPcbSkin_c, sizeof socPcbSkin_c,
                     "%d°C %d°C %hu°C %d%%",
                     (int)SOC_temperatureF,
                     (int)PCB_temperatureF,
                     (uint16_t)(skin_temperaturemiliC / 1000),
                     duty);
            snprintf(skin_temperature_c, sizeof skin_temperature_c,
                     "%s%s",
                     componentTemps_c, socPcbSkin_c);
            splitBotTemps_c[0] = '\0';
        } else if (settings.showComponentTemps && !settings.showSocPcbSkinTemps) {
            // Component temps only: include divider + fan icon + duty so fan renders correctly
            snprintf(skin_temperature_c, sizeof skin_temperature_c, "%s %d%%", componentTemps_c, duty);
            splitBotTemps_c[0] = '\0';
        } else {
            // SOC/PCB/Skin (default) or grid mode bottom row: SOC PCB Skin fan%
            snprintf(skin_temperature_c, sizeof skin_temperature_c,
                     "%d°C %d°C %hu°C %d%%",
                     (int)SOC_temperatureF,
                     (int)PCB_temperatureF,
                     (uint16_t)(skin_temperaturemiliC / 1000),
                     duty);
            splitBotTemps_c[0] = '\0';
        }
        
        //if (settings.realVolts) {
        //    snprintf(SOC_volt_c, sizeof(SOC_volt_c), "%u.%u mV", 
        //        realSOC_mV/1000, (realSOC_mV/100)%10);
        //}

        /* ── SoC voltage ───────────────────────────── */
        if (settings.realVolts && settings.showSOCVoltage) {
            const uint32_t mv = realSOC_mV / 1000;
            snprintf(SOC_volt_c, sizeof(SOC_volt_c), "%u mV", mv);
        } else {
            SOC_volt_c[0] = '\0'; // Clear the buffer when disabled
        }

        // Resolution processing
        //char RES_var_compressed_c[32] = "";
        if (GameRunning && NxFps && resolutionShow) {
            if (!resolutionLookup) {
                if (NxFps && SharedMemoryUsed) {
                    (NxFps -> renderCalls[0].calls) = 0xFFFF;
                    resolutionLookup = 1;
                }
            }
            else if (resolutionLookup == 1) {
                if (NxFps && SharedMemoryUsed && (NxFps -> renderCalls[0].calls) != 0xFFFF) {
                    resolutionLookup = 2;
                }
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

                // Prioritize 16:9 aspect ratios (e.g. 1280x720, 1920x1080) so the
                // actual game render resolution appears before UI/buffer resolutions.
                // stable_partition preserves call-count ordering within each group.
                std::stable_partition(m_resolutionOutput, m_resolutionOutput + 8,
                    [](const resolutionCalls& r) {
                        return r.width != 0 && (r.width * 9 == r.height * 16);
                    });

                // Anti-flicker swap logic
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
                
                // Format resolution string
                if (m_resolutionOutput[0].width) {
                    if (settings.showFullResolution) {
                        if (!m_resolutionOutput[1].width) {
                            snprintf(RES_var_compressed_c, sizeof(RES_var_compressed_c), "%dx%d", 
                                m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                        }
                        else {
                            snprintf(RES_var_compressed_c, sizeof(RES_var_compressed_c), "%dx%d%dx%d", 
                                m_resolutionOutput[0].width, m_resolutionOutput[0].height, 
                                m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                        }
                    } else {
                        if (!m_resolutionOutput[1].width) {
                            snprintf(RES_var_compressed_c, sizeof(RES_var_compressed_c), "%dp", 
                                m_resolutionOutput[0].height);
                        }
                        else {
                            snprintf(RES_var_compressed_c, sizeof(RES_var_compressed_c), "%dp%dp", 
                                m_resolutionOutput[0].height, m_resolutionOutput[1].height);
                        }
                    }
                }
                
                // Always store current resolutions for next frame comparison
                old_res[0] = std::make_pair(m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                old_res[1] = std::make_pair(m_resolutionOutput[1].width, m_resolutionOutput[1].height);
            }
        }
        else if (!GameRunning && resolutionLookup != 0) {
            resolutionLookup = 0;
        }

        // FPS
        snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "%2.1f", useOldFPSavg ? FPSavg_old : FPSavg);


        // Read Speed
        if (GameRunning && NxFps && SharedMemoryUsed) {
            float readSpeed = 0.0f;
            memcpy(&readSpeed, &(NxFps->readSpeedPerSecond), sizeof(float));
            if (readSpeed != 0.f) {
                snprintf(READ_var_compressed_c, sizeof(READ_var_compressed_c), "%.2f MiB/s", readSpeed / 1048576.f);
            } else {
                strcpy(READ_var_compressed_c, "n/d");
            }
        } else {
            strcpy(READ_var_compressed_c, "n/d");
        }

        mutexUnlock(&mutex_Misc);

        //static bool skipOnce = true;
    
        if (!skipOnce) {
            if (runOnce) {
                if (!(tsl::notification && tsl::notification->isActive())) {
                    isRendering = true;
                    leventClear(&renderingStopEvent);
                } else {
                    wasRendering = true;
                }
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
            //triggerRumbleDoubleClick.store(true, std::memory_order_release);
            skipOnce = true;
            runOnce = true;
            //TeslaFPS = 60;
            if (skipMain) {
                //lastSelectedItem = "Micro";
                lastMode = "returning";
                //tsl::swapTo<MainMenu>();
                tsl::goBack();
            }
            else {
                tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem Micro");
                tsl::Overlay::get()->close();
            }
            return true;
        }
        return false;
    }
};