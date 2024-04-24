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

// Create a struct for ESP NOW pairing
typedef struct PairMessage
{
    uint8_t macAddress[6];
    bool encrypt;
    uint8_t channel;
    uint8_t crc; // Simple check - add MAC together and limit to 8 bit
} PairMessage;

// Callback when data is sent
#ifdef COMPILE_ESPNOW
void espnowOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    //  systemPrint("Last Packet Send Status: ");
    //  if (status == ESP_NOW_SEND_SUCCESS)
    //    systemPrintln("Delivery Success");
    //  else
    //    systemPrintln("Delivery Fail");
}
#endif // COMPILE_ESPNOW

// Callback when data is received
void espnowOnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
#ifdef COMPILE_ESPNOW
    if (espnowState == ESPNOW_PAIRING)
    {
        if (len == sizeof(PairMessage)) // First error check
        {
            PairMessage pairMessage;
            memcpy(&pairMessage, incomingData, sizeof(pairMessage));

            // Check CRC
            uint8_t tempCRC = 0;
            for (int x = 0; x < 6; x++)
                tempCRC += pairMessage.macAddress[x];

            if (tempCRC == pairMessage.crc) // 2nd error check
            {
                memcpy(&receivedMAC, pairMessage.macAddress, 6);
                espnowSetState(ESPNOW_MAC_RECEIVED);
            }
            // else Pair CRC failed
        }
    }
    else
    {
        espnowRSSI = packetRSSI; // Record this packets RSSI as an ESP NOW packet

        // Pass RTCM bytes (presumably) from ESP NOW out ESP32-UART2 to ZED-UART1 / SPI
        if (USE_I2C_GNSS)
            serialGNSS.write(incomingData, len);
        else
            theGNSS.pushRawData((uint8_t *)incomingData, len);
        if (!inMainMenu)
            log_d("ESPNOW received %d RTCM bytes, pushed to ZED, RSSI: %d", len, espnowRSSI);

        espnowIncomingRTCM = true;
        lastEspnowRssiUpdate = millis();
    }
#endif // COMPILE_ESPNOW
}

// Callback for all RX Packets
// Get RSSI of all incoming management packets: https://esp32.com/viewtopic.php?t=13889
#ifdef COMPILE_ESPNOW
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
    if (type != WIFI_PKT_MGMT)
        return;

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
    packetRSSI = ppkt->rx_ctrl.rssi;
}
#endif // COMPILE_ESPNOW

// If WiFi is already enabled, simply add the LR protocol
// If the radio is off entirely, start the radio, turn on only the LR protocol
void espnowStart()
{
#ifdef COMPILE_ESPNOW

    esp_err_t response;

    if (wifiState == WIFI_STATE_OFF && espnowState == ESPNOW_OFF)
    {
        if (WiFi.getMode() != WIFI_STA)
            WiFi.mode(WIFI_STA);

        // Radio is off, turn it on
        // esp_wifi_set_protocol requires WiFi to be started
        response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR); // Stops WiFi Station.
        if (response != ESP_OK)
            systemPrintf("espnowStart: Error setting ESP-Now lone protocol: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi off, ESP-Now added to protocols");
    }
    // If WiFi is on but ESP NOW is off, then enable LR protocol
    else if (wifiState > WIFI_STATE_OFF && espnowState == ESPNOW_OFF)
    {
        if (WiFi.getMode() != WIFI_STA)
            WiFi.mode(WIFI_STA);

        // Enable WiFi + ESP-Now
        // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
        // esp_wifi_set_protocol requires WiFi to be started
        response = esp_wifi_set_protocol(WIFI_IF_STA,
                                         WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
        if (response != ESP_OK)
            systemPrintf("espnowStart: Error setting ESP-Now + WiFi protocols: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi on, ESP-Now added to protocols");
    }

    // If ESP-Now is already active, do nothing
    else
    {
        log_d("ESP-Now already on");
    }

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        systemPrintln("espnowStart: Error starting ESP-Now");
        return;
    }

    // Use promiscuous callback to capture RSSI of packet
    response = esp_wifi_set_promiscuous(true);
    if (response != ESP_OK)
        systemPrintf("espnowStart: Error setting promiscuous mode: %s\r\n", esp_err_to_name(response));

    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);

    // Register callbacks
    // esp_now_register_send_cb(espnowOnDataSent);
    esp_now_register_recv_cb(espnowOnDataReceived);

    if (settings.espnowPeerCount == 0)
    {
        espnowSetState(ESPNOW_ON);
    }
    else
    {
        // If we already have peers, move to paired state
        espnowSetState(ESPNOW_PAIRED);

        log_d("Adding %d espnow peers", settings.espnowPeerCount);
        for (int x = 0; x < settings.espnowPeerCount; x++)
        {
            if (esp_now_is_peer_exist(settings.espnowPeers[x]) == true)
                log_d("Peer already exists");
            else
            {
                esp_err_t result = espnowAddPeer(settings.espnowPeers[x]);
                if (result != ESP_OK)
                    log_d("Failed to add peer #%d", x);
            }
        }
    }

    if (settings.espnowBroadcast == true)
    {
        // Add broadcast peer if override is turned on
        uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        if (esp_now_is_peer_exist(broadcastMac) == true)
            log_d("Broadcast peer already exists");
        else
        {
            esp_err_t result = espnowAddPeer(broadcastMac, false); // Encryption not support for broadcast MAC
            if (result != ESP_OK)
                log_d("Failed to add broadcast peer");
        }
    }

    systemPrintln("ESP-Now Started");
#endif // COMPILE_ESPNOW
}

// If WiFi is already enabled, simply remove the LR protocol
// If WiFi is off, stop the radio entirely
void espnowStop()
{
#ifdef COMPILE_ESPNOW
    if (espnowState == ESPNOW_OFF)
        return;

    // Turn off promiscuous WiFi mode
    esp_err_t response = esp_wifi_set_promiscuous(false);
    if (response != ESP_OK)
        systemPrintf("espnowStop: Failed to set promiscuous mode: %s\r\n", esp_err_to_name(response));

    esp_wifi_set_promiscuous_rx_cb(nullptr);

    // Deregister callbacks
    // esp_now_unregister_send_cb();
    response = esp_now_unregister_recv_cb();
    if (response != ESP_OK)
        systemPrintf("espnowStop: Failed to unregister receive callback: %s\r\n", esp_err_to_name(response));

    // Forget all ESP-Now Peers
    for (int x = 0; x < settings.espnowPeerCount; x++)
        espnowRemovePeer(settings.espnowPeers[x]);

    if (WiFi.getMode() != WIFI_STA)
        WiFi.mode(WIFI_STA);

    // Leave WiFi with default settings (no WIFI_PROTOCOL_LR for ESP NOW)
    // esp_wifi_set_protocol requires WiFi to be started
    response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (response != ESP_OK)
        systemPrintf("espnowStop: Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
    else
        log_d("WiFi on, ESP-Now added to protocols");

    // Deinit ESP-NOW
    if (esp_now_deinit() != ESP_OK)
    {
        systemPrintln("Error deinitializing ESP-NOW");
        return;
    }

    espnowSetState(ESPNOW_OFF);

    if (wifiState == WIFI_STATE_OFF)
    {
        // ESP Now was the only thing using the radio so turn WiFi radio off entirely
        WiFi.mode(WIFI_OFF);

        log_d("WiFi Radio off entirely");
    }
    // If WiFi is on, then restart WiFi
    else if (wifiState > WIFI_STATE_OFF)
    {
        log_d("ESP-Now starting WiFi");
        wifiStart(); // Force WiFi to restart
    }

#endif // COMPILE_ESPNOW
}

// Start ESP-Now if needed, put ESP-Now into broadcast state
void espnowBeginPairing()
{
    espnowStart();

    // To begin pairing, we must add the broadcast MAC to the peer list
    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    espnowAddPeer(broadcastMac, false); // Encryption is not supported for multicast addresses

    espnowSetState(ESPNOW_PAIRING);
}

// Regularly call during pairing to see if we've received a Pairing message
bool espnowIsPaired()
{
#ifdef COMPILE_ESPNOW
    if (espnowState == ESPNOW_MAC_RECEIVED)
    {

        if (settings.espnowBroadcast == false)
        {
            // Remove broadcast peer
            uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            espnowRemovePeer(broadcastMac);
        }

        if (esp_now_is_peer_exist(receivedMAC) == true)
            log_d("Peer already exists");
        else
        {
            // Add new peer to system
            espnowAddPeer(receivedMAC);

            // Record this MAC to peer list
            memcpy(settings.espnowPeers[settings.espnowPeerCount], receivedMAC, 6);
            settings.espnowPeerCount++;
            settings.espnowPeerCount %= ESPNOW_MAX_PEERS;
        }

        // Send message directly to the received MAC (not unicast), then exit
        espnowSendPairMessage(receivedMAC);

        // Enable radio. User may have arrived here from the setup menu rather than serial menu.
        settings.radioType = RADIO_ESPNOW;

        recordSystemSettings(); // Record radioType and espnowPeerCount to NVM

        espnowSetState(ESPNOW_PAIRED);
        return (true);
    }
#endif // COMPILE_ESPNOW
    return (false);
}

// Create special pair packet to a given MAC
esp_err_t espnowSendPairMessage(uint8_t *sendToMac)
{
#ifdef COMPILE_ESPNOW
    // Assemble message to send
    PairMessage pairMessage;

    // Get unit MAC address
    memcpy(pairMessage.macAddress, wifiMACAddress, 6);
    pairMessage.encrypt = false;
    pairMessage.channel = 0;

    pairMessage.crc = 0; // Calculate CRC
    for (int x = 0; x < 6; x++)
        pairMessage.crc += wifiMACAddress[x];

    return (esp_now_send(sendToMac, (uint8_t *)&pairMessage, sizeof(pairMessage))); // Send packet to given MAC
#else                                                                               // COMPILE_ESPNOW
    return (ESP_OK);
#endif                                                                              // COMPILE_ESPNOW
}

// Add a given MAC address to the peer list
esp_err_t espnowAddPeer(uint8_t *peerMac)
{
    return (espnowAddPeer(peerMac, true)); // Encrypt by default
}

esp_err_t espnowAddPeer(uint8_t *peerMac, bool encrypt)
{
#ifdef COMPILE_ESPNOW
    esp_now_peer_info_t peerInfo;

    memcpy(peerInfo.peer_addr, peerMac, 6);
    peerInfo.channel = 0;
    peerInfo.ifidx = WIFI_IF_STA;
    // memcpy(peerInfo.lmk, "RTKProductsLMK56", 16);
    // peerInfo.encrypt = encrypt;
    peerInfo.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result != ESP_OK)
        log_d("Failed to add peer: 0x%02X%02X%02X%02X%02X%02X", peerMac[0], peerMac[1], peerMac[2], peerMac[3],
              peerMac[4], peerMac[5]);
    return (result);
#else  // COMPILE_ESPNOW
    return (ESP_OK);
#endif // COMPILE_ESPNOW
}

// Remove a given MAC address from the peer list
esp_err_t espnowRemovePeer(uint8_t *peerMac)
{
#ifdef COMPILE_ESPNOW
    esp_err_t response = esp_now_del_peer(peerMac);
    if (response != ESP_OK)
        log_d("Failed to remove peer: %s", esp_err_to_name(response));

    return (response);
#else  // COMPILE_ESPNOW
    return (ESP_OK);
#endif // COMPILE_ESPNOW
}

// Update the state of the ESP Now state machine
void espnowSetState(ESPNOWState newState)
{
    if (espnowState == newState)
        systemPrint("*");
    espnowState = newState;

    systemPrint("espnowState: ");
    switch (newState)
    {
    case ESPNOW_OFF:
        systemPrintln("ESPNOW_OFF");
        break;
    case ESPNOW_ON:
        systemPrintln("ESPNOW_ON");
        break;
    case ESPNOW_PAIRING:
        systemPrintln("ESPNOW_PAIRING");
        break;
    case ESPNOW_MAC_RECEIVED:
        systemPrintln("ESPNOW_MAC_RECEIVED");
        break;
    case ESPNOW_PAIRED:
        systemPrintln("ESPNOW_PAIRED");
        break;
    default:
        systemPrintf("Unknown ESPNOW state: %d\r\n", newState);
        break;
    }
}

void espnowProcessRTCM(byte incoming)
{
#ifdef COMPILE_ESPNOW
    if (espnowState == ESPNOW_PAIRED)
    {
        // Move this byte into ESP NOW to send buffer
        espnowOutgoing[espnowOutgoingSpot++] = incoming;
        espnowLastAdd = millis();

        if (espnowOutgoingSpot == sizeof(espnowOutgoing))
        {
            espnowOutgoingSpot = 0; // Wrap

            if (settings.espnowBroadcast == false)
                esp_now_send(0, (uint8_t *)&espnowOutgoing, sizeof(espnowOutgoing)); // Send packet to all peers
            else
            {
                uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                esp_now_send(broadcastMac, (uint8_t *)&espnowOutgoing,
                             sizeof(espnowOutgoing)); // Send packet via broadcast
            }

            delay(10); // We need a small delay between sending multiple packets

            espnowBytesSent += sizeof(espnowOutgoing);

            espnowOutgoingRTCM = true;
        }
    }
#endif // COMPILE_ESPNOW
}

// A blocking function that is used to pair two devices
// either through the serial menu or AP config
void espnowStaticPairing()
{
    systemPrintln("Begin ESP NOW Pairing");

    // Start ESP-Now if needed, put ESP-Now into broadcast state
    espnowBeginPairing();

    // Begin sending our MAC every 250ms until a remote device sends us there info
    randomSeed(millis());

    systemPrintln("Begin pairing. Place other unit in pairing mode. Press any key to exit.");
    clearBuffer();

    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    bool exitPair = false;
    while (exitPair == false)
    {
        if (systemAvailable())
        {
            systemPrintln("User pressed button. Pairing canceled.");
            break;
        }

        int timeout = 1000 + random(0, 100); // Delay 1000 to 1100ms
        for (int x = 0; x < timeout; x++)
        {
            delay(1);

            if (espnowIsPaired() == true) // Check if we've received a pairing message
            {
                systemPrintln("Pairing compete");
                exitPair = true;
                break;
            }
        }

        espnowSendPairMessage(broadcastMac); // Send unit's MAC address over broadcast, no ack, no encryption

        systemPrintln("Scanning for other radio...");
    }
}
