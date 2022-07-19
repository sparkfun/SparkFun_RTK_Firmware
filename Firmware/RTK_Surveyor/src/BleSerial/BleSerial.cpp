#include "BleSerial.h"
using namespace std;

bool BleSerial::connected()
{
	return Server->getConnectedCount() > 0;
}

void BleSerial::onConnect(BLEServer *pServer)
{
	bleConnected = true;
	if(ENABLE_LED) digitalWrite(BLUE_LED, HIGH);
}

void BleSerial::onDisconnect(BLEServer *pServer)
{
	bleConnected = false;
	if(ENABLE_LED) digitalWrite(BLUE_LED, LOW);
	Server->startAdvertising();
}

int BleSerial::read()
{
	uint8_t result = this->receiveBuffer.pop();
	if (result == (uint8_t)'\n')
	{
		this->numAvailableLines--;
	}
	return result;
}

size_t BleSerial::readBytes(uint8_t *buffer, size_t bufferSize)
{
	int i = 0;
	while (i < bufferSize && available())
	{
		buffer[i] = this->receiveBuffer.pop();
		i++;
	}
	return i;
}

int BleSerial::peek()
{
	if (this->receiveBuffer.getLength() == 0)
		return -1;
	return this->receiveBuffer.get(0);
}

int BleSerial::available()
{
	return this->receiveBuffer.getLength();
}

size_t BleSerial::print(const char *str)
{
	if (Server->getConnectedCount() <= 0)
	{
		return 0;
	}

	size_t written = 0;
	for (size_t i = 0; str[i] != '\0'; i++)
	{
		written += this->write(str[i]);
	}
	flush();
	return written;
}

size_t BleSerial::write(const uint8_t *buffer, size_t bufferSize)
{
	if (Server->getConnectedCount() <= 0)
	{
		return 0;
	}
	size_t written = 0;
	for (int i = 0; i < bufferSize; i++)
	{
		written += this->write(buffer[i]);
	}
	flush();
	return written;
}

size_t BleSerial::write(uint8_t byte)
{
	if (Server->getConnectedCount() <= 0)
	{
		return 0;
	}
	this->transmitBuffer[this->transmitBufferLength] = byte;
	this->transmitBufferLength++;
	if (this->transmitBufferLength == sizeof(this->transmitBuffer))
	{
		flush();
	}
	return 1;
}

void BleSerial::flush()
{
	if (this->transmitBufferLength > 0)
	{
		TxCharacteristic->setValue(this->transmitBuffer, this->transmitBufferLength);
		this->transmitBufferLength = 0;
	}
	this->lastFlushTime = millis();
	TxCharacteristic->notify(true);
}

void BleSerial::begin(const char *name)
{
	//characteristic property is what the other device does.

	ConnectedDeviceCount = 0;
	BLEDevice::init(name);
	//BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

	Server = BLEDevice::createServer();
	Server->setCallbacks(this);

	SetupSerialService();

	pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(BLE_SERIAL_SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	pAdvertising->start();

	//pSecurity = new BLESecurity();

	//Set static pin
	//uint32_t passkey = 123456;
	//esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
	//pSecurity->setCapability(ESP_IO_CAP_OUT);
}

void BleSerial::end()
{
	BLEDevice::deinit();
}

void BleSerial::onWrite(BLECharacteristic *pCharacteristic)
{
	if (pCharacteristic->getUUID().toString() == BLE_RX_UUID)
	{
		std::string value = pCharacteristic->getValue();

		if (value.length() > 0)
		{
			for (int i = 0; i < value.length(); i++)
				receiveBuffer.add(value[i]);
		}
	}
}

void BleSerial::SetupSerialService()
{
	SerialService = Server->createService(BLE_SERIAL_SERVICE_UUID);

	RxCharacteristic = SerialService->createCharacteristic(
		BLE_RX_UUID, BLECharacteristic::PROPERTY_WRITE);

	TxCharacteristic = SerialService->createCharacteristic(
		BLE_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);

	TxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
	RxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);

	TxCharacteristic->addDescriptor(new BLE2902());
	RxCharacteristic->addDescriptor(new BLE2902());

	TxCharacteristic->setReadProperty(true);
	RxCharacteristic->setWriteProperty(true);
	RxCharacteristic->setCallbacks(this);
	SerialService->start();
}

BleSerial::BleSerial()
{
}

