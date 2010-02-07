#include "base64.h"

/** @file */

/** The characters of the base64 string */
static const char *base =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(char *str)
{
	int inlen = strlen(str);
	/* calculate the length of the string */
	int outlen = (inlen + 3 - inlen % 3) * 4 / 3 + 1;
	int i;

	char *result = calloc(outlen, sizeof(char));
	char *ptr;

	for (ptr = result; *str != 0; str += i, ptr += 4) {
		/* get the length of the chunk, max 3 */
		for (i = 0; i < 3; i++) {
			if (str[i] == 0)
				break;
		}

		ptr[0] = base[str[0] >> 2];
		ptr[1] = base[((str[0] & 0x03) << 4) | ((str[1] & 0xf0) >> 4)];
		ptr[2] = i < 2 ? '=' :
		    base[((str[1] & 0x0f) << 2) | ((str[2] & 0xc0) >> 6)];
		ptr[3] = i < 3 ? '=' : base[str[2] & 0x3f];
	}

	return result;
}
