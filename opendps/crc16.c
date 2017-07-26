#include "crc16.h"

uint16_t crc16(uint8_t *data, uint16_t length)
{
    unsigned char x;
    uint16_t crc = 0; // 0x1d0F; // 0x1021;
    if (data && length) {
	    while (length--){
	        x = crc >> 8 ^ *data++;
	        x ^= x>>4;
	        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
	    }
    }
    return crc & 0xFFFF;
}
