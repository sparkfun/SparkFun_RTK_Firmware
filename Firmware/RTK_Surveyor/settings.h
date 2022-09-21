//System can enter a variety of states
//See statemachine diagram at: https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
typedef enum
{
  STATE_ROVER_NOT_STARTED = 0,
  STATE_ROVER_NO_FIX,
  STATE_ROVER_FIX,
  STATE_ROVER_RTK_FLOAT,
  STATE_ROVER_RTK_FIX,
  STATE_BASE_NOT_STARTED,
  STATE_BASE_TEMP_SETTLE, //User has indicated base, but current pos accuracy is too low
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
  STATE_KEYS_PROVISION_WIFI_TIMEOUT,
  STATE_ESPNOW_PAIRING_NOT_STARTED,
  STATE_ESPNOW_PAIRING,
  STATE_SHUTDOWN,
} SystemState;
volatile SystemState systemState = STATE_ROVER_NOT_STARTED;
SystemState lastSystemState = STATE_ROVER_NOT_STARTED;
SystemState requestedSystemState = STATE_ROVER_NOT_STARTED;
bool newSystemStateRequested = false;

//The setup display can show a limited set of states
//When user pauses for X amount of time, system will enter that state
SystemState setupState = STATE_MARK_EVENT;

typedef enum
{
  RTK_SURVEYOR = 0,
  RTK_EXPRESS,
  RTK_FACET,
  RTK_EXPRESS_PLUS,
  RTK_FACET_LBAND,
} ProductVariant;
ProductVariant productVariant = RTK_SURVEYOR;

typedef enum
{
  BUTTON_ROVER = 0,
  BUTTON_BASE,
} ButtonState;
ButtonState buttonPreviousState = BUTTON_ROVER;

//Data port mux (RTK Express) can enter one of four different connections
typedef enum muxConnectionType_e
{
  MUX_UBLOX_NMEA = 0,
  MUX_PPS_EVENTTRIGGER,
  MUX_I2C_WT,
  MUX_ADC_DAC,
} muxConnectionType_e;

//User can enter fixed base coordinates in ECEF or degrees
typedef enum
{
  COORD_TYPE_ECEF = 0,
  COORD_TYPE_GEODETIC,
} coordinateType_e;

//User can select output pulse as either falling or rising edge
typedef enum
{
  PULSE_FALLING_EDGE = 0,
  PULSE_RISING_EDGE,
} pulseEdgeType_e;

//Custom NMEA sentence types output to the log file
typedef enum
{
  CUSTOM_NMEA_TYPE_RESET_REASON = 0,
  CUSTOM_NMEA_TYPE_WAYPOINT,
  CUSTOM_NMEA_TYPE_EVENT,
  CUSTOM_NMEA_TYPE_SYSTEM_VERSION,
  CUSTOM_NMEA_TYPE_ZED_VERSION,
  CUSTOM_NMEA_TYPE_STATUS,
  CUSTOM_NMEA_TYPE_LOGTEST_STATUS,
} customNmeaType_e;

//Freeze and blink LEDs if we hit a bad error
typedef enum
{
  ERROR_NO_I2C = 2, //Avoid 0 and 1 as these are bad blink codes
  ERROR_GPS_CONFIG_FAIL,
} t_errorNumber;

//Even though WiFi and ESP-Now operate on the same radio, we treat
//then as different states so that we can leave the radio on if
//either WiFi or ESP-Now are active
enum WiFiState
{
  WIFI_OFF = 0,
  WIFI_ON,
  WIFI_NOTCONNECTED,
  WIFI_CONNECTED,
};
volatile byte wifiState = WIFI_OFF;

typedef enum ESPNOWState
{
  ESPNOW_OFF,
  ESPNOW_ON,
  ESPNOW_PAIRING,
  ESPNOW_MAC_RECEIVED,
  ESPNOW_PAIRED,
} ESPNOWState;
volatile ESPNOWState espnowState = ESPNOW_OFF;

enum NTRIPClientState
{
  NTRIP_CLIENT_OFF = 0,         //Using Bluetooth or NTRIP server
  NTRIP_CLIENT_ON,              //WIFI_ON state
  NTRIP_CLIENT_WIFI_CONNECTING, //Connecting to WiFi access point
  NTRIP_CLIENT_WIFI_CONNECTED,  //WiFi connected to an access point
  NTRIP_CLIENT_CONNECTING,      //Attempting a connection to the NTRIP caster
  NTRIP_CLIENT_CONNECTED,       //Connected to the NTRIP caster
};
volatile byte ntripClientState = NTRIP_CLIENT_OFF;

enum NTRIPServerState
{
  NTRIP_SERVER_OFF = 0,         //Using Bluetooth or NTRIP client
  NTRIP_SERVER_ON,              //WIFI_ON state
  NTRIP_SERVER_WIFI_CONNECTING, //Connecting to WiFi access point
  NTRIP_SERVER_WIFI_CONNECTED,  //WiFi connected to an access point
  NTRIP_SERVER_WAIT_GNSS_DATA,  //Waiting for correction data from GNSS
  NTRIP_SERVER_CONNECTING,      //Attempting a connection to the NTRIP caster
  NTRIP_SERVER_AUTHORIZATION,   //Validate the credentials
  NTRIP_SERVER_CASTING,         //Sending correction data to the NTRIP caster
};
volatile byte ntripServerState = NTRIP_SERVER_OFF;

enum RtcmTransportState
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
};

typedef enum RadioType_e
{
  RADIO_EXTERNAL = 0,
  RADIO_ESPNOW,
} RadioType_e;

typedef enum BluetoothRadioType_e
{
  BLUETOOTH_RADIO_SPP = 0,
  BLUETOOTH_RADIO_BLE,
  BLUETOOTH_RADIO_OFF,
} BluetoothRadioType_e;

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
} ;
uint8_t logTestState = LOGTEST_END;

//Radio status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum BTState
{
  BT_OFF = 0,
  BT_NOTCONNECTED,
  BT_CONNECTED,
};

//Return values for getByteChoice()
enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X = 254,
};

//Return values for getMenuChoice()
enum getMenuChoiceStatus
{
  GMCS_TIMEOUT = -2,
  GMCS_OVERFLOW = -1,
  GMCS_CHARACTER = 0
};

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

//Each constellation will have its config key, enable, and a visible name
typedef struct ubxConstellation
{
  uint32_t configKey;
  uint8_t gnssID;
  bool enabled;
  char textName[30];
} ubxConstellation;

//These are the allowable constellations to receive from and log (if enabled)
//Tested with u-center v21.02
#define MAX_CONSTELLATIONS 6 //(sizeof(ubxConstellations)/sizeof(ubxConstellation))


//Different ZED modules support different messages (F9P vs F9R vs F9T)
//Create binary packed struct for different platforms
typedef enum ubxPlatform
{
  PLATFORM_F9P = 0b0001,
  PLATFORM_F9R = 0b0010,
  PLATFORM_F9T = 0b0100,
} ubxPlatform;

//Each message will have a rate, a visible name, and a class
typedef struct ubxMsg
{
  uint32_t msgConfigKey;
  uint8_t msgID;
  uint8_t msgClass;
  uint8_t msgRate;
  char msgTextName[30];
  uint8_t supported;
} ubxMsg;

//These are the allowable messages to broadcast and log (if enabled)
//Tested with u-center v21.02
#define MAX_UBX_MSG (13 + 25 + 5 + 10 + 3 + 12 + 5) //(sizeof(ubxMessages)/sizeof(ubxMsg))

//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
typedef struct {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
  bool printDebugMessages = false;
  bool enableSD = true;
  bool enableDisplay = true;
  int maxLogTime_minutes = 60 * 24; //Default to 24 hours
  int observationSeconds = 60; //Default survey in time of 60 seconds
  float observationPositionAccuracy = 5.0; //Default survey in pos accy of 5m
  bool fixedBase = false; //Use survey-in by default
  bool fixedBaseCoordinateType = COORD_TYPE_ECEF;
  double fixedEcefX = -1280206.568;
  double fixedEcefY = -4716804.403;
  double fixedEcefZ = 4086665.484;
  double fixedLat = 40.09029479;
  double fixedLong = -105.18505761;
  double fixedAltitude = 1560.089;
  uint32_t dataPortBaud = 460800; //Default to 460800bps to support >10Hz update rates
  uint32_t radioPortBaud = 57600; //Default to 57600bps to support connection to SiK1000 radios
  float surveyInStartingAccuracy = 1.0; //Wait for 1m horizontal positional accuracy before starting survey in
  uint16_t measurementRate = 250; //Elapsed ms between GNSS measurements. 25ms to 65535ms. Default 4Hz.
  uint16_t navigationRate = 1; //Ratio between number of measurements and navigation solutions. Default 1 for 4Hz (with measurementRate).
  bool enableI2Cdebug = false; //Turn on to display GNSS library debug messages
  bool enableHeapReport = false; //Turn on to display free heap
  bool enableTaskReports = false; //Turn on to display task high water marks
  muxConnectionType_e dataPortChannel = MUX_UBLOX_NMEA; //Mux default to ublox UART1
  uint16_t spiFrequency = 16; //By default, use 16MHz SPI
  bool enableLogging = true; //If an SD card is present, log default sentences
  uint16_t sppRxQueueSize = 2048;
  uint16_t sppTxQueueSize = 512;
  uint8_t dynamicModel = DYN_MODEL_PORTABLE;
  SystemState lastState = STATE_ROVER_NOT_STARTED; //For Express, start unit in last known state
  bool enableSensorFusion = false; //If IMU is available, avoid using it unless user specifically selects automotive
  bool autoIMUmountAlignment = true; //Allows unit to automatically establish device orientation in vehicle
  bool enableResetDisplay = false;
  uint8_t resetCount = 0;
  bool enableExternalPulse = true; //Send pulse once lock is achieved
  uint32_t externalPulseTimeBetweenPulse_us = 900000; //us between pulses, max of 65s
  uint32_t externalPulseLength_us = 100000; //us length of pulse
  pulseEdgeType_e externalPulsePolarity = PULSE_RISING_EDGE; //Pulse rises for pulse length, then falls
  bool enableExternalHardwareEventLogging = false; //Log when INT/TM2 pin goes low
  bool enableMarksFile = false; //Log marks to the marks file

  ubxMsg ubxMessages[MAX_UBX_MSG] = //Report rates for all known messages
  {
    //NMEA
    {UBLOX_CFG_MSGOUT_NMEA_ID_DTM_UART1, UBX_NMEA_DTM, UBX_CLASS_NMEA, 0, "UBX_NMEA_DTM", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GBS_UART1, UBX_NMEA_GBS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GBS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, UBX_NMEA_GGA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GGA", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, UBX_NMEA_GLL, UBX_CLASS_NMEA, 0, "UBX_NMEA_GLL", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GNS_UART1, UBX_NMEA_GNS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GNS", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_NMEA_ID_GRS_UART1, UBX_NMEA_GRS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GRS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, UBX_NMEA_GSA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GSA", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART1, UBX_NMEA_GST, UBX_CLASS_NMEA, 1, "UBX_NMEA_GST", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, UBX_NMEA_GSV, UBX_CLASS_NMEA, 4, "UBX_NMEA_GSV", (PLATFORM_F9P | PLATFORM_F9R)}, //Default to 1 update every 4 fixes
    {UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, UBX_NMEA_RMC, UBX_CLASS_NMEA, 1, "UBX_NMEA_RMC", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_NMEA_ID_VLW_UART1, UBX_NMEA_VLW, UBX_CLASS_NMEA, 0, "UBX_NMEA_VLW", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, UBX_NMEA_VTG, UBX_CLASS_NMEA, 0, "UBX_NMEA_VTG", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_UART1, UBX_NMEA_ZDA, UBX_CLASS_NMEA, 0, "UBX_NMEA_ZDA", (PLATFORM_F9P | PLATFORM_F9R)},

    //NAV
    {UBLOX_CFG_MSGOUT_UBX_NAV_ATT_UART1, UBX_NAV_ATT, UBX_CLASS_NAV, 0, "UBX_NAV_ATT", (PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_UART1, UBX_NAV_CLOCK, UBX_CLASS_NAV, 0, "UBX_NAV_CLOCK", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_DOP_UART1, UBX_NAV_DOP, UBX_CLASS_NAV, 0, "UBX_NAV_DOP", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_EOE_UART1, UBX_NAV_EOE, UBX_CLASS_NAV, 0, "UBX_NAV_EOE", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_UART1, UBX_NAV_GEOFENCE, UBX_CLASS_NAV, 0, "UBX_NAV_GEOFENCE", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_UART1, UBX_NAV_HPPOSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSECEF", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_UART1, UBX_NAV_HPPOSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSLLH", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_ODO_UART1, UBX_NAV_ODO, UBX_CLASS_NAV, 0, "UBX_NAV_ODO", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_ORB_UART1, UBX_NAV_ORB, UBX_CLASS_NAV, 0, "UBX_NAV_ORB", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_UART1, UBX_NAV_POSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_POSECEF", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART1, UBX_NAV_POSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_POSLLH", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1, UBX_NAV_PVT, UBX_CLASS_NAV, 0, "UBX_NAV_PVT", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_UART1, UBX_NAV_RELPOSNED, UBX_CLASS_NAV, 0, "UBX_NAV_RELPOSNED", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_SAT_UART1, UBX_NAV_SAT, UBX_CLASS_NAV, 0, "UBX_NAV_SAT", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_SIG_UART1, UBX_NAV_SIG, UBX_CLASS_NAV, 0, "UBX_NAV_SIG", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART1, UBX_NAV_STATUS, UBX_CLASS_NAV, 0, "UBX_NAV_STATUS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_UART1, UBX_NAV_SVIN, UBX_CLASS_NAV, 0, "UBX_NAV_SVIN", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_UART1, UBX_NAV_TIMEBDS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEBDS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_UART1, UBX_NAV_TIMEGAL, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGAL", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_UART1, UBX_NAV_TIMEGLO, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGLO", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_UART1, UBX_NAV_TIMEGPS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGPS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_UART1, UBX_NAV_TIMELS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMELS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_UART1, UBX_NAV_TIMEUTC, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEUTC", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_UART1, UBX_NAV_VELECEF, UBX_CLASS_NAV, 0, "UBX_NAV_VELECEF", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_UART1, UBX_NAV_VELNED, UBX_CLASS_NAV, 0, "UBX_NAV_VELNED", (PLATFORM_F9P | PLATFORM_F9R)},

    //RXM
    {UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_UART1, UBX_RXM_MEASX, UBX_CLASS_RXM, 0, "UBX_RXM_MEASX", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART1, UBX_RXM_RAWX, UBX_CLASS_RXM, 0, "UBX_RXM_RAWX", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_RXM_RLM_UART1, UBX_RXM_RLM, UBX_CLASS_RXM, 0, "UBX_RXM_RLM", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_UART1, UBX_RXM_RTCM, UBX_CLASS_RXM, 0, "UBX_RXM_RTCM", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART1, UBX_RXM_SFRBX, UBX_CLASS_RXM, 0, "UBX_RXM_SFRBX", (PLATFORM_F9P | PLATFORM_F9R)},

    //MON
    {UBLOX_CFG_MSGOUT_UBX_MON_COMMS_UART1, UBX_MON_COMMS, UBX_CLASS_MON, 0, "UBX_MON_COMMS", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW2_UART1, UBX_MON_HW2, UBX_CLASS_MON, 0, "UBX_MON_HW2", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW3_UART1, UBX_MON_HW3, UBX_CLASS_MON, 0, "UBX_MON_HW3", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_HW_UART1, UBX_MON_HW, UBX_CLASS_MON, 0, "UBX_MON_HW", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_IO_UART1, UBX_MON_IO, UBX_CLASS_MON, 0, "UBX_MON_IO", (PLATFORM_F9P | PLATFORM_F9R)},

    {UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_UART1, UBX_MON_MSGPP, UBX_CLASS_MON, 0, "UBX_MON_MSGPP", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_RF_UART1, UBX_MON_RF, UBX_CLASS_MON, 0, "UBX_MON_RF", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_UART1, UBX_MON_RXBUF, UBX_CLASS_MON, 0, "UBX_MON_RXBUF", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_RXR_UART1, UBX_MON_RXR, UBX_CLASS_MON, 0, "UBX_MON_RXR", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_UART1, UBX_MON_TXBUF, UBX_CLASS_MON, 0, "UBX_MON_TXBUF", (PLATFORM_F9P | PLATFORM_F9R)},

    //TIM
    {UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART1, UBX_TIM_TM2, UBX_CLASS_TIM, 0, "UBX_TIM_TM2", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_TIM_TP_UART1, UBX_TIM_TP, UBX_CLASS_TIM, 0, "UBX_TIM_TP", (PLATFORM_F9P | PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_UART1, UBX_TIM_VRFY, UBX_CLASS_TIM, 0, "UBX_TIM_VRFY", (PLATFORM_F9P | PLATFORM_F9R)},

    //RTCM
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART1, UBX_RTCM_1005, UBX_RTCM_MSB, 0, "UBX_RTCM_1005", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART1, UBX_RTCM_1074, UBX_RTCM_MSB, 0, "UBX_RTCM_1074", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART1, UBX_RTCM_1077, UBX_RTCM_MSB, 0, "UBX_RTCM_1077", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART1, UBX_RTCM_1084, UBX_RTCM_MSB, 0, "UBX_RTCM_1084", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART1, UBX_RTCM_1087, UBX_RTCM_MSB, 0, "UBX_RTCM_1087", (PLATFORM_F9P)},

    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART1, UBX_RTCM_1094, UBX_RTCM_MSB, 0, "UBX_RTCM_1094", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART1, UBX_RTCM_1097, UBX_RTCM_MSB, 0, "UBX_RTCM_1097", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART1, UBX_RTCM_1124, UBX_RTCM_MSB, 0, "UBX_RTCM_1124", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART1, UBX_RTCM_1127, UBX_RTCM_MSB, 0, "UBX_RTCM_1127", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART1, UBX_RTCM_1230, UBX_RTCM_MSB, 0, "UBX_RTCM_1230", (PLATFORM_F9P)},

    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_UART1, UBX_RTCM_4072_0, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_0", (PLATFORM_F9P)},
    {UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_UART1, UBX_RTCM_4072_1, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_1", (PLATFORM_F9P)},

    //ESF
    {UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_UART1, UBX_ESF_MEAS, UBX_CLASS_ESF, 0, "UBX_ESF_MEAS", (PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_ESF_RAW_UART1, UBX_ESF_RAW, UBX_CLASS_ESF, 0, "UBX_ESF_RAW", (PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_UART1, UBX_ESF_STATUS, UBX_CLASS_ESF, 0, "UBX_ESF_STATUS", (PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_ESF_ALG_UART1, UBX_ESF_ALG, UBX_CLASS_ESF, 0, "UBX_ESF_ALG", (PLATFORM_F9R)},
    {UBLOX_CFG_MSGOUT_UBX_ESF_INS_UART1, UBX_ESF_INS, UBX_CLASS_ESF, 0, "UBX_ESF_INS", (PLATFORM_F9R)},
  };

  //Constellations monitored/used for fix
  ubxConstellation ubxConstellations[MAX_CONSTELLATIONS] =
  {
    {UBLOX_CFG_SIGNAL_GPS_ENA, SFE_UBLOX_GNSS_ID_GPS, true, "GPS"},
    {UBLOX_CFG_SIGNAL_SBAS_ENA, SFE_UBLOX_GNSS_ID_SBAS, true, "SBAS"},
    {UBLOX_CFG_SIGNAL_GAL_ENA, SFE_UBLOX_GNSS_ID_GALILEO, true, "Galileo"},
    {UBLOX_CFG_SIGNAL_BDS_ENA, SFE_UBLOX_GNSS_ID_BEIDOU, true, "BeiDou"},
    //{UBLOX_CFG_SIGNAL_QZSS_ENA, SFE_UBLOX_GNSS_ID_IMES, false, "IMES"}, //Not yet supported? Config key does not exist?
    {UBLOX_CFG_SIGNAL_QZSS_ENA, SFE_UBLOX_GNSS_ID_QZSS, true, "QZSS"},
    {UBLOX_CFG_SIGNAL_GLO_ENA, SFE_UBLOX_GNSS_ID_GLONASS, true, "GLONASS"},
  };

  int maxLogLength_minutes = 60 * 24; //Default to 24 hours
  char profileName[50] = "";

  //NTRIP Server
  bool enableNtripServer = false;
  bool ntripServer_StartAtSurveyIn = false; //true = Start WiFi instead of Bluetooth at Survey-In
  char ntripServer_CasterHost[50] = "rtk2go.com"; //It's free...
  uint16_t ntripServer_CasterPort = 2101;
  char ntripServer_CasterUser[50] = "test@test.com"; //Some free casters require auth. User must provide their own email address to use RTK2Go
  char ntripServer_CasterUserPW[50] = "";
  char ntripServer_MountPoint[50] = "bldr_dwntwn2"; //NTRIP Server
  char ntripServer_MountPointPW[50] = "WR5wRo4H";
  char ntripServer_wifiSSID[50] = "TRex"; //NTRIP Server WiFi
  char ntripServer_wifiPW[50] = "parachutes";

  //NTRIP Client
  bool enableNtripClient = false;
  char ntripClient_CasterHost[50] = "rtk2go.com"; //It's free...
  uint16_t ntripClient_CasterPort = 2101;
  char ntripClient_CasterUser[50] = "test@test.com"; //Some free casters require auth. User must provide their own email address to use RTK2Go
  char ntripClient_CasterUserPW[50] = "";
  char ntripClient_MountPoint[50] = "bldr_SparkFun1";
  char ntripClient_MountPointPW[50] = "";
  char ntripClient_wifiSSID[50] = "TRex"; //NTRIP Server WiFi
  char ntripClient_wifiPW[50] = "parachutes";
  bool ntripClient_TransmitGGA = true;

  int16_t serialTimeoutGNSS = 1; //In ms - used during SerialGNSS.begin. Number of ms to pass of no data before hardware serial reports data available.

  char pointPerfectDeviceProfileToken[40] = "";
  bool enablePointPerfectCorrections = true;
  char home_wifiSSID[50] = ""; //WiFi network to use when attempting to obtain PointPerfect keys and ThingStream provisioning
  char home_wifiPW[50] = "";
  bool autoKeyRenewal = true; //Attempt to get keys if we get under 28 days from the expiration date
  char pointPerfectClientID[50] = "";
  char pointPerfectBrokerHost[50] = ""; // pp.services.u-blox.com
  char pointPerfectLBandTopic[20] = ""; // /pp/key/Lb

  char pointPerfectCurrentKey[33] = ""; //32 hexadecimal digits = 128 bits = 16 Bytes
  uint64_t pointPerfectCurrentKeyDuration = 0;
  uint64_t pointPerfectCurrentKeyStart = 0;

  char pointPerfectNextKey[33] = "";
  uint64_t pointPerfectNextKeyDuration = 0;
  uint64_t pointPerfectNextKeyStart = 0;

  uint64_t lastKeyAttempt = 0; //Epoch time of last attempt at obtaining keys
  bool updateZEDSettings = true; //When in doubt, update the ZED with current settings
  uint32_t LBandFreq = 1556290000; //Default to US band

  //Time Zone - Default to UTC
  int8_t timeZoneHours = 0;
  int8_t timeZoneMinutes = 0;
  int8_t timeZoneSeconds = 0;

  //Debug settings
  bool enablePrintWifiIpAddress = true;
  bool enablePrintState = false;
  bool enablePrintWifiState = false;
  bool enablePrintNtripClientState = false;
  bool enablePrintNtripServerState = false;
  bool enablePrintPosition = false;
  bool enablePrintIdleTime = false;
  bool enablePrintBatteryMessages = true;
  bool enablePrintRoverAccuracy = true;
  bool enablePrintBadMessages = false;
  bool enablePrintLogFileMessages = false;
  bool enablePrintLogFileStatus = true;
  bool enablePrintRingBufferOffsets = false;
  bool enablePrintNtripServerRtcm = false;
  bool enablePrintNtripClientRtcm = false;
  bool enablePrintStates = true;
  bool enablePrintDuplicateStates = false;
  RadioType_e radioType = RADIO_EXTERNAL;
  uint8_t espnowPeers[5][6]; //Max of 5 peers. Contains the MAC addresses (6 bytes) of paired units
  uint8_t espnowPeerCount = 0;
  bool enableRtcmMessageChecking = false;
  BluetoothRadioType_e bluetoothRadioType = BLUETOOTH_RADIO_SPP;
  bool runLogTest = false; //When set to true, device will create a series of test logs
  bool enableNmeaClient = false;
  bool enableNmeaServer = false;
  bool enablePrintNmeaTcpStatus = false;
  bool espnowBroadcast = true; //When true, overrides peers and sends all data via broadcast
  uint16_t antennaHeight = 0; //in mm
  float antennaReferencePoint = 0.0; //in mm
} Settings;
Settings settings;
const Settings defaultSettings = Settings();

//Monitor which devices on the device are on or offline.
struct struct_online {
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
  bool ntripServer = false;
  bool lband = false;
  bool lbandCorrections = false;
  bool i2c = false;
  bool nmeaClient = false;
  bool nmeaServer = false;
} online;

#ifdef COMPILE_WIFI
#ifdef COMPILE_L_BAND
//AWS certificate for PointPerfect API
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
#endif
#endif
