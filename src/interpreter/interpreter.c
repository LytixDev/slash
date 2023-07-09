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
#include <stdio.h>

#include "arena_ll.h"
#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/core/exec.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include "str_view.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

/*
 * helpers
 */
static void exec_loop_block(Interpreter *interpreter, BlockStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->statements, item)
    {
	exec(interpreter, item->value);
    }
}

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
	SlashValue v = eval(interpreter, item->value);
	if (!(v.type == SLASH_STR || v.type == SLASH_SHLIT))
	    slash_exit_interpreter_err("only support evaluing str args");

	char *str = scope_alloc(interpreter->scope, v.str.size + 1);
	memcpy(str, v.str.view, v.str.size);
	str[v.str.size] = 0;
	argv[i] = str;
	i++;
    }

    argv[argc] = NULL;
    return argv;
}

static void exec_program_stub(Interpreter *interpreter, CmdStmt *stmt)
{
    char **argv_owning = cmd_args_fmt(interpreter, stmt);
    exec_program(argv_owning);
    free(argv_owning);
}

static void check_num_operands(SlashValue *left, SlashValue *right)
{
    if (!(left->type == SLASH_NUM && right->type == SLASH_NUM))
	slash_exit_interpreter_err("only support operations on numbers");
}


/*
 * expression evaluation functions
 */
static SlashValue eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    return (SlashValue){ 0 };
}

static SlashValue eval_binary_values(SlashValue left, SlashValue right, TokenType operator)
{
    SlashValue result = { 0 };

    switch (operator) {
    case t_greater:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = left.num > right.num };
	break;
    case t_greater_equal:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = left.num >= right.num };
	break;
    case t_less:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = left.num < right.num };
	break;
    case t_less_equal:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = left.num <= right.num };
	break;

    case t_plus:
	if (left.type == SLASH_NUM && right.type == SLASH_NUM) {
	    result = (SlashValue){ .type = SLASH_NUM, .num = left.num + right.num };
	} else if (left.type == SLASH_LIST && right.type == SLASH_LIST) {
	    slash_list_append_list(&left.list, &right.list);
	    result = left;
	} else if (left.type == SLASH_LIST) {
	    slash_list_append(&left.list, right);
	    result = left;
	} else {
	    slash_exit_interpreter_err(
		"plus operator only supported for num and num or list and list");
	    ASSERT_NOT_REACHED;
	}
	break;

    case t_minus:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = left.num - right.num };
	break;

    case t_equal_equal:
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = slash_value_eq(&left, &right) };
	break;

    case t_bang_equal:
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = !slash_value_eq(&left, &right) };
	break;

    default:
	slash_exit_interpreter_err("binary operator not supported");
	ASSERT_NOT_REACHED;
	return (SlashValue){ 0 };
    }

    return result;
}

static SlashValue eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    SlashValue left = eval(interpreter, expr->left);
    SlashValue right = eval(interpreter, expr->right);
    return eval_binary_values(left, right, expr->operator_);
}

static SlashValue eval_literal(Interpreter *interpreter, LiteralExpr *expr)
{
    return expr->value;
}

static SlashValue eval_access(Interpreter *interpreter, AccessExpr *expr)
{
    ScopeAndValue sv = var_get(interpreter->scope, &expr->var_name);
    /* If variable is not defined, then return None. Same behaviour as POSIX sh I think */
    if (sv.value == NULL)
	return (SlashValue){ .type = SLASH_NONE };

    return *sv.value;
}

static SlashValue eval_item_access(Interpreter *interpreter, ItemAccessExpr *expr)
{
    StrView var_name = expr->var_name;
    SlashValue access_index = eval(interpreter, expr->access_value);

    ScopeAndValue value = var_get(interpreter->scope, &var_name);
    SlashValue *collection = value.value;

    SlashItemGetFunc func = slash_item_get[collection->type];
    SlashValue *item = func(collection, &access_index);
    return *item;
}

static SlashValue eval_subshell(Interpreter *interpreter, SubshellExpr *expr)
{
    // TODO: dynamic buffer
    char buffer[1024];
    // TODO: currently assuming expr->stmt is of type CmdStmt
    char **argv_owning = cmd_args_fmt(interpreter, (CmdStmt *)expr->stmt);
    exec_capture(argv_owning, buffer);
    free(argv_owning);

    /* create SlashStr from result of buffer */
    // TODO: this is ugly!
    size_t size = strlen(buffer);
    char *str_view = scope_alloc(interpreter->scope, size);
    strncpy(str_view, buffer, size);

    return (SlashValue){ .type = SLASH_STR, .str = { .view = str_view, .size = size } };
}

static SlashValue eval_tuple(Interpreter *interpreter, ListExpr *expr)
{
    SlashTuple tuple = { .size = expr->exprs->size, .values = NULL };
    /* size is 0 */
    if (expr->exprs == NULL) {
	assert(tuple.size == 0);
	return (SlashValue){ .type = SLASH_TUPLE, .tuple = tuple };
    }

    tuple.values = scope_alloc(interpreter->scope, sizeof(SlashValue) * tuple.size);

    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(expr->exprs, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	tuple.values[i++] = element_value;
    }

    return (SlashValue){ .type = SLASH_TUPLE, .tuple = tuple };
}

static SlashValue eval_list(Interpreter *interpreter, ListExpr *expr)
{
    if (expr->list_type == SLASH_TUPLE)
	return eval_tuple(interpreter, expr);

    SlashValue value = { .type = SLASH_LIST };
    slash_list_init(&value.list);

    if (expr->exprs == NULL)
	return value;

    LLItem *item;
    ARENA_LL_FOR_EACH(expr->exprs, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	slash_list_append(&value.list, element_value);
    }

    return value;
}

static SlashValue eval_map(Interpreter *interpreter, MapExpr *expr)
{
    SlashValue map = { .type = SLASH_MAP };
    slash_map_init(&map.map);

    if (expr->key_value_pairs == NULL)
	return map;

    LLItem *item;
    KeyValuePair *pair;
    ARENA_LL_FOR_EACH(expr->key_value_pairs, item)
    {
	pair = item->value;
	SlashValue key = eval(interpreter, pair->key);
	SlashValue value = eval(interpreter, pair->value);
	slash_map_put(&map.map, &key, &value);
    }

    return map;
}


/*
 * statment execution functions
 */
static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    SlashPrintFunc print_func = slash_print[value.type];
    print_func(&value);
    putchar('\n');
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name, &value);
}

static void exec_echo_temporary(Interpreter *interpreter, ArenaLL *args)
{
    // TODO: echo builtin, or something better than this at least
    LLItem *item;
    ARENA_LL_FOR_EACH(args, item)
    {
	SlashValue v = eval(interpreter, item->value);
	SlashPrintFunc print_func = slash_print[v.type];
	print_func(&v);
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
    SlashValue r = eval(interpreter, stmt->condition);
    if (is_truthy(&r))
	exec(interpreter, stmt->then_branch);
    else if (stmt->else_branch != NULL)
	exec(interpreter, stmt->else_branch);
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

static void exec_item_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    assert(stmt->var->type == EXPR_ITEM_ACCESS);

    ItemAccessExpr *access = (ItemAccessExpr *)stmt->var;
    StrView var_name = access->var_name;
    SlashValue access_index = eval(interpreter, access->access_value);
    SlashValue new_value = eval(interpreter, stmt->value);

    ScopeAndValue current = var_get(interpreter->scope, &var_name);
    /* the underlying collection who's index (access_index) we're trying to modify */
    SlashValue *collection = current.value;

    if (stmt->assignment_op == t_equal) {
	SlashItemAssignFunc func = slash_item_assign[collection->type];
	func(collection, &access_index, &new_value);
	return;
    }

    // TODO: this is inefficient as we have to re-eval the access_index
    SlashValue current_item_value = eval_item_access(interpreter, access);
    /* convert from += to + and -= to - */
    TokenType operator= stmt->assignment_op == t_plus_equal ? t_plus : t_minus;
    new_value = eval_binary_values(current_item_value, new_value, operator);

    SlashItemAssignFunc func = slash_item_assign[collection->type];
    func(collection, &access_index, &new_value);
}

static void exec_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    if (stmt->var->type == EXPR_ITEM_ACCESS) {
	exec_item_assign(interpreter, stmt);
	return;
    }

    if (stmt->var->type != EXPR_ACCESS) {
	slash_exit_interpreter_err("can't assign that type !!");
	ASSERT_NOT_REACHED;
    }

    AccessExpr *access = (AccessExpr *)stmt->var;
    StrView var_name = access->var_name;
    ScopeAndValue current = var_get(interpreter->scope, &var_name);
    if (current.value == NULL)
	slash_exit_interpreter_err("cannot modify undefined variable");

    SlashValue new_value = eval(interpreter, stmt->value);

    if (stmt->assignment_op == t_equal) {
	var_assign(interpreter->scope, &var_name, &new_value);
	return;
    }

    /* convert from += to + and -= to - */
    TokenType operator= stmt->assignment_op == t_plus_equal ? t_plus : t_minus;
    new_value = eval_binary_values(*current.value, new_value, operator);
    var_assign(interpreter->scope, &var_name, &new_value);
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

static void exec_iter_loop_list(Interpreter *interpreter, IterLoopStmt *stmt, SlashList iterable)
{
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < iterable.underlying.size; i++) {
	iterator_value = slash_list_get(&iterable, i);
	var_assign(interpreter->scope, &stmt->var_name, iterator_value);
	exec_loop_block(interpreter, stmt->body_block);
    }
}

static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, StrView iterable)
{
    StrView str = { .view = iterable.view, .size = 1 };
    SlashValue iterator_value = { .type = SLASH_STR, .str = str };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, &iterator_value);

    /* increase while the mem addr of view is less than the final mem addr of the iterable */
    while (iterator_value.str.view < iterable.view + iterable.size) {
	exec_loop_block(interpreter, stmt->body_block);
	iterator_value.str.view++;
	var_assign(interpreter->scope, &stmt->var_name, &iterator_value);
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange *iterable)
{
    SlashValue iterator_value = { .type = SLASH_NUM, .num = iterable->start };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, &iterator_value);

    while (iterator_value.num != iterable->end) {
	exec_loop_block(interpreter, stmt->body_block);
	iterator_value.num++;
	// TODO: bloat? we could store a reference instead of a copy to the iterator_value
	var_assign(interpreter->scope, &stmt->var_name, &iterator_value);
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue underlying = eval(interpreter, stmt->underlying_iterable);
    if (underlying.type == SLASH_STR) {
	exec_iter_loop_str(interpreter, stmt, underlying.str);
    } else if (underlying.type == SLASH_RANGE) {
	exec_iter_loop_range(interpreter, stmt, &underlying.range);
    } else if (underlying.type == SLASH_LIST) {
	exec_iter_loop_list(interpreter, stmt, underlying.list);
    } else {
	slash_exit_interpreter_err("type can't be iterated over");
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

static SlashValue eval(Interpreter *interpreter, Expr *expr)
{
    switch (expr->type) {
    case EXPR_UNARY:
	return eval_unary(interpreter, (UnaryExpr *)expr);

    case EXPR_BINARY:
	return eval_binary(interpreter, (BinaryExpr *)expr);

    case EXPR_LITERAL:
	return eval_literal(interpreter, (LiteralExpr *)expr);

    case EXPR_ACCESS:
	return eval_access(interpreter, (AccessExpr *)expr);

    case EXPR_ITEM_ACCESS:
	return eval_item_access(interpreter, (ItemAccessExpr *)expr);

    case EXPR_SUBSHELL:
	return eval_subshell(interpreter, (SubshellExpr *)expr);

    case EXPR_LIST:
	return eval_list(interpreter, (ListExpr *)expr);

    case EXPR_MAP:
	return eval_map(interpreter, (MapExpr *)expr);

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
    m_arena_init_dynamic(&interpreter.arena, 1, 16384);

    scope_init_global(&interpreter.globals, &interpreter.arena);
    interpreter.scope = &interpreter.globals;

    for (size_t i = 0; i < statements->size; i++)
	exec(&interpreter, *(Stmt **)arraylist_get(statements, i));

    scope_destroy(&interpreter.globals);

    return interpreter.exit_code;
}
