#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define READ_BUFFER_SIZE 4096
#define READ_SIZE_LIMIT UINT32_C(0x80000000)

uint8_t* readFile(uint32_t* size, const char* fileName) {
	uint8_t* data = NULL;
	uint32_t readSize = 0;
	FILE* fp;
	if (size == NULL || fileName == NULL) return NULL;
	fp = fopen(fileName, "rb");
	if (fp == NULL) {
		perror("input file open failed");
		return NULL;
	}
	readSize = 0;
	for (;;) {
		uint8_t* newData = realloc(data, readSize + READ_BUFFER_SIZE);
		if (newData == NULL) {
			perror("realloc failed");
			fclose(fp);
			free(data);
			return NULL;
		}
		data = newData;
		size_t ret = fread(data + readSize, 1, READ_BUFFER_SIZE, fp);
		readSize += ret;
		if (readSize >= READ_SIZE_LIMIT) {
			fputs("input file too big\n", stderr);
			fclose(fp);
			free(data);
			return NULL;
		}
		if (ret < READ_BUFFER_SIZE) break;
	}
	fclose(fp);
	*size = readSize;
	return data;
}
