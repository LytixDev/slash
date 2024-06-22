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
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#include "interpreter/ast.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"


int builtin_time(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    if (ast_nodes == NULL) {
        SLASH_PRINT_ERR(&interpreter->stream_ctx, "time: no argument received");
        return 1;
    }
    /* Build CmdStmt */
    Expr *first = (Expr *)ast_nodes->head->value;
    assert(first->type == EXPR_LITERAL);
    SlashValue cmd_name = ((LiteralExpr *)first)->value;
    assert(IS_TEXT_LIT(cmd_name));

    ast_nodes->size--;
    if (ast_nodes->size == 0)
        ast_nodes = NULL;
    else
        ast_nodes->head = ast_nodes->head->next;

    struct timeval t0, t1;
    gettimeofday(&t0, 0);
    struct rusage start_usage;
    getrusage(RUSAGE_SELF, &start_usage);

    CmdStmt cmd = { .type = STMT_CMD, .cmd_name = cmd_name.text_lit, .arg_exprs = ast_nodes };
    exec_cmd(interpreter, &cmd);

    gettimeofday(&t1, 0);
    struct rusage end_usage;
    getrusage(RUSAGE_CHILDREN, &end_usage);

    double real_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1000000.0;
    double user_time = (double)(end_usage.ru_utime.tv_sec + end_usage.ru_utime.tv_usec / 1000000.0);
    double sys_time = (double)(end_usage.ru_stime.tv_sec + end_usage.ru_stime.tv_usec / 1000000.0);
    printf("\nreal\t%.3f\n", real_time);
    printf("user\t%.3f\n", user_time);
    printf("sys\t%.3f\n", sys_time);

    return interpreter->prev_exit_code;
}
