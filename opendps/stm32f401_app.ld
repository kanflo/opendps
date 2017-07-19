/* Linker script for STM32F100x8, 64K flash, 8K RAM. */

boot_size = 4096;
past_size = 2048;
bootcom_size = 16;
app_size = 64k - boot_size - past_size;
vector_size = 336;

/* Define memory regions. */
MEMORY
{
    boot          (rx) : ORIGIN = 0x08000000, LENGTH = boot_size
    rom           (rx) : ORIGIN = 0x08001000, LENGTH = app_size
    past           (r) : ORIGIN = 0x0800F800, LENGTH = past_size
    ram_vect     (rwx) : ORIGIN = 0x20000000, LENGTH = vector_size
    ram          (rwx) : ORIGIN = 0x20000000, LENGTH = 8K - vector_size - bootcom_size
    bootcom_ram  (rwx) : ORIGIN = 0x20001FF0, LENGTH = bootcom_size
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f1.ld

SECTIONS {
    .past : {
        _past_start = .;
        . = . + past_size;
        _past_end = .;
     } >past
    
    .bootcom : {
        _bootcom_start = .;
        . = . + bootcom_size;
        _bootcom_end = .;
     } >bootcom_ram
}