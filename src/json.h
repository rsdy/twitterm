#ifndef __JSON_H
#define __JSON_H
#include "main.h"

/** @file */

/** The enum that defines the possible types of the json_element */
typedef enum _json_type {
	JSON_UNSET,
	JSON_NUM,

	JSON_TRUE = 't',
	JSON_FALSE = 'f',
	JSON_NULL = 'n',

	JSON_STRING = '"',
	JSON_ARRAY = '[',
	JSON_OBJECT = '{'
} json_type;

typedef struct _json_element *json_element;

/** A node of a linked list */
struct _json_element {

	/** The type */
	json_type type;

	/** The name (or key) of the element. May be NULL if empty */
	char *name;

	/** The data. It may be double, char *, NULL, json_element */
	void *data;

	/** The next node in the linked list. NULL if last in a chain */
	json_element next;
};

/** The function allocates a json_element
 * @return a pointer to the allocated struct */
json_element json_alloc();

/** The function frees a json_element and it's members recursively (->data)
 * @param elem the json_element to free */
void json_free(json_element elem);

/** Appends a json_element to the end of the root chain
 * @param root an element of the chain to append to
 * @param elem the element to append
 * @return elem or NULL if root == NULL */
json_element json_append(json_element root, json_element elem);

json_element json_append_or_set(json_element root, json_element elem, json_element *to_set);

/** This function allocates a json_element using json_alloc() and sets its type to type
 * @param type the type to allocate
 * @return a json_element of type type */
json_element json_create_element(json_type type);

/** Allocates a json_element with the type JSON_STRING and sets name and data fields to strdup()s of the params.
 * @param key the key
 * @param value the value
 * @return a json_element of type JSON_STRING */
json_element json_create_string(char *key, char *value);

/** Allocates a json_element with the type JSON_NUM and sets name and data fields to copys of the params.
 * @param key the key
 * @param value the value
 * @return a json_element of type JSON_NUM */
json_element json_create_numeric(char *key, double value);

/** Returns the element with the given name or NULL if not found among the elements of the JSON_OBJECT
 * @param obj the object in which to search
 * @param name the name to look for
 * @return the json_element, or NULL if not found */
json_element json_get_element_by_name(json_element obj, char *name);

/** Parses a json string and returns with the parse tree.
 *
 * IT DOESN'T DO VALIDATION!!
 * @param str the string to parse
 * @return the root element of the parse tree generated
 */
json_element json_parse(char *str);

/** Formats the json_element into a JSON string
 * @param elem the json_element to process
 * @return a string allocated on the heap
 */
char *json_to_string(json_element elem);
#endif
