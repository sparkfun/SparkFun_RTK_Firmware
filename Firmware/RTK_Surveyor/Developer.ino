/*
pvtServer.ino

  The code in this module is only compiled when features are disabled in developer
  mode (ENABLE_DEVELOPER defined).
*/

#ifndef COMPILE_ETHERNET

//----------------------------------------
// Ethernet
//----------------------------------------

void menuEthernet() {systemPrintln("NOT compiled");}
void ethernetBegin() {}
IPAddress ethernetGetIpAddress() {return IPAddress((uint32_t)0);}
void ethernetUpdate() {}

void ethernetNTPServerBegin() {}
void ethernetNTPServerUpdate() {}

void ethernetPvtClientSendData(uint8_t *data, uint16_t length) {}
void ethernetPvtClientUpdate() {}

void ethernetWebServerStartESP32W5500() {}
void ethernetWebServerStopESP32W5500() {}

#endif // COMPILE_ETHERNET

#if !COMPILE_NETWORK

//----------------------------------------
// NTRIP client
//----------------------------------------

void ntripClientPrintStatus() {systemPrint("NOT compiled");}
void ntripClientStart()
{
    systemPrintln("NTRIP Client not available: Ethernet and WiFi not compiled");
}
void ntripClientStop(bool clientAllocated) {online.ntripClient = false;}
void ntripClientUpdate() {}

//----------------------------------------
// NTRIP server
//----------------------------------------

void ntripServerPrintStatus() {systemPrint("NOT compiled");}
void ntripServerProcessRTCM(uint8_t incoming) {}
void ntripServerStart()
{
    systemPrintln("NTRIP Server not available: Ethernet and WiFi not compiled");
}
void ntripServerStop(bool clientAllocated) {online.ntripServer = false;}
void ntripServerUpdate() {}

#endif // COMPILE_NETWORK

#ifndef COMPILE_WIFI

//----------------------------------------
// PVT client
//----------------------------------------

uint16_t pvtClientSendData(uint16_t dataHead) {return 0;};
void pvtClientUpdate() {}

//----------------------------------------
// PVT server
//----------------------------------------

bool pvtServerActive() {return false;}
int pvtServerSendData(uint16_t dataHead) {return 0;}
void pvtServerUpdate() {}

//----------------------------------------
// WiFi
//----------------------------------------

void menuWiFi() {systemPrintln("NOT compiled");};
bool wifiConnect(unsigned long timeout) {return false;}
IPAddress wifiGetGatewayIpAddress() {return IPAddress((uint32_t)0);}
IPAddress wifiGetIpAddress() {return IPAddress((uint32_t)0);}
int wifiGetRssi() {return -999;}
bool wifiInConfigMode() {return false;}
bool wifiIsConnected() {return false;}
bool wifiIsNeeded() {return false;}
int wifiNetworkCount() {return 0;}
void wifiPrintNetworkInfo() {}
void wifiStart() {}
void wifiStop() {}
void wifiUpdate() {}

#endif // COMPILE_WIFI
