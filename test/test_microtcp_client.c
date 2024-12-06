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

/*
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */
#include "../lib/microtcp.h"
#include "../utils/crc32.h"
#include <errno.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    // void* buffer;
    // int port;
    // char* ip_string = NULL;
    // int exit_code = 0;
    // char *payload;
    // int data_size;
    // char *received_data;
    // microtcp_header_t *received_header;

    // if (argc < 2)
    //     fprintf(stderr, "Error: Not enough arguments. \n");
    // else
    // {        
    //     // if((ip_string = strdup(argv[1])) == NULL)
    //     //     fprintf(stderr, "Error: Insufficient memory. %s\n",strerror(errno));
    //     if((port = atoi(argv[1]))== __INT32_MAX__)
    //         fprintf(stderr, "Error: Overflow occured. %s\n",strerror(errno));
    //     fprintf(stdout, "%d\n", port);
    // }

    // struct sockaddr clientAddress = create_sockaddr_self(port);
    // struct sockaddr serverAddress = create_sockaddr_self(port);
    // microtcp_sock_t clientSocket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    // fprintf(stdout, "Server IP address: %s and port %d\n", inet_ntoa(((struct sockaddr_in *)(&serverAddress))->sin_addr),ntohs(((struct sockaddr_in *)(&serverAddress))->sin_port));   
    // fprintf(stdout, "Client IP address: %s and port %d\n", inet_ntoa(((struct sockaddr_in *)(&clientAddress))->sin_addr),((struct sockaddr_in *)(&clientAddress))->sin_port);//ntohs(((struct sockaddr_in *)(&clientAddress))->sin_port));

    // // if (microtcp_bind(&clientSocket, &clientAddress, 16) == - 1)
    // //      fprintf("Error: Error binding socket. %s\n", strerror(errno));


    // if(microtcp_connect(&clientSocket, &serverAddress, 16) == -1){
    //     fprintf(stderr, "Error: Something went wrong with server microctp_connect call.\n %s\n Exiting program...\n", 
    //             strerror(errno));
    //     exit(-1);
    // }
    // // buffer = microtcp_create_packet(clientSocket.seq_number, clientSocket.ack_number, 0, 0, 0, 0, 66546, 0, (void*)0);
    // // if (send(clientSocket.sd, buffer, sizeof(microtcp_header_t), 0));


    
    // received_header = malloc(sizeof(microtcp_header_t));
    // payload = malloc(sizeof(char)*(strlen("Erwthsh: Mpamphs??")+1));
    // strcpy(payload, "Erwthsh: Mpamphs??");  
    // buffer = microtcp_create_packet(clientSocket.seq_number, clientSocket.ack_number, 0, 0, 0, 0, 66546, strlen(payload)+1, payload);
    // if ((data_size = send(clientSocket.sd, buffer, (strlen(payload) + 1)+ sizeof(microtcp_header_t), 0)) == -1) {
    //     fprintf(stderr, "Error while making fetch request to server. %s", strerror(errno));
    //     fprintf(stdout, "Making fetch request to server. Data size: %d\n", data_size);
    //     free(payload);
    //     free(buffer);
    //     return -1;
    // }
    // fprintf(stdout, "Making fetch request to server. Data size: %d\n", data_size);
    // free(buffer);
    // buffer = malloc(sizeof(microtcp_header_t));
    // if ((data_size = recv(clientSocket.sd, buffer,
    //     sizeof(microtcp_header_t), 0)) == -1) {
    //     fprintf(stderr, "Error: Something went wrong with fetch request receive %s\n", strerror(errno));
    //     fprintf(stdout, "Recv fetch request. Data size: %d\n", data_size);
    //     return -1;
    // }
    // fprintf(stdout, "Recv fetch request ack. Data size: %d\n", data_size);
    // if (!microtcp_unpack(buffer, received_header, &received_data)) {
    //     fprintf(stderr, "Error: unpacking failed (at Checksum check).");
    //     return -1;
    // }
    // fprintf(stdout, "Data size %d\n", data_size);
    // if (!(microtcp_isAck(received_header) &&
    //     !microtcp_isSyn(received_header) &&
    //     !microtcp_isRst(received_header) &&
    //     !microtcp_isFin(received_header) &&
    //     received_header->ack_number == clientSocket.seq_number + strlen(payload) + 1)) {
    //     fprintf(stderr, "Error: packet's flags were not corresponding to the microtcp acknowledgement protocol.");
    //     return -1;
    // }

    // clientSocket.seq_number += strlen(payload) + 1;
    // free(payload);
    // free(buffer);
    // free(received_data);
    // buffer = malloc(sizeof(microtcp_header_t)+ MAX_PAYLOAD);
    // if ((data_size = recv(clientSocket.sd, buffer,
    //     sizeof(microtcp_header_t) + MAX_PAYLOAD, 0)) == -1) {
    //     fprintf(stderr, "Error: Something went wrong with receiving fetched data %s\n", strerror(errno));
    //     fprintf(stdout, "Fetched data recv. Data size: %d\n", data_size);
    // }
    // fprintf(stdout, "Fetched data recv. Data size: %d\n", data_size);
    // buffer = realloc(buffer, sizeof(microtcp_header_t) + sizeof(char)
    //         * ((microtcp_header_t *)buffer)->data_len);
    // if (!microtcp_unpack(buffer, received_header, &received_data)) {
    //    perror("Error: unpacking failed (at Checksum check).");
    //    return -1;
    // }
    // fprintf(stdout, "Data size %d\n", data_size);
    // if (microtcp_isAck(received_header) || microtcp_isSyn(received_header) || 
    //      microtcp_isRst(received_header) || microtcp_isFin(received_header)
    //     || (clientSocket.ack_number != received_header->seq_number)){
    //     fprintf(stdout, "Error: Fetch packet has flags(should not have flags) %s\n", received_data);
    //     free(received_data);
    //     free(buffer);
    //     return -1;
    // }
    // fprintf(stdout, "%s\n", received_data);
    // clientSocket.ack_number += received_header->data_len;
    // free(received_data);
    // free(buffer);
    // buffer = microtcp_create_packet(clientSocket.seq_number, clientSocket.ack_number, 1, 0, 0, 0, 66546, 0, (void*)0);
    // if ((data_size = send(clientSocket.sd, buffer, sizeof(microtcp_header_t), 0)) == -1)
    // {
    //     fprintf(stderr, "Error: Error while sending ack packet %s", strerror(errno));
    //     fprintf(stdout, "Ack send. Data size: %d\n", data_size);
    //     free(buffer);
    //     return -1;
    // }
    // fprintf(stdout, "Ack send data size: %d\n", data_size);
    // free(buffer);

    
    // microtcp_shutdownClient(&clientSocket, 0);
    // if(close(clientSocket.sd) == -1)
    //     fprintf(stdout, "Error: Something went wrong with closing the socket.");

 
}

