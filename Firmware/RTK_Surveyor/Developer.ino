/*
pvtServer.ino

  The code in this module is only compiled when features are disabled in developer
  mode (ENABLE_DEVELOPER defined).
*/

#ifndef COMPILE_ETHERNET

//----------------------------------------
// Ethernet
//----------------------------------------

void menuEthernet() {systemPrintln("Ethernet not compiled");}
void ethernetBegin() {}
IPAddress ethernetGetIpAddress() {return IPAddress((uint32_t)0);}
void ethernetUpdate() {}
void ethernetVerifyTables() {}

void ethernetPvtClientSendData(uint8_t *data, uint16_t length) {}
void ethernetPvtClientUpdate() {}

void ethernetWebServerStartESP32W5500() {}
void ethernetWebServerStopESP32W5500() {}

//----------------------------------------
// NTP: Network Time Protocol
//----------------------------------------

void menuNTP() {systemPrint("NTP not compiled");}
void ntpServerBegin() {}
void ntpServerUpdate() {}
void ntpValidateTables() {}
void ntpServerStop() {}

#endif // COMPILE_ETHERNET

#if !COMPILE_NETWORK

//----------------------------------------
// Network layer
//----------------------------------------

void menuNetwork() {systemPrint("Network not compiled");}
void networkUpdate() {}
void networkVerifyTables() {}
void networkStop(uint8_t networkType) {}

//----------------------------------------
// NTRIP client
//----------------------------------------

void ntripClientPrintStatus() {systemPrint("NTRIP Client not compiled");}
void ntripClientStop(bool clientAllocated) {online.ntripClient = false;}
void ntripClientUpdate() {}
void ntripClientValidateTables() {}

//----------------------------------------
// NTRIP server
//----------------------------------------

bool ntripServerIsCasting(int serverIndex) {return false;}
void ntripServerPrintStatus(int serverIndex) {systemPrintf("**NTRIP Server %d not compiled**\r\n", serverIndex);}
void ntripServerProcessRTCM(int serverIndex, uint8_t incoming) {}
void ntripServerStop(int serverIndex, bool clientAllocated) {online.ntripServer[serverIndex] = false;}
void ntripServerUpdate() {}
void ntripServerValidateTables() {}

//----------------------------------------
// OTA client
//----------------------------------------

void otaVerifyTables() {}

//----------------------------------------
// PVT client
//----------------------------------------

int32_t pvtClientSendData(uint16_t dataHead) {return 0;}
void pvtClientUpdate() {}
void pvtClientValidateTables() {}
void pvtClientZeroTail() {}
void discardPvtClientBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail) {}

//----------------------------------------
// PVT UDP server
//----------------------------------------

int32_t pvtUdpServerSendData(uint16_t dataHead) {return 0;}
void pvtUdpServerStop() {}
void pvtUdpServerUpdate() {}
void pvtUdpServerZeroTail() {}
void discardPvtUdpServerBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail) {}

#endif // COMPILE_NETWORK

//----------------------------------------
// Web Server
//----------------------------------------

#ifndef COMPILE_AP

bool startWebServer(bool startWiFi = true, int httpPort = 80)
{
    systemPrintln("AP not compiled");
    return false;
}
void stopWebServer() {}
bool parseIncomingSettings() {return false;}

#endif  // COMPILE_AP
#ifndef COMPILE_WIFI

//----------------------------------------
// PVT server
//----------------------------------------

int32_t pvtServerSendData(uint16_t dataHead) {return 0;}
void pvtServerStop() {}
void pvtServerUpdate() {}
void pvtServerZeroTail() {}
void pvtServerValidateTables() {}
void discardPvtServerBytes(RING_BUFFER_OFFSET previousTail, RING_BUFFER_OFFSET newTail) {}

//----------------------------------------
// WiFi
//----------------------------------------

void menuWiFi() {systemPrintln("WiFi not compiled");};
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
void wifiShutdown() {}
#define WIFI_STOP() {}

#endif // COMPILE_WIFI
