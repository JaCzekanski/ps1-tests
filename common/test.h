#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TEST {
    bool quiet;
    int failedAssertions;
    int passedAssertions;
};

extern struct TEST __test;
#ifdef __cplusplus
}
#endif

#define assertEquals(given, expected)                        \
[](auto FUNCTION, auto GIVEN, auto EXPECTED) -> bool {       \
    if (GIVEN == EXPECTED) {                                 \
        __test.passedAssertions++;                           \
        if (!__test.quiet) {                                 \
            printf("pass - %s\n", FUNCTION);                 \
        }                                                    \
        return true;                                         \
    } else {                                                 \
        __test.failedAssertions++;                           \
        printf("fail - %s:%d `"#given" == "#expected"`,"     \
               " given: 0x%x, expected: 0x%x\n",             \
               FUNCTION, __LINE__, GIVEN, EXPECTED);         \
        return false;                                        \
    }                                                        \
}(__FUNCTION__, given, expected)

#define TEST_MULTIPLE_BEGIN() \
[]() {                        \
    __test.quiet = true;      \
}()

#define TEST_MULTIPLE_END()                     \
[](auto FUNCTION) {                             \
    bool passed = __test.failedAssertions == 0; \
    __test.quiet = false;                       \
    __test.failedAssertions = 0;                \
    __test.passedAssertions = 0;                \
    if (passed) {                               \
        printf("pass - %s\n", FUNCTION);        \
        return true;                            \
    } else {                                    \
        return false;                           \
    }                                           \
}(__FUNCTION__)
