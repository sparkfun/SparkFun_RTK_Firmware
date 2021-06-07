//Control the messages that get logged to SD
//Control max logging time (limit to a certain number of minutes)
//The main use case is the setup for a base station to log RAW sentences that then get post processed
void menuLog()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Logging Menu"));

    if (settings.enableSD && online.microSD)
      Serial.println(F("microSD card is online"));
    else
    {
      beginSD(); //Test if SD is present
      if (online.microSD == true)
        Serial.println(F("microSD card online"));
      else
        Serial.println(F("No microSD card is detected"));
    }

    Serial.print(F("1) Log to microSD: "));
    if (settings.enableLogging == true) Serial.println(F("Enabled"));
    else Serial.println(F("Disabled"));

    if (settings.enableLogging == true)
    {
      Serial.print(F("2) Set max logging time: "));
      Serial.print(settings.maxLogTime_minutes);
      Serial.println(F(" minutes"));
    }

    Serial.println(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.enableLogging ^= 1;
    }
    else if (incoming == '2' && settings.enableLogging == true)
    {
      Serial.print(F("Enter max minutes to log data: "));
      int maxMinutes = getNumber(menuTimeout); //Timeout after x seconds
      if (maxMinutes < 0 || maxMinutes > 60 * 48) //Arbitrary 48 hour limit
      {
        Serial.println(F("Error: max minutes out of range"));
      }
      else
      {
        settings.maxLogTime_minutes = maxMinutes; //Recorded to NVM and file at main menu exit
      }
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

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessages()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Messages Menu"));

    Serial.println(F("1) Set NMEA Messages"));
    Serial.println(F("2) Set RTCM Messages"));
    Serial.println(F("3) Set RXM Messages"));
    Serial.println(F("4) Set NAV Messages"));
    Serial.println(F("5) Set MON Messages"));
    Serial.println(F("6) Set TIM Messages"));
    Serial.println(F("7) Reset to Surveying Defaults (NMEAx5)"));
    Serial.println(F("8) Reset to PPP Logging Defaults (NMEAx5 + RXMx2)"));
    Serial.println(F("9) Turn off all messages"));
    Serial.println(F("10) Turn on all messages"));

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      menuMessagesNMEA();
    else if (incoming == 2)
      menuMessagesRTCM();
    else if (incoming == 3)
      menuMessagesRXM();
    else if (incoming == 4)
      menuMessagesNAV();
    else if (incoming == 5)
      menuMessagesMON();
    else if (incoming == 6)
      menuMessagesTIM();
    else if (incoming == 7)
    {
      setGNSSMessageRates(0); //Turn off all messages
      settings.message.nmea_gga.msgRate = 1;
      settings.message.nmea_gsa.msgRate = 1;
      settings.message.nmea_gst.msgRate = 1;
      settings.message.nmea_gsv.msgRate = 4; //One update per 4 fixes to avoid swamping SPP connection
      settings.message.nmea_rmc.msgRate = 1;
      Serial.println(F("Reset to Surveying Defaults (NMEAx5)"));
    }
    else if (incoming == 8)
    {
      setGNSSMessageRates(0); //Turn off all messages
      settings.message.nmea_gga.msgRate = 1;
      settings.message.nmea_gsa.msgRate = 1;
      settings.message.nmea_gst.msgRate = 1;
      settings.message.nmea_gsv.msgRate = 4; //One update per 4 fixes to avoid swamping SPP connection
      settings.message.nmea_rmc.msgRate = 1;

      settings.message.rxm_rawx.msgRate = 1;
      settings.message.rxm_sfrbx.msgRate = 1;
      Serial.println(F("Reset to PPP Logging Defaults (NMEAx5 + RXMx2)"));
    }
    else if (incoming == 9)
    {
      setGNSSMessageRates(0); //Turn off all messages
      Serial.println(F("All messages disabled"));
    }
    else if (incoming == 10)
    {
      setGNSSMessageRates(1); //Turn on all messages
      Serial.println(F("All messages enabled"));
    }

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars

  bool response = configureGNSSMessageRates(); //Make sure the appropriate messages are enabled
  if (response == false)
  {
    Serial.println(F("menuMessages: Failed to enable UART1 messages - Try 1"));
    //Try again
    response = configureGNSSMessageRates(); //Make sure the appropriate messages are enabled
    if (response == false)
      Serial.println(F("menuMessages: Failed to enable UART1 messages - Try 2"));
    else
      Serial.println(F("menuMessages: UART1 messages successfully enabled"));
  }
  else
  {
    Serial.println(F("menuMessages: UART1 messages successfully enabled"));
  }

}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesNMEA()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message NMEA Menu"));

    Serial.print(F("1) Message NMEA DTM: "));
    Serial.println(settings.message.nmea_dtm.msgRate);

    Serial.print(F("2) Message NMEA GBS: "));
    Serial.println(settings.message.nmea_gbs.msgRate);

    Serial.print(F("3) Message NMEA GGA: "));
    Serial.println(settings.message.nmea_gga.msgRate);

    Serial.print(F("4) Message NMEA GLL: "));
    Serial.println(settings.message.nmea_gll.msgRate);

    Serial.print(F("5) Message NMEA GNS: "));
    Serial.println(settings.message.nmea_gns.msgRate);


    Serial.print(F("6) Message NMEA GRS: "));
    Serial.println(settings.message.nmea_grs.msgRate);

    Serial.print(F("7) Message NMEA GSA: "));
    Serial.println(settings.message.nmea_gsa.msgRate);

    Serial.print(F("8) Message NMEA GST: "));
    Serial.println(settings.message.nmea_gst.msgRate);

    Serial.print(F("9) Message NMEA GSV: "));
    Serial.println(settings.message.nmea_gsv.msgRate);

    Serial.print(F("10) Message NMEA RMC: "));
    Serial.println(settings.message.nmea_rmc.msgRate);


    Serial.print(F("11) Message NMEA VLW: "));
    Serial.println(settings.message.nmea_vlw.msgRate);

    Serial.print(F("12) Message NMEA VTG: "));
    Serial.println(settings.message.nmea_vtg.msgRate);

    Serial.print(F("13) Message NMEA ZDA: "));
    Serial.println(settings.message.nmea_zda.msgRate);


    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.nmea_dtm);
    else if (incoming == 2)
      inputMessageRate(settings.message.nmea_gbs);
    else if (incoming == 3)
      inputMessageRate(settings.message.nmea_gga);
    else if (incoming == 4)
      inputMessageRate(settings.message.nmea_gll);
    else if (incoming == 5)
      inputMessageRate(settings.message.nmea_gns);
    else if (incoming == 6)
      inputMessageRate(settings.message.nmea_grs);
    else if (incoming == 7)
      inputMessageRate(settings.message.nmea_gsa);
    else if (incoming == 8)
      inputMessageRate(settings.message.nmea_gst);
    else if (incoming == 9)
      inputMessageRate(settings.message.nmea_gsv);
    else if (incoming == 10)
      inputMessageRate(settings.message.nmea_rmc);
    else if (incoming == 11)
      inputMessageRate(settings.message.nmea_vlw);
    else if (incoming == 12)
      inputMessageRate(settings.message.nmea_vtg);
    else if (incoming == 13)
      inputMessageRate(settings.message.nmea_zda);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesNAV()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message NAV Menu"));

    Serial.print(F("1) Message NAV CLOCK: "));
    Serial.println(settings.message.nav_clock.msgRate);

    Serial.print(F("2) Message NAV DOP: "));
    Serial.println(settings.message.nav_dop.msgRate);

    Serial.print(F("3) Message NAV EOE: "));
    Serial.println(settings.message.nav_eoe.msgRate);

    Serial.print(F("4) Message NAV GEOFENCE: "));
    Serial.println(settings.message.nav_geofence.msgRate);

    Serial.print(F("5) Message NAV HPPOSECEF: "));
    Serial.println(settings.message.nav_hpposecef.msgRate);


    Serial.print(F("6) Message NAV HPPOSLLH: "));
    Serial.println(settings.message.nav_hpposllh.msgRate);

    Serial.print(F("7) Message NAV ODO: "));
    Serial.println(settings.message.nav_odo.msgRate);

    Serial.print(F("8) Message NAV ORB: "));
    Serial.println(settings.message.nav_orb.msgRate);

    Serial.print(F("9) Message NAV POSECEF: "));
    Serial.println(settings.message.nav_posecef.msgRate);

    Serial.print(F("10) Message NAV POSLLH: "));
    Serial.println(settings.message.nav_posllh.msgRate);


    Serial.print(F("11) Message NAV PVT: "));
    Serial.println(settings.message.nav_pvt.msgRate);

    Serial.print(F("12) Message NAV RELPOSNED: "));
    Serial.println(settings.message.nav_relposned.msgRate);

    Serial.print(F("13) Message NAV SAT: "));
    Serial.println(settings.message.nav_sat.msgRate);

    Serial.print(F("14) Message NAV SIG: "));
    Serial.println(settings.message.nav_sig.msgRate);

    Serial.print(F("15) Message NAV STATUS: "));
    Serial.println(settings.message.nav_status.msgRate);


    Serial.print(F("16) Message NAV SVIN: "));
    Serial.println(settings.message.nav_svin.msgRate);

    Serial.print(F("17) Message NAV TIMEBDS: "));
    Serial.println(settings.message.nav_timebds.msgRate);

    Serial.print(F("18) Message NAV TIMEGAL: "));
    Serial.println(settings.message.nav_timegal.msgRate);

    Serial.print(F("19) Message NAV TIMEGLO: "));
    Serial.println(settings.message.nav_timeglo.msgRate);

    Serial.print(F("20) Message NAV TIMEGPS: "));
    Serial.println(settings.message.nav_timegps.msgRate);


    Serial.print(F("21) Message NAV TIMELS: "));
    Serial.println(settings.message.nav_timels.msgRate);

    Serial.print(F("22) Message NAV TIMEUTC: "));
    Serial.println(settings.message.nav_timeutc.msgRate);

    Serial.print(F("23) Message NAV VELECEF: "));
    Serial.println(settings.message.nav_velecef.msgRate);

    Serial.print(F("24) Message NAV VELNED: "));
    Serial.println(settings.message.nav_velned.msgRate);


    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.nav_clock);
    else if (incoming == 2)
      inputMessageRate(settings.message.nav_dop);
    else if (incoming == 3)
      inputMessageRate(settings.message.nav_eoe);
    else if (incoming == 4)
      inputMessageRate(settings.message.nav_geofence);
    else if (incoming == 5)
      inputMessageRate(settings.message.nav_hpposecef);

    else if (incoming == 6)
      inputMessageRate(settings.message.nav_hpposllh);
    else if (incoming == 7)
      inputMessageRate(settings.message.nav_odo);
    else if (incoming == 8)
      inputMessageRate(settings.message.nav_orb);
    else if (incoming == 9)
      inputMessageRate(settings.message.nav_posecef);
    else if (incoming == 10)
      inputMessageRate(settings.message.nav_posllh);

    else if (incoming == 11)
      inputMessageRate(settings.message.nav_pvt);
    else if (incoming == 12)
      inputMessageRate(settings.message.nav_relposned);
    else if (incoming == 13)
      inputMessageRate(settings.message.nav_sat);
    else if (incoming == 14)
      inputMessageRate(settings.message.nav_sig);
    else if (incoming == 15)
      inputMessageRate(settings.message.nav_status);

    else if (incoming == 16)
      inputMessageRate(settings.message.nav_svin);
    else if (incoming == 17)
      inputMessageRate(settings.message.nav_timebds);
    else if (incoming == 18)
      inputMessageRate(settings.message.nav_timegal);
    else if (incoming == 19)
      inputMessageRate(settings.message.nav_timeglo);
    else if (incoming == 20)
      inputMessageRate(settings.message.nav_timegps);

    else if (incoming == 21)
      inputMessageRate(settings.message.nav_timels);
    else if (incoming == 22)
      inputMessageRate(settings.message.nav_timeutc);
    else if (incoming == 23)
      inputMessageRate(settings.message.nav_velecef);
    else if (incoming == 24)
      inputMessageRate(settings.message.nav_velned);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesRXM()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message RXM Menu"));

    Serial.print(F("1) Message RXM MEASX: "));
    Serial.println(settings.message.rxm_measx.msgRate);

    Serial.print(F("2) Message RXM RAWX: "));
    Serial.println(settings.message.rxm_rawx.msgRate);

    Serial.print(F("3) Message RXM RLM: "));
    Serial.println(settings.message.rxm_rlm.msgRate);

    Serial.print(F("4) Message RXM RTCM: "));
    Serial.println(settings.message.rxm_rtcm.msgRate);

    Serial.print(F("5) Message RXM SFRBX: "));
    Serial.println(settings.message.rxm_sfrbx.msgRate);


    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.rxm_measx);
    else if (incoming == 2)
      inputMessageRate(settings.message.rxm_rawx);
    else if (incoming == 3)
      inputMessageRate(settings.message.rxm_rlm);
    else if (incoming == 4)
      inputMessageRate(settings.message.rxm_rtcm);
    else if (incoming == 5)
      inputMessageRate(settings.message.rxm_sfrbx);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesMON()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message MON Menu"));

    Serial.print(F("1) Message MON COMMS: "));
    Serial.println(settings.message.mon_comms.msgRate);

    Serial.print(F("2) Message MON HW2: "));
    Serial.println(settings.message.mon_hw2.msgRate);

    Serial.print(F("3) Message MON HW3: "));
    Serial.println(settings.message.mon_hw3.msgRate);

    Serial.print(F("4) Message MON HW: "));
    Serial.println(settings.message.mon_hw.msgRate);

    Serial.print(F("5) Message MON IO: "));
    Serial.println(settings.message.mon_io.msgRate);


    Serial.print(F("6) Message MON MSGPP: "));
    Serial.println(settings.message.mon_msgpp.msgRate);

    Serial.print(F("7) Message MON RF: "));
    Serial.println(settings.message.mon_rf.msgRate);

    Serial.print(F("8) Message MON RXBUF: "));
    Serial.println(settings.message.mon_rxbuf.msgRate);

    Serial.print(F("9) Message MON RXR: "));
    Serial.println(settings.message.mon_rxr.msgRate);

    Serial.print(F("10) Message MON TXBUF: "));
    Serial.println(settings.message.mon_txbuf.msgRate);



    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.mon_comms);
    else if (incoming == 2)
      inputMessageRate(settings.message.mon_hw2);
    else if (incoming == 3)
      inputMessageRate(settings.message.mon_hw3);
    else if (incoming == 4)
      inputMessageRate(settings.message.mon_hw);
    else if (incoming == 5)
      inputMessageRate(settings.message.mon_io);

    else if (incoming == 6)
      inputMessageRate(settings.message.mon_msgpp);
    else if (incoming == 7)
      inputMessageRate(settings.message.mon_rf);
    else if (incoming == 8)
      inputMessageRate(settings.message.mon_rxbuf);
    else if (incoming == 9)
      inputMessageRate(settings.message.mon_rxr);
    else if (incoming == 10)
      inputMessageRate(settings.message.mon_txbuf);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesTIM()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message TIM Menu"));

    Serial.print(F("1) Message TIM TM2: "));
    Serial.println(settings.message.tim_tm2.msgRate);

    Serial.print(F("2) Message TIM TP: "));
    Serial.println(settings.message.tim_tp.msgRate);

    Serial.print(F("3) Message TIM VRFY: "));
    Serial.println(settings.message.tim_vrfy.msgRate);

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.tim_tm2);
    else if (incoming == 2)
      inputMessageRate(settings.message.tim_tp);
    else if (incoming == 3)
      inputMessageRate(settings.message.tim_vrfy);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Control the messages that get broadcast over Bluetooth and logged (if enabled)
void menuMessagesRTCM()
{
  while (1)
  {
    Serial.println();
    Serial.println(F("Menu: Message RTCM Menu"));

    Serial.print(F("1) Message RTCM 1005: "));
    Serial.println(settings.message.rtcm_1005.msgRate);

    Serial.print(F("2) Message RTCM 1074: "));
    Serial.println(settings.message.rtcm_1074.msgRate);

    Serial.print(F("3) Message RTCM 1077: "));
    Serial.println(settings.message.rtcm_1077.msgRate);

    Serial.print(F("4) Message RTCM 1084: "));
    Serial.println(settings.message.rtcm_1084.msgRate);

    Serial.print(F("5) Message RTCM 1087: "));
    Serial.println(settings.message.rtcm_1087.msgRate);


    Serial.print(F("6) Message RTCM 1094: "));
    Serial.println(settings.message.rtcm_1094.msgRate);

    Serial.print(F("7) Message RTCM 1097: "));
    Serial.println(settings.message.rtcm_1097.msgRate);

    Serial.print(F("8) Message RTCM 1124: "));
    Serial.println(settings.message.rtcm_1124.msgRate);

    Serial.print(F("9) Message RTCM 1127: "));
    Serial.println(settings.message.rtcm_1127.msgRate);

    Serial.print(F("10) Message RTCM 1230: "));
    Serial.println(settings.message.rtcm_1230.msgRate);


    Serial.print(F("11) Message RTCM 4072.0: "));
    Serial.println(settings.message.rtcm_4072_0.msgRate);

    Serial.print(F("12) Message RTCM 4072.1: "));
    Serial.println(settings.message.rtcm_4072_1.msgRate);

    Serial.println(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      inputMessageRate(settings.message.rtcm_1005);
    else if (incoming == 2)
      inputMessageRate(settings.message.rtcm_1074);
    else if (incoming == 3)
      inputMessageRate(settings.message.rtcm_1077);
    else if (incoming == 4)
      inputMessageRate(settings.message.rtcm_1084);
    else if (incoming == 5)
      inputMessageRate(settings.message.rtcm_1087);

    else if (incoming == 6)
      inputMessageRate(settings.message.rtcm_1094);
    else if (incoming == 7)
      inputMessageRate(settings.message.rtcm_1097);
    else if (incoming == 8)
      inputMessageRate(settings.message.rtcm_1124);
    else if (incoming == 9)
      inputMessageRate(settings.message.rtcm_1127);
    else if (incoming == 10)
      inputMessageRate(settings.message.rtcm_1230);

    else if (incoming == 11)
      inputMessageRate(settings.message.rtcm_4072_0);
    else if (incoming == 12)
      inputMessageRate(settings.message.rtcm_4072_1);

    else if (incoming == STATUS_PRESSED_X)
      break;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }

  while (Serial.available()) Serial.read(); //Empty buffer of any newline chars
}

//Prompt the user to enter the message rate for a given ID
//Assign the given value to the message
void inputMessageRate(ubxMsg &message)
{
  Serial.printf("Enter %s message rate (0 to disable): ", message.msgTextName);
  int64_t rate = getNumber(menuTimeout); //Timeout after x seconds

  while (rate < 0 || rate > 60) //Arbitrary 60 fixes per report limit
  {
    Serial.println(F("Error: message rate out of range"));
    Serial.printf("Enter %s message rate (0 to disable): ", message.msgTextName);
    rate = getNumber(menuTimeout); //Timeout after x seconds

    if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
      return; //Give up
  }

  if (rate == STATUS_GETNUMBER_TIMEOUT || rate == STATUS_PRESSED_X)
    return;

  message.msgRate = rate;
}

//Updates the message rates on the ZED-F9P for all known messages
bool configureGNSSMessageRates()
{
  bool response = true;

  //NMEA
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_dtm);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gbs);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gga);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gll);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gns);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_grs);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gsa);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gst);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_gsv);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_rmc);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_vlw);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_vtg);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nmea_zda);

  //NAV
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_clock);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_dop);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_eoe);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_geofence);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_hpposecef);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_hpposllh);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_odo);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_orb);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_posecef);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_posllh);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_pvt);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_relposned);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_sat);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_sig);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_status);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_svin);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timebds);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timegal);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timeglo);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timegps);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timels);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_timeutc);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_velecef);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.nav_velned);

  //RXM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_measx);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rawx);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rlm);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_rtcm);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rxm_sfrbx);

  //MON
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_comms);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw2);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw3);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_hw);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_io);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_msgpp);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rf);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rxbuf);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_rxr);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.mon_txbuf);

  //TIM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_tm2);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_tp);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.tim_vrfy);

  //RTCM
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1005);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1074);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1077);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1084);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1087);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1094);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1097);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1124);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1127);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_1230);

  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_4072_0);
  response &= configureMessageRate(COM_PORT_UART1, settings.message.rtcm_4072_1);

  return (response);
}

//Set all GNSS message report rates to one value
//Useful for turning on or off all messages for resetting and testing
void setGNSSMessageRates(uint8_t msgRate)
{
  //NMEA
  settings.message.nmea_dtm.msgRate = msgRate;
  settings.message.nmea_gbs.msgRate = msgRate;
  settings.message.nmea_gga.msgRate = msgRate;
  settings.message.nmea_gll.msgRate = msgRate;
  settings.message.nmea_gns.msgRate = msgRate;

  settings.message.nmea_grs.msgRate = msgRate;
  settings.message.nmea_gsa.msgRate = msgRate;
  settings.message.nmea_gst.msgRate = msgRate;
  settings.message.nmea_gsv.msgRate = msgRate;
  settings.message.nmea_rmc.msgRate = msgRate;

  settings.message.nmea_vlw.msgRate = msgRate;
  settings.message.nmea_vtg.msgRate = msgRate;
  settings.message.nmea_zda.msgRate = msgRate;

  //NAV
  settings.message.nav_clock.msgRate = msgRate;
  settings.message.nav_dop.msgRate = msgRate;
  settings.message.nav_eoe.msgRate = msgRate;
  settings.message.nav_geofence.msgRate = msgRate;
  settings.message.nav_hpposecef.msgRate = msgRate;

  settings.message.nav_hpposllh.msgRate = msgRate;
  settings.message.nav_odo.msgRate = msgRate;
  settings.message.nav_orb.msgRate = msgRate;
  settings.message.nav_posecef.msgRate = msgRate;
  settings.message.nav_posllh.msgRate = msgRate;

  settings.message.nav_pvt.msgRate = msgRate;
  settings.message.nav_relposned.msgRate = msgRate;
  settings.message.nav_sat.msgRate = msgRate;
  settings.message.nav_sig.msgRate = msgRate;
  settings.message.nav_status.msgRate = msgRate;

  settings.message.nav_svin.msgRate = msgRate;
  settings.message.nav_timebds.msgRate = msgRate;
  settings.message.nav_timegal.msgRate = msgRate;
  settings.message.nav_timeglo.msgRate = msgRate;
  settings.message.nav_timegps.msgRate = msgRate;

  settings.message.nav_timels.msgRate = msgRate;
  settings.message.nav_timeutc.msgRate = msgRate;
  settings.message.nav_velecef.msgRate = msgRate;
  settings.message.nav_velned.msgRate = msgRate;

  //RXM
  settings.message.rxm_measx.msgRate = msgRate;
  settings.message.rxm_rawx.msgRate = msgRate;
  settings.message.rxm_rlm.msgRate = msgRate;
  settings.message.rxm_rtcm.msgRate = msgRate;
  settings.message.rxm_sfrbx.msgRate = msgRate;

  //MON
  settings.message.mon_comms.msgRate = msgRate;
  settings.message.mon_hw2.msgRate = msgRate;
  settings.message.mon_hw3.msgRate = msgRate;
  settings.message.mon_hw.msgRate = msgRate;
  settings.message.mon_io.msgRate = msgRate;

  settings.message.mon_msgpp.msgRate = msgRate;
  settings.message.mon_rf.msgRate = msgRate;
  settings.message.mon_rxbuf.msgRate = msgRate;
  settings.message.mon_rxr.msgRate = msgRate;
  settings.message.mon_txbuf.msgRate = msgRate;

  //TIM
  settings.message.tim_tm2.msgRate = msgRate;
  settings.message.tim_tp.msgRate = msgRate;
  settings.message.tim_vrfy.msgRate = msgRate;

  //RTCM
  settings.message.rtcm_1005.msgRate = msgRate;
  settings.message.rtcm_1074.msgRate = msgRate;
  settings.message.rtcm_1077.msgRate = msgRate;
  settings.message.rtcm_1084.msgRate = msgRate;
  settings.message.rtcm_1087.msgRate = msgRate;

  settings.message.rtcm_1094.msgRate = msgRate;
  settings.message.rtcm_1097.msgRate = msgRate;
  settings.message.rtcm_1124.msgRate = msgRate;
  settings.message.rtcm_1127.msgRate = msgRate;
  settings.message.rtcm_1230.msgRate = msgRate;

  settings.message.rtcm_4072_0.msgRate = msgRate;
  settings.message.rtcm_4072_1.msgRate = msgRate;
}

//Given a message, set the message rate on the ZED-F9P
bool configureMessageRate(uint8_t portID, ubxMsg message)
{
  uint8_t currentSendRate = getMessageRate(message.msgClass, message.msgID, portID); //Qeury the module for the current setting

  bool response = true;
  if (currentSendRate != message.msgRate)
    response &= i2cGNSS.configureMessage(message.msgClass, message.msgID, portID, message.msgRate); //Update setting
  return response;
}

//Lookup the send rate for a given message+port
uint8_t getMessageRate(uint8_t msgClass, uint8_t msgID, uint8_t portID)
{
  ubxPacket customCfg = {0, 0, 0, 0, 0, settingPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

  customCfg.cls = UBX_CLASS_CFG; // This is the message Class
  customCfg.id = UBX_CFG_MSG; // This is the message ID
  customCfg.len = 2;
  customCfg.startingSpot = 0; // Always set the startingSpot to zero (unless you really know what you are doing)

  uint16_t maxWait = 1250; // Wait for up to 1250ms (Serial may need a lot longer e.g. 1100)

  settingPayload[0] = msgClass;
  settingPayload[1] = msgID;

  // Read the current setting. The results will be loaded into customCfg.
  if (i2cGNSS.sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED) // We are expecting data and an ACK
  {
    Serial.printf("getMessageSetting failed: Class-0x%02X ID-0x%02X\n\r", msgClass, msgID);
    return (false);
  }

  uint8_t sendRate = settingPayload[2 + portID];

  return (sendRate);
}

//Creates a log if logging is enabled, and SD is detected
//Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
void beginLogging()
{
  if (online.logging == false)
  {
    if (online.microSD == true && settings.enableLogging == true)
    {
      char fileName[40] = "";

      i2cGNSS.checkUblox();

      if (reuseLastLog == true) //attempt to use previous log
      {
        if (findLastLog(fileName) == false)
        {
          Serial.println(F("Failed to find last log. Making new one."));
        }
      }

      if (strlen(fileName) == 0)
      {
        //Based on GPS data/time, create a log file in the format SFE_Surveyor_YYMMDD_HHMMSS.ubx
        bool timeValid = false;
        if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
          timeValid = true;
        if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
          timeValid = true;

        if (timeValid == false)
        {
          Serial.println(F("beginLoggingUBX: No date/time available. No file created."));
          delay(1000); //Give the receiver time to get a lock
          online.logging = false;
          return;
        }

        sprintf(fileName, "%s_%02d%02d%02d_%02d%02d%02d.ubx", //SdFat library
                platformFilePrefix,
                i2cGNSS.getYear() - 2000, i2cGNSS.getMonth(), i2cGNSS.getDay(),
                i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond()
               );
      }

      //Attempt to write to file system. This avoids collisions with file writing in F9PSerialReadTask()
      if (xSemaphoreTake(xFATSemaphore, fatSemaphore_longWait_ms) == pdPASS)
      {
        // O_CREAT - create the file if it does not exist
        // O_APPEND - seek to the end of the file prior to each write
        // O_WRITE - open for write
        if (ubxFile.open(fileName, O_CREAT | O_APPEND | O_WRITE) == false)
        {
          Serial.printf("Failed to create GNSS UBX data file: %s\n\r", fileName);
          delay(1000);
          online.logging = false;
          xSemaphoreGive(xFATSemaphore);
          return;
        }

        updateDataFileCreate(&ubxFile); // Update the file to create time & date

        startLogTime_minutes = millis() / 1000L / 60; //Mark now as start of logging

        //TODO For debug only, remove
        if (reuseLastLog == true)
        {
          Serial.println(F("Appending last available log"));
          ubxFile.println("Append file due to system reset");
        }

        xSemaphoreGive(xFATSemaphore);
      }
      else
      {
        Serial.println(F("Failed to get file system lock to create GNSS UBX data file"));
        online.logging = false;
        return;
      }

      Serial.printf("Log file created: %s\n\r", fileName);
      online.logging = true;
    }
    else
      online.logging = false;
  }
}

//Updates the timestemp on a given data file
void updateDataFileAccess(SdFile *dataFile)
{
  bool timeValid = false;
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
    timeValid = true;
  if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
    timeValid = true;

  if (timeValid == true)
  {
    //Update the file access time
    dataFile->timestamp(T_ACCESS, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
    //Update the file write time
    dataFile->timestamp(T_WRITE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
  }
}

void updateDataFileCreate(SdFile *dataFile)
{
  bool timeValid = false;
  if (i2cGNSS.getTimeValid() == true && i2cGNSS.getDateValid() == true) //Will pass if ZED's RTC is reporting (regardless of GNSS fix)
    timeValid = true;
  if (i2cGNSS.getConfirmedTime() == true && i2cGNSS.getConfirmedDate() == true) //Requires GNSS fix
    timeValid = true;

  if (timeValid == true)
  {
    //Update the file create time
    dataFile->timestamp(T_CREATE, i2cGNSS.getYear(), i2cGNSS.getMonth(), i2cGNSS.getDay(), i2cGNSS.getHour(), i2cGNSS.getMinute(), i2cGNSS.getSecond());
  }
}

//Finds last log
//Returns true if succesful
bool findLastLog(char *lastLogName)
{
  bool foundAFile = false;

  if (online.microSD == true)
  {
    //Attempt to access file system. This avoids collisions with file writing in F9PSerialReadTask()
    //Wait up to 5s, this is important
    if (xSemaphoreTake(xFATSemaphore, 5000 / portTICK_PERIOD_MS) == pdPASS)
    {
      //Count available binaries
      SdFile tempFile;
      SdFile dir;
      const char* LOG_EXTENSION = "ubx";
      const char* LOG_PREFIX = platformFilePrefix;
      char fname[50]; //Handle long file names

      dir.open("/"); //Open root

      while (tempFile.openNext(&dir, O_READ))
      {
        if (tempFile.isFile())
        {
          tempFile.getName(fname, sizeof(fname));

          //Check for matching file name prefix and extension
          if (strcmp(LOG_EXTENSION, &fname[strlen(fname) - strlen(LOG_EXTENSION)]) == 0)
          {
            if (strstr(fname, LOG_PREFIX) != NULL)
            {
              strcpy(lastLogName, fname); //Store this file as last known log file
              foundAFile = true;
            }
          }
        }
        tempFile.close();
      }

      xSemaphoreGive(xFATSemaphore);
    }
  }

  return (foundAFile);
}
