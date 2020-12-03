/*
  August 31, 2020
  SparkFun Electronics
  Nathan Seidle

*/

int ledPin =  13; //Status LED connected to digital pin 13

const char *testString = ":abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ#";
int bytesPerSecond = 100; //Start at 100 and build up to 4k


void setup()
{
  pinMode(ledPin, OUTPUT);

  //Serial.begin(115200);
  Serial.begin(57600);
  Serial1.begin(57600); //Default speed for most SiK based radios

  delay(500); //Wait for radio to power up

  Serial.println();
  Serial.println("Radio Distance Test");
  Serial1.println();
  Serial1.println("Radio Distance Test");
}

void loop()
{
  //Run 10 transmissions at the current byte count amount
  for (int x = 0 ; x < 2 ; x++)
  {
    int currentByteCounter = 0;
    int lineNumber = 0;
    unsigned long startTime = millis();

    //Put the output together
    char myChar[5000];
//    char myChar[1000];
    myChar[0] = '\0'; //Clear buffer

    while (currentByteCounter < bytesPerSecond)
    {
      char temp[2];
      sprintf(temp, "%d", lineNumber++);
      if (lineNumber == 10) lineNumber = 0;

      strcat(myChar, temp); //Add the line number
      strcat(myChar, testString);

      sprintf(temp, "\n");
      strcat(myChar, temp); 

      currentByteCounter += strlen(testString);
    }
    
    //Transmit the buffer
    digitalWrite(ledPin, HIGH);

    Serial1.print(myChar);
    Serial1.print("Characters pushed: ");
    Serial1.print(currentByteCounter);
    Serial1.println();
    Serial1.println();

    Serial.print(myChar);
    Serial.print("Characters pushed: ");
    Serial.print(currentByteCounter);
    Serial.println();
    Serial.println();

    digitalWrite(ledPin, LOW);

    //Wait for second to expire
    while (millis() - startTime < 1000) delay(1);
  }

  //Increase byte count to next level
  if (bytesPerSecond == 100)
    bytesPerSecond = 300;
  else if (bytesPerSecond == 300)
    bytesPerSecond = 500;
  else if (bytesPerSecond == 500)
    bytesPerSecond = 1000;
  else if (bytesPerSecond == 1000)
//    bytesPerSecond = 100;
    bytesPerSecond = 2000;
  else if (bytesPerSecond == 2000)
    bytesPerSecond = 4000;
  else if (bytesPerSecond == 4000)
    bytesPerSecond = 100;
}
