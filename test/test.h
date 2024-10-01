
#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdint.h>

extern uint32_t lTestsFailed;
extern uint32_t lTestsPassed;

#define ASSERT_EQUAL(expected, actual, ...) \
    do { \
        if ((expected) != (actual)) { \
            lTestsFailed++; \
            printf("*   FAIL: "); \
        } \
        else { \
            lTestsPassed++; \
            printf("    PASS: "); \
        } \
        printf("    "__VA_ARGS__); \
        printf(", file %s, line %d\n", __FILE__, __LINE__); \
    } while (0)

#define PRINT_TEST_RESULTS \
    do { \
        if (lTestsFailed == 0) { \
            printf("All tests passed.\n"); \
        } else { \
            printf("There were %u test failures. %u tests passed.\n", lTestsFailed, lTestsPassed); \
        } \
    } while (0)
    
#define PRINT_DEBUG(...) \
    do { \
        printf(__VA_ARGS__); \
    } while (0)
    
#define FAIL(...) \
    do { \
        lTestsFailed++; \
        printf("*   FAIL: "); \
        printf(__VA_ARGS__); \
        printf(", file %s, line %d\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } while (0)
    
#endif /* TEST_H */

