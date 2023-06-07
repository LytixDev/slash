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
#include "interpreter.h"
#include "ast.h"
#include "common.h"
#include "nicc/nicc.h"
#include "slash_str.h"
#include "slash_value.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

static SlashValue eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    return (SlashValue){ 0 };
}

static SlashValue eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    return (SlashValue){ 0 };
}

static SlashValue eval_arg(Interpreter *interpreter, ArgExpr *expr)
{
    return (SlashValue){ 0 };
}


static SlashValue eval_literal(Interpreter *interpreter, LiteralExpr *expr)
{
    return expr->value;
}


static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->initializer);
    hashmap_put(&interpreter->variables, &stmt->name->lexeme, sizeof(SlashStr), &value,
		sizeof(SlashValue), false);
#include <stdio.h>
    slash_str_print(stmt->name->lexeme);
    putchar('=');
    SlashValue *v = hashmap_get(&interpreter->variables, &stmt->name->lexeme, sizeof(SlashStr));
    slash_str_print(*(SlashStr *)v->p);
}

static void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
}


// TODO: table better for this
static SlashValue eval(Interpreter *interpreter, Expr *expr)
{
    switch (expr->type) {
    case EXPR_UNARY:
	return eval_unary(interpreter, (UnaryExpr *)expr);

    case EXPR_BINARY:
	return eval_binary(interpreter, (BinaryExpr *)expr);

    case EXPR_LITERAL:
	return eval_literal(interpreter, (LiteralExpr *)expr);

    case EXPR_ARG:
	return eval_arg(interpreter, (ArgExpr *)expr);

    default:
	slash_exit_internal_err("expr enum count wtf");
	// will never happen, but lets make the compiler hapyy
	return (SlashValue){ 0 };
    }
}

static void exec(Interpreter *interpreter, Stmt *stmt)
{
    switch (stmt->type) {
    case STMT_VAR:
	exec_var(interpreter, (VarStmt *)stmt);
	break;

    case STMT_EXPRESSION:
	eval(interpreter, ((ExpressionStmt *)stmt)->expression);
	break;

    case STMT_CMD:
	exec_cmd(interpreter, (CmdStmt *)stmt);
	break;

    default:
	slash_exit_internal_err("stmt enum count wtf");
    }
}


int interpret(struct darr_t *statements)
{
    Interpreter interpreter = { 0 };
    hashmap_init(&interpreter.variables);

    for (size_t i = 0; i < statements->size; i++)
	exec(&interpreter, darr_get(statements, i));

    return interpreter.exit_code;
}
