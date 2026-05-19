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

// rgltr_services.cpp (no changes needed here—just compile it once)
#include <switch.h>
#include "rgltr.h"
#include "rgltr_services.h"  // for extern Service g_rgltrSrv, etc.



#if defined(__cplusplus)
extern "C"
{
#endif

#include <sysclk/client/ipc.h>
#include <hocclk/client/ipc.h>

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
Thread t5;
Thread t6;
Thread t7;
uint64_t systemtickfrequency = 19200000;

LEvent threadexit;
PwmChannelSession g_ICon;
const std::string folderpath = "sdmc:/switch/.overlays/";

// Use const string_view for paths to avoid string copying
constexpr const char* directoryPath = "sdmc:/config/status-monitor/";
constexpr const char* configIniPath = "sdmc:/config/status-monitor/config.ini";
constexpr const char* ultrahandConfigIniPath = "sdmc:/config/ultrahand/config.ini";
constexpr const char* teslaConfigIniPath = "sdmc:/config/tesla/config.ini";

std::string filename;
std::string filepath;
std::string keyCombo = "ZL+ZR+DDOWN"; // default Ultrahand Menu combo

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
Result hocclkCheck = 1;
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
//uint64_t idletick0 = systemtickfrequency;
//uint64_t idletick1 = systemtickfrequency;
//uint64_t idletick2 = systemtickfrequency;
//uint64_t idletick3 = systemtickfrequency;

std::atomic<uint64_t> idletick0{systemtickfrequency};
std::atomic<uint64_t> idletick1{systemtickfrequency};
std::atomic<uint64_t> idletick2{systemtickfrequency};
std::atomic<uint64_t> idletick3{systemtickfrequency};


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
    union {
        struct {
            bool handheld: 1;
            bool docked: 1;
            unsigned int reserved: 6;
        } NX_PACKED ds;
        uint8_t general;
    } displaySync;
    resolutionCalls renderCalls[8];
    resolutionCalls viewportCalls[8];
    bool forceOriginalRefreshRate;
    bool dontForce60InDocked;
    bool forceSuspend;
    uint8_t currentRefreshRate;
    float readSpeedPerSecond;
    uint8_t FPSlockedDocked;
    uint64_t frameNumber;
} NX_PACKED;

NxFpsSharedBlock* NxFps = 0;
std::atomic<bool> GameRunning{false};
std::atomic<bool> check{true};
std::atomic<bool> SaltySD{false};
std::atomic<bool> realVoltsPolling{false};
uintptr_t FPSaddress = 0;
uintptr_t FPSavgaddress = 0;
uint64_t PID = 0;
uint32_t FPS = 0xFE;
float FPSmin = 254; 
float FPSmax = 0; 
float FPSavg = 254;
float FPSavg_old = 254;
bool useOldFPSavg = false;
SharedMemory _sharedmemory = {};
std::atomic<bool> SharedMemoryUsed{false};
Handle remoteSharedMemory = 1;
uint64_t lastFrameNumber = 0;

//Read real freqs from sys-clk sysmodule
uint32_t realCPU_Hz = 0;
uint32_t realGPU_Hz = 0;
uint32_t realRAM_Hz = 0;
uint32_t ramLoad[SysClkRamLoad_EnumMax];
uint32_t realCPU_mV = 0; 
uint32_t realGPU_mV = 0; 
uint32_t realRAM_mV = 0; 
uint32_t realSOC_mV = 0; 
uint32_t componentCPU_mC = 0;  // CPU die temp (milliCelsius) - HOC IPC or SOCTHERM direct
uint32_t componentGPU_mC = 0;  // GPU die temp (milliCelsius) - HOC IPC or SOCTHERM direct
uint32_t componentRAM_mC = 0;  // MEM die temp (milliCelsius) - HOC IPC or SOCTHERM direct
uint8_t refreshRate = 0;

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
//  SOCTHERM – CPU/GPU/MEM die temperatures (non-HOC path)
//
//  HOS does not enable the SENSOR_TEMP1/TEMP2 combined outputs — it reads
//  temperatures through its own internal path.  We must initialise the
//  sensors ourselves: read per-chip fuse calibration, compute therma/thermb
//  coefficients, write them to each sensor's CFG2 register, then enable
//  TSENSOR_CLKEN.  After that the hardware updates SENSOR_TEMP1/2 every
//  measurement cycle and we just read them.
//
//  CAR is NOT touched — HOS already runs the SOCTHERM clock and we must not
//  disturb the clock controller.  Sleep detection uses TSENSOR_CLKEN readback
//  instead of the CAR registers HOC uses.
//
//  Requires ovll.json kernel_capabilities:
//    0x700E2000 (SOCTHERM, 4KB) read+write
//    0x7000F000 (FUSE,     4KB) read-only
// ─────────────────────────────────────────────────────────────────────────────
namespace Soctherm {

// Physical base addresses
static constexpr u64 PA_SOC  = 0x700E2000;
static constexpr u64 PA_FUSE = 0x7000F000;

// SOCTHERM register offsets
static constexpr u32 CFG0             = 0x00;
static constexpr u32 CFG1             = 0x04;
static constexpr u32 CFG2             = 0x08;
static constexpr u32 CFG0_TALL_SHIFT  = 8;
static constexpr u32 CFG1_TSMP_SHIFT  = 0;
static constexpr u32 CFG1_TIDDQ_SHIFT = 15;
static constexpr u32 CFG1_TEN_SHIFT   = 24;
static constexpr u32 CFG1_TEMP_ENABLE = 1u << 31;
static constexpr u32 CFG2_THERMA_SHIFT= 16;

static constexpr u32 SENSOR_PDIV        = 0x1C0;
static constexpr u32 SENSOR_HOTSPOT_OFF = 0x1C4;
static constexpr u32 SENSOR_TEMP1       = 0x1C8; // [31:16]=CPU [15:0]=GPU
static constexpr u32 SENSOR_TEMP2       = 0x1CC; // [31:16]=MEM [15:0]=PLLX
static constexpr u32 TSENSOR_CLKEN      = 0x1DC;
static constexpr u32 TSENSOR_ENABLE     = 225u;

static constexpr u32 PDIV_T210B0 = 0xCC0Cu;
static constexpr u32 PDIV_T210   = 0x8888u;
static constexpr u32 HOTSPOT_VAL = 0x000A0500u;
static constexpr u32 PDIV_MASK_T210B0  = 0xFFFF00F0u;
static constexpr u32 PDIV_MASK_T210    = 0xFFFF0000u;
static constexpr u32 HSPOT_MASK_T210B0 = 0xFF0000FFu;
static constexpr u32 HSPOT_MASK_T210   = 0xFF000000u;

// READBACK format bits
static constexpr u32 RB_MASK  = 0xFF00u;
static constexpr u32 RB_SHIFT = 8;
static constexpr u32 RB_HALF  = 1u << 7;
static constexpr u32 RB_NEG   = 1u << 0;

// FUSE offsets (relative to PA_FUSE)
static constexpr u32 FUSE_CACHE_OFF    = 0x800;
static constexpr u32 FUSE_COMMON       = 0xA80; // 0x7000FA80 absolute
static constexpr u32 FUSE_CP_MASK      = 0x3FFu << 11;
static constexpr u32 FUSE_CP_SHIFT     = 11;
static constexpr u32 FUSE_FT_MASK      = 0x7FFu << 21;
static constexpr u32 FUSE_FT_SHIFT     = 21;
static constexpr u32 FUSE_FT_BASE_MASK = 0x1FFFu << 13;
static constexpr u32 FUSE_FT_BASE_SHIFT= 13;
static constexpr u32 FUSE_SFT_FT_MASK  = 0x1Fu << 6;
static constexpr u32 FUSE_SFT_FT_SHIFT = 6;
static constexpr s32 NOMINAL_CP        = 25;
static constexpr s32 NOMINAL_FT        = 105;
static constexpr s64 CALIB_COEFF       = 1000000LL;

struct TSConf { u32 tall,tiddq,ten,pdiv,pdiv_ate,tsmp,tsmp_ate; };
struct FCorr  { s32 alpha, beta; };
struct TSGrp  { u32 temp_off, pdiv_mask; };
struct TSens  { u32 base; const TSConf* cfg; u32 fuse_off; FCorr corr; const TSGrp* grp; };

static constexpr TSConf eristaConf = {16300,1,1, 8, 8,120,480};
static constexpr TSConf marikoConf = {16300,1,1,12, 6,240,480};

static constexpr TSGrp gCpu = {SENSOR_TEMP1, 0xFu << 12};
static constexpr TSGrp gGpu = {SENSOR_TEMP1, 0xFu <<  8};
static constexpr TSGrp gPll = {SENSOR_TEMP2, 0xFu <<  0};
static constexpr TSGrp gMem = {SENSOR_TEMP2, 0xFu <<  4};

static constexpr TSens eS[] = {
    {0x0C0,&eristaConf,0x198,{1085000, 3244200},&gCpu},
    {0x0E0,&eristaConf,0x184,{1126200,  -67500},&gCpu},
    {0x100,&eristaConf,0x188,{1098400, 2251100},&gCpu},
    {0x120,&eristaConf,0x22C,{1108000,  602700},&gCpu},
    {0x180,&eristaConf,0x254,{1074300, 2734900},&gGpu},
    {0x1A0,&eristaConf,0x260,{1039700, 6829100},&gPll},
    {0x140,&eristaConf,0x258,{1069200, 3549900},&gMem},
    {0x160,&eristaConf,0x25C,{1173700,-6263600},&gMem},
};
static constexpr TSens mS[] = {
    {0x0C0,&marikoConf,0x198,{1085000, 3244200},&gCpu},
    {0x0E0,&marikoConf,0x184,{1126200,  -67500},&gCpu},
    {0x100,&marikoConf,0x188,{1098400, 2251100},&gCpu},
    {0x120,&marikoConf,0x22C,{1108000,  602700},&gCpu},
    {0x180,&marikoConf,0x254,{1074300, 2734900},&gGpu},
    {0x1A0,&marikoConf,0x260,{1039700, 6829100},&gPll},
};

static u64  vSoc  = 0;
static u64  vFuse = 0;
static u32  cal[8] = {};
static bool ready = false;

template<typename T=u32> static T    Rd(u64 b, u32 o)    { return *reinterpret_cast<volatile T*>(b+o); }
template<typename T=u32> static void Wr(u64 b, u32 o, T v){ *reinterpret_cast<volatile T*>(b+o) = v;   }
template<typename T=u32> static void Sb(u64 b, u32 o, T m){ Wr<T>(b, o, Rd<T>(b,o)|m); }

static bool MapPA(u64& va, u64 pa) {
    if (hosversionAtLeast(10,0,0)) {
        u64 sz = 0;
        return R_SUCCEEDED(svcQueryMemoryMapping(&va, &sz, pa, 0x1000));
    } else {
        return R_SUCCEEDED(svcLegacyQueryIoMapping(&va, pa, 0x1000));
    }
}

static s32  Sext32(u32 v, int idx) { u8 sh = 31-idx; return (s32)(v<<sh)>>sh; }
static s64  Div64p(s64 a, s32 b)   { s64 al=a<<16; return ((al*2+1)/(2*(s64)b))>>16; }

// Decode SOCTHERM READBACK format to milliCelsius — identical to HOC TranslateTemp.
static s32 Trans(u16 val) {
    s32 t = ((val & RB_MASK) >> RB_SHIFT) * 1000;
    if (val & RB_HALF) t += 500;
    if (val & RB_NEG)  t  = -t;
    return t;
}

static void CalcCal(const TSens& s, u32 bcp, u32 bft, s32 tcp, s32 tft, u32& out) {
    u32 raw  = Rd(vFuse, s.fuse_off + FUSE_CACHE_OFF);
    s32 tscp = (s32)(bcp*64) + Sext32(raw, 12);
    s32 tsft = (s32)(bft*32) + Sext32((raw & FUSE_FT_BASE_MASK) >> FUSE_FT_BASE_SHIFT, 12);
    s32 ds   = tsft - tscp,  dt = tft - tcp;
    s32 mult = s.cfg->pdiv * s.cfg->tsmp_ate;
    s32 div  = s.cfg->tsmp * s.cfg->pdiv_ate;
    s64 tmp  = (s64)dt * (1LL<<13) * mult;
    s16 A    = (s16)Div64p(tmp, (s64)ds * div);
    tmp      = (s64)tsft * tcp - (s64)tscp * tft;
    s16 B    = (s16)Div64p(tmp, ds);
    tmp      = (s64)A * s.corr.alpha;
    A        = (s16)Div64p(tmp, CALIB_COEFF);
    tmp      = (s64)B * s.corr.alpha + s.corr.beta;
    B        = (s16)Div64p(tmp, CALIB_COEFF);
    out      = ((u16)A << CFG2_THERMA_SHIFT) | (u16)B;
}

static void EnSensor(const TSens& s, u32 c) {
    Wr(vSoc, s.base + CFG0,
        s.cfg->tall << CFG0_TALL_SHIFT);
    Wr(vSoc, s.base + CFG1,
        ((s.cfg->tsmp - 1) << CFG1_TSMP_SHIFT) |
        (s.cfg->tiddq      << CFG1_TIDDQ_SHIFT) |
        (s.cfg->ten        << CFG1_TEN_SHIFT)   |
        CFG1_TEMP_ENABLE);
    Wr(vSoc, s.base + CFG2, c);
}

static void StartSensors() {
    if (isMariko) {
        for (u32 i = 0; i < 6; ++i) EnSensor(mS[i], cal[i]);
        Wr(vSoc, SENSOR_PDIV,
            (Rd(vSoc,SENSOR_PDIV) & PDIV_MASK_T210B0) | PDIV_T210B0);
        Wr(vSoc, SENSOR_HOTSPOT_OFF,
            (Rd(vSoc,SENSOR_HOTSPOT_OFF) & HSPOT_MASK_T210B0) | HOTSPOT_VAL);
    } else {
        for (u32 i = 0; i < 8; ++i) EnSensor(eS[i], cal[i]);
        Wr(vSoc, SENSOR_PDIV,
            (Rd(vSoc,SENSOR_PDIV) & PDIV_MASK_T210) | PDIV_T210);
        Wr(vSoc, SENSOR_HOTSPOT_OFF,
            (Rd(vSoc,SENSOR_HOTSPOT_OFF) & HSPOT_MASK_T210) | HOTSPOT_VAL);
    }
    Wr(vSoc, TSENSOR_CLKEN, TSENSOR_ENABLE);
}

// One-time init: map hardware, compute fuse calibration, enable sensors.
static void Initialize() {
    if (ready) return;
    if (!MapPA(vSoc,  PA_SOC))  return;
    if (!MapPA(vFuse, PA_FUSE)) { vSoc = 0; return; }

    // Read per-chip fuse calibration (HOS clock is already on — no CAR writes needed)
    u32 fc  = Rd(vFuse, FUSE_COMMON);
    u32 bcp = (fc & FUSE_CP_MASK)      >> FUSE_CP_SHIFT;
    u32 bft = (fc & FUSE_FT_MASK)      >> FUSE_FT_SHIFT;
    s32 sft = Sext32((fc & FUSE_SFT_FT_MASK) >> FUSE_SFT_FT_SHIFT, 4);
    s32 scp = Sext32(fc, 5);
    s32 tcp = 2 * NOMINAL_CP + scp;
    s32 tft = 2 * NOMINAL_FT + sft;

    if (isMariko) { for (u32 i = 0; i < 6; ++i) CalcCal(mS[i],bcp,bft,tcp,tft,cal[i]); }
    else          { for (u32 i = 0; i < 8; ++i) CalcCal(eS[i],bcp,bft,tcp,tft,cal[i]); }

    StartSensors();
    ready = true;
}

// Called each poll cycle.
// Sleep detection: if TSENSOR_CLKEN was cleared (e.g. sleep/wake), re-enable.
static void Read() {
    if (!ready || !vSoc) return;
    if (Rd(vSoc, TSENSOR_CLKEN) != TSENSOR_ENABLE) { StartSensors(); return; }
    u32 t1 = Rd(vSoc, SENSOR_TEMP1);
    u32 t2 = Rd(vSoc, SENSOR_TEMP2);
    if (!t1 && !t2) return; // sensors still warming up; keep last values
    componentCPU_mC = (u32)Trans((u16)(t1 >> 16));
    componentGPU_mC = (u32)Trans((u16)(t1 & 0xFFFFu));
    // Erista: dedicated MEM sensor in TEMP2[31:16]
    // Mariko: no MEM sensor — use PLLX [15:0] as proxy (same as HOC)
    componentRAM_mC = isMariko
        ? (u32)Trans((u16)(t2 & 0xFFFFu))
        : (u32)Trans((u16)(t2 >> 16));
}

} // namespace Soctherm




int compare (const void* elem1, const void* elem2) {
    if ((((resolutionCalls*)(elem1))->calls) > (((resolutionCalls*)(elem2))->calls)) return -1;
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
    if (!base || !SharedMemoryUsed) {
        NxFps = 0;
        return;
    }
    
    ptrdiff_t search_offset = 0;
    const uintptr_t memory_end = base + 0x1000;
    
    while (search_offset < 0x1000) {
        const uintptr_t current_addr = base + search_offset;
        
        // Ensure we don't read past the end of shared memory
        if (current_addr + sizeof(NxFpsSharedBlock) > memory_end) {
            break;
        }
        
        NxFps = (NxFpsSharedBlock*)current_addr;
        
        // Add bounds checking and magic validation
        if (NxFps && current_addr >= base && NxFps->MAGIC == 0x465053) {
            return;
        }
        search_offset += 4;
    }
    NxFps = 0;
}

//Check if SaltyNX is working
bool CheckPort() {
    Handle saltysd;
    
    // Try up to 67 times with exponential backoff for better responsiveness
    for (int i = 0; i < 50; i++) {
        if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
            svcCloseHandle(saltysd);
            return true;
        }
        
        // Progressive sleep - start fast, then slow down
        //if (i < 10) {
        //    svcSleepThread(100'000);    // 0.1ms for first 10 attempts
        //} else if (i < 30) {
        //    svcSleepThread(500'000);    // 0.5ms for next 20 attempts  
        //} else {
        //    svcSleepThread(1'000'000);  // 1ms for remaining attempts
        //}
    }
    
    return false;
}

Mutex mutex_Misc = {0};

void CheckIfGameRunning(void*) {
    do {
        mutexLock(&mutex_Misc);
        if (!check && R_FAILED(pmdmntGetApplicationProcessId(&PID))) {
            GameRunning = false;
            check = true;
        }
        else if (!GameRunning && SharedMemoryUsed) {
            const uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
            searchSharedMemoryBlock(base);
            if (NxFps) {
                (NxFps->pluginActive) = false;
                mutexUnlock(&mutex_Misc);  // ← Fix: Unlock before return
                if (leventWait(&threadexit, 100'000'000)) {
                    return;
                }
                mutexLock(&mutex_Misc);
                if ((NxFps->pluginActive)) {
                    GameRunning = true;
                    check = false;
                }
            }
        }
        mutexUnlock(&mutex_Misc);
    } while (!leventWait(&threadexit, 1'000'000'000));
}

// Utils.hpp or your relevant header
static constexpr size_t CACHE_ELEMENTS = sizeof(BatteryTimeCache) / sizeof(BatteryTimeCache[0]);

Mutex mutex_BatteryChecker = {0};
void BatteryChecker(void*) {
    if (R_FAILED(psmCheck) || R_FAILED(i2cCheck)){
        return;
    }
    uint16_t data = 0;
    float tempV = 0.0;
    float tempA = 0.0;
    size_t ArraySize = 10;
    if (batteryFiltered) {
        ArraySize = 1;
    }
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
        float batteryTimeEstimateInMinutes = (5.625 * data) / 60;
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
        const uint64_t startTick = svcGetSystemTick();

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
            const uint64_t new_tick_TTE = svcGetSystemTick();
            if (armTicksToNs(new_tick_TTE - tick_TTE) / 1'000'000'000 >= batteryTimeLeftRefreshRate) {
                const size_t to_divide = itr < cacheElements ? itr : cacheElements;
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
    threadCreate(&t7, BatteryChecker, NULL, NULL, 0x2000, 0x3F, 3);
    threadStart(&t7);
}

void CloseBatteryThread() {
    leventSignal(&threadexit);
    threadWaitForExit(&t7);
    threadClose(&t7);
}



void gpuLoadThread(void*) {
    #define gpu_samples_average 8
    uint32_t gpu_load_array[gpu_samples_average] = {0};
    size_t i = 0;
    if (!GPULoadPerFrame && R_SUCCEEDED(nvCheck)) do {
        u32 temp;
        if (R_SUCCEEDED(nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &temp))) {
            gpu_load_array[i++ % gpu_samples_average] = temp;
            GPU_Load_u = std::accumulate(&gpu_load_array[0], &gpu_load_array[gpu_samples_average], 0) / gpu_samples_average;
        }
    } while(!leventWait(&threadexit, 16'666'000));
}

std::string getVersionString() {
    char buf[0x100] = "";  // 256 bytes — safe for any expected version string
    Result rc = sysclkIpcGetVersionString(buf, sizeof(buf));
    if (R_FAILED(rc) || buf[0] == '\0') {
        return "unknown";
    }
    return std::string(buf);
}

bool usingEOS() {
    const std::string versionString = getVersionString();
    return versionString.find("hoc") != std::string::npos;
}


bool usingHOC() {
    // If hoc-clk (hoc:clk) is connected, we are in HOC mode
    if (R_SUCCEEDED(hocclkCheck)) return true;
    // Otherwise check if sys-clk-hoc (sys:clk) reports "hoc" in its version string
    const std::string versionString = getVersionString();
    return versionString.find("hoc") != std::string::npos;
}


// === ULTRA-FAST VOLTAGE READING ===
static constexpr PowerDomainId domains[] = {
    PcvPowerDomainId_Max77621_Cpu,    // [0] CPU
    PcvPowerDomainId_Max77621_Gpu,    // [1] GPU  
    PcvPowerDomainId_Max77812_Dram,   // [2] VDD2 (EMC/DRAM)
    PcvPowerDomainId_Max77620_Sd0,    // [3] SOC
    PcvPowerDomainId_Max77620_Sd1     // [4] VDDQ
};


// Stuff that doesn't need multithreading
void Misc(void*) {
    const uint64_t timeout_ns = TeslaFPS < 10 ? (1'000'000'000 / TeslaFPS) : 100'000'000;
    const bool isUsingEOSorHOC = usingEOS() || usingHOC();
    
    // Initialize voltage reading if needed
    bool canReadVoltages = false;
    if (!isUsingEOSorHOC && realVoltsPolling) {
        canReadVoltages = R_SUCCEEDED(rgltrInitialize());
        if (!canReadVoltages) {
            realVoltsPolling = false;
        }
    }

    // Initialize SOCTHERM hardware for direct die-temp reading (non-HOC path).
    // Maps SOCTHERM + FUSE + CAR, computes fuse calibration, starts sensors.
    if (!usingHOC()) Soctherm::Initialize();
    
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
        
        // Get sys-clk data (sys-clk-hoc: sys:clk service)
        if (R_SUCCEEDED(sysclkCheck)) {
            SysClkContext sysclkCTX;
            if (R_SUCCEEDED(sysclkIpcGetCurrentContext(&sysclkCTX))) {
                realCPU_Hz = sysclkCTX.realFreqs[SysClkModule_CPU];
                realGPU_Hz = sysclkCTX.realFreqs[SysClkModule_GPU];
                realRAM_Hz = sysclkCTX.realFreqs[SysClkModule_MEM];
                ramLoad[SysClkRamLoad_All] = sysclkCTX.ramLoad[SysClkRamLoad_All];
                ramLoad[SysClkRamLoad_Cpu] = sysclkCTX.ramLoad[SysClkRamLoad_Cpu];
                
                // Voltages from sys-clk (EOS/HOC path)
                if (isUsingEOSorHOC && realVoltsPolling) {
                    realCPU_mV = sysclkCTX.realVolts[0]; 
                    realGPU_mV = sysclkCTX.realVolts[1]; 
                    realRAM_mV = sysclkCTX.realVolts[2]; 
                    realSOC_mV = sysclkCTX.realVolts[3];
                }
                // HOC die temps via sys-clk-hoc IPC (packed into perfConfId/realProfile/reserved[0])
                if (usingHOC()) {
                    componentCPU_mC = sysclkCTX.perfConfId;
                    componentGPU_mC = (uint32_t)sysclkCTX.realProfile;
                    componentRAM_mC = (uint32_t)sysclkCTX.reserved[0];
                }
            }
        }
        // Get hoc-clk data (Horizon OC native hoc:clk IPC)
        else if (R_SUCCEEDED(hocclkCheck)) {
            HocClkContext hocclkCTX;
            if (R_SUCCEEDED(hocclkIpcGetCurrentContext(&hocclkCTX))) {
                realCPU_Hz = hocclkCTX.stable.realFreqs[HocClkModule_CPU];
                realGPU_Hz = hocclkCTX.stable.realFreqs[HocClkModule_GPU];
                realRAM_Hz = hocclkCTX.stable.realFreqs[HocClkModule_MEM];
                ramLoad[SysClkRamLoad_All] = hocclkCTX.stable.partLoad[HocClkPartLoad_EMC];
                ramLoad[SysClkRamLoad_Cpu] = hocclkCTX.stable.partLoad[HocClkPartLoad_EMCCpu];
                if (realVoltsPolling) {
                    realCPU_mV = hocclkCTX.stable.voltages[HocClkVoltage_CPU];
                    realGPU_mV = hocclkCTX.stable.voltages[HocClkVoltage_GPU];
                    // voltages[] are in µV. Convert to mV then pack:
                    // vdd2_mV * 100000 + vddq_mV * 10  (display unpacks same way as sys-clk-hoc)
                    // Erista: EMCVDDQ == EMCVDD2 so skip VDDQ (matches sys-clk-hoc ipc_service.cpp)
                    {
                        const float    hoc_vdd2_mV = (float)hocclkCTX.stable.voltages[HocClkVoltage_EMCVDD2] / 1000.0f;
                        const uint32_t hoc_vddq_mV = hocclkCTX.stable.voltages[HocClkVoltage_EMCVDDQ] / 1000u;
                        realRAM_mV = (uint32_t)(hoc_vdd2_mV * 100000.0f);
                        if (hoc_vddq_mV < 1000u) {  // Mariko: VDDQ is separate lower rail
                            realRAM_mV += hoc_vddq_mV * 10u;
                        }
                    }
                    realSOC_mV = hocclkCTX.stable.voltages[HocClkVoltage_SOC];
                }
                // HOC die temps directly from hoc-clk IPC (per-die SOCTHERM values)
                componentCPU_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_CPU];
                componentGPU_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_GPU];
                componentRAM_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_MEM];
            }
        }

        // Non-HOC: read CPU/GPU/MEM die temps directly from SOCTHERM hardware
        if (!usingHOC()) Soctherm::Read();
        
        // Read voltages directly if not using EOS
        if (canReadVoltages) {
            RgltrSession session;
            u32 vdd2_raw = 0, vddq_raw = 0;
            
            // CPU voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77621_Cpu))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realCPU_mV))) {
                    realCPU_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // GPU voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77621_Gpu))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realGPU_mV))) {
                    realGPU_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // SOC voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77620_Sd0))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realSOC_mV))) {
                    realSOC_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // VDD2 (DRAM) - different domains for Mariko vs Erista
            if (isMariko) {
                if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77620_Sd1))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vdd2_raw))) {
                        vdd2_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            } else {
                // Erista uses Max77620_Sd1 for VDD2
                if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77620_Sd1))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vdd2_raw))) {
                        vdd2_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            }
            
            // VDDQ
            if (isMariko) {
                if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77812_Dram))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vddq_raw))) {
                        vddq_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            }
            
            // Pack VDD2 and VDDQ into realRAM_mV in sys-clk format
            const u32 vdd2_mV = vdd2_raw / 1000;  // µV to mV
            const u32 vddq_mV = vddq_raw / 1000;  // µV to mV
            realRAM_mV = vdd2_mV * 100000 + vddq_mV * 10;
        }
        
        // Temperatures
        if (R_SUCCEEDED(i2cCheck)) {
            Tmp451GetSocTemp(&SOC_temperatureF);
            Tmp451GetPcbTemp(&PCB_temperatureF);
        }
        if (R_SUCCEEDED(tcCheck)) {
            tcGetSkinTemperatureMilliC(&skin_temperaturemiliC);
        }
        
        // RAM Memory Used
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
        
        // Fan
        if (R_SUCCEEDED(pwmCheck)) {
            double temp = 0;
            if (R_SUCCEEDED(pwmChannelSessionGetDutyCycle(&g_ICon, &temp))) {
                temp *= 10;
                temp = trunc(temp);
                temp /= 10;
                Rotation_Duty = 100.0 - temp;
                if (Rotation_Duty <= 0) {
                    Rotation_Duty = 0.0000001;
                }
            }
        }
        
        // GPU Load
        if (R_SUCCEEDED(nvCheck) && GPULoadPerFrame) {
            nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &GPU_Load_u);
        }
        
        // FPS - with proper null checks
        if (GameRunning) {
            if (NxFps && SharedMemoryUsed) {
                FPS = NxFps->FPS;
                const size_t element_count = sizeof(NxFps->FPSticks) / sizeof(NxFps->FPSticks[0]);
                FPSavg_old = static_cast<float>(systemtickfrequency) / 
                            (std::accumulate(&NxFps->FPSticks[0], &NxFps->FPSticks[element_count], 0.0f) / element_count);
                
                const float FPS_in = static_cast<float>(FPS);
                if (FPSavg_old >= (FPS_in - 0.25f) && FPSavg_old <= (FPS_in + 0.25f)) {
                    FPSavg = FPS_in;
                } else {
                    FPSavg = FPSavg_old;
                }
                
                lastFrameNumber = NxFps->frameNumber;
                
                if (FPSavg > FPSmax) FPSmax = FPSavg;
                if (FPSavg < FPSmin) FPSmin = FPSavg;
            }
        } else {
            FPSavg = 254;
            FPSmin = 254;
            FPSmax = 0;
        }
        
        mutexUnlock(&mutex_Misc);
        
    } while (!leventWait(&threadexit, timeout_ns));
    
    // Cleanup voltage reading if initialized
    if (canReadVoltages) {
        rgltrExit();
    }
}

void Misc2(void*) {
    u32 dummy = 0;
    do {
        //DSP
        if (R_SUCCEEDED(audsnoopCheck)) audsnoopGetDspUsage(&DSP_Load_u);

        //Multimedia clock rates
        if (R_SUCCEEDED(nvdecCheck)) mmuRequestGet(&nvdecRequest, &NVDEC_Hz);
        if (R_SUCCEEDED(nvencCheck)) mmuRequestGet(&nvencRequest, &NVENC_Hz);
        if (R_SUCCEEDED(nvjpgCheck)) mmuRequestGet(&nvjpgRequest, &NVJPG_Hz);

        if (R_SUCCEEDED(nifmCheck)) {
            //u32 dummy = 0;
            Nifm_internet_rc = nifmGetInternetConnectionStatus(&NifmConnectionType, &dummy, &NifmConnectionStatus);
            if (!Nifm_internet_rc && (NifmConnectionType == NifmInternetConnectionType_WiFi))
                Nifm_profile_rc = nifmGetCurrentNetworkProfile((NifmNetworkProfileData*)&Nifm_profile);
        }
    } while (!leventWait(&threadexit, 100'000'000));
}

void Misc3(void*) {
    const bool isUsingEOSorHOC = usingEOS() || usingHOC();
    
    // Initialize voltage reading if needed
    bool canReadVoltages = false;
    if (!isUsingEOSorHOC && realVoltsPolling) {
        canReadVoltages = R_SUCCEEDED(rgltrInitialize());
        if (!canReadVoltages) {
            realVoltsPolling = false;
        }
    }
    
    do {
        mutexLock(&mutex_Misc);
        
        // Get sys-clk data (sys-clk-hoc: sys:clk service)
        if (R_SUCCEEDED(sysclkCheck)) {
            SysClkContext sysclkCTX;
            if (R_SUCCEEDED(sysclkIpcGetCurrentContext(&sysclkCTX))) {
                ramLoad[SysClkRamLoad_All] = sysclkCTX.ramLoad[SysClkRamLoad_All];
                ramLoad[SysClkRamLoad_Cpu] = sysclkCTX.ramLoad[SysClkRamLoad_Cpu];
                
                // Voltages from sys-clk (EOS/HOC path)
                if (isUsingEOSorHOC && realVoltsPolling) {
                    realCPU_mV = sysclkCTX.realVolts[0]; 
                    realGPU_mV = sysclkCTX.realVolts[1]; 
                    realRAM_mV = sysclkCTX.realVolts[2]; 
                    realSOC_mV = sysclkCTX.realVolts[3];
                }
                // HOC die temps via sys-clk-hoc IPC (packed into perfConfId/realProfile/reserved[0])
                if (usingHOC()) {
                    componentCPU_mC = sysclkCTX.perfConfId;
                    componentGPU_mC = (uint32_t)sysclkCTX.realProfile;
                    componentRAM_mC = (uint32_t)sysclkCTX.reserved[0];
                }
            }
        }
        // Get hoc-clk data (Horizon OC native hoc:clk IPC)
        else if (R_SUCCEEDED(hocclkCheck)) {
            HocClkContext hocclkCTX;
            if (R_SUCCEEDED(hocclkIpcGetCurrentContext(&hocclkCTX))) {
                ramLoad[SysClkRamLoad_All] = hocclkCTX.stable.partLoad[HocClkPartLoad_EMC];
                ramLoad[SysClkRamLoad_Cpu] = hocclkCTX.stable.partLoad[HocClkPartLoad_EMCCpu];
                if (realVoltsPolling) {
                    realCPU_mV = hocclkCTX.stable.voltages[HocClkVoltage_CPU];
                    realGPU_mV = hocclkCTX.stable.voltages[HocClkVoltage_GPU];
                    // voltages[] are in µV. Convert to mV then pack:
                    // vdd2_mV * 100000 + vddq_mV * 10  (display unpacks same way as sys-clk-hoc)
                    // Erista: EMCVDDQ == EMCVDD2 so skip VDDQ (matches sys-clk-hoc ipc_service.cpp)
                    {
                        const float    hoc_vdd2_mV = (float)hocclkCTX.stable.voltages[HocClkVoltage_EMCVDD2] / 1000.0f;
                        const uint32_t hoc_vddq_mV = hocclkCTX.stable.voltages[HocClkVoltage_EMCVDDQ] / 1000u;
                        realRAM_mV = (uint32_t)(hoc_vdd2_mV * 100000.0f);
                        if (hoc_vddq_mV < 1000u) {  // Mariko: VDDQ is separate lower rail
                            realRAM_mV += hoc_vddq_mV * 10u;
                        }
                    }
                    realSOC_mV = hocclkCTX.stable.voltages[HocClkVoltage_SOC];
                }
                // HOC die temps directly from hoc-clk IPC (per-die SOCTHERM values)
                componentCPU_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_CPU];
                componentGPU_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_GPU];
                componentRAM_mC = (uint32_t)hocclkCTX.stable.temps[HocClkThermalSensor_MEM];
            }
        }

        // Non-HOC: read CPU/GPU/MEM die temps directly from SOCTHERM hardware
        if (!usingHOC()) Soctherm::Read();
        
        // Read voltages directly if not using EOS
        if (canReadVoltages) {
            RgltrSession session;
            u32 vdd2_raw = 0, vddq_raw = 0;
            
            // CPU voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, domains[0]))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realCPU_mV))) {
                    realCPU_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // GPU voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, domains[1]))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realGPU_mV))) {
                    realGPU_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // VDD2 (DRAM) - different domains for Mariko vs Erista
            if (isMariko) {
                if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77812_Dram))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vdd2_raw))) {
                        vdd2_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            } else {
                // Erista uses Max77620_Sd1 for VDD2
                if (R_SUCCEEDED(rgltrOpenSession(&session, PcvPowerDomainId_Max77620_Sd1))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vdd2_raw))) {
                        vdd2_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            }
            
            // SOC voltage
            if (R_SUCCEEDED(rgltrOpenSession(&session, domains[3]))) {
                if (R_FAILED(rgltrGetVoltage(&session, &realSOC_mV))) {
                    realSOC_mV = 0;
                }
                rgltrCloseSession(&session);
            }
            
            // VDDQ
            if (isMariko) {
                if (R_SUCCEEDED(rgltrOpenSession(&session, domains[4]))) {
                    if (R_FAILED(rgltrGetVoltage(&session, &vddq_raw))) {
                        vddq_raw = 0;
                    }
                    rgltrCloseSession(&session);
                }
            }
            
            // Pack VDD2 and VDDQ into realRAM_mV in sys-clk format
            const u32 vdd2_mV = vdd2_raw / 1000;  // µV to mV
            const u32 vddq_mV = vddq_raw / 1000;  // µV to mV
            realRAM_mV = vdd2_mV * 100000 + vddq_mV * 10;
        }
        
        // Temperatures
        if (R_SUCCEEDED(i2cCheck)) {
            Tmp451GetSocTemp(&SOC_temperatureF);
            Tmp451GetPcbTemp(&PCB_temperatureF);
        }
        if (R_SUCCEEDED(tcCheck)) {
            tcGetSkinTemperatureMilliC(&skin_temperaturemiliC);
        }
        
        // Fan
        if (R_SUCCEEDED(pwmCheck)) {
            double temp = 0;
            if (R_SUCCEEDED(pwmChannelSessionGetDutyCycle(&g_ICon, &temp))) {
                temp *= 10;
                temp = trunc(temp);
                temp /= 10;
                Rotation_Duty = 100.0 - temp;
                if (Rotation_Duty <= 0) {
                    Rotation_Duty = 0.0000001;
                }
            }
        }
        
        // GPU Load
        if (R_SUCCEEDED(nvCheck)) {
            nvIoctl(fd, NVGPU_GPU_IOCTL_PMU_GET_GPU_LOAD, &GPU_Load_u);
        }
        
        mutexUnlock(&mutex_Misc);
        
    } while (!leventWait(&threadexit, 1'000'000'000)); // 1 second timeout
    
    // Cleanup voltage reading if initialized
    if (canReadVoltages) {
        rgltrExit();
    }
}

//Check each core for idled ticks in intervals, they cannot read info about other core than they are assigned
//In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
//This is because making each loop also takes time, which is not considered because this will take also additional time

//Check each core for idled ticks in intervals, they cannot read info about other core than they are assigned
//In case of getting more than systemtickfrequency in idle, make it equal to systemtickfrequency to get 0% as output and nothing less
//This is because making each loop also takes time, which is not considered because this will take also additional time
//void CheckCore0(void*) {
//    uint64_t timeout_ns = 1'000'000'000 / TeslaFPS;
//    while(true) {
//        uint64_t idletick_a0 = 0;
//        uint64_t idletick_b0 = 0;
//        svcGetInfo(&idletick_b0, InfoType_IdleTickCount, INVALID_HANDLE, 0);
//        if (leventWait(&threadexit, timeout_ns))
//            return;
//        svcGetInfo(&idletick_a0, InfoType_IdleTickCount, INVALID_HANDLE, 0);
//        idletick0 = idletick_a0 - idletick_b0;
//    }
//}
//
//void CheckCore1(void*) {
//    uint64_t timeout_ns = 1'000'000'000 / TeslaFPS;
//    while(true) {
//        uint64_t idletick_a1 = 0;
//        uint64_t idletick_b1 = 0;
//        svcGetInfo(&idletick_b1, InfoType_IdleTickCount, INVALID_HANDLE, 1);
//        if (leventWait(&threadexit, timeout_ns))
//            return;
//        svcGetInfo(&idletick_a1, InfoType_IdleTickCount, INVALID_HANDLE, 1);
//        idletick1 = idletick_a1 - idletick_b1;
//    }
//}
//
//void CheckCore2(void*) {
//    uint64_t timeout_ns = 1'000'000'000 / TeslaFPS;
//    while(true) {
//        uint64_t idletick_a2 = 0;
//        uint64_t idletick_b2 = 0;
//        svcGetInfo(&idletick_b2, InfoType_IdleTickCount, INVALID_HANDLE, 2);
//        if (leventWait(&threadexit, timeout_ns))
//            return;
//        svcGetInfo(&idletick_a2, InfoType_IdleTickCount, INVALID_HANDLE, 2);
//        idletick2 = idletick_a2 - idletick_b2;
//    }
//}
//
//void CheckCore3(void*) {
//    uint64_t timeout_ns = 1'000'000'000 / TeslaFPS;
//    while(true) {
//        uint64_t idletick_a3 = 0;
//        uint64_t idletick_b3 = 0;
//        svcGetInfo(&idletick_b3, InfoType_IdleTickCount, INVALID_HANDLE, 3);
//        if (leventWait(&threadexit, timeout_ns))
//            return;
//        svcGetInfo(&idletick_a3, InfoType_IdleTickCount, INVALID_HANDLE, 3);
//        idletick3 = idletick_a3 - idletick_b3;
//    }
//}

void CheckCore(void* idletick_ptr) {
    const uint64_t timeout_ns = 1'000'000'000ULL / TeslaFPS;
    std::atomic<uint64_t>* idletick = (std::atomic<uint64_t>*)idletick_ptr;
    while (true) {
        uint64_t idletick_a;
        uint64_t idletick_b;
        svcGetInfo(&idletick_b, InfoType_IdleTickCount, INVALID_HANDLE, -1);
        Result rc_break = leventWait(&threadexit, timeout_ns);
        svcGetInfo(&idletick_a, InfoType_IdleTickCount, INVALID_HANDLE, -1);
        if (rc_break) return;
        idletick->store(idletick_a - idletick_b, std::memory_order_release);
    }
}


//Start reading all stats
void StartThreads() {
    // Clear the thread exit event for new threads
    leventClear(&threadexit);

    threadCreate(&t0, CheckCore, &idletick0, NULL, 0x1000, 0x10, 0);
    threadCreate(&t1, CheckCore, &idletick1, NULL, 0x1000, 0x10, 1);
    threadCreate(&t2, CheckCore, &idletick2, NULL, 0x1000, 0x10, 2);
    threadCreate(&t3, CheckCore, &idletick3, NULL, 0x1000, 0x10, 3);

    //threadCreate(&t0, CheckCore, &coreIds[0], NULL, 0x1000, 0x10, 0);
    //threadCreate(&t1, CheckCore, &coreIds[1], NULL, 0x1000, 0x10, 1);
    //threadCreate(&t2, CheckCore, &coreIds[2], NULL, 0x1000, 0x10, 2);
    //threadCreate(&t3, CheckCore, &coreIds[3], NULL, 0x1000, 0x10, 3);
    threadCreate(&t4, Misc, NULL, NULL, 0x4000, 0x3F, -2);
    threadCreate(&t5, gpuLoadThread, NULL, NULL, 0x1000, 0x3F, -2);
    threadCreate(&t7, BatteryChecker, NULL, NULL, 0x4000, 0x3F, -2);

    threadStart(&t0);
    threadStart(&t1);
    threadStart(&t2);
    threadStart(&t3);
    threadStart(&t4);
    threadStart(&t5);
    threadStart(&t7);

    if (SaltySD) {
        //Assign NX-FPS to default core
        threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
        threadStart(&t6);
    }
}

//End reading all stats
void CloseThreads() {
    leventSignal(&threadexit);
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
}

//Separate functions dedicated to "FPS Counter" mode
void FPSCounter(void*) {
    const uint64_t timeout_ns = 1'000'000'000 / TeslaFPS;
    do {
        if (GameRunning) {
            if (SharedMemoryUsed && NxFps) {
                FPS = (NxFps -> FPS);
                const size_t element_count = sizeof(NxFps -> FPSticks) / sizeof(NxFps -> FPSticks[0]);
                FPSavg_old = (float)systemtickfrequency / (std::accumulate<uint32_t*, float>(&NxFps->FPSticks[0], &NxFps->FPSticks[element_count], 0) / element_count);
                const float FPS_in = (float)FPS;
                if (FPSavg_old >= (FPS_in-0.25) && FPSavg_old <= (FPS_in+0.25)) 
                    FPSavg = FPS_in;
                else FPSavg = FPSavg_old;
                lastFrameNumber = NxFps -> frameNumber;
            }
        }
        else FPSavg = 254;
    } while (!leventWait(&threadexit, timeout_ns));
}

void StartFPSCounterThread() {
    leventClear(&threadexit);

    threadCreate(&t6, CheckIfGameRunning, NULL, NULL, 0x1000, 0x38, -2);
    threadStart(&t6);

    threadCreate(&t4, FPSCounter, NULL, NULL, 0x1000, 0x3F, 3);
    threadStart(&t4);
}

void EndFPSCounterThread() {
    leventSignal(&threadexit);
    threadWaitForExit(&t6);
    threadWaitForExit(&t4);
    threadClose(&t6);
    threadClose(&t4);
}

void StartInfoThread() {    
    // Clear the thread exit event for new threads
    leventClear(&threadexit);
    
    threadCreate(&t0, CheckCore, &idletick0, NULL, 0x1000, 0x10, 0);
    threadCreate(&t1, CheckCore, &idletick1, NULL, 0x1000, 0x10, 1);
    threadCreate(&t2, CheckCore, &idletick2, NULL, 0x1000, 0x10, 2);
    threadCreate(&t3, CheckCore, &idletick3, NULL, 0x1000, 0x10, 3);

    //threadCreate(&t1, CheckCore, &coreIds[0], NULL, 0x1000, 0x10, 0);
    //threadCreate(&t2, CheckCore, &coreIds[1], NULL, 0x1000, 0x10, 1);
    //threadCreate(&t3, CheckCore, &coreIds[2], NULL, 0x1000, 0x10, 2);
    //threadCreate(&t4, CheckCore, &coreIds[3], NULL, 0x1000, 0x10, 3);
    threadCreate(&t7, Misc3, NULL, NULL, 0x1000, 0x3F, -2);

    threadStart(&t0);
    threadStart(&t1);
    threadStart(&t2);
    threadStart(&t3);
    threadStart(&t7);
}

void EndInfoThread() {
    // Signal the thread exit event
    leventSignal(&threadexit);
    
    // Wait for all threads to exit
    threadWaitForExit(&t0);
    threadWaitForExit(&t1);
    threadWaitForExit(&t2);
    threadWaitForExit(&t3);
    threadWaitForExit(&t7);
    
    // Close thread handles
    threadClose(&t0);
    threadClose(&t1);
    threadClose(&t2);
    threadClose(&t3);
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

    static std::string button;
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

//uint64_t comboBitmask = 0;
//
//constexpr uint64_t MapButtons(const std::string& buttonCombo) {
//    static std::map<std::string, uint64_t> buttonMap = {
//        {"A", HidNpadButton_A},
//        {"B", HidNpadButton_B},
//        {"X", HidNpadButton_X},
//        {"Y", HidNpadButton_Y},
//        {"L", HidNpadButton_L},
//        {"R", HidNpadButton_R},
//        {"ZL", HidNpadButton_ZL},
//        {"ZR", HidNpadButton_ZR},
//        {"PLUS", HidNpadButton_Plus},
//        {"MINUS", HidNpadButton_Minus},
//        {"DUP", HidNpadButton_Up},
//        {"DDOWN", HidNpadButton_Down},
//        {"DLEFT", HidNpadButton_Left},
//        {"DRIGHT", HidNpadButton_Right},
//        {"SL", HidNpadButton_AnySL},
//        {"SR", HidNpadButton_AnySR},
//        {"LSTICK", HidNpadButton_StickL},
//        {"RSTICK", HidNpadButton_StickR},
//        {"LS", HidNpadButton_StickL},
//        {"RS", HidNpadButton_StickR},
//        {"UP", HidNpadButton_AnyUp},
//        {"DOWN", HidNpadButton_AnyDown},
//        {"LEFT", HidNpadButton_AnyLeft},
//        {"RIGHT", HidNpadButton_AnyRight}
//    };
//
//    
//    std::string comboCopy = buttonCombo;  // Make a copy of buttonCombo
//
//    static const std::string delimiter = "+";
//    size_t pos = 0;
//    static std::string button;
//    size_t max_delimiters = 4;
//    while ((pos = comboCopy.find(delimiter)) != std::string::npos) {
//        button = comboCopy.substr(0, pos);
//        if (buttonMap.find(button) != buttonMap.end()) {
//            comboBitmask |= buttonMap[button];
//        }
//        comboCopy.erase(0, pos + delimiter.length());
//        if (!--max_delimiters) {
//            return comboBitmask;
//        }
//    }
//    if (buttonMap.find(comboCopy) != buttonMap.end()) {
//        comboBitmask |= buttonMap[comboCopy];
//    }
//    return comboBitmask;
//}

ALWAYS_INLINE bool isKeyComboPressed(uint64_t keysHeld, uint64_t keysDown) {
    // Check if any of the combo buttons are pressed down this frame
    // while the rest of the combo buttons are being held
    
    const uint64_t comboButtonsDown = keysDown & tsl::cfg::launchCombo;
    const uint64_t comboButtonsHeld = keysHeld & tsl::cfg::launchCombo;
    
    // If any combo buttons are pressed down this frame
    if (comboButtonsDown != 0) {
        // Check if the remaining combo buttons are being held
        // (the full combo should be active when combining held + down)
        const uint64_t totalComboActive = comboButtonsHeld | comboButtonsDown;
        
        if (totalComboActive == tsl::cfg::launchCombo) {
            fixHiding = true; // for fixing hiding when returning
            //triggerRumbleDoubleClick.store(true, std::memory_order_release);
            //triggerExitSound.store(true, std::memory_order_release);
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
    uint64_t requiredKeys = tsl::cfg::launchCombo;
    bool hasKeyDown = false; // Tracks if at least one key is in keysDown

    static uint64_t keyBit;
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
    // Check and create directory if needed
    //struct stat st;
    //if (stat(directoryPath, &st) != 0) {
    //    mkdir(directoryPath, 0777);
    //}
    ult::createSingleDirectory(directoryPath);

    // Load main config INI once
    auto configData = ult::getParsedDataFromIniFile(configIniPath);
    auto statusIt = configData.find("status-monitor");
    
    if (statusIt != configData.end()) {
        const auto& statusSection = statusIt->second;
        std::string key;

        // Process all settings with direct lookups
        auto batteryFilterIt = statusSection.find("battery_avg_iir_filter");
        if (batteryFilterIt != statusSection.end()) {
            key = batteryFilterIt->second;
            convertToUpper(key);
            batteryFiltered = (key == "TRUE");
        }
        
        auto refreshRateIt = statusSection.find("battery_time_left_refreshrate");
        if (refreshRateIt != statusSection.end()) {
            batteryTimeLeftRefreshRate = std::clamp(atol(refreshRateIt->second.c_str()), 1L, 60L);
        }
        
        auto gpuLoadIt = statusSection.find("average_gpu_load");
        if (gpuLoadIt != statusSection.end()) {
            key = gpuLoadIt->second;
            convertToUpper(key);
            GPULoadPerFrame = (key != "TRUE");
        }

        auto fpsAvgIt = statusSection.find("use_old_fps_average");
        if (fpsAvgIt != statusSection.end()) {
            key = fpsAvgIt->second;
            convertToUpper(key);
            useOldFPSavg = (key == "TRUE");
        }
    }

    // Handle external combo - load each file once
    const struct { const char* path; const char* section; } externalConfigs[] = {
        {ultrahandConfigIniPath, "ultrahand"},
        {teslaConfigIniPath, "tesla"}
    };
    
    for (const auto& config : externalConfigs) {
        auto extConfigData = ult::getParsedDataFromIniFile(config.path);
        auto sectionIt = extConfigData.find(config.section);
        
        if (sectionIt != extConfigData.end()) {
            auto keyComboIt = sectionIt->second.find("key_combo");
            if (keyComboIt != sectionIt->second.end() && !keyComboIt->second.empty()) {
                keyCombo = keyComboIt->second;
                removeSpaces(keyCombo);
                convertToUpper(keyCombo);
                break;
            }
        }
    }
    
    //comboBitmask = MapButtons(keyCombo);
}

ALWAYS_INLINE bool isValidRGBA4Color(const std::string& hexColor) {
    const char* data = hexColor.data();
    const size_t size = hexColor.size();
    
    static unsigned char c;
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
    bool showRDSD;
    bool useDynamicColors;
    bool disableScreenshots;
    uint16_t separatorColor;
    uint16_t catColor1;
    uint16_t catColor2;
    uint16_t textColor;
};

struct MiniSettings {
    uint8_t refreshRate;
    bool realFrequencies;
    bool realVolts;
    bool showLabels;
    bool showFullCPU;
    bool showFullCPUMaxCore012; // true = brackets show [max(c0,c1,c2) c3] instead of [c0 c1 c2 c3]
    bool showStackedFullCPU; // true = brackets top, freq bottom (stacked); false = [brackets]@freq inline
    bool showFullResolution;
    bool showFanPercentage;
    bool showSOCVoltage;
    bool showStackedFanSOC;  // true = fan row1, volt row2 (stacked); false = fan+volt inline
    bool showStackedVDDQ;   // true = VDD2 row1, VDDQ row2 stacked (Mariko only); false = VDD2+VDDQ inline
    bool showCPUTemp;           // show CPU die temp below CPU volt
    bool showGPUTemp;           // show GPU die temp below GPU volt
    bool showRAMTemp;           // show RAM die temp (next to VDDQ)
    bool showStackedCPUTemp; // true = split row below volt (stacked); false = CPU temp inline
    bool showStackedGPUTemp; // true = split row below volt (stacked); false = GPU temp inline
    bool showStackedRAMTemp; // true = stacked separate row; false = RAM temp inline
    bool voltageAtEndCPU;       // true = volt at end (below/after temp; swap positions)
    bool voltageAtEndGPU;       // true = volt at end
    bool voltageAtEndRAM;       // true = volt at end
    bool voltageAtEndTMP;       // true = SOC volt at top, fan at bottom (swapped)
    bool useDynamicColors;
    bool showVDDQ;
    bool showVDD2;
    bool decimalVDD2;
    bool showDTC;
    bool useDTCSymbol;
    bool useIntegerFPS;
    std::string dtcFormat;   // derived at load time: format1 + DIVIDER + format2 (or just format1 when format2 is "None")
    std::string dtcFormat1;  // top half (always present)
    std::string dtcFormat2;  // bottom half (== ult::OPTION_SYMBOL means "None" — no bottom half, no divider)
    size_t handheldFontSize;
    size_t dockedFontSize;
    size_t spacing;
    uint16_t backgroundColor;
    uint16_t focusBackgroundColor;
    uint16_t separatorColor;
    uint16_t catColor;
    uint16_t textColor;
    std::string show;
    bool showRAMLoad;
    bool showRAMLoadCPUGPU;
    bool showStackedRAMLoad; // true = split rows (stacked); false = [cpu% gpu%]total%@freq inline
    bool showComponentTemps;    // dual-row TMP: show CPU/GPU/RAM die temps row
    bool showSocPcbSkinTemps;   // show SOC/PCB/Skin temps row (default: true)
    bool invertBatteryDisplay;
    bool showStackedBAT;   // true = draw on top, pct on bottom stacked; false = draw+pct inline
    bool showStackedDTC;   // true = split DTC at DIVIDER_SYMBOL into 2 stacked rows; false = single line
    bool disableScreenshots;
    //int setPos;
    int frameOffsetX;
    int frameOffsetY;
    size_t framePadding;
};

struct MicroSettings {
    uint8_t refreshRate;
    bool realFrequencies;
    bool realVolts; 
    bool showFullCPU;
    bool showFullCPUMaxCore012; // true = brackets show [max(c0,c1,c2) c3] instead of [c0 c1 c2 c3]
    bool showStackedFullCPU; // true = brackets top, freq bottom (stacked); false = [brackets]@freq inline
    bool showFullResolution;
    bool showSOCVoltage;
    bool showStackedFanSOC;  // true = fan row1, volt row2 (stacked); false = fan+volt inline
    bool showStackedVDDQ;   // true = VDD2 row1, VDDQ row2 stacked (Mariko only); false = VDD2+VDDQ inline
    bool showCPUTemp;           // show CPU die temp below CPU volt
    bool showGPUTemp;           // show GPU die temp below GPU volt
    bool showRAMTemp;           // show RAM die temp (next to VDDQ)
    bool showStackedCPUTemp; // true = split row below volt (stacked); false = CPU temp inline
    bool showStackedGPUTemp; // true = split row below volt (stacked); false = GPU temp inline
    bool showStackedRAMTemp; // true = stacked separate row; false = RAM temp inline
    bool voltageAtEndCPU;       // true = volt at end (below/after temp; swap positions)
    bool voltageAtEndGPU;       // true = volt at end
    bool voltageAtEndRAM;       // true = volt at end
    bool voltageAtEndTMP;       // true = SOC volt at top, fan at bottom (swapped)
    bool showFanPercentage;
    bool useDynamicColors;
    bool showVDDQ;
    bool showVDD2;
    bool decimalVDD2;
    bool showDTC;
    bool useDTCSymbol;
    bool useIntegerFPS;
    std::string dtcFormat;   // derived at load time: format1 + DIVIDER + format2 (or just format1 when format2 is "None")
    std::string dtcFormat1;  // top half (always present)
    std::string dtcFormat2;  // bottom half (== ult::OPTION_SYMBOL means "None" — no bottom half, no divider)
    bool invertBatteryDisplay;
    bool showStackedBAT;   // true = draw on top, pct on bottom stacked; false = draw+pct inline
    bool showStackedDTC;   // true = split DTC at DIVIDER_SYMBOL into 2 stacked rows; false = single line
    size_t handheldFontSize;
    size_t dockedFontSize;
    uint8_t alignTo;
    uint16_t backgroundColor;
    uint16_t separatorColor;
    uint16_t catColor;
    uint16_t textColor;
    std::string show;
    bool showRAMLoad;
    bool showRAMLoadCPUGPU;      // show CPU/GPU load breakdown
    bool showStackedRAMLoad; // true = split rows (stacked); false = [cpu% gpu%]total%@freq inline
    bool showComponentTemps;    // show CPU/GPU/RAM die temps (default: false)
    bool showSocPcbSkinTemps;   // show SOC/PCB/Skin temps (default: true)
    bool showStackedTemps;   // true = temp groups on separate rows (stacked); false = one line with divider
    bool setPosBottom;
    bool disableScreenshots;
    uint8_t horizontalPadding;  // 0-10 px, left/right gap from screen edge to text; default 8
    uint8_t verticalPadding;    // 0-8 px, gap above/below text within bar; default 2
    uint8_t labelPadding;       // 2-8 px, gap between element label and its value; 0 = auto (font-size-based)
};

struct FpsCounterSettings {
    uint8_t refreshRate;
    size_t handheldFontSize;
    size_t dockedFontSize;
    uint16_t backgroundColor;
    uint16_t focusBackgroundColor;
    uint16_t textColor;
    //int setPos;
    bool useIntegerFPS;
    bool disableScreenshots;
    int frameOffsetX;
    int frameOffsetY;
    size_t framePadding;
};

struct FpsGraphSettings {
    bool showInfo;
    uint8_t refreshRate;
    uint16_t backgroundColor;
    uint16_t focusBackgroundColor;
    uint16_t fpsColor;
    uint16_t mainLineColor;
    uint16_t roundedLineColor;
    uint16_t perfectLineColor;
    uint16_t dashedLineColor;
    uint16_t borderColor;
    uint16_t maxFPSTextColor;
    uint16_t minFPSTextColor;
    uint16_t textColor;
    uint16_t catColor;
    //int setPos;
    bool useDynamicColors;
    bool useIntegerFPS;
    bool disableScreenshots;
    int frameOffsetX;
    int frameOffsetY;
    size_t framePadding;
};

struct ResolutionSettings {
    uint8_t refreshRate;
    uint16_t backgroundColor;
    uint16_t focusBackgroundColor;
    uint16_t catColor;
    //uint16_t catColor2;
    uint16_t textColor;
    //int setPos;
    bool disableScreenshots;

    int frameOffsetX;
    int frameOffsetY;
    size_t framePadding;
};

ALWAYS_INLINE void GetConfigSettings(MiniSettings* settings) {
    // Initialize defaults
    settings->realFrequencies = true;
    settings->realVolts = true;
    settings->showFullCPU = false;
    settings->showFullCPUMaxCore012 = false;
    settings->showStackedFullCPU = false;
    settings->showFullResolution = true;
    settings->showFanPercentage = true;
    settings->useDynamicColors = true;
    settings->showFullCPU = false;
    settings->showSOCVoltage = true;
    settings->showStackedFanSOC = true;
    settings->showStackedVDDQ = true;
    settings->showCPUTemp = false;
    settings->showGPUTemp = false;
    settings->showRAMTemp = false;
    settings->showStackedCPUTemp = true;
    settings->showStackedGPUTemp = true;
    settings->showStackedRAMTemp = true;
    settings->voltageAtEndCPU = false;
    settings->voltageAtEndGPU = false;
    settings->voltageAtEndRAM = false;
    settings->voltageAtEndTMP = false;
    settings->showVDDQ = true;
    settings->showVDD2 = true;
    settings->decimalVDD2 = false;
    settings->showDTC = true;
    settings->useDTCSymbol = true;
    settings->useIntegerFPS = false;
    settings->dtcFormat1 = "%a, %b %d";
    settings->dtcFormat2 = "%I:%M %p";
    settings->dtcFormat  = settings->dtcFormat1 + ult::DIVIDER_SYMBOL + settings->dtcFormat2;
    settings->handheldFontSize = 15;
    settings->dockedFontSize = 15;
    settings->spacing = 8;
    convertStrToRGBA4444("#0009", &(settings->backgroundColor));
    convertStrToRGBA4444("#000F", &(settings->focusBackgroundColor));
    convertStrToRGBA4444("#2DFF", &(settings->separatorColor));
    convertStrToRGBA4444("#2DFF", &(settings->catColor));
    convertStrToRGBA4444("#FFFF", &(settings->textColor));
    settings->show = "DTC+BAT+CPU+GPU+RAM+TMP+FPS+RES";
    settings->showLabels = true;
    settings->showRAMLoad = true;
    settings->showRAMLoadCPUGPU = false;
    settings->showStackedRAMLoad = false;
    settings->showComponentTemps = true;
    settings->showSocPcbSkinTemps = true;
    settings->invertBatteryDisplay = true;
    settings->showStackedBAT = false;
    settings->showStackedDTC = false;
    settings->refreshRate = 3;
    settings->disableScreenshots = false;
    //settings->setPos = 0;
    settings->frameOffsetX = 6;
    settings->frameOffsetY = 6;
    settings->framePadding = 6;

    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    
    std::string key;
    uint16_t temp;

    const auto& section = sectionIt->second;

    // Process refresh_rate
    auto it = section.find("refresh_rate");
    if (it != section.end()) {
        settings->refreshRate = std::clamp(atol(it->second.c_str()), 1L, 60L);
    }
    
    // Process boolean flags
    it = section.find("real_freqs");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->realFrequencies = (key == "TRUE");
    }
    
    it = section.find("real_volts");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->realVolts = (key == "TRUE");
    }
    
    // Process font sizes with shared bounds
    static constexpr long minFontSize = 8;
    static constexpr long maxFontSize = 22;
    
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
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }
    it = section.find("focus_background_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->focusBackgroundColor = temp;
    }
    
    it = section.find("separator_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->separatorColor = temp;
    }

    it = section.find("cat_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor = temp;
    }
    
    it = section.find("text_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
    
    it = section.find("show_labels");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showLabels = !(key == "FALSE");
    }

    // Process RAM load flag
    it = section.find("show_full_cpu");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullCPU = !(key == "FALSE");
    }

    it = section.find("show_full_cpu_max_core_012");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullCPUMaxCore012 = (key == "TRUE");
    }

    it = section.find("show_side_by_side_full_cpu");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedFullCPU = (key == "FALSE");
    }

    it = section.find("show_full_res");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullResolution = !(key == "FALSE");
    }

    it = section.find("show_soc_voltage");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showSOCVoltage = !(key == "FALSE");
    }

    it = section.find("show_side_by_side_fan_soc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedFanSOC = (key == "FALSE");
    }

    it = section.find("show_side_by_side_vddq");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedVDDQ = (key == "FALSE");
    }

    it = section.find("show_cpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showCPUTemp = (key == "TRUE");
    }

    it = section.find("show_gpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showGPUTemp = (key == "TRUE");
    }

    it = section.find("show_ram_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMTemp = (key == "TRUE");
    }

    it = section.find("show_side_by_side_cpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedCPUTemp = !(key == "TRUE");
    }

    it = section.find("show_side_by_side_gpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedGPUTemp = !(key == "TRUE");
    }

    it = section.find("show_side_by_side_ram_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedRAMTemp = !(key == "TRUE");
    }

    it = section.find("voltage_at_end_cpu");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndCPU = (key == "TRUE"); }
    it = section.find("voltage_at_end_gpu");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndGPU = (key == "TRUE"); }
    it = section.find("voltage_at_end_ram");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndRAM = (key == "TRUE"); }
    it = section.find("voltage_at_end_tmp");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndTMP = (key == "TRUE"); }

    it = section.find("use_dynamic_colors");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDynamicColors = (key == "TRUE");
    }

    it = section.find("show_vddq");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showVDDQ = !(key == "FALSE");
    }

    it = section.find("show_vdd2");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showVDD2 = !(key == "FALSE");
    }

    it = section.find("decimal_vdd2");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->decimalVDD2 = !(key == "FALSE");
    }

    it = section.find("show_dtc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showDTC = !(key == "FALSE");
    }

    it = section.find("use_dtc_symbol");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDTCSymbol = !(key == "FALSE");
    }

    it = section.find("integer_fps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useIntegerFPS = (key != "FALSE");
    }

    // DTC format: prefer the new two-part keys (dtc_format_1, dtc_format_2). Fall back
    // to splitting the legacy dtc_format key at DIVIDER_SYMBOL so existing user configs
    // keep working. Final dtcFormat is rebuilt as format1 + DIVIDER + format2 (or just
    // format1 when format2 is OPTION_SYMBOL / "None"), so the rendering pipeline (which
    // already splits dtcFormat at DIVIDER_SYMBOL for stacking) doesn't need to change.
    {
        const auto it1 = section.find("dtc_format_1");
        const auto it2 = section.find("dtc_format_2");
        const auto itLegacy = section.find("dtc_format");
        if (it1 != section.end() || it2 != section.end()) {
            if (it1 != section.end()) settings->dtcFormat1 = it1->second;
            if (it2 != section.end()) settings->dtcFormat2 = it2->second;
        } else if (itLegacy != section.end()) {
            // Back-compat: split the legacy combined value at DIVIDER_SYMBOL.
            const std::string legacy = itLegacy->second;
            const size_t divPos = legacy.find(ult::DIVIDER_SYMBOL);
            if (divPos != std::string::npos) {
                settings->dtcFormat1 = legacy.substr(0, divPos);
                settings->dtcFormat2 = legacy.substr(divPos + ult::DIVIDER_SYMBOL.length());
            } else {
                settings->dtcFormat1 = legacy;
                settings->dtcFormat2 = ult::OPTION_SYMBOL;
            }
        }
        // Rebuild the runtime dtcFormat that the rendering code consumes.
        if (settings->dtcFormat2 == ult::OPTION_SYMBOL || settings->dtcFormat2.empty()) {
            settings->dtcFormat = settings->dtcFormat1;
        } else {
            settings->dtcFormat = settings->dtcFormat1 + ult::DIVIDER_SYMBOL + settings->dtcFormat2;
        }
    }

    // Process show string
    it = section.find("show");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->show = std::move(key);
    }
    
    // Process RAM load flag
    it = section.find("replace_MB_with_RAM_load");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMLoad = (key != "FALSE");
    }

    // Process CPU/GPU RAM load flag
    it = section.find("show_RAM_load_CPU_GPU");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMLoadCPUGPU = (key != "FALSE");
    }

    it = section.find("show_side_by_side_ram_load");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedRAMLoad = (key == "FALSE");
    }

    // Process CPU/GPU/RAM component temps flag (dual-row TMP only)
    it = section.find("show_component_temps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showComponentTemps = (key != "FALSE");
    }

    // Process SOC/PCB/Skin temps flag
    it = section.find("show_soc_pcb_skin_temps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showSocPcbSkinTemps = (key != "FALSE");
    }
    // Enforce mutual exclusivity: at least one must be on
    if (!settings->showComponentTemps && !settings->showSocPcbSkinTemps)
        settings->showSocPcbSkinTemps = true;

    // Invert the battery display value
    it = section.find("invert_battery_display");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->invertBatteryDisplay = (key != "FALSE");
    }

    it = section.find("show_side_by_side_bat");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedBAT = (key == "FALSE");
    }

    it = section.find("show_side_by_side_dtc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedDTC = (key == "FALSE");
    }

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }

    // Process alignment settings
    //it = section.find("layer_width_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos = 1;
    //    } else if (key == "RIGHT") {
    //        settings->setPos = 2;
    //    }
    //}
    //
    //it = section.find("layer_height_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos += 3;
    //    } else if (key == "BOTTOM") {
    //        settings->setPos += 6;
    //    }
    //}

    it = section.find("frame_offset_x");
    if (it != section.end()) {
        settings->frameOffsetX = atol(it->second.c_str());
    }

    it = section.find("frame_offset_y");
    if (it != section.end()) {
        settings->frameOffsetY = atol(it->second.c_str());
    }

    it = section.find("frame_padding");
    if (it != section.end()) {
        settings->framePadding = atol(it->second.c_str());
    }
}

ALWAYS_INLINE void GetConfigSettings(MicroSettings* settings) {
    // Initialize defaults
    settings->realFrequencies = true;
    settings->realVolts = true;
    settings->showFullCPU = false;
    settings->showFullCPUMaxCore012 = false;
    settings->showStackedFullCPU = false;
    settings->showFullResolution = false;
    settings->showSOCVoltage = true;
    settings->showStackedFanSOC = true;  // default: fan+volt stacked
    settings->showStackedVDDQ = true;   // default: VDD2+VDDQ stacked
    settings->showCPUTemp = false;
    settings->showGPUTemp = false;
    settings->showRAMTemp = false;
    settings->showStackedCPUTemp = true;
    settings->showStackedGPUTemp = true;
    settings->showStackedRAMTemp = true;
    settings->voltageAtEndCPU = false;
    settings->voltageAtEndGPU = false;
    settings->voltageAtEndRAM = false;
    settings->voltageAtEndTMP = false;
    settings->showFanPercentage = true;
    settings->useDynamicColors = true;
    settings->showVDDQ = true;
    settings->showVDD2 = true;
    settings->decimalVDD2 = false;
    settings->showDTC = true;
    settings->useDTCSymbol = true;
    settings->useIntegerFPS = false;
    settings->dtcFormat1 = "%H:%M";
    settings->dtcFormat2 = ult::OPTION_SYMBOL; // "None" — single-line, no second format
    settings->dtcFormat  = settings->dtcFormat1; // no divider, no format2 (matches dtcFormat2 == OPTION_SYMBOL)
    settings->invertBatteryDisplay = false;
    settings->showStackedBAT = false;
    settings->showStackedDTC = false;
    settings->handheldFontSize = 15;
    settings->dockedFontSize = 15;
    settings->alignTo = 1; // CENTER
    convertStrToRGBA4444("#0009", &(settings->backgroundColor));
    convertStrToRGBA4444("#2DFF", &(settings->separatorColor));
    convertStrToRGBA4444("#2DFF", &(settings->catColor));
    convertStrToRGBA4444("#FFFF", &(settings->textColor));
    settings->show = "FPS+CPU+GPU+RAM+TMP+BAT+DTC";
    settings->showRAMLoad = true;
    settings->showRAMLoadCPUGPU = false;
    settings->showStackedRAMLoad = false;
    settings->showComponentTemps = true;
    settings->showSocPcbSkinTemps = true;
    settings->showStackedTemps = true;
    settings->setPosBottom = false;
    settings->disableScreenshots = false;
    settings->refreshRate = 3;
    settings->horizontalPadding = 12;
    settings->verticalPadding   = 6;
    settings->labelPadding      = 0;  // 0 = auto (font-size-based)

    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    auto sectionIt = parsedData.find("micro");
    if (sectionIt == parsedData.end()) return;
    
    std::string key;
    uint16_t temp;

    const auto& section = sectionIt->second;

    // Process refresh_rate
    auto it = section.find("refresh_rate");
    if (it != section.end()) {
        settings->refreshRate = std::clamp(atol(it->second.c_str()), 1L, 60L);
    }
    
    // Process boolean flags
    it = section.find("real_freqs");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->realFrequencies = (key == "TRUE");
    }
    
    it = section.find("real_volts");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->realVolts = (key == "TRUE");
    }
    
    it = section.find("show_full_cpu");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullCPU = (key == "TRUE");
    }

    it = section.find("show_full_cpu_max_core_012");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullCPUMaxCore012 = (key == "TRUE");
    }

    it = section.find("show_side_by_side_full_cpu");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedFullCPU = (key == "FALSE");
    }
    
    it = section.find("show_full_res");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFullResolution = (key == "TRUE");
    }

    it = section.find("show_soc_voltage");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showSOCVoltage = !(key == "FALSE");
    }

    it = section.find("show_side_by_side_fan_soc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedFanSOC = (key == "FALSE");
    }

    it = section.find("show_side_by_side_vddq");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedVDDQ = (key == "FALSE");
    }

    it = section.find("show_cpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showCPUTemp = (key == "TRUE");
    }

    it = section.find("show_gpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showGPUTemp = (key == "TRUE");
    }

    it = section.find("show_ram_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMTemp = (key == "TRUE");
    }

    it = section.find("show_side_by_side_cpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedCPUTemp = !(key == "TRUE");
    }

    it = section.find("show_side_by_side_gpu_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedGPUTemp = !(key == "TRUE");
    }

    it = section.find("show_side_by_side_ram_temp");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedRAMTemp = !(key == "TRUE");
    }

    it = section.find("voltage_at_end_cpu");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndCPU = (key == "TRUE"); }
    it = section.find("voltage_at_end_gpu");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndGPU = (key == "TRUE"); }
    it = section.find("voltage_at_end_ram");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndRAM = (key == "TRUE"); }
    it = section.find("voltage_at_end_tmp");
    if (it != section.end()) { key = it->second; convertToUpper(key); settings->voltageAtEndTMP = (key == "TRUE"); }

    it = section.find("use_dynamic_colors");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDynamicColors = (key == "TRUE");
    }

    it = section.find("show_vddq");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showVDDQ = !(key == "FALSE");
    }

    it = section.find("show_vdd2");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showVDD2 = !(key == "FALSE");
    }

    it = section.find("decimal_vdd2");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->decimalVDD2 = !(key == "FALSE");
    }

    it = section.find("show_dtc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showDTC = !(key == "FALSE");
    }

    it = section.find("use_dtc_symbol");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDTCSymbol = !(key == "FALSE");
    }

    it = section.find("integer_fps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useIntegerFPS = (key != "FALSE");
    }

    // DTC format: prefer the new two-part keys (dtc_format_1, dtc_format_2). Fall back
    // to splitting the legacy dtc_format key at DIVIDER_SYMBOL so existing user configs
    // keep working. Final dtcFormat is rebuilt as format1 + DIVIDER + format2 (or just
    // format1 when format2 is OPTION_SYMBOL / "None").
    {
        const auto it1 = section.find("dtc_format_1");
        const auto it2 = section.find("dtc_format_2");
        const auto itLegacy = section.find("dtc_format");
        if (it1 != section.end() || it2 != section.end()) {
            if (it1 != section.end()) settings->dtcFormat1 = it1->second;
            if (it2 != section.end()) settings->dtcFormat2 = it2->second;
        } else if (itLegacy != section.end()) {
            const std::string legacy = itLegacy->second;
            const size_t divPos = legacy.find(ult::DIVIDER_SYMBOL);
            if (divPos != std::string::npos) {
                settings->dtcFormat1 = legacy.substr(0, divPos);
                settings->dtcFormat2 = legacy.substr(divPos + ult::DIVIDER_SYMBOL.length());
            } else {
                settings->dtcFormat1 = legacy;
                settings->dtcFormat2 = ult::OPTION_SYMBOL;
            }
        }
        if (settings->dtcFormat2 == ult::OPTION_SYMBOL || settings->dtcFormat2.empty()) {
            settings->dtcFormat = settings->dtcFormat1;
        } else {
            settings->dtcFormat = settings->dtcFormat1 + ult::DIVIDER_SYMBOL + settings->dtcFormat2;
        }
    }

    // Invert the battery display value
    it = section.find("invert_battery_display");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->invertBatteryDisplay = (key != "FALSE");
    }

    it = section.find("show_side_by_side_bat");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedBAT = (key == "FALSE");
    }

    it = section.find("show_side_by_side_dtc");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedDTC = (key == "FALSE");
    }
    
    // Process font sizes with shared bounds
    static constexpr long minFontSize = 8;
    static constexpr long maxFontSize = 18;
    
    it = section.find("handheld_font_size");
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
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }
    
    it = section.find("separator_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->separatorColor = temp;
    }
    
    it = section.find("cat_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor = temp;
    }
    
    it = section.find("text_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
    
    // Process text alignment
    it = section.find("text_align");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        if (key == "LEFT") {
            settings->alignTo = 0;
        } else if (key == "CENTER") {
            settings->alignTo = 1;
        } else if (key == "RIGHT") {
            settings->alignTo = 2;
        }
    }
    
    // Process RAM load flag
    it = section.find("replace_GB_with_RAM_load");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMLoad = (key != "FALSE");
    }
    
    // Process show string
    it = section.find("show");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->show = std::move(key);
    }
    
    // Process layer height alignment
    it = section.find("layer_height_align");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->setPosBottom = (key == "BOTTOM");
    }

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }

    it = section.find("show_RAM_load_CPU_GPU");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRAMLoadCPUGPU = (key != "FALSE");
    }

    it = section.find("show_side_by_side_ram_load");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedRAMLoad = (key == "FALSE");
    }

    // Process component temps flag (also used in Micro)
    it = section.find("show_component_temps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showComponentTemps = (key != "FALSE");
    }

    // Process SOC/PCB/Skin temps flag
    it = section.find("show_soc_pcb_skin_temps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showSocPcbSkinTemps = (key != "FALSE");
    }

    // Process stacked temps flag
    it = section.find("show_side_by_side_temps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showStackedTemps = (key == "FALSE");
    }

    // Enforce mutual exclusivity: at least one temp group must be on
    if (!settings->showComponentTemps && !settings->showSocPcbSkinTemps)
        settings->showSocPcbSkinTemps = true;

    // Horizontal/vertical padding for bar text margins
    it = section.find("horizontal_padding");
    if (it != section.end()) {
        settings->horizontalPadding = (uint8_t)std::clamp(atoi(it->second.c_str()), 0, 20);
    }
    it = section.find("vertical_padding");
    if (it != section.end()) {
        settings->verticalPadding = (uint8_t)std::clamp(atoi(it->second.c_str()), 0, 20);
    }
    it = section.find("label_padding");
    if (it != section.end()) {
        settings->labelPadding = (uint8_t)std::clamp(atoi(it->second.c_str()), 4, 12);
    }

}

ALWAYS_INLINE void GetConfigSettings(FpsCounterSettings* settings) {
    // Initialize defaults
    settings->handheldFontSize = 40;
    settings->dockedFontSize = 40;
    convertStrToRGBA4444("#0009", &(settings->backgroundColor));
    convertStrToRGBA4444("#000F", &(settings->focusBackgroundColor));
    convertStrToRGBA4444("#8CFF", &(settings->textColor));
    //settings->setPos = 0;
    settings->refreshRate = 5;
    settings->useIntegerFPS = false;
    settings->disableScreenshots = false;

    settings->frameOffsetX = 10;
    settings->frameOffsetY = 10;
    settings->framePadding = 10;

    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    

    std::string key;
    uint16_t temp;

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
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }

    it = section.find("focus_background_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->focusBackgroundColor = temp;
    }

    
    it = section.find("text_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
    
    // Process alignment settings
    //it = section.find("layer_width_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos = 1;
    //    } else if (key == "RIGHT") {
    //        settings->setPos = 2;
    //    }
    //}
    
    //it = section.find("layer_height_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos += 3;
    //    } else if (key == "BOTTOM") {
    //        settings->setPos += 6;
    //    }
    //}

    it = section.find("integer_fps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useIntegerFPS = (key != "FALSE");
    }

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }

    it = section.find("frame_offset_x");
    if (it != section.end()) {
        settings->frameOffsetX = atol(it->second.c_str());
    }

    it = section.find("frame_offset_y");
    if (it != section.end()) {
        settings->frameOffsetY = atol(it->second.c_str());
    }

    it = section.find("frame_padding");
    if (it != section.end()) {
        settings->framePadding = atol(it->second.c_str());
    }
}

ALWAYS_INLINE void GetConfigSettings(FpsGraphSettings* settings) {
    // Initialize defaults
    settings->showInfo = true;
    //settings->setPos = 0;
    convertStrToRGBA4444("#0009", &(settings->backgroundColor));
    convertStrToRGBA4444("#000F", &(settings->focusBackgroundColor));
    convertStrToRGBA4444("#888C", &(settings->fpsColor));
    convertStrToRGBA4444("#2DFF", &(settings->borderColor));
    convertStrToRGBA4444("#8888", &(settings->dashedLineColor));
    convertStrToRGBA4444("#FFFF", &(settings->maxFPSTextColor));
    convertStrToRGBA4444("#FFFF", &(settings->minFPSTextColor));
    convertStrToRGBA4444("#FFFF", &(settings->mainLineColor));
    convertStrToRGBA4444("#F0FF", &(settings->roundedLineColor));
    convertStrToRGBA4444("#0C0F", &(settings->perfectLineColor));

    convertStrToRGBA4444("#FFFF", &(settings->textColor));
    convertStrToRGBA4444("#2DFF", &(settings->catColor));

    settings->refreshRate = 5;
    settings->useDynamicColors = true;
    settings->useIntegerFPS = false;
    settings->disableScreenshots = false;

    settings->frameOffsetX = 6;
    settings->frameOffsetY = 6;
    settings->framePadding = 6;


    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    
    std::string key;
    uint16_t temp;

    const auto& section = sectionIt->second;
    
    // Process alignment settings
    //auto it = section.find("layer_width_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos = 1;
    //    } else if (key == "RIGHT") {
    //        settings->setPos = 2;
    //    }
    //}
    //
    //it = section.find("layer_height_align");
    //if (it != section.end()) {
    //    key = it->second;
    //    convertToUpper(key);
    //    if (key == "CENTER") {
    //        settings->setPos += 3;
    //    } else if (key == "BOTTOM") {
    //        settings->setPos += 6;
    //    }
    //}
    
    // Process show_info boolean
    auto it = section.find("show_info");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showInfo = (key == "TRUE");
    }

    it = section.find("use_dynamic_colors");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDynamicColors = (key == "TRUE");
    }

    it = section.find("integer_fps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useIntegerFPS = (key != "FALSE");
    }

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }

    it = section.find("frame_offset_x");
    if (it != section.end()) {
        settings->frameOffsetX = atol(it->second.c_str());
    }

    it = section.find("frame_offset_y");
    if (it != section.end()) {
        settings->frameOffsetY = atol(it->second.c_str());
    }

    it = section.find("frame_padding");
    if (it != section.end()) {
        settings->framePadding = atol(it->second.c_str());
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
        {"focus_background_color", &settings->focusBackgroundColor},
        {"fps_counter_color", &settings->fpsColor},
        {"border_color", &settings->borderColor},
        {"dashed_line_color", &settings->dashedLineColor},
        {"main_line_color", &settings->mainLineColor},
        {"rounded_line_color", &settings->roundedLineColor},
        {"perfect_line_color", &settings->perfectLineColor},
        {"text_color", &settings->textColor},
        {"cat_color", &settings->catColor}
    };
    
    for (const auto& mapping : colorMappings) {
        it = section.find(mapping.key);
        if (it != section.end()) {
            temp = 0;
            if (convertStrToRGBA4444(it->second, &temp))
                *(mapping.target) = temp;
        }
    }
}

ALWAYS_INLINE void GetConfigSettings(FullSettings* settings) {
    // Initialize defaults
    settings->setPosRight = false;
    settings->refreshRate = 3;
    settings->showRealFreqs = true;
    settings->showDeltas = true;
    settings->showTargetFreqs = true;
    settings->showFPS = true;
    settings->showRES = true;
    settings->showRDSD = true;
    settings->useDynamicColors = true;
    settings->disableScreenshots = false;
    convertStrToRGBA4444("#888F", &(settings->separatorColor));
    convertStrToRGBA4444("#8FFF", &(settings->catColor1));
    convertStrToRGBA4444("#8CFF", &(settings->catColor2));
    convertStrToRGBA4444("#FFFF", &(settings->textColor));

    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    auto sectionIt = parsedData.find("full");
    if (sectionIt == parsedData.end()) return;
    
    std::string key;
    uint16_t temp;
    
    const auto& section = sectionIt->second;

    // Process refresh_rate
    auto it = section.find("refresh_rate");
    if (it != section.end()) {
        settings->refreshRate = std::clamp(atol(it->second.c_str()), 1L, 60L);
    }
    
    // Process layer position
    it = section.find("layer_width_align");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->setPosRight = (key == "RIGHT");
    }
    
    // Process boolean flags
    it = section.find("show_real_freqs");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRealFreqs = !(key == "FALSE");
    }
    
    it = section.find("show_deltas");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showDeltas = !(key == "FALSE");
    }
    
    it = section.find("show_target_freqs");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showTargetFreqs = !(key == "FALSE");
    }
    
    it = section.find("show_fps");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showFPS = !(key == "FALSE");
    }
    
    it = section.find("show_res");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRES = !(key == "FALSE");
    }
    
    it = section.find("show_read_speed");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->showRDSD = !(key == "FALSE");
    }
    
    it = section.find("use_dynamic_colors");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->useDynamicColors = (key == "TRUE");
    }

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }

    it = section.find("separator_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->separatorColor = temp;
    }

    it = section.find("cat_color_1");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor1 = temp;
    }
    
    it = section.find("cat_color_2");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor2 = temp;
    }

    it = section.find("text_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }
}

ALWAYS_INLINE void GetConfigSettings(ResolutionSettings* settings) {
    // Initialize defaults
    convertStrToRGBA4444("#0009", &(settings->backgroundColor));
    convertStrToRGBA4444("#000F", &(settings->focusBackgroundColor));
    convertStrToRGBA4444("#8FFF", &(settings->catColor));
    //convertStrToRGBA4444("#8CFF", &(settings->catColor2));
    convertStrToRGBA4444("#FFFF", &(settings->textColor));
    settings->refreshRate = 3;
    //ettings->setPos = 0;
    settings->disableScreenshots = false;

    settings->frameOffsetX = 6;
    settings->frameOffsetY = 6;
    settings->framePadding = 6;


    // Open and read file efficiently
    FILE* configFile = fopen(configIniPath, "r");
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
    auto sectionIt = parsedData.find("game_resolutions");
    if (sectionIt == parsedData.end()) return;
    
    std::string key;
    
    const auto& section = sectionIt->second;

    // Process refresh_rate
    auto it = section.find("refresh_rate");
    if (it != section.end()) {
        settings->refreshRate = std::clamp(atol(it->second.c_str()), 1L, 60L);
    }

    uint16_t temp;

    // Process colors
    it = section.find("background_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->backgroundColor = temp;
    }

    it = section.find("focus_background_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->focusBackgroundColor = temp;
    }
    
    it = section.find("cat_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->catColor = temp;
    }

    //it = section.find("cat_color_2");
    //if (it != section.end()) {
    //    temp = 0;
    //    if (convertStrToRGBA4444(it->second, &temp))
    //        settings->catColor2 = temp;
    //}

    
    it = section.find("text_color");
    if (it != section.end()) {
        temp = 0;
        if (convertStrToRGBA4444(it->second, &temp))
            settings->textColor = temp;
    }

    it = section.find("frame_offset_x");
    if (it != section.end()) {
        settings->frameOffsetX = atol(it->second.c_str());
    }

    it = section.find("frame_offset_y");
    if (it != section.end()) {
        settings->frameOffsetY = atol(it->second.c_str());
    }

    it = section.find("frame_padding");
    if (it != section.end()) {
        settings->framePadding = atol(it->second.c_str());
    }
    
    // Process alignment settings
   //it = section.find("layer_width_align");
   //if (it != section.end()) {
   //    key = it->second;
   //    convertToUpper(key);
   //    if (key == "CENTER") {
   //        settings->setPos = 1;
   //    } else if (key == "RIGHT") {
   //        settings->setPos = 2;
   //    }
   //}
   //
   //it = section.find("layer_height_align");
   //if (it != section.end()) {
   //    key = it->second;
   //    convertToUpper(key);
   //    if (key == "CENTER") {
   //        settings->setPos += 3;
   //    } else if (key == "BOTTOM") {
   //        settings->setPos += 6;
   //    }
   //}

    // Process disable screenshots
    it = section.find("disable_screenshots");
    if (it != section.end()) {
        key = it->second;
        convertToUpper(key);
        settings->disableScreenshots = (key != "FALSE");
    }
}