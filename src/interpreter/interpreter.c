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
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "arena_ll.h"
#include "interpreter/ast.h"
#include "interpreter/core/exec.h"
#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/scope.h"
#include "interpreter/types/cast.h"
#include "interpreter/types/method.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "interpreter/types/trait.h"
#include "nicc/nicc.h"
#include "str_view.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

/*
 * helpers
 */
static void exec_block_body(Interpreter *interpreter, BlockStmt *stmt)
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

	// TODO: better num to str
	if (v.type == SLASH_NUM) {
	    char *str = scope_alloc(interpreter->scope, 128);
	    sprintf(str, "%f", v.num);
	    argv[i] = str;
	} else if (v.type == SLASH_STR || v.type == SLASH_SHIDENT) {
	    char *str = scope_alloc(interpreter->scope, v.str.size + 1);
	    memcpy(str, v.str.view, v.str.size);
	    str[v.str.size] = 0;
	    argv[i] = str;
	} else {
	    report_runtime_error("Currently only support evaluating number and string arguments");
	}
	i++;
    }

    argv[argc] = NULL;
    return argv;
}

static void exec_program_stub(Interpreter *interpreter, CmdStmt *stmt)
{
    char **argv_owning = cmd_args_fmt(interpreter, stmt);
    int exit_code = exec_program(interpreter->stream_ctx, argv_owning);
    interpreter->prev_exit_code = exit_code;
    free(argv_owning);
}

static void check_num_operands(SlashValue *left, SlashValue *right)
{
    if (!(left->type == SLASH_NUM && right->type == SLASH_NUM))
	report_runtime_error("Binary operations only supported on numbers");
}


/*
 * expression evaluation functions
 */
static SlashValue eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    SlashValue right = eval(interpreter, expr->right);
    if (expr->operator_ == t_not)
	return (SlashValue){ .type = SLASH_BOOL, .boolean = !is_truthy(&right) };
    if (expr->operator_ == t_minus) {
	if (right.type != SLASH_NUM)
	    report_runtime_error("'-' operator only supported for numbers");
	right.num = -right.num;
	return right;
    }

    ASSERT_NOT_REACHED;
    return (SlashValue){ 0 };
}

static SlashValue cmp_binary_values(SlashValue left, SlashValue right, TokenType operator)
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

    case t_plus: {
	if (left.type == SLASH_NUM && right.type == SLASH_NUM) {
	    result = (SlashValue){ .type = SLASH_NUM, .num = left.num + right.num };
	} else if (IS_OBJ(left.type) && left.obj->type == SLASH_OBJ_LIST) {
	    if (IS_OBJ(right.type) && right.obj->type == SLASH_OBJ_LIST) {
		slash_list_append_list((SlashList *)left.obj, (SlashList *)right.obj);
		return left;
	    } else if (!IS_OBJ(right.type)) {
		slash_list_append((SlashList *)left.obj, right);
		return left;
	    } else {
		report_runtime_error("list + object not supported");
		ASSERT_NOT_REACHED;
	    }
	} else {
	    report_runtime_error(
		"Plus operator only supported for number and number or list and list");
	    ASSERT_NOT_REACHED;
	}
	break;
    }

    case t_minus:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = left.num - right.num };
	break;

    case t_slash: {
	if (right.num == 0)
	    report_runtime_error("Division by zero error");
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = left.num / right.num };
	break;
    }
    case t_slash_slash: {
	if (right.num == 0)
	    report_runtime_error("Division by zero error");
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = (int)(left.num / right.num) };
	break;
    }

    case t_percent: {
	if (right.num == 0)
	    report_runtime_error("Modulo by zero error");
	check_num_operands(&left, &right);
	double m = fmod(left.num, right.num);
	/* same behaviour as we tend to see in maths */
	m = m < 0 && right.num > 0 ? m + right.num : m;
	result = (SlashValue){ .type = SLASH_NUM, .num = m };
	break;
    }

    case t_star:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = left.num * right.num };
	break;
    case t_star_star:
	check_num_operands(&left, &right);
	result = (SlashValue){ .type = SLASH_NUM, .num = pow(left.num, right.num) };
	break;

    case t_equal_equal:
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = slash_value_eq(&left, &right) };
	break;

    case t_bang_equal:
	result = (SlashValue){ .type = SLASH_BOOL, .boolean = !slash_value_eq(&left, &right) };
	break;

    default:
	report_runtime_error("Binary operator not supported");
	ASSERT_NOT_REACHED;
	return (SlashValue){ 0 };
    }

    return result;
}

static SlashValue eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    SlashValue left = eval(interpreter, expr->left);
    /* logical operators */
    if (expr->operator_ == t_and) {
	if (!is_truthy(&left))
	    return (SlashValue){ .type = SLASH_BOOL, .boolean = false };
	SlashValue right = eval(interpreter, expr->right);
	return (SlashValue){ .type = SLASH_BOOL, .boolean = is_truthy(&right) };
    }
    SlashValue right = eval(interpreter, expr->right);
    if (expr->operator_ == t_or)
	return (SlashValue){ .type = SLASH_BOOL, .boolean = is_truthy(&left) || is_truthy(&right) };

    if (expr->operator_ != t_in)
	return cmp_binary_values(left, right, expr->operator_);

    /* left "IN" right */
    TraitItemIn func = trait_item_in[right.type];
    bool rc = func(&right, &left);
    return (SlashValue){ .type = SLASH_BOOL, .boolean = rc };
}

static SlashValue eval_literal(Interpreter *interpreter, LiteralExpr *expr)
{
    (void)interpreter;
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

static SlashValue eval_subscript(Interpreter *interpreter, SubscriptExpr *expr)
{
    SlashValue value = eval(interpreter, expr->expr);
    SlashValue access_index = eval(interpreter, expr->access_value);
    TraitItemGet func = trait_item_get[value.type];
    SlashValue item = func(interpreter, &value, &access_index);
    return item;
}

static SlashValue eval_subshell(Interpreter *interpreter, SubshellExpr *expr)
{
    /* pipe output will be written to */
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = interpreter->stream_ctx;
    int original_write_fd = stream_ctx->write_fd;
    /* set the write fd to the newly created pipe */
    stream_ctx->write_fd = fd[STREAM_WRITE_END];

    exec(interpreter, expr->stmt);
    close(fd[1]);
    /* restore original write fd */
    stream_ctx->write_fd = original_write_fd;

    // TODO: dynamic buffer
    char buffer[4096] = { 0 };
    read(fd[0], buffer, 4096);
    close(fd[0]);

    size_t size = strlen(buffer);
    char *str_view = scope_alloc(interpreter->scope, size);
    strncpy(str_view, buffer, size);
    return (SlashValue){ .type = SLASH_STR, .str = { .view = str_view, .size = size } };
}

static SlashValue eval_tuple(Interpreter *interpreter, SequenceExpr *expr)
{
    SlashTuple *tuple = (SlashTuple *)gc_alloc(interpreter, SLASH_OBJ_TUPLE);
    SlashValue value = { .type = SLASH_OBJ, .obj = (SlashObj *)tuple };
    // TODO: possible?
    if (expr->seq.size == 0) {
	slash_tuple_init(tuple, 0);
	return value;
    }

    slash_tuple_init(tuple, expr->seq.size);
    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	tuple->values[i++] = element_value;
    }

    return value;
}

static SlashValue eval_list(Interpreter *interpreter, ListExpr *expr)
{
    SlashList *list = (SlashList *)gc_alloc(interpreter, SLASH_OBJ_LIST);
    slash_list_init(list);
    SlashValue value = { .type = SLASH_OBJ, .obj = (SlashObj *)list };

    if (expr->exprs == NULL)
	return value;

    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->exprs->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	slash_list_append(list, element_value);
    }

    return value;
}

static SlashValue eval_map(Interpreter *interpreter, MapExpr *expr)
{
    SlashMap *map = (SlashMap *)gc_alloc(interpreter, SLASH_OBJ_MAP);
    slash_map_init(map);
    SlashValue value = { .type = SLASH_OBJ, .obj = (SlashObj *)map };

    if (expr->key_value_pairs == NULL)
	return value;

    LLItem *item;
    KeyValuePair *pair;
    ARENA_LL_FOR_EACH(expr->key_value_pairs, item)
    {
	pair = item->value;
	SlashValue k = eval(interpreter, pair->key);
	SlashValue v = eval(interpreter, pair->value);
	value.obj->traits->item_assign(&value, &k, &v);
    }

    return value;
}

static SlashValue eval_method(Interpreter *interpreter, MethodExpr *expr)
{
    SlashValue self = eval(interpreter, expr->obj);
    // TODO: fix cursed manual conversion from str_view to cstr
    size_t method_name_size = expr->method_name.size;
    char method_name[method_name_size + 1];
    memcpy(method_name, expr->method_name.view, method_name_size);
    method_name[method_name_size] = 0;

    /* get method */
    MethodFunc method = get_method(&self, method_name);
    if (method == NULL)
	report_runtime_error("Method does not exist");

    if (expr->args == NULL)
	return method(interpreter, &self, 0, NULL);

    SlashValue argv[expr->args->seq.size];
    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->args->seq, item)
    {
	SlashValue sv = eval(interpreter, item->value);
	argv[i++] = sv;
    }

    assert(i == expr->args->seq.size);
    return method(interpreter, &self, i, argv);
}

static SlashValue eval_grouping(Interpreter *interpreter, GroupingExpr *expr)
{
    return eval(interpreter, expr->expr);
}

static SlashValue eval_cast(Interpreter *interpreter, CastExpr *expr)
{
    SlashValue value = eval(interpreter, expr->expr);
    /*
     * When LHS is a subshell and RHS is boolean then the final exit code of the subshell
     * expression determines the boolean value.
     */
    if (expr->as == SLASH_BOOL && expr->expr->type == EXPR_SUBSHELL)
	return (SlashValue){ .type = SLASH_BOOL,
			     .boolean = interpreter->prev_exit_code == 0 ? true : false };
    return dynamic_cast(interpreter, value, expr->as);
}


/*
 * statement execution functions
 */
static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    TraitPrint print_func = trait_print[value.type];
    print_func(&value);
    // putchar('\n');
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    /* Make sure variable is not defined already */
    ScopeAndValue current = var_get(interpreter->scope, &stmt->name);
    if (current.scope == interpreter->scope)
	report_runtime_error("Variable redefinition");

    SlashValue value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name, &value);
}

static void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
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

    exec_block_body(interpreter, stmt);

    interpreter->scope = block_scope.enclosing;
    scope_destroy(&block_scope);
}

static void exec_subscript_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    SubscriptExpr *subscript = (SubscriptExpr *)stmt->var;
    /* this would mean assigning to an inline variable which would do nothing */
    if (subscript->expr->type != EXPR_ACCESS)
	return;

    AccessExpr *access = (AccessExpr *)subscript->expr;
    StrView var_name = access->var_name;
    SlashValue access_index = eval(interpreter, subscript->access_value);
    SlashValue new_value = eval(interpreter, stmt->value);

    ScopeAndValue current = var_get(interpreter->scope, &var_name);
    /* the underlying self who's index (access_index) we're trying to modify */
    SlashValue *self = current.value;

    if (stmt->assignment_op == t_equal) {
	TraitItemAssign func = trait_item_assign[self->type];
	func(self, &access_index, &new_value);
	return;
    }

    // TODO: this is inefficient as we have to re-eval the access_index
    SlashValue current_item_value = eval_subscript(interpreter, subscript);
    /* convert from += to + and -= to - */
    TokenType operator= stmt->assignment_op == t_plus_equal ? t_plus : t_minus;
    new_value = cmp_binary_values(current_item_value, new_value, operator);

    TraitItemAssign func = trait_item_assign[self->type];
    func(self, &access_index, &new_value);
}

static void exec_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    if (stmt->var->type == EXPR_SUBSCRIPT) {
	exec_subscript_assign(interpreter, stmt);
	return;
    }

    if (stmt->var->type != EXPR_ACCESS) {
	report_runtime_error("Internal error: bad expr type");
	ASSERT_NOT_REACHED;
    }

    AccessExpr *access = (AccessExpr *)stmt->var;
    StrView var_name = access->var_name;
    ScopeAndValue variable = var_get(interpreter->scope, &var_name);
    if (variable.value == NULL)
	report_runtime_error("Cannot modify undefined variable");

    SlashValue new_value = eval(interpreter, stmt->value);

    if (stmt->assignment_op == t_equal) {
	var_assign(&var_name, variable.scope, &new_value);
	return;
    }

    /* convert from assignment operator to correct binary operator */
    TokenType operator_ = t_none;
    switch (stmt->assignment_op) {
    case t_plus_equal:
	operator_ = t_plus;
	break;
    case t_minus_equal:
	operator_ = t_minus;
	break;
    case t_star_equal:
	operator_ = t_star;
	break;
    case t_star_star_equal:
	operator_ = t_star_star;
	break;
    case t_slash_equal:
	operator_ = t_slash;
	break;
    case t_slash_slash_equal:
	operator_ = t_slash_slash;
	break;
    case t_percent_equal:
	operator_ = t_percent;
	break;
    default:
	report_runtime_error("Assignment operator not supported");
	ASSERT_NOT_REACHED;
    }
    new_value = cmp_binary_values(*variable.value, new_value, operator_);
    /*
     * For dynamic types the binary compare will update the underlying object.
     * Therefore, we only assign if the variable type is not dynamic.
     */
    if (!IS_OBJ(variable.value->type))
	var_assign(&var_name, variable.scope, &new_value);
}

static void exec_pipeline(Interpreter *interpreter, PipelineStmt *stmt)
{
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = interpreter->stream_ctx;
    /* store a copy of the final write file descriptor */
    int final_out_fd = stream_ctx->write_fd;

    stream_ctx->write_fd = fd[STREAM_WRITE_END];
    exec_cmd(interpreter, stmt->left);

    /* push fie descriptors onto active_fds list/stack */
    arraylist_append(&stream_ctx->active_fds, &fd[0]);
    arraylist_append(&stream_ctx->active_fds, &fd[1]);

    stream_ctx->write_fd = final_out_fd;
    stream_ctx->read_fd = fd[STREAM_READ_END];

    exec(interpreter, stmt->right);

    /* pop file descriptors from list/stack */
    arraylist_rm(&stream_ctx->active_fds, stream_ctx->active_fds.size - 1);
    arraylist_rm(&stream_ctx->active_fds, stream_ctx->active_fds.size - 1);
}

static void exec_assert(Interpreter *interpreter, AssertStmt *stmt)
{
    SlashValue result = eval(interpreter, stmt->expr);
    if (!is_truthy(&result))
	report_runtime_error("Assertion failed");
}

static void exec_loop(Interpreter *interpreter, LoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue r = eval(interpreter, stmt->condition);
    while (is_truthy(&r)) {
	exec_block_body(interpreter, stmt->body_block);
	r = eval(interpreter, stmt->condition);
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

static void exec_iter_loop_list(Interpreter *interpreter, IterLoopStmt *stmt, SlashList *iterable)
{
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < iterable->underlying.size; i++) {
	iterator_value = slash_list_get(iterable, i);
	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
	exec_block_body(interpreter, stmt->body_block);
    }
}

static void exec_iter_loop_tuple(Interpreter *interpreter, IterLoopStmt *stmt, SlashTuple *iterable)
{
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < iterable->size; i++) {
	iterator_value = &iterable->values[i];
	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
	exec_block_body(interpreter, stmt->body_block);
    }
}

static void exec_iter_loop_map(Interpreter *interpreter, IterLoopStmt *stmt, SlashMap *iterable)
{
    if (iterable->underlying.len == 0)
	return;

    SlashValue keys = slash_map_get_keys(interpreter, iterable);
    SlashTuple *keys_tuple = (SlashTuple *)keys.obj;
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < keys_tuple->size; i++) {
	iterator_value = &keys_tuple->values[i];
	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
	exec_block_body(interpreter, stmt->body_block);
    }
}

static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, StrView iterable)
{
    StrView str = { .view = iterable.view, .size = 0 };
    SlashValue iterator_value = { .type = SLASH_STR, .str = str };

    // TODO: fix this I am writing under the influence
    char underlying[iterable.size + 1];
    memcpy(underlying, iterable.view, iterable.size);
    underlying[iterable.size] = 0;

    ScopeAndValue lol = var_get(interpreter->scope, &(StrView){ .view = "IFS", .size = 3 });
    StrView ifs = lol.value->str;

    char ifs_char[ifs.size + 1];
    memcpy(ifs_char, ifs.view, ifs.size);
    ifs_char[ifs.size] = 0;

    var_define(interpreter->scope, &stmt->var_name, &iterator_value);
    char *t = strtok(underlying, ifs_char);
    while (t != NULL) {
	str = (StrView){ .view = t, .size = strlen(t) };
	iterator_value.str = str;
	var_assign(&stmt->var_name, interpreter->scope, &iterator_value);
	exec_block_body(interpreter, stmt->body_block);
	t = strtok(NULL, ifs_char);
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange iterable)
{
    SlashValue iterator_value = { .type = SLASH_NUM, .num = iterable.start };

    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, &iterator_value);

    while (iterator_value.num != iterable.end) {
	exec_block_body(interpreter, stmt->body_block);
	iterator_value.num++;
	var_assign(&stmt->var_name, interpreter->scope, &iterator_value);
    }

    /* don't need to undefine the iterator value as the scope will be destroyed imminently */
}

static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue underlying = eval(interpreter, stmt->underlying_iterable);
    switch (underlying.type) {
    case SLASH_STR:
	exec_iter_loop_str(interpreter, stmt, underlying.str);
	break;
    case SLASH_RANGE:
	exec_iter_loop_range(interpreter, stmt, underlying.range);
	break;
    case SLASH_OBJ: {
	switch (underlying.obj->type) {
	case SLASH_OBJ_LIST:
	    exec_iter_loop_list(interpreter, stmt, (SlashList *)underlying.obj);
	    break;
	case SLASH_OBJ_TUPLE:
	    exec_iter_loop_tuple(interpreter, stmt, (SlashTuple *)underlying.obj);
	    break;
	case SLASH_OBJ_MAP:
	    exec_iter_loop_map(interpreter, stmt, (SlashMap *)underlying.obj);
	    break;
	default:
	    report_runtime_error("Object type can't be iterated over");
	    ASSERT_NOT_REACHED;
	}

    } break;
    default:
	report_runtime_error("Type can't be iterated over");
	ASSERT_NOT_REACHED;
    }

    interpreter->scope = loop_scope.enclosing;
    scope_destroy(&loop_scope);
}

static void exec_andor(Interpreter *interpreter, BinaryStmt *stmt)
{
    /*
     * L ( "&&" | "||" ) R.
     * If L is an expression statement then we use the result of expression statement
     * instead of the previous exit code.
     */

    bool predicate;
    if (stmt->left->type == STMT_EXPRESSION) {
	ExpressionStmt *left = (ExpressionStmt *)stmt->left;
	SlashValue value = eval(interpreter, left->expression);
	predicate = is_truthy(&value);
    } else {
	exec(interpreter, stmt->left);
	predicate = interpreter->prev_exit_code == 0 ? true : false;
    }

    if ((stmt->operator_ == t_anp_anp && predicate) ||
	(stmt->operator_ == t_pipe_pipe && !predicate))
	exec(interpreter, stmt->right_stmt);
}

static void exec_redirect(Interpreter *interpreter, BinaryStmt *stmt)
{
    // TODO: here we assume the cmd_stmt can NOT mutate the stmt->right_expr, however, this may not
    //      always be guaranteed ?.

    SlashValue value = eval(interpreter, stmt->right_expr);
    // TODO: to_str trait
    if (!(value.type == SLASH_STR || value.type == SLASH_SHIDENT))
	report_runtime_error("redirect failed: implement to_str trait!");
    char file_name[value.str.size + 1];
    str_view_to_cstr(value.str, file_name);

    StreamCtx *stream_ctx = interpreter->stream_ctx;
    int og_read = stream_ctx->read_fd;
    int og_write = stream_ctx->write_fd;

    bool new_write_fd = true;
    FILE *file = NULL;
    if (stmt->operator_ == t_greater) {
	file = fopen(file_name, "w");
    } else if (stmt->operator_ == t_greater_greater) {
	file = fopen(file_name, "a+");
    } else if (stmt->operator_ == t_less) {
	file = fopen(file_name, "r");
	new_write_fd = false;
    }

    // TODO: give good errors: "could not open file x: permission denied" or "x not a file"
    if (file == NULL) {
	report_runtime_error("could not open file 'PLACEHOLDER (TODO: implement better errors)'");
	ASSERT_NOT_REACHED;
    }

    if (new_write_fd)
	stream_ctx->write_fd = fileno(file);
    else
	stream_ctx->read_fd = fileno(file);
    exec_cmd(interpreter, (CmdStmt *)stmt->left);
    fclose(file);
    stream_ctx->read_fd = og_read;
    stream_ctx->write_fd = og_write;
}

static void exec_binary(Interpreter *interpreter, BinaryStmt *stmt)
{
    if (stmt->operator_ == t_anp_anp || stmt->operator_ == t_pipe_pipe)
	exec_andor(interpreter, stmt);
    else
	exec_redirect(interpreter, stmt);
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

    case EXPR_SUBSCRIPT:
	return eval_subscript(interpreter, (SubscriptExpr *)expr);

    case EXPR_SUBSHELL:
	return eval_subshell(interpreter, (SubshellExpr *)expr);

    case EXPR_LIST:
	return eval_list(interpreter, (ListExpr *)expr);

    case EXPR_MAP:
	return eval_map(interpreter, (MapExpr *)expr);

    case EXPR_METHOD:
	return eval_method(interpreter, (MethodExpr *)expr);

    case EXPR_SEQUENCE:
	return eval_tuple(interpreter, (SequenceExpr *)expr);

    case EXPR_GROUPING:
	return eval_grouping(interpreter, (GroupingExpr *)expr);

    case EXPR_CAST:
	return eval_cast(interpreter, (CastExpr *)expr);

    default:
	report_runtime_error("Expression type not handled");
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

    case STMT_PIPELINE:
	exec_pipeline(interpreter, (PipelineStmt *)stmt);
	break;

    case STMT_ASSERT:
	exec_assert(interpreter, (AssertStmt *)stmt);
	break;

    case STMT_BINARY:
	exec_binary(interpreter, (BinaryStmt *)stmt);
	break;


    default:
	report_runtime_error("Statement type not handled");
    }
}


int interpret(ArrayList *statements)
{
    Interpreter interpreter = { 0 };
    m_arena_init_dynamic(&interpreter.arena, 1, 16384);

    scope_init_global(&interpreter.globals, &interpreter.arena);
    interpreter.scope = &interpreter.globals;

    linkedlist_init(&interpreter.gc_objs, sizeof(SlashObj *));
    arraylist_init(&interpreter.gc_gray_stack, sizeof(SlashObj *));
    interpreter.obj_alloced_since_next_gc = 0;

    /* init default StreamCtx */
    StreamCtx stream_ctx = { .read_fd = STDIN_FILENO, .write_fd = STDOUT_FILENO };
    arraylist_init(&stream_ctx.active_fds, sizeof(int));
    interpreter.stream_ctx = &stream_ctx;

    for (size_t i = 0; i < statements->size; i++)
	exec(&interpreter, *(Stmt **)arraylist_get(statements, i));

    gc_collect_all(&interpreter.gc_objs);

    scope_destroy(&interpreter.globals);
    arraylist_free(&stream_ctx.active_fds);
    linkedlist_free(&interpreter.gc_objs);
    arraylist_free(&interpreter.gc_gray_stack);

    return interpreter.prev_exit_code;
}
