//----------------------------------------
// Constants
//----------------------------------------

//Left top
#define ICON_WIFI_SYMBOL_LEFT                1  //  0,  0
#define ICON_DOWN_ARROW                      2  // 16,  0
#define ICON_UP_ARROW                        4  // 16,  0
#define ICON_BT_SYMBOL                       8  //  4,  0
#define ICON_MAC_ADDRESS                  0x10  //  0,  3

//Center top
#define ICON_WIFI_SYMBOL_CENTER           0x20  // center, 0
#define ICON_BASE_TEMPORARY               0x40  // 27,  0
#define ICON_BASE_FIXED                   0x80  // 27,  0
#define ICON_ROVER_FUSION                0x100  // 27,  2
#define ICON_ROVER_FUSION_EMPTY          0x200  // 27,  2
#define ICON_DYNAMIC_MODEL               0x400  // 27,  0

//Right top
#define ICON_BATTERY                     0x800  // 45,  0

//Left center
#define ICON_CROSS_HAIR                 0x1000  //  0, 18
#define ICON_CROSS_HAIR_DUAL            0x2000  //  0, 18

//Right center
#define ICON_HORIZONTAL_ACCURACY        0x4000  // 16, 20

//Left bottom
#define ICON_SIV_ANTENNA                0x8000  //  2, 35
#define ICON_SIV_ANTENNA_LBAND         0x10000  //  2, 35

//Right bottom
#define ICON_LOGGING                   0x20000  // right, bottom

//----------------------------------------
// Locals
//----------------------------------------

static QwiicMicroOLED oled;
static uint32_t blinking_icons;
static uint32_t icons;

// Fonts
#include <res/qw_fnt_5x7.h>
#include <res/qw_fnt_8x16.h>
#include <res/qw_fnt_largenum.h>

// Icons
#include "icons.h"

//----------------------------------------
// Routines
//----------------------------------------

void beginDisplay()
{
  blinking_icons = 0;
  if (oled.begin() == true)
  {
    online.display = true;

    Serial.println(F("Display started"));
    displaySplash();
    splashStart = millis();
  }
  else
  {
    if (productVariant == RTK_SURVEYOR)
    {
      Serial.println(F("Display not detected"));
    }
    else if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
    {
      Serial.println(F("Display Error: Not detected."));
    }
  }
}

//Given the system state, display the appropriate information
void updateDisplay()
{
  //Update the display if connected
  if (online.display == true)
  {
    if (millis() - lastDisplayUpdate > 500 || forceDisplayUpdate == true) //Update display at 2Hz
    {
      lastDisplayUpdate = millis();
      forceDisplayUpdate = false;

      oled.reset(false); //Incase of previous corruption, force re-alignment of CGRAM. Do not init buffers as it takes time and causes screen to blink.

      oled.erase();

      icons = 0;
      switch (systemState)
      {

        /*
                       111111111122222222223333333333444444444455555555556666
             0123456789012345678901234567890123456789012345678901234567890123
            .----------------------------------------------------------------
           0|   *******         **             **         *****************
           1|  *       *        **             **         *               *
           2| *  *****  *       **          ******        * ***  ***  *** *
           3|*  *     *  *      **         *      *       * ***  ***  *** ***
           4|  *  ***  *        **       * * **** * *     * ***  ***  ***   *
           5|    *   *       ** ** **    * * **** * *     * ***  ***  ***   *
           6|      *          ******     * *      * *     * ***  ***  ***   *
           7|     ***          ****      * *      * *     * ***  ***  ***   *
           8|      *            **       * *      * *     * ***  ***  *** ***
           9|                            * *      * *     * ***  ***  *** *
          10|                              *      *       *               *
          11|                               ******        *****************
          12|
          13|
          14|
          15|
          16|
          17|
          18|       *
          19|       *
          20|    *******
          21|   *   *   *               ***               ***      ***
          22|  *    *    *             *   *             *   *    *   *
          23|  *    *    *             *   *             *   *    *   *
          24|  *    *    *     **       * *               * *      * *
          25|******* *******   **        *                 *        *
          26|  *    *    *              * *               * *      * *
          27|  *    *    *             *   *             *   *    *   *
          28|  *    *    *             *   *             *   *    *   *
          29|   *   *   *      **      *   *     **      *   *    *   *
          30|    *******       **       ***      **       ***      ***
          31|       *
          32|       *
          33|
          34|
          35|
          36|   **                                                  *******
          37|   * *                    ***      ***                 *     **
          38|   *  *   *              *   *    *   *                *      **
          39|   *   * *               *   *    *   *                *       *
          40|    *   *        **       * *      * *                 * ***** *
          41|    *    *       **        *        *                  *       *
          42|     *    *               * *      * *                 * ***** *
          43|     **    *             *   *    *   *                *       *
          44|     ****   *            *   *    *   *                * ***** *
          45|     **  ****    **      *   *    *   *                *       *
          46|     **          **       ***      ***                 *       *
          47|   ******                                              *********
        */

        case (STATE_ROVER_NOT_STARTED):
          icons = paintWirelessIcon() //Top left
                | ICON_BATTERY        //Top right
                | ICON_CROSS_HAIR     //Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;
        case (STATE_ROVER_NO_FIX):
          icons = paintWirelessIcon() //Top left
                | ICON_DYNAMIC_MODEL  //Top center
                | ICON_BATTERY        //Top right
                | ICON_CROSS_HAIR     //Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;
        case (STATE_ROVER_FIX):
          icons = paintWirelessIcon() //Top left
                | ICON_DYNAMIC_MODEL  //Top center
                | ICON_BATTERY        //Top right
                | ICON_CROSS_HAIR     //Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;
        case (STATE_ROVER_RTK_FLOAT):
          blinking_icons ^= ICON_CROSS_HAIR_DUAL;
          icons = paintWirelessIcon() //Top left
                | ICON_DYNAMIC_MODEL  //Top center
                | ICON_BATTERY        //Top right
                | (blinking_icons & ICON_CROSS_HAIR_DUAL)  //Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;
        case (STATE_ROVER_RTK_FIX):
          icons = paintWirelessIcon() //Top left
                | ICON_DYNAMIC_MODEL  //Top center
                | ICON_BATTERY        //Top right
                | ICON_CROSS_HAIR_DUAL//Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;

        case (STATE_BASE_NOT_STARTED):
          //Do nothing. Static display shown during state change.
          break;

        //Start of base / survey in / NTRIP mode
        //Screen is displayed while we are waiting for horz accuracy to drop to appropriate level
        //Blink crosshair icon until we have we have horz accuracy < user defined level
        case (STATE_BASE_TEMP_SETTLE):
          blinking_icons ^= ICON_BASE_TEMPORARY | ICON_CROSS_HAIR;
          icons = paintWirelessIcon() //Top left
                | (blinking_icons & ICON_BASE_TEMPORARY)  //Top center
                | ICON_BATTERY        //Top right
                | (blinking_icons & ICON_CROSS_HAIR)  //Center left
                | ICON_HORIZONTAL_ACCURACY //Center right
                | paintSIV()          //Bottom left
                | ICON_LOGGING;       //Bottom right
          break;
        case (STATE_BASE_TEMP_SURVEY_STARTED):
          blinking_icons ^= ICON_BASE_TEMPORARY;
          icons = paintWirelessIcon() //Top left
                | (blinking_icons & ICON_BASE_TEMPORARY)  //Top center
                | ICON_BATTERY        //Top right
                | ICON_LOGGING;       //Bottom right
          paintBaseTempSurveyStarted();
          break;
        case (STATE_BASE_TEMP_TRANSMITTING):
          icons = paintWirelessIcon() //Top left
                | ICON_BASE_TEMPORARY //Top center
                | ICON_BATTERY        //Top right
                | ICON_LOGGING;       //Bottom right
          paintRTCM();
          break;
        case (STATE_BASE_FIXED_NOT_STARTED):
          icons = paintWirelessIcon() //Top left
                | ICON_BATTERY;       //Top right
          break;
        case (STATE_BASE_FIXED_TRANSMITTING):
          icons = paintWirelessIcon() //Top left
                | ICON_BASE_FIXED     //Top center
                | ICON_BATTERY        //Top right
                | ICON_LOGGING;       //Bottom right
          paintRTCM();
          break;
        case (STATE_BUBBLE_LEVEL):
          paintBubbleLevel();
          break;
        case (STATE_PROFILE):
          paintProfile(displayProfile);
          break;
        case (STATE_MARK_EVENT):
          //Do nothing. Static display shown during state change.
          break;
        case (STATE_DISPLAY_SETUP):
          paintDisplaySetup();
          break;
        case (STATE_WIFI_CONFIG_NOT_STARTED):
          displayWiFiConfigNotStarted(); //Display 'WiFi Config'
          break;
        case (STATE_WIFI_CONFIG):
          icons = displayWiFiConfig(); //Display SSID and IP
          break;
        case (STATE_TEST):
          paintSystemTest();
          break;
        case (STATE_TESTING):
          paintSystemTest();
          break;

        case (STATE_KEYS_STARTED):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_NEEDED):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_WIFI_STARTED):
          icons = paintWirelessIcon();  //Top left
          paintGettingKeys();
          break;
        case (STATE_KEYS_WIFI_CONNECTED):
          icons = paintWirelessIcon();  //Top left
          paintGettingKeys();
          break;
        case (STATE_KEYS_WIFI_TIMEOUT):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_EXPIRED):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_DAYS_REMAINING):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_LBAND_CONFIGURE):
          paintLBandConfigure();
          break;
        case (STATE_KEYS_LBAND_ENCRYPTED):
          //Do nothing. Quick, fall through state.
          break;
        case (STATE_KEYS_PROVISION_WIFI_STARTED):
          icons = paintWirelessIcon();  //Top left
          paintGettingKeys();
          break;
        case (STATE_KEYS_PROVISION_WIFI_CONNECTED):
          icons = paintWirelessIcon();  //Top left
          paintGettingKeys();
          break;
        case (STATE_KEYS_PROVISION_WIFI_TIMEOUT):
          //Do nothing. Quick, fall through state.
          break;

        case (STATE_SHUTDOWN):
          displayShutdown();
          break;
        default:
          Serial.printf("Unknown display: %d\n\r", systemState);
          displayError("Display");
          break;
      }

      //Top left corner
      if (icons & ICON_WIFI_SYMBOL_LEFT)
      {
        displayBitmap(0, 0, WiFi_Symbol_Width, WiFi_Symbol_Height, WiFi_Symbol);
        if (icons & ICON_DOWN_ARROW)
        {
          displayBitmap(16, 0, DownloadArrow_Width, DownloadArrow_Height, DownloadArrow);
          online.rxRtcmCorrectionData = false;
        }
        else if (icons & ICON_UP_ARROW)
          displayBitmap(16, 0, UploadArrow_Width, UploadArrow_Height, UploadArrow);
      }
      else if (icons & ICON_BT_SYMBOL)
      {
        displayBitmap(4, 0, BT_Symbol_Width, BT_Symbol_Height, BT_Symbol);
        if (btState == BT_CONNECTED)
        {
          if (icons & ICON_DOWN_ARROW)
          {
            displayBitmap(16, 0, DownloadArrow_Width, DownloadArrow_Height, DownloadArrow);
            online.rxRtcmCorrectionData = false;
          }
          else if (icons & ICON_UP_ARROW)
            displayBitmap(16, 0, UploadArrow_Width, UploadArrow_Height, UploadArrow);
        }
      }
      else if (icons & ICON_MAC_ADDRESS)
      {
        char macAddress[5];
#ifdef COMPILE_BT
        sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);
#else
        sprintf(macAddress, "%02X%02X", 0, 0); //If BT is not available, print zeroes
#endif
        oled.setFont(QW_FONT_5X7); //Set font to smallest
        oled.setCursor(0, 3);
        oled.print(macAddress);
      }

      //Top center
      if (icons & ICON_WIFI_SYMBOL_CENTER)
        displayBitmap((oled.getWidth() / 2) - (WiFi_Symbol_Width / 2), 0, WiFi_Symbol_Width, WiFi_Symbol_Height, WiFi_Symbol);
      else if ((icons & ICON_DYNAMIC_MODEL) && (online.gnss == true))
        paintDynamicModel();
      else if (icons & ICON_BASE_TEMPORARY)
        displayBitmap(27, 0, BaseTemporary_Width, BaseTemporary_Height, BaseTemporary);
      else if (icons & ICON_BASE_FIXED)
        displayBitmap(27, 0, BaseFixed_Width, BaseFixed_Height, BaseFixed); //true - blend with other pixels

      //Top right corner
      if (icons & ICON_BATTERY)
        paintBatteryLevel();

      //Center left
      if (icons & ICON_CROSS_HAIR)
        displayBitmap(0, 18, CrossHair_Width, CrossHair_Height, CrossHair);
      else if (icons & ICON_CROSS_HAIR_DUAL)
        displayBitmap(0, 18, CrossHairDual_Width, CrossHairDual_Height, CrossHairDual);

      //Center right
      if (icons & ICON_HORIZONTAL_ACCURACY)
        paintHorizontalAccuracy();

      //Bottom left corner
      if (icons & ICON_SIV_ANTENNA)
        displayBitmap(2, 35, SIV_Antenna_Width, SIV_Antenna_Height, SIV_Antenna);
      else if (icons & ICON_SIV_ANTENNA_LBAND)
        displayBitmap(2, 35, SIV_Antenna_LBand_Width, SIV_Antenna_LBand_Height, SIV_Antenna_LBand);

      //Bottom right corner
      if (icons & ICON_LOGGING)
        paintLogging();

      oled.display(); //Push internal buffer to display
    }
  } //End display online
}

void displaySplash()
{
  if (online.display == true)
  {
    oled.erase();

    int yPos = 0;
    int fontHeight = 8;

    printTextCenter("SparkFun", yPos, QW_FONT_5X7, 1, false); //text, y, font type, kerning, inverted

    yPos = yPos + fontHeight + 2;
    printTextCenter("RTK", yPos, QW_FONT_8X16, 1, false);

    yPos = yPos + fontHeight + 5;

    if (productVariant == RTK_SURVEYOR)
    {
      printTextCenter("Surveyor", yPos, QW_FONT_8X16, 1, false);
    }
    else if (productVariant == RTK_EXPRESS)
    {
      printTextCenter("Express", yPos, QW_FONT_8X16, 1, false);
    }
    else if (productVariant == RTK_EXPRESS_PLUS)
    {
      printTextCenter("Express+", yPos, QW_FONT_8X16, 1, false);
    }
    else if (productVariant == RTK_FACET)
    {
      printTextCenter("Facet", yPos, QW_FONT_8X16, 1, false);
    }
    else if (productVariant == RTK_FACET_LBAND)
    {
      printTextCenter("Facet LB", yPos, QW_FONT_8X16, 1, false);
    }

    yPos = yPos + fontHeight + 7;
    char unitFirmware[50];
#ifdef ENABLE_DEVELOPER
    sprintf(unitFirmware, "v%d.%d-DEV", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
#else
    sprintf(unitFirmware, "v%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR);
#endif
    printTextCenter(unitFirmware, yPos, QW_FONT_5X7, 1, false);

    oled.display();
  }
}

void displayShutdown()
{
  displayMessage("Shutting Down...", 0);
}

//Displays a small error message then hard freeze
//Text wraps and is small but legible
void displayError(const char * errorMessage)
{
  if (online.display == true)
  {
    oled.erase(); // Clear the display's internal buffer

    oled.setCursor(0, 0); //x, y
    oled.setFont(QW_FONT_5X7); //Set font to smallest
    oled.print(F("Error:"));

    oled.setCursor(2, 10);
    //oled.setFont(QW_FONT_8X16);
    oled.print(errorMessage);

    oled.display(); //Push internal buffer to display

    while (1) delay(10); //Hard freeze
  }
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
   0|                                             *****************
   1|                                             *               *
   2|                                             * ***  ***  *** *
   3|                                             * ***  ***  *** ***
   4|                                             * ***  ***  ***   *
   5|                                             * ***  ***  ***   *
   6|                                             * ***  ***  ***   *
   7|                                             * ***  ***  ***   *
   8|                                             * ***  ***  *** ***
   9|                                             * ***  ***  *** *
  10|                                             *               *
  11|                                             *****************
*/

//Print the classic battery icon with levels
void paintBatteryLevel()
{
  if (online.display == true)
  {
    //Current battery charge level
    if (battLevel < 25)
      displayBitmap(45, 0, Battery_0_Width, Battery_0_Height, Battery_0);
    else if (battLevel < 50)
      displayBitmap(45, 0, Battery_1_Width, Battery_1_Height, Battery_1);
    else if (battLevel < 75)
      displayBitmap(45, 0, Battery_2_Width, Battery_2_Height, Battery_2);
    else //batt level > 75
      displayBitmap(45, 0, Battery_3_Width, Battery_3_Height, Battery_3);
  }
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
   0|
   1|
   2|
   3| ***   ***   ***   ***
   4|*   * *   * *   * *   *
   5|*   * *   * *   * *   *
   6| ***   ***   ***   ***
   7|*   * *   * *   * *   *
   8|*   * *   * *   * *   *
   9| ***   ***   ***   ***
  10|
  11|

or

               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
   0|       *
   1|       **
   2|       ***
   3|    *  * **
   4|    ** * **
   5|     *****
   6|      ***
   7|      ***
   8|     *****
   9|    ** * **
  10|    *  * **
  11|       ***
  12|       **
  13|       *

or

               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
   0|   *******         **
   1|  *       *        **
   2| *  *****  *       **
   3|*  *     *  *      **
   4|  *  ***  *        **
   5|    *   *       ** ** **
   6|      *          ******
   7|     ***          ****
   8|      *            **
*/

//Display Bluetooth icon, Bluetooth MAC, or WiFi depending on connection state
uint32_t paintWirelessIcon()
{
  uint32_t icons;

  //TODO resolve if both BT and WiFi are on/connected
  icons = 0;
  if (online.display == true)
  {
    //Bluetooth icon if paired, or Bluetooth MAC address if not paired
    if (btState == BT_CONNECTED)
    {
      icons = ICON_BT_SYMBOL;
      if ((systemState <= STATE_BASE_NOT_STARTED) && online.rxRtcmCorrectionData)
        icons |= ICON_DOWN_ARROW;
      else if (systemState <= STATE_BUBBLE_LEVEL)
        icons |= ICON_UP_ARROW;
    }
    else if (wifiState == WIFI_NOTCONNECTED)
    {
      //Blink WiFi icon
      blinking_icons ^= ICON_WIFI_SYMBOL_LEFT;
      if (blinking_icons & ICON_WIFI_SYMBOL_LEFT)
        icons = ICON_WIFI_SYMBOL_LEFT;
    }
    else if (wifiState == WIFI_CONNECTED)
    {
      icons = ICON_WIFI_SYMBOL_LEFT;

      //If we are connected to NTRIP Client, show download arrow
      if ((online.ntripClient == true) && online.rxRtcmCorrectionData)
        icons |= ICON_DOWN_ARROW;
      else if (online.ntripServer == true)
        icons |= ICON_UP_ARROW;
    }
    else
      icons = ICON_MAC_ADDRESS;
  }
  return icons;
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
  17|
  18|
  19|
  20|
  21|                           ***               ***      ***
  22|                          *   *             *   *    *   *
  23|                          *   *             *   *    *   *
  24|                  **       * *               * *      * *
  25|                  **        *                 *        *
  26|                           * *               * *      * *
  27|                          *   *             *   *    *   *
  28|                          *   *             *   *    *   *
  29|                  **      *   *     **      *   *    *   *
  30|                  **       ***      **       ***      ***
  31|
  32|
*/

//Display horizontal accuracy
void paintHorizontalAccuracy()
{
  oled.setFont(QW_FONT_8X16); //Set font to type 1: 8x16
  oled.setCursor(16, 20); //x, y
  oled.print(":");

  if (online.gnss == false)
  {
    oled.print(F("N/A"));
  }
  else if (horizontalAccuracy > 30.0)
  {
    oled.print(F(">30m"));
  }
  else if (horizontalAccuracy > 9.9)
  {
    oled.print(horizontalAccuracy, 1); //Print down to decimeter
  }
  else if (horizontalAccuracy > 1.0)
  {
    oled.print(horizontalAccuracy, 2); //Print down to centimeter
  }
  else
  {
    oled.print("."); //Remove leading zero
    oled.printf("%03d", (int)(horizontalAccuracy * 1000)); //Print down to millimeter
  }
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
   0|                                  **
   1|                                  **
   2|                               ******
   3|                              *      *
   4|                            * * **** * *
   5|                            * * **** * *
   6|                            * *      * *
   7|                            * *      * *
   8|                            * *      * *
   9|                            * *      * *
  10|                              *      *
  11|                               ******
  12|
*/

//Draw the rover icon depending on screen
void paintDynamicModel()
{
  //Display icon associated with current Dynamic Model
  switch (settings.dynamicModel)
  {
    case (DYN_MODEL_PORTABLE):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_1_Portable);
      break;
    case (DYN_MODEL_STATIONARY):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_2_Stationary);
      break;
    case (DYN_MODEL_PEDESTRIAN):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_3_Pedestrian);
      break;
    case (DYN_MODEL_AUTOMOTIVE):
      //Normal rover for ZED-F9P, fusion rover for ZED-F9R
      if (zedModuleType == PLATFORM_F9P)
      {
        displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_4_Automotive);
      }
      else if (zedModuleType == PLATFORM_F9R)
      {
        //Blink fusion rover until we have calibration
        if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 0) //Initializing
        {
          //Blink Fusion Rover icon until sensor calibration is complete
          if (millis() - lastBaseIconUpdate > 500)
          {
            lastBaseIconUpdate = millis();
            if (baseIconDisplayed == false)
            {
              baseIconDisplayed = true;

              //Draw the icon
              displayBitmap(27, 2, Rover_Fusion_Width, Rover_Fusion_Height, Rover_Fusion);
            }
            else
              baseIconDisplayed = false;
          }
        }
        else if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 1) //Calibrated
        {
          //Solid fusion rover
          displayBitmap(27, 2, Rover_Fusion_Width, Rover_Fusion_Height, Rover_Fusion);
        }
        else if (i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 2 || i2cGNSS.packetUBXESFSTATUS->data.fusionMode == 3) //Suspended or disabled
        {
          //Empty rover
          displayBitmap(27, 2, Rover_Fusion_Empty_Width, Rover_Fusion_Empty_Height, Rover_Fusion_Empty);
        }
      }
      break;
    case (DYN_MODEL_SEA):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_5_Sea);
      break;
    case (DYN_MODEL_AIRBORNE1g):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_6_Airborne1g);
      break;
    case (DYN_MODEL_AIRBORNE2g):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_7_Airborne2g);
      break;
    case (DYN_MODEL_AIRBORNE4g):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_8_Airborne4g);
      break;
    case (DYN_MODEL_WRIST):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_9_Wrist);
      break;
    case (DYN_MODEL_BIKE):
      displayBitmap(27, 0, DynamicModel_Width, DynamicModel_Height, DynamicModel_10_Bike);
      break;
  }
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
  35|
  36|   **
  37|   * *                    ***      ***
  38|   *  *   *              *   *    *   *
  39|   *   * *               *   *    *   *
  40|    *   *        **       * *      * *
  41|    *    *       **        *        *
  42|     *    *               * *      * *
  43|     **    *             *   *    *   *
  44|     ****   *            *   *    *   *
  45|     **  ****    **      *   *    *   *
  46|     **          **       ***      ***
  47|   ******
*/

//Select satellite icon and draw sats in view
//Blink icon if no fix
uint32_t paintSIV()
{
  uint32_t blinking;
  uint32_t icons;

  oled.setFont(QW_FONT_8X16); //Set font to type 1: 8x16
  oled.setCursor(16, 36); //x, y
  oled.print(":");

  if (online.gnss)
  {
    if (fixType == 0) //0 = No Fix
      oled.print("0");
    else
      oled.print(numSV);

    paintResets();

    //Determine which icon to display
    icons = 0;
    if (lbandCorrectionsReceived)
      blinking = ICON_SIV_ANTENNA_LBAND;
    else
      blinking = ICON_SIV_ANTENNA;

    //Determine if there is a fix
    if (fixType == 3 || fixType == 4 || fixType == 5) //3D, 3D+DR, or Time
    {
      //Fix, turn on icon
      icons = blinking;
    }
    else
    {
      //Blink satellite dish icon if we don't have a fix
      blinking_icons ^= blinking;
      if (blinking_icons & blinking)
        icons = blinking;
    }
  } //End gnss online
  else
  {
    oled.print("X");

    icons = ICON_SIV_ANTENNA;
  }
  return icons;
}

/*
               111111111122222222223333333333444444444455555555556666
     0123456789012345678901234567890123456789012345678901234567890123
    .----------------------------------------------------------------
  35|
  36|                                                       *******
  37|                                                       *     **
  38|                                                       *      **
  39|                                                       *       *
  40|                                                       * ***** *
  41|                                                       *       *
  42|                                                       * ***** *
  43|                                                       *       *
  44|                                                       * ***** *
  45|                                                       *       *
  46|                                                       *       *
  47|                                                       *********
*/

//Draw log icon
//Turn off icon if log file fails to get bigger
void paintLogging()
{
  //Animate icon to show system running
  loggingIconDisplayed++; //Goto next icon
  loggingIconDisplayed %= 4; //Wrap
  if (online.logging == true && logIncreasing == true)
  {
    if (loggingIconDisplayed == 0)
      displayBitmap(64 - Logging_0_Width, 48 - Logging_0_Height, Logging_0_Width, Logging_0_Height, Logging_0);
    else if (loggingIconDisplayed == 1)
      displayBitmap(64 - Logging_1_Width, 48 - Logging_1_Height, Logging_1_Width, Logging_1_Height, Logging_1);
    else if (loggingIconDisplayed == 2)
      displayBitmap(64 - Logging_2_Width, 48 - Logging_2_Height, Logging_2_Width, Logging_2_Height, Logging_2);
    else if (loggingIconDisplayed == 3)
      displayBitmap(64 - Logging_3_Width, 48 - Logging_3_Height, Logging_3_Width, Logging_3_Height, Logging_3);
  }
  else
  {
    const int pulseX = 64 - 4;
    const int pulseY = oled.getHeight();
    int height;

    //Paint pulse to show system activity
    height = loggingIconDisplayed << 2;
    if (height)
    {
      oled.line(pulseX, pulseY, pulseX, pulseY - height);
      oled.line(pulseX - 1, pulseY, pulseX - 1, pulseY - height);
    }
  }
}

//Survey in is running. Show 3D Mean and elapsed time.
void paintBaseTempSurveyStarted()
{
  oled.setFont(QW_FONT_5X7);
  oled.setCursor(0, 23); //x, y
  oled.print("Mean:");

  oled.setCursor(29, 20); //x, y
  oled.setFont(QW_FONT_8X16);
  if (svinMeanAccuracy < 10.0) //Error check
    oled.print(svinMeanAccuracy, 2);
  else
    oled.print(">10");

  oled.setCursor(0, 39); //x, y
  oled.setFont(QW_FONT_5X7);
  oled.print("Time:");

  oled.setCursor(30, 36); //x, y
  oled.setFont(QW_FONT_8X16);
  if (svinObservationTime < 1000) //Error check
    oled.print(svinObservationTime);
  else
    oled.print("0");
}

//Given text, a position, and kerning, print text to display
//This is helpful for squishing or stretching a string to appropriately fill the display
void printTextwithKerning(const char *newText, uint8_t xPos, uint8_t yPos, uint8_t kerning)
{
  for (int x = 0 ; x < strlen(newText) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(newText[x]);
    xPos += kerning;
  }
}

//Show transmission of RTCM correction data packets to NTRIP caster
void paintRTCM()
{
  int textY = 17;
  int textKerning = 8;
  oled.setFont(QW_FONT_8X16);
  if (btState != BT_OFF)
  {
    int textX = 1;
    printTextwithKerning("Xmitting", textX, textY, textKerning);  //via Bluetooth
  }
  else
  {
    int textX = 4;
    printTextwithKerning("Casting", textX, textY, textKerning);   //via WiFi
  }

  oled.setCursor(0, 39); //x, y
  oled.setFont(QW_FONT_5X7);
  oled.print("RTCM:");

  if (rtcmPacketsSent < 100)
    oled.setCursor(30, 36); //x, y - Give space for two digits
  else
    oled.setCursor(28, 36); //x, y - Push towards colon to make room for log icon

  oled.setFont(QW_FONT_8X16); //Set font to type 1: 8x16
  oled.print(rtcmPacketsSent); //rtcmPacketsSent is controlled in processRTCM()

  paintResets();
}

//Show connecting to NTRIP caster service
void paintConnectingToNtripCaster()
{
  int textX = 11;
  int textY = 18;
  int textKerning = 8;

  printTextwithKerning("Caster", textX, textY, textKerning);

  textX = 3;
  textY = 33;
  textKerning = 6;
  oled.setFont(QW_FONT_8X16);

  printTextwithKerning("Connecting", textX, textY, textKerning);
}

void displayBaseStart(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15; //Assume fontsize 1
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Base", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    oled.display();

    oled.display();

    delay(displayTime);
  }
}

void displayBaseSuccess(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15; //Assume fontsize 1
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Base", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Started", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displayBaseFail(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15; //Assume fontsize 1
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Base", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Failed", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displayGNSSFail(uint16_t displayTime)
{
  displayMessage("GNSS Failed", displayTime);
}

void displayRoverStart(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Rover", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    //  printTextCenter("Started", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displayRoverSuccess(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Rover", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Started", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displayRoverFail(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Rover", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Failed", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displayAccelFail(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Accel", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Failed", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

//When user enters serial config menu the display will freeze so show splash while config happens
void displaySerialConfig()
{
  displayMessage("Serial Config", 0);
}

//When user enters WiFi Config mode from setup, show splash while config happens
void displayWiFiConfigNotStarted()
{
  displayMessage("WiFi Config", 0);
}

uint32_t displayWiFiConfig()
{
  uint32_t icons;

  //Draw the WiFi icon
  icons = 0;
  if (wifiState == WIFI_NOTCONNECTED)
  {
    //Blink WiFi icon
    blinking_icons ^= ICON_WIFI_SYMBOL_LEFT;
    if (blinking_icons & ICON_WIFI_SYMBOL_LEFT)
      icons = ICON_WIFI_SYMBOL_CENTER;
  }
  else if (wifiState == WIFI_CONNECTED)
    icons = ICON_WIFI_SYMBOL_CENTER;

  int yPos = WiFi_Symbol_Height + 2;
  int fontHeight = 8;

  printTextCenter("SSID:", yPos, QW_FONT_5X7, 1, false); //text, y, font type, kerning, inverted

  yPos = yPos + fontHeight + 1;
  printTextCenter("RTK Config", yPos, QW_FONT_5X7, 1, false);

  yPos = yPos + fontHeight + 3;
  printTextCenter("IP:", yPos, QW_FONT_5X7, 1, false);

  yPos = yPos + fontHeight + 1;
  printTextCenter("192.168.4.1", yPos, QW_FONT_5X7, 1, false);
  return icons;
}

//When user does a factory reset, let us know
void displaySytemReset()
{
  displayMessage("Factory Reset", 0);
}

void displaySurveyStart(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Survey", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    //printTextCenter("Started", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

void displaySurveyStarted(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Survey", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("Started", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

//If the SD card is detected but is not formatted correctly, display warning
void displaySDFail(uint16_t displayTime)
{
  if (online.display == true)
  {
    oled.erase();

    uint8_t fontHeight = 15;
    uint8_t yPos = oled.getHeight() / 2 - fontHeight;

    printTextCenter("Format", yPos, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted
    printTextCenter("SD Card", yPos + fontHeight, QW_FONT_8X16, 1, false);  //text, y, font type, kerning, inverted

    oled.display();

    delay(displayTime);
  }
}

//Draw a frame at outside edge
void drawFrame()
{
  //Init and draw box at edge to see screen alignment
  int xMax = 63;
  int yMax = 47;
  oled.line(0, 0, xMax, 0); //Top
  oled.line(0, 0, 0, yMax); //Left
  oled.line(0, yMax, xMax, yMax); //Bottom
  oled.line(xMax, 0, xMax, yMax); //Right
}

void displayForcedFirmwareUpdate()
{
  displayMessage("Forced Update", 0);
}

void displayFirmwareUpdateProgress(int percentComplete)
{
  //Update the display if connected
  if (online.display == true)
  {
    oled.erase(); // Clear the display's internal buffer

    int yPos = 3;
    int fontHeight = 8;

    printTextCenter("Firmware", yPos, QW_FONT_5X7, 1, false); //text, y, font type, kerning, inverted

    yPos = yPos + fontHeight + 1;
    printTextCenter("Update", yPos, QW_FONT_5X7, 1, false); //text, y, font type, kerning, inverted

    yPos = yPos + fontHeight + 3;
    char temp[50];
    sprintf(temp, "%d%%", percentComplete);
    printTextCenter(temp, yPos, QW_FONT_8X16, 1, false); //text, y, font type, kerning, inverted

    oled.display(); //Push internal buffer to display
  }
}

void displayEventMarked(uint16_t displayTime)
{
  displayMessage("Event Marked", displayTime);
}

void displayNoLogging(uint16_t displayTime)
{
  displayMessage("No Logging", displayTime);
}

void displayMarked(uint16_t displayTime)
{
  displayMessage("Marked", displayTime);
}

void displayMarkFailure(uint16_t displayTime)
{
  displayMessage("Mark Failure", displayTime);
}

void displayNotMarked(uint16_t displayTime)
{
  displayMessage("Not Marked", displayTime);
}

//Show 'Loading Home2' profile identified
//Profiles may not be sequential (user might have empty profile #2, but filled #3) so we load the profile unit, not the number
void paintProfile(uint8_t profileUnit)
{
  char profileMessage[20]; //'Loading HomeStar' max of ~18 chars

  char profileName[8 + 1];
  if (getProfileNameFromUnit(profileUnit, profileName, sizeof(profileName)) == true) //Load the profile name, limited to 8 chars
  {
    settings.updateZEDSettings = true; //When this profile is loaded next, force system to update ZED settings.
    recordSystemSettings(); //Before switching, we need to record the current settings to LittleFS and SD

    //Lookup profileNumber based on unit
    uint8_t profileNumber = getProfileNumberFromUnit(profileUnit);
    recordProfileNumber(profileNumber); //Update internal settings with user's choice, mark unit for config update

    log_d("Going to profile number %d from unit %d, name '%s'", profileNumber, profileUnit, profileName);

    snprintf(profileMessage, sizeof(profileMessage), "Loading %s", profileName);
    displayMessage(profileMessage, 2000);
    ESP.restart(); //Profiles require full restart to take effect
  }
}

//Display unit self-tests until user presses a button to exit
//Allows operator to check:
// Display alignment
// Internal connections to: SD, Accel, Fuel guage, GNSS
// External connections: Loop back test on DATA
void paintSystemTest()
{
  if (online.display == true)
  {
    //Toggle between two displays
    if (millis() - systemTestDisplayTime > 3000)
    {
      systemTestDisplayTime = millis();
      systemTestDisplayNumber++;
      systemTestDisplayNumber %= 2;
    }

    if (systemTestDisplayNumber == 1 || productVariant != RTK_FACET_LBAND)
    {
      int xOffset = 2;
      int yOffset = 2;

      int charHeight = 7;

      char macAddress[5];
      sprintf(macAddress, "%02X%02X", unitMACAddress[4], unitMACAddress[5]);

      drawFrame(); //Outside edge

      //Test SD, accel, batt, GNSS, mux
      oled.setFont(QW_FONT_5X7); //Set font to smallest
      oled.setCursor(xOffset, yOffset); //x, y
      oled.print(F("SD:"));

      if (online.microSD == false)
        beginSD(); //Test if SD is present
      if (online.microSD == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
      {
        oled.setCursor(xOffset, yOffset + (1 * charHeight) ); //x, y
        oled.print(F("Accel:"));
        if (online.accelerometer == true)
          oled.print(F("OK"));
        else
          oled.print(F("FAIL"));
      }

      oled.setCursor(xOffset, yOffset + (2 * charHeight) ); //x, y
      oled.print(F("Batt:"));
      if (online.battery == true)
        oled.print(F("OK"));
      else
        oled.print(F("FAIL"));

      oled.setCursor(xOffset, yOffset + (3 * charHeight) ); //x, y
      oled.print(F("GNSS:"));
      if (online.gnss == true)
      {
        i2cGNSS.checkUblox(); //Regularly poll to get latest data and any RTCM
        i2cGNSS.checkCallbacks(); //Process any callbacks: ie, eventTriggerReceived

        int satsInView = numSV;
        if (satsInView > 5)
        {
          oled.print(F("OK"));
          oled.print(F("/"));
          oled.print(satsInView);
        }
        else
          oled.print(F("FAIL"));
      }
      else
        oled.print(F("FAIL"));

      if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS || productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
      {
        oled.setCursor(xOffset, yOffset + (4 * charHeight) ); //x, y
        oled.print(F("Mux:"));

        //Set mux to channel 3 and toggle pin and verify with loop back jumper wire inserted by test technician

        setMuxport(MUX_ADC_DAC); //Set mux to DAC so we can toggle back/forth
        pinMode(pin_dac26, OUTPUT);
        pinMode(pin_adc39, INPUT_PULLUP);

        digitalWrite(pin_dac26, HIGH);
        if (digitalRead(pin_adc39) == HIGH)
        {
          digitalWrite(pin_dac26, LOW);
          if (digitalRead(pin_adc39) == LOW)
            oled.print(F("OK"));
          else
            oled.print(F("FAIL"));
        }
        else
          oled.print(F("FAIL"));
      }

      //Display MAC address
      oled.setCursor(xOffset, yOffset + (5 * charHeight) ); //x, y
      oled.print(macAddress);
      oled.print(":");

      //Verify the ESP UART2 can communicate TX/RX to ZED UART1
      if (zedUartPassed == false)
      {
        Serial.println("GNSS test");

        setMuxport(MUX_UBLOX_NMEA); //Set mux to UART so we can debug over data port
        delay(20);

        //Clear out buffer before starting
        while (serialGNSS.available()) serialGNSS.read();
        serialGNSS.flush();

        SFE_UBLOX_GNSS myGNSS;

        //begin() attempts 3 connections
        if (myGNSS.begin(serialGNSS) == true)
        {

          zedUartPassed = true;
          oled.print(F("OK"));
        }
        else
          oled.print(F("FAIL"));
      }
      else
        oled.print(F("OK"));
    } //End display 1

    if (productVariant == RTK_FACET_LBAND)
    {
      if (systemTestDisplayNumber == 0)
      {
        int xOffset = 2;
        int yOffset = 2;

        int charHeight = 7;

        drawFrame(); //Outside edge

        //Test ZED Firmware, L-Band

        oled.setFont(QW_FONT_5X7); //Set font to smallest

        oled.setCursor(xOffset, yOffset); //x, y
        oled.print(F("ZED Firm:"));
        oled.setCursor(xOffset, yOffset + (1 * charHeight) ); //x, y
        oled.print("  ");
        oled.print(zedFirmwareVersionInt);
        oled.print(F("-"));
        if (zedFirmwareVersionInt < 130)
          oled.print(F("FAIL"));
        else
          oled.print(F("OK"));

        oled.setCursor(xOffset, yOffset + (2 * charHeight) ); //x, y
        oled.print(F("LBand:"));
        if (online.lband == true)
          oled.print(F("OK"));
        else
          oled.print(F("FAIL"));
      } //End display 0
    } //End Facet L-Band testing
  }
}

//Globals but only used for Bubble Level
double averagedRoll = 0.0;
double averagedPitch = 0.0;

//A bubble level
void paintBubbleLevel()
{
  if (online.accelerometer == true)
  {
    forceDisplayUpdate = true; //Update the display as quickly as possible

    getAngles();

    //Draw dot in middle
    oled.pixel(oled.getWidth() / 2, oled.getHeight() / 2);
    oled.pixel(oled.getWidth() / 2 + 1, oled.getHeight() / 2);
    oled.pixel(oled.getWidth() / 2, oled.getHeight() / 2 + 1);
    oled.pixel(oled.getWidth() / 2 + 1, oled.getHeight() / 2 + 1);

    //Draw circle relative to dot
    const int radiusLarge = 10;
    const int radiusSmall = 4;

    oled.circle(oled.getWidth() / 2 - averagedPitch, oled.getHeight() / 2 + averagedRoll, radiusLarge);
    oled.circle(oled.getWidth() / 2 - averagedPitch, oled.getHeight() / 2 + averagedRoll, radiusSmall);
  }
  else
  {
    displayAccelFail(0);
  }
}

void getAngles()
{
  if (online.accelerometer == true)
  {
    averagedRoll = 0.0;
    averagedPitch = 0.0;
    const int avgAmount = 16;

    //Take an average readings
    for (int reading = 0 ; reading < avgAmount ; reading++)
    {
      while (accel.available() == false) delay(1);

      float accelX = 0;
      float accelY = 0;
      float accelZ = 0;

      //Express Accel orientation is different from Facet
      if (productVariant == RTK_EXPRESS || productVariant == RTK_EXPRESS_PLUS)
      {
        accelX = accel.getX();
        accelZ = accel.getY();
        accelY = accel.getZ();
        accelZ *= -1.0;
        accelX *= -1.0;
      }
      else if (productVariant == RTK_FACET || productVariant == RTK_FACET_LBAND)
      {
        accelZ = accel.getX();
        accelX = accel.getY();
        accelY = accel.getZ();
        accelZ *= -1.0;
        accelY *= -1.0;
        accelX *= -1.0;
      }

      double roll = atan2(accelY , accelZ) * 57.3;
      double pitch = atan2((-accelX) , sqrt(accelY * accelY + accelZ * accelZ)) * 57.3;

      averagedRoll += roll;
      averagedPitch += pitch;
    }

    averagedRoll /= (float)avgAmount;
    averagedPitch /= (float)avgAmount;

    //Avoid -0 since we're not printing the decimal portion
    if (averagedRoll < 0.5 && averagedRoll > -0.5) averagedRoll = 0;
    if (averagedPitch < 0.5 && averagedPitch > -0.5) averagedPitch = 0;
  }
}

//Display the setup profiles
void paintDisplaySetupProfile(const char * firstState)
{
  int index;
  int itemsDisplayed;
  char profileName[8 + 1];

  //Display the first state if this is the first profile
  itemsDisplayed = 0;
  if (displayProfile == 0)
  {
    printTextCenter(firstState, 12 * itemsDisplayed, QW_FONT_8X16, 1, false);
    itemsDisplayed++;
  }

  //Display Bubble if this is the second profile
  if (displayProfile <= 1)
  {
    printTextCenter("Bubble", 12 * itemsDisplayed, QW_FONT_8X16, 1, false);
    itemsDisplayed++;
  }

  //Display Config if this is the third profile
  if (displayProfile <= 2)
  {
    printTextCenter("Config", 12 * itemsDisplayed, QW_FONT_8X16, 1, false);
    itemsDisplayed++;
  }

  //  displayProfile  itemsDisplayed  index
  //        0               3           0
  //        1               2           0
  //        2               1           0
  //        3               0           0
  //        4               0           1
  //        5               0           2
  //        n >= 3          0           n - 3

  //Display the profile names
  for (index = (displayProfile >= 3) ? displayProfile - 3 : 0; itemsDisplayed < 4; itemsDisplayed++)
  {
    //Lookup next available profile, limit to 8 characters
    getProfileNameFromUnit(index, profileName, sizeof(profileName));
    printTextCenter(profileName, 12 * itemsDisplayed, QW_FONT_8X16, 1, itemsDisplayed == 3);
    index++;
  }
}

//Show different menu 'buttons' to allow user to pause on one to select it
void paintDisplaySetup()
{
  if (zedModuleType == PLATFORM_F9P)
  {
    if (setupState == STATE_MARK_EVENT)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, true); //string, y, font type, kerning, inverted
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Base", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_ROVER_NOT_STARTED)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false);
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, true);
      printTextCenter("Base", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_BASE_NOT_STARTED)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false); //string, y, font type, kerning, inverted
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Base", 12 * 2, QW_FONT_8X16, 1, true);
      printTextCenter("Bubble", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_BUBBLE_LEVEL)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false); //string, y, font type, kerning, inverted
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Base", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 3, QW_FONT_8X16, 1, true);
    }
    else if (setupState == STATE_WIFI_CONFIG_NOT_STARTED)
    {
      printTextCenter("Rover", 12 * 0, QW_FONT_8X16, 1, false);
      printTextCenter("Base", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Config", 12 * 3, QW_FONT_8X16, 1, true);
    }
    else if (setupState == STATE_PROFILE)
      paintDisplaySetupProfile("Base");
  } //end type F9P
  else if (zedModuleType == PLATFORM_F9R)
  {
    if (setupState == STATE_MARK_EVENT)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, true); //string, y, font type, kerning, inverted
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Config", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_ROVER_NOT_STARTED)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false);
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, true);
      printTextCenter("Bubble", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Config", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_BUBBLE_LEVEL)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false);
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 2, QW_FONT_8X16, 1, true);
      printTextCenter("Config", 12 * 3, QW_FONT_8X16, 1, false);
    }
    else if (setupState == STATE_WIFI_CONFIG_NOT_STARTED)
    {
      printTextCenter("Mark", 12 * 0, QW_FONT_8X16, 1, false);
      printTextCenter("Rover", 12 * 1, QW_FONT_8X16, 1, false);
      printTextCenter("Bubble", 12 * 2, QW_FONT_8X16, 1, false);
      printTextCenter("Config", 12 * 3, QW_FONT_8X16, 1, true);
    }
    else if (setupState == STATE_PROFILE)
      paintDisplaySetupProfile("Rover");
  } //end type F9R
}

//Given text, and location, print text center of the screen
void printTextCenter(const char *text, uint8_t yPos, QwiicFont & fontType, uint8_t kerning, bool highlight) //text, y, font type, kearning, inverted
{
  oled.setFont(fontType);
  oled.setDrawMode(grROPXOR);

  uint8_t fontWidth = fontType.width;
  if (fontWidth == 8) fontWidth = 7; //8x16, but widest character is only 7 pixels.

  uint8_t xStart = (oled.getWidth() / 2) - ((strlen(text) * (fontWidth + kerning)) / 2) + 1;

  uint8_t xPos = xStart;
  for (int x = 0 ; x < strlen(text) ; x++)
  {
    oled.setCursor(xPos, yPos);
    oled.print(text[x]);
    xPos += fontWidth + kerning;
  }

  if (highlight) //Draw a box, inverted over text
  {
    uint8_t textPixelWidth = strlen(text) * (fontWidth + kerning);

    //Error check
    int xBoxStart = xStart - 5;
    if (xBoxStart < 0) xBoxStart = 0;
    int xBoxEnd = textPixelWidth + 9;
    if (xBoxEnd > oled.getWidth() - 1) xBoxEnd = oled.getWidth() - 1;

    oled.rectangleFill(xBoxStart, yPos, xBoxEnd, 12, 1); //x, y, width, height, color
  }
}

//Given a message (one or two words) display centered
void displayMessage(const char* message, uint16_t displayTime)
{
  if (online.display == true)
  {
    char temp[20];
    uint8_t fontHeight = 15; //Assume fontsize 1

    //Count words based on spaces
    uint8_t wordCount = 0;
    strcpy(temp, message); //strtok modifies the message so make copy
    char * token = strtok(temp, " ");
    while (token != NULL)
    {
      wordCount++;
      token = strtok(NULL, " ");
    }

    uint8_t yPos = (oled.getHeight() / 2) - (fontHeight / 2);
    if (wordCount == 2) yPos -= (fontHeight / 2);

    oled.erase();

    //drawFrame();

    strcpy(temp, message);
    token = strtok(temp, " ");
    while (token != NULL)
    {
      printTextCenter(token, yPos, QW_FONT_8X16, 1, false); //text, y, font type, kerning, inverted
      token = strtok(NULL, " ");
      yPos += fontHeight;
    }

    oled.display();

    delay(displayTime);
  }
}

void paintResets()
{
  if (settings.enableResetDisplay == true)
  {
    oled.setFont(QW_FONT_5X7); //Small font
    oled.setCursor(16 + (8 * 3) + 6, 38); //x, y
    oled.print(settings.resetCount);
  }
}

//Wrapper to avoid needing to pass width/height data twice
void displayBitmap(uint8_t x, uint8_t y, uint8_t imageWidth, uint8_t imageHeight, uint8_t *imageData)
{
  oled.bitmap(x, y, x + imageWidth, y + imageHeight, imageData, imageWidth, imageHeight);
}

void displayKeysUpdated()
{
  displayMessage("Keys Updated", 2000);
}

void paintKeyDaysRemaining(int daysRemaining, uint16_t displayTime)
{
  //28 days
  //until PP
  //keys expire

  if (online.display == true)
  {
    oled.erase();

    if (daysRemaining < 0) daysRemaining = 0;

    int rightSideStart = 24; //Force the small text to rightside of screen

    oled.setFont(QW_FONT_LARGENUM);

    String days = String(daysRemaining);
    int dayTextWidth = oled.getStringWidth(days);

    int largeTextX = (rightSideStart / 2) - (dayTextWidth / 2); //Center point for x coord

    oled.setCursor(largeTextX, 0);
    oled.print(daysRemaining);

    oled.setFont(QW_FONT_5X7);

    int x = ((oled.getWidth() - rightSideStart) / 2) + rightSideStart; //Center point for x coord
    int y = 0;
    int fontHeight = 10;
    int textX;

    textX = x - (oled.getStringWidth("days") / 2); //Starting point of text
    oled.setCursor(textX, y);
    oled.print("Days");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Until") / 2);
    oled.setCursor(textX, y);
    oled.print("Until");

    y += fontHeight;
    textX = x - (oled.getStringWidth("PP") / 2);
    oled.setCursor(textX, y);
    oled.print("PP");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Keys") / 2);
    oled.setCursor(textX, y);
    oled.print("Keys");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Expire") / 2);
    oled.setCursor(textX, y);
    oled.print("Expire");

    oled.display();

    delay(displayTime);
  }
}

void paintKeyWiFiFail(uint16_t displayTime)
{
  //PP
  //Update
  //Failed
  //No WiFi

  if (online.display == true)
  {
    oled.erase();

    oled.setFont(QW_FONT_8X16);

    int x = (oled.getWidth() / 2); //Center point for x coord
    int y = 0;
    int fontHeight = 13;
    int textX;

    textX = x - (oled.getStringWidth("PP") / 2); //Starting point of text
    oled.setCursor(textX, y);
    oled.print("PP");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Update") / 2);
    oled.setCursor(textX, y);
    oled.print("Update");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Failed") / 2);
    oled.setCursor(textX, y);
    oled.print("Failed");

    oled.setFont(QW_FONT_5X7);
    y += fontHeight + 1;
    textX = x - (oled.getStringWidth("No WiFi") / 2);
    oled.setCursor(textX, y);
    oled.print("No WiFi");

    oled.display();

    delay(displayTime);
  }
}

void paintNtripWiFiFail(uint16_t displayTime, bool Client)
{
  //NTRIP
  //Client or Server
  //Failed
  //No WiFi

  if (online.display == true)
  {
    oled.erase();

    oled.setFont(QW_FONT_8X16);

    int x = (oled.getWidth() / 2); //Center point for x coord
    int y = 0;
    int fontHeight = 13;
    int textX;
    const char * string = Client ? "Client" : "Server";

    textX = x - (oled.getStringWidth("NTRIP") / 2); //Starting point of text
    oled.setCursor(textX, y);
    oled.print("NTRIP");

    y += fontHeight;
    textX = x - (oled.getStringWidth(string) / 2);
    oled.setCursor(textX, y);
    oled.print(string);

    y += fontHeight;
    textX = x - (oled.getStringWidth("Failed") / 2);
    oled.setCursor(textX, y);
    oled.print("Failed");

    oled.setFont(QW_FONT_5X7);
    y += fontHeight + 1;
    textX = x - (oled.getStringWidth("No WiFi") / 2);
    oled.setCursor(textX, y);
    oled.print("No WiFi");

    oled.display();

    delay(displayTime);
  }
}

void paintKeysExpired()
{
  displayMessage("Keys Expired", 4000);
}

void paintLBandConfigure()
{
  displayMessage("L-Band Config", 0);
}

void paintGettingKeys()
{
  displayMessage("Getting Keys", 0);
}

void paintKeyProvisionFail(uint16_t displayTime)
{
  //Whitelist Error

  //ZTP
  //Failed
  //ID:
  //10chars

  if (online.display == true)
  {
    oled.erase();

    oled.setFont(QW_FONT_5X7);

    int x = (oled.getWidth() / 2); //Center point for x coord
    int y = 0;
    int fontHeight = 8;
    int textX;

    textX = x - (oled.getStringWidth("ZTP") / 2); //Starting point of text
    oled.setCursor(textX, y);
    oled.print("ZTP");

    y += fontHeight;
    textX = x - (oled.getStringWidth("Failed") / 2);
    oled.setCursor(textX, y);
    oled.print("Failed");

    y += fontHeight;
    textX = x - (oled.getStringWidth("ID:") / 2);
    oled.setCursor(textX, y);
    oled.print("ID:");

    //The MAC address is characters long so we have to split it onto two lines
    char hardwareID[13];
    sprintf(hardwareID, "%02X%02X%02X", unitMACAddress[0], unitMACAddress[1], unitMACAddress[2]);
    String macAddress = String(hardwareID);

    y += fontHeight;
    textX = x - (oled.getStringWidth(macAddress) / 2);
    oled.setCursor(textX, y);
    oled.print(hardwareID);

    sprintf(hardwareID, "%02X%02X%02X", unitMACAddress[3], unitMACAddress[4], unitMACAddress[5]);
    macAddress = String(hardwareID);

    y += fontHeight;
    textX = x - (oled.getStringWidth(macAddress) / 2);
    oled.setCursor(textX, y);
    oled.print(hardwareID);

    oled.display();

    delay(displayTime);
  }
}
