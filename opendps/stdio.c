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

#include <stdio.h>
#include <errno.h>
#include <usart.h>

int _write(int file, char *ptr, int len);

/**
  * @brief Called by stdlib when doing printf
  * @param file file descriptor (1 for stdout)
  * @param ptr pointer to string to output
  * @param len length of string
  * @retval number of characters output or -1 in case of errors
  */
int _write(int file, char *ptr, int len)
{
    int i;
    if (file == 1) {
        for (i = 0; i < len; i++) {
            usart_send_blocking(USART1, ptr[i]);
        }
        return i;
    }
    errno = EIO;
    return -1;
}

