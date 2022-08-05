/*
  Use ESP NOW protocol to transmit RTCM between RTK Products via 2.4GHz

  How pairing works:
    1. Device enters pairing mode
    2. Device adds the broadcast MAC (all 0xFFs) as peer
    3. Device waits for incoming pairing packet from remote
    4. If valid pairing packet received, add peer, immediately transmit a pairing packet to that peer and exit.

    ESP NOW is bare metal, there is no guaranteed packet delivery. For RTCM byte transmissions using ESP NOW:
      We don't care about dropped packets or packets out of order. The ZED will check the integrity of the RTCM packet.
      We don't care if the ESP NOW packet is corrupt or not. RTCM has its own CRC. RTK needs valid RTCM once every
      few seconds so a single dropped frame is not critical.
*/


//Create a struct for ESP NOW pairing
typedef struct PairMessage {
  uint8_t macAddress[6];
  bool encrypt;
  uint8_t channel;
  uint8_t crc; //Simple check - add MAC together and limit to 8 bit
} PairMessage;

// Callback when data is sent
#ifdef COMPILE_WIFI
void espnowOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  //  Serial.print("Last Packet Send Status: ");
  //  if (status == ESP_NOW_SEND_SUCCESS)
  //    Serial.println("Delivery Success");
  //  else
  //    Serial.println("Delivery Fail");
}
#endif

// Callback when data is received
void espnowOnDataRecieved(const uint8_t *mac, const uint8_t *incomingData, int len)
{
#ifdef COMPILE_WIFI
  if (wifiState == WIFI_ESPNOW_PAIRING)
  {
    if (len == sizeof(PairMessage)) //First error check
    {
      PairMessage pairMessage;
      memcpy(&pairMessage, incomingData, sizeof(pairMessage));

      //Check CRC
      uint8_t tempCRC = 0;
      for (int x = 0 ; x < 6 ; x++)
        tempCRC += pairMessage.macAddress[x];

      if (tempCRC == pairMessage.crc) //2nd error check
      {
        //Record this MAC to peer list
        memcpy(settings.espnowPeers[settings.espnowPeerCount++], &pairMessage.macAddress, 6);
        settings.espnowPeerCount %= ESPNOW_MAX_PEERS;

        wifiSetState(WIFI_ESPNOW_MAC_RECEIVED);
      }
      //else Pair CRC failed
    }
  }
  else
  {
    espnowRSSI = packetRSSI; //Record this packets RSSI as an ESP NOW packet

    //Pass RTCM bytes (presumably) from ESP NOW out ESP32-UART2 to ZED-UART1
    serialGNSS.write(incomingData, len);
    log_d("ESPNOW: Rececived %d bytes, RSSI: %d", len, espnowRSSI);

    online.rxRtcmCorrectionData = true;
    lastEspnowRssiUpdate = millis();
  }
#endif
}

// Callback for all RX Packets
// Get RSSI of all incoming management packets: https://esp32.com/viewtopic.php?t=13889
#ifdef COMPILE_WIFI
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  packetRSSI = ppkt->rx_ctrl.rssi;
}
#endif

void espnowStart()
{
#ifdef COMPILE_WIFI
  // If we are in WIFI_OFF or WIFI_ON, CONNECTED, NOTCONNECTED, doesn't matter
  // General WiFi will be turned off/disconnected and replaced by ESP NOW

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Use promiscuous callback to capture RSSI of packet
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);

  // Register send callbacks
  esp_now_register_send_cb(espnowOnDataSent);
  esp_now_register_recv_cb(espnowOnDataRecieved);

  if (settings.espnowPeerCount == 0)
  {
    wifiSetState(WIFI_ESPNOW_ON);
  }
  else
  {
    //If we already have peers, move to paired state
    wifiSetState(WIFI_ESPNOW_PAIRED);

    log_d("Adding %d espnow peers", settings.espnowPeerCount);
    for (int x = 0 ; x < settings.espnowPeerCount ; x++)
    {
      esp_err_t result = espnowAddPeer(settings.espnowPeers[x]); // Enable encryption to peers
      if (result != ESP_OK)
        log_d("Failed to add peer #%d\n\r", x);
    }
  }

#else
  Serial.println("Error - WiFi not compiled");
#endif
}

//Begin broadcasting our MAC and wait for remote unit to respond
void espnowBeginPairing()
{
#ifndef COMPILE_WIFI  
  return;
#endif

  if (wifiState != WIFI_ESPNOW_ON
      || wifiState != WIFI_ESPNOW_PAIRING
      || wifiState != WIFI_ESPNOW_MAC_RECEIVED
      || wifiState != WIFI_ESPNOW_PAIRED
     )
  {
    espnowStart();
  }

  // To begin pairing, we must add the broadcast MAC to the peer list
  uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  espnowAddPeer(broadcastMac, false); // Encryption is not supported for multicast addresses

  wifiSetState(WIFI_ESPNOW_PAIRING);

  //Begin sending our MAC every 250ms until a remote device sends us there info
  randomSeed(millis());

  Serial.println("Begin pairing. Place other unit in pairing mode. Press any key to exit.");
  while (Serial.available()) Serial.read();

  while (1)
  {
    if (Serial.available()) break;

    int timeout = 1000 + random(0, 100); //Delay 1000 to 1100ms
    for (int x = 0 ; x < timeout ; x++)
    {
      delay(1);

      if (wifiState == WIFI_ESPNOW_MAC_RECEIVED)
      {
        //Add new peer to system
        espnowAddPeer(settings.espnowPeers[settings.espnowPeerCount - 1]);

        //Send message directly to the last peer stored (not unicast), then exit
        espnowSendPairMessage(settings.espnowPeers[settings.espnowPeerCount - 1]);

        wifiSetState(WIFI_ESPNOW_PAIRED);
        Serial.println("Pairing compete");
        return;
      }
    }

    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    espnowSendPairMessage(broadcastMac); //Send unit's MAC address over broadcast, no ack, no encryption

    Serial.println("Scanning for other radio...");
  }

  Serial.println("User pressed button. Pairing canceled.");
}

//Create special pair packet to a given MAC
esp_err_t espnowSendPairMessage(uint8_t *sendToMac)
{
#ifdef COMPILE_WIFI
  // Assemble message to send
  PairMessage pairMessage;

  //Get unit MAC address
  uint8_t unitMACAddress[6];
  esp_read_mac(unitMACAddress, ESP_MAC_WIFI_STA);
  memcpy(pairMessage.macAddress, unitMACAddress, 6);
  pairMessage.encrypt = false;
  pairMessage.channel = 0;

  pairMessage.crc = 0; //Calculate CRC
  for (int x = 0 ; x < 6 ; x++)
    pairMessage.crc += unitMACAddress[x];

  return (esp_now_send(sendToMac, (uint8_t *) &pairMessage, sizeof(pairMessage))); //Send packet to given MAC
#else
  return (ESP_OK);
#endif
}

//Add a given MAC address to the peer list
esp_err_t espnowAddPeer(uint8_t *peerMac)
{
  return (espnowAddPeer(peerMac, true)); //Encrypt by default
}

esp_err_t espnowAddPeer(uint8_t *peerMac, bool encrypt)
{
#ifdef COMPILE_WIFI
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, peerMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  //memcpy(peerInfo.lmk, "RTKProductsLMK56", 16);
  //peerInfo.encrypt = true;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result != ESP_OK)
    log_d("Failed to add peer");
  return (result);
#else
  return (ESP_OK);
#endif
}
