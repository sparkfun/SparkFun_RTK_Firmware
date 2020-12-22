//These are all the low frequency tasks that are called by Ticker

//Control BT status LED according to bluetoothState
void updateBTled()
{
  if (radioState == BT_ON_NOCONNECTION)
    digitalWrite(bluetoothStatusLED, !digitalRead(bluetoothStatusLED));
  else if (radioState == BT_CONNECTED)
    digitalWrite(bluetoothStatusLED, HIGH);
  else
    digitalWrite(bluetoothStatusLED, LOW);
}
