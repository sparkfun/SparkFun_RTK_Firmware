#ifndef __NETWORK_CLIENT_H__
#define __NETWORK_CLIENT_H__

extern uint8_t networkGetType(uint8_t user);

class NetworkClient : public Client
{
  protected:

    Client * _client; // Ethernet or WiFi client
    bool _friendClass;
    uint8_t _networkType;

  public:

    //------------------------------
    // Create the network client
    //------------------------------
    NetworkClient(Client * client, uint8_t networkType)
    {
        _friendClass = true;
        _networkType = networkType;
        _client = client;
    }

    NetworkClient(uint8_t user)
    {
        _friendClass = false;
        _networkType = networkGetType(user);
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            _client = new EthernetClient;
        else
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
            _client = new WiFiClient;
#else   // COMPILE_WIFI
            _client = nullptr;
#endif  // COMPILE_WIFI
    };

    //------------------------------
    // Delete the network client
    //------------------------------
    ~NetworkClient()
    {
        if (_client)
        {
            _client->stop();
            if (!_friendClass)
                delete _client;
            _client = nullptr;
        }
    };

    //------------------------------
    // Determine if receive data is available
    //------------------------------

    int available()
    {
        if (_client)
            return _client->available();
        return 0;
    }

    //------------------------------
    // Determine if the network client was allocated
    //------------------------------

    operator bool()
    {
        return _client;
    }

    //------------------------------
    // Connect to the server
    //------------------------------

    int connect(IPAddress ip, uint16_t port)
    {
        if (_client)
            return _client->connect(ip, port);
        return 0;
    }

    int connect(const char *host, uint16_t port)
    {
        if (_client)
            return _client->connect(host, port);
        return 0;
    }

    //------------------------------
    // Determine if the client is connected to the server
    //------------------------------

    uint8_t connected()
    {
        if (_client)
            return _client->connected();
        return 0;
    }

    //------------------------------
    // Finish transmitting all the data to the server
    //------------------------------

    void flush()
    {
        if (_client)
            _client->flush();
    }

    //------------------------------
    // Look at the next received byte in the data stream
    //------------------------------

    int peek()
    {
        if (_client)
            return _client->peek();
        return -1;
    }

    //------------------------------
    // Display the network client status
    //------------------------------
    size_t print(const char *printMe)
    {
        if (_client)
            return _client->print(printMe);
        return 0;
    };

    //------------------------------
    // Receive a data byte from the server
    //------------------------------

    int read()
    {
        if (_client)
            return _client->read();
        return 0;
    }

    //------------------------------
    // Receive a buffer of data from the server
    //------------------------------

    int read(uint8_t *buf, size_t size)
    {
        if (_client)
            return _client->read(buf, size);
        return 0;
    }

    //------------------------------
    // Get the remote IP address
    //------------------------------

    IPAddress remoteIP()
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            return ((EthernetClient *)_client)->remoteIP();
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            return ((WiFiClient *)_client)->remoteIP();
#endif  // COMPILE_WIFI
        return IPAddress((uint32_t)0);
    }

    //------------------------------
    // Get the remote port number
    //------------------------------

    uint16_t remotePort()
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            return ((EthernetClient *)_client)->remotePort();
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            return ((WiFiClient *)_client)->remotePort();
#endif  // COMPILE_WIFI
        return 0;
    }

    //------------------------------
    // Stop the network client
    //------------------------------

    void stop()
    {
        if (_client)
            _client->stop();
    }

    //------------------------------
    // Send a data byte to the server
    //------------------------------

    size_t write(uint8_t b)
    {
        if (_client)
            return _client->write(b);
        return 0;
    }

    //------------------------------
    // Send a buffer of data to the server
    //------------------------------

    size_t write(const uint8_t *buf, size_t size)
    {
        if (_client)
            return _client->write(buf, size);
        return 0;
    }

  protected:

    //------------------------------
    // Return the IP address
    //------------------------------

    uint8_t* rawIPAddress(IPAddress& addr)
    {
        return Client::rawIPAddress(addr);
    }

    //------------------------------
    // Declare the friend classes
    //------------------------------

    friend class NetworkEthernetClient;
    friend class NetworkWiFiClient;
};

#ifdef  COMPILE_ETHERNET
class NetworkEthernetClient : public NetworkClient
{
  private:

    EthernetClient _client;

  public:

    NetworkEthernetClient(EthernetClient& client) :
        _client{client},
        NetworkClient(&_client, NETWORK_TYPE_ETHERNET)
    {
    }

    ~NetworkEthernetClient()
    {
        this->~NetworkClient();
    }
};
#endif  // COMPILE_ETHERNET

#ifdef  COMPILE_WIFI
class NetworkWiFiClient : public NetworkClient
{
  private:

    WiFiClient _client;

  public:

    NetworkWiFiClient(WiFiClient& client) :
        _client{client},
        NetworkClient(&_client, NETWORK_TYPE_WIFI)
    {
    }

    ~NetworkWiFiClient()
    {
        this->~NetworkClient();
    }
};
#endif  // COMPILE_WIFI

#endif  // __NETWORK_CLIENT_H__
