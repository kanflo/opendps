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

void protocol_create_response(frame_t *frame, command_t cmd, uint8_t success)
{
	set_frame_header(frame);
	pack8(frame, cmd_response | cmd);
	pack8(frame, success);
	end_frame(frame);
}

void protocol_create_ping(frame_t *frame)
{
	set_frame_header(frame);
	pack8(frame, cmd_ping);
	end_frame(frame);
}

void protocol_create_status(frame_t *frame)
{
	set_frame_header(frame);
	pack8(frame, cmd_query);
	end_frame(frame);
}

void protocol_create_wifi_status(frame_t *frame, wifi_status_t status)
{
	set_frame_header(frame);
	pack8(frame, cmd_wifi_status);
	pack8(frame, status);
	end_frame(frame);
}

void protocol_create_lock(frame_t *frame, uint8_t locked)
{
	set_frame_header(frame);
	pack8(frame, cmd_lock);
	pack8(frame, !!locked);
	end_frame(frame);
}

void protocol_create_ocp(frame_t *frame, uint16_t i_cut)
{
	set_frame_header(frame);
	pack8(frame, cmd_ocp_event);
	pack16(frame, i_cut);
	end_frame(frame);
}

bool protocol_unpack_response(frame_t *frame, command_t *cmd, uint8_t *success)
{
	start_frame_unpacking(frame);
	unpack8(frame, cmd);
	unpack8(frame, success);
	return frame->length == 0;
}

bool protocol_unpack_query_response(frame_t *frame, uint16_t *v_in, uint16_t *v_out_setting, uint16_t *v_out, uint16_t *i_out, uint16_t *i_limit, uint8_t *power_enabled)
{
	command_t cmd;
	uint8_t status;

	start_frame_unpacking(frame);
	unpack8(frame, &cmd);
	unpack8(frame, &status);
	unpack16(frame, v_in);
	unpack16(frame, v_out_setting);
	unpack16(frame, v_out);
	unpack16(frame, i_out);
	unpack16(frame, i_limit);
	unpack8(frame, power_enabled);
	*power_enabled = !!(*power_enabled);
	(void) status;

	return frame->length == 0 && cmd == (cmd_response | cmd_query);
}

bool protocol_unpack_wifi_status(frame_t *frame, wifi_status_t *status)
{
	command_t cmd;

	start_frame_unpacking(frame);
	unpack8(frame, &cmd);
	unpack8(frame, status);

	return frame->length == 0 && cmd == cmd_wifi_status;
}

bool protocol_unpack_lock(frame_t *frame, uint8_t *locked)
{
	command_t cmd;

	start_frame_unpacking(frame);
	unpack8(frame, &cmd);
	unpack8(frame, locked);
	*locked = !!(*locked);

	return frame->length == 0 && cmd == cmd_lock;
}

bool protocol_unpack_upgrade_start(frame_t *frame, uint16_t *chunk_size, uint16_t *crc)
{
	command_t cmd;

	start_frame_unpacking(frame);
	unpack8(frame, &cmd);
	unpack16(frame, chunk_size);
	unpack16(frame, crc);

	return frame->length == 0 && cmd == cmd_upgrade_start;
}

bool protocol_unpack_ocp(frame_t *frame, uint16_t *i_cut)
{
	command_t cmd;

	start_frame_unpacking(frame);
	unpack8(frame, &cmd);
	unpack16(frame, i_cut);

	return frame->length == 0 && cmd == cmd_ocp_event;
}

