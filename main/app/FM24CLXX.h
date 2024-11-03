

#pragma once
#include "Arduino.h"
#include <Wire.h>

class FM24CLXX
{
public:
  FM24CLXX(uint8_t deviceAddress);

  bool begin();
  bool writeByte(uint16_t address, uint8_t data);
  bool readByte(uint16_t address, uint8_t *data);
  bool writeData(uint16_t address, const uint8_t *data, size_t length);
  bool readData(uint16_t address, uint8_t *data, size_t length);
  bool readUInt32(uint16_t address, uint32_t *data);
  bool writeUInt32(uint16_t address, uint32_t data);
  bool writeUInt64(uint16_t address, uint64_t data);
  bool readUInt64(uint16_t address, uint64_t *data);

private:
  uint8_t _deviceAddress;
};
