class MainMenu;

class com_FPS : public tsl::Gui {
private:
    char FPSavg_c[8];
    FpsCounterSettings settings;
    size_t fontsize = 0;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    bool skipOnce = true;
    bool runOnce = true;

    // Repositioning variables
    int frameOffsetX = 0;
    int frameOffsetY = 0;
    bool isDragging = false;
    size_t framePadding = 10;
    static constexpr int border = 8;

    float  displayScale   = 1.0f;  // 1.5 in 1080p pixel-perfect mode, 1.0 otherwise
    int    screenWidth    = 1280;  // framebuffer coordinate space
    int    screenHeight   = 720;
    int    _frameOffsetX1080 = 0; // content X offset within 832px buffer (updated by updateLayerPos)
    int    _frameOffsetY1080 = 0; // content Y offset within buffer (updated by updateLayerPos)

    // Dock-state-change auto-relaunch (mirrors Mini logic — see Mini.hpp for full comments).
    bool wasDockedAtLaunch = false;
    bool relaunchRequested = false;

    // Authoritative rendered height — set each draw; used by handleInput for maxY clamp.
    size_t drawCachedHeight = 0;

    u64 lastDataUpdateTick = 0;

    bool originalUseRightAlignment = ult::useRightAlignment;
    
    struct ButtonState {
        std::atomic<bool> plusDragActive{false};
        std::atomic<bool> touchDragActive{false};
    } buttonState;

    Thread touchPollThread;
    std::atomic<bool> touchPollRunning{false};

    // Store actual rendered dimensions
    size_t actualTextWidth = 0;
    size_t actualTotalWidth = 0;
    size_t actualTotalHeight = 0;

    void updateLayerPos() {
        if (ult::windowedLayerPixelPerfect) {
            // X: sliding-window — mirrors Mini exactly.
            const int contentViX = (int)(frameOffsetX * 1.5f + 0.5f);
            const int maxLayerX  = (int)tsl::cfg::ScreenWidth - (int)tsl::cfg::LayerWidth;
            const int layerViX   = std::max(0, std::min(contentViX - (int)framePadding, maxLayerX));
            _frameOffsetX1080    = contentViX - layerViX;

            if (!ult::limitedMemory) {
                // Full 1080p: 832x1080 layer pinned at Y=0 — always top+bottom anchored.
                tsl::gfx::Renderer::get().setLayerPos(layerViX, 0);
                return;
            }

            // Limited 1080p: 832x554 framebuffer, layer slides vertically.
            // Use live cfg values so underscan expansion is reflected correctly.
            const int maxLayerY      = (int)tsl::cfg::ScreenHeight - (int)tsl::cfg::LayerHeight;
            const int centeredLayerY = std::max(0, maxLayerY / 2);
            const int contentViY     = (int)(frameOffsetY * 1.5f + 0.5f);
            int layerViY;
            const int contentH   = (int)drawCachedHeight + 2 * (int)framePadding;
            const bool fitsInBuffer = (contentH <= 554);
            if (!fitsInBuffer) {
                layerViY          = centeredLayerY;
                _frameOffsetY1080 = contentViY - centeredLayerY;
            } else {
                layerViY          = std::max(0, std::min(contentViY - (int)framePadding, maxLayerY));
                _frameOffsetY1080 = contentViY - layerViY;
            }
            _frameOffsetY1080 = std::max(0, std::min(_frameOffsetY1080, 553));
            tsl::gfx::Renderer::get().setLayerPos(layerViX, layerViY);
            return;
        }
        if (!ult::limitedMemory) return;
        const int pos = std::max(std::min(
            (int)(frameOffsetX * 1.5f + 0.5f) - tsl::impl::currentUnderscanPixels.first,
            1280 - 32 - tsl::impl::currentUnderscanPixels.first), 0);
        tsl::gfx::Renderer::get().setLayerPos(pos, 0);
        ult::layerEdge = frameOffsetX;  // touch-space (1280px), NOT VI-space (pos)
    }

public:
    com_FPS() { 
        tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        GetConfigSettings(&settings);

        // Determine 1080p mode — mirrors Mini constructor logic exactly.
        // ult::windowedLayerPixelPerfect is set by main() via setup1080pIfEnabled
        // before tsl::loop<> is called.
        const bool is1080p      = ult::windowedLayerPixelPerfect;
        const bool isLimited1080p = is1080p && ult::limitedMemory;
        displayScale = is1080p ? 1.5f : 1.0f;
        screenWidth  = is1080p ?  832 : 1280;
        screenHeight = isLimited1080p ? 554 : (is1080p ? 1080 : 720);

        // Initial font: use framebuffer-mode knowledge from main(); apm is not yet
        // initialised when the constructor runs.
        apmGetPerformanceMode(&performanceMode);
        if (is1080p) {
            fontsize = settings.docked1080pFontSize;
            performanceMode = ApmPerformanceMode_Boost;
        } else if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        } else {
            fontsize = settings.dockedFontSize;
        }

        // Capture dock state for the auto-relaunch guard in update().
        wasDockedAtLaunch = is1080p || (performanceMode == ApmPerformanceMode_Boost);

        // Load saved frame offsets
        frameOffsetX = settings.frameOffsetX;
        frameOffsetY = settings.frameOffsetY;
        // Scale framePadding so it represents the same visual size across modes.
        framePadding = (size_t)(settings.framePadding * displayScale + 0.5f);

        updateLayerPos();
        
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        StartFPSCounterThread();

        // Start touch polling thread for instant response at low FPS
        touchPollRunning.store(true, std::memory_order_release);
        threadCreate(&touchPollThread, [](void* arg) -> void {
            com_FPS* overlay = static_cast<com_FPS*>(arg);
            
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
            // Touch must ORIGINATE inside the overlay bounds to arm repositioning.
            // wasTouching tracks finger-down state across polls so we can detect the
            // press edge; touchSequenceValid latches whether that press landed in bounds.
            // A finger that starts elsewhere and slides onto the overlay stays invalid
            // for the whole contact, so it can never trigger an accidental move.
            bool wasTouching = false;
            bool touchSequenceValid = false;
            const u64 TOUCH_HOLD_THRESHOLD_NS = (u64)overlay->settings.touchMoveDelayMs * 1'000'000ULL;
            const u64 PLUS_HOLD_THRESHOLD_NS  = (u64)overlay->settings.buttonMoveDelayMs * 1'000'000ULL;
        
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
                            // Fallback calculation (no renderer available)
                            size_t approxFontSize = overlay->fontsize;
                            if (approxFontSize == 0) approxFontSize = 50;
                            const size_t textWidth = approxFontSize * 4;
                            const size_t margin = (approxFontSize / 8);
                            const size_t innerWidth = textWidth + margin;
                            const size_t innerHeight = approxFontSize;
                            // Estimate space width from font size (mirrors miniSpaceWidthPx fallback).
                            const float spaceEst = (float)approxFontSize * 0.25f;
                            const int btPx = std::max(1, (int)(spaceEst * (float)overlay->settings.borderThickness / 10.0f + 0.5f));
                            const int bExpFb = overlay->settings.useBorder ? btPx : 0;
                            totalWidth = innerWidth + (2 * border) + (2 * bExpFb);
                            totalHeight = innerHeight + (2 * border) + (2 * bExpFb);
                        }
                        
                        // Apply frame offsets
                        const int overlayX = overlay->frameOffsetX;
                        const int overlayY = overlay->frameOffsetY;
                        
                        // Touch padding
                        const int touchPadding = 4;
                        const int touchableX = overlayX - touchPadding;
                        const int touchableY = overlayY - touchPadding;
                        const int touchableWidth = totalWidth + (touchPadding * 2);
                        const int touchableHeight = totalHeight + (touchPadding * 2);
                        
                        // Check if touch is within bounds — hold required.
                        const bool inBounds = (
                            touchX >= touchableX && touchX <= touchableX + touchableWidth &&
                            touchY >= touchableY && touchY <= touchableY + touchableHeight);
                        
                        // Latch validity on the press edge only. If the finger went down
                        // outside the overlay, this contact can never arm repositioning,
                        // even if it is later dragged over the overlay.
                        if (!wasTouching) touchSequenceValid = inBounds;
                        wasTouching = true;
                        
                        if (inBounds && touchSequenceValid) {
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
                        // No touch detected — finger lifted. Reset hold timer, the
                        // drag-ready flag, and the origin latch for the next contact.
                        touchHoldStart = 0;
                        wasTouching = false;
                        touchSequenceValid = false;
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
    }

    ~com_FPS() {
        // Stop touch polling thread
        touchPollRunning.store(false, std::memory_order_release);
        threadWaitForExit(&touchPollThread);
        threadClose(&touchPollThread);

        TeslaFPS = 60;
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

    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            // Calculate text dimensions
            const auto [textWidth, textHeight] = renderer->getTextDimensions(
                (FPSavg != 254.0) ? FPSavg_c : "--", false, fontsize
            );

            const size_t margin = (fontsize / 8);
            
            // Inner rectangle dimensions (content area)
            const size_t innerWidth = textWidth + margin;
            const size_t innerHeight = textHeight;

            // Space width at fixed reference size (same measurement used for cornerRadius below).
            // Used to convert borderThickness (tenths of a space) into pixels so that the border
            // stays proportional to the font regardless of font size scaling.
            const float _crSpW = (float)renderer->getTextDimensions(" ", false, 16).first;
            const float _crSpace = (_crSpW > 0.5f) ? _crSpW : 4.0f;
            const int borderThicknessPx = std::max(1, (int)(_crSpace * (float)settings.borderThickness / 10.0f + 0.5f));

            // Expand the rounded rect by borderThicknessPx on each side when the
            // border is on, so the border stroke never overlaps interior content.
            // Both axes expand by 2*bExpand (left+right, top+bottom) so the gap
            // is symmetric on all sides.
            const int bExpand = settings.useBorder ? borderThicknessPx : 0;
            
            // Total dimensions including border (+ border-compensation when on)
            const size_t totalWidth = innerWidth + (2 * border) + (2 * bExpand);
            const size_t totalHeight = innerHeight + (2 * border) + (2 * bExpand);
            
            // Store actual dimensions for input handling
            actualTextWidth = textWidth;
            actualTotalWidth = totalWidth;
            actualTotalHeight = totalHeight;
            
            // Calculate position with frame offsets
            //int posX = frameOffsetX;
            //int posY = frameOffsetY;

            int posX, posY;

            // Store rendered height for handleInput maxY clamp.
            drawCachedHeight = totalHeight;

            if (ult::windowedLayerPixelPerfect) {
                // 1080p pixel-perfect mode.
                // updateLayerPos() computes _frameOffsetX1080 / _frameOffsetY1080 so
                // screenPos = layerViPos + bufferOffset = contentViPos (seamless).
                // Clamp frameOffsetX (720-logical) then let updateLayerPos derive buffer coords.
                const int minFrameX = (int)((framePadding * 2 + 2) / 3);
                const int maxLayerX1080 = (int)tsl::cfg::ScreenWidth - (int)tsl::cfg::LayerWidth;
                const int maxBufX1080   = screenWidth - (int)totalWidth - (int)framePadding;
                const int maxFrameX = std::max(minFrameX,
                    (int)((maxLayerX1080 + std::max(0, maxBufX1080)) / 1.5f));
                frameOffsetX = std::max(minFrameX, std::min(maxFrameX, frameOffsetX));

                if (!ult::limitedMemory) {
                    // Full 1080p: Y runs 0..1080 in framebuffer space.
                    const int minFrameY = (int)framePadding;
                    const int maxFrameY = screenHeight - (int)framePadding - (int)totalHeight;
                    frameOffsetY = std::max(minFrameY, std::min(maxFrameY, frameOffsetY));
                } else {
                    // Limited 1080p: Y is in 720-logical space, updateLayerPos maps to buffer.
                    const int maxLayerY1080 = (int)tsl::cfg::ScreenHeight - (int)tsl::cfg::LayerHeight;
                    const int maxBufY1080   = screenHeight - (int)framePadding - (int)totalHeight;
                    const int minFrameY720  = (int)(framePadding / 1.5f + 0.5f);
                    const int maxFrameY720  = std::max(minFrameY720,
                        (int)((maxLayerY1080 + std::max(0, maxBufY1080)) / 1.5f));
                    frameOffsetY = std::max(minFrameY720, std::min(maxFrameY720, frameOffsetY));
                }

                updateLayerPos();

                posX = std::max((int)framePadding,
                    std::min(_frameOffsetX1080, screenWidth - (int)totalWidth - (int)framePadding));
                if (!ult::limitedMemory) {
                    posY = frameOffsetY; // 1:1 with framebuffer in full 1080p
                } else {
                    posY = std::max((int)framePadding,
                        std::min(_frameOffsetY1080, screenHeight - (int)totalHeight - (int)framePadding));
                }
            } else if (ult::limitedMemory) {
                // Legacy limited-memory mode (448px wide layer, 720p).
                const int minFrameX = (int)framePadding;
                const int maxFrameX = 1280 - (int)framePadding - (int)totalWidth;
                frameOffsetX = std::max(minFrameX, std::min(maxFrameX, frameOffsetX));

                const int minFrameY = (int)framePadding;
                const int maxFrameY = screenHeight - (int)framePadding - (int)totalHeight;
                frameOffsetY = std::max(minFrameY, std::min(maxFrameY, frameOffsetY));

                // Map from full 1280px coordinate space to 448px buffer.
                const int _frameOffsetX = std::max(0, frameOffsetX - (1280 - 448));
                posX = _frameOffsetX;
                posY = frameOffsetY;
                updateLayerPos();
            } else {
                // Full 720p (non-limited, non-1080p).
                posX = std::max((int)framePadding,
                    std::min(frameOffsetX, screenWidth - (int)totalWidth - (int)framePadding));
                posY = std::max((int)framePadding,
                    std::min(frameOffsetY, screenHeight - (int)totalHeight - (int)framePadding));
            }
            
            // Draw the rounded rectangle (background)
            const tsl::Color bgColor = !isDragging
                ? settings.backgroundColor
                : settings.focusBackgroundColor;
            
            // Configurable Switch 2 frame border; offset 0 when border is off.
            const s32 borderOffset = settings.useBorder ? 1 : 0;
            // Corner radius in sp (tenths of a space) -> px. _crSpace is already
            // computed above (reused from the borderThicknessPx calculation).
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
                renderer->drawBorderedRoundedRect(
                    posX, posY, totalWidth, totalHeight,
                    borderThicknessPx, cornerRadius,
                    aWithOpacity(settings.borderColor),
                    settings.useDynamicBorder ? &w2 : nullptr);
            }
            
            // Calculate centered text position within the bordered area.
            // bExpand shifts the content origin right and down to stay inside
            // the extra space added by the border compensation.
            const int textX = posX + border + (margin / 2) + bExpand;
            const int textY = posY + border + (fontsize - margin) + bExpand;
            
            // Draw the text
            renderer->drawString(
                (FPSavg != 254.0) ? FPSavg_c : "--",
                false, 
                textX, 
                textY, 
                fontsize, 
                settings.textColor
            );
        });

        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        apmGetPerformanceMode(&performanceMode);

        // ── Dock-state-change auto-relaunch (mirrors Mini) ────────────────
        if (!relaunchRequested &&
            (performanceMode == ApmPerformanceMode_Normal ||
             performanceMode == ApmPerformanceMode_Boost)) {
            const bool isDockedNow = (performanceMode == ApmPerformanceMode_Boost);
            if (isDockedNow != wasDockedAtLaunch) {
                const std::string val = ult::parseValueFromIniSection(
                    configIniPath, "fps-counter", "use_1080p_docked");
                bool wants1080p = false;
                if (!val.empty()) {
                    std::string upper = val;
                    for (char& c : upper) c = (char)std::toupper((unsigned char)c);
                    wants1080p = (upper != "FALSE");
                }
                if (wants1080p) {
                    relaunchRequested = true;
                    std::string ovlPath = filepath;
                    if (ovlPath.empty()) ovlPath = folderpath + filename;
                    std::string newArgs;
                    if (!originalLaunchArgs.empty())
                        newArgs = originalLaunchArgs + " --silentLaunch";
                    else
                        newArgs = "--silentLaunch";
                    tsl::setNextOverlay(ovlPath, newArgs);
                    skipClosingExitFeedback = true;
                    launchComboHasTriggered.store(true, std::memory_order_release);
                    ult::launchingOverlay.store(true, std::memory_order_release);
                    tsl::Overlay::get()->close();
                    return;
                }
            }
        }

        if (ult::windowedLayerPixelPerfect) {
            // In 1080p mode the font size is fixed at docked1080pFontSize regardless of
            // live dock state — the relaunch above handles the actual mode transition.
            fontsize = settings.docked1080pFontSize;
        } else if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            fontsize = settings.dockedFontSize;
        }
        const u64 _nowTick = armGetSystemTick();
        const bool shouldUpdateData = (_nowTick - lastDataUpdateTick) >= (systemtickfrequency / settings.sampleRate);
        if (shouldUpdateData) {
            lastDataUpdateTick = _nowTick;
            if (settings.useIntegerFPS) {
                snprintf(FPSavg_c, sizeof FPSavg_c, "%d", (int)round(useOldFPSavg ? FPSavg_old : FPSavg));
            } else {
                snprintf(FPSavg_c, sizeof FPSavg_c, "%2.1f", useOldFPSavg ? FPSavg_old : FPSavg);
            }
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
        const bool currentTouchDetected = ((int)touchPos.x > 0 && (int)touchPos.y > 0 &&
                                    (int)touchPos.x < screenWidth && (int)touchPos.y < screenHeight);
        
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
            // Fallback calculation if not yet rendered (no renderer available)
            const size_t textWidth = fontsize * 4;
            const size_t margin = (fontsize / 8);
            const size_t innerWidth = textWidth + margin;
            const size_t innerHeight = fontsize + (margin / 2);
            // Estimate space width from font size (mirrors miniSpaceWidthPx fallback).
            const float spaceEst = (float)fontsize * 0.25f;
            const int btPx = std::max(1, (int)(spaceEst * (float)settings.borderThickness / 10.0f + 0.5f));
            const int bExpFb = settings.useBorder ? btPx : 0;
            totalWidth = innerWidth + (2 * border) + (2 * bExpFb);
            totalHeight = innerHeight + (2 * border) + (2 * bExpFb);
        }
        
        // Screen boundaries for clamping — sliding-window-aware, mirrors Mini.
        const int maxLayerX1080 = (int)tsl::cfg::ScreenWidth - (int)tsl::cfg::LayerWidth;
        const int maxBufX1080   = screenWidth - (int)totalWidth - (int)framePadding;
        const int minX = ult::windowedLayerPixelPerfect
                       ? ((int)(framePadding * 2 + 2) / 3)
                       : (int)framePadding;
        const int maxX = ult::windowedLayerPixelPerfect
                       ? std::max(minX, (int)((maxLayerX1080 + std::max(0, maxBufX1080)) / 1.5f))
                       : (screenWidth - (int)totalWidth - (int)framePadding);

        // Y bounds — use 720-logical space for limited 1080p (same as Mini).
        const int logicalScreenH  = (ult::windowedLayerPixelPerfect && ult::limitedMemory) ? 720 : screenHeight;
        const int overlayHeight   = (drawCachedHeight > 0) ? (int)drawCachedHeight : (int)totalHeight;
        const int logicalOverlayH = (ult::windowedLayerPixelPerfect && ult::limitedMemory)
                                  ? (int)(overlayHeight / 1.5f + 0.5f) : overlayHeight;
        const int logicalPadding  = (ult::windowedLayerPixelPerfect && ult::limitedMemory)
                                  ? (int)(framePadding / 1.5f + 0.5f) : (int)framePadding;
        const int minY = logicalPadding;
        const int maxLayerY1080   = (int)tsl::cfg::ScreenHeight - (int)tsl::cfg::LayerHeight;
        const int maxBufY1080     = screenHeight - (int)framePadding - overlayHeight;
        const int maxY = (ult::windowedLayerPixelPerfect && ult::limitedMemory)
                       ? std::max(minY, (int)((maxLayerY1080 + std::max(0, maxBufY1080)) / 1.5f))
                       : (logicalScreenH - logicalOverlayH - logicalPadding);

        // Y-movement rate normalisation — identical to Mini (see Mini.hpp for full comment).
        // Full 1080p is 1:1 with VI (1 frameOffsetY unit = 1 display px), while every
        // other mode produces 1.5 display px per unit.  Scale Y by 1.5 in full 1080p only.
        const float yMovementScale = (ult::windowedLayerPixelPerfect && !ult::limitedMemory) ? 1.5f : 1.0f;

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
            const int deltaX    = touchX - initialTouchPos.x;
            const int rawDeltaY = touchY - initialTouchPos.y;
            const int deltaY    = (yMovementScale == 1.0f)
                                ? rawDeltaY
                                : (int)(rawDeltaY * yMovementScale + (rawDeltaY >= 0 ? 0.5f : -0.5f));
            
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
                iniData["fps-counter"]["frame_offset_x"] = std::to_string(frameOffsetX);
                iniData["fps-counter"]["frame_offset_y"] = std::to_string(frameOffsetY);
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
            // Accept EITHER stick while PLUS is held: use whichever is deflected
            // further from center, so PLUS + left stick and PLUS + right stick both
            // reposition the overlay identically.
            const long leftMag2  = (long)joyStickPosLeft.x  * (long)joyStickPosLeft.x  +
                                   (long)joyStickPosLeft.y  * (long)joyStickPosLeft.y;
            const long rightMag2 = (long)joyStickPosRight.x * (long)joyStickPosRight.x +
                                   (long)joyStickPosRight.y * (long)joyStickPosRight.y;
            const HidAnalogStickState& activeJoystick =
                (rightMag2 > leftMag2) ? joyStickPosRight : joyStickPosLeft;
            
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
                accumulatedY += -(float)activeJoystick.y * currentSensitivity * yMovementScale;
                
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
            iniData["fps-counter"]["frame_offset_x"] = std::to_string(frameOffsetX);
            iniData["fps-counter"]["frame_offset_y"] = std::to_string(frameOffsetY);
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
                lastSelectedItem = "FPS Counter";
                lastMode = "";
                if (skipMain) {
                    lastMode = "return";
                    tsl::goBack();
                }
                else {
                    tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem 'FPS Counter'");
                    tsl::Overlay::get()->close();
                }
                return true;
            }
        }
        
        // Return true if we handled the input (during dragging)
        return isDragging;
    }
};