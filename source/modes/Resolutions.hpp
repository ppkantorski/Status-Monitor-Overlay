class MainMenu;

class ResolutionsOverlay : public tsl::Gui {
private:
    char Resolutions_c[512];
    char Resolutions2_c[512];
    ResolutionSettings settings;
    bool skipOnce = true;
    bool runOnce = true;
    bool positionOnce = true;

    bool originalUseRightAlignment = ult::useRightAlignment;

public:
    ResolutionsOverlay() {
    	tsl::hlp::requestForeground(false);
        disableJumpTo = true;
        GetConfigSettings(&settings);
        switch(settings.setPos) {
            case 1:
            case 4:
            case 7:
                tsl::gfx::Renderer::get().setLayerPos(639, 0);
                break;
            case 2:
            case 5:
            case 8:
                const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
                tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
                break;
        }
        
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        //alphabackground = 0x0;
        FullMode = false;
        TeslaFPS = settings.refreshRate;
        StartFPSCounterThread();
    }
    ~ResolutionsOverlay() {
        EndFPSCounterThread();
        TeslaFPS = 60;
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
        ult::useRightAlignment = originalUseRightAlignment;
        //tsl::hlp::requestForeground(true);
        fixForeground = true;
        //alphabackground = 0xD;
        FullMode = true;
        //if (settings.setPos)
        //    tsl::gfx::Renderer::get().setLayerPos(0, 0);
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    bool gameStart = false;
    uint8_t resolutionLookup = 0;

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
                    tsl::gfx::Renderer::get().setLayerPos(639, 0);
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
            int base_y = 0;
            int base_x = 0;
            //static constexpr int frameWidth = 448; // Assuming the frame width is 448 pixels
        
            // Adjust base_x and base_y based on setPos
            switch(settings.setPos) {
                case 1:
                    base_x = 48;
                    break;
                case 2:
                    base_x = 96;
                    break;
                case 3:
                    base_y = 260;
                    break;
                case 4:
                    base_x = 48;
                    base_y = 260;
                    break;
                case 5:
                    base_x = 96;
                    base_y = 260;
                    break;
                case 6:
                    base_y = 520;
                    break;
                case 7:
                    base_x = 48;
                    base_y = 520;
                    break;
                case 8:
                    base_x = 96;
                    base_y = 520;
                    break;    
            }
        
            // Adjust for right-side alignment
            //if (ult::useRightAlignment) {
            //    base_x = frameWidth - base_x - 360; // Subtract width of the box (360px) from the frame width
            //}
        
            // Drawing when game is running and NVN is used
            if (gameStart && NxFps -> API >= 1) {
                renderer->drawRect(base_x, base_y, 360, 200, aWithOpacity(settings.backgroundColor));
        
                renderer->drawString("Depth:", false, base_x + 20, base_y + 20, 20, (settings.catColor));
                renderer->drawString(Resolutions_c, false, base_x + 20, base_y + 55, 18, (settings.textColor));
                renderer->drawString("Viewport:", false, base_x + 180, base_y + 20, 20, (settings.catColor));
                renderer->drawString(Resolutions2_c, false, base_x + 180, base_y + 55, 18, (settings.textColor));
            }
            // When game is not using NVN or is incompatible
            else {
                switch(settings.setPos) {
                    case 3 ... 5:
                        base_y = 346;
                        break;
                    case 6 ... 8:
                        base_y = 692;
                        break;
                }
        
                renderer->drawRect(base_x, base_y, 360, 28, aWithOpacity(settings.backgroundColor));
                renderer->drawString("Game is not running or it's incompatible.", false, base_x, base_y+20, 18, (0xF00F));
            }
        });
        
        tsl::elm::HeaderOverlayFrame* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {

        if (gameStart && NxFps) {
            if (!resolutionLookup) {
                NxFps -> renderCalls[0].calls = 0xFFFF;
                resolutionLookup = 1;
            }
            else if (resolutionLookup == 1) {
                if ((NxFps -> renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
                else return;
            }
            memcpy(&m_resolutionRenderCalls, &(NxFps -> renderCalls), sizeof(m_resolutionRenderCalls));
            memcpy(&m_resolutionViewportCalls, &(NxFps -> viewportCalls), sizeof(m_resolutionViewportCalls));
            qsort(m_resolutionRenderCalls, 8, sizeof(resolutionCalls), compare);
            qsort(m_resolutionViewportCalls, 8, sizeof(resolutionCalls), compare);
            snprintf(Resolutions_c, sizeof Resolutions_c,
                "1. %dx%d, %d\n"
                "2. %dx%d, %d\n"
                "3. %dx%d, %d\n"
                "4. %dx%d, %d\n"
                "5. %dx%d, %d\n"
                "6. %dx%d, %d\n"
                "7. %dx%d, %d\n"
                "8. %dx%d, %d",
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
            skipOnce = true;
            runOnce = true;
            lastSelectedItem = "Game Resolutions";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            return true;
        }
        return false;
    }
};
