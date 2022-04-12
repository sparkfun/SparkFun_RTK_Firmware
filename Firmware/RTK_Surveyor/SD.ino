/* 
  These are low level functions to aid in detecting whether a card is present or not.
  Because of ESP32 v2 core, SdFat can only operate using Shared SPI. This makes the sd.begin test take over 1s
  which causes the RTK product to boot slowly. To circumvent this, we will ping the SD card directly to see if it responds.
  Failures take 2ms, successes take 1ms.

  From Prototype puzzle: https://github.com/sparkfunX/ThePrototype/blob/master/Firmware/TestSketches/sdLocker/sdLocker.ino
  License: Public domain. This code is based on Karl Lunt's work: https://www.seanet.com/~karllunt/sdlocker2.html  
*/

//Define commands for the SD card
#define  SD_GO_IDLE      (0x40 + 0)      // CMD0 - go to idle state
#define  SD_INIT      (0x40 + 1)      // CMD1 - start initialization
#define  SD_SEND_IF_COND  (0x40 + 8)      // CMD8 - send interface (conditional), works for SDHC only
#define  SD_SEND_STATUS   (0x40 + 13)     // CMD13 - send card status
#define  SD_SET_BLK_LEN   (0x40 + 16)     // CMD16 - set length of block in bytes
#define  SD_LOCK_UNLOCK   (0x40 + 42)     // CMD42 - lock/unlock card
#define  CMD55        (0x40 + 55)     // multi-byte preface command
#define  SD_READ_OCR    (0x40 + 58)     // read OCR
#define  SD_ADV_INIT    (0xc0 + 41)     // ACMD41, for SDHC cards - advanced start initialization

//Define options for accessing the SD card's PWD (CMD42)
#define  MASK_ERASE         0x08    //erase the entire card
#define  MASK_LOCK_UNLOCK   0x04    //lock or unlock the card with password
#define  MASK_CLR_PWD       0x02    //clear password
#define  MASK_SET_PWD       0x01    //set password

//Define bit masks for fields in the lock/unlock command (CMD42) data structure
#define  SET_PWD_MASK    (1<<0)
#define  CLR_PWD_MASK   (1<<1)
#define  LOCK_UNLOCK_MASK (1<<2)
#define  ERASE_MASK     (1<<3)

//Begin initialization by sending CMD0 and waiting until SD card
//responds with In Idle Mode (0x01).  If the response is not 0x01
//within a reasonable amount of time, there is no SD card on the bus.
//Returns false if not card is detected
//Returns true if a card responds
bool sdPresent(void)
{
  byte response = 0;

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  pinMode(pin_microSD_CS, OUTPUT);

  //Sending clocks while card power stabilizes...
  deselectCard();             // always make sure
  for (byte i = 0; i < 30; i++) // send several clocks while card power stabilizes
    xchg(0xff);

  //Sending CMD0 - GO IDLE...
  for (byte i = 0; i < 0x10; i++) //Attempt to go idle
  {
    response = sdSendCommand(SD_GO_IDLE, 0);  // send CMD0 - go to idle state
    if (response == 1) break;
  }
  if (response != 1) return (false); //Card failed to respond to idle

  return (true);
}

/*
    sdSendCommand      send raw command to SD card, return response

    This routine accepts a single SD command and a 4-byte argument.  It sends
    the command plus argument, adding the appropriate CRC.  It then returns
    the one-byte response from the SD card.

    For advanced commands (those with a command byte having bit 7 set), this
    routine automatically sends the required preface command (CMD55) before
    sending the requested command.

    Upon exit, this routine returns the response byte from the SD card.
    Possible responses are:
      0xff  No response from card; card might actually be missing
      0x01  SD card returned 0x01, which is OK for most commands
      0x??  other responses are command-specific
*/
byte sdSendCommand(byte command, unsigned long arg)
{
  byte response;

  if (command & 0x80)         // special case, ACMD(n) is sent as CMD55 and CMDn
  {
    command &= 0x7f;   // strip high bit for later
    response = sdSendCommand(CMD55, 0); // send first part (recursion)
    if (response > 1) return (response);
  }

  deselectCard();
  xchg(0xFF);
  selectCard(); // enable CS
  xchg(0xFF);

  xchg(command | 0x40);       // command always has bit 6 set!
  xchg((byte)(arg >> 24)); // send data, starting with top byte
  xchg((byte)(arg >> 16));
  xchg((byte)(arg >> 8));
  xchg((byte)(arg & 0xFF));

  byte crc = 0x01;             // good for most cases
  if (command == SD_GO_IDLE) crc = 0x95;     // this will be good enough for most commands
  if (command == SD_SEND_IF_COND) crc = 0x87;  // special case, have to use different CRC
  xchg(crc);                  // send final byte

  for (int i = 0; i < 30; i++)    // loop until timeout or response
  {
    response = xchg(0xFF);
    if ((response & 0x80) == 0) break; // high bit cleared means we got a response
  }

  /*
      We have issued the command but the SD card is still selected.  We
      only deselectCard the card if the command we just sent is NOT a command
      that requires additional data exchange, such as reading or writing
      a block.
  */
  if ((command != SD_READ_OCR) &&
      (command != SD_SEND_STATUS) &&
      (command != SD_SEND_IF_COND) &&
      (command != SD_LOCK_UNLOCK))
  {
    deselectCard();             // all done
    xchg(0xFF);             // close with eight more clocks
  }

  return (response);        // let the caller sort it out
}

//Select (enable) the SD card
void selectCard(void)
{
  digitalWrite(pin_microSD_CS, LOW);
}

//Deselect (disable) the SD card
void deselectCard(void)
{
  digitalWrite(pin_microSD_CS, HIGH);
}

//Exchange a byte of data with the SD card via host's SPI bus
byte xchg(byte val)
{
  byte receivedVal = SPI.transfer(val);
  return receivedVal;
}
