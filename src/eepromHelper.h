#ifndef _EEPROM_HELPER_H
#define _EEPROM_HELPER_H

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_AVALIABLE (1)
#define EEPROM_FORCE_WRITE (0)

#define EEPROM_VERSION (2)
#define EEPROM_SIGNATURE (0xFA009C93C1832A09)

#define EEPROM_CRC_LENGTH (sizeof(uint64_t))
#define EEPROM_VERSION_LENGTH (sizeof(uint32_t))
#define EEPROM_SIGNATUE_LENGTH (sizeof(uint64_t))

#define EEPROM_CRC_ADDR (0)
#define EEPROM_VERSION_ADDR (EEPROM_CRC_ADDR + EEPROM_CRC_LENGTH)
#define EEPROM_SIGNATURE_ADDR (EEPROM_VERSION_ADDR + EEPROM_VERSION_LENGTH)

#define EEPROM_USEABLE_ADDR (EEPROM_SIGNATURE_ADDR + EEPROM_SIGNATUE_LENGTH)

const uint64_t CRC_TABLE[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};

uint64_t eeprom_crc(uint16_t length);
bool is_eeprom_valid(uint16_t length);
bool commit_eeprom_with_headers(uint16_t length);

int write_eeprom_raw(int address, void *data, int length);
int write_eeprom(int address, uint32_t data);
int write_eeprom(int address, uint64_t data);

int read_eeprom_raw(int address, void *target, int length);

uint32_t read_eeprom_uint32(int address);
uint64_t read_eeprom_uint64(int address);
#endif