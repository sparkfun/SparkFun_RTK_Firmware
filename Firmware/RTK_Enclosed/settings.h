//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int rtkIdentifier = RTK_IDENTIFIER; // rtkIdentifier **must** be the second entry
  bool printDebugMessages = false;
  bool enableSD = true;
  bool enableDisplay = true;
  bool zedOutputLogging = false;
  bool gnssRAWOutput = false;
  bool frequentFileAccessTimestamps = false;
  int maxLogTime_minutes = 60*10; //Default to 10 hours
} settings;

//These are the devices on board RTK Surveyor that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool display = false;
  bool dataLogging = false;
  bool serialOutput = false;
  bool eeprom = false;
} online;
