#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/stat.h>

#include "twc.h"

static int openrfd(const char *name)
{
        int fd;

	if (!name) {
		fd = STDIN_FILENO;
	} else {
		fd = open(name, O_RDONLY);
		if (fd == -1)
		        fd = -1;
	}

	return fd;
}

static void count_lines(struct wc *wc, const char *name)
{
	int fd, long_line;
	ssize_t bytes_read;
        char *p, *end, buf[BUFFER_SIZE + 1];

	fd = openrfd(name);
	if (fd == -1) {
		fprintf(stderr, "%s: No such file was found\n",
			name);
	        exit(EXIT_FAILURE);
	}

	long_line = 0;
        wc->lsz = 0;

        for (;;) {
		bytes_read = read(fd, buf, sizeof(buf));
		if (bytes_read <= 0)
		        break;

		p = buf;
		end = p + bytes_read;

		if (long_line) {
			*end = '\n';

			for (; (p = memchr(p, '\n', (size_t)-1)) < end; p++)
				wc->lsz++;
		} else {
			while (p < end)
				wc->lsz += *p++ == '\n';
		}

		long_line = 10 * wc->lsz <= bytes_read;
	}

	close(fd);
}

static void count_chars(struct wc *wc, const char *name)
{
	int fd;
	ssize_t bytes_read;
	char buf[BUFFER_SIZE + 1];
	struct stat st;

	fd = openrfd(name);
	if (fd == -1) {
		fprintf(stderr, "%s: No such file was found\n",
			name);
	        exit(EXIT_FAILURE);
	}

	wc->csz = 0;
	if (fstat(fd, &st) == -1) {
		close(fd);
		perror("fstat()");
	        exit(EXIT_FAILURE);
	}

	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		close(fd);
	        fprintf(stderr, "%s: is a directory\n%jd %s\n",
			name, wc->csz, name);
	        exit(EXIT_FAILURE);
	} else if ((st.st_mode & S_IFMT) == S_IFREG ||
		   ((st.st_mode & S_IFMT) == S_IFLNK)) {
		wc->csz = (intmax_t)st.st_size;
		if (wc->csz == 0)
			goto try_reading_chunks;

		close(fd);
		return;
	} else {
		close(fd);
		fprintf(stderr, "%s: don't know what it is.\n", name);
	        exit(EXIT_FAILURE);
	}

try_reading_chunks:
	while ((bytes_read = read(fd, buf, sizeof(buf))))
		wc->csz += (intmax_t)bytes_read;

	close(fd);
}

static void count_words(struct wc *wc, const char *name, int force)
{
	int fd;
	ssize_t bytes_read;
	char *p, buf[BUFFER_SIZE + 1];
	int gotsp = 0;

        fd = openrfd(name);
	if (fd == -1) {
	        fprintf(stderr, "%s: No such file was found\n",
			name);
	        exit(EXIT_FAILURE);
	}

	wc->wsz = 0;
	if (read(fd, buf, (size_t)1) == -1) {
		close(fd);
		perror("read()");
	        exit(EXIT_FAILURE);
	}

	/* This implementation of count words doesn't not support multibytes.
	   It only checks whether the first byte is a non alphabetic and numeric
	   character.
	   This behaviour is optional.
	*/
	if (!force && !isalnum(*buf)) {
		close(fd);
	        exit(EXIT_FAILURE);
	}

        for (;;) {
		bytes_read = read(fd, buf, sizeof(buf));
		if (bytes_read <= 0)
		        break;

		p = buf;
		bytes_read += 1;
	        while (bytes_read--) {
			if (isspace(*p++)) {
			        gotsp = 1;
			} else if (gotsp) {
				gotsp = 0;
				wc->wsz++;
			}
		}
	}

	close(fd);
}

static void usage(void)
{
	fputs("twc\nusage:\n"
	      " --lines (-l)    count lines\n"
	      " --chars (-c)    count chars\n"
	      " --words (-w)    count words (only alphabetic & numeric chars)\n"
	      " --rbyte (-b)    force to count non alphabetic & numeric chars\n",
	      stdout);
}

int main(int argc, char **argv)
{
	struct wc wc;
	struct opts opts = {
		.opt = 0, .lopt = 0, .copt = 0,
		.wopt = 0, .bopt = 0, .i = 0
	};
        intmax_t s0, s1;

        struct option loptions[] = {
		{ "lines", no_argument, 0, 'l' },
		{ "chars", no_argument, 0, 'c' },
		{ "words", no_argument, 0, 'w' },
		{ "rbyte", no_argument, 0, 'b' },
		{ "help",  no_argument, 0, 'h' }
	};


        s0 = s1 = 0;

	if (argc < 2 ||
	    (argc >= 2 && argv[1][0] == '-'
	     && argv[1][1] == '\0')) {
		usage();
		exit(EXIT_FAILURE);
	}

	if (argc > 2 && argv[1][0] != '-') {
		for (opts.i = 1; opts.i < argc; opts.i++) {
			count_lines(&wc, argv[opts.i]);
		        count_chars(&wc, argv[opts.i]);
			s0 += wc.lsz;
		        s1 += wc.csz;
			fprintf(stdout, "%jd %jd %s\n",
				wc.lsz, wc.csz, argv[opts.i]);
		}
		
		if (opts.i > 2)
			fprintf(stdout,
				"%jd %jd total\n", s0, s1);
		exit(EXIT_SUCCESS);
	}
	
	while ((opts.opt = getopt_long(argc, argv,
				       "lcwbh", loptions, NULL)) != -1) {
		switch (opts.opt) {
		case 'l':
			opts.lopt = 1;
			break;

		case 'c':
			opts.copt = 1;
			break;

		case 'w':
			opts.wopt = 1;
			break;

		case 'b':
			opts.bopt = 1;
			break;

		case 'h':
			usage();
		        FALLTHROUGH;

		default:
			exit(EXIT_SUCCESS);
		}
	}

	if (!opts.wopt && opts.bopt) {
	        fputs("Error: You must use this option "
		      "alongside --words (-w)\n",
		      stderr);
		exit(EXIT_FAILURE);
	}

	if (opts.lopt) {
	        if (argc <= 2) {
			count_lines(&wc, NULL);
			fprintf(stdout, "%jd\n", wc.lsz);
			exit(EXIT_SUCCESS);
		}

		for (opts.i = 2; opts.i < argc; opts.i++) {
			count_lines(&wc, argv[opts.i]);
		        s0 += wc.lsz; /* sum */
			fprintf(stdout, "%jd %s\n",
				wc.lsz, argv[opts.i]);
		}
	}

	if (opts.copt) {
		if (argc <= 2) {
			count_chars(&wc, NULL);
			fprintf(stdout, "%jd\n", wc.csz);
			exit(EXIT_SUCCESS);
		}

		for (opts.i = 2; opts.i < argc; opts.i++) {
			count_chars(&wc, argv[opts.i]);
			s0 += wc.csz; /* sum */
			fprintf(stdout, "%jd %s\n",
				wc.csz, argv[opts.i]);
		}
	}

	if (opts.wopt) {
		if (argc <= 2) {
			count_words(&wc, NULL, opts.bopt);
			fprintf(stdout, "%jd\n", wc.wsz);
			exit(EXIT_SUCCESS);
		}

	        for (opts.i = 2; opts.i < argc; opts.i++) {
			count_words(&wc, argv[opts.i], opts.bopt);
		        s0 += wc.wsz; /* sum */
			fprintf(stdout, "%jd %s\n",
				wc.wsz, argv[opts.i]);
		}
	}

	if (opts.i > 3)
		fprintf(stdout, "%jd total\n", s0);
}
