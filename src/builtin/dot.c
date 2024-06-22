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

#include <assert.h>
#include <stdio.h>


#include "interpreter/ast.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"
#include "options.h"

/*
 * '.' builtin is used to execute commands from a specified file
 */
int builtin_dot(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    if (ast_nodes == NULL) {
        SLASH_PRINT_ERR(&interpreter->stream_ctx, ".: not enough arguments\n");
        return 1;
    }

    /* Eval first argument */
    Expr *first = (Expr *)ast_nodes->head->value;
    assert(first->type == EXPR_LITERAL);
    SlashValue cmd_name = ((LiteralExpr *)first)->value;
    assert(IS_TEXT_LIT(cmd_name));

    /* prepend '.' to first argument */
    char program_name[cmd_name.text_lit.size + 2]; // + 1 for '.' and + 1 for null termination
    program_name[0] = '.';
    strncpy(program_name + 1, cmd_name.text_lit.view, cmd_name.text_lit.size);
    program_name[cmd_name.text_lit.size + 1] = 0;

    TODO_LOG("dot builtin: Check if specified file exists and is executable");

    ArenaLL ast_nodes_cpy = { .size = ast_nodes->size - 1 };
    if (ast_nodes_cpy.size == 0) {
        exec_program_stub(interpreter, program_name, NULL);
    } else {
        ast_nodes_cpy.head = ast_nodes->head->next;
        ast_nodes_cpy.tail = ast_nodes->tail;
        exec_program_stub(interpreter, program_name, &ast_nodes_cpy);
    }

    return interpreter->prev_exit_code;
}
