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
#include <stdlib.h>
#include <string.h>

#ifndef _NOESP
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
#include <lwip/tcp.h>
#include <lwip/ip_addr.h>
#include <lwip/api.h>
#include <lwip/netbuf.h>
#include <lwip/igmp.h>
#include <esp/hwrand.h>
#include <esp8266.h>
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <espressif/esp_wifi.h>
#include "uhej.h"
#include "hexdump.h"
#include "packer.h"

//#define UHEJ_HELLO_TASK

/** User friendly FreeRTOS delay macro */
#define delay_ms(ms) vTaskDelay(ms / portTICK_PERIOD_MS)

#else // _NOESP
 #define ip_addr_t uint32_t
#endif // _NOESP

#ifdef CONFIFG_HEJ_DEBUG
 #define _DBG(x) x
#else
 #define _DBG(x)
#endif // CONFIFG_HEJ_DEBUG

/** Multicast group information */
#define UHEJ_MCAST_GRP  ("225.0.1.250")
#define UHEJ_MCAST_PORT (4242)

/** uHej frame magic */
#define UHEJ_MAGIC (0xfedebeda)

/** Frame types */
#define UHEJ_HELLO      (0)
#define UHEJ_ANNOUNCE   (1)
#define UHEJ_QUERY      (2)
#define UHEJ_BEACON     (3)

/** Annnounce frame size */
#define MAX_ANNOUNCE_FRAME_SIZE   (128)

/** Service types */
typedef enum {
    UHEJ_UDP = 0,
    UHEJ_TCP,
    UHEJ_MCAST,
    UHEJ_NUM_SERVICES
} uhej_service_t;

char *g_service_names[] = { "UDP", "TCP", "MCAST" }; /** Must match uhej_service_t */

#define MAX_SERVICES      (8) /** Number of services we provide */
#define MAX_SERVICE_NAME (16) /** Max service name length */
//#define MAX_SUBSCRIBERS   (8) /** Max number of subscribers  @todo When the subscription model is implemented */

/** The service descriptor */
typedef struct {
    char name[MAX_SERVICE_NAME];
    uhej_service_t type;
    uint16_t port;
    ip_addr_t ipgroup; /** Used for multicast services */
//    ip_addr_t subscribers[MAX_SUBSCRIBERS]; /** @todo  When the subscription model is implemented */
} uhej_service_desc_t;

/** The list of services provided */
static uhej_service_desc_t g_services[MAX_SERVICES];


/**
  * @brief Find a free service descriptor
  * @retval pointer to service descriptor or NULL if all are full
  */
static uhej_service_desc_t* find_free_service(void)
{
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (!g_services[i].name[0]) {
            return &g_services[i];
        }
    }
    printf("Error: found no free service\n");
    return NULL;
}

/**
  * @brief Find service
  * @param name name of service
  * @param type type of service
  * @retval pointer to service descriptor or NULL if not found
  */
static uhej_service_desc_t* find_service(char *name, uhej_service_t type)
{
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (strncmp(g_services[i].name, name, MAX_SERVICE_NAME) == 0 && g_services[i].type == type) {
            return &g_services[i];
        }
    }
    return NULL;
}

/**
  * @brief Dump all services on stdout
  * @retval none
  */
#ifdef _NOESP
static void dump_services(void)
{
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (g_services[i].name[0]) {
            printf("%d %s (%d)\n", i, g_services[i].name, g_services[i].port);
        } else {
            printf("%d [empty]\n", i);
        }
    }    
}
#endif //  _NOESP

/**
  * @brief Remove specified service from list
  * @param name name of service
  * @param type type of service
  * @retval true if removal was successful
  */
static bool cancel_service(char *name, uhej_service_t type)
{
    uhej_service_desc_t *s = find_service(name, type);
    if (!s) {
        printf("Error, service '%s' [%d] not found\n", name, type);
        return false;
    } else {
        memset(s, 0, sizeof(uhej_service_desc_t));
        return true;
    }
}

/**
  * @brief This function joins a multicast group witht he specified ip/port
  * @param group_ip the specified multicast group ip
  * @param group_port the specified multicast port number
  * @param recv the lwip UDP callback
  * @retval udp_pcb* or NULL if joining failed
  */
static struct udp_pcb* mcast_join_group(char *group_ip, uint16_t group_port, void (* recv)(void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port))
{
    bool status = false;
    struct udp_pcb *upcb;

    _DBG(printf("Joining mcast group %s:%d\n", group_ip, group_port);)
    do {
        upcb = udp_new();
        if (!upcb) {
            printf("Error, udp_new failed");
            break;
        }
        udp_bind(upcb, IP_ADDR_ANY, group_port);
        struct netif* netif = sdk_system_get_netif(STATION_IF);
        if (!netif) {
            printf("Error, netif is null");
            break;
        }
        if (!(netif->flags & NETIF_FLAG_IGMP)) {
            netif->flags |= NETIF_FLAG_IGMP;
            igmp_start(netif);
        }
        ip_addr_t ipgroup;
        ipaddr_aton(group_ip, &ipgroup);
        err_t err = igmp_joingroup(&netif->ip_addr, &ipgroup);
        if(ERR_OK != err) {
            printf("Failed to join multicast group: %d", err);
            break;
        }
        status = true;
    } while(0);

    if (status) {
        _DBG(printf("Join successs\n");)
        udp_recv(upcb, recv, upcb);
    } else {
        if (upcb) {
            udp_remove(upcb);
        }
        upcb = NULL;   
    }
    return upcb;
}

/**
  * @brief Decode a hello frame
  * @param buf pointer to frame
  * @param len length of frame
  * @retval None
  */
static void decode_hello_frame(uint8_t *buf, uint32_t len)
{
    _DBG(printf("Hello frame\n");)
    _DBG(hexdump(buf, len);)
}

/**
  * @brief Decode a announce frame
  * @param buf pointer to frame
  * @param len length of frame
  * @retval None
  */
static void decode_announce_frame(uint8_t *buf, uint32_t len)
{
    _DBG(printf("Announce frame\n");)
    _DBG(hexdump(buf, len);)
}

/**
  * @brief Decode a query frame
  * @param buf pointer to frame
  * @param len length of frame
  * @param upcb open socket to multicast group
  * @retval None
  */
static void decode_query_frame(uint8_t *buf, uint32_t len, struct udp_pcb *upcb)
{
    uhej_service_desc_t *d = NULL;
    char name[MAX_SERVICE_NAME];
    uint8_t type;
    bool wildcard = strcmp(name, "*") == 0;
    _DBG(hexdump(buf, len);)
    UNPACK_START(buf, len);
    UNPACK_UINT8(type);
    UNPACK_CSTR(name, MAX_SERVICE_NAME);
    UNPACK_END();
UNPACK_ERROR_HANDLER:
    printf("Error, query frame is corrupt\n");
    UNPACK_DONE();
UNPACK_SUCCESS_HANDLER:
    UNPACK_DONE();
UNPACK_DONE_HANDLER:
    d = find_service(name, type); /** @todo Handle wildcard query */
    if (d || wildcard) {
        if (d) {
            _DBG(printf("Responding to query for %s service '%s'\n", g_service_names[type], name);)
        } else {
            _DBG(printf("Responding to wildcard query \n");)
        }
        uint8_t *buffer = (uint8_t*) malloc(MAX_ANNOUNCE_FRAME_SIZE);
        struct pbuf *p;
        ip_addr_t ipgroup;
        if (!buffer) {
            printf("Error, failed to allocate announce frame buffer\n");
            return;
        }
        PACK_START(buffer, MAX_ANNOUNCE_FRAME_SIZE);
        PACK_UINT32(UHEJ_MAGIC);
        PACK_UINT8(UHEJ_ANNOUNCE);
        if (d) {
            PACK_UINT8(d->type);
            PACK_UINT16(d->port);
            PACK_CSTR(d->name);
        } else {
            // Wildcard
            for (int i = 0; i < MAX_SERVICES; i++) {
                if (g_services[i].name[0]) {
                    PACK_UINT8(g_services[i].type);
                    PACK_UINT16(g_services[i].port);
                    PACK_CSTR(g_services[i].name);
                }
            }
        }
        PACK_END(len);
    PACK_ERROR_HANDLER:
        printf("Error, overflow while packing frame.\n");
        PACK_DONE();
    PACK_SUCCESS_HANDLER:
        _DBG(printf("Pack frame ok, %d bytes\n", len);)
        _DBG(hexdump(buffer, len);)
        PACK_DONE();
    PACK_DONE_HANDLER:
        p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
        ipaddr_aton(UHEJ_MCAST_GRP, &ipgroup);
        if (!p) {
            printf("Failed to allocate %d byte transport buffer\n", len);
        } else {
            memcpy(p->payload, buffer, len);
            /** @todo Do not send to mcast group but to requesting client only */
            err_t err = udp_sendto(upcb, p, &ipgroup, UHEJ_MCAST_PORT);
            if (err < 0) {
                printf("Error sending message: %s (%d)\n", lwip_strerr(err), err);
            }
            pbuf_free(p);
        }
    }
}

/**
  * @brief Decode a beacon frame
  * @param buf pointer to frame
  * @param len length of frame
  * @retval None
  */
static void decode_beacon_frame(uint8_t *buf, uint32_t len)
{
    _DBG(printf("Beacon frame\n");)
    _DBG(hexdump(buf, len);)
}

/**
  * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
  * @param arg user supplied argument (udp_pcb.recv_arg)
  * @param pcb the udp_pcb which received data
  * @param p the packet buffer that was received
  * @param addr the remote IP address from which the packet was received
  * @param port the remote port from which the packet was received
  * @retval None
  */
static void uhej_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    if (p) {
        uint32_t magic;
        uint8_t type;
        _DBG(printf("Msg received port:%d len:%d\n", port, p->len);)
        uint8_t *buf = (uint8_t*) p->payload;
        UNPACK_START(buf, p->len);
        UNPACK_UINT32(magic);
        if (magic != UHEJ_MAGIC) {
            printf("Error, unknown magic 0x%08x\n", magic);
        } else {
            UNPACK_UINT8(type);
            _DBG(hexdump(buf, 5);)
            switch(type) {
                case UHEJ_HELLO:
                    decode_hello_frame(&(buf[5]), p->len-5);
                    break;
                case UHEJ_ANNOUNCE:
                    decode_announce_frame(&(buf[5]), p->len-5);
                    break;
                case UHEJ_QUERY:
                    decode_query_frame(&(buf[5]), p->len-5, upcb);
                    break;
                case UHEJ_BEACON:
                    decode_beacon_frame(&(buf[5]), p->len-5);
                    break;
                default:
                    printf("Error, unknown frame type %d\n", type);
                    _DBG(hexdump(buf, p->len);)
            }
        }
        UNPACK_END();
    UNPACK_ERROR_HANDLER:
        printf("UDP cb: unpack frame error\n");
        UNPACK_DONE();
    UNPACK_SUCCESS_HANDLER:
        _DBG(printf("Unpack frame ok\n");)
        UNPACK_DONE();
    UNPACK_DONE_HANDLER:
        pbuf_free(p);
    }
}

/**
  * @brief This is the multicast task
  * @param arg user supplied argument from xTaskCreate
  * @retval None
  */
static void uhej_task(void *arg)
{
    ip_addr_t ipgroup;
    ipaddr_aton(UHEJ_MCAST_GRP, &ipgroup);
    struct udp_pcb *upcb = mcast_join_group(UHEJ_MCAST_GRP, UHEJ_MCAST_PORT, uhej_receive_callback);

    while(1) {
#ifndef UHEJ_HELLO_TASK
        (void) upcb;
#else // UHEJ_HELLO_TASK
        struct pbuf *p;
        uint32_t message_count = 0;
        char msg[64];
        snprintf((char*)msg, sizeof(msg), "Hello #%u", message_count++);
        p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg)+1, PBUF_RAM);
        if (!p) {
            printf("Failed to allocate transport buffer\n");
        } else {
            memcpy(p->payload, msg, strlen(msg)+1);
            err_t err = udp_sendto(upcb, p, &ipgroup, UHEJ_MCAST_PORT);
            if (err < 0) {
                printf("Error sending message: %s (%d)\n", lwip_strerr(err), err);
            } else {
                printf("Sent message '%s'\n", msg);
            }
            pbuf_free(p);
        }
#endif // UHEJ_HELLO_TASK
        delay_ms(2000);
    }
}

/** @brief Required by LWIP for multicast 
  * @retval 32 bit random number
  */
uint32_t my_rand(void)
{
    return hwrand();
}

/**
  * @brief Initialize the uHej server
  * @retval true if init was succcessful
  */
bool uhej_server_init(void)
{
    memset(g_services, 0, sizeof(g_services));
    xTaskCreate(&uhej_task, "uhej_task", 1024, NULL, 4, NULL);
    return true;
}

/**
  * @brief Announce UDP service with given name and port
  * @param name name of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_udp(char *name, uint16_t port)
{
    uhej_service_desc_t *s = find_free_service();
    if (!s) {
        printf("Error, all services are occupied.\n");
        return false;
    } else {
        strncpy(s->name, name, MAX_SERVICE_NAME);
        s->port = port;
        s->type = UHEJ_UDP;
        return true;
    }
}

/**
  * @brief Announce TCP service with given name and port
  * @param name name of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_tcp(char *name, uint16_t port)
{
    return false; /** @todo Implement TCP support */
}

/**
  * @brief Announce multiceast UDP service with given name, ip and port
  * @param name name of service
  * @param ip multicast ip of service
  * @param port port of service
  * @retval true if succcessful
  */
bool uhej_announce_mcast(char *name, char *ip, uint16_t port)
{
    return false; /** @todo Implement multicast support */
}

/**
  * @brief Remove announcement of UDP service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_udp(char *name)
{
    return cancel_service(name, UHEJ_UDP);
}

/**
  * @brief Remove announcement of TCP service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_tcp(char *name)
{
    return cancel_service(name, UHEJ_TCP);
}

/**
  * @brief Remove announcement of UDP multicast service 
  * @param name name of service
  * @retval true if succcessful
  */
bool uhej_cancel_mcast(char *name)
{
    return cancel_service(name, UHEJ_MCAST);
}

#ifdef _NOESP
int main(int argc, char const *argv[])
{
    uhej_server_init();
    printf("init\n");
    uhej_announce_udp("tftp", 69);
    uhej_announce_udp("test service", 5000);
    dump_services();

    uhej_service_desc_t *s;
    s = find_service("tftp", UHEJ_UDP);
    s = find_service("test service", UHEJ_UDP);
    s = find_service("rusk", UHEJ_UDP);
    s = find_service("tftp", UHEJ_TCP);

    uhej_cancel_udp("tftp");
    uhej_cancel_udp("test service");
    dump_services();

    return 0;
}
#endif //  _NOESP
