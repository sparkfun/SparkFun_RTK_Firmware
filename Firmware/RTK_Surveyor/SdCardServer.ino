#ifdef COMPILE_WIFI

#include "SdCardServer.h"      //Get from: https://github.com/LeeLeahy2/SdCardServer

//----------------------------------------
// Locals
//----------------------------------------

static SdCardServer * sdCardServer;
static int sdCardNotFoundEventStarted = 0;
static int sdCardWebSiteStarted = 0;

//----------------------------------------
// Local Routines
//----------------------------------------

int sdCardServerIsSdCardPresent()
{
  return (settings.enableSD && online.microSD);
}

//----------------------------------------
// Global Routines
//----------------------------------------

//Start the SD card server
void
sdCardServerBegin(AsyncWebServer * server, bool createWebSite, bool redirect)
{
  //Enable the SD card server
  if (settings.enableSdCardServer && (!sdCardServer))
  {
    static const char * sdCardPages = "/microSD/";
    sdCardServer = new SdCardServer(sd, sdCardServerIsSdCardPresent, sdCardPages, NULL);
  }

  //Initialize the SD card server
  if (sdCardServer)
  {

    //Enable the pages that display the SD card contents and download SD card files
    if (!sdCardNotFoundEventStarted)
    {
      sdCardNotFoundEventStarted = 1;
      sdCardServer->onNotFound(server);
    }

    //Enable the main page if requested
    if (createWebSite && (!sdCardWebSiteStarted))
    {
      sdCardWebSiteStarted = 1;
      sdCardServer->sdCardWebSite(server, redirect);
    }

    //Display the IP address
    wifiDisplayIpAddress();
  }

  //Start the web server
  server->begin();
}

//Stop the SD card server
void
sdCardServerEnd()
{
  if (sdCardServer)
    delete sdCardServer;
  sdCardServer = NULL;
  sdCardWebSiteStarted = 0;
  sdCardNotFoundEventStarted = 0;
}

#endif //COMPILE_WIFI
