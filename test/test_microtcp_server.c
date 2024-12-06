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
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */
#include "../lib/microtcp.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>
#include <arpa/inet.h>


int
main(int argc, char **argv)
{

    // int port;
    // char* ip_string = NULL; 
    // int exit_code = 0;
    // void* buffer;
    // char* received_data;
    // int data_size;
    // char *payload;
    // microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));
    // microtcp_socket_image image;
 
    // if (argc < 2)
    //     fprintf(stderr, "Error: Not enough arguments. \n");
    // else
    // {        
    //     // if ((ip_string = strdup(argv[1])) == NULL)
    //     //     fprintf(stderr, "Error: Insufficient memory. %s\n",strerror(errno));
    //     if((port = atoi(argv[1])) == __INT32_MAX__)    
    //         fprintf(stderr, "Error: Overflow occured. %s\n",strerror(errno));   
    //     fprintf(stdout, "%d\n", port);
    // }    
    
    // struct sockaddr serverAddress = create_sockaddr_self(port); // 1270001
    // struct sockaddr clientAddress = create_sockaddr_self(port);//create_sockaddr(ip_string, port);
    // printf("Server IP address: %s and port %d\n", inet_ntoa(((struct sockaddr_in *)(&serverAddress))->sin_addr),ntohs(((struct sockaddr_in *)(&serverAddress))->sin_port));   
    // printf("Client IP address: %s and port %d\n", inet_ntoa(((struct sockaddr_in *)(&clientAddress))->sin_addr),ntohs(((struct sockaddr_in *)(&clientAddress))->sin_port));

    // microtcp_sock_t serverSocket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    // if (microtcp_bind(&serverSocket, &serverAddress, 16) == - 1)
    //     fprintf(stderr, "Error: Error binding socket. %s\n", strerror(errno));
    
    

    // payload = malloc(sizeof(char));
    // if(microtcp_accept(&serverSocket, &clientAddress, 16) == -1){
    //     fprintf(stderr, "Error: Something went wrong with server microctp_accept call.\n%s\n Exiting program...\n", 
    //             strerror(errno));
    //     exit(-1);
    // }
    // else {
    //     while (1) {
    //         buffer = malloc(sizeof(microtcp_header_t)+ MAX_PAYLOAD);
    //         if ((data_size = recv(serverSocket.sd, buffer,
    //             sizeof(microtcp_header_t)+ MAX_PAYLOAD, 0)) == -1) {
    //             fprintf(stderr, "Error: Something went wrong with receive %s\n", strerror(errno));
    //             printf("microtcp_recv in microtcp_shutdownClient data size: %d\n", data_size);
    //         }
    //         printf("Data size %d\n", data_size);
    //         buffer = realloc(buffer,sizeof(char) * data_size);
    //         if (!microtcp_unpack(buffer, received_header, &received_data)) 
    //         {
    //             perror("Error: unpacking failed (at Checksum check).");
    //             continue;
    //         }
    //         create_sock_image(&image, &serverSocket);

    //         printf("Ack number: %d\nSequence number %d\n", serverSocket.ack_number, received_header->seq_number);
    //         printf("Control bits: %d\n", received_header->control);

    //         if ((microtcp_isAck(received_header) && !microtcp_isSyn(received_header) && 
    //         !microtcp_isRst(received_header) && microtcp_isFin(received_header)
    //         && serverSocket.seq_number == received_header->ack_number))
    //         {
    //             fprintf(stdout, "Shutdown commencing ...\n");
    //             if(microtcp_shutdownServer(&serverSocket, 0) == -1){
    //                 fprintf(stderr, "Error: Something went wrong with shutdownServer %s\n", strerror(errno));
    //                 rst_socket(image,&serverSocket);
    //                 free(buffer);
    //                 free(received_data);
    //                 continue;
    //             }
    //             printf("Shutdown successful\n");
    //             free(buffer);
    //             free(received_data);
    //             break;
    //         }

    //         if ((microtcp_isAck(received_header) && !microtcp_isSyn(received_header) &&
    //             !microtcp_isRst(received_header) && !microtcp_isFin(received_header)
    //             && (serverSocket.seq_number + strlen(payload) +1 == received_header->ack_number))){
    //             fprintf(stdout, "Ack packet received, analyzing\n");
    //             free(buffer);
    //             free(received_data);
    //             serverSocket.seq_number += strlen(payload) + 1;
    //             continue;
    //         }

    //         if ((!microtcp_isAck(received_header) &&
    //             !microtcp_isSyn(received_header) &&
    //             !microtcp_isRst(received_header) &&
    //             !microtcp_isFin(received_header)
    //             && (received_header->seq_number == 
    //                 serverSocket.ack_number))) {
    //             fprintf(stdout, "Received data: %s\n", received_data);
    //             free(received_data);
    //             free(buffer);
    //             serverSocket.ack_number += received_header->data_len;
    //             buffer = microtcp_create_packet(serverSocket.seq_number, serverSocket.ack_number, 1, 0, 0, 0, 66546, 0, (void*) 0);
    //             if ((data_size = send(serverSocket.sd, buffer, sizeof(microtcp_header_t), 0)) == -1)
    //             {
    //                 fprintf(stderr, "Error: Error while sending ack packet %s", strerror(errno));
    //                 fprintf(stdout, "Ack send. Data size: %d\n", data_size);
    //                 free(buffer);
    //                 continue;
    //             }
    //             fprintf(stdout, "Ack send data size: %d\n", data_size);
    //             free(buffer);
    //             payload = realloc(payload, sizeof(char) * (strlen("Apanthsh: Mpamphs") + 1));
    //             strcpy(payload, "Apanthsh: Mpamphs");
    //             buffer = microtcp_create_packet(serverSocket.seq_number, serverSocket.ack_number, 0, 0, 0, 0, 66546, strlen(payload) + 1, payload);
    //             if ((data_size = send(serverSocket.sd, buffer, sizeof(microtcp_header_t) + strlen(payload) + 1, 0)) == -1)
    //             {
    //                 fprintf(stderr, "Error: Error at fetching requested packet %s", strerror(errno));
    //                 fprintf(stdout, "Fetching packet... Data size: %d\n", data_size);
    //                 free(buffer);
    //                 continue;
    //             }
    //             fprintf(stdout, "Fetching packet... Data size: %d\n", data_size);
    //             free(buffer);
    //             continue;



    //         }
    //         fprintf(stderr, "Warning: Packet's flags were not corresponding to any available option.\n");
    //     }
    // }    
    

    // free(received_header);
    // free(payload);

    // if(close(serverSocket.sd) == -1)
    //     printf("Who knows???");
    
    // return exit_code;
}