class MainMenu;

class com_FPS : public tsl::Gui {
private:
    char FPSavg_c[8];
    FpsCounterSettings settings;
    size_t fontsize = 0;
    ApmPerformanceMode performanceMode = ApmPerformanceMode_Invalid;
    bool positionOnce = true;
    bool skipOnce = true;
    bool runOnce = true;

    bool originalUseRightAlignment = ult::useRightAlignment;
public:
    com_FPS() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            fontsize = settings.dockedFontSize;
        }
        
        //alphabackground = 0x0;
        tsl::hlp::requestForeground(false);
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        StartFPSCounterThread();
    }
    ~com_FPS() {
        TeslaFPS = 60;
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

    virtual tsl::elm::Element* createUI() override {
         
        if (positionOnce) {
            if (settings.setPos != 8) {
                tsl::gfx::Renderer::get().setLayerPos(0, 0);
                ult::useRightAlignment = false;
            }

            switch(settings.setPos) {
                case 1:
                case 4:
                case 7:
                    tsl::gfx::Renderer::get().setLayerPos(624, 0);
                    break;
                case 2:
                case 5:
                case 8:
                    const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
                    tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
                    ult::useRightAlignment = true;
                    break;
            }
            positionOnce = false;
        }


        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            //auto dimensions = renderer->drawString(FPSavg_c, false, 0, fontsize, fontsize, tsl::Color(0x0000));
            const auto width = renderer->getTextDimensions(FPSavg_c, false, fontsize).first;
            const size_t rectangleWidth = width;
            const size_t margin = (fontsize / 8);
            int base_x = 0;
            int base_y = 0;
        
            // Vertical alignment (base_y) - add missing cases
            switch (settings.setPos) {
                case 0:  // Left Top
                    base_y = 0;
                    break;
                case 1:  // Center Top
                    base_y = 0;  // Top of the frame
                    break;
                case 2:  // Right Top
                    base_y = 0;  // Top-right corner
                    break;
                case 3:  // Left Center
                    base_y = 360 - ((fontsize + (margin / 2)) / 2);  // Left center
                    break;
                case 4:  // Center Center
                    base_y = 360 - ((fontsize + (margin / 2)) / 2);  // Centered vertically
                    break;
                case 5:  // Right Center
                    base_y = 360 - ((fontsize + (margin / 2)) / 2);  // Centered vertically, right-aligned
                    break;
                case 6:  // Left Bottom
                    base_y = 720 - (fontsize + (margin / 2));  // Left bottom
                    break;
                case 7:  // Center Bottom
                    base_y = 720 - (fontsize + (margin / 2));  // Bottom of the frame
                    break;
                case 8:  // Right Bottom
                    base_y = 720 - (fontsize + (margin / 2));  // Bottom-right corner
                    break;
            }
            
            // Horizontal alignment (base_x) - add missing cases
            switch (settings.setPos) {
                case 0:  // Left Top
                case 3:  // Left Center
                case 6:  // Left Bottom
                    base_x = 0;  // Align to the left
                    break;
                case 1:  // Center Top
                case 4:  // Center Center
                case 7:  // Center Bottom
                    base_x = 224 - ((rectangleWidth + margin) / 2);  // Centered horizontally
                    break;
                case 2:  // Right Top
                case 5:  // Right Center
                case 8:  // Right Bottom
                    base_x = 448 - (rectangleWidth + margin);  // Align to the right
                    break;
            }
            //}
        
            // Draw rectangle and text
            renderer->drawRect(base_x, base_y, rectangleWidth + margin, fontsize + (margin / 2), aWithOpacity(settings.backgroundColor));
            renderer->drawString((FPSavg != 254.0) ? FPSavg_c : "0.0", false, base_x + (margin / 2), base_y + (fontsize - margin), fontsize, settings.textColor);
        });

        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {
        apmGetPerformanceMode(&performanceMode);
        if (performanceMode == ApmPerformanceMode_Normal) {
            fontsize = settings.handheldFontSize;
        }
        else if (performanceMode == ApmPerformanceMode_Boost) {
            fontsize = settings.dockedFontSize;
        }
        snprintf(FPSavg_c, sizeof FPSavg_c, "%2.1f", useOldFPSavg ? FPSavg_old : FPSavg);
        
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
            lastSelectedItem = "FPS Counter";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            return true;
        }
        return false;
    }
};