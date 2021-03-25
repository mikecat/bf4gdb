#ifndef BFENGINE_H_GUARD_E9356C93_B6A8_473D_894D_5EB5E6514D98
#define BFENGINE_H_GUARD_E9356C93_B6A8_473D_894D_5EB5E6514D98

#include <inttypes.h>

#define BF_STATUS_NORMAL 0x0000

#define BF_STATUS_EXIT 0x01
#define BF_STATUS_MEMORY_RANGE_ERROR 0x02
#define BF_STATUS_BRANCH_ERROR 0x04

struct bfdata;

struct bfdata* bf_init(const uint8_t* program, uint32_t programSize, uint32_t dataNumElements, int dataElementSize);

void bf_free(struct bfdata* bf);

int bf_step(struct bfdata* bf);

#endif
