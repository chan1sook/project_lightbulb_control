#include "eepromHelper.h"

uint64_t eeprom_crc(uint16_t length)
{
  uint64_t crc = ~0L;
  for (int index = EEPROM_VERSION_ADDR; index < length; ++index)
  {
    crc = CRC_TABLE[(crc ^ EEPROM.read(index)) & 0x0f] ^ (crc >> 4);
    crc = CRC_TABLE[(crc ^ (EEPROM.read(index) >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }

  return crc;
}

bool is_eeprom_valid(uint16_t length)
{
  Serial.print(F("CRC: "));
  Serial.print(read_eeprom_uint64(EEPROM_CRC_ADDR), 16);
  Serial.print(F(" | "));
  Serial.println(eeprom_crc(length), 16);
  Serial.print(F("Version: "));
  Serial.print(read_eeprom_uint32(EEPROM_VERSION_ADDR));
  Serial.print(F(" | "));
  Serial.println(EEPROM_VERSION);
  Serial.print(F("Sig: "));
  Serial.print(read_eeprom_uint64(EEPROM_SIGNATURE_ADDR), 16);
  Serial.print(F(" | "));
  Serial.println(EEPROM_SIGNATURE, 16);

  return (eeprom_crc(length) == read_eeprom_uint64(EEPROM_CRC_ADDR)) && (read_eeprom_uint32(EEPROM_VERSION_ADDR) == EEPROM_VERSION) && (read_eeprom_uint64(EEPROM_SIGNATURE_ADDR) == EEPROM_SIGNATURE);
}

bool commit_eeprom_with_headers(uint16_t length)
{
  write_eeprom(EEPROM_VERSION_ADDR, (uint32_t)EEPROM_VERSION);
  write_eeprom(EEPROM_SIGNATURE_ADDR, (uint64_t)EEPROM_SIGNATURE);
  write_eeprom(EEPROM_CRC_ADDR, (uint64_t)eeprom_crc(length));
  return EEPROM.commit();
}

int write_eeprom_raw(int address, void *data, int length)
{
  for (int i = 0; i < length; i++)
  {
    uint8_t *pt = (uint8_t *)data + i;
    EEPROM.write(address + i, *pt);
  }
  return length;
}

int write_eeprom(int address, uint32_t data)
{
  return write_eeprom_raw(address, &data, sizeof(uint32_t));
}
int write_eeprom(int address, uint64_t data)
{
  return write_eeprom_raw(address, &data, sizeof(uint64_t));
}

int read_eeprom_raw(int address, void *target, int length)
{
  for (int i = 0; i < length; i++)
  {
    uint8_t *pt = (uint8_t *)target + i;
    *pt = EEPROM.read(address + i);
  }
  return length;
}

uint32_t read_eeprom_uint32(int address)
{
  uint32_t target;
  read_eeprom_raw(address, &target, sizeof(uint32_t));
  return target;
}
uint64_t read_eeprom_uint64(int address)
{
  uint64_t target;
  read_eeprom_raw(address, &target, sizeof(uint64_t));
  return target;
}