//Simple two endpoint comms channel for sending/receiving objects and commands.
//Assumes same endianness at each endpoint. Not thread safe.
//No error recovery. If things go out of sync and an error callback is received, bin the underlying connection and start again.

#ifndef COMMS_PROTOCOL_H
#define COMMS_PROTOCOL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define COMMS_MESSAGE_TYPE_OBJECT   0xAA
#define COMMS_MESSAGE_TYPE_COMMAND  0xAB

#define COMMS_STATE_MESSAGE_TYPE    0x00
#define COMMS_STATE_OBJECT_ID_1     0x10
#define COMMS_STATE_OBJECT_ID_2     0x11
#define COMMS_STATE_COMMAND_ID_1    0x20
#define COMMS_STATE_COMMAND_ID_2    0x21
#define COMMS_STATE_LENGTH_1        0x30
#define COMMS_STATE_LENGTH_2        0x31
#define COMMS_STATE_PAYLOAD         0x40
#define COMMS_STATE_ERROR           0xFF

#define COMMS_OBJECT_HEADER_LENGTH  5
#define COMMS_COMMAND_HEADER_LENGTH 3

#define COMMS_ERROR_MESSAGE_TYPE_INVALID   0x00
#define COMMS_ERROR_LENGTH_INVALID         0x01

struct sdfComms;

typedef void (*Transmit)(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength);
typedef void (*ObjectReceived)(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData);
typedef void (*CommandReceived)(struct sdfComms* psdcComms, uint16_t nCommandID);
typedef bool (*LengthCheck)(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength);
typedef void (*Error)(struct sdfComms* psdcComms, uint8_t cErrorCode);

struct sdfComms
{
    uint8_t cState;
    uint16_t nReceivingID;
    uint16_t nReceivingLength;
    uint8_t* pcPayload;
    uint16_t nPayloadWrite;
    
    uint32_t lID;
    
    Transmit transmit;
    ObjectReceived objectReceived;
    CommandReceived commandReceived;
    LengthCheck lengthCheck;
    Error error;
};

void Comms_Initialise(struct sdfComms* psdcComms,
                      uint32_t lID,
                      Transmit transmit,
                      ObjectReceived objectReceived,
                      CommandReceived commandReceived,
                      LengthCheck lengthCheck,
                      Error error);

void Comms_Receive(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength);
void Comms_SendObject(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData);
void Comms_SendCommand(struct sdfComms* psdcComms, uint16_t nCommandID);

#endif
