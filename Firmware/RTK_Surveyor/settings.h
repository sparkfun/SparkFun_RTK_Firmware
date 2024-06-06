#ifndef __SETTINGS_H__
#define __SETTINGS_H__

// System can enter a variety of states
// See statemachine diagram at:
// https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
typedef enum
{
    STATE_ROVER_NOT_STARTED = 0,
    STATE_ROVER_NO_FIX,
    STATE_ROVER_FIX,
    STATE_ROVER_RTK_FLOAT,
    STATE_ROVER_RTK_FIX,
    STATE_BASE_NOT_STARTED,
    STATE_BASE_TEMP_SETTLE, // User has indicated base, but current pos accuracy is too low
    STATE_BASE_TEMP_SURVEY_STARTED,
    STATE_BASE_TEMP_TRANSMITTING,
    STATE_BASE_FIXED_NOT_STARTED,
    STATE_BASE_FIXED_TRANSMITTING,
    STATE_BUBBLE_LEVEL,
    STATE_MARK_EVENT,
    STATE_DISPLAY_SETUP,
    STATE_WIFI_CONFIG_NOT_STARTED,
    STATE_WIFI_CONFIG,
    STATE_TEST,
    STATE_TESTING,
    STATE_PROFILE,
    STATE_KEYS_STARTED,
    STATE_KEYS_NEEDED,
    STATE_KEYS_WIFI_STARTED,
    STATE_KEYS_WIFI_CONNECTED,
    STATE_KEYS_WIFI_TIMEOUT,
    STATE_KEYS_EXPIRED,
    STATE_KEYS_DAYS_REMAINING,
    STATE_KEYS_LBAND_CONFIGURE,
    STATE_KEYS_LBAND_ENCRYPTED,
    STATE_KEYS_PROVISION_WIFI_STARTED,
    STATE_KEYS_PROVISION_WIFI_CONNECTED,
    STATE_ESPNOW_PAIRING_NOT_STARTED,
    STATE_ESPNOW_PAIRING,
    STATE_NTPSERVER_NOT_STARTED,
    STATE_NTPSERVER_NO_SYNC,
    STATE_NTPSERVER_SYNC,
    STATE_CONFIG_VIA_ETH_NOT_STARTED,
    STATE_CONFIG_VIA_ETH_STARTED,
    STATE_CONFIG_VIA_ETH,
    STATE_CONFIG_VIA_ETH_RESTART_BASE,
    STATE_SHUTDOWN,
    STATE_NOT_SET, // Must be last on list
} SystemState;
volatile SystemState systemState = STATE_NOT_SET;
SystemState lastSystemState = STATE_NOT_SET;
SystemState requestedSystemState = STATE_NOT_SET;
bool newSystemStateRequested = false;

// The setup display can show a limited set of states
// When user pauses for X amount of time, system will enter that state
SystemState setupState = STATE_MARK_EVENT;

// Base modes set with RTK_MODE
#define RTK_MODE_BASE_FIXED 0x0001
#define RTK_MODE_BASE_SURVEY_IN 0x0002
#define RTK_MODE_BUBBLE_LEVEL 0x0004
#define RTK_MODE_ETHERNET_CONFIG 0x0008
#define RTK_MODE_NTP 0x0010
#define RTK_MODE_ROVER 0x0020
#define RTK_MODE_TESTING 0x0040
#define RTK_MODE_WIFI_CONFIG 0x0080

typedef uint8_t RtkMode_t;

#define RTK_MODE(mode) rtkMode = mode;

#define EQ_RTK_MODE(mode) (rtkMode && (rtkMode == (mode & rtkMode)))
#define NEQ_RTK_MODE(mode) (rtkMode && (rtkMode != (mode & rtkMode)))

typedef enum
{
    RTK_SURVEYOR = 0,
    RTK_EXPRESS,
    RTK_FACET,
    RTK_EXPRESS_PLUS,
    RTK_FACET_LBAND,
    REFERENCE_STATION,
    RTK_FACET_LBAND_DIRECT,
    // Add new values just above this line
    RTK_UNKNOWN,
} ProductVariant;
ProductVariant productVariant = RTK_SURVEYOR;

const char *const productDisplayNames[] = {
    "Surveyor",
    "Express",
    "Facet",
    "Express+",
    "Facet LB",
    "Ref Stn",
    "Facet LD",
    // Add new values just above this line
    "Unknown",
};
const int productDisplayNamesEntries = sizeof(productDisplayNames) / sizeof(productDisplayNames[0]);

const char *const platformFilePrefixTable[] = {
    "SFE_Surveyor",
    "SFE_Express",
    "SFE_Facet",
    "SFE_Express_Plus",
    "SFE_Facet_LBand",
    "SFE_Reference_Station",
    "SFE_Facet_LBand_Direct",
    // Add new values just above this line
    "SFE_Unknown",
};
const int platformFilePrefixTableEntries = sizeof(platformFilePrefixTable) / sizeof(platformFilePrefixTable[0]);

const char *const platformPrefixTable[] = {
    "Surveyor",
    "Express",
    "Facet",
    "Express Plus",
    "Facet L-Band",
    "Reference Station",
    "Facet L-Band Direct",
    // Add new values just above this line
    "Unknown",
};
const int platformPrefixTableEntries = sizeof(platformPrefixTable) / sizeof(platformPrefixTable[0]);

// Macros to show if the GNSS is I2C or SPI
#define USE_SPI_GNSS (productVariant == REFERENCE_STATION)
#define USE_I2C_GNSS (!USE_SPI_GNSS)

// Macros to show if the microSD is SPI or SDIO
#define USE_MMC_MICROSD (productVariant == REFERENCE_STATION)
#define USE_SPI_MICROSD (!USE_MMC_MICROSD)

// Macro to show if the the RTK variant has Ethernet
#ifdef COMPILE_ETHERNET
#define HAS_ETHERNET (productVariant == REFERENCE_STATION)
#else // COMPILE_ETHERNET
#define HAS_ETHERNET false
#endif // COMPILE_ETHERNET

// Macro to show if the the RTK variant has a GNSS TP interrupt - for accurate clock setting
// The GNSS UBX PVT message is sent ahead of the top-of-second
// The rising edge of the TP signal indicates the true top-of-second
#define HAS_GNSS_TP_INT (productVariant == REFERENCE_STATION)

// Macro to show if the the RTK variant has no battery
#define HAS_NO_BATTERY (productVariant == REFERENCE_STATION)
#define HAS_BATTERY (!HAS_NO_BATTERY)

// Macro to show if the the RTK variant has antenna short circuit / open circuit detection
#define HAS_ANTENNA_SHORT_OPEN (productVariant == REFERENCE_STATION)

typedef enum
{
    BUTTON_ROVER = 0,
    BUTTON_BASE,
} ButtonState;
ButtonState buttonPreviousState = BUTTON_ROVER;

// Data port mux (RTK Express) can enter one of four different connections
typedef enum
{
    MUX_UBLOX_NMEA = 0,
    MUX_PPS_EVENTTRIGGER,
    MUX_I2C_WT,
    MUX_ADC_DAC,
} muxConnectionType_e;

// User can enter fixed base coordinates in ECEF or degrees
typedef enum
{
    COORD_TYPE_ECEF = 0,
    COORD_TYPE_GEODETIC,
} coordinateType_e;

// User can select output pulse as either falling or rising edge
typedef enum
{
    PULSE_FALLING_EDGE = 0,
    PULSE_RISING_EDGE,
} pulseEdgeType_e;

// Custom NMEA sentence types output to the log file
typedef enum
{
    CUSTOM_NMEA_TYPE_RESET_REASON = 0,
    CUSTOM_NMEA_TYPE_WAYPOINT,
    CUSTOM_NMEA_TYPE_EVENT,
    CUSTOM_NMEA_TYPE_SYSTEM_VERSION,
    CUSTOM_NMEA_TYPE_ZED_VERSION,
    CUSTOM_NMEA_TYPE_STATUS,
    CUSTOM_NMEA_TYPE_LOGTEST_STATUS,
    CUSTOM_NMEA_TYPE_DEVICE_BT_ID,
    CUSTOM_NMEA_TYPE_PARSER_STATS,
    CUSTOM_NMEA_TYPE_CURRENT_DATE,
    CUSTOM_NMEA_TYPE_ARP_ECEF_XYZH,
    CUSTOM_NMEA_TYPE_ZED_UNIQUE_ID,
} customNmeaType_e;

// Freeze and blink LEDs if we hit a bad error
typedef enum
{
    ERROR_NO_I2C = 2, // Avoid 0 and 1 as these are bad blink codes
    ERROR_GPS_CONFIG_FAIL,
} t_errorNumber;

// Define the types of network
enum NetworkTypes
{
    NETWORK_TYPE_WIFI = 0,
    NETWORK_TYPE_ETHERNET,
    // Last hardware network type
    NETWORK_TYPE_MAX,

    // Special cases
    NETWORK_TYPE_USE_DEFAULT = NETWORK_TYPE_MAX,
    NETWORK_TYPE_ACTIVE,
    // Last network type
    NETWORK_TYPE_LAST,
};

// Define the states of the network device
enum NetworkStates
{
    NETWORK_STATE_OFF = 0,
    NETWORK_STATE_DELAY,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_IN_USE,
    NETWORK_STATE_WAIT_NO_USERS,
    // Last network state
    NETWORK_STATE_MAX
};

// Define the network users
enum NetworkUsers
{
    NETWORK_USER_NTP_SERVER = 0,      // NTP server
    NETWORK_USER_NTRIP_CLIENT,        // NTRIP client
    NETWORK_USER_OTA_FIRMWARE_UPDATE, // Over-The-Air firmware updates
    NETWORK_USER_PVT_CLIENT,          // PVT client
    NETWORK_USER_PVT_SERVER,          // PVT server
    NETWORK_USER_PVT_UDP_SERVER,      // PVT UDP server

    // Add new users above this line
    NETWORK_USER_NTRIP_SERVER,        // NTRIP server
    // Last network user
    NETWORK_USER_MAX = NETWORK_USER_NTRIP_SERVER + NTRIP_SERVER_MAX
};

typedef uint16_t NETWORK_USER;

typedef struct _NETWORK_DATA
{
    uint8_t requestedNetwork;  // Type of network requested
    uint8_t type;              // Type of network
    NETWORK_USER activeUsers;  // Active users of this network device
    NETWORK_USER userOpens;    // Users requesting access to this network
    uint8_t connectionAttempt; // Number of previous connection attempts
    bool restart;              // Set if restart is allowed
    bool shutdown;             // Network is shutting down
    uint8_t state;             // Current state of the network
    uint32_t timeout;          // Timer timeout value
    uint32_t timerStart;       // Starting millis for the timer
} NETWORK_DATA;

// Even though WiFi and ESP-Now operate on the same radio, we treat
// then as different states so that we can leave the radio on if
// either WiFi or ESP-Now are active
enum WiFiState
{
    WIFI_STATE_OFF = 0,
    WIFI_STATE_START,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
};
volatile byte wifiState = WIFI_STATE_OFF;

#include "NetworkClient.h" // Built-in - Supports both WiFiClient and EthernetClient
#include "NetworkUDP.h"    //Built-in - Supports both WiFiUdp and EthernetUdp

// NTRIP Server data
typedef struct _NTRIP_SERVER_DATA
{
    // Network connection used to push RTCM to NTRIP caster
    NetworkClient *networkClient;
    volatile uint8_t state;

    // Count of bytes sent by the NTRIP server to the NTRIP caster
    uint32_t bytesSent;

    // Throttle the time between connection attempts
    // ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
    uint32_t connectionAttemptTimeout;
    uint32_t lastConnectionAttempt;
    int connectionAttempts; // Count the number of connection attempts between restarts

    // NTRIP server timer usage:
    //  * Reconnection delay
    //  * Measure the connection response time
    //  * Receive RTCM correction data timeout
    //  * Monitor last RTCM byte received for frame counting
    uint32_t timer;
    uint32_t startTime;
    int connectionAttemptsTotal; // Count the number of connection attempts absolutely

    // Additional count / times for ntripServerProcessRTCM
    uint32_t zedBytesSent ;
    uint32_t previousMilliseconds;
} NTRIP_SERVER_DATA;

typedef enum
{
    ESPNOW_OFF,
    ESPNOW_ON,
    ESPNOW_PAIRING,
    ESPNOW_MAC_RECEIVED,
    ESPNOW_PAIRED,
} ESPNOWState;
volatile ESPNOWState espnowState = ESPNOW_OFF;

typedef enum
{
    RTCM_TRANSPORT_STATE_WAIT_FOR_PREAMBLE_D3 = 0,
    RTCM_TRANSPORT_STATE_READ_LENGTH_1,
    RTCM_TRANSPORT_STATE_READ_LENGTH_2,
    RTCM_TRANSPORT_STATE_READ_MESSAGE_1,
    RTCM_TRANSPORT_STATE_READ_MESSAGE_2,
    RTCM_TRANSPORT_STATE_READ_DATA,
    RTCM_TRANSPORT_STATE_READ_CRC_1,
    RTCM_TRANSPORT_STATE_READ_CRC_2,
    RTCM_TRANSPORT_STATE_READ_CRC_3,
    RTCM_TRANSPORT_STATE_CHECK_CRC
} RtcmTransportState;

typedef enum
{
    RADIO_EXTERNAL = 0,
    RADIO_ESPNOW,
} RadioType_e;

typedef enum
{
    BLUETOOTH_RADIO_SPP = 0,
    BLUETOOTH_RADIO_BLE,
    BLUETOOTH_RADIO_OFF,
} BluetoothRadioType_e;

// Don't make this a typedef enum as logTestState
// can be incremented beyond LOGTEST_END
enum LogTestState
{
    LOGTEST_START = 0,
    LOGTEST_4HZ_5MSG_10MS,
    LOGTEST_4HZ_7MSG_10MS,
    LOGTEST_10HZ_5MSG_10MS,
    LOGTEST_10HZ_7MSG_10MS,
    LOGTEST_4HZ_5MSG_0MS,
    LOGTEST_4HZ_7MSG_0MS,
    LOGTEST_10HZ_5MSG_0MS,
    LOGTEST_10HZ_7MSG_0MS,
    LOGTEST_4HZ_5MSG_50MS,
    LOGTEST_4HZ_7MSG_50MS,
    LOGTEST_10HZ_5MSG_50MS,
    LOGTEST_10HZ_7MSG_50MS,
    LOGTEST_END,
};
uint8_t logTestState = LOGTEST_END;

typedef struct WiFiNetwork
{
    char ssid[50];
    char password[50];
} WiFiNetwork;

#define MAX_WIFI_NETWORKS 4

typedef uint16_t RING_BUFFER_OFFSET;

typedef enum
{
    ETH_NOT_STARTED = 0,
    ETH_STARTED_CHECK_CABLE,
    ETH_STARTED_START_DHCP,
    ETH_CONNECTED,
    ETH_CAN_NOT_BEGIN,
    // Add new states here
    ETH_MAX_STATE
} ethernetStatus_e;

const char *const ethernetStates[] = {
    "ETH_NOT_STARTED", "ETH_STARTED_CHECK_CABLE", "ETH_STARTED_START_DHCP", "ETH_CONNECTED", "ETH_CAN_NOT_BEGIN",
};

const int ethernetStateEntries = sizeof(ethernetStates) / sizeof(ethernetStates[0]);

// Radio status LED goes from off (LED off), no connection (blinking), to connected (solid)
typedef enum
{
    BT_OFF = 0,
    BT_NOTCONNECTED,
    BT_CONNECTED,
} BTState;

// Return values for getString()
typedef enum
{
    INPUT_RESPONSE_GETNUMBER_EXIT =
        -9999999, // Less than min ECEF. User may be prompted for number but wants to exit without entering data
    INPUT_RESPONSE_GETNUMBER_TIMEOUT = -9999998,
    INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT = 255,
    INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY = 254,
    INPUT_RESPONSE_INVALID = -4,
    INPUT_RESPONSE_TIMEOUT = -3,
    INPUT_RESPONSE_OVERFLOW = -2,
    INPUT_RESPONSE_EMPTY = -1,
    INPUT_RESPONSE_VALID = 1,
} InputResponse;

typedef enum
{
    PRINT_ENDPOINT_SERIAL = 0,
    PRINT_ENDPOINT_BLUETOOTH,
    PRINT_ENDPOINT_ALL,
} PrintEndpoint;
PrintEndpoint printEndpoint = PRINT_ENDPOINT_SERIAL; // Controls where the configuration menu gets piped to

typedef enum
{
    FUNCTION_NOT_SET = 0,
    FUNCTION_SYNC,
    FUNCTION_WRITESD,
    FUNCTION_FILESIZE,
    FUNCTION_EVENT,
    FUNCTION_BEGINSD,
    FUNCTION_RECORDSETTINGS,
    FUNCTION_LOADSETTINGS,
    FUNCTION_MARKEVENT,
    FUNCTION_GETLINE,
    FUNCTION_REMOVEFILE,
    FUNCTION_RECORDLINE,
    FUNCTION_CREATEFILE,
    FUNCTION_ENDLOGGING,
    FUNCTION_FINDLOG,
    FUNCTION_LOGTEST,
    FUNCTION_FILELIST,
    FUNCTION_FILEMANAGER_OPEN1,
    FUNCTION_FILEMANAGER_OPEN2,
    FUNCTION_FILEMANAGER_OPEN3,
    FUNCTION_FILEMANAGER_UPLOAD1,
    FUNCTION_FILEMANAGER_UPLOAD2,
    FUNCTION_FILEMANAGER_UPLOAD3,
    FUNCTION_SDSIZECHECK,
    FUNCTION_LOG_CLOSURE,
    FUNCTION_PRINT_FILE_LIST,
    FUNCTION_NTPEVENT,

} SemaphoreFunction;

#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3

// Each constellation will have its config key, enable, and a visible name
typedef struct
{
    uint32_t configKey;
    uint8_t gnssID;
    bool enabled;
    char textName[30];
} ubxConstellation;

// These are the allowable constellations to receive from and log (if enabled)
// Tested with u-center v21.02
#define MAX_CONSTELLATIONS 6 //(sizeof(ubxConstellations)/sizeof(ubxConstellation))

// Different ZED modules support different messages (F9P vs F9R vs F9T)
// Create binary packed struct for different platforms
typedef enum
{
    PLATFORM_F9P = 0b0001,
    PLATFORM_F9R = 0b0010,
    PLATFORM_F9T = 0b0100,
} ubxPlatform;

// Print the base coordinates in different formats, depending on the type the user has entered
// These are the different supported types
typedef enum
{
    COORDINATE_INPUT_TYPE_DD = 0,                   // Default DD.ddddddddd
    COORDINATE_INPUT_TYPE_DDMM,                     // DDMM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM,                    // DD MM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM_DASH,               // DD-MM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM_SYMBOL,             // DD°MM.mmmmmmm'
    COORDINATE_INPUT_TYPE_DDMMSS,                   // DD MM SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS,                 // DD MM SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS_DASH,            // DD-MM-SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL,          // DD°MM'SS.ssssss"
    COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL,        // DDMMSS - No decimal
    COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL,      // DD MM SS - No decimal
    COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL, // DD-MM-SS - No decimal
    COORDINATE_INPUT_TYPE_INVALID_UNKNOWN,
} CoordinateInputType;

#define UBX_ID_NOT_AVAILABLE 0xFF

// Define the periodic display values
typedef uint32_t PeriodicDisplay_t;

enum PeriodDisplayValues
{
    PD_BLUETOOTH_DATA_RX = 0, //  0
    PD_BLUETOOTH_DATA_TX,     //  1

    PD_ETHERNET_IP_ADDRESS, //  2
    PD_ETHERNET_STATE,      //  3

    PD_NETWORK_STATE, //  4

    PD_NTP_SERVER_DATA,  //  5
    PD_NTP_SERVER_STATE, //  6

    PD_NTRIP_CLIENT_DATA,  //  7
    PD_NTRIP_CLIENT_GGA,   //  8
    PD_NTRIP_CLIENT_STATE, //  9

    PD_NTRIP_SERVER_DATA,  // 10
    PD_NTRIP_SERVER_STATE, // 11

    PD_PVT_CLIENT_DATA,  // 12
    PD_PVT_CLIENT_STATE, // 13

    PD_PVT_SERVER_DATA,        // 14
    PD_PVT_SERVER_STATE,       // 15
    PD_PVT_SERVER_CLIENT_DATA, // 16

    PD_PVT_UDP_SERVER_DATA,           // 17
    PD_PVT_UDP_SERVER_STATE,          // 18
    PD_PVT_UDP_SERVER_BROADCAST_DATA, // 19

    PD_RING_BUFFER_MILLIS, // 20

    PD_SD_LOG_WRITE, // 21

    PD_TASK_BLUETOOTH_READ,   // 22
    PD_TASK_BUTTON_CHECK,     // 23
    PD_TASK_GNSS_READ,        // 24
    PD_TASK_HANDLE_GNSS_DATA, // 25
    PD_TASK_SD_SIZE_CHECK,    // 26

    PD_WIFI_IP_ADDRESS, // 27
    PD_WIFI_STATE,      // 28

    PD_ZED_DATA_RX, // 29
    PD_ZED_DATA_TX, // 30

    PD_OTA_CLIENT_STATE, // 31
    // Add new values before this line
};

#define PERIODIC_MASK(x) (1 << x)
#define PERIODIC_DISPLAY(x) (periodicDisplay & PERIODIC_MASK(x))
#define PERIODIC_CLEAR(x) periodicDisplay &= ~PERIODIC_MASK(x)
#define PERIODIC_SETTING(x) (settings.periodicDisplay & PERIODIC_MASK(x))
#define PERIODIC_TOGGLE(x) settings.periodicDisplay ^= PERIODIC_MASK(x)

// These are the allowable messages to broadcast and log (if enabled)

// Struct to describe the necessary info for each type of UBX message
// Each message will have a key, ID, class, visible name, and various info about which platforms the message is
// supported on Message rates are store within NVM
typedef struct
{
    const uint32_t msgConfigKey;
    const uint8_t msgID;
    const uint8_t msgClass;
    const uint8_t msgDefaultRate;
    const char msgTextName[20];
    const uint32_t filterMask;
    const uint16_t f9pFirmwareVersionSupported; // The minimum version this message is supported. 0 = all versions. 9999
                                                // = Not supported
    const uint16_t f9rFirmwareVersionSupported;
} ubxMsg;

// Static array containing all the compatible messages
const ubxMsg ubxMessages[] = {
    // NMEA
    {UBLOX_CFG_MSGOUT_NMEA_ID_DTM_UART1, UBX_NMEA_DTM, UBX_CLASS_NMEA, 0, "UBX_NMEA_DTM", SFE_UBLOX_FILTER_NMEA_DTM,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GBS_UART1, UBX_NMEA_GBS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GBS", SFE_UBLOX_FILTER_NMEA_GBS,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, UBX_NMEA_GGA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GGA", SFE_UBLOX_FILTER_NMEA_GGA,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, UBX_NMEA_GLL, UBX_CLASS_NMEA, 0, "UBX_NMEA_GLL", SFE_UBLOX_FILTER_NMEA_GLL,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GNS_UART1, UBX_NMEA_GNS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GNS", SFE_UBLOX_FILTER_NMEA_GNS,
     112, 120},

    {UBLOX_CFG_MSGOUT_NMEA_ID_GRS_UART1, UBX_NMEA_GRS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GRS", SFE_UBLOX_FILTER_NMEA_GRS,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, UBX_NMEA_GSA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GSA", SFE_UBLOX_FILTER_NMEA_GSA,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART1, UBX_NMEA_GST, UBX_CLASS_NMEA, 1, "UBX_NMEA_GST", SFE_UBLOX_FILTER_NMEA_GST,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, UBX_NMEA_GSV, UBX_CLASS_NMEA, 4, "UBX_NMEA_GSV", SFE_UBLOX_FILTER_NMEA_GSV,
     112, 120}, // Default to 1 update every 4 fixes
    {UBLOX_CFG_MSGOUT_NMEA_ID_RLM_UART1, UBX_NMEA_RLM, UBX_CLASS_NMEA, 0, "UBX_NMEA_RLM", SFE_UBLOX_FILTER_NMEA_RLM,
     113, 120}, // No F9P 112 support

    {UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, UBX_NMEA_RMC, UBX_CLASS_NMEA, 1, "UBX_NMEA_RMC", SFE_UBLOX_FILTER_NMEA_RMC,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_THS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NMEA, 0, "UBX_NMEA_THS",
     SFE_UBLOX_FILTER_NMEA_THS, 9999, 120}, // Not supported F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_NMEA_ID_VLW_UART1, UBX_NMEA_VLW, UBX_CLASS_NMEA, 0, "UBX_NMEA_VLW", SFE_UBLOX_FILTER_NMEA_VLW,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, UBX_NMEA_VTG, UBX_CLASS_NMEA, 0, "UBX_NMEA_VTG", SFE_UBLOX_FILTER_NMEA_VTG,
     112, 120},
    {UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_UART1, UBX_NMEA_ZDA, UBX_CLASS_NMEA, 0, "UBX_NMEA_ZDA", SFE_UBLOX_FILTER_NMEA_ZDA,
     112, 120},

    // NMEA NAV2
    // F9P not supported 112, 113, 120. Supported starting 130.
    // F9R not supported 120. Supported starting 130.
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_UART1, UBX_NMEA_GGA, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_GGA",
     SFE_UBLOX_FILTER_NMEA_GGA, 130, 130},
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_UART1, UBX_NMEA_GLL, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_GLL",
     SFE_UBLOX_FILTER_NMEA_GLL, 130, 130},
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_UART1, UBX_NMEA_GNS, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_GNS",
     SFE_UBLOX_FILTER_NMEA_GNS, 130, 130},
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_UART1, UBX_NMEA_GSA, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_GSA",
     SFE_UBLOX_FILTER_NMEA_GSA, 130, 130},
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_UART1, UBX_NMEA_RMC, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_RMC",
     SFE_UBLOX_FILTER_NMEA_RMC, 130, 130},

    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_UART1, UBX_NMEA_VTG, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_VTG",
     SFE_UBLOX_FILTER_NMEA_VTG, 130, 130},
    {UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_UART1, UBX_NMEA_ZDA, UBX_CLASS_NMEA, 0, "UBX_NMEANAV2_ZDA",
     SFE_UBLOX_FILTER_NMEA_ZDA, 130, 130},

    // PUBX
    // F9P support 130
    {UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_PUBX, 0, "UBX_PUBX_POLYP", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_PUBX, 0, "UBX_PUBX_POLYS", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_PUBX, 0, "UBX_PUBX_POLYT", 0, 112, 120},

    // RTCM
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART1, UBX_RTCM_1005, UBX_RTCM_MSB, 1, "UBX_RTCM_1005",
     SFE_UBLOX_FILTER_RTCM_TYPE1005, 112, 9999}, // Not supported on F9R
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART1, UBX_RTCM_1074, UBX_RTCM_MSB, 1, "UBX_RTCM_1074",
     SFE_UBLOX_FILTER_RTCM_TYPE1074, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART1, UBX_RTCM_1077, UBX_RTCM_MSB, 0, "UBX_RTCM_1077",
     SFE_UBLOX_FILTER_RTCM_TYPE1077, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART1, UBX_RTCM_1084, UBX_RTCM_MSB, 1, "UBX_RTCM_1084",
     SFE_UBLOX_FILTER_RTCM_TYPE1084, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART1, UBX_RTCM_1087, UBX_RTCM_MSB, 0, "UBX_RTCM_1087",
     SFE_UBLOX_FILTER_RTCM_TYPE1087, 112, 9999},

    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART1, UBX_RTCM_1094, UBX_RTCM_MSB, 1, "UBX_RTCM_1094",
     SFE_UBLOX_FILTER_RTCM_TYPE1094, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART1, UBX_RTCM_1097, UBX_RTCM_MSB, 0, "UBX_RTCM_1097",
     SFE_UBLOX_FILTER_RTCM_TYPE1097, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART1, UBX_RTCM_1124, UBX_RTCM_MSB, 1, "UBX_RTCM_1124",
     SFE_UBLOX_FILTER_RTCM_TYPE1124, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART1, UBX_RTCM_1127, UBX_RTCM_MSB, 0, "UBX_RTCM_1127",
     SFE_UBLOX_FILTER_RTCM_TYPE1127, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART1, UBX_RTCM_1230, UBX_RTCM_MSB, 10, "UBX_RTCM_1230",
     SFE_UBLOX_FILTER_RTCM_TYPE1230, 112, 9999},

    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_UART1, UBX_RTCM_4072_0, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_0",
     SFE_UBLOX_FILTER_RTCM_TYPE4072_0, 112, 9999},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_UART1, UBX_RTCM_4072_1, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_1",
     SFE_UBLOX_FILTER_RTCM_TYPE4072_1, 112, 9999},

    // AID

    // ESF
    {UBLOX_CFG_MSGOUT_UBX_ESF_ALG_UART1, UBX_ESF_ALG, UBX_CLASS_ESF, 0, "UBX_ESF_ALG", 0, 9999,
     120}, // Not supported on F9P
    {UBLOX_CFG_MSGOUT_UBX_ESF_INS_UART1, UBX_ESF_INS, UBX_CLASS_ESF, 0, "UBX_ESF_INS", 0, 9999, 120},
    {UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_UART1, UBX_ESF_MEAS, UBX_CLASS_ESF, 0, "UBX_ESF_MEAS", 0, 9999, 120},
    {UBLOX_CFG_MSGOUT_UBX_ESF_RAW_UART1, UBX_ESF_RAW, UBX_CLASS_ESF, 0, "UBX_ESF_RAW", 0, 9999, 120},
    {UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_UART1, UBX_ESF_STATUS, UBX_CLASS_ESF, 0, "UBX_ESF_STATUS", 0, 9999, 120},

    // HNR

    // LOG
    // F9P supports LOG_INFO at 112

    // MON
    {UBLOX_CFG_MSGOUT_UBX_MON_COMMS_UART1, UBX_MON_COMMS, UBX_CLASS_MON, 0, "UBX_MON_COMMS", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW2_UART1, UBX_MON_HW2, UBX_CLASS_MON, 0, "UBX_MON_HW2", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW3_UART1, UBX_MON_HW3, UBX_CLASS_MON, 0, "UBX_MON_HW3", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW_UART1, UBX_MON_HW, UBX_CLASS_MON, 0, "UBX_MON_HW", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_IO_UART1, UBX_MON_IO, UBX_CLASS_MON, 0, "UBX_MON_IO", 0, 112, 120},

    {UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_UART1, UBX_MON_MSGPP, UBX_CLASS_MON, 0, "UBX_MON_MSGPP", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_RF_UART1, UBX_MON_RF, UBX_CLASS_MON, 0, "UBX_MON_RF", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_UART1, UBX_MON_RXBUF, UBX_CLASS_MON, 0, "UBX_MON_RXBUF", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_RXR_UART1, UBX_MON_RXR, UBX_CLASS_MON, 0, "UBX_MON_RXR", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_MON_SPAN_UART1, UBX_MON_SPAN, UBX_CLASS_MON, 0, "UBX_MON_SPAN", 0, 113,
     120}, // Not supported F9P 112

    {UBLOX_CFG_MSGOUT_UBX_MON_SYS_UART1, UBX_MON_SYS, UBX_CLASS_MON, 0, "UBX_MON_SYS", 0, 9999,
     130}, // Not supported F9R 121, F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_UART1, UBX_MON_TXBUF, UBX_CLASS_MON, 0, "UBX_MON_TXBUF", 0, 112, 120},

    // NAV2
    // F9P not supported 112, 113, 120. Supported starting 130. F9P 130, 132 supports all but not EELL, PVAT, TIMENAVIC
    // F9R not supported 120. Supported starting 130. F9R 130 supports EELL, PVAT but not SVIN, TIMENAVIC.
    {UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_CLOCK", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_COV_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_COV", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_DOP", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_EELL", 0, 9999,
     130}, // Not supported F9P
    {UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_EOE", 0, 130, 130},

    {UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_ODO", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_POSECEF", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_POSLLH", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_PVAT", 0, 9999,
     130}, // Not supported F9P
    {UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_PVT", 0, 130, 130},

    {UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_SAT", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_SBAS", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_SIG", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_SLAS", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_STATUS", 0, 130, 130},

    //{UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_SVIN", 0, 9999, 9999},
    ////No support yet
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEBDS", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEGAL", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEGLO", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEGPS", 0, 130, 130},

    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMELS", 0, 130, 130},
    //{UBLOX_CFG_MSGOUT_UBX_NAV2_TIMENAVIC_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMENAVIC", 0, 9999,
    // 9999}, //No support yet
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEQZSS", 0, 130,
     130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_TIMEUTC", 0, 130, 130},
    {UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_VELECEF", 0, 130, 130},

    {UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV2_VELNED", 0, 130, 130},

    // NAV
    //{UBLOX_CFG_MSGOUT_UBX_NAV_AOPSTATUS_UART1, UBX_NAV_AOPSTATUS, UBX_CLASS_NAV, 0, "UBX_NAV_AOPSTATUS", 0, 9999,
    // 9999}, //Not supported on F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_ATT_UART1, UBX_NAV_ATT, UBX_CLASS_NAV, 0, "UBX_NAV_ATT", 0, 9999,
     120}, // Not supported on F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_UART1, UBX_NAV_CLOCK, UBX_CLASS_NAV, 0, "UBX_NAV_CLOCK", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_COV_UART1, UBX_NAV_COV, UBX_CLASS_NAV, 0, "UBX_NAV_COV", 0, 112, 120},
    //{UBLOX_CFG_MSGOUT_UBX_NAV_DGPS_UART1, UBX_NAV_DGPS, UBX_CLASS_NAV, 0, "UBX_NAV_DGPS", 0, 9999, 9999}, //Not
    // supported on F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_DOP_UART1, UBX_NAV_DOP, UBX_CLASS_NAV, 0, "UBX_NAV_DOP", 0, 112, 120},

    {UBLOX_CFG_MSGOUT_UBX_NAV_EELL_UART1, UBX_NAV_EELL, UBX_CLASS_NAV, 0, "UBX_NAV_EELL", 0, 9999,
     120}, // Not supported on F9P
    {UBLOX_CFG_MSGOUT_UBX_NAV_EOE_UART1, UBX_NAV_EOE, UBX_CLASS_NAV, 0, "UBX_NAV_EOE", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_UART1, UBX_NAV_GEOFENCE, UBX_CLASS_NAV, 0, "UBX_NAV_GEOFENCE", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_UART1, UBX_NAV_HPPOSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSECEF", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_UART1, UBX_NAV_HPPOSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSLLH", 0, 112, 120},

    //{UBLOX_CFG_MSGOUT_UBX_NAV_NMI_UART1, UBX_NAV_NMI, UBX_CLASS_NAV, 0, "UBX_NAV_NMI", 0, 9999, 9999}, //Not supported
    // on F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_ODO_UART1, UBX_NAV_ODO, UBX_CLASS_NAV, 0, "UBX_NAV_ODO", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_ORB_UART1, UBX_NAV_ORB, UBX_CLASS_NAV, 0, "UBX_NAV_ORB", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_PL_UART1, UBX_NAV_PL, UBX_CLASS_NAV, 0, "UBX_NAV_PL", 0, 9999,
     130}, // Not supported F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_UART1, UBX_NAV_POSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_POSECEF", 0, 112, 120},

    {UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART1, UBX_NAV_POSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_POSLLH", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_UART1, UBX_NAV_PVAT, UBX_CLASS_NAV, 0, "UBX_NAV_PVAT", 0, 9999,
     121}, // Not supported on F9P 112, 113, 120, 130, F9R 120
    {UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1, UBX_NAV_PVT, UBX_CLASS_NAV, 0, "UBX_NAV_PVT", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_UART1, UBX_NAV_RELPOSNED, UBX_CLASS_NAV, 0, "UBX_NAV_RELPOSNED", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_SAT_UART1, UBX_NAV_SAT, UBX_CLASS_NAV, 0, "UBX_NAV_SAT", 0, 112, 120},

    {UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_UART1, UBX_NAV_SBAS, UBX_CLASS_NAV, 0, "UBX_NAV_SBAS", 0, 113,
     120}, // Not supported F9P 112
    {UBLOX_CFG_MSGOUT_UBX_NAV_SIG_UART1, UBX_NAV_SIG, UBX_CLASS_NAV, 0, "UBX_NAV_SIG", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_UART1, UBX_NAV_SLAS, UBX_CLASS_NAV, 0, "UBX_NAV_SLAS", 0, 113,
     130}, // Not supported F9R 121 or F9P 112
           //{UBLOX_CFG_MSGOUT_UBX_NAV_SOL_UART1, UBX_NAV_SOL, UBX_CLASS_NAV, 0, "UBX_NAV_SOL", 0, 9999, 9999}, //Not
           // supported F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART1, UBX_NAV_STATUS, UBX_CLASS_NAV, 0, "UBX_NAV_STATUS", 0, 112, 120},
    //{UBLOX_CFG_MSGOUT_UBX_NAV_SVINFO_UART1, UBX_NAV_SVINFO, UBX_CLASS_NAV, 0, "UBX_NAV_SVINFO", 0, 9999, 9999}, //Not
    // supported F9R 120 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_UART1, UBX_NAV_SVIN, UBX_CLASS_NAV, 0, "UBX_NAV_SVIN", 0, 112,
     9999}, // Not supported on F9R 120, 121, 130
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_UART1, UBX_NAV_TIMEBDS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEBDS", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_UART1, UBX_NAV_TIMEGAL, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGAL", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_UART1, UBX_NAV_TIMEGLO, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGLO", 0, 112, 120},

    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_UART1, UBX_NAV_TIMEGPS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGPS", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_UART1, UBX_NAV_TIMELS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMELS", 0, 112, 120},
    //{UBLOX_CFG_MSGOUT_UBX_NAV_TIMENAVIC_UART1, UBX_NAV_TIMENAVIC, UBX_CLASS_NAV, 0, "UBX_NAV_TIMENAVIC", 0, 9999,
    // 9999}, //Not supported F9R 121 or F9P 132
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_UART1, UBX_ID_NOT_AVAILABLE, UBX_CLASS_NAV, 0, "UBX_NAV_QZSS", 0, 113,
     130}, // Not supported F9R 121 or F9P 112
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_UART1, UBX_NAV_TIMEUTC, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEUTC", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_UART1, UBX_NAV_VELECEF, UBX_CLASS_NAV, 0, "UBX_NAV_VELECEF", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_UART1, UBX_NAV_VELNED, UBX_CLASS_NAV, 0, "UBX_NAV_VELNED", 0, 112, 120},

    // RXM
    //{UBLOX_CFG_MSGOUT_UBX_RXM_ALM_UART1, UBX_RXM_ALM, UBX_CLASS_RXM, 0, "UBX_RXM_ALM", 0, 9999, 9999}, //Not supported
    // F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_RXM_COR_UART1, UBX_RXM_COR, UBX_CLASS_RXM, 0, "UBX_RXM_COR", 0, 9999,
     130}, // Not supported F9R 121 or F9P 112, 113, 120, 130, 132
           //{UBLOX_CFG_MSGOUT_UBX_RXM_EPH_UART1, UBX_RXM_EPH, UBX_CLASS_RXM, 0, "UBX_RXM_EPH", 0, 9999, 9999}, //Not
           // supported F9R 121 or F9P 112, 113, 120, 130, 132 {UBLOX_CFG_MSGOUT_UBX_RXM_IMES_UART1, UBX_RXM_IMES,
    // UBX_CLASS_RXM, 0, "UBX_RXM_IMES", 0, 9999, 9999}, //Not supported F9R 121 or F9P 112, 113, 120, 130, 132
    //{UBLOX_CFG_MSGOUT_UBX_RXM_MEAS20_UART1, UBX_RXM_MEAS20, UBX_CLASS_RXM, 0, "UBX_RXM_MEAS20", 0, 9999, 9999},
    ////Not supported F9R 121 or F9P 112, 113, 120, 130, 132 {UBLOX_CFG_MSGOUT_UBX_RXM_MEAS50_UART1,
    // UBX_RXM_MEAS50, UBX_CLASS_RXM, 0, "UBX_RXM_MEAS50", 0, 9999, 9999}, //Not supported F9R 121 or F9P 112,
    // 113, 120, 130, 132 {UBLOX_CFG_MSGOUT_UBX_RXM_MEASC12_UART1, UBX_RXM_MEASC12, UBX_CLASS_RXM, 0,
    //"UBX_RXM_MEASC12", 0, 9999, 9999}, //Not supported F9R 121 or F9P 112, 113, 120, 130, 132
    //{UBLOX_CFG_MSGOUT_UBX_RXM_MEASD12_UART1, UBX_RXM_MEASD12, UBX_CLASS_RXM, 0, "UBX_RXM_MEASD12", 0, 9999,
    // 9999}, //Not supported F9R 121 or F9P 112, 113, 120, 130, 132
    {UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_UART1, UBX_RXM_MEASX, UBX_CLASS_RXM, 0, "UBX_RXM_MEASX", 0, 112, 120},
    //{UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART1, UBX_RXM_PMP, UBX_CLASS_RXM, 0, "UBX_RXM_PMP", 0, 9999, 9999}, //Not supported
    // F9R 121 or F9P 112, 113, 120, 130, 132 {UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_UART1, UBX_RXM_QZSSL6, UBX_CLASS_RXM, 0,
    //"UBX_RXM_QZSSL6", 0, 9999, 9999}, //Not supported F9R 121, F9P 112, 113, 120, 130
    {UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART1, UBX_RXM_RAWX, UBX_CLASS_RXM, 0, "UBX_RXM_RAWX", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_RXM_RLM_UART1, UBX_RXM_RLM, UBX_CLASS_RXM, 0, "UBX_RXM_RLM", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_UART1, UBX_RXM_RTCM, UBX_CLASS_RXM, 0, "UBX_RXM_RTCM", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART1, UBX_RXM_SFRBX, UBX_CLASS_RXM, 0, "UBX_RXM_SFRBX", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_UART1, UBX_RXM_SPARTN, UBX_CLASS_RXM, 0, "UBX_RXM_SPARTN", 0, 9999,
     121}, // Not supported F9R 120 or F9P 112, 113, 120, 130
           //{UBLOX_CFG_MSGOUT_UBX_RXM_SVSI_UART1, UBX_RXM_SVSI, UBX_CLASS_RXM, 0, "UBX_RXM_SVSI", 0, 9999, 9999}, //Not
           // supported F9R 121 or F9P 112, 113, 120, 130, 132 {UBLOX_CFG_MSGOUT_UBX_RXM_TM_UART1, UBX_RXM_TM,
    // UBX_CLASS_RXM, 0, "UBX_RXM_TM", 0, 9999, 9999}, //Not supported F9R 121 or F9P 112, 113, 120, 130, 132

    // SEC
    // No support F9P 112.

    // TIM
    //{UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_UART1, UBX_TIM_SVIN, UBX_CLASS_TIM, 0, "UBX_TIM_SVIN", 0, 9999, 9999}, //Appears on
    // F9P 132 but not supported
    {UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART1, UBX_TIM_TM2, UBX_CLASS_TIM, 0, "UBX_TIM_TM2", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_TIM_TP_UART1, UBX_TIM_TP, UBX_CLASS_TIM, 0, "UBX_TIM_TP", 0, 112, 120},
    {UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_UART1, UBX_TIM_VRFY, UBX_CLASS_TIM, 0, "UBX_TIM_VRFY", 0, 112, 120},

};

#define MAX_UBX_MSG (sizeof(ubxMessages) / sizeof(ubxMsg))
#define MAX_UBX_MSG_RTCM (12)

#define MAX_SET_MESSAGES_RETRIES 5 // Try up to five times to set all the messages. Occasionally fails if set to 2

// Struct to describe the necessary info for each UBX command
// Each command will have a key, and minimum F9P/F9R versions that support that command
typedef struct
{
    const uint32_t cmdKey;
    const char cmdTextName[30];
    const uint16_t f9pFirmwareVersionSupported; // The minimum version this message is supported. 0 = all versions. 9999
                                                // = Not supported
    const uint16_t f9rFirmwareVersionSupported;
} ubxCmd;

// Static array containing all the compatible commands
const ubxCmd ubxCommands[] = {
    {UBLOX_CFG_TMODE_MODE, "CFG_TMODE_MODE", 0, 9999}, // Survey mode is only available on ZED-F9P modules

    //The F9R is unique WRT RTCM *output*. u-center can correctly enable/disable the RTCM output, but it cannot
    //be set with setVal commands. Applies to HPS 120, 121, 130.

    {UBLOX_CFG_UART1OUTPROT_RTCM3X, "CFG_UART1OUTPROT_RTCM3X", 0, 9999}, // F9R: RTCM output not supported
    {UBLOX_CFG_UART1INPROT_SPARTN, "CFG_UART1INPROT_SPARTN", 120,
     121}, // Supported on F9P 120 and up. F9R: SPARTN supported starting HPS 121

    {UBLOX_CFG_UART2OUTPROT_RTCM3X, "CFG_UART2OUTPROT_RTCM3X", 0, 9999}, // F9R: RTCM output not supported
    {UBLOX_CFG_UART2INPROT_SPARTN, "CFG_UART2INPROT_SPARTN", 120, 121}, // F9R: SPARTN supported starting HPS 121

    {UBLOX_CFG_SPIOUTPROT_RTCM3X, "CFG_SPIOUTPROT_RTCM3X", 0, 9999}, // F9R: RTCM output not supported
    {UBLOX_CFG_SPIINPROT_SPARTN, "CFG_SPIINPROT_SPARTN", 120, 121}, // F9R: SPARTN supported starting HPS 121

    {UBLOX_CFG_I2COUTPROT_RTCM3X, "CFG_I2COUTPROT_RTCM3X", 0, 9999}, // F9R: RTCM output not supported
    {UBLOX_CFG_I2CINPROT_SPARTN, "CFG_I2CINPROT_SPARTN", 120, 121}, // F9R: SPARTN supported starting HPS 121

    {UBLOX_CFG_USBOUTPROT_RTCM3X, "CFG_USBOUTPROT_RTCM3X", 0, 9999}, // F9R: RTCM output not supported
    {UBLOX_CFG_USBINPROT_SPARTN, "CFG_USBINPROT_SPARTN", 120, 121}, // F9R: SPARTN supported starting HPS 121

    {UBLOX_CFG_NAV2_OUT_ENABLED, "CFG_NAV2_OUT_ENABLED", 130,
     130}, // Supported on F9P 130 and up. Supported on F9R 130 and up.
    {UBLOX_CFG_NAVSPG_INFIL_MINCNO, "CFG_NAVSPG_INFIL_MINCNO", 0, 0}, //
};

#define MAX_UBX_CMD (sizeof(ubxCommands) / sizeof(ubxCmd))

// Regional Support
// Do we want the user to be able to specify which region they are in?
// Or do we want to figure it out based on position?
// If we define a simple 'square' area for each region, we can do both.
// Note: the best way to obtain the L-Band frequencies would be from the MQTT /pp/frequencies/Lb topic.
//       But it is easier to record them here, in case we don't have access to MQTT...
// Note: the key distribution topic is provided during ZTP. We don't need to record it here.

typedef struct
{
    const double latNorth; // Degrees
    const double latSouth; // Degrees
    const double lonEast; // Degrees
    const double lonWest; // Degrees
} Regional_Area;

typedef struct
{
    const char *name; // As defined in the ZTP subscriptions description: EU, US, KR, AU, Japan
    const char *topicRegion; // As used in the corrections topic path
    const Regional_Area area;
    const uint32_t frequency; // L-Band frequency, Hz, if supported. 0 if not supported
} Regional_Information;

const Regional_Information Regional_Information_Table[] =
{
    { "US", "us", { 50.0,  25.0, -60.0, -125.0}, 1556290000 },
    { "EU", "eu", { 72.0,  36.0,  32.0,  -11.0}, 1545260000 },
    // Note: we only include regions with L-Band coverage. AU, KR and Japan are not included here.
};
const int numRegionalAreas = sizeof(Regional_Information_Table) / sizeof(Regional_Information_Table[0]);

// This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
typedef struct
{
    int sizeOfSettings = 0;             // sizeOfSettings **must** be the first entry and must be int
    int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
    bool printDebugMessages = false;
    bool enableSD = true;
    bool enableDisplay = true;
    int maxLogTime_minutes = 60 * 24;        // Default to 24 hours
    int observationSeconds = 60;             // Default survey in time of 60 seconds
    float observationPositionAccuracy = 5.0; // Default survey in pos accy of 5m
    bool fixedBase = false;                  // Use survey-in by default
    bool fixedBaseCoordinateType = COORD_TYPE_ECEF;
    double fixedEcefX = -1280206.568;
    double fixedEcefY = -4716804.403;
    double fixedEcefZ = 4086665.484;
    double fixedLat = 40.09029479;
    double fixedLong = -105.18505761;
    double fixedAltitude = 1560.089;
    uint32_t dataPortBaud =
        (115200 * 2); // Default to 230400bps. This limits GNSS fixes at 4Hz but allows SD buffer to be reduced to 6k.
    uint32_t radioPortBaud = 57600;       // Default to 57600bps to support connection to SiK1000 type telemetry radios
    float surveyInStartingAccuracy = 1.0; // Wait for 1m horizontal positional accuracy before starting survey in
    uint16_t measurementRate = 250;       // Elapsed ms between GNSS measurements. 25ms to 65535ms. Default 4Hz.
    uint16_t navigationRate =
        1; // Ratio between number of measurements and navigation solutions. Default 1 for 4Hz (with measurementRate).
    bool enableI2Cdebug = false;                          // Turn on to display GNSS library debug messages
    bool enableHeapReport = false;                        // Turn on to display free heap
    bool enableTaskReports = false;                       // Turn on to display task high water marks
    muxConnectionType_e dataPortChannel = MUX_UBLOX_NMEA; // Mux default to ublox UART1
    uint16_t spiFrequency = 16;                           // By default, use 16MHz SPI
    bool enableLogging = true;                            // If an SD card is present, log default sentences
    bool enableARPLogging = false;      // Log the Antenna Reference Position from RTCM 1005/1006 - if available
    uint16_t ARPLoggingInterval_s = 10; // Log the ARP every 10 seconds - if available
    uint16_t sppRxQueueSize = 512 * 4;
    uint16_t sppTxQueueSize = 32;
    uint8_t dynamicModel = DYN_MODEL_PORTABLE;
    SystemState lastState = STATE_NOT_SET; // Start unit in last known state
    bool enableSensorFusion = false; // If IMU is available, avoid using it unless user specifically selects automotive
    bool autoIMUmountAlignment = true; // Allows unit to automatically establish device orientation in vehicle
    bool enableResetDisplay = false;
    uint8_t resetCount = 0;
    bool enableExternalPulse = true;                           // Send pulse once lock is achieved
    uint64_t externalPulseTimeBetweenPulse_us = 1000000;       // us between pulses, max of 60s = 60 * 1000 * 1000
    uint64_t externalPulseLength_us = 100000;                  // us length of pulse, max of 60s = 60 * 1000 * 1000
    pulseEdgeType_e externalPulsePolarity = PULSE_RISING_EDGE; // Pulse rises for pulse length, then falls
    bool enableExternalHardwareEventLogging = false;           // Log when INT/TM2 pin goes low
    bool enableMarksFile = false;                              // Log marks to the marks file
    bool enableUART2UBXIn = false;                             // UBX Protocol In on UART2

    uint8_t ubxMessageRates[MAX_UBX_MSG] = {254}; // Mark first record with key so defaults will be applied.

    // Constellations monitored/used for fix
    ubxConstellation ubxConstellations[MAX_CONSTELLATIONS] = {
        {UBLOX_CFG_SIGNAL_GPS_ENA, SFE_UBLOX_GNSS_ID_GPS, true, "GPS"},
        {UBLOX_CFG_SIGNAL_SBAS_ENA, SFE_UBLOX_GNSS_ID_SBAS, true, "SBAS"},
        {UBLOX_CFG_SIGNAL_GAL_ENA, SFE_UBLOX_GNSS_ID_GALILEO, true, "Galileo"},
        {UBLOX_CFG_SIGNAL_BDS_ENA, SFE_UBLOX_GNSS_ID_BEIDOU, true, "BeiDou"},
        //{UBLOX_CFG_SIGNAL_QZSS_ENA, SFE_UBLOX_GNSS_ID_IMES, false, "IMES"}, //Not yet supported? Config key does not
        // exist?
        {UBLOX_CFG_SIGNAL_QZSS_ENA, SFE_UBLOX_GNSS_ID_QZSS, true, "QZSS"},
        {UBLOX_CFG_SIGNAL_GLO_ENA, SFE_UBLOX_GNSS_ID_GLONASS, true, "GLONASS"},
    };

    int maxLogLength_minutes = 60 * 24; // Default to 24 hours
    char profileName[50] = "";

    int16_t serialTimeoutGNSS = 1; // In ms - used during SerialGNSS.begin. Number of ms to pass of no data before
                                   // hardware serial reports data available.

    // Point Perfect
    char pointPerfectDeviceProfileToken[40] = "";
    bool enablePointPerfectCorrections = true;
    bool autoKeyRenewal = true; // Attempt to get keys if we get under 28 days from the expiration date
    char pointPerfectClientID[50] = "";
    char pointPerfectBrokerHost[50] = ""; // pp.services.u-blox.com
    char pointPerfectLBandTopic[20] = ""; // /pp/ubx/0236/Lb

    char pointPerfectCurrentKey[33] = ""; // 32 hexadecimal digits = 128 bits = 16 Bytes
    uint64_t pointPerfectCurrentKeyDuration = 0;
    uint64_t pointPerfectCurrentKeyStart = 0;

    char pointPerfectNextKey[33] = "";
    uint64_t pointPerfectNextKeyDuration = 0;
    uint64_t pointPerfectNextKeyStart = 0;

    uint64_t lastKeyAttempt = 0;     // Epoch time of last attempt at obtaining keys
    bool updateZEDSettings = true;   // When in doubt, update the ZED with current settings

    bool debugPpCertificate = false; // Debug Point Perfect certificate management

    // Time Zone - Default to UTC
    int8_t timeZoneHours = 0;
    int8_t timeZoneMinutes = 0;
    int8_t timeZoneSeconds = 0;

    // Debug settings
    bool enablePrintState = false;
    bool enablePrintPosition = false;
    bool enablePrintIdleTime = false;
    bool enablePrintBatteryMessages = true;
    bool enablePrintRoverAccuracy = true;
    bool enablePrintBadMessages = false;
    bool enablePrintLogFileMessages = false;
    bool enablePrintLogFileStatus = true;
    bool enablePrintRingBufferOffsets = false;
    bool enablePrintStates = true;
    bool enablePrintDuplicateStates = false;
    bool enablePrintRtcSync = false;
    RadioType_e radioType = RADIO_EXTERNAL;
    uint8_t espnowPeers[5][6] = {0}; // Max of 5 peers. Contains the MAC addresses (6 bytes) of paired units
    uint8_t espnowPeerCount = 0;
    bool enableRtcmMessageChecking = false;
    BluetoothRadioType_e bluetoothRadioType = BLUETOOTH_RADIO_SPP;
    bool runLogTest = false;           // When set to true, device will create a series of test logs
    bool espnowBroadcast = true;       // When true, overrides peers and sends all data via broadcast
    int16_t antennaHeight = 0;         // in mm
    float antennaReferencePoint = 0.0; // in mm
    bool echoUserInput = true;
    int uartReceiveBufferSize = 1024 * 2; // This buffer is filled automatically as the UART receives characters.
    int gnssHandlerBufferSize =
        1024 * 4; // This buffer is filled from the UART receive buffer, and is then written to SD

    bool enablePrintBufferOverrun = false;
    bool enablePrintSDBuffers = false;
    PeriodicDisplay_t periodicDisplay = (PeriodicDisplay_t)0; // Turn off all periodic debug displays by default.
    uint32_t periodicDisplayInterval = 15 * 1000;

    uint32_t rebootSeconds = (uint32_t)-1; // Disabled, reboots after uptime reaches this number of seconds
    bool forceResetOnSDFail = false;       // Set to true to reset system if SD is detected but fails to start.

    uint8_t minElev = 10; // Minimum elevation (in deg) for a GNSS satellite to be used in NAV
    uint8_t ubxMessageRatesBase[MAX_UBX_MSG_RTCM] = {
        254}; // Mark first record with key so defaults will be applied. Int value for each supported message - Report
              // rates for RTCM Base. Default to u-blox recommended rates.
    uint32_t imuYaw = 0;  // User defined IMU mount yaw angle (0 to 36,000) CFG-SFIMU-IMU_MNTALG_YAW
    int16_t imuPitch = 0; // User defined IMU mount pitch angle (-9000 to 9000) CFG-SFIMU-IMU_MNTALG_PITCH
    int16_t imuRoll = 0;  // User defined IMU mount roll angle (-18000 to 18000) CFG-SFIMU-IMU_MNTALG_ROLL
    bool sfDisableWheelDirection = false; // CFG-SFODO-DIS_AUTODIRPINPOL
    bool sfCombineWheelTicks = false;     // CFG-SFODO-COMBINE_TICKS
    uint8_t rateNavPrio = 0;              // Output rate of priority nav mode message - CFG-RATE-NAV_PRIO
    // CFG-SFIMU-AUTO_MNTALG_ENA 0 = autoIMUmountAlignment
    bool sfUseSpeed = false; // CFG-SFODO-USE_SPEED

    CoordinateInputType coordinateInputType = COORDINATE_INPUT_TYPE_DD; // Default DD.ddddddddd
    uint16_t lbandFixTimeout_seconds = 180; // Number of seconds of no L-Band fix before resetting ZED
    int16_t minCNO_F9P = 6;                 // Minimum satellite signal level for navigation. ZED-F9P default is 6 dBHz
    int16_t minCNO_F9R = 20;                // Minimum satellite signal level for navigation. ZED-F9R default is 20 dBHz
    uint16_t serialGNSSRxFullThreshold = 50; // RX FIFO full interrupt. Max of ~128. See pinUART2Task().
    uint8_t btReadTaskPriority = 1; // Read from BT SPP and Write to GNSS. 3 being the highest, and 0 being the lowest
    uint8_t gnssReadTaskPriority =
        1; // Read from ZED-F9x and Write to circular buffer (SD, TCP, BT). 3 being the highest, and 0 being the lowest
    uint8_t handleGnssDataTaskPriority = 1; // Read from the cicular buffer and dole out to end points (SD, TCP, BT).
    uint8_t btReadTaskCore = 1;             // Core where task should run, 0=core, 1=Arduino
    uint8_t gnssReadTaskCore = 1;           // Core where task should run, 0=core, 1=Arduino
    uint8_t handleGnssDataTaskCore = 1;     // Core where task should run, 0=core, 1=Arduino
    uint8_t gnssUartInterruptsCore =
        1; // Core where hardware is started and interrupts are assigned to, 0=core, 1=Arduino
    uint8_t bluetoothInterruptsCore =
        1;                         // Core where hardware is started and interrupts are assigned to, 0=core, 1=Arduino
    uint8_t i2cInterruptsCore = 1; // Core where hardware is started and interrupts are assigned to, 0=core, 1=Arduino
    uint32_t shutdownNoChargeTimeout_s = 0; // If > 0, shut down unit after timeout if not charging
    bool disableSetupButton = false;        // By default, allow setup through the overlay button(s)
    bool useI2cForLbandCorrections =
        true; // Set to false to stop I2C callback. Corrections will require direct ZED to NEO UART2 connections.
    bool useI2cForLbandCorrectionsConfigured = false; // If a user sets useI2cForLbandCorrections, this goes true.

    // Ethernet
    bool enablePrintEthernetDiag = false;
    bool ethernetDHCP = true;
    IPAddress ethernetIP = {192, 168, 0, 123};
    IPAddress ethernetDNS = {194, 168, 4, 100};
    IPAddress ethernetGateway = {192, 168, 0, 1};
    IPAddress ethernetSubnet = {255, 255, 255, 0};
    uint16_t httpPort = 80;

    // WiFi
    bool debugWifiState = false;
    bool wifiConfigOverAP = true; // Configure device over Access Point or have it connect to WiFi
    WiFiNetwork wifiNetworks[MAX_WIFI_NETWORKS] = {
        {"", ""},
        {"", ""},
        {"", ""},
        {"", ""},
    };

    // Network layer
    uint8_t defaultNetworkType = NETWORK_TYPE_USE_DEFAULT;
    bool debugNetworkLayer = false;    // Enable debugging of the network layer
    bool enableNetworkFailover = true; // Enable failover between Ethernet / WiFi
    bool printNetworkStatus = true;    // Print network status (delays, failovers, IP address)

    // Multicast DNS Server
    bool mdnsEnable = true; // Allows locating of device from browser address 'rtk.local'

    // NTP
    bool debugNtp = false;
    uint16_t ethernetNtpPort = 123;
    bool enableNTPFile = false;  // Log NTP requests to file
    uint8_t ntpPollExponent = 6; // NTPpacket::defaultPollExponent 2^6 = 64 seconds
    int8_t ntpPrecision = -20;   // NTPpacket::defaultPrecision 2^-20 = 0.95us
    uint32_t ntpRootDelay = 0;   // NTPpacket::defaultRootDelay = 0. ntpRootDelay is defined in microseconds.
                                 // ntpProcessOneRequest will convert it to seconds and fraction.
    uint32_t ntpRootDispersion =
        1000; // NTPpacket::defaultRootDispersion 1007us = 2^-16 * 66. ntpRootDispersion is defined in microseconds.
              // ntpProcessOneRequest will convert it to seconds and fraction.
    char ntpReferenceId[5] = {'G', 'P', 'S', 0,
                              0}; // NTPpacket::defaultReferenceId. Ref ID is 4 chars. Add one extra for a NULL.

    // NTRIP Client
    bool debugNtripClientRtcm = false;
    bool debugNtripClientState = false;
    bool enableNtripClient = false;
    char ntripClient_CasterHost[50] = "rtk2go.com"; // It's free...
    uint16_t ntripClient_CasterPort = 2101;
    char ntripClient_CasterUser[50] =
        "test@test.com"; // Some free casters require auth. User must provide their own email address to use RTK2Go
    char ntripClient_CasterUserPW[50] = "";
    char ntripClient_MountPoint[50] = "bldr_SparkFun1";
    char ntripClient_MountPointPW[50] = "";
    bool ntripClient_TransmitGGA = true;

    // NTRIP Server
    bool debugNtripServerRtcm = false;
    bool debugNtripServerState = false;
    bool enableNtripServer = false;
    bool ntripServer_StartAtSurveyIn = false;       // true = Start WiFi instead of Bluetooth at Survey-In
    char ntripServer_CasterHost[NTRIP_SERVER_MAX][50] = // It's free...
    {
        "rtk2go.com",
        "",
    };
    uint16_t ntripServer_CasterPort[NTRIP_SERVER_MAX] =
    {
        2101,
        0,
    };
    char ntripServer_CasterUser[NTRIP_SERVER_MAX][50] =
    {
        "test@test.com" // Some free casters require auth. User must provide their own email address to use RTK2Go
        "",
    };
    char ntripServer_CasterUserPW[NTRIP_SERVER_MAX][50] =
    {
        "",
        "",
    };
    char ntripServer_MountPoint[NTRIP_SERVER_MAX][50] =
    {
        "bldr_dwntwn2", // NTRIP Server
        "",
    };
    char ntripServer_MountPointPW[NTRIP_SERVER_MAX][50] =
    {
        "WR5wRo4H",
        "",
    };

    // TCP Client
    bool debugPvtClient = false;
    bool enablePvtClient = false;
    uint16_t pvtClientPort = 2948; // PVT client port. 2948 is GPS Daemon: http://tcp-udp-ports.com/port-2948.htm
    char pvtClientHost[50] = "";

    // TCP Server
    bool debugPvtServer = false;
    bool enablePvtServer = false;
    uint16_t pvtServerPort = 2948; // PVT server port, 2948 is GPS Daemon: http://tcp-udp-ports.com/port-2948.htm

    // UDP Server
    bool debugPvtUdpServer = false;
    bool enablePvtUdpServer = false;
    uint16_t pvtUdpServerPort =
        10110; // https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=nmea

    uint8_t rtcmTimeoutBeforeUsingLBand_s =
        10; // If 10s have passed without RTCM, enable PMP corrections over L-Band if available

    // Automatic Firmware Update
    bool debugFirmwareUpdate = false;
    bool enableAutoFirmwareUpdate = false;
    uint32_t autoFirmwareCheckMinutes = 24 * 60;

    bool debugLBand = false;
    bool enableCaptivePortal = true;
    bool enableZedUsb = true; //Can be used to disable ZED USB config

    bool debugWiFiConfig = false;

    int geographicRegion = 0; // Default to US - first entry in Regional_Information_Table

    // Add new settings above <------------------------------------------------------------>

} Settings;
Settings settings;

// Monitor which devices on the device are on or offline.
struct struct_online
{
    bool microSD = false;
    bool display = false;
    bool gnss = false;
    bool logging = false;
    bool serialOutput = false;
    bool fs = false;
    bool rtc = false;
    bool battery = false;
    bool accelerometer = false;
    bool ntripClient = false;
    bool ntripServer[NTRIP_SERVER_MAX] = {false, false};
    bool lband = false;
    bool lbandCorrections = false;
    bool i2c = false;
    bool pvtClient = false;
    bool pvtServer = false;
    bool pvtUdpServer = false;
    ethernetStatus_e ethernetStatus = ETH_NOT_STARTED;
    bool NTPServer = false; // EthernetUDP
    bool otaFirmwareUpdate = false;
} online;

#ifdef COMPILE_WIFI
#ifdef COMPILE_L_BAND
// AWS certificate for PointPerfect API
static const char *AWS_PUBLIC_CERT = R"=====(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)=====";
#endif // COMPILE_L_BAND
#endif // COMPILE_WIFI
#endif // __SETTINGS_H__
