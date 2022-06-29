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

    if (online.lband == true)
      Serial.println(F("P) Configure PointPerfect"));

    Serial.println(F("s) System Status"));

    if (binCount > 0)
      Serial.println(F("f) Firmware upgrade"));

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
    else if (incoming == 'P' && online.lband == true)
      menuPointPerfect();
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

  if (restartBase == true)
  {
    restartBase = false;
    requestChangeState(STATE_BASE_NOT_STARTED); //Restart base upon exit for latest changes to take effect
  }

  if (restartRover == true)
  {
    restartRover = false;
    requestChangeState(STATE_ROVER_NOT_STARTED); //Restart rover upon exit for latest changes to take effect
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
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
  int menuTimeoutExtended = 30; //Increase time needed for complex data entry (mount point ID, ECEF coords, etc).
  uint8_t originalProfileNumber = profileNumber;

  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: User Profiles Menu"));

    //List available profiles
    for (int x = 0 ; x < MAX_PROFILE_COUNT ; x++)
    {
      if (strlen(profileNames[x]) > 0)
        Serial.printf("%d) Select %s", x + 1, profileNames[x]);
      else
        Serial.printf("%d) Select (Empty)", x + 1);

      if (x == profileNumber) Serial.print(" <- Current");

      Serial.println();
    }

    Serial.printf("%d) Edit profile name: %s\n\r", MAX_PROFILE_COUNT + 1, profileNames[profileNumber]);

    Serial.printf("%d) Delete profile '%s'\n\r", MAX_PROFILE_COUNT + 2, profileNames[profileNumber]);

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming >= 1 && incoming <= MAX_PROFILE_COUNT)
    {
      settings.updateZEDSettings = true; //When this profile is loaded next, force system to update ZED settings.
      recordSystemSettings(); //Before switching, we need to record the current settings to LittleFS and SD

      recordProfileNumber(incoming - 1); //Align to array
      profileNumber = incoming - 1;

      sprintf(settingsFileName, "/%s_Settings_%d.txt", platformFilePrefix, profileNumber); //Enables Delete Profile

      //We need to load these settings from file so that we can record a profile name change correctly
      bool responseLFS = loadSystemSettingsFromFileLFS(settingsFileName, &settings);
      bool responseSD = loadSystemSettingsFromFileSD(settingsFileName, &settings);

      //If this is an empty/new profile slot, overwrite our current settings with defaults
      if (responseLFS == false && responseSD == false)
      {
        Settings tempSettings;
        settings = tempSettings;
      }
    }
    else if (incoming == MAX_PROFILE_COUNT + 1)
    {
      Serial.print("Enter new profile name: ");
      readLine(settings.profileName, sizeof(settings.profileName), menuTimeoutExtended);
      recordSystemSettings(); //We need to update this immediately in case user lists the available profiles again

      strcpy(profileNames[profileNumber], settings.profileName); //Update array
    }
    else if (incoming == MAX_PROFILE_COUNT + 2)
    {
      Serial.printf("\r\nDelete profile '%s'. Press 'y' to confirm:", profileNames[profileNumber]);
      byte bContinue = getByteChoice(menuTimeout);
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
          Settings tempSettings;
          settings = tempSettings;
        }

        activeProfiles = loadProfileNames(); //Count is used during menu display
      }
      else
        Serial.println(F("Delete aborted"));
    }

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  if (originalProfileNumber != profileNumber)
  {
    Serial.println(F("Changing profiles. Rebooting. Goodbye!"));
    delay(2000);
    ESP.restart();
  }

  //A user may edit the name of a profile, but then switch back to original profile.
  //Thus, no reset, and activeProfiles is not updated. Do it here.
  activeProfiles = loadProfileNames(); //Count is used during menu display

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Erase all settings. Upon restart, unit will use defaults
void factoryReset()
{
  displaySytemReset(); //Display friendly message on OLED

  Serial.println("Formatting settings file system...");
  LittleFS.format();

  //Attempt to write to file system. This avoids collisions with file writing from other functions like recordSystemSettingsToFile() and F9PSerialReadTask()
  if (settings.enableSD && online.microSD)
  {
    if (xSemaphoreTake(sdCardSemaphore, fatSemaphore_longWait_ms) == pdPASS)
    {
      //Remove this specific settings file. Don't remove the other profiles.
      sd->remove(settingsFileName);
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

  if (online.gnss == true)
    i2cGNSS.factoryReset(); //Reset everything: baud rate, I2C address, update rate, everything.

  Serial.println(F("Settings erased successfully. Rebooting. Goodbye!"));
  delay(2000);
  ESP.restart();
}
