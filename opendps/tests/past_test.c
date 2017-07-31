#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "past.h"
#include "flash.h"

uint32_t g_num_fail, g_num_pass;


uint8_t past_block1[1024];
uint8_t past_block2[1024];

void lock_flash(void) {}
void unlock_flash(void) {}

past_t past;

void flash_erase_page(uint32_t address)
{
    memset((char*) address, 0xff, 1024);
}

void flash_program_word(uint32_t address, uint32_t data)
{
//    printf("[0x%08x] = 0x%08x\n", address, data);
    *((uint32_t*) address) = data;
}

uint32_t flash_get_status_flags(void)
{
    return FLASH_SR_EOP;
}


#define VERBOSE_ERRORS


#ifdef VERBOSE_ERRORS
// http://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
static void hexdump(char *desc, void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
    // Output description if given.
    if (desc != NULL)
        printf ("%s (0x%08x):\n", desc, (uint32_t) addr);
    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }
    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);
            // Output the offset.
            printf ("  %04x ", i);
        }
        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);
        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }
    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }
    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
#endif // VERBOSE_ERRORS

int main(int argc, char const *argv[])
{
    uint32_t itest = 0x11223344;
    char *stest1 = "Hello World!!";
    char *stest2 = "Hello World again!!";

    memset((void*) &past_block1, 0xcd, sizeof(past_block1));
    memset((void*) &past_block2, 0xcd, sizeof(past_block2));

    past.blocks[0] = (uint32_t) past_block1;
    past.blocks[1] = (uint32_t) past_block2;
    if (past_init(&past)) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_write_unit(&past, 1, (void*) &itest, sizeof(itest))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_write_unit(&past, 2, (void*) stest1, strlen(stest1))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_erase_unit(&past, 1)) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_erase_unit(&past, 2)) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    // Run init again, simulating a reboot
    if (past_init(&past)) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_write_unit(&past, 1, (void*) &itest, sizeof(itest))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_write_unit(&past, 2, (void*) stest1, strlen(stest1))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_write_unit(&past, 2, (void*) stest2, strlen(stest2))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    uint32_t *p1, length1;
    if (past_read_unit(&past, 1, (const void**) &p1, &length1) && length1 == 4 && *p1 == itest) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    char *p2;
    uint32_t length2;
    if (past_read_unit(&past, 2, (const void**) &p2, &length2) && length2 == strlen(stest2) && strcmp(p2, stest2) == 0) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    uint8_t buf[16*56];
    memset(buf, 0xcd, sizeof(buf));
    if (past_write_unit(&past, 3, (void*) &buf, sizeof(buf))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_erase_unit(&past, 3)) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }


    if (past_write_unit(&past, 1, (void*) &itest, sizeof(itest))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    // Past block 1 is full at this point, another write will cause compaction
    itest = 0x99887766;

    if (past_write_unit(&past, 1, (void*) &itest, sizeof(itest))) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_read_unit(&past, 1, (const void**) &p1, &length1) && length1 == 4 && *p1 == itest) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }

    if (past_read_unit(&past, 2, (const void**) &p2, &length2) && length2 == strlen(stest2) && strcmp(p2, stest2) == 0) {
        g_num_pass++;
    } else {
        g_num_fail++;
    }


//    hexdump("block 1", past_block1, sizeof(past_block1));
//    hexdump("block 2", past_block2, sizeof(past_block2));

    if (g_num_fail == 0) {
        printf("All tests passed\n");
    } else if (g_num_pass == 0) {
        printf("All tests failed!\n");
    } else {
        printf ("%d/%d test failed\n", g_num_fail, g_num_pass);
    }
    printf("\n");

	return 0;
}
