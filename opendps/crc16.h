#ifndef __CRC16_H__
#define __CRC16_H__

#include <stdint.h>

/**
  * @brief Add byte to crc
  * @param crc crc calculated so far
  * @param byte byte to add to crc calculation
  * @retval crc after adding byte
  * @note This function is used for streamin operations. Initialize with crc=0
  */
uint16_t crc16_add(uint16_t crc, uint8_t byte);

/**
  * @brief Calculate 16 bit crc
  * @param data pointer to data
  * @param length length of data
  * @retval 16 bit crc of data
  */
uint16_t crc16(uint8_t *data, uint16_t length);

#endif // __CRC_H__
