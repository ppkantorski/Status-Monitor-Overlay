class MainMenu;

class com_FPSGraph : public tsl::Gui {
private:
    uint8_t refreshRate = 0;
    char FPSavg_c[8];
    FpsGraphSettings settings;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    uint32_t cnt = 0;
    char CPU_Load_c[12] = "";
    char GPU_Load_c[12] = "";
    char RAM_Load_c[12] = "";
    char TEMP_c[32] = "";
    bool skipOnce = true;
    bool runOnce = true;
    bool positionOnce = true;

    bool originalUseRightAlignment = ult::useRightAlignment;
public:
    bool isStarted = false;
    com_FPSGraph() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);

        if (R_SUCCEEDED(SaltySD_Connect())) {
            if (R_FAILED(SaltySD_GetDisplayRefreshRate(&refreshRate)))
                refreshRate = 0;
            svcSleepThread(100'000);
            SaltySD_Term();
        }
        //alphabackground = 0x0;
        tsl::hlp::requestForeground(false);
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        mutexInit(&mutex_Misc);
        StartInfoThread();
        StartFPSCounterThread();
    }

    ~com_FPSGraph() {
        EndInfoThread();
        EndFPSCounterThread();
        //if (settings.setPos)
        //    tsl::gfx::Renderer::get().setLayerPos(0, 0);
        FullMode = true;
        fixForeground = true;
        ult::useRightAlignment = originalUseRightAlignment;
        //tsl::hlp::requestForeground(true);
        //alphabackground = 0xD;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
        
    }

    struct stats {
        s16 value;
        bool zero_rounded;
    };

    std::vector<stats> readings;

    s16 base_y = 0;
    s16 base_x = 0;
    s16 rectangle_width = 180;
    s16 rectangle_height = 60;
    s16 rectangle_x = 15;
    s16 rectangle_y = 5;
    s16 rectangle_range_max = 60;
    s16 rectangle_range_min = 0;
    char legend_max[4] = "60";
    char legend_min[2] = "0";
    s32 range = std::abs(rectangle_range_max - rectangle_range_min) + 1;
    s16 x_end = rectangle_x + rectangle_width;
    s16 y_old = rectangle_y+rectangle_height;
    s16 y_30FPS = rectangle_y+(rectangle_height / 2);
    s16 y_60FPS = rectangle_y;
    bool isAbove = false;

    virtual tsl::elm::Element* createUI() override {

        if (positionOnce) {
            if (settings.setPos != 8) {
                tsl::gfx::Renderer::get().setLayerPos(0, 0);
                ult::useRightAlignment = false;
            }

            switch(settings.setPos) {
                case 1: // Center Top
                case 4: // Center Center
                case 7: // Center Bottom
                    tsl::gfx::Renderer::get().setLayerPos(624, 0);
                    break;
                case 2: // Right Top
                case 5: // Right Center
                case 8: // Right Bottom
                    const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
                    tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
                    ult::useRightAlignment = true;
                    break;
            }
            positionOnce = false;
        }

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {

            // Calculate total width based on whether we're showing info
            const s16 refresh_rate_offset = (refreshRate < 100) ? 21 : 28;
            const s16 info_width = settings.showInfo ? (6 + rectangle_width/2 - 4) : 0;
            const s16 total_width = rectangle_width + refresh_rate_offset + info_width;

            if (refreshRate && refreshRate < 240) {
                rectangle_height = refreshRate;
                rectangle_range_max = refreshRate;
                if (refreshRate < 100) {
                    rectangle_x = 15;
                    legend_max[0] = 0x30 + (refreshRate / 10);
                    legend_max[1] = 0x30 + (refreshRate % 10);
                    legend_max[2] = 0;
                }
                else {
                    rectangle_x = 22;
                    legend_max[0] = 0x30 + (refreshRate / 100);
                    legend_max[1] = 0x30 + ((refreshRate / 10) % 10);
                    legend_max[2] = 0x30 + (refreshRate % 10);
                }
                y_30FPS = rectangle_y+(rectangle_height / 2);
                range = std::abs(rectangle_range_max - rectangle_range_min) + 1;
            };
            
            switch(settings.setPos) {
                case 0:  // Left Top
                    base_x = 0;
                    break;
                case 1:  // Center Top
                    base_x = 224 - (total_width / 2);
                    break;
                case 2:  // Right Top
                    base_x = 448 - total_width;
                    break;
                case 3:  // Left Center
                    base_x = 0;
                    base_y = 360 - ((rectangle_height + 12) / 2);
                    break;
                case 4:  // Center Center
                    base_x = 224 - (total_width / 2);
                    base_y = 360 - ((rectangle_height + 12) / 2);
                    break;
                case 5:  // Right Center
                    base_x = 448 - total_width;
                    base_y = 360 - ((rectangle_height + 12) / 2);
                    break;
                case 6:  // Left Bottom
                    base_x = 0;
                    base_y = 720 - (rectangle_height + 12);
                    break;
                case 7:  // Center Bottom
                    base_x = 224 - (total_width / 2);
                    base_y = 720 - (rectangle_height + 12);
                    break;
                case 8:  // Right Bottom
                    base_x = 448 - total_width;
                    base_y = 720 - (rectangle_height + 12);
                    break;
            }

            // Horizontal alignment (base_x)
            //if (ult::useRightAlignment) {
            //    base_x = 448 - (rectangle_width + 21);  // Align to the right
            //} else {
            // Default horizontal alignment based on settings.setPos
            //switch (settings.setPos) {
            //    case 1:
            //    case 4:
            //    case 7:
            //        base_x = 224 - ((rectangle_width + 21) / 2);  // Centered horizontally
            //        break;
            //    case 2:
            //    case 5:
            //    case 8:
            //        base_x = 448 - (rectangle_width + 21);  // Align to the right
            //        break;
            //}
            //}
            
            // Draw the main rectangle (extended to include info area if needed)
            renderer->drawRect(base_x, base_y, total_width, rectangle_height + 12, aWithOpacity(settings.backgroundColor));

            const s16 size = (refreshRate > 60 || !refreshRate) ? 63 : (s32)(63.0/(60.0/refreshRate));
            //std::pair<u32, u32> dimensions = renderer->drawString(FPSavg_c, false, 0, 0, size, renderer->a(0x0000));
            const auto width = renderer->getTextDimensions(FPSavg_c, false, size).first;

            const s16 pos_y = size + base_y + rectangle_y + ((rectangle_height - size) / 2);
            const s16 pos_x = base_x + rectangle_x + ((rectangle_width - width) / 2);

            if (FPSavg != 254.0)
                renderer->drawString(FPSavg_c, false, pos_x, pos_y-5, size, settings.fpsColor);
            renderer->drawEmptyRect(base_x+(rectangle_x - 1), base_y+(rectangle_y - 1), rectangle_width + 2, rectangle_height + 4, aWithOpacity(settings.borderColor));
            renderer->drawDashedLine(base_x+rectangle_x, base_y+y_30FPS, base_x+rectangle_x+rectangle_width, base_y+y_30FPS, 6, a(settings.dashedLineColor));
            renderer->drawString(&legend_max[0], false, base_x+(rectangle_x-((refreshRate < 100) ? 15 : 22)), base_y+(rectangle_y+7), 10, (settings.maxFPSTextColor));
            renderer->drawString(&legend_min[0], false, base_x+(rectangle_x-10), base_y+(rectangle_y+rectangle_height+3), 10, settings.minFPSTextColor);

            size_t last_element = readings.size() - 1;

            s16 offset = 0;
            if (refreshRate >= 100) offset = 7;

            static s32 y_on_range;
            static tsl::Color color = {0};
            for (s16 x = x_end; x > static_cast<s16>(x_end-readings.size()); x--) {
                y_on_range = readings[last_element].value + std::abs(rectangle_range_min) + 1;
                if (y_on_range < 0) {
                    y_on_range = 0;
                }
                else if (y_on_range > range) {
                    isAbove = true;
                    y_on_range = range; 
                }
                
                const s16 y = rectangle_y + static_cast<s16>(std::lround((float)rectangle_height * ((float)(range - y_on_range) / (float)range))); // 320 + (80 * ((61 - 61)/61)) = 320
                color = renderer->a(settings.mainLineColor);
                if (y == y_old && !isAbove && readings[last_element].zero_rounded) {
                    if ((y == y_30FPS || y == y_60FPS))
                        color = renderer->a(settings.perfectLineColor);
                    else
                        color = renderer->a(settings.dashedLineColor);
                }

                if (x == x_end) {
                    y_old = y;
                }
                /*
                else if (y - y_old > 0) {
                    if (y_old + 1 <= rectangle_y+rectangle_height) 
                        y_old += 1;
                }
                else if (y - y_old < 0) {
                    if (y_old - 1 >= rectangle_y) 
                        y_old -= 1;
                }
                */

                renderer->drawLine(base_x+x+offset, base_y+y, base_x+x+offset, base_y+y_old, color);
                isAbove = false;
                y_old = y;
                last_element--;
            }

            if (settings.showInfo) {
                const s16 info_x = base_x+rectangle_width+rectangle_x + 6;
                const s16 info_y = base_y + 3;
                //renderer->drawRect(info_x, info_y, rectangle_width /2 - 4, rectangle_height + 12, a(settings.backgroundColor));
                renderer->drawString("CPU\nGPU\nRAM\nSOC\nPCB\nSKN", false, info_x, info_y+11, 11, (settings.borderColor));

                renderer->drawString(CPU_Load_c, false, info_x + 40, info_y+11, 11, settings.minFPSTextColor);
                renderer->drawString(GPU_Load_c, false, info_x + 40, info_y+22, 11, settings.minFPSTextColor);
                renderer->drawString(RAM_Load_c, false, info_x + 40, info_y+33, 11, settings.minFPSTextColor);
                renderer->drawString(TEMP_c, false, info_x + 40, info_y+44, 11, settings.minFPSTextColor);
            }
        });

        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        cnt++;
        if (cnt >= TeslaFPS)
            cnt = 0;


        ///FPS
        stats temp = {0, false};
        static uint64_t lastFrame = 0;
        
        snprintf(FPSavg_c, sizeof FPSavg_c, "%2.1f",  FPSavg);
        const uint8_t SaltySharedDisplayRefreshRate = *(uint8_t*)((uintptr_t)shmemGetAddr(&_sharedmemory) + 1);
        if (SaltySharedDisplayRefreshRate) 
            refreshRate = SaltySharedDisplayRefreshRate;
        else refreshRate = 60;
        if (FPSavg < 254) {
            snprintf(FPSavg_c, sizeof(FPSavg_c), "%.1f", useOldFPSavg ? FPSavg_old : FPSavg);

            if (lastFrame == lastFrameNumber) return;
            else lastFrame = lastFrameNumber;
            if ((s16)(readings.size()) >= rectangle_width) {
                readings.erase(readings.begin());
            }
            const float whole = std::round(useOldFPSavg ? FPSavg_old : FPSavg);
            temp.value = static_cast<s16>(std::lround(useOldFPSavg ? FPSavg_old : FPSavg));
            if ((useOldFPSavg ? FPSavg_old : FPSavg) < whole+0.04 && (useOldFPSavg ? FPSavg_old : FPSavg) > whole-0.05) {
                temp.zero_rounded = true;
            }
            readings.push_back(temp);
        }
        else {
            if (readings.size()) {
                readings.clear();
                readings.shrink_to_fit();
                lastFrame = 0;
            }
            FPSavg_c[0] = 0;
        }
        
        //read_value:

        if (cnt)
            return;

        mutexLock(&mutex_Misc);
        
        snprintf(TEMP_c, sizeof TEMP_c, 
            "%2.1f\u00B0C\n%2.1f\u00B0C\n%2d.%d\u00B0C", 
            SOC_temperatureF, PCB_temperatureF, skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);
        
        // Atomically snapshot each idle tick once
        const uint64_t idle0 = idletick0.load(std::memory_order_acquire);
        const uint64_t idle1 = idletick1.load(std::memory_order_acquire);
        const uint64_t idle2 = idletick2.load(std::memory_order_acquire);
        const uint64_t idle3 = idletick3.load(std::memory_order_acquire);
        
        // Clamp values to systemtickfrequency_impl (avoid div-by-zero / runaway)
        const uint64_t safe0 = std::min(idle0, systemtickfrequency_impl);
        const uint64_t safe1 = std::min(idle1, systemtickfrequency_impl);
        const uint64_t safe2 = std::min(idle2, systemtickfrequency_impl);
        const uint64_t safe3 = std::min(idle3, systemtickfrequency_impl);
        
        // Compute per-core CPU usage
        const double cpu_usage0 = (1.0 - (static_cast<double>(safe0) / systemtickfrequency_impl)) * 100.0;
        const double cpu_usage1 = (1.0 - (static_cast<double>(safe1) / systemtickfrequency_impl)) * 100.0;
        const double cpu_usage2 = (1.0 - (static_cast<double>(safe2) / systemtickfrequency_impl)) * 100.0;
        const double cpu_usage3 = (1.0 - (static_cast<double>(safe3) / systemtickfrequency_impl)) * 100.0;
        
        // Compute max core load (the highest usage)
        const double cpu_usageM = std::max({cpu_usage0, cpu_usage1, cpu_usage2, cpu_usage3});
        
        // Format output strings
        snprintf(CPU_Load_c, sizeof(CPU_Load_c), "%.1f%%", cpu_usageM);
        snprintf(GPU_Load_c, sizeof(GPU_Load_c), "%d.%d%%", GPU_Load_u / 10, GPU_Load_u % 10);
        snprintf(RAM_Load_c, sizeof(RAM_Load_c), "%hu.%hhu%%",
                 ramLoad[SysClkRamLoad_All] / 10,
                 ramLoad[SysClkRamLoad_All] % 10);
        
        mutexUnlock(&mutex_Misc);
        
        //static bool skipOnce = true;
    
        if (!skipOnce) {
            //static bool runOnce = true;
            if (runOnce) {
                isRendering = true;
                leventClear(&renderingStopEvent);
                runOnce = false;  // Add this to prevent repeated calls
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
            positionOnce = true;
            runOnce = true;
            skipOnce = true;
            TeslaFPS = 60;
            lastSelectedItem = "FPS Graph";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            return true;
        }
        return false;
    }
};