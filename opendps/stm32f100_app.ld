/* App linker script for STM32F100x8, 64K flash, 8K RAM. */

flash_size = 64k;
ram_size   = 8k;

boot_size = 5k;
past_size = 2048;
bootcom_size = 16;
app_size = flash_size - boot_size - past_size;
vector_size = 336;

/* Define memory regions. */
MEMORY
{
    boot          (rx) : ORIGIN = 0x08000000, LENGTH = boot_size
    rom           (rx) : ORIGIN = 0x08000000 + boot_size, LENGTH = app_size
    past           (r) : ORIGIN = 0x0800F800, LENGTH = past_size
    ram_vect     (rwx) : ORIGIN = 0x20000000, LENGTH = vector_size
    ram          (rwx) : ORIGIN = 0x20000000 + vector_size, LENGTH = ram_size - vector_size - bootcom_size
    bootcom_ram  (rwx) : ORIGIN = 0x20001FF0, LENGTH = bootcom_size
}

/* Include the common ld script. */
INCLUDE cortex-m-generic.ld

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

   .ram_vect : {
        _ram_vect_start = .;
        . = . + vector_size;
        _ram_vect_end = .;
     } >ram_vect
}
