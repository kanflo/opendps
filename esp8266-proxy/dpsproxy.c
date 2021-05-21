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

#include <esp8266.h>
#include <espressif/esp_common.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <ota-tftp.h>
#include <rboot-api.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
#include <lwip/tcp.h>
#include <lwip/ip_addr.h>
#include <lwip/api.h>
#include <lwip/netbuf.h>
#include <lwip/igmp.h>
#include <ssid_config.h>
#include <espressif/esp_wifi.h>
#include <stdin_uart_interrupt.h>
#include "lwipopts.h"
#include "uhej.h"
#include "protocol.h"
#include "uframe.h"
#include "hexdump.h"

/** User friendly FreeRTOS delay macro */
#define delay_ms(ms) vTaskDelay(ms / portTICK_PERIOD_MS)
#define systime_ms() (xTaskGetTickCount() * portTICK_PERIOD_MS)

/** Semaphore to signal wifi availability */
static SemaphoreHandle_t wifi_alive_sem;

/** A queue to synchronize UART comms */
static QueueHandle_t tx_queue;

#define TX_QUEUE_DEPTH  (5)

#define UART_RX_TIMEOUT_MS  (250)

/** A structure used in the tx_queue */
typedef struct {
    /** if client_port != 0, send the respnse frame to client_addr:client_port
        otherwise, deal with it locally */
    struct udp_pcb *upcb;
    ip_addr_t client_addr;
    uint16_t client_port;
    frame_t frame;
} tx_item_t;


/**
  * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
  * @param arg user supplied argument (udp_pcb.recv_arg)
  * @param pcb the udp_pcb which received data
  * @param p the packet buffer that was received
  * @param addr the remote IP address from which the packet was received
  * @param port the remote port from which the packet was received
  * @retval None
  */
//static struct udp_pcb* mcast_join_group(char *group_ip, uint16_t group_port, void (* recv)(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port))
static void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p) {
        tx_item_t item;
        memcpy((void*) &item.client_addr, (void*) addr, sizeof(ip_addr_t));
        item.upcb = upcb;
        item.client_port = port;
        memcpy((void*) item.frame.buffer, (void*) p->payload, p->len);
        item.frame.length = p->len;
        if (pdPASS != xQueueSend(tx_queue, (void*) &item, 1000/portTICK_PERIOD_MS)) {
            printf("Failed to enqueue\n");
            /** @todo Handle queue error */
        }
        pbuf_free(p);
    }
}

/**
  * @brief Send buffer on UART 0
  * @param buffer buffer to send
  * @param length length of buffer
  * @retval None
  */
static void uart_tx(uint8_t *buffer, uint32_t length)
{
    do {
        uart_putc(0, *buffer);
        buffer++;
    } while(length--);
}

/**
  * @brief Receive a frame (SOF..EOF) on UART 0
  * @param buffer buffer to store frame
  * @param length length of buffer
  * @retval length of frame received
  */
static uint32_t uart_rx_frame(uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t size = 0;
    uint8_t ch;
    bool sof = false;
    bool timeout = false;
    uint32_t start_time = systime_ms();
    while(size < buffer_size && !timeout) {
        if (uart0_num_char() > 0) {
            if (read(0, (void*) &ch, 1)) { // 0 is stdin
                if (ch == _SOF) {
                    size = 0;
                    sof = true;
                }
                if (sof) {
                    buffer[size++] = ch;
                }
                if (ch == _EOF) {
                    sof = false;
                    break;
                }
            }
        } else {
            delay_ms(5);
        }
        timeout = systime_ms() - start_time >= UART_RX_TIMEOUT_MS;
    }
    if (timeout) {
        size = 0;
    }
    return size;
}

/**
  * @brief Set wifi status in the DPS
  * @param status guess :)
  * @retval None
  */
void set_dps_wifi_status(wifi_status_t status)
{
    tx_item_t item;
    item.client_port = 0; // Don't transmit to any client
    protocol_create_wifi_status(&item.frame, status);
    if (pdPASS != xQueueSend(tx_queue, (void*) &item, 1000/portTICK_PERIOD_MS)) {
        printf("failed to enqueue %d\n", status);
        /** @todo: handle error */
    }
}

/**
  * @brief This is the DPS server task
  * @param arg user supplied argument from xTaskCreate
  * @retval None
  */
static void uhej_task(void *arg)
{
    err_t err;
    bool success = false;
    struct udp_pcb *upcb;

    xSemaphoreTake(wifi_alive_sem, portMAX_DELAY);
    xSemaphoreGive(wifi_alive_sem);

    (void) uhej_server_init();
    (void) uhej_announce_udp("tftp", 69);
    (void) uhej_announce_udp("opendps", 5005);

    sys_lock_tcpip_core();
    do {
        upcb = udp_new();
        if (!upcb) {
            printf("Failed to create UDP context\n");
            break;
        }
        err = udp_bind(upcb, IP_ADDR_ANY, 5005);
        if(ERR_OK != err) {
            printf("Failed to join multicast group: %d\n", err);
            break;
        }
        udp_recv(upcb, udp_receive_callback, upcb);
        success = true;
    } while(0);
    sys_unlock_tcpip_core();

    if (!success) {
        /** @todo: handle failure */
    }

    while(1) {
        delay_ms(10000);
    }
}

/**
  * @brief This is the task that communicates with the DPS
  * @param arg user supplied argument from xTaskCreate
  * @retval None
  */
static void uart_comm_task(void *arg)
{
    tx_item_t item;
    while(1) {
        if (pdPASS != xQueueReceive(tx_queue, (void*) &item, portMAX_DELAY)) {
            printf("xQueueReceive failed\n");
            delay_ms(1000);
        } else {
            uint8_t buffer[MAX_FRAME_LENGTH];
            uint32_t size;
            uart_tx((uint8_t*) &item.frame, item.frame.length);

            size = uart_rx_frame((uint8_t*) &buffer, sizeof(buffer));
            if (size > 0) {
                if (item.client_port > 0) {
                    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
                    if (!p) {
                        printf("Failed to allocate transport buffer\n");
                    } else {
                        memcpy(p->payload, buffer, size);
                        err_t err = udp_sendto(item.upcb, p, &item.client_addr, item.client_port);
                        if (err < 0) {
                            printf("Error sending message: %s (%d)\n", lwip_strerr(err), err);
                        }
                        pbuf_free(p);
                    }
                }
            } else {
                printf("Timeout from DPS\n");
                /** @todo: handle timeout */
            }
        }
    }
}

/**
  * @brief This is the wifi connection task
  * @param arg user supplied argument from xTaskCreate
  * @retval None
  */
static void wifi_task(void *pvParameters)
{
    (void) pvParameters;
    uint8_t status = 0;
    uint8_t retries = 30;
    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    // As the uart_rx_frame currently has no timeout and libnet80211.a spams at
    // boot there is a risk of the DPS missing the wifi status update causing
    // the uart_comm_task task to lock up and the DPS is now uncontrollable
    // via wifi which kind of was the idea...

    //set_dps_wifi_status(wifi_connecting); // Do this early as there will be lots of spam when wifi connection begins
    xSemaphoreTake(wifi_alive_sem, portMAX_DELAY);
    //printf("WiFi: connecting to WiFi\n"); /** Don't use too much output as it will be seen by the DPS device */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    while(1) {
        while (status != STATION_GOT_IP && retries) {
            status = sdk_wifi_station_get_connect_status();
            if(status == STATION_WRONG_PASSWORD) {
                set_dps_wifi_status(wifi_error);
                //printf("WiFi: wrong password\n");
                break;
            } else if(status == STATION_NO_AP_FOUND) {
                set_dps_wifi_status(wifi_error);
                //printf("WiFi: AP not found\n");
                break;
            } else if(status == STATION_CONNECT_FAIL) {
                set_dps_wifi_status(wifi_error);
                //printf("WiFi: connection failed\n");
                break;
            }
            delay_ms(1000);
            retries--;
        }
        if (status == STATION_GOT_IP) {
            //printf("WiFi: connected\n");
            xSemaphoreGive(wifi_alive_sem);
            set_dps_wifi_status(wifi_connected);
            taskYIELD();
        }

        while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP) {
            delay_ms(1000);
            xSemaphoreGive(wifi_alive_sem);
            taskYIELD();
        }
        //printf("WiFi: disconnected\n");
        sdk_wifi_station_disconnect();
        set_dps_wifi_status(wifi_connecting);
        delay_ms(1000);
    }
}

void user_init(void)
{
    uart_set_baud(0, CONFIG_BAUDRATE);  /** Baudrate set in makefile */
    uart_clear_txfifo(0);
    vSemaphoreCreateBinary(wifi_alive_sem);
    tx_queue = xQueueCreate(TX_QUEUE_DEPTH, sizeof(tx_item_t));
    ota_tftp_init_server(TFTP_PORT);
    xTaskCreate(&uart_comm_task, "uart_comm_task", 2048, NULL, 4, NULL);
    xTaskCreate(&wifi_task, "wifi_task",  256, NULL, 2, NULL);
    xTaskCreate(&uhej_task, "uhej_task",  256, NULL, 3, NULL);
}
