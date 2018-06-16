#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "past.h"

#define FLASH_SIZE  (2048)

static uint8_t flash[FLASH_SIZE];
static char *past_name;
bool persistent;

void flash_emul_init(past_t *past, char *_past_name, bool _persistent)
{
    past_name = _past_name;
    persistent = _persistent;
    past->blocks[0] = 0;
    past->blocks[1] = 1024;
    memset(flash, 0xff, FLASH_SIZE);
    if (past_name) {
        FILE *f = fopen(past_name, "rb");
        if (f) {
            printf("Reading past from %s (with%s persistence)\n", past_name, _persistent ? "" : "out");
            fread((void*) flash, FLASH_SIZE, 1, f);
            fclose(f);
        } else {
            printf("Past file %s does not exist\n", past_name);
        }
    }
}

static void save_past(void)
{
    if (persistent && past_name) {
        printf("Saving past to %s\n", past_name);
        FILE *f = fopen(past_name, "wb+");
        if (!f) {
            fprintf(stderr, "Error: failed to save %s\n", past_name);
            exit(EXIT_FAILURE);
        }
        fwrite((void*) flash, 1, FLASH_SIZE, f);
        fclose(f);
    }
}

void lock_flash(void) {}
void unlock_flash(void) {}

void flash_erase_page(uint32_t address)
{
    if (address > FLASH_SIZE) {
        printf("Flash out of bound erase access at 0x%08x\n", address);
        exit(EXIT_FAILURE);
    }
    memset(&flash[address], 0xff, 1024);
    save_past();
}

void flash_program_word(uint32_t address, uint32_t data)
{
    if (address > FLASH_SIZE) {
        printf("Flash out of bound write access at 0x%08x\n", address);
        exit(EXIT_FAILURE);
    }
    uint32_t *temp = (uint32_t*) &flash[address];
    *temp = data;
    save_past();
}

uint32_t flash_read_word(uint32_t address)
{
    if (address > FLASH_SIZE) {
        printf("Flash out of bound read access at 0x%08x\n", address);
        exit(EXIT_FAILURE);
    }
    uint32_t *temp = (uint32_t*) &flash[address];
    return *temp;
}

uint32_t flash_get_status_flags(void)
{
    return FLASH_SR_EOP;
}

// http://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
void hexdump(char *desc, void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
    printf("%p %02x\n", pc, ((char*)pc)[0]);
    printf("%p\n", pc);
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
