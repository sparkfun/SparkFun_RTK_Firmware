//Define a hybrid class which can support both WiFiClient and EthernetClient

#ifdef COMPILE_WIFI

#ifdef COMPILE_ETHERNET

class NTRIPClient : public WiFiClient, public EthernetClient

#else

class NTRIPClient : public WiFiClient

#endif

{
  public:

    NTRIPClient()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        _ntripClientEthernet = new EthernetClient;
      else
#endif
        _ntripClientWiFi = new WiFiClient;
    };

    ~NTRIPClient()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
      {
        _ntripClientEthernet->stop();
        delete _ntripClientEthernet;
        _ntripClientEthernet = nullptr;
      }
      else
#endif
      {
        //EthernetClient does not have a destructor. It is virtual.
        delete _ntripClientWiFi;
        _ntripClientWiFi = nullptr;
      }
    };

    operator bool()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet;
      else
#endif
        return _ntripClientWiFi;
    };

    int connect(const char *host, uint16_t port)
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->connect(host, port);
      else
#endif
        return _ntripClientWiFi->connect(host, port);
    };

    size_t write(uint8_t b)
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->write(b);
      else
#endif
        return _ntripClientWiFi->write(b);
    };

    size_t write(const uint8_t *buf, size_t size)
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->write(buf, size);
      else
#endif
        return _ntripClientWiFi->write(buf, size);
    };

    int available()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->available();
      else
#endif
        return _ntripClientWiFi->available();
    };

    int read(uint8_t *buf, size_t size)
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->read();
      else
#endif
        return _ntripClientWiFi->read();
    };

    int read()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->read();
      else
#endif
        return _ntripClientWiFi->read();
    };

    void stop()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        _ntripClientEthernet->stop();
      else
#endif
        _ntripClientWiFi->stop();
    };

    uint8_t connected()
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->connected();
      else
#endif
        return _ntripClientWiFi->connected();
    };

    size_t print(const char *printMe)
    {
#ifdef COMPILE_ETHERNET
      if (HAS_ETHERNET)
        return _ntripClientEthernet->print(printMe);
      else
#endif
        return _ntripClientWiFi->print(printMe);
    };

  protected:
    WiFiClient * _ntripClientWiFi;
#ifdef COMPILE_ETHERNET
    EthernetClient * _ntripClientEthernet;
#endif
};

#endif
