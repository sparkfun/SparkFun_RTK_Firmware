#ifdef COMPILE_BT

#include "src/BluetoothSerial/BluetoothSerial.h"
#include <BleSerial.h> // Click here to get the library: http://librarymanager/All#ESP32_BleSerial by Avinab Malla

class BTSerialInterface
{
    public:
        virtual bool begin(String deviceName, bool isMaster, uint16_t rxQueueSize, uint16_t txQueueSize) = 0;
        virtual void disconnect() = 0;
        virtual void end() = 0;
        virtual esp_err_t register_callback(esp_spp_cb_t * callback) = 0;
        virtual void setTimeout(unsigned long timeout) = 0;
 
        virtual int available() = 0;
        virtual size_t readBytes(uint8_t *buffer, size_t bufferSize) = 0;

        virtual bool isCongested() = 0;
        virtual size_t write(const uint8_t *buffer, size_t size) = 0;
        virtual void flush() = 0;
};


class BTClassicSerial : public virtual BTSerialInterface, public BluetoothSerial
{
    // Everything is already implemented in BluetoothSerial since the code was
    // originally written using that class
    public:
        bool begin(String deviceName, bool isMaster, uint16_t rxQueueSize, uint16_t txQueueSize)
        {
            return BluetoothSerial::begin(deviceName, isMaster, rxQueueSize, txQueueSize);
        }

        void disconnect()
        {
            BluetoothSerial::disconnect();
        }

        void end()
        {
            BluetoothSerial::end();
        }

        esp_err_t register_callback(esp_spp_cb_t * callback)
        {
            return BluetoothSerial::register_callback(callback);
        }

        void setTimeout(unsigned long timeout)
        {
            BluetoothSerial::setTimeout(timeout);
        }
 
        int available()
        {
            return BluetoothSerial::available();
        }

        size_t readBytes(uint8_t *buffer, size_t bufferSize)
        {
            return BluetoothSerial::readBytes(buffer, bufferSize);
        }

        bool isCongested()
        {
            return BluetoothSerial::isCongested();
        }

        size_t write(const uint8_t *buffer, size_t size)
        {
            return BluetoothSerial::write(buffer, size);
        }

        void flush()
        {
            BluetoothSerial::flush();
        }
};


class BTLESerial: public virtual BTSerialInterface, public BleSerial
{
    public:
        // Missing from BleSerial
        bool begin(String deviceName, bool isMaster, uint16_t rxQueueSuze, uint16_t txQueueSize)
        {
            // Curretnly ignoring rxQueueSize
            // transmitBufferLength = txQueueSize;
            BleSerial::begin(deviceName.c_str());
            return true;
        }

        void disconnect()
        {
            Server->disconnect(Server->getConnId());
        }

        void end()
        {
            BleSerial::end();
        }

        esp_err_t register_callback(esp_spp_cb_t * callback)
        {
            connectionCallback = callback;
            return ESP_OK;
        }

        void setTimeout(unsigned long timeout)
        {
            BleSerial::setTimeout(timeout);
        }
 
        int available()
        {
            return BleSerial::available();
        }

        size_t readBytes(uint8_t *buffer, size_t bufferSize)
        {
            return BleSerial::readBytes(buffer, bufferSize);
        }

        bool isCongested()
        {
            // not currently supported in this implementation
            return false;
        }

        size_t write(const uint8_t *buffer, size_t size)
        {
            return BleSerial::write(buffer, size);
        }

        void flush()
        {
            BleSerial::flush();
        }

        // override BLEServerCallbacks
        void onConnect(BLEServer *pServer)
        {
            bleConnected = true;
            connectionCallback(ESP_SPP_SRV_OPEN_EVT, nullptr);
        }

        void onDisconnect(BLEServer *pServer)
        {
            bleConnected = false;
            connectionCallback(ESP_SPP_CLOSE_EVT, nullptr);
            Server->startAdvertising();
        }

    private:
        esp_spp_cb_t * connectionCallback;

};


#endif
