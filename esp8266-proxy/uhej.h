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

/**
  * @brief Initialize the uHej server
  * @retval true if init was succcessful
  */
bool uhej_server_init(void);

/**
  * @brief Announce UDP service with given name and port
  * @param name name of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_udp(char *name, uint16_t port);

/**
  * @brief Announce TCP service with given name and port
  * @param name name of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_tcp(char *name, uint16_t port);

/**
  * @brief Announce multiceast UDP service with given name, ip and port
  * @param name name of service
  * @param ip multicast ip of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_mcast(char *name, char *ip, uint16_t port);

/**
  * @brief Remove announcement of UDP service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_udp(char *name);

/**
  * @brief Remove announcement of TCP service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_tcp(char *name);

/**
  * @brief Remove announcement of UDP multicast service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_mcast(char *name);
