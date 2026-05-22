class MainMenu;
// Sentinel character prepended to single-digit percentages so fixed-width bracket
// strings like "[#4% #8%]" keep a stable pixel width across 1- and 2-digit values.
// The renderer draws this character transparent (cpuPadChars / cpuPadTransparent).
// Change to any single ASCII character to test alternative padding glyphs.
static constexpr char kPadSentinelMicro = 'B';

class MicroOverlay : public tsl::Gui {
private:
    char GPU_Load_c[32];
    char RAM_var_compressed_c[128];
    char RAM_load_bot_c[32] = "";
    char RAM_bw_c[16] = "";       // RAM bandwidth string e.g. "2.2GB/s" (ACTMON direct)
    char CPU_compressed_c[160];
    char CPU_freq_c[16];          // freq part for full CPU split mode e.g. "@1000.0"
    char CPU_Usage0[32];
    char CPU_Usage1[32];
    char CPU_Usage2[32];
    char CPU_Usage3[32];
    char CPU_UsageM[32];
    char soc_temperature_c[32];
    char skin_temperature_c[64];  // expanded: holds combined SBS string too
    char componentTemps_c[64];    // dual-row TMP: CPU/GPU/RAM die temps
    char splitBotTemps_c[64];     // split mode: bottom row temps (SOC/PCB/Skin for dual-row, empty for single-group)
    char cpu_temp_c[16];          // CPU die temp string e.g. "52°C"
    char gpu_temp_c[16];          // GPU die temp string e.g. "48°C"
    char ram_temp_c[16];          // RAM die temp string e.g. "45°C"
    char FPS_var_compressed_c[64];
    char Battery_c[32];
    char Battery_draw_c[16] = "";
    char Battery_pct_c[32] = "";
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
    bool tmpIsSplit = false; // true when fan on row 1, SOC volt on row 2 (showStackedFanSOC=true)
    bool ramIsSplit = false; // true when VDD2 on row 1, VDDQ on row 2 (showStackedVDDQ=true, Mariko only)
    bool cpuIsSplit = false; // true when CPU volt on row 1, CPU temp on row 2
    bool cpuFullIsSplit = false; // true when Full CPU brackets on top row, freq on bottom row
    bool batIsSplit     = false; // true when BAT draw/pct stack vertically
    bool dtcIsSplit     = false; // true when DTC splits at DIVIDER_SYMBOL into 2 stacked rows
    bool ramLoadIsSplit = false; // true when RAM CPU/GPU load stacks above total@freq
    bool ramBWIsSplit   = false; // true when RAM bandwidth stacks above the normal RAM line
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

    // Swipe-to-flip position detection.
    // microSwipeExitEvent is a global (zero-initialized like threadexit) — using a
    // class member LEvent without leventCreate causes uninitialized handle corruption
    // on the 2nd/3rd open when heap memory is reused with stale content.
    // swipeFlipPending is the one-way signal from poll thread → handleInput.
    // plusFocusActive is set by the poll thread while Plus is held long enough
    // to activate focus/reposition mode; cleared by the poll thread on release.
    std::atomic<bool> swipeFlipPending{false};
    std::atomic<bool> plusFocusActive{false};   // true while Plus-hold focus mode is live
    bool plusFocusWasActive = false;             // edge-detect: was active last handleInput frame
    bool swipeClearOnRelease = false;
    Thread swipePollThread;
    
    // Fixed spacing system - calculate actual widths at render time
    struct LayoutMetrics {
        uint32_t label_data_gap = 8;      // Fixed gap between label and data
        uint32_t volt_separator_gap = 0;   // Fixed gap before voltage separator
        uint32_t volt_data_gap = 0;        // Fixed gap after voltage separator
        uint32_t item_spacing = 16;        // Minimum spacing between complete items
        uint32_t side_margin = 8;          // Left/right edge padding — set to label_data_gap in calculateLayoutMetrics
        bool calculated = false;
    } layout;

    // -------- Width anti-flicker damper --------
    // Per-item-type rolling-max width with periodic decay. Grows instantly (so new
    // rows like FPS/RES, or values getting wider, are never clipped); shrinks only
    // when the high-water mark of an observation window stays below the held width
    // for a full window. This kills the sub-pixel jitter that makes the entire row
    // shift left/right as numeric values fluctuate by 1px, while still allowing the
    // layout to genuinely tighten when a metric moves to a smaller regime.
    //
    // Damping is applied ONLY to total_width (the value that drives item_positions).
    // label_width / data_width / volt_width are left untouched so the internal cell
    // layout math (current_x advances) keeps working unchanged.
    //
    // Keyed by item.type (0..9). regime_fp guards against layout-mode flips (e.g.
    // ramBWIsSplit toggling) which change the total_width formula: when the fp
    // changes, the damper resets so the new regime takes effect immediately.
    struct WidthDamper {
        uint32_t sticky_width = 0;   // currently held (displayed) width
        uint32_t recent_max   = 0;   // high-water mark within the current window
        uint16_t frames_held  = 0;   // frames elapsed in current observation window
        uint32_t regime_fp    = 0;   // fingerprint of split-mode flags relevant to this item
    };
    static constexpr size_t kNumItemTypes = 10;        // item.type values 0..9
    // Hold the max width for ~1.5 real seconds before allowing shrink.
    // Computed from TeslaFPS (the actual overlay refresh rate, e.g. 3 Hz default)
    // so the window is always ~1.5s regardless of the user's refresh rate setting.
    static constexpr float kDampWindowSeconds = 1.5f;
    inline uint16_t dampWindowFrames() const {
        const uint8_t fps = (TeslaFPS > 0) ? TeslaFPS : 1;
        // At least 2 frames so a single-frame spike is always held for one full cycle.
        const uint16_t frames = static_cast<uint16_t>(kDampWindowSeconds * fps + 0.5f);
        return (frames < 2) ? 2 : frames;
    }
    WidthDamper widthDampers[kNumItemTypes] = {};

    // Build a fingerprint of layout-affecting flags for a given item.type.
    // Only bits that change THIS item's total_width formula are mixed in, so a
    // flip of (say) gpuIsSplit does not pointlessly reset the CPU damper.
    inline uint32_t computeRegimeFp(uint8_t itemType) const {
        uint32_t fp = 0;
        switch (itemType) {
            case 0: // CPU
                fp = (cpuIsSplit ? 1u : 0u)
                   | (cpuFullIsSplit ? 2u : 0u);
                break;
            case 1: // GPU
                fp = (gpuIsSplit ? 1u : 0u);
                break;
            case 2: // RAM
                fp = (ramIsSplit ? 1u : 0u)
                   | (ramTempSplit ? 2u : 0u)
                   | (ramLoadIsSplit ? 4u : 0u)
                   | (ramBWIsSplit ? 8u : 0u);
                break;
            case 4: // TMP
                fp = (tmpIsGrid ? 1u : 0u)
                   | (tmpIsSplit ? 2u : 0u);
                break;
            case 6: // BAT
                fp = (batIsSplit ? 1u : 0u);
                break;
            case 8: // DTC
                fp = (dtcIsSplit ? 1u : 0u);
                break;
            default: // SOC(3), FPS(5), RES(7), READ(9) — no split modes
                fp = 0;
                break;
        }
        return fp;
    }

    // Apply rolling-max damper to a freshly-measured width for the given item type.
    // Returns the damped (display) width. Always >= raw, so text is never clipped.
    inline uint32_t dampItemWidth(uint8_t itemType, uint32_t raw) {
        if (itemType >= kNumItemTypes) return raw;
        WidthDamper& d = widthDampers[itemType];

        // Regime change (a split flag flipped) — formula for total_width changed,
        // any held value is meaningless. Snap to the new raw and start fresh.
        const uint32_t fp = computeRegimeFp(itemType);
        if (fp != d.regime_fp) {
            d.regime_fp    = fp;
            d.sticky_width = raw;
            d.recent_max   = raw;
            d.frames_held  = 0;
            return raw;
        }

        // Grow instantly — never clip text. Reset the window when we grow so the
        // new peak gets a full window of observation before any potential shrink.
        if (raw > d.sticky_width) {
            d.sticky_width = raw;
            d.recent_max   = raw;
            d.frames_held  = 0;
            return d.sticky_width;
        }

        // Track high-water mark inside the current window and advance.
        if (raw > d.recent_max) d.recent_max = raw;
        d.frames_held++;

        // Window closed: collapse the held width down to whatever the peak was.
        // If the metric truly shrank for a full window, sticky_width tightens to
        // that smaller peak. If it ever briefly touched the old width during the
        // window, recent_max held it and we don't shrink at all.
        if (d.frames_held >= dampWindowFrames()) {
            d.sticky_width = d.recent_max;
            d.recent_max   = raw;  // start the next window from the current raw
            d.frames_held  = 0;
        }

        return d.sticky_width;
    }

    // Reset all dampers (called on overlay (re)initialization so a freshly-opened
    // overlay doesn't carry stale widths from a previous session).
    inline void resetWidthDampers() {
        for (auto& d : widthDampers) d = WidthDamper{};
    }
    
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
        // Override label_data_gap if user has set a label_padding value (non-zero = explicit)
        if (settings.labelPadding != 0) {
            layout.label_data_gap = settings.labelPadding;
        }
        // Horizontal padding setting controls left/right gap from screen edge to text.
        // The bar rectangle always spans full screen width; only the text is inset.
        layout.side_margin = settings.horizontalPadding;
        
        layout.calculated = true;
    }

    // Poll thread: wakes every ~32 ms via leventWait (same pattern as gpuLoadThread /
    // BatteryChecker in Utils.hpp). leventWait returns true when microSwipeExitEvent is
    // signalled → thread exits immediately.
    // On swipe trigger: stops the frame limiter (isRendering=false + leventSignal) and
    // sets swipeFlipPending. handleInput re-enables the limiter via swipeClearOnRelease.
    // Plus-hold focus mode: sets plusFocusActive after a 1-second hold; cleared on release.
    // While active, handleInput switches to focusBackgroundColor and handles joystick flips.
    static void swipePollFunc(void* arg) {
        auto* self = static_cast<MicroOverlay*>(arg);

        static constexpr u64 POLL_NS              = 32'000'000ULL;   // 32 ms sleep / exit check
        static constexpr u64 SWIPE_WINDOW_NS      = 150'000'000ULL;  // 150 ms gesture window
        static constexpr int SWIPE_DIST_PX        = 84;              // framebuffer pixels
        static constexpr int SWIPE_EDGE_PX        = 32;              // touch must start within 16 px of top/bottom edge
        static constexpr int SCREEN_HEIGHT_PX     = 720;             // framebuffer height
        static constexpr u64 PLUS_HOLD_NS         = 1'000'000'000ULL; // 1 s hold to activate

        // HID setup — same as Mini's touch poll thread: allow P1 + Handheld.
        const HidNpadIdType id_list[2] = { HidNpadIdType_No1, HidNpadIdType_Handheld };
        hidSetSupportedNpadIdType(id_list, 2);
        padConfigureInput(2, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
        PadState pad_p1;
        PadState pad_handheld;
        padInitialize(&pad_p1,       HidNpadIdType_No1);
        padInitialize(&pad_handheld, HidNpadIdType_Handheld);

        HidTouchScreenState state = {0};
        bool touching             = false;
        int  initialY             = 0;
        u64  touchStartNs         = 0;
        u64  plusHoldStart        = 0;

        do {
            const u64 nowNs = armTicksToNs(armGetSystemTick());

            // ── Touch-swipe-to-flip ───────────────────────────────────────────
            if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                const int ty = static_cast<int>(state.touches[0].y);

                if (!touching) {
                    // Finger just placed — record origin
                    touching     = true;
                    initialY     = ty;
                    touchStartNs = nowNs;
                } else if (!self->swipeFlipPending.load(std::memory_order_acquire)) {
                    // Gesture in progress, no flip queued yet — check thresholds
                    const u64 elapsed = nowNs - touchStartNs;
                    if (elapsed <= SWIPE_WINDOW_NS) {
                        const int  deltaY   = ty - initialY;
                        const bool atTop    = !self->settings.setPosBottom;
                        const bool atBottom =  self->settings.setPosBottom;
                        // Touch must have started within SWIPE_EDGE_PX of the
                        // relevant screen edge — same guard tesla uses for swipe-to-open.
                        const bool startedAtEdge = (atTop    && initialY <= SWIPE_EDGE_PX) ||
                                                   (atBottom && initialY >= SCREEN_HEIGHT_PX - SWIPE_EDGE_PX);
                        if (startedAtEdge &&
                            ((atTop    && deltaY >=  SWIPE_DIST_PX) ||
                             (atBottom && deltaY <= -SWIPE_DIST_PX))) {
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
                // No touch — reset so the next finger-down starts a fresh gesture
                touching = false;
            }

            // ── Plus-hold focus mode ──────────────────────────────────────────
            padUpdate(&pad_p1);
            padUpdate(&pad_handheld);
            const u64 keysHeld = padGetButtons(&pad_p1) | padGetButtons(&pad_handheld);
            const bool plusOnly = (keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK);

            if (plusOnly) {
                if (plusHoldStart == 0) plusHoldStart = nowNs;
                if (!self->plusFocusActive.load(std::memory_order_acquire) &&
                    (nowNs - plusHoldStart) >= PLUS_HOLD_NS) {
                    // Hold threshold reached — activate focus mode and stop the
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
                    // Plus released while focus mode was active — deactivate.
                    self->plusFocusActive.store(false, std::memory_order_release);
                    triggerOffFeedback(true);
                }
                plusHoldStart = 0;
            }

        } while (!leventWait(&microSwipeExitEvent, POLL_NS));
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

        // Start swipe-to-flip poll thread.
        // leventClear ensures the exit event starts non-signalled before threadStart.
        leventClear(&microSwipeExitEvent);
        threadCreate(&swipePollThread, swipePollFunc, this, nullptr, 0x1000, 0x2c, -2);
        threadStart(&swipePollThread);
        
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
        
        {
            const float charge = (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f;
            snprintf(Battery_draw_c, sizeof(Battery_draw_c), "%.2f W", drawW);
            snprintf(Battery_pct_c,  sizeof(Battery_pct_c),  "%.1f%% [%s]", charge, remainingBatteryLife);
            if (settings.showStackedBAT) {
                if (!settings.invertBatteryDisplay)
                    snprintf(Battery_c, sizeof(Battery_c), "%.2f W", drawW);
                else
                    snprintf(Battery_c, sizeof(Battery_c), "%.1f%% [%s]", charge, remainingBatteryLife);
            } else {
                if (!settings.invertBatteryDisplay)
                    snprintf(Battery_c, sizeof(Battery_c), "%.2f W%.1f%% [%s]", drawW, charge, remainingBatteryLife);
                else
                    snprintf(Battery_c, sizeof(Battery_c), "%.1f%% [%s]%.2f W", charge, remainingBatteryLife, drawW);
            }
        }

        
    }
    
    ~MicroOverlay() {
        leventSignal(&microSwipeExitEvent);
        threadWaitForExit(&swipePollThread);
        threadClose(&swipePollThread);
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
                            const bool cpuSplit    = settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                            const bool cpuSBSDefer = settings.voltageAtEndCPU && settings.showCPUTemp && !settings.showStackedCPUTemp && settings.realVolts;
                            const bool cpuFreqSplit = settings.showFullCPU && settings.showStackedFullCPU;
                            renderItems.push_back({0, "CPU", CPU_compressed_c, CPU_volt_c, settings.realVolts && !cpuSplit && !cpuSBSDefer && !cpuFreqSplit});
                            seen_flags |= 1;
                        }
                        break;
                    case 0x475055: // "GPU"
                        if (!(seen_flags & 2)) {
                            const bool gpuSplit    = settings.showGPUTemp && settings.showStackedGPUTemp && settings.realVolts;
                            const bool gpuSBSDefer = settings.voltageAtEndGPU && settings.showGPUTemp && !settings.showStackedGPUTemp && settings.realVolts;
                            renderItems.push_back({1, "GPU", GPU_Load_c, GPU_volt_c, settings.realVolts && !gpuSplit && !gpuSBSDefer});
                            seen_flags |= 2;
                        }
                        break;
                    case 0x52414D: // "RAM"
                        if (!(seen_flags & 4)) {
                            // In split VDD2/VDDQ mode, voltage is drawn by the renderer
                            const bool splitRAMVolt = settings.showStackedVDDQ &&
                                                      settings.realVolts && settings.showVDD2 &&
                                                      settings.showVDDQ && isMariko;
                            // ramTempSplit: VDDQ-only Mariko with RAM temp split (volt drawn by renderer)
                            const bool ramTempSplitCond = settings.showRAMTemp && settings.showStackedRAMTemp &&
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
                            // Volt is drawn entirely by the TMP renderer in all paths (split/grid/single-row).
                            // has_voltage=false so the generic volt block never fires for TMP.
                            renderItems.push_back({4, "TMP", skin_temperature_c, SOC_volt_c, false});
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
        bool hasBat = false;
        bool hasDtc = false;
        for (const auto& item : renderItems) {
            if (item.type == 4) hasTmp = true;
            if (item.type == 0) hasCpu = true;
            if (item.type == 1) hasGpu = true;
            if (item.type == 2) hasRam = true;
            if (item.type == 6) hasBat = true;
            if (item.type == 8) hasDtc = true;
        }
        tmpIsSplit = hasTmp &&
                     settings.showStackedFanSOC &&
                     settings.realVolts && settings.showSOCVoltage;
        tmpIsGrid = hasTmp &&
                    settings.showComponentTemps &&
                    settings.showSocPcbSkinTemps &&
                    settings.showStackedTemps &&
                    !tmpIsSplit;  // split takes priority over grid

        ramIsSplit = hasRam &&
                     settings.showStackedVDDQ &&
                     settings.realVolts && settings.showVDD2 &&
                     settings.showVDDQ && isMariko;

        // CPU die temp split: volt on top row, CPU temp on bottom row
        cpuIsSplit = hasCpu && settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
        // Full CPU freq split: brackets on top row, freq on bottom row (right-aligned)
        cpuFullIsSplit = hasCpu && settings.showFullCPU && settings.showStackedFullCPU;
        // BAT stacked: power draw on top row, pct+estimate on bottom row
        batIsSplit = hasBat && settings.showStackedBAT;
        // DTC stacked: split user's dtcFormat at DIVIDER_SYMBOL into 2 rows
        dtcIsSplit = hasDtc && settings.showDTC && settings.showStackedDTC;
        ramLoadIsSplit = hasRam && settings.showRAMLoad && settings.showRAMLoadCPUGPU && settings.showStackedRAMLoadCPUGPU;
        // GPU die temp split: volt on top row, GPU temp on bottom row
        gpuIsSplit = hasGpu && settings.showGPUTemp && settings.showStackedGPUTemp && settings.realVolts;
        // RAM temp split: VDDQ-only Mariko → VDDQ top row, RAM temp bottom row
        ramTempSplit = hasRam && settings.showRAMTemp && settings.showStackedRAMTemp && !ramIsSplit &&
                       settings.realVolts &&
                       ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                        (settings.showVDD2 && !settings.showVDDQ));
        // RAM bandwidth split: BW on top row, normal RAM line on bottom row.
        // Only blocked when load-CPU/GPU-split already owns both rows with load data.
        // VDDQ split and RAM temp split are right-side column decorations — orthogonal to BW
        // row layout — and must NOT block BW from taking the top row.
        ramBWIsSplit = hasRam && settings.showRAMBandwidth && settings.showStackedRAMBandwidth &&
                       !ramLoadIsSplit;
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
                resetWidthDampers();       // clear sticky widths from any prior session
                tsl::hlp::requestForeground(false);
            }
            
            //renderer->drawRect(0, 0, tsl::cfg::FramebufferWidth, cachedMargin + 4, a(settings.backgroundColor));
            // Update render items so tmpIsGrid is correct before bar draw
            prepareRenderItems();
            calculateLayoutMetrics(renderer);
            {
                const int32_t gridExtraHeight = (tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit || cpuFullIsSplit || batIsSplit || ramLoadIsSplit || ramBWIsSplit || dtcIsSplit) ? ((int32_t)cachedMargin + gridGap) : 0;
                const int32_t vPad = (int32_t)settings.verticalPadding;
                // Visual text height = ascent + |descent|  (excludes lineGap)
                const int32_t textVisualH = cachedAscent + cachedDescentAbs;
                // barH: textVisualH + N above + N below, plus any grid expansion
                const int32_t barH = textVisualH + 2 * vPad + gridExtraHeight;
                // barY: top position → flush at Y=0; bottom → bar bottom at FramebufferHeight
                const int32_t barY = settings.setPosBottom
                    ? (int32_t)tsl::cfg::FramebufferHeight - barH
                    : 0;
                // Use focus background color while Plus-hold reposition mode is active.
                const uint16_t barBgColor = plusFocusActive.load(std::memory_order_acquire)
                    ? settings.focusBackgroundColor
                    : settings.backgroundColor;
                renderer->drawRect(0, barY, tsl::cfg::FramebufferWidth, barH, a(barBgColor));
            }

            
            // Separate battery from other items
            std::vector<RenderItem> main_items;
            //RenderItem* battery_item = nullptr;
            
            for (auto& item : renderItems) {
                //if (item.type == 6) { // BAT
                //    battery_item = &item;
                if (item.type == 5 && (!GameRunning || (strcmp(FPS_var_compressed_c, "254.0") == 0) || (strcmp(FPS_var_compressed_c, "254") == 0))) {
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
                if (item.has_voltage && item.volt_ptr && item.volt_ptr[0]) {
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

                    // Grid and single-row TMP: volt is drawn inline by the renderer after the fan,
                    // but data_width only covers temps+fan from the data string (no volt).
                    // Widen total_width by sep+volt so the column is allocated enough space.
                    if ((tmpIsGrid || !tmpIsSplit) && item.type == 4 &&
                        settings.realVolts && settings.showSOCVoltage && item.volt_ptr && item.volt_ptr[0]) {
                        const uint32_t volt_w = renderer->getTextDimensions(item.volt_ptr, false, fontsize).first;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + sep_width + volt_w;
                    }

                    // In split RAM mode both rows share the volt column: take max(vdd2_w, vddq_w).
                    // When SBS RAM temp is ON, temp draws at the center row (singleItemY) — use max of all 3.
                    // When SBS RAM temp is OFF, temp appends to VDDQ row — widen vddq_w accordingly.
                    if (ramIsSplit && item.type == 2 && (vdd2_only_c[0] || vddq_only_c[0])) {
                        const uint32_t vdd2_w = vdd2_only_c[0] ? renderer->getTextDimensions(vdd2_only_c, false, fontsize).first : 0;
                        const uint32_t vddq_w_base = vddq_only_c[0] ? renderer->getTextDimensions(vddq_only_c, false, fontsize).first : 0;
                        // The draw code uses rdiv_w = getTextDimensions(DIVIDER_SYMBOL) as the left connector
                        // between data_width and VDD2/VDDQ. Use div_w here (same char) so layout matches draw.
                        const uint32_t div_w = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        if (settings.showRAMTemp && ram_temp_c[0]) {
                            const uint32_t temp_w = renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                            if (!settings.showStackedRAMTemp) {
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
                                // Stacked temp (showStackedRAMTemp=true): temp shares a row with a voltage.
                                // VoltAtEnd OFF: temp appended after VDDQ on the bottom row.
                                //   right-of-voltColX = div_w + max(vdd2_w, vddq_w_base + div_w + temp_w)
                                // VoltAtEnd ON:  temp+DIV+VDD2 on top row, VDDQ alone on bottom row.
                                //   top row right-of-voltColX = div_w + temp_w + div_w + vdd2_w
                                //   bot row right-of-voltColX = div_w + vddq_w_base
                                //   → div_w + max(temp_w + div_w + vdd2_w, vddq_w_base)
                                if (!settings.voltageAtEndRAM) {
                                    const uint32_t vddq_w = vddq_w_base + div_w + temp_w;
                                    item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                              item_layout.data_width + div_w + std::max(vdd2_w, vddq_w);
                                } else {
                                    item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                              item_layout.data_width + div_w +
                                                              std::max(temp_w + div_w + vdd2_w, vddq_w_base);
                                }
                            }
                        } else {
                            item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                      item_layout.data_width + div_w + std::max(vdd2_w, vddq_w_base);
                        }
                    }

                    // CPU split: volt top row, CPU temp bottom row
                    // Draw uses divW = getTextDimensions(DIVIDER_SYMBOL) as connector; match here.
                    if (cpuIsSplit && item.type == 0) {
                        const uint32_t div_w  = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const uint32_t volt_w = CPU_volt_c[0] ? renderer->getTextDimensions(CPU_volt_c, false, fontsize).first : 0;
                        const uint32_t temp_w = cpu_temp_c[0] ? renderer->getTextDimensions(cpu_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + div_w + std::max(volt_w, temp_w);
                    }

                    // GPU split: volt top row, GPU temp bottom row
                    if (gpuIsSplit && item.type == 1) {
                        const uint32_t div_w  = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const uint32_t volt_w = GPU_volt_c[0] ? renderer->getTextDimensions(GPU_volt_c, false, fontsize).first : 0;
                        const uint32_t temp_w = gpu_temp_c[0] ? renderer->getTextDimensions(gpu_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + div_w + std::max(volt_w, temp_w);
                    }

                    // RAM temp split (single volt rail): volt top, RAM temp bottom
                    if (ramTempSplit && item.type == 2) {
                        const uint32_t div_w  = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const char* topVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                        const uint32_t volt_w = topVolt[0] ? renderer->getTextDimensions(topVolt, false, fontsize).first : 0;
                        const uint32_t temp_w = ram_temp_c[0] ? renderer->getTextDimensions(ram_temp_c, false, fontsize).first : 0;
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                  item_layout.data_width + div_w + std::max(volt_w, temp_w);
                    }

                    // Full CPU freq split: brackets on top row, freq right-aligned on bottom row.
                    // data_width = brackets_w; freq must fit within it (or extend it if wider).
                    if (cpuFullIsSplit && item.type == 0) {
                        const uint32_t freq_w = CPU_freq_c[0] ? renderer->getTextDimensions(CPU_freq_c, false, fontsize).first : 0;
                        const uint32_t dw = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        if (cpuIsSplit) {
                            // Stacked (SBS CPU Temp OFF + realVolts): volt/temp share the right column.
                            // cpuIsSplit block already computed the right column as dw+max(volt_w,temp_w);
                            // we just need to also consider freq_w vs brackets_w for the left column.
                            const uint32_t volt_w = CPU_volt_c[0] ? renderer->getTextDimensions(CPU_volt_c, false, fontsize).first : 0;
                            const uint32_t temp_w = cpu_temp_c[0] ? renderer->getTextDimensions(cpu_temp_c, false, fontsize).first : 0;
                            item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                      std::max(item_layout.data_width, freq_w) + dw + std::max(volt_w, temp_w);
                        } else {
                            // Inline volt/temp at singleItemY (SBS CPU Temp ON, or no temp/volt)
                            uint32_t secondary_w = 0;
                            if (settings.voltageAtEndCPU) {
                                if (cpu_temp_c[0])
                                    secondary_w += dw + renderer->getTextDimensions(cpu_temp_c, false, fontsize).first;
                                if (settings.realVolts && CPU_volt_c[0])
                                    secondary_w += dw + renderer->getTextDimensions(CPU_volt_c, false, fontsize).first;
                            } else {
                                if (settings.realVolts && CPU_volt_c[0])
                                    secondary_w += dw + renderer->getTextDimensions(CPU_volt_c, false, fontsize).first;
                                if (cpu_temp_c[0])
                                    secondary_w += dw + renderer->getTextDimensions(cpu_temp_c, false, fontsize).first;
                            }
                            item_layout.total_width = item_layout.label_width + layout.label_data_gap +
                                                      std::max(item_layout.data_width, freq_w) + secondary_w;
                        }
                    }
                }

                // For SBS temp modes, add the extra temp width.
                // When voltageAtEnd ON, has_voltage=false so we must also include the full sep+volt.
                {
                    const uint32_t div_w = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    if (!cpuIsSplit && !cpuFullIsSplit && item.type == 0 && settings.showCPUTemp && !settings.showStackedCPUTemp && cpu_temp_c[0]) {
                        const uint32_t temp_w = renderer->getTextDimensions(cpu_temp_c, false, fontsize).first;
                        if (settings.voltageAtEndCPU && settings.realVolts) {
                            const uint32_t volt_w = CPU_volt_c[0] ? renderer->getTextDimensions(CPU_volt_c, false, fontsize).first : 0;
                            item_layout.total_width += layout.volt_separator_gap + sep_width + layout.volt_data_gap + div_w + temp_w + volt_w;
                        } else {
                            item_layout.total_width += div_w + temp_w;
                        }
                    }
                    if (!gpuIsSplit && item.type == 1 && settings.showGPUTemp && !settings.showStackedGPUTemp && gpu_temp_c[0]) {
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
                
                // RAM load split: width = label + gap + max(brackets_w, total@freq_w)
                if (ramLoadIsSplit && item.type == 2) {
                    const uint32_t top_w = RAM_var_compressed_c[0] ? renderer->getTextDimensions(RAM_var_compressed_c, false, fontsize).first : 0;
                    const uint32_t bot_w = RAM_load_bot_c[0] ? renderer->getTextDimensions(RAM_load_bot_c, false, fontsize).first : 0;
                    uint32_t corrected = std::max(top_w, bot_w);
                    // bwLeftOfLoadSplit: BW NOT stacked but load IS stacked.
                    // Width = BW_w + divW + max(top_w, bot_w).
                    // RAM_bw_c is always populated (live value or "0.00 GB/s"), so measure directly.
                    if (settings.showRAMBandwidth && !settings.showStackedRAMBandwidth && RAM_bw_c[0]) {
                        const uint32_t bw_w   = renderer->getTextDimensions(RAM_bw_c, false, fontsize).first;
                        const uint32_t div_w  = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        corrected = bw_w + div_w + corrected;
                    }
                    item_layout.total_width += corrected - item_layout.data_width;
                    item_layout.data_width = corrected;
                }

                // RAM bandwidth split: width = max(BW top row, RAM content bottom row).
                // After the data-build swap, RAM_var_compressed_c holds the live BW string
                // (top row) and RAM_load_bot_c holds the regular RAM content (bottom row).
                // Measure both from live content — same pattern as every other split here.
                if (ramBWIsSplit && item.type == 2) {
                    const uint32_t top_w = RAM_var_compressed_c[0]
                        ? renderer->getTextDimensions(RAM_var_compressed_c, false, fontsize).first : 0;
                    const uint32_t bot_w = RAM_load_bot_c[0]
                        ? renderer->getTextDimensions(RAM_load_bot_c, false, fontsize).first : 0;
                    const uint32_t corrected = std::max(top_w, bot_w);
                    if (corrected > item_layout.data_width) {
                        item_layout.total_width += corrected - item_layout.data_width;
                        item_layout.data_width = corrected;
                    }
                }

                // BAT split: width = label + gap + max(draw_w, pct_w)
                if (batIsSplit && item.type == 6) {
                    const uint32_t draw_w = Battery_draw_c[0] ? renderer->getTextDimensions(Battery_draw_c, false, fontsize).first : 0;
                    const uint32_t pct_w  = Battery_pct_c[0]  ? renderer->getTextDimensions(Battery_pct_c,  false, fontsize).first : 0;
                    item_layout.total_width = item_layout.label_width + layout.label_data_gap + std::max(draw_w, pct_w);
                }

                // DTC split: width = label + gap + max(topHalf, botHalf) computed from the
                // LIVE DTC_c string (the actual rendered content) — not from probe samples.
                // Sample-based probes can be wider than the live string, which would push
                // the right-aligned content rightward and leave visible empty space on the
                // left of the cell. Using live widths makes the cell tightly hug whatever
                // is being drawn this frame, the same way inline DTC already works.
                // If the user's format has no DIVIDER_SYMBOL, leave data_width alone (it
                // was already set from item.data_ptr above).
                if (dtcIsSplit && item.type == 8 && settings.showDTC && item.data_ptr && item.data_ptr[0]) {
                    const std::string s(item.data_ptr);
                    const size_t divPos = s.find(ult::DIVIDER_SYMBOL);
                    if (divPos != std::string::npos) {
                        const std::string topHalf = s.substr(0, divPos);
                        const std::string botHalf = s.substr(divPos + ult::DIVIDER_SYMBOL.length());
                        const uint32_t tw = topHalf.empty() ? 0 : renderer->getTextDimensions(topHalf, false, fontsize).first;
                        const uint32_t bw = botHalf.empty() ? 0 : renderer->getTextDimensions(botHalf, false, fontsize).first;
                        const uint32_t maxW = std::max(tw, bw);
                        item_layout.total_width = item_layout.label_width + layout.label_data_gap + maxW;
                        item_layout.data_width  = maxW;
                    }
                }

                // ---- Anti-flicker damping ----
                // Damp ONLY total_width. label_width / data_width / volt_width are used
                // by the draw loop to advance current_x inside the cell, so they must
                // reflect the live text exactly. The damper enforces sticky_width >= raw,
                // so the cell footprint is always large enough to contain the text — any
                // slack appears as a few px of extra space at the right edge of the cell,
                // not as clipped text. Item positions downstream use this damped value.
                item_layout.total_width = dampItemWidth(item.type, item_layout.total_width);

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
            static const std::vector<std::string> cpuPadChars = {std::string(1, kPadSentinelMicro)};
            static const tsl::Color cpuPadTransparent{0, 0, 0, 0};
            // Bracket-aware draw: only the [...] portion of a string contains pad sentinels.
            // Everything outside the brackets is drawn with specialChars (so embedded
            // DIVIDER_SYMBOL characters get separatorColor). The bracket content is drawn
            // with cpuPadChars so the sentinel character is rendered fully transparent.
            // Returns the total advance width of the string.
            auto drawBracketAware = [&](const std::string& s, int x, int y) -> int {
                const size_t lbPos = s.find('[');
                const size_t rbPos = (lbPos != std::string::npos) ? s.find(']', lbPos) : std::string::npos;
                if (lbPos == std::string::npos || rbPos == std::string::npos) {
                    // No brackets -- no sentinels. Draw entirely with specialChars.
                    return (int)renderer->drawStringWithColoredSections(s, false, specialChars, x, y, fontsize, textColorA, (settings.separatorColor)).first;
                }
                int cx = x;
                if (lbPos > 0) {
                    const std::string pre = s.substr(0, lbPos);
                    cx += (int)renderer->drawStringWithColoredSections(pre, false, specialChars, cx, y, fontsize, textColorA, (settings.separatorColor)).first;
                }
                const std::string brk = s.substr(lbPos, rbPos - lbPos + 1);
                cx += (int)renderer->drawStringWithColoredSections(brk, false, cpuPadChars, cx, y, fontsize, textColorA, cpuPadTransparent).first;
                if (rbPos + 1 < s.size()) {
                    const std::string suf = s.substr(rbPos + 1);
                    cx += (int)renderer->drawStringWithColoredSections(suf, false, specialChars, cx, y, fontsize, textColorA, (settings.separatorColor)).first;
                }
                return cx - x;
            };

            // \u2500\u2500 Grid-mode Y coordinates \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
            // When tmpIsGrid: bar is taller; other items centered in expanded bar;
            // TMP renders as two rows (componentTemps_c top, skin_temperature_c bottom).
            const int32_t gridExtraH  = (tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit || cpuFullIsSplit || batIsSplit || ramLoadIsSplit || ramBWIsSplit || dtcIsSplit) ? ((int32_t)cachedMargin + gridGap) : 0;
            const int32_t halfExtra   = gridExtraH / 2;
            // Y for non-TMP items — centered in expanded bar (grid or split)
            const int32_t singleItemY = (int32_t)base_y + (int32_t)cachedMargin +
                                        ((tmpIsGrid || tmpIsSplit || ramIsSplit || cpuIsSplit || gpuIsSplit || ramTempSplit || cpuFullIsSplit || batIsSplit || ramLoadIsSplit || ramBWIsSplit || dtcIsSplit) ? (settings.setPosBottom ? -halfExtra : halfExtra) : 0);
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
                if (item.type == 3) { // SOC temperature
                    std::string dataStr(item.data_ptr);
                    const size_t degreesPos = dataStr.find("°");
                    if (degreesPos != std::string::npos) {
                        const size_t cPos = dataStr.find("C", degreesPos);
                        if (cPos != std::string::npos) {
                            const std::string tempPart = dataStr.substr(0, cPos + 1);
                            const std::string restPart = dataStr.substr(cPos + 1);
                            const int temp = atoi(item.data_ptr);
                            renderer->drawString(tempPart, false, current_x, singleItemY, fontsize, (settings.useDynamicColors ? tsl::GradientColor((float)temp) : textColorA));
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
                        // Dual-row split (splitBotTemps_c set): temps fill both rows, fan top, volt bottom.
                        // Single-group (!splitBotTemps_c[0]): temps at singleItemY (centered),
                        //   fan at tmpFanY and volt at tmpVoltY, both starting at the fan column X.
                        const bool splitIsDual = splitBotTemps_c[0] != 0;
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
                            // Temps draw at tmpFanY (dual-row) or singleItemY (single-group)
                            const int32_t tempDrawY = splitIsDual ? tmpFanY : singleItemY;
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
                                const bool useHigh = topIsComp && (settings.showStackedTemps || !pastTopDiv);
                                renderer->drawStringWithColoredSections(tp, false, specialChars, rx, tempDrawY, fontsize,
                                    settings.useDynamicColors ? (useHigh ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                                                         : tsl::GradientColor((float)tv)) : textColorA,
                                    (settings.separatorColor));
                                rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                rpos = cp + 1;
                            }
                            fanColX = rx; // mark where fan column starts -- divider+fan drawn below by fan row section
                            if (!rOK) {
                                {
                                    static const std::string socFanIcon_ = "";
                                    const std::string socStr_(skin_temperature_c);
                                    const size_t fanPos_ = socStr_.find(socFanIcon_);
                                    uint32_t cx_ = current_x;
                                    if (fanPos_ != std::string::npos) {
                                        if (fanPos_ > 0)
                                            cx_ += renderer->drawStringWithColoredSections(socStr_.substr(0, fanPos_), false, specialChars, cx_, tempDrawY, fontsize, textColorA, (settings.separatorColor)).first;
                                        cx_ += renderer->drawString(socFanIcon_, false, cx_, tempDrawY, fontsize, (settings.catColor)).first;
                                        const std::string afterFan_ = socStr_.substr(fanPos_ + socFanIcon_.size());
                                        if (!afterFan_.empty())
                                            renderer->drawStringWithColoredSections(afterFan_, false, specialChars, cx_, tempDrawY, fontsize, textColorA, (settings.separatorColor));
                                    } else {
                                        renderer->drawStringWithColoredSections(socStr_, false, specialChars, cx_, tempDrawY, fontsize, textColorA, (settings.separatorColor));
                                    }
                                }
                                fanColX = current_x + renderer->getTextDimensions(skin_temperature_c, false, fontsize).first;
                            }
                        }
                        // Scissored 3-stack at fanColX (mirrors Mini TMP_SFAN/TMP_SVOLT).
                        // Fan and volt each occupy one physical row; they swap when voltageAtEndTMP is OFF.
                        //   voltageAtEndTMP ON : fan=gridTopY(TOP), volt=gridBotY(BOT)
                        //   voltageAtEndTMP OFF: volt=gridTopY(TOP), fan=gridBotY(BOT)
                        const bool tmpHasFan  = settings.showFanPercentage;
                        const bool tmpHasVolt = SOC_volt_c[0] != 0;
                        const bool tmpFanIsTop = settings.voltageAtEndTMP;
                        int tmpSplitCOff = 0, tmpSplitLP = 0, tmpSplitFW = 0;
                        const bool tmpSplitScOK = divider_scissor::getDividerScissorMetrics(
                            fontsize, tmpSplitCOff, tmpSplitLP, tmpSplitFW);
                        int tmpCutTC = 0, tmpCutCB = 0;
                        if (tmpSplitScOK) {
                            tmpCutTC = divider_scissor::computeDividerCutY(
                                (int)gridTopY, (int)singleItemY, tmpSplitCOff);
                            tmpCutCB = divider_scissor::computeDividerCutY(
                                (int)singleItemY, (int)gridBotY, tmpSplitCOff);
                        }
                        const uint32_t tmpDivW = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;

                        // Center connector divider (scissored to center band based on which rows are present)
                        if (splitIsDual || tmpHasFan || tmpHasVolt) {
                            const bool hasTmpTopRow = tmpFanIsTop ? tmpHasFan : tmpHasVolt;
                            const bool hasTmpBotRow = tmpFanIsTop ? tmpHasVolt : tmpHasFan;
                            if (tmpSplitScOK && hasTmpTopRow && hasTmpBotRow) {
                                divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)singleItemY,
                                    tmpCutTC, tmpCutCB, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                            } else if (tmpSplitScOK && hasTmpTopRow) {
                                divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)singleItemY,
                                    tmpCutTC, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                            } else if (tmpSplitScOK && hasTmpBotRow) {
                                divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)singleItemY,
                                    divider_scissor::kNoUpperCut, tmpCutCB, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                            } else {
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)fanColX, (float)singleItemY, fontsize, (settings.separatorColor));
                            }
                        }

                        // Fan row: scissored div at its physical row, then fan content
                        if (tmpHasFan) {
                            if (tmpSplitScOK) {
                                if (tmpFanIsTop)
                                    divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)gridTopY,
                                        divider_scissor::kNoUpperCut, tmpCutTC, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                                else
                                    divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)gridBotY,
                                        tmpCutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                            } else {
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)fanColX, (float)fanDrawY, fontsize, (settings.separatorColor));
                            }
                            // Fan content: build directly from Rotation_Duty (mirrors Mini TMP_SFAN)
                            {
                                const int tmpFanDuty = safeFanDuty((int)Rotation_Duty);
                                char tmpFanPctStr[24];
                                snprintf(tmpFanPctStr, sizeof(tmpFanPctStr), " %d%%", tmpFanDuty);
                                static const std::vector<std::string> tmpFanFic = {""};
                                renderer->drawStringWithColoredSections(std::string(tmpFanPctStr), false, tmpFanFic,
                                    fanColX + tmpDivW, fanDrawY, fontsize, textColorA, catColorA);
                            }
                        }

                        // Bottom row: optional dual-row bot temps + SOC voltage
                        {
                            // Dual-row: draw bot temps at tmpVoltY starting from current_x (left side)
                            if (splitIsDual) {
                                uint32_t rx = current_x;
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
                                        settings.useDynamicColors ? tsl::GradientColor((float)tv) : textColorA, (settings.separatorColor));
                                    rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                    rpos = cp + 1;
                                }
                                if (!rOK) {
                                    renderer->drawStringWithColoredSections(splitBotTemps_c, false, specialChars, current_x, tmpVoltY, fontsize, textColorA, (settings.separatorColor));
                                }
                            }
                            // SOC voltage: anchored at fanColX (same column as fan), mirrors Mini TMP_SVOLT.
                            // Volt divider is scissored to its physical row band.
                            if (tmpHasVolt) {
                                if (tmpSplitScOK) {
                                    if (!tmpFanIsTop)
                                        divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)gridTopY,
                                            divider_scissor::kNoUpperCut, tmpCutTC, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                                    else
                                        divider_scissor::drawDividerScissored(renderer, (int)fanColX, (int)gridBotY,
                                            tmpCutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), tmpSplitLP, tmpSplitFW);
                                } else {
                                    renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)fanColX, (float)voltDrawY, fontsize, (settings.separatorColor));
                                }
                                renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars,
                                    fanColX + tmpDivW, voltDrawY, fontsize, textColorA, (settings.separatorColor));
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
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA, (settings.separatorColor));
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
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv) : textColorA, (settings.separatorColor));
                                rx += renderer->getTextDimensions(tp, false, fontsize).first;
                                rpos = cp + 1;
                            }
                            if (rpos < rowStr.length()) {
                                const std::string rest = rowStr.substr(rpos);
                                const size_t dv = rest.find(ult::DIVIDER_SYMBOL);
                                if (dv != std::string::npos) {
                                    const size_t dl = ult::DIVIDER_SYMBOL.length();
                                    // Left side is 2-row (top+bot temps) -> 3-tall connector before fan/volt,
                                    // mirroring Mini TMP_TOP lines 1243-1250.
                                    divider_scissor::drawDividerStack3(renderer, rx, gridTopY, singleItemY, gridBotY, fontsize, (settings.separatorColor));
                                    rx += renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                    const std::string ad = rest.substr(dv + dl);
                                    const bool gridHasVolt = settings.realVolts && settings.showSOCVoltage && SOC_volt_c[0];
                                    // voltageAtEndTMP OFF: volt before fan (mirrors Mini TMP_COMP ordering)
                                    if (!settings.voltageAtEndTMP && gridHasVolt) {
                                        renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                        rx += renderer->getTextDimensions(SOC_volt_c, false, fontsize).first;
                                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                                    }
                                    if (!ad.empty()) {
                                        static const std::vector<std::string> fic2 = {""};
                                        renderer->drawStringWithColoredSections(ad, false, fic2, rx, singleItemY, fontsize, textColorA, catColorA);
                                        rx += renderer->getTextDimensions(ad, false, fontsize).first;
                                    }
                                    // voltageAtEndTMP ON: volt after fan
                                    if (settings.voltageAtEndTMP && gridHasVolt) {
                                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                                        renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                    }
                                } else {
                                    renderer->drawStringWithColoredSections(rest, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                }
                            }
                            if (!rOK) {
                            static const std::string socFanIcon2_ = "";
                            const std::string socStr2_(skin_temperature_c);
                            const size_t fanPos2_ = socStr2_.find(socFanIcon2_);
                            uint32_t cx2_ = current_x;
                            if (fanPos2_ != std::string::npos) {
                                if (fanPos2_ > 0)
                                    cx2_ += renderer->drawStringWithColoredSections(socStr2_.substr(0, fanPos2_), false, specialChars, cx2_, gridBotY, fontsize, textColorA, (settings.separatorColor)).first;
                                cx2_ += renderer->drawString(socFanIcon2_, false, cx2_, gridBotY, fontsize, (settings.catColor)).first;
                                const std::string afterFan2_ = socStr2_.substr(fanPos2_ + socFanIcon2_.size());
                                if (!afterFan2_.empty())
                                    renderer->drawStringWithColoredSections(afterFan2_, false, specialChars, cx2_, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            } else {
                                renderer->drawStringWithColoredSections(socStr2_, false, specialChars, cx2_, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        }
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
                            const bool useHigh = compOnly || (!settings.showStackedTemps && !pastDivider && settings.showComponentTemps);
                            const tsl::Color tempColor = useHigh ?
                                tsl::GradientColor((float)temp, tsl::DEFAULT_TEMP_RANGE_HIGH) :
                                tsl::GradientColor((float)temp);
                            renderer->drawStringWithColoredSections(tempPart, false, specialChars, renderX, singleItemY, fontsize, settings.useDynamicColors ? tempColor : textColorA, (settings.separatorColor));
                            renderX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                            pos = cPos + 1;
                        }
                        // Single-row volt + fan: mirrors Mini TMP_COMP ordering.
                        // voltageAtEndTMP OFF => div+volt then div+fan; ON => div+fan then div+volt.
                        {
                            const bool singleHasVolt = settings.realVolts && settings.showSOCVoltage && SOC_volt_c[0];
                            // volt BEFORE fan when voltageAtEndTMP is OFF
                            if (!settings.voltageAtEndTMP && singleHasVolt) {
                                renderX += renderer->drawString(ult::DIVIDER_SYMBOL, false, renderX, singleItemY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars, renderX, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                renderX += renderer->getTextDimensions(SOC_volt_c, false, fontsize).first;
                            }
                            // fan from data string (embedded as  duty%) — only if temps drew ok
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
                                        renderX += renderer->getTextDimensions(afterDiv, false, fontsize).first;
                                    }
                                } else {
                                    renderer->drawStringWithColoredSections(restPart, false, specialChars, renderX, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                    renderX += renderer->getTextDimensions(restPart, false, fontsize).first;
                                }
                            } else if (!parseSuccess && renderX == current_x) {
                                // Full fallback: nothing drew from parse loop
                                renderer->drawStringWithColoredSections(item.data_ptr, false, specialChars, current_x, singleItemY, fontsize, textColorA, (settings.separatorColor));
                                renderX += renderer->getTextDimensions(item.data_ptr, false, fontsize).first;
                            }
                            // volt AFTER fan when voltageAtEndTMP is ON
                            if (settings.voltageAtEndTMP && singleHasVolt) {
                                renderX += renderer->drawString(ult::DIVIDER_SYMBOL, false, renderX, singleItemY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(SOC_volt_c, false, specialChars, renderX, singleItemY, fontsize, textColorA, (settings.separatorColor));
                            }
                        }
                    }
                } else {
                    // Dynamic colors off: normal rendering
                    // cpuFullIsSplit: brackets go on the top row, not the center row

                    // DTC split: pre-compute top half of the formatted datetime (everything
                    // before DIVIDER_SYMBOL). Bottom half is drawn separately below.
                    // If the user's format has no DIVIDER_SYMBOL, dtcDoSplit stays false and
                    // we fall through to the normal full-string draw.
                    std::string dtcTopHalfStr;
                    bool dtcDoSplit = false;
                    if (dtcIsSplit && item.type == 8 && item.data_ptr && item.data_ptr[0]) {
                        const std::string dtStr(item.data_ptr);
                        const size_t divPos = dtStr.find(ult::DIVIDER_SYMBOL);
                        if (divPos != std::string::npos) {
                            dtcTopHalfStr = dtStr.substr(0, divPos);
                            dtcDoSplit = true;
                        }
                    }
                    const char* dtcTopPtr = dtcDoSplit ? dtcTopHalfStr.c_str() : item.data_ptr;
                    const uint32_t dtcTopW = dtcDoSplit
                        ? renderer->getTextDimensions(dtcTopHalfStr, false, fontsize).first : 0u;

                    const int32_t dataY = (cpuFullIsSplit && item.type == 0) ? gridTopY : (batIsSplit && item.type == 6) ? gridTopY : ((ramLoadIsSplit || ramBWIsSplit) && item.type == 2) ? gridTopY : (dtcDoSplit) ? gridTopY : singleItemY;
                    // ramLoadIsSplit top row: right-align [cpu% gpu%] so its right edge matches total@freq
                    // cpuFullIsSplit top row: right-align brackets within max(brackets_w, freq_w) so both
                    //   rows share the same right edge and neither overflows left into the label area.
                    const uint32_t cpuFullFreqW = (cpuFullIsSplit && item.type == 0 && CPU_freq_c[0])
                        ? (uint32_t)renderer->getTextDimensions(CPU_freq_c, false, fontsize).first : 0u;
                    const uint32_t cpuFullColW = (cpuFullIsSplit && item.type == 0)
                        ? std::max(item_layout.data_width, cpuFullFreqW) : item_layout.data_width;
                    // bwLeftCol: BW inline (not stacked) + load IS stacked. BW draws as left column.
                    const bool bwLeftCol = (ramLoadIsSplit && item.type == 2)
                                         && settings.showRAMBandwidth && !settings.showStackedRAMBandwidth
                                         && RAM_bw_c[0];
                    const uint32_t bwLeftW  = bwLeftCol ? renderer->getTextDimensions(std::string(RAM_bw_c), false, fontsize).first : 0u;
                    const uint32_t bwDivW   = bwLeftCol ? renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first : 0u;
                    // loadDataColW: width of the stacked-load portion only (excludes BW + divider)
                    const uint32_t loadDataColW = bwLeftCol ? (item_layout.data_width - bwLeftW - bwDivW) : item_layout.data_width;
                    if (bwLeftCol) {
                        // Draw BW centered (at singleItemY) in the left column.
                        // Nothing occupies the bottom row of this column, so centering it
                        // aligns it with the RAM label and the center divider.
                        renderer->drawString(std::string(RAM_bw_c), false, current_x, singleItemY, fontsize, textColorA);
                        // Draw 3-stack dividers: top, center, and bot rows at current_x + bwLeftW
                        const uint32_t divX = current_x + bwLeftW;
                        {
                            int cOff=0, lP=0, fW=0;
                            if (divider_scissor::getDividerScissorMetrics(fontsize, cOff, lP, fW)) {
                                const int cutTC = divider_scissor::computeDividerCutY((int)gridTopY, (int)singleItemY, cOff);
                                const int cutCB = divider_scissor::computeDividerCutY((int)singleItemY, (int)gridBotY, cOff);
                                divider_scissor::drawDividerScissored(renderer, (int)divX, (int)gridTopY,   divider_scissor::kNoUpperCut, cutTC, fontsize, (settings.separatorColor), lP, fW);
                                divider_scissor::drawDividerScissored(renderer, (int)divX, (int)singleItemY, cutTC, cutCB, fontsize, (settings.separatorColor), lP, fW);
                                divider_scissor::drawDividerScissored(renderer, (int)divX, (int)gridBotY,   cutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), lP, fW);
                            } else {
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, divX, gridTopY,    fontsize, (settings.separatorColor));
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, divX, singleItemY, fontsize, (settings.separatorColor));
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, divX, gridBotY,    fontsize, (settings.separatorColor));
                            }
                        }
                        RAM_bw_c[0] = '\0';
                    }
                    // loadDataStartX: left edge of the stacked-load content (after BW+div if bwLeftCol)
                    const uint32_t loadDataStartX = current_x + (bwLeftCol ? bwLeftW + bwDivW : 0u);
                    const uint32_t drawX = ((ramLoadIsSplit || ramBWIsSplit) && item.type == 2 && RAM_load_bot_c[0])
                        ? loadDataStartX + loadDataColW - renderer->getTextDimensions(item.data_ptr, false, fontsize).first
                        : (cpuFullIsSplit && item.type == 0)
                        ? current_x + (cpuFullColW - item_layout.data_width)  // right-align brackets
                        : (batIsSplit && item.type == 6)
                        ? current_x + item_layout.data_width - renderer->getTextDimensions(item.data_ptr, false, fontsize).first  // right-align top bat row
                        : (dtcDoSplit)
                        ? current_x + item_layout.data_width - dtcTopW  // right-align top DTC half
                        : current_x;

                    if (item.type == 0 || item.type == 2) {
                        // CPU (type 0) and RAM load brackets (type 2) may contain '#' padding
                        // sentinels from mkPad/mkPadR — draw them fully transparent to hide them
                        // while preserving the glyph-advance spacing.
                        // RAM (type 2) in the BW-stacked + load-stacked case embeds a DIVIDER_SYMBOL
                        // between the BW string and the bracket string. Split at DIVIDER_SYMBOL so the
                        // BW part uses specialChars (separator color for DIVIDER) and the bracket part
                        // uses cpuPadChars (transparent '#' sentinels). CPU strings never embed DIVIDER
                        // here, so the else branch is taken for CPU (safe no-op split-check).
                        const std::string topStr(dtcTopPtr);
                        const size_t divPos = (item.type == 2) ? topStr.find(ult::DIVIDER_SYMBOL) : std::string::npos;
                        if (divPos != std::string::npos) {
                            const std::string bwPart  = topStr.substr(0, divPos);
                            const std::string brkPart = topStr.substr(divPos + ult::DIVIDER_SYMBOL.length());
                            int cx = drawX;
                            cx += (int)renderer->drawStringWithColoredSections(bwPart, false, specialChars, cx, dataY, fontsize, textColorA, (settings.separatorColor)).first;
                            cx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, dataY, fontsize, (settings.separatorColor)).first;
                            drawBracketAware(brkPart, cx, dataY);
                        } else {
                            drawBracketAware(std::string(dtcTopPtr), drawX, dataY);
                        }
                    } else {
                        renderer->drawStringWithColoredSections(dtcTopPtr, false, specialChars, drawX, dataY, fontsize, textColorA, (settings.separatorColor));
                    }
                }
                current_x += item_layout.data_width;
                // Save the start of the data column before voltage may advance current_x.
                // Used by ramLoadIsSplit to draw the bottom row at the correct X.
                const uint32_t dataColStartX = current_x - item_layout.data_width;

                // Draw voltage if present -- inline at singleItemY for all modes including TMP grid.
                // When ramLoadIsSplit/ramBWIsSplit for a RAM item, drawDividerStack3 in rLoadHasRight
                // already drew the center divider scissored; skip drawing "" here to avoid doubling it.
                // Still advance current_x by sep_width so the volt text lands at the correct X.
                if (item.has_voltage && item.volt_ptr && item.volt_ptr[0]) {
                    const bool ramLoadSuppressSep = (ramLoadIsSplit || ramBWIsSplit) && item.type == 2;
                    current_x += layout.volt_separator_gap;
                    if (!ramLoadSuppressSep)
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
                    // Determine which rows will be drawn at voltColX for scissoring the center
                    const bool ramSplitHasTop = !settings.voltageAtEndRAM ? vdd2_only_c[0] != 0
                        : (settings.showStackedRAMTemp && ram_temp_c[0]) ? vdd2_only_c[0] != 0
                        : ramLoadIsSplit;
                    const bool ramSplitHasBot = !settings.voltageAtEndRAM ? vddq_only_c[0] != 0
                        : (settings.showStackedRAMTemp && ram_temp_c[0]) ? vddq_only_c[0] != 0
                        : ramLoadIsSplit;
                    // Center divider at voltColX: only when NOT ramLoadIsSplit/ramBWIsSplit.
                    // When those are true, drawDividerStack3 in rLoadHasRight already drew
                    // all three rows scissored; re-drawing center here would double the glyph.
                    if (!ramLoadIsSplit && !ramBWIsSplit) {
                        int cOff=0,lP=0,fW=0;
                        if (divider_scissor::getDividerScissorMetrics(fontsize,cOff,lP,fW)) {
                            const int cutTC = ramSplitHasTop ? divider_scissor::computeDividerCutY((int)gridTopY,(int)singleItemY,cOff) : divider_scissor::kNoUpperCut;
                            const int cutCB = ramSplitHasBot ? divider_scissor::computeDividerCutY((int)singleItemY,(int)gridBotY,cOff) : divider_scissor::kNoLowerCut;
                            divider_scissor::drawDividerScissored(renderer,(int)voltColX,(int)singleItemY,cutTC,cutCB,fontsize,(settings.separatorColor),lP,fW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, singleItemY, fontsize, (settings.separatorColor));
                        }
                    }
                    if (!settings.voltageAtEndRAM) {
                        // ── Normal ──
                        if (vdd2_only_c[0]) {
                            uint32_t rx = voltColX;
                            // When ramLoadIsSplit/ramBWIsSplit, drawDividerStack3 already drew
                            // the top divider; just advance past it. Otherwise draw it now.
                            if (!(ramLoadIsSplit || ramBWIsSplit))
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                            else
                                rx += rdiv_w;
                            renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                        }
                        if (vddq_only_c[0]) {
                            uint32_t rx = voltColX;
                            // Same for bot divider.
                            if (!(ramLoadIsSplit || ramBWIsSplit))
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                            else
                                rx += rdiv_w;
                            renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            if (settings.showRAMTemp && settings.showStackedRAMTemp && ram_temp_c[0]) {
                                rx += renderer->getTextDimensions(vddq_only_c, false, fontsize).first;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, rx, gridBotY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                            }
                        }
                        if (settings.showRAMTemp && !settings.showStackedRAMTemp && ram_temp_c[0]) {
                            const uint32_t vdd2_rw = vdd2_only_c[0] ? renderer->getTextDimensions(vdd2_only_c, false, fontsize).first : 0;
                            const uint32_t vddq_rw = vddq_only_c[0] ? renderer->getTextDimensions(vddq_only_c, false, fontsize).first : 0;
                            uint32_t cx = voltColX + rdiv_w + std::max(vdd2_rw, vddq_rw);
                            // Single center divider: temp is 1-row at singleItemY; 3-tall only between two 2-row blocks
                            cx += renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, singleItemY, fontsize, (settings.separatorColor)).first;
                            const int rtv = atoi(ram_temp_c);
                            renderer->drawString(ram_temp_c, false, cx, singleItemY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    } else {
                        // ── VoltAtEnd ON ──
                        if (settings.showStackedRAMTemp && ram_temp_c[0]) {
                            // SBS off (stacked temp): temp+DIV+VDD2 on top row, VDDQ on bottom row.
                            // When ramLoadIsSplit/ramBWIsSplit, drawDividerStack3 already drew the
                            // top and bot row dividers scissored; skip re-drawing them and just
                            // advance rx past the divider width so text lands at the correct X.
                            if (vdd2_only_c[0]) {
                                uint32_t rx = voltColX;
                                if (!(ramLoadIsSplit || ramBWIsSplit))
                                    rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                                else
                                    rx += rdiv_w;
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, rx, gridTopY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                                rx += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                                rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridTopY, fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                            }
                            if (vddq_only_c[0]) {
                                uint32_t rx = voltColX;
                                if (!(ramLoadIsSplit || ramBWIsSplit))
                                    rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, gridBotY, fontsize, (settings.separatorColor)).first;
                                else
                                    rx += rdiv_w;
                                renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        } else {
                            // SBS on + VoltAtEnd: temp at center first (left-connector IS the divider before temp),
                            // then a right-fork connector, then VDD2/VDDQ to the right.
                            // Layout from voltColX:  ╸[temp]╸[VDD2 top]
                            //                        ╸[temp]╸  (right connector at center)
                            //                        ╸[temp]╸[VDDQ bot]
                            // Left connector at voltColX: center already drawn at singleItemY above.
                            // Add top+bot only when left data block is also 2-row (ramLoadIsSplit);
                            // 3-tall only when both sides are 2-row. 1-row data needs just the center.
                            // When ramLoadIsSplit, rLoadHasRight already drew top+bot dividers at voltColX.
                            // No need to draw them again here.
                            uint32_t voltStartX = voltColX + rdiv_w; // right after left connector; temp starts here
                            if (settings.showRAMTemp && !settings.showStackedRAMTemp && ram_temp_c[0]) {
                                // Temp at center row; right-fork is 3-tall before the volt column
                                const int rtv = atoi(ram_temp_c);
                                renderer->drawString(ram_temp_c, false, voltStartX, singleItemY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                                voltStartX += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                                // Right-fork at voltStartX: scissored 3-stack (center + top + bot)
                                { int cOff=0,lP=0,fW=0;
                                  if (divider_scissor::getDividerScissorMetrics(fontsize,cOff,lP,fW)) {
                                    const bool rfHasTop = vdd2_only_c[0] != 0;
                                    const bool rfHasBot = vddq_only_c[0] != 0;
                                    const int cutTC = rfHasTop ? divider_scissor::computeDividerCutY((int)gridTopY,(int)singleItemY,cOff) : divider_scissor::kNoUpperCut;
                                    const int cutCB = rfHasBot ? divider_scissor::computeDividerCutY((int)singleItemY,(int)gridBotY,cOff) : divider_scissor::kNoLowerCut;
                                    divider_scissor::drawDividerScissored(renderer,(int)voltStartX,(int)singleItemY,cutTC,cutCB,fontsize,(settings.separatorColor),lP,fW);
                                    if (rfHasTop) divider_scissor::drawDividerScissored(renderer,(int)voltStartX,(int)gridTopY,divider_scissor::kNoUpperCut,cutTC,fontsize,(settings.separatorColor),lP,fW);
                                    if (rfHasBot) divider_scissor::drawDividerScissored(renderer,(int)voltStartX,(int)gridBotY,cutCB,divider_scissor::kNoLowerCut,fontsize,(settings.separatorColor),lP,fW);
                                  } else {
                                    renderer->drawString(ult::DIVIDER_SYMBOL, false, voltStartX, singleItemY, fontsize, (settings.separatorColor));
                                  }
                                }
                            }
                            // VDD2/VDDQ: draw content only.
                            // When ramLoadIsSplit, rLoadHasRight drew the divider at voltColX already;
                            // voltStartX = voltColX+rdiv_w is exactly where the volt text starts.
                            // When not ramLoadIsSplit, voltStartX+rdiv_w is correct (center div
                            // at voltColX was the only left connector, text follows at +rdiv_w).
                            if (vdd2_only_c[0]) {
                                uint32_t rx = ramLoadIsSplit ? voltStartX : voltStartX + rdiv_w;
                                renderer->drawStringWithColoredSections(vdd2_only_c, false, specialChars, rx, gridTopY, fontsize, textColorA, (settings.separatorColor));
                            }
                            if (vddq_only_c[0]) {
                                uint32_t rx = ramLoadIsSplit ? voltStartX : voltStartX + rdiv_w;
                                renderer->drawStringWithColoredSections(vddq_only_c, false, specialChars, rx, gridBotY, fontsize, textColorA, (settings.separatorColor));
                            }
                        }
                    }
                }

                // When cpuFullIsSplit, the left column is max(brackets_w, freq_w) wide so both rows
                // share the same right edge. Pre-compute here so cpuIsSplit can use the correct voltColX
                // (otherwise volt/temp would land inside the freq text when freq_w > brackets_w).
                const uint32_t cpuFullMeasColW = (cpuFullIsSplit && item.type == 0)
                    ? std::max(item_layout.data_width,
                               CPU_freq_c[0] ? (uint32_t)renderer->getTextDimensions(CPU_freq_c, false, fontsize).first : 0u)
                    : item_layout.data_width;

                // CPU split: volt/temp on top/bottom rows; swapped when voltageAtEndCPU.
                // voltColX uses cpuFullMeasColW so the divider sits at the shared right edge even
                // when the freq line is wider than the brackets.
                if (cpuIsSplit && item.type == 0) {
                    // CPU split: center row = data label; top/bot rows = volt/temp (swapped by voltageAtEndCPU).
                    // Mini-style 3-stack scissoring: center div scissored to center band,
                    // top/bot row divs each scissored to their own band, drawn before their content.
                    const uint32_t voltColX = dataColStartX + cpuFullMeasColW;
                    const int32_t cpuVoltY = settings.voltageAtEndCPU ? gridBotY : gridTopY;
                    const int32_t cpuTempY = settings.voltageAtEndCPU ? gridTopY : gridBotY;
                    const bool cpuHasVolt = CPU_volt_c[0] != 0;
                    const bool cpuHasTemp = cpu_temp_c[0] != 0;
                    // Scissor metrics
                    int cpuSplitCOff = 0, cpuSplitLP = 0, cpuSplitFW = 0;
                    const bool cpuSplitScOK = divider_scissor::getDividerScissorMetrics(
                        fontsize, cpuSplitCOff, cpuSplitLP, cpuSplitFW);
                    // Always compute cuts in screen-ascending order (gridTopY < singleItemY < gridBotY).
                    const int cpuSplitCutTC = cpuSplitScOK
                        ? divider_scissor::computeDividerCutY((int)gridTopY, (int)singleItemY, cpuSplitCOff) : 0;
                    const int cpuSplitCutCB = cpuSplitScOK
                        ? divider_scissor::computeDividerCutY((int)singleItemY, (int)gridBotY, cpuSplitCOff) : 0;
                    // Center divider lambda: cuts toward whichever physical rows are present.
                    // voltIsOnTop = volt occupies gridTopY row (!voltageAtEndCPU).
                    const bool cpuVoltIsOnTop = !settings.voltageAtEndCPU;
                    const bool cpuHasTopRow = cpuVoltIsOnTop ? cpuHasVolt : cpuHasTemp;
                    const bool cpuHasBotRow = cpuVoltIsOnTop ? cpuHasTemp : cpuHasVolt;
                    auto cpuDrawCenterDiv = [&](uint32_t dx) {
                        if (cpuSplitScOK && cpuHasTopRow && cpuHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                cpuSplitCutTC, cpuSplitCutCB, fontsize, (settings.separatorColor), cpuSplitLP, cpuSplitFW);
                        } else if (cpuSplitScOK && cpuHasTopRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                cpuSplitCutTC, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), cpuSplitLP, cpuSplitFW);
                        } else if (cpuSplitScOK && cpuHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                divider_scissor::kNoUpperCut, cpuSplitCutCB, fontsize, (settings.separatorColor), cpuSplitLP, cpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)singleItemY, fontsize, (settings.separatorColor));
                        }
                    };
                    // Top-row divider lambda (always at gridTopY, scissored to top band)
                    auto cpuDrawTopRowDiv = [&](uint32_t dx) {
                        if (cpuSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridTopY,
                                divider_scissor::kNoUpperCut, cpuSplitCutTC, fontsize, (settings.separatorColor), cpuSplitLP, cpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)cpuVoltY, fontsize, (settings.separatorColor));
                        }
                    };
                    // Bot-row divider lambda (always at gridBotY, scissored to bottom band)
                    auto cpuDrawBotRowDiv = [&](uint32_t dx) {
                        if (cpuSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridBotY,
                                cpuSplitCutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), cpuSplitLP, cpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)cpuTempY, fontsize, (settings.separatorColor));
                        }
                    };
                    const uint32_t divW = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    // Centre connector divider
                    cpuDrawCenterDiv(voltColX);
                    // Top physical row: draw scissored top-band div, then content at that row's Y
                    if (cpuHasTopRow) {
                        cpuDrawTopRowDiv(voltColX);
                        if (cpuVoltIsOnTop && cpuHasVolt)
                            renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars,
                                voltColX + divW, cpuVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (!cpuVoltIsOnTop && cpuHasTemp) {
                            const int tv = atoi(cpu_temp_c);
                            renderer->drawString(cpu_temp_c, false, voltColX + divW, cpuTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                    // Bottom physical row: draw scissored bot-band div, then content at that row's Y
                    if (cpuHasBotRow) {
                        cpuDrawBotRowDiv(voltColX);
                        if (!cpuVoltIsOnTop && cpuHasVolt)
                            renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars,
                                voltColX + divW, cpuVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (cpuVoltIsOnTop && cpuHasTemp) {
                            const int tv = atoi(cpu_temp_c);
                            renderer->drawString(cpu_temp_c, false, voltColX + divW, cpuTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                }

                // Full CPU freq split: draw center connector + inline volt/temp at singleItemY (when !cpuIsSplit),
                // and @freq on bottom row right-aligned to the shared column right edge.
                if (cpuFullIsSplit && item.type == 0) {
                    // colW already computed above as cpuFullMeasColW = max(brackets_w, freq_w).
                    const uint32_t colW = cpuFullMeasColW;
                    const int32_t bracketsRightX = (int32_t)(dataColStartX + colW);
                    // Center connector + inline volt/temp (only when cpuIsSplit is NOT handling stacked case)
                    if (!cpuIsSplit) {
                        // Center connector: 3-tall when volt/temp content will follow, scissored
                        const bool cpuFullWillDraw = cpu_temp_c[0] || (settings.realVolts && CPU_volt_c[0]);
                        if (cpuFullWillDraw) {
                            divider_scissor::drawDividerStack3(renderer, (int)bracketsRightX,
                                (int)gridTopY, (int)singleItemY, (int)gridBotY, fontsize, (settings.separatorColor));
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, (uint32_t)bracketsRightX,
                                singleItemY, fontsize, (settings.separatorColor));
                        }
                        // Inline volt/temp at singleItemY, right of center connector.
                        // rx starts PAST the center divider (already drawn by the stack above)
                        // so the first inline item must NOT draw another leading divider.
                        const uint32_t divW = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        uint32_t rx = cpuFullWillDraw ? (uint32_t)bracketsRightX + divW : (uint32_t)bracketsRightX;
                        bool cpuFullDrewFirst = false;
                        if (settings.voltageAtEndCPU) {
                            if (cpu_temp_c[0]) {
                                const int tv = atoi(cpu_temp_c);
                                rx += renderer->drawString(cpu_temp_c, false, rx, singleItemY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA).first;
                                cpuFullDrewFirst = true;
                            }
                            if (settings.realVolts && CPU_volt_c[0]) {
                                if (cpuFullDrewFirst)
                                    rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY,
                                        fontsize, (settings.separatorColor)).first;
                                renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars, rx,
                                    singleItemY, fontsize, textColorA, (settings.separatorColor));
                            }
                        } else {
                            if (settings.realVolts && CPU_volt_c[0]) {
                                const int tv2 = 0; (void)tv2;
                                renderer->drawStringWithColoredSections(CPU_volt_c, false, specialChars, rx,
                                    singleItemY, fontsize, textColorA, (settings.separatorColor));
                                rx += renderer->getTextDimensions(CPU_volt_c, false, fontsize).first;
                                cpuFullDrewFirst = true;
                            }
                            if (cpu_temp_c[0]) {
                                if (cpuFullDrewFirst)
                                    rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY,
                                        fontsize, (settings.separatorColor)).first;
                                const int tv = atoi(cpu_temp_c);
                                renderer->drawString(cpu_temp_c, false, rx, singleItemY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                            }
                        }
                    }
                    // Draw @freq on bottom row, right-aligned so its right edge equals brackets right edge
                    if (CPU_freq_c[0]) {
                        const uint32_t freqW = renderer->getTextDimensions(CPU_freq_c, false, fontsize).first;
                        const int32_t freqX = bracketsRightX - (int32_t)freqW;
                        renderer->drawString(CPU_freq_c, false, (uint32_t)std::max((int32_t)0, freqX),
                            gridBotY, fontsize, textColorA);
                    }
                }

                // GPU split: volt/temp on top/bottom rows; swapped when voltageAtEndGPU
                if (gpuIsSplit && item.type == 1) {
                    // GPU split: center row = data label; top/bot rows = volt/temp (swapped by voltageAtEndGPU).
                    // Mini-style 3-stack scissoring: center div scissored to center band,
                    // top/bot row divs each scissored to their own band, drawn before their content.
                    const uint32_t voltColX = current_x;
                    const int32_t gpuVoltY = settings.voltageAtEndGPU ? gridBotY : gridTopY;
                    const int32_t gpuTempY = settings.voltageAtEndGPU ? gridTopY : gridBotY;
                    const bool gpuHasVolt = GPU_volt_c[0] != 0;
                    const bool gpuHasTemp = gpu_temp_c[0] != 0;
                    // Scissor metrics
                    int gpuSplitCOff = 0, gpuSplitLP = 0, gpuSplitFW = 0;
                    const bool gpuSplitScOK = divider_scissor::getDividerScissorMetrics(
                        fontsize, gpuSplitCOff, gpuSplitLP, gpuSplitFW);
                    const int gpuSplitCutTC = gpuSplitScOK
                        ? divider_scissor::computeDividerCutY((int)gridTopY, (int)singleItemY, gpuSplitCOff) : 0;
                    const int gpuSplitCutCB = gpuSplitScOK
                        ? divider_scissor::computeDividerCutY((int)singleItemY, (int)gridBotY, gpuSplitCOff) : 0;
                    const bool gpuVoltIsOnTop = !settings.voltageAtEndGPU;
                    const bool gpuHasTopRow = gpuVoltIsOnTop ? gpuHasVolt : gpuHasTemp;
                    const bool gpuHasBotRow = gpuVoltIsOnTop ? gpuHasTemp : gpuHasVolt;
                    auto gpuDrawCenterDiv = [&](uint32_t dx) {
                        if (gpuSplitScOK && gpuHasTopRow && gpuHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                gpuSplitCutTC, gpuSplitCutCB, fontsize, (settings.separatorColor), gpuSplitLP, gpuSplitFW);
                        } else if (gpuSplitScOK && gpuHasTopRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                gpuSplitCutTC, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), gpuSplitLP, gpuSplitFW);
                        } else if (gpuSplitScOK && gpuHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                divider_scissor::kNoUpperCut, gpuSplitCutCB, fontsize, (settings.separatorColor), gpuSplitLP, gpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)singleItemY, fontsize, (settings.separatorColor));
                        }
                    };
                    auto gpuDrawTopRowDiv = [&](uint32_t dx) {
                        if (gpuSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridTopY,
                                divider_scissor::kNoUpperCut, gpuSplitCutTC, fontsize, (settings.separatorColor), gpuSplitLP, gpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)gpuVoltY, fontsize, (settings.separatorColor));
                        }
                    };
                    auto gpuDrawBotRowDiv = [&](uint32_t dx) {
                        if (gpuSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridBotY,
                                gpuSplitCutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), gpuSplitLP, gpuSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)gpuTempY, fontsize, (settings.separatorColor));
                        }
                    };
                    const uint32_t divW = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    gpuDrawCenterDiv(voltColX);
                    if (gpuHasTopRow) {
                        gpuDrawTopRowDiv(voltColX);
                        if (gpuVoltIsOnTop && gpuHasVolt)
                            renderer->drawStringWithColoredSections(GPU_volt_c, false, specialChars,
                                voltColX + divW, gpuVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (!gpuVoltIsOnTop && gpuHasTemp) {
                            const int tv = atoi(gpu_temp_c);
                            renderer->drawString(gpu_temp_c, false, voltColX + divW, gpuTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                    if (gpuHasBotRow) {
                        gpuDrawBotRowDiv(voltColX);
                        if (!gpuVoltIsOnTop && gpuHasVolt)
                            renderer->drawStringWithColoredSections(GPU_volt_c, false, specialChars,
                                voltColX + divW, gpuVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (gpuVoltIsOnTop && gpuHasTemp) {
                            const int tv = atoi(gpu_temp_c);
                            renderer->drawString(gpu_temp_c, false, voltColX + divW, gpuTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                }

                // RAM temp split (single volt rail): volt/temp on top/bottom; swapped when voltageAtEndRAM
                if (ramTempSplit && item.type == 2) {
                    // RAM temp split (single volt rail): center row = data label; top/bot = volt/temp.
                    // Mini-style 3-stack scissoring: center div scissored to center band,
                    // top/bot row divs each scissored to their own band, drawn before their content.
                    const uint32_t voltColX = current_x;
                    const int32_t rtsVoltY = settings.voltageAtEndRAM ? gridBotY : gridTopY;
                    const int32_t rtsTempY = settings.voltageAtEndRAM ? gridTopY : gridBotY;
                    const char* topVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                    const bool rtsHasVolt = topVolt[0] != 0;
                    const bool rtsHasTemp = ram_temp_c[0] != 0;
                    // Scissor metrics
                    int rtsSplitCOff = 0, rtsSplitLP = 0, rtsSplitFW = 0;
                    const bool rtsSplitScOK = divider_scissor::getDividerScissorMetrics(
                        fontsize, rtsSplitCOff, rtsSplitLP, rtsSplitFW);
                    const int rtsSplitCutTC = rtsSplitScOK
                        ? divider_scissor::computeDividerCutY((int)gridTopY, (int)singleItemY, rtsSplitCOff) : 0;
                    const int rtsSplitCutCB = rtsSplitScOK
                        ? divider_scissor::computeDividerCutY((int)singleItemY, (int)gridBotY, rtsSplitCOff) : 0;
                    const bool rtsVoltIsOnTop = !settings.voltageAtEndRAM;
                    const bool rtsHasTopRow = rtsVoltIsOnTop ? rtsHasVolt : rtsHasTemp;
                    const bool rtsHasBotRow = rtsVoltIsOnTop ? rtsHasTemp : rtsHasVolt;
                    auto rtsDrawCenterDiv = [&](uint32_t dx) {
                        if (rtsSplitScOK && rtsHasTopRow && rtsHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                rtsSplitCutTC, rtsSplitCutCB, fontsize, (settings.separatorColor), rtsSplitLP, rtsSplitFW);
                        } else if (rtsSplitScOK && rtsHasTopRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                rtsSplitCutTC, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), rtsSplitLP, rtsSplitFW);
                        } else if (rtsSplitScOK && rtsHasBotRow) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)singleItemY,
                                divider_scissor::kNoUpperCut, rtsSplitCutCB, fontsize, (settings.separatorColor), rtsSplitLP, rtsSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)singleItemY, fontsize, (settings.separatorColor));
                        }
                    };
                    auto rtsDrawTopRowDiv = [&](uint32_t dx) {
                        if (rtsSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridTopY,
                                divider_scissor::kNoUpperCut, rtsSplitCutTC, fontsize, (settings.separatorColor), rtsSplitLP, rtsSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)rtsVoltY, fontsize, (settings.separatorColor));
                        }
                    };
                    auto rtsDrawBotRowDiv = [&](uint32_t dx) {
                        if (rtsSplitScOK) {
                            divider_scissor::drawDividerScissored(renderer, (int)dx, (int)gridBotY,
                                rtsSplitCutCB, divider_scissor::kNoLowerCut, fontsize, (settings.separatorColor), rtsSplitLP, rtsSplitFW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, (float)rtsTempY, fontsize, (settings.separatorColor));
                        }
                    };
                    const uint32_t divW = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    // When ramLoadIsSplit/ramBWIsSplit, drawDividerStack3 already drew all three rows
                    // scissored. Skip center, top, and bot individual draws to avoid doubling.
                    // When NOT load-split, draw them individually as before.
                    if (!ramLoadIsSplit && !ramBWIsSplit)
                        rtsDrawCenterDiv(voltColX);
                    if (rtsHasTopRow) {
                        if (!(ramLoadIsSplit || ramBWIsSplit))
                            rtsDrawTopRowDiv(voltColX);
                        if (rtsVoltIsOnTop && rtsHasVolt)
                            renderer->drawStringWithColoredSections(topVolt, false, specialChars,
                                voltColX + divW, rtsVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (!rtsVoltIsOnTop && rtsHasTemp) {
                            const int tv = atoi(ram_temp_c);
                            renderer->drawString(ram_temp_c, false, voltColX + divW, rtsTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                    if (rtsHasBotRow) {
                        if (!(ramLoadIsSplit || ramBWIsSplit))
                            rtsDrawBotRowDiv(voltColX);
                        if (!rtsVoltIsOnTop && rtsHasVolt)
                            renderer->drawStringWithColoredSections(topVolt, false, specialChars,
                                voltColX + divW, rtsVoltY, fontsize, textColorA, (settings.separatorColor));
                        else if (rtsVoltIsOnTop && rtsHasTemp) {
                            const int tv = atoi(ram_temp_c);
                            renderer->drawString(ram_temp_c, false, voltColX + divW, rtsTempY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        }
                    }
                }

                // SBS CPU temp: normal = DIV+temp after volt; voltAtEnd = sep+DIV+temp+volt (volt drawn last)
                if (!cpuIsSplit && !cpuFullIsSplit && item.type == 0 && settings.showCPUTemp && !settings.showStackedCPUTemp && cpu_temp_c[0]) {
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

                // BAT split: draw on top row, pct on bottom row (or swapped when invertBatteryDisplay)
                // current_x has already advanced past data_width; step back to the data column start.
                if (batIsSplit && item.type == 6) {
                    const char* botStr = settings.invertBatteryDisplay ? Battery_draw_c : Battery_pct_c;
                    const uint32_t botW = botStr[0] ? renderer->getTextDimensions(botStr, false, fontsize).first : 0;
                    const uint32_t dataStartX = current_x - item_layout.data_width;
                    const uint32_t botX = dataStartX + item_layout.data_width - botW;  // right-align
                    renderer->drawString(botStr, false, botX, gridBotY, fontsize, textColorA);
                }

                // DTC split: draw bottom half (everything after DIVIDER_SYMBOL in the user's
                // dtcFormat). Right-aligned to share the same right edge as the top half,
                // so the shorter line fits flush-right against the wider line.
                if (dtcIsSplit && item.type == 8 && item.data_ptr && item.data_ptr[0]) {
                    const std::string dtStr(item.data_ptr);
                    const size_t divPos = dtStr.find(ult::DIVIDER_SYMBOL);
                    if (divPos != std::string::npos) {
                        const std::string botHalf = dtStr.substr(divPos + ult::DIVIDER_SYMBOL.length());
                        if (!botHalf.empty()) {
                            const uint32_t botW = renderer->getTextDimensions(botHalf, false, fontsize).first;
                            const uint32_t dataStartX = current_x - item_layout.data_width;
                            const uint32_t botX = dataStartX + item_layout.data_width - botW;  // right-align
                            renderer->drawStringWithColoredSections(botHalf, false, specialChars, botX, gridBotY, fontsize, textColorA, (settings.separatorColor));
                        }
                    }
                }

                // RAM load split: draw total@freq on bottom row aligned with [cpu% gpu%].
                // Also draw a 3-tall divider stack at the volt column to visually connect
                // the 2-row load data block with whatever right-side content follows.
                // ramBWIsSplit shares this code path — the buffers were swapped at data-build
                // time so RAM_var_compressed_c holds the top (BW) and RAM_load_bot_c holds
                // the bottom (regular RAM line). Volt/temp right-side content remains valid.
                if ((ramLoadIsSplit || ramBWIsSplit) && item.type == 2) {
                    if (RAM_load_bot_c[0]) {
                        const uint32_t botW_draw = renderer->getTextDimensions(RAM_load_bot_c, false, fontsize).first;
                        const uint32_t botDrawX  = dataColStartX + item_layout.data_width - botW_draw;
                        // Bot row may contain '#' sentinels (e.g. "[#4% #8%] 12%@1600.0" when
                        // ramBWIsSplit=true: BW on top, load-SBS on bottom). Use cpuPadChars so
                        // the '#' padding renders transparent rather than as a literal '#' character.
                        drawBracketAware(std::string(RAM_load_bot_c), botDrawX, gridBotY);
                    }
                    // rLoadHasRight: is there any right-side content that needs the 3-tall
                    // divider stack (top+bot) drawn at the volt-column X?
                    // Guard has_voltage with volt_ptr[0] so an empty RAM_volt_c (realVolts=true
                    // but no VDD2/VDDQ configured) doesn't produce phantom dividers.
                    const bool rLoadHasRight = (item.has_voltage && item.volt_ptr && item.volt_ptr[0])
                                             || ramIsSplit || ramTempSplit
                                             || (!ramIsSplit && !ramTempSplit && settings.showRAMTemp && ram_temp_c[0]);
                    if (rLoadHasRight) {
                        const uint32_t rLoadVoltColX = dataColStartX + item_layout.data_width;
                        // Draw all three rows of the triple-stack divider at once using
                        // drawDividerStack3 so every row is scissored to its own band and
                        // no alpha-blended edges bleed into adjacent rows (mirrors Mini).
                        divider_scissor::drawDividerStack3(renderer,
                            (int)rLoadVoltColX,
                            (int)gridTopY, (int)singleItemY, (int)gridBotY,
                            fontsize, (settings.separatorColor));
                    }
                }

                // SBS GPU temp: normal = DIV+temp after volt; voltAtEnd = sep+DIV+temp+volt
                if (!gpuIsSplit && item.type == 1 && settings.showGPUTemp && !settings.showStackedGPUTemp && gpu_temp_c[0]) {
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
                        // Leading DIV is the center row of the triple-stack. When ramLoadIsSplit/ramBWIsSplit
                        // that center was already drawn scissored by drawDividerStack3; skip drawing it again,
                        // but still advance rx so temp and volt land at the correct positions.
                        uint32_t rx = current_x;
                        if (!(ramLoadIsSplit || ramBWIsSplit))
                            rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        else
                            rx += renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        renderer->drawString(ram_temp_c, false, rx, singleItemY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColorA);
                        rx += renderer->getTextDimensions(ram_temp_c, false, fontsize).first;
                        rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        renderer->drawStringWithColoredSections(RAM_volt_c, false, specialChars, rx, singleItemY, fontsize, textColorA, (settings.separatorColor));
                    } else {
                        uint32_t rx = current_x;
                        if (!(ramLoadIsSplit || ramBWIsSplit))
                            rx += renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, singleItemY, fontsize, (settings.separatorColor)).first;
                        else
                            rx += renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
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
                const double load = (1.0 - (idletick * inv_freq)) * 100.0;
                snprintf(buf, size, "%.0f%%", std::max(0.0, std::min(100.0, load)));
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
            // When showFullCPUMaxCore012 is on, brackets condense cores 0-2 into one max value.
            // Build a small per-frame buffer for that max so the snprintf format strings below
            // can use the same %s slots regardless of mode.
            char CPU_UsageMax012[32] = "";
            if (settings.showFullCPUMaxCore012) {
                const double _m0 = strtod(CPU_Usage0, nullptr);
                const double _m1 = strtod(CPU_Usage1, nullptr);
                const double _m2 = strtod(CPU_Usage2, nullptr);
                const double _maxCore012 = std::max({_m0, _m1, _m2});
                snprintf(CPU_UsageMax012, sizeof(CPU_UsageMax012), "%.0f%%", _maxCore012);
            }
            // Pad single-digit core % with a leading space so bracket contents
            // stay fixed-width: "1%" -> " 1%", "24%" unchanged, "100%" unchanged.
            // strlen("X%") == 2 means single digit — write " X%" into a local buffer.
            auto mkPad = [](const char* s, char (&buf)[4]) -> const char* {
                if (s[0] != '\0' && s[1] == '%' && s[2] == '\0') {
                    buf[0] = kPadSentinelMicro; buf[1] = s[0]; buf[2] = '%'; buf[3] = '\0';
                    return buf;
                }
                return s;
            };
            char _pb0[4], _pb1[4], _pb2[4], _pb3[4], _pbM[4];
            const char* p0 = mkPad(CPU_Usage0,       _pb0);
            const char* p1 = mkPad(CPU_Usage1,       _pb1);
            const char* p2 = mkPad(CPU_Usage2,       _pb2);
            const char* p3 = mkPad(CPU_Usage3,       _pb3);
            const char* pM = mkPad(CPU_UsageMax012,  _pbM);
            if (!settings.showStackedFullCPU) {
                // Existing inline mode: [23%,29%,0%,32%]@1000.0
                if (settings.showFullCPUMaxCore012) {
                    // Condensed: [max(c0,c1,c2) c3]@freq
                    snprintf(CPU_compressed_c, sizeof(CPU_compressed_c),
                        "[%s %s]%s%u.%u",
                        pM, p3,
                        cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
                } else {
                    snprintf(CPU_compressed_c, sizeof(CPU_compressed_c),
                        "[%s %s %s %s]%s%u.%u",
                        p0, p1, p2, p3,
                        cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
                }
                CPU_freq_c[0] = '\0';
            } else {
                // Split mode: brackets on top row, max%@freq on bottom row
                if (settings.showFullCPUMaxCore012) {
                    // Condensed: [max(c0,c1,c2) c3] on top
                    snprintf(CPU_compressed_c, sizeof(CPU_compressed_c),
                        "[%s %s]",
                        pM, p3);
                } else {
                    snprintf(CPU_compressed_c, sizeof(CPU_compressed_c),
                        "[%s %s %s %s]",
                        p0, p1, p2, p3);
                }
                const double _u0 = strtod(CPU_Usage0, nullptr);
                const double _u1 = strtod(CPU_Usage1, nullptr);
                const double _u2 = strtod(CPU_Usage2, nullptr);
                const double _u3 = strtod(CPU_Usage3, nullptr);
                const double _maxU = std::max({_u0, _u1, _u2, _u3});
                snprintf(CPU_freq_c, sizeof(CPU_freq_c),
                    "%.0f%%%s%u.%u",
                    _maxU, cpuDiff, cpuFreq / 1000000, (cpuFreq / 100000) % 10);
            }
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
            CPU_freq_c[0] = '\0';
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
            // User wants percentage display. ramLoad[All] is populated regardless of sysmodule:
            //   - With sys-clk/hoc-clk IPC: filled from sysclkCTX/hocclkCTX in Misc thread.
            //   - Without any sysmodule: back-filled from ACTMON MC_ALL in Misc thread
            //     (same EMC bus-utilisation metric, computed identically to sys-clk's t210.c).
            snprintf(MICRO_RAM_all_c, sizeof(MICRO_RAM_all_c), "%hu%%",
                     (ramLoad[SysClkRamLoad_All] + 5) / 10);
        }

        const char* ramDiff = "@";
        if (realRAM_Hz) {
            const int32_t deltaRAM = (int32_t)(realRAM_Hz / 1000) - (RAM_Hz / 1000);
            ramDiff = getDifferenceSymbol(deltaRAM);
        }
        
        const uint32_t ramFreq = settings.realFrequencies && realRAM_Hz ? realRAM_Hz : RAM_Hz;
        const unsigned ramMHz   = ramFreq / 1000000;
        const unsigned ramMHz10 = (ramFreq / 100000) % 10;

        RAM_load_bot_c[0] = '\0';
        RAM_bw_c[0] = '\0';
        // Build RAM bandwidth string ("X.X GB/s") when enabled.
        // Always written — "0.00 GB/s" placeholder when ACTMON hasn't delivered data yet
        // so the stacked layout is stable from frame 1 and never jumps.
        //   < 1 GB/s  →  "0.XY GB/s"  (2 decimal places, e.g. "0.54 GB/s")
        //   1–9.99    →  "X.XX GB/s"  (2 decimal places, e.g. "2.54 GB/s")
        //  10+        →  "XX.X GB/s"  (1 decimal place,  e.g. "22.4 GB/s")
        // Max-width case "44.4 GB/s" is unchanged so layout metrics remain valid.
        if (settings.showRAMBandwidth) {
            if (ramBW_MBs > 0) {
                if (ramBW_MBs >= 10000) {
                    // >= 10 GB/s: 1 decimal place
                    const unsigned bwInt = ramBW_MBs / 1000;
                    const unsigned bwDec = (ramBW_MBs % 1000) / 100;
                    snprintf(RAM_bw_c, sizeof(RAM_bw_c), "%u.%u GB/s", bwInt, bwDec);
                } else if (ramBW_MBs >= 1000) {
                    // 1–9.99 GB/s: 2 decimal places
                    const unsigned bwInt = ramBW_MBs / 1000;
                    const unsigned bwDec = (ramBW_MBs % 1000) / 10;
                    snprintf(RAM_bw_c, sizeof(RAM_bw_c), "%u.%02u GB/s", bwInt, bwDec);
                } else {
                    // < 1 GB/s: 2 decimal places, leading "0."
                    const unsigned bwDec = ramBW_MBs / 10;
                    snprintf(RAM_bw_c, sizeof(RAM_bw_c), "0.%02u GB/s", bwDec);
                }
            } else {
                snprintf(RAM_bw_c, sizeof(RAM_bw_c), "0.00 GB/s");
            }
        }
        if (settings.showRAMLoad && settings.showRAMLoadCPUGPU) {
            // ramLoad[All] and ramLoad[Cpu] are populated from sys-clk IPC when available,
            // otherwise from ACTMON MC_ALL + MC_CPU back-fill (see Utils.hpp Misc thread).
            // GPU portion is the derived remainder (All - Cpu), matching sys-clk's internal model.
            const unsigned cpuL = (ramLoad[SysClkRamLoad_Cpu] + 5) / 10;
            const int gpuRaw = ramLoad[SysClkRamLoad_All] - ramLoad[SysClkRamLoad_Cpu];
            const unsigned gpuL = (unsigned)(gpuRaw > 0 ? (gpuRaw + 5) / 10 : 0);
            const unsigned totL = (ramLoad[SysClkRamLoad_All] + 5) / 10;
            // Pad single-digit RAM load values with '#' sentinel (same as CPU mkPad).
            // '#' is drawn transparent at render time so the bracket width stays fixed.
            auto mkPadR = [](unsigned v, char (&buf)[4]) -> const char* {
                if (v < 10) {
                    buf[0] = kPadSentinelMicro; buf[1] = '0' + (char)v; buf[2] = '%'; buf[3] = '\0';
                    return buf;
                }
                // Two-or-three digit: format normally
                snprintf(buf, sizeof(buf), "%u%%", v);
                return buf;
            };
            char _rb0[4], _rb1[4];
            const char* rp0 = mkPadR(cpuL, _rb0);
            const char* rp1 = mkPadR(gpuL, _rb1);
            if (!settings.showStackedRAMLoadCPUGPU) {
                // SBS: [cpu% gpu%]total%@freq on one line
                snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c),
                    "[%s %s] %u%%%s%u.%u", rp0, rp1, totL, ramDiff, ramMHz, ramMHz10);
            } else {
                // Split: brackets top, total@freq bottom
                snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c),
                    "[%s %s]", rp0, rp1);
                snprintf(RAM_load_bot_c, sizeof(RAM_load_bot_c),
                    "%u%%%s%u.%u", totL, ramDiff, ramMHz, ramMHz10);
            }
        } else {
            snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c),
                "%s%s%u.%u", MICRO_RAM_all_c, ramDiff, ramMHz, ramMHz10);
        }

        // Recompute split flags here in update() so the buffer swap below always uses
        // current values from the very first frame — independent of when prepareRenderItems()
        // (draw-callback) has last run.  These are cheap boolean expressions derived
        // solely from settings; no render-item list required.
        {
            const bool _hasRam = (settings.show.find("RAM") != std::string::npos);
            ramLoadIsSplit = _hasRam && settings.showRAMLoad && settings.showRAMLoadCPUGPU &&
                             settings.showStackedRAMLoadCPUGPU;
            ramBWIsSplit   = _hasRam && settings.showRAMBandwidth && settings.showStackedRAMBandwidth &&
                             !ramLoadIsSplit;
        }

        // RAM bandwidth integration (mirrors Mini.hpp). Three layouts:
        //   1) Inline BW (BW on, stacked BW off): prepend "X.XGB/s" + DIVIDER_SYMBOL to
        //      RAM_var_compressed_c. Works in all sub-modes incl. stacked-load top row.
        //   2) Stacked BW + stacked load (both stacks on): BW prepended inline to the [c% g%]
        //      top row (ramBWIsSplit=false since ramLoadIsSplit owns both rows); RAM_load_bot_c
        //      already holds total%@freq from the load-split data build.
        //   3) Stacked BW + NO load-split active (incl. VDDQ/temp split cases): BW goes on its
        //      own top row, regular RAM line on bottom. VDDQ/temp splits are right-side decorations
        //      and do NOT block ramBWIsSplit. We SWAP content into the existing split-row buffers
        //      and treat ramBWIsSplit identically to ramLoadIsSplit in the renderer.
        //   4) BW inline (NOT stacked) + load split active: BW draws as a separate LEFT column
        //      before the stacked-load block. RAM_bw_c is left populated for the renderer.
        if (RAM_bw_c[0]) {
            // bwLeftOfLoadSplit: BW not stacked, load IS stacked → BW draws as left column.
            const bool bwLeftOfLoadSplit = !settings.showStackedRAMBandwidth && ramLoadIsSplit;
            if (ramBWIsSplit) {
                // Case (3): BW stacked, no other split active — swap buffers.
                // RAM_var_compressed_c → RAM_load_bot_c (bottom), BW → RAM_var_compressed_c (top).
                snprintf(RAM_load_bot_c, sizeof(RAM_load_bot_c), "%s", RAM_var_compressed_c);
                snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c), "%s", RAM_bw_c);
                RAM_bw_c[0] = '\0';
            } else if (bwLeftOfLoadSplit) {
                // Case (4): BW inline + load stacked. Leave RAM_bw_c populated for the renderer.
                // The renderer draws BW at the left of the data column, then the 3-stack divider,
                // then the stacked-load content right-aligned to the right of the divider.
            } else {
                // Cases (1) and (2): BW inline (no load split), or BW stacked + load stacked
                // (prepend BW\xEE\x80\xB1 to the top row [c% g%]).
                char tmp[sizeof(RAM_var_compressed_c)];
                snprintf(tmp, sizeof(tmp), "%s%s",
                         RAM_bw_c, RAM_var_compressed_c);
                memcpy(RAM_var_compressed_c, tmp, sizeof(RAM_var_compressed_c));
                RAM_bw_c[0] = '\0';
            }
        }
            
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

        {
            const float charge = (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f;
            snprintf(Battery_draw_c, sizeof(Battery_draw_c), "%.2f W", drawW);
            snprintf(Battery_pct_c,  sizeof(Battery_pct_c),  "%.1f%% [%s]", charge, remainingBatteryLife);
            if (settings.showStackedBAT) {
                if (!settings.invertBatteryDisplay)
                    snprintf(Battery_c, sizeof Battery_c, "%.2f W", drawW);
                else
                    snprintf(Battery_c, sizeof Battery_c, "%.1f%% [%s]", charge, remainingBatteryLife);
            } else {
                if (!settings.invertBatteryDisplay)
                    snprintf(Battery_c, sizeof Battery_c, "%.2f W%.1f%% [%s]", drawW, charge, remainingBatteryLife);
                else
                    snprintf(Battery_c, sizeof Battery_c, "%.1f%% [%s]%.2f W", charge, remainingBatteryLife, drawW);
            }
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
        //  - Split         (!showStackedFanSOC): top=temps+fan, bot=temps only
        //  - Side-By-Side  (both on, SBS on):       compTemps +  + SOC/PCB/Skin+fan
        //  - Grid          (both on, SBS off):       SOC/PCB/Skin+fan  (compTemps read from member)
        //  - Comp-only     (comp on, soc off):       CPU/GPU/RAM+fan
        //  - Soc-only      (default):                SOC/PCB/Skin+fan
        const bool splitFanSOC = settings.showStackedFanSOC &&
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
        } else if (settings.showComponentTemps && settings.showSocPcbSkinTemps && !settings.showStackedTemps) {
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
        if (settings.useIntegerFPS)
            snprintf(FPS_var_compressed_c, sizeof FPS_var_compressed_c, "%d", (int)round(useOldFPSavg ? FPSavg_old : FPSavg));
        else
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
        // ── Plus-hold focus/reposition mode ───────────────────────────────────
        // The poll thread sets plusFocusActive after a 1-second Plus hold and
        // stops the frame limiter. Here we:
        //   1. Re-enable the frame limiter the frame after focus mode ends.
        //   2. While active, map left-joystick full-up/down to position flips
        //      (same logic as swipeFlipPending: toggle setPosBottom, move layer,
        //       force re-init, then wait for one drawn frame before re-enabling).
        {
            const bool focusNow = plusFocusActive.load(std::memory_order_acquire);

            if (focusNow) {
                // Frame limiter is already stopped by the poll thread.
                // Check left joystick for top/bottom snap.
                static constexpr int JOYSTICK_SNAP_THRESHOLD = 28000; // ~85 % of 32767
                static bool joystickFlipArmed = true; // re-arm once stick returns to center

                const int jy = joyStickPosLeft.y;
                const bool stickAtBottom = (jy <= -JOYSTICK_SNAP_THRESHOLD);
                const bool stickAtTop    = (jy >=  JOYSTICK_SNAP_THRESHOLD);
                const bool stickNeutral  = (abs(jy) < JOYSTICK_SNAP_THRESHOLD / 2);

                if (stickNeutral) {
                    joystickFlipArmed = true; // stick returned to center; allow next flip
                }

                if (joystickFlipArmed && (stickAtBottom || stickAtTop)) {
                    // Only flip if the position would actually change.
                    const bool wantBottom = stickAtBottom;
                    if (wantBottom != settings.setPosBottom) {
                        settings.setPosBottom = wantBottom;
                        ult::setIniFileValue(configIniPath, "micro", "layer_height_align",
                                             settings.setPosBottom ? "bottom" : "top");

                        if (ult::limitedMemory) {
                            if (settings.setPosBottom) {
                                const auto [hUScan, vUScan] = tsl::gfx::getUnderscanPixels();
                                const float bottomVI = 1080.0f - static_cast<float>(tsl::cfg::FramebufferHeight) * 1.5f;
                                tsl::gfx::Renderer::get().setLayerPos(0u,
                                    static_cast<uint32_t>(std::max(0.0f, bottomVI - static_cast<float>(vUScan))));
                            } else {
                                tsl::gfx::Renderer::get().setLayerPos(0u, 0u);
                            }
                        }

                        Initialized       = false;
                        layout.calculated = false;
                        renderDataDirty   = true;
                        triggerMoveFeedback();
                    }
                    joystickFlipArmed = false; // require stick to return to center before next flip
                }
            }

            // Edge: focus mode just deactivated → re-enable frame limiter.
            if (!focusNow && plusFocusWasActive) {
                isRendering = true;
                leventClear(&renderingStopEvent);
            }
            plusFocusWasActive = focusNow;
        }

        // ── Swipe-to-flip: re-enable rendering after position transition ──────
        // Poll thread stopped the frame limiter when the swipe fired. Once the
        // flip is applied and one frame is drawn with the new position, re-enable.
        if (swipeClearOnRelease && !isRendering) {
            swipeClearOnRelease = false;
            isRendering = true;
            leventClear(&renderingStopEvent);
        }

        // ── Swipe-to-flip: apply position change ──────────────────────────────
        // Poll thread sets swipeFlipPending; consumed here (main thread) so all
        // settings/render state mutations are safe and single-threaded.
        if (swipeFlipPending.exchange(false, std::memory_order_acq_rel)) {
            settings.setPosBottom = !settings.setPosBottom;
            ult::setIniFileValue(configIniPath, "micro", "layer_height_align",
                                 settings.setPosBottom ? "bottom" : "top");

            // limitedMemory: VI layer must move to the correct screen half.
            if (ult::limitedMemory) {
                if (settings.setPosBottom) {
                    const auto [hUScan, vUScan] = tsl::gfx::getUnderscanPixels();
                    const float bottomVI = 1080.0f - static_cast<float>(tsl::cfg::FramebufferHeight) * 1.5f;
                    tsl::gfx::Renderer::get().setLayerPos(0u,
                        static_cast<uint32_t>(std::max(0.0f, bottomVI - static_cast<float>(vUScan))));
                } else {
                    tsl::gfx::Renderer::get().setLayerPos(0u, 0u);
                }
            }

            // Force full re-init: base_y, barY, gridTopY/singleItemY/gridBotY all
            // depend on setPosBottom and are recomputed on the next draw frame.
            Initialized       = false;
            layout.calculated = false;
            renderDataDirty   = true;
            swipeClearOnRelease = true;
        }
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