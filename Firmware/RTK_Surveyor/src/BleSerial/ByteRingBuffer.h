#pragma once
#include <Arduino.h>


template <size_t N>
class ByteRingBuffer
{
private:
	uint8_t ringBuffer[N];
	size_t newestIndex = 0;
	size_t length = 0;

public:
	void add(uint8_t value)
	{
		ringBuffer[newestIndex] = value;
		newestIndex = (newestIndex + 1) % N;
		length = min(length + 1, N);
	}
	uint8_t pop()
	{ // pops the oldest value off the ring buffer
		if (length == 0)
		{
			return -1;
		}
		uint8_t result = ringBuffer[(N + newestIndex - length) % N];
		length -= 1;
		return result;
	}
	void clear()
	{
		newestIndex = 0;
		length = 0;
	}
	uint8_t get(size_t index)
	{ // this.get(0) is the oldest value, this.get(this.getLength() - 1) is the newest value
		if (index < 0 || index >= length)
		{
			return -1;
		}
		return ringBuffer[(N + newestIndex - length + index) % N];
	}
	size_t getLength() { return length; }
};

