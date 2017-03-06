#include <stdio.h>
#include "hexdump.h"

/**
  * @brief Dump data in hex format, DUMP_LINE_WIDTH on each line
  * @param buf pointer to data
  * @param len length of data
  * @retval none
  */
void hexdump(uint8_t *buf, uint32_t len)
{
    uint32_t i, j, last_line = 0;
    for(i=0; i<len; i++) {
        if (i && !(i%DUMP_LINE_WIDTH)) {
            printf("   ");
            for(j = 0; j < DUMP_LINE_WIDTH && last_line+j < len; j++) {
                printf("%c", (buf[last_line+j]<' ' || buf[last_line+j]>'~') ? '.' : buf[last_line+j]);
            }
            printf("\n");
            last_line = i;
        }
        printf(" %02x", buf[i]);
    }
    if (i && (i%DUMP_LINE_WIDTH)) {
        printf("   ");
        for(j = i%DUMP_LINE_WIDTH; j<DUMP_LINE_WIDTH; j++) {
            printf("   ");
        }
        for(uint32_t j = 0; j < DUMP_LINE_WIDTH && last_line+j < len; j++) {
            printf("%c", (buf[last_line+j]<' ' || buf[last_line+j]>'~') ? '.' : buf[last_line+j]);
        }
        printf("\n");
    }
    printf("\n");
}
