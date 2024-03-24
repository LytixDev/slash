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

#include <stdlib.h>

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"


int builtin_exit(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    if (ast_nodes == NULL)
	exit(0);

    size_t argc = ast_nodes->size;
    SlashValue *argv[argc + 1];
    ast_ll_to_argv(interpreter, ast_nodes, argv);

    SlashValue arg = *argv[0];
    if (IS_NUM(arg))
	exit(arg.num);

    if (IS_TEXT_LIT(arg)) {
	int exit_code = str_view_to_int(arg.text_lit);
	exit(exit_code);
    }
    exit(2);
}
