/*
 *  Copyright (C) 2024 Nicolai Brand (https://lytix.dev)
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

#include <stdio.h>
#include <unistd.h>

#include "interpreter/exec.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"

/*
 * '.' builtin is used to execute commands from a specified file
 */
int builtin_dot(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    if (argc == 0) {
	fprintf(stderr, ".: not enough arguments\n");
	return 1;
    }

    char *exec_argv[argc + 1]; // NULL terminated
    exec_argv[argc] = NULL;

    gc_barrier_start(&interpreter->gc);
    VERIFY_TRAIT_IMPL(to_str, argv[0], ".: could not take to_str of type '%s'", argv[0].T->name);
    TraitToStr to_str = argv[0].T->to_str;
    SlashStr *program_str = AS_STR(to_str(interpreter, argv[0]));
    /* prepend './' to first argument */
    char program_name[program_str->len + 3]; // + 2 (for './) and + 1 for null termination
    program_name[0] = '.';
    program_name[1] = '/';
    strncpy(program_name + 2, program_str->str, program_str->len);
    program_name[program_str->len + 2] = 0;
    exec_argv[0] = program_name;

    for (size_t i = 1; i < argc; i++) {
	VERIFY_TRAIT_IMPL(to_str, argv[i], ".: could not take to_str of type '%s'",
			  argv[i].T->name);
	to_str = argv[i].T->to_str;
	SlashStr *param_str = AS_STR(to_str(interpreter, argv[i]));
	exec_argv[i] = param_str->str;
    }

    int rc = exec_program(&interpreter->stream_ctx, exec_argv);
    gc_barrier_end(&interpreter->gc);

    return rc;
}
