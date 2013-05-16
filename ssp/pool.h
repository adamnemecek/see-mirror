
struct pool;

struct pool *pool_new(void);
void pool_destroy(struct pool *);
void *pool_malloc(struct pool *, size_t);

