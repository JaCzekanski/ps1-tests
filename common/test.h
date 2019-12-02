#pragma once

#define assertEquals(given, expected)                     \
do {                                                      \
    auto GIVEN = (given);                                 \
    auto EXPECTED = (expected);                           \
    if (GIVEN == EXPECTED) {                              \
        printf("pass - %s\n", __FUNCTION__);              \
    } else {                                              \
        printf("fail - %s:%d `"#given" == "#expected"`,"  \
               " given: 0x%x, expected: 0x%x\n",          \
               __FUNCTION__, __LINE__, GIVEN, EXPECTED);  \
    }                                                     \
} while(0)
