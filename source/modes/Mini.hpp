class MainMenu;

namespace mini_divider_scissor {
    // ─────────────────────────────────────────────────────────────────────────
    // 3-DIVIDER STACK SCISSORING HELPERS
    //
    // The DIVIDER_SYMBOL glyph has alpha-blended edges. When three are stacked
    // vertically at the same x with overlapping pixel rows, the overlap regions
    // double-blend and produce a visibly darker shade. These helpers draw each
    // divider with a tight scissor band so the three glyphs meet exactly at the
    // midpoint between their visual centers — no overlap, no shading artifact.
    //
    // Layout convention (libtesla: larger Y is lower on screen):
    //
    //   topY     ┊  (top divider — clipped above the upper cut)
    //            ┊
    //            ┊──── cut at midpoint between top and center glyph centers
    //            ┊
    //   centerY  ┊  (center divider — clipped between the two cuts)
    //            ┊
    //            ┊──── cut at midpoint between center and bot glyph centers
    //            ┊
    //   botY     ┊  (bot divider — clipped below the lower cut)
    //
    // The cut Y in screen space is:
    //   ((Yupper + Ylower) / 2) + bounds[1] + glyphHeight/2
    // which is the midpoint between the two glyphs' visual vertical centers.
    // (bounds[1] is negative — it's the y-offset from baseline to glyph top.)
    //
    // Scissor stack note (tesla.hpp): enableScissoring/disableScissoring are a
    // simple push/pop stack; only the top entry is consulted by the renderer.
    // Every enable MUST be paired with a disable on every path.
    // ─────────────────────────────────────────────────────────────────────────

    // Fetch the divider glyph's vertical-center offset from its baseline Y,
    // plus the horizontal scissor bounds needed to fully cover its pixels.
    // Returns false if the glyph could not be obtained (caller should fall
    // back to drawing without scissoring in that unlikely case).
    static inline bool getDividerScissorMetrics(uint32_t fontsize,
                                                 int& outCenterOffsetY,
                                                 int& outLeftPad,
                                                 int& outFullWidth) {
        // DIVIDER_SYMBOL is a single codepoint (U+E031) in the PUA. Get its
        // glyph metrics from the shared FontManager cache.
        const uint32_t cp = 0xE031;
        auto glyph = tsl::gfx::FontManager::getOrCreateGlyph(cp, false, fontsize);
        if (!glyph) return false;
        // bounds[1] is negative (above baseline); height is positive.
        // Visual vertical center of the rasterised glyph relative to baseline:
        outCenterOffsetY = glyph->bounds[1] + glyph->height / 2;
        // Horizontal extent of the rasterised pixels (relative to draw x):
        outLeftPad   = glyph->bounds[0];                 // may be negative
        outFullWidth = glyph->width;
        if (outFullWidth < 1) outFullWidth = 1;
        return true;
    }

    // Compute the screen-Y at which to cut between two adjacent dividers
    // (Yupper < Ylower in screen space). This is the midpoint between their
    // visual glyph centers. Pixels with screen-y < cut belong to the upper
    // divider; pixels with screen-y >= cut belong to the lower.
    static inline int computeDividerCutY(int Yupper, int Ylower, int centerOffsetY) {
        return ((Yupper + Ylower) / 2) + centerOffsetY;
    }

    // Sentinel "no cut on this side" values used by drawDividerScissored.
    // These are plain integers far outside any plausible screen Y, avoiding
    // any dependency on <climits>.
    static constexpr int kNoUpperCut = -1000000;
    static constexpr int kNoLowerCut =  1000000;

    // Draws one divider at (x, baselineY) clipped to screen-y rows
    // [cutAbove, cutBelow). Pass cutAbove == kNoUpperCut for "no upper cut"
    // and cutBelow == kNoLowerCut for "no lower cut". The horizontal scissor
    // extent is computed from the glyph's bitmap bounds so the glyph is
    // never accidentally clipped on its sides.
    static inline void drawDividerScissored(tsl::gfx::Renderer* renderer,
                                            int x, int baselineY,
                                            int cutAbove, int cutBelow,
                                            uint32_t fontsize,
                                            const tsl::Color& color,
                                            int leftPad, int fullWidth) {
        // Clamp cuts to framebuffer-valid u32 range. enableScissoring takes
        // (x, y, w, h) where the inclusive rect is [x, x+w) x [y, y+h).
        const int fbH = (int)tsl::cfg::FramebufferHeight;
        int yLo = (cutAbove <= kNoUpperCut) ? 0   : cutAbove;
        int yHi = (cutBelow >= kNoLowerCut) ? fbH : cutBelow;
        if (yLo < 0)   yLo = 0;
        if (yHi > fbH) yHi = fbH;
        if (yHi <= yLo) return; // nothing visible — skip draw entirely

        // Horizontal scissor: cover the full glyph bitmap with a tiny pad so
        // anti-aliased side pixels are never clipped.
        const int fbW = (int)tsl::cfg::FramebufferWidth;
        int xLo = x + leftPad - 1;
        int xHi = x + leftPad + fullWidth + 1;
        if (xLo < 0)   xLo = 0;
        if (xHi > fbW) xHi = fbW;
        if (xHi <= xLo) return;

        renderer->enableScissoring((u32)xLo, (u32)yLo,
                                   (u32)(xHi - xLo), (u32)(yHi - yLo));
        renderer->drawString(ult::DIVIDER_SYMBOL, false,
                             (float)x, (float)baselineY, fontsize, color);
        renderer->disableScissoring();
    }

    // High-level helper: draws three dividers at the same x at three distinct
    // baseline Y values (topY < centerY < botY in screen space). Each draw is
    // scissored to a clean non-overlapping band so the three glyphs meet at
    // the midpoint between their visual centers. No layout changes — the
    // baselines remain exactly where the caller put them.
    static inline void drawDividerStack3(tsl::gfx::Renderer* renderer,
                                         int x,
                                         int topY, int centerY, int botY,
                                         uint32_t fontsize,
                                         const tsl::Color& color) {
        int centerOff = 0, leftPad = 0, fullW = 0;
        if (!getDividerScissorMetrics(fontsize, centerOff, leftPad, fullW)) {
            // Fallback: draw without scissoring (preserves prior behaviour).
            renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)x, (float)topY,    fontsize, color);
            renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)x, (float)centerY, fontsize, color);
            renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)x, (float)botY,    fontsize, color);
            return;
        }
        const int cutTC = computeDividerCutY(topY,    centerY, centerOff);
        const int cutCB = computeDividerCutY(centerY, botY,    centerOff);
        drawDividerScissored(renderer, x, topY,    kNoUpperCut, cutTC,        fontsize, color, leftPad, fullW);
        drawDividerScissored(renderer, x, centerY, cutTC,       cutCB,        fontsize, color, leftPad, fullW);
        drawDividerScissored(renderer, x, botY,    cutCB,       kNoLowerCut,  fontsize, color, leftPad, fullW);
    }
} // namespace mini_divider_scissor

class MiniOverlay : public tsl::Gui {
private:
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_var_compressed_c[128] = "";
    char Battery_c[64] = "";
    char Battery_draw_c[16] = "";
    char Battery_pct_c[32] = "";
    char soc_temperature_c[64] = "";
    char skin_temperature_c[64] = "";
    char componentTemps_c[64] = "";  // dual-row TMP component die temps: "CPU°C GPU°C RAM°C"
    char MINI_SOC_volt_c[16] = "";   // SOC voltage string, e.g. "1234 mV"
    char MINI_CPU_volt_c[16] = "";   // CPU voltage string, e.g. "1100 mV"
    char MINI_CPU_freq_c[16] = "";   // freq part for full CPU split mode e.g. "@1000.0"
    char MINI_GPU_volt_c[20] = "";   // GPU voltage string, e.g. "900 mV"
    char vdd2_only_c[16] = "";        // split RAM mode: VDD2 string alone
    char vddq_only_c[16] = "";        // split RAM mode: VDDQ string alone
    char mini_cpu_temp_c[16] = "";    // CPU die temp string e.g. "52°C"
    char mini_gpu_temp_c[16] = "";    // GPU die temp string e.g. "48°C"
    char mini_ram_temp_c[16] = "";    // RAM die temp string e.g. "45°C"
    char MINI_RAM_load_bot_c[32] = ""; // RAM load split row 2: "total%@freq"
    char MINI_RAM_bw_c[16] = "";       // RAM bandwidth string e.g. "2.2GB/s" (ACTMON direct)

    uint32_t rectangleWidth;
    char Variables[512];
    size_t fontsize;
    MiniSettings settings;
    bool Initialized = false;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    int frameOffsetX, frameOffsetY;
    int topPadding, bottomPadding;
    int cachedBaselineOffset = 0;  // ascent pixels; used for currentY start
    int cachedDescentAbs     = 0;  // descent pixels below baseline; used for last-row glyph bottom
    bool isDragging = false;

    bool skipOnce = true;
    bool runOnce = true;

    size_t framePadding = 10;
    static constexpr int screenWidth = 1280;
    static constexpr int screenHeight = 720;

    struct ButtonState {
        std::atomic<bool> plusDragActive{false};
        std::atomic<bool> touchDragActive{false};
    } buttonState;

    Thread touchPollThread;
    std::atomic<bool> touchPollRunning{false};
    
    void updateLayerPos() {
        if (!ult::limitedMemory) return;
        const int pos = std::max(std::min(
            (int)(frameOffsetX * 1.5f + 0.5f) - tsl::impl::currentUnderscanPixels.first,
            1280 - 32 - tsl::impl::currentUnderscanPixels.first), 0);
        tsl::gfx::Renderer::get().setLayerPos(pos, 0);
        ult::layerEdge = frameOffsetX;  // touch-space (1280px), NOT VI-space (pos)
    }
public:
    MiniOverlay() { 
        tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        //tsl::initializeUltrahandSettings();
        PowerConsumption = 0.0f;
        batTimeEstimate = -1;
        strcpy(Battery_c, "-.-- W-.-% [--:--]"); // Default display

        GetConfigSettings(&settings);
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else fontsize = settings.dockedFontSize;

        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        //alphabackground = 0x0;
        frameOffsetX = settings.frameOffsetX;
        frameOffsetY = settings.frameOffsetY;
        framePadding = settings.framePadding;
        topPadding = 5;
        bottomPadding = 5;
        cachedBaselineOffset = (int)fontsize;  // updated to true ascent once font is loaded
        cachedDescentAbs     = 0;              // updated to true descent once font is loaded

        //if (ult::limitedMemory) {
        //    tsl::gfx::Renderer::get().setLayerPos(std::max(std::min((int)(frameOffsetX*1.5 + 0.5) - tsl::impl::currentUnderscanPixels.first, 1280-32 - tsl::impl::currentUnderscanPixels.first), 0), 0);
        //}
        updateLayerPos();
        
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        deactivateOriginalFooter = true;
        realVoltsPolling = settings.realVolts;
        StartThreads();

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




        // Start touch polling thread for instant response at low FPS
        touchPollRunning.store(true, std::memory_order_release);
        threadCreate(&touchPollThread, [](void* arg) -> void {
            MiniOverlay* overlay = static_cast<MiniOverlay*>(arg);
            
            // Allow only Player 1 and handheld mode
            const HidNpadIdType id_list[2] = { HidNpadIdType_No1, HidNpadIdType_Handheld };
            
            // Configure HID system to only listen to these IDs
            hidSetSupportedNpadIdType(id_list, 2);
            
            // Configure input for up to 2 supported controllers (P1 + Handheld)
            padConfigureInput(2, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
            
            // Initialize separate pad states for both controllers
            PadState pad_p1;
            PadState pad_handheld;
            padInitialize(&pad_p1, HidNpadIdType_No1);
            padInitialize(&pad_handheld, HidNpadIdType_Handheld);
            
            u64 plusHoldStart = 0;
            u64 touchHoldStart = 0;
            static constexpr u64 HOLD_THRESHOLD_NS = 500'000'000ULL;
        
            HidTouchScreenState state = {0};
            bool inputDetected;
            size_t actualEntryCount;
        
            while (overlay->touchPollRunning.load(std::memory_order_acquire)) {
                // Only poll when rendering and not dragging
                {
                    inputDetected = false;
                    const u64 now = armTicksToNs(armGetSystemTick());
                    
                    // Check touch in bounds
                    if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                        const int touchX = state.touches[0].x;
                        const int touchY = state.touches[0].y;
                        
                        // Calculate bounds (same logic as handleInput)
                        const uint32_t margin = (overlay->fontsize * 4);
                        const int overlayX = overlay->frameOffsetX;
                        const int overlayY = overlay->frameOffsetY;
                        const int overlayWidth = margin + overlay->rectangleWidth + (overlay->fontsize / 3);
                        
                        // Calculate height from Variables string
                        actualEntryCount = 1;
                        for (size_t i = 0; overlay->Variables[i] != '\0'; i++) {
                            if (overlay->Variables[i] == '\n') {
                                actualEntryCount++;
                            }
                        }
                        const int overlayHeight = ((overlay->fontsize + overlay->settings.spacing) * actualEntryCount) + 
                                                 overlay->settings.spacing * actualEntryCount + 
                                                 overlay->topPadding + overlay->bottomPadding;
                        
                        // Add touch padding
                        const int touchPadding = 4;
                        const int touchableX = overlayX - touchPadding;
                        const int touchableY = overlayY - touchPadding;
                        const int touchableWidth = overlayWidth + (touchPadding * 2);
                        const int touchableHeight = overlayHeight + (touchPadding * 2);
                        
                        // Check if touch is within bounds — 500ms hold required
                        if (touchX >= touchableX && touchX <= touchableX + touchableWidth &&
                            touchY >= touchableY && touchY <= touchableY + touchableHeight) {
                            if (overlay->buttonState.touchDragActive.load(std::memory_order_acquire)) {
                                // Drag already confirmed — keep input signalled
                                inputDetected = true;
                            } else {
                                if (touchHoldStart == 0) touchHoldStart = now;
                                if (now - touchHoldStart >= HOLD_THRESHOLD_NS) {
                                    inputDetected = true;
                                    overlay->buttonState.touchDragActive.exchange(true, std::memory_order_acq_rel);
                                }
                            }
                        } else {
                            // Touch out of overlay bounds — reset hold timer
                            touchHoldStart = 0;
                            overlay->buttonState.touchDragActive.exchange(false, std::memory_order_acq_rel);
                        }
                    } else {
                        // No touch detected — reset hold timer and drag-ready flag
                        touchHoldStart = 0;
                        overlay->buttonState.touchDragActive.exchange(false, std::memory_order_acq_rel);
                    }
                    
                    // Poll buttons from both controllers
                    padUpdate(&pad_p1);
                    padUpdate(&pad_handheld);
                    //const u64 keysHeld_p1 = padGetButtons(&pad_p1);
                    //const u64 keysHeld_handheld = padGetButtons(&pad_handheld);
                    
                    // Combine input from both controllers
                    const u64 keysHeld = padGetButtons(&pad_p1) | padGetButtons(&pad_handheld);
                    
                    // Track PLUS hold duration
                    if ((keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK)) {
                        if (plusHoldStart == 0) {
                            plusHoldStart = now;
                        }
                        if (now - plusHoldStart >= HOLD_THRESHOLD_NS) {
                            // Long enough to start drag
                            inputDetected = true;
                            overlay->buttonState.plusDragActive.exchange(true, std::memory_order_acq_rel);
                        }
                    }

                    else {
                        plusHoldStart = 0;
                        overlay->buttonState.plusDragActive.exchange(false, std::memory_order_acq_rel);
                    }
                    
                    // Disable rendering on any input, re-enable when no input
                    static bool resetOnce = true;
                    if (inputDetected) {
                        if (resetOnce && isRendering) {
                            isRendering = false;
                            leventSignal(&renderingStopEvent);
                            resetOnce = false;
                        }
                    } else {
                        resetOnce = true;
                    }
                }

                if (ult::limitedMemory) {
                    static auto lastUnderscanPixels = std::make_pair(0, 0);

                    if (lastUnderscanPixels != tsl::impl::currentUnderscanPixels) {
                        for (int i = 0; i < 2; i++) {
                            tsl::gfx::Renderer::get().updateLayerSize();
                            overlay->updateLayerPos();
                            //tsl::gfx::Renderer::get().setLayerPos(std::max(std::min((int)(overlay->frameOffsetX*1.5 + 0.5) - tsl::impl::currentUnderscanPixels.first, 1280-32 - tsl::impl::currentUnderscanPixels.first), 0), 0);
                        }
                    }
                    lastUnderscanPixels = tsl::impl::currentUnderscanPixels;
                }
                
                svcSleepThread(32000000ULL); // 32ms polling
            }
        }, this, NULL, 0x1000, 0x2B, -2);
        threadStart(&touchPollThread);
    }
    ~MiniOverlay() {
        // Stop touch polling thread
        touchPollRunning.store(false, std::memory_order_release);
        threadWaitForExit(&touchPollThread);
        threadClose(&touchPollThread);

        CloseThreads();
        FullMode = true;
        fixForeground = true;
        //tsl::hlp::requestForeground(true);
        //alphabackground = 0xD;
        deactivateOriginalFooter = false;
        if (ult::limitedMemory)
            ult::layerEdge = (ult::useRightAlignment && ult::correctFrameSize) ? (1280 - 448) : 0;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;
    bool resolutionShow = false;

    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            //static constexpr u16 frameWidth = 448;

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
            const bool contentChanged = (std::string(Variables) != lastVariables) || 
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
                        const bool cpuSplit     = settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                        const bool cpuFullSplit = settings.showFullCPU && settings.showStackedFullCPU;
                        // Common sub-widths used across multiple branches.
                        const uint32_t dw      = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const uint32_t volt_w  = settings.realVolts ? renderer->getTextDimensions("444 mV", false, fontsize).first : 0;
                        const uint32_t temp_w  = settings.showCPUTemp ? renderer->getTextDimensions("88°C", false, fontsize).first : 0;
                        if (cpuFullSplit) {
                            // Stacked full-CPU split: two rows (brackets top, freq bottom).
                            // Bracket worst-case depends on MaxCore012 mode (2 values vs 4).
                            const char* bracketWorst = settings.showFullCPUMaxCore012
                                ? "[100% 100%]" : "[100% 100% 44% 100%]";
                            const uint32_t bwRaw  = renderer->getTextDimensions(bracketWorst, false, fontsize).first;
                            const uint32_t freqWC = renderer->getTextDimensions("100%@4444.4", false, fontsize).first;
                            // Left column = max(brackets, freq): shared right edge for both rows.
                            const uint32_t bw = std::max(bwRaw, freqWC);
                            // cpuFullStacked: volt and temp each get their own row in the right column.
                            // Right-col width = dw + max(volt_w, temp_w) when either is present.
                            const bool cpuFullStacked = settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                            if (cpuFullStacked) {
                                // Right col holds whichever of volt/temp is wider (one per row).
                                const uint32_t rightColW = std::max(volt_w, temp_w);
                                width = bw + (rightColW > 0 ? dw + rightColW : 0);
                            } else {
                                // Inline: volt and temp both appear on the center row, each preceded by a divider.
                                width = bw;
                                if (settings.realVolts)  width += dw + volt_w;
                                if (settings.showCPUTemp) width += dw + temp_w;
                            }
                        } else if (!settings.realVolts) {
                            // No volt: inline [brackets]@freq or usage@freq.
                            if (settings.showFullCPU) {
                                // MaxCore012 condenses to 2 values; use the appropriate worst-case bracket.
                                const char* bw = settings.showFullCPUMaxCore012
                                    ? "[100% 100%]@4444.4" : "[100% 100% 44% 100%]@4444.4";
                                width = renderer->getTextDimensions(bw, false, fontsize).first;
                            } else {
                                width = renderer->getTextDimensions("100%@4444.4", false, fontsize).first;
                            }
                        } else {
                            // Volt present: inline [brackets]@freq|volt or usage@freq|volt.
                            // Build width from components so MaxCore012 uses the correct bracket proxy.
                            if (settings.showFullCPU) {
                                const char* bracketFreq = settings.showFullCPUMaxCore012
                                    ? "[100% 100%]@4444.4" : "[100% 100% 44% 100%]@4444.4";
                                width = renderer->getTextDimensions(bracketFreq, false, fontsize).first
                                      + dw + volt_w;
                            } else {
                                width = renderer->getTextDimensions("100%@4444.4", false, fontsize).first
                                      + dw + volt_w;
                            }
                        }
                        // SBS CPU temp (not stacked, not handled by cpuSplit/cpuFullSplit above):
                        // appended inline after the base width.  Layout differs by voltAtEnd:
                        //   Normal:   ...@freq | volt | temp   -> base already has volt; add dw+temp
                        //   VoltAtEnd: ...@freq | temp | volt  -> base already has volt; add dw+temp
                        // Either way the net addition is exactly dw+temp_w.
                        if (!cpuFullSplit && !cpuSplit && settings.showCPUTemp && !settings.showStackedCPUTemp) {
                            width += dw + temp_w;
                        }

                    } else if (key == "GPU" || (key == "RAM" && settings.showRAMLoad && (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck)))) {
                        // Component-based width: measure each piece once, sum by actual row topology.
                        const bool splitVDDQ_A  = key == "RAM" && settings.showStackedVDDQ &&
                                                  settings.realVolts && settings.showVDD2 &&
                                                  settings.showVDDQ && isMariko;
                        const bool stackedVDDQ_A = key == "RAM" && !settings.showStackedVDDQ &&
                                                   settings.realVolts && settings.showVDD2 &&
                                                   settings.showVDDQ && isMariko;

                        // --- Measure every component that can appear in any row ---
                        auto mw = [&](const std::string& s) -> uint32_t {
                            return renderer->getTextDimensions(s, false, fontsize).first;
                        };
                        const uint32_t dw = mw(ult::DIVIDER_SYMBOL);

                        // Data column (load line): depends on how the load is displayed.
                        // RAM bandwidth (when enabled) prepends "44.4GB/s" + DIVIDER inline, or
                        // sits on its own row (max-of-the-two width) when stacked BW is active.
                        const bool bw_active = settings.showRAMBandwidth;
                        // bw_left_load_stacked: BW NOT stacked (inline) + load IS stacked.
                        // BW draws as a separate LEFT column before the triple-stack divider + stacked-load block.
                        // Width = bw_w + dw + max(top_w, bot_w).
                        const bool bw_left_load_stacked = bw_active && !settings.showStackedRAMBandwidth &&
                                                          settings.showRAMLoadCPUGPU && settings.showStackedRAMLoadCPUGPU &&
                                                          (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck));
                        const bool bw_inline = bw_active && !settings.showStackedRAMBandwidth;
                        const uint32_t bw_w = bw_active ? mw("44.4 GB/s") : 0u;
                        const uint32_t data_w = [&]() -> uint32_t {
                            // Base content shape (without BW)
                            uint32_t base_data;
                            if (!settings.showRAMLoadCPUGPU)
                                base_data = mw("100%@4444.4");
                            else if (!settings.showStackedRAMLoadCPUGPU)
                                base_data = mw("[44% 44%] 100%@4444.4");
                            else
                                base_data = std::max(mw("[44% 44%]"), mw("100%@4444.4"));

                            if (!bw_active) return base_data;

                            // bw_stacked_load_stacked: BW IS stacked AND load IS stacked.
                            // bwOnOwnRow=false (load-split already owns both rows), so BW is prepended
                            // inline to the top row [cpu% gpu%] -> "BW<DIV>[cpu% gpu%]".
                            // Top row rendered string: "44.4 GB/s" + DIVIDER + "[44% 44%]" (worst-case).
                            // The renderer in RAM_SLOAD_TOP measures `currentLine` as one whole string via
                            // getTextDimensions(currentLine, ...). To guarantee the width calc matches the
                            // renderer's measurement exactly (including private-use DIVIDER advance),
                            // we measure the worst-case top row as ONE concatenated string here, and also
                            // sum the parts — taking the maximum to be defensive against any font/glyph
                            // edge case where one measurement style differs from the other.
                            // Bottom row is "100%@4444.4" (RAM total + freq).
                            const bool bw_stacked_load_stacked = settings.showStackedRAMBandwidth &&
                                                                  settings.showRAMLoadCPUGPU &&
                                                                  settings.showStackedRAMLoadCPUGPU &&
                                                                  (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck));
                            if (bw_stacked_load_stacked) {
                                //const uint32_t top_whole = mw(std::string("44.4 GB/s") + ult::DIVIDER_SYMBOL + "[100% 44%]");
                                //const uint32_t top_sum   = bw_w + dw + mw("[100% 44%]");

                                // THIS IS NOT PERFECT, BUT VISUALLY IT LOOKS ACCURATE SO IM KEEPING IT. SET 44% to 100% in the component
                                const uint32_t top_w     = bw_w + dw + mw("[100% 44%]"); 
                                return std::max(top_w, mw("100%@4444.4"));
                            }

                            // bw_left_load_stacked: BW NOT stacked (inline) + load IS stacked.
                            // BW draws as a separate LEFT column before the stacked-load block.
                            // Width = bw_w + dw + max(top_w, bot_w).  base_data is already max(top,bot).
                            // bw_inline (load not stacked): BW + DIVIDER prepended to the single row.
                            if (bw_inline || bw_left_load_stacked) {
                                return base_data + bw_w + dw;
                            }
                            // Stacked BW (BW on its own top row, RAM content on bottom row): max of the two.
                            return std::max(bw_w, base_data);
                        }();

                        // Volt/temp components: always use worst-case proxies, never live snapshots.
                        // The width calc runs once at init; a narrow live value snapshotted now (e.g.
                        // "1100 mV") would under-allocate when the value later widens (e.g. "1234 mV").
                        // Proxies match the max digit count of each format so they bound every live value.
                        const uint32_t vdd2_w  = (key == "RAM" && settings.realVolts && settings.showVDD2)
                            ? (settings.decimalVDD2 ? mw("4444.4 mV") : mw("4444 mV"))
                            : 0u;
                        const uint32_t vddq_w  = (key == "RAM" && settings.realVolts && settings.showVDDQ && isMariko)
                            ? mw("444 mV")
                            : 0u;
                        const uint32_t volt1_w = (key == "GPU" || (key == "RAM" && settings.realVolts
                                                                    && !settings.showVDD2 && !settings.showVDDQ))
                            ? mw("444 mV") : 0u;  // single-rail volt (non-Mariko or GPU)
                        const uint32_t temp_w  = (key == "RAM" && settings.showRAMTemp)
                            ? mw("88\xc2\xb0""C")
                            : 0u;
                        const uint32_t gpu_temp_w = (key == "GPU" && settings.showGPUTemp && !settings.showStackedGPUTemp)
                            ? mw("88\xc2\xb0""C") : 0u;

                        if (key == "GPU") {
                            // GPU row: data + (dw + volt1) + (dw + gpu_temp)
                            width = data_w
                                + (settings.realVolts ? dw + volt1_w : 0u)
                                + (gpu_temp_w ? dw + gpu_temp_w : 0u);
                        } else {
                            // RAM with load enabled.
                            // Determine which rows exist and their widths, then take the max.
                            if (!settings.realVolts) {
                                // No volts at all — just the data line
                                width = data_w;
                            } else if (splitVDDQ_A) {
                                // Stacked VDD2 / VDDQ: two rows share the data left-column.
                                // Layout depends on showStackedRAMTemp (temp-stacked) and voltageAtEndRAM.
                                //
                                // showStackedRAMTemp=false = temp at center row (next to volt)
                                // showStackedRAMTemp=true  = temp on its own top or bottom row
                                uint32_t top_row_w = 0, bot_row_w = 0, center_row_w = 0;

                                if (!settings.showRAMTemp || !temp_w) {
                                    // No temp: TOP=data+dw+vdd2, BOT=data+dw+vddq
                                    top_row_w    = data_w + dw + vdd2_w;
                                    bot_row_w    = data_w + dw + vddq_w;
                                } else if (!settings.showStackedRAMTemp) {
                                    // Temp at center row (SBS mode):
                                    if (settings.voltageAtEndRAM) {
                                        // CENTER: data + dw + temp + dw + vdd2 (fork after temp)
                                        // TOP:    (fork) + dw + vdd2  — same right edge as center
                                        // BOT:    (fork) + dw + vddq  — same right edge as vddq
                                        // Width governed by the CENTER row
                                        center_row_w = data_w + dw + temp_w + dw + std::max(vdd2_w, vddq_w);
                                    } else {
                                        // Normal: CENTER: data + dw + max(vdd2,vddq) + dw + temp
                                        center_row_w = data_w + dw + std::max(vdd2_w, vddq_w) + dw + temp_w;
                                    }
                                    top_row_w = data_w + dw + vdd2_w;
                                    bot_row_w = data_w + dw + vddq_w;
                                } else {
                                    // Temp on its own row (stacked):
                                    if (settings.voltageAtEndRAM) {
                                        // TOP: data + dw + temp + dw + vdd2
                                        // BOT: data + dw + vddq
                                        top_row_w = data_w + dw + temp_w + dw + vdd2_w;
                                        bot_row_w = data_w + dw + vddq_w;
                                    } else {
                                        // TOP: data + dw + vdd2
                                        // BOT: data + dw + vddq + dw + temp  (bot row starts at baseX)
                                        top_row_w = data_w + dw + vdd2_w;
                                        bot_row_w = data_w + dw + vddq_w + dw + temp_w;
                                    }
                                }
                                width = std::max({top_row_w, bot_row_w, center_row_w});

                            } else if (stackedVDDQ_A) {
                                // Non-stacked VDD2+VDDQ: both rails inline on the same row
                                // Layout: data + dw + vdd2 + dw + vddq (inline)
                                uint32_t base_row = data_w + dw + vdd2_w + dw + vddq_w;
                                width = base_row;
                                if (settings.showRAMTemp && temp_w) {
                                    const bool rts = settings.showStackedRAMTemp;
                                    if (!rts) width += dw + temp_w;
                                    // stacked: temp on separate row, base_row >= temp row
                                }
                            } else {
                                // Single rail (Mariko: only one of VDD2/VDDQ on) or non-Mariko.
                                // When BOTH VDD2 and VDDQ are off on Mariko, single_v=0 and no volt
                                // overhead is added (no divider, no volt slot).
                                const uint32_t single_v = isMariko
                                    ? (settings.showVDD2 ? vdd2_w : settings.showVDDQ ? vddq_w : 0u)
                                    : volt1_w;
                                uint32_t base_row = data_w + (single_v ? dw + single_v : 0u);
                                width = base_row;
                                if (settings.showRAMTemp && temp_w) {
                                    const bool rts = settings.showStackedRAMTemp && settings.realVolts &&
                                                     ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                                      (settings.showVDD2 && !settings.showVDDQ));
                                    if (!rts) width = base_row + dw + temp_w;
                                }
                            }
                        }
                    } else if (key == "RAM" && (!settings.showRAMLoad || (R_FAILED(sysclkCheck) && R_FAILED(hocclkCheck)))) {
                        // RAM without clock/load: component-based width.
                        auto mwR = [&](const std::string& s) -> uint32_t {
                            return renderer->getTextDimensions(s.c_str(), false, fontsize).first;
                        };
                        const uint32_t dw_R   = mwR(ult::DIVIDER_SYMBOL);
                        const uint32_t data_wR = mwR("100%@4444.4");  // RAM usage proxy
                        const uint32_t vdd2_wR = (settings.realVolts && settings.showVDD2 && isMariko)
                            ? (settings.decimalVDD2 ? mwR("4444.4 mV") : mwR("4444 mV"))
                            : 0u;
                        const uint32_t vddq_wR = (settings.realVolts && settings.showVDDQ && isMariko)
                            ? mwR("444 mV")
                            : 0u;
                        const uint32_t svolt_wR= (settings.realVolts && !isMariko)
                            ? (settings.decimalVDD2 ? mwR("4444.4 mV") : mwR("4444 mV")) : 0u;
                        const uint32_t temp_wR = settings.showRAMTemp
                            ? mwR("88\xc2\xb0""C")
                            : 0u;
                        const bool splitVDDQ_B  = settings.showStackedVDDQ && settings.realVolts &&
                                                  settings.showVDD2 && settings.showVDDQ && isMariko;
                        const bool stackedVDDQ_B = !settings.showStackedVDDQ && settings.realVolts &&
                                                   settings.showVDD2 && settings.showVDDQ && isMariko;

                        if (!settings.realVolts) {
                            width = data_wR + (temp_wR ? dw_R + temp_wR : 0u);
                        } else if (splitVDDQ_B) {
                            // Stacked VDD2/VDDQ: same topology as splitVDDQ_A but data_w = RAM usage proxy
                            uint32_t top_row_w = 0, bot_row_w = 0, center_row_w = 0;
                            if (!settings.showRAMTemp || !temp_wR) {
                                top_row_w = data_wR + dw_R + vdd2_wR;
                                bot_row_w = data_wR + dw_R + vddq_wR;
                            } else if (!settings.showStackedRAMTemp) {
                                if (settings.voltageAtEndRAM)
                                    center_row_w = data_wR + dw_R + temp_wR + dw_R + std::max(vdd2_wR, vddq_wR);
                                else
                                    center_row_w = data_wR + dw_R + std::max(vdd2_wR, vddq_wR) + dw_R + temp_wR;
                                top_row_w = data_wR + dw_R + vdd2_wR;
                                bot_row_w = data_wR + dw_R + vddq_wR;
                            } else {
                                if (settings.voltageAtEndRAM) {
                                    top_row_w = data_wR + dw_R + temp_wR + dw_R + vdd2_wR;
                                    bot_row_w = data_wR + dw_R + vddq_wR;
                                } else {
                                    top_row_w = data_wR + dw_R + vdd2_wR;
                                    bot_row_w = data_wR + dw_R + vddq_wR + dw_R + temp_wR;
                                }
                            }
                            width = std::max({top_row_w, bot_row_w, center_row_w});
                        } else if (stackedVDDQ_B) {
                            // Inline VDD2+VDDQ
                            width = data_wR + dw_R + vdd2_wR + dw_R + vddq_wR;
                            if (settings.showRAMTemp && temp_wR && !settings.showStackedRAMTemp)
                                width += dw_R + temp_wR;
                        } else {
                            // Single rail or non-Mariko
                            const uint32_t single_v = isMariko
                                ? (settings.showVDD2 ? vdd2_wR : settings.showVDDQ ? vddq_wR : 0u)
                                : svolt_wR;
                            width = data_wR + (single_v ? dw_R + single_v : 0u);
                            if (settings.showRAMTemp && temp_wR) {
                                const bool rts = settings.showStackedRAMTemp && settings.realVolts &&
                                                 ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                                  (settings.showVDD2 && !settings.showVDDQ));
                                if (!rts) width += dw_R + temp_wR;
                            }
                        }
                        // RAM bandwidth width adjustment (no-load branch):
                        // Inline BW prepends "44.4GB/s" + DIVIDER to the data row.
                        // Stacked BW puts BW on its own row → max(width, bw_w).
                        if (settings.showRAMBandwidth) {
                            const uint32_t bw_wR = mwR("44.4 GB/s");
                            if (settings.showStackedRAMBandwidth) {
                                if (bw_wR > width) width = bw_wR;
                            } else {
                                width += bw_wR + dw_R;
                            }
                        }
                    } else if (key == "SOC") {                // new block
                        if (!settings.realVolts)
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88°C (100%)", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88°C", false, fontsize).first;
                        else
                            if (settings.showSOCVoltage) {
                                if (settings.showFanPercentage)
                                    width = renderer->getTextDimensions("88°C 100%444 mV", false, fontsize).first;
                                else
                                    width = renderer->getTextDimensions("88°C444 mV", false, fontsize).first;
                            } else {
                                if (settings.showFanPercentage)
                                    width = renderer->getTextDimensions("88°C 100%", false, fontsize).first;
                                else
                                    width = renderer->getTextDimensions("88°C", false, fontsize).first;
                            }
                    } else if (key == "TMP") {
                        //dimensions = renderer->drawString("88.8\u00B0C88.8\u00B0C88.8\u00B0C (100%)", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (!settings.realVolts) {
                            if (settings.showFanPercentage)
                                width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C 100%", false, fontsize).first;
                            else
                                width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C", false, fontsize).first;
                        } else if (settings.showStackedFanSOC &&
                                   settings.showFanPercentage &&
                                   settings.showSOCVoltage) {
                            // Split mode: fan on row 1, SOC volt on row 2 — width = max of both rows
                            const uint32_t splitRow1 = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C 100%", false, fontsize).first;
                            const bool bothGroups = settings.showComponentTemps && settings.showSocPcbSkinTemps;
                            const uint32_t splitRow2 = bothGroups
                                ? renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C444 mV", false, fontsize).first
                                : renderer->getTextDimensions("444 mV", false, fontsize).first;
                            width = splitRow1 > splitRow2 ? splitRow1 : splitRow2;
                        } else {
                            if (settings.showSOCVoltage) {
                                if (settings.showFanPercentage)
                                    width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C 100%444 mV", false, fontsize).first;
                                else
                                    width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C444 mV", false, fontsize).first;
                            } else {
                                if (settings.showFanPercentage)
                                    width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C 100%", false, fontsize).first;
                                else
                                    width = renderer->getTextDimensions("88\u00B0C 88\u00B0C 88\u00B0C", false, fontsize).first;
                            }
                        }
                    } else if (key == "BAT") {
                        if (settings.showStackedBAT) {
                            const uint32_t draw_w = renderer->getTextDimensions("-14.44 W", false, fontsize).first;
                            const uint32_t pct_w  = renderer->getTextDimensions("100.0% [44:44]", false, fontsize).first;
                            width = std::max(draw_w, pct_w);
                        } else {
                            width = renderer->getTextDimensions("-14.44 W100.0% [44:44]", false, fontsize).first;
                        }
                    } else if (key == "FPS") {
                        //dimensions = renderer->drawString("144.4", false, 0, 0, fontsize, renderer->a(0x0000));
                        width = renderer->getTextDimensions("144.4", false, fontsize).first;
                    } else if (key == "RES") {
                        //dimensions = renderer->drawString("3840x21603840x2160", false, 0, 0, fontsize, renderer->a(0x0000));
                        if (settings.showFullResolution)
                            width = renderer->getTextDimensions("3840x21603840x2160", false, fontsize).first;
                        else
                            width = renderer->getTextDimensions("2160p2160p", false, fontsize).first;
                    } else if (key == "READ") {
                        // Calculate width for read speed display
                        width = renderer->getTextDimensions("999.99 MiB/s", false, fontsize).first;
                    } else if (key == "MEM") {
                        // Calculate width for system memory display (System + divider + memory value)
                        const u32 memValueWidth = renderer->getTextDimensions("999.9 MB "+ult::FREE, false, fontsize).first;
                        const u32 dividerWidth = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const u32 systemWidth = renderer->getTextDimensions("System", false, fontsize).first;
                        width = memValueWidth + dividerWidth + systemWidth;
                    } else if (key == "DTC" && settings.showDTC) {
                        // Calculate width based on the datetime format
                        // Use multiple sample dates to ensure longest possible textual output
                    
                        char sampleDateTime[64];
                        size_t maxWidth = 0;
                    
                        // Representative dates that produce long names in most locales
                        constexpr int testDays[][3] = {
                            {2025, 11, 26}, // Wednesday – longest weekday
                            {2025, 9, 30},  // September – long month
                            {2025, 12, 31}, // December – another long month
                            {2025, 2, 28},  // February – to cover locales where it's long
                        };
                    
                        struct tm t = {};
                        for (auto &d : testDays) {
                            
                            t.tm_year = d[0] - 1900;
                            t.tm_mon  = d[1] - 1;
                            t.tm_mday = d[2];
                            t.tm_hour = 23;
                            t.tm_min  = 59;
                            t.tm_sec  = 59;
                            mktime(&t); // normalize (sets weekday, etc.)
                    
                            strftime(sampleDateTime, sizeof(sampleDateTime), settings.dtcFormat.c_str(), &t);

                            if (settings.showStackedDTC) {
                                // When stacked, the line is split at DIVIDER_SYMBOL into 2 rows.
                                // Column width = wider of the two halves (the shorter one right-aligns
                                // to that edge). User formats may or may not contain DIVIDER_SYMBOL;
                                // if absent, fall back to measuring the whole string.
                                const std::string sampleStr(sampleDateTime);
                                const size_t divPos = sampleStr.find(ult::DIVIDER_SYMBOL);
                                size_t w;
                                if (divPos != std::string::npos) {
                                    const std::string topHalf = sampleStr.substr(0, divPos);
                                    const std::string botHalf = sampleStr.substr(divPos + ult::DIVIDER_SYMBOL.length());
                                    const size_t topW = renderer->getTextDimensions(topHalf + "  ", false, fontsize).first;
                                    const size_t botW = renderer->getTextDimensions(botHalf + "  ", false, fontsize).first;
                                    w = std::max(topW, botW);
                                } else {
                                    w = renderer->getTextDimensions(sampleStr + "  ", false, fontsize).first;
                                }
                                if (w > maxWidth) maxWidth = w;
                            } else {
                                const size_t w = renderer->getTextDimensions(std::string(sampleDateTime) + "  ", false, fontsize).first;
                                if (w > maxWidth)
                                    maxWidth = w;
                            }
                        }
                    
                        width = maxWidth;
                    
                    } else {
                        continue;
                    }
                    
                    if (rectangleWidth < width) {
                        rectangleWidth = width;
                    }
                }
                {
                    // drawString(Y) places the baseline at Y.  We want the top and bottom gaps
                    // (inside the rounded rect) to be visually equal.
                    //
                    // Using the font-wide ascent/descent metrics does NOT achieve this because:
                    //   • ascent includes room for accents/tall glyphs; typical caps only reach
                    //     cap_height (< ascent), leaving extra blank space above the glyph pixels.
                    //   • descentAbs is the font-wide maximum descent; most Mini rows (RAM, DTC,
                    //     FPS, …) have no descenders so the glyph bottom sits at the baseline.
                    // Result: top gap looks visually larger than bottom gap.
                    //
                    // Fix: measure the ACTUAL pixel bounds of 'A' (representative tall cap):
                    //   bounds[1]  = top   offset from baseline (negative → glyph top is above baseline)
                    //   bounds[3]  = bottom offset from baseline (0 for caps, positive for descenders)
                    // Setting cachedBaselineOffset = -bounds[1] places currentY so the topmost
                    // pixel of 'A' lands exactly (spacing + topPadding) below the box top.
                    // Setting cachedDescentAbs = bounds[3] (= 0 for 'A') makes the bottom gap
                    // equal to (spacing + bottomPadding) == top gap for all-cap rows like DTC/RAM.
                    // Rows with true descenders (g, p, y) use the spacing+bottomPadding buffer
                    // (default 8+5 = 13 px) which is ample.
                    const auto glyphA = tsl::gfx::FontManager::getOrCreateGlyph('A', false, fontsize);
                    if (glyphA && glyphA->height > 0) {
                        cachedBaselineOffset = -glyphA->bounds[1]; // cap height in px (positive)
                        cachedDescentAbs     =  glyphA->bounds[3]; // 0 for caps; > 0 for descenders
                        bottomPadding        = topPadding;         // equal padding → equal visual gaps
                        Initialized          = true;
                        needsRecalc          = true;
                    }
                    // else: glyph/font not ready yet — retry next frame
                }
            }

            
            // Recalculate layout when needed (including content changes)
            if (needsRecalc) {
                // Build label lines array for individual centering
                labelLines.clear();
                entryCount = 0;
                uint16_t flags = 0;
                
                bool shouldAdd;
                std::string labelText;
                for (const auto& key : showKeys) {
                    shouldAdd = false;
                    labelText = "";
                    
                    if (key == "CPU" && !(flags & 1)) {
                        const bool wantCPUFullSplit = settings.showFullCPU && settings.showStackedFullCPU;
                        const bool wantCPUSplit = !wantCPUFullSplit && settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                        if (wantCPUFullSplit) {
                            labelLines.push_back("CPU_SFULL");
                            labelLines.push_back("CPU_SFREQ");
                            entryCount++;
                        } else if (wantCPUSplit) {
                            labelLines.push_back("CPU_SVOLT");
                            labelLines.push_back("CPU_STEMP");
                            entryCount++;
                        } else {
                            shouldAdd = true;
                            labelText = "CPU";
                        }
                        flags |= 1;
                    } else if (key == "GPU" && !(flags & 2)) {
                        const bool wantGPUSplit = settings.showGPUTemp && settings.showStackedGPUTemp && settings.realVolts;
                        if (wantGPUSplit) {
                            labelLines.push_back("GPU_SVOLT");
                            labelLines.push_back("GPU_STEMP");
                            entryCount++;
                        } else {
                            shouldAdd = true;
                            labelText = "GPU";
                        }
                        flags |= 2;
                    } else if (key == "RAM" && !(flags & 4)) {
                        const bool wantSplitVDDQ = settings.showStackedVDDQ &&
                                                   settings.realVolts && settings.showVDD2 &&
                                                   settings.showVDDQ && isMariko;
                        // ramTempSplit: single volt rail (VDDQ-only Mariko, or VDD2-only any platform), RAM temp non-SBS
                        const bool wantRAMTempSplit = settings.showRAMTemp && settings.showStackedRAMTemp &&
                                                      !wantSplitVDDQ && settings.realVolts &&
                                                      ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                                       (settings.showVDD2 && !settings.showVDDQ));
                        const bool wantRAMLoadCPUGPUSplit = settings.showRAMLoad && settings.showRAMLoadCPUGPU &&
                                                      settings.showStackedRAMLoadCPUGPU &&
                                                      (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck));
                        // BW stacked: BW occupies the top row, RAM content the bottom row,
                        // reusing the RAM_SLOAD_TOP/BOT infrastructure for right-alignment + volts.
                        // VDDQ split and RAM temp split are right-side decorations — orthogonal to row
                        // layout — so they must NOT block BW stacking.
                        // Only blocked when load-CPU/GPU-split already owns both rows with load data.
                        const bool wantBWStackedSplit = settings.showRAMBandwidth &&
                                                        settings.showStackedRAMBandwidth &&
                                                        !wantRAMLoadCPUGPUSplit;
                        // Either condition drives the 2-row SLOAD split path.
                        // wantBWStackedSplit takes priority: RAM_SLOAD_TOP/BOT handles rSplitVDDQ and
                        // rSplitTemp internally, so we never fall through to RAM_SVDD2/RAM_SVDDQ etc.
                        const bool wantAnySplit = wantRAMLoadCPUGPUSplit || wantBWStackedSplit;

                        if (wantAnySplit) {
                            // RAM_SLOAD_TOP/BOT handles all cases: load-split, BW-stacked, and
                            // BW-stacked with VDDQ/temp split on the right side.
                            labelLines.push_back("RAM_SLOAD_TOP");
                            labelLines.push_back("RAM_SLOAD_BOT");
                            entryCount++;
                        } else if (wantSplitVDDQ) {
                            labelLines.push_back("RAM_SVDD2");
                            labelLines.push_back("RAM_SVDDQ");
                            entryCount++;  // two rows count as one extra
                        } else if (wantRAMTempSplit) {
                            labelLines.push_back("RAM_SVDDQ_ONLY");
                            labelLines.push_back("RAM_STEMP");
                            entryCount++;
                        } else {
                            shouldAdd = true;
                            labelText = "RAM";
                        }
                        flags |= 4;
                    } else if (key == "SOC" && !(flags & 8)) {
                        shouldAdd = true;
                        labelText = "SOC";
                        flags |= 8;
                    } else if (key == "TMP" && !(flags & 16)) {
                        // Split mode: fan on row 1, SOC volt on row 2 (requires both fan AND volt active)
                        const bool wantSplit = settings.showStackedFanSOC &&
                                               settings.showFanPercentage &&
                                               settings.realVolts && settings.showSOCVoltage;
                        if (wantSplit) {
                            // Split mode: always 2 rows regardless of temp group config
                            labelLines.push_back("TMP_SFAN");
                            labelLines.push_back("TMP_SVOLT");
                        } else if (settings.showComponentTemps && settings.showSocPcbSkinTemps) {
                            // Dual-row mode: TMP expands to two rows
                            labelLines.push_back("TMP_TOP");
                            labelLines.push_back("TMP_BOT");
                        } else if (settings.showComponentTemps) {
                            // Only component temps: single row with HIGH gradient
                            labelLines.push_back("TMP_COMP");
                            entryCount++;
                        } else {
                            // Only SOC/PCB/Skin (default): single row
                            shouldAdd = true;
                            labelText = "TMP";
                        }
                        flags |= 16;
                    } else if ((key == "BAT" || key == "DRAW") && !(flags & 32)) {
                        if (settings.showStackedBAT) {
                            labelLines.push_back("BAT_STOP");
                            labelLines.push_back("BAT_SBOT");
                            entryCount += 2;
                        } else {
                            shouldAdd = true;
                            labelText = "BAT";
                        }
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
                    } else if (key == "READ" && !(flags & 512) && GameRunning) {
                        shouldAdd = true;
                        labelText = "READ";
                        flags |= 512;
                    } else if (key == "DTC" && !(flags & 256) && settings.showDTC) {
                        if (settings.showStackedDTC) {
                            labelLines.push_back("DTC_STOP");
                            labelLines.push_back("DTC_SBOT");
                            entryCount += 2;
                        } else {
                            shouldAdd = true;
                            labelText = settings.useDTCSymbol ? "\uE007" : "DTC";
                        }
                        flags |= 256;
                    } else if (key == "MEM" && !(flags & 1024)) {
                        shouldAdd = true;
                        labelText = "MEM";
                        flags |= 1024;
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
                
                // Use labelLines.size() (just computed above) — always in sync with current settings,
                // unlike Variables[] which is written by update() and may lag behind needsRecalc.
                const size_t actualEntryCount = labelLines.size();
                
                // Height layout (drawString puts baseline at Y):
                //   currentY for row 1 = boxTop + ascent + spacing + topPadding
                //     -> first glyph top pixel sits at: spacing + topPadding   (= top gap)
                //   each subsequent row advances by (fontsize + spacing)
                //   last row's baseline   = boxTop + ascent + spacing + topPadding + (N-1)*(fontsize+spacing)
                //     -> last glyph bottom = last baseline + descentAbs
                //   We want bottom gap == top gap == (spacing + bottomPadding), with bottomPadding == topPadding.
                //   So: cachedHeight = last_glyph_bottom + spacing + bottomPadding
                //                    = ascent + descentAbs + 2*spacing + topPadding + bottomPadding
                //                      + (N-1)*(fontsize+spacing)
                //   Using ascent+descentAbs (measured) instead of fontsize avoids the 1-2 px truncation
                //   error stbtt's int-cast leaves in (ascent+descentAbs) vs fontsize -- that error was
                //   leaking entirely into the bottom gap, making it visibly fatter than the top.
                const int   firstRowExtent = cachedBaselineOffset + cachedDescentAbs;  // ascent + descentAbs
                cachedHeight = (firstRowExtent
                              + ((fontsize + settings.spacing) * (actualEntryCount > 0 ? actualEntryCount - 1 : 0))
                              + 2 * settings.spacing
                              + topPadding + bottomPadding);


                // THIS BLOCK IS THE ISSUE. IT IS APPLYING EVEN WHEN STUFF IS NOT SHOWING. IT IS NOT SPECIFIC TO THE STACKED LINE CASES THAT ARE VISIBLLY HAPPENING ONLY WHEN THEY ARE HAPPENING AND VISIBLE
                // Two-row blocks (dual-row TMP, TMP split, RAM split) use half inter-row spacing, trim box height
                //if ((settings.showComponentTemps && settings.showSocPcbSkinTemps) ||
                //    (settings.showStackedFanSOC && settings.showFanPercentage &&
                //     settings.realVolts && settings.showSOCVoltage))
                //    cachedHeight -= settings.spacing / 2;
                //if (settings.showStackedVDDQ && settings.realVolts &&
                //    settings.showVDD2 && settings.showVDDQ && isMariko)
                //    cachedHeight -= settings.spacing / 2;

                // Split-row compression handling
                {
                    const int splitCompression = settings.spacing / 2;
                
                    const auto hasKey = [&](const char* key) {
                        return std::find(showKeys.begin(), showKeys.end(), key) != showKeys.end();
                    };
                
                    //
                    // TMP
                    //
                
                    // TMP dual-row:
                    //   TOP = component temps
                    //   BOT = SOC/PCB/Skin temps
                    //
                    // TMP split:
                    //   centered fan/volt row between TMP rows
                    //
                    // Both compress the TMP block into a shared 2-row layout.
                    const bool wantTMPDualRow =
                        settings.showComponentTemps &&
                        settings.showSocPcbSkinTemps;
                
                    const bool wantTMPSplit =
                        settings.showStackedFanSOC &&
                        settings.showFanPercentage &&
                        settings.realVolts &&
                        settings.showSOCVoltage;
                
                    if ((wantTMPDualRow || wantTMPSplit) && hasKey("TMP")) {
                        cachedHeight -= splitCompression;
                    }
                
                    //
                    // CPU / GPU
                    //
                
                    // CPU full split owns the CPU row layout and suppresses temp split ownership.
                    const bool wantCPUFullSplit =
                        settings.showFullCPU &&
                        settings.showStackedFullCPU;
                
                    const bool wantCPUSplit =
                        !wantCPUFullSplit &&
                        settings.showCPUTemp &&
                        settings.showStackedCPUTemp &&
                        settings.realVolts;
                
                    const bool wantGPUSplit =
                        settings.showGPUTemp &&
                        settings.showStackedGPUTemp &&
                        settings.realVolts;
                
                    if ((wantCPUSplit || wantCPUFullSplit) && hasKey("CPU")) {
                        cachedHeight -= splitCompression;
                    }
                
                    if (wantGPUSplit && hasKey("GPU")) {
                        cachedHeight -= splitCompression;
                    }
                
                    //
                    // RAM layout ownership
                    //
                
                    // Stacked VDD2/VDDQ split
                    const bool wantVDDQSplit =
                        settings.showStackedVDDQ &&
                        settings.realVolts &&
                        settings.showVDD2 &&
                        settings.showVDDQ &&
                        isMariko;
                
                    // RAM temp split:
                    // only active when exactly one voltage rail is visible.
                    const bool wantRAMTempSplit =
                        settings.showRAMTemp &&
                        settings.showStackedRAMTemp &&
                        settings.realVolts &&
                        (
                            (settings.showVDDQ && isMariko && !settings.showVDD2) ||
                            (settings.showVDD2 && !settings.showVDDQ)
                        );
                
                    // RAM load split owns the RAM row layout completely.
                    const bool wantRAMLoadSplit =
                        settings.showRAMLoad &&
                        settings.showRAMLoadCPUGPU &&
                        settings.showStackedRAMLoadCPUGPU;
                
                    // BW stacked split also owns the RAM row layout.
                    const bool wantBWStackedSplit =
                        settings.showRAMBandwidth &&
                        settings.showStackedRAMBandwidth;
                
                    // Determine which RAM feature owns row compression.
                    //
                    // Priority:
                    //   1. RAM load split
                    //   2. BW stacked split
                    //   3. VDDQ split
                    //   4. RAM temp split
                    //
                    // VDDQ/temp are considered decorations once a higher-level
                    // stacked RAM layout takes ownership.
                    bool applyRAMCompression = false;
                
                    if (wantRAMLoadSplit) {
                        applyRAMCompression = true;
                    } else if (wantBWStackedSplit) {
                        applyRAMCompression = true;
                    } else if (wantVDDQSplit) {
                        applyRAMCompression = true;
                    } else if (wantRAMTempSplit) {
                        applyRAMCompression = true;
                    }
                
                    if (applyRAMCompression && hasKey("RAM")) {
                        cachedHeight -= splitCompression;
                    }
                
                    //
                    // BAT
                    //
                
                    // BAT stacked layout:
                    //   BAT_TOP
                    //   BAT_BOT
                    if (settings.showStackedBAT && (hasKey("BAT") || hasKey("DRAW"))) {
                        cachedHeight -= splitCompression;
                    }
                
                    //
                    // DTC
                    //
                
                    // DTC stacked layout:
                    //   DTC_TOP
                    //   DTC_BOT
                    if (settings.showStackedDTC &&
                        settings.showDTC &&
                        hasKey("DTC")) {
                        cachedHeight -= splitCompression;
                    }
                }
                //const uint32_t margin = (fontsize * 4);
                
                cachedBaseX = 0;
                cachedBaseY = 0;
                
                needsRecalc = false;
            }
            
            // Fast rendering using cached values
            const uint32_t margin = (fontsize * 4);
            
            // Draw background
            const tsl::Color bgColor = !isDragging
                ? settings.backgroundColor // full opacity
                : settings.focusBackgroundColor;
            int clippingOffsetX = 0, clippingOffsetY = 0;
            
            const int leftPadding = settings.showLabels ? 0 : settings.spacing + bottomPadding;
            const uint32_t overlayWidth = settings.showLabels 
                ? (margin + rectangleWidth + (fontsize / 3))
                : (rectangleWidth + (fontsize / 3) * 2 + leftPadding);
            
            int _frameOffsetX;
            
            // In limited memory mode, correct frameOffsetX first, then calculate _frameOffsetX
            if (ult::limitedMemory) {
                // Calculate valid range for frameOffsetX (in full 1280px coordinate space)
                const int minFrameX = framePadding - cachedBaseX;
                const int maxFrameX = screenWidth - framePadding - overlayWidth - cachedBaseX;
                
                // Clamp frameOffsetX to valid bounds
                if (frameOffsetX < minFrameX) {
                    frameOffsetX = minFrameX;
                } else if (frameOffsetX > maxFrameX) {
                    frameOffsetX = maxFrameX;
                }
                
                // Check Y bounds
                const int minFrameY = framePadding - cachedBaseY;
                const int maxFrameY = screenHeight - framePadding - cachedHeight - cachedBaseY;
                
                if (frameOffsetY < minFrameY) {
                    frameOffsetY = minFrameY;
                } else if (frameOffsetY > maxFrameY) {
                    frameOffsetY = maxFrameY;
                }
                
                // Now calculate _frameOffsetX for the 448px layer
                _frameOffsetX = std::max(0, frameOffsetX - (1280-448));
                
                // Update layer position
                updateLayerPos();
            } else {
                // Non-limited memory mode - use original clipping offset logic
                _frameOffsetX = frameOffsetX;
                
                // Check X bounds and calculate clipping offset
                if (cachedBaseX + _frameOffsetX < int(framePadding)) {
                    clippingOffsetX = framePadding - (cachedBaseX + _frameOffsetX);
                } else if ((cachedBaseX + _frameOffsetX + overlayWidth) > screenWidth - framePadding) {
                    clippingOffsetX = (screenWidth - framePadding) - (cachedBaseX + _frameOffsetX + overlayWidth);
                }
                
                // Check Y bounds and calculate clipping offset  
                if (cachedBaseY + frameOffsetY < int(framePadding)) {
                    clippingOffsetY = framePadding - (cachedBaseY + frameOffsetY);
                } else if ((cachedBaseY + frameOffsetY + cachedHeight) > screenHeight - framePadding) {
                    clippingOffsetY = (screenHeight - framePadding) - (cachedBaseY + frameOffsetY + cachedHeight);
                }
            }
            
            // Apply to all drawing calls
            renderer->drawRoundedRectSingleThreaded(
                cachedBaseX + _frameOffsetX + clippingOffsetX, 
                cachedBaseY + frameOffsetY + clippingOffsetY, 
                overlayWidth, 
                cachedHeight, 
                16, 
                a(bgColor)
            );
                        
            
            // Split Variables into lines for individual positioning
            std::vector<std::string> variableLines;
            const std::string variablesStr(Variables);
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
            uint32_t currentY = cachedBaseY + cachedBaselineOffset + settings.spacing + topPadding;
            size_t labelIndex = 0;
            
            static const std::vector<std::string> specialChars = {""};
            static const std::vector<std::string> cpuPadChars = {"#"};
            static const tsl::Color cpuPadTransparent{0, 0, 0, 0};

            static uint32_t labelWidth, labelCenterX;
            static int sfanFanColX = 0;     // shared between TMP_SFAN and TMP_SVOLT
            static int sfanBaseY = 0;       // TMP_SFAN baseY, used by TMP_SVOLT for volt-at-top swap
            static int ramSplitVoltColX = 0; // shared between RAM_SVDD2 and RAM_SVDDQ
            static int cpuSplitVoltColX = 0; // shared between CPU_SVOLT and CPU_STEMP
            static int gpuSplitVoltColX = 0; // shared between GPU_SVOLT and GPU_STEMP
            static int ramTempVoltColX = 0;  // shared between RAM_SVDDQ_ONLY and RAM_STEMP
            static int ramLoadVoltColX = 0;  // shared between RAM_SLOAD_TOP and RAM_SLOAD_BOT
            static int ramLoadTopBaseY = 0;  // RAM_SLOAD_TOP baseY, used by BOT to compute center-Y
            // Cross-block 3-divider scissor cuts: written in the TOP/CENTER label block,
            // read in the BOT label block (which runs on the next iteration). These are
            // screen-Y values; the BOT-row divider should be drawn scissored to
            // [cutCB, kNoLowerCut). The TOP-row divider in the same block as the center
            // is drawn locally with [kNoUpperCut, cutTC); the center divider with [cutTC, cutCB).
            // A negative value indicates "no active cut for this pair this frame".
            static int sfanStackCutCB         = -1; // TMP_SFAN center → TMP_SVOLT bot
            static int sfanStackCutTC         = -1; // (for symmetry; may not be needed if no top in SFAN itself)
            static int ramSplitStackCutCB     = -1; // RAM_SVDD2 center → RAM_SVDDQ bot
            static int cpuSplitStackCutCB     = -1; // CPU_SVOLT center → CPU_STEMP bot
            static int gpuSplitStackCutCB     = -1; // GPU_SVOLT center → GPU_STEMP bot
            static int ramTempSplitStackCutCB = -1; // RAM_SVDDQ_ONLY center → RAM_STEMP bot
            static int ramLoadStackCutCB      = -1; // RAM_SLOAD_TOP center → RAM_SLOAD_BOT bot
            // Glyph metrics for the divider (cached per fontsize at start of each frame
            // by the helper). Filled in by getDividerScissorMetrics when first needed.
            // Used to scissor cross-block draws without re-querying glyph metrics.
            static int dividerCenterOffY = 0;
            static int dividerLeftPad    = 0;
            static int dividerFullW      = 0;
            static int cpuFullSplitBracketsW = 0; // brackets pixel width for CPU_SFULL/CPU_SFREQ alignment
            static int cpuFullSplitColW      = 0; // column width = max(bracketsW, freqW) — whichever line is longer
            static std::vector<std::string> _variableLines;
            if (!delayUpdate)
                _variableLines = variableLines;
            for (size_t i = 0; i < _variableLines.size() && labelIndex < labelLines.size(); i++) {
                // Check for RES label without corresponding RES data
                if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RES") {
                    //const std::string& currentLine = _variableLines[i];
                    // Check if this data line looks like resolution data (contains 'x' or 'p')
                    bool isResolutionData = m_resolutionOutput[0].width;
                    
                    if (!isResolutionData) {
                        // This is RES label but data isn't resolution data - skip the label
                        ++labelIndex;
                        // Don't increment i, so the current data line gets paired with the next label
                        --i;
                        continue;
                    }
                }

                // Draw label (centered in label region)
                // TMP_TOP/TMP_BOT/TMP_COMP/TMP_SFAN/TMP_SVOLT: skip default label draw (we draw "TMP" manually in the data section)
                const bool isTmpDualRow = (labelIndex < labelLines.size() &&
                    (labelLines[labelIndex] == "TMP_TOP" || labelLines[labelIndex] == "TMP_BOT" ||
                     labelLines[labelIndex] == "TMP_COMP" ||
                     labelLines[labelIndex] == "TMP_SFAN" || labelLines[labelIndex] == "TMP_SVOLT" ||
                     labelLines[labelIndex] == "RAM_SVDD2" || labelLines[labelIndex] == "RAM_SVDDQ" ||
                     labelLines[labelIndex] == "CPU_SVOLT" || labelLines[labelIndex] == "CPU_STEMP" ||
                     labelLines[labelIndex] == "GPU_SVOLT" || labelLines[labelIndex] == "GPU_STEMP" ||
                     labelLines[labelIndex] == "CPU_SFULL" || labelLines[labelIndex] == "CPU_SFREQ" ||
                     labelLines[labelIndex] == "RAM_SVDDQ_ONLY" || labelLines[labelIndex] == "RAM_STEMP" ||
                     labelLines[labelIndex] == "BAT_STOP" || labelLines[labelIndex] == "BAT_SBOT" ||
                     labelLines[labelIndex] == "DTC_STOP" || labelLines[labelIndex] == "DTC_SBOT" ||
                     labelLines[labelIndex] == "RAM_SLOAD_TOP" || labelLines[labelIndex] == "RAM_SLOAD_BOT"));
                if (!isTmpDualRow && settings.showLabels && !labelLines[labelIndex].empty()) {
                    labelWidth = renderer->getTextDimensions(labelLines[labelIndex], false, fontsize).first;
                    labelCenterX = cachedBaseX + (margin / 2) - (labelWidth / 2);
                    renderer->drawString(labelLines[labelIndex], false, labelCenterX + _frameOffsetX + clippingOffsetX, currentY + frameOffsetY + clippingOffsetY, fontsize, settings.catColor);
                }
                
                // Determine rendering method based on label type
                const std::string& currentLine = _variableLines[i];
                const int leftPadding = settings.showLabels ? 0 : settings.spacing + bottomPadding;
                const int baseX = settings.showLabels 
                    ? (cachedBaseX + margin + _frameOffsetX + clippingOffsetX)
                    : (cachedBaseX + (fontsize / 3) + leftPadding + _frameOffsetX + clippingOffsetX);
                const int baseY = currentY + frameOffsetY + clippingOffsetY;
                
                if (labelIndex < labelLines.size() && labelLines[labelIndex] == "SOC") {
                    // SOC temperature rendering with gradient
                    const size_t degreesPos = currentLine.find("°");
                    if (degreesPos != std::string::npos) {
                        // Find the 'C' after the degrees symbol
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos != std::string::npos) {
                            const size_t tempEnd = cPos + 1; // Include the 'C'
                            
                            // Extract temperature value and apply gradient
                            const int temp = atoi(currentLine.c_str());
                            const tsl::Color tempColor = tsl::GradientColor((float)temp);
                            
                            // Split into temperature part and remaining part
                            const std::string tempPart = currentLine.substr(0, tempEnd);
                            const std::string restPart = currentLine.substr(tempEnd);
                            
                            // Render temperature with gradient color
                            int currentX = baseX;
                            renderer->drawString(tempPart, false, currentX, baseY, fontsize, tempColor);
                            
                            // Render remaining text: separator in separatorColor, fan icon in catColor
                            if (!restPart.empty()) {
                                currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                                static const std::string socFanIcon = "";
                                const size_t fanPos = restPart.find(socFanIcon);
                                if (fanPos != std::string::npos) {
                                    // Before fan icon: separator char -> separatorColor
                                    if (fanPos > 0)
                                        currentX += renderer->drawStringWithColoredSections(restPart.substr(0, fanPos), false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor).first;
                                    // Fan icon alone -> catColor
                                    currentX += renderer->drawString(socFanIcon, false, currentX, baseY, fontsize, settings.catColor).first;
                                    // After fan icon: remaining text (% value, optional volt sep+value) -> separatorColor for sep char
                                    const std::string afterFan = restPart.substr(fanPos + socFanIcon.size());
                                    if (!afterFan.empty())
                                        renderer->drawStringWithColoredSections(afterFan, false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor);
                                } else {
                                    renderer->drawStringWithColoredSections(restPart, false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor);
                                }
                            }
                        } else {
                            // Fallback: no C found after degrees, render normally
                            renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    } else {
                        // Fallback: no degrees symbol found, render normally
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                    
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP") {
                    // TMP multiple temperatures rendering with gradients
                    int currentX = baseX;
                    size_t pos = 0;
                    bool parseSuccess = true;
                    
                    // Parse up to 3 temperatures in the format "XX°C XX°C XX°C (XX%)"
                    for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                        // Find start of current temperature (skip any leading spaces)
                        while (pos < currentLine.length() && currentLine[pos] == ' ') {
                            renderer->drawString(" ", false, currentX, baseY, fontsize, settings.textColor);
                            currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                            pos++;
                        }
                        
                        if (pos >= currentLine.length()) break;
                        
                        // Find degrees symbol
                        const size_t degreesPos = currentLine.find("°", pos);
                        if (degreesPos == std::string::npos) {
                            parseSuccess = false;
                            break;
                        }
                        
                        // Find 'C' after degrees symbol
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos == std::string::npos) {
                            parseSuccess = false;
                            break;
                        }
                        
                        const size_t tempEnd = cPos + 1; // Include the 'C'
                        
                        // Extract and render temperature with gradient
                        const std::string tempPart = currentLine.substr(pos, tempEnd - pos);
                        const int temp = atoi(tempPart.c_str());
                        const tsl::Color tempColor = tsl::GradientColor((float)temp);
                        
                        renderer->drawString(tempPart, false, currentX, baseY, fontsize, tempColor);
                        currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                        
                        pos = tempEnd;
                    }
                    
                    // voltageAtEndTMP OFF => div+volt then div+fan; ON => div+fan then div+volt.
                    const bool tmpSOCHasVolt = settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0];
                    if (!settings.voltageAtEndTMP && tmpSOCHasVolt) {
                        currentX += renderer->drawString(ult::DIVIDER_SYMBOL, false, currentX, baseY, fontsize, settings.separatorColor).first;
                        renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        currentX += (int)renderer->getTextDimensions(std::string(MINI_SOC_volt_c), false, fontsize).first;
                    }
                    // Render remaining: div(sep) + fan(cat)  [volt no longer in data string]
                    if (pos < currentLine.length()) {
                        std::string restPart = currentLine.substr(pos);
                        const size_t divLen = ult::DIVIDER_SYMBOL.length();
                        const size_t div1Pos = restPart.find(ult::DIVIDER_SYMBOL);
                        if (div1Pos != std::string::npos) {
                            // Draw divider (before fan) in separatorColor
                            currentX += renderer->drawString(restPart.substr(0, div1Pos + divLen), false, currentX, baseY, fontsize, settings.separatorColor).first;
                            const std::string afterDiv1 = restPart.substr(div1Pos + divLen);
                            // Fan only (volt drawn explicitly), fan icon in catColor
                            if (!afterDiv1.empty()) {
                                static const std::vector<std::string> tmpFanIconChars = {""};
                                currentX += renderer->drawStringWithColoredSections(afterDiv1, false, tmpFanIconChars,
                                    currentX, baseY, fontsize, settings.textColor, settings.catColor).first;
                            }
                        } else {
                            currentX += renderer->drawStringWithColoredSections(restPart, false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor).first;
                        }
                    }
                    if (settings.voltageAtEndTMP && tmpSOCHasVolt) {
                        currentX += renderer->drawString(ult::DIVIDER_SYMBOL, false, currentX, baseY, fontsize, settings.separatorColor).first;
                        renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, currentX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                    // If parsing failed, fall back to normal rendering
                    if (!parseSuccess) {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP_TOP") {
                    // Dual-row TMP: top row = component die temps (CPU/GPU/RAM) with gradient
                    int currentX = baseX;
                    size_t pos = 0;
                    bool parseSuccess = true;
                    for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                        while (pos < currentLine.length() && currentLine[pos] == ' ') {
                            renderer->drawString(" ", false, currentX, baseY, fontsize, settings.textColor);
                            currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                            pos++;
                        }
                        if (pos >= currentLine.length()) break;
                        const size_t degreesPos = currentLine.find("\u00B0", pos);
                        if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos == std::string::npos) { parseSuccess = false; break; }
                        const std::string tempPart = currentLine.substr(pos, cPos + 1 - pos);
                        const int temp = atoi(tempPart.c_str());
                        renderer->drawString(tempPart, false, currentX, baseY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)temp, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                        pos = cPos + 1;
                    }
                    if (!parseSuccess) {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                    // Draw "TMP" label and fan speed centered between the two rows
                    // Rows use half inter-row spacing, so center is at (fontsize + spacing/2) / 2
                    const int centerY = (int)currentY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    if (settings.showLabels) {
                        const std::string tmpLbl = "TMP";
                        const uint32_t tmpLblW = renderer->getTextDimensions(tmpLbl, false, fontsize).first;
                        const uint32_t tmpLblX = cachedBaseX + (margin / 2) - (tmpLblW / 2);
                        renderer->drawString(tmpLbl, false, tmpLblX + _frameOffsetX + clippingOffsetX,
                            centerY + frameOffsetY + clippingOffsetY, fontsize, settings.catColor);
                    }
                    //if (settings.showFanPercentage) {
                    //    const int fanDuty = safeFanDuty((int)Rotation_Duty);
                    //    char fanStr[24];
                    //    snprintf(fanStr, sizeof(fanStr), "%sX %d%%", ult::DIVIDER_SYMBOL.c_str(), fanDuty);
                    //    // Draw fan at the X where it would naturally fall after the SOC/PCB/Skin temps
                    //    const int fanX = baseX + (int)renderer->getTextDimensions(std::string(skin_temperature_c), false, fontsize).first;
                    //    renderer->drawStringWithColoredSections(std::string(fanStr), false, specialChars,
                    //        fanX, centerY + frameOffsetY + clippingOffsetY, fontsize,
                    //        settings.textColor, settings.separatorColor);
                    //}

                    // Fan and voltage are both drawn centered between the two rows
                    if (settings.showFanPercentage || (settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0])) {
                        const int fanY = centerY + frameOffsetY + clippingOffsetY;
                        // Anchor starts right after the SOC/PCB/Skin temp text
                        const int fanX = baseX + (int)renderer->getTextDimensions(std::string(skin_temperature_c), false, fontsize).first;
                        int afterContentX = fanX;
                        // 3-tall divider stack: connecting divider between 2-row temp data and centered fan/volt.
                        // Draw at top-row Y (baseY), center Y (fanY), and bot-row Y so it spans the full block height.
                        // Scissored so the three glyphs meet pixel-perfect without alpha-blend overlap shading.
                        const int tmpTopRowY = baseY;
                        const int tmpBotRowY = baseY + (int)fontsize + (int)settings.spacing / 2;
                        mini_divider_scissor::drawDividerStack3(renderer, fanX,
                            tmpTopRowY, fanY, tmpBotRowY,
                            fontsize, settings.separatorColor);
                        afterContentX = fanX + (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const bool tmpTopHasVolt = settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0];
                        // voltageAtEndTMP OFF: volt right after structural DIV (no extra leading DIV), then DIV, then fan.
                        // voltageAtEndTMP ON:  fan first, then DIV, then volt (original order).
                        if (!settings.voltageAtEndTMP && tmpTopHasVolt) {
                            // No extra divider before volt: structural divider already serves as separator.
                            renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, afterContentX, fanY, fontsize, settings.textColor, settings.separatorColor);
                            afterContentX += (int)renderer->getTextDimensions(std::string(MINI_SOC_volt_c), false, fontsize).first;
                            // Divider between volt and fan.
                            afterContentX += renderer->drawString(ult::DIVIDER_SYMBOL, false, afterContentX, fanY, fontsize, settings.separatorColor).first;
                        }
                        if (settings.showFanPercentage) {
                            const int fanDuty = safeFanDuty((int)Rotation_Duty);
                            char fanPctStr[24];
                            snprintf(fanPctStr, sizeof(fanPctStr), " %d%%", fanDuty);
                            static const std::vector<std::string> fanIconChars = {""};
                            renderer->drawStringWithColoredSections(std::string(fanPctStr), false, fanIconChars,
                                afterContentX, fanY, fontsize, settings.textColor, settings.catColor);
                            afterContentX += (int)renderer->getTextDimensions(std::string(fanPctStr), false, fontsize).first;
                        }
                        if (settings.voltageAtEndTMP && tmpTopHasVolt) {
                            afterContentX += renderer->drawString(ult::DIVIDER_SYMBOL, false, afterContentX, fanY, fontsize, settings.separatorColor).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, afterContentX, fanY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP_BOT") {
                    // Dual-row TMP: bottom row = SOC/PCB/Skin temps, shifted up by spacing/2
                    const int botY = baseY - (int)settings.spacing / 2;
                    int currentX = baseX;
                    size_t pos = 0;
                    bool parseSuccess = true;
                    for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                        while (pos < currentLine.length() && currentLine[pos] == ' ') {
                            renderer->drawString(" ", false, currentX, botY, fontsize, settings.textColor);
                            currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                            pos++;
                        }
                        if (pos >= currentLine.length()) break;
                        const size_t degreesPos = currentLine.find("\u00B0", pos);
                        if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos == std::string::npos) { parseSuccess = false; break; }
                        const std::string tempPart = currentLine.substr(pos, cPos + 1 - pos);
                        const int temp = atoi(tempPart.c_str());
                        renderer->drawString(tempPart, false, currentX, botY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)temp) : settings.textColor);
                        currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                        pos = cPos + 1;
                    }
                    if (!parseSuccess) {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, botY, fontsize, settings.textColor, settings.separatorColor);
                        currentX = baseX + (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    }
                                    // Compensate: loop will add full spacing, but visually we drew spacing/2 less,
                    // so pull currentY back by spacing/2 so the next row gap stays normal.
                    currentY -= (int)settings.spacing / 2;
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP_COMP") {
                    // Component temps only: single row with HIGH gradient
                    int currentX = baseX;
                    size_t pos = 0;
                    bool parseSuccess = true;
                    for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                        while (pos < currentLine.length() && currentLine[pos] == ' ') {
                            renderer->drawString(" ", false, currentX, baseY, fontsize, settings.textColor);
                            currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                            pos++;
                        }
                        if (pos >= currentLine.length()) break;
                        const size_t degreesPos = currentLine.find("\u00B0", pos);
                        if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos == std::string::npos) { parseSuccess = false; break; }
                        const std::string tempPart = currentLine.substr(pos, cPos + 1 - pos);
                        const int temp = atoi(tempPart.c_str());
                        renderer->drawString(tempPart, false, currentX, baseY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)temp, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                        pos = cPos + 1;
                    }
                    if (!parseSuccess) {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        // Update currentX so fan is positioned after the fallback-drawn string
                        currentX = baseX + (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    }
                    // Draw "TMP" label at normal Y (baseY already has offsets)
                    if (settings.showLabels) {
                        const std::string tmpLbl = "TMP";
                        const uint32_t tmpLblW = renderer->getTextDimensions(tmpLbl, false, fontsize).first;
                        const uint32_t tmpLblX = cachedBaseX + (margin / 2) - (tmpLblW / 2);
                        renderer->drawString(tmpLbl, false, tmpLblX + _frameOffsetX + clippingOffsetX,
                            baseY, fontsize, settings.catColor);
                    }
                    // voltageAtEndTMP OFF => div+volt then div+fan; ON => div+fan then div+volt.
                    int afterFanX = currentX;
                    const bool compHasVolt = settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0];
                    if (!settings.voltageAtEndTMP && compHasVolt) {
                        const int voltDivX = afterFanX + (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterFanX, baseY, fontsize, settings.separatorColor).first;
                        renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, voltDivX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        afterFanX = voltDivX + (int)renderer->getTextDimensions(std::string(MINI_SOC_volt_c), false, fontsize).first;
                    }
                    if (settings.showFanPercentage) {
                        const int fanDuty = safeFanDuty((int)Rotation_Duty);
                        const int fanY = baseY;
                        const int afterDivX = afterFanX + renderer->drawString(
                            ult::DIVIDER_SYMBOL, false, afterFanX, fanY, fontsize, settings.separatorColor).first;
                        char fanPctStr[24];
                        snprintf(fanPctStr, sizeof(fanPctStr), " %d%%", fanDuty);
                        static const std::vector<std::string> compFanIconChars = {""};
                        renderer->drawStringWithColoredSections(std::string(fanPctStr), false, compFanIconChars,
                            afterDivX, fanY, fontsize, settings.textColor, settings.catColor);
                        afterFanX = afterDivX + (int)renderer->getTextDimensions(std::string(fanPctStr), false, fontsize).first;
                    }
                    if (settings.voltageAtEndTMP && compHasVolt) {
                        const int voltDivX = afterFanX + (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterFanX, baseY, fontsize, settings.separatorColor).first;
                        renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars, voltDivX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_SLOAD_TOP") {
                    // RAM load split row 1: [cpu% gpu%] at baseY. "RAM" label centered between rows.
                    // Right side mirrors the non-loadsplit layout: rSplitVDDQ stacks VDD2 here / VDDQ on BOT;
                    // rSplitTemp puts volt or temp on top depending on voltageAtEndRAM; otherwise nothing on top.
                    const int ramLoadCtrY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    ramLoadTopBaseY = baseY;  // store TOP baseY so BOT can compute center-Y consistently
                    // 3-stack scissoring: top=baseY, center=ramLoadCtrY, bot=ramLoadBotY (in RAM_SLOAD_BOT).
                    // Predicted bot Y = baseY + fontsize + spacing/2.
                    const int ramLoadPredictedBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    bool ramLoadScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int ramLoadCutTC = 0;
                    int ramLoadCutCB = 0;
                    if (ramLoadScissorOK) {
                        ramLoadCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, ramLoadCtrY, dividerCenterOffY);
                        ramLoadCutCB = mini_divider_scissor::computeDividerCutY(
                            ramLoadCtrY, ramLoadPredictedBotY, dividerCenterOffY);
                    }
                    ramLoadStackCutCB = ramLoadScissorOK ? ramLoadCutCB : -1;

                    auto rlDrawCenterDiv = [&](int dx) {
                        if (ramLoadScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, ramLoadCtrY,
                                ramLoadCutTC, ramLoadCutCB, fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, ramLoadCtrY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    auto rlDrawTopRowDiv = [&](int dx) {
                        if (ramLoadScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, baseY,
                                mini_divider_scissor::kNoUpperCut, ramLoadCutTC,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, baseY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    // Bot-row DIV drawn FROM TOP (only in voltAtEnd+SBS path), at the pre-fork ramLoadVoltColX.
                    // Y = rLoadBotY = ramLoadPredictedBotY.
                    auto rlDrawBotRowDivFromTop = [&](int dx) {
                        if (ramLoadScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, ramLoadPredictedBotY,
                                ramLoadCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, ramLoadPredictedBotY,
                                fontsize, settings.separatorColor);
                        }
                    };

                    if (settings.showLabels) {
                        const std::string ramLbl = "RAM";
                        const uint32_t ramLblW = renderer->getTextDimensions(ramLbl, false, fontsize).first;
                        const uint32_t ramLblX = cachedBaseX + (margin / 2) - (ramLblW / 2);
                        renderer->drawString(ramLbl, false, ramLblX + _frameOffsetX + clippingOffsetX,
                            ramLoadCtrY, fontsize, settings.catColor);
                    }
                    // Compute widths first so we can right-align the top row.
                    // bwLeftCol: BW is drawn as a separate left column before the stacked-load block.
                    // MINI_RAM_bw_c is populated by update() only in the "BW inline + load stacked" case.
                    const bool bwLeftCol = MINI_RAM_bw_c[0] != '\0';
                    const int bwLeftColW = bwLeftCol
                        ? (int)renderer->getTextDimensions(std::string(MINI_RAM_bw_c), false, fontsize).first : 0;
                    const uint32_t divW = (uint32_t)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    // loadBaseX: left edge of the stacked-load data block (shifted right when BW is left-column)
                    const int loadBaseX = baseX + (bwLeftCol ? bwLeftColW + (int)divW : 0);

                    if (bwLeftCol) {
                        // Draw BW string centered (at ramLoadCtrY) in the left column.
                        // Nothing occupies the bottom row of this column, so centering it
                        // aligns it with the RAM label and the center divider.
                        renderer->drawString(std::string(MINI_RAM_bw_c), false, baseX, ramLoadCtrY, fontsize, settings.textColor);
                        // Draw 3-stack dividers at baseX+bwLeftColW: top, center, and bot rows
                        rlDrawTopRowDiv(baseX + bwLeftColW);
                        rlDrawCenterDiv(baseX + bwLeftColW);
                        rlDrawBotRowDivFromTop(baseX + bwLeftColW);
                        MINI_RAM_bw_c[0] = '\0';
                    }

                    const uint32_t topW = renderer->getTextDimensions(currentLine, false, fontsize).first;
                    const uint32_t botW = MINI_RAM_load_bot_c[0]
                        ? renderer->getTextDimensions(std::string(MINI_RAM_load_bot_c), false, fontsize).first
                        : 0;
                    ramLoadVoltColX = loadBaseX + (int)std::max(topW, botW);

                    // Draw top row right-aligned. currentLine may be one of:
                    //   a) "[44% 44%]"                 — load-only, no BW embedded
                    //   b) "10.0 GB/s"                   — BW-on-own-row (top=BW, bot=load)
                    //   c) "10.0 GB/s" + DIVIDER + "[44% 44%]" — BW stacked + load stacked
                    //      (bwOnOwnRow=false, bwLeftOfLoadSplit=false → inline prepend with DIVIDER)
                    // Cases (a)/(b): no DIVIDER embedded → draw with cpuPadChars to hide '#' sentinels.
                    // Case (c): split at DIVIDER so the BW part uses specialChars (for separator color),
                    //           DIVIDER is drawn in separator color, and the bracket part uses cpuPadChars.
                    const int topDrawX = loadBaseX + (int)std::max(topW, botW) - (int)topW;
                    {
                        const size_t divPos = currentLine.find(ult::DIVIDER_SYMBOL);
                        if (divPos != std::string::npos) {
                            // Case (c): BW + DIVIDER + [#cpu% #gpu%]
                            const std::string bwPart  = currentLine.substr(0, divPos);
                            const std::string brkPart = currentLine.substr(divPos + ult::DIVIDER_SYMBOL.length());
                            int cx = topDrawX;
                            cx += (int)renderer->drawStringWithColoredSections(bwPart,  false, specialChars,  cx, baseY, fontsize, settings.textColor, settings.separatorColor).first;
                            cx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, baseY, fontsize, settings.separatorColor).first;
                            renderer->drawStringWithColoredSections(brkPart, false, cpuPadChars, cx, baseY, fontsize, settings.textColor, cpuPadTransparent);
                        } else {
                            // Cases (a)/(b): no embedded DIVIDER
                            renderer->drawStringWithColoredSections(currentLine, false, cpuPadChars, topDrawX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                        }
                    }

                    // Determine right-side mode (mirrors label-building conditions)
                    const bool rSplitVDDQ = settings.showStackedVDDQ && settings.realVolts &&
                                            settings.showVDD2 && settings.showVDDQ && isMariko;
                    const bool rSplitTemp = settings.showRAMTemp && settings.showStackedRAMTemp &&
                                            !rSplitVDDQ && settings.realVolts &&
                                            ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                             (settings.showVDD2 && !settings.showVDDQ));

                    if (rSplitVDDQ) {
                        // Stacked VDD2/VDDQ. Mirror SVDD2 top-row logic.
                        // Centre connector divider at ramLoadCtrY, linking the TOP-row DIV (before VDD2)
                        // and the BOT-row DIV (before VDDQ) into a continuous fork.
                        rlDrawCenterDiv(ramLoadVoltColX);
                        const uint32_t sbs_dw = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        if (settings.showRAMTemp && mini_ram_temp_c[0] && settings.voltageAtEndRAM &&
                            !settings.showStackedRAMTemp) {
                            // VoltAtEnd + SBS temp: temp at center Y BEFORE the volts in its own column.
                            // Layout mirrors SVDD2 lines 1599-1608:
                            //   center connector | temp | right-fork connector | DIV + VDD2 (top) / DIV + VDDQ (bot)
                            // Shift ramLoadVoltColX past the temp column so BOT's VDDQ aligns with TOP's VDD2.
                            // The LEFT side of the temp column needs a 3-tall divider stack:
                            // center connector already drawn above; add top-row and bot-row dividers here.
                            // We draw the BOT row's divider from here (since ramLoadVoltColX will mutate
                            // before RAM_SLOAD_BOT runs). We also clear ramLoadStackCutCB so BOT doesn't
                            // re-draw it (BOT will use the new ramLoadVoltColX for its own DIVs which are
                            // separate from this pre-fork stack).
                            rlDrawTopRowDiv(ramLoadVoltColX);
                            rlDrawBotRowDivFromTop(ramLoadVoltColX);
                            const int rtv = atoi(mini_ram_temp_c);
                            int cx = ramLoadVoltColX + (int)sbs_dw;
                            renderer->drawString(std::string(mini_ram_temp_c), false, cx, ramLoadCtrY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            cx += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                            // Right-fork connector at center Y, before the new volt column.
                            // This is the center of a NEW 3-stack (new col). Bot of that new stack is the
                            // VDDQ divider drawn by RAM_SLOAD_BOT at the new ramLoadVoltColX — so the cut
                            // we wrote at ramLoadStackCutCB is correct for it (same Y math).
                            rlDrawCenterDiv(cx);
                            ramLoadVoltColX = cx;  // VDD2/VDDQ start at the right-fork connector X
                            // Draw VDD2 at the new column on baseY
                            if (vdd2_only_c[0]) {
                                int rx = ramLoadVoltColX;
                                rlDrawTopRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        } else if (settings.showRAMTemp && mini_ram_temp_c[0] && settings.voltageAtEndRAM &&
                            settings.showStackedRAMTemp) {
                            // VoltAtEnd + non-SBS temp: DIV+temp+DIV+VDD2 on TOP row (mirroring SVDD2 lines 1618-1631
                            // but with an explicit left DIVIDER before temp so it appears in its own column).
                            // BOT row will then draw VDDQ only (no temp) at ramLoadVoltColX.
                            if (vdd2_only_c[0]) {
                                const int rtv = atoi(mini_ram_temp_c);
                                int rx = ramLoadVoltColX;
                                rlDrawTopRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawString(std::string(mini_ram_temp_c), false, rx, baseY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                                rx += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                                // Div between temp and VDD2 on the top row — different X from the
                                // ramLoadVoltColX 3-stack column, so unscissored (no paired top/bot here).
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, baseY,
                                    fontsize, settings.separatorColor).first;
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        } else {
                            // Default rSplitVDDQ TOP: VDD2 at ramLoadVoltColX on baseY
                            if (vdd2_only_c[0]) {
                                int rx = ramLoadVoltColX;
                                rlDrawTopRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        }
                    } else if (rSplitTemp) {
                        // Single rail + non-SBS temp: mirror SVDDQ_ONLY top row.
                        // Centre connector divider at ramLoadCtrY (links TOP DIV and BOT DIV).
                        rlDrawCenterDiv(ramLoadVoltColX);
                        const char* rTopVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                        if (settings.voltageAtEndRAM) {
                            // VoltAtEnd: temp at top
                            if (mini_ram_temp_c[0]) {
                                int rx = ramLoadVoltColX;
                                rlDrawTopRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                const int tv = atoi(mini_ram_temp_c);
                                renderer->drawString(std::string(mini_ram_temp_c), false, rx, baseY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        } else {
                            // Normal: volt at top
                            if (rTopVolt[0]) {
                                int rx = ramLoadVoltColX;
                                rlDrawTopRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawStringWithColoredSections(std::string(rTopVolt), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        }
                    }
                    // else: inline mode — TOP row draws nothing extra; BOT row handles all volt/temp.
                    // In inline mode, no center/top DIVs in this label block; BOT will run the inline path.

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_SLOAD_BOT") {
                    // RAM load split row 2: total%@freq at baseX, then right-side at ramLoadVoltColX.
                    const int ramLoadBotY = baseY - (int)settings.spacing / 2;
                    // Center Y is computed from TOP's baseY (stored by RAM_SLOAD_TOP) so it matches
                    // TOP's center connector position exactly. Using BOT's baseY directly would point
                    // BELOW the bot row instead of between the two rows.
                    const int ramLoadCtrY = ramLoadTopBaseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    // 3-stack scissoring: BOT row. ramLoadStackCutCB was written by RAM_SLOAD_TOP.
                    // (In inline mode, the inline block computes its own cuts locally and doesn't read this.)
                    const bool ramLoadBotScissorOK = (ramLoadStackCutCB >= 0);
                    auto rlbDrawBotRowDiv = [&](int dx) {
                        if (ramLoadBotScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, ramLoadBotY,
                                ramLoadStackCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, ramLoadBotY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    if (MINI_RAM_load_bot_c[0]) {
                        const uint32_t botW_draw = renderer->getTextDimensions(std::string(MINI_RAM_load_bot_c), false, fontsize).first;
                        // ramLoadVoltColX = baseX + max(topW, botW) — set by RAM_SLOAD_TOP.
                        // Right-align bot row so its right edge matches the top row's right edge.
                        const int botDrawX = ramLoadVoltColX - (int)botW_draw;
                        // Bot row may contain '#' sentinels (e.g. "[#4% #8%] 12%@1600.0" when
                        // bwOnOwnRow=true: BW on top, load-SBS on bottom). Use cpuPadChars so the
                        // '#' padding renders transparent rather than as a literal '#' character.
                        renderer->drawStringWithColoredSections(std::string(MINI_RAM_load_bot_c), false, cpuPadChars, botDrawX, ramLoadBotY, fontsize, settings.textColor, cpuPadTransparent);
                    }
                    // Use shared column X (computed by RAM_SLOAD_TOP); fall back to baseX if unset
                    int afterBotX = ramLoadVoltColX ? ramLoadVoltColX : baseX;

                    // Same right-side mode determination as TOP
                    const bool rSplitVDDQ = settings.showStackedVDDQ && settings.realVolts &&
                                            settings.showVDD2 && settings.showVDDQ && isMariko;
                    const bool rSplitTemp = settings.showRAMTemp && settings.showStackedRAMTemp &&
                                            !rSplitVDDQ && settings.realVolts &&
                                            ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                             (settings.showVDD2 && !settings.showVDDQ));

                    if (rSplitVDDQ) {
                        // BOT row: VDDQ at ramLoadVoltColX. Temp placement mirrors SVDDQ:
                        //  - SBS temp ON: temp at ramLoadCtrY after max(vdd2,vddq) (or voltAtEnd center variant)
                        //  - SBS temp OFF + !voltAtEnd: temp inline after VDDQ on BOT
                        //  - SBS temp OFF + voltAtEnd: temp was drawn on TOP row (skip here)
                        const uint32_t sbs_dw = renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        if (vddq_only_c[0]) {
                            int rx = afterBotX;
                            // Bot-row DIV before VDDQ — scissored to bottom band of the 3-stack.
                            rlbDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(vddq_only_c), false,
                                specialChars, rx, ramLoadBotY, fontsize, settings.textColor, settings.separatorColor);
                            // Non-SBS temp appended after VDDQ at BOT (skip when voltAtEnd — temp on TOP)
                            if (settings.showRAMTemp && settings.showStackedRAMTemp &&
                                !settings.voltageAtEndRAM && mini_ram_temp_c[0]) {
                                rx += (int)renderer->getTextDimensions(std::string(vddq_only_c), false, fontsize).first;
                                // Div before temp — different X from 3-stack column (afterBotX) → unscissored.
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, ramLoadBotY,
                                    fontsize, settings.separatorColor).first;
                                const int rtv = atoi(mini_ram_temp_c);
                                renderer->drawString(std::string(mini_ram_temp_c), false, rx, ramLoadBotY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        }
                        // SBS temp: temp drawn at center Y between the two rows.
                        // Normal SBS:    temp after max(vdd2,vddq) at ramLoadCtrY
                        // VoltAtEnd+SBS: temp was already drawn at center Y BEFORE the volts in TOP row
                        //                (which also shifted ramLoadVoltColX) — skip here.
                        if (settings.showRAMTemp && !settings.showStackedRAMTemp && mini_ram_temp_c[0] &&
                            !settings.voltageAtEndRAM) {
                            const uint32_t vdd2_rw = vdd2_only_c[0]
                                ? renderer->getTextDimensions(std::string(vdd2_only_c), false, fontsize).first : 0;
                            const uint32_t vddq_rw = vddq_only_c[0]
                                ? renderer->getTextDimensions(std::string(vddq_only_c), false, fontsize).first : 0;
                            const int rtv = atoi(mini_ram_temp_c);
                            const int dividerX = afterBotX + (int)sbs_dw + (int)std::max(vdd2_rw, vddq_rw);
                            // 3-tall divider stack: top row, center, bot row — connects the 2-row volt column
                            // (VDD2 top, VDDQ bot) to the single-row temp at center Y.
                            // Scissored so the three glyphs meet pixel-perfect without alpha-blend overlap shading.
                            mini_divider_scissor::drawDividerStack3(renderer, dividerX,
                                ramLoadTopBaseY, ramLoadCtrY, ramLoadBotY,
                                fontsize, settings.separatorColor);
                            int cx = dividerX + (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawString(std::string(mini_ram_temp_c), false, cx, ramLoadCtrY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        }
                    } else if (rSplitTemp) {
                        // Single rail + non-SBS temp: mirror STEMP bot row.
                        const char* rBotVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                        if (settings.voltageAtEndRAM) {
                            // VoltAtEnd: volt at bottom
                            if (rBotVolt[0]) {
                                int rx = afterBotX;
                                rlbDrawBotRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawStringWithColoredSections(std::string(rBotVolt), false,
                                    specialChars, rx, ramLoadBotY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        } else {
                            // Normal: temp at bottom
                            if (mini_ram_temp_c[0]) {
                                int rx = afterBotX;
                                rlbDrawBotRowDiv(rx);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                const int tv = atoi(mini_ram_temp_c);
                                renderer->drawString(std::string(mini_ram_temp_c), false, rx, ramLoadBotY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        }
                    } else {
                        // Inline mode: draw all volt/temp at ramLoadCtrY (centered between rows,
                        // inline with the "RAM" label) starting at ramLoadVoltColX.
                        const int yInline = ramLoadCtrY;
                        // Determine if any inline content will be drawn — if so, draw a 3-tall divider stack
                        // at ramLoadVoltColX (top row baseY, center Y, bot row Y) so the connector visually
                        // spans the full height of the 2-row data block on the left.
                        const bool anyInlineVolt = settings.realVolts && (vdd2_only_c[0] || vddq_only_c[0]);
                        const bool anyInlineTemp = settings.showRAMTemp && mini_ram_temp_c[0];
                        const bool willDrawInline = anyInlineVolt || anyInlineTemp;
                        // 3-tall divider stack scissoring: top + bot drawn here, center is drawn naturally
                        // as the FIRST inline DIV below at the same X (afterBotX==ramLoadVoltColX) at yInline.
                        // Compute scissor cuts once and flag the first inline DIV to be drawn scissored.
                        int rsbInlineCenterOff = 0, rsbInlineLeftPad = 0, rsbInlineFullW = 0;
                        int rsbInlineCutTC = 0, rsbInlineCutCB = 0;
                        bool rsbInlineFirstDivScissor = false;
                        if (willDrawInline) {
                            if (mini_divider_scissor::getDividerScissorMetrics(
                                    fontsize, rsbInlineCenterOff, rsbInlineLeftPad, rsbInlineFullW)) {
                                rsbInlineCutTC = mini_divider_scissor::computeDividerCutY(
                                    ramLoadTopBaseY, yInline, rsbInlineCenterOff);
                                rsbInlineCutCB = mini_divider_scissor::computeDividerCutY(
                                    yInline, ramLoadBotY, rsbInlineCenterOff);
                                mini_divider_scissor::drawDividerScissored(renderer,
                                    ramLoadVoltColX, ramLoadTopBaseY,
                                    mini_divider_scissor::kNoUpperCut, rsbInlineCutTC,
                                    fontsize, settings.separatorColor,
                                    rsbInlineLeftPad, rsbInlineFullW);
                                mini_divider_scissor::drawDividerScissored(renderer,
                                    ramLoadVoltColX, ramLoadBotY,
                                    rsbInlineCutCB, mini_divider_scissor::kNoLowerCut,
                                    fontsize, settings.separatorColor,
                                    rsbInlineLeftPad, rsbInlineFullW);
                                rsbInlineFirstDivScissor = true;
                            } else {
                                // Metrics unavailable — fallback to original unscissored draws.
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, ramLoadVoltColX, ramLoadTopBaseY,
                                    fontsize, settings.separatorColor);
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, ramLoadVoltColX, ramLoadBotY,
                                    fontsize, settings.separatorColor);
                            }
                        }
                        // Lambda for the first inline DIV at afterBotX==ramLoadVoltColX, yInline.
                        // Scissors that one draw to the center band of the 3-stack; for all
                        // subsequent inline DIVs (at different X positions) falls through to a
                        // plain unscissored draw. Returns the divider's text width like drawString.first.
                        auto drawInlineDivMaybeScissored = [&](int dx) -> int {
                            if (rsbInlineFirstDivScissor && dx == ramLoadVoltColX) {
                                rsbInlineFirstDivScissor = false; // only the first one needs scissor
                                mini_divider_scissor::drawDividerScissored(renderer,
                                    dx, yInline,
                                    rsbInlineCutTC, rsbInlineCutCB,
                                    fontsize, settings.separatorColor,
                                    rsbInlineLeftPad, rsbInlineFullW);
                                return (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            }
                            return (int)renderer->drawString(ult::DIVIDER_SYMBOL, false,
                                dx, yInline, fontsize, settings.separatorColor).first;
                        };
                        // Volt inline before temp (when voltageAtEnd is OFF)
                        if (!settings.voltageAtEndRAM && settings.realVolts && (vdd2_only_c[0] || vddq_only_c[0])) {
                            if (vdd2_only_c[0] && vddq_only_c[0]) {
                                // DIV + VDD2 + DIV + VDDQ (matches the SBS visualization with connector between rails)
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                                afterBotX += (int)renderer->getTextDimensions(std::string(vdd2_only_c), false, fontsize).first;
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(vddq_only_c), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                                afterBotX += (int)renderer->getTextDimensions(std::string(vddq_only_c), false, fontsize).first;
                            } else {
                                const char* voltStr = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(voltStr), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                                afterBotX += (int)renderer->getTextDimensions(std::string(voltStr), false, fontsize).first;
                            }
                        }
                        // Temp inline
                        if (settings.showRAMTemp && mini_ram_temp_c[0]) {
                            afterBotX += drawInlineDivMaybeScissored(afterBotX);
                            const int rtv = atoi(mini_ram_temp_c);
                            renderer->drawString(std::string(mini_ram_temp_c), false, afterBotX, yInline, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            afterBotX += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                        }
                        // Volt at end (when voltageAtEnd is ON)
                        if (settings.voltageAtEndRAM && settings.realVolts && (vdd2_only_c[0] || vddq_only_c[0])) {
                            if (vdd2_only_c[0] && vddq_only_c[0]) {
                                // Match default RAM renderer voltAtEnd path: DIV + VDD2 + DIV + VDDQ
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                                afterBotX += (int)renderer->getTextDimensions(std::string(vdd2_only_c), false, fontsize).first;
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(vddq_only_c), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                            } else {
                                const char* voltStr = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                                afterBotX += drawInlineDivMaybeScissored(afterBotX);
                                renderer->drawStringWithColoredSections(std::string(voltStr), false, specialChars,
                                    afterBotX, yInline, fontsize, settings.textColor, settings.separatorColor);
                            }
                        }
                    }
                    ramLoadStackCutCB = -1;
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_SVDD2") {
                    // Split VDD2/VDDQ row 1.
                    // Usage drawn at centre (between rows), VDD2 at baseY (top), mirroring TMP_SFAN single-group.
                    const int ramCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    // 3-stack scissoring: top=baseY, center=ramCenterY, bot=svddqY (drawn in RAM_SVDDQ).
                    // svddqY (predicted) = baseY_SVDDQ - spacing/2 = (baseY + fontsize + spacing) - spacing/2
                    //                    = baseY + fontsize + spacing/2.
                    const int ramSplitPredictedBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    bool ramSplitScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int ramSplitCutTC = 0;
                    int ramSplitCutCB = 0;
                    if (ramSplitScissorOK) {
                        ramSplitCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, ramCenterY, dividerCenterOffY);
                        ramSplitCutCB = mini_divider_scissor::computeDividerCutY(
                            ramCenterY, ramSplitPredictedBotY, dividerCenterOffY);
                    }
                    // Hand off bottom cut to RAM_SVDDQ via static var (consumed next iteration).
                    ramSplitStackCutCB = ramSplitScissorOK ? ramSplitCutCB : -1;

                    // Local lambdas: scissored DIV draws using the cuts above.
                    auto drawCenterDiv = [&](int dx) {
                        if (ramSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, ramCenterY,
                                ramSplitCutTC, ramSplitCutCB, fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, ramCenterY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    auto drawTopRowDiv = [&](int dx) {
                        if (ramSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, baseY,
                                mini_divider_scissor::kNoUpperCut, ramSplitCutTC,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, baseY,
                                fontsize, settings.separatorColor);
                        }
                    };

                    int currentX = baseX;
                    // Usage at centre Y
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars,
                        currentX, ramCenterY, fontsize, settings.textColor, settings.separatorColor);
                    currentX += (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    ramSplitVoltColX = currentX;
                    // "RAM" label at centre Y
                    if (settings.showLabels) {
                        const std::string ramLbl = "RAM";
                        const uint32_t ramLblW = renderer->getTextDimensions(ramLbl, false, fontsize).first;
                        const uint32_t ramLblX = cachedBaseX + (margin / 2) - (ramLblW / 2);
                        renderer->drawString(ramLbl, false, ramLblX + _frameOffsetX + clippingOffsetX,
                            ramCenterY, fontsize, settings.catColor);
                    }
                    // Divider at ramSplitVoltColX — the 3-stack column shared by TOP, CENTER, BOT.
                    // Cases:
                    //  • VoltAtEnd+!SBS (!showStackedRAMTemp): temp goes at center, no top content
                    //    after this divider on baseY — BUT it IS the true 3-stack column.
                    //    Previous code drew drawDividerStack3 (all 3 at once) here. Keep that.
                    //  • VoltAtEnd+SBS (showStackedRAMTemp): temp goes on baseY (top row) starting
                    //    ramSplitVoltColX+dw. So this X has TOP content (temp) AND CENTER (usage)
                    //    AND BOT (vddq). Draw top+center here; bot is handled by RAM_SVDDQ.
                    //  • All other cases: only center is relevant here; top/bot drawn elsewhere.
                    if (settings.showRAMTemp && mini_ram_temp_c[0] &&
                        !settings.showStackedRAMTemp && settings.voltageAtEndRAM) {
                        // VoltAtEnd+!SBS: no top-row content at this X — draw full spine.
                        mini_divider_scissor::drawDividerStack3(renderer, ramSplitVoltColX,
                            baseY, ramCenterY, ramSplitPredictedBotY,
                            fontsize, settings.separatorColor);
                    } else if (settings.showRAMTemp && mini_ram_temp_c[0] &&
                               settings.showStackedRAMTemp && settings.voltageAtEndRAM) {
                        // VoltAtEnd+SBS: temp on top row → true 3-stack here; draw top+center.
                        // Bot is scissored in RAM_SVDDQ via ramSplitStackCutCB.
                        drawTopRowDiv(ramSplitVoltColX);
                        drawCenterDiv(ramSplitVoltColX);
                    } else {
                        drawCenterDiv(ramSplitVoltColX);
                    }
                    // SBS RAM temp + VoltAtEnd logic for RAM_SVDD2 (top row):
                    // Normal SBS:    temp after max(vdd2,vddq) at center
                    // VoltAtEnd+SBS: left-connector IS divider before temp; temp at center;
                    //                right-fork connector after temp; VDD2/VDDQ start there
                    // VoltAtEnd+!SBS: draw temp+DIV before VDD2 on top row (baseY)
                    int vdd2StartX = ramSplitVoltColX; // X where VDD2 volt column starts
                    if (settings.showRAMTemp && mini_ram_temp_c[0]) {
                        const uint32_t sbs_dw = (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        const int rtv = atoi(mini_ram_temp_c);
                        if (!settings.showStackedRAMTemp) {
                            if (settings.voltageAtEndRAM) {
                                // VoltAtEnd+SBS: temp at center starting at ramSplitVoltColX+sbs_dw
                                // (left connector already drawn is the divider before temp)
                                int cx = ramSplitVoltColX + sbs_dw;
                                renderer->drawString(std::string(mini_ram_temp_c), false, cx, ramCenterY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                                cx += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                                // Right-fork connector before volt column (scissored — part of the new 3-stack at new column X)
                                drawCenterDiv(cx);
                                vdd2StartX = cx; // VDD2/VDDQ start at the right-fork connector X
                            } else {
                                // Normal SBS: temp after max(vdd2,vddq)
                                const uint32_t vdd2_rw = vdd2_only_c[0] ? (int)renderer->getTextDimensions(vdd2_only_c, false, fontsize).first : 0;
                                const uint32_t vddq_rw = vddq_only_c[0] ? (int)renderer->getTextDimensions(vddq_only_c, false, fontsize).first : 0;
                                int cx = ramSplitVoltColX + sbs_dw + std::max(vdd2_rw, vddq_rw);
                                // Center DIV before temp at cx — isolated (no top/bot at this X),
                                // draw plain/unscissored so its full glyph is visible.
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, ramCenterY,
                                    fontsize, settings.separatorColor);
                                cx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawString(std::string(mini_ram_temp_c), false, cx, ramCenterY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        } else if (settings.voltageAtEndRAM) {
                            // VoltAtEnd+!SBS: draw temp+DIVIDER before VDD2 on the top row (baseY)
                            // This is handled here so RAM_SVDDQ doesn't draw it (we block that below via ramSplitVoltColX update)
                            if (vdd2_only_c[0]) {
                                int rx = ramSplitVoltColX + sbs_dw;
                                renderer->drawString(std::string(mini_ram_temp_c), false, rx, baseY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                                rx += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                                // DIV between temp and VDD2 — different X from the 3-stack column
                                // (ramSplitVoltColX); not part of the 3-stack. Draw plain/unscissored.
                                renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)rx, (float)baseY,
                                    fontsize, settings.separatorColor);
                                rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                            // Skip the normal VDD2 draw below by setting vdd2StartX sentinel
                            vdd2StartX = -1;
                        }
                    }
                    // Update shared column X so RAM_SVDDQ aligns with VDD2
                    if (settings.voltageAtEndRAM && settings.showRAMTemp && !settings.showStackedRAMTemp)
                        ramSplitVoltColX = vdd2StartX;
                    // VDD2 at top row (baseY) — skip if VoltAtEnd+!SBS (already drawn above)
                    if (vdd2_only_c[0] && vdd2StartX != -1 && !(settings.voltageAtEndRAM && settings.showRAMTemp && mini_ram_temp_c[0] && settings.showStackedRAMTemp)) {
                        int rx = vdd2StartX;
                        // Top-row DIV before VDD2 — scissored to the top band of the 3-stack.
                        drawTopRowDiv(rx);
                        rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false,
                            specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_SVDDQ") {
                    // Split VDD2/VDDQ row 2: VDDQ at the volt column; uses spacing/2 compression.
                    const int svddqY = baseY - (int)settings.spacing / 2;
                    // 3-stack scissoring: this is the BOT row of the SVDD2/SVDDQ stack.
                    // ramSplitStackCutCB was written by RAM_SVDD2 on the previous iteration.
                    const bool svddqScissorOK = (ramSplitStackCutCB >= 0);
                    auto drawBotRowDiv = [&](int dx) {
                        if (svddqScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, svddqY,
                                ramSplitStackCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, svddqY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    if (vddq_only_c[0]) {
                        int rx = ramSplitVoltColX;
                        // Bot-row DIV before VDDQ — scissored to the bottom band of the 3-stack.
                        drawBotRowDiv(rx);
                        rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        renderer->drawStringWithColoredSections(std::string(vddq_only_c), false,
                            specialChars, rx, svddqY, fontsize, settings.textColor, settings.separatorColor);
                        // non-SBS: RAM temp appended after VDDQ at bottom row (skip when VoltAtEnd — temp is drawn on top row instead)
                        if (settings.showRAMTemp && settings.showStackedRAMTemp && !settings.voltageAtEndRAM && mini_ram_temp_c[0]) {
                            rx += (int)renderer->getTextDimensions(std::string(vddq_only_c), false, fontsize).first;
                            // Div before temp — different X from 3-stack column (ramSplitVoltColX) → unscissored.
                            rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, svddqY,
                                fontsize, settings.separatorColor).first;
                            const int rtv = atoi(mini_ram_temp_c);
                            renderer->drawString(std::string(mini_ram_temp_c), false, rx, svddqY, fontsize,
                                settings.useDynamicColors
                                    ? tsl::GradientColor((float)rtv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                    : settings.textColor);

                        }
                    }
                    // Cut consumed — clear it so a stale value doesn't leak into a future frame
                    // where SVDD2 might not run before SVDDQ (defensive; current label order makes this safe anyway).
                    ramSplitStackCutCB = -1;
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "CPU_SVOLT") {
                    // CPU split row 1: data centered; normal=volt at top, voltAtEnd=temp at top.
                    const int cpuCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    // 3-stack scissoring: top=baseY, center=cpuCenterY, bot=stempY (drawn in CPU_STEMP).
                    // stempY (predicted) = baseY_STEMP - spacing/2 = baseY + fontsize + spacing/2.
                    const int cpuSplitPredictedBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    bool cpuSplitScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int cpuSplitCutTC = 0;
                    int cpuSplitCutCB = 0;
                    if (cpuSplitScissorOK) {
                        cpuSplitCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, cpuCenterY, dividerCenterOffY);
                        cpuSplitCutCB = mini_divider_scissor::computeDividerCutY(
                            cpuCenterY, cpuSplitPredictedBotY, dividerCenterOffY);
                    }
                    cpuSplitStackCutCB = cpuSplitScissorOK ? cpuSplitCutCB : -1;

                    auto cpuDrawCenterDiv = [&](int dx) {
                        if (cpuSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, cpuCenterY,
                                cpuSplitCutTC, cpuSplitCutCB, fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, cpuCenterY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    auto cpuDrawTopRowDiv = [&](int dx) {
                        if (cpuSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, baseY,
                                mini_divider_scissor::kNoUpperCut, cpuSplitCutTC,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, baseY,
                                fontsize, settings.separatorColor);
                        }
                    };

                    int currentX = baseX;
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars,
                        currentX, cpuCenterY, fontsize, settings.textColor, settings.separatorColor);
                    currentX += (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    cpuSplitVoltColX = currentX;
                    if (settings.showLabels) {
                        const std::string cpuLbl = "CPU";
                        const uint32_t cpuLblW = renderer->getTextDimensions(cpuLbl, false, fontsize).first;
                        const uint32_t cpuLblX = cachedBaseX + (margin / 2) - (cpuLblW / 2);
                        renderer->drawString(cpuLbl, false, cpuLblX + _frameOffsetX + clippingOffsetX,
                            cpuCenterY, fontsize, settings.catColor);
                    }
                    // Centre connector divider — scissored to the center band of the 3-stack.
                    cpuDrawCenterDiv(cpuSplitVoltColX);
                    if (settings.voltageAtEndCPU) {
                        // VoltAtEnd: temp at top row
                        if (mini_cpu_temp_c[0]) {
                            int rx = cpuSplitVoltColX;
                            // Top-row DIV before temp — scissored to top band.
                            cpuDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_cpu_temp_c);
                            renderer->drawString(std::string(mini_cpu_temp_c), false, rx, baseY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        }
                    } else {
                        // Normal: volt at top row
                        if (settings.realVolts && MINI_CPU_volt_c[0]) {
                            int rx = cpuSplitVoltColX;
                            // Top-row DIV before volt — scissored to top band.
                            cpuDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "CPU_STEMP") {
                    // CPU split row 2: normal=temp at bottom; voltAtEnd=volt at bottom.
                    const int stempY = baseY - (int)settings.spacing / 2;
                    // 3-stack scissoring: this is the BOT row. cpuSplitStackCutCB was written by CPU_SVOLT.
                    const bool cpuStempScissorOK = (cpuSplitStackCutCB >= 0);
                    auto cpuDrawBotRowDiv = [&](int dx) {
                        if (cpuStempScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, stempY,
                                cpuSplitStackCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, stempY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    int rx = cpuSplitVoltColX ? cpuSplitVoltColX : baseX;
                    if (settings.voltageAtEndCPU) {
                        if (settings.realVolts && MINI_CPU_volt_c[0]) {
                            // Bot-row DIV before volt — scissored to bottom band.
                            cpuDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                specialChars, rx, stempY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    } else {
                        if (mini_cpu_temp_c[0]) {
                            // Bot-row DIV before temp — scissored to bottom band.
                            cpuDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_cpu_temp_c);
                            renderer->drawString(std::string(mini_cpu_temp_c), false, rx, stempY, fontsize,
                                settings.useDynamicColors
                                    ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                    : settings.textColor);
                        }
                    }
                    cpuSplitStackCutCB = -1;
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "CPU_SFULL") {
                    // Full CPU freq split — row 1.
                    // cpuFullStacked=true (SBS-cpu-temp OFF + realVolts + showCPUTemp):
                    //   mirrors CPU_SVOLT exactly — brackets on center row, volt (or temp if voltAtEnd) at baseY top.
                    // cpuFullStacked=false: brackets on baseY (top), volt+temp inline after brackets.
                    const bool cpuFullStacked = settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                    const int cpuFullCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    cpuFullSplitBracketsW = (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    // Column width = whichever line is longer: brackets or the freq line below.
                    // This ensures both rows are right-aligned to the same edge so neither overflows
                    // left into the CPU label area and the divider column stays cleanly to the right.
                    {
                        const int freqMeasW = MINI_CPU_freq_c[0]
                            ? (int)renderer->getTextDimensions(std::string(MINI_CPU_freq_c), false, fontsize).first
                            : 0;
                        cpuFullSplitColW = std::max(cpuFullSplitBracketsW, freqMeasW);
                    }
                    // X offset so that the shorter line is pushed right to share the same right edge.
                    const int bracketsOffsetX = cpuFullSplitColW - cpuFullSplitBracketsW;
                    const std::string cpuLbl = "CPU";
                    const uint32_t cpuLblW = renderer->getTextDimensions(cpuLbl, false, fontsize).first;
                    const uint32_t cpuLblX = cachedBaseX + (margin / 2) - (cpuLblW / 2);
                    if (cpuFullStacked) {
                        // ── Stacked mode: brackets on TOP row (baseY), volt inline after brackets.
                        // CPU_SFREQ (bottom row) draws @freq right-aligned + temp right of volt column.
                        // Brackets are right-aligned within cpuFullSplitColW so both rows share the same right edge.
                        int currentX = baseX + bracketsOffsetX;
                        renderer->drawStringWithColoredSections(currentLine, false, cpuPadChars,
                            currentX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                        currentX = baseX + cpuFullSplitColW; // volt column starts at shared right edge
                        // Center connector divider — connects top-row | and bottom-row | into one vertical bar
                        renderer->drawString(ult::DIVIDER_SYMBOL, false, currentX, cpuFullCenterY,
                            fontsize, settings.separatorColor);
                        if (settings.showLabels)
                            renderer->drawString(cpuLbl, false, cpuLblX + _frameOffsetX + clippingOffsetX,
                                cpuFullCenterY, fontsize, settings.catColor);
                        // Draw volt (normal) or temp (voltAtEnd) inline after brackets on top row
                        if (settings.voltageAtEndCPU) {
                            if (mini_cpu_temp_c[0]) {
                                int rx = currentX;
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, baseY,
                                    fontsize, settings.separatorColor).first;
                                const int tv = atoi(mini_cpu_temp_c);
                                renderer->drawString(std::string(mini_cpu_temp_c), false, rx, baseY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        } else {
                            if (settings.realVolts && MINI_CPU_volt_c[0]) {
                                int rx = currentX;
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, baseY,
                                    fontsize, settings.separatorColor).first;
                                renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                    specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        }
                    } else {
                        // ── Inline mode: brackets on baseY (top row), volt+temp on cpuFullCenterY (center row) ──
                        // Brackets are right-aligned within cpuFullSplitColW; the divider/volt column sits at
                        // baseX + cpuFullSplitColW so it never overlaps with either line's text.
                        renderer->drawStringWithColoredSections(currentLine, false, cpuPadChars,
                            baseX + bracketsOffsetX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                        // 3-tall divider stack at the shared right edge of the column.
                        // Top and bottom extensions are only drawn when volt/temp content follows them.
                        const int voltColX = baseX + cpuFullSplitColW;
                        const bool willDrawAnyCpu = (settings.realVolts && MINI_CPU_volt_c[0]) ||
                                                     mini_cpu_temp_c[0];
                        if (willDrawAnyCpu) {
                            const int cpuFullBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                            // 3-divider stack: top (baseY), center (cpuFullCenterY), bot (cpuFullBotY).
                            // Scissored so the three glyphs meet pixel-perfect without alpha-blend overlap.
                            mini_divider_scissor::drawDividerStack3(renderer, voltColX,
                                baseY, cpuFullCenterY, cpuFullBotY,
                                fontsize, settings.separatorColor);
                        } else {
                            // No volt/temp content — center connector only; unscissored (isolated).
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, voltColX, cpuFullCenterY,
                                fontsize, settings.separatorColor);
                        }
                        if (settings.showLabels)
                            renderer->drawString(cpuLbl, false, cpuLblX + _frameOffsetX + clippingOffsetX,
                                cpuFullCenterY, fontsize, settings.catColor);
                        // Volt/temp on center row (cpuFullCenterY), right of volt column.
                        // The 3-stack already drew a divider at (voltColX, cpuFullCenterY), so the FIRST
                        // inline item draws directly at rx = voltColX + divider_width with no leading divider
                        // (otherwise it would stack on top of the center stack divider). Subsequent items
                        // get a normal plain divider separator between them.
                        const int dividerW = (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                        int rx = willDrawAnyCpu ? (voltColX + dividerW) : voltColX;
                        bool cpuInlineDrewFirst = false;
                        if (settings.voltageAtEndCPU) {
                            if (mini_cpu_temp_c[0]) {
                                if (cpuInlineDrewFirst) {
                                    rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuFullCenterY,
                                        fontsize, settings.separatorColor).first;
                                }
                                const int tv = atoi(mini_cpu_temp_c);
                                const std::string tempStr(mini_cpu_temp_c);
                                renderer->drawString(tempStr, false, rx, cpuFullCenterY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                                rx += (int)renderer->getTextDimensions(tempStr, false, fontsize).first;
                                cpuInlineDrewFirst = true;
                            }
                            if (settings.realVolts && MINI_CPU_volt_c[0]) {
                                if (cpuInlineDrewFirst) {
                                    rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuFullCenterY,
                                        fontsize, settings.separatorColor).first;
                                }
                                renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                    specialChars, rx, cpuFullCenterY, fontsize, settings.textColor, settings.separatorColor);
                                rx += (int)renderer->getTextDimensions(std::string(MINI_CPU_volt_c), false, fontsize).first;
                                cpuInlineDrewFirst = true;
                            }
                        } else {
                            if (settings.realVolts && MINI_CPU_volt_c[0]) {
                                if (cpuInlineDrewFirst) {
                                    rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuFullCenterY,
                                        fontsize, settings.separatorColor).first;
                                }
                                renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                    specialChars, rx, cpuFullCenterY, fontsize, settings.textColor, settings.separatorColor);
                                rx += (int)renderer->getTextDimensions(std::string(MINI_CPU_volt_c), false, fontsize).first;
                                cpuInlineDrewFirst = true;
                            }
                            if (mini_cpu_temp_c[0]) {
                                if (cpuInlineDrewFirst) {
                                    rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, cpuFullCenterY,
                                        fontsize, settings.separatorColor).first;
                                }
                                const int tv = atoi(mini_cpu_temp_c);
                                renderer->drawString(std::string(mini_cpu_temp_c), false, rx, cpuFullCenterY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                                rx += (int)renderer->getTextDimensions(std::string(mini_cpu_temp_c), false, fontsize).first;
                                cpuInlineDrewFirst = true;
                            }
                        }
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "CPU_SFREQ") {
                    // Full CPU freq split — row 2.
                    // Both the brackets above (CPU_SFULL) and this freq line are right-aligned to
                    // cpuFullSplitColW — the width of whichever of the two is longer.
                    // This means when freq is wider, it stays at baseX and brackets were pushed right;
                    // when brackets are wider, freq is pushed right and brackets stay at baseX.
                    // Either way the divider / volt column sits cleanly at baseX + cpuFullSplitColW.
                    const bool cpuFullStacked = settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                    const int freqY = baseY - (int)settings.spacing / 2;
                    const int freqW = currentLine.empty() ? 0 : (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    // Right-align freq to the shared column right edge.
                    const int freqX = baseX + cpuFullSplitColW - freqW;
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars,
                        (uint32_t)std::max(baseX, freqX), freqY, fontsize,
                        settings.textColor, settings.separatorColor);
                    // Stacked mode: draw bottom-row volt/temp to the right of the shared column edge.
                    if (cpuFullStacked) {
                        int rx = baseX + cpuFullSplitColW; // column right edge = shared boundary
                        if (settings.voltageAtEndCPU) {
                            // voltAtEnd: volt on bottom row
                            if (settings.realVolts && MINI_CPU_volt_c[0]) {
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, freqY,
                                    fontsize, settings.separatorColor).first;
                                renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false,
                                    specialChars, rx, freqY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        } else {
                            // Normal: temp on bottom row
                            if (mini_cpu_temp_c[0]) {
                                rx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, rx, freqY,
                                    fontsize, settings.separatorColor).first;
                                const int tv = atoi(mini_cpu_temp_c);
                                renderer->drawString(std::string(mini_cpu_temp_c), false, rx, freqY, fontsize,
                                    settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                            }
                        }
                    }
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "GPU_SVOLT") {
                    // GPU split row 1: normal=volt at top; voltAtEnd=temp at top.
                    const int gpuCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    // 3-stack scissoring: top=baseY, center=gpuCenterY, bot=stempY (drawn in GPU_STEMP).
                    const int gpuSplitPredictedBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    bool gpuSplitScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int gpuSplitCutTC = 0;
                    int gpuSplitCutCB = 0;
                    if (gpuSplitScissorOK) {
                        gpuSplitCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, gpuCenterY, dividerCenterOffY);
                        gpuSplitCutCB = mini_divider_scissor::computeDividerCutY(
                            gpuCenterY, gpuSplitPredictedBotY, dividerCenterOffY);
                    }
                    gpuSplitStackCutCB = gpuSplitScissorOK ? gpuSplitCutCB : -1;

                    auto gpuDrawCenterDiv = [&](int dx) {
                        if (gpuSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, gpuCenterY,
                                gpuSplitCutTC, gpuSplitCutCB, fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, gpuCenterY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    auto gpuDrawTopRowDiv = [&](int dx) {
                        if (gpuSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, baseY,
                                mini_divider_scissor::kNoUpperCut, gpuSplitCutTC,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, baseY,
                                fontsize, settings.separatorColor);
                        }
                    };

                    int currentX = baseX;
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars,
                        currentX, gpuCenterY, fontsize, settings.textColor, settings.separatorColor);
                    currentX += (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    gpuSplitVoltColX = currentX;
                    if (settings.showLabels) {
                        const std::string gpuLbl = "GPU";
                        const uint32_t gpuLblW = renderer->getTextDimensions(gpuLbl, false, fontsize).first;
                        const uint32_t gpuLblX = cachedBaseX + (margin / 2) - (gpuLblW / 2);
                        renderer->drawString(gpuLbl, false, gpuLblX + _frameOffsetX + clippingOffsetX,
                            gpuCenterY, fontsize, settings.catColor);
                    }
                    // Centre connector divider — scissored to center band.
                    gpuDrawCenterDiv(gpuSplitVoltColX);
                    if (settings.voltageAtEndGPU) {
                        if (mini_gpu_temp_c[0]) {
                            int rx = gpuSplitVoltColX;
                            gpuDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_gpu_temp_c);
                            renderer->drawString(std::string(mini_gpu_temp_c), false, rx, baseY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        }
                    } else {
                        if (settings.realVolts && MINI_GPU_volt_c[0]) {
                            int rx = gpuSplitVoltColX;
                            gpuDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_GPU_volt_c), false,
                                specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "GPU_STEMP") {
                    // GPU split row 2: normal=temp at bottom; voltAtEnd=volt at bottom.
                    const int stempY = baseY - (int)settings.spacing / 2;
                    // 3-stack scissoring: this is the BOT row. gpuSplitStackCutCB was written by GPU_SVOLT.
                    const bool gpuStempScissorOK = (gpuSplitStackCutCB >= 0);
                    auto gpuDrawBotRowDiv = [&](int dx) {
                        if (gpuStempScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, stempY,
                                gpuSplitStackCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, stempY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    int rx = gpuSplitVoltColX ? gpuSplitVoltColX : baseX;
                    if (settings.voltageAtEndGPU) {
                        if (settings.realVolts && MINI_GPU_volt_c[0]) {
                            gpuDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_GPU_volt_c), false,
                                specialChars, rx, stempY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    } else {
                        if (mini_gpu_temp_c[0]) {
                            gpuDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_gpu_temp_c);
                            renderer->drawString(std::string(mini_gpu_temp_c), false, rx, stempY, fontsize,
                                settings.useDynamicColors
                                    ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                    : settings.textColor);
                        }
                    }
                    gpuSplitStackCutCB = -1;
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_SVDDQ_ONLY") {
                    // RAM temp split row 1 (single volt rail): data centered, volt at top (baseY).
                    const int ramTCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    // 3-stack scissoring: top=baseY, center=ramTCenterY, bot=stempY (drawn in RAM_STEMP).
                    const int ramTSplitPredictedBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    bool ramTSplitScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int ramTSplitCutTC = 0;
                    int ramTSplitCutCB = 0;
                    if (ramTSplitScissorOK) {
                        ramTSplitCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, ramTCenterY, dividerCenterOffY);
                        ramTSplitCutCB = mini_divider_scissor::computeDividerCutY(
                            ramTCenterY, ramTSplitPredictedBotY, dividerCenterOffY);
                    }
                    ramTempSplitStackCutCB = ramTSplitScissorOK ? ramTSplitCutCB : -1;

                    auto rtDrawCenterDiv = [&](int dx) {
                        if (ramTSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, ramTCenterY,
                                ramTSplitCutTC, ramTSplitCutCB, fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, ramTCenterY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    auto rtDrawTopRowDiv = [&](int dx) {
                        if (ramTSplitScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, baseY,
                                mini_divider_scissor::kNoUpperCut, ramTSplitCutTC,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, baseY,
                                fontsize, settings.separatorColor);
                        }
                    };

                    int currentX = baseX;
                    // RAM data at center Y
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars,
                        currentX, ramTCenterY, fontsize, settings.textColor, settings.separatorColor);
                    currentX += (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    ramTempVoltColX = currentX;
                    // RAM label centered (if showLabels)
                    if (settings.showLabels) {
                        const std::string ramLbl = "RAM";
                        const uint32_t ramLblW = renderer->getTextDimensions(ramLbl, false, fontsize).first;
                        const uint32_t ramLblX = cachedBaseX + (margin / 2) - (ramLblW / 2);
                        renderer->drawString(ramLbl, false, ramLblX + _frameOffsetX + clippingOffsetX,
                            ramTCenterY, fontsize, settings.catColor);
                    }
                    // Centre connector divider — scissored to center band.
                    rtDrawCenterDiv(ramTempVoltColX);
                    const char* ramSplitTopVolt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                    if (settings.voltageAtEndRAM) {
                        // VoltAtEnd: temp at top row
                        if (mini_ram_temp_c[0]) {
                            int rx = ramTempVoltColX;
                            rtDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_ram_temp_c);
                            renderer->drawString(std::string(mini_ram_temp_c), false, rx, baseY, fontsize,
                                settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        }
                    } else {
                        // Normal: volt at top row
                        if (ramSplitTopVolt[0]) {
                            int rx = ramTempVoltColX;
                            rtDrawTopRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(ramSplitTopVolt), false,
                                specialChars, rx, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM_STEMP") {
                    // RAM temp split row 2: normal=temp at bottom; voltAtEnd=volt at bottom.
                    const int stempY = baseY - (int)settings.spacing / 2;
                    // 3-stack scissoring: BOT row. ramTempSplitStackCutCB was written by RAM_SVDDQ_ONLY.
                    const bool ramStempScissorOK = (ramTempSplitStackCutCB >= 0);
                    auto rtDrawBotRowDiv = [&](int dx) {
                        if (ramStempScissorOK) {
                            mini_divider_scissor::drawDividerScissored(renderer, dx, stempY,
                                ramTempSplitStackCutCB, mini_divider_scissor::kNoLowerCut,
                                fontsize, settings.separatorColor,
                                dividerLeftPad, dividerFullW);
                        } else {
                            renderer->drawString(ult::DIVIDER_SYMBOL, false, dx, stempY,
                                fontsize, settings.separatorColor);
                        }
                    };
                    int rx = ramTempVoltColX ? ramTempVoltColX : baseX;
                    const char* rts2Volt = vddq_only_c[0] ? vddq_only_c : vdd2_only_c;
                    if (settings.voltageAtEndRAM) {
                        if (rts2Volt[0]) {
                            rtDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            renderer->drawStringWithColoredSections(std::string(rts2Volt), false,
                                specialChars, rx, stempY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    } else {
                        if (mini_ram_temp_c[0]) {
                            rtDrawBotRowDiv(rx);
                            rx += (int)renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                            const int tv = atoi(mini_ram_temp_c);
                            renderer->drawString(std::string(mini_ram_temp_c), false, rx, stempY, fontsize,
                                settings.useDynamicColors
                                    ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                    : settings.textColor);
                        }
                    }
                    ramTempSplitStackCutCB = -1;
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "BAT_STOP") {
                    // BAT split row 1: top row. "BAT" label centered between both rows.
                    const int batCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    if (settings.showLabels) {
                        const std::string batLbl = "BAT";
                        const uint32_t batLblW = renderer->getTextDimensions(batLbl, false, fontsize).first;
                        const uint32_t batLblX = cachedBaseX + (margin / 2) - (batLblW / 2);
                        renderer->drawString(batLbl, false, batLblX + _frameOffsetX + clippingOffsetX,
                            batCenterY, fontsize, settings.catColor);
                    }
                    // Right-align both rows within max(topW, botW) so the shorter line
                    // aligns to the same right edge as the wider line (mirrors CPU/RAM behaviour).
                    {
                        const char* botStr = settings.invertBatteryDisplay ? Battery_draw_c : Battery_pct_c;
                        const uint32_t topW = renderer->getTextDimensions(currentLine, false, fontsize).first;
                        const uint32_t botW = botStr[0] ? renderer->getTextDimensions(std::string(botStr), false, fontsize).first : 0;
                        const int batColW = (int)std::max(topW, botW);
                        const int topDrawX = baseX + batColW - (int)topW;
                        renderer->drawString(currentLine, false, topDrawX, baseY, fontsize, settings.textColor);
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "BAT_SBOT") {
                    // BAT split row 2: bottom row, right-aligned to match top row.
                    const int batBotY = baseY - (int)settings.spacing / 2;
                    {
                        const char* topStr = settings.invertBatteryDisplay ? Battery_pct_c : Battery_c;
                        const uint32_t topW = topStr[0] ? renderer->getTextDimensions(std::string(topStr), false, fontsize).first : 0;
                        const uint32_t botW = renderer->getTextDimensions(currentLine, false, fontsize).first;
                        const int batColW = (int)std::max(topW, botW);
                        const int botDrawX = baseX + batColW - (int)botW;
                        renderer->drawString(currentLine, false, botDrawX, batBotY, fontsize, settings.textColor);
                    }
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "DTC_STOP") {
                    // DTC split row 1: top half (text before DIVIDER_SYMBOL in user format).
                    // DTC label centered between both rows. The bottom row is sourced from the
                    // next variable line (_variableLines[i+1]) so we can right-align both halves
                    // within max(topW, botW) — shorter half right-aligns to the wider edge.
                    const int dtcCenterY = baseY + ((int)fontsize + (int)settings.spacing / 2) / 2;
                    if (settings.showLabels) {
                        const std::string dtcLbl = settings.useDTCSymbol ? "\uE007" : "DTC";
                        const uint32_t dtcLblW = renderer->getTextDimensions(dtcLbl, false, fontsize).first;
                        const uint32_t dtcLblX = cachedBaseX + (margin / 2) - (dtcLblW / 2);
                        renderer->drawString(dtcLbl, false, dtcLblX + _frameOffsetX + clippingOffsetX,
                            dtcCenterY, fontsize, settings.catColor);
                    }
                    {
                        const std::string botStr = (i + 1 < _variableLines.size()) ? _variableLines[i + 1] : std::string();
                        const uint32_t topW = renderer->getTextDimensions(currentLine, false, fontsize).first;
                        const uint32_t botW = botStr.empty() ? 0 : renderer->getTextDimensions(botStr, false, fontsize).first;
                        const int dtcColW = (int)std::max(topW, botW);
                        const int topDrawX = baseX + dtcColW - (int)topW;
                        renderer->drawString(currentLine, false, topDrawX, baseY, fontsize, settings.textColor);
                    }

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "DTC_SBOT") {
                    // DTC split row 2: bottom half (text after DIVIDER_SYMBOL), right-aligned to match top.
                    const int dtcBotY = baseY - (int)settings.spacing / 2;
                    {
                        const std::string topStr = (i >= 1) ? _variableLines[i - 1] : std::string();
                        const uint32_t topW = topStr.empty() ? 0 : renderer->getTextDimensions(topStr, false, fontsize).first;
                        const uint32_t botW = renderer->getTextDimensions(currentLine, false, fontsize).first;
                        const int dtcColW = (int)std::max(topW, botW);
                        const int botDrawX = baseX + dtcColW - (int)botW;
                        renderer->drawString(currentLine, false, botDrawX, dtcBotY, fontsize, settings.textColor);
                    }
                    currentY -= (int)settings.spacing / 2;

                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP_SFAN") {
                    // Split fan row: temps + fan on row 1; "TMP" label drawn manually.
                    // Dual-row split: temps at baseY (top row), label centered between both rows. (top row), label centered between both rows.
                    // Single-group split: temps+label centered in the 2-row block; fan at baseY (top row).
                    const bool sfanIsDual = settings.showComponentTemps && settings.showSocPcbSkinTemps;
                    const bool sfanHighGrad = settings.showComponentTemps;
                    const int sfanTempY = sfanIsDual
                        ? baseY
                        : (baseY + ((int)fontsize + (int)settings.spacing / 2) / 2);
                    const int sfanLabelY = sfanIsDual
                        ? (baseY + ((int)fontsize + (int)settings.spacing / 2) / 2)
                        : sfanTempY;
                    int currentX = baseX;
                    size_t pos = 0;
                    bool parseSuccess = true;
                    for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                        while (pos < currentLine.length() && currentLine[pos] == ' ') {
                            renderer->drawString(" ", false, currentX, sfanTempY, fontsize, settings.textColor);
                            currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                            pos++;
                        }
                        if (pos >= currentLine.length()) break;
                        const size_t degreesPos = currentLine.find("\u00B0", pos);
                        if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                        const size_t cPos = currentLine.find("C", degreesPos);
                        if (cPos == std::string::npos) { parseSuccess = false; break; }
                        const std::string tempPart = currentLine.substr(pos, cPos + 1 - pos);
                        const int temp = atoi(tempPart.c_str());
                        renderer->drawString(tempPart, false, currentX, sfanTempY, fontsize,
                            settings.useDynamicColors
                                ? (sfanHighGrad
                                    ? tsl::GradientColor((float)temp, tsl::DEFAULT_TEMP_RANGE_HIGH)
                                    : tsl::GradientColor((float)temp))
                                : settings.textColor);
                        currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                        pos = cPos + 1;
                    }
                    if (!parseSuccess) {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, sfanTempY, fontsize, settings.textColor, settings.separatorColor);
                        currentX = baseX + (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    }
                    // Draw "TMP" label centered (isTmpDualRow skips generic label draw)
                    if (settings.showLabels) {
                        const std::string tmpLbl = "TMP";
                        const uint32_t tmpLblW = renderer->getTextDimensions(tmpLbl, false, fontsize).first;
                        const uint32_t tmpLblX = cachedBaseX + (margin / 2) - (tmpLblW / 2);
                        renderer->drawString(tmpLbl, false, tmpLblX + _frameOffsetX + clippingOffsetX,
                            sfanLabelY, fontsize, settings.catColor);
                    }
                    // Record where the fan column starts (used by TMP_SVOLT for single-group volt X alignment)
                    sfanFanColX = currentX;
                    sfanBaseY = baseY;  // store top-row Y for TMP_SVOLT volt-swap

                    // ── Scissored 3-stack at sfanFanColX ─────────────────────────────────
                    // Three dividers share sfanFanColX across TMP_SFAN and TMP_SVOLT:
                    //   voltAtEnd ON : fan=TOP(baseY), center=sfanCenterY, volt=BOT(svoltY)
                    //   voltAtEnd OFF: volt=TOP(baseY), center=sfanCenterY, fan=BOT(botY)
                    // Cuts are computed here; sfanStackCutCB/TC are handed to TMP_SVOLT.
                    const int sfanCenterY  = sfanIsDual ? sfanLabelY : sfanTempY;
                    const int sfanPredBotY = baseY + (int)fontsize + (int)settings.spacing / 2;
                    const bool sfanScissorOK = mini_divider_scissor::getDividerScissorMetrics(
                        fontsize, dividerCenterOffY, dividerLeftPad, dividerFullW);
                    int sfanCutTC = 0, sfanCutCB = 0;
                    if (sfanScissorOK) {
                        sfanCutTC = mini_divider_scissor::computeDividerCutY(
                            baseY, sfanCenterY, dividerCenterOffY);
                        sfanCutCB = mini_divider_scissor::computeDividerCutY(
                            sfanCenterY, sfanPredBotY, dividerCenterOffY);
                    }
                    sfanStackCutTC = sfanScissorOK ? sfanCutTC : -1;
                    sfanStackCutCB = sfanScissorOK ? sfanCutCB : -1;

                    // Center connector divider (scissored to center band)
                    if (sfanScissorOK) {
                        mini_divider_scissor::drawDividerScissored(renderer, sfanFanColX, sfanCenterY,
                            sfanCutTC, sfanCutCB, fontsize, settings.separatorColor,
                            dividerLeftPad, dividerFullW);
                    } else {
                        renderer->drawString(ult::DIVIDER_SYMBOL, false, (float)sfanFanColX,
                            (float)sfanCenterY, fontsize, settings.separatorColor);
                    }

                    // Draw fan: voltAtEnd ON=TOP(baseY), voltAtEnd OFF=BOT(botY)
                    if (settings.showFanPercentage) {
                        const int fanDuty = safeFanDuty((int)Rotation_Duty);
                        const int fanDrawY = settings.voltageAtEndTMP
                            ? baseY
                            : (baseY + (int)fontsize + (int)settings.spacing / 2);
                        // Fan is TOP when voltAtEnd ON, BOT when voltAtEnd OFF
                        int afterDivX;
                        if (sfanScissorOK) {
                            if (settings.voltageAtEndTMP) {
                                mini_divider_scissor::drawDividerScissored(renderer, currentX, fanDrawY,
                                    mini_divider_scissor::kNoUpperCut, sfanCutTC,
                                    fontsize, settings.separatorColor, dividerLeftPad, dividerFullW);
                            } else {
                                mini_divider_scissor::drawDividerScissored(renderer, currentX, fanDrawY,
                                    sfanCutCB, mini_divider_scissor::kNoLowerCut,
                                    fontsize, settings.separatorColor, dividerLeftPad, dividerFullW);
                            }
                            afterDivX = currentX + (int)renderer->getTextDimensions(
                                ult::DIVIDER_SYMBOL, false, fontsize).first;
                        } else {
                            afterDivX = currentX + (int)renderer->drawString(
                                ult::DIVIDER_SYMBOL, false, (float)currentX, (float)fanDrawY,
                                fontsize, settings.separatorColor).first;
                        }
                        char fanPctStr[24];
                        snprintf(fanPctStr, sizeof(fanPctStr), " %d%%", fanDuty);
                        static const std::vector<std::string> sfanIconChars = {""};
                        renderer->drawStringWithColoredSections(std::string(fanPctStr), false, sfanIconChars,
                            afterDivX, fanDrawY, fontsize, settings.textColor, settings.catColor);
                    }
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "TMP_SVOLT") {
                    // Split volt row: row 2 draws optional SOC/PCB/Skin temps then SOC voltage.
                    // Dual-row split (hasTemps): mirrors TMP_BOT with spacing/2 compression.
                    // Single-group (!hasTemps): volt drawn at sfanFanColX (same X column as fan above).
                    const bool hasTemps = currentLine.find("\u00B0") != std::string::npos;
                    const int svoltY = baseY - (int)settings.spacing / 2;
                    int currentX = hasTemps ? baseX : sfanFanColX;
                    if (hasTemps) {
                        size_t pos = 0;
                        bool parseSuccess = true;
                        for (int tempCount = 0; tempCount < 3 && parseSuccess && pos < currentLine.length(); tempCount++) {
                            while (pos < currentLine.length() && currentLine[pos] == ' ') {
                                renderer->drawString(" ", false, currentX, svoltY, fontsize, settings.textColor);
                                currentX += renderer->getTextDimensions(" ", false, fontsize).first;
                                pos++;
                            }
                            if (pos >= currentLine.length()) break;
                            const size_t degreesPos = currentLine.find("\u00B0", pos);
                            if (degreesPos == std::string::npos) { parseSuccess = false; break; }
                            const size_t cPos = currentLine.find("C", degreesPos);
                            if (cPos == std::string::npos) { parseSuccess = false; break; }
                            const std::string tempPart = currentLine.substr(pos, cPos + 1 - pos);
                            const int temp = atoi(tempPart.c_str());
                            renderer->drawString(tempPart, false, currentX, svoltY, fontsize,
                                settings.useDynamicColors
                                    ? tsl::GradientColor((float)temp)
                                    : settings.textColor);
                            currentX += renderer->getTextDimensions(tempPart, false, fontsize).first;
                            pos = cPos + 1;
                        }
                        if (!parseSuccess) {
                            renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, svoltY, fontsize, settings.textColor, settings.separatorColor);
                            currentX = baseX + (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                        }
                    }
                    // Draw SOC voltage: voltAtEnd ON=svoltY (BOT), voltAtEnd OFF=sfanBaseY (TOP)
                    // The volt divider is ALWAYS the 3rd element of the scissored stack at sfanFanColX.
                    // Both single-group and dual-row cases draw the volt column starting at sfanFanColX.
                    if (settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0]) {
                        const int voltDrawY = settings.voltageAtEndTMP ? svoltY : sfanBaseY;
                        const int voltColX = sfanFanColX;
                        int voltDivX;
                        if (sfanStackCutCB >= 0) {
                            if (settings.voltageAtEndTMP) {
                                // Volt is BOT: scissor [sfanStackCutCB, kNoLowerCut)
                                mini_divider_scissor::drawDividerScissored(renderer, voltColX, voltDrawY,
                                    sfanStackCutCB, mini_divider_scissor::kNoLowerCut,
                                    fontsize, settings.separatorColor, dividerLeftPad, dividerFullW);
                            } else {
                                // Volt is TOP: scissor [kNoUpperCut, sfanStackCutTC)
                                mini_divider_scissor::drawDividerScissored(renderer, voltColX, voltDrawY,
                                    mini_divider_scissor::kNoUpperCut, sfanStackCutTC,
                                    fontsize, settings.separatorColor, dividerLeftPad, dividerFullW);
                            }
                            voltDivX = voltColX + (int)renderer->getTextDimensions(
                                ult::DIVIDER_SYMBOL, false, fontsize).first;
                        } else {
                            voltDivX = voltColX + (int)renderer->drawString(
                                ult::DIVIDER_SYMBOL, false, (float)voltColX, (float)voltDrawY,
                                fontsize, settings.separatorColor).first;
                        }
                        renderer->drawStringWithColoredSections(std::string(MINI_SOC_volt_c), false, specialChars,
                            voltDivX, voltDrawY, fontsize, settings.textColor, settings.separatorColor);
                    }
                    // Clear sfan cuts so stale values do not leak into the next frame.
                    sfanStackCutTC = -1;
                    sfanStackCutCB = -1;
                    // Both dual-row and single-group split use spacing/2 compression: compensate currentY
                    currentY -= (int)settings.spacing / 2;
                } else if (labelIndex < labelLines.size() && labelLines[labelIndex] == "MEM") {
                    // MEM memory rendering with gradient color
                    // Extract numeric value for color determination
                    float sysValue = 0.0f;
                    sscanf(currentLine.c_str(), "%f", &sysValue);
                    
                    // Determine unit from string
                    const bool isGB = (currentLine.find("GB") != std::string::npos);
                    const float sysValueMB = isGB ? (sysValue * 1024.0f) : sysValue;
                    
                    // Apply same color scheme as in drawMemoryWidget
                    tsl::Color sysColor = settings.textColor;
                    if (sysValueMB >= 9.0f) {
                        sysColor = settings.useDynamicColors ? tsl::healthyRamTextColor : settings.textColor;
                    } else if (sysValueMB >= 3.0f) {
                        sysColor = settings.useDynamicColors ? tsl::neutralRamTextColor : settings.textColor;
                    } else {
                        sysColor = settings.useDynamicColors ? tsl::badRamTextColor : settings.textColor;
                    }
                    
                    // Draw the memory value with color on the left
                    int currentX = baseX;
                    renderer->drawStringWithColoredSections(currentLine, false, specialChars, currentX, baseY, fontsize, sysColor, settings.separatorColor);
                    currentX += renderer->getTextDimensions(currentLine, false, fontsize).first;
                    
                    // Draw separator " | "
                    renderer->drawString(ult::DIVIDER_SYMBOL, false, currentX, baseY, fontsize, settings.separatorColor);
                    currentX += renderer->getTextDimensions(ult::DIVIDER_SYMBOL, false, fontsize).first;
                    
                    // Draw "System" on the right
                    renderer->drawString("System", false, currentX, baseY, fontsize, settings.textColor);
                } else {
                    // Normal rendering for all other line types (CPU, GPU, RAM, BAT, FPS, RES, DTC)
                    const std::string& curLabel = (labelIndex < labelLines.size()) ? labelLines[labelIndex] : "";
                    if (curLabel == "CPU") {
                        // CPU may have a DIVIDER_SYMBOL suffix (volt inline): split at it so the
                        // divider still gets separatorColor while '#' padding gets transparent.
                        const std::string line = currentLine;
                        const size_t divPos = line.find(ult::DIVIDER_SYMBOL);
                        if (divPos != std::string::npos) {
                            renderer->drawStringWithColoredSections(line.substr(0, divPos), false, cpuPadChars, baseX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                            const int divX = baseX + (int)renderer->getTextDimensions(line.substr(0, divPos), false, fontsize).first;
                            renderer->drawStringWithColoredSections(line.substr(divPos), false, specialChars, divX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        } else {
                            renderer->drawStringWithColoredSections(line, false, cpuPadChars, baseX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                        }
                    } else if (curLabel == "RAM") {
                        // RAM string may embed a DIVIDER_SYMBOL when BW is inline:
                        //   "10.5 GB/s" + DIVIDER + "50%@1432.1"         — BW + load (no brackets)
                        //   "10.5 GB/s" + DIVIDER + "[#44% #44%] 50%@…"  — BW + CPU/GPU load SBS
                        //   "[#44% #44%] 50%@…"                           — CPU/GPU load SBS, no BW
                        // Split at DIVIDER if present: left uses specialChars (colors the DIVIDER),
                        // right uses cpuPadChars (hides '#' sentinels). When no DIVIDER is embedded,
                        // fall through to plain cpuPadChars draw.
                        {
                            const size_t divPos = currentLine.find(ult::DIVIDER_SYMBOL);
                            if (divPos != std::string::npos) {
                                const std::string leftPart  = currentLine.substr(0, divPos);
                                const std::string rightPart = currentLine.substr(divPos + ult::DIVIDER_SYMBOL.length());
                                int cx = baseX;
                                cx += (int)renderer->drawStringWithColoredSections(leftPart, false, specialChars, cx, baseY, fontsize, settings.textColor, settings.separatorColor).first;
                                cx += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, cx, baseY, fontsize, settings.separatorColor).first;
                                renderer->drawStringWithColoredSections(rightPart, false, cpuPadChars, cx, baseY, fontsize, settings.textColor, cpuPadTransparent);
                            } else {
                                renderer->drawStringWithColoredSections(currentLine, false, cpuPadChars, baseX, baseY, fontsize, settings.textColor, cpuPadTransparent);
                            }
                        }
                    } else {
                        renderer->drawStringWithColoredSections(currentLine, false, specialChars, baseX, baseY, fontsize, settings.textColor, settings.separatorColor);
                    }
                    int afterX = baseX + (int)renderer->getTextDimensions(currentLine, false, fontsize).first;
                    if (labelIndex < labelLines.size() && labelLines[labelIndex] == "CPU" &&
                        settings.showCPUTemp && !settings.showStackedCPUTemp && mini_cpu_temp_c[0]) {
                        afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                        const int tv = atoi(mini_cpu_temp_c);
                        renderer->drawString(std::string(mini_cpu_temp_c), false, afterX, baseY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        afterX += (int)renderer->getTextDimensions(std::string(mini_cpu_temp_c), false, fontsize).first;
                        if (settings.voltageAtEndCPU && settings.realVolts && MINI_CPU_volt_c[0]) {
                            afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_CPU_volt_c), false, specialChars, afterX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }
                    if (labelIndex < labelLines.size() && labelLines[labelIndex] == "GPU" &&
                        settings.showGPUTemp && !settings.showStackedGPUTemp && mini_gpu_temp_c[0]) {
                        afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                        const int tv = atoi(mini_gpu_temp_c);
                        renderer->drawString(std::string(mini_gpu_temp_c), false, afterX, baseY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        afterX += (int)renderer->getTextDimensions(std::string(mini_gpu_temp_c), false, fontsize).first;
                        if (settings.voltageAtEndGPU && settings.realVolts && MINI_GPU_volt_c[0]) {
                            afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                            renderer->drawStringWithColoredSections(std::string(MINI_GPU_volt_c), false, specialChars, afterX, baseY, fontsize, settings.textColor, settings.separatorColor);
                        }
                    }
                    if (labelIndex < labelLines.size() && labelLines[labelIndex] == "RAM" &&
                        settings.showRAMTemp && mini_ram_temp_c[0]) {
                        afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                        const int tv = atoi(mini_ram_temp_c);
                        renderer->drawString(std::string(mini_ram_temp_c), false, afterX, baseY, fontsize,
                            settings.useDynamicColors ? tsl::GradientColor((float)tv, tsl::DEFAULT_TEMP_RANGE_HIGH) : settings.textColor);
                        afterX += (int)renderer->getTextDimensions(std::string(mini_ram_temp_c), false, fontsize).first;
                        if (settings.voltageAtEndRAM && settings.realVolts && (vdd2_only_c[0] || vddq_only_c[0])) {
                            afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                            if (vdd2_only_c[0] && vddq_only_c[0]) {
                                // Both rails present: draw VDD2 divider VDDQ
                                renderer->drawStringWithColoredSections(std::string(vdd2_only_c), false, specialChars, afterX, baseY, fontsize, settings.textColor, settings.separatorColor);
                                afterX += (int)renderer->getTextDimensions(std::string(vdd2_only_c), false, fontsize).first;
                                afterX += (int)renderer->drawString(ult::DIVIDER_SYMBOL, false, afterX, baseY, fontsize, settings.separatorColor).first;
                                renderer->drawStringWithColoredSections(std::string(vddq_only_c), false, specialChars, afterX, baseY, fontsize, settings.textColor, settings.separatorColor);
                            } else {
                                renderer->drawStringWithColoredSections(vddq_only_c[0] ? std::string(vddq_only_c) : std::string(vdd2_only_c), false, specialChars, afterX, baseY, fontsize, settings.textColor, settings.separatorColor);
                            }
                        }
                    }
                }
                
                currentY += fontsize + settings.spacing;
                ++labelIndex;
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
                threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
                threadStart(&t6);
            }
        }
    
        // Handle performance mode changes
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            if (fontsize != settings.handheldFontSize) {
                Initialized = false;
                fontsize = settings.handheldFontSize;
                cachedBaselineOffset = (int)fontsize;
                cachedDescentAbs     = 0;
            }
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            if (fontsize != settings.dockedFontSize) {
                Initialized = false;
                fontsize = settings.dockedFontSize;
                cachedBaselineOffset = (int)fontsize;
                cachedDescentAbs     = 0;
            }
        }
    
        // Parse show settings to determine what to calculate
        std::vector<std::string> showKeys;
        {
            size_t start = 0, end = 0;
            const std::string& show = settings.show;
            while ((end = show.find('+', start)) != std::string::npos) {
                showKeys.emplace_back(show.substr(start, end - start));
                start = end + 1;
            }
            if (start < show.length()) {
                showKeys.emplace_back(show.substr(start));
            }
        }
    
        // Helper to check if a key is active
        static auto isActive = [&showKeys](const std::string& key) {
            return std::find(showKeys.begin(), showKeys.end(), key) != showKeys.end();
        };
    
        mutexLock(&mutex_Misc);
    
        // Variables to store formatted strings
        char MINI_CPU_compressed_c[42] = "";
        char MINI_GPU_Load_c[20] = "";
        char MINI_RAM_var_compressed_c[35] = "";
        char MINI_RAM_volt_c[32] = "";
        char MINI_MEM_RAM_c[32] = "";
        MINI_SOC_volt_c[0] = '\0';  // reset each frame
        MINI_CPU_volt_c[0] = '\0';  // reset each frame
        MINI_GPU_volt_c[0] = '\0';  // reset each frame
        MINI_CPU_freq_c[0] = '\0';  // reset each frame (freq split mode)
        MINI_RAM_load_bot_c[0] = '\0'; // reset each frame
        MINI_RAM_bw_c[0] = '\0';       // reset each frame (RAM bandwidth)

        // Only process CPU if needed
        if (isActive("CPU")) {
            char MINI_CPU_Usage0[7], MINI_CPU_Usage1[7], MINI_CPU_Usage2[7], MINI_CPU_Usage3[7];
            
            const uint64_t idle0 = idletick0.load(std::memory_order_acquire);
            const uint64_t idle1 = idletick1.load(std::memory_order_acquire);
            const uint64_t idle2 = idletick2.load(std::memory_order_acquire);
            const uint64_t idle3 = idletick3.load(std::memory_order_acquire);
            
            auto formatMiniUsage = [this](char* buf, size_t size, uint64_t idletick) {
                if (idletick > systemtickfrequency_impl) {
                    strcpy(buf, "0%");
                } else {
                    const double load = (1.0 - (static_cast<double>(idletick) / systemtickfrequency_impl)) * 100.0;
                    snprintf(buf, size, "%.0f%%", std::max(0.0, std::min(100.0, load)));
                }
            };
            
            formatMiniUsage(MINI_CPU_Usage0, sizeof(MINI_CPU_Usage0), idle0);
            formatMiniUsage(MINI_CPU_Usage1, sizeof(MINI_CPU_Usage1), idle1);
            formatMiniUsage(MINI_CPU_Usage2, sizeof(MINI_CPU_Usage2), idle2);
            formatMiniUsage(MINI_CPU_Usage3, sizeof(MINI_CPU_Usage3), idle3);
    
            if (settings.showFullCPU) {
                const uint16_t _cpuHz = (settings.realFrequencies && realCPU_Hz) ? realCPU_Hz / 1000000 : CPU_Hz / 1000000;
                const uint8_t  _cpuHz10 = (settings.realFrequencies && realCPU_Hz) ? (realCPU_Hz / 100000) % 10 : (CPU_Hz / 100000) % 10;
                // When showFullCPUMaxCore012 is on, brackets condense cores 0-2 into one max value.
                // Build a small per-frame buffer for that max so the snprintf format strings below
                // can use the same %s slots regardless of mode.
                char MINI_CPU_UsageMax012[7] = "";
                if (settings.showFullCPUMaxCore012) {
                    double _m0 = 0, _m1 = 0, _m2 = 0;
                    sscanf(MINI_CPU_Usage0, "%lf%%", &_m0);
                    sscanf(MINI_CPU_Usage1, "%lf%%", &_m1);
                    sscanf(MINI_CPU_Usage2, "%lf%%", &_m2);
                    const double _maxCore012 = std::max({_m0, _m1, _m2});
                    snprintf(MINI_CPU_UsageMax012, sizeof(MINI_CPU_UsageMax012), "%.0f%%", _maxCore012);
                }
                // Pad single-digit core % with a leading space so bracket contents
                // stay fixed-width: "1%" -> " 1%", "24%" unchanged, "100%" unchanged.
                // strlen("X%") == 2 means single digit — write " X%" into a local buffer.
                auto mkPad = [](const char* s, char (&buf)[4]) -> const char* {
                    if (s[0] != '\0' && s[1] == '%' && s[2] == '\0') {
                        buf[0] = '#'; buf[1] = s[0]; buf[2] = '%'; buf[3] = '\0';
                        return buf;
                    }
                    return s;
                };
                char _pb0[4], _pb1[4], _pb2[4], _pb3[4], _pbM[4];
                const char* p0   = mkPad(MINI_CPU_Usage0,    _pb0);
                const char* p1   = mkPad(MINI_CPU_Usage1,    _pb1);
                const char* p2   = mkPad(MINI_CPU_Usage2,    _pb2);
                const char* p3   = mkPad(MINI_CPU_Usage3,    _pb3);
                const char* pM   = mkPad(MINI_CPU_UsageMax012, _pbM);
                if (!settings.showStackedFullCPU) {
                    // Inline mode (default): [23%,29%,0%,32%]@1000.0
                    if (settings.showFullCPUMaxCore012) {
                        // Condensed: [max(c0,c1,c2) c3]@freq
                        snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c),
                            "[%s %s]@%hu.%hhu",
                            pM, p3,
                            _cpuHz, _cpuHz10);
                    } else {
                        snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c),
                            "[%s %s %s %s]@%hu.%hhu",
                            p0, p1, p2, p3,
                            _cpuHz, _cpuHz10);
                    }
                    // MINI_CPU_freq_c already reset to '\0' above
                } else {
                    // Split mode: brackets on top row, max%@freq on bottom row
                    if (settings.showFullCPUMaxCore012) {
                        // Condensed: [max(c0,c1,c2) c3] on top
                        snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c),
                            "[%s %s]",
                            pM, p3);
                    } else {
                        snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c),
                            "[%s %s %s %s]",
                            p0, p1, p2, p3);
                    }
                    double _u0 = 0, _u1 = 0, _u2 = 0, _u3 = 0;
                    sscanf(MINI_CPU_Usage0, "%lf%%", &_u0);
                    sscanf(MINI_CPU_Usage1, "%lf%%", &_u1);
                    sscanf(MINI_CPU_Usage2, "%lf%%", &_u2);
                    sscanf(MINI_CPU_Usage3, "%lf%%", &_u3);
                    const double _maxU = std::max({_u0, _u1, _u2, _u3});
                    snprintf(MINI_CPU_freq_c, sizeof(MINI_CPU_freq_c),
                        "%.0f%%@%hu.%hhu", _maxU, _cpuHz, _cpuHz10);
                }
            } else {
                double usage0 = 0, usage1 = 0, usage2 = 0, usage3 = 0;
                sscanf(MINI_CPU_Usage0, "%lf%%", &usage0);
                sscanf(MINI_CPU_Usage1, "%lf%%", &usage1);
                sscanf(MINI_CPU_Usage2, "%lf%%", &usage2);
                sscanf(MINI_CPU_Usage3, "%lf%%", &usage3);
                
                double maxUsage = std::max({usage0, usage1, usage2, usage3});
                
                if (settings.realFrequencies && realCPU_Hz) {
                    snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                        "%.0f%%@%hu.%hhu", maxUsage,
                        realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
                } else {
                    snprintf(MINI_CPU_compressed_c, sizeof(MINI_CPU_compressed_c), 
                        "%.0f%%@%hu.%hhu", maxUsage,
                        CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
                }
            }
    
            if (settings.realVolts) {
                const uint32_t mv = realCPU_mV / 1000;
                snprintf(MINI_CPU_volt_c, sizeof(MINI_CPU_volt_c), "%u mV", mv);
            }
            // Individual CPU die temp for per-component display
            if (settings.showCPUTemp)
                snprintf(mini_cpu_temp_c, sizeof(mini_cpu_temp_c), "%d\u00B0C", (int)(componentCPU_mC / 1000));
            else
                mini_cpu_temp_c[0] = '\0';
        }
    
        // Only process GPU if needed
        if (isActive("GPU")) {
            if (settings.realFrequencies && realGPU_Hz) {
                snprintf(MINI_GPU_Load_c, sizeof(MINI_GPU_Load_c),
                         "%hu%%%s%hu.%hhu",
                         GPU_Load_u / 10, "@",
                         realGPU_Hz / 1000000, (realGPU_Hz / 100000) % 10);
            } else {
                snprintf(MINI_GPU_Load_c, sizeof(MINI_GPU_Load_c),
                         "%hu%%%s%hu.%hhu",
                         GPU_Load_u / 10, "@",
                         GPU_Hz / 1000000, (GPU_Hz / 100000) % 10);
            }
    
            if (settings.realVolts) {
                const uint32_t mv = realGPU_mV / 1000;
                snprintf(MINI_GPU_volt_c, sizeof(MINI_GPU_volt_c), "%u mV", mv);
            }
            // Individual GPU die temp for per-component display
            if (settings.showGPUTemp)
                snprintf(mini_gpu_temp_c, sizeof(mini_gpu_temp_c), "%d\u00B0C", (int)(componentGPU_mC / 1000));
            else
                mini_gpu_temp_c[0] = '\0';
        }
    
        // Only process RAM if needed
        if (isActive("RAM")) {
            // Build RAM bandwidth string ("X.X GB/s") when enabled.
            // Always written — "0.0 GB/s" placeholder when ACTMON hasn't delivered data yet
            // so the stacked layout is stable from frame 1 and never jumps.
            if (settings.showRAMBandwidth) {
                if (ramBW_MBs > 0) {
                    const unsigned bwInt = ramBW_MBs / 1000;
                    const unsigned bwDec = (ramBW_MBs % 1000) / 100;
                    snprintf(MINI_RAM_bw_c, sizeof(MINI_RAM_bw_c), "%u.%u GB/s", bwInt, bwDec);
                } else {
                    snprintf(MINI_RAM_bw_c, sizeof(MINI_RAM_bw_c), "0.0 GB/s");
                }
            }

            if (!settings.showRAMLoad) {
                const float ramTotalGiB = (RAM_Total_application_u + RAM_Total_applet_u +
                                     RAM_Total_system_u + RAM_Total_systemunsafe_u) /
                                    (1024.0f * 1024.0f);
                const float ramUsedGiB  = (RAM_Used_application_u + RAM_Used_applet_u +
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
                unsigned ramLoadInt;
                
                if (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck)) {
                    ramLoadInt = (ramLoad[SysClkRamLoad_All] + 5) / 10;
                    
                    if (settings.showRAMLoadCPUGPU) {
                        unsigned ramCpuLoadInt = (ramLoad[SysClkRamLoad_Cpu] + 5) / 10;
                        int RAM_GPU_Load = ramLoad[SysClkRamLoad_All] - ramLoad[SysClkRamLoad_Cpu];
                        unsigned ramGpuLoadInt = (unsigned)(RAM_GPU_Load > 0 ? (RAM_GPU_Load + 5) / 10 : 0);
                        const uint32_t useHz = (settings.realFrequencies && realRAM_Hz) ? realRAM_Hz : (uint32_t)RAM_Hz;
                        const unsigned ramMHz   = useHz / 1000000;
                        const unsigned ramMHz10 = (useHz / 100000) % 10;
                        // Pad single-digit RAM load values with '#' sentinel (same as CPU mkPad).
                        // '#' is drawn transparent at render time so bracket width stays fixed.
                        auto mkPadR = [](unsigned v, char (&buf)[4]) -> const char* {
                            if (v < 10) {
                                buf[0] = '#'; buf[1] = '0' + (char)v; buf[2] = '%'; buf[3] = '\0';
                                return buf;
                            }
                            snprintf(buf, sizeof(buf), "%u%%", v);
                            return buf;
                        };
                        char _rb0[4], _rb1[4];
                        const char* rp0 = mkPadR(ramCpuLoadInt, _rb0);
                        const char* rp1 = mkPadR(ramGpuLoadInt, _rb1);
                        if (!settings.showStackedRAMLoadCPUGPU) {
                            // SBS: [cpu% gpu%]total%@freq on one line
                            snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                     "[%s %s] %u%%@%u.%u",
                                     rp0, rp1, ramLoadInt, ramMHz, ramMHz10);
                        } else {
                            // Split: [cpu% gpu%] top, total%@freq bottom
                            snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                     "[%s %s]", rp0, rp1);
                            snprintf(MINI_RAM_load_bot_c, sizeof(MINI_RAM_load_bot_c),
                                     "%u%%@%u.%u", ramLoadInt, ramMHz, ramMHz10);
                        }
                    } else {
                        if (settings.realFrequencies && realRAM_Hz) {
                            snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                     "%u%%@%hu.%hhu", ramLoadInt,
                                     realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
                        } else {
                            snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                     "%u%%@%hu.%hhu", ramLoadInt,
                                     RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
                        }
                    }
                } else {
                    const uint64_t RAM_Total_all = RAM_Total_application_u + RAM_Total_applet_u + 
                                                   RAM_Total_system_u + RAM_Total_systemunsafe_u;
                    const uint64_t RAM_Used_all = RAM_Used_application_u + RAM_Used_applet_u + 
                                                  RAM_Used_system_u + RAM_Used_systemunsafe_u;
                    // Round to nearest by adding half-divisor before dividing — matches the
                    // (value + 5) / 10 pattern used by the IPC RAM load path above.
                    ramLoadInt = (RAM_Total_all > 0)
                        ? (unsigned)((RAM_Used_all * 100 + RAM_Total_all / 2) / RAM_Total_all)
                        : 0;
                    
                    if (settings.realFrequencies && realRAM_Hz) {
                        snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                 "%u%%@%hu.%hhu", ramLoadInt,
                                 realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
                    } else {
                        snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c),
                                 "%u%%@%hu.%hhu", ramLoadInt,
                                 RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
                    }
                }
            }

            // RAM bandwidth integration. Two layouts:
            //   Inline BW (stacked BW off): prepend "X.XGB/s" + DIVIDER to MINI_RAM_var_compressed_c.
            //     Works for all sub-modes including stacked-load top row.
            //   Stacked BW (stacked BW on): BW goes on top row, existing RAM content on bottom row,
            //     reusing the RAM_SLOAD_TOP/BOT infrastructure for right-alignment and volt rendering.
            //     When load-CPU/GPU-split is already active (stacked load on), BW prepends inline to
            //     the existing top row [c% g%] instead — that split already owns both rows with data.
            //     VDDQ and RAM temp splits are right-side decorations; they never block BW stacking.
            if (MINI_RAM_bw_c[0]) {
                const bool dat_wantRAMLoadCPUGPUSplit = settings.showRAMLoad && settings.showRAMLoadCPUGPU &&
                                                  settings.showStackedRAMLoadCPUGPU &&
                                                  (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck));
                // bwOnOwnRow: stacked BW is on, and load-CPU/GPU-split is not already consuming both rows.
                // VDDQ split and RAM temp split are right-side column decorations — they are orthogonal
                // to BW row layout and must NOT block BW from taking the top row.
                // The renderer will use MINI_RAM_var_compressed_c as the top row (BW) and
                // MINI_RAM_load_bot_c as the bottom row (RAM content), reusing RAM_SLOAD_TOP/BOT.
                const bool bwOnOwnRow = settings.showStackedRAMBandwidth && !dat_wantRAMLoadCPUGPUSplit;
                // bwLeftOfLoadSplit: BW NOT stacked + load-split active.
                // BW is drawn as a separate LEFT column; MINI_RAM_bw_c is left populated for the renderer.
                // The renderer draws BW at baseX, then a triple-stack divider, then the stacked-load
                // content right-aligned within the remainder of the data width.
                const bool bwLeftOfLoadSplit = !settings.showStackedRAMBandwidth && dat_wantRAMLoadCPUGPUSplit;
                if (bwOnOwnRow) {
                    // Stacked BW (no other split): swap — BW goes to top row, RAM content to bottom row.
                    // RAM_SLOAD_TOP/BOT renderer handles right-alignment and volt columns.
                    snprintf(MINI_RAM_load_bot_c, sizeof(MINI_RAM_load_bot_c), "%s", MINI_RAM_var_compressed_c);
                    snprintf(MINI_RAM_var_compressed_c, sizeof(MINI_RAM_var_compressed_c), "%s", MINI_RAM_bw_c);
                    MINI_RAM_bw_c[0] = '\0';
                } else if (bwLeftOfLoadSplit) {
                    // BW inline + load stacked: leave MINI_RAM_bw_c populated for the RAM_SLOAD_TOP
                    // renderer to draw as the left column. Do NOT prepend to RAM_var_compressed_c.
                    // (MINI_RAM_bw_c will be cleared by the renderer after drawing.)
                } else {
                    // Inline BW (stacked BW off, load not stacked): prepend "BW\xEE\x80\xB1" to data row.
                    char tmp[sizeof(MINI_RAM_var_compressed_c)];
                    snprintf(tmp, sizeof(tmp), "%s%s",
                             MINI_RAM_bw_c, MINI_RAM_var_compressed_c);
                    memcpy(MINI_RAM_var_compressed_c, tmp, sizeof(MINI_RAM_var_compressed_c));
                    MINI_RAM_bw_c[0] = '\0';
                }
            }
    
            if (settings.realVolts) {
                const float mv_vdd2_f = realRAM_mV / 100000.0f;
                const uint32_t mv_vdd2_i = realRAM_mV / 100000;
                const uint32_t mv_vddq   = (realRAM_mV % 10000) / 10;

                // Build separate single-volt strings (used by split VDD2/VDDQ mode)
                vdd2_only_c[0] = '\0';
                vddq_only_c[0] = '\0';
                if (settings.showVDD2) {
                    if (settings.decimalVDD2)
                        snprintf(vdd2_only_c, sizeof(vdd2_only_c), "%.1f mV", mv_vdd2_f);
                    else
                        snprintf(vdd2_only_c, sizeof(vdd2_only_c), "%u mV", mv_vdd2_i);
                }
                if (settings.showVDDQ && isMariko)
                    snprintf(vddq_only_c, sizeof(vddq_only_c), "%u mV", mv_vddq);

                if (isMariko) {
                    if (settings.showVDDQ && settings.showVDD2) {
                        if (settings.decimalVDD2)
                            snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c),
                                     "%.1f mV%u mV", mv_vdd2_f, mv_vddq);
                        else
                            snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c),
                                     "%u mV%u mV", mv_vdd2_i, mv_vddq);
                    } else if (settings.showVDDQ) {
                        snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%u mV", mv_vddq);
                    } else if (settings.showVDD2) {
                        if (settings.decimalVDD2)
                            snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%.1f mV", mv_vdd2_f);
                        else
                            snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%u mV", mv_vdd2_i);
                    }
                } else {
                    if (settings.decimalVDD2)
                        snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%.1f mV", mv_vdd2_f);
                    else
                        snprintf(MINI_RAM_volt_c, sizeof(MINI_RAM_volt_c), "%u mV", mv_vdd2_i);
                }
            }
        }
    
        // Individual RAM die temp for per-component display
        if (isActive("RAM") && settings.showRAMTemp)
            snprintf(mini_ram_temp_c, sizeof(mini_ram_temp_c), "%d\u00B0C", (int)(componentRAM_mC / 1000));
        else
            mini_ram_temp_c[0] = '\0';

        // Only process MEM if needed
        if (isActive("MEM")) {
            const u64 freeRamBytes = RAM_Total_system_u - RAM_Used_system_u;
            
            float value;
            const char* unit;
            
            if (freeRamBytes >= 1024ULL * 1024 * 1024) {
                value = static_cast<float>(freeRamBytes) / (1024.0f * 1024.0f * 1024.0f);
                unit = "GB";
            } else {
                value = static_cast<float>(freeRamBytes) / (1024.0f * 1024.0f);
                unit = "MB";
            }
            
            int decimalPlaces;
            if (value >= 1000.0f) {
                decimalPlaces = 0;
            } else if (value >= 100.0f) {
                decimalPlaces = 1;
            } else if (value >= 10.0f) {
                decimalPlaces = 2;
            } else {
                decimalPlaces = 3;
            }
            
            snprintf(MINI_MEM_RAM_c, sizeof(MINI_MEM_RAM_c), "%.*f %s %s", 
                     decimalPlaces, value, unit, ult::FREE.c_str());
        }
    
        // Only process temperature/fan data if SOC or TMP is active
        if (isActive("TMP") || isActive("SOC")) {
            const int duty = safeFanDuty((int)Rotation_Duty);
    
            if (settings.realVolts && settings.showSOCVoltage) {
                const uint32_t mv = realSOC_mV / 1000;
                snprintf(MINI_SOC_volt_c, sizeof(MINI_SOC_volt_c), "%u mV", mv);
            }
    
            if (isActive("SOC")) {
                if (settings.showFanPercentage) {
                    snprintf(soc_temperature_c, sizeof(soc_temperature_c),
                             "%d°C %d%%", (int)SOC_temperatureF, duty);
                } else {
                    snprintf(soc_temperature_c, sizeof(soc_temperature_c),
                             "%d°C", (int)SOC_temperatureF);
                }
            }
    
            if (isActive("TMP")) {
                if (settings.showComponentTemps && settings.showSocPcbSkinTemps) {
                    // Dual-row mode: top row = die temps, bottom row = board temps (fan shown centered)
                    snprintf(componentTemps_c, sizeof(componentTemps_c),
                        "%d\u00B0C %d\u00B0C %d\u00B0C",
                        (int)(componentCPU_mC / 1000),
                        (int)(componentGPU_mC / 1000),
                        (int)(componentRAM_mC / 1000));
                    // SOC/PCB/Skin without fan (fan drawn centered between rows)
                    snprintf(skin_temperature_c, sizeof(skin_temperature_c),
                        "%d\u00B0C %d\u00B0C %hu\u00B0C",
                        (int)SOC_temperatureF, (int)PCB_temperatureF,
                        skin_temperaturemiliC / 1000);
                } else if (settings.showComponentTemps) {
                    // Only component die temps (no SOC/PCB/Skin)
                    snprintf(componentTemps_c, sizeof(componentTemps_c),
                        "%d\u00B0C %d\u00B0C %d\u00B0C",
                        (int)(componentCPU_mC / 1000),
                        (int)(componentGPU_mC / 1000),
                        (int)(componentRAM_mC / 1000));
                    skin_temperature_c[0] = '\0';
                } else {
                    // Only SOC/PCB/Skin temps (default single-row, or split row 1)
                    componentTemps_c[0] = '\0';
                    // In split mode the fan is drawn by the renderer; strip it from the data string
                    const bool splitFanSOC = settings.showStackedFanSOC &&
                                             settings.showFanPercentage &&
                                             settings.realVolts && settings.showSOCVoltage;
                    if (!splitFanSOC && settings.showFanPercentage) {
                        snprintf(skin_temperature_c, sizeof(skin_temperature_c),
                            "%d\u00B0C %d\u00B0C %hu\u00B0C %d%%",
                            (int)SOC_temperatureF, (int)PCB_temperatureF,
                            skin_temperaturemiliC / 1000, duty);
                    } else {
                        snprintf(skin_temperature_c, sizeof(skin_temperature_c),
                            "%d\u00B0C %d\u00B0C %hu\u00B0C",
                            (int)SOC_temperatureF, (int)PCB_temperatureF,
                            skin_temperaturemiliC / 1000);
                    }
                }
            }
        }
    
        // Only process resolution if RES is active and game is running
        if (isActive("RES") && GameRunning && NxFps) {
            if (!resolutionLookup) {
                (NxFps->renderCalls[0].calls) = 0xFFFF;
                resolutionLookup = 1;
            } else if (resolutionLookup == 1) {
                if ((NxFps->renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
            } else {
                memcpy(&m_resolutionRenderCalls, &(NxFps->renderCalls), sizeof(m_resolutionRenderCalls));
                memcpy(&m_resolutionViewportCalls, &(NxFps->viewportCalls), sizeof(m_resolutionViewportCalls));
                qsort(m_resolutionRenderCalls, 8, sizeof(resolutionCalls), compare);
                qsort(m_resolutionViewportCalls, 8, sizeof(resolutionCalls), compare);
                memset(&m_resolutionOutput, 0, sizeof(m_resolutionOutput));
                size_t out_iter = 0;
                bool found = false;
                for (size_t i = 0; i < 8; i++) {
                    for (size_t x = 0; x < 8; x++) {
                        if (m_resolutionRenderCalls[i].width == 0) break;
                        if ((m_resolutionRenderCalls[i].width == m_resolutionViewportCalls[x].width) && 
                            (m_resolutionRenderCalls[i].height == m_resolutionViewportCalls[x].height)) {
                            m_resolutionOutput[out_iter].width = m_resolutionRenderCalls[i].width;
                            m_resolutionOutput[out_iter].height = m_resolutionRenderCalls[i].height;
                            m_resolutionOutput[out_iter].calls = (m_resolutionRenderCalls[i].calls > m_resolutionViewportCalls[x].calls) 
                                ? m_resolutionRenderCalls[i].calls : m_resolutionViewportCalls[x].calls;
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
                            if (m_resolutionViewportCalls[x].width == 0) break;
                            if ((m_resolutionViewportCalls[x].width == m_resolutionOutput[y].width) && 
                                (m_resolutionViewportCalls[x].height == m_resolutionOutput[y].height)) {
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
            }
        } else if (!GameRunning && resolutionLookup != 0) {
            resolutionLookup = 0;
        }
        
        mutexUnlock(&mutex_Misc);
    
        // Battery/power draw - always update since BAT/DRAW might be displayed
        if (isActive("BAT") || isActive("DRAW")) {
            char remainingBatteryLife[8];
            const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f : PowerConsumption;
            
            mutexLock(&mutex_BatteryChecker);
            
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
            
            mutexUnlock(&mutex_BatteryChecker);
        }
    
        // Build Variables string
        char Temp[512] = "";
        uint16_t flags = 0;
        
        for (const auto& key : showKeys) {
            if (key == "CPU" && !(flags & 1)) {
                if (Temp[0]) strcat(Temp, "\n");
                const bool cpuFullSplit_v = settings.showFullCPU && settings.showStackedFullCPU;
                const bool cpuSplit_v = !cpuFullSplit_v && settings.showCPUTemp && settings.showStackedCPUTemp && settings.realVolts;
                if (cpuFullSplit_v) {
                    // Two rows: brackets (top) then freq (bottom, right-aligned to bracket right edge)
                    strcat(Temp, MINI_CPU_compressed_c);
                    strcat(Temp, "\n");
                    strcat(Temp, MINI_CPU_freq_c);
                } else if (cpuSplit_v) {
                    strcat(Temp, MINI_CPU_compressed_c);
                    strcat(Temp, "\n");
                    strcat(Temp, " ");
                } else {
                    strcat(Temp, MINI_CPU_compressed_c);
                    if (settings.realVolts && MINI_CPU_volt_c[0] && (!settings.voltageAtEndCPU || !mini_cpu_temp_c[0])) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_CPU_volt_c);
                    }

                }
                flags |= 1;
            }
            else if (key == "GPU" && !(flags & 2)) {
                if (Temp[0]) strcat(Temp, "\n");
                const bool gpuSplit_v = settings.showGPUTemp && settings.showStackedGPUTemp && settings.realVolts;
                if (gpuSplit_v) {
                    strcat(Temp, MINI_GPU_Load_c);
                    strcat(Temp, "\n");
                    strcat(Temp, " ");
                } else {
                    strcat(Temp, MINI_GPU_Load_c);
                    if (settings.realVolts && MINI_GPU_volt_c[0] && (!settings.voltageAtEndGPU || !mini_gpu_temp_c[0])) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_GPU_volt_c);
                    }

                }
                flags |= 2;
            }
            else if (key == "RAM" && !(flags & 4)) {
                if (Temp[0]) strcat(Temp, "\n");
                const bool splitVDDQ_t = settings.showStackedVDDQ &&
                                        settings.realVolts && settings.showVDD2 &&
                                        settings.showVDDQ && isMariko;
                const bool ramTempSplit_t = settings.showRAMTemp && settings.showStackedRAMTemp &&
                                            !splitVDDQ_t && settings.realVolts &&
                                            ((settings.showVDDQ && isMariko && !settings.showVDD2) ||
                                             (settings.showVDD2 && !settings.showVDDQ));
                const bool ramLoadSplit_t = settings.showRAMLoad && settings.showRAMLoadCPUGPU &&
                                            settings.showStackedRAMLoadCPUGPU &&
                                            (R_SUCCEEDED(sysclkCheck) || R_SUCCEEDED(hocclkCheck));
                // wantAnySplit mirrors the label-building predicate: fires for load-split OR BW-stacked.
                // VDDQ/temp splits are right-side decorations — they must NOT block BW stacking.
                const bool wantBWStackedSplit_t = settings.showRAMBandwidth && settings.showStackedRAMBandwidth &&
                                                  !ramLoadSplit_t;
                const bool wantAnySplit_t = ramLoadSplit_t || wantBWStackedSplit_t;

                if (wantAnySplit_t) {
                    // Row 1: top content (BW string or [cpu% gpu%]); Row 2: placeholder
                    // (MINI_RAM_load_bot_c drawn directly by RAM_SLOAD_BOT renderer)
                    strcat(Temp, MINI_RAM_var_compressed_c);
                    strcat(Temp, "\n");
                    strcat(Temp, " ");
                } else if (splitVDDQ_t) {
                    // Row 1: usage only (VDD2 drawn by renderer)
                    strcat(Temp, MINI_RAM_var_compressed_c);
                    strcat(Temp, "\n");
                    // Row 2: space placeholder (VDDQ drawn by renderer)
                    strcat(Temp, " ");
                } else if (ramTempSplit_t) {
                    // Row 1: RAM data only (VDDQ top drawn by renderer)
                    strcat(Temp, MINI_RAM_var_compressed_c);
                    strcat(Temp, "\n");
                    // Row 2: placeholder (RAM temp bottom drawn by renderer)
                    strcat(Temp, " ");
                } else {
                    strcat(Temp, MINI_RAM_var_compressed_c);
                    if (settings.realVolts && MINI_RAM_volt_c[0] &&
                        !(settings.voltageAtEndRAM && settings.showRAMTemp && !settings.showStackedRAMTemp)) {
                        strcat(Temp, "");
                        strcat(Temp, MINI_RAM_volt_c);
                    }

                }
                flags |= 4;
            }
            else if (key == "SOC" && !(flags & 8)) {
                if (Temp[0]) strcat(Temp, "\n");
                strcat(Temp, soc_temperature_c);
                if (settings.realVolts && settings.showSOCVoltage && MINI_SOC_volt_c[0]) {
                    strcat(Temp, "");
                    strcat(Temp, MINI_SOC_volt_c);
                }
                flags |= 8;
            }
            else if (key == "TMP" && !(flags & 16)) {
                if (Temp[0]) strcat(Temp, "\n");
                // Determine if split mode is active (fan on row 1, volt on row 2)
                const bool splitFanSOC = settings.showStackedFanSOC &&
                                         settings.showFanPercentage &&
                                         settings.realVolts && settings.showSOCVoltage;
                if (splitFanSOC) {
                    // Split mode: row 1 = fan-row temps, row 2 = volt-row temps (or space)
                    if (settings.showComponentTemps) {
                        strcat(Temp, componentTemps_c);   // Row 1: comp temps (fan drawn by renderer)
                    } else {
                        strcat(Temp, skin_temperature_c); // Row 1: SOC/PCB/Skin temps (fan drawn by renderer)
                    }
                    strcat(Temp, "\n");
                    if (settings.showComponentTemps && settings.showSocPcbSkinTemps) {
                        strcat(Temp, skin_temperature_c); // Row 2: SOC/PCB/Skin temps (volt drawn by renderer)
                    } else {
                        strcat(Temp, " ");                // Row 2: empty (volt drawn by renderer)
                    }
                } else if (settings.showComponentTemps && settings.showSocPcbSkinTemps && componentTemps_c[0]) {
                    // Both rows: component temps first, SOC/PCB/Skin second
                    strcat(Temp, componentTemps_c);
                    strcat(Temp, "\n");
                    strcat(Temp, skin_temperature_c);
                } else if (settings.showComponentTemps && componentTemps_c[0]) {
                    // Only component die temps: single row, no voltage
                    strcat(Temp, componentTemps_c);
                } else {
                    // Only SOC/PCB/Skin temps: single row, volt drawn explicitly by renderer (respects voltageAtEndTMP)
                    strcat(Temp, skin_temperature_c);
                }
                flags |= 16;
            }
            else if ((key == "BAT" || key == "DRAW") && !(flags & 32)) {
                if (Temp[0]) strcat(Temp, "\n");
                if (settings.showStackedBAT) {
                    // Split: top row = Battery_c (draw or pct per invert), bottom = the other
                    const char* botStr = settings.invertBatteryDisplay ? Battery_draw_c : Battery_pct_c;
                    strcat(Temp, Battery_c);
                    strcat(Temp, "\n");
                    strcat(Temp, botStr);
                } else {
                    strcat(Temp, Battery_c);
                }
                flags |= 32;
            }
            else if (key == "FPS" && !(flags & 64) && GameRunning) {
                if (Temp[0]) strcat(Temp, "\n");
                char Temp_s[24];
                if (settings.useIntegerFPS)
                    snprintf(Temp_s, sizeof(Temp_s), "%d [%d - %d]", (int)round(FPSavg), (int)round(FPSmin), (int)round(FPSmax));
                else
                    snprintf(Temp_s, sizeof(Temp_s), "%2.1f [%2.1f - %2.1f]", FPSavg, FPSmin, FPSmax);
                strcat(Temp, Temp_s);
                flags |= 64;
            }
            else if (key == "RES" && !(flags & 128) && GameRunning && m_resolutionOutput[0].width) {
                if (Temp[0]) strcat(Temp, "\n");
                char Temp_s[32] = "";
                static std::pair<uint16_t, uint16_t> old_res[2];
                
                uint16_t w0 = m_resolutionOutput[0].width;
                uint16_t h0 = m_resolutionOutput[0].height;
                uint16_t w1 = m_resolutionOutput[1].width;
                uint16_t h1 = m_resolutionOutput[1].height;
                
                if (w0 || w1) {
                    if (w0 && w1) {
                        if ((w0 == old_res[1].first && h0 == old_res[1].second) ||
                            (w1 == old_res[0].first && h1 == old_res[0].second)) {
                            std::swap(w0, w1);
                            std::swap(h0, h1);
                        }
                    }
                    
                    
                    // Format based on whether we show full resolution or just height (p)
                    if (settings.showFullResolution) {
                        if (!w1 || !h1)
                            snprintf(Temp_s, sizeof(Temp_s), "%dx%d", w0 ? w0 : w1, h0 ? h0 : h1);
                        else
                            snprintf(Temp_s, sizeof(Temp_s), "%dx%d%dx%d", w0, h0, w1, h1);
                    } else {
                        if (!w1 || !h1)
                            snprintf(Temp_s, sizeof(Temp_s), "%dp", h0 ? h0 : h1);
                        else
                            snprintf(Temp_s, sizeof(Temp_s), "%dp%dp", h0, h1);
                    }
                    
                    // Update last-frame cache
                    old_res[0] = {m_resolutionOutput[0].width, m_resolutionOutput[0].height};
                    old_res[1] = {m_resolutionOutput[1].width, m_resolutionOutput[1].height};
        
                    strcat(Temp, Temp_s);
                    flags |= 128;
                }
            }
            else if (key == "READ" && !(flags & 512) && GameRunning && NxFps) {
                if (Temp[0]) strcat(Temp, "\n");
                char Temp_s[24];
                if ((NxFps->readSpeedPerSecond) != 0.f) {
                    snprintf(Temp_s, sizeof(Temp_s), "%.2f MiB/s", (NxFps->readSpeedPerSecond) / 1048576.f);
                } else {
                    strcpy(Temp_s, "n/d");
                }
                strcat(Temp, Temp_s);
                flags |= 512;
            }
            else if (key == "DTC" && !(flags & 256) && settings.showDTC) {
                if (Temp[0]) strcat(Temp, "\n");
                char dateTimeStr[64];
                time_t rawtime = time(NULL);
                struct tm *timeinfo = localtime(&rawtime);
                strftime(dateTimeStr, sizeof(dateTimeStr), settings.dtcFormat.c_str(), timeinfo);
                if (settings.showStackedDTC) {
                    // Split at DIVIDER_SYMBOL: top half goes on row 1, bottom half on row 2.
                    // If the user's format has no DIVIDER_SYMBOL, fall back to the whole
                    // string on the top row + empty bottom row (still 2 label slots).
                    const std::string dtStr(dateTimeStr);
                    const size_t divPos = dtStr.find(ult::DIVIDER_SYMBOL);
                    if (divPos != std::string::npos) {
                        const std::string topHalf = dtStr.substr(0, divPos);
                        const std::string botHalf = dtStr.substr(divPos + ult::DIVIDER_SYMBOL.length());
                        strcat(Temp, topHalf.c_str());
                        strcat(Temp, "\n");
                        strcat(Temp, botHalf.c_str());
                    } else {
                        strcat(Temp, dateTimeStr);
                        strcat(Temp, "\n");
                    }
                } else {
                    strcat(Temp, dateTimeStr);
                }
                flags |= 256;
            }
            else if (key == "MEM" && !(flags & 1024)) {
                if (Temp[0]) strcat(Temp, "\n");
                strcat(Temp, MINI_MEM_RAM_c);
                flags |= 1024;
            }
        }
        
        strcpy(Variables, Temp);
    
        // Enable rendering if needed
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
        // Static variables to maintain drag state between function calls
        static bool oldTouchDetected = false;
        static bool oldPlusHeld = false;

        static HidTouchState initialTouchPos = {0};
        static int initialFrameOffsetX = 0;
        static int initialFrameOffsetY = 0;
        static constexpr int TOUCH_THRESHOLD = 8;
        static bool hasMoved = false;
        //static u64 lastTouchTime = 0;
        //static constexpr u64 TOUCH_DEBOUNCE_NS = 16000000ULL; // 16ms debounce
    
        // Get current time for debouncing
        //const u64 currentTime = armTicksToNs(armGetSystemTick());
        
        // Better touch detection - check if coordinates are within reasonable screen bounds
        const bool currentTouchDetected = (touchPos.x > 0 && touchPos.y > 0 && 
                                    touchPos.x < screenWidth && touchPos.y < screenHeight);
        
        // Debounce touch detection
        //if (currentTouchDetected && !oldTouchDetected) {
        //    if (currentTime - lastTouchTime < TOUCH_DEBOUNCE_NS) {
        //        currentTouchDetected = false; // Ignore rapid touch changes
        //    } else {
        //        lastTouchTime = currentTime;
        //    }
        //}

        static bool clearOnRelease = false;

        if (clearOnRelease && !isRendering) {
            clearOnRelease = false;
            if (!(tsl::notification && tsl::notification->isActive())) {
                isRendering = true;
                leventClear(&renderingStopEvent);
            } else {
                wasRendering = true; // hand off — notification completion path re-enables when done
            }
        }
        
        // Calculate overlay bounds
        const uint32_t margin = (fontsize * 4);
        
        // Cache bounds calculation
        static int cachedBaseX = 0;
        static int cachedBaseY = 0;
        static uint32_t cachedOverlayHeight = 0;
        static bool boundsNeedUpdate = true;
        static std::string lastVariables = "";
        
        // Only recalculate bounds when Variables change or first time
        if (boundsNeedUpdate || std::string(Variables) != lastVariables) {
            // Calculate actual entry count from Variables string
            size_t actualEntryCount = 1;
            for (size_t i = 0; Variables[i] != '\0'; i++) {
                if (Variables[i] == '\n') {
                    actualEntryCount++;
                }
            }
            
            cachedOverlayHeight = ((fontsize + settings.spacing) * actualEntryCount) + 
                                 settings.spacing + topPadding + bottomPadding;
            if (settings.showComponentTemps && settings.showSocPcbSkinTemps)
                cachedOverlayHeight -= settings.spacing / 2;
            
            // Position calculation based on settings
            cachedBaseX = 0;
            cachedBaseY = 0;
            
            boundsNeedUpdate = false;
            lastVariables = Variables;
        }
        
        const int leftPadding = settings.showLabels ? 0 : settings.spacing + bottomPadding;
        const int overlayWidth = settings.showLabels 
            ? (margin + rectangleWidth + (fontsize / 3))
            : (rectangleWidth + (fontsize / 3) * 2 + leftPadding);
        const int overlayHeight = cachedOverlayHeight;
        
        // Screen boundaries for clamping
        const int minX = -cachedBaseX + framePadding;
        const int maxX = screenWidth - overlayWidth - cachedBaseX - framePadding;
        const int minY = -cachedBaseY + framePadding;
        const int maxY = screenHeight - overlayHeight - cachedBaseY - framePadding;
        
        const bool plusDragReady = buttonState.plusDragActive.load(std::memory_order_acquire);

        // Check button states
        //const bool currentMinusHeld = (keysHeld & KEY_MINUS) && !(keysHeld & ~KEY_MINUS & ALL_KEYS_MASK) && minusDragReady;
        const bool currentPlusHeld = (keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK) && plusDragReady;
            
        // Handle touch dragging — activation timed by poll thread (500ms hold)
        const bool touchDragReady = buttonState.touchDragActive.load(std::memory_order_acquire);
        static bool oldTouchDragReady = false;
        
        if (currentTouchDetected && !isDragging && touchDragReady && !oldTouchDragReady) {
            // Poll thread confirmed 500ms in-bounds hold — start drag now
            isDragging = true;
            triggerOnFeedback();
            hasMoved = false;
            initialTouchPos = touchPos;
            initialFrameOffsetX = frameOffsetX;
            initialFrameOffsetY = frameOffsetY;
        } else if (currentTouchDetected && isDragging && !currentPlusHeld) {
            // Continue touch dragging (only if neither MINUS nor PLUS is held)
            const int touchX = touchPos.x;
            const int touchY = touchPos.y;
            const int deltaX = touchX - initialTouchPos.x;
            const int deltaY = touchY - initialTouchPos.y;
            
            // Check if we've moved enough to consider this a drag
            if (!hasMoved) {
                const int totalMovement = abs(deltaX) + abs(deltaY);
                if (totalMovement >= TOUCH_THRESHOLD) {
                    hasMoved = true;
                }
            }
            
            if (hasMoved) {
                // Update frame offsets with boundary checking
                const int newFrameOffsetX = std::max(minX, std::min(maxX, initialFrameOffsetX + deltaX));
                const int newFrameOffsetY = std::max(minY, std::min(maxY, initialFrameOffsetY + deltaY));
                
                frameOffsetX = newFrameOffsetX;
                frameOffsetY = newFrameOffsetY;

                updateLayerPos();
                boundsNeedUpdate = true;
            }
        } else if (!currentTouchDetected && oldTouchDetected && isDragging && !currentPlusHeld) {
            // Touch just released and we were touch dragging
            if (hasMoved) {
                // Save position when touch drag ends
                auto iniData = ult::getParsedDataFromIniFile(configIniPath);
                iniData["mini"]["frame_offset_x"] = std::to_string(frameOffsetX);
                iniData["mini"]["frame_offset_y"] = std::to_string(frameOffsetY);
                ult::saveIniFileData(configIniPath, iniData);
            }
            
            // Reset touch drag state
            isDragging = false;
            hasMoved = false;
            //isRendering = true;
            //leventClear(&renderingStopEvent);
            clearOnRelease = true;
            triggerOffFeedback(true);
        }
        oldTouchDragReady = touchDragReady && currentTouchDetected;
        

        // Handle joystick dragging (MINUS + right joystick OR PLUS + left joystick)
        if (currentPlusHeld && !isDragging) {
            // Start joystick dragging
            isDragging = true;
            //isRendering = false;
            //leventSignal(&renderingStopEvent);
            triggerOnFeedback();
        } else if (currentPlusHeld && isDragging) {
            // Continue joystick dragging
            static constexpr int JOYSTICK_DEADZONE = 20;
            
            // Choose the appropriate joystick based on which button is held
            const HidAnalogStickState& activeJoystick = joyStickPosLeft;
            
            // Only move if joystick is outside deadzone
            if (abs(activeJoystick.x) > JOYSTICK_DEADZONE || abs(activeJoystick.y) > JOYSTICK_DEADZONE) {
                // Calculate joystick magnitude (distance from center)
                const float magnitude = sqrt((float)(activeJoystick.x * activeJoystick.x + activeJoystick.y * activeJoystick.y));
                const float normalizedMagnitude = magnitude / 32767.0f; // Normalize to 0-1 range
                
                // Single smooth curve: stays very slow for wide range, then accelerates
                static constexpr float baseSensitivity = 0.00008f;  // Higher so small movements register
                static constexpr float maxSensitivity = 0.0005f;
                
                // Use x^8 curve - stays very low until ~70% then curves up sharply
                const float curveValue = pow(normalizedMagnitude, 8.0f);
                const float currentSensitivity = baseSensitivity + (maxSensitivity - baseSensitivity) * curveValue;
                
                // Calculate movement delta with fractional accumulation
                static float accumulatedX = 0.0f;
                static float accumulatedY = 0.0f;
                
                accumulatedX += (float)activeJoystick.x * currentSensitivity;
                accumulatedY += -(float)activeJoystick.y * currentSensitivity;
                
                // Extract integer movement and keep fractional part
                const int deltaX = (int)accumulatedX;
                const int deltaY = (int)accumulatedY;
                accumulatedX -= deltaX;
                accumulatedY -= deltaY;
                
                // Update frame offsets with boundary checking
                const int newFrameOffsetX = std::max(minX, std::min(maxX, frameOffsetX + deltaX));
                const int newFrameOffsetY = std::max(minY, std::min(maxY, frameOffsetY + deltaY));
                
                frameOffsetX = newFrameOffsetX;
                frameOffsetY = newFrameOffsetY;

                //if (ult::limitedMemory) {
                //    tsl::gfx::Renderer::get().setLayerPos(std::max(std::min((int)(frameOffsetX*1.5 + 0.5) - tsl::impl::currentUnderscanPixels.first, 1280-32 - tsl::impl::currentUnderscanPixels.first), 0), 0);
                //}
                updateLayerPos();
                boundsNeedUpdate = true;
            }
        } else if ((!currentPlusHeld && oldPlusHeld) && isDragging) {
            // Button just released - stop joystick dragging
            auto iniData = ult::getParsedDataFromIniFile(configIniPath);
            iniData["mini"]["frame_offset_x"] = std::to_string(frameOffsetX);
            iniData["mini"]["frame_offset_y"] = std::to_string(frameOffsetY);
            ult::saveIniFileData(configIniPath, iniData);
            isDragging = false;
            //isRendering = true;
            //leventClear(&renderingStopEvent);
            clearOnRelease = true;
            triggerOffFeedback(true);
        }
        
        // Update state for next frame
        oldTouchDetected = currentTouchDetected;
        oldPlusHeld = currentPlusHeld;
        
        // Handle existing key input logic (but don't interfere with dragging)
        if (!isDragging) {
            if (isKeyComboPressed(keysHeld, keysDown)) {
                isRendering = false;
                leventSignal(&renderingStopEvent);
                //triggerRumbleDoubleClick.store(true, std::memory_order_release);
                skipOnce = true;
                runOnce = true;
                //TeslaFPS = 60;
                if (skipMain) {
                    //lastSelectedItem = "Mini";
                    lastMode = "returning";
                    tsl::goBack();
                }
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
        }
        
        // Return true if we handled the input (during dragging)
        return isDragging;
    }
};