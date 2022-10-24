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
