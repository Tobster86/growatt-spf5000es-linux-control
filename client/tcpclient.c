
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "tcpclient.h"

bool tcpclient_init()
{
    // Define the server's IP address and port
    const char* server_ip = "127.0.0.1"; // Replace with the server's IP address
    const int server_port = 20069; // Replace with the server's port

    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        printf("Error creating socket");
        return false;
    }

    // Create a sockaddr_in structure to hold the server's address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        printf("Error converting IP address");
        close(client_socket);
        return false;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        printf("Error connecting to the server");
        close(client_socket);
        return false;
    }

    // Send a message to the server
    const char* message = "Yay!";
    if (send(client_socket, message, strlen(message), 0) == -1)
    {
        printf("Error sending data to the server");
        close(client_socket);
        return false;
    }

    // Receive and print the server's response
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received == -1)
    {
        printf("Error receiving data from the server");
        close(client_socket);
        return false;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received data
    printf("Received from server: %s\n", buffer);

    // Close the socket
    close(client_socket);

    return true;
}
