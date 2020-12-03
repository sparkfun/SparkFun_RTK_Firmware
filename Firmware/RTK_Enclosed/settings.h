//This is all the settings that can be set on RTK Surveyor. It's recorded to NVM and the config file.
struct struct_settings {
  bool printDebugMessages = false;
  bool enableSD = true;
  bool enableDisplay = true;
  bool zedOutputLogging = false;
  bool gnssRAWOutput = false;
  bool frequentFileAccessTimestamps = false;
} settings;

//These are the devices on board RTK Surveyor that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool display = false;
  bool dataLogging = false;
  bool serialOutput = false;
} online;
