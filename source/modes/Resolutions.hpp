class MainMenu;

class ResolutionsOverlay : public tsl::Gui {
private:
    char Resolutions_c[512] = {0};
    char Resolutions2_c[512] = {0};
    ResolutionSettings settings;
    bool skipOnce = true;
    bool runOnce = true;
    u64 lastDataUpdateTick = 0;

    // Repositioning variables (matching Mini)
    int frameOffsetX = 0;
    int frameOffsetY = 0;
    bool isDragging = false;
    size_t framePadding = 10;
    // Framebuffer coordinate space: 1280x720 normally, 832x1080 (full) or 832x554
    // (limited) in 1080p pixel-perfect docked mode. Set in the constructor.
    int screenWidth  = 1280;
    int screenHeight = 720;

    // ── 1080p pixel-perfect docked mode (mirrors FPS Counter / Mini) ──
    float  displayScale      = 1.0f;   // 1.5 in 1080p pixel-perfect mode, 1.0 otherwise
    int    _frameOffsetX1080 = 0;      // content X offset within 832px buffer (updated by updateLayerPos)
    int    _frameOffsetY1080 = 0;      // content Y offset within buffer (updated by updateLayerPos)
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    bool wasDockedAtLaunch = false;    // dock-state-change auto-relaunch guard
    bool relaunchRequested = false;
    size_t drawCachedHeight = 0;       // authoritative rendered height (set each draw)
    int    actualTotalWidth  = 0;      // rendered box size (incl. border expansion), for input clamping
    int    actualTotalHeight = 0;

    // Scale a base (720p) pixel distance into framebuffer pixels.
    // Identity at displayScale == 1.0, so 720p geometry is unchanged.
    inline int sc(float v) const { return (int)lround(v * displayScale); }

    bool originalUseRightAlignment = ult::useRightAlignment;

    struct ButtonState {
        std::atomic<bool> plusDragActive{false};
        std::atomic<bool> touchDragActive{false};
    } buttonState;

    Thread touchPollThread;
    std::atomic<bool> touchPollRunning{false};

    std::atomic<bool> inputDetected{false};

    void updateLayerPos() {
        if (ult::windowedLayerPixelPerfect) {
            // X: sliding-window — mirrors FPS Counter / Mini exactly.
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
    ResolutionsOverlay() {
        tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        GetConfigSettings(&settings);

        // Determine 1080p mode — mirrors FPS Counter / Mini constructor logic.
        // ult::windowedLayerPixelPerfect is set by main() via setup1080pIfEnabled
        // before tsl::loop<> is called.
        const bool is1080p        = ult::windowedLayerPixelPerfect;
        const bool isLimited1080p = is1080p && ult::limitedMemory;
        displayScale = is1080p ? 1.5f : 1.0f;
        screenWidth  = is1080p ?  832 : 1280;
        screenHeight = isLimited1080p ? 554 : (is1080p ? 1080 : 720);

        apmGetPerformanceMode(&performanceMode);
        if (is1080p) performanceMode = ApmPerformanceMode_Boost;
        // Capture dock state for the auto-relaunch guard in update().
        wasDockedAtLaunch = is1080p || (performanceMode == ApmPerformanceMode_Boost);
        
        // Load saved frame offsets
        frameOffsetX = settings.frameOffsetX;
        frameOffsetY = settings.frameOffsetY;
        // Scale framePadding so it represents the same visual size across modes.
        framePadding = (size_t)(settings.framePadding * displayScale + 0.5f);
        
        updateLayerPos();

        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        StartFPSCounterThread();

        // Start touch polling thread for instant response at low FPS
        touchPollRunning.store(true, std::memory_order_release);
        threadCreate(&touchPollThread, [](void* arg) -> void {
            ResolutionsOverlay* overlay = static_cast<ResolutionsOverlay*>(arg);
            
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
            
        
            while (overlay->touchPollRunning.load(std::memory_order_acquire)) {
                // Sleep gate: touch panel is off during sleep — block until wake.
                if (tsl::hlp::waitWhileSleeping()) continue;

                // Only poll when rendering and not dragging
                {
                    overlay->inputDetected.store(false, std::memory_order_release);
                    const u64 now = armTicksToNs(armGetSystemTick());
                    
                    // Check touch in bounds
                    if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                        const int touchX = state.touches[0].x;
                        const int touchY = state.touches[0].y;
                        
                        // Calculate bounds (same logic as handleInput)
                        const int overlayX = overlay->frameOffsetX;
                        const int overlayY = overlay->frameOffsetY;
                        
                        // Overlay dimensions based on game state
                        int overlayWidth, overlayHeight;
                        overlayWidth = 360-20;
                        overlayHeight = 200;
                        // Expand to match draw path when border is on
                        if (overlay->settings.useBorder) {
                            // Fixed reference space width (4.0px at size 16), same fallback as _crSpace floor.
                            const float spaceRef = 4.0f;
                            const int btPx = std::max(1, (int)(spaceRef * (float)overlay->settings.borderThickness / 10.0f + 0.5f));
                            overlayWidth  += btPx;
                            overlayHeight += 2 * btPx;
                        }
                        
                        // Add touch padding
                        const int touchPadding = 4;
                        const int touchableX = overlayX - touchPadding;
                        const int touchableY = overlayY - touchPadding;
                        const int touchableWidth = overlayWidth + (touchPadding * 2);
                        const int touchableHeight = overlayHeight + (touchPadding * 2);
                        
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
                                overlay->inputDetected.store(true, std::memory_order_release);
                            } else {
                                if (touchHoldStart == 0) touchHoldStart = now;
                                if (now - touchHoldStart >= TOUCH_HOLD_THRESHOLD_NS) {
                                    overlay->inputDetected.store(true, std::memory_order_release);
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
                            // Long enough to start drag
                            overlay->inputDetected.store(true, std::memory_order_release);
                            overlay->buttonState.plusDragActive.exchange(true, std::memory_order_acq_rel);
                        }
                    }

                    else {
                        plusHoldStart = 0;
                        overlay->buttonState.plusDragActive.exchange(false, std::memory_order_acq_rel);
                    }
                    
                    // Disable rendering on any input, re-enable when no input
                    static bool resetOnce = true;
                    if (overlay->inputDetected.load(std::memory_order_acquire)) {
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

    ~ResolutionsOverlay() {
        // Stop touch polling thread
        touchPollRunning.store(false, std::memory_order_release);
        threadWaitForExit(&touchPollThread);
        threadClose(&touchPollThread);

        EndFPSCounterThread();
        TeslaFPS = 60;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
        ult::useRightAlignment = originalUseRightAlignment;
        fixForeground = true;
        FullMode = true;

        if (ult::limitedMemory)
            ult::layerEdge = (ult::useRightAlignment && ult::correctFrameSize) ? (1280 - 448) : 0;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    bool gameStart = false;
    uint8_t resolutionLookup = 0;
    u64 lastGameSeenTick = 0;
    bool waitingForGame = true;

    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            int base_y = 0;
            int base_x = 0;
            int clippingOffsetX = 0, clippingOffsetY = 0;
        
            int total_width = sc(360 - 20);
            int total_height = sc(200);

            // Space width at fixed reference size — used for both borderThicknessPx and
            // cornerRadius below, so we compute it once here before bExpand.
            const float _crSpW = (float)renderer->getTextDimensions(" ", false, sc(16)).first;
            const float _crSpace = (_crSpW > 0.5f) ? _crSpW : 4.0f;
            const int borderThicknessPx = std::max(1, (int)(_crSpace * (float)settings.borderThickness / 10.0f + 0.5f));

            // Expand the rounded rect by borderThicknessPx on each side when the
            // border is on, so the border stroke never overlaps interior content.
            // Mirrors the Mini fix: right side += borderThicknessPx,
            // top + bottom += borderThicknessPx each.
            const int bExpand = settings.useBorder ? borderThicknessPx : 0;
            total_width  += bExpand;
            total_height += 2 * bExpand;

            // Store rendered size for handleInput clamping / limited-1080p layer fit.
            actualTotalWidth  = total_width;
            actualTotalHeight = total_height;
            drawCachedHeight  = total_height;
            
            int _frameOffsetX;
            
            if (ult::windowedLayerPixelPerfect) {
                // 1080p pixel-perfect mode (mirrors FPS Counter).
                // updateLayerPos() computes _frameOffsetX1080 / _frameOffsetY1080 so
                // screenPos = layerViPos + bufferOffset = contentViPos (seamless).
                // frameOffsetX is the box's left edge in 720-logical space.
                const int minFrameX = (int)((framePadding * 2 + 2) / 3);
                const int maxLayerX1080 = (int)tsl::cfg::ScreenWidth - (int)tsl::cfg::LayerWidth;
                const int maxBufX1080   = screenWidth - total_width - (int)framePadding;
                const int maxFrameX = std::max(minFrameX,
                    (int)((maxLayerX1080 + std::max(0, maxBufX1080)) / 1.5f));
                frameOffsetX = std::max(minFrameX, std::min(maxFrameX, frameOffsetX));

                if (!ult::limitedMemory) {
                    // Full 1080p: Y runs 0..1080 in framebuffer space.
                    const int minFrameY = (int)framePadding;
                    const int maxFrameY = screenHeight - (int)framePadding - total_height;
                    frameOffsetY = std::max(minFrameY, std::min(maxFrameY, frameOffsetY));
                } else {
                    // Limited 1080p: Y is in 720-logical space, updateLayerPos maps to buffer.
                    const int maxLayerY1080 = (int)tsl::cfg::ScreenHeight - (int)tsl::cfg::LayerHeight;
                    const int maxBufY1080   = screenHeight - (int)framePadding - total_height;
                    const int minFrameY720  = (int)(framePadding / 1.5f + 0.5f);
                    const int maxFrameY720  = std::max(minFrameY720,
                        (int)((maxLayerY1080 + std::max(0, maxBufY1080)) / 1.5f));
                    frameOffsetY = std::max(minFrameY720, std::min(maxFrameY720, frameOffsetY));
                }

                updateLayerPos();

                // Draw position inside the 832px buffer. Expressed via clippingOffset so the
                // shared final_base_x/y computation below is unchanged:
                //   final_base = base(0) + _frameOffsetX + clippingOffset.
                _frameOffsetX = std::max((int)framePadding,
                    std::min(_frameOffsetX1080, screenWidth - total_width - (int)framePadding));
                if (!ult::limitedMemory) {
                    clippingOffsetY = 0; // frameOffsetY is 1:1 with the framebuffer
                } else {
                    const int bufY = std::max((int)framePadding,
                        std::min(_frameOffsetY1080, screenHeight - total_height - (int)framePadding));
                    clippingOffsetY = bufY - frameOffsetY;
                }
            // In limited memory mode, correct frameOffsetX first, then calculate _frameOffsetX
            } else if (ult::limitedMemory) {
                // Calculate valid range for frameOffsetX (in full 1280px coordinate space)
                const int minFrameX = framePadding - base_x;
                const int maxFrameX = screenWidth - framePadding - total_width - base_x;
                
                // Clamp frameOffsetX to valid bounds
                if (frameOffsetX < minFrameX) {
                    frameOffsetX = minFrameX;
                } else if (frameOffsetX > maxFrameX) {
                    frameOffsetX = maxFrameX;
                }
                
                // Check Y bounds
                const int minFrameY = framePadding - base_y;
                const int maxFrameY = screenHeight - framePadding - total_height - base_y;
                
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
                
                // Check clipping bounds
                if (base_x + _frameOffsetX < int(framePadding))
                    clippingOffsetX = framePadding - (base_x + _frameOffsetX);
                else if ((base_x + _frameOffsetX + total_width) > static_cast<int>(screenWidth - framePadding))
                    clippingOffsetX = (screenWidth - framePadding) - (base_x + _frameOffsetX + total_width);
            
                if (base_y + frameOffsetY < int(framePadding))
                    clippingOffsetY = framePadding - (base_y + frameOffsetY);
                else if ((base_y + frameOffsetY + total_height) > static_cast<int>(screenHeight - framePadding))
                    clippingOffsetY = (screenHeight - framePadding) - (base_y + frameOffsetY + total_height);
            }

            const int final_base_x = base_x + _frameOffsetX + clippingOffsetX;
            const int final_base_y = base_y + frameOffsetY + clippingOffsetY;
        
            const tsl::Color bgColor = !isDragging ? settings.backgroundColor : settings.focusBackgroundColor;
        
            const u64 curTick = armGetSystemTick();

            // guard: ensure lastGameSeenTick is initialized to something reasonable
            if (lastGameSeenTick == 0) lastGameSeenTick = curTick;

            // threshold in ns (100 ms)
            static constexpr u64 CHECK_NS = 2000000000ULL;
        
            // Configurable Switch 2 frame border; offset 0 when border is off.
            const int borderOffset = settings.useBorder ? 1 : 0;
            // Corner radius in sp (tenths of a space) -> px. _crSpace is already
            // computed above (reused from the borderThicknessPx calculation).
            const int cornerRadius = (int)(_crSpace * (float)settings.cornerRadiusSp / 10.0f + 0.5f);
            // Game detected
            if (gameStart && NxFps && NxFps->API >= 1 && (Resolutions_c[0] != '\0' && Resolutions2_c[0] != '\0')) {
                lastGameSeenTick = curTick;
                waitingForGame = true; // reset waiting state so next missing cycle shows "Checking..."
                renderer->drawRoundedRectSingleThreaded(final_base_x + borderOffset, final_base_y + borderOffset, total_width - (2*borderOffset), total_height - (2*borderOffset), cornerRadius, aWithOpacity(bgColor));
        
                int xOffset = sc(10);
                int yOffset = sc(10) + bExpand; // shift down by bExpand to stay clear of expanded top border
                renderer->drawString("Depth", false, xOffset + final_base_x + sc(20), yOffset + final_base_y + sc(20), sc(20), settings.catColor);
                renderer->drawString(Resolutions_c, false, xOffset + final_base_x + sc(20), yOffset + final_base_y + sc(55), sc(18), settings.textColor);
                renderer->drawString("Viewport", false, xOffset + final_base_x + sc(180), yOffset + final_base_y + sc(20), sc(20), settings.catColor);
                renderer->drawString(Resolutions2_c, false, xOffset + final_base_x + sc(180), yOffset + final_base_y + sc(55), sc(18), settings.textColor);
            }
            // Game not detected
            else {
                renderer->drawRoundedRectSingleThreaded(final_base_x + borderOffset, final_base_y + borderOffset, total_width - (2*borderOffset), total_height - (2*borderOffset), cornerRadius, aWithOpacity(bgColor));
        
                // Check elapsed time since last game detection
                u64 elapsed_ns = armTicksToNs(curTick - lastGameSeenTick);
                const bool under100ms = (elapsed_ns < CHECK_NS); // 100ms
        
                std::string msg;
                if (under100ms && waitingForGame)
                    msg = "Checking for game...";
                else {
                    msg = "Game is not running\nor is incompatible.";
                    waitingForGame = false;
                }
        
                const int msgFontSize = sc(20);
                const auto [textWidth, textHeight] = renderer->getTextDimensions(msg, false, msgFontSize);
                const int text_x = final_base_x + (total_width - textWidth) / 2;
                const int text_y = final_base_y + (total_height) / 2;
                
                renderer->drawString(msg, false, text_x, (under100ms && waitingForGame) ? text_y+textHeight/2 : text_y, msgFontSize, (under100ms && waitingForGame) ? 0xFFFF : 0xF00F);
            }

            if (settings.useBorder) {
                const auto w2 = makeBorderWheel(settings);
                renderer->drawBorderedRoundedRect(final_base_x, final_base_y, total_width, total_height,
                    borderThicknessPx, cornerRadius,
                    aWithOpacity(settings.borderColor),
                    settings.useDynamicBorder ? &w2 : nullptr);
            }
        });
        
        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        apmGetPerformanceMode(&performanceMode);

        // ── Dock-state-change auto-relaunch (mirrors FPS Counter / Mini) ──
        // The framebuffer size is chosen at launch, so when 1080p docked is enabled
        // a dock/undock transition relaunches silently to pick the right layout.
        if (!relaunchRequested &&
            (performanceMode == ApmPerformanceMode_Normal ||
             performanceMode == ApmPerformanceMode_Boost)) {
            const bool isDockedNow = (performanceMode == ApmPerformanceMode_Boost);
            if (isDockedNow != wasDockedAtLaunch) {
                const std::string val = ult::parseValueFromIniSection(
                    configIniPath, "game_resolutions", "use_1080p_docked");
                // Missing key defaults to true (matches setup1080pIfEnabled).
                bool wants1080p = true;
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

        const u64 _nowTick = armGetSystemTick();
        const bool shouldUpdateData = (_nowTick - lastDataUpdateTick) >= (systemtickfrequency / settings.sampleRate);
        if (shouldUpdateData) lastDataUpdateTick = _nowTick;

        if (shouldUpdateData && gameStart && NxFps) {
            if (!resolutionLookup) {
                NxFps -> renderCalls[0].calls = 0xFFFF;
                resolutionLookup = 1;
                return; // wait for game to overwrite sentinel before displaying
            }
            else if (resolutionLookup == 1) {
                if ((NxFps -> renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
                else return;
            }
            memcpy(&m_resolutionRenderCalls, &(NxFps -> renderCalls), sizeof(m_resolutionRenderCalls));
            memcpy(&m_resolutionViewportCalls, &(NxFps -> viewportCalls), sizeof(m_resolutionViewportCalls));
            qsort(m_resolutionRenderCalls, 8, sizeof(resolutionCalls), settings.prioritizeMax169 ? compare : compareOriginal);
            qsort(m_resolutionViewportCalls, 8, sizeof(resolutionCalls), settings.prioritizeMax169 ? compare : compareOriginal);
            if (settings.prioritizeMax169) {
                prioritizeMax169Order(m_resolutionRenderCalls, 8);
                prioritizeMax169Order(m_resolutionViewportCalls, 8);
            }
            snprintf(Resolutions_c, sizeof Resolutions_c,
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d",
                m_resolutionRenderCalls[0].width, m_resolutionRenderCalls[0].height, m_resolutionRenderCalls[0].calls,
                m_resolutionRenderCalls[1].width, m_resolutionRenderCalls[1].height, m_resolutionRenderCalls[1].calls,
                m_resolutionRenderCalls[2].width, m_resolutionRenderCalls[2].height, m_resolutionRenderCalls[2].calls,
                m_resolutionRenderCalls[3].width, m_resolutionRenderCalls[3].height, m_resolutionRenderCalls[3].calls,
                m_resolutionRenderCalls[4].width, m_resolutionRenderCalls[4].height, m_resolutionRenderCalls[4].calls,
                m_resolutionRenderCalls[5].width, m_resolutionRenderCalls[5].height, m_resolutionRenderCalls[5].calls,
                m_resolutionRenderCalls[6].width, m_resolutionRenderCalls[6].height, m_resolutionRenderCalls[6].calls,
                m_resolutionRenderCalls[7].width, m_resolutionRenderCalls[7].height, m_resolutionRenderCalls[7].calls
            );
            snprintf(Resolutions2_c, sizeof Resolutions2_c,
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d\n"
                "%dx%d, %d",
                m_resolutionViewportCalls[0].width, m_resolutionViewportCalls[0].height, m_resolutionViewportCalls[0].calls,
                m_resolutionViewportCalls[1].width, m_resolutionViewportCalls[1].height, m_resolutionViewportCalls[1].calls,
                m_resolutionViewportCalls[2].width, m_resolutionViewportCalls[2].height, m_resolutionViewportCalls[2].calls,
                m_resolutionViewportCalls[3].width, m_resolutionViewportCalls[3].height, m_resolutionViewportCalls[3].calls,
                m_resolutionViewportCalls[4].width, m_resolutionViewportCalls[4].height, m_resolutionViewportCalls[4].calls,
                m_resolutionViewportCalls[5].width, m_resolutionViewportCalls[5].height, m_resolutionViewportCalls[5].calls,
                m_resolutionViewportCalls[6].width, m_resolutionViewportCalls[6].height, m_resolutionViewportCalls[6].calls,
                m_resolutionViewportCalls[7].width, m_resolutionViewportCalls[7].height, m_resolutionViewportCalls[7].calls
            );
        }
        if (FPSavg < 254) {
            gameStart = true;
        }
        else {
            gameStart = false;
            resolutionLookup = false;
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
    
        // Better touch detection - check if coordinates are within reasonable screen bounds
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
        
        // Calculate overlay bounds
        // Cache bounds calculation
        static int cachedBaseX = 0;
        static int cachedBaseY = 0;
        static bool boundsNeedUpdate = true;
        
        // Only recalculate bounds when needed
        if (boundsNeedUpdate) {
            cachedBaseX = 0;
            cachedBaseY = 0;
            boundsNeedUpdate = false;
        }
        
        // Overlay dimensions based on game state
        int overlayWidth, overlayHeight;
        if (actualTotalWidth > 0) {
            // Exact size from the last draw (already scaled + border-expanded).
            overlayWidth  = actualTotalWidth;
            overlayHeight = actualTotalHeight;
        } else {
            overlayWidth = sc(360-20);
            overlayHeight = sc(200);
            // Expand to match draw path when border is on
            if (settings.useBorder) {
                // Fixed reference space width (4.0px at size 16), same fallback as _crSpace floor.
                const float spaceRef = 4.0f * displayScale;
                const int btPx = std::max(1, (int)(spaceRef * (float)settings.borderThickness / 10.0f + 0.5f));
                overlayWidth  += btPx;
                overlayHeight += 2 * btPx;
            }
        }
        
        // Screen boundaries for clamping
        int minX, maxX, minY, maxY;
        if (ult::windowedLayerPixelPerfect) {
            // 1080p: sliding-window-aware bounds — mirrors FPS Counter / Mini.
            const int maxLayerX1080 = (int)tsl::cfg::ScreenWidth - (int)tsl::cfg::LayerWidth;
            const int maxBufX1080   = screenWidth - overlayWidth - (int)framePadding;
            minX = (int)(framePadding * 2 + 2) / 3;
            maxX = std::max(minX, (int)((maxLayerX1080 + std::max(0, maxBufX1080)) / 1.5f));
            if (ult::limitedMemory) {
                // Limited 1080p: Y in 720-logical space.
                const int maxLayerY1080 = (int)tsl::cfg::ScreenHeight - (int)tsl::cfg::LayerHeight;
                const int maxBufY1080   = screenHeight - (int)framePadding - overlayHeight;
                minY = (int)(framePadding / 1.5f + 0.5f);
                maxY = std::max(minY, (int)((maxLayerY1080 + std::max(0, maxBufY1080)) / 1.5f));
            } else {
                // Full 1080p: Y in framebuffer space (0..1080).
                minY = (int)framePadding;
                maxY = screenHeight - overlayHeight - (int)framePadding;
            }
        } else {
            minX = -cachedBaseX + framePadding;
            maxX = screenWidth - overlayWidth - cachedBaseX - framePadding;
            minY = -cachedBaseY + framePadding;
            maxY = screenHeight - overlayHeight - cachedBaseY - framePadding;
        }

        // Y-movement rate normalisation (mirrors FPS Counter / Mini). Full 1080p is 1:1
        // with VI (1 frameOffsetY unit = 1 display px) while every other mode moves
        // 1.5 display px per unit, so scale Y by 1.5 in full 1080p only.
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
            triggerRepositionOnFeedback();
            hasMoved = false;
            initialTouchPos = touchPos;
            initialFrameOffsetX = frameOffsetX;
            initialFrameOffsetY = frameOffsetY;
        } else if (currentTouchDetected && isDragging && !currentPlusHeld) {
            // Continue touch dragging
            const int touchX = touchPos.x;
            const int touchY = touchPos.y;
            const int deltaX = touchX - initialTouchPos.x;
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
                const int newFrameOffsetX = std::max(minX, std::min(maxX, initialFrameOffsetX + deltaX));
                const int newFrameOffsetY = std::max(minY, std::min(maxY, initialFrameOffsetY + deltaY));
                
                frameOffsetX = newFrameOffsetX;
                frameOffsetY = newFrameOffsetY;

                updateLayerPos();

                boundsNeedUpdate = true;
            }
        } else if (!currentTouchDetected && oldTouchDetected && isDragging && !currentPlusHeld) {
            // Touch just released
            if (hasMoved) {
                // Save position when touch drag ends
                auto iniData = ult::getParsedDataFromIniFile(configIniPath);
                iniData["game_resolutions"]["frame_offset_x"] = std::to_string(frameOffsetX);
                iniData["game_resolutions"]["frame_offset_y"] = std::to_string(frameOffsetY);
                ult::saveIniFileData(configIniPath, iniData);
            }
            
            // Reset touch drag state
            isDragging = false;
            hasMoved = false;
            clearOnRelease = true;
            triggerRepositionOffFeedback(true);
        }
        oldTouchDragReady = touchDragReady && currentTouchDetected;
    
        // Handle joystick dragging (MINUS + right joystick OR PLUS + left joystick)
        if (currentPlusHeld && !isDragging) {
            // Start joystick dragging
            isDragging = true;
            triggerRepositionOnFeedback();
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
                const int newFrameOffsetX = std::max(minX, std::min(maxX, frameOffsetX + deltaX));
                const int newFrameOffsetY = std::max(minY, std::min(maxY, frameOffsetY + deltaY));
                
                frameOffsetX = newFrameOffsetX;
                frameOffsetY = newFrameOffsetY;

                updateLayerPos();

                boundsNeedUpdate = true;
            }
        } else if ((!currentPlusHeld && oldPlusHeld) && isDragging) {
            // Button just released - stop joystick dragging
            auto iniData = ult::getParsedDataFromIniFile(configIniPath);
            iniData["game_resolutions"]["frame_offset_x"] = std::to_string(frameOffsetX);
            iniData["game_resolutions"]["frame_offset_y"] = std::to_string(frameOffsetY);
            ult::saveIniFileData(configIniPath, iniData);
            isDragging = false;
            clearOnRelease = true;
            triggerRepositionOffFeedback(true);
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
                lastSelectedItem = "Game Resolutions";
                lastMode = "";
                if (skipMain) {
                    lastMode = "return";
                    tsl::goBack();
                }
                else {
                    tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem 'Game Resolutions'");
                    tsl::Overlay::get()->close();
                }
                return true;
            }
        }
        
        // Return true if we handled the input (during dragging)
        return isDragging;
    }
};