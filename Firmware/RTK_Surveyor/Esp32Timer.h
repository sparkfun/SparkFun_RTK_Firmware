// Esp32Timer.h

#ifndef __ESP32_TIMER_H__
#define __ESP32_TIMER_H__

extern void systemPrintf(const char * format, ...);

#define TIMG0                       0x3ff5f000
#define TIMG1                       0x3ff60000

#define TIMG_T0CONFIG_REG           0x0000  // RW
#define TIMG_T0LO_REG               0x0004  // RO
#define TIMG_T0HI_REG               0x0008  // RO
#define TIMG_T0UPDATE_REG           0x000c  // WO
#define TIMG_T0ALARMLO_REG          0x0010  // RW
#define TIMG_T0ALARMHI_REG          0x0014  // RW
#define TIMG_T0LOADLO_REG           0x0018  // RW
#define TIMG_T0LOADHI_REG           0x001c  // RW
#define TIMG_T0LOAD_REG             0x0020  // WO

#define TIMG_T1CONFIG_REG           0x0024  // RW
#define TIMG_T1LO_REG               0x0028  // RO
#define TIMG_T1HI_REG               0x002c  // RO
#define TIMG_T1UPDATE_REG           0x0030  // WO
#define TIMG_T1ALARMLO_REG          0x0034  // RW
#define TIMG_T1ALARMHI_REG          0x0038  // RW
#define TIMG_T1LOADLO_REG           0x003c  // RW
#define TIMG_T1LOADHI_REG           0x0040  // RW
#define TIMG_T1LOAD_REG             0x0044  // WO

#define TIMG_T_WDTCONFIG0_REG       0x0048  // RW
#define TIMG_T_WDTCONFIG1_REG       0x004c  // RW, Clock prescale * 12.5ns
#define TIMG_T_WDTCONFIG2_REG       0x0050  // RW
#define TIMG_T_WDTCONFIG3_REG       0x0054  // RW
#define TIMG_T_WDTCONFIG4_REG       0x0058  // RW
#define TIMG_T_WDTCONFIG5_REG       0x005c  // RW
#define TIMG_T_WDTFEED_REG          0x0060  // WO
#define TIMG_T_WDTWPROTECT_REG      0x0064  // RW

#define TIMG_RTCCALICFG_REG         0x0068  // varies
#define TIMG_RTCCALICFG1_REG        0x006c  // RO

#define TIMG_T_INT_ENA_REG          0x0098  // RW
#define TIMG_T_INT_RAW_REG          0x009c  // RO
#define TIMG_T_INT_ST_REG           0x00a0  // RO
#define TIMG_T_INT_CLR_REG          0x00a4  // WO

// TIMG_TxCONFIG_REG
#define TIMG_Tx_EN                  0x80000000  // Enable the timer
#define TIMG_Tx_INCREASE            0x40000000  // Timer value increases every clock tick
#define TIMG_Tx_AUTORELOAD          0x20000000  // Reload timer upon alarm
#define TIMG_Tx_DIVIDER             0x1ffff000  // Clock prescale value
#define TIMG_Tx_EDGE_INT_EN         0x00000800  // Alarm generates edge interrupt
#define TIMG_Tx_LEVEL_INT_EN        0x00000400  // Alarm generates level interrupt
#define TIMG_Tx_ALARM_EN            0x00000200  // Alarm enable

// TIMG_T_WDTCONFIG0_REG
#define TIMG_T_WDT_EN               0x80000000  // Enable MWDT

#define TIMG_T_WDT_STG0             0x60000000  // Stage 0 configuration
#define TIMG_T_WDT_STG0_RST_SYSTEM  0x60000000
#define TIMG_T_WDT_STG0_RST_CPU     0x40000000
#define TIMG_T_WDT_STG0_INTERRUPT   0x20000000
#define TIMG_T_WDT_STG0_OFF         0x00000000

#define TIMG_T_WDT_STG1             0x18000000  // Stage 1 configuration
#define TIMG_T_WDT_STG1_RST_SYSTEM  0x18000000
#define TIMG_T_WDT_STG1_RST_CPU     0x10000000
#define TIMG_T_WDT_STG1_INTERRUPT   0x08000000
#define TIMG_T_WDT_STG1_OFF         0x00000000

#define TIMG_T_WDT_STG2             0x06000000  // Stage 2 configuration
#define TIMG_T_WDT_STG2_RST_SYSTEM  0x06000000
#define TIMG_T_WDT_STG2_RST_CPU     0x04000000
#define TIMG_T_WDT_STG2_INTERRUPT   0x02000000
#define TIMG_T_WDT_STG2_OFF         0x00000000

#define TIMG_T_WDT_STG3             0x01800000  // Stage 3 configuration
#define TIMG_T_WDT_STG3_RST_SYSTEM  0x01800000
#define TIMG_T_WDT_STG3_RST_CPU     0x01000000
#define TIMG_T_WDT_STG3_INTERRUPT   0x00800000
#define TIMG_T_WDT_STG3_OFF         0x00000000

#define TIMG_T_WDT_EDGE_INT_EN      0x00400000  // Enable edge interrupts
#define TIMG_T_WDT_LEVEL_INT_EN     0x00200000  // Enable level interrupts

#define TIMG_T_WDT_CPU_RESET_LENGTH 0x001c0000  // CPU reset pulse width
#define TIMG_T_WDT_CPU_RESET_3200ns 0x001c0000
#define TIMG_T_WDT_CPU_RESET_1600ns 0x00180000
#define TIMG_T_WDT_CPU_RESET_800ns  0x00140000
#define TIMG_T_WDT_CPU_RESET_500ns  0x00100000
#define TIMG_T_WDT_CPU_RESET_400ns  0x000c0000
#define TIMG_T_WDT_CPU_RESET_300ns  0x00080000
#define TIMG_T_WDT_CPU_RESET_200ns  0x00040000
#define TIMG_T_WDT_CPU_RESET_100ns  0x00000000

#define TIMG_T_WDT_SYS_RESET_LENGTH 0x00038000  // System reset pulse width
#define TIMG_T_WDT_SYS_RESET_3200ns 0x00038000
#define TIMG_T_WDT_SYS_RESET_1600ns 0x00030000
#define TIMG_T_WDT_SYS_RESET_800ns  0x00028000
#define TIMG_T_WDT_SYS_RESET_500ns  0x00020000
#define TIMG_T_WDT_SYS_RESET_400ns  0x00018000
#define TIMG_T_WDT_SYS_RESET_300ns  0x00010000
#define TIMG_T_WDT_SYS_RESET_200ns  0x00008000
#define TIMG_T_WDT_SYS_RESET_100ns  0x00000000

#define TIMG_T_WDT_FLASHBOOT_MOD_EN 0x00004000

double printClockPeriod(uint32_t config)
{
    uint32_t clocks;
    double clockPeriod;
    double multiplier;
    const char * units;

    clocks = config >> 16;
    clockPeriod = 0.0000000125 * clocks;
    if (clockPeriod >= 1.)
    {
        units = "Sec";
        multiplier = 1;
    }
    else if (clockPeriod >= 0.001)
    {
        units = "mSec";
        multiplier = 1000.;
    }
    else if (clockPeriod >= 0.000001)
    {
        units = "uSec";
        multiplier = 1000000;
    }
    else
    {
        units = "nSec";
        multiplier = 1000000000.;
    }
    systemPrintf("      Clock period: %7.3f %s (%d - 12.5 nSec clocks)", clockPeriod * multiplier, units, clocks);
    return clockPeriod;
}

void printWdtTimeout(double clockPeriod, uint32_t clocks)
{
    double multiplier;
    double timeout;
    const char * units;

    timeout = clockPeriod * (double)clocks;
    if (timeout >= 1.)
    {
        units = "Sec";
        multiplier = 1;
    }
    else if (timeout >= 0.001)
    {
        units = "mSec";
        multiplier = 1000.;
    }
    else if (timeout >= 0.000000)
    {
        units = "uSec";
        multiplier = 1000000.;
    }
    else
    {
        units = "nSec";
        multiplier = 1000000000.;
    }
    systemPrintf(", timeout: %5.1f %s (%d clocks)\r\n", timeout * multiplier, units, clocks);
}

void printWdt(intptr_t baseAddress)
{
    double clockPeriod;
    const char * const config[] =
    {
        "Off",
        "Interrupt",
        "Reset CPU",
        "Reset System"
    };
    uint32_t protect;
    const int pulseWidth[] = {100, 200, 300, 400, 500, 800, 1600, 3200};
    uint32_t value[6];

    systemPrintf("0x%08x: Watch Dog Timer\r\n", baseAddress);
    value[0] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG0_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG0_REG\r\n", value[0]);
    value[1] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG1_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG1_REG\r\n", value[1]);
    value[2] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG2_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG2_REG\r\n", value[2]);
    value[3] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG3_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG3_REG\r\n", value[3]);
    value[4] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG4_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG4_REG\r\n", value[4]);
    value[5] = *(uint32_t *)(baseAddress + TIMG_T_WDTCONFIG5_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTCONFIG5_REG\r\n", value[5]);
    protect = *(uint32_t *)(baseAddress + TIMG_T_WDTWPROTECT_REG);
    systemPrintf("    0x%08x: TIMG_T_WDTWPROTECT_REG\r\n", protect);

    if (value[0] & TIMG_T_WDT_EN)
    {
        // TIMG_T_WDTCONFIG0_REG
        systemPrintf("        Watch dog enabled\r\n");
        clockPeriod = printClockPeriod(value[1]);
        systemPrintf("        Stage %d: %s", 0, config[(value[0] >> 29) & 3]);
        if (value[0] & TIMG_T_WDT_STG0_RST_SYSTEM)
            printWdtTimeout(clockPeriod, value[2]);
        systemPrintf("\r\n");
        systemPrintf("        Stage %d: %s", 1, config[(value[0] >> 27) & 3]);
        if (value[0] & TIMG_T_WDT_STG1_RST_SYSTEM)
            printWdtTimeout(clockPeriod, value[3]);
        systemPrintf("\r\n");
        systemPrintf("        Stage %d: %s", 2, config[(value[0] >> 25) & 3]);
        if (value[0] & TIMG_T_WDT_STG2_RST_SYSTEM)
            printWdtTimeout(clockPeriod, value[4]);
        systemPrintf("\r\n");
        systemPrintf("        Stage %d: %s", 3, config[(value[0] >> 23) & 3]);
        if (value[0] & TIMG_T_WDT_STG3_RST_SYSTEM)
            printWdtTimeout(clockPeriod, value[5]);
        systemPrintf("\r\n");
        if ((value[0] & TIMG_T_WDT_STG0_INTERRUPT)
            || (value[0] & TIMG_T_WDT_STG1_INTERRUPT)
            || (value[0] & TIMG_T_WDT_STG2_INTERRUPT)
            || (value[0] & TIMG_T_WDT_STG3_INTERRUPT))
        {
            if (value[0] & TIMG_T_WDT_EDGE_INT_EN)
                systemPrintf("        Generate edge interrupt\r\n");
            if (value[0] & TIMG_T_WDT_LEVEL_INT_EN)
                systemPrintf("        Generate level interrupt\r\n");
        }
        if (((value[0] & TIMG_T_WDT_STG0_RST_SYSTEM) == TIMG_T_WDT_STG0_RST_CPU)
            || ((value[0] & TIMG_T_WDT_STG1_RST_SYSTEM) == TIMG_T_WDT_STG1_RST_CPU)
            || ((value[0] & TIMG_T_WDT_STG2_RST_SYSTEM) == TIMG_T_WDT_STG2_RST_CPU)
            || ((value[0] & TIMG_T_WDT_STG3_RST_SYSTEM) == TIMG_T_WDT_STG3_RST_CPU))
        {
            systemPrintf("        CPU reset pulse: %d nSec\r\n", pulseWidth[(value[0] >> 18) & 7]);
        }
        if (((value[0] & TIMG_T_WDT_STG0_RST_SYSTEM) == TIMG_T_WDT_STG0_RST_SYSTEM)
            || ((value[0] & TIMG_T_WDT_STG1_RST_SYSTEM) == TIMG_T_WDT_STG1_RST_SYSTEM)
            || ((value[0] & TIMG_T_WDT_STG2_RST_SYSTEM) == TIMG_T_WDT_STG2_RST_SYSTEM)
            || ((value[0] & TIMG_T_WDT_STG3_RST_SYSTEM) == TIMG_T_WDT_STG3_RST_SYSTEM))
        {
            systemPrintf("        System reset pulse: %d nSec\r\n", pulseWidth[(value[0] >> 15) & 7]);
        }
        if (value[0] & TIMG_T_WDT_FLASHBOOT_MOD_EN)
            systemPrintf("        Flash boot protection enabled\r\n");
    }
    else
        systemPrintf("        Watch dog disabled\r\n");
}

#endif  // __ESP32_TIMER_H__
