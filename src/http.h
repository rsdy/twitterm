#ifndef __HTTP_H
#define __HTTP_H
#include "main.h"

/** @file */

/** Sends an HTTP GET request to the server w/o authentication
 * @param domain the name of the server
 * @param file the file to request
 * @return the content of file (sans HTTP header)
 */
int http_get(char *domain, char *file, char **output);

/** Sends an HTTP GET to the server with authentication
 * @param domain the name of the server
 * @param file the file to request
 * @param user: username
 * @param pwd: password
 * @return the content of file (sans HTTP header)
 */
int http_get_auth(char *domain, char *file, char **output, char *user,
		  char *pwd);

/** Sends an HTTP POST to the server with authentication
 * @param domain the name of the server
 * @param file the file to request
 * @param user username
 * @param pwd password
 * @param data the message body
 * @return the content of file (sans HTTP header)
 */
int http_post_auth(char *domain, char *file, char **output, char *data,
		   char *user, char *pwd);

#endif
