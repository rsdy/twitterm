#ifndef __UI_H
#define __UI_H

/** @file */

/** Initialises the user interface of the application
	*
	* Basically reads the user input, checks if there is a command available
	* and calls the appropriate function if found, or the invalid function.
	* The full user input is passed over to functions.
	* @param conffile if the parameter isn't NULL, reads the file determined by the parameter and creates the parse tree */
void init_ui(char *conffile);

#endif
