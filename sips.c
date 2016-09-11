#include <byteswap.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sips.h"

#define _POSIX_C_SOURCE 200809L
#define USAGE "Usage: sips [-v] <input> <patch> <output>"

int log_flag = 0;
uint8_t *input_bytes;

int check_header(FILE *patch)
{
	uint8_t buf[5];

	fread(buf, 5*sizeof(uint8_t), 1, patch);

	if (strncmp((char*)buf, "PATCH", 5) != 0) {
		puts("Invalid patch file.");
		return -1;
	}
	return 0;
}
int read_records(FILE *patch)
{
	uint8_t offset_bytes[3];
	uint8_t size_bytes[2];
	uint32_t offset = 0;
	uint16_t size = 0;

	if (log_flag)
		printf("Offset\tSize\n");

	for (;;) {
		fread(offset_bytes, 3*sizeof(uint8_t), 1, patch);
		fread(size_bytes, 2*sizeof(uint8_t), 1, patch);

		if (feof(patch))
			break;

		offset = byte3_to_uint(offset_bytes);
		size = byte2_to_uint(size_bytes);

		if (size) {
			apply_record(patch, size, offset);
		} else {
			fread(size_bytes, 2*sizeof(uint8_t), 1, patch);
			size = byte2_to_uint(size_bytes);
			apply_record_rle(patch, size, offset);
		}
	}
	return 0;
}
int apply_record(FILE *patch, uint16_t size, uint32_t offset)
{
	uint8_t patchbyte = 0;

	if (log_flag)
		printf("%06X\t%04X\n", offset, size);
	for (int i = 0; i < size; i++) {
		patchbyte = fgetc(patch);
		input_bytes[offset + i] = patchbyte;
	}
	return 0;
}
int apply_record_rle(FILE *patch, uint16_t rle_size, uint32_t offset)
{
	puts("RLE");
	return 0;
}
inline uint16_t byte2_to_uint(uint8_t *bytes)
{
	uint16_t ret = (((uint16_t)bytes[0] << 8) & 0xFF00) | \
		  (((uint16_t)bytes[1])      & 0x00FF);

	return ret;
}
inline uint32_t byte3_to_uint(uint8_t *bytes)
{
	uint32_t ret =  (((uint32_t)bytes[0] << 16) & 0x00FF0000) | \
			(((uint32_t)bytes[1] << 8) & 0x0000FF00) | \
			(((uint32_t)bytes[2]));
	return ret;
}
int arghandler(int argc, char **argv, FILE **in, FILE **patch, FILE **out)
{
	if (argc < 4 || argc > 5)
		return -1;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0) {
			log_flag = 1;
		}
		else if (!(*in)) {
			*in = fopen(argv[i], "rb");
		}
		else if (!(*patch)) {
			*patch = fopen(argv[i], "rb");
		}
		else if (!(*out)) {
			*out = fopen(argv[i], "wb+");
		}
	}

	if (!(*in) || !(*patch) || !(*out)) {
		return -1;
	}

}
int main(int argc, char **argv)
{
	FILE *in = 0;
	FILE *patch = 0;
	FILE *out = 0;
	size_t input_size;

	if (arghandler(argc, argv, &in, &patch, &out) < 0) {
		puts(USAGE);
		return 0;
	}

	fseek(in, 0, SEEK_END);
	input_size = ftell(in);
	rewind(in);

	input_bytes = malloc(input_size * sizeof(uint8_t));

	fread(input_bytes, input_size, 1, in);
	fclose(in);

	check_header(patch);
	read_records(patch);

	fwrite(input_bytes, input_size, 1, out);
	return 0;
}
