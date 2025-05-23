/*
 * coap_io.h -- Default network I/O functions for libcoap
 *
 * Copyright (C) 2012-2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#ifndef COAP_IO_H_
#define COAP_IO_H_

//#include <sys/types.h>

#include "address.h"

#ifndef COAP_RXBUFFER_SIZE
#define COAP_RXBUFFER_SIZE 1472
#endif /* COAP_RXBUFFER_SIZE */

/*
 * It may may make sense to define this larger on busy systems
 * (lots of sessions, large number of which are active), by using
 * -DCOAP_MAX_EPOLL_EVENTS=nn at compile time.
 */
#ifndef COAP_MAX_EPOLL_EVENTS
#define COAP_MAX_EPOLL_EVENTS 10
#endif /* COAP_MAX_EPOLL_EVENTS */

#ifdef _WIN32
typedef SOCKET coap_fd_t;
#define coap_closesocket closesocket
#define COAP_SOCKET_ERROR SOCKET_ERROR
#define COAP_INVALID_SOCKET INVALID_SOCKET
#else
typedef int coap_fd_t;
#define coap_closesocket close
#define COAP_SOCKET_ERROR (-1)
#define COAP_INVALID_SOCKET (-1)
#endif

struct libcoap_coap_packet_t;
struct coap_session_t;
struct coap_context_t;
struct coap_pdu_t;

typedef uint16_t coap_socket_flags_t;

typedef struct coap_addr_tuple_t {
  coap_address_t remote;       /**< remote address and port */
  coap_address_t local;        /**< local address and port */
} coap_addr_tuple_t;

typedef struct coap_socket_t {
#if defined(WITH_LWIP)
  struct udp_pcb *pcb;
#elif defined(WITH_CONTIKI)
  void *conn;
#else
  coap_fd_t fd;
#endif /* WITH_LWIP */
  coap_socket_flags_t flags;
  struct coap_session_t *session; /* Used by the epoll logic for an active session.
                                     Note: It must mot be wrapped with COAP_EPOLL_SUPPORT as
                                     coap_socket_t is seen in applications embedded in
                                     coap_session_t etc. */
  struct coap_endpoint_t *endpoint; /* Used by the epoll logic for a listening endpoint.
                                       Note: It must mot be wrapped with COAP_EPOLL_SUPPORT as
                                       coap_socket_t is seen in applications embedded in
                                       coap_session_t etc. */
} coap_socket_t;

/**
 * coap_socket_flags_t values
 */
#define COAP_SOCKET_EMPTY        0x0000  /**< the socket is not used */
#define COAP_SOCKET_NOT_EMPTY    0x0001  /**< the socket is not empty */
#define COAP_SOCKET_BOUND        0x0002  /**< the socket is bound */
#define COAP_SOCKET_CONNECTED    0x0004  /**< the socket is connected */
#define COAP_SOCKET_WANT_READ    0x0010  /**< non blocking socket is waiting for reading */
#define COAP_SOCKET_WANT_WRITE   0x0020  /**< non blocking socket is waiting for writing */
#define COAP_SOCKET_WANT_ACCEPT  0x0040  /**< non blocking server socket is waiting for accept */
#define COAP_SOCKET_WANT_CONNECT 0x0080  /**< non blocking client socket is waiting for connect */
#define COAP_SOCKET_CAN_READ     0x0100  /**< non blocking socket can now read without blocking */
#define COAP_SOCKET_CAN_WRITE    0x0200  /**< non blocking socket can now write without blocking */
#define COAP_SOCKET_CAN_ACCEPT   0x0400  /**< non blocking server socket can now accept without blocking */
#define COAP_SOCKET_CAN_CONNECT  0x0800  /**< non blocking client socket can now connect without blocking */
#define COAP_SOCKET_MULTICAST    0x1000  /**< socket is used for multicast communication */

struct coap_endpoint_t *coap_malloc_endpoint( void );
void coap_mfree_endpoint( struct coap_endpoint_t *ep );

int
coap_socket_connect_udp(coap_socket_t *sock,
                        const coap_address_t *local_if,
                        const coap_address_t *server,
                        int default_port,
                        coap_address_t *local_addr,
                        coap_address_t *remote_addr);

int
coap_socket_bind_udp(coap_socket_t *sock,
                     const coap_address_t *listen_addr,
                     coap_address_t *bound_addr );

int
coap_socket_connect_tcp1(coap_socket_t *sock,
                         const coap_address_t *local_if,
                         const coap_address_t *server,
                         int default_port,
                         coap_address_t *local_addr,
                         coap_address_t *remote_addr);

int
coap_socket_connect_tcp2(coap_socket_t *sock,
                         coap_address_t *local_addr,
                         coap_address_t *remote_addr);

int
coap_socket_bind_tcp(coap_socket_t *sock,
                     const coap_address_t *listen_addr,
                     coap_address_t *bound_addr);

int
coap_socket_accept_tcp(coap_socket_t *server,
                       coap_socket_t *new_client,
                       coap_address_t *local_addr,
                       coap_address_t *remote_addr);

void coap_socket_close(coap_socket_t *sock);

int
coap_socket_send( coap_socket_t *sock, struct coap_session_t *session,
                  const uint8_t *data, size_t data_len );

int
coap_socket_write(coap_socket_t *sock, const uint8_t *data, size_t data_len);

int
coap_socket_read(coap_socket_t *sock, uint8_t *data, size_t data_len);

void
coap_epoll_ctl_mod(coap_socket_t *sock, uint32_t events, const char *func);

#ifdef WITH_LWIP
int
coap_socket_send_pdu( coap_socket_t *sock, struct coap_session_t *session,
                      struct coap_pdu_t *pdu );
#endif

const char *coap_socket_strerror( void );

/**
 * Function interface for data transmission. This function returns the number of
 * bytes that have been transmitted, or a value less than zero on error.
 *
 * @param sock             Socket to send data with
 * @param session          Addressing information for unconnected sockets, or NULL
 * @param data             The data to send.
 * @param datalen          The actual length of @p data.
 *
 * @return                 The number of bytes written on success, or a value
 *                         less than zero on error.
 */
int coap_network_send( coap_socket_t *sock, const struct coap_session_t *session, const uint8_t *data, size_t datalen );

/**
 * Function interface for reading data. This function returns the number of
 * bytes that have been read, or a value less than zero on error. In case of an
 * error, @p *packet is set to NULL.
 *
 * @param sock   Socket to read data from
 * @param packet Received packet metadata and payload. src and dst should be preset.
 *
 * @return       The number of bytes received on success, or a value less than
 *               zero on error.
 */
int coap_network_read( coap_socket_t *sock, struct libcoap_coap_packet_t *packet );

#ifndef coap_mcast_interface
# define coap_mcast_interface(Local) 0
#endif

/**
 * Given a packet, set msg and msg_len to an address and length of the packet's
 * data in memory.
 * */
void coap_packet_get_memmapped(struct libcoap_coap_packet_t *packet,
                               unsigned char **address,
                               size_t *length);

#ifdef WITH_LWIP
/**
 * Get the pbuf of a packet. The caller takes over responsibility for freeing
 * the pbuf.
 */
struct pbuf *coap_packet_extract_pbuf(struct libcoap_coap_packet_t *packet);
#endif

#if defined(WITH_LWIP)
/*
 * This is only included in coap_io.h instead of .c in order to be available for
 * sizeof in lwippools.h.
 * Simple carry-over of the incoming pbuf that is later turned into a node.
 *
 * Source address data is currently side-banded via ip_current_dest_addr & co
 * as the packets have limited lifetime anyway.
 */
struct libcoap_coap_packet_t {
  struct pbuf *pbuf;
  const struct coap_endpoint_t *local_interface;
  coap_addr_tuple_t addr_info; /**< local and remote addresses */
  int ifindex;                /**< the interface index */
//  uint16_t srcport;
};
#else
struct coap_packet_t {
  coap_addr_tuple_t addr_info; /**< local and remote addresses */
  int ifindex;                /**< the interface index */
  size_t length;              /**< length of payload */
  unsigned char payload[COAP_RXBUFFER_SIZE]; /**< payload */
};
#endif
typedef struct libcoap_coap_packet_t libcoap_coap_packet_t;

typedef enum {
  COAP_NACK_TOO_MANY_RETRIES,
  COAP_NACK_NOT_DELIVERABLE,
  COAP_NACK_RST,
  COAP_NACK_TLS_FAILED,
  COAP_NACK_ICMP_ISSUE
} coap_nack_reason_t;

#endif /* COAP_IO_H_ */
