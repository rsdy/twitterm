#include "ui.h"
#include "main.h"
#include "http.h"
#include "json.h"
#include <ctype.h>
#include <stdio.h>

/** @file */

#define PARAM_SEPARATOR " \t"

/** URL's connected to twitter services */
#define TW_HOST "twitter.com"
#define TW_TIMELINE "/statuses/friends_timeline.json"
#define TW_UPDATE "/statuses/update.xml"
#define TW_FRIENDS "/statuses/friends.json"
#define TW_FOLLOWERS "/statuses/followers.json"
#define TW_AUTH "/account/verify_credentials.json"

/** print an error message and then return (used in command functions)*/
#define _OOPS(x) printf("ERROR: "); printf((x)); return
#define _OOPS_AUTH _OOPS("cannot authenticate with the server: "\
		"no user-password pair is given\n")
#define _OOPS_RESP(x) printf("HTTP error code: %d\n", (x)); \
	_OOPS("could not download server response!\n");
#define _OOPS_AUTH_USAGE _OOPS("usage: a username password\n")
#define _OOPS_CREAT_USAGE _OOPS("usage: c groupname comma,separated,list\n")

/** Prototype command function, the full user input is passed as a parameter */
typedef void (*command_fn) (char *full);

/** Appends a json_element to the configuration tree
	* @param elem the element to append
	* @return elem */
static json_element _config_append(json_element elem);

/** Reads the config given and sets the config variable to the JSON_ARRAY that was returned by json_parse()
	* @param config the name of the parameter to read
	* @retval true if succeeded
	* @retval false if failed */
static int _read_config(char *config);

/** Looks up a JSON_STRING in the elem tree identified by name, and prints it to stdout with the given prefix
	* @param elem the first element of the chain in which to search
	* @param name the name field of the json_element
	* @param prefix the prefix to print*/
static void _print_json_string(json_element elem, char *name, char *prefix);

/** Returns true, if depending on the parameter list the tweet by the given screen name should be printed, false otherwise
	* @param params the parameter list returned by _get_param_list
	* @param sname the screen name to look for
	* @retval true if a) there are no parameters b) there's no config c) the list named the first parameter exists, and sname is a member
	* @retval false if neither of above happen */
static int _screen_name_filter(char *params, char *sname);

/** Checks if there is a user/password pair in the config chain, and if there is, sets to pointer accordingly
	* @param user the pointer is set to the element in the config chain if succeeded, but NOT NULL'ed if failed
	* @param pwd the pointer is set to the element in the config chain if succeeded, but NOT NULL'ed if failed
	* @retval true if a user/password pair was found
	* @retval false if no user/password pair was found */
static int _check_auth(json_element * user, json_element * pwd);

static char *_get_param_list(char *full);

static void _com_fetch(char *full);
static void _com_post(char *full);
static void _com_list(char *full);
static void _com_auth(char *full);
static void _com_write(char *full);
static void _com_creat(char *full);
static void _com_inval(char *full);

/** The data structure to hold the function pointers and their commands in */
struct __com_t {

	/** The character which triggers the command */
	char name;

	/** The function pointer */
	command_fn fn;
};

/** The array of the available commands. */
static struct __com_t commands[] = {
	{'f', _com_fetch},
	{'p', _com_post},
	{'l', _com_list},
	{'a', _com_auth},
	{'w', _com_write},
	{'c', _com_creat},
	{0, _com_inval}
};

/** The array to hold the configuration */
static json_element config = NULL;

/** The size of the buffer to read from stdio */
#define BUFSIZE 512
void init_ui(char *conffile)
{
	char buff[BUFSIZE];
	int i;

	if (conffile != NULL && _read_config(conffile) < 0)
		return;

	while (!feof(stdin)) {
		printf("twitterm> ");	/* print the prompt */
		if (fgets(buff, BUFSIZE, stdin) == NULL || buff[0] == 'q')
			break;	/* read; break on EOF, and 'q' command */

		fflush(stdin);	/* just to be on the safe side */

		i = 0;
		while (i + 1 < sizeof(commands) / sizeof(struct __com_t)
		       && commands[i].name != buff[0]) {
			i++;
		}

		(commands[i].fn) (buff);	/* call the function */
	}

	json_free(config);
}

void _com_inval(char *full)
{
	_OOPS("The command you entered is not recognised. "
	      "Consult the user manual on available commands and their usage\n");
}

void _com_fetch(char *full)
{
	json_element user,
	 pwd,
	 timeline,
	 current,
	 tmp;
	char *resp;
	int errcode;

	if (_check_auth(&user, &pwd) < 0) {
		_OOPS_AUTH;
	}

	errcode =
	    http_get_auth(TW_HOST, TW_TIMELINE, &resp, user->data, pwd->data);
	if (errcode != 200) {
		_OOPS_RESP(errcode);
	}

	timeline = json_parse(resp);
	free(resp);

	/* no error handling since twitter always sends these correctly
	 * if it didn't, the HTTP layer has already thrown up */
	for (current = timeline->data; current != NULL; current = current->next) {
		tmp = json_get_element_by_name(current, "user");
		tmp = json_get_element_by_name(tmp, "screen_name");

		if (_screen_name_filter
		    (_get_param_list(full), (char *) tmp->data)) {
			printf("-- %s: ", (char *) tmp->data);
			_print_json_string(current, "text", "");
			_print_json_string(current, "created_at", " -at: ");
			_print_json_string(current, "in_reply_to_screen_name",
					   " -in reply to: ");
			putchar('\n');
		}
	}

	json_free(timeline);
}

void _com_post(char *full)
{
	json_element user,
	 pwd;
	int resp;
	char *param,
	*data;

	param = _get_param_list(full);
	if (param == NULL) {
		_OOPS("usage: p message\n");
	}
	data = calloc(8 + strlen(param), sizeof(*data));	/* status= + data + 1 */
	sprintf(data, "%s%s", "status=", param);	/* constructing the message */

	if (_check_auth(&user, &pwd) < 0) {
		free(data);
		_OOPS_AUTH;
	}

	resp =
	    http_post_auth(TW_HOST, TW_UPDATE, NULL, data, user->data,
			   pwd->data);
	if (resp != 200) {
		free(data);
		_OOPS_RESP(resp);
	}

	printf("You tweeted!\n");
	free(data);
}

void _com_list(char *full)
{
	json_element user,
	 pwd,
	 list,
	 current,
	 tmp;
	char *resp,
	*page,
	*params = _get_param_list(full);
	int errcode;

	if (params != NULL && params[0] == 'o')
		page = TW_FOLLOWERS;
	else
		page = TW_FRIENDS;

	if (_check_auth(&user, &pwd) < 0) {
		_OOPS_AUTH;
	}

	errcode = http_get_auth(TW_HOST, page, &resp, user->data, pwd->data);
	if (errcode != 200) {
		_OOPS_RESP(errcode);
	}

	list = json_parse(resp);	/* create the parse tree from the HTTP response */
	free(resp);

	for (current = list->data; current != NULL; current = current->next) {
		tmp = json_get_element_by_name(current, "screen_name");

		if (params != NULL && params[0] == 'f') {
			if (_screen_name_filter
			    (_get_param_list(params), (char *) tmp->data)) {
				printf("%s\n", (char *) tmp->data);
			}
		}
		else {
			printf("%s\n", (char *) tmp->data);
		}
	}

	json_free(list);
}

void _com_auth(char *full)
{
	json_element user,
	 pwd,
	 tmp;
	char *userstr,
	*pwdstr;
	int errcode;

	userstr = strtok(_get_param_list(full), PARAM_SEPARATOR);
	pwdstr = strtok(NULL, PARAM_SEPARATOR);

	if (userstr == NULL || pwdstr == NULL) {
		_OOPS_AUTH_USAGE;
	}

	if (!_check_auth(&user, &pwd)) {
		free(user->data);
		free(pwd->data);

		user->data = mystrdup(userstr);
		pwd->data = mystrdup(pwdstr);
	}
	else {
		tmp = _config_append(json_create_element(JSON_OBJECT));
		user = tmp->data = json_create_string("user", userstr);
		pwd = json_append(tmp->data, json_create_string("pwd", pwdstr));
	}

	errcode = http_get_auth(TW_HOST, TW_AUTH, NULL, user->data, pwd->data);
	if (errcode == 403) {
		_OOPS("Authentication failure: no such user-password pair\n");
	}
	else if (errcode == 200) {
		printf("Authentication succeeded!\n");
	}
	else {
		_OOPS_RESP(errcode);
	}
}

void _com_write(char *full)
{
	FILE *fp;
	char *json;
	char *fname = _get_param_list(full);

	if (fname == NULL) {
		_OOPS("usage: w /path/to/file\n");
	}
	if (config == NULL) {
		_OOPS("No valid configuration is set!\n");
	}
	fp = fopen(fname, "w");
	if (fp == NULL) {
		_OOPS("Failed to open file for writing\n");
	}
	json = json_to_string(config);

	fprintf(fp, "%s", json);

	free(json);
	fclose(fp);
	printf("Configuration file %s is successfully written!\n", fname);
}

void _com_creat(char *full)
{
	json_element root = NULL,
	    current = NULL,
	    tmp;
	char *name,
	*data;

	name = strtok(_get_param_list(full), PARAM_SEPARATOR);
	data = strtok(NULL, PARAM_SEPARATOR);

	if (name == NULL || data == NULL) {
		_OOPS_CREAT_USAGE;
	}

	if (config != NULL) {
		for (current = config->data; current != NULL;
		     current = current->next) {
			tmp = json_get_element_by_name(current, "groups");
			if (tmp != NULL) {
				root = current;
				break;
			}
		}
	}
	if (root == NULL) {
		root = json_create_element(JSON_OBJECT);
		root->data = json_create_element(JSON_TRUE);
		((json_element) root->data)->name = mystrdup("groups");

		_config_append(root);
	}

	current = json_create_string(name, data);
	json_append(root->data, current);
	printf("List \"%s\" successfully created with members:\n\t%s\n", name,
	       data);
}

char *_get_param_list(char *full)
{
	char *ret;
	char *end;

	full++;
	while (*full != 0 && !isalnum(*full)) {
		full++;
	}
	if (*full == 0) {
		return NULL;
	}

	ret = full;
	end = strstr(ret, "\n");
	if (end != NULL) {
		*end = 0;
	}

	return ret;
}

json_element _config_append(json_element elem)
{
	if (config == NULL)
		config = json_create_element(JSON_ARRAY);

	json_append_or_set(config->data, elem,
			   (json_element *) & (config->data));

	return elem;
}

int _read_config(char *conffile)
{
	FILE *fp = fopen(conffile, "r");
	int fsize;
	char *conf;

	if (fp == NULL) {
		printf("ERROR: error while opening file: %s\n", conffile);
		return -1;
	}

	fseek(fp, 0, SEEK_END);	/* seek to the end of the file */
	fsize = ftell(fp);	/* how far it is from the beginning */
	fseek(fp, 0, SEEK_SET);	/* seek to the beginning */

	conf = calloc(fsize + 2, sizeof(*conf));

	if (fread(conf, sizeof(char), fsize, fp) != fsize / sizeof(char)) {
		printf("ERROR: error while reading file: %s\n", conffile);
		fclose(fp);
		return -2;
	}
	fclose(fp);
	config = json_parse(conf);
	free(conf);
	return 0;
}

int _check_auth(json_element * user, json_element * pwd)
{
json_element current;
	if (config == NULL)
		return -1;

	for (current = config->data; current != NULL; current = current->next) {
		*pwd = json_get_element_by_name(current, "pwd");
		*user = json_get_element_by_name(current, "user");
		/* it is possible that one gets its value, while the other doesn't, but
		 * if user and pwd are in different objects, the whole search is pointless
		 */
		if (!(*user == NULL || *pwd == NULL)) {
			return 0;
		}
	}

	return -2;
}

void _print_json_string(json_element elem, char *name, char *prefix)
{
	elem = json_get_element_by_name(elem, name);
	if (elem != NULL && elem->type == JSON_STRING) {
		printf("%s%s\n", prefix, (char *) elem->data);
	}
}

int _screen_name_filter(char *params, char *sname)
{
	json_element current,
	 tmp;
	if (config == NULL || params == NULL)
		return 1;	/* no config, no parameter */

	for (current = config->data; current != NULL; current = current->next) {
		tmp = json_get_element_by_name(current, "groups");
		if (tmp != NULL && tmp->type == JSON_TRUE &&	/* if this is the object of groups */
		    (tmp = json_get_element_by_name(current, params)) != NULL && tmp->type == JSON_STRING)	/* if the group exists, and is type of string */
			return strstr((char *) tmp->data, sname) != NULL;	/* return true if != NULL */
	}
	return 0;
}
