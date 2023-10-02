#ifndef __NETWORK_UDP_H__
#define __NETWORK_UDP_H__

extern uint8_t networkGetType(uint8_t user);

class NetworkUDP : public UDP
{
  protected:

    UDP * _udp; // Ethernet or WiFi udp
    bool _friendClass;
    uint8_t _networkType;

  public:

    //------------------------------
    // Create the network client
    //------------------------------
    NetworkUDP(UDP * udp, uint8_t networkType)
    {
        _friendClass = true;
        _networkType = networkType;
        _udp = udp;
    }

    NetworkUDP(uint8_t user)
    {
        _friendClass = false;
        _networkType = networkGetType(user);
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            _udp = new EthernetUDP;
        else
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
            _udp = new WiFiUDP;
#else   // COMPILE_WIFI
            _udp = nullptr;
#endif  // COMPILE_WIFI
    };

    //------------------------------
    // Delete the network client
    //------------------------------
    ~NetworkUDP()
    {
        if (_udp)
        {
            _udp->stop();
            if (!_friendClass)
                delete _udp;
            _udp = nullptr;
        }
    };

    
    //------------------------------
    // Determine if the network client was allocated
    //------------------------------

    operator bool()
    {
        return _udp;
    }

    //------------------------------
    // Start to the server
    //------------------------------

    uint8_t begin(uint16_t port)
    {
        if (_udp)
            return _udp->begin(port);
        return 0;
    }

    //------------------------------
    // Stop the network client
    //------------------------------

    void stop()
    {
        if (_udp)
            _udp->stop();
    }

    //------------------------------
    // Determine if receive data is available
    //------------------------------

    int available()
    {
        if (_udp)
            return _udp->available();
        return 0;
    }

    //------------------------------
    // Read available data
    //------------------------------

    int read()
    {
        if (_udp)
            return _udp->read();
        return 0;
    }

    //------------------------------
    // Read available data
    //------------------------------

    int read(unsigned char* buf, size_t length)
    {
        if (_udp)
            return _udp->read(buf, length);
        return 0;
    }

    //------------------------------
    // Read available data
    //------------------------------

    int read(char* buf, size_t length)
    {
        if (_udp)
            return _udp->read(buf, length);
        return 0;
    }

    //------------------------------
    // Look at the next received byte in the data stream
    //------------------------------

    int peek()
    {
        if (_udp)
            return _udp->peek();
        return 0;
    }

    //------------------------------
    // Finish transmitting all the data
    //------------------------------

    void flush()
    {
        if (_udp)
            _udp->flush();
    }

    //------------------------------
    // Send a data byte to the server
    //------------------------------

    size_t write(uint8_t b)
    {
        if (_udp)
            return _udp->write(b);
        return 0;
    }

    //------------------------------
    // Send a buffer of data to the server
    //------------------------------

    size_t write(const uint8_t *buf, size_t size)
    {
        if (_udp)
            return _udp->write(buf, size);
        return 0;
    }

    //------------------------------
    // Begin a UDP packet
    //------------------------------

    int beginPacket(IPAddress ip, uint16_t port)
    {
        if (_udp)
            return _udp->beginPacket(ip, port);
        return 0;
    }

    //------------------------------
    // Begin a UDP packet
    //------------------------------

    int beginPacket(const char* host, uint16_t port)
    {
        if (_udp)
            return _udp->beginPacket(host, port);
        return 0;
    }

    //------------------------------
    // Parse UDP packet
    //------------------------------

    int parsePacket()
    {
        if (_udp)
            return _udp->parsePacket();
        return 0;
    }

    //------------------------------
    // End the current UDP packet
    //------------------------------

    int endPacket()
    {
        if (_udp)
            return _udp->endPacket();
        return 0;
    }

    //------------------------------
    // Get the remote IP address
    //------------------------------

    IPAddress remoteIP()
    {
#if defined(COMPILE_ETHERNET)
        if (_networkType == NETWORK_TYPE_ETHERNET)
            return ((EthernetUDP *)_udp)->remoteIP();
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            return ((WiFiUDP *)_udp)->remoteIP();
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
            return ((EthernetUDP *)_udp)->remotePort();
#endif // COMPILE_ETHERNET
#if defined(COMPILE_WIFI)
        if (_networkType == NETWORK_TYPE_WIFI)
            return ((WiFiUDP *)_udp)->remotePort();
#endif  // COMPILE_WIFI
        return 0;
    }

  protected:

    //------------------------------
    // Declare the friend classes
    //------------------------------

    friend class NetworkEthernetUdp;
    friend class NetworkWiFiUdp;
};

#ifdef  COMPILE_ETHERNET
class NetworkEthernetUdp : public NetworkUDP
{
  private:

    EthernetUDP _udp;

  public:

    NetworkEthernetUdp(EthernetUDP& udp) :
        _udp{udp},
        NetworkUDP(&_udp, NETWORK_TYPE_ETHERNET)
    {
    }

    ~NetworkEthernetUdp()
    {
        this->~NetworkUDP();
    }
};
#endif  // COMPILE_ETHERNET

#ifdef  COMPILE_WIFI
class NetworkWiFiUdp : public NetworkUDP
{
  private:

    WiFiUDP _udp;

  public:

    NetworkWiFiUdp(WiFiUDP& udp) :
        _udp{udp},
        NetworkUDP(&_udp, NETWORK_TYPE_WIFI)
    {
    }

    ~NetworkWiFiUdp()
    {
        this->~NetworkUDP();
    }
};
#endif  // COMPILE_WIFI

#endif  // __NETWORK_CLIENT_H__
