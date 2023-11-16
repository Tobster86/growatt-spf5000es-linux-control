
#include <stdio.h>

#include "test.h"
#include "comms_protocol.h"

#define ID_BOB 0xAAAAAAAA
#define ID_SUE 0x55555555

#define COMMAND_ID_LOVE_ME              0xEE
#define OBJECT_ID_LOVE_MESSAGE          0x69
#define OBJECT_ID_LOVE_REPLY            0x11

#define EXPECTED_TRANSMIT_CALLBACKS         4
#define EXPECTED_OBJECT_RECEIVED_CALLBACKS  3
#define EXPECTED_COMMAND_RECEIVED_CALLBACKS 1
#define EXPECTED_LENGTH_CHECK_CALLBACKS     3
#define EXPECTED_ERROR_CALLBACKS            0

struct sdfComms sdcCommsBob;
struct sdfComms sdcCommsSue;

char bobsMessage[] = "I love you, Sue.";
char suesMessage[] = "I love you too, Bob.";

int lTransmitCallbacks = 0;
int lObjectReceivedCallbacks = 0;
int lCommandReceivedCallbacks = 0;
int lLengthCheckCallbacks = 0;
int lErrorCallbacks = 0;

bool bSueDontReply = false;

void dump_hex_msg(const char* msg, uint8_t* pcData, uint16_t nLength)
{
    printf("%s", msg);
    
    for(int i = 0; i < nLength; i++)
    {
        printf("%02X ", pcData[i]);
    }
    
    printf("\n");
}

void transmit_callback(struct sdfComms* psdcComms, uint8_t* pcData, uint16_t nLength)
{
    switch(psdcComms->lID)
    {
        case ID_BOB:
        {
            dump_hex_msg("Bob is transmitting: ", pcData, nLength);
        
            int i = 0;
        
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_MESSAGE_TYPE, "Sue's comms state is COMMS_STATE_MESSAGE_TYPE");
            
            Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_OBJECT_ID_1, "Sue's comms state is COMMS_STATE_OBJECT_ID_1");
            
            Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_OBJECT_ID_2, "Sue's comms state is COMMS_STATE_OBJECT_ID_2");
            
            Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_LENGTH_1, "Sue's comms state is COMMS_STATE_LENGTH_1");
            
            Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_LENGTH_2, "Sue's comms state is COMMS_STATE_LENGTH_2");
            
            Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            
            while(i < nLength)
            {
                ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_PAYLOAD, "Sue's comms state is COMMS_STATE_PAYLOAD");
                
                ASSERT_EQUAL(sdcCommsSue.nReceivingLength,
                             nLength - i,
                             "Sue's receiving length should be %u and is %u",
                             nLength - i,
                             sdcCommsSue.nReceivingLength);
                             
                ASSERT_EQUAL(sdcCommsSue.nPayloadWrite,
                             i - COMMS_OBJECT_HEADER_LENGTH,
                             "Sue's payload write should be %u and is %u",
                             i - COMMS_OBJECT_HEADER_LENGTH,
                             sdcCommsSue.nPayloadWrite);
                             
                Comms_Receive(&sdcCommsSue, &pcData[i++], 1);
            }
            
            ASSERT_EQUAL(sdcCommsSue.nReceivingLength,
                         0,
                         "Sue's receiving length should have reset to 0 and is %u",
                         sdcCommsSue.nReceivingLength);
            ASSERT_EQUAL(sdcCommsSue.nPayloadWrite,
                         0,
                         "Sue's payload write should have reset to 0 and is %u",
                         sdcCommsSue.nPayloadWrite);
            ASSERT_EQUAL(sdcCommsSue.cState, COMMS_STATE_MESSAGE_TYPE, "Sue's comms state reset to COMMS_STATE_MESSAGE_TYPE");
        }
        break;
        
        case ID_SUE:
        {
            dump_hex_msg("Sue is transmitting: ", pcData, nLength);
            Comms_Receive(&sdcCommsBob, pcData, nLength);
        }
        break;
        
        default:
        {
            FAIL("Unknown comms ID 0x%08X trying to transmit", psdcComms->lID);
        }
    }
    
    lTransmitCallbacks++;
}

void objectReceived_callback(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength, uint8_t* pcData)
{
    switch(psdcComms->lID)
    {
        case ID_BOB:
        {
            printf("(Bob receives Sue's love)\n");
            ASSERT_EQUAL(nObjectID, OBJECT_ID_LOVE_REPLY, "Bob received Sue's message ID");
            ASSERT_EQUAL(nLength, sizeof(suesMessage), "Sue's message was the correct length");
            ASSERT_EQUAL(memcmp(pcData, suesMessage, nLength), 0, "Sue's message content arrived");
        }
        break;
        
        case ID_SUE:
        {
            printf("(Sue receives Bob's love)\n");
            ASSERT_EQUAL(nObjectID, OBJECT_ID_LOVE_MESSAGE, "Sue received Bob's message ID");
            ASSERT_EQUAL(nLength, sizeof(bobsMessage), "Bob's message was the correct length");
            ASSERT_EQUAL(memcmp(pcData, bobsMessage, nLength), 0, "Bob's message content arrived");
            
            if(!bSueDontReply)
            {
                printf("(Sue sends a love reply)\n");
                Comms_SendObject(&sdcCommsSue, OBJECT_ID_LOVE_REPLY, sizeof(suesMessage), suesMessage);
            }
        }
        break;
        
        default:
        {
            FAIL("Unknown comms ID 0x%08X received object", psdcComms->lID);
        }
    }
    
    lObjectReceivedCallbacks++;
}

void commandReceived_callback(struct sdfComms* psdcComms, uint16_t nCommandID)
{
    ASSERT_EQUAL(nCommandID, COMMAND_ID_LOVE_ME, "Love me command received");
    ASSERT_EQUAL(psdcComms->lID, ID_BOB, "By Bob");
    
    if(COMMAND_ID_LOVE_ME == nCommandID)
    {
        printf("(Bob responds to love command)\n");
        Comms_SendObject(&sdcCommsBob, OBJECT_ID_LOVE_MESSAGE, sizeof(bobsMessage), bobsMessage);
    }

    lCommandReceivedCallbacks++;
}

bool lengthCheck_callback(struct sdfComms* psdcComms, uint16_t nObjectID, uint16_t nLength)
{
    switch(nObjectID)
    {
        case OBJECT_ID_LOVE_MESSAGE:
        {
            ASSERT_EQUAL(nLength, sizeof(bobsMessage), "Bob's message length checks out");
        }
        break;
        
        case OBJECT_ID_LOVE_REPLY:
        {
            ASSERT_EQUAL(nLength, sizeof(suesMessage), "Sues's message length checks out");
        }
        break;
        
        default:
        {
            FAIL("Unknown object ID 0x%04X of length %u", nObjectID, nLength);
        }
    }

    lLengthCheckCallbacks++;
}

void error_callback(struct sdfComms* psdcComms, uint8_t cErrorCode)
{
    lErrorCallbacks++;
}

void test_happy_path()
{
    Comms_Initialise(&sdcCommsBob,
                     ID_BOB,
                     transmit_callback,
                     objectReceived_callback,
                     commandReceived_callback,
                     lengthCheck_callback,
                     error_callback);
                     
    Comms_Initialise(&sdcCommsSue,
                     ID_SUE,
                     transmit_callback,
                     objectReceived_callback,
                     commandReceived_callback,
                     lengthCheck_callback,
                     error_callback);
    
    printf("Test 1 - Bob sends a love message object to Sue, who replies similarly.\n");
    Comms_SendObject(&sdcCommsBob, OBJECT_ID_LOVE_MESSAGE, sizeof(bobsMessage), bobsMessage);
    
    printf("Test 2 - Sue commands that Bob loves her.\n");
    bSueDontReply = true;
    Comms_SendCommand(&sdcCommsSue, COMMAND_ID_LOVE_ME);
    
    ASSERT_EQUAL(lTransmitCallbacks,
                 EXPECTED_TRANSMIT_CALLBACKS,
                 "Correct number of transmit callbacks (exp %d act %d)",
                 EXPECTED_TRANSMIT_CALLBACKS,
                 lTransmitCallbacks);
                 
    ASSERT_EQUAL(lObjectReceivedCallbacks,
                 EXPECTED_OBJECT_RECEIVED_CALLBACKS,
                 "Correct number of object callbacks (exp %d act %d)",
                 EXPECTED_OBJECT_RECEIVED_CALLBACKS,
                 lObjectReceivedCallbacks);
    
    ASSERT_EQUAL(lCommandReceivedCallbacks,
                 EXPECTED_COMMAND_RECEIVED_CALLBACKS,
                 "Correct number of command callbacks (exp %d act %d)",
                 EXPECTED_COMMAND_RECEIVED_CALLBACKS,
                 lCommandReceivedCallbacks);
    
    ASSERT_EQUAL(lLengthCheckCallbacks,
                 EXPECTED_LENGTH_CHECK_CALLBACKS,
                 "Correct number of length check callbacks (exp %d act %d)",
                 EXPECTED_LENGTH_CHECK_CALLBACKS,
                 lLengthCheckCallbacks);
    
    ASSERT_EQUAL(lErrorCallbacks,
                 EXPECTED_ERROR_CALLBACKS,
                 "Correct number of error callbacks (exp %d act %d)",
                 EXPECTED_ERROR_CALLBACKS,
                 lErrorCallbacks);
    
    ASSERT_EQUAL(sdcCommsBob.cState,
                 0x00,
                 "Bob's comms state is idle (exp 0x%02X act 0x%02X)",
                 0x00,
                 sdcCommsBob.cState);
    
    ASSERT_EQUAL(sdcCommsBob.nReceivingID,
                 0x0000,
                 "Bob's receiving ID is 0 (exp 0x%02X act 0x%02X)",
                 0x0000,
                 sdcCommsBob.nReceivingID);
    
    ASSERT_EQUAL(sdcCommsBob.nReceivingLength,
                 0,
                 "Bob's receiving length is 0 (exp %u act %u)",
                 0,
                 sdcCommsBob.nReceivingLength);
    
    ASSERT_EQUAL(sdcCommsBob.nPayloadWrite,
                 0,
                 "Bob's payload write is 0 (exp %u act %u)",
                 0,
                 sdcCommsBob.nPayloadWrite);
    
    ASSERT_EQUAL(sdcCommsSue.cState,
                 0x00,
                 "Sue's comms state is idle (exp 0x%02X act 0x%02X)",
                 0x00,
                 sdcCommsSue.cState);
    
    ASSERT_EQUAL(sdcCommsSue.nReceivingID,
                 0x0000,
                 "Sue's receiving ID is 0 (exp 0x%02X act 0x%02X)",
                 0x0000,
                 sdcCommsSue.nReceivingID);
    
    ASSERT_EQUAL(sdcCommsSue.nReceivingLength,
                 0,
                 "Sue's receiving length is 0 (exp %u act %u)",
                 0,
                 sdcCommsSue.nReceivingLength);
    
    ASSERT_EQUAL(sdcCommsSue.nPayloadWrite,
                 0,
                 "Sue's payload write is 0 (exp %u act %u)",
                 0,
                 sdcCommsSue.nPayloadWrite);
}

int main()
{
    test_happy_path();
    
    PRINT_TEST_RESULTS;
    
    return 0;
}

