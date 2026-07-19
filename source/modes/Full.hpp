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
    char SOC_temperature_c[32] = "";
    char PCB_temperature_c[32] = "";
    char skin_temperature_c[32] = "";
    char CPU_temp_c[16] = "";
    char GPU_temp_c[16] = "";
    char MEM_temp_c[16] = "";
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
    u64  lastDataUpdateTick = 0;  // tick of last sensor data format; used to gate updates when frame limiter is off

    // Swipe-to-flip position detection (left/right for Full, mirrors Micro's
    // top/bottom version). fullSwipeExitEvent is a global (zero-initialized
    // like threadexit) - using a class member LEvent without leventCreate
    // causes uninitialized handle corruption on the 2nd/3rd open when heap
    // memory is reused with stale content.
    // swipeFlipPending is the one-way signal from poll thread -> handleInput.
    // plusFocusActive is set by the poll thread while Plus is held long enough
    // to activate focus/reposition mode; cleared by the poll thread on release.
    std::atomic<bool> swipeFlipPending{false};
    std::atomic<bool> plusFocusActive{false};   // true while Plus-hold focus mode is live
    bool plusFocusWasActive = false;             // edge-detect: was active last handleInput frame
    bool swipeClearOnRelease = false;
    bool focusClearOnRelease = false;            // deferred frame-limiter re-enable after focus ends
    Thread swipePollThread;

    bool originalUseRightAlignment = ult::useRightAlignment;
    tsl::Color originalBackgroundColor = tsl::defaultBackgroundColor;

    // Poll thread: wakes every ~32 ms via leventWait (same pattern as Micro's
    // swipePollFunc / gpuLoadThread / BatteryChecker in Utils.hpp). leventWait
    // returns true when fullSwipeExitEvent is signalled -> thread exits immediately.
    // On swipe trigger: stops the frame limiter (isRendering=false + leventSignal) and
    // sets swipeFlipPending. handleInput re-enables the limiter via swipeClearOnRelease.
    // Plus-hold focus mode: sets plusFocusActive after a configurable hold
    // ("Button Move Delay", default 1000 ms); cleared on release.
    // While active, handleInput switches to focusBackgroundColor and handles joystick flips.
    // The touch-swipe edge/distance bounds mirror tesla.hpp's own swipe-to-open
    // detection (16 px edge guard, 84 px travel, 150 ms window) since Full's
    // flip is a left/right gesture just like swipe-to-open.
    static void swipePollFunc(void* arg) {
        auto* self = static_cast<FullOverlay*>(arg);

        static constexpr u64 POLL_NS          = 32'000'000ULL;   // 32 ms sleep / exit check
        static constexpr u64 SWIPE_WINDOW_NS  = 150'000'000ULL;  // 150 ms gesture window
        static constexpr int SWIPE_DIST_PX    = 84;              // framebuffer pixels
        static constexpr int SWIPE_EDGE_PX    = 16;              // touch must start within 16 px of left/right edge
        static constexpr int SCREEN_WIDTH_PX  = 1280;            // framebuffer width
        // Plus-hold threshold is user-configurable ("Button Move Delay").
        // settings is fully populated by GetConfigSettings() in the constructor,
        // which runs before this thread is created, so reading it here is safe.
        // Note: there is no touch_move_delay for this mode — touch repositioning
        // is a swipe gesture, not a press-and-hold.
        const u64 PLUS_HOLD_NS = (u64)self->settings.buttonMoveDelayMs * 1'000'000ULL;

        // HID setup - same as Micro's touch poll thread: allow P1 + Handheld.
        const HidNpadIdType id_list[2] = { HidNpadIdType_No1, HidNpadIdType_Handheld };
        hidSetSupportedNpadIdType(id_list, 2);
        padConfigureInput(2, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
        PadState pad_p1;
        PadState pad_handheld;
        padInitialize(&pad_p1,       HidNpadIdType_No1);
        padInitialize(&pad_handheld, HidNpadIdType_Handheld);

        HidTouchScreenState state = {0};
        bool touching             = false;
        int  initialX             = 0;
        u64  touchStartNs         = 0;
        u64  plusHoldStart        = 0;

        do {
            // Sleep gate: swipe and button input are impossible while the
            // system is sleeping and the display is off. Block here until
            // wake; the while condition checks fullSwipeExitEvent on resume
            // so shutdown during sleep still exits cleanly within one POLL_NS.
            if (tsl::hlp::waitWhileSleeping(POLL_NS)) continue;

            const u64 nowNs = armTicksToNs(armGetSystemTick());

            // -- Touch-swipe-to-flip ------------------------------------------
            if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                const int tx = static_cast<int>(state.touches[0].x);

                if (!touching) {
                    // Finger just placed - record origin
                    touching     = true;
                    initialX     = tx;
                    touchStartNs = nowNs;
                } else if (!self->swipeFlipPending.load(std::memory_order_acquire)) {
                    // Gesture in progress, no flip queued yet - check thresholds
                    const u64 elapsed = nowNs - touchStartNs;
                    if (elapsed <= SWIPE_WINDOW_NS) {
                        const int  deltaX  = tx - initialX;
                        const bool atLeft  = !self->settings.setPosRight;
                        const bool atRight =  self->settings.setPosRight;
                        // Touch must have started within SWIPE_EDGE_PX of the
                        // relevant screen edge - same guard tesla uses for swipe-to-open.
                        const bool startedAtEdge = (atLeft  && initialX <= SWIPE_EDGE_PX) ||
                                                   (atRight && initialX >= SCREEN_WIDTH_PX - SWIPE_EDGE_PX);
                        if (startedAtEdge &&
                            ((atLeft  && deltaX >=  SWIPE_DIST_PX) ||
                             (atRight && deltaX <= -SWIPE_DIST_PX))) {
                            if (isRendering) {
                                isRendering = false;
                                leventSignal(&renderingStopEvent);
                            }
                            self->swipeFlipPending.store(true, std::memory_order_release);
                            triggerMoveFeedback();
                        }
                    }
                }
            } else {
                // No touch - reset so the next finger-down starts a fresh gesture
                touching = false;
            }

            // -- Plus-hold focus mode ------------------------------------------
            padUpdate(&pad_p1);
            padUpdate(&pad_handheld);
            const u64 keysHeld = padGetButtons(&pad_p1) | padGetButtons(&pad_handheld);
            const bool plusOnly = (keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK);

            if (plusOnly) {
                if (plusHoldStart == 0) plusHoldStart = nowNs;
                if (!self->plusFocusActive.load(std::memory_order_acquire) &&
                    (nowNs - plusHoldStart) >= PLUS_HOLD_NS) {
                    // Hold threshold reached - activate focus mode and stop the
                    // frame limiter so handleInput can render at full speed.
                    self->plusFocusActive.store(true, std::memory_order_release);
                    if (isRendering) {
                        isRendering = false;
                        leventSignal(&renderingStopEvent);
                    }
                    triggerOnFeedback();
                }
            } else {
                if (self->plusFocusActive.load(std::memory_order_acquire)) {
                    // Plus released while focus mode was active - deactivate.
                    self->plusFocusActive.store(false, std::memory_order_release);
                    triggerOffFeedback(true);
                }
                plusHoldStart = 0;
            }

        } while (!leventWait(&fullSwipeExitEvent, POLL_NS));
    }

    // Applies a left/right reposition: updates settings, persists to the ini,
    // flips alignment, and moves the VI layer. No-op if already at the
    // requested side. Shared by both the joystick-flip and swipe-flip paths
    // in handleInput (always called from the main thread).
    void applyPositionFlip(bool wantRight) {
        if (wantRight == settings.setPosRight) return;
        settings.setPosRight = wantRight;
        ult::setIniFileValue(configIniPath, "full", "layer_width_align", wantRight ? "right" : "left");
        ult::useRightAlignment = wantRight;
        const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
        if (wantRight) {
            tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
        } else {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
        }
        // Note: callers are responsible for triggering move feedback.
        // The swipe path already fires it in the poll thread; the joystick
        // path calls triggerMoveFeedback() explicitly after applyPositionFlip.
    }

public:
    FullOverlay() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);
        tsl::defaultBackgroundColor = tsl::Color(settings.backgroundColor); // apply Full's bg color to the tesla draw path
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

        // Start swipe-to-flip poll thread.
        // leventClear ensures the exit event starts non-signalled before threadStart.
        leventClear(&fullSwipeExitEvent);
        threadCreate(&swipePollThread, swipePollFunc, this, nullptr, 0x1000, 0x2c, -2);
        threadStart(&swipePollThread);
    }
    ~FullOverlay() {
        leventSignal(&fullSwipeExitEvent);
        threadWaitForExit(&swipePollThread);
        threadClose(&swipePollThread);
        CloseThreads();
        fixForeground = true;
        ult::useRightAlignment = originalUseRightAlignment;
        tsl::defaultBackgroundColor = originalBackgroundColor; // restore for non-full modes
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

            static constexpr size_t valueOffset = 150+10;
            static constexpr size_t deltaOffset = 246+10;
            static constexpr size_t ramPercentageOffset = 350+10;

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
                
                uint32_t height_offset = 306;
                if (realGPU_Hz && settings.showRealFreqs) {
                    height_offset = 313;
                }

                renderer->drawString("GPU Usage", false, COMMON_MARGIN, 271, 20, (settings.catColor1));
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
                
                uint32_t height_offset = 397;
                if (realRAM_Hz && settings.showRealFreqs) {
                    height_offset += 7;
                }

                renderer->drawString("RAM Usage", false, COMMON_MARGIN, 362, 20, (settings.catColor1));
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
                    {
                        static std::vector<std::string> ramLoadColoredChars = {"CPU", "GPU"};
                        //static auto loadLabelWidth = renderer->getTextDimensions("Load: ", false, 15).first;
                        renderer->drawString("Load", false, COMMON_MARGIN, height_offset+15, 15, (settings.catColor2));
                        renderer->drawStringWithColoredSections(RAM_load_c, false, ramLoadColoredChars, COMMON_MARGIN + valueOffset, height_offset+15, 15, (settings.textColor), settings.catColor2);
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
                renderer->drawString("Board", false, 20, 536+2, 20, (settings.catColor1));
                if (R_SUCCEEDED(i2cCheck)) {
                    renderer->drawString("Battery Power Flow", false, COMMON_MARGIN, 561+2, 15, (settings.catColor2));
                    renderer->drawStringWithColoredSections(BatteryDraw_c, false, specialChars, COMMON_MARGIN + valueOffset, 561+2, 15, (settings.textColor), settings.separatorColor);
                }
                if (R_SUCCEEDED(pwmCheck)) {
                    renderer->drawString("Fan Rotation Level", false, COMMON_MARGIN, 576+2, 15, (settings.catColor2));
                    renderer->drawString(Rotation_SpeedLevel_c, false, COMMON_MARGIN + valueOffset, 576+2, 15, (settings.textColor));
                }
                if (R_SUCCEEDED(i2cCheck) || R_SUCCEEDED(tcCheck)) {
                    // Pre-measure label widths for column alignment
                    static auto lbl_cpu  = renderer->getTextDimensions("CPU  ", false, 15).first;
                    static auto lbl_gpu  = renderer->getTextDimensions("GPU  ", false, 15).first;
                    static auto lbl_mem  = renderer->getTextDimensions("MEM  ", false, 15).first;
                    static auto lbl_soc  = renderer->getTextDimensions("SOC  ", false, 15).first;
                    static auto lbl_pcb  = renderer->getTextDimensions("PCB  ", false, 15).first;
                    static auto lbl_skin = renderer->getTextDimensions("Skin  ", false, 15).first;

                    // Per-column label width: align CPU/SOC, GPU/PCB, MEM/Skin
                    const auto col1_lbl = std::max(lbl_cpu, lbl_soc);
                    const auto col2_lbl = std::max(lbl_gpu, lbl_pcb);
                    const auto col3_lbl = std::max(lbl_mem, lbl_skin);

                    // Value width — use SOC as reference (all values similar width)
                    const auto val_w   = renderer->getTextDimensions(SOC_temperature_c, false, 15).first;
                    const uint32_t col_gap = 14;  // gap between value end and next column label

                    // Total grid width, then compute centered start_x
                    const uint32_t total_w = col1_lbl + val_w + col_gap
                                           + col2_lbl + val_w + col_gap
                                           + col3_lbl + val_w;
                    const uint32_t start_x = (448 - total_w) / 2;

                    const uint32_t col1_x = start_x;
                    const uint32_t col2_x = col1_x + col1_lbl + val_w + col_gap;
                    const uint32_t col3_x = col2_x + col2_lbl + val_w + col_gap;

                    // Temperatures label moves up with the Board section;
                    // grid rows are hardcoded so they stay fixed in place
                    const uint32_t temp_label_y = 591 + 2;
                    const uint32_t row1_y = 620-2-1;  // fixed: CPU / GPU / MEM
                    const uint32_t row2_y = 635-2+1;  // fixed: SOC / PCB / Skin

                    // Gradient colors for board temps
                    const tsl::Color socColor  = settings.useDynamicColors ? tsl::GradientColor(SOC_temperatureF) : settings.textColor;
                    const tsl::Color pcbColor  = settings.useDynamicColors ? tsl::GradientColor(PCB_temperatureF) : settings.textColor;
                    const tsl::Color skinColor = settings.useDynamicColors ? tsl::GradientColor(static_cast<float>(skin_temperaturemiliC) / 1000.0f) : settings.textColor;

                    // Gradient colors for component die temps
                    const tsl::Color cpuColor = settings.useDynamicColors ? tsl::GradientColor(componentCPU_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor;
                    const tsl::Color gpuColor = settings.useDynamicColors ? tsl::GradientColor(componentGPU_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor;
                    const tsl::Color memColor = settings.useDynamicColors ? tsl::GradientColor(componentRAM_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor;

                    renderer->drawString("Temperatures", false, COMMON_MARGIN, temp_label_y, 15, settings.catColor2);

                    // --- Row 1: CPU | GPU | MEM ---
                    renderer->drawString("CPU  ", false, col1_x, row1_y, 15, settings.catColor2);
                    renderer->drawString(CPU_temp_c,   false, col1_x + col1_lbl, row1_y, 15, cpuColor);

                    renderer->drawString("GPU  ", false, col2_x, row1_y, 15, settings.catColor2);
                    renderer->drawString(GPU_temp_c,   false, col2_x + col2_lbl, row1_y, 15, gpuColor);

                    renderer->drawString("MEM  ", false, col3_x, row1_y, 15, settings.catColor2);
                    renderer->drawString(MEM_temp_c,   false, col3_x + col3_lbl, row1_y, 15, memColor);

                    // --- Row 2: SOC | PCB | Skin ---
                    renderer->drawString("SOC  ", false, col1_x, row2_y, 15, settings.catColor2);
                    renderer->drawString(SOC_temperature_c, false, col1_x + col1_lbl, row2_y, 15, socColor);

                    renderer->drawString("PCB  ", false, col2_x, row2_y, 15, settings.catColor2);
                    renderer->drawString(PCB_temperature_c, false, col2_x + col2_lbl, row2_y, 15, pcbColor);

                    renderer->drawString("Skin  ", false, col3_x, row2_y, 15, settings.catColor2);
                    renderer->drawString(skin_temperature_c, false, col3_x + col3_lbl, row2_y, 15, skinColor);
                }
            }
            
            ///FPS
            if (GameRunning) {
                const uint32_t width_offset = valueOffset;
                if (settings.showFPS || settings.showRES || settings.showRDSD) {
                    renderer->drawString("Game", false, COMMON_MARGIN + width_offset, 185+12-1, 20, (settings.catColor1));
                }
                uint32_t height = 210+12+1;
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
        // While Plus-hold focus mode is active, swap to the focus background
        // color so the user gets immediate visual feedback even though data
        // formatting below is still throttled to sampleRate. This runs every
        // frame (not gated) since the frame limiter may be unlocked here.
        tsl::defaultBackgroundColor = tsl::Color(
            plusFocusActive.load(std::memory_order_acquire) ? settings.focusBackgroundColor
                                                              : settings.backgroundColor);

        // Throttle data formatting to the user-specified sample rate even when
        // the frame limiter is off (e.g. during Plus-hold focus mode). The render
        // loop may call update() at vsync speed (~60 fps) while focus mode is
        // active, but we only rebuild the display strings at 1/sampleRate intervals.
        const u64 nowTick = armGetSystemTick();
        const u64 pollIntervalTicks = systemtickfrequency / settings.sampleRate;
        const bool shouldUpdateData = (nowTick - lastDataUpdateTick) >= pollIntervalTicks;
        if (shouldUpdateData) lastDataUpdateTick = nowTick;

        if (shouldUpdateData) {
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
        
        {
            const int RAM_GPU_Load = (int)ramLoad[SysClkRamLoad_All] - (int)ramLoad[SysClkRamLoad_Cpu];
            const unsigned gpuLoad = (unsigned)(RAM_GPU_Load > 0 ? RAM_GPU_Load : 0);
            snprintf(RAM_load_c, sizeof RAM_load_c, 
                "%u.%u%%    CPU  %u.%u%%   GPU  %u.%u%%",
                ramLoad[SysClkRamLoad_All] / 10, ramLoad[SysClkRamLoad_All] % 10,
                ramLoad[SysClkRamLoad_Cpu] / 10, ramLoad[SysClkRamLoad_Cpu] % 10,
                gpuLoad / 10, gpuLoad % 10);
        }
        ///Thermal
        snprintf(SOC_temperature_c, sizeof SOC_temperature_c, "%.1f\u00B0C", SOC_temperatureF);
        snprintf(PCB_temperature_c, sizeof PCB_temperature_c, "%.1f\u00B0C", PCB_temperatureF);
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "%d.%d\u00B0C", skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);
        // HOC component die temps (always populate; displayed when available)
        snprintf(CPU_temp_c, sizeof CPU_temp_c, "%.1f\u00B0C", componentCPU_mC / 1000.0f);
        snprintf(GPU_temp_c, sizeof GPU_temp_c, "%.1f\u00B0C", componentGPU_mC / 1000.0f);
        snprintf(MEM_temp_c, sizeof MEM_temp_c, "%.1f\u00B0C", componentRAM_mC / 1000.0f);

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

        /* ── Battery / power draw ───────────────────────────────────── */
        char remainingBatteryLife[8];
        
        /* Normalise "-0.00" → "0.00" W */
        const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f
                                                         : PowerConsumption;
        
        mutexLock(&mutex_BatteryChecker);
        
        /* keep "--:--" whenever estimate is negative */
        if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
            snprintf(remainingBatteryLife, sizeof(remainingBatteryLife),
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }
        
        const float batteryPercent = (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f;
        
        snprintf(BatteryDraw_c, sizeof(BatteryDraw_c),
                 "%.2f W%.0f%% [%s]",
                 drawW,
                 batteryPercent,
                 remainingBatteryLife);
        
        mutexUnlock(&mutex_BatteryChecker);
        } // end shouldUpdateData
        
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
        // -- Focus-end: re-enable frame limiter one frame after color reverts ----
        // Checked at the TOP of handleInput so there is always at least one full
        // loop() cycle (update -> draw -> endFrame, no block since isRendering is
        // still false) between when focusClearOnRelease is set (bottom of this
        // function, the frame focus ends) and when isRendering is restored here.
        // This mirrors Mini's clearOnRelease pattern exactly.
        if (focusClearOnRelease && !isRendering) {
            focusClearOnRelease = false;
            isRendering = true;
            leventClear(&renderingStopEvent);
        }

        // -- Plus-hold focus/reposition mode -------------------------------------
        // The poll thread sets plusFocusActive after a 1-second Plus hold and
        // stops the frame limiter. Here we:
        //   1. Re-enable the frame limiter the frame after focus mode ends.
        //   2. While active, map either-joystick full-left/right to position flips
        //      (same logic as swipeFlipPending: toggle setPosRight and move layer).
        {
            const bool focusNow = plusFocusActive.load(std::memory_order_acquire);

            if (focusNow) {
                // Frame limiter is already stopped by the poll thread.
                // Check both joysticks for left/right snap.
                static constexpr int JOYSTICK_SNAP_THRESHOLD = 28000; // ~85 % of 32767
                static bool joystickFlipArmed = true; // re-arm once stick returns to center

                // Accept EITHER stick: use whichever X axis is deflected further, so
                // PLUS + left stick and PLUS + right stick both flip the position.
                const int jxL = joyStickPosLeft.x;
                const int jxR = joyStickPosRight.x;
                const int jx  = (abs(jxR) > abs(jxL)) ? jxR : jxL;
                const bool stickAtRight = (jx >=  JOYSTICK_SNAP_THRESHOLD);
                const bool stickAtLeft  = (jx <= -JOYSTICK_SNAP_THRESHOLD);
                const bool stickNeutral = (abs(jx) < JOYSTICK_SNAP_THRESHOLD / 2);

                if (stickNeutral) {
                    joystickFlipArmed = true; // stick returned to center; allow next flip
                }

                if (joystickFlipArmed && (stickAtRight || stickAtLeft)) {
                    applyPositionFlip(stickAtRight);
                    triggerMoveFeedback();
                    joystickFlipArmed = false; // require stick to return to center before next flip
                }
            }

            // Edge: focus mode just deactivated -> defer frame-limiter re-enable
            // by one frame so loop() draws the reverted background color before
            // endFrame() starts blocking at the slow TeslaFPS interval again.
            if (!focusNow && plusFocusWasActive) {
                focusClearOnRelease = true;
            }
            plusFocusWasActive = focusNow;
        }

        // -- Swipe-to-flip: re-enable rendering after position transition --------
        // Poll thread stopped the frame limiter when the swipe fired. Once the
        // flip is applied and one frame is drawn with the new position, re-enable.
        if (swipeClearOnRelease && !isRendering) {
            swipeClearOnRelease = false;
            isRendering = true;
            leventClear(&renderingStopEvent);
        }

        // -- Swipe-to-flip: apply position change ---------------------------------
        // Poll thread sets swipeFlipPending; consumed here (main thread) so all
        // settings/render state mutations are safe and single-threaded.
        if (swipeFlipPending.exchange(false, std::memory_order_acq_rel)) {
            applyPositionFlip(!settings.setPosRight);
            swipeClearOnRelease = true;
        }

        if (isKeyComboPressed(keysHeld, keysDown)) {
            isRendering = false;
            leventSignal(&renderingStopEvent);
            
            skipOnce = true;
            runOnce = true;
            TeslaFPS = 60;
            lastSelectedItem = "Full";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            if (skipMain) {
                //lastSelectedItem = "Micro";
                lastMode = "returning";
                tsl::Overlay::get()->close();
                
            }
            else {
                triggerExitFeedback();
                tsl::swapTo<MainMenu>();
            }
            return true;
        }
        return false;
    }
};