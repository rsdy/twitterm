#include "http.h"
#include "base64.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/** @file http.c */

#define NEWLINE "\r\n"
#define HEADER_END "\r\n\r\n"
#define SEND_NEWLINE(x) write((x), NEWLINE, 2)
#define SEND_SPACE(x) write((x), " ", 1)
#define HTTP_PORT_STR "80"

/** Creates a socket and tries to connect to it
 * @param host the host to connect
 * @param port the port to connect (usually HTTP_PORT_STR)
 * @retval fd a file descriptor if succeeded
 * @retval negative value if failed */
static int _socket_connect(char *host, char *port);

/** Disconnects from a socket.
 * Just a wrapper to close(), with portability in mind */
static int _socket_disconnect(int sock);

/** Writes the HTTP header into the given stream.
 * @param sock the socket to use
 * @param host 
 * @param file the file on the host
 * @param method the HTTP request type to use: POST, GET */
static void _http_header_send(int sock, char *host, char *file, char *method);

/** Writes the data necessary for authentication into the given socket
 * @param sock the socket to use
 * @param user
 * @param pwd */
static void _http_auth_send(int sock, char *user, char *pwd);

static int _http_response_handle(int sock, char **output);

int http_get(char *domain, char *file, char **output)
{
	int sock = _socket_connect(domain, HTTP_PORT_STR);
	int ret;

	_http_header_send(sock, domain, file, "GET");
	SEND_NEWLINE(sock);

	ret = _http_response_handle(sock, output);
	_socket_disconnect(sock);
	return ret;
}

int http_get_auth(char *domain, char *file, char **output, char *user,
		  char *pwd)
{
	int sock = _socket_connect(domain, HTTP_PORT_STR);
	int ret;

	_http_header_send(sock, domain, file, "GET");
	_http_auth_send(sock, user, pwd);
	SEND_NEWLINE(sock);

	ret = _http_response_handle(sock, output);
	_socket_disconnect(sock);
	return ret;
}

int http_post_auth(char *domain, char *file, char **output, char *data,
		   char *user, char *pwd)
{
	int datalen;		/* the length of data (last param) */
	char lbuff[10];		/* the string representation of datalen */
	int sock = _socket_connect(domain, HTTP_PORT_STR);
	int ret;

	_http_header_send(sock, domain, file, "POST");
	_http_auth_send(sock, user, pwd);

	datalen = strlen(data);
	sprintf(lbuff, "%d", datalen);	/* convert datalen to string */

	write(sock, "Content-Length: ", 16);
	write(sock, lbuff, strlen(lbuff));
	write(sock, HEADER_END, 4);	/* the end of the header */
	write(sock, data, datalen);	/* the message body */

	ret = _http_response_handle(sock, output);
	_socket_disconnect(sock);
	return ret;
}

/** The size of the buffer to use when reading from the server
	* Note that while the Twitter response header should fit into 1024 bytes,
	* others may not.
	*/
#define BUFSIZE 1024
int _http_response_handle(int sock, char **output)
{
	char buf[BUFSIZE];	/* the buffer to read into */
	char *ptr = NULL;	/* pointer with multiple uses */

	int errcode;		/* HTTP error code */
	int readsize;		/* read size */
	int headsize;		/* header size */
	int bodysize;		/* body size */
	int bodyinbuf;
	int leftover;

	/*
	 * read the header, and get the HTTP return code
	 */
	memset(buf, 0, BUFSIZE);
	headsize = read(sock, buf, BUFSIZE);

	ptr = buf + 9;		/* skip the "HTTP/1.1 " part */
	errcode = atoi(ptr);
	if (errcode != 200 || output == NULL) {
		return errcode;
	}

	ptr = strstr(buf, "Content-Length:");
	if (ptr == NULL) {
		return -1;
	}
	ptr += 15;		/* skip the "Content-Length " part */
	bodysize = atoi(ptr);

	/*
	 * seek to the beginning of the content (in the stream) 
	 */
	ptr = strstr(buf, HEADER_END);
	if (ptr == NULL) {
		return -1;
	}
	ptr += 4;

	/*
	 * this is the size of the body in the buffer
	 */
	bodyinbuf = headsize - ((ptr - buf) * sizeof(char));

	/*
	 * copy the response body into the allocated memory
	 */
	*output = malloc(bodysize * sizeof(**output) + 1);
	(*output)[bodysize] = 0;	/* it's gonna be a string! */
	memcpy(*output, ptr, bodyinbuf);	/* copy the remnants over from buffer */

	/*
	 * download the whole body from the server 
	 */
	ptr = (*output) + bodyinbuf;	/* skip the downloaded part */
	leftover = bodysize - bodyinbuf;
	while ((readsize = read(sock, ptr, leftover)) > 0) {
		leftover -= readsize;
		ptr += readsize;
	}

	/* connection failed, return with NULL */
	if (readsize == -1) {
		free(*output);
		return -1;
	}

	return errcode;
}

void _http_auth_send(int sock, char *user, char *pwd)
{
	char *authstr = NULL;
	char *basestr = NULL;
	int authlen;

	authlen = strlen(user) + strlen(pwd) + 2;
	/*
	 * using calloc since this way the memory is filled with zeros, and
	 * therefore strcat appends the first string to the start 
	 */
	authstr = calloc(authlen, sizeof(*authstr));
	if (authstr == NULL)
		return;

	/*
	 * construct the string which later will be base64 encoded 
	 */
	strcat(authstr, user);
	strcat(authstr, ":");
	strcat(authstr, pwd);
	basestr = base64_encode(authstr);
	free(authstr);

	/*
	 * write out the stuff 
	 */
	write(sock, "Authorization: Basic ", 21);
	write(sock, basestr, strlen(basestr));
	SEND_NEWLINE(sock);
	free(basestr);
}

void _http_header_send(int sock, char *host, char *file, char *method)
{
	write(sock, method, strlen(method));
	SEND_SPACE(sock);
	write(sock, file, strlen(file));
	SEND_SPACE(sock);
	write(sock, "HTTP/1.1" NEWLINE "Host: ", 16);
	write(sock, host, strlen(host));
	write(sock, NEWLINE "Connection: close" NEWLINE, 21);
}

static int _socket_connect(char *host, char *portn)
{
	int sock;
	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *ptr;

	/*
	 * whichever IP version may be used, but only TCP connections wanted 
	 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(host, portn, &hints, &res);
	if (ret != 0) {
		return -1;
	}

	/*
	 * try all the supported methods for establishing connection until we
	 * finally succeed 
	 */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		sock =
		    socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sock == -1) {
			continue;	/* not supported socket type */
		}

		ret = connect(sock, ptr->ai_addr, ptr->ai_addrlen);
		if (ret != -1) {
			break;	/* connection OK, this is the one we want! */
		}

		/* socket was good enough, but connection didn't succeed */
		close(sock);
	}

	/*
	 * clean up 
	 */
	freeaddrinfo(res);
	return sock;
}

static int _socket_disconnect(int socket)
{
	return close(socket);
}
