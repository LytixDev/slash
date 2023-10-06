/*
 *  Copyright (C) 2023 Nicolai Brand (https://lytix.dev)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <string.h>

#include "interpreter/types/method.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_str.h"
#include "interpreter/types/slash_value.h"


char slash_value_to_char[] = {
    'b', // bool
    'n', // num
    '-', // shident
    'r', // range
    'o', // any object
    '-', // none
};


static char value_to_char_type(SlashValue *value)
{
    if (IS_OBJ(value->type)) {
	switch (value->obj->type) {
	case SLASH_OBJ_LIST:
	    return 'l';
	case SLASH_OBJ_TUPLE:
	    return 't';
	case SLASH_OBJ_MAP:
	    return 'm';
	case SLASH_OBJ_STR:
	    return 's';
	default:
	    return 'o';
	}
    }
    return slash_value_to_char[value->type];
}

MethodFunc get_method(SlashValue *self, char *method_name)
{
    size_t methods_count;
    SlashMethod *methods;

    switch (self->type) {
    case SLASH_OBJ: {
	switch (self->obj->type) {
	case SLASH_OBJ_LIST:
	    methods_count = SLASH_LIST_METHODS_COUNT;
	    methods = slash_list_methods;
	    break;
	case SLASH_OBJ_MAP:
	    methods_count = SLASH_MAP_METHODS_COUNT;
	    methods = slash_map_methods;
	    break;
	case SLASH_OBJ_STR:
	    methods_count = SLASH_STR_METHODS_COUNT;
	    methods = slash_str_methods;
	    break;
	default:
	    return NULL;
	};

	break;
    };
    default:
	return NULL;
    }

    /* linear search */
    SlashMethod current;
    for (size_t i = 0; i < methods_count; i++) {
	current = methods[i];
	// TODO: strcmp does not exit early
	if (strcmp(current.name, method_name) == 0)
	    return current.fp;
    }
    return NULL;
}

/*
 * count occurrences of whitespace + 1
 */
static size_t signature_arg_count(char *signature)
{
    assert(signature != NULL);

    if (signature[0] == 0)
	return 0;

    size_t count = 1;
    for (char c = *signature; c != 0; c = *(++signature)) {
	if (c == ' ')
	    count++;
    }
    return count;
}

bool match_signature(char *signature, size_t argc, SlashValue *argv)
{
    size_t signature_argc = signature_arg_count(signature);
    if (signature_argc != argc)
	return false;

    /* split on whitespace */
    char *arg = strtok(signature, " ");
    size_t i = 0;

    while (arg != NULL) {
	char type = value_to_char_type(&argv[i]);
	bool okay = false;
	while (*arg != 0 && *arg != ' ') {
	    if (*signature == 'a' || *signature == type) {
		okay = true;
		break;
	    }
	    arg++;
	}

	/* while loop finished without finding a valid type */
	if (!okay)
	    return false;

	arg = strtok(NULL, " ");
	i++;
    }

    return true;
}
