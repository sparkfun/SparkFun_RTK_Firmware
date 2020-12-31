
const int batteryLevelLED_Red = 32;
const int batteryLevelLED_Green = 33;

const int freq = 5000;
const int ledRedChannel = 0;
const int ledGreenChannel = 1;
const int resolution = 8;

const int positionAccuracyLED_1cm = 2;
const int baseStatusLED = 4;
const int baseSwitch = 5;
const int bluetoothStatusLED = 12;
const int positionAccuracyLED_100cm = 13;
const int positionAccuracyLED_10cm = 15;
const byte PIN_MICROSD_CHIP_SELECT = 25;
const int zed_tx_ready = 26;
const int zed_reset = 27;
const int batteryLevel_alert = 36;

void setup() {
  ledcSetup(ledRedChannel, freq, resolution);
  ledcSetup(ledGreenChannel, freq, resolution);

  ledcAttachPin(batteryLevelLED_Red, ledRedChannel);
  ledcAttachPin(batteryLevelLED_Green, ledGreenChannel);

  //ledcWrite(ledRedChannel, 128);
  //ledcWrite(ledGreenChannel, 128);

  pinMode(positionAccuracyLED_1cm, OUTPUT);
  pinMode(positionAccuracyLED_10cm, OUTPUT);
  pinMode(positionAccuracyLED_100cm, OUTPUT);
  pinMode(baseStatusLED, OUTPUT);
  pinMode(bluetoothStatusLED, OUTPUT);

  digitalWrite(positionAccuracyLED_1cm, HIGH);
  digitalWrite(positionAccuracyLED_10cm, HIGH);
  digitalWrite(positionAccuracyLED_100cm, HIGH);
  digitalWrite(baseStatusLED, HIGH);
  digitalWrite(bluetoothStatusLED, HIGH);
  
}

void loop() {
  ledcWrite(ledRedChannel, 128);
  ledcWrite(ledGreenChannel, 128);
  delay(2500);                       // wait for a second
  ledcWrite(ledRedChannel, 0);
  ledcWrite(ledGreenChannel, 0);
  delay(2500);                       // wait for a second
}
