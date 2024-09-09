
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "tcpserver.h"
#include "comms_protocol.h"
#include "comms_defs.h"
#include "system_defs.h"

#define PORT 20069
#define MAX_CLIENTS 5

#define CLIENT_TIMEOUT 30 //Bin clients after this many seconds.

pthread_t serverThread;
bool bServerRunning = false;

int server_socket;

void transmit_callback(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength)
{
    send(psdcComms->lID, pcData, nLength, 0);
}

void objectReceived_callback(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData)
{
    printf("Client socket %u sent us object ID %u unexpectedly.\n", psdcComms->lID, nObjectID);
}

void commandReceived_callback(struct sdfComms* psdcComms, uint16_t nCommandID)
{
    switch(nCommandID)
    {
        case COMMAND_REQUEST_STATUS:
        {
            uint32_t lLength;
            uint8_t* pStatus;
            _tcpserver_GetStatus(&pStatus, &lLength);
            Comms_SendObject(psdcComms, OBJECT_STATUS, lLength, pStatus);
        }
        break;
        
        case COMMAND_REQUEST_GRID:
        {
            _tcpserver_SetGrid();
        }
        break;
        
        case COMMAND_REQUEST_BATTS:
        {
            _tcpserver_SetBatts();
        }
        break;
        
        case COMMAND_REQUEST_BOOST:
        {
            _tcpserver_SetBoost();
        }
        break;
    
        default: printf("Client socket %u sent unknown command %u.\n", psdcComms->lID, nCommandID);
    }
}

bool lengthCheck_callback(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength)
{
    switch(nObjectID)
    {
        case OBJECT_STATUS:
        {
            return (nLength == sizeof(struct SystemStatus));
        }
        break;
    
        default: printf("Client socket %u requested length check for unknown object %u.\n", psdcComms->lID, nObjectID);
    }
    
    return false;
}

void error_callback(struct sdfComms* psdcComms, uint8_t cErrorCode)
{
    switch(cErrorCode)
    {
        case COMMS_ERROR_MESSAGE_TYPE_INVALID: printf("Client socket %u comms threw error TYPE_INVALID.\n", psdcComms->lID); break;
        case COMMS_ERROR_LENGTH_INVALID: printf("Client socket %u comms threw error LENGTH_INVALID.\n", psdcComms->lID); break;
        default: printf("Client socket %u comms threw an unknown error code.\n", psdcComms->lID);
    }
}

static void *handle_client(void *arg)
{
    pthread_detach(pthread_self());
    
    int client_socket = *((int *)arg);
    bool bClientRunning = true;
    struct sdfComms sdcComms;
    
    Comms_Initialise(&sdcComms,
                     client_socket,
                     transmit_callback,
                     objectReceived_callback,
                     commandReceived_callback,
                     lengthCheck_callback,
                     error_callback);
    
    printf("Client connected.\n");

    struct timeval tv;
    tv.tv_sec = CLIENT_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while(bClientRunning && bServerRunning && (COMMS_ERROR_NONE == Comms_LastError(&sdcComms)))
    {
        uint8_t buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Read data from the client
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received < 0)
        {
            printf("Client socket is dead.\n");
            bClientRunning = false;
        }
        else if(0 == bytes_received)
        {
            printf("Client closed the socket.\n");
            bClientRunning = false;
        }
        else
        {
            Comms_Receive(&sdcComms, buffer, bytes_received);
        }
        
        usleep(10000);
    }
    
    Comms_Deinit(&sdcComms);

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
        
        usleep(10000);
    }

    close(server_socket);
    printf("Server socket closed\n");
    pthread_exit(NULL);
}

bool tcpserver_init()
{
    struct sockaddr_in server_addr;
    
    while(!bServerRunning)
    {
        // Create socket
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("Error creating socket\n");
            sleep(1);
        }
        else
        {
            // Initialize server address
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(PORT);
            server_addr.sin_addr.s_addr = INADDR_ANY;

            // Bind socket to address
            if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                printf("Error binding\n");
                close(server_socket);
                sleep(1);
            }
            else
            {
                // Listen for incoming connections
                if (listen(server_socket, MAX_CLIENTS) == -1) {
                    printf("Error listening\n");
                    close(server_socket);
                    sleep(1);
                }
                else
                {
                    bServerRunning = true;
                
                    if(0 != pthread_create(&serverThread, NULL, &handle_server, NULL))
                    {
                        printf("Error creating server thread\n");
                        close(server_socket);
                        bServerRunning = false;
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void tcpserver_deinit()
{
    bServerRunning = false;
    close(server_socket);
    pthread_cancel(serverThread);
    pthread_join(serverThread, NULL);
}
