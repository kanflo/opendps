/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Johan Kanflo (github.com/kanflo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>

/** A big endian frame packer/unpacker abstraction. 
  *
  * These macros take care of the complexity of dealing with network protocol
  * frames. Say we have received the following frame:
  *
  * [ 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xff ]
  * 
  * and according to the protocol specification, it contsins an uint32
  * a uint16 and a uint8 (these happen to be 0x11223344, 0xaabb and 0xff)
  *
  * We need to parse the frame, shift bytes into ints and keep an eye of the
  * length of the received data. This can be accomplished with the following
  * code:
  *
  *     uint8_t buffer[] = { 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xff };
  *     uint32_t i32;
  *     uint16_t i16;
  *     uint8_t  i8;
  * 
  *     UNPACK_START(buffer, sizeof(buffer));
  *     UNPACK_UINT32(i32);
  *     UNPACK_UINT16(i16);
  *     UNPACK_UINT8(i8);
  *     UNPACK_END();
  * UNPACK_ERROR_HANDLER:
  *     printf("Unpack frame error\n");
  *     UNPACK_DONE();
  * UNPACK_SUCCESS_HANDLER:
  *     printf("Unpack frame ok\n");
  *     printf(" i32  : 0x%08x\n", i32);
  *     printf(" i16  : 0x%04x\n", i16);
  *     printf(" i8   : 0x%02x\n", i8);
  *     UNPACK_DONE();
  * UNPACK_DONE_HANDLER:
  *     printf("Unpacker done\n");
  *
  * Running the program would yield the following output:
  *
  *  Unpack frame ok
  *  i32  : 0x11223344
  *  i16  : 0xaabb
  *  i8   : 0xff
  *  Unpacker done
  *
  * If the frame is too short, we will get the error "Unpack frame error"
  * as we automatically end up in the UNPACK_ERROR_HANDLER if we try to read
  * more data from the frame than its size allows.
  * 
  * We can also construct frames using the PACK_* macros:
  * 
  *     uint8_t buffer[32];
  *     uint32_t len;
  *     memset(buffer, 0, sizeof(buffer));
  *     PACK_START(buffer, sizeof(buffer));
  *     PACK_UINT32(0x11223344);
  *     PACK_UINT16(0xaabb);
  *     PACK_UINT8(0xff);
  *     PACK_CSTR("hej");
  *     PACK_END(len);
  * PACK_ERROR_HANDLER:
  *     printf("Pack frame error\n");
  *     PACK_DONE();
  * PACK_SUCCESS_HANDLER:
  *     printf("Pack frame ok, %d bytes\n", len);
  *     hexdump(buffer, len);
  * PACK_DONE_HANDLER:
  *     return;
  * 
  * The buffer now holds the following data:
  *
  *    11 22 33 44 aa bb ff 68 65 6a 00  [."3D...hej.]
  *
  */

/** Declare start of unpacker of given buffer and length */
#define UNPACK_START(buf, len) \
	uint8_t *_frame = (uint8_t *) buf; \
	uint32_t _len = len, _pos = 0

/** Unpack a uint32_t */
#define UNPACK_UINT32(i) \
	if (_pos+4 > _len) goto _unpack_frame_error; \
	i = (_frame[_pos] << 24) | (_frame[_pos+1] << 16) | (_frame[_pos+2] << 8) | (_frame[_pos+3]); \
	_pos += 4

/** Unpack a uint16_t */
#define UNPACK_UINT16(i) \
	if (_pos+2 > _len) goto _unpack_frame_error; \
	i = (_frame[_pos] << 8) | (_frame[_pos+1]); \
	_pos += 2

/** Unpack a uint8_t */
#define UNPACK_UINT8(i) \
    if (_pos > _len) goto _unpack_frame_error; \
    i = _frame[_pos]; \
    _pos += 1

/** Unpack a C string (null terminated) */
#define UNPACK_CSTR(str, str_len) \
    strncpy((char*)str, (char*) &_frame[_pos], str_len); \
    if (_pos+strlen((char*) str) + 1 > _len) goto _unpack_frame_error; \
    _pos += strlen((char*) str) + 1

/** Declare end of unpacker */
#define UNPACK_END() \
    goto _unpack_frame_ok

/** Declare unpacking done */
#define UNPACK_DONE() \
    goto _unpack_frame_done

/** Unpacking handlers */
#define UNPACK_ERROR_HANDLER _unpack_frame_error
#define UNPACK_SUCCESS_HANDLER _unpack_frame_ok
#define UNPACK_DONE_HANDLER _unpack_frame_done




/** Declare start of packer of given buffer and length */
#define PACK_START(buf, len) \
    uint8_t *_frame = (uint8_t *) buf; \
    uint32_t _len = len, _pos = 0

/** Pack a uint32_t */
#define PACK_UINT32(i) \
    if (_pos+4 > _len) goto _pack_frame_error; \
    _frame[_pos++] = (i>>24) & 0xff; \
    _frame[_pos++] = (i>>16) & 0xff; \
    _frame[_pos++] = (i>>8) & 0xff; \
    _frame[_pos++] = (i) & 0xff

/** Pack a uint16_t */
#define PACK_UINT16(i) \
    if (_pos+2 > _len) goto _pack_frame_error; \
    _frame[_pos++] = (i>>8) & 0xff; \
    _frame[_pos++] = (i) & 0xff

/** Pack a uint8_t */
#define PACK_UINT8(i) \
    if (_pos+1 > _len) goto _pack_frame_error; \
    _frame[_pos++] = (i) & 0xff

/** Pack a C string (with null terminator) */
#define PACK_CSTR(str) \
    if (_pos+strlen(str)+1 > _len) goto _pack_frame_error; \
    strncpy((char*) &_frame[_pos], (char*)str, _len - _pos); \
    _pos += strlen((char*) str); \
    _frame[_pos++] = 0

/** Declare end of packer, return length */
#define PACK_END(len) \
    len = _pos; \
    goto _pack_frame_ok

/** Declare packing done */
#define PACK_DONE() \
    goto _pack_frame_done


/** Packing handlers */
#define PACK_ERROR_HANDLER _pack_frame_error
#define PACK_SUCCESS_HANDLER _pack_frame_ok
#define PACK_DONE_HANDLER _pack_frame_done
