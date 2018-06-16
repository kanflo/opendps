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
#include <string.h>
#include "protocol.h"
#include "uframe.h"


// Boiler plate code
#define COPY_FRAME_RETURN() \
	if (_length <= length) { \
		memcpy((void*) frame, (void*) _buffer, _length); \
		return _length; \
	} else { \
		return 0; \
	}

uint32_t protocol_create_response(uint8_t *frame, uint32_t length, command_t cmd, uint8_t success)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_response | cmd);
	PACK8(success);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

uint32_t protocol_create_ping(uint8_t *frame, uint32_t length)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_ping);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

uint32_t protocol_create_status(uint8_t *frame, uint32_t length)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_query);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

uint32_t protocol_create_wifi_status(uint8_t *frame, uint32_t length, wifi_status_t status)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_wifi_status);
	PACK8(status);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

uint32_t protocol_create_lock(uint8_t *frame, uint32_t length, uint8_t locked)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_lock);
	PACK8(!!locked);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

uint32_t protocol_create_ocp(uint8_t *frame, uint32_t length, uint16_t i_cut)
{
	DECLARE_FRAME(MAX_FRAME_LENGTH);
	PACK8(cmd_ocp_event);
	PACK16(i_cut);
	FINISH_FRAME();
	COPY_FRAME_RETURN();
}

bool protocol_unpack_response(uint8_t *payload, uint32_t length, command_t *cmd, uint8_t *success)
{
	DECLARE_UNPACK(payload, length);
	UNPACK8(*cmd);
	UNPACK8(*success);
	return _remain == 0;
}

#if 0
bool protocol_unpack_power_enable(uint8_t *payload, uint32_t length, uint8_t *enable)
{
    command_t cmd;
    DECLARE_UNPACK(payload, length);
    UNPACK8(cmd);
    UNPACK8(*enable);
    *enable = !!(*enable);
    return _remain == 0 && cmd == cmd_power_enable;
}

bool protocol_unpack_vout(uint8_t *payload, uint32_t length, uint16_t *vout_mv)
{
	command_t cmd;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK16(*vout_mv);
	return _remain == 0 && cmd == cmd_set_vout;
}

#endif

bool protocol_unpack_query_response(uint8_t *payload, uint32_t length, uint16_t *v_in, uint16_t *v_out_setting, uint16_t *v_out, uint16_t *i_out, uint16_t *i_limit, uint8_t *power_enabled)
{
	command_t cmd;
	uint8_t status;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK8(status);
	UNPACK16(*v_in);
	UNPACK16(*v_out_setting);
	UNPACK16(*v_out);
	UNPACK16(*i_out);
	UNPACK16(*i_limit);
	UNPACK8(*power_enabled);
	*power_enabled = !!(*power_enabled);
	(void) status;
	return _remain == 0 && cmd == (cmd_response | cmd_query);
}

bool protocol_unpack_wifi_status(uint8_t *payload, uint32_t length, wifi_status_t *status)
{
	command_t cmd;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK8(*status);
	return _remain == 0 && cmd == cmd_wifi_status;
}

bool protocol_unpack_lock(uint8_t *payload, uint32_t length, uint8_t *locked)
{
	command_t cmd;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK8(*locked);
	*locked = !!(*locked);
	return _remain == 0 && cmd == cmd_lock;
}

bool protocol_unpack_upgrade_start(uint8_t *payload, uint32_t length, uint16_t *chunk_size, uint16_t *crc)
{
	command_t cmd;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK16(*chunk_size);
	UNPACK16(*crc);
	return _remain == 0 && cmd == cmd_upgrade_start;
}

bool protocol_unpack_ocp(uint8_t *payload, uint32_t length, uint16_t *i_cut)
{
	command_t cmd;
	DECLARE_UNPACK(payload, length);
	UNPACK8(cmd);
	UNPACK16(*i_cut);
	return _remain == 0 && cmd == cmd_ocp_event;
}

