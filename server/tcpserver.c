
#include "tcpserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 20069
#define MAX_CLIENTS 5

#define CLIENT_TIMEOUT 30 //Bin clients after this many seconds.

pthread_t serverThread;
bool bServerRunning = true;

int server_socket;

static void *handle_client(void *arg)
{
    pthread_detach(pthread_self());
    int client_socket = *((int *)arg);
    bool bClientRunning = true;
    
    printf("Client connected.\n");

    struct timeval tv;
    tv.tv_sec = CLIENT_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while(bClientRunning && bServerRunning)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Read data from the client
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0)
        {
            printf("Client socket is dead\n");
            bClientRunning = false;
        }
        else
        {
            printf("Received data from client: %s\n", buffer);
            send(client_socket, buffer, bytes_received, 0);
            send(client_socket, "lol", 3, 0);
        }
    }

    // Close the client socket and exit the thread
    close(client_socket);
    printf("Client socket closed\n");
    return NULL;
}

void *handle_server(void *arg)
{
    printf("Server listening on port %d...\n", PORT);

    while (bServerRunning)
    {
        int client_socket;

        // Accept a new connection
        if ((client_socket = accept(server_socket, NULL, NULL)) == -1)
        {
            printf("Timeout/error\n");
        }
        else
        {
            // Create a new thread to handle the connection
            if (pthread_create(&serverThread, NULL, handle_client, &client_socket) != 0)
            {
                printf("Error creating thread\n");
                close(client_socket);
            }
        }
    }

    close(server_socket);
    printf("Server socket closed\n");
    pthread_exit(NULL);
}

bool tcpserver_init()
{
    struct sockaddr_in server_addr;

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error creating socket\n");
        return false;
    }

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Error binding\n");
        close(server_socket);
        return false;
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        printf("Error listening\n");
        close(server_socket);
        return false;
    }

    if(0 != pthread_create(&serverThread, NULL, &handle_server, NULL))
    {
        printf("Error creating server thread\n");
        close(server_socket);
        return false;
    }

    return true;
}


void tcpserver_deinit()
{
    bServerRunning = false;
    close(server_socket);
    pthread_cancel(serverThread);
}


void tcpserver_die()
{
    pthread_join(serverThread, NULL);
}

