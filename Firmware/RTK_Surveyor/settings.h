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
  STATE_BASE_TEMP_WIFI_CONNECTED,
  STATE_BASE_TEMP_CASTER_STARTED,
  STATE_BASE_TEMP_CASTER_CONNECTED,
  STATE_BASE_FIXED_NOT_STARTED,
  STATE_BASE_FIXED_TRANSMITTING,
  STATE_BASE_FIXED_WIFI_STARTED,
  STATE_BASE_FIXED_WIFI_CONNECTED,
  STATE_BASE_FIXED_CASTER_STARTED,
  STATE_BASE_FIXED_CASTER_CONNECTED,
} SystemState;
volatile SystemState systemState = STATE_ROVER_NOT_STARTED;

typedef enum
{
  RTK_SURVEYOR = 0,
  RTK_EXPRESS,
  RTK_FACET,
} ProductVariant;
ProductVariant productVariant = RTK_SURVEYOR;

typedef enum
{
  BUTTON_ROVER = 0,
  BUTTON_BASE,
  BUTTON_PRESSED,
  BUTTON_RELEASED,
} ButtonState;
ButtonState buttonPreviousState = BUTTON_ROVER;
ButtonState setupButtonState = BUTTON_RELEASED; //RTK Express Setup Button

//Data port mux (RTK Express) can enter one of four different connections
typedef enum muxConnectionType_e
{
  MUX_UBLOX_NMEA = 0,
  MUX_PPS_EVENTTRIGGER,
  MUX_I2C,
  MUX_ADC_DAC,
} muxConnectionType_e;

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

//Return values for getByteChoice()
enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X = 254,
};

//Each message will have a rate, a visible name, and a class
typedef struct ubxMsg
{
  uint8_t msgID;
  uint8_t msgClass;
  uint8_t msgRate;
  char msgTextName[30];
} ubxMsg;

#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS

//These are the allowable messages to broadcast and log (if enabled)
//Tested with u-center v21.02
struct ubxMsgs
{
  struct ubxMsg nmea_dtm = {UBX_NMEA_DTM, UBX_CLASS_NMEA, 0, "UBX_NMEA_DTM"};
  struct ubxMsg nmea_gbs = {UBX_NMEA_GBS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GBS"};
  struct ubxMsg nmea_gga = {UBX_NMEA_GGA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GGA"};
  struct ubxMsg nmea_gll = {UBX_NMEA_GLL, UBX_CLASS_NMEA, 0, "UBX_NMEA_GLL"};
  struct ubxMsg nmea_gns = {UBX_NMEA_GNS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GNS"};

  struct ubxMsg nmea_grs = {UBX_NMEA_GRS, UBX_CLASS_NMEA, 0, "UBX_NMEA_GRS"};
  struct ubxMsg nmea_gsa = {UBX_NMEA_GSA, UBX_CLASS_NMEA, 1, "UBX_NMEA_GSA"};
  struct ubxMsg nmea_gst = {UBX_NMEA_GST, UBX_CLASS_NMEA, 1, "UBX_NMEA_GST"};
  struct ubxMsg nmea_gsv = {UBX_NMEA_GSV, UBX_CLASS_NMEA, 1, "UBX_NMEA_GSV"};
  struct ubxMsg nmea_rmc = {UBX_NMEA_RMC, UBX_CLASS_NMEA, 1, "UBX_NMEA_RMC"};
  
  struct ubxMsg nmea_vlw = {UBX_NMEA_VLW, UBX_CLASS_NMEA, 0, "UBX_NMEA_VLW"};
  struct ubxMsg nmea_vtg = {UBX_NMEA_VTG, UBX_CLASS_NMEA, 0, "UBX_NMEA_VTG"};
  struct ubxMsg nmea_zda = {UBX_NMEA_ZDA, UBX_CLASS_NMEA, 0, "UBX_NMEA_ZDA"};
  
  //  uint8_t nmea_msb = 0; //Not supported by u-center
  //  uint8_t nmea_gaq = 0; //Not supported by u-center
  //  uint8_t nmea_gbq = 0; //Not supported by u-center
  //  uint8_t nmea_glq = 0; //Not supported by u-center
  //  uint8_t nmea_gnq = 0; //Not supported by u-center
  //  uint8_t nmea_gpq = 0; //Not supported by u-center
  //  uint8_t nmea_gqq = 0; //Not supported by u-center
  //  uint8_t nmea_rlm = 0; //Not supported by u-center
  //  uint8_t nmea_txt = 0; //Not supported by u-center
  //  uint8_t nmea_ths = 0; //Not supported by ZED-F9P

  struct ubxMsg nav_clock = {UBX_NAV_CLOCK, UBX_CLASS_NAV, 0, "UBX_NAV_CLOCK"};
  struct ubxMsg nav_dop = {UBX_NAV_DOP, UBX_CLASS_NAV, 0, "UBX_NAV_DOP"};
  struct ubxMsg nav_eoe = {UBX_NAV_EOE, UBX_CLASS_NAV, 0, "UBX_NAV_EOE"};
  struct ubxMsg nav_geofence = {UBX_NAV_GEOFENCE, UBX_CLASS_NAV, 0, "UBX_NAV_GEOFENCE"};
  struct ubxMsg nav_hpposecef = {UBX_NAV_HPPOSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSECEF"};

  struct ubxMsg nav_hpposllh = {UBX_NAV_HPPOSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_HPPOSLLH"};
  struct ubxMsg nav_odo = {UBX_NAV_ODO, UBX_CLASS_NAV, 0, "UBX_NAV_ODO"};
  struct ubxMsg nav_orb = {UBX_NAV_ORB, UBX_CLASS_NAV, 0, "UBX_NAV_ORB"};
  struct ubxMsg nav_posecef = {UBX_NAV_POSECEF, UBX_CLASS_NAV, 0, "UBX_NAV_POSECEF"};
  struct ubxMsg nav_posllh = {UBX_NAV_POSLLH, UBX_CLASS_NAV, 0, "UBX_NAV_POSLLH"};
  
  struct ubxMsg nav_pvt = {UBX_NAV_PVT, UBX_CLASS_NAV, 0, "UBX_NAV_PVT"};
  struct ubxMsg nav_relposned = {UBX_NAV_RELPOSNED, UBX_CLASS_NAV, 0, "UBX_NAV_RELPOSNED"};
  struct ubxMsg nav_sat = {UBX_NAV_SAT, UBX_CLASS_NAV, 0, "UBX_NAV_SAT"};
  struct ubxMsg nav_sig = {UBX_NAV_SIG, UBX_CLASS_NAV, 0, "UBX_NAV_SIG"};
  struct ubxMsg nav_status = {UBX_NAV_STATUS, UBX_CLASS_NAV, 0, "UBX_NAV_STATUS"};
  
  struct ubxMsg nav_svin = {UBX_NAV_SVIN, UBX_CLASS_NAV, 0, "UBX_NAV_SVIN"};
  struct ubxMsg nav_timebds = {UBX_NAV_TIMEBDS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEBDS"};
  struct ubxMsg nav_timegal = {UBX_NAV_TIMEGAL, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGAL"};
  struct ubxMsg nav_timeglo = {UBX_NAV_TIMEGLO, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGLO"};
  struct ubxMsg nav_timegps = {UBX_NAV_TIMEGPS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEGPS"};
  
  struct ubxMsg nav_timels = {UBX_NAV_TIMELS, UBX_CLASS_NAV, 0, "UBX_NAV_TIMELS"};
  struct ubxMsg nav_timeutc = {UBX_NAV_TIMEUTC, UBX_CLASS_NAV, 0, "UBX_NAV_TIMEUTC"};
  struct ubxMsg nav_velecef = {UBX_NAV_VELECEF, UBX_CLASS_NAV, 0, "UBX_NAV_VELECEF"};
  struct ubxMsg nav_velned = {UBX_NAV_VELNED, UBX_CLASS_NAV, 0, "UBX_NAV_VELNED"};

  //  uint8_t nav_aopstatus = 0; //Not supported by library or ZED-F9P
  //  uint8_t nav_att = 0; //Not supported by ZED-F9P
  //  uint8_t nav_cov = 0; //Not supported by library
  //  uint8_t nav_resetodo = 0; //Not supported u-center 21.02
  //  uint8_t nav_sbas = 0; //Not supported by library
  //  uint8_t nav_slas = 0; //Not supported by library
  //  uint8_t nav_timeqzss = 0; //Not supported in library
  //  uint8_t nav_dgps = 0; //Not supported by ZED-F9P
  //  uint8_t nav_eell = 0; //Not supported in library
  //  uint8_t nav_ekfstatus = 0; //Not supported by ZED-F9P
  //  uint8_t nav_nmi = 0; //Not supported by ZED-F9P or library
  //  uint8_t nav_sol = 0; //Not supported by ZED-F9P or library
  //  uint8_t nav_svinfo = 0; //Not supported by ZED-F9P or library
  
  struct ubxMsg rxm_measx = {UBX_RXM_MEASX, UBX_CLASS_RXM, 0, "UBX_RXM_MEASX"};
  struct ubxMsg rxm_rawx = {UBX_RXM_RAWX, UBX_CLASS_RXM, 0, "UBX_RXM_RAWX"};
  struct ubxMsg rxm_rlm = {UBX_RXM_RLM, UBX_CLASS_RXM, 0, "UBX_RXM_RLM"};
  struct ubxMsg rxm_rtcm = {UBX_RXM_RTCM, UBX_CLASS_RXM, 0, "UBX_RXM_RTCM"};

  struct ubxMsg rxm_sfrbx = {UBX_RXM_SFRBX, UBX_CLASS_RXM, 0, "UBX_RXM_SFRBX"};

//  uint8_t rxm_alm = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_eph = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_imes = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_pmp = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_raw = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_sfrb = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_spartn = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_svsi = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_tm = 0; //Not supported by library or ZED-F9P
//  uint8_t rxm_pmreq = 0; //Not supported by u-center
  
//  uint8_t hnr_att = 0; //Not supported by ZED-F9P
//  uint8_t hnr_ins = 0; //Not supported by ZED-F9P
//  uint8_t hnr_pvt = 0; //Not supported by ZED-F9P

  struct ubxMsg mon_comms = {UBX_MON_COMMS, UBX_CLASS_MON, 0, "UBX_MON_COMMS"};
  struct ubxMsg mon_hw2 = {UBX_MON_HW2, UBX_CLASS_MON, 0, "UBX_MON_HW2"};
  struct ubxMsg mon_hw3 = {UBX_MON_HW3, UBX_CLASS_MON, 0, "UBX_MON_HW3"};
  struct ubxMsg mon_hw = {UBX_MON_HW, UBX_CLASS_MON, 0, "UBX_MON_HW"};
  struct ubxMsg mon_io = {UBX_MON_IO, UBX_CLASS_MON, 0, "UBX_MON_IO"};

  struct ubxMsg mon_msgpp = {UBX_MON_MSGPP, UBX_CLASS_MON, 0, "UBX_MON_MSGPP"};
  struct ubxMsg mon_rf = {UBX_MON_RF, UBX_CLASS_MON, 0, "UBX_MON_RF"};
  struct ubxMsg mon_rxbuf = {UBX_MON_RXBUF, UBX_CLASS_MON, 0, "UBX_MON_RXBUF"};
  struct ubxMsg mon_rxr = {UBX_MON_RXR, UBX_CLASS_MON, 0, "UBX_MON_RXR"};
  struct ubxMsg mon_txbuf = {UBX_MON_TXBUF, UBX_CLASS_MON, 0, "UBX_MON_TXBUF"};

//  uint8_t mon_gnss = 0; //Not supported by u-center
//  uint8_t mon_patch = 0; //Not supported by u-center
  //uint8_t mon_smgr = 0; //Not supported by library or ZED-F9P
  //uint8_t mon_span = 0; //Not supported by library
//  uint8_t mon_ver = 0; //Not supported by u-center

  struct ubxMsg tim_tm2 = {UBX_TIM_TM2, UBX_CLASS_TIM, 0, "UBX_TIM_TM2"};
  struct ubxMsg tim_tp = {UBX_TIM_TP, UBX_CLASS_TIM, 0, "UBX_TIM_TP"};
  struct ubxMsg tim_vrfy = {UBX_TIM_VRFY, UBX_CLASS_TIM, 0, "UBX_TIM_VRFY"};

//  uint8_t tim_dosc = 0; //Not supported by library or ZED-F9P
//  uint8_t tim_fchg = 0; //Not supported by library or ZED-F9P
//  uint8_t tim_smeas = 0; //Not supported by library or ZED-F9P
//  uint8_t tim_svin = 0; //Not supported by library or ZED-F9P
//  uint8_t tim_tos = 0; //Not supported by library or ZED-F9P
//  uint8_t tim_vcocal = 0; //Not supported by library or ZED-F9P

//  uint8_t esf_alg = 0; //Not supported by ZED-F9P
//  uint8_t esf_ins = 0; //Not supported by ZED-F9P
//  uint8_t esf_meas = 0; //Not supported by ZED-F9P
//  uint8_t esf_raw = 0; //Not supported by ZED-F9P
//  uint8_t esf_status = 0; //Not supported by ZED-F9P
  //uint8_t esf_resetalg = 0; //Not supported by u-center

  struct ubxMsg rtcm_1005 = {UBX_RTCM_1005, UBX_RTCM_MSB, 0, "UBX_RTCM_1005"};
  struct ubxMsg rtcm_1074 = {UBX_RTCM_1074, UBX_RTCM_MSB, 0, "UBX_RTCM_1074"};
  struct ubxMsg rtcm_1077 = {UBX_RTCM_1077, UBX_RTCM_MSB, 0, "UBX_RTCM_1077"};
  struct ubxMsg rtcm_1084 = {UBX_RTCM_1084, UBX_RTCM_MSB, 0, "UBX_RTCM_1084"};
  struct ubxMsg rtcm_1087 = {UBX_RTCM_1087, UBX_RTCM_MSB, 0, "UBX_RTCM_1087"};

  struct ubxMsg rtcm_1094 = {UBX_RTCM_1094, UBX_RTCM_MSB, 0, "UBX_RTCM_1094"};
  struct ubxMsg rtcm_1097 = {UBX_RTCM_1097, UBX_RTCM_MSB, 0, "UBX_RTCM_1097"};
  struct ubxMsg rtcm_1124 = {UBX_RTCM_1124, UBX_RTCM_MSB, 0, "UBX_RTCM_1124"};
  struct ubxMsg rtcm_1127 = {UBX_RTCM_1127, UBX_RTCM_MSB, 0, "UBX_RTCM_1127"};
  struct ubxMsg rtcm_1230 = {UBX_RTCM_1230, UBX_RTCM_MSB, 0, "UBX_RTCM_1230"};

  struct ubxMsg rtcm_4072_0 = {UBX_RTCM_4072_0, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_0"};
  struct ubxMsg rtcm_4072_1 = {UBX_RTCM_4072_1, UBX_RTCM_MSB, 0, "UBX_RTCM_4072_1"};
};

//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
  bool printDebugMessages = false;
  bool enableSD = true;
  bool enableDisplay = true;
  int maxLogTime_minutes = 60 * 10; //Default to 10 hours
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
  uint32_t dataPortBaud = 460800; //Default to 460800bps to support >10Hz update rates
  uint32_t radioPortBaud = 57600; //Default to 57600bps to support connection to SiK1000 radios
  bool enableSBAS = false; //Bug in ZED-F9P v1.13 firmware causes RTK LED to not light when RTK Floating with SBAS on.
  bool enableNtripServer = false;
  char casterHost[50] = "rtk2go.com"; //It's free...
  uint16_t casterPort = 2101;
  char mountPoint[50] = "bldr_dwntwn2";
  char mountPointPW[50] = "WR5wRo4H";
  char wifiSSID[50] = "TRex";
  char wifiPW[50] = "parachutes";
  float surveyInStartingAccuracy = 1.0; //Wait for 1m horizontal positional accuracy before starting survey in
  uint16_t measurementRate = 250; //Elapsed ms between GNSS measurements. 25ms to 65535ms. Default 4Hz.
  uint16_t navigationRate = 1; //Ratio between number of measurements and navigation solutions. Default 1 for 4Hz (with measurementRate).
  ubxMsgs message; //Report rates for all known messages
  bool enableI2Cdebug = false; //Turn on to display GNSS library debug messages
  bool enableHeapReport = false; //Turn on to display free heap
  muxConnectionType_e dataPortChannel = MUX_UBLOX_NMEA; //Mux default to ublox UART1
  uint16_t spiFrequency = 8; //By default, use 8MHz SPI
  bool enableLogging = true; //If an SD card is present, log default sentences
} settings;

//These are the devices on board RTK Surveyor that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool display = false;
  bool gnss = false;
  bool logging = false;
  bool serialOutput = false;
  bool eeprom = false;
  bool rtc = false;
  bool battery = false;
  bool accelerometer = false;
} online;
