#include "FM24CLXX.h"

FM24CLXX::FM24CLXX(uint8_t deviceAddress)
    : _deviceAddress(deviceAddress) {}

bool FM24CLXX::begin()
{
  // Wire.begin();
  Wire.beginTransmission(_deviceAddress);
  byte error = Wire.endTransmission();
  if (error == 0)
  {
    return true;
  }

  return false;
}

bool FM24CLXX::writeByte(uint16_t address, uint8_t data)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write((uint8_t)(address >> 8));   // MSB
  Wire.write((uint8_t)(address & 0xFF)); // LSB
  Wire.write(data);
  return (Wire.endTransmission() == 0);
}

bool FM24CLXX::readByte(uint16_t address, uint8_t *data)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write((uint8_t)(address >> 8));   // MSB
  Wire.write((uint8_t)(address & 0xFF)); // LSB
  if (Wire.endTransmission() != 0)
  {
    return false;
  }
  Wire.requestFrom(_deviceAddress, (uint8_t)1);
  if (Wire.available())
  {
    *data = Wire.read();
    return true;
  }
  return false;
}

bool FM24CLXX::writeData(uint16_t address, const uint8_t *data, size_t length)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write((uint8_t)(address >> 8));   // MSB
  Wire.write((uint8_t)(address & 0xFF)); // LSB
  for (size_t i = 0; i < length; i++)
  {
    Wire.write(data[i]);
  }
  return (Wire.endTransmission() == 0);
}

bool FM24CLXX::readData(uint16_t address, uint8_t *data, size_t length)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write((uint8_t)(address >> 8));   // MSB
  Wire.write((uint8_t)(address & 0xFF)); // LSB
  if (Wire.endTransmission() != 0)
  {
    return false;
  }
  Wire.requestFrom(_deviceAddress, (uint8_t)length);
  size_t index = 0;
  while (Wire.available() && index < length)
  {
    data[index++] = Wire.read();
  }
  return (index == length);
}
bool FM24CLXX::writeUInt32(uint16_t address, uint32_t data)
{
  uint8_t bytes[4];
  bytes[0] = (uint8_t)(data >> 24); // MSB
  bytes[1] = (uint8_t)(data >> 16);
  bytes[2] = (uint8_t)(data >> 8);
  bytes[3] = (uint8_t)data; // LSB
  return writeData(address, bytes, 4);
}

bool FM24CLXX::readUInt32(uint16_t address, uint32_t *data)
{
  uint8_t bytes[4];
  if (!readData(address, bytes, 4))
  {
    return false;
  }
  *data = ((uint32_t)bytes[0] << 24) |
          ((uint32_t)bytes[1] << 16) |
          ((uint32_t)bytes[2] << 8) |
          (uint32_t)bytes[3];
  return true;
}

bool FM24CLXX::writeUInt64(uint16_t address, uint64_t data)
{
  uint8_t bytes[8];
  bytes[0] = (uint8_t)(data >> 56); // MSB
  bytes[1] = (uint8_t)(data >> 48);
  bytes[2] = (uint8_t)(data >> 40);
  bytes[3] = (uint8_t)(data >> 32);
  bytes[4] = (uint8_t)(data >> 24);
  bytes[5] = (uint8_t)(data >> 16);
  bytes[6] = (uint8_t)(data >> 8);
  bytes[7] = (uint8_t)data; // LSB
  return writeData(address, bytes, 8);
}
bool FM24CLXX::readUInt64(uint16_t address, uint64_t *data)
{
  uint8_t bytes[8];
  if (!readData(address, bytes, 8))
  {
    return false;
  }
  *data = ((uint64_t)bytes[0] << 56) |
          ((uint64_t)bytes[1] << 48) |
          ((uint64_t)bytes[2] << 40) |
          ((uint64_t)bytes[3] << 32) |
          ((uint64_t)bytes[4] << 24) |
          ((uint64_t)bytes[5] << 16) |
          ((uint64_t)bytes[6] << 8) |
          (uint64_t)bytes[7];
  return true;
}
