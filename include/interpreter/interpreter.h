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
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdio.h>

#include "interpreter/ast.h"
#include "interpreter/gc.h"
#include "interpreter/scope.h"
#include "lib/arena_ll.h"
#include "nicc/nicc.h"
#include "sac/sac.h"


#define STREAM_WRITE_END 1
#define STREAM_READ_END 0

typedef struct expr_t Expr; // forward decl


typedef enum {
	RT_NORMAL,
	RT_RETURN,
	RT_BREAK,
	RT_CONTINUE,
} ExecResultType;

typedef struct {
	ExecResultType type;
	Expr *return_expr;
} ExecResult;

#define EXEC_NORMAL                            \
	(ExecResult)                               \
	{                                          \
		.type = RT_NORMAL, .return_expr = NULL \
	}

#define SLASH_PRINT(__stream_ctx, ...) dprintf((__stream_ctx)->out_fd, __VA_ARGS__)
#define SLASH_PRINT_ERR(__stream_ctx, ...) dprintf((__stream_ctx)->err_fd, __VA_ARGS__)


typedef struct {
	int in_fd; // defaults to fileno(STDIN)
	int out_fd; // defaults to fileno(STDOUT)
	int err_fd; // defaults to fileno(STDERR)
	ArrayList active_fds; // list/stack of open file descriptors that need to be closed on fork()
} StreamCtx;

typedef struct interpreter_t {
	Arena arena;
	Scope globals;
	Scope *scope;
	GC gc;
	StreamCtx stream_ctx;
	HashMap type_register;
	int prev_exit_code;
	ExecResult exec_res_ctx;
	int source_line; // file number we are currently interpreting
} Interpreter;


void interpreter_init(Interpreter *interpreter, int argc, char **argv);
void interpreter_free(Interpreter *interpreter);
int interpreter_run(Interpreter *interpreter, ArrayList *statements);
int interpret(ArrayList *statements, int argc, char **argv);

void exec_cmd(Interpreter *interpreter, CmdStmt *stmt);
void ast_ll_to_argv(Interpreter *interpreter, ArenaLL *ast_nodes, SlashValue *result);
void exec_program_stub(Interpreter *interpreter, char *program_path, ArenaLL *ast_nodes);


#endif /* INTERPRETER_H */
