/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Johan Kanflo (github.com/kanflo)
 *
 * Aaron Keith Socket code put into it's own unit. Added TCP option
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MINGW32__
  #if !(__has_include(<pthread.h>))
    #error MinGW users. Please install Pthreads-w32 from www.sourceware.org/pthreads-win32/
  #endif
#endif
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#ifdef __MINGW32__
  #include <Winsock2.h>  //MinGW needs to use Winsocks
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
#endif

#include "esp8266_emu.h"
#include "event.h"
#include "dbg_printf.h"

#define UDP_RX_BUF_LEN       (512)
#define DPS_PORT            (5005)
#define DPS_TCP_PORT        (5025)

/** Handles to the comms thread thread */
pthread_t udp_th;
pthread_t tcp_th;

/** UDP socket handle */
int comm_sock;

/** Current connected client, one at a time please */
struct sockaddr_in comm_client_sock;



/**
 * @brief      Send a frame on the emulator 'USART' which is the UDP port.
 *             Called from protocol_handler.c
 *
 * @param      frame   The frame
 * @param[in]  length  The length
 */
void dps_emul_send_frame(uint8_t *frame, uint32_t length)
{
    int slen = sizeof(comm_client_sock);
    if (sendto(comm_sock, (const char*)frame, length, 0, (struct sockaddr*) &comm_client_sock, slen) == -1) {
        printf("Error: sendto()\n");
    }
}


/**
 * @brief      This is the UDP communications thread
 *
 * @param[in]  arg   thread arguments
 */
void* comm_thread(void *arg)
{

    (void) arg;

    printf("Comms thread listening on UDP port %d\n", DPS_PORT);
    struct sockaddr_in si_me;
    size_t recv_len;
    char buf[UDP_RX_BUF_LEN];
    socklen_t slen = sizeof(comm_client_sock);

    if ((comm_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("Error: socket\n");
        exit(EXIT_FAILURE);
    }

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(DPS_PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(comm_sock, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        printf("Error: could not bind to port %d\n", DPS_PORT);
    }

    while(1) {

      recv_len = recvfrom(comm_sock, buf, UDP_RX_BUF_LEN, 0, (struct sockaddr *) &comm_client_sock, &slen);

        if ((signed)recv_len == -1) {
            printf("Error: recvfrom()\n");
        }
        printf("[Com] Received %lu bytes\n", recv_len);
        for (uint32_t i = 0; i < recv_len; i++) {
            if (!event_put(event_uart_rx, buf[i])) {
                dbg_printf("Error: event queue overflowed\n");
            }
        }
    }

    //close(sock);
    return NULL;
}





/**
 * @brief      This is the TCP communications thread
 *
 * @param[in]  arg   thread arguments
 */
void* comm_tcp_thread(void *arg)
{
  (void) arg;

    printf("Comms thread listening on TCP port %d\n", DPS_TCP_PORT);
    int server_sock = 0;
    struct sockaddr_in si_me;
    int recv_len;
    char buf[UDP_RX_BUF_LEN];

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error: socket\n");
        exit(EXIT_FAILURE);
    }

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(DPS_TCP_PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(server_sock, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        printf("Error: could not bind to port %d\n", DPS_TCP_PORT);
    }

    listen(server_sock, 10);

    while(1)
    {
      comm_sock = accept(server_sock, (struct sockaddr*)NULL, NULL);
      if (comm_sock == -1){
              printf("Error: with accept\n");
          }
      //printf("Accepting client\n");
      while(1) {
              recv_len = recv(comm_sock, buf, UDP_RX_BUF_LEN, 0);

              if (recv_len  == -1) {
                  printf("Error: recv()\n");
                  break;
              }

              if (recv_len == 0){
               //printf("Connection has been gracefully closed\n");
                break;
              }

              printf("[Com] Received %lu bytes\n", recv_len);
              for (int i = 0; i < recv_len; i++) {
                  if (!event_put(event_uart_rx, buf[i])) {
                      dbg_printf("Error: event queue overflowed\n");
                  }
              }
          }

        close(comm_sock);
     }
    return NULL;
}


/**
 * @brief    Creates socket UDP or TCP and emulates what the esp8266 would do in the opendps device.
 *           Called from protocol_handler.c
 *
 */
void Init_ESP8266_emu(void)
{

#ifdef __MINGW32__

  setbuf(stdout, NULL); //Fix for No Console Output in Eclipse with CDT in Windows

  WSADATA wsaData;
  int iResult;

  //MinGW needs to use Winsocks and they need to be initialized
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("Error! WSAStartup failed: %d\n", iResult);
    }

#endif


// pthread_create(&udp_th, NULL, comm_thread,(void *) "UDP comms thread");
   pthread_create(&tcp_th, NULL, comm_tcp_thread, "TCP comms thread");

}
