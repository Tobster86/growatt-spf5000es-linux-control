
#include "test.h"
#include "test_comms_protocol.h"
#include "test_utils.h"

uint32_t lTestsFailed = 0;
uint32_t lTestsPassed = 0;

int main()
{
    test_comms_protocol();
    test_utils();
    
    PRINT_TEST_RESULTS;
    
    return 0;
}

