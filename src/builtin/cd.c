/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/type_funcs.h"
#include "lib/arena_ll.h"


int builtin_cd(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    if (ast_nodes == NULL) {
        SLASH_PRINT_ERR(&interpreter->stream_ctx, "cd: no argument received\n");
        return 1;
    }

    size_t argc = ast_nodes->size;
    SlashValue argv[argc];
    ast_ll_to_argv(interpreter, ast_nodes, argv);

    SlashValue param = argv[0];
    VERIFY_TRAIT_IMPL(to_str, param, ".: could not take to_str of type '%s'", param.T->name);
    TraitToStr to_str = param.T->to_str;
    SlashStr *param_str = AS_STR(to_str(interpreter, param));
    return chdir(param_str->str);
}
