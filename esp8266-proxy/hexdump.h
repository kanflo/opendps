#include <stdint.h>

/** Number of bytes to dump per line */
#define DUMP_LINE_WIDTH  (16)

/**
  * @brief Dump data in hex format, DUMP_LINE_WIDTH on each line
  * @param buf pointer to data
  * @param len length of data
  * @retval none
  */
void hexdump(uint8_t *buf, uint32_t len);
