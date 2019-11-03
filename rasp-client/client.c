#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <string.h> //strlen
#include <unistd.h> //close(socket_desc)

char *buffer;
const int read_size = 4096 * 8;

int send_to_server(char *ip, uint16_t port, char *msg) {
    //create a socket, returns -1 of error happened
    /*
     Address Family - AF_INET (this is IP version 4)
     Type - SOCK_STREAM (this means connection oriented TCP protocol)
     Protocol - 0 [ or IPPROTO_IP This is IP protocol]
     */
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create a socket\n");
    }

    //create a server
    struct sockaddr_in server;


    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port); //specify the open port_number

    //connect to the server
    if (connect(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        puts("Connect error\n");
        return 1;
    }
    puts("Connected\n");

    if (send(socket_desc, msg, strlen(msg), 0) < 0) {
        puts("Send failed\n");
        return 1;
    }
    puts("Data send\n");

    //receive data from server
    int idx = 0;
    int received;
    while ((received = recv(socket_desc, buffer + idx, read_size, 0)) > 0) {
        idx += received;
        printf("received: %d. Total: %d\n", idx, received);
    }
    buffer[idx] = 0;
    printf("finished Total: %d", idx);
    close(socket_desc);
    return 0;
}

int main(){

    buffer = malloc(2 * 1000 * 1000);
//    send_to_server("127.0.0.1", 8888, "Stest-out.png");

//    int exit = fopen("chat.c2", "r") != NULL;
    send_to_server("127.0.0.1", 8888, "Dtest-out.png-0");

    free(buffer);

    return 0;
}
