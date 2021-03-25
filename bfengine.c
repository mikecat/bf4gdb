#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "bfengine.h"

#define SIZE_LIMIT UINT32_C(0x80000000)
#define BRANCH_NEXT_INVALID UINT32_C(0xffffffff)

struct bfdata {
	uint8_t* program;
	uint32_t programSize;
	void* data;
	uint32_t dataNumElements;
	int dataElementSize;

	uint32_t* programNext;
	uint32_t* branchNext;

	uint32_t programPtr;
	uint32_t dataPtr;
};

static int isInstruction(uint8_t c) {
	static const char table[256] = {
		['+'] = 1, ['-'] = 1, ['<'] = 1, ['>'] = 1,
		[','] = 1, ['.'] = 1, ['['] = 1, [']'] = 1
	};
	return table[c];
}

static void constructNextCache(struct bfdata* bf) {
	// use programNext as stack because the size for whole program is allocated
	uint32_t stackCount = 0;
	for (uint32_t i = 0; i < bf->programSize; i++) {
		bf->branchNext[i] = BRANCH_NEXT_INVALID;
		if (bf->program[i] == '[') {
			bf->programNext[stackCount++] = i;
		} else if (bf->program[i] == ']') {
			if (stackCount > 0) {
				stackCount--;
				bf->branchNext[i] = bf->programNext[stackCount];
				bf->branchNext[bf->programNext[stackCount]] = i;
			}
		}
	}
	// after that, construct programNext
	if (bf->programSize > 0) {
		uint32_t next = bf->programSize;
		for (uint32_t i = bf->programSize - 1; i > 0; i--) {
			bf->programNext[i] = next;
			if (isInstruction(bf->program[i])) next = i;
		}
		bf->programNext[0] = next;
	}
}

struct bfdata* bf_init(const uint8_t* program, uint32_t programSize, uint32_t dataNumElements, int dataElementSize) {
	if (program == NULL) return NULL;
	if (dataElementSize != 1 && dataElementSize != 2 && dataElementSize != 4 && dataElementSize != 8) {
		fputs("data element size must be one of 1, 2, 4, 8\n", stderr);
		return NULL;
	}
	if (programSize > SIZE_LIMIT) {
		fputs("program too large\n", stderr);
		return NULL;
	}
	if(dataNumElements > SIZE_LIMIT / (unsigned int)dataElementSize) {
		fputs("data too large\n", stderr);
		return NULL;
	}
	struct bfdata* bf = malloc(sizeof(*bf));
	if (bf == NULL) {
		perror("memory allocation failed");
		return NULL;
	}
	bf->program = malloc(programSize);
	bf->data = calloc(dataNumElements, dataElementSize);
	bf->programNext = malloc(sizeof(bf->programNext[0]) * programSize);
	bf->branchNext = malloc(sizeof(bf->branchNext[0]) * programSize);
	if (bf->program == NULL || bf->data == NULL || bf->programNext == NULL || bf->branchNext == NULL) {
		perror("memory allocation failed");
		free(bf->program);
		free(bf->data);
		free(bf->programNext);
		free(bf->branchNext);
		free(bf);
		return NULL;
	}
	memcpy(bf->program, program, programSize);
	bf->programSize = programSize;
	bf->dataNumElements = dataNumElements;
	bf->dataElementSize = dataElementSize;
	bf->programPtr = 0;
	bf->dataPtr = 0;
	constructNextCache(bf);
	return bf;
}

void bf_free(struct bfdata* bf) {
	if (bf != NULL) {
		free(bf->program);
		free(bf->data);
		free(bf->programNext);
		free(bf->branchNext);
		free(bf);
	}
}

static uint64_t readDataElement(struct bfdata* bf, uint32_t index) {
	switch (bf->dataElementSize) {
		case 1:
			return ((int8_t*)bf->data)[index];
		case 2:
			return ((int16_t*)bf->data)[index];
		case 4:
			return ((int32_t*)bf->data)[index];
		case 8:
			return ((int64_t*)bf->data)[index];
	}
	return 0;
}

static void writeDataElement(struct bfdata* bf, uint32_t index, uint64_t data) {
	switch (bf->dataElementSize) {
		case 1:
			((uint8_t*)bf->data)[index] = (uint8_t)data;
			break;
		case 2:
			((uint16_t*)bf->data)[index] = (uint16_t)data;
			break;
		case 4:
			((uint32_t*)bf->data)[index] = (uint32_t)data;
			break;
		case 8:
			((uint64_t*)bf->data)[index] = data;
			break;
	}
}

int bf_step(struct bfdata* bf) {
	if (bf == NULL) return BF_STATUS_BRANCH_ERROR;
	if (bf->programPtr >= bf->programSize) return BF_STATUS_EXIT;
	uint32_t nextProgramPtr = bf->programNext[bf->programPtr];
	switch (bf->program[bf->programPtr]) {
		case '+':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			writeDataElement(bf, bf->dataPtr, readDataElement(bf, bf->dataPtr) + 1);
			break;
		case '-':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			writeDataElement(bf, bf->dataPtr, readDataElement(bf, bf->dataPtr) - 1);
			break;
		case '<':
			if (bf->dataPtr == 0) return BF_STATUS_MEMORY_RANGE_ERROR;
			bf->dataPtr--;
			break;
		case '>':
			if (bf->dataPtr  + 1 >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			bf->dataPtr++;
			break;
		case ',':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			{
				int in = getchar();
				writeDataElement(bf, bf->dataPtr, in == EOF ? UINT64_C(0xffffffffffffffff) : (unsigned char)in);
			}
			break;
		case '.':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			putchar(readDataElement(bf, bf->dataPtr) & 0xff);
			break;
		case '[':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			if (readDataElement(bf, bf->dataPtr) == 0) {
				uint32_t next = bf->branchNext[bf->programPtr];
				if (next == BRANCH_NEXT_INVALID) return BF_STATUS_BRANCH_ERROR;
				nextProgramPtr = bf->programNext[next];
			}
			break;
		case ']':
			if (bf->dataPtr >= bf->dataNumElements) return BF_STATUS_MEMORY_RANGE_ERROR;
			if (readDataElement(bf, bf->dataPtr) != 0) {
				uint32_t next = bf->branchNext[bf->programPtr];
				if (next == BRANCH_NEXT_INVALID) return BF_STATUS_BRANCH_ERROR;
				nextProgramPtr = bf->programNext[next];
			}
			break;
	}
	bf->programPtr = nextProgramPtr;
	return bf->programPtr < bf->programSize ? BF_STATUS_NORMAL : BF_STATUS_EXIT;
}
