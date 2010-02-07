#include "json.h"
#include "main.h"
#include <stdio.h>

/** @file */

/** The number format in which the output is produced */
#define NUMFORMAT "%.10g"

/** Writes the string representation of a json_element to the given character array starting by index pos with length of len
 * @param elem the object to process
 * @param json the array in which to write
 * @param length the length of the array
 * @param pos the position where it should start writing */
static int _json_object_to_string(json_element elem, char *json, int length,
				  int pos);

/** Computes the length of the output string
 * @param elem the element which has to be processed
 * @return the length of the string representation */
static int _json_get_chain_length(json_element elem);

/** Writes the source string to the destination, embracing it within quotation marks
 * @param dst the destination array
 * @param src the source array
 *	@param pos the position where it should start writing */
static int _string_print(char *dst, char *src, int pos);

char *json_to_string(json_element elem)
{
int length = _json_get_chain_length(elem);
char *json = calloc(length + 1, sizeof(*json));

	_json_object_to_string(elem, json, length, 0);
	return json;
}

int _json_object_to_string(json_element elem, char *json, int length, int pos)
{
	json_element current;

	for (current = elem;; current = current->next) {
		if (current->name != NULL) {
			pos = _string_print(json, current->name, pos);
			json[pos++] = ':';	/* keys and values are separated by colons */
		}

		switch (current->type) {	/* copy stuff into the json string */
		case JSON_ARRAY:
		case JSON_OBJECT:
			json[pos++] = current->type;	/* here put the bracket start */
			pos =
			    _json_object_to_string(current->data, json, length,
						   pos);
			json[pos++] = current->type + 2;	/* here put the bracket end */
			break;
		case JSON_NULL:
			strcat((json + pos), "null");
			pos += 4;
			break;
		case JSON_TRUE:
			strcat((json + pos), "true");
			pos += 4;
			break;
		case JSON_FALSE:
			strcat((json + pos), "false");
			pos += 5;
			break;
		case JSON_STRING:
			pos = _string_print(json, current->data, pos);
			break;
		case JSON_NUM:
			sprintf((json + pos), NUMFORMAT,
				*(double *) current->data);
			pos += strlen((json + pos));
		default:
			break;
		}

		if (current->next != NULL)
			json[pos++] = ',';
		else
			break;
	}

	return pos;
}

int _json_get_chain_length(json_element elem)
{
static char buf[30];
int len;
json_element current = elem;

	for (len = 0; current != NULL; current = current->next, len++) {
		if (current->name != NULL)
			len += strlen(current->name) + 3;	/* "": */
		switch (current->type) {
		case JSON_ARRAY:
		case JSON_OBJECT:
			len += _json_get_chain_length(current->data) + 2;
			break;
		case JSON_STRING:
			len += strlen(current->data) + 2;
			break;
		case JSON_FALSE:
			len += 5;
			break;
		case JSON_NULL:
		case JSON_TRUE:
			len += 4;
			break;
		case JSON_NUM:	/* this is NASTY. I don't know of a better way, though */
			sprintf(buf, NUMFORMAT, *(double *) current->data);
			len += strlen(buf);
		default:
			break;
		}
	}

	return len - 1;		/* the len++ stands for the comma, so decrement */
}

int _string_print(char *dst, char *src, int pos)
{
	dst[pos++] = '"';
	strcat(dst + pos, src);
	pos += strlen(src);
	dst[pos++] = '"';
	return pos;
}
