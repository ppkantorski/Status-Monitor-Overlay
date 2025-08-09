#pragma once
#define ALWAYS_INLINE inline __attribute__((always_inline))
#include "SaltyNX.h"

#include "Battery.hpp"
#include "audsnoop.h"
#include "Misc.hpp"
#include "max17050.h"
#include "tmp451.h"
#include "pwm.h"
#include <numeric>
#include <tesla.hpp>
#include <sys/stat.h>

#if defined(__cplusplus)
extern "C"
{
#endif

#include <sysclk/client/ipc.h>

#if defined(__cplusplus)
}
#endif

#define NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD 0x80044715
#define FieldDescriptor uint32_t
#define BASE_SNS_UOHM 5000

static bool fixHiding = false;
static bool fixForeground = false;

//Common
bool isMariko = false;
Thread t0;
Thread t1;
Thread t2;
Thread t3;
Thread t4;
Thread t6;
Thread t7;
Thread t5;
uint64_t systemtickfrequency = 19200000;
//volatile bool threadexit = false;
//volatile bool threadexit2 = false;

LEvent threadexit = {0};
PwmChannelSession g_ICon;
std::string folderpath = "sdmc:/switch/.overlays/";
std::string filename = "";
std::string filepath = "";
std::string keyCombo = "L+DDOWN+RSTICK"; // default Tesla Menu combo

//Misc2
MmuRequest nvdecRequest;
MmuRequest nvencRequest;
MmuRequest nvjpgRequest;

//Checks
Result clkrstCheck = 1;
Result nvCheck = 1;
Result pcvCheck = 1;
Result i2cCheck = 1;
Result pwmCheck = 1;
Result tcCheck = 1;
Result Hinted = 1;
Result pmdmntCheck = 1;
Result psmCheck = 1;
Result audsnoopCheck = 1;
Result nvdecCheck = 1;
Result nvencCheck = 1;
Result nvjpgCheck = 1;
Result nifmCheck = 1;
Result sysclkCheck = 1;
Result pwmDutyCycleCheck = 1;

//Wi-Fi
NifmInternetConnectionType NifmConnectionType = (NifmInternetConnectionType)-1;
NifmInternetConnectionStatus NifmConnectionStatus = (NifmInternetConnectionStatus)-1;
bool Nifm_showpass = false;
Result Nifm_internet_rc = -1;
Result Nifm_profile_rc = -1;
NifmNetworkProfileData_new Nifm_profile = {0};

//Multimedia engines
uint32_t NVDEC_Hz = 0;
uint32_t NVENC_Hz = 0;
uint32_t NVJPG_Hz = 0;

//DSP
uint32_t DSP_Load_u = -1;

//Battery
Service* psmService = 0;
BatteryChargeInfoFields _batteryChargeInfoFields = {0};
float batCurrentAvg = 0;
float batVoltageAvg = 0;
float PowerConsumption = 0;
int16_t batTimeEstimate = -1;
float actualFullBatCapacity = 0;
float designedFullBatCapacity = 0;
bool batteryFiltered = false;
uint8_t batteryTimeLeftRefreshRate = 60;
int32_t BatteryTimeCache[120];

//Temperatures
float SOC_temperatureF = 0;
float PCB_temperatureF = 0;
int32_t skin_temperaturemiliC = 0;

//CPU Usage
uint64_t idletick0 = systemtickfrequency;
uint64_t idletick1 = systemtickfrequency;
uint64_t idletick2 = systemtickfrequency;
uint64_t idletick3 = systemtickfrequency;

//Frequency
uint32_t CPU_Hz = 0;
uint32_t GPU_Hz = 0;
uint32_t RAM_Hz = 0;

//RAM Size
uint64_t RAM_Total_all_u = 0;
uint64_t RAM_Total_application_u = 0;
uint64_t RAM_Total_applet_u = 0;
uint64_t RAM_Total_system_u = 0;
uint64_t RAM_Total_systemunsafe_u = 0;
uint64_t RAM_Used_all_u = 0;
uint64_t RAM_Used_application_u = 0;
uint64_t RAM_Used_applet_u = 0;
uint64_t RAM_Used_system_u = 0;
uint64_t RAM_Used_systemunsafe_u = 0;

//Fan
double Rotation_Duty = 0;

//GPU Usage
FieldDescriptor fd = 0;
uint32_t GPU_Load_u = 0;
bool GPULoadPerFrame = true;

//NX-FPS

struct resolutionCalls {
    uint16_t width;
    uint16_t height;
    uint16_t calls;
};

struct NxFpsSharedBlock {
    uint32_t MAGIC;
    uint8_t FPS;
    float FPSavg;
    bool pluginActive;
    uint8_t FPSlocked;
    uint8_t FPSmode;
    uint8_t ZeroSync;
    uint8_t patchApplied;
    uint8_t API;
    uint32_t FPSticks[10];
    uint8_t Buffers;
    uint8_t SetBuffers;
    uint8_t ActiveBuffers;
    uint8_t SetActiveBuffers;
    uint8_t displaySync;
    resolutionCalls renderCalls[8];
    resolutionCalls viewportCalls[8];
    bool forceOriginalRefreshRate;
} NX_PACKED;

NxFpsSharedBlock* NxFps = 0;
bool GameRunning = false;
bool check = true;
bool SaltySD = false;
uintptr_t FPSaddress = 0;
uintptr_t FPSavgaddress = 0;
uint64_t PID = 0;
uint32_t FPS = 0xFE;
float FPSavg = 254;
float FPSmin = 254; 
float FPSmax = 0; 
SharedMemory _sharedmemory = {};
bool SharedMemoryUsed = false;
Handle remoteSharedMemory = 1;

//Read real freqs from sys-clk sysmodule
uint32_t realCPU_Hz = 0;
uint32_t realGPU_Hz = 0;
uint32_t realRAM_Hz = 0;
uint32_t ramLoad[SysClkRamLoad_EnumMax];
uint32_t realCPU_mV = 0; 
uint32_t realGPU_mV = 0; 
uint32_t realRAM_mV = 0; 
uint32_t realSOC_mV = 0; 
uint8_t refreshRate = 0;

int compare (const void* elem1, const void* elem2) {
    if ((((resolutionCalls*)(elem1)) -> calls) > (((resolutionCalls*)(elem2)) -> calls)) return -1;
    else return 1;
}

void LoadSharedMemoryAndRefreshRate() {
    if (SaltySD_Connect())
        return;

    SaltySD_GetSharedMemoryHandle(&remoteSharedMemory);
    SaltySD_GetDisplayRefreshRate(&refreshRate);
    SaltySD_Term();

    shmemLoadRemote(&_sharedmemory, remoteSharedMemory, 0x1000, Perm_Rw);
    if (!shmemMap(&_sharedmemory))
        SharedMemoryUsed = true;
    else FPS = 1234;
}

void LoadSharedMemory() {
    if (SaltySD_Connect())
        return;

    SaltySD_GetSharedMemoryHandle(&remoteSharedMemory);
    SaltySD_Term();

    shmemLoadRemote(&_sharedmemory, remoteSharedMemory, 0x1000, Perm_Rw);
    if (!shmemMap(&_sharedmemory))
        SharedMemoryUsed = true;
    else FPS = 1234;
}

void searchSharedMemoryBlock(uintptr_t base) {
    ptrdiff_t search_offset = 0;
    while(search_offset < 0x1000) {
        NxFps = (NxFpsSharedBlock*)(base + search_offset);
        if (NxFps -> MAGIC == 0x465053) {
            return;
        }
        else search_offset += 4;
    }
    NxFps = 0;
    return;
}

//Check if SaltyNX is working
bool CheckPort() {
    Handle saltysd;
    
    // Try immediate connection first (most common case)
    if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
        svcCloseHandle(saltysd);
        return true;
    }
    
    // Reduce retry attempts and initial delay
    static constexpr u64 initial_delay = 10'000;    // 0.05ms (was 0.1ms)
    //static constexpr u64 max_delay = 5'000'000;     // 5ms (was 10ms)
    static constexpr int max_attempts = 50;         // Reduced from 20
    
    u64 delay = initial_delay;
    for (int i = 0; i < max_attempts; i++) {
        svcSleepThread(delay);
        if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
            svcCloseHandle(saltysd);
            return true;
        }
        //delay = std::min(delay * 2, max_delay);
    }
    return false;
}

void CheckIfGameRunning(void*) {
    do {
        if (!check && R_FAILED(pmdmntGetApplicationProcessId(&PID))) {
            GameRunning = false;
            if (SharedMemoryUsed) {
                (NxFps -> MAGIC) = 0;
                (NxFps -> pluginActive) = false;
                (NxFps -> FPS) = 0;
                (NxFps -> FPSavg) = 0.0;
                FPS = 254;
                FPSavg = 254.0;
            }
            check = true;
        }
        else if (!GameRunning && SharedMemoryUsed) {
                uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
                searchSharedMemoryBlock(base);
                if (NxFps) {
                    (NxFps -> pluginActive) = false;
                    svcSleepThread(100'000'000);
                    if ((NxFps -> pluginActive)) {
                        GameRunning = true;
                        check = false;
                    }
                }
        }
    } while (!leventWait(&threadexit, 1'000'000'000));
}


Mutex mutex_BatteryChecker = {0};
void BatteryChecker(void*) {
    if (R_FAILED(psmCheck) || R_FAILED(i2cCheck)){
        return;
    }
    uint16_t data = 0;
    float tempV = 0.0;
    float tempA = 0.0;
    const size_t ArraySize = batteryFiltered ? 1 : 10;
    //if (batteryFiltered) {
    //    ArraySize = 1;
    //}
    float* readingsAmp = new float[ArraySize];
    float* readingsVolt = new float[ArraySize];

    Max17050ReadReg(MAX17050_AvgCurrent, &data);
    tempA = (1.5625 / (max17050SenseResistor * max17050CGain)) * (s16)data;
    for (size_t i = 0; i < ArraySize; i++) {
        readingsAmp[i] = tempA;
    }
    Max17050ReadReg(MAX17050_AvgVCELL, &data);
    tempV = 0.625 * (data >> 3);
    for (size_t i = 0; i < ArraySize; i++) {
        readingsVolt[i] = tempV;
    }
    if (!actualFullBatCapacity) {
        Max17050ReadReg(MAX17050_FullCAP, &data);
        actualFullBatCapacity = data * (BASE_SNS_UOHM / MAX17050_BOARD_SNS_RESISTOR_UOHM) / MAX17050_BOARD_CGAIN;
    }
    if (!designedFullBatCapacity) {
        Max17050ReadReg(MAX17050_DesignCap, &data);
        designedFullBatCapacity = data * (BASE_SNS_UOHM / MAX17050_BOARD_SNS_RESISTOR_UOHM) / MAX17050_BOARD_CGAIN;
    }
    if (readingsAmp[0] >= 0) {
        batTimeEstimate = -1;
    }
    else {
        Max17050ReadReg(MAX17050_TTE, &data);
        const float batteryTimeEstimateInMinutes = (5.625 * data) / 60;
        if (batteryTimeEstimateInMinutes > (99.0*60.0)+59.0) {
            batTimeEstimate = (99*60)+59;
        }
        else batTimeEstimate = (int16_t)batteryTimeEstimateInMinutes;
    }

    size_t counter = 0;
    uint64_t tick_TTE = svcGetSystemTick();
    uint64_t nanoseconds = 1000;
    do {
        mutexLock(&mutex_BatteryChecker);
        uint64_t startTick = svcGetSystemTick();

        psmGetBatteryChargeInfoFields(psmService, &_batteryChargeInfoFields);

        // Calculation is based on Hekate's max17050.c
        // Source: https://github.com/CTCaer/hekate/blob/master/bdk/power/max17050.c

        if (!batteryFiltered) {
            Max17050ReadReg(MAX17050_Current, &data);
            tempA = (1.5625 / (max17050SenseResistor * max17050CGain)) * (s16)data;
            Max17050ReadReg(MAX17050_VCELL, &data);
            tempV = 0.625 * (data >> 3);
        } else {
            Max17050ReadReg(MAX17050_AvgCurrent, &data);
            tempA = (1.5625 / (max17050SenseResistor * max17050CGain)) * (s16)data;
            Max17050ReadReg(MAX17050_AvgVCELL, &data);
            tempV = 0.625 * (data >> 3);
        }

        if (tempA && tempV) {
            readingsAmp[counter % ArraySize] = tempA;
            readingsVolt[counter % ArraySize] = tempV;
            counter++;
        }

        float batCurrent = 0.0;
        float batVoltage = 0.0;
        float batPowerAvg = 0.0;
        for (size_t x = 0; x < ArraySize; x++) {
            batCurrent += readingsAmp[x];
            batVoltage += readingsVolt[x];
            batPowerAvg += (readingsAmp[x] * readingsVolt[x]) / 1'000;
        }
        batCurrent /= ArraySize;
        batVoltage /= ArraySize;
        batCurrentAvg = batCurrent;
        batVoltageAvg = batVoltage;
        batPowerAvg /= ArraySize * 1000;
        PowerConsumption = batPowerAvg;

        if (batCurrentAvg >= 0) {
            batTimeEstimate = -1;
        } 
        else {
            static float batteryTimeEstimateInMinutes = 0;
            Max17050ReadReg(MAX17050_TTE, &data);
            batteryTimeEstimateInMinutes = (5.625 * data) / 60;
            if (batteryTimeEstimateInMinutes > (99.0*60.0)+59.0) {
                batteryTimeEstimateInMinutes = (99.0*60.0)+59.0;
            }
            static int itr = 0;
            const int cacheElements = (sizeof(BatteryTimeCache) / sizeof(BatteryTimeCache[0]));
            BatteryTimeCache[itr++ % cacheElements] = (int32_t)batteryTimeEstimateInMinutes;
            uint64_t new_tick_TTE = svcGetSystemTick();
            if (armTicksToNs(new_tick_TTE - tick_TTE) / 1'000'000'000 >= batteryTimeLeftRefreshRate) {
                size_t to_divide = itr < cacheElements ? itr : cacheElements;
                batTimeEstimate = (int16_t)(std::accumulate(&BatteryTimeCache[0], &BatteryTimeCache[to_divide], 0) / to_divide);
                tick_TTE = new_tick_TTE;
            }
        }

        mutexUnlock(&mutex_BatteryChecker);
        nanoseconds = armTicksToNs(svcGetSystemTick() - startTick);
        if (nanoseconds < 1'000'000'000 / 2) {
            nanoseconds = (1'000'000'000 / 2) - nanoseconds;
        } else {
            nanoseconds = 1000;
        }
    } while(!leventWait(&threadexit, nanoseconds));

    batTimeEstimate = -1;
    _batteryChargeInfoFields = {0};
    memset(BatteryTimeCache, 0, sizeof(BatteryTimeCache));
    delete[] readingsAmp;
    delete[] readingsVolt;
}

void StartBatteryThread() {
    //if (!skip) {
    //    threadWaitForExit(&t7);
    //    threadClose(&t7);
    //    leventClear(&threadexit);
    //}
    leventClear(&threadexit);
    threadCreate(&t7, BatteryChecker, NULL, NULL, 0x4000, 0x3F, 3);
    threadStart(&t7);
}

void CloseBatteryThread() {
    leventSignal(&threadexit);
    threadWaitForExit(&t7);
    threadClose(&t7);
}

Mutex mutex_Misc = {0};

void gpuLoadThread(void*) {
    if (!GPULoadPerFrame && R_SUCCEEDED(nvCheck)) do {
        u8 average = 5;
        u32 temp = 0;
        nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &temp);
        GPU_Load_u = ((GPU_Load_u * (average-1)) + temp) / average;
    } while(!leventWait(&threadexit, 16'666'000));
}

//Stuff that doesn't need multithreading
//Stuff that doesn't need multithreading
void Misc(void*) {
    uint64_t timeout_ns = TeslaFPS < 10 ? (1'000'000'000 / TeslaFPS) : 100'000'000;
    do {
        mutexLock(&mutex_Misc);
        // CPU, GPU and RAM Frequency
        if (R_SUCCEEDED(clkrstCheck)) {
            ClkrstSession clkSession;
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_CpuBus, 3))) {
                clkrstGetClockRate(&clkSession, &CPU_Hz);
                clkrstCloseSession(&clkSession);
            }
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_GPU, 3))) {
                clkrstGetClockRate(&clkSession, &GPU_Hz);
                clkrstCloseSession(&clkSession);
            }
            if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_EMC, 3))) {
                clkrstGetClockRate(&clkSession, &RAM_Hz);
                clkrstCloseSession(&clkSession);
            }
        }
        else if (R_SUCCEEDED(pcvCheck)) {
            pcvGetClockRate(PcvModule_CpuBus, &CPU_Hz);
            pcvGetClockRate(PcvModule_GPU, &GPU_Hz);
            pcvGetClockRate(PcvModule_EMC, &RAM_Hz);
        }
        if (R_SUCCEEDED(sysclkCheck)) {
            SysClkContext sysclkCTX;
            if (R_SUCCEEDED(sysclkIpcGetCurrentContext(&sysclkCTX))) {
                realCPU_Hz = sysclkCTX.realFreqs[SysClkModule_CPU];
                realGPU_Hz = sysclkCTX.realFreqs[SysClkModule_GPU];
                realRAM_Hz = sysclkCTX.realFreqs[SysClkModule_MEM];
                ramLoad[SysClkRamLoad_All] = sysclkCTX.ramLoad[SysClkRamLoad_All];
                ramLoad[SysClkRamLoad_Cpu] = sysclkCTX.ramLoad[SysClkRamLoad_Cpu];
                realCPU_mV = sysclkCTX.realVolts[0]; 
                realGPU_mV = sysclkCTX.realVolts[1]; 
                realRAM_mV = sysclkCTX.realVolts[2]; 
                realSOC_mV = sysclkCTX.realVolts[3]; 
            }
        }
        
        //Temperatures
        if (R_SUCCEEDED(i2cCheck)) {
            Tmp451GetSocTemp(&SOC_temperatureF);
            Tmp451GetPcbTemp(&PCB_temperatureF);
        }
        if (R_SUCCEEDED(tcCheck)) tcGetSkinTemperatureMilliC(&skin_temperaturemiliC);
        
        //RAM Memory Used
        if (R_SUCCEEDED(Hinted)) {
            svcGetSystemInfo(&RAM_Total_application_u, 0, INVALID_HANDLE, 0);
            svcGetSystemInfo(&RAM_Total_applet_u, 0, INVALID_HANDLE, 1);
            svcGetSystemInfo(&RAM_Total_system_u, 0, INVALID_HANDLE, 2);
            svcGetSystemInfo(&RAM_Total_systemunsafe_u, 0, INVALID_HANDLE, 3);
            svcGetSystemInfo(&RAM_Used_application_u, 1, INVALID_HANDLE, 0);
            svcGetSystemInfo(&RAM_Used_applet_u, 1, INVALID_HANDLE, 1);
            svcGetSystemInfo(&RAM_Used_system_u, 1, INVALID_HANDLE, 2);
            svcGetSystemInfo(&RAM_Used_systemunsafe_u, 1, INVALID_HANDLE, 3);
        }
        
        //Fan
        if (R_SUCCEEDED(pwmCheck)) {
            double temp = 0;
            if (R_SUCCEEDED(pwmChannelSessionGetDutyCycle(&g_ICon, &temp))) {
                temp *= 10;
                temp = trunc(temp);
                temp /= 10;
                Rotation_Duty = 100.0 - temp;
                if (Rotation_Duty <= 0) Rotation_Duty = 0.0000001;
            }
        }
        
        //GPU Load
        if (R_SUCCEEDED(nvCheck) && GPULoadPerFrame) nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &GPU_Load_u);
        
        //FPS
        if (GameRunning) {
            if (SharedMemoryUsed) {
                FPS = (NxFps -> FPS);
                FPSavg = 19'200'000.f / (std::accumulate<uint32_t*, float>(&(NxFps -> FPSticks[0]), &(NxFps -> FPSticks[10]), 0) / 10);
                if (FPSavg > FPSmax)    FPSmax = FPSavg; 
                if (FPSavg < FPSmin)    FPSmin = FPSavg; 
            }
        }
        else {
            FPSavg = 254;
            FPSmin = 254; 
            FPSmax = 0; 
        }
        
        // Interval
        mutexUnlock(&mutex_Misc);
        timeout_ns = (TeslaFPS < 10) ? (1'000'000'000 / TeslaFPS) : 100'000'000;
    } while (!leventWait(&threadexit, timeout_ns));
}

void Misc2(void*) {
    do {
        //DSP
        if (R_SUCCEEDED(audsnoopCheck)) audsnoopGetDspUsage(&DSP_Load_u);

        //Multimedia clock rates
        if (R_SUCCEEDED(nvdecCheck)) mmuRequestGet(&nvdecRequest, &NVDEC_Hz);
        if (R_SUCCEEDED(nvencCheck)) mmuRequestGet(&nvencRequest, &NVENC_Hz);
        if (R_SUCCEEDED(nvjpgCheck)) mmuRequestGet(&nvjpgRequest, &NVJPG_Hz);

        if (R_SUCCEEDED(nifmCheck)) {
            u32 dummy = 0;
            Nifm_internet_rc = nifmGetInternetConnectionStatus(&NifmConnectionType, &dummy, &NifmConnectionStatus);
            if (!Nifm_internet_rc && (NifmConnectionType == NifmInternetConnectionType_WiFi))
                Nifm_profile_rc = nifmGetCurrentNetworkProfile((NifmNetworkProfileData*)&Nifm_profile);
        }
    } while (!leventWait(&threadexit, 100'000'000));
}

void Misc3(void*) {
    do {
        mutexLock(&mutex_Misc);
        if (R_SUCCEEDED(sysclkCheck)) {
            SysClkContext sysclkCTX;
            if (R_SUCCEEDED(sysclkIpcGetCurrentContext(&sysclkCTX))) {
                ramLoad[SysClkRamLoad_All] = sysclkCTX.ramLoad[SysClkRamLoad_All];
                ramLoad[SysClkRamLoad_Cpu] = sysclkCTX.ramLoad[SysClkRamLoad_Cpu];
                // Add voltage readings to Misc3 as well
                realCPU_mV = sysclkCTX.realVolts[0]; 
                realGPU_mV = sysclkCTX.realVolts[1]; 
                realRAM_mV = sysclkCTX.realVolts[2]; 
                realSOC_mV = sysclkCTX.realVolts[3];
            }
        }
        //Temperatures
        if (R_SUCCEEDED(i2cCheck)) {
            Tmp451GetSocTemp(&SOC_temperatureF);
            Tmp451GetPcbTemp(&PCB_temperatureF);
        }
        if (R_SUCCEEDED(tcCheck)) tcGetSkinTemperatureMilliC(&skin_temperaturemiliC);
        //Fan
        if (R_SUCCEEDED(pwmCheck)) {
            double temp = 0;
            if (R_SUCCEEDED(pwmChannelSessionGetDutyCycle(&g_ICon, &temp))) {
                temp *= 10;
                temp = trunc(temp);
                temp /= 10;
                Rotation_Duty = 100.0 - temp;
            }
        }
        //GPU Load
        if (R_SUCCEEDED(nvCheck)) nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &GPU_Load_u);
        // Interval
        mutexUnlock(&mutex_Misc);
    } while (!leventWait(&threadexit, 1'000'000'000)); // 1 second timeout
}

//Check each core for idled ticks in intervals, they cannot read info about other core than they are assigned
//In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
//This is because making each loop also takes time, which is not considered because this will take also additional time

void CheckCore(void* arg) {
    int coreIndex = *((int*)arg);
    uint64_t* output = nullptr;
    switch (coreIndex) {
        case 0: output = &idletick0; break;
        case 1: output = &idletick1; break;
        case 2: output = &idletick2; break;
        case 3: output = &idletick3; break;
        default: return;
    }
    uint64_t prevIdleTick = 0, currIdleTick = 0;
    svcGetInfo(&prevIdleTick, InfoType_IdleTickCount, INVALID_HANDLE, coreIndex);
    static u32 lastFPS = 0;
    static u64 cachedIntervalNs = 0;
    static u64 cachedTargetTicks = 0;
    uint64_t delta, tickCap;
    
    do {
        if (__builtin_expect(TeslaFPS != lastFPS, 0)) {
            cachedIntervalNs = 1'000'000'000 / TeslaFPS;
            cachedTargetTicks = armNsToTicks(cachedIntervalNs);
            lastFPS = TeslaFPS;
        }
        
        // Wait for the calculated interval or thread exit event
        if (leventWait(&threadexit, cachedIntervalNs)) break;
        
        // Get idle ticks and calculate delta
        svcGetInfo(&currIdleTick, InfoType_IdleTickCount, INVALID_HANDLE, coreIndex);
        delta = currIdleTick - prevIdleTick;
        tickCap = cachedTargetTicks; // Cap to the expected sleep ticks
        *output = (delta > tickCap) ? tickCap : delta;
        prevIdleTick = currIdleTick;
    } while (true);
}

static int coreIds[] = {0, 1, 2, 3};

//Start reading all stats
void StartThreads() {
    // Clear the thread exit event for new threads
    leventClear(&threadexit);

    // Wait for existing threads to exit
    //threadWaitForExit(&t0);
    //threadWaitForExit(&t1);
    //threadWaitForExit(&t2);
    //threadWaitForExit(&t3);
    //threadWaitForExit(&t4);
    //threadWaitForExit(&t5);
    //threadWaitForExit(&t6);
    //threadWaitForExit(&t7);
    
    // Close and recreate threads
    //threadClose(&t0);
    threadCreate(&t0, CheckCore, &coreIds[0], NULL, 0x1000, 0x10, 0);
    threadStart(&t0);
    
    //threadClose(&t1);
    threadCreate(&t1, CheckCore, &coreIds[1], NULL, 0x1000, 0x10, 1);
    threadStart(&t1);
    
    //threadClose(&t2);
    threadCreate(&t2, CheckCore, &coreIds[2], NULL, 0x1000, 0x10, 2);
    threadStart(&t2);
    
    //threadClose(&t3);
    threadCreate(&t3, CheckCore, &coreIds[3], NULL, 0x1000, 0x10, 3);
    threadStart(&t3);
    
    //threadClose(&t4);
    threadCreate(&t4, Misc, NULL, NULL, 0x1000, 0x3F, -2);
    threadStart(&t4);
    
    //threadClose(&t5);
    threadCreate(&t5, gpuLoadThread, NULL, NULL, 0x1000, 0x3F, -2);
    threadStart(&t5);
    
    //threadClose(&t6);
    if (SaltySD) {
        //Assign NX-FPS to default core
        threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
        //Start NX-FPS detection
        threadStart(&t6);
    }
    
    //threadClose(&t7);
    threadCreate(&t7, BatteryChecker, NULL, NULL, 0x4000, 0x3F, 3);
    threadStart(&t7);
}

//End reading all stats
void CloseThreads() {
    leventSignal(&threadexit);
    //if (wait) {
    threadWaitForExit(&t0);
    threadWaitForExit(&t1);
    threadWaitForExit(&t2);
    threadWaitForExit(&t3);
    threadWaitForExit(&t4);
    threadWaitForExit(&t5);
    threadWaitForExit(&t6);
    threadWaitForExit(&t7);
    threadClose(&t0);
    threadClose(&t1);
    threadClose(&t2);
    threadClose(&t3);
    threadClose(&t4);
    threadClose(&t5);
    threadClose(&t6);
    threadClose(&t7);
    //}
}

//Separate functions dedicated to "FPS Counter" mode
void FPSCounter(void*) {
    u64 sleepNs = 1'000'000'000ULL / TeslaFPS;
    do {
        if (GameRunning) {
            if (SharedMemoryUsed) {
                FPS = (NxFps -> FPS);
                FPSavg = 19'200'000.f / (std::accumulate<uint32_t*, float>(&(NxFps -> FPSticks[0]), &(NxFps -> FPSticks[10]), 0) / 10);
            }
        }
        else FPSavg = 254;
        
        // Wait for interval based on TeslaFPS or thread exit event
        sleepNs = 1'000'000'000ULL / TeslaFPS;
    } while (!leventWait(&threadexit, sleepNs));
}

void StartFPSCounterThread() {
    //threadWaitForExit(&t0);
    //threadWaitForExit(&t6);
    leventClear(&threadexit);

    //threadClose(&t6);
    threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
    threadStart(&t6);

    //threadClose(&t0);
    threadCreate(&t0, FPSCounter, NULL, NULL, 0x1000, 0x3F, 3);
    threadStart(&t0);
}

void EndFPSCounterThread() {
    leventSignal(&threadexit);
    threadWaitForExit(&t6);
    threadClose(&t6);
    threadWaitForExit(&t0);
    threadClose(&t0);
}

void StartInfoThread() {
    // Wait for existing threads to exit
    //threadWaitForExit(&t1);
    //threadWaitForExit(&t2);
    //threadWaitForExit(&t3);
    //threadWaitForExit(&t4);
    //threadWaitForExit(&t7);
    
    // Clear the thread exit event for new threads
    leventClear(&threadexit);
    
    // Close and recreate threads
    //threadClose(&t1);
    threadCreate(&t1, CheckCore, &coreIds[0], NULL, 0x1000, 0x10, 0);
    threadStart(&t1);
    
    //threadClose(&t2);
    threadCreate(&t2, CheckCore, &coreIds[1], NULL, 0x1000, 0x10, 1);
    threadStart(&t2);
    
    //threadClose(&t3);
    threadCreate(&t3, CheckCore, &coreIds[2], NULL, 0x1000, 0x10, 2);
    threadStart(&t3);
    
    //threadClose(&t4);
    threadCreate(&t4, CheckCore, &coreIds[3], NULL, 0x1000, 0x10, 3);
    threadStart(&t4);
    
    //threadClose(&t7);
    threadCreate(&t7, Misc3, NULL, NULL, 0x1000, 0x3F, -2);
    threadStart(&t7);
}

void EndInfoThread() {
    // Signal the thread exit event
    leventSignal(&threadexit);
    
    // Wait for all threads to exit
    threadWaitForExit(&t1);
    threadWaitForExit(&t2);
    threadWaitForExit(&t3);
    threadWaitForExit(&t4);
    threadWaitForExit(&t7);
    
    // Close thread handles
    threadClose(&t1);
    threadClose(&t2);
    threadClose(&t3);
    threadClose(&t4);
    threadClose(&t7);
}

// String formatting functions
void removeSpaces(std::string& str) {
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
}

void convertToUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void convertToLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

std::map<std::string, std::string> replaces{
    {"A", "\uE0E0"},
    {"B", "\uE0E1"},
    {"X", "\uE0E2"},
    {"Y", "\uE0E3"},
    {"L", "\uE0E4"},
    {"R", "\uE0E5"},
    {"ZL", "\uE0E6"},
    {"ZR", "\uE0E7"},
    {"SL", "\uE0E8"},
    {"SR", "\uE0E9"},
    {"DUP", "\uE0EB"},
    {"DDOWN", "\uE0EC"},
    {"DLEFT", "\uE0ED"},
    {"DRIGHT", "\uE0EE"},
    {"PLUS", "\uE0EF"},
    {"MINUS", "\uE0F0"},
    {"LSTICK", "\uE104"},
    {"RSTICK", "\uE105"},
    {"RS", "\uE105"},
    {"LS", "\uE104"}
};

void formatButtonCombination(std::string& line) {
    // Remove all spaces from the line
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

    // Replace '+' with ' + '
    size_t pos = 0;
    size_t max_pluses = 3;
    while ((pos = line.find('+', pos)) != std::string::npos) {
        if (!max_pluses) {
            line = line.substr(0, pos);
            return;
        }
        if (pos > 0 && pos < line.size() - 1) {
            if (std::isalnum(line[pos - 1]) && std::isalnum(line[pos + 1])) {
                line.replace(pos, 1, " + ");
                pos += 3;
            }
        }
        ++pos;
        max_pluses--;
    }
    pos = 0;
    size_t old_pos = 0;
    std::string button;
    while ((pos = line.find(" + ", pos)) != std::string::npos) {

        button = line.substr(old_pos, pos - old_pos);
        if (replaces.find(button) != replaces.end()) {
            line.replace(old_pos, button.length(), replaces[button]);
            pos = 0;
            old_pos = 0;
        }
        else pos += 3;
        old_pos = pos;
    }
    button = line.substr(old_pos);
    if (replaces.find(button) != replaces.end()) {
        line.replace(old_pos, button.length(), replaces[button]);
    }    
}

uint64_t comboBitmask = 0;

constexpr uint64_t MapButtons(const std::string& buttonCombo) {
    static std::map<std::string, uint64_t> buttonMap = {
        {"A", HidNpadButton_A},
        {"B", HidNpadButton_B},
        {"X", HidNpadButton_X},
        {"Y", HidNpadButton_Y},
        {"L", HidNpadButton_L},
        {"R", HidNpadButton_R},
        {"ZL", HidNpadButton_ZL},
        {"ZR", HidNpadButton_ZR},
        {"PLUS", HidNpadButton_Plus},
        {"MINUS", HidNpadButton_Minus},
        {"DUP", HidNpadButton_Up},
        {"DDOWN", HidNpadButton_Down},
        {"DLEFT", HidNpadButton_Left},
        {"DRIGHT", HidNpadButton_Right},
        {"SL", HidNpadButton_AnySL},
        {"SR", HidNpadButton_AnySR},
        {"LSTICK", HidNpadButton_StickL},
        {"RSTICK", HidNpadButton_StickR},
        {"LS", HidNpadButton_StickL},
        {"RS", HidNpadButton_StickR},
        {"UP", HidNpadButton_AnyUp},
        {"DOWN", HidNpadButton_AnyDown},
        {"LEFT", HidNpadButton_AnyLeft},
        {"RIGHT", HidNpadButton_AnyRight}
    };

    
    std::string comboCopy = buttonCombo;  // Make a copy of buttonCombo

    std::string delimiter = "+";
    size_t pos = 0;
    std::string button;
    size_t max_delimiters = 4;
    while ((pos = comboCopy.find(delimiter)) != std::string::npos) {
        button = comboCopy.substr(0, pos);
        if (buttonMap.find(button) != buttonMap.end()) {
            comboBitmask |= buttonMap[button];
        }
        comboCopy.erase(0, pos + delimiter.length());
        if (!--max_delimiters) {
            return comboBitmask;
        }
    }
    if (buttonMap.find(comboCopy) != buttonMap.end()) {
        comboBitmask |= buttonMap[comboCopy];
    }
    return comboBitmask;
}

ALWAYS_INLINE bool isKeyComboPressed(uint64_t keysHeld, uint64_t keysDown) {
    // Check if any of the combo buttons are pressed down this frame
    // while the rest of the combo buttons are being held
    
    const uint64_t comboButtonsDown = keysDown & comboBitmask;
    const uint64_t comboButtonsHeld = keysHeld & comboBitmask;
    
    // If any combo buttons are pressed down this frame
    if (comboButtonsDown != 0) {
        // Check if the remaining combo buttons are being held
        // (the full combo should be active when combining held + down)
        const uint64_t totalComboActive = comboButtonsHeld | comboButtonsDown;
        
        if (totalComboActive == comboBitmask) {
            fixHiding = true; // for fixing hiding when returning
            return true;
        }
    }
    
    return false;
}

inline int safeFanDuty(int raw) {
    if (raw < 0)   return 0;
    if (raw > 100) return 100;
    return raw;
}

// Helper function to check if comboBitmask is satisfied with at least one key in keysDown and the rest in keysHeld
bool isKeyComboPressed2(uint64_t keysDown, uint64_t keysHeld) {
    uint64_t requiredKeys = comboBitmask;
    bool hasKeyDown = false; // Tracks if at least one key is in keysDown

    uint64_t keyBit;
    // Iterate over each bit in the comboBitmask
    while (requiredKeys) {
        keyBit = requiredKeys & ~(requiredKeys - 1); // Get the lowest bit set in requiredKeys

        // Check if the key is in keysDown or keysHeld
        if (keysDown & keyBit) {
            hasKeyDown = true; // Found at least one key in keysDown
        } else if (!(keysHeld & keyBit)) {
            return false; // If the key is neither in keysDown nor keysHeld, the combo is incomplete
        }

        // Remove the lowest bit and continue to check other keys
        requiredKeys &= ~keyBit;
    }

    // Ensure that at least one key was in keysDown and the rest were in keysHeld
    return hasKeyDown;
}


// Custom utility function for parsing an ini file
void ParseIniFile() {
    // Use const string_view for paths to avoid string copying
    constexpr const char* directoryPath = "sdmc:/config/status-monitor/";
    //constexpr const char* ultrahandDirectoryPath = "sdmc:/config/ultrahand/";
    //constexpr const char* teslaDirectoryPath = "sdmc:/config/tesla/";
    constexpr const char* configIniPath = "sdmc:/config/status-monitor/config.ini";
    constexpr const char* ultrahandConfigIniPath = "sdmc:/config/ultrahand/config.ini";
    constexpr const char* teslaConfigIniPath = "sdmc:/config/tesla/config.ini";
    
    tsl::hlp::ini::IniData parsedData;
    
    // Check and create directory if needed
    struct stat st;
    if (stat(directoryPath, &st) != 0) {
        mkdir(directoryPath, 0777);
    }

    //bool readExternalCombo = false;
    
    // Try to open main config file
    FILE* configFile = fopen(configIniPath, "r");
    if (configFile) {
        // Get file size more efficiently
        fseek(configFile, 0, SEEK_END);
        const long fileSize = ftell(configFile);
        fseek(configFile, 0, SEEK_SET); // Use SEEK_SET instead of rewind for consistency
        
        // Reserve string capacity and read directly
        std::string fileData;
        fileData.resize(fileSize);
        fread(fileData.data(), 1, fileSize, configFile);
        fclose(configFile);

        parsedData = ult::parseIni(fileData);
        
        // Cache the section lookup to avoid repeated map lookups
        auto statusMonitorIt = parsedData.find("status-monitor");
        if (statusMonitorIt != parsedData.end()) {
            const auto& section = statusMonitorIt->second;
            
            // Process key_combo
            //auto keyComboIt = section.find("key_combo");
            //if (keyComboIt != section.end()) {
            //    keyCombo = keyComboIt->second;
            //    removeSpaces(keyCombo);
            //    convertToUpper(keyCombo);
            //} else {
            //    readExternalCombo = true;
            //}
            
            // Process battery_avg_iir_filter
            auto batteryFilterIt = section.find("battery_avg_iir_filter");
            if (batteryFilterIt != section.end()) {
                std::string key = batteryFilterIt->second;
                convertToUpper(key);
                batteryFiltered = (key == "TRUE");
            }
            
            // Process font_cache
            //auto fontCacheIt = section.find("font_cache");
            //if (fontCacheIt != section.end()) {
            //    std::string key = fontCacheIt->second;
            //    convertToUpper(key);
            //    fontCache = (key == "TRUE");
            //}
            //
            // Process battery_time_left_refreshrate
            auto refreshRateIt = section.find("battery_time_left_refreshrate");
            if (refreshRateIt != section.end()) {
                long rate = std::clamp(atol(refreshRateIt->second.c_str()), 1L, 60L);
                batteryTimeLeftRefreshRate = rate;
            }
            
            // Process average_gpu_load
            auto gpuLoadIt = section.find("average_gpu_load");
            if (gpuLoadIt != section.end()) {
                std::string key = gpuLoadIt->second;
                convertToUpper(key);
                GPULoadPerFrame = (key != "TRUE");
            }
        }
    //} else {
    //    readExternalCombo = true;
    }

    // Handle external combo reading
    //if (readExternalCombo) {
    // Try ultrahand first, then tesla
    const char* configPaths[] = {ultrahandConfigIniPath, teslaConfigIniPath};
    const char* sectionNames[] = {"ultrahand", "tesla"};
    
    std::string fileData;
    for (int i = 0; i < 2; ++i) {
        FILE* extConfigFile = fopen(configPaths[i], "r");
        if (extConfigFile) {
            // Get file size and read efficiently
            fseek(extConfigFile, 0, SEEK_END);
            const long fileSize = ftell(extConfigFile);
            fseek(extConfigFile, 0, SEEK_SET);
            
            fileData = "";
            fileData.resize(fileSize);
            fread(fileData.data(), 1, fileSize, extConfigFile);
            fclose(extConfigFile);
            
            parsedData = ult::parseIni(fileData);
            
            auto sectionIt = parsedData.find(sectionNames[i]);
            if (sectionIt != parsedData.end()) {
                auto keyComboIt = sectionIt->second.find("key_combo");
                if (keyComboIt != sectionIt->second.end()) {
                    keyCombo = keyComboIt->second;
                    removeSpaces(keyCombo);
                    convertToUpper(keyCombo);
                    break; // Found combo, exit loop
                }
            }
        }
    }
    //}
    
    comboBitmask = MapButtons(keyCombo);
}


ALWAYS_INLINE bool isValidRGBA4Color(const std::string& hexColor) {
    const char* data = hexColor.data();
    const size_t size = hexColor.size();
    
    unsigned char c;
    for (size_t i = 0; i < size; ++i) {
        c = data[i];
        // Branchless hex digit check: (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')
        if (!((c - '0') <= 9 || (c - 'A') <= 5 || (c - 'a') <= 5)) {
            return false;
        }
    }
    
    return true;
}

bool convertStrToRGBA4444(std::string hexColor, uint16_t* returnValue) {
    // Check if # is present
    if (hexColor.size() != 5 || hexColor[0] != '#')
        return false;
    
    hexColor = hexColor.substr(1);

    if (isValidRGBA4Color(hexColor)) {
        *returnValue = std::stoi(std::string(hexColor.rbegin(), hexColor.rend()), nullptr, 16);
        return true;
    }
    return false;
}

struct FullSettings {
    uint8_t refreshRate;
    bool setPosRight;
    bool showRealFreqs;
    bool realVolts; 
    bool showDeltas;
    bool showTargetFreqs;
    bool showFPS;
    bool showRES;
};

struct MiniSettings {
    uint8_t refreshRate;
    bool realFrequencies;
    bool realVolts;
    bool showFullCPU;
    bool showFullResolution;
    bool showFanPercentage;
    bool showVDDQ;
    bool showVDD2;
    size_t handheldFontSize;
    size_t dockedFontSize;
    size_t spacing;
    uint16_t backgroundColor;
    uint16_t separatorColor;
    uint16_t catColor;
    uint16_t textColor;
    std::string show;
    bool showRAMLoad;
    int setPos;
};

struct MicroSettings {
    uint8_t refreshRate;
    bool realFrequencies;
    bool realVolts; 
    bool showFullCPU; 
    bool showFullResolution;
    size_t handheldFontSize;
    size_t dockedFontSize;
    uint8_t alignTo;
    uint16_t backgroundColor;
    uint16_t separatorColor;
    uint16_t catColor;
    uint16_t textColor;
    std::string show;
    bool showRAMLoad;
    bool setPosBottom;
};

struct FpsCounterSettings {
    uint8_t refreshRate;
    size_t handheldFontSize;
    size_t dockedFontSize;
    uint16_t backgroundColor;
    uint16_t textColor;
    int setPos;
};

struct FpsGraphSettings {
    bool showInfo;
    uint8_t refreshRate;
    uint16_t backgroundColor;
    uint16_t fpsColor;
    uint16_t mainLineColor;
    uint16_t roundedLineColor;
    uint16_t perfectLineColor;
    uint16_t dashedLineColor;
    uint16_t borderColor;
    uint16_t maxFPSTextColor;
    uint16_t minFPSTextColor;
    int setPos;
};

struct ResolutionSettings {
    uint8_t refreshRate;
    uint16_t backgroundColor;
    uint16_t catColor;
    uint16_t textColor;
    int setPos;
};

ALWAYS_INLINE void GetConfigSettings(MiniSettings* settings) {
    // Initialize defaults
    settings->realFrequencies = true;
    settings->realVolts = true;
    settings -> showFullCPU = false;
    settings -> showFullResolution = true;
    settings -> showFanPercentage = true;
    settings -> showFullCPU = false;
    settings -> showVDDQ = true;
    settings -> showVDD2 = false;
    settings->handheldFontSize = 15;
    settings->dockedFontSize = 15;
    settings->spacing = 8;
    convertStrToRGBA4444("#0009", &(settings -> backgroundColor));
    convertStrToRGBA4444("#2DFF", &(settings -> separatorColor));
    convertStrToRGBA4444("#2DFF", &(settings -> catColor));
    convertStrToRGBA4444("#FFFF", &(settings -> textColor));
    settings->show = "CPU+GPU+RAM+SOC+BAT+FPS+RES";
    settings->showRAMLoad = true;
    settings->refreshRate = 1;
    settings->setPos = 0;

    // Open and read file efficiently
    FILE* configFile = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFile) return;
    
    fseek(configFile, 0, SEEK_END);
    const long fileSize = ftell(configFile);
    fseek(configFile, 0, SEEK_SET);

    std::string fileData;
    fileData.resize(fileSize);
    fread(fileData.data(), 1, fileSize, configFile);
    fclose(configFile);
    
    auto parsedData = ult::parseIni(fileData);

    // Cache section lookup
    auto sectionIt = parsedData.find("mini");
    if (sectionIt == parsedData.end()) return;
    
    const auto& section = sectionIt->second;
    
    // Process refresh_rate
    auto it = section.find("refresh_rate");
    if (it != section.end()) {
        settings->refreshRate = std::clamp(atol(it->second.c_str()), 1L, 60L);
    }
    
    // Process boolean flags
    it = section.find("real_freqs");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->realFrequencies = (key == "TRUE");
    }
    
    it = section.find("real_volts");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->realVolts = (key == "TRUE");
    }
    
    // Process font sizes with shared bounds
    constexpr long minFontSize = 8;
    constexpr long maxFontSize = 22;
    
    it = section.find("handheld_font_size");
    if (it != section.end()) {
        settings->handheldFontSize = std::clamp(atol(it->second.c_str()), minFontSize, maxFontSize);
    }
    
    it = section.find("docked_font_size");
    if (it != section.end()) {
        settings->dockedFontSize = std::clamp(atol(it->second.c_str()), minFontSize, maxFontSize);
    }

    it = section.find("spacing");
    if (it != section.end()) {
        settings->spacing = atol(it->second.c_str());
    }
    
    // Process colors
    it = section.find("background_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }
    
    it = section.find("separator_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->separatorColor = temp;
    }
    

    it = section.find("cat_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor = temp;
    }
    
    it = section.find("text_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
    
    // Process RAM load flag
    it = section.find("show_full_cpu");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showFullCPU = !(key == "FALSE");
    }

    it = section.find("show_full_res");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showFullResolution = !(key == "FALSE");
    }

    it = section.find("show_show_vddq");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showVDDQ = !(key == "FALSE");
    }

    it = section.find("show_show_vdd2");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showVDD2 = !(key == "FALSE");
    }

    // Process show string
    it = section.find("show");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->show = std::move(key);
    }
    
    // Process RAM load flag
    it = section.find("replace_MB_with_RAM_load");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showRAMLoad = (key != "FALSE");
    }
    
    // Process alignment settings
    it = section.find("layer_width_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos = 1;
        } else if (key == "RIGHT") {
            settings->setPos = 2;
        }
    }
    
    it = section.find("layer_height_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos += 3;
        } else if (key == "BOTTOM") {
            settings->setPos += 6;
        }
    }
}

ALWAYS_INLINE void GetConfigSettings(MicroSettings* settings) {
    settings -> realFrequencies = true;
    settings -> realVolts = true;
    settings -> showFullCPU = false;
    settings -> showFullResolution = false;
    settings -> handheldFontSize = 15;
    settings -> dockedFontSize = 15;
    settings -> alignTo = 1;
    convertStrToRGBA4444("#0009", &(settings -> backgroundColor));
    convertStrToRGBA4444("#2DFF", &(settings -> separatorColor));
    convertStrToRGBA4444("#2DFF", &(settings -> catColor));
    convertStrToRGBA4444("#FFFF", &(settings -> textColor));
    settings -> show = "FPS+CPU+GPU+RAM+SOC+BAT";
    settings -> showRAMLoad = true;
    settings -> setPosBottom = false;
    settings -> refreshRate = 1;

    FILE* configFileIn = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFileIn)
        return;
    fseek(configFileIn, 0, SEEK_END);
    const long fileSize = ftell(configFileIn);
    rewind(configFileIn);

    std::string fileDataString(fileSize, '\0');
    fread(&fileDataString[0], sizeof(char), fileSize, configFileIn);
    fclose(configFileIn);
    
    auto parsedData = ult::parseIni(fileDataString);

    std::string key;
    const char* mode = "micro";
    if (parsedData.find(mode) == parsedData.end())
        return;
    if (parsedData[mode].find("refresh_rate") != parsedData[mode].end()) {
        static constexpr long maxFPS = 60;
        static constexpr long minFPS = 1;
        
        key = parsedData[mode]["refresh_rate"];
        const long rate = atol(key.c_str());
        if (rate < minFPS) {
            settings -> refreshRate = minFPS;
        }
        else if (rate > maxFPS)
            settings -> refreshRate = maxFPS;
        else settings -> refreshRate = rate;    
    }
    if (parsedData[mode].find("real_freqs") != parsedData[mode].end()) {
        key = parsedData[mode]["real_freqs"];
        convertToUpper(key);
        settings -> realFrequencies = !(key.compare("TRUE"));
    }
    if (parsedData[mode].find("real_volts") != parsedData[mode].end()) {  
        key = parsedData[mode]["real_volts"];  
        convertToUpper(key);  
        settings -> realVolts = !(key.compare("TRUE"));  
    }  
    if (parsedData[mode].find("show_full_cpu") != parsedData[mode].end()) { 
        key = parsedData[mode]["show_full_cpu"]; 
        convertToUpper(key); 
        settings -> showFullCPU = !(key.compare("TRUE")); 
    } 
    if (parsedData[mode].find("show_full_res") != parsedData[mode].end()) { 
        key = parsedData[mode]["show_full_res"]; 
        convertToUpper(key);
        settings -> showFullResolution = !(key.compare("TRUE")); 
    } 
    if (parsedData[mode].find("text_align") != parsedData[mode].end()) {
        key = parsedData[mode]["text_align"];
        convertToUpper(key);
        if (!key.compare("LEFT")) {
            settings -> alignTo = 0;
        }
        else if (!key.compare("CENTER")) {
            settings -> alignTo = 1;
        }        
        else if (!key.compare("RIGHT")) {
            settings -> alignTo = 2;
        }
    }

    static constexpr long maxFontSize = 18;
    static constexpr long minFontSize = 8;

    if (parsedData[mode].find("handheld_font_size") != parsedData[mode].end()) {
        key = parsedData[mode]["handheld_font_size"];
        long fontsize = atol(key.c_str());
        if (fontsize < minFontSize)
            settings -> handheldFontSize = minFontSize;
        else if (fontsize > maxFontSize)
            settings -> handheldFontSize = maxFontSize;
        else settings -> handheldFontSize = fontsize;    
    }
    if (parsedData[mode].find("docked_font_size") != parsedData[mode].end()) {
        key = parsedData[mode]["docked_font_size"];
        long fontsize = atol(key.c_str());
        if (fontsize < minFontSize)
            settings -> dockedFontSize = minFontSize;
        else if (fontsize > maxFontSize)
            settings -> dockedFontSize = maxFontSize;
        else settings -> dockedFontSize = fontsize;    
    }
    if (parsedData[mode].find("background_color") != parsedData[mode].end()) {
        key = parsedData[mode]["background_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> backgroundColor = temp;
    }
    if (parsedData[mode].find("separator_color") != parsedData[mode].end()) {
        key = parsedData[mode]["separator_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> separatorColor = temp;
    }
    if (parsedData[mode].find("cat_color") != parsedData[mode].end()) {
        key = parsedData[mode]["cat_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> catColor = temp;
    }
    if (parsedData[mode].find("text_color") != parsedData[mode].end()) {
        key = parsedData[mode]["text_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> textColor = temp;
    }
    if (parsedData[mode].find("replace_GB_with_RAM_load") != parsedData[mode].end()) {
        key = parsedData[mode]["replace_GB_with_RAM_load"];
        convertToUpper(key);
        settings -> showRAMLoad = key.compare("FALSE");
    }
    if (parsedData[mode].find("show") != parsedData[mode].end()) {
        key = parsedData[mode]["show"];
        convertToUpper(key);
        settings -> show = key;
    }
    if (parsedData[mode].find("layer_height_align") != parsedData[mode].end()) {
        key = parsedData[mode]["layer_height_align"];
        convertToUpper(key);
        settings -> setPosBottom = !key.compare("BOTTOM");
    }
}

ALWAYS_INLINE void GetConfigSettings(FpsCounterSettings* settings) {
    // Initialize defaults
    settings->handheldFontSize = 40;
    settings->dockedFontSize = 40;
    convertStrToRGBA4444("#1117", &(settings->backgroundColor));
    convertStrToRGBA4444("#FFFF", &(settings->textColor));
    settings->setPos = 0;
    settings->refreshRate = 31;

    // Open and read file efficiently
    FILE* configFile = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFile) return;
    
    fseek(configFile, 0, SEEK_END);
    const long fileSize = ftell(configFile);
    fseek(configFile, 0, SEEK_SET);

    std::string fileData;
    fileData.resize(fileSize);
    fread(fileData.data(), 1, fileSize, configFile);
    fclose(configFile);
    
    auto parsedData = ult::parseIni(fileData);

    // Cache section lookup
    auto sectionIt = parsedData.find("fps-counter");
    if (sectionIt == parsedData.end()) return;
    
    const auto& section = sectionIt->second;
    
    // Process font sizes with shared bounds
    static constexpr long minFontSize = 8;
    static constexpr long maxFontSize = 150;
    
    auto it = section.find("handheld_font_size");
    if (it != section.end()) {
        settings->handheldFontSize = std::clamp(atol(it->second.c_str()), minFontSize, maxFontSize);
    }
    
    it = section.find("docked_font_size");
    if (it != section.end()) {
        settings->dockedFontSize = std::clamp(atol(it->second.c_str()), minFontSize, maxFontSize);
    }
    
    // Process colors
    it = section.find("background_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }
    
    it = section.find("text_color");
    if (it != section.end()) {
        uint16_t temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
    
    // Process alignment settings
    it = section.find("layer_width_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos = 1;
        } else if (key == "RIGHT") {
            settings->setPos = 2;
        }
    }
    
    it = section.find("layer_height_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos += 3;
        } else if (key == "BOTTOM") {
            settings->setPos += 6;
        }
    }
}

ALWAYS_INLINE void GetConfigSettings(FpsGraphSettings* settings) {
    // Initialize defaults
    settings->showInfo = false;
    settings->setPos = 0;
    convertStrToRGBA4444("#1117", &(settings->backgroundColor));
    convertStrToRGBA4444("#4444", &(settings->fpsColor));
    convertStrToRGBA4444("#F77F", &(settings->borderColor));
    convertStrToRGBA4444("#8888", &(settings->dashedLineColor));
    convertStrToRGBA4444("#FFFF", &(settings->maxFPSTextColor));
    convertStrToRGBA4444("#FFFF", &(settings->minFPSTextColor));
    convertStrToRGBA4444("#FFFF", &(settings->mainLineColor));
    convertStrToRGBA4444("#F0FF", &(settings->roundedLineColor));
    convertStrToRGBA4444("#0C0F", &(settings->perfectLineColor));
    settings->refreshRate = 31;

    // Open and read file efficiently
    FILE* configFile = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFile) return;
    
    fseek(configFile, 0, SEEK_END);
    const long fileSize = ftell(configFile);
    fseek(configFile, 0, SEEK_SET);

    std::string fileData;
    fileData.resize(fileSize);
    fread(fileData.data(), 1, fileSize, configFile);
    fclose(configFile);
    
    auto parsedData = ult::parseIni(fileData);

    // Cache section lookup
    auto sectionIt = parsedData.find("fps-graph");
    if (sectionIt == parsedData.end()) return;
    
    const auto& section = sectionIt->second;
    
    // Process alignment settings
    auto it = section.find("layer_width_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos = 1;
        } else if (key == "RIGHT") {
            settings->setPos = 2;
        }
    }
    
    it = section.find("layer_height_align");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        if (key == "CENTER") {
            settings->setPos += 3;
        } else if (key == "BOTTOM") {
            settings->setPos += 6;
        }
    }
    
    // Process colors - using a struct for cleaner code
    struct ColorMapping {
        const char* key;
        uint16_t* target;
    };
    
    const ColorMapping colorMappings[] = {
        {"min_fps_text_color", &settings->minFPSTextColor},
        {"max_fps_text_color", &settings->maxFPSTextColor},
        {"background_color", &settings->backgroundColor},
        {"fps_counter_color", &settings->fpsColor},
        {"border_color", &settings->borderColor},
        {"dashed_line_color", &settings->dashedLineColor},
        {"main_line_color", &settings->mainLineColor},
        {"rounded_line_color", &settings->roundedLineColor},
        {"perfect_line_color", &settings->perfectLineColor}
    };
    
    for (const auto& mapping : colorMappings) {
        it = section.find(mapping.key);
        if (it != section.end()) {
            uint16_t temp = 0;
            if (convertStrToRGBA4444(it->second, &temp))
                *(mapping.target) = temp;
        }
    }
    
    // Process show_info boolean
    it = section.find("show_info");
    if (it != section.end()) {
        std::string key = it->second;
        convertToUpper(key);
        settings->showInfo = (key == "TRUE");
    }
}

ALWAYS_INLINE void GetConfigSettings(FullSettings* settings) {
    settings -> setPosRight = false;
    settings -> refreshRate = 1;
    settings -> showRealFreqs = true;
    settings -> showDeltas = true;
    settings -> showTargetFreqs = true;
    settings -> showFPS = true;
    settings -> showRES = true;

    FILE* configFileIn = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFileIn)
        return;
    fseek(configFileIn, 0, SEEK_END);
    const long fileSize = ftell(configFileIn);
    rewind(configFileIn);

    std::string fileDataString(fileSize, '\0');
    fread(&fileDataString[0], sizeof(char), fileSize, configFileIn);
    fclose(configFileIn);
    
    auto parsedData = ult::parseIni(fileDataString);

    std::string key;
    const char* mode = "full";
    if (parsedData.find(mode) == parsedData.end())
        return;
    if (parsedData[mode].find("refresh_rate") != parsedData[mode].end()) {
        static constexpr long maxFPS = 60;
        static constexpr long minFPS = 1;
        
        key = parsedData[mode]["refresh_rate"];
        long rate = atol(key.c_str());
        if (rate < minFPS) {
            settings -> refreshRate = minFPS;
        }
        else if (rate > maxFPS)
            settings -> refreshRate = maxFPS;
        else settings -> refreshRate = rate;    
    }
    if (parsedData[mode].find("layer_width_align") != parsedData[mode].end()) {
        key = parsedData[mode]["layer_width_align"];
        convertToUpper(key);
        settings -> setPosRight = !key.compare("RIGHT");
    }
    if (parsedData[mode].find("show_real_freqs") != parsedData[mode].end()) {
        key = parsedData[mode]["show_real_freqs"];
        convertToUpper(key);
        settings -> showRealFreqs = key.compare("FALSE");
    }
    if (parsedData[mode].find("show_deltas") != parsedData[mode].end()) {
        key = parsedData[mode]["show_deltas"];
        convertToUpper(key);
        settings -> showDeltas = key.compare("FALSE");
    }
    if (parsedData[mode].find("show_target_freqs") != parsedData[mode].end()) {
        key = parsedData[mode]["show_target_freqs"];
        convertToUpper(key);
        settings -> showTargetFreqs = key.compare("FALSE");
    }
    if (parsedData[mode].find("show_fps") != parsedData[mode].end()) {
        key = parsedData[mode]["show_fps"];
        convertToUpper(key);
        settings -> showFPS = key.compare("FALSE");
    }
    if (parsedData[mode].find("show_res") != parsedData[mode].end()) {
        key = parsedData[mode]["show_res"];
        convertToUpper(key);
        settings -> showRES = key.compare("FALSE");
    }
}

ALWAYS_INLINE void GetConfigSettings(ResolutionSettings* settings) {
    convertStrToRGBA4444("#1117", &(settings -> backgroundColor));
    convertStrToRGBA4444("#FFFF", &(settings -> catColor));
    convertStrToRGBA4444("#FFFF", &(settings -> textColor));
    settings -> refreshRate = 10;
    settings -> setPos = 0;

    FILE* configFileIn = fopen("sdmc:/config/status-monitor/config.ini", "r");
    if (!configFileIn)
        return;
    fseek(configFileIn, 0, SEEK_END);
    const long fileSize = ftell(configFileIn);
    rewind(configFileIn);

    std::string fileDataString(fileSize, '\0');
    fread(&fileDataString[0], sizeof(char), fileSize, configFileIn);
    fclose(configFileIn);
    
    auto parsedData = ult::parseIni(fileDataString);

    std::string key;
    const char* mode = "game_resolutions";
    if (parsedData.find("game_resolutions") == parsedData.end())
        return;
    if (parsedData[mode].find("refresh_rate") != parsedData[mode].end()) {
        static constexpr long maxFPS = 60;
        static constexpr long minFPS = 1;

        key = parsedData[mode]["refresh_rate"];
        const long rate = atol(key.c_str());
        if (rate < minFPS) {
            settings -> refreshRate = minFPS;
        }
        else if (rate > maxFPS)
            settings -> refreshRate = maxFPS;
        else settings -> refreshRate = rate;    
    }

    if (parsedData[mode].find("background_color") != parsedData[mode].end()) {
        key = parsedData[mode]["background_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> backgroundColor = temp;
    }
    if (parsedData[mode].find("cat_color") != parsedData[mode].end()) {
        key = parsedData[mode]["cat_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> catColor = temp;
    }
    if (parsedData[mode].find("text_color") != parsedData[mode].end()) {
        key = parsedData[mode]["text_color"];
        uint16_t temp = 0;
        if (convertStrToRGBA4444(key, &temp))
            settings -> textColor = temp;
    }
    if (parsedData[mode].find("layer_width_align") != parsedData[mode].end()) {
        key = parsedData[mode]["layer_width_align"];
        convertToUpper(key);
        if (!key.compare("CENTER")) {
            settings -> setPos = 1;
        }
        if (!key.compare("RIGHT")) {
            settings -> setPos = 2;
        }
    }
    if (parsedData[mode].find("layer_height_align") != parsedData[mode].end()) {
        key = parsedData[mode]["layer_height_align"];
        convertToUpper(key);
        if (!key.compare("CENTER")) {
            settings -> setPos += 3;
        }
        if (!key.compare("BOTTOM")) {
            settings -> setPos += 6;
        }
    }
}