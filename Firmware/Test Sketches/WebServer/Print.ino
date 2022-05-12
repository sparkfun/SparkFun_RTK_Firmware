void printIpAddress(IPAddress ip) {

    // Display the IP address
    Serial.println(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printWiFiGatewayIp() {

    // Display the subnet mask
    Serial.print ("Gateway: ");
    IPAddress ip = WiFi.gatewayIP();
    printIpAddress(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printWiFiIpAddress() {

    // Display the IP address
    Serial.print ("IP Address: ");
    IPAddress ip = WiFi.localIP();
    printIpAddress(ip);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printWiFiMacAddress() {
    // Display the MAC address
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    Serial.print(mac[5], HEX);
    Serial.print(":");
    Serial.print(mac[4], HEX);
    Serial.print(":");
    Serial.print(mac[3], HEX);
    Serial.print(":");
    Serial.print(mac[2], HEX);
    Serial.print(":");
    Serial.print(mac[1], HEX);
    Serial.print(":");
    Serial.println(mac[0], HEX);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printWiFiNetwork() {

    // Display the SSID
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // Display the receive signal strength
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI):");
    Serial.println(rssi);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printWiFiSubnetMask() {

    // Display the subnet mask
    Serial.print ("Subnet Mask: ");
    IPAddress ip = WiFi.subnetMask();
    printIpAddress(ip);
}
