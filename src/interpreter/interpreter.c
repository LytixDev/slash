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
#include "interpreter/interpreter.h"
#include "arena_ll.h"
#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/lang/slash_range.h"
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
    return slash_value_cmp(&interpreter->scope->value_arena, eval(interpreter, expr->left),
			   eval(interpreter, expr->right), expr->operator_->type);
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
    ScopeAndValue sv = var_get(interpreter->scope, &expr->var_name->lexeme);
    if (sv.value != NULL)
	return *sv.value;
    return (SlashValue){ .p = NULL, .type = SVT_NONE };
}

static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    slash_value_println(value);
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name->lexeme, &value);
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

static void exec_loop_block(Interpreter *interpreter, BlockStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->statements, item)
    {
	exec(interpreter, item->p);
    }
}

static void exec_block(Interpreter *interpreter, BlockStmt *stmt)
{
    Scope block_scope;
    scope_init(&block_scope, interpreter->scope);
    interpreter->scope = &block_scope;

    exec_loop_block(interpreter, stmt);

    interpreter->scope = block_scope.enclosing;
    scope_destroy(&block_scope);
}

static void exec_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    SlashValue new_value = eval(interpreter, stmt->value);
    if (stmt->assignment_op->type != t_equal) {
	ScopeAndValue current_value = var_get(interpreter->scope, &stmt->name->lexeme);
	// TODO: if sv.value == NULL -> runtime error
	new_value =
	    slash_value_cmp(&current_value.scope->value_arena, *current_value.value, new_value,
			    stmt->assignment_op->type == t_plus_equal ? t_plus : t_minus);
    }
    var_assign(interpreter->scope, &stmt->name->lexeme, &new_value);
}

static void exec_loop(Interpreter *interpreter, LoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue r = eval(interpreter, stmt->condition);
    while (is_truthy(&r)) {
	exec_loop_block(interpreter, stmt->body_block);
	r = eval(interpreter, stmt->condition);
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, SlashStr *iterable)
{
    SlashStr current = { .p = iterable->p, .size = 1 };
    SlashValue iterator_value = { .p = &current, .type = SVT_STR };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name->lexeme, &iterator_value);

    size_t pos = 0;
    while (pos < iterable->size) {
	exec_loop_block(interpreter, stmt->body_block);
	current.p = current.p + 1;
	pos++;
	var_assign(interpreter->scope, &stmt->var_name->lexeme, &iterator_value);
    }
}

static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange *iterable)
{
    double current = iterable->start;
    SlashValue iterator_value = { .p = &current, .type = SVT_NUM };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name->lexeme, &iterator_value);

    while (current != iterable->end) {
	exec_loop_block(interpreter, stmt->body_block);
	current++;
	var_assign(interpreter->scope, &stmt->var_name->lexeme, &iterator_value);
    }
}

static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue underlying = eval(interpreter, stmt->underlying_iterable);
    if (underlying.type == SVT_STR) {
	exec_iter_loop_str(interpreter, stmt, underlying.p);
    } else if (underlying.type == SVT_RANGE) {
	exec_iter_loop_range(interpreter, stmt, underlying.p);
    } else {
	slash_exit_interpreter_err("only str and range can be iterated over currently");
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
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
	slash_exit_internal_err("interpreter: expr type not handled");
	/* will never happen, but lets make the compiler happy */
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

    case STMT_ITER_LOOP:
	exec_iter_loop(interpreter, (IterLoopStmt *)stmt);
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
	slash_exit_internal_err("interpreter: stmt type not handled");
    }
}


int interpret(ArrayList *statements)
{
    Interpreter interpreter = { 0 };
    scope_init(&interpreter.globals, NULL);
    interpreter.scope = &interpreter.globals;

    for (size_t i = 0; i < statements->size; i++)
	exec(&interpreter, *(Stmt **)arraylist_get(statements, i));

    scope_destroy(&interpreter.globals);

    return interpreter.exit_code;
}
