class BatteryOverlay : public tsl::Gui {
private:
    char Battery_c[512];
public:
    BatteryOverlay() {
        disableJumpTo = true;
        mutexInit(&mutex_BatteryChecker);
        StartBatteryThread();
    }
    ~BatteryOverlay() {
        CloseBatteryThread();
        fixForeground = true;
    }

    virtual tsl::elm::Element* createUI() override {

        auto* Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            renderer->drawString("Battery/Charger Stats:", false, 20, 120, 20, 0xFFFF);
            renderer->drawString(Battery_c, false, 20, 155, 18, 0xFFFF);
        });

        tsl::elm::OverlayFrame* rootFrame = new tsl::elm::OverlayFrame("Status Monitor", APP_VERSION, true);
        rootFrame->setContent(Status);

        return rootFrame;
    }

    virtual void update() override {

        ///Battery

        mutexLock(&mutex_BatteryChecker);
        char tempBatTimeEstimate[8] = "--:--";
        if (batTimeEstimate >= 0) {
            snprintf(&tempBatTimeEstimate[0], sizeof(tempBatTimeEstimate), "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        }

        const BatteryChargeInfoFieldsChargerType ChargerConnected = hosversionAtLeast(17,0,0) ? ((BatteryChargeInfoFields17*)&_batteryChargeInfoFields)->ChargerType : _batteryChargeInfoFields.ChargerType;
        const int32_t ChargerVoltageLimit = hosversionAtLeast(17,0,0) ? ((BatteryChargeInfoFields17*)&_batteryChargeInfoFields)->ChargerVoltageLimit : _batteryChargeInfoFields.ChargerVoltageLimit;
        const int32_t ChargerCurrentLimit = hosversionAtLeast(17,0,0) ? ((BatteryChargeInfoFields17*)&_batteryChargeInfoFields)->ChargerCurrentLimit : _batteryChargeInfoFields.ChargerCurrentLimit;

        if (ChargerConnected)
            snprintf(Battery_c, sizeof Battery_c,
                "Battery Actual Capacity: %.0f mAh\n"
                "Battery Designed Capacity: %.0f mAh\n"
                "Battery Temperature: %.1f\u00B0C\n"
                "Battery Raw Charge: %.1f%%\n"
                "Battery Age: %.1f%%\n"
                "Battery Voltage (%ds AVG): %.0f mV\n"
                "Battery Current Flow (%ss AVG): %+.0f mA\n"
                "Battery Power Flow%s: %+.3f W\n"
                "Battery Remaining Time: %s\n"
                "Input Current Limt: %d mA\n"
                "VBUS Current Limit: %d mA\n" 
                "Charge Voltage Limit: %d mV\n"
                "Charge Current Limit: %d mA\n"
                "Charger Type: %u\n"
                "Charger Max Voltage: %u mV\n"
                "Charger Max Current: %u mA",
                actualFullBatCapacity,
                designedFullBatCapacity,
                (float)_batteryChargeInfoFields.BatteryTemperature / 1000,
                (float)_batteryChargeInfoFields.RawBatteryCharge / 1000,
                (float)_batteryChargeInfoFields.BatteryAge / 1000,
                batteryFiltered ? 45 : 5, batVoltageAvg,
                batteryFiltered ? "11.25" : "5", batCurrentAvg,
                batteryFiltered ? "" : " (5s AVG)", PowerConsumption, 
                tempBatTimeEstimate,
                _batteryChargeInfoFields.InputCurrentLimit,
                _batteryChargeInfoFields.VBUSCurrentLimit,
                _batteryChargeInfoFields.ChargeVoltageLimit,
                _batteryChargeInfoFields.ChargeCurrentLimit,
                ChargerConnected,
                ChargerVoltageLimit,
                ChargerCurrentLimit
            );
        else
            snprintf(Battery_c, sizeof Battery_c,
                "Battery Actual Capacity: %.0f mAh\n"
                "Battery Designed Capacity: %.0f mAh\n"
                "Battery Temperature: %.1f\u00B0C\n"
                "Battery Raw Charge: %.1f%%\n"
                "Battery Age: %.1f%%\n"
                "Battery Voltage (%ds AVG): %.0f mV\n"
                "Battery Current Flow (%ss AVG): %+.0f mA\n"
                "Battery Power Flow%s: %+.3f W\n"
                "Battery Remaining Time: %s",
                actualFullBatCapacity,
                designedFullBatCapacity,
                (float)_batteryChargeInfoFields.BatteryTemperature / 1000,
                (float)_batteryChargeInfoFields.RawBatteryCharge / 1000,
                (float)_batteryChargeInfoFields.BatteryAge / 1000,
                batteryFiltered ? 45 : 5, batVoltageAvg,
                batteryFiltered ? "11.25" : "5", batCurrentAvg,
                batteryFiltered ? "" : " (5s AVG)", PowerConsumption, 
                tempBatTimeEstimate
            );
        mutexUnlock(&mutex_BatteryChecker);
        
        static bool skipOnce = true;
    
        if (!skipOnce) {
            static bool runOnce = true;
            if (runOnce) {
                isRendering = true;
                leventClear(&renderingStopEvent);
                runOnce = false;  // Add this to prevent repeated calls
            }
        } else {
            skipOnce = false;
        }
    }
    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            isRendering = false;
            leventSignal(&renderingStopEvent);
            tsl::goBack();
            return true;
        }
        return false;
    }
};
