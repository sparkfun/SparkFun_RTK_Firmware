#ifndef __NETWORK_CLIENT_H__
#define __NETWORK_CLIENT_H__

class NetworkClient : public Client
{
  public:

    //------------------------------
    // Create the network client
    //------------------------------
    NetworkClient(bool useWiFiNotEthernet)
    {
#if defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET && (!useWiFiNotEthernet))
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

    bool _useWiFiNotEthernet;
    Client * _client; // Ethernet or WiFi client

    //------------------------------
    // Return the IP address
    //------------------------------

    uint8_t* rawIPAddress(IPAddress& addr)
    {
        return Client::rawIPAddress(addr);
    }
};

#endif  // __NETWORK_CLIENT_H__
