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

#include "interactive/prompt.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"


int builtin_read(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    /* Usage: takes one argument, variable. */
    if (ast_nodes == NULL) {
	SLASH_PRINT_ERR(&interpreter->stream_ctx, "read: no argument received\n");
	return 1;
    }
    if (ast_nodes->size > 1) {
	SLASH_PRINT_ERR(&interpreter->stream_ctx,
			"read: too many arguments received, expected one\n");
	return 1;
    }

    size_t argc = ast_nodes->size;
    SlashValue *argv[argc + 1];
    ast_ll_to_argv(interpreter, ast_nodes, argv);

    SlashValue arg = *argv[0];
    if (!IS_TEXT_LIT(arg)) {
	SLASH_PRINT_ERR(&interpreter->stream_ctx, "read: expected argument to be text, not '%s'\n",
			arg.T->name);
	return 1;
    }

    Prompt prompt;
    prompt_init(&prompt, ">>>");
    prompt_run(&prompt);
    StrView input = (StrView){ .view = prompt.buf, .size = prompt.buf_len - 2 };

    SlashObj *str = gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_view(interpreter, (SlashStr *)str, &input);
    var_define(interpreter->scope, &arg.text_lit, &AS_VALUE(str));

    prompt_free(&prompt);
    return 0;
}
