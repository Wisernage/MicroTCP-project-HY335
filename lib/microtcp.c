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
    
    #include "microtcp.h"
    #include "../utils/crc32.h"
    #include <errno.h>
    #include <limits.h>
    #include <sys/param.h>
    #include <ctype.h>
    #include <stdio.h>
    #include <math.h>
    
    
    microtcp_sock_t
    microtcp_socket(int domain, int type, int protocol)
    {
        microtcp_sock_t new_sock_t;
        memset(&new_sock_t, 0, sizeof(microtcp_sock_t));
        
        new_sock_t.sd = socket(domain, type, protocol);
    
        if (new_sock_t.sd == -1)
        {
            fprintf(stderr, "Error: Something went wrong with creating the socket.%s\n", strerror(errno));
            new_sock_t.state = INVALID;
        }
        else
            new_sock_t.state = UKNOWN;
        return new_sock_t;
    }
    
    int
    microtcp_bind(microtcp_sock_t* socket, const struct sockaddr* address,
        socklen_t address_len)
    {  
        return bind(socket->sd, address, address_len);
    }
    
    int
        microtcp_connect(microtcp_sock_t* clientSocket, const struct sockaddr* serverAddress,
            socklen_t address_len)
    {
        void* buffer;
        char* received_data;
        time_t t;
        int data_size;

        if (set_socket_timeout(clientSocket, MICROTCP_ACK_TIMEOUT_US) == -1) {
            fprintf(stderr, "Error: Error in set_socket_timeout.\n");
            return -1;
         }
    
    
        srand((unsigned int)time(&t) + 15143);
        int init_seq_num = rand() % 3000;
        clientSocket->seq_number = init_seq_num;
        microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));
    
        buffer = microtcp_create_packet(clientSocket->seq_number, 0, 0, 0, 1, 0, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
    
    
        if ((data_size = sendto(clientSocket->sd, buffer, sizeof(microtcp_header_t),
            0, serverAddress, address_len)) == -1) {
            fprintf(stderr, "Error: Something went wrong with SYN send. %s\n", strerror(errno));
            fprintf(stdout, "sendto SYN in microtcp_connect data size: %d\n", data_size);
            clientSocket->state = INVALID;
            FREE(received_header, buffer);
            return -1;
        }
            
        free(buffer);
    
        buffer = malloc(sizeof(microtcp_header_t));
    
        if ((data_size = recvfrom(clientSocket->sd, buffer,
        sizeof(microtcp_header_t), 0, serverAddress, &address_len)) == -1) {
            if(errno == EAGAIN)
                fprintf(stderr, "Error: timeout occured. %s\n", strerror(errno));
            fprintf(stderr, "Error: Something went wrong with SYN/ACK receive %s\n", strerror(errno));
            fprintf(stdout, "recvfrom SYN/ACK in microtcp_connect data size: %d\n", data_size);
            clientSocket->state = INVALID;
            FREE(received_header, buffer);
            return -1;
        };
    
        if (!microtcp_unpack(buffer, received_header, &received_data)) {
            fprintf(stderr,"Error: unpacking failed (at Checksum check)");
            clientSocket->state = INVALID;
            FREE(received_header, buffer);
            return -1;
        }
        if (!(check_control(received_header, 1, 0, 1, 0) && received_header->ack_number == clientSocket->seq_number + 1)) {
            fprintf(stderr, "Error: ACK in microtcp_connect. Packet's flags were not corresponding to the three-way handshake.");
            clientSocket->state = INVALID;
            FREE(received_header, buffer, received_data);
            return -1;
        }   
    
        clientSocket->ack_number = received_header->seq_number + 1;
        clientSocket->seq_number = received_header->ack_number;
    
        free(buffer);    
        buffer = microtcp_create_packet(clientSocket->seq_number, clientSocket->ack_number, 1, 0, 0, 0, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0); 
        if ((data_size = sendto(clientSocket->sd, buffer, sizeof(microtcp_header_t), 0, serverAddress, address_len)) == -1) {
            fprintf(stderr, "Error: Something went wrong sendto ACK in microtcp_connect. %s\n", strerror(errno));
            printf("sendto ACK in microtcp_connect data size: %d\n", data_size);
            clientSocket->state = INVALID;
            FREE(received_header, buffer);
            return -1;
        }
        FREE(buffer, received_data, received_header);

        if (connect(clientSocket->sd, serverAddress, address_len) == -1) {
            fprintf(stderr, "Error: Something went wrong with connect (PEER). %s\n", strerror(errno));
            clientSocket->state = INVALID;
            return -1;
        }
       
        
        clientSocket->state = ESTABLISHED_PEER; 
        clientSocket->init_win_size = clientSocket->curr_win_size = MICROTCP_WIN_SIZE;
        clientSocket->recvbuf = malloc(sizeof(uint8_t) * MICROTCP_RECVBUF_LEN);
        clientSocket->cwnd = MICROTCP_INIT_CWND;
        clientSocket->ssthresh = MICROTCP_INIT_SSTHRESH;
        clientSocket->buf_fill_level = 0;
    
        return 0;
    }
    
    int set_socket_timeout(microtcp_sock_t* socket, int duration )
    {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = duration;
        return setsockopt(socket->sd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
            sizeof(struct timeval));
    }
    
    int
    microtcp_accept(microtcp_sock_t* serverSocket, struct sockaddr* clientAddress,
        socklen_t address_len)
    {
        assert(serverSocket);
        assert(clientAddress);
        void* buffer;
        char* received_data;
        int data_size;
        time_t t; 
        
    
        srand((unsigned int)time(&t) + 152024);    
        int init_seq_num = rand() % 3000;
        serverSocket->seq_number = init_seq_num;
        microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));
    
        buffer = malloc(sizeof(microtcp_header_t));
       
        if((data_size = recvfrom(serverSocket->sd, buffer, 
            sizeof(microtcp_header_t), 0, clientAddress, &address_len)) == -1)
        {
            fprintf(stderr, "Error: recvfrom SYN in microtcp_accept. %s", strerror(errno));
            printf("recvfrom SYN in connect data size: %d\n", data_size);
            FREE(received_header, buffer);
            return -1;
        }
    
        if (!microtcp_unpack(buffer, received_header, &received_data)) {
            perror("Error: unpacking failed (at Checksum check).");
            FREE(received_header, buffer);
            return -1;
        }
        
        if (!(check_control(received_header, 0, 0, 1, 0))) {
    
            fprintf(stderr, "Error: packet's flags were not corresponding to the three-way handshake.");
            FREE(received_header, buffer, received_data);
            return -1;
        }
    
        serverSocket->ack_number = received_header->seq_number + 1;    
        free(received_data);
        free(buffer);
    
        buffer = microtcp_create_packet(serverSocket->seq_number, serverSocket->ack_number, 1, 0, 1, 0, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
        if((data_size = sendto(serverSocket->sd, buffer, 
            sizeof(microtcp_header_t), 0, clientAddress, address_len)) == -1){
            fprintf(stderr, "Error: sendto SYN/ACK in microtcp_accept. %s", strerror(errno));
            fprintf(stdout, "sendto SYN/ACK in microtcp_connect data size: %d\n", data_size);
            FREE(received_header, buffer);
            return -1;
        }
        free(buffer);    
        
        if (set_socket_timeout(serverSocket, MICROTCP_ACK_TIMEOUT_US) == -1) {
            fprintf(stderr, "Error: Error in set_socket_timeout.\n");
            FREE(received_header);
            return -1;
        }
        buffer = malloc(sizeof(microtcp_header_t));
        if((data_size = recvfrom(serverSocket->sd, buffer, 
            sizeof(microtcp_header_t), 0, clientAddress, &address_len)) == -1){
            if(errno == EINPROGRESS)
                fprintf(stderr,"Error: A timeout occured.\n");
                /*In case of time-out, terminate*/
            fprintf(stderr, "Error: recvfrom ACK in microtcp_accept. %s", strerror(errno));
            printf("recvfrom ACK in microtcp_accept data size: %d\n", data_size);
            FREE(received_header, buffer);
            return -1;
        }
        
        if (!microtcp_unpack(buffer, received_header, &received_data)) {
            fprintf(stderr, "Error: unpacking failed (at Checksum check).");
            FREE(received_header, buffer);
            return -1;
        }
        
        if (!(check_control(received_header, 1, 0, 0, 0) && received_header->ack_number == serverSocket->seq_number + 1)) {
            fprintf(stderr, "Error: packet's flags were not corresponding to the three-way handshake.");
            FREE(received_header, buffer, received_data);
            return -1;
        }
        serverSocket->seq_number++;

        FREE(buffer, received_data, received_header);

        if (connect(serverSocket->sd, clientAddress, address_len) == -1) {
            if(errno == EINPROGRESS)
                fprintf(stderr,"Error: A timeout occured.\n");
                /*In case of time-out, terminate*/
            fprintf(stderr, "Error: Something went wrong with connect (HOST). %s\n", strerror(errno));
            serverSocket->state = INVALID;
            return -1;
        }
        serverSocket->state = ESTABLISHED_HOST;
        serverSocket->init_win_size = serverSocket->curr_win_size = MICROTCP_WIN_SIZE;
        serverSocket->recvbuf = malloc(sizeof(uint8_t)*MICROTCP_RECVBUF_LEN);
        serverSocket->cwnd = MICROTCP_INIT_CWND;
        serverSocket->ssthresh = MICROTCP_INIT_SSTHRESH;
        serverSocket->buf_fill_level = 0;
        return 0;
    }
    
    
    int microtcp_shutdown(microtcp_sock_t* socket, int how)
    {
        void* buffer;
        char* received_data;
        int data_size;
        microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));
        /*------Shutdown host------*/
        if(socket->state == CLOSING_BY_PEER)
        {
            int option = 0;
            do
            {
                if(!option)
                {
                    buffer = microtcp_create_packet(socket->seq_number, socket->ack_number, 1, 0, 0, 0, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
                        if ((data_size = send(socket->sd, buffer, sizeof(microtcp_header_t), 0)) == -1) {
                            fprintf(stderr, "Error: microtcp_send ACK in microtcp_shutdownServer. %s", strerror(errno));
                            fprintf(stdout, "sendto ACK in microtcp_shutdownSever data size: %d\n", data_size);
                            FREE(received_header, buffer);
                            return -1;
                        }
                    free(buffer);
                }
    
                buffer = microtcp_create_packet(socket->seq_number, socket->ack_number, 1, 0, 0, 1, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
                if ((data_size = send(socket->sd, buffer, sizeof(microtcp_header_t), 0)) == -1) {
                    fprintf(stderr, "Error: microtcp_send Fin/ACK in microtcp_shutdownServer. %s", strerror(errno));
                    fprintf(stdout, "send Fin/ACK in microtcp_shutdownServer data size: %d\n", data_size);
                    FREE(received_header, buffer);
                    return -1;
                }
                free(buffer);
                buffer = malloc(sizeof(microtcp_header_t));
                if ((data_size = recv(socket->sd, buffer,
                    sizeof(microtcp_header_t), 0)) == -1) {
                    if(errno == EAGAIN)
                    {
                        fprintf(stdout, "\"Error\": timeout occured.\n");  
                        option = 1;
                        free(buffer);
                        continue;
                    }
                    fprintf(stderr, "Error: Something went wrong with ACK receive %s\n", strerror(errno));
                    printf("recvfrom ACK in microtcp_shutdownServer data size: %d\n", data_size);
                    FREE(received_header, buffer);
                    return -1;
                }
                if (!microtcp_unpack(buffer, received_header, &received_data)) {
                    fprintf(stdout, "Error: checksum error.\n");
                    option = 1;
                    FREE(buffer, received_data);
                    continue;
                }
                if ((check_control(received_header, 1, 0, 0, 0) && received_header->ack_number == socket->seq_number + 1)) 
                    break;
                if ((check_control(received_header, 1, 0, 0, 1) && received_header->ack_number == socket->seq_number )) 
                {
                    option = 0;
                    FREE(buffer, received_data);
                    continue;
                }
                
            }while(1);
    
            socket->seq_number++;
            FREE(buffer, received_data, received_header, socket->recvbuf);
            socket->state = CLOSED;
        }  
        /*------Shutdown peer------*/
        else{
            int option = 0;        
    
            do
            {
                if(option > 0)
                {
                    if (set_socket_timeout(socket, MICROTCP_ACK_TIMEOUT_US) == -1) {
                        fprintf(stderr, "Error: Error in set_socket_timeout.\n");
                        free(received_header);
                        return -1;
                    }
                }
                if(option == 2)
                {    
                    buffer = microtcp_create_packet(socket->seq_number, socket->ack_number, 1, 0, 0, 0, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
                    if ((data_size = send(socket->sd, buffer, sizeof(microtcp_header_t), 0)) == -1) {
                        fprintf(stderr, "Error: microtcp_send ACK in microtcp_shutdownClient. %s", strerror(errno));
                        fprintf(stdout, "sendto ACK in microtcp_shutdownClient data size: %d\n", data_size);
                        socket->state = INVALID;
                        FREE(buffer, received_header);
                        return -1;
                    }
                    break;
                }
                if(option == 0)
                {
                    if (set_socket_timeout(socket, MICROTCP_ACK_TIMEOUT_US) == -1) {
                        fprintf(stderr, "Error: Error in set_socket_timeout.\n");
                        free(received_header);
                        return -1;
                    }
                    buffer = microtcp_create_packet(socket->seq_number, socket->ack_number, 1, 0, 0, 1, MICROTCP_WIN_SIZE, 0, (void*)0, 0, 0, 0);
                    if ((data_size = send(socket->sd, buffer, sizeof(microtcp_header_t), 0)) == -1) {
                        fprintf(stderr, "Error: microtcp_send ACK/FIN in microtcp_shutdownClient. %s", strerror(errno));
                        fprintf(stdout, "sendto ACK/FIN in microtcp_shutdownClient data size: %d\n", data_size);
                        socket->state = INVALID;
                        FREE(buffer, received_header);
                        return -1;
                    }
                    free(buffer);
                }
    
                buffer = malloc(sizeof(microtcp_header_t));
                if ((data_size = recv(socket->sd, buffer,
                    sizeof(microtcp_header_t), 0)) == -1) {
                    if(errno == EAGAIN)
                    {
                        switch(option)
                        {
                            case 0:
                                fprintf(stderr, "Error: A timeout occured.\n");
                                free(buffer);
                                continue;
                            case 1:
                                fprintf(stderr, "Error: A bad timeout occured.\n");
                                FREE(buffer, received_header);
                                socket->state = INVALID;
                                return -1;
                            case 2:
                                break;
                            default:
                                assert(0);
                                fprintf(stderr, "There is no such option.\n");
                                continue;
                        }
                        break;
                        
                    }
                    fprintf(stderr, "Error: Something went wrong with ACK receive %s\n", strerror(errno));
                    printf("recvfrom ACK in microtcp_shutdownClient data size: %d\n", data_size);
                    socket->state = INVALID;
                    FREE(buffer, received_header);
                    return -1;
                }
                if (!microtcp_unpack(buffer, received_header, &received_data)) {
                    perror("Error: unpacking failed (at Checksum check).");
                    free(buffer);
                    continue;
                }
                if ((check_control(received_header, 1, 0, 0, 0) && received_header->ack_number == socket->seq_number + 1)) 
                {
                    FREE(buffer, received_data);
                    socket->seq_number++;
                    socket->state = CLOSING_BY_HOST;
                    option = 1;
                    continue;
                }
                if (check_control(received_header, 1, 0, 0, 1))
                {
                    socket->ack_number++;
                    FREE(buffer, received_data);
                    option = 2;
                }
                
            }while(1);

            FREE(buffer, received_header, socket->recvbuf);

            socket->state = CLOSED;
        }
        return 0;
    }
    
    int check_control(microtcp_header_t* header, int ack, int rst, int syn, int fin)
    {
        assert(header);
        int ack_res, rst_res, syn_res, fin_res;
        
        ack_res = (ack == microtcp_isAck(header)); 
        rst_res = (rst == microtcp_isRst(header));
        syn_res = (syn == microtcp_isSyn(header));
        fin_res = (fin == microtcp_isFin(header));
        
        return ack_res && rst_res && syn_res && fin_res;
    }
    
    ssize_t send_ack(microtcp_sock_t* socket)
    {
        ssize_t data_size;
        printf("Inside send_ack \n");
        void* send_buffer = microtcp_create_packet(socket->seq_number, socket->ack_number, 
                    1, 0, 0, 0, socket->curr_win_size, 0, (void*)0, 0, 0, 0);
                if((data_size = send(socket->sd, send_buffer, sizeof(microtcp_header_t), 0)) == -1)
                {
                    fprintf(stderr, "Error: Something went wrong with send. %s\n", strerror(errno));
                    free(send_buffer);
                    return -1;
                }
        free(send_buffer);
        return data_size; 
    }

    ssize_t
    microtcp_send(microtcp_sock_t* socket, const void* buffer, size_t length,
        int flags)
    {
        int data_size;
        int recv_data_size;
        void *recv_buffer;
        void *send_buffer;
        microtcp_header_t *received_header;
        received_header = malloc(sizeof(microtcp_header_t));
        char *received_data;
        void* temp_buffer;
        int ignore = 0;
        int seg_count = 0;
        int data_sent;
        int zero_len = (length) ? 0 : 1;
        int temp_len;
        int data_acked = 0;
        short unsigned int window = socket->init_win_size;
        int slow_start;
        uint32_t control_limit;
        do
        {   
            
            slow_start = socket->cwnd <= socket->ssthresh;
            printf("Slowstart = %d\n", slow_start);
            if(!ignore)
            {
                printf("Ignore = 0 .\n");
                data_sent = data_acked;

                if (microtcp_zero_win_send(socket, &window, length, data_sent, 0) == -1) {
                    free(received_header);
                    return -1;
                }
                
                control_limit = MIN3(length - data_sent, socket->cwnd, window);
                while (data_sent - data_acked < control_limit)
                {
                    temp_len = MIN(control_limit - (data_sent - data_acked), MICROTCP_MSS);
                    temp_buffer = malloc(sizeof(uint8_t) * (control_limit ? temp_len : 1));

                    memcpy(temp_buffer, (char*)buffer + data_sent, temp_len);

                    //printf("temp len = %u, control limit = %u\n", temp_len, control_limit);

                    send_buffer = microtcp_create_packet(socket->seq_number, socket->ack_number,
                     0, 0, 0, 0, socket->curr_win_size, temp_len, temp_buffer, length, data_sent, control_limit);

                    if ((data_size = send(socket->sd, send_buffer, sizeof(microtcp_header_t) + temp_len, flags)) == -1) {
                        FREE(send_buffer, temp_buffer, received_header);
                        return -1;
                    }

                    data_sent += data_size - sizeof(microtcp_header_t);
                    FREE(send_buffer, temp_buffer);
                }  
            }         
            ignore = 1;
            recv_buffer = malloc(sizeof(microtcp_header_t));
            
            /*Receiving packet.*/
            if((recv_data_size = recv(socket->sd, recv_buffer, sizeof(microtcp_header_t), 0)) == -1)
            {
                if (errno == EAGAIN)
                {
                    fprintf(stderr, "Error: A timeout occured.\n");
                    free(recv_buffer);  
                    // send dup ack                  
                    if (send_ack(socket) == -1) {
                        FREE(recv_buffer, received_header);
                        return -1;
                    }
                    socket->ssthresh = socket->cwnd / 2;
                    socket->cwnd = MIN(MICROTCP_MSS, socket->ssthresh);
                    continue;
                }
                fprintf(stderr, "Error: Something went wrong with recv. %s\n", strerror(errno));
                FREE(recv_buffer, received_header);
                return -1;
            }
            
            /*Checksum*/
            if (!microtcp_unpack(recv_buffer, received_header, &received_data)) 
            {
                if (send_ack(socket) == -1) {
                    FREE(recv_buffer, received_header);
                    return -1;
                }
                free(recv_buffer);
                continue;
            }
            /*Data*/
            if(check_control(received_header, 0, 0, 0, 0))             
            {
                printf("Data .\n");
                if (send_ack(socket) == -1) {
                    FREE(recv_buffer, received_header, received_data);
                    return -1;
                }
                continue;
            } 
            /*Ack*/
            else if (check_control(received_header, 1, 0, 0, 0))
            {
                printf("Ack : ");
                /*Final Ack received*/
                if (received_header->ack_number == socket->seq_number + length) 
                {
                    printf("Final Ack.\n");
                    socket->dup_ack = 0;
                    socket->cwnd += slow_start ? socket->cwnd : MICROTCP_MSS;
                    window = received_header->window;
                    break;
                }
                /*Up to congestion window ack received*/
                else if (received_header->ack_number == socket->seq_number + data_sent)
                {
                    printf("Congestion limit ack\n");
                    socket->dup_ack = 0;
                    ignore = 0;
                    data_acked = received_header->ack_number - socket->seq_number;
                    socket->cwnd += slow_start ? socket->cwnd : MICROTCP_MSS;
                }
                else if (received_header->ack_number >= socket->seq_number + data_acked 
                && received_header->ack_number < socket->seq_number + data_sent) 
                {
                    printf("Possibly Dup Ack.\n");
                    if (data_acked == received_header->ack_number - socket->seq_number) 
                    {
                        socket->dup_ack++;
                    }
                    else {
                        data_acked = received_header->ack_number - socket->seq_number;
                        socket->dup_ack = 0;
                    }
                    if (socket->dup_ack == 3)
                    {         
                      socket->dup_ack = 0;
                      ignore = 0;  
                      socket->ssthresh = socket->cwnd / 2;
                      socket->cwnd = socket->cwnd/2 + 1;
                    }
                }
            }
            else if (check_control(received_header, 0, 1, 0, 0))
            {
                socket->state = INVALID;
                FREE(received_data, received_header);
                return -1;
            }
            
            window = received_header->window;
            FREE(received_data, recv_buffer);
            
        }while(1);
        socket->seq_number += length;
        FREE(recv_buffer, received_data, received_header);
        return data_size;
    }

    

    ssize_t microtcp_check_dupAck(microtcp_sock_t* socket, int* dup_ack, 
        int* last_ack_sent, microtcp_header_node** header_list)
    {
        if (send_ack(socket) == -1)
            return -1;
        if (socket->ack_number == *last_ack_sent) {
            *dup_ack++;
            if (*dup_ack == 3) {
                *dup_ack = 0;
                free_microtcp_header_node(header_list);
            }
        }
        else {
            *last_ack_sent = socket->ack_number;
            *dup_ack = 0;
        }  
        return 0;
    } 


    // :JUMP
    ssize_t
    microtcp_recv(microtcp_sock_t* socket, void* buffer, size_t length, int flags)
    {
        int data_size;
        int recv_data_size;
        void* recv_buffer;
        int retun_value;
        ssize_t return_value;
        size_t dup_ack = 0;
        ssize_t last_ack_sent = -1;
        microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));
        microtcp_header_node* header_list = NULL;
        microtcp_header_t header;
        char* received_data;
        char* node_data;

        socket->curr_win_size = MICROTCP_WIN_SIZE;

        do {
            recv_buffer = malloc(length);
            /* Receiving*/   
            if ((recv_data_size = recv(socket->sd, recv_buffer, length, flags)) == -1) {
                if (errno == EAGAIN)
                {
                    fprintf(stderr, "Error: A timeout occured.\n");
                    free(recv_buffer);                    
                    if (microtcp_check_dupAck(socket, &dup_ack, &last_ack_sent, &header_list) == -1) {
                        FREE(recv_buffer, received_header);
                        free_microtcp_header_node(&header_list);
                        return -1;
                    }
                    continue;
                }
                fprintf(stderr, "Error: Something went wrong with recv. %s\n", strerror(errno));
                FREE(recv_buffer, received_header);
                free_microtcp_header_node(&header_list);
                return -1;
            }

            /*Ignore packet*/
            if (!microtcp_unpack(recv_buffer, received_header, &received_data))
            {
                printf("In Bad Unpack.\n");
                free(recv_buffer);
                continue;
            }

            free(recv_buffer);
            /*Correct packet, fragmentation*/

            if(check_control(received_header, 0, 0, 0, 0)
            && received_header->seq_number == socket->ack_number - socket->bytes_received)
            {
                printf("In data packet.\n");
                if(received_header->data_offset == socket->bytes_received)
                {
                    printf("In in order received Packet.\n");
                    socket->ack_number += received_header->data_len;
                    memcpy((char*)socket->recvbuf + socket->buf_fill_level, received_data, sizeof(uint8_t) * received_header->data_len); 
                    socket->bytes_received += received_header->data_len;
                    socket->buf_fill_level += received_header->data_len;
                    socket->curr_win_size -= received_header->data_len;

                    /*Check ordered list*/
                    while(header_list)
                    {   
                        fprintf(stdout,"In header list while.\n");
                        header = get_microtcp_header_node_list_header(header_list);
                        
                        if(header.data_offset == socket->bytes_received)
                        {
                            node_data = pop_microtcp_header_node(&header_list);       
                            socket->ack_number += header.data_len;
                            memcpy((char*)socket->recvbuf + socket->buf_fill_level, node_data, sizeof(uint8_t) * header.data_len); 
                            socket->bytes_received += header.data_len;
                            socket->buf_fill_level += header.data_len;
                            socket->curr_win_size -= header.data_len;
                            free(node_data);
                        }
                        else
                            break;                       
                    } 

                    dup_ack = 0;
                    /* Return if bytes received successfully , we don't care about window size*/
                    if(received_header->total_data_size == socket->bytes_received) 
                    {  
                        // printf("Final Ack sent : %u\n", socket->ack_number);
                        // printf("total_data_size = %u\n", received_header->total_data_size);
                        // printf("Bytes received = %u\n", socket->bytes_received);
                        socket->curr_win_size = MICROTCP_RECVBUF_LEN;                      
                        socket->bytes_received = 0;
                        if (send_ack(socket) == -1) {
                            FREE(received_data, received_header);
                            free_microtcp_header_node(&header_list);
                            return -1;
                        }
                        break;                        
                    }                    
                    /* Window size = 0, return back to main OR
                    /* Congestion or flow limit reached. Continue, free received header list.*/
                    else if (!socket->curr_win_size || received_header->control_limit == socket->buf_fill_level) 
                    {                        
                        // printf("Control Ack sent : %u\n", socket->ack_number);
                        // printf("total_data_size = %u\n", received_header->total_data_size);
                        // printf("Bytes received = %u\n", socket->bytes_received);
                        socket->curr_win_size = MICROTCP_RECVBUF_LEN;
                        if (send_ack(socket) == -1) {
                            FREE(received_data, received_header);
                            free_microtcp_header_node(&header_list);
                            return -1;
                        }
                        break;    
                    }
                } 
                /*Out of sequence received packet, insert to list.*/
                else if(received_header->data_offset > socket->bytes_received) 
                {
                    printf("Out of order received Packet.\n");
                    header_list = add_microtcp_header_node(header_list, received_header, received_data);
                }
                /*Ignore*/
            }
            /*Shutdown*/
            else if ((check_control(received_header, 1, 0, 0, 1) && socket->ack_number == received_header->seq_number))
            {
                socket->ack_number++;
                socket->state = CLOSING_BY_PEER;
                free_microtcp_header_node(&header_list);
                FREE(received_data, received_header);
                return -1;
            }
            else if (check_control(received_header, 0, 1, 0, 0))
            {
                socket->state = INVALID;
                free_microtcp_header_node(&header_list);
                FREE(received_data, received_header);
                return -1;
            }
            else 
                if (microtcp_check_dupAck(socket, &dup_ack, &last_ack_sent, &header_list) == -1) {
                    FREE(received_data, received_header);
                    free_microtcp_header_node(&header_list);
                    return -1;
                }

            FREE(received_data);
            printf("End of while\n");
        } while (1);
        printf("Out of rcv while.\n");
        memcpy(buffer, socket->recvbuf, sizeof(uint8_t) * socket->buf_fill_level);
        return_value = socket->buf_fill_level;
        socket->buf_fill_level = 0;
        free_microtcp_header_node(&header_list);
        FREE(received_data, received_header);
        return return_value;
    }
    
    
    
    //-----------helper functions------------------
    
    
    // Dummy functions 
    int
    microtcp_shutdownClient(microtcp_sock_t* clientSocket, int how)
    {
        return -1;
    }
    
    int
    microtcp_shutdownServer(microtcp_sock_t* serverSocket, int how)
    {
        return -1;
    }
    
    
    void*
    microtcp_create_packet(uint32_t seq_num, uint32_t ack_num, size_t ack,
    size_t rst, size_t syn, size_t fin, uint16_t window,
    uint32_t data_len, char* data, uint32_t total_data_size,
    uint32_t data_offset, uint32_t control_limit)
    {
        // TODO: Network byte order
        void* packet;
        microtcp_header_t* header;
        char* payload;
        packet = malloc(sizeof(microtcp_header_t) + sizeof(char) * data_len);
        header = packet;
        header[0] = microtcp_create_header(seq_num, ack_num, ack, rst, syn, fin, window, data_len, total_data_size, data_offset, control_limit);
        if (data_len) 
        {
            payload = packet;
            memcpy(payload + sizeof(microtcp_header_t), data, sizeof(uint8_t)*data_len);
        }
        header[0].checksum = crc32((uint8_t*)packet, sizeof(microtcp_header_t) + sizeof(char) * data_len);
        header[0].checksum = htonl(header[0].checksum);
        return packet;
    }
    
    microtcp_header_t
    microtcp_create_header(uint32_t seq_num, uint32_t ack_num, size_t ack,
    size_t rst, size_t syn, size_t fin, uint16_t window,
    uint32_t data_len, uint32_t total_data_size,
    uint32_t data_offset, uint32_t control_limit)
    {
        microtcp_header_t header;
        memset(&header, 0, sizeof(microtcp_header_t));
        header.seq_number = htonl(seq_num);
        header.ack_number = htonl(ack_num);
        header.control = htons(microtcp_create_control(ack, rst, syn, fin));
        header.window = htons(window);
        header.data_len = htonl(data_len);
        header.total_data_size = htonl(total_data_size);
        header.data_offset = htonl(data_offset);
        header.control_limit = htonl(control_limit);
        return header;
    }
    
    short
    microtcp_create_control(int ack, int rst, int syn, int fin)
    {
        short control = 0;
        if (ack) control += 8;
        if (rst) control += 4;
        if (syn) control += 2;
        if (fin) control += 1;
        return control;
    }

    void unpack_ntoh(microtcp_header_t* head_er)
    {
        microtcp_header_t header;
        memset(&header, 0, sizeof(microtcp_header_t));
        header.seq_number = ntohl(head_er->seq_number);
        header.ack_number = ntohl(head_er->ack_number);
        header.control = ntohs(head_er->control);
        header.window = ntohs(head_er->window);
        header.data_len = ntohl(head_er->data_len);
        header.total_data_size = ntohl(head_er->total_data_size);
        header.data_offset = ntohl(head_er->data_offset);
        header.control_limit = ntohl(head_er->control_limit);
        *head_er = header;
    }

    unsigned int
    microtcp_unpack(void* packet, microtcp_header_t* header, char** data)
    {
        microtcp_header_t* h = packet;
        char* d = packet;

        h->checksum = ntohl(h->checksum);
        if (!microtcp_checksum_check(packet)) return 0;
        unpack_ntoh(h);
        *header = *h;
        if (!h->data_len)
            h->data_len = 1;
        *data = malloc(sizeof(char) * h->data_len);
        memcpy(*data, d + sizeof(microtcp_header_t), sizeof(char) * h->data_len);
        // TODO: Network byte order
        
        return 1;
    }
    
    unsigned int
    microtcp_checksum_check(void* packet)
    {
        microtcp_header_t* h = packet;
        uint32_t old_checksum = h[0].checksum;
        uint32_t data_len = ntohl(h[0].data_len);
        h[0].checksum = 0;
        uint32_t a = crc32((uint8_t*)packet, sizeof(microtcp_header_t) + sizeof(char) * data_len);
    
        printf("old_checksum = %u\nnew_checksum = %u\n", old_checksum, a);
        if (old_checksum != crc32((uint8_t*)packet, sizeof(microtcp_header_t) + sizeof(char) * data_len))
            return 0;
        return 1;
    }
    
    
    unsigned int
    microtcp_isAck(microtcp_header_t* header) {
        // printf("ISACK:%d\n",header->control >> 3);
        if (header->control >> 3) return 1;
        return 0;
    }
    
    unsigned int
    microtcp_isRst(microtcp_header_t* header) {
        // printf("ISRST:%d\n",(header->control >> 2) % 2);
        if ((header->control >> 2) % 2) return 1;
        return 0;
    }
    
    unsigned int
    microtcp_isSyn(microtcp_header_t* header) {
        // printf("ISSYN:%d\n",(header->control >> 1)%2);
        if ((header->control >> 1) % 2) return 1;
        return 0;
    }
    
    unsigned int
    microtcp_isFin(microtcp_header_t* header) {
        // printf("ISFIN:%d\n",(header->control%2));
        if ((header->control) % 2) return 1;
        return 0;
    }
    
    struct sockaddr create_sockaddr(const char* ip_string, int port)
    {
        assert(ip_string);
        struct sockaddr_in saddr;   
         
        saddr.sin_port = htons((uint16_t)port);
        if(!strcmp(ip_string, "INADDR_ANY"))
            saddr.sin_addr.s_addr = INADDR_ANY;     
        else saddr.sin_addr.s_addr = inet_addr(ip_string);
        saddr.sin_family = AF_INET;
        memset(&saddr.sin_zero, 0, sizeof(unsigned char)*8);
        return *(struct sockaddr*)(&saddr);
    }
    
    
    struct sockaddr create_sockaddr_self(int port)
    {
        struct sockaddr;
        char* ip_string;
        struct hostent* host_entry;
        int hostname;
        host_entry = gethostbyname("localhost");
        ip_string = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
        return create_sockaddr(ip_string, port);
    }
    
    int rst_socket(microtcp_socket_image image, microtcp_sock_t *socket){
        socket->seq_number = image.seq_number;
        socket->ack_number = image.ack_number;
        socket->state = image.state;
        return 1;
    }
    
    int create_sock_image(microtcp_socket_image *image, microtcp_sock_t *socket){
        image->seq_number = socket->seq_number ;
        image->ack_number = socket->ack_number ;
        image->state = socket->state;
        return 1;
    }


    void free_(char* msg, ...)
    {
        va_list args;
        va_start(args, msg);
        void* temp;
        while((temp = va_arg(args, void*)))
            free(temp);
        va_end(args);
    }

    void error_msg(const char* msg)
    {
        fprintf(stderr, msg, strerror(errno));
    }


microtcp_header_node* add_microtcp_header_node(microtcp_header_node* list, microtcp_header_t* header, uint8_t* payload)
{
    microtcp_header_node* new_node = create_microtcp_header_node(header, payload);
    microtcp_header_node* iter = list;
    microtcp_header_node* prev = NULL;
    /*List is empty. Placed at Start*/
    if(!list)
    	list = new_node;
    while(iter)
    {
    	if(header->data_offset < iter->header.data_offset)
    	{
    		new_node->next = iter;
    		/*Placed at middle*/
    		if(prev)
    			prev->next = new_node;
    		/*Placed at start*/
			else
				list = new_node;
			return list;
		}
		else if(header->data_offset > iter->header.data_offset)
		{
			prev = iter;
			iter = iter->next;
		}
		/*sth is wrong probably packet was resent*/
		else
			return list;
	}
	/*Placed at end*/
    if(prev)
	   prev->next = new_node;
	return list;
}


microtcp_header_node* create_microtcp_header_node(microtcp_header_t* header, uint8_t* payload)
{
    microtcp_header_node* new;
    new->header = *header;
    new->payload = malloc(sizeof(uint8_t) * header->data_len);
    memcpy(new->payload, payload, header->data_len*sizeof(uint8_t));
    new->next = NULL;
    return new;
}


microtcp_header_t get_microtcp_header_node_list_header(microtcp_header_node* list)
{
	assert(list);
	return list->header;
}

uint8_t* pop_microtcp_header_node(microtcp_header_node** list)
{
	microtcp_header_node* head = list;
	*list = (*list)->next;
	return head->payload;
}

void free_microtcp_header_node(microtcp_header_node** list)
{
    if(*list != NULL)
    {
      microtcp_header_node* iter = *list;
      while(iter)
      {
        free(iter->payload);
        iter = iter->next;          
      }
      *list = NULL;
    }
}

// :JUMP
ssize_t microtcp_zero_win_send(microtcp_sock_t* socket, uint16_t* window, 
    uint32_t total_data_size, uint32_t data_offset, uint32_t control_limit)
{
    time_t t;
    uint8_t* send_buffer;
    uint8_t* recv_buffer;
    char* received_data;
    size_t recv_data_size;
    size_t data_size;
    microtcp_header_t* received_header = malloc(sizeof(microtcp_header_t));

    while (!*window) {
        printf("Inside microtcp_zero_win_send.\n");
        srand((unsigned int)time(&t) + 152024);
        sleep(rand() % MICROTCP_ACK_TIMEOUT_US);
        send_buffer = microtcp_create_packet(socket->seq_number, socket->ack_number,
            0, 0, 0, 0, socket->curr_win_size, 0, NULL, total_data_size, data_offset, control_limit);
        if ((data_size = send(socket->sd, send_buffer, sizeof(microtcp_header_t), 0)) == -1)
            return -1;
        free(send_buffer);
        recv_buffer = malloc(sizeof(microtcp_header_t));
        if ((recv_data_size = recv(socket->sd, recv_buffer, sizeof(microtcp_header_t), 0)) == -1)
        {
            if (errno == EAGAIN)
              fprintf(stderr, "Error: A timeout occured.\n");
            else {
                FREE(received_header, recv_buffer);
                return -1;
            }
            free(recv_buffer);
            continue;
        }
        if (!microtcp_unpack(recv_buffer, received_header, &received_data)){
            free(recv_buffer);
            continue;
        }
        if (check_control(received_header, 1, 0, 0, 0) && socket->seq_number + data_offset == received_header->ack_number)
        {
            *window = received_header->window;
            printf("Ack inside window size zero ack. window: %d\n",*window); 
        }   
        else if (check_control(received_header, 0, 0, 0, 0))
        {
            if (send_ack(socket) == -1) {
                FREE(received_header, recv_buffer, received_data);
                return -1;
            }
            *window = received_header->window;
        }
        FREE(received_data, recv_buffer);
    }
    free(received_header);
    return 0;
}