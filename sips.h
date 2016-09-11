/* sips: a simple ips patcher */

int check_header(FILE *patch);
int read_records(FILE *patch);
int apply_record(FILE *patch, uint16_t size, uint32_t offset);
int apply_record_rle(FILE *patch, uint16_t size, uint32_t offset);
static inline uint16_t byte2_to_uint(uint8_t *bytes);
static inline uint32_t byte3_to_uint(uint8_t *bytes);
int arghandler(int argc, char **argv, FILE **in, FILE **patch, FILE **out);
