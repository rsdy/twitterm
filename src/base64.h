#ifndef __BASE64_H
#define __BASE64_H

/** @file */
#include "main.h"

/** Encodes a string into base64.
 *
 * The function allocates the memory for itself!
 * @param to_encode the string to encode
 * @return the pointer to the encoded string
 */
char *base64_encode(char *to_encode);

#endif
