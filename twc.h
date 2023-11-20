#ifndef TWC_H
#define TWC_H

#undef FALLTHROUGH
#define FALLTHROUGH    __attribute__((fallthrough))

#define BUFFER_SIZE    (1024 * 16)

struct wc {
	intmax_t lsz;
	intmax_t csz;
	intmax_t wsz;
};

struct opts {
	int opt;
	int lopt;
	int copt;
	int wopt;
	int bopt;
	int i;
};

#endif /* TWC_H */
