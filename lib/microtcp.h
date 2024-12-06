/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_MICROTCP_H_
#define LIB_MICROTCP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <math.h>
 /*
  * Several useful constants
  */
#define MICROTCP_ACK_TIMEOUT_US 200000
#define MICROTCP_MSS 1400
#define MICROTCP_RECVBUF_LEN 8192
#define MICROTCP_WIN_SIZE MICROTCP_RECVBUF_LEN
#define MICROTCP_INIT_CWND (3 * MICROTCP_MSS)
#define MICROTCP_INIT_SSTHRESH MICROTCP_WIN_SIZE
#define MAX_PAYLOAD 508
#define data_offset future_use0
#define total_data_size future_use1
#define control_limit future_use2
#define MIN(x,y) x < y ? x : y
#define MIN3(x,y,z) x < MIN(y,z) ? x : MIN(y,z)

#define FREE(...) free_("", __VA_ARGS__, NULL)
// #define WINDOW_SIZE 66546
  /**
   * Possible states of the microTCP socket
   *
   * NOTE: You can insert any other possible state
   * for your own convenience
   */
typedef enum
{
    
    ESTABLISHED,
    /*Added states*/
    ESTABLISHED_PEER,   
    ESTABLISHED_HOST,
    UKNOWN,
    /*------------*/
    CLOSING_BY_PEER,
    CLOSING_BY_HOST,
    CLOSED,
    INVALID    
} microtcp_state_t;


/**
 * This is the microTCP socket structure. It holds all the necessary
 * information of each microTCP socket.
 *
 * NOTE: Fill free to insert additional fields.
 */
typedef struct
{
    int sd;                       /**< The underline UDP socket descriptor */
    microtcp_state_t state;       /**< The state of the microTCP socket */
    size_t init_win_size;         /**< The window size negotiated at the 3-way handshake */
    size_t curr_win_size;         /**< The current window size */
    uint8_t* recvbuf;             /**< The *receive* buffer of the TCP
                                       connection. It is allocated during the connection establishment and
                                       is freed at the shutdown of the connection. This buffer is used
                                       to retrieve the data from the network. */
    size_t buf_fill_level;        /**< Amount of data in the buffer */

    size_t cwnd;
    size_t ssthresh;

    uint32_t seq_number;            /**< Keep the state of the sequence number */
    uint32_t ack_number;            /**< Keep the state of the ack number */
    uint64_t packets_send;
    uint64_t packets_received;
    uint64_t packets_lost;
    uint64_t bytes_send;
    uint64_t bytes_received;
    uint64_t bytes_lost;
    uint64_t dup_ack;
} microtcp_sock_t;


/**
 * microTCP header structure
 * NOTE: DO NOT CHANGE!
 */
typedef struct
{
    uint32_t seq_number;          /**< Sequence number */
    uint32_t ack_number;          /**< ACK number */
    uint16_t control;             /**< Control bits (e.g. SYN, ACK, FIN) */
    uint16_t window;              /**< Window size in bytes */
    uint32_t data_len;            /**< Data length in bytes (EXCLUDING header) */
    uint32_t future_use0;         /**< 32-bits total_data_size */
    uint32_t future_use1;         /**< 32-bits data_offset*/
    uint32_t future_use2;         /**< 32-bits congestion_limit*/
    uint32_t checksum;            /**< CRC-32 checksum, see crc32() in utils folder */
} microtcp_header_t;

typedef struct {
  microtcp_state_t state;
  uint32_t seq_number;
  uint32_t ack_number;
} microtcp_socket_image;


typedef struct 
{
    microtcp_header_t header;
    uint8_t* payload;
    microtcp_header_t* next;

} microtcp_header_node;


microtcp_sock_t
microtcp_socket(int domain, int type, int protocol);

int
microtcp_bind(microtcp_sock_t* socket, const struct sockaddr* address,
    socklen_t address_len);

int
microtcp_connect(microtcp_sock_t* socket, const struct sockaddr* address,
    socklen_t address_len);

/**
 * Blocks waiting for a new connection from a remote peer.
 *
 * @param socket the socket structure
 * @param address pointer to store the address information of the connected peer
 * @param address_len the length of the address structure.
 * @return ATTENTION despite the original accept() this function returns
 * 0 on success or -1 on failure
 */
int
microtcp_accept(microtcp_sock_t* socket, struct sockaddr* address,
    socklen_t address_len);

ssize_t
microtcp_send(microtcp_sock_t* socket, const void* buffer, size_t length,
    int flags);

ssize_t
microtcp_recv(microtcp_sock_t* socket, void* buffer, size_t length, int flags);


microtcp_header_t
microtcp_create_header(uint32_t seq_num, uint32_t ack_num, size_t ack,
    size_t rst, size_t syn, size_t fin, uint16_t window,
    uint32_t data_len, uint32_t total_data_size,
    uint32_t data_offset, uint32_t congestion_limit);

void*
microtcp_create_packet(uint32_t seq_num, uint32_t ack_num, size_t ack,
    size_t rst, size_t syn, size_t fin, uint16_t window,
    uint32_t data_len, char* data, uint32_t total_data_size,
    uint32_t data_offset, uint32_t congestion_limit);

short
microtcp_create_control(int ack, int rst, int syn, int fin);

unsigned int
microtcp_unpack(void* packet, microtcp_header_t* header, char** data);

unsigned int
microtcp_checksum(microtcp_header_t* header);

int F_timeout(microtcp_sock_t* socket, int duration);


int
microtcp_shutdownClient(microtcp_sock_t* clientSocket, int how);


int microtcp_shutdown(microtcp_sock_t* clientSocket, int how);

int
microtcp_shutdownServer(microtcp_sock_t* serverSocket, int how);

unsigned int microtcp_checksum_check(void* packet);

unsigned int microtcp_isAck(microtcp_header_t* header);

unsigned int microtcp_isRst(microtcp_header_t* header);

unsigned int microtcp_isSyn(microtcp_header_t* header);

unsigned int microtcp_isFin(microtcp_header_t* header);

struct sockaddr create_sockaddr(const char* ip_string, int port);

struct in_addr get_address(const char* ip_string);

struct sockaddr create_sockaddr_self(int port);

int check_control(microtcp_header_t* header, int ack, int rst, int syn, int fin);

ssize_t send_ack(microtcp_sock_t* socket);

ssize_t microtcp_check_dupAck(microtcp_sock_t* socket, int* dup_ack, int* last_ack_sent, microtcp_header_node** header_list);

int rst_socket(microtcp_socket_image image, microtcp_sock_t* socket);

int create_sock_image(microtcp_socket_image* image, microtcp_sock_t* socket);






microtcp_header_node* add_microtcp_header_node(microtcp_header_node* list, microtcp_header_t* header, uint8_t* payload);
microtcp_header_node* create_microtcp_header_node(microtcp_header_t* header, uint8_t* payload);
microtcp_header_t get_microtcp_header_node_list_header(microtcp_header_node* list);
uint8_t* pop_microtcp_header_node(microtcp_header_node** list);

void free_microtcp_header_node(microtcp_header_node**);


ssize_t microtcp_zero_win_send(microtcp_sock_t* socket, uint16_t* window, 
    uint32_t total_data_size, uint32_t data_offset, uint32_t control_limit);

void free_(char* msg, ...);
void error_msg(const char* msg);


#endif /* LIB_MICROTCP_H_ */
