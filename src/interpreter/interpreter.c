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
// TODO: remove
#include <stdio.h>

#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/interpreter.h"
#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "interpreter/lexer.h"
#include "interpreter/scope.h"
#include "nicc/nicc.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

static SlashValue eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    return (SlashValue){ 0 };
}

static SlashValue eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    switch (expr->operator_->type) {
    case t_plus:
	return slash_plus(eval(interpreter, expr->left), eval(interpreter, expr->right));
    case t_minus:
	return slash_minus(eval(interpreter, expr->left), eval(interpreter, expr->right));
    case t_greater:
	return slash_greater(eval(interpreter, expr->left), eval(interpreter, expr->right));
    case t_equal_equal:
	return slash_equal(eval(interpreter, expr->left), eval(interpreter, expr->right));
    case t_bang_equal:
	return slash_not_equal(eval(interpreter, expr->left), eval(interpreter, expr->right));
    default:
	slash_exit_interpreter_err("operator not supported");
    }
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

static SlashValue eval_interpolation(Interpreter *interpreter, InterpolationExpr *expr)
{
    return var_get(interpreter->scope, &expr->var_name->lexeme);
}

static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    slash_value_println(value);
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->initializer);
    var_set(interpreter->scope, &stmt->name->lexeme, &value);
}

static void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->args_ll->this);
    slash_value_println(value);
}

static void exec_if(Interpreter *interpreter, IfStmt *stmt)
{
    SlashValue r = eval(interpreter, stmt->condition);
    if (is_truthy(&r))
	exec(interpreter, stmt->then_branch);
    else if (stmt->else_branch != NULL)
	exec(interpreter, stmt->else_branch);
}

static void exec_block(Interpreter *interpreter, BlockStmt *stmt)
{
    for (size_t i = 0; i < stmt->statements->size; i++)
	exec(interpreter, darr_get(stmt->statements, i));
}

static void exec_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->value);
    if (stmt->assignment_op->type != t_equal) {
	SlashValue current = var_get(interpreter->scope, &stmt->name->lexeme);
	value = stmt->assignment_op->type == t_plus_equal ? slash_plus(current, value)
							  : slash_minus(current, value);
    }
    var_set(interpreter->scope, &stmt->name->lexeme, &value);
}

static void exec_loop(Interpreter *interpreter, LoopStmt *stmt)
{
    SlashValue r = eval(interpreter, stmt->condition);
    while (is_truthy(&r)) {
	exec(interpreter, stmt->body);
	r = eval(interpreter, stmt->condition);
    }
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

    case EXPR_INTERPOLATION:
	return eval_interpolation(interpreter, (InterpolationExpr *)expr);

    case EXPR_ARG:
	return eval_arg(interpreter, (ArgExpr *)expr);

    default:
	slash_exit_internal_err("expr enum count wtf");
	// will never happen, but lets make the compiler happy
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
	exec_expr(interpreter, (ExpressionStmt *)stmt);
	break;

    case STMT_CMD:
	exec_cmd(interpreter, (CmdStmt *)stmt);
	break;

    case STMT_LOOP:
	exec_loop(interpreter, (LoopStmt *)stmt);
	break;

    case STMT_IF:
	exec_if(interpreter, (IfStmt *)stmt);
	break;

    case STMT_BLOCK:
	exec_block(interpreter, (BlockStmt *)stmt);
	break;

    case STMT_ASSIGN:
	exec_assign(interpreter, (AssignStmt *)stmt);
	break;


    default:
	slash_exit_internal_err("stmt enum count wtf");
    }
}


int interpret(struct darr_t *statements)
{
    // TODO: remove
    printf("--- interpreter ---\n");

    Interpreter interpreter = { 0 };
    scope_init(&interpreter.globals, NULL);
    interpreter.scope = &interpreter.globals;

    for (size_t i = 0; i < statements->size; i++)
	exec(&interpreter, darr_get(statements, i));

    return interpreter.exit_code;
}
