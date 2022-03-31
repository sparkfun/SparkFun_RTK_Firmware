//Check to see if we've received serial over USB
//Report status if ~ received, otherwise present config menu
void updateSerial()
{
  if (Serial.available()) 
  {
    byte incoming = Serial.read();
    
    if(incoming == '~')
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
  displaySerialConfig(); //Display 'Serial Config' while user is configuring

  while (1)
  {
    Serial.println();
    Serial.printf("SparkFun RTK %s v%d.%d-%s\r\n", platformPrefix, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, __DATE__);

    Serial.print(F("** Bluetooth broadcasting as: "));
    Serial.print(deviceName);
    Serial.println(F(" **"));

    Serial.println(F("Menu: Main Menu"));

    Serial.println(F("1) Configure GNSS Receiver"));

    Serial.println(F("2) Configure GNSS Messages"));

    if (zedModuleType == PLATFORM_F9P)
      Serial.println(F("3) Configure Base"));
    else if (zedModuleType == PLATFORM_F9R)
      Serial.println(F("3) Configure Sensor Fusion"));

    Serial.println(F("4) Configure Ports"));

    Serial.println(F("5) Configure Logging"));

    Serial.println(F("p) Configure Profiles"));

    Serial.println(F("s) System Status"));

    if (binCount > 0)
      Serial.println(F("f) Firmware upgrade"));

    //Serial.println(F("t) Test menu"));

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
      menuGNSS();
    else if (incoming == '2')
      menuMessages();
    else if (incoming == '3' && zedModuleType == PLATFORM_F9P)
      menuBase();
    else if (incoming == '3' && zedModuleType == PLATFORM_F9R)
      menuSensorFusion();
    else if (incoming == '4')
      menuPorts();
    else if (incoming == '5')
      menuLog();
    else if (incoming == 's')
      menuSystem();
    else if (incoming == 'p')
      menuUserProfiles();
    else if (incoming == 'f' && binCount > 0)
      menuFirmware();
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  recordSystemSettings(); //Once all menus have exited, record the new settings to LittleFS and config file

  if (online.gnss == true)
    i2cGNSS.saveConfiguration(); //Save the current settings to flash and BBR on the ZED-F9P

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Change system wide settings based on current user profile
void menuUserProfiles()
{
  int menuTimeoutExtended = 30; //Increase time needed for complex data entry (mount point ID, ECEF coords, etc).
  uint8_t originalProfileNumber = getProfileNumber();

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: User Profiles Menu"));

    Serial.printf("Current profile: %d-%s\n\r", getProfileNumber() + 1, settings.profileName);
    Serial.println("1) Change profile");

    Serial.printf("2) Edit profile name: %s\n\r", settings.profileName);

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      Serial.println();
      Serial.println("Available Profiles:");

      //List available profiles
      for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
      {
        //With the given profile number, load appropriate settings file
        char settingsFileName[40];
        sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, x);

        char tempProfileName[50];

        if (LittleFS.exists(settingsFileName))
        {
          //Open this profile and get the profile name from it
          File settingsFile = LittleFS.open(settingsFileName, FILE_READ);

          Settings tempSettings;
          uint8_t *settingsBytes = (uint8_t *)&tempSettings; // Cast the struct into a uint8_t ptr

          uint16_t fileSize = settingsFile.size();
          if (fileSize > sizeof(tempSettings)) fileSize = sizeof(tempSettings); //Trim to max setting struct size

          settingsFile.read(settingsBytes, fileSize); //Copy the bytes from file into testSettings struct
          settingsFile.close();

          strcpy(tempProfileName, tempSettings.profileName);
        }
        else
        {
          sprintf(tempProfileName, "(Empty)", x + 1);
        }

        Serial.printf("%d) %s\n\r", x + 1, tempProfileName);
      }

      //Wait for user to select new profile number
      int incoming = getNumber(menuTimeout); //Timeout after x seconds

      if (incoming < 1 || incoming > MAX_PROFILE_COUNT)
      {
        Serial.println(F("Error: Profile number out of range"));
      }
      else
      {
        recordSystemSettings(); //Before switching, we need to record the current settings to LittleFS and SD

        recordProfileNumber(incoming - 1); //Align to array
      }
      //Reset settings to default
      Settings defaultSettings;
      settings = defaultSettings;

      //Load the settings file with the most recent profileNumber
      //If the settings file doesn't exist, defaults will be applied
      getSettings(getProfileNumber(), settings);
    }
    else if (incoming == '2')
    {
      Serial.print("Enter new profile name: ");
      readLine(settings.profileName, sizeof(settings.profileName), menuTimeoutExtended);
      recordSystemSettings(); //We need to update this immediately in case user lists the available profiles again
    }

    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
    {
      break;
    }
    else
      printUnknown(incoming);
  }

  if (originalProfileNumber != getProfileNumber())
  {
    Serial.println(F("Changing profiles. Rebooting. Goodbye!"));
    delay(2000);
    ESP.restart();
  }

  //A user may edit the name of a profile, but then switch back to original profile.
  //Thus, no reset, and activeProfiles is not updated. Do it here.
  activeProfiles = getActiveProfiles(); //Count is used during menu display

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Erase all settings. Upon restart, unit will use defaults
void factoryReset()
{
  displaySytemReset(); //Display friendly message on OLED

  LittleFS.format();

  //Assemble settings file name
  char settingsFileName[40]; //SFE_Express_Plus_Settings.txt
  strcpy(settingsFileName, platformFilePrefix);
  strcat(settingsFileName, "_Settings.txt");

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (settings.enableSD && online.microSD)
  {
    if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      if (sd.exists(settingsFileName))
        sd.remove(settingsFileName);
      xSemaphoreGive(xFATSemaphore);
    } //End xFATSemaphore
  }

  if (online.gnss == true)
    i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

  Serial.println(F("Settings erased successfully. Rebooting. Goodbye!"));
  delay(2000);
  ESP.restart();
}
