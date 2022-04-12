//System can enter a variety of states starting at Rover_No_Fix at power on
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
  STATE_BASE_TEMP_WIFI_STARTED,
  STATE_BASE_TEMP_WIFI_CONNECTED, //10
  STATE_BASE_TEMP_CASTER_STARTED,
  STATE_BASE_TEMP_CASTER_CONNECTED,
  STATE_BASE_FIXED_NOT_STARTED,
  STATE_BASE_FIXED_TRANSMITTING,
  STATE_BASE_FIXED_WIFI_STARTED,
  STATE_BASE_FIXED_WIFI_CONNECTED,
  STATE_BASE_FIXED_CASTER_STARTED,
  STATE_BASE_FIXED_CASTER_CONNECTED,
  STATE_BUBBLE_LEVEL,
  STATE_MARK_EVENT, //20
  STATE_DISPLAY_SETUP,
  STATE_WIFI_CONFIG_NOT_STARTED,
  STATE_WIFI_CONFIG,
  STATE_TEST,
  STATE_TESTING, //25
  STATE_PROFILE_1,
  STATE_PROFILE_2,
  STATE_PROFILE_3,
  STATE_PROFILE_4,
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
} customNmeaType_e;

//Freeze and blink LEDs if we hit a bad error
typedef enum
{
  ERROR_NO_I2C = 2, //Avoid 0 and 1 as these are bad blink codes
  ERROR_GPS_CONFIG_FAIL,
} t_errorNumber;

//Radio status LED goes from off (LED off), no connection (blinking), to connected (solid)
enum RadioState
{
  RADIO_OFF = 0,
  BT_ON_NOCONNECTION, //WiFi is off
  BT_CONNECTED,
  WIFI_ON_NOCONNECTION, //BT is off
  WIFI_CONNECTED,
};
volatile byte radioState = RADIO_OFF;

//Return values for getByteChoice()
enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X = 254,
};

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
typedef struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  //int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
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
  bool enableNtripServer = false;
  char casterHost[50] = "rtk2go.com"; //It's free...
  uint16_t casterPort = 2101;
  char casterUser[50] = "test@test.com"; //Some free casters require auth. User must provide their own email address to use RTK2Go
  char casterUserPW[50] = "";
  char mountPointUpload[50] = "bldr_dwntwn2";
  char mountPointUploadPW[50] = "WR5wRo4H";
  char mountPointDownload[50] = "bldr_SparkFun1";
  char mountPointDownloadPW[50] = "";
  bool casterTransmitGGA = true;
  char wifiSSID[50] = "TRex";
  char wifiPW[50] = "parachutes";
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
  SystemState lastState = STATE_ROVER_NOT_STARTED; //For Express, start unit in last known state
  bool throttleDuringSPPCongestion = true;
  bool enableSensorFusion = false; //If IMU is available, avoid using it unless user specifically selects automotive
  bool autoIMUmountAlignment = true; //Allows unit to automatically establish device orientation in vehicle
  bool enableResetDisplay = false;
  uint8_t resetCount = 0;
  bool enableExternalPulse = true; //Send pulse once lock is achieved
  uint32_t externalPulseTimeBetweenPulse_us = 1000000; //us between pulses, max of 65s
  uint32_t externalPulseLength_us = 100000; //us length of pulse
  pulseEdgeType_e externalPulsePolarity = PULSE_RISING_EDGE; //Pulse rises for pulse length, then falls
  bool enableExternalHardwareEventLogging = false; //Log when INT/TM2 pin goes low

  int maxLogLength_minutes = 60 * 24; //Default to 24 hours
  char profileName[50] = "Default";

} Settings;
Settings settings;

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
} online;
