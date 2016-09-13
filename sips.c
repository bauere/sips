/* sips: a simple ips patcher */
/* See LICENSE for license details. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define _POSIX_C_SOURCE 200809L

static int log_flag = 0;
static uint8_t *input_bytes;
static size_t input_size;

/* Prevent big endian byte swaps. */
uint16_t
byte2_to_uint(uint8_t *bytes)
{
	return (((uint16_t)bytes[0] << 8) & 0xFF00) | \
	       (((uint16_t)bytes[1])      & 0x00FF);
}
uint32_t
byte3_to_uint(uint8_t *bytes)
{
	return (((uint32_t)bytes[0] << 16) & 0x00FF0000) | \
	       (((uint32_t)bytes[1] << 8) & 0x0000FF00) | \
	       (((uint32_t)bytes[2]));
}
int
apply_record(FILE *patch, uint16_t size, uint32_t offset)
{
	uint8_t patchbyte;

	if (log_flag) {
		printf("%06X\t%04X\tno\tn/a\t\t%06X\n", \
		       offset, size, input_size);
	}
	if ((offset + size) > input_size) {
		input_bytes = realloc(input_bytes, offset + size);
		if (!input_bytes)
			return -1;

		input_size = offset + size;
	}
	for (int i = 0; i < size; i++) {
		patchbyte = fgetc(patch);
		input_bytes[offset + i] = patchbyte;
	}
	return 0;
}
int
apply_record_rle(FILE *patch, uint16_t size, uint32_t offset)
{
	uint8_t patchbyte = fgetc(patch);

	if (log_flag) {
		printf("%06X\t0000\tyes\t%04X\t\t%06X\n", \
		       offset, size, input_size);
	}
	if ((offset + size) > input_size) {
		input_bytes = realloc(input_bytes, offset + size);
		if (!input_bytes)
			return -1;

		input_size = offset + size;
	}
	for (int i = 0; i < size; i++)
		input_bytes[offset + i] = patchbyte;
	return 0;
}
int
check_header(FILE *patch)
{
	uint8_t buf[5];

	fread(buf, 5*sizeof(uint8_t), 1, patch);

	if (strncmp((char*)buf, "PATCH", 5) != 0) {
		puts("Invalid patch file.");
		return -1;
	}
	return 0;
}
int
read_records(FILE *patch)
{
	uint8_t offset_bytes[3];
	uint8_t size_bytes[2];
	uint32_t offset;
	uint16_t size;

	if (log_flag)
		printf("Offset\tSize\tRLE\tRLE Size\tCurrent output size\n");

	for (;;) {
		fread(offset_bytes, 3*sizeof(uint8_t), 1, patch);
		fread(size_bytes, 2*sizeof(uint8_t), 1, patch);

		if (feof(patch))
			break;

		offset = byte3_to_uint(offset_bytes);
		size = byte2_to_uint(size_bytes);

		if (size) {
			/* Failed realloc. */
			if (apply_record(patch, size, offset) < 0)
				return -1;
		} else {
			fread(size_bytes, 2*sizeof(uint8_t), 1, patch);
			size = byte2_to_uint(size_bytes);
			/* Failed realloc. */
			if (apply_record_rle(patch, size, offset) < 0)
				return -1;
		}
	}
	return 0;
}
int
arghandler(int argc, char **argv, FILE **in, FILE **patch, FILE **out)
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
	/* If a file pointer hasn't been successfully created, error. */
	if (!(*in) || !(*patch) || !(*out)) {
		return -1;
	}

}
int
main(int argc, char **argv)
{
	FILE *in = 0;
	FILE *patch = 0;
	FILE *out = 0;

	if (arghandler(argc, argv, &in, &patch, &out) < 0) {
		puts("Usage: sips [-v] <input> <patch> <output>");
		return -1;
	}

	fseek(in, 0, SEEK_END);
	input_size = ftell(in);
	rewind(in);

	input_bytes = malloc(input_size * sizeof(uint8_t));

	fread(input_bytes, input_size, 1, in);
	fclose(in);

	if (check_header(patch) < 0) {
		puts("Invalid patch file.");
		return -1;
	}
	if (read_records(patch) < 0) {
		puts("Memory allocation error.");
		return -1;
	}

	fwrite(input_bytes, input_size, 1, out);

	fclose(patch);
	fclose(out);
	free(input_bytes);

	return 0;
}
