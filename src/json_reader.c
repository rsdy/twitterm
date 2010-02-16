#include "json.h"
#include <stdio.h>
#include <ctype.h>

/** @file */

/** Parses a JSON_OBJECT type element beginning from *str to the closing bracket
 *
 * The children are in a linked list in the ->data field
 * @param str the pointer where the object starts
 * @return an allocated json_element, type JSON_OBJECT */
static json_element _json_parse_object(char **str);

/** Parses a JSON_ARRAY type element beginning from *str to the closing bracket
 *
 * The children are in a linked list in the ->data field
 * @param str the pointer where the object starts
 * @return an allocated json_element, type JSON_ARRAY */
static json_element _json_parse_array(char **str);

/** Parses a key-value pair in an object.
 *
 * The children can be of any type, and are the ->data field
 * @param str the pointer where the object starts
 * @return an allocated json_element of any type */
static json_element _json_parse_pair(char **str);

/** Determines the type of the value given, and sets the ->data field in the json_element accordingly
 * @param str the pointer where the object starts
 * @param elem the of element to set the value of
 * @return elem */
static json_element _json_set_value(char **str, json_element elem);

/** Parses the number beginning at *str into a double
 * @param str the pointer where the number starts
 * @return the double value of the string */
static double *_get_num(char **str);

/** Parses the string beginning at *str into a char *
 *
 * The function handles the different escape sequences used by the JSON protocol
 * @param str the pointer where the string starts
 * @return the allocated string */
static char *_get_string(char **str);

/** Converts the escaped unicode-32 character to an utf-8 equivalent
 *
 * Note 1: only uses the first 3 bytes
 *
 * Note 2: based on reference implementation:
 *
 * http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
 * @param u32 the 4 bytes to convert
 * @param arr the character array to append the result to
 * @return the count of actual bytes written (max 3) */
static char _get_unicode_char(unsigned int u32, char *arr);

json_element json_alloc()
{
	json_element elem = malloc(sizeof(*elem));
	if (elem == NULL)
		return NULL;

	elem->type = JSON_UNSET;
	elem->name = NULL;
	elem->data = NULL;
	elem->next = NULL;

	return elem;
}

void json_free(json_element elem)
{
	if (elem == NULL)
		return;

	if (elem->data != NULL &&
			(elem->type == JSON_ARRAY || elem->type == JSON_OBJECT)) {
		json_free(elem->data);
		elem->data = NULL;
	}

	if (elem->name != NULL)
		free(elem->name);
	if (elem->data != NULL)
		free(elem->data);
	if (elem->next != NULL)
		json_free(elem->next);

	free(elem);
}

json_element json_append(json_element root, json_element elem)
{
	json_element current = root;
	if (current == NULL)
		return NULL;

	while (current->next != NULL)
		current = current->next;

	current->next = elem;

	return elem;
}

json_element json_append_or_set(json_element root, json_element elem,
				json_element *to_set)
{
	if (root == NULL) {
		*to_set = elem;
		return *to_set;
	}
	else {
		return json_append(root, elem);
	}
}

json_element json_create_element(json_type type)
{
	json_element elem = json_alloc();
	elem->type = type;
	return elem;
}

json_element json_create_string(char *key, char *value)
{
	json_element elem = json_create_element(JSON_STRING);

	elem->name = mystrdup(key);
	elem->data = mystrdup(value);

	return elem;
}

json_element json_create_numeric(char *key, double value)
{
	json_element elem = json_create_element(JSON_NUM);

	elem->name = mystrdup(key);
	elem->data = malloc(sizeof(double));
	*(double *) (elem->data) = value;

	return elem;
}

json_element json_get_element_by_name(json_element obj, char *name)
{
	json_element current = obj->data;

	for (; current != NULL; current = current->next) {
		if (!(current->name == NULL || strcmp(name, current->name)))
			return current;
	}

	return NULL;
}

json_element json_parse(char *str)
{
	json_element root = NULL;
	json_element current = NULL;
	json_element new = NULL;

	while (*str != 0) {
		if (*str == JSON_ARRAY) {
			new = json_alloc();
			new->type = JSON_ARRAY;
			new->data = _json_parse_array(&str);

			current = json_append_or_set(current, new, &root);
		}
		else {
			str++;
		}
	}

	return root;
}

/* ************************************
 * static functions
 */
static json_element _json_parse_array(char **str)
{
	json_element root = NULL;
	json_element current = NULL;
	json_element new = NULL;

	/* increment the pointer, since the type has already been determined */
	for ((*str)++; **str != 0 && **str != ']'; (*str)++) {
		new = json_alloc();
		_json_set_value(str, new);

		current = json_append_or_set(current, new, &root);
	}

	return root;
}

static json_element _json_parse_object(char **str)
{
	json_element root = NULL;
	json_element current = NULL;
	json_element new = NULL;

	(**str)++; /* skip the type */
	while (**str && **str != '}') {
		if (**str == JSON_STRING) {
			new = _json_parse_pair(str);

			current = json_append_or_set(current, new, &root);
		}
		else {
			(*str)++;
		}
	}

	(*str)++;
	return root;
}

static json_element _json_parse_pair(char **str)
{
	json_element val = json_alloc();
	val->name = _get_string(str);

	/* advance the pointer, because _json_set_value() expects a pointer
	 * that starts at a command character
	 */
	(*str)++;

	return _json_set_value(str, val);
}

json_element _json_set_value(char **str, json_element val)
{
	val->type = **str;

	switch (**str) {
	case JSON_ARRAY:
		val->data = _json_parse_array(str);
		break;
	case JSON_OBJECT:
		val->data = _json_parse_object(str);
		break;
	case JSON_STRING:
		val->data = _get_string(str);
		break;
	default:
		if (isdigit(**str)) {
			val->type = JSON_NUM;
			val->data = (void *) _get_num(str);
		}		/* type has already been set if it's not a digit! */
	}

	return val;
}

static double *_get_num(char **str)
{
	double *num = malloc(sizeof(*num));
	if (num == NULL)
		return NULL;

	*num = strtod(*str, str);
	return num;
}

static char *_get_string(char **str)
{
	int length = 0;
	unsigned int u32;	/* where the utf32 encoded char is stored */
	char *ptr;		/* ptr is the default working pointer */
	char *ret;

	if (**str == '"')
		(*str)++;

	for (ptr = *str; *ptr && *ptr != '"'; ptr++) {
		if (*ptr == '\\')
			ptr++;

		length++;
	}
	ret = malloc((length + 1) * sizeof(*ret));

	for (ptr = ret; **str != '"'; ptr++, (*str)++) {
		/* we need to handle characters beginning with \ differently */
		if (**str == '\\') {
			(*str)++;
			switch (**str) {
				/* these actually are just string representations */
			case 'n':
				*ptr = '\n';
				break;
			case 'r':
				*ptr = '\r';
				break;
			case 't':
				*ptr = '\t';
				break;
			case 'b':
				*ptr = '\b';
				break;
			case 'f':
				*ptr = '\f';
				break;
			case 'u':
				sscanf(++*str, "%4x", &u32);
				*str += 3;
				ptr += _get_unicode_char(u32, ptr);
				break;
			default:
				/* special characters that need to be escaped: \ " / */
				*ptr = **str;
			}
		}
		else {
			*ptr = **str;
		}
	}

	*ptr = 0;		/* terminate the string just created */
	(*str)++;		/* set position to after the processed block */
	return ret;
}

static char _get_unicode_char(unsigned int u32, char *arr)
{
	const short int firstbyte[] = { 0x00, 0xc0, 0xe0, 0xf0 };
	int length = 2;		/* how many bytes the output will be - 1 */

	if (u32 < 0x80)
		length = 0;
	else if (u32 < 0x800)
		length = 1;

	switch (length) {
	case 2:
		arr[2] = (char) ((u32 | 0x80) & 0xbf);
		u32 >>= 6;
	case 1:
		arr[1] = (char) ((u32 | 0x80) & 0xbf);
		u32 >>= 6;
	case 0:
		*arr = (char) u32 | firstbyte[length];
	}

	return length;
}
