/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
NTRIP Client States:
    NTRIP_CLIENT_OFF: Using Bluetooth or NTRIP server
    NTRIP_CLIENT_ON: WIFI_ON state
    NTRIP_CLIENT_WIFI_CONNECTING: Connecting to WiFi access point
    NTRIP_CLIENT_WIFI_CONNECTED: WiFi connected to an access point
    NTRIP_CLIENT_CONNECTING: Attempting a connection to the NTRIP caster
    NTRIP_CLIENT_CONNECTED: Connected to the NTRIP caster

                               NTRIP_CLIENT_OFF
                                    |   ^
                   ntripClientStart |   | ntripClientStop
                                    v   |
                               NTRIP_CLIENT_ON
                                    |   ^
                                    |   |
                                    v   |
                         NTRIP_CLIENT_WIFI_CONNECTING
                                    |   ^
                                    |   |
                                    v   |
                         NTRIP_CLIENT_WIFI_CONNECTED
                                    |   ^
                                    |   |
                                    v   |
                           NTRIP_CLIENT_CONNECTING
                                    |   ^
                                    |   |
                                    v   |
                            NTRIP_CLIENT_CONNECTED

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Global NTRIP Client Routines
//----------------------------------------

void ntripClientStart()
{
#ifdef  COMPILE_WIFI
#endif  //COMPILE_WIFI
}

//Check for the arrival of any correction data. Push it to the GNSS.
//Stop task if the connection has dropped or if we receive no data for maxTimeBeforeHangup_ms
void ntripClientUpdate()
{
#ifdef  COMPILE_WIFI
#endif  //COMPILE_WIFI
}
