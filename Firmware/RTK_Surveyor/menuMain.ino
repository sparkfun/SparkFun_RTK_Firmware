//Check to see if we've received serial over USB
//Report status if ~ received, otherwise present config menu
void updateSerial()
{
  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == '~')
    {
      //Output custom GNTXT message with all current system data
      printCurrentConditionsNMEA();
    }
    else
      menuMain(); //Present user menu
  }
}

//Display the options
//If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
  inMainMenu = true;
  displaySerialConfig(); //Display 'Serial Config' while user is configuring

  while (1)
  {
    Serial.println();
#ifdef ENABLE_DEVELOPER
    Serial.printf("SparkFun RTK %s v%d.%d-RC-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
#else
    Serial.printf("SparkFun RTK %s v%d.%d-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);
#endif

#ifdef COMPILE_BT
    Serial.print("** Bluetooth broadcasting as: ");
    Serial.print(deviceName);
    Serial.println(" **");
#else
    Serial.println("** Bluetooth Not Compiled **");
#endif

    Serial.println("Menu: Main");

    Serial.println("1) Configure GNSS Receiver");

    Serial.println("2) Configure GNSS Messages");

    if (zedModuleType == PLATFORM_F9P)
      Serial.println("3) Configure Base");
    else if (zedModuleType == PLATFORM_F9R)
      Serial.println("3) Configure Sensor Fusion");

    Serial.println("4) Configure Ports");

    Serial.println("5) Configure Logging");

    Serial.println("p) Configure User Profiles");

#ifdef COMPILE_ESPNOW
    Serial.println("r) Configure Radios");
#endif

    if (online.lband == true)
      Serial.println("P) Configure PointPerfect");

    Serial.println("s) Configure System");

    if (binCount > 0)
      Serial.println("f) Firmware upgrade");

    Serial.println("x) Exit");

    byte incoming = getCharacterNumber();

    if (incoming == 1)
      menuGNSS();
    else if (incoming == 2)
      menuMessages();
    else if (incoming == 3 && zedModuleType == PLATFORM_F9P)
      menuBase();
    else if (incoming == 3 && zedModuleType == PLATFORM_F9R)
      menuSensorFusion();
    else if (incoming == 4)
      menuPorts();
    else if (incoming == 5)
      menuLog();
    else if (incoming == 's')
      menuSystem();
    else if (incoming == 'p')
      menuUserProfiles();
    else if (incoming == 'P' && online.lband == true)
      menuPointPerfect();
#ifdef COMPILE_ESPNOW
    else if (incoming == 'r')
      menuRadio();
#endif
    else if (incoming == 'f' && binCount > 0)
      menuFirmware();
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_EMPTY)
      break;
    else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  recordSystemSettings(); //Once all menus have exited, record the new settings to LittleFS and config file

  if (online.gnss == true)
    i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR on the ZED-F9P

  //Reboot as base only if currently operating as a base station
  if (restartBase && (systemState >= STATE_BASE_NOT_STARTED) && (systemState < STATE_BUBBLE_LEVEL))
  {
    restartBase = false;
    requestChangeState(STATE_BASE_NOT_STARTED); //Restart base upon exit for latest changes to take effect
  }

  if (restartRover == true)
  {
    restartRover = false;
    requestChangeState(STATE_ROVER_NOT_STARTED); //Restart rover upon exit for latest changes to take effect
  }

  clearBuffer(); //Empty buffer of any newline chars
  inMainMenu = false;
}

//Change system wide settings based on current user profile
//Ways to change the ZED settings:
//Menus - we apply ZED changes at the exit of each sub menu
//Settings file - we detect differences between NVM and settings txt file and updateZEDSettings = true
//Profile - Before profile is changed, set updateZEDSettings = true
//AP - once new settings are parsed, set updateZEDSettings = true
//Setup button -
//Factory reset - updatesZEDSettings = true by default
void menuUserProfiles()
{
  uint8_t originalProfileNumber = profileNumber;

  bool forceReset = false; //If we reset a profile to default, the profile number has not changed, but we still need to reset

  while (1)
  {
    Serial.println();
    Serial.println("Menu: User Profiles");

    //List available profiles
    for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
    {
      if (activeProfiles & (1 << x))
        Serial.printf("%d) Select %s", x + 1, profileNames[x]);
      else
        Serial.printf("%d) Select (Empty)", x + 1);

      if (x == profileNumber) Serial.print(" <- Current");

      Serial.println();
    }

    Serial.printf("%d) Edit profile name: %s\r\n", MAX_PROFILE_COUNT + 1, profileNames[profileNumber]);

    Serial.printf("%d) Set profile '%s' to factory defaults\r\n", MAX_PROFILE_COUNT + 2, profileNames[profileNumber]);

    Serial.printf("%d) Delete profile '%s'\r\n", MAX_PROFILE_COUNT + 3, profileNames[profileNumber]);

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming >= 1 && incoming <= MAX_PROFILE_COUNT)
    {
      changeProfileNumber(incoming - 1); //Align inputs to array
    }
    else if (incoming == MAX_PROFILE_COUNT + 1)
    {
      Serial.print("Enter new profile name: ");
      getString(settings.profileName, sizeof(settings.profileName));
      recordSystemSettings(); //We need to update this immediately in case user lists the available profiles again
      setProfileName(profileNumber);
    }
    else if (incoming == MAX_PROFILE_COUNT + 2)
    {
      Serial.printf("\r\nReset profile '%s' to factory defaults. Press 'y' to confirm:", profileNames[profileNumber]);
      byte bContinue = getCharacterNumber();
      if (bContinue == 'y')
      {
        settingsToDefaults(); //Overwrite our current settings with defaults

        recordSystemSettings(); //Overwrite profile file and NVM with these settings

        //Get bitmask of active profiles
        activeProfiles = loadProfileNames();

        forceReset = true; //Upon exit of menu, reset the device
      }
      else
        Serial.println("Reset aborted");
    }
    else if (incoming == MAX_PROFILE_COUNT + 3)
    {
      Serial.printf("\r\nDelete profile '%s'. Press 'y' to confirm:", profileNames[profileNumber]);
      byte bContinue = getCharacterNumber();
      if (bContinue == 'y')
      {
        //Remove profile from LittleFS
        if (LittleFS.exists(settingsFileName))
          LittleFS.remove(settingsFileName);

        //Remove profile from SD if available
        if (online.microSD == true)
        {
          if (sd->exists(settingsFileName))
            sd->remove(settingsFileName);
        }

        recordProfileNumber(0); //Move to Profile1
        profileNumber = 0;

        sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, profileNumber); //Update file name with new profileNumber

        //We need to load these settings from file so that we can record a profile name change correctly
        bool responseLFS = loadSystemSettingsFromFileLFS(settingsFileName, &settings);
        bool responseSD = loadSystemSettingsFromFileSD(settingsFileName, &settings);

        //If this is an empty/new profile slot, overwrite our current settings with defaults
        if (responseLFS == false && responseSD == false)
        {
          settingsToDefaults();
        }

        //Get bitmask of active profiles
        activeProfiles = loadProfileNames();
      }
      else
        Serial.println("Delete aborted");
    }

    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  if (originalProfileNumber != profileNumber || forceReset == true)
  {
    Serial.println("Rebooting to apply new profile settings. Goodbye!");
    delay(2000);
    ESP.restart();
  }

  //A user may edit the name of a profile, but then switch back to original profile.
  //Thus, no reset, and activeProfiles is not updated. Do it here.
  //Get bitmask of active profiles
  activeProfiles = loadProfileNames();

  clearBuffer(); //Empty buffer of any newline chars
}

//Change the active profile number, without unit reset
void changeProfileNumber(byte newProfileNumber)
{
  settings.updateZEDSettings = true; //When this profile is loaded next, force system to update ZED settings.
  recordSystemSettings(); //Before switching, we need to record the current settings to LittleFS and SD

  recordProfileNumber(newProfileNumber);
  profileNumber = newProfileNumber;
  setSettingsFileName(); //Load the settings file name into memory (enabled profile name delete)

  //We need to load these settings from file so that we can record a profile name change correctly
  bool responseLFS = loadSystemSettingsFromFileLFS(settingsFileName, &settings);
  bool responseSD = loadSystemSettingsFromFileSD(settingsFileName, &settings);

  //If this is an empty/new profile slot, overwrite our current settings with defaults
  if (responseLFS == false && responseSD == false)
  {
    Serial.println("Default the settings");
    settingsToDefaults();
  }
}

//Erase all settings. Upon restart, unit will use defaults
void factoryReset()
{
  displaySytemReset(); //Display friendly message on OLED

  tasksStopUART2();

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (settings.enableSD && online.microSD)
  {
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      //Remove this specific settings file. Don't remove the other profiles.
      sd->remove(settingsFileName);

      sd->remove(stationCoordinateECEFFileName); //Remove station files
      sd->remove(stationCoordinateGeodeticFileName);
      
      xSemaphoreGive(sdCardSemaphore);
    } //End sdCardSemaphore
    else
    {
      //An error occurs when a settings file is on the microSD card and it is not
      //deleted, as such the settings on the microSD card will be loaded when the
      //RTK reboots, resulting in failure to achieve the factory reset condition
      Serial.printf("sdCardSemaphore failed to yield, menuMain.ino line %d\r\n", __LINE__);
    }
  }
  
  Serial.println("Formatting file system...");
  LittleFS.format();

  if (online.gnss == true)
    i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

  Serial.println("Settings erased successfully. Rebooting. Goodbye!");
  delay(2000);
  ESP.restart();
}

//Configure the internal radio, if available
void menuRadio()
{
#ifdef COMPILE_ESPNOW
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Radios");

    Serial.print("1) Select Radio Type: ");
    if (settings.radioType == RADIO_EXTERNAL) Serial.println("External only");
    else if (settings.radioType == RADIO_ESPNOW) Serial.println("Internal ESP-Now");

    if (settings.radioType == RADIO_ESPNOW)
    {
      //Pretty print the MAC of all radios
      Serial.print("  Radio MAC: ");
      for (int x = 0 ; x < 5 ; x++)
        Serial.printf("%02X:", wifiMACAddress[x]);
      Serial.printf("%02X\r\n", wifiMACAddress[5]);

      if (settings.espnowPeerCount > 0)
      {
        Serial.println("  Paired Radios: ");
        for (int x = 0 ; x < settings.espnowPeerCount ; x++)
        {
          Serial.print("    ");
          for (int y = 0 ; y < 5 ; y++)
            Serial.printf("%02X:", settings.espnowPeers[x][y]);
          Serial.printf("%02X\r\n", settings.espnowPeers[x][5]);
        }
      }
      else
        Serial.println("  No Paired Radios");


      Serial.println("2) Pair radios");
      Serial.println("3) Forget all radios");
#ifdef ENABLE_DEVELOPER
      Serial.println("4) Add dummy radio");
      Serial.println("5) Send dummy data");
      Serial.println("6) Broadcast dummy data");
#endif
    }

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      if (settings.radioType == RADIO_EXTERNAL) settings.radioType = RADIO_ESPNOW;
      else if (settings.radioType == RADIO_ESPNOW) settings.radioType = RADIO_EXTERNAL;
    }
    else if (settings.radioType == RADIO_ESPNOW && incoming == 2)
    {
      espnowStaticPairing();
    }
    else if (settings.radioType == RADIO_ESPNOW && incoming == 3)
    {
      Serial.println("\r\nForgetting all paired radios. Press 'y' to confirm:");
      byte bContinue = getCharacterNumber();
      if (bContinue == 'y')
      {
        if (espnowState > ESPNOW_OFF)
        {
          for (int x = 0 ; x < settings.espnowPeerCount ; x++)
            espnowRemovePeer(settings.espnowPeers[x]);
        }
        settings.espnowPeerCount = 0;
        Serial.println("Radios forgotten");
      }
    }
#ifdef ENABLE_DEVELOPER
    else if (settings.radioType == RADIO_ESPNOW && incoming == 4)
    {
      uint8_t peer1[] = {0xB8, 0xD6, 0x1A, 0x0D, 0x8F, 0x9C}; //Random MAC
      if (esp_now_is_peer_exist(peer1) == true)
        log_d("Peer already exists");
      else
      {
        //Add new peer to system
        espnowAddPeer(peer1);

        //Record this MAC to peer list
        memcpy(settings.espnowPeers[settings.espnowPeerCount], peer1, 6);
        settings.espnowPeerCount++;
        settings.espnowPeerCount %= ESPNOW_MAX_PEERS;
        recordSystemSettings();
      }

      espnowSetState(ESPNOW_PAIRED);
    }
    else if (settings.radioType == RADIO_ESPNOW && incoming == 5)
    {
      uint8_t espnowData[] = "This is the long string to test how quickly we can send one string to the other unit. I am going to need a much longer sentence if I want to get a long amount of data into one transmission. This is nearing 200 characters but needs to be near 250.";
      esp_now_send(0, (uint8_t *) &espnowData, sizeof(espnowData)); //Send packet to all peers
    }
    else if (settings.radioType == RADIO_ESPNOW && incoming == 6)
    {
      uint8_t espnowData[] = "This is the long string to test how quickly we can send one string to the other unit. I am going to need a much longer sentence if I want to get a long amount of data into one transmission. This is nearing 200 characters but needs to be near 250.";
      uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      esp_now_send(broadcastMac, (uint8_t *) &espnowData, sizeof(espnowData)); //Send packet to all peers
    }
#endif

    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  radioStart();

  clearBuffer(); //Empty buffer of any newline chars
#endif
}
