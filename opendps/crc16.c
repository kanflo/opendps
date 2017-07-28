#include "crc16.h"

/**
  * @brief Add byte to crc
  * @param crc crc calculated so far
  * @param byte byte to add to crc calculation
  * @retval crc after adding byte
  * @note This function is used for streamin operations. Initialize with crc=0
  */
uint16_t crc16_add(uint16_t crc, uint8_t byte)
{
    uint8_t x = crc >> 8 ^ byte;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    return crc & 0xFFFF;
}

/**
  * @brief Calculate 16 bit crc
  * @param data pointer to data
  * @param length length of data
  * @retval 16 bit crc of data
  */
uint16_t crc16(uint8_t *data, uint16_t length)
{
    uint8_t x;
    uint16_t crc = 0; // 0x1d0F; // 0x1021;
    if (data && length) {
        while (length--){
            x = crc >> 8 ^ *data++;
            x ^= x >> 4;
            crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
        }
    }
    return crc;
}
