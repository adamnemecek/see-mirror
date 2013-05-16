
struct header;

void ssp_init(void);
void process_request(FILE *fp, const char *method,
	const char *uri, struct header *headers);

