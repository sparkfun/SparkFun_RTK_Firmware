void printDebug(String thingToPrint)
{
  if(settings.printDebugMessages == true)
  {
    Serial.print(thingToPrint);    
  }
}

//Option not known
void printUnknown(uint8_t unknownChoice)
{
  Serial.print(F("Unknown choice: "));
  Serial.write(unknownChoice);
  Serial.println();
}
void printUnknown(int unknownValue)
{
  Serial.print(F("Unknown value: "));
  Serial.write(unknownValue);
  Serial.println();
}

//Get single byte from user
//Waits for and returns the character that the user provides
//Returns STATUS_GETNUMBER_TIMEOUT if input times out
//Returns 'x' if user presses 'x'
uint8_t getByteChoice(int numberOfSeconds)
{
  Serial.flush();
  for (int i = 0; i < 50; i++) //Wait for any incoming chars to hit buffer
  {
    //checkBattery();
    delay(1);
  }
  while (Serial.available() > 0) Serial.read(); //Clear buffer

  long startTime = millis();
  byte incoming;
  while (1)
  {
    if (Serial.available() > 0)
    {
      incoming = Serial.read();
      //      Serial.print(F("byte: 0x"));
      //      Serial.println(incoming, HEX);
      if (incoming >= 'a' && incoming <= 'z') break;
      if (incoming >= 'A' && incoming <= 'Z') break;
      if (incoming >= '0' && incoming <= '9') break;
    }

    if ( (millis() - startTime) / 1000 >= numberOfSeconds)
    {
      Serial.println(F("No user input received."));
      return (STATUS_GETBYTE_TIMEOUT); //Timeout. No user input.
    }

    //checkBattery();
    delay(1);
  }

  return (incoming);
}
