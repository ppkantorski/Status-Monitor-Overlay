class MicroOverlay : public tsl::Gui {
private:
    char GPU_Load_c[32];
    char RAM_var_compressed_c[128];
    char CPU_compressed_c[160];
    char CPU_Usage0[32];
    char CPU_Usage1[32];
    char CPU_Usage2[32];
    char CPU_Usage3[32];
    char soc_temperature_c[32];
    char skin_temperature_c[32];
    char FPS_var_compressed_c[64];
    char Battery_c[32];
    char CPU_volt_c[16];
    char GPU_volt_c[16];
    char RAM_volt_c[32];
    char SOC_volt_c[16];

    const uint32_t margin = 4;

    // Performance optimization members
    bool Initialized = false;
    MicroSettings settings;
    size_t text_width = 0;
    size_t fps_width = 0;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    size_t fontsize = 0;
    bool showFPS = false;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    
    // Pre-compiled render data structures
    struct RenderItem {
        uint8_t type;
        const char* label;
        const char* data_ptr;
        const char* volt_ptr;
        bool has_voltage;
    };
    
    std::vector<RenderItem> renderItems;
    uint32_t cachedMargin = 0;
    tsl::Color catColorA = 0;
    tsl::Color textColorA = 0;
    uint32_t base_y = 0;
    bool renderDataDirty = true;
    
    // Fixed spacing system - calculate actual widths at render time
    struct LayoutMetrics {
        uint32_t label_data_gap = 8;      // Fixed gap between label and data
        uint32_t volt_separator_gap = 8;   // Fixed gap before voltage separator
        uint32_t volt_data_gap = 8;        // Fixed gap after voltage separator
        uint32_t item_spacing = 16;        // Minimum spacing between complete items
        uint32_t side_margin = 6;          // Margins on left and right
        bool calculated = false;
    } layout;
    
    // Lookup table for difference symbols
    static constexpr const char* diffSymbols[4] = {"△", "@", "▽", "≠"};
    
    inline const char* getDifferenceSymbol(int32_t delta) {
        if (delta > 20000) return diffSymbols[0];      // △
        if (delta > -20000) return diffSymbols[1];     // @
        if (delta < -50000) return diffSymbols[3];     // ≠
        return diffSymbols[2];                         // ▽
    }
    
    void calculateLayoutMetrics(tsl::gfx::Renderer *renderer) {
        if (layout.calculated) return;
        
        // Use font size to determine appropriate spacing
        if (fontsize <= 16) {
            layout.label_data_gap = 6;
            layout.volt_separator_gap = 6;
            layout.volt_data_gap = 6;
            layout.item_spacing = 12;
        } else if (fontsize <= 20) {
            layout.label_data_gap = 8;
            layout.volt_separator_gap = 8;
            layout.volt_data_gap = 8;
            layout.item_spacing = 16;
        } else {
            layout.label_data_gap = 10;
            layout.volt_separator_gap = 10;
            layout.volt_data_gap = 10;
            layout.item_spacing = 20;
        }
        
        layout.calculated = true;
    }

public:
    MicroOverlay() { 
        tsl::initializeUltrahandSettings();
        GetConfigSettings(&settings);
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else fontsize = settings.dockedFontSize;
        if (settings.setPosBottom) {
            auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
            tsl::gfx::Renderer::get().setLayerPos(0, 1038-verticalUnderscanPixels);
        }
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        alphabackground = 0x0;
        deactivateOriginalFooter = true;
        
        // Pre-allocate render items vector
        renderItems.reserve(8);
        
        StartThreads();
    }
    
    ~MicroOverlay() {
        CloseThreads();
    }
    
    // Fast parsing and render item preparation
    void prepareRenderItems() {
        if (!renderDataDirty) return;
        
        renderItems.clear();
        
        // Fast manual parsing of settings.show
        const std::string& show = settings.show;
        size_t start = 0, end = 0;
        uint8_t seen_flags = 0;
        
        while (start < show.length()) {
            end = show.find('+', start);
            if (end == std::string::npos) end = show.length();
            
            size_t len = end - start;
            if (len >= 3) {
                const char* key = &show[start];
                
                // Use first 3 chars for fast comparison
                uint32_t key3 = (key[0] << 16) | (key[1] << 8) | key[2];
                
                switch (key3) {
                    case 0x435055: // "CPU"
                        if (!(seen_flags & 1)) {
                            renderItems.push_back({0, "CPU", CPU_compressed_c, CPU_volt_c, settings.realVolts});
                            seen_flags |= 1;
                        }
                        break;
                    case 0x475055: // "GPU"
                        if (!(seen_flags & 2)) {
                            renderItems.push_back({1, "GPU", GPU_Load_c, GPU_volt_c, settings.realVolts});
                            seen_flags |= 2;
                        }
                        break;
                    case 0x52414D: // "RAM"
                        if (!(seen_flags & 4)) {
                            renderItems.push_back({2, "RAM", RAM_var_compressed_c, RAM_volt_c, settings.realVolts});
                            seen_flags |= 4;
                        }
                        break;
                    case 0x534F43: // "SOC"
                        if (!(seen_flags & 8)) {
                            renderItems.push_back({3, "SOC", soc_temperature_c, SOC_volt_c, settings.realVolts});
                            seen_flags |= 8;
                        }
                        break;
                    case 0x544D50: // "TMP"
                        if (!(seen_flags & 16)) {
                            renderItems.push_back({4, "TMP", skin_temperature_c, SOC_volt_c, settings.realVolts});
                            seen_flags |= 16;
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
                }
            }
            start = end + 1;
        }
        
        renderDataDirty = false;
    }
    
    virtual tsl::elm::Element* createUI() override {
        rootFrame = new tsl::elm::OverlayFrame("", "");
    
        auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            
            if (!Initialized) {
                cachedMargin = renderer->drawString(" ", false, 0, fontsize, fontsize, renderer->a(0x0000)).first;
                catColorA = renderer->a(settings.catColor);
                textColorA = renderer->a(settings.textColor);
                base_y = settings.setPosBottom ? 
                    tsl::cfg::FramebufferHeight - (fontsize + (fontsize / 4)) : 2;
                Initialized = true;
                renderDataDirty = true;
                layout.calculated = false; // Force recalculation
                tsl::hlp::requestForeground(false);
            }
            
            // Prepare render items if settings changed
            prepareRenderItems();
            calculateLayoutMetrics(renderer);
            
            // Separate battery from other items
            std::vector<RenderItem> main_items;
            RenderItem* battery_item = nullptr;
            
            for (auto& item : renderItems) {
                if (item.type == 6) { // BAT
                    battery_item = &item;
                } else if (item.type == 5 && !GameRunning) {
                    // Skip FPS if no game running
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
            
            for (const auto& item : main_items) {
                ItemLayout item_layout = {};
                
                // Calculate actual label width
                auto label_dim = renderer->drawString(item.label, false, 0, 0, fontsize, renderer->a(0x0000));
                item_layout.label_width = label_dim.first;
                
                // Calculate actual data width
                auto data_dim = renderer->drawString(item.data_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
                item_layout.data_width = data_dim.first;
                
                // Calculate voltage width if present
                if (item.has_voltage && item.volt_ptr) {
                    auto volt_dim = renderer->drawString(item.volt_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
                    item_layout.volt_width = volt_dim.first;
                    
                    // Total: label + gap + data + gap + "|" + gap + voltage
                    auto sep_dim = renderer->drawString("", false, 0, 0, fontsize, renderer->a(0x0000));
                    item_layout.total_width = item_layout.label_width + layout.label_data_gap + 
                                            item_layout.data_width + layout.volt_separator_gap + 
                                            sep_dim.first + layout.volt_data_gap + item_layout.volt_width;
                } else {
                    // Total: label + gap + data
                    item_layout.total_width = item_layout.label_width + layout.label_data_gap + item_layout.data_width;
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
            if (battery_item) {
                auto bat_label_dim = renderer->drawString("BAT", false, 0, 0, fontsize, renderer->a(0x0000));
                auto bat_data_dim = renderer->drawString(battery_item->data_ptr, false, 0, 0, fontsize, renderer->a(0x0000));
                
                ItemLayout battery_layout = {};
                battery_layout.label_width = bat_label_dim.first;
                battery_layout.data_width = bat_data_dim.first;
                battery_layout.volt_width = 0;
                battery_layout.total_width = battery_layout.label_width + layout.label_data_gap + battery_layout.data_width;
                
                all_items_ordered.push_back(*battery_item);
                all_layouts_ordered.push_back(battery_layout);
            }
            
            // Calculate total width of all items
            uint32_t total_all_width = 0;
            for (const auto& item_layout : all_layouts_ordered) {
                total_all_width += item_layout.total_width;
            }
            
            // Calculate available space for distribution
            uint32_t available_width = tsl::cfg::FramebufferWidth - (2 * layout.side_margin);
            uint32_t remaining_space = available_width - total_all_width;
            
            // Calculate positions for even distribution
            std::vector<uint32_t> item_positions;
            
            if (all_items_ordered.size() <= 1) {
                // Single item: center it
                item_positions.push_back(layout.side_margin);
            } else {
                // Multiple items: distribute evenly across full width
                // First item starts at left margin, last item ends at right margin
                item_positions.push_back(layout.side_margin);
                
                if (all_items_ordered.size() > 1) {
                    uint32_t spacing_between = remaining_space / (all_items_ordered.size() - 1);
                    
                    for (size_t i = 1; i < all_items_ordered.size(); i++) {
                        uint32_t prev_pos = item_positions[i-1];
                        uint32_t prev_width = all_layouts_ordered[i-1].total_width;
                        item_positions.push_back(prev_pos + prev_width + spacing_between);
                    }
                }
            }
            
            // Render all items at calculated positions
            for (size_t i = 0; i < all_items_ordered.size(); i++) {
                const auto& item = all_items_ordered[i];
                const auto& item_layout = all_layouts_ordered[i];
                uint32_t current_x = item_positions[i];
                
                // Draw label
                renderer->drawString(item.label, false, current_x, base_y + fontsize, fontsize, catColorA);
                current_x += item_layout.label_width + layout.label_data_gap;
                
                // Draw data
                renderer->drawString(item.data_ptr, false, current_x, base_y + fontsize, fontsize, textColorA);
                current_x += item_layout.data_width;
                
                // Draw voltage if present
                if (item.has_voltage && item.volt_ptr) {
                    current_x += layout.volt_separator_gap;
                    renderer->drawString("", false, current_x, base_y + fontsize, fontsize, textColorA);
                    auto sep_dim = renderer->drawString("", false, 0, 0, fontsize, renderer->a(0x0000));
                    current_x += sep_dim.first + layout.volt_data_gap;
                    renderer->drawString(item.volt_ptr, false, current_x, base_y + fontsize, fontsize, textColorA);
                }
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
        const auto formatUsage = [this](char* buf, size_t size, uint64_t idletick, double inv_freq) {
            if (idletick > systemtickfrequency_impl) {
                strcpy(buf, "0%");
            } else {
                snprintf(buf, size, "%.0f%%", (1.0 - (idletick * inv_freq)) * 100.0);
            }
        };
        
        formatUsage(CPU_Usage0, sizeof(CPU_Usage0), idletick0, inv_freq);
        formatUsage(CPU_Usage1, sizeof(CPU_Usage1), idletick1, inv_freq);
        formatUsage(CPU_Usage2, sizeof(CPU_Usage2), idletick2, inv_freq);
        formatUsage(CPU_Usage3, sizeof(CPU_Usage3), idletick3, inv_freq);

        mutexLock(&mutex_Misc);
        
        // CPU frequency and voltage
        const char* cpuDiff = "@";
        if (realCPU_Hz) {
            int32_t deltaCPU = (int32_t)(realCPU_Hz / 1000) - (CPU_Hz / 1000);
            cpuDiff = getDifferenceSymbol(deltaCPU);
        }
        
        uint32_t cpuFreq = settings.realFrequencies && realCPU_Hz ? realCPU_Hz : CPU_Hz;
        snprintf(CPU_compressed_c, sizeof(CPU_compressed_c), 
            "[%s,%s,%s,%s]%s%u.%u", 
            CPU_Usage0, CPU_Usage1, CPU_Usage2, CPU_Usage3, 
            cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
            
        if (settings.realVolts) {
            snprintf(CPU_volt_c, sizeof(CPU_volt_c), "%u.%u mV", 
                realCPU_mV/1000, (isMariko ? (realCPU_mV/100)%10 : (realCPU_mV/10)%100));
        }
        
        // GPU frequency and voltage
        const char* gpuDiff = "@";
        if (realGPU_Hz) {
            int32_t deltaGPU = (int32_t)(realGPU_Hz / 1000) - (GPU_Hz / 1000);
            gpuDiff = getDifferenceSymbol(deltaGPU);
        }
        
        uint32_t gpuFreq = settings.realFrequencies && realGPU_Hz ? realGPU_Hz : GPU_Hz;
        snprintf(GPU_Load_c, sizeof(GPU_Load_c), 
            "%u.%u%%%s%u.%u", 
            GPU_Load_u / 10, GPU_Load_u % 10, 
            gpuDiff, gpuFreq / 1000000, (gpuFreq / 100000) % 10);
            
        if (settings.realVolts) {
            snprintf(GPU_volt_c, sizeof(GPU_volt_c), "%u.%u mV", 
                realGPU_mV/1000, (isMariko ? (realGPU_mV/100)%10 : (realGPU_mV/10)%100));
        }
        
        // RAM usage and frequency
        char MICRO_RAM_all_c[12];
        if (!settings.showRAMLoad || R_FAILED(sysclkCheck)) {
            float RAM_Total_all_f = (RAM_Total_application_u + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u) / (1024.0f * 1024.0f * 1024.0f);
            float RAM_Used_all_f = (RAM_Used_application_u + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u) / (1024.0f * 1024.0f * 1024.0f);
            snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%.1f%.1fGB", RAM_Used_all_f, RAM_Total_all_f);
        } else {
            snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%hu.%hhu%%", 
                ramLoad[SysClkRamLoad_All] / 10, ramLoad[SysClkRamLoad_All] % 10);
        }

        const char* ramDiff = "@";
        if (realRAM_Hz) {
            int32_t deltaRAM = (int32_t)(realRAM_Hz / 1000) - (RAM_Hz / 1000);
            ramDiff = getDifferenceSymbol(deltaRAM);
        }
        
        uint32_t ramFreq = settings.realFrequencies && realRAM_Hz ? realRAM_Hz : RAM_Hz;
        snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c), 
            "%s%s%u.%u", MICRO_RAM_all_c, ramDiff, 
            ramFreq / 1000000, (ramFreq / 100000) % 10);
            
        if (settings.realVolts) {
            uint32_t vdd2 = realRAM_mV / 10000;
            uint32_t vddq = realRAM_mV % 10000;
            if (isMariko) {
                snprintf(RAM_volt_c, sizeof(RAM_volt_c), "%u.%u%u.%u mV", 
                    vdd2/10, vdd2%10, vddq/10, vddq%10);
            } else {
                snprintf(RAM_volt_c, sizeof(RAM_volt_c), "%u.%u mV", vdd2/10, vdd2%10);
            }
        }
        
        // Battery info
        char remainingBatteryLife[8];
        mutexLock(&mutex_BatteryChecker);
        if (batTimeEstimate >= 0) {
            snprintf(remainingBatteryLife, sizeof(remainingBatteryLife), "%d:%02d", 
                batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }

        snprintf(Battery_c, sizeof(Battery_c), "%0.2fW%.1f%% [%s]", 
            PowerConsumption, (float)_batteryChargeInfoFields.RawBatteryCharge / 1000, remainingBatteryLife);
        mutexUnlock(&mutex_BatteryChecker);

        // Thermal info
        snprintf(soc_temperature_c, sizeof(soc_temperature_c), 
            "%2.1f°C (%2.0f%%)", SOC_temperatureF, Rotation_Duty);
        snprintf(skin_temperature_c, sizeof(skin_temperature_c), 
            "%2.1f%2.1f%hu.%hhu°C (%2.0f%%)", 
            SOC_temperatureF, PCB_temperatureF, 
            skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10, 
            Rotation_Duty);
        
        if (settings.realVolts) {
            snprintf(SOC_volt_c, sizeof(SOC_volt_c), "%u.%u mV", 
                realSOC_mV/1000, (realSOC_mV/100)%10);
        }

        // FPS
        snprintf(FPS_var_compressed_c, sizeof(FPS_var_compressed_c), "%2.1f", FPSavg);

        mutexUnlock(&mutex_Misc);
    }
    
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (isKeyComboPressed(keysHeld, keysDown)) {
            TeslaFPS = 60;
            if (skipMain)
                tsl::goBack();
            else {
                tsl::setNextOverlay(filepath.c_str());
                tsl::Overlay::get()->close();
            }
            return true;
        }
        return false;
    }
};
