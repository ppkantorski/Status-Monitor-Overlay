class MainMenu;

class com_FPSGraph : public tsl::Gui {
private:
    uint8_t refreshRate = 0;
    char FPSavg_c[8];
    FpsGraphSettings settings;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    uint32_t cnt = 0;
    u64 lastDataUpdateTick = 0;
    char CPU_Load_c[12] = "    -";
    char GPU_Load_c[12] = "    -";
    char RAM_Load_c[12] = "    -";
    char SOC_TEMP_c[12] = "    -";
    char PCB_TEMP_c[12] = "    -";
    char SKIN_TEMP_c[12] = "    -";
    bool skipOnce = true;
    bool runOnce = true;
    
    // Repositioning variables (matching Mini)
    int frameOffsetX = 0;
    int frameOffsetY = 0;
    bool isDragging = false;
    size_t framePadding = 10;
    static constexpr int screenWidth = 1280;
    static constexpr int screenHeight = 720;
    static constexpr int border = 8;

    bool originalUseRightAlignment = ult::useRightAlignment;

    struct ButtonState {
        std::atomic<bool> plusDragActive{false};
        std::atomic<bool> touchDragActive{false};
    } buttonState;

    Thread touchPollThread;
    std::atomic<bool> touchPollRunning{false};

    Thread graphSampleThread;
    std::atomic<bool> graphSampleRunning{false};

    // Store actual rendered dimensions (including border)
    size_t actualTotalWidth = 0;
    size_t actualTotalHeight = 0;

    void updateLayerPos() {
        if (!ult::limitedMemory) return;
        const int pos = std::max(std::min(
            (int)(frameOffsetX * 1.5f + 0.5f) - tsl::impl::currentUnderscanPixels.first,
            1280 - 32 - tsl::impl::currentUnderscanPixels.first), 0);
        tsl::gfx::Renderer::get().setLayerPos(pos, 0);
        ult::layerEdge = frameOffsetX;  // touch-space (1280px), NOT VI-space (pos)
    }
public:
    bool isStarted = false;
    com_FPSGraph() { 
        tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        GetConfigSettings(&settings);

        if (R_SUCCEEDED(SaltySD_Connect())) {
            if (R_FAILED(SaltySD_GetDisplayRefreshRate(&refreshRate)))
                refreshRate = 0;
            svcSleepThread(100'000);
            SaltySD_Term();
        }
        
        // Load saved frame offsets
        frameOffsetX = settings.frameOffsetX;
        frameOffsetY = settings.frameOffsetY;
        framePadding = settings.framePadding;
        
       
        updateLayerPos();

        FullMode = false;
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        mutexInit(&mutex_Misc);
        mutexInit(&readings_mutex);
        StartInfoThread();
        StartFPSCounterThread();

        // Start touch polling thread for instant response at low FPS
        touchPollRunning.store(true, std::memory_order_release);
        threadCreate(&touchPollThread, [](void* arg) -> void {
            com_FPSGraph* overlay = static_cast<com_FPSGraph*>(arg);
            
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
            static constexpr u64 TOUCH_HOLD_THRESHOLD_NS = 500'000'000ULL;  // 500ms
            static constexpr u64 PLUS_HOLD_THRESHOLD_NS  = 1'000'000'000ULL; // 1s
        
            HidTouchScreenState state = {0};
            bool inputDetected;
        
            while (overlay->touchPollRunning.load(std::memory_order_acquire)) {
                // Sleep gate: touch panel is off during sleep — block until wake.
                if (tsl::hlp::waitWhileSleeping()) continue;

                // Only poll when rendering and not dragging
                {
                    inputDetected = false;
                    const u64 now = armTicksToNs(armGetSystemTick());
                    
                    // Check touch in bounds
                    if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                        const int touchX = state.touches[0].x;
                        const int touchY = state.touches[0].y;
                        
                        // Use actual dimensions, fallback to estimate if not yet rendered
                        size_t totalWidth = overlay->actualTotalWidth;
                        size_t totalHeight = overlay->actualTotalHeight;
                        
                        if (totalWidth == 0) {
                            // Fallback calculation
                            const s16 refresh_rate_offset = (overlay->refreshRate < 100) ? 21 : 28;
                            const s16 info_width = overlay->settings.showInfo ? (6 + overlay->rectangle_width/2 - 4) : 0;
                            const s16 content_width = overlay->rectangle_width + refresh_rate_offset + info_width + 1;
                            const s16 content_height = overlay->rectangle_height + 12;
                            const int bExpandFb = overlay->settings.useBorder ? (int)overlay->settings.borderThickness : 0;
                            totalWidth = content_width + (2 * border) + bExpandFb;
                            totalHeight = content_height + (2 * border) + (2 * bExpandFb);
                        }
                        
                        // Apply frame offsets — in limitedMemory the layer slides to frameOffsetX,
                        // so the overlay's logical screen position IS frameOffsetX/frameOffsetY.
                        const int overlayX = overlay->base_x + overlay->frameOffsetX;
                        const int overlayY = overlay->base_y + overlay->frameOffsetY;
                        
                        // Touch padding
                        const int touchPadding = 4;
                        const int touchableX = overlayX - touchPadding;
                        const int touchableY = overlayY - touchPadding;
                        const int touchableWidth = totalWidth + (touchPadding * 2);
                        const int touchableHeight = totalHeight + (touchPadding * 2);
                        
                        // Check if touch is within bounds — 500ms hold required
                        if (touchX >= touchableX && touchX <= touchableX + touchableWidth &&
                            touchY >= touchableY && touchY <= touchableY + touchableHeight) {
                            if (overlay->buttonState.touchDragActive.load(std::memory_order_acquire)) {
                                // Drag already confirmed — keep input signalled
                                inputDetected = true;
                            } else {
                                if (touchHoldStart == 0) touchHoldStart = now;
                                if (now - touchHoldStart >= TOUCH_HOLD_THRESHOLD_NS) {
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
                    
                    // Combine input from both controllers
                    const u64 keysHeld = padGetButtons(&pad_p1) | padGetButtons(&pad_handheld);
                    
                    // Track PLUS hold duration
                    if ((keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK)) {
                        if (plusHoldStart == 0) {
                            plusHoldStart = now;
                        }
                        if (now - plusHoldStart >= PLUS_HOLD_THRESHOLD_NS) {
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
                        }
                    }
                    lastUnderscanPixels = tsl::impl::currentUnderscanPixels;
                }
                
                svcSleepThread(16000000ULL*2); // 16ms polling
            }
        }, this, NULL, 0x1000, 0x2B, -2);
        threadStart(&touchPollThread);

        // Graph sampler thread: fixed 10 Hz (100 ms), capped at 180 samples.
        // Mirrors the Mini embedded graph sampler so the time window is always
        // 18 seconds regardless of the overlay refresh rate setting.
        graphSampleRunning.store(true, std::memory_order_release);
        threadCreate(&graphSampleThread, [](void* arg) -> void {
            com_FPSGraph* ov = static_cast<com_FPSGraph*>(arg);
            while (ov->graphSampleRunning.load(std::memory_order_acquire)) {
                svcSleepThread(100'000'000ULL); // 100 ms = 10 Hz
                if (tsl::hlp::waitWhileSleeping()) continue;
                if (!ov->graphSampleRunning.load(std::memory_order_acquire)) break;
                const float avg = useOldFPSavg ? FPSavg_old : FPSavg;
                if (avg >= 254.0f) {
                    mutexLock(&ov->readings_mutex);
                    if (!ov->readings.empty()) {
                        ov->readings.clear();
                        ov->readings.shrink_to_fit();
                    }
                    mutexUnlock(&ov->readings_mutex);
                    continue;
                }
                stats temp = {0, false};
                temp.value = static_cast<s16>(std::lround(avg));
                const float whole = std::round(avg);
                if (avg >= whole - 0.05f && avg <= whole + 0.04f)
                    temp.zero_rounded = true;
                mutexLock(&ov->readings_mutex);
                // +1: the draw loop's leftmost x = x_end - nReadings + 1. With
                // nReadings = rectangle_width + 1 that x = rectangle_x + rectangle_width
                // - rectangle_width = rectangle_x, placing the leftmost line pixel at
                // final_base_x + rectangle_x + offset + 1 = final_base_x + rectangle_x + 2,
                // flush against the border's inner-left face at final_base_x + rectangle_x + 2.
                if ((s16)ov->readings.size() >= ov->rectangle_width + 1)
                    ov->readings.erase(ov->readings.begin());
                ov->readings.push_back(temp);
                mutexUnlock(&ov->readings_mutex);
            }
        }, this, NULL, 0x1000, 0x2B, -2);
        threadStart(&graphSampleThread);
    }

    ~com_FPSGraph() {
        // Stop touch polling thread
        touchPollRunning.store(false, std::memory_order_release);
        threadWaitForExit(&touchPollThread);
        threadClose(&touchPollThread);

        // Stop graph sampler thread
        graphSampleRunning.store(false, std::memory_order_release);
        threadWaitForExit(&graphSampleThread);
        threadClose(&graphSampleThread);

        EndInfoThread();
        EndFPSCounterThread();
        FullMode = true;
        fixForeground = true;
        ult::useRightAlignment = originalUseRightAlignment;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
        if (ult::limitedMemory)
            ult::layerEdge = (ult::useRightAlignment && ult::correctFrameSize) ? (1280 - 448) : 0;
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
    s16 y_30FPS = rectangle_y+(rectangle_height / 2);
    s16 y_60FPS = rectangle_y;

    Mutex readings_mutex;  // guards 'readings' (per-frame push in update() and the draw loop)

    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {

            // Calculate content dimensions (what goes inside the border)
            const s16 refresh_rate_offset = (refreshRate < 100) ? 21 : 28;
            const s16 info_width = settings.showInfo ? (6 + rectangle_width/2 - 4) : 6;
            const s16 content_width = rectangle_width + refresh_rate_offset + info_width + 1;
            const s16 content_height = rectangle_height + 12;
            
            // Total dimensions including border
            // Expand by borderThickness on each side when the border is on so the
            // border stroke never overlaps the interior content (mirrors Mini fix).
            const int bExpand = settings.useBorder ? (int)settings.borderThickness : 0;
            const size_t totalWidth = content_width + (2 * border) + bExpand;
            const size_t totalHeight = content_height + (2 * border) + (2 * bExpand);
            
            // Store actual dimensions for input handling
            actualTotalWidth = totalWidth;
            actualTotalHeight = totalHeight;

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

            int _frameOffsetX;
            int posX, posY;
            
            // In limited memory mode, correct frameOffsetX first, then calculate display position
            if (ult::limitedMemory) {
                // Calculate valid range for frameOffsetX (in full 1280px coordinate space)
                // Matches Mini: minFrameX = framePadding - cachedBaseX (cachedBaseX==base_x==0)
                const int minFrameX = framePadding - base_x;
                const int maxFrameX = screenWidth - framePadding - totalWidth - base_x;
                
                // Clamp frameOffsetX to valid bounds
                if (frameOffsetX < minFrameX) {
                    frameOffsetX = minFrameX;
                } else if (frameOffsetX > maxFrameX) {
                    frameOffsetX = maxFrameX;
                }
                
                // Check Y bounds (same correction — drop the erroneous +border offset)
                const int minFrameY = framePadding - base_y;
                const int maxFrameY = screenHeight - framePadding - totalHeight - base_y;
                
                if (frameOffsetY < minFrameY) {
                    frameOffsetY = minFrameY;
                } else if (frameOffsetY > maxFrameY) {
                    frameOffsetY = maxFrameY;
                }
                
                // Now calculate _frameOffsetX for the 448px layer (matches Mini)
                _frameOffsetX = std::max(0, frameOffsetX - (1280-448));
                
                // Update layer position
                updateLayerPos();
                
                // Draw at base_x + _frameOffsetX, matching Mini's cachedBaseX + _frameOffsetX.
                // Do NOT subtract border — that pushes posX negative and breaks arc geometry.
                posX = base_x + _frameOffsetX;
                posY = base_y + frameOffsetY;
            } else {
                // Non-limited memory mode - use original logic with clamping
                _frameOffsetX = frameOffsetX;
                
                // Calculate position with frame offsets (for the rounded rect, which includes border)
                posX = base_x + _frameOffsetX - border;
                posY = base_y + frameOffsetY - border;
                
                // Clamp to screen bounds (accounting for total size including border)
                posX = std::max(int(framePadding), std::min(posX, static_cast<int>(screenWidth - totalWidth - framePadding)));
                posY = std::max(int(framePadding), std::min(posY, static_cast<int>(screenHeight - totalHeight - framePadding)));
            }
            
            // Draw the rounded rectangle (background)
            const tsl::Color bgColor = !isDragging
                ? settings.backgroundColor
                : settings.focusBackgroundColor;
            
            // Configurable Switch 2 frame border; offset 0 when border is off.
            const s32 borderOffset = settings.useBorder ? 1 : 0;
            // Corner radius in sp (tenths of a space) -> px (mirrors Mini), measured
            // at a fixed reference size so the default stays stable (4.0 sp ~= 16 px).
            const float _crSpW = (float)renderer->getTextDimensions(" ", false, 16).first;
            const float _crSpace = (_crSpW > 0.5f) ? _crSpW : 4.0f;
            const s32 cornerRadius = (s32)(_crSpace * (float)settings.cornerRadiusSp / 10.0f + 0.5f);
            renderer->drawRoundedRectSingleThreaded(
                posX + borderOffset, 
                posY + borderOffset, 
                totalWidth - (2*borderOffset), 
                totalHeight - (2*borderOffset),
                cornerRadius, 
                aWithOpacity(bgColor)
            );

            if (settings.useBorder) {
                const auto w2 = makeBorderWheel(settings);
                renderer->drawBorderedRoundedRect(posX, posY, totalWidth, totalHeight,
                    settings.borderThickness, cornerRadius,
                    aWithOpacity(settings.borderColor),
                    settings.useDynamicBorder ? &w2 : nullptr);
            }
            
            posX += 4;

            // Content drawing position (inside the border).
            // bExpand shifts origin right/down to account for the extra space
            // added by border compensation, keeping content clear of the stroke.
            const int final_base_x = posX + border + bExpand;
            const int final_base_y = posY + border + bExpand;

            const s16 size = (refreshRate > 60 || !refreshRate) ? 63 : (s32)(63.0/(60.0/refreshRate));
            const auto width = renderer->getTextDimensions(FPSavg_c, false, size).first;

            const s16 pos_y = size + final_base_y + rectangle_y + ((rectangle_height - size) / 2);
            const s16 pos_x = final_base_x + rectangle_x + (((rectangle_width + 1) - width) / 2);

            // Plot background: drawn immediately after the border, so the dashed
            // line, legend, FPS counter, and data line all render on top of it.
            // Skipped entirely when alpha is 0 to avoid an unnecessary draw call.
            // plotBackgroundColor is uint16_t (RGBA4444); alpha is the top nibble.
            if (settings.useGraphBackground && ((settings.plotBackgroundColor >> 12) & 0xF))
                renderer->drawRect(
                    final_base_x + rectangle_x + 2,
                    final_base_y + rectangle_y,
                    rectangle_width + 1, // +1px right: data sits on the rightmost column; widen so it no longer touches the border
                    rectangle_height + 2,
                    aWithOpacity(settings.plotBackgroundColor));

            renderer->drawDashedLine(final_base_x+rectangle_x+2 +3, final_base_y+y_30FPS, final_base_x+rectangle_x+rectangle_width+2 -3, final_base_y+y_30FPS, 6, aWithOpacity(settings.dashedLineColor));
            renderer->drawString(&legend_max[0], false, final_base_x+(rectangle_x-((refreshRate < 100) ? 15 : 22)), final_base_y+(rectangle_y+7), 10, (settings.maxFPSTextColor));
            // Right-align the min "0" with the trailing '0' of the max label:
            // place its cursor at the same x as the final '0' in legend_max so both
            // glyphs occupy the exact same pixel columns at any refresh rate.
            // zeroAdvanceFps uses getTextDimensions — the same truncating static_cast<s32>
            // path as maxAdvanceFps — so both cursor offsets round identically.
            // (lround(xAdvance * currFontSize) can differ by 1 px, causing misalignment.)
            {
                const int zeroAdvanceFps  = renderer->getTextDimensions("0", false, 10).first;
                const int maxAdvanceFps   = renderer->getTextDimensions(&legend_max[0], false, 10).first;
                renderer->drawString(&legend_min[0], false,
                    final_base_x + (rectangle_x - ((refreshRate < 100) ? 15 : 22)) + maxAdvanceFps - zeroAdvanceFps,
                    final_base_y+(rectangle_y+rectangle_height+3), 10, settings.minFPSTextColor);
            }

            // FPS counter drawn above the dashed line but below the data line.
            if (FPSavg != 254.0)
                renderer->drawString(FPSavg_c, false, pos_x, pos_y-5, size, settings.fpsColor);

            mutexLock(&readings_mutex);
            const size_t nReadings = readings.size();
            // y_old_local is a stack variable reset each draw call -- no cross-frame
            // contamination from stale state when a new sample is pushed mid-frame.
            s16 y_old_local = rectangle_y + rectangle_height;
            bool isAboveLocal = false;

            // offset = 1: the main line draws at final_base_x + x + offset + 1.
            // With x = x_end = rectangle_x + rectangle_width, the rightmost line pixel
            // lands at final_base_x + rectangle_x + rectangle_width + 2, directly left
            // of the border's right outer column at +3. Shadow falls on the border column
            // and is overwritten when the border is drawn after the loop.
            // x_end already incorporates rectangle_x (15 or 22), so no separate per-rate
            // shift is needed — offset is the same constant at all refresh rates.
            const s16 offset = 1;

            for (s16 x = x_end; x > static_cast<s16>(x_end - (s16)nReadings); x--) {
                const size_t idx = nReadings - 1 - (size_t)(x_end - x);
                s32 y_on_range = readings[idx].value + std::abs(rectangle_range_min) + 1;
                if (y_on_range < 0) {
                    y_on_range = 0;
                } else if (y_on_range > range) {
                    isAboveLocal = true;
                    y_on_range = range;
                }

                const s16 y = rectangle_y + static_cast<s16>(std::lround(
                    (float)rectangle_height * ((float)(range - y_on_range) / (float)range)));

                // Snap y_old_local on the rightmost column BEFORE evaluating color
                // so the first segment is zero-length (a point) and the color check
                // y == y_old_local fires correctly instead of comparing against the
                // initialised floor value.
                if (x == x_end)
                    y_old_local = y;

                tsl::Color color = settings.mainLineColor;
                if (y == y_old_local && !isAboveLocal && readings[idx].zero_rounded) {
                    if (y == y_30FPS || y == y_60FPS)
                        color = settings.perfectLineColor;
                    else
                        color = settings.roundedLineColor;
                }

                // Shadow: same segment offset +1x, +1y in translucent black.
                // Drawn first so the main line paints on top.
                renderer->drawLine(final_base_x+x+offset+2, final_base_y+y+1,
                                   final_base_x+x+offset+2, final_base_y+y_old_local+1,
                                   tsl::Color(0, 0, 0, 5));
                renderer->drawLine(final_base_x+x+offset+1, final_base_y+y,
                                   final_base_x+x+offset+1, final_base_y+y_old_local, color);
                isAboveLocal = false;
                y_old_local = y;
            }

            if (settings.useGraphBorder) {
                const auto w2g = makeBorderWheel(settings);
                renderer->drawBorderedRoundedRect(final_base_x+(rectangle_x - 1)+2, final_base_y+(rectangle_y - 1), rectangle_width + 3, rectangle_height + 4,
                    1, 0,
                    aWithOpacity(settings.borderColor),
                    settings.useDynamicBorder ? &w2g : nullptr);
            }

            mutexUnlock(&readings_mutex);

            if (settings.showInfo) {
                const s16 info_x = final_base_x+rectangle_width+rectangle_x + 6 +8 +1; // +1: follow the 1px-wider border
                const s16 info_y = final_base_y + 3;
                const s16 fontSize = 11;
                
                // Get line height from font size (we'll use the actual rendered height)
                const auto testDimensions = renderer->getTextDimensions("A", false, fontSize);
                const s16 lineHeight = testDimensions.second;
                
                // Starting Y position for first line
                const s16 startY = info_y + lineHeight;
                
                // Value X position (offset from labels)
                const s16 value_x = info_x + 40;

                static constexpr s16 SPACING = 1;
                
                // Compute gradient colors for temperatures
                const tsl::Color socColor = settings.useDynamicColors ? tsl::GradientColor(SOC_temperatureF) : settings.textColor;
                const tsl::Color pcbColor = settings.useDynamicColors ? tsl::GradientColor(PCB_temperatureF) : settings.textColor;
                const tsl::Color skinColor = settings.useDynamicColors ? tsl::GradientColor(static_cast<float>(skin_temperaturemiliC) / 1000.0f) : settings.textColor;
                
                // Draw each label and value pair on the same baseline
                // Line 0: CPU
                renderer->drawString("CPU", false, info_x, startY, fontSize, settings.catColor);
                renderer->drawString(CPU_Load_c, false, value_x, startY, fontSize, settings.textColor);
                
                // Line 1: GPU
                renderer->drawString("GPU", false, info_x, startY + lineHeight+SPACING, fontSize, settings.catColor);
                renderer->drawString(GPU_Load_c, false, value_x, startY + lineHeight+SPACING, fontSize, settings.textColor);
                
                // Line 2: RAM
                renderer->drawString("RAM", false, info_x, startY + lineHeight * 2+2*SPACING, fontSize, settings.catColor);
                renderer->drawString(RAM_Load_c, false, value_x, startY + lineHeight * 2+2*SPACING, fontSize, settings.textColor);
                
                // Line 3: SOC (with gradient color)
                renderer->drawString("SOC", false, info_x, startY + lineHeight * 3+3*SPACING, fontSize, settings.catColor);
                renderer->drawString(SOC_TEMP_c, false, value_x, startY + lineHeight * 3+3*SPACING, fontSize, socColor);
                
                // Line 4: PCB (with gradient color)
                renderer->drawString("PCB", false, info_x, startY + lineHeight * 4+4*SPACING, fontSize, settings.catColor);
                renderer->drawString(PCB_TEMP_c, false, value_x, startY + lineHeight * 4+4*SPACING, fontSize, pcbColor);
                
                // Line 5: SKIN (with gradient color)
                renderer->drawString("Skin", false, info_x, startY + lineHeight * 5+5*SPACING, fontSize, settings.catColor);
                renderer->drawString(SKIN_TEMP_c, false, value_x, startY + lineHeight * 5+5*SPACING, fontSize, skinColor);
            }
        });

        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        const u64 _nowTick = armGetSystemTick();

        // --- Info / value text: refresh at the SAMPLE rate (<= refresh rate) --
        // settings.sampleRate governs how often the numeric readouts (FPS number,
        // temps, CPU/GPU/RAM) are re-formatted. Decoupling this from the per-frame
        // plot keeps fast-changing numbers from flickering and trims redundant work.
        if ((_nowTick - lastDataUpdateTick) >= (systemtickfrequency / settings.sampleRate)) {
            lastDataUpdateTick = _nowTick;

            // Read display refresh rate from shared memory
            const uint8_t SaltySharedDisplayRefreshRate = *(uint8_t*)((uintptr_t)shmemGetAddr(&_sharedmemory) + 1);
            if (SaltySharedDisplayRefreshRate)
                refreshRate = SaltySharedDisplayRefreshRate;
            else refreshRate = 60;

            // Format the FPS counter string
            if (FPSavg < 254) {
                if (settings.useIntegerFPS)
                    snprintf(FPSavg_c, sizeof(FPSavg_c), "%d", (int)round(useOldFPSavg ? FPSavg_old : FPSavg));
                else
                    snprintf(FPSavg_c, sizeof(FPSavg_c), "%.1f", useOldFPSavg ? FPSavg_old : FPSavg);
            } else {
                FPSavg_c[0] = 0;
            }

            mutexLock(&mutex_Misc);

            // Format temperature strings separately for proper alignment
            snprintf(SOC_TEMP_c, sizeof SOC_TEMP_c, "%2.1f\u00B0C", SOC_temperatureF);
            snprintf(PCB_TEMP_c, sizeof PCB_TEMP_c, "%2.1f\u00B0C", PCB_temperatureF);
            snprintf(SKIN_TEMP_c, sizeof SKIN_TEMP_c, "%2d.%d\u00B0C",
                     skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);

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

            // Compute max core load (the highest usage), clamped to [0, 100]
            const double cpu_usageM = std::max(0.0, std::min(100.0,
                std::max({cpu_usage0, cpu_usage1, cpu_usage2, cpu_usage3})));

            // Clamp GPU (tenths, 0-1000) and RAM (permille, 0-1000) before display
            const uint32_t gpu_clamped = std::min(GPU_Load_u, (uint32_t)1000);
            const uint32_t ram_clamped = std::min(ramLoad[SysClkRamLoad_All], (uint32_t)1000);

            // Format output strings
            snprintf(CPU_Load_c, sizeof(CPU_Load_c), "%.1f%%", cpu_usageM);
            snprintf(GPU_Load_c, sizeof(GPU_Load_c), "%u.%u%%", gpu_clamped / 10, gpu_clamped % 10);
            snprintf(RAM_Load_c, sizeof(RAM_Load_c), "%u.%u%%",
                     ram_clamped / 10,
                     ram_clamped % 10);

            mutexUnlock(&mutex_Misc);
        }

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
    
        // Touch detection
        const bool currentTouchDetected = (touchPos.x > 0 && touchPos.y > 0 && 
                                    touchPos.x < screenWidth && touchPos.y < screenHeight);
        
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
        
        // Use actual dimensions from last render, fallback to estimate if not available
        size_t totalWidth = actualTotalWidth;
        size_t totalHeight = actualTotalHeight;
        
        if (totalWidth == 0) {
            // Fallback calculation if not yet rendered
            const s16 refresh_rate_offset = (refreshRate < 100) ? 21 : 28;
            const s16 info_width = settings.showInfo ? (6 + rectangle_width/2 - 4) : 0;
            const s16 content_width = rectangle_width + refresh_rate_offset + info_width + 1;
            const s16 content_height = rectangle_height + 12;
            const int bExpandFb = settings.useBorder ? (int)settings.borderThickness : 0;
            totalWidth = content_width + (2 * border) + bExpandFb;
            totalHeight = content_height + (2 * border) + (2 * bExpandFb);
        }
        
        // Screen boundaries for clamping.
        // limitedMemory: posX = _frameOffsetX (no border offset), frameOffsetX range = [framePadding, screenWidth-totalWidth-framePadding]
        // non-limited:   posX = frameOffsetX - border, so add border to both ends of the range
        const int borderOffset = ult::limitedMemory ? 0 : border;
        const int minX = framePadding - base_x + borderOffset;
        const int maxX = screenWidth - totalWidth - base_x - framePadding + borderOffset;
        const int minY = framePadding - base_y + borderOffset;
        const int maxY = screenHeight - totalHeight - base_y - framePadding + borderOffset;

        const bool plusDragReady = buttonState.plusDragActive.load(std::memory_order_acquire);

        // Check button states
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
            // Continue touch dragging
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
                frameOffsetX = std::max(minX, std::min(maxX, initialFrameOffsetX + deltaX));
                frameOffsetY = std::max(minY, std::min(maxY, initialFrameOffsetY + deltaY));

                updateLayerPos();
            }
        } else if (!currentTouchDetected && oldTouchDetected && isDragging && !currentPlusHeld) {
            // Touch just released
            if (hasMoved) {
                // Save position when touch drag ends
                auto iniData = ult::getParsedDataFromIniFile(configIniPath);
                iniData["fps-graph"]["frame_offset_x"] = std::to_string(frameOffsetX);
                iniData["fps-graph"]["frame_offset_y"] = std::to_string(frameOffsetY);
                ult::saveIniFileData(configIniPath, iniData);
            }
            
            // Reset touch drag state
            isDragging = false;
            hasMoved = false;
            clearOnRelease = true;
            triggerOffFeedback(true);
        }
        oldTouchDragReady = touchDragReady && currentTouchDetected;
    
        // Handle joystick dragging (MINUS + right joystick OR PLUS + left joystick)
        if (currentPlusHeld && !isDragging) {
            // Start joystick dragging
            isDragging = true;
            triggerOnFeedback();
        } else if (currentPlusHeld && isDragging) {
            // Continue joystick dragging
            static constexpr int JOYSTICK_DEADZONE = 20;
            
            // Choose the appropriate joystick based on which button is held
            const HidAnalogStickState& activeJoystick = joyStickPosLeft;
            
            // Only move if joystick is outside deadzone
            if (abs(activeJoystick.x) > JOYSTICK_DEADZONE || abs(activeJoystick.y) > JOYSTICK_DEADZONE) {
                // Calculate joystick magnitude
                const float magnitude = sqrt((float)(activeJoystick.x * activeJoystick.x + activeJoystick.y * activeJoystick.y));
                const float normalizedMagnitude = magnitude / 32767.0f;
                
                // Smooth curve for sensitivity
                static constexpr float baseSensitivity = 0.00008f;
                static constexpr float maxSensitivity = 0.0005f;
                
                const float curveValue = pow(normalizedMagnitude, 8.0f);
                const float currentSensitivity = baseSensitivity + (maxSensitivity - baseSensitivity) * curveValue;
                
                // Calculate movement delta with fractional accumulation
                static float accumulatedX = 0.0f;
                static float accumulatedY = 0.0f;
                
                accumulatedX += (float)activeJoystick.x * currentSensitivity;
                accumulatedY += -(float)activeJoystick.y * currentSensitivity;
                
                const int deltaX = (int)accumulatedX;
                const int deltaY = (int)accumulatedY;
                accumulatedX -= deltaX;
                accumulatedY -= deltaY;
                
                // Update frame offsets with boundary checking
                frameOffsetX = std::max(minX, std::min(maxX, frameOffsetX + deltaX));
                frameOffsetY = std::max(minY, std::min(maxY, frameOffsetY + deltaY));

                updateLayerPos();
            }
        } else if ((!currentPlusHeld && oldPlusHeld) && isDragging) {
            // Button just released - stop joystick dragging
            auto iniData = ult::getParsedDataFromIniFile(configIniPath);
            iniData["fps-graph"]["frame_offset_x"] = std::to_string(frameOffsetX);
            iniData["fps-graph"]["frame_offset_y"] = std::to_string(frameOffsetY);
            ult::saveIniFileData(configIniPath, iniData);
            isDragging = false;
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
                runOnce = true;
                skipOnce = true;
                TeslaFPS = 60;
                lastSelectedItem = "FPS Graph";
                lastMode = "";
                if (skipMain) {
                    lastMode = "return";
                    tsl::goBack();
                }
                else {
                    tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem 'FPS Graph'");
                    tsl::Overlay::get()->close();
                }
                return true;
            }
        }
        
        // Return true if we handled the input (during dragging)
        return isDragging;
    }
};