#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>
#include "past.h"

#define FLASH_SR_EOP (1)

uint32_t _flash_read32(uint32_t address);
void flash_erase_page(uint32_t address);
void flash_program_word(uint32_t address, uint32_t data);
uint32_t flash_get_status_flags(void);
void hexdump(char *desc, void *addr, int len);

#ifdef DPS_EMULATOR
void flash_emul_init(past_t *past, char *file_name, bool save_past);
uint32_t flash_read_word(uint32_t address);
#endif // DPS_EMULATOR

#endif // __FLASH_H__
