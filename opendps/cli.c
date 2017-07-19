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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usart.h>
#include "cli.h"

#ifdef CONFIG_CMD_EXTRA_PADDING
 #define CMD_EXTRA_PADDING CONFIG_CMD_EXTRA_PADDING
#else
 #define CMD_EXTRA_PADDING (4)
#endif

#ifdef CONFIG_CMD_MAX_ARGC
 #define MAX_ARGC CONFIG_CMD_MAX_ARGC
#else
 #define MAX_ARGC (16)
#endif

#ifdef CONFIG_PROMPT_STR
 #define PROMPT_STR CONFIG_PROMPT_STR
#else
 #define PROMPT_STR ""
#endif

#ifdef CONFIG_ARG_DELIMITER_STR
 #define ARG_DELIMITER_STR CONFIG_ARG_DELIMITER_STR
#else
 #define ARG_DELIMITER_STR " "
#endif

/**
  * @brief Handle command input from the user
  * @param cmds array of supported commands
  * @param num number of commands in the cmds array
  * @param line user input to parsr
  * @retval None
  */
void cli_run(const cli_command_t cmds[], const uint32_t num, char *line)
{
    bool found_cmd = false;
    // Split string "<command> <argument 1> <argument 2>  ...  <argument N>"
    // into argc, argv style
    char *argv[MAX_ARGC];
    uint32_t argc = 1;
    char *temp, *rover;
    memset((void*) argv, 0, sizeof(argv));
    argv[0] = line;
    rover = line;
    rover = strstr(line, ARG_DELIMITER_STR);
    while(argc < MAX_ARGC && (temp = strstr(rover, ARG_DELIMITER_STR))) {
        temp++;
        argv[argc++] = temp;
        *rover = 0;
        rover = temp+1;
    }

    for (uint32_t i=0; i<num; i++) {
        if (cmds[i].cmd && strcmp(argv[0], cmds[i].cmd) == 0) {
            if (cmds[i].min_arg > argc-1 || cmds[i].max_arg < argc-1) {
                printf("Wrong number of arguments:\n");
                if (cmds[i].usage) {
                    printf("Usage: %s%s%s;\n", cmds[i].cmd, ARG_DELIMITER_STR, cmds[i].usage);
                }
            } else {
                cmds[i].handler(argc, argv);
            }
            found_cmd = true;
        }
    }

    if (!found_cmd) {
        if (strcmp(argv[0], "help") == 0) {
            uint32_t max_len = 0;
            // Make sure command list is nicely padded
            for (uint32_t i=0; i<num; i++) {
                if (cmds[i].help && max_len < strlen(cmds[i].cmd)) {
                    max_len = strlen(cmds[i].cmd);
                }
            }
            for (uint32_t i=0; i<num; i++) {
                if (cmds[i].help) {
                    printf("%-*s %s\n", (int) max_len+CMD_EXTRA_PADDING, cmds[i].cmd, cmds[i].help);
                }
            }
            printf("(end commands with semicolon, not enter).\n");
        } else {
            printf("Unknown command, try 'help'\n");
        }
    }
}
