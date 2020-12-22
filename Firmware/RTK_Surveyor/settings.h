//User can enter fixed base coordinates in ECEF or degrees
typedef enum
{
  COORD_TYPE_ECEF = 0,
  COORD_TYPE_GEOGRAPHIC,
} coordinateType_e;

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

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
typedef enum
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_NOTSTARTED, //User has indicated base, but current pos accuracy is too low
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
} BaseState;
volatile BaseState baseState = BASE_OFF;
unsigned long baseStateBlinkTime = 0;
const unsigned long maxSurveyInWait_s = 60L * 15L; //Re-start survey-in after X seconds

//Return values for getByteChoice()
enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X,
};

//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
  uint8_t gnssMeasurementFrequency = 4; //Number of fixes per second
  bool printDebugMessages = false;
  bool enableSD = true;
  bool enableDisplay = true;
  bool zedOutputLogging = false;
  bool gnssRAWOutput = false;
  bool frequentFileAccessTimestamps = false;
  int maxLogTime_minutes = 60*10; //Default to 10 hours
  int observationSeconds = 60; //Default survey in time of 60 seconds
  float observationPositionAccuracy = 5.0; //Default survey in pos accy of 5m
  bool fixedBase = false; //Use survey-in by default
  bool fixedBaseCoordinateType = COORD_TYPE_ECEF;
  double fixedEcefX = 0.0;
  double fixedEcefY = 0.0;
  double fixedEcefZ = 0.0;
  double fixedLat = 0.0;
  double fixedLong = 0.0;
  double fixedAltitude = 0.0;
  uint32_t dataPortBaud = 115200; //Default to 115200bps
  uint32_t radioPortBaud = 57600; //Default to 57600bps to support connection to SiK1000 radios
  bool outputSentenceGGA = true;
  bool outputSentenceGSA = true;
  bool outputSentenceGSV = true;
  bool outputSentenceRMC = true;
  bool outputSentenceGST = true;
  bool enableSBAS = false; //Bug in ZED-F9P v1.13 firmware causes RTK LED to not light when RTK Floating with SBAS on.
  bool enableNtripServer = false;
  char casterHost[50] = "rtk2go.com"; //It's free...
  uint16_t casterPort = 2101;
  char mountPoint[50] = "bldr_dwntwn2";
  char mountPointPW[50] = "WR5wRo4H";
  char wifiSSID[50] = "TRex";
  char wifiPW[50] = "parachutes";  
} settings;

//These are the devices on board RTK Surveyor that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool display = false;
  bool dataLogging = false;
  bool serialOutput = false;
  bool eeprom = false;
} online;
