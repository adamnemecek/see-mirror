
/* David Leonard, 2006. Public domain. */

#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>

#include "httpd.h"
#include "ssp.h"

/*
 * A simple, threaded HTTP server.
 * This is just for demonstrating the SSP layer. 
 * It's not a very standard-conformant HTTP server.
 */


#define PORT	"8000"

/* Prototypes */
static int getline(FILE *f, char *buf, size_t bufsz);
static void free_headers(struct header *header);
static int read_headers(FILE *f, struct header **headerp);
static void *request_thread(void *arg);
static void *server_thread(void *arg);
static void create_server_threads(const char *service);

int sflag = 0;

/*
 * Reads a line up to the next CRLF from a FILE. Strips the CRLF.
 * Returns 0 on success
 */
static int
getline(f, buf, bufsz)
	FILE *f;
	char *buf;
	size_t bufsz;
{
	int ch;
	int pos;

	for (pos = 0; (ch = fgetc(f)) != EOF;) {
		if (ch == '\r') {
			ch = fgetc(f);
			if (ch != '\n') {
				warnx("getline: expected LF after CR");
				return -1;
			}
			buf[pos] = 0;
			return 0;
		}
		if (pos >= bufsz - 1) {
			warnx("getline: out of space");
			return -1;
		}
		buf[pos++] = ch;
	}
	warnx("getline: bad read");
	return -1;
}

/* Releases storage allocated for a header list */
static void
free_headers(header)
	struct header *header;
{
	struct header *h;

	while (header) {
		h = header;
		header = h->next;
		free(h->name);
		free(h->value);
		free(h);
	}
}

/*
 * Reads header lines from the HTTP request until a blank line,
 * and creates a reverse linked list of the headers.
 */
static int
read_headers(f, headerp)
	FILE *f;
	struct header **headerp;
{
	struct header *header, *h;
	char buf[8192];
	char *p;

	header = NULL;
	for (;;) {
		if (getline(f, buf, sizeof buf))
			goto fail;
		if (!buf[0])
			break;
		if (buf[0] == ' ' || buf[0] == '\t') {
			/* Append a line to the last header */
			if (!header) {
				warnx("bad header");
				goto fail;
			}
			p = (char *)malloc(strlen(header->value) + 
				   strlen(buf) + 1);
			if (!p) {
				warnx("malloc");
				goto fail;
			}
			strcpy(p, header->value);
			strcat(p, buf);
			free(header->value);
			header->value = p;
			continue;
		}
		/* Search for colon, and turn label to lowercase */
		for (p = buf; *p != ':'; p++) {
			if (!*p) {
				warnx("missing colon");
				goto fail;
			}
			if (*p >= 'A' && *p <= 'Z')
				*p = *p - 'A' + 'a';
		}
		*p++ = 0;
		while (*p == ' '|| *p == '\t') p++;

		h = (struct header *)malloc(sizeof (struct header));
		if (!h) {
			warnx("malloc");
			goto fail;
		}
		h->name = strdup(buf);
		if (!h->name) {
			warnx("malloc");
			free(h);
			goto fail;
		}
		h->value = strdup(p);
		if (!h->value) {
			warnx("malloc");
			free(h->name);
			free(h);
			goto fail;
		}
		h->next = header;
		header = h;
	}

	*headerp = header;
	return 0;

fail:
	free_headers(header);
	return -1;
}

/*
 * Services a request on a socket.
 * Reads the HTTP request line and the headers.
 * Argument is a pointer to a file descriptor.
 * The pointer will be freed immediately.
 * The file descriptor will be closed when the request completes.
 */
static void *
request_thread(arg)
	void *arg;
{
	int t;
	FILE *tf;
	char line[1024], *p;
	const char *method, *uri, *version;
	struct header *header = NULL;

	t = *(int *)arg;
	free(arg);

	tf = fdopen(t, "r+b");
	if (!tf) {
		warn("fdopen");
		close(t);
		return NULL;
	}

	if (getline(tf, line, sizeof line))
		goto fail;

	method = line;
	p = strchr(line, ' ');
	if (!p || !*p) {
		warnx("bad req: %s", line);
		goto fail;
	}
	*p++ = '\0';

	uri = p;
	p = strchr(uri, ' ');
	if (!p || !*p)
		version = "";
	else {
		*p++ = '\0';
		version = p;
	}

	printf("method: '%s'\nuri   : '%s'\nversion: '%s'\n",
		method, uri, version);

	header = NULL;
	if (read_headers(tf, &header))
		goto fail;

	process_request(tf, method, uri, header);
	goto out;

fail:
	fprintf(tf, "HTTP/1.0 500 Internal error \r\n\r\n");
out:
	free_headers(header);
	fclose(tf);
	return NULL;
}

/*
 * Accepts connections on a socket and starts new threads to handle them
 * Argument is pointer to a file descriptor. Pointer will be freed.
 */
static void *
server_thread(arg)
	void *arg;
{
	int s, t, *tp, error;
	struct sockaddr addr;
	socklen_t addrlen;
	pthread_t thread;
	pthread_attr_t attr;

	s = *(int *)arg;
	free(arg);

	printf("server listening on fd %d\n", s);

	error = pthread_attr_init(&attr);
	if (error) {
		warnx("pthread_attr_init: %s", strerror(error));
		close(s);
		return NULL;
	}

	error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (error) {
		warnx("pthread_attr_setdetachstate: %s", strerror(error));
		pthread_attr_destroy(&attr);
		close(s);
		return NULL;
	}

	for (;;) {
		addrlen = sizeof addr;
		t = accept(s, &addr, &addrlen);
		if (t < 0) {
			warn("accept");
			continue;
		}
		tp = (int *)malloc(sizeof (int));
		if (!tp) {
			warnx("malloc");
			close(t);
			continue;
		}
		*tp = t;
		if (sflag) { request_thread(tp); return NULL; }
		error = pthread_create(&thread, &attr, request_thread, tp);
		if (error) {
			warnx("pthread_create: %s", strerror(error));
			close(t);
			free(tp);
		}
	}

	return NULL;
}

/*
 * Creates server sockets, and a server thread for each.
 */
static void
create_server_threads(service)
	const char *service;
{
	int s;
	int *sp;
	int error;
	int opt;
	pthread_t thread;
	struct addrinfo hints, *res, *res0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = sflag ? PF_INET : PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo(NULL, service, &hints, &res0);
	if (error) {
		errx(1, "%s", gai_strerror(error));
		/*NOTREACHED*/
	}
	for (res = res0; res; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (s < 0) {
			warn("socket");
			continue;
		}

		if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
			warn("bind");
			close(s);
			continue;
		}

		(void) listen(s, 5);

		printf("listening on port %s\n", service);

		opt = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, 
		    sizeof opt) < 0)
			warn("setsockopt SO_REUSEADDR");

		opt = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &opt,
		    sizeof opt) < 0)
			warn("setsockopt SO_REUSEPORT");

		sp = (int *)malloc(sizeof (int));
		if (!sp)
			errx(1, "malloc");
		*sp = s;
		if (sflag) { server_thread(sp); return; }
		error = pthread_create(&thread, NULL, server_thread, sp);
		if (error) {
			warnx("pthread_create: %s", strerror(error));
			close(s);
			free(sp);
		}
	}
	freeaddrinfo(res0);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	ssp_init();
	create_server_threads(PORT);
	pthread_exit(NULL);
}
