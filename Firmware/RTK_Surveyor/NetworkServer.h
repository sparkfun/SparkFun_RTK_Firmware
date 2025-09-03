#ifndef __NETWORK_SERVER_H__
#define __NETWORK_SERVER_H__

extern uint8_t networkGetType(uint8_t user);

class NetworkServer : public Server
{
  protected:

    bool _friendClass;
    Server * _server; // Ethernet or WiFi server
    uint8_t _networkType;
    uint16_t _port;
    Client * _client;

  public:

    //------------------------------
    // Create the network server
    //------------------------------
    NetworkServer(Server * server, uint8_t networkType, uint16_t port) :
        _friendClass(true),
        _server{server},
        _networkType{networkType},
        _port{port}
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
        {
            _client = new EthernetClient;
        }
        else
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
        {
            _client = new WiFiClient;
        }
#else   // COMPILE_WIFI
        {
            _client = nullptr;
        }
#endif  // COMPILE_WIFI
    }

    //------------------------------
    // Create the network server - implemented in Network.ino
    //------------------------------
    NetworkServer(uint8_t user, uint16_t port);

    //------------------------------
    // Delete the network server
    //------------------------------
    ~NetworkServer()
    {
        if (_server)
        {
#if defined(COMPILE_WIFI)
            if (_networkType == NETWORK_TYPE_WIFI)
            {
                ((WiFiServer *)_server)->stop();
            }
#endif  // COMPILE_WIFI
            if (!_friendClass)
                delete _server;
            _server = nullptr;
        }
        if (_client)
            delete _client;
        _client = nullptr;
    }

    //------------------------------
    // Begin the network server
    //------------------------------

    void begin()
    {
        if (_server)
            _server->begin();
    }

    //------------------------------
    // Determine if new client is available
    //------------------------------

    Client *available()
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            if (_server)
            {
                *_client = ((EthernetServer *)_server)->available();
                return _client;
            }
#endif  // COMPILE_WIFI
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            if (_server)
            {
                *_client = ((WiFiServer *)_server)->available();
                return _client;
            }
#endif  // COMPILE_WIFI
        return nullptr;
    }

    //------------------------------
    // Accept new client
    //------------------------------

    Client *accept()
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            if (_server)
            {
                *_client = ((EthernetServer *)_server)->accept();
                return _client;
            }
#endif  // COMPILE_WIFI
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            if (_server)
            {
                *_client = ((WiFiServer *)_server)->accept();
                return _client;
            }
#endif  // COMPILE_WIFI
        return nullptr;
    }

    //------------------------------
    // Determine if the network server was allocated
    //------------------------------

    operator bool()
    {
        return _server;
    }

    //------------------------------
    // Stop the network server
    //------------------------------

    void stop() // WiFiServer only - EthernetServer has no stop method
    {
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            if (_server)
                ((WiFiServer *)_server)->stop();
#endif  // COMPILE_WIFI
    }

    //------------------------------
    // Send a data byte to the server
    //------------------------------

    size_t write(uint8_t b)
    {
        if (_server)
            return _server->write(b);
        return 0;
    }

    //------------------------------
    // Send a buffer of data to the server
    //------------------------------

    size_t write(const uint8_t *buf, size_t size)
    {
        if (_server)
            return _server->write(buf, size);
        return 0;
    }

  protected:

    //------------------------------
    // Declare the friend classes
    //------------------------------

    friend class NetworkEthernetServer;
    friend class NetworkWiFiServer;
};

#ifdef  COMPILE_ETHERNET
class NetworkEthernetServer : public NetworkServer
{
  protected:

    EthernetServer _ethernetServer;

  public:

    NetworkEthernetServer(uint16_t port) : 
        _ethernetServer{EthernetServer(port)},
        NetworkServer(&_ethernetServer, NETWORK_TYPE_ETHERNET, port)
    {
    }

    NetworkEthernetServer(EthernetServer& server, uint16_t port) : 
        _ethernetServer{server},
        NetworkServer(&_ethernetServer, NETWORK_TYPE_ETHERNET, port)
    {
    }

    ~NetworkEthernetServer()
    {
        this->~NetworkServer();
    }
};
#endif  // COMPILE_ETHERNET

#ifdef  COMPILE_WIFI
class NetworkWiFiServer : public NetworkServer
{
  protected:

    WiFiServer _wifiServer;

  public:

    NetworkWiFiServer(uint16_t port) :
        NetworkServer(&_wifiServer, NETWORK_TYPE_WIFI, port)
    {
    }

    NetworkWiFiServer(WiFiServer& server, uint16_t port) :
        _wifiServer{server},
        NetworkServer(&_wifiServer, NETWORK_TYPE_WIFI, port)
    {
    }

    ~NetworkWiFiServer()
    {
        this->~NetworkServer();
    }
};
#endif  // COMPILE_WIFI

#endif  // __NETWORK_SERVER_H__
