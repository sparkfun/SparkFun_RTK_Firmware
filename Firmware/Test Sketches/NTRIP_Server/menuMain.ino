

//Base status goes from Rover-Mode (LED off), surveying in (blinking), to survey is complete/trasmitting RTCM (solid)
typedef enum
{
  BASE_OFF = 0,
  BASE_SURVEYING_IN_NOTSTARTED, //User has indicated base, but current pos accuracy is too low
  BASE_SURVEYING_IN_SLOW,
  BASE_SURVEYING_IN_FAST,
  BASE_TRANSMITTING,
} BaseState;


void menuMain()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu:");
    Serial.println("1) Connect to RTK2GO");

    byte incoming = getByteChoice(30); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.println("Xmit to RTK2Go. Press key to stop");
      delay(10); //Wait for any serial to arrive
      while (Serial.available()) Serial.read(); //Flush

      WiFiClient client;

      while (Serial.available() == 0)
      {
        //Spin until we have a frame ready
        //Serial.print("Getting frame");
        while (toCastFrameStale == true)
        {
          //Serial.print(".");
          delay(100);
        }

        //Connect if we are not already
        if (client.connected() == false)
        {
          Serial.printf("Opening socket to %s\n", casterHost);

          if (client.connect(casterHost, casterPort) == true) //Attempt connection
          {
            Serial.printf("Connected to %s:%d\n", casterHost, casterPort);

            const int SERVER_BUFFER_SIZE  = 512;
            char serverBuffer[SERVER_BUFFER_SIZE];

            snprintf(serverBuffer, SERVER_BUFFER_SIZE, "SOURCE %s /%s\r\nSource-Agent: NTRIP %s/v%d.%d\r\n\r\n",
                     mntpnt_pw, mntpnt, ntrip_server_name, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);

            Serial.printf("Sending credentials:\n%s\n", serverBuffer);
            client.write(serverBuffer, strlen(serverBuffer));

            //Wait for response
            unsigned long timeout = millis();
            while (client.available() == 0)
            {
              if (millis() - timeout > 5000)
              {
                Serial.println(">>> Client Timeout !");
                client.stop();
                return;
              }
              delay(10);
            }

            //Check reply
            bool connectionSuccess = false;
            char response[512];
            int responseSpot = 0;
            while (client.available())
            {
              response[responseSpot++] = client.read();
              if (strstr(response, "200") > 0) //Look for 'ICY 200 OK'
                connectionSuccess = true;
              if (responseSpot == 512 - 1) break;
            }
            response[responseSpot] = '\0';
            //Serial.printf("RTK2Go response: %s", response);

            if (connectionSuccess == false)
            {
              Serial.printf("Failed to connect to RTK2Go: %s", response);
            }
          } //End attempt to connect
          else
          {
            Serial.println("Connection to host failed");
          }
        } //End connected == false

        if (client.connected() == true)
        {
          //We are already connected so now push RTCM frame
          client.write(toCastBuffer, castSpot);
          serverBytesSent += castSpot;
          toCastFrameStale = true;
          Serial.printf("Sent: %d / Total sent: %d\n", castSpot, serverBytesSent);
        }

        //Close socket if we don't have new data for 10s
        if (millis() - lastReceivedCastFrame_ms > maxTimeBeforeHangup_ms)
        {
          Serial.println("RTCM timeout. Disconnecting...");
          client.stop();
        }

        delay(10);
      }

      Serial.println("User pressed a key");
      Serial.println("Disconnecting...");
      client.stop();
    }
    else if (incoming == '2')
    {
      Serial.println("Hi");
    }
    else if (incoming == 0xFF)
      break;
    else
    {
      Serial.printf("Unknown: 0x%02X\n", incoming);
    }
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

int baseState = BASE_TRANSMITTING;

//If the ZED has any new NMEA data, pass it out over Bluetooth
//Task for reading data from the GNSS receiver.
void F9PSerialReadTask(void *e)
{
  while (true)
  {
    //If we are in base transmitting mode and user wants to serve to client, check if 100ms has passed without new serial.
    //This indicates end of RTCM frame. This presumes NMEA and RAWX sentences are turned off, and 1Hz
    if (baseState == BASE_TRANSMITTING) //&& settings.serveToCaster == true)
    {
      if (toCastFrameComplete == false)
      {
        if (millis() - lastReceivedRTCM_ms > 800)
        {
          toCastFrameComplete = true;
          toCastFrameStale = false; //ntripServer() will now post buffer next socket connection
          lastReceivedCastFrame_ms = millis();
        }
      }
    }

    if (GPS.available())
    {
      auto s = GPS.readBytes(rBuffer, SERIAL_SIZE_RX);

      //If we are in base transmitting mode and user wants to serve to client, then fill toCastBuffer
      if (baseState == BASE_TRANSMITTING) //&& settings.serveToCaster == true)
      {
        if (toCastFrameComplete == true) //Start of a new frame so flush old data
        {
          castSpot = 0;
          toCastFrameComplete = false;
        }

        lastReceivedRTCM_ms = millis();

        //Append latest incoming UART bytes (should be RTCM) to outgoing cast buffer
        if (castSpot + s >= TOCAST_BUFFER_SIZE)
        {
          Serial.println("Overrun");
          for (int x = 0 ; x < TOCAST_BUFFER_SIZE ; x++)
            Serial.write(toCastBuffer[x]);
        }
        else
        {
          for (int x = 0 ; x < s ; x++)
          {
            toCastBuffer[castSpot] = rBuffer[x];
            castSpot++;
          }
        }
      }
      //      else if (SerialBT.connected())
      //      {
      //        SerialBT.write(rBuffer, s);
      //      }
    }
    taskYIELD();
  }
}
