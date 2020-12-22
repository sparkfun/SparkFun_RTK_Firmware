//Get single byte from user
//Waits for and returns the character that the user provides
//Returns STATUS_GETNUMBER_TIMEOUT if input times out
//Returns 'x' if user presses 'x'
uint8_t getByteChoice(int numberOfSeconds)
{
  Serial.flush();
  delay(50);//Wait for any incoming chars to hit buffer
  while (Serial.available() > 0) Serial.read(); //Clear buffer

  long startTime = millis();
  byte incoming;
  while (1)
  {
    delay(10); //Yield to processor

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
      return (-1); //Timeout. No user input.
    }
  }

  return (incoming);
}
