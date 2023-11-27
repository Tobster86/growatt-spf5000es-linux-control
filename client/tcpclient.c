
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "tcpclient.h"
#include "comms_defs.h"
#include "system_defs.h"
#include "comms_protocol.h"

pthread_t clientThread;
int client_socket;

const char* server_ip = "192.168.8.69";
const int server_port = 20069;

struct sdfComms sdcComms;

enum ClientState
{
    INIT,
    PROCESS,
    DEINIT,
    DIE
};

enum ClientState clientState = INIT;

void transmit_callback(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength)
{
    if(PROCESS == clientState)
    {
        printf("Sending some command data...\n");
        
        //Ignore errors - let the receive thread handle problems with the socket.
        send(client_socket, pcData, nLength, 0);
    }
}

void objectReceived_callback(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData)
{
    switch(nObjectID)
    {
        case OBJECT_STATUS:
        {
            ReceiveStatus(pcData, nLength);
        }
        break;
    
        default: printf("Socket sent unknown object %u.\n", nObjectID);
    }
}

void commandReceived_callback(struct sdfComms* psdcComms, uint16_t nCommandID)
{
    printf("Server sent us command ID %u unexpectedly.\n", nCommandID);
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
    
        default: printf("Socket requested length check for unknown object %u.\n", nObjectID);
    }
    
    return false;
}

void error_callback(struct sdfComms* psdcComms, uint8_t cErrorCode)
{
    switch(cErrorCode)
    {
        case COMMS_ERROR_MESSAGE_TYPE_INVALID: printf("Socket comms threw error TYPE_INVALID.\n"); break;
        case COMMS_ERROR_LENGTH_INVALID: printf("Socket comms threw error LENGTH_INVALID.\n"); break;
        default: printf("Socket comms threw an unknown error code.\n");
    }
    
    close(client_socket);
    Comms_Deinit(&sdcComms);
    clientState = INIT;
}

void *handle_client(void *arg)
{
    uint32_t lConnections = 0;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        printf("Error converting IP address\n");
        clientState = DIE;
    }

    while(DIE != clientState)
    {
        switch(clientState)
        {
            case INIT:
            {
                sleep(1);
                
                client_socket = socket(AF_INET, SOCK_STREAM, 0);
                if (client_socket == -1)
                {
                    printf("Error creating socket\n");
                    break;
                }
                
                if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
                {
                    printf("Error connecting to the server\n");
                    close(client_socket);
                    break;
                }
                
                Comms_Initialise(&sdcComms,
                                 lConnections++,
                                 transmit_callback,
                                 objectReceived_callback,
                                 commandReceived_callback,
                                 lengthCheck_callback,
                                 error_callback);
                                 
                printf("Comms initialised. Going to processing.\n");
                clientState = PROCESS;
            }
            break;
            
            case PROCESS:
            {
                // Receive and print the server's response
                uint8_t buffer[1024];
                ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytes_received == -1)
                {
                    printf("Error receiving data from the server\n");
                    close(client_socket);
                    Comms_Deinit(&sdcComms);
                    clientState = INIT;
                }
                else if(0 == bytes_received)
                {
                    printf("Client closed the socket.\n");
                    close(client_socket);
                    Comms_Deinit(&sdcComms);
                    clientState = INIT;
                }
                else
                {
                    printf("Received some data from the server.\n");
                    Comms_Receive(&sdcComms, &buffer[0], bytes_received);
                }
            }
            break;
            
            case DEINIT:
            {
                close(client_socket);
                Comms_Deinit(&sdcComms);
                clientState = DIE;
            }
            break;

            default: {}
        }
    }
    
    pthread_exit(NULL);
}

bool tcpclient_GetConnected()
{
    return (PROCESS == clientState);
}

void tcpclient_SendCommand(uint16_t nCommandID)
{    
    Comms_SendCommand(&sdcComms, nCommandID);
}

bool tcpclient_init()
{
    if(0 != pthread_create(&clientThread, NULL, &handle_client, NULL))
    {
        printf("Error creating client thread\n");
        return false;
    }

    return true;
}
