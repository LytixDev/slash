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

#include "builtin/builtin.h"
#include "interpreter/interpreter.h"
#include "str_view.h"


char *builtin_names[] = { "cd", "vars", "which" };
#define BUILTINS_COUNT (sizeof((builtin_names)) / sizeof((builtin_names[0])))


int builtin_which(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    return 0;
}

WhichResult which(StrView cmd)
{
    char command[cmd.size + 1];
    str_view_to_cstr(cmd, command);

    // TODO: is there a faster solution than a linear search?
    WhichResult result = { 0 };
    for (size_t i = 0; i < BUILTINS_COUNT; i++) {
	char *builtin_name = builtin_names[i];
	if (strcmp(builtin_name, command) == 0) {
	    result.type = WHICH_BUILTIN;
	    // TODO: X macro would work nicely here I think
	    /* set correct function pointer */
	    if (strcmp(command, "cd") == 0) {
		result.builtin = builtin_cd;
	    } else if (strcmp(command, "which") == 0) {
		result.builtin = builtin_which;
	    } else {
		result.builtin = builtin_vars;
	    }
	}
    }

    if (result.type == WHICH_BUILTIN) {
	return result;
    }

    return result;
}
