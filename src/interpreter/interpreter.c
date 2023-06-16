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
// TODO: temporary
#include <stdio.h>

#include "arena_ll.h"
#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/core/exec.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value_all.h"
#include "nicc/nicc.h"


static SlashValue *eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

/*
 * helpers
 */
// TODO: this is temporary
static char **cmd_args_fmt(Interpreter *interpreter, CmdStmt *stmt)
{
    size_t argc = 1;
    if (stmt->arg_exprs != NULL)
	argc += stmt->arg_exprs->size;

    char **argv = malloc(sizeof(char *) * (argc + 1));

    char *cmd_name = scope_alloc(interpreter->scope, stmt->cmd_name.size + 6);
    // TODO: 'which' builtin or something
    memcpy(cmd_name, "/bin/", 5);
    memcpy(cmd_name + 5, stmt->cmd_name.view, stmt->cmd_name.size);
    cmd_name[stmt->cmd_name.size + 5] = 0;
    argv[0] = cmd_name;

    if (stmt->arg_exprs == NULL) {
	argv[argc] = NULL;
	return argv;
    }

    size_t i = 1;
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->arg_exprs, item)
    {
	SlashValue *v = eval(interpreter, item->p);
	if (!(v->type == SLASH_STR || v->type == SLASH_SHLIT))
	    slash_exit_interpreter_err("only support evaluing str args");

	SlashStr *s = (SlashStr *)v;
	char *str = scope_alloc(interpreter->scope, s->str.size + 1);
	memcpy(str, s->str.view, s->str.size);
	str[s->str.size] = 0;
	argv[i] = str;
	i++;
    }

    argv[argc] = NULL;
    return argv;
}

static void exec_program_stub(Interpreter *interpreter, CmdStmt *stmt)
{
    char **argv = cmd_args_fmt(interpreter, stmt);
    exec_program(argv);
    free(argv);
}

static void check_num_operands(SlashValue *left, SlashValue *right)
{
    if (!(left->type == SLASH_NUM && right->type == SLASH_NUM))
	slash_exit_interpreter_err("only support operations on numbers");
}


/*
 * expresseion evaluation functions
 */
static SlashValue *eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    return NULL;
}

static SlashValue *eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    SlashValue *left = eval(interpreter, expr->left);
    SlashValue *right = eval(interpreter, expr->right);
    check_num_operands(left, right);

    SlashNum *a = (SlashNum *)left;
    SlashNum *b = (SlashNum *)right;
    SlashBool *result = scope_alloc(interpreter->scope, sizeof(SlashBool));
    result->type = SLASH_BOOL;

    switch (expr->operator_) {
    case t_greater:
	result->value = a->num > b->num;
	break;
    case t_greater_equal:
	result->value = a->num >= b->num;
	break;

    default:
	// TODO: remove
	slash_exit_interpreter_err("binary operator not supported");

	// TODO: assert unreachable
	return NULL;
    }

    return (SlashValue *)result;
}


static SlashValue *eval_literal(Interpreter *interpreter, LiteralExpr *expr)
{
    return expr->value;
}

static SlashValue *eval_interpolation(Interpreter *interpreter, InterpolationExpr *expr)
{
    ScopeAndValue sv = var_get(interpreter->scope, &expr->var_name);
    if (sv.value != NULL)
	return sv.value;
    // return (SlashValue){ .p = NULL, .type = SVT_NONE };
    return NULL;
}

static SlashValue *eval_subshell(Interpreter *interpreter, SubshellExpr *expr)
{
    // TODO: dynamic buffer
    char buffer[1024];
    // TODO: currently assuming expr->stmt is of type CmdStmt
    char **argv = cmd_args_fmt(interpreter, (CmdStmt *)expr->stmt);
    exec_capture(argv, buffer);
    free(argv);

    /* create SlashStr from result of buffer */
    // TODO: this is ugly!
    size_t size = strlen(buffer);
    char *str_p = scope_alloc(interpreter->scope, size);
    strncpy(str_p, buffer, size);

    SlashStr *str = scope_alloc(interpreter->scope, sizeof(SlashStr));
    str->type = SLASH_STR;
    str->str = (StrView){ .view = str_p, .size = size };
    return (SlashValue *)str;
}

static SlashValue *eval_list(Interpreter *interpreter, ListExpr *expr)
{
    // SlashList *list;
}


/*
 * statment execution functions
 */
static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue *value = eval(interpreter, stmt->expression);
    slash_value_print(value);
    putchar('\n');
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    SlashValue *value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name, (SlashValue *)value);
}

static void exec_echo_temporary(Interpreter *interpreter, ArenaLL *args)
{
    // TODO: echo builtin, or something better than this at least
    LLItem *item;
    ARENA_LL_FOR_EACH(args, item)
    {
	SlashValue *v = eval(interpreter, item->p);
	slash_value_print(v);
	putchar(' ');
    }

    putchar('\n');
}

static void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
    if (str_view_eq(stmt->cmd_name, (StrView){ .view = "echo", .size = 4 })) {
	exec_echo_temporary(interpreter, stmt->arg_exprs);
	return;
    }

    exec_program_stub(interpreter, stmt);
}

static void exec_if(Interpreter *interpreter, IfStmt *stmt)
{
    SlashValue *r = eval(interpreter, stmt->condition);
    if (is_truthy(r))
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
    SlashValue *new_value = eval(interpreter, stmt->value);
    if (stmt->assignment_op == t_equal) {
	var_assign(interpreter->scope, &stmt->name, new_value);
	return;
    }

    ScopeAndValue current_value = var_get(interpreter->scope, &stmt->name);
    // TODO: support += operator for str and list
    check_num_operands(new_value, current_value.value);

    SlashNum *new_num_value = (SlashNum *)new_value;
    SlashNum *current_num_value = (SlashNum *)current_value.value;
    if (stmt->assignment_op == t_plus_equal)
	current_num_value->num += new_num_value->num;
    else
	current_num_value->num -= new_num_value->num;
    // TODO: since the hasmap currently only holds pointers, could we just update the pointer
    // directly
    //  and remove all calls to assign?
    // var_assign(interpreter->scope, &stmt->name, (SlashValue *)new_num_value);
}

static void exec_loop(Interpreter *interpreter, LoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue *r = eval(interpreter, stmt->condition);
    while (is_truthy(r)) {
	exec_loop_block(interpreter, stmt->body_block);
	r = eval(interpreter, stmt->condition);
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, StrView iterable)
{
    StrView current = { .view = iterable.view, .size = 1 };
    SlashStr iterator_value = { .type = SLASH_STR, .str = current };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, (SlashValue *)&iterator_value);

    /* increase while the mem addr of view is less than the final mem addr of the iterable */
    while (iterator_value.str.view < iterable.view + iterable.size) {
	exec_loop_block(interpreter, stmt->body_block);
	iterator_value.str.view++;
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange *iterable)
{
    SlashNum iterator_value = { .type = SLASH_NUM, .num = iterable->start };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, (SlashValue *)&iterator_value);

    while (iterator_value.num != iterable->end) {
	exec_loop_block(interpreter, stmt->body_block);
	iterator_value.num++;
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue *underlying = eval(interpreter, stmt->underlying_iterable);
    if (underlying->type == SLASH_STR) {
	exec_iter_loop_str(interpreter, stmt, ((SlashStr *)underlying)->str);
    } else if (underlying->type == SLASH_RANGE) {
	exec_iter_loop_range(interpreter, stmt, (SlashRange *)underlying);
    } else {
	slash_exit_interpreter_err("only str and range can be iterated over currently");
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

// TODO: table better for this
static SlashValue *eval(Interpreter *interpreter, Expr *expr)
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

    case EXPR_SUBSHELL:
	return eval_subshell(interpreter, (SubshellExpr *)expr);

    case EXPR_LIST:
	return eval_list(interpreter, (ListExpr *)expr);

    default:
	slash_exit_internal_err("interpreter: expr type not handled");
	/* will never happen, but lets make the compiler happy */
	return NULL;
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
