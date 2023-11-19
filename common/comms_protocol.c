
#include "comms_protocol.h"

static void Comms_Error(struct sdfComms* psdcComms, uint8_t cErrorCode)
{
    if(NULL != psdcComms->pcPayload)
    {
        free(psdcComms->pcPayload);
        psdcComms->pcPayload = NULL;
    }
        
    psdcComms->cState = COMMS_STATE_ERROR;
    psdcComms->cLastError = cErrorCode;
    psdcComms->error(psdcComms, cErrorCode);
}

static void Comms_FinishMessage(struct sdfComms* psdcComms)
{
    psdcComms->cState = COMMS_STATE_MESSAGE_TYPE;
    psdcComms->nReceivingID = 0x0000;
    psdcComms->nReceivingLength = 0x0000;
    psdcComms->nPayloadWrite = 0x0000;
    
    if(NULL != psdcComms->pcPayload)
    {
        free(psdcComms->pcPayload);
        psdcComms->pcPayload = NULL;
    }
}

void Comms_Initialise(struct sdfComms* psdcComms,
                      uint32_t lID,
                      Transmit transmit,
                      ObjectReceived objectReceived,
                      CommandReceived commandReceived,
                      LengthCheck lengthCheck,
                      Error error)
{
    memset(psdcComms, 0x00, sizeof(struct sdfComms));
    psdcComms->lID = lID;
    psdcComms->transmit = transmit;
    psdcComms->objectReceived = objectReceived;
    psdcComms->commandReceived = commandReceived;
    psdcComms->lengthCheck = lengthCheck;
    psdcComms->error = error;
}

void Comms_Receive(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength)
{
    for(uint16_t i = 0; i < nLength; i++)
    {
        switch(psdcComms->cState)
        {
            case COMMS_STATE_MESSAGE_TYPE:
            {
                if(COMMS_MESSAGE_TYPE_OBJECT == pcData[i])
                    psdcComms->cState = COMMS_STATE_OBJECT_ID_1;
                else if(COMMS_MESSAGE_TYPE_COMMAND == pcData[i])
                    psdcComms->cState = COMMS_STATE_COMMAND_ID_1;
                else
                    Comms_Error(psdcComms, COMMS_ERROR_MESSAGE_TYPE_INVALID);
            }
            break;
            
            case COMMS_STATE_OBJECT_ID_1:
            case COMMS_STATE_COMMAND_ID_1:
            {
                psdcComms->nReceivingID = pcData[i];
                psdcComms->cState++;
            }
            break;
            
            case COMMS_STATE_OBJECT_ID_2:
            {
                psdcComms->nReceivingID |= (uint16_t)pcData[i] << 8;
                psdcComms->cState = COMMS_STATE_LENGTH_1;
            }
            break;
            
            case COMMS_STATE_COMMAND_ID_2:
            {
                psdcComms->nReceivingID |= (uint16_t)pcData[i] << 8;
                psdcComms->commandReceived(psdcComms, psdcComms->nReceivingID);
                Comms_FinishMessage(psdcComms);
            }
            break;
            
            case COMMS_STATE_LENGTH_1:
            {
                psdcComms->nReceivingLength = pcData[i];
                psdcComms->cState++;
            }
            break;
            
            case COMMS_STATE_LENGTH_2:
            {
                psdcComms->nReceivingLength |= (uint16_t)pcData[i] << 8;
                psdcComms->pcPayload = (uint8_t*)malloc(psdcComms->nReceivingLength);
                
                if(psdcComms->lengthCheck(psdcComms, psdcComms->nReceivingID, psdcComms->nReceivingLength))
                    psdcComms->cState = COMMS_STATE_PAYLOAD;
                else
                    Comms_Error(psdcComms, COMMS_ERROR_LENGTH_INVALID);
            }
            break;
            
            case COMMS_STATE_PAYLOAD:
            {
                uint8_t cBytesLeft = nLength - i;
                
                if(cBytesLeft > psdcComms->nReceivingLength)
                    cBytesLeft = psdcComms->nReceivingLength;
                    
                memcpy(&psdcComms->pcPayload[psdcComms->nPayloadWrite], &pcData[i], cBytesLeft);
                psdcComms->nPayloadWrite += cBytesLeft;
                psdcComms->nReceivingLength -= cBytesLeft;
                i += cBytesLeft;
                
                if(0 == psdcComms->nReceivingLength)
                {
                    psdcComms->objectReceived(psdcComms, psdcComms->nReceivingID, psdcComms->nPayloadWrite, psdcComms->pcPayload);
                    Comms_FinishMessage(psdcComms);
                }
            }
            break;

            default: { /* COMMS_STATE_ERROR or corrupted, do nothing. */ }
        }
    }
}

void Comms_SendObject(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData)
{
    uint8_t* pcSendData = (uint8_t*)malloc(COMMS_OBJECT_HEADER_LENGTH + nLength);
    pcSendData[0] = COMMS_MESSAGE_TYPE_OBJECT;
    memcpy(&pcSendData[1], &nObjectID, sizeof(uint16_t));
    memcpy(&pcSendData[3], &nLength, sizeof(uint16_t));
    memcpy(&pcSendData[COMMS_OBJECT_HEADER_LENGTH], pcData, nLength);
    
    psdcComms->transmit(psdcComms, pcSendData, COMMS_OBJECT_HEADER_LENGTH + nLength);
    
    free(pcSendData);
}

void Comms_SendCommand(struct sdfComms* psdcComms, uint16_t nCommandID)
{
    uint8_t* pcSendData = (uint8_t*)malloc(COMMS_COMMAND_HEADER_LENGTH);
    pcSendData[0] = COMMS_MESSAGE_TYPE_COMMAND;
    memcpy(&pcSendData[1], &nCommandID, sizeof(uint16_t));
    
    psdcComms->transmit(psdcComms, pcSendData, COMMS_COMMAND_HEADER_LENGTH);
    
    free(pcSendData);
}

uint8_t Comms_LastError(struct sdfComms* psdcComms)
{
    return psdcComms->cLastError;
}

void Comms_Deinit(struct sdfComms* psdcComms)
{
    if(NULL != psdcComms->pcPayload)
    {
        free(psdcComms->pcPayload);
        psdcComms->pcPayload = NULL;
    }
}


