#include "hexdump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hexdump.h"
#include "packer.h"


void pack_test(void)
{
    printf("### Pack test #################################\n");
    uint8_t buffer[32];
    uint32_t len;
    memset(buffer, 0, sizeof(buffer));

    PACK_START(buffer, sizeof(buffer));
    PACK_UINT32(0x11223344);
    PACK_UINT16(0xaabb);
    PACK_UINT8(0xff);
    PACK_CSTR("hej");
    PACK_END(len);
PACK_ERROR_HANDLER:
    printf("Pack frame error\n");
    PACK_DONE();
PACK_SUCCESS_HANDLER:
    printf("Pack frame ok, %d bytes\n", len);
    hexdump(buffer, len);
PACK_DONE_HANDLER:
    printf("Packer done\n");
}

void unpack_test(void)
{
    printf("### Unpack test #################################\n");
    uint8_t buffer[] = { 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xff };
    uint32_t i32;
    uint16_t i16;
    uint8_t i8;
    hexdump((uint8_t*) buffer, sizeof(buffer));
    UNPACK_START(buffer, sizeof(buffer));
    UNPACK_UINT32(i32);
    UNPACK_UINT16(i16);
    UNPACK_UINT8(i8);
    UNPACK_END();
UNPACK_ERROR_HANDLER:
    printf("Unpack frame error\n");
    UNPACK_DONE();
UNPACK_SUCCESS_HANDLER:
    printf("Unpack frame ok\n");
    printf(" i32  : 0x%08x\n", i32);
    printf(" i16  : 0x%04x\n", i16);
    printf(" i8   : 0x%02x\n", i8);
    UNPACK_DONE();
UNPACK_DONE_HANDLER:
    printf("Unpacker done\n");
}

int main(int argc, char const *argv[])
{
    pack_test();
    unpack_test();
    return 0;
}
