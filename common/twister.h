#include "stdint.h"


#ifdef __cplusplus
extern "C" {
#endif

void seedMT(uint32_t seed);
uint32_t reloadMT(void);
uint32_t randomMT(void);

#ifdef __cplusplus
}
#endif