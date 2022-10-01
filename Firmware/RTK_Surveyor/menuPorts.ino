void menuPorts()
{
  if (productVariant == RTK_SURVEYOR)
    menuPortsSurveyor();
  else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
    menuPortsMultiplexed();
}

//Set the baud rates for the radio and data ports
void menuPortsSurveyor()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Ports");

    Serial.print("1) Set serial baud rate for Radio Port: ");
    Serial.print(getSerialRate(COM_PORT_UART2));
    Serial.println(" bps");

    Serial.print("2) Set serial baud rate for Data Port: ");
    Serial.print(getSerialRate(COM_PORT_UART1));
    Serial.println(" bps");

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      Serial.print("Enter baud rate (4800 to 921600) for Radio Port: ");
      int newBaud = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (newBaud == 4800
            || newBaud == 9600
            || newBaud == 19200
            || newBaud == 38400
            || newBaud == 57600
            || newBaud == 115200
            || newBaud == 230400
            || newBaud == 460800
            || newBaud == 921600
           )
        {
          settings.radioPortBaud = newBaud;
          if (online.gnss == true)
            i2cGNSS.setSerialRate(newBaud, COM_PORT_UART2); //Set Radio Port
        }
        else
        {
          Serial.println("Error: Baud rate out of range");
        }
      }
    }
    else if (incoming == 2)
    {
      Serial.print("Enter baud rate (4800 to 921600) for Data Port: ");
      int newBaud = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (newBaud == 4800
            || newBaud == 9600
            || newBaud == 19200
            || newBaud == 38400
            || newBaud == 57600
            || newBaud == 115200
            || newBaud == 230400
            || newBaud == 460800
            || newBaud == 921600
           )
        {
          settings.dataPortBaud = newBaud;
          if (online.gnss == true)
            i2cGNSS.setSerialRate(newBaud, COM_PORT_UART1); //Set Data Port
        }
        else
        {
          Serial.println("Error: Baud rate out of range");
        }
      }
    }

    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Set the baud rates for the radio and data ports
void menuPortsMultiplexed()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Ports");

    Serial.print("1) Set Radio port serial baud rate: ");
    Serial.print(getSerialRate(COM_PORT_UART2));
    Serial.println(" bps");

    Serial.print("2) Set Data port connections: ");
    if (settings.dataPortChannel == MUX_UBLOX_NMEA)
      Serial.println("NMEA TX Out/RX In");
    else if (settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
      Serial.println("PPS OUT/Event Trigger In");
    else if (settings.dataPortChannel == MUX_I2C_WT)
    {
      if (zedModuleType == PLATFORM_F9P)
        Serial.println("I2C SDA/SCL");
      else if (zedModuleType == PLATFORM_F9R)
        Serial.println("Wheel Tick/Direction");
    }
    else if (settings.dataPortChannel == MUX_ADC_DAC)
      Serial.println("ESP32 DAC Out/ADC In");


    if (settings.dataPortChannel == MUX_UBLOX_NMEA)
    {
      Serial.print("3) Set Data port serial baud rate: ");
      Serial.print(getSerialRate(COM_PORT_UART1));
      Serial.println(" bps");
    }
    else if (settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
    {
      Serial.println("3) Configure External Triggers");
    }

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      Serial.print("Enter baud rate (4800 to 921600) for Radio Port: ");
      int newBaud = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (newBaud == 4800
            || newBaud == 9600
            || newBaud == 19200
            || newBaud == 38400
            || newBaud == 57600
            || newBaud == 115200
            || newBaud == 230400
            || newBaud == 460800
            || newBaud == 921600
           )
        {
          settings.radioPortBaud = newBaud;
          if (online.gnss == true)
            i2cGNSS.setSerialRate(newBaud, COM_PORT_UART2); //Set Radio Port
        }
        else
        {
          Serial.println("Error: Baud rate out of range");
        }
      }
    }
    else if (incoming == 2)
    {
      Serial.println("\n\rEnter the pin connection to use (1 to 4) for Data Port: ");
      Serial.println("1) NMEA TX Out/RX In");
      Serial.println("2) PPS OUT/Event Trigger In");
      if (zedModuleType == PLATFORM_F9P)
        Serial.println("3) I2C SDA/SCL");
      else if (zedModuleType == PLATFORM_F9R)
        Serial.println("3) Wheel Tick/Direction");
      Serial.println("4) ESP32 DAC Out/ADC In");

      int muxPort = getNumber(); //Returns EXIT, TIMEOUT, or long
      if (muxPort < 1 || muxPort > 4)
      {
        Serial.println("Error: Pin connection out of range");
      }
      else
      {
        settings.dataPortChannel = (muxConnectionType_e)(muxPort - 1); //Adjust user input from 1-4 to 0-3
        setMuxport(settings.dataPortChannel);
      }
    }
    else if (incoming == 3 && settings.dataPortChannel == MUX_UBLOX_NMEA)
    {
      Serial.print("Enter baud rate (4800 to 921600) for Data Port: ");
      int newBaud = getNumber(); //Returns EXIT, TIMEOUT, or long
      if ((newBaud != INPUT_RESPONSE_GETNUMBER_EXIT) && (newBaud != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
      {
        if (newBaud == 4800
            || newBaud == 9600
            || newBaud == 19200
            || newBaud == 38400
            || newBaud == 57600
            || newBaud == 115200
            || newBaud == 230400
            || newBaud == 460800
            || newBaud == 921600
           )
        {
          settings.dataPortBaud = newBaud;
          if (online.gnss == true)
            i2cGNSS.setSerialRate(newBaud, COM_PORT_UART1); //Set Data Port
        }
        else
        {
          Serial.println("Error: Baud rate out of range");
        }
      }
    }
    else if (incoming == 3 && settings.dataPortChannel == MUX_PPS_EVENTTRIGGER)
    {
      menuPortHardwareTriggers();
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars
}

//Configure the behavior of the PPS and INT pins on the ZED-F9P
//Most often used for logging events (inputs) and when external triggers (outputs) occur
void menuPortHardwareTriggers()
{
  while (1)
  {
    Serial.println();
    Serial.println("Menu: Port Hardware Trigger");

    Serial.print("1) Enable External Pulse: ");
    if (settings.enableExternalPulse == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    if (settings.enableExternalPulse == true)
    {
      Serial.print("2) Set time between pulses: ");
      Serial.print(settings.externalPulseTimeBetweenPulse_us / 1000.0, 0);
      Serial.println("ms");

      Serial.print("3) Set pulse length: ");
      Serial.print(settings.externalPulseLength_us / 1000.0, 0);
      Serial.println("ms");

      Serial.print("4) Set pulse polarity: ");
      if (settings.externalPulsePolarity == PULSE_RISING_EDGE) Serial.println("Rising");
      else Serial.println("Falling");
    }

    Serial.print("5) Log External Events: ");
    if (settings.enableExternalHardwareEventLogging == true) Serial.println("Enabled");
    else Serial.println("Disabled");

    Serial.println("x) Exit");

    int incoming = getNumber(); //Returns EXIT, TIMEOUT, or long

    if (incoming == 1)
    {
      settings.enableExternalPulse ^= 1;
    }
    else if (incoming == 2 && settings.enableExternalPulse == true)
    {
      Serial.print("Time between pulses in milliseconds: ");
      long pulseTime = getNumber(); //Returns EXIT, TIMEOUT, or long

      if (pulseTime != INPUT_RESPONSE_GETNUMBER_TIMEOUT && pulseTime != INPUT_RESPONSE_GETNUMBER_EXIT)
      {
        if (pulseTime < 1 || pulseTime > 60000) //60s max
          Serial.println("Error: Time between pulses out of range");
        else
        {
          settings.externalPulseTimeBetweenPulse_us = pulseTime * 1000;

          if (pulseTime < (settings.externalPulseLength_us / 1000)) //pulseTime must be longer than pulseLength
            settings.externalPulseLength_us = settings.externalPulseTimeBetweenPulse_us / 2; //Force pulse length to be 1/2 time between pulses
        }
      }

    }
    else if (incoming == 3 && settings.enableExternalPulse == true)
    {
      Serial.print("Pulse length in milliseconds: ");
      long pulseLength = getNumber(); //Returns EXIT, TIMEOUT, or long

      if (pulseLength != INPUT_RESPONSE_GETNUMBER_TIMEOUT && pulseLength != INPUT_RESPONSE_GETNUMBER_EXIT)
      {
        if (pulseLength > (settings.externalPulseTimeBetweenPulse_us / 1000)) //pulseLength must be shorter than pulseTime
          Serial.println("Error: Pulse length must be shorter than time between pulses");
        else
          settings.externalPulseLength_us = pulseLength * 1000;
      }
    }
    else if (incoming == 4 && settings.enableExternalPulse == true)
    {
      if (settings.externalPulsePolarity == PULSE_RISING_EDGE)
        settings.externalPulsePolarity = PULSE_FALLING_EDGE;
      else
        settings.externalPulsePolarity = PULSE_RISING_EDGE;
    }
    else if (incoming == 5)
    {
      settings.enableExternalHardwareEventLogging ^= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
      break;
    else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  clearBuffer(); //Empty buffer of any newline chars

  beginExternalTriggers(); //Update with new settings
}

void eventTriggerReceived(UBX_TIM_TM2_data_t ubxDataStruct)
{
  // It is the rising edge of the sound event (TRIG) which is important
  // The falling edge is less useful, as it will be "debounced" by the loop code
  if (ubxDataStruct.flags.bits.newRisingEdge) // 1 if a new rising edge was detected
  {
    Serial.println("Rising Edge Event");

    triggerCount = ubxDataStruct.count;
    towMsR = ubxDataStruct.towMsR; // Time Of Week of rising edge (ms)
    towSubMsR = ubxDataStruct.towSubMsR; // Millisecond fraction of Time Of Week of rising edge in nanoseconds

    newEventToRecord = true;
  }
}
