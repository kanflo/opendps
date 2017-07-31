#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_SR_EOP (1)

void flash_erase_page(uint32_t address);
void flash_program_word(uint32_t address, uint32_t data);
uint32_t flash_get_status_flags(void);

#endif // __FLASH_H__
