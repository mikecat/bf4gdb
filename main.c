#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "fileio.h"
#include "bfengine.h"

enum option_type {
	OPTION_TYPE_EXISTANCE,
	OPTION_TYPE_INTEGER,
	OPTION_TYPE_STRING
};

struct option_info {
	const char* name[2];
	enum option_type type;
	char* optionRead;
	long integerRead;
};

enum option_index {
	OPTION_HELP = 0
};

int main(int argc, char* argv[]) {
	struct option_info options[] = {
		[OPTION_HELP] = {{"-h", "--help"}, OPTION_TYPE_EXISTANCE, NULL, 0}
	};
	char* programFileName = NULL;
	int invalid = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			int found = 0;
			for (size_t j = 0; j < sizeof(options) / sizeof(*options); j++) {
				int match = 0;
				for (size_t k = 0; k < sizeof(options[j].name) / sizeof(options[j].name[0]); k++) {
					if (options[j].name[k] != NULL && strcmp(options[j].name[k], argv[i]) == 0) {
						match = 1;
						break;
					}
				}
				if (match) {
					if (options[j].optionRead) {
						fprintf(stderr, "duplicate option: %s\n", argv[i]);
						invalid = 1;
					} else {
						if (options[j].type == OPTION_TYPE_EXISTANCE) {
							options[j].optionRead = argv[i];
						} else {
							if (i + 1 >= argc) {
								fprintf(stderr, "missing parameter for %s\n", argv[i]);
								invalid = 1;
							} else {
								i++;
								options[j].optionRead = argv[i];
								if (options[j].type == OPTION_TYPE_INTEGER) {
									errno = 0;
									char* end;
									options[j].integerRead = strtol(argv[i], &end, 10);
									if (errno != 0 || argv[i][0] == '\0' || *end != '\0') {
										fprintf(stderr, "invalid parameter for %s\n", argv[i - 1]);
										invalid = 1;
									}
								}
							}
						}
					}
					found = 1;
					break;
				}
			}
			if (!found) {
				fprintf(stderr, "unrecognized option: %s\n", argv[i]);
				invalid = 1;
			}
		} else if (programFileName == NULL) {
			programFileName = argv[i];
		} else {
			fprintf(stderr, "invalid argument: %s\n", argv[i]);
			invalid = 1;
		}
	}
	if (programFileName == NULL) {
		fputs("program file not specified\n", stderr);
		invalid = 1;
	}
	if (options[OPTION_HELP].optionRead || invalid) {
		fprintf(stderr, "Usage: %s program_file_name [options]\n", argc > 0 ? argv[0] : "bf4gdb");
		return invalid ? 1 : 0;
	}

	uint32_t programSize = 0;
	uint8_t* program = readFile(&programSize, programFileName);
	if (program == NULL) return 1;

	struct bfdata* bf = bf_init(program, programSize, 0x10000, 1);
	if (bf == NULL) {
		fputs("failed to initialize Brainf*ck engine\n", stderr);
		free(program);
		return 1;
	}

	int status = BF_STATUS_NORMAL;
	for (;;) {
		if (status != BF_STATUS_NORMAL) {
			break;
		}
		status = bf_step(bf);
	}

	bf_free(bf);
	free(program);
	return status == BF_STATUS_EXIT ? 0 : 1;
}
