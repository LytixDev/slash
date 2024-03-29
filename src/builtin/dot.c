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

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"

/*
 * '.' builtin is used to execute commands from a specified file
 */
int builtin_dot(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    if (ast_nodes == NULL) {
	fprintf(stderr, ".: not enough arguments\n");
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

    ast_nodes->size--;
    if (ast_nodes->size == 0)
	ast_nodes = NULL;
    else
	ast_nodes->head = ast_nodes->head->next;

    exec_program_stub(interpreter, program_name, ast_nodes);
    return interpreter->prev_exit_code;
}
