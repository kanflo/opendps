/* 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2018 Johan Kanflo (github.com/kanflo)
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
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "dpsemul.h"
#include "flash.h"
#include "event.h"
#include "tft.h"
#include "dbg_printf.h"
#include "uframe.h"

#define UDP_RX_BUF_LEN       (512)
#define DPS_PORT            (5005)
#define EVENT_PORT          (5006)

/** Handles to the comms thread and event thread */
pthread_t udp_th, event_th;

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
void dps_emul_send_frame(frame_t *frame)
{
    int slen = sizeof(comm_client_sock);

    printf("[Com] Transmitted %u bytes\n", frame->length);
    for (uint32_t i = 0; i < frame->length; ++i)
         printf(" 0x%02X\n", frame->buffer[i]);

    if (sendto(comm_sock, frame->buffer, frame->length, 0, (struct sockaddr*) &comm_client_sock, slen) == -1) {
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
        if ((recv_len = recvfrom(comm_sock, buf, UDP_RX_BUF_LEN, 0, (struct sockaddr *) &comm_client_sock, &slen)) == -1) {
            printf("Error: recvfrom()\n");
        }
        printf("[Com] Received %lu bytes\n", recv_len);
        for (int i = 0; i < recv_len; i++) {
            if (!event_put(event_uart_rx, buf[i])) {
                dbg_printf("Error: event queue overflowed\n");
            }
        }
    }
    
    //close(sock);
    return NULL;
}

/**
 * @brief      This is the UDP communications thread
 *
 * @param[in]  arg   thread arguments
 */
void* event_thread(void *arg)
{
    printf("Event thread listening on UDP port %d\n", EVENT_PORT);
    struct sockaddr_in si_me;
    size_t recv_len;
    char buf[UDP_RX_BUF_LEN];
    struct sockaddr_in client_sock;
    socklen_t slen = sizeof(client_sock);
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("Error: socket\n");
        exit(EXIT_FAILURE);
    }
    
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(EVENT_PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sock, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        printf("Error: could not bind to port %d\n", DPS_PORT);
    }
    
    while(1) {
        if ((recv_len = recvfrom(sock, buf, UDP_RX_BUF_LEN, 0, (struct sockaddr *) &client_sock, &slen)) == -1) {
            printf("Error: recvfrom()\n");
        }
        if (buf[recv_len-1] == '\n') {
            buf[recv_len-1] = 0;
            recv_len--;
        }
        printf("[Evt] Received %lu bytes from %s:%d [%s]\n", recv_len, inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port), buf);
        if (strcmp("draw", buf) == 0) {
            printf("Drawing UI\n");
            emul_tft_draw();
            printf("---\n");
        }
    }
    
    //close(sock);
    return NULL;
}

/**
 * @brief      Emulator init
 *
 * @param[in]  past       pointer to the past
 * @param[in]  argc argv  program argument
 */
void dps_emul_init(past_t *past, int argc, char const *argv[])
{
	printf("OpenDPS Emulator\n");

    pthread_create(&udp_th, NULL, comm_thread, "UDP comms thread");
    pthread_create(&event_th, NULL, event_thread, "UDP event thread");

    size_t optind;
    char *file_name = 0;
    bool write_past = false;
    for (optind = 1; optind < argc; optind++) {
        switch (argv[optind][1]) {
	        case 'p':
	        	file_name = (char*) argv[optind+1];
	        	optind++;
	        	break;
	        case 'w':
			    write_past = true;
	        	break;
	        default:
	            fprintf(stderr, "Usage: %s [-p past.bin] [-w]\n", argv[0]);
	            exit(EXIT_FAILURE);
        }   
    }   

	flash_emul_init(past, file_name, write_past);
}
