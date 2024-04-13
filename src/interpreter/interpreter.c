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
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtin/builtin.h"
#include "interpreter/ast.h"
#include "interpreter/error.h"
#include "interpreter/exec.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/scope.h"
#include "interpreter/value/cast.h"
/// #include "interpreter/value/method.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/type_funcs.h"
#include "lib/arena_ll.h"
#include "lib/str_builder.h"
#include "lib/str_view.h"
#include "nicc/nicc.h"
#include "sac/sac.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);
static void exec_block(Interpreter *interpreter, BlockStmt *stmt);

/*
 * helpers
 */
static void set_exit_code(Interpreter *interpreter, int exit_code)
{
    interpreter->prev_exit_code = exit_code;
    SlashValue value = { .T = &num_type_info, .num = interpreter->prev_exit_code };
    var_assign(&(StrView){ .view = "?", .size = 1 }, &interpreter->globals, &value);
}

static inline ExecResult consume_exec_result(Interpreter *interpreter)
{
    ExecResult tmp = interpreter->exec_res_ctx;
    interpreter->exec_res_ctx = EXEC_NORMAL;
    return tmp;
}


static ExecResult exec_block_body(Interpreter *interpreter, BlockStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->statements, item)
    {
	exec(interpreter, item->value);
	if (interpreter->exec_res_ctx.type != RT_NORMAL)
	    return consume_exec_result(interpreter);
    }
    return EXEC_NORMAL;
}

void exec_program_stub(Interpreter *interpreter, char *program_path, ArenaLL *ast_nodes)
{
    size_t argc = 1;
    if (ast_nodes != NULL)
	argc += ast_nodes->size;
    char *argv[argc + 1]; // + 1 because last element is NULL
    argv[0] = program_path;

    gc_barrier_start(&interpreter->gc);
    size_t i = 1;
    if (ast_nodes != NULL) {
	LLItem *item;
	ARENA_LL_FOR_EACH(ast_nodes, item)
	{
	    SlashValue value = eval(interpreter, item->value);
	    VERIFY_TRAIT_IMPL(to_str, value, "Could not take 'to_str' of type '%s'", value.T->name);
	    SlashValue value_str_repr = value.T->to_str(interpreter, value);
	    argv[i++] = AS_STR(value_str_repr)->str;
	}
    }

    argv[i] = NULL;
    gc_barrier_end(&interpreter->gc);
    int exit_code = exec_program(&interpreter->stream_ctx, argv);
    set_exit_code(interpreter, exit_code);
}

static SlashValue eval_binary_operators(Interpreter *interpreter, SlashValue left, SlashValue right,
					TokenType op)
{
    if (IS_NONE(left) && !IS_NONE(right))
	return (SlashValue){ .T = &bool_type_info, .boolean = false };

    if (!TYPE_EQ(left, right))
	REPORT_RUNTIME_ERROR("Binary operation failed: type mismatch between '%s' and '%s'",
			     left.T->name, right.T->name);

    switch (op) {
    case t_greater:
	VERIFY_TRAIT_IMPL(cmp, left, "'>' operator not defined for type '%s'", right.T->name);
	return (SlashValue){ .T = &bool_type_info, .boolean = left.T->cmp(left, right) > 0 };
    case t_greater_equal:
	VERIFY_TRAIT_IMPL(cmp, left, "'>=' operator not defined for type '%s'", right.T->name);
	return (SlashValue){ .T = &bool_type_info, .boolean = (left.T->cmp(left, right) >= 0) };
    case t_less:
	VERIFY_TRAIT_IMPL(cmp, left, "'<' operator not defined for type '%s'", right.T->name);
	return (SlashValue){ .T = &bool_type_info, .boolean = (left.T->cmp(left, right) < 0) };
    case t_less_equal:
	VERIFY_TRAIT_IMPL(cmp, left, "'<=' operator not defined for type '%s'", right.T->name);
	return (SlashValue){ .T = &bool_type_info, .boolean = (left.T->cmp(left, right) <= 0) };
    case t_plus:
    case t_plus_equal:
	VERIFY_TRAIT_IMPL(plus, left, "'+' operator not defined for type '%s'", right.T->name);
	return left.T->plus(interpreter, left, right);
    case t_minus:
    case t_minus_equal:
	VERIFY_TRAIT_IMPL(minus, left, "'-' operator not defined for type '%s'", right.T->name);
	return left.T->minus(left, right);
    case t_slash:
    case t_slash_equal:
	VERIFY_TRAIT_IMPL(div, left, "'/' operator not defined for type '%s'", right.T->name);
	return left.T->div(left, right);
    case t_slash_slash:
    case t_slash_slash_equal:
	VERIFY_TRAIT_IMPL(int_div, left, "'//' operator not defined for type '%s'", right.T->name);
	return left.T->int_div(left, right);
    case t_percent:
    case t_percent_equal:
	VERIFY_TRAIT_IMPL(mod, left, "'%%' operator not defined for type '%s'", right.T->name);
	return left.T->mod(left, right);
    case t_star:
    case t_star_equal:
	VERIFY_TRAIT_IMPL(mul, left, "'*' operator not defined for type '%s'", right.T->name);
	return left.T->mul(interpreter, left, right);
    case t_star_star:
    case t_star_star_equal:
	VERIFY_TRAIT_IMPL(mul, left, "'**' operator not defined for type '%s'", right.T->name);
	return left.T->pow(left, right);
    case t_equal_equal:
	return (SlashValue){ .T = &bool_type_info, .boolean = left.T->eq(left, right) };
    case t_bang_equal:
	return (SlashValue){ .T = &bool_type_info, .boolean = !left.T->eq(left, right) };

    default:
	REPORT_RUNTIME_ERROR("Unrecognized binary operator");
    }

    ASSERT_NOT_REACHED;
    return NoneSingleton;
}


/*
 * expression evaluation functions
 */
static SlashValue eval_unary(Interpreter *interpreter, UnaryExpr *expr)
{
    SlashValue right = eval(interpreter, expr->right);
    if (expr->operator_ == t_not) {
	VERIFY_TRAIT_IMPL(unary_not, right, "'not' operator not defined for type '%s'",
			  right.T->name);
	return right.T->unary_not(right);
    } else if (expr->operator_ == t_minus) {
	VERIFY_TRAIT_IMPL(unary_minus, right, "Unary '-' not defined for type '%s'", right.T->name);
	return right.T->unary_minus(right);
    }

    REPORT_RUNTIME_ERROR("Internal error: Unsupported unary operator parsed correctly.");
    ASSERT_NOT_REACHED;
    return NoneSingleton; // make the compiler happy
}

static SlashValue eval_binary(Interpreter *interpreter, BinaryExpr *expr)
{
    gc_barrier_start(&interpreter->gc);
    SlashValue left = eval(interpreter, expr->left);
    SlashValue right;
    SlashValue return_value;

    /* logical operators */
    if (expr->operator_ == t_and) {
	if (!left.T->truthy(left)) {
	    return_value = (SlashValue){ .T = &bool_type_info, .boolean = false };
	    goto defer_gc_barrier_end;
	}
	right = eval(interpreter, expr->right);
	return_value = (SlashValue){ .T = &bool_type_info, .boolean = right.T->truthy(right) };
	goto defer_gc_barrier_end;
    }
    right = eval(interpreter, expr->right);
    if (expr->operator_ == t_or) {
	bool truthy = left.T->truthy(left) || right.T->truthy(right);
	return_value = (SlashValue){ .T = &bool_type_info, .boolean = truthy };
	goto defer_gc_barrier_end;
    }

    /* range initializer */
    if (expr->operator_ == t_dot_dot) {
	if (!(IS_NUM(left) && NUM_IS_INT(left) && IS_NUM(right) && NUM_IS_INT(right)))
	    REPORT_RUNTIME_ERROR("Bad range initializer");
	SlashRange range = { .start = left.num, .end = right.num };
	return_value = (SlashValue){ .T = &range_type_info, .range = range };
	goto defer_gc_barrier_end;
    }

    /* binary operators */
    if (expr->operator_ != t_in) {
	return_value = eval_binary_operators(interpreter, left, right, expr->operator_);
	goto defer_gc_barrier_end;
    }

    /* left "IN" right */
    VERIFY_TRAIT_IMPL(item_in, right, "'in' operator not defined for type '%s'", right.T->name);
    bool rc = right.T->item_in(right, left);
    return_value = (SlashValue){ .T = &bool_type_info, .boolean = rc };


defer_gc_barrier_end:
    gc_barrier_end(&interpreter->gc);
    return return_value;
}

static SlashValue eval_literal(Interpreter *interpreter, LiteralExpr *expr)
{
    (void)interpreter;
    return expr->value;
}

static SlashValue eval_access(Interpreter *interpreter, AccessExpr *expr)
{
    ScopeAndValue sv = var_get_or_runtime_error(interpreter->scope, &expr->var_name);
    /* If variable is not defined, then return None. Same behaviour as POSIX sh I think */
    if (sv.value == NULL)
	return NoneSingleton;

    return *sv.value;
}

static SlashValue eval_subscript(Interpreter *interpreter, SubscriptExpr *expr)
{
    SlashValue value = eval(interpreter, expr->expr);
    SlashValue access_index = eval(interpreter, expr->access_value);
    VERIFY_TRAIT_IMPL(item_get, value, "'[]' operator not defined for type '%s'", value.T->name);
    return value.T->item_get(interpreter, value, access_index);
}

static SlashValue eval_subshell(Interpreter *interpreter, SubshellExpr *expr)
{
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = &interpreter->stream_ctx;
    int original_write_fd = stream_ctx->out_fd;
    /* set the write fd to the newly created pipe */
    stream_ctx->out_fd = fd[STREAM_WRITE_END];

    exec(interpreter, expr->stmt);
    close(fd[1]);
    /* restore original write fd */
    stream_ctx->out_fd = original_write_fd;

    ArenaTmp scratch_arena = m_arena_tmp_init(&interpreter->arena);
    size_t bytes_read = 0;
    char buffer[4096] = { 0 };
    StrBuilder sb;
    str_builder_init(&sb, scratch_arena.arena);
    while ((bytes_read = read(fd[0], buffer, 4096)) > 0)
	str_builder_append(&sb, buffer, bytes_read);

    close(fd[0]);

    if (str_builder_peek(&sb) == '\n')
	sb.len--;

    StrView final = str_builder_complete(&sb);
    SlashStr *str = (SlashStr *)gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_view(interpreter, str, &final);
    m_arena_tmp_release(scratch_arena);

    return AS_VALUE(str);
}

static SlashValue eval_tuple(Interpreter *interpreter, SequenceExpr *expr)
{
    gc_barrier_start(&interpreter->gc);
    SlashTuple *tuple = (SlashTuple *)gc_new_T(interpreter, &tuple_type_info);
    slash_tuple_init(interpreter, tuple, expr->seq.size);
    if (expr->seq.size == 0) {
	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(tuple);
    }

    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	tuple->items[i++] = element_value;
    }

    gc_barrier_end(&interpreter->gc);
    return AS_VALUE(tuple);
}

static SlashValue eval_str(Interpreter *interpreter, StrExpr *expr)
{
    SlashStr *str = (SlashStr *)gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_view(interpreter, str, &expr->view);
    return AS_VALUE(str);
}

static SlashValue eval_list(Interpreter *interpreter, ListExpr *expr)
{
    gc_barrier_start(&interpreter->gc);
    SlashList *list = (SlashList *)gc_new_T(interpreter, &list_type_info);
    slash_list_impl_init(interpreter, list);
    if (expr->exprs == NULL) {
	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(list);
    }

    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->exprs->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	slash_list_impl_append(interpreter, list, element_value);
    }

    gc_barrier_end(&interpreter->gc);
    return AS_VALUE(list);
}

static SlashValue eval_function(Interpreter *interpreter, FunctionExpr *expr)
{
    /*
     * SlashFunction is "special" in that it holds pointers to the AST and source (through StrViews)
     * When using the REPL, the AST will and source will be reset on each new command, meaning we
     * have to make copies of the params and function body.
     */
    ArenaLL params;
    arena_ll_init(interpreter->scope->arena_tmp.arena, &params);
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->params, item)
    {
	StrView *param = item->value;
	StrView *param_cpy = scope_alloc(interpreter->scope, sizeof(StrView));
	*param_cpy = str_view_arena_copy(interpreter->scope->arena_tmp.arena, *param);
	arena_ll_append(&params, param);
    }

    Stmt *body_cpy = stmt_copy(interpreter->scope->arena_tmp.arena, (Stmt *)expr->body);
    SlashFunction function = { .params = params, .body = (BlockStmt *)body_cpy };
    return (SlashValue){ .T = &function_type_info, .function = function };
}


static SlashValue eval_map(Interpreter *interpreter, MapExpr *expr)
{
    gc_barrier_start(&interpreter->gc);
    SlashMap *map = (SlashMap *)gc_new_T(interpreter, &map_type_info);
    slash_map_impl_init(interpreter, map);
    if (expr->key_value_pairs == NULL) {
	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(map);
    }

    LLItem *item;
    KeyValuePair *pair;
    ARENA_LL_FOR_EACH(expr->key_value_pairs, item)
    {
	pair = item->value;
	SlashValue k = eval(interpreter, pair->key);
	SlashValue v = eval(interpreter, pair->value);
	slash_map_impl_put(interpreter, map, k, v);
    }

    gc_barrier_end(&interpreter->gc);
    return AS_VALUE(map);
}

/// static SlashValue eval_method(Interpreter *interpreter, MethodExpr *expr)
///{
///     SlashValue self = eval(interpreter, expr->obj);
///     if (IS_OBJ(self.type))
///	gc_shadow_push(&interpreter->gc_shadow_stack, self.obj);
///
///     // TODO: fix cursed manual conversion from str_view to cstr
///     size_t method_name_size = expr->method_name.size;
///     char method_name[method_name_size + 1];
///     memcpy(method_name, expr->method_name.view, method_name_size);
///     method_name[method_name_size] = 0;
///
///     /* get method */
///     MethodFunc method = get_method(&self, method_name);
///     if (method == NULL)
///	REPORT_RUNTIME_ERROR("Method '%s' does not exist on type '%s'", method_name,
///			     SLASH_TYPE_TO_STR(&self));
///
///     SlashValue return_value;
///     size_t shadow_push_count = 0;
///     if (expr->args == NULL) {
///	return_value = method(interpreter, &self, 0, NULL);
///     } else {
///	SlashValue argv[expr->args->seq.size];
///	size_t i = 0;
///	LLItem *item;
///	ARENA_LL_FOR_EACH(&expr->args->seq, item)
///	{
///	    SlashValue sv = eval(interpreter, item->value);
///	    if (IS_OBJ(sv.type)) {
///		gc_shadow_push(&interpreter->gc_shadow_stack, sv.obj);
///		shadow_push_count++;
///	    }
///	    argv[i++] = sv;
///	}
///
///	assert(i == expr->args->seq.size);
///	return_value = method(interpreter, &self, i, argv);
///     }
///
///     for (size_t n = 0; n < shadow_push_count; n++)
///	gc_shadow_pop(&interpreter->gc_shadow_stack);
///     if (IS_OBJ(self.type))
///	gc_shadow_pop(&interpreter->gc_shadow_stack);
///
///     return return_value;
/// }
///
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
    if (hashmap_get(&interpreter->type_register, expr->type_name.view, expr->type_name.size) ==
	    &bool_type_info &&
	expr->expr->type == EXPR_SUBSHELL)
	return (SlashValue){ .T = &bool_type_info,
			     .boolean = interpreter->prev_exit_code == 0 ? true : false };

    return dynamic_cast(interpreter, value, expr->type_name);
}

static SlashValue eval_call(Interpreter *interpreter, CallExpr *expr)
{
    SlashValue callee = eval(interpreter, expr->callee);
    if (!IS_FUNCTION(callee))
	REPORT_RUNTIME_ERROR("Can not call value of type '%s'", callee.T->name);

    SlashFunction function = callee.function;
    size_t call_params_size = expr->args == NULL ? 0 : expr->args->seq.size;
    /* Arity check */
    if (function.params.size != call_params_size)
	REPORT_RUNTIME_ERROR("Function 'FOO' takes '%zu' arguments, but '%zu' where given",
			     function.params.size, call_params_size)

    Scope *function_scope = scope_alloc(interpreter->scope, sizeof(Scope));
    scope_init(function_scope, interpreter->scope);
    interpreter->scope = function_scope;
    /* Assign arguments */
    if (function.params.size != 0) {
	LLItem *param = function.params.head;
	LLItem *arg = expr->args->seq.head;
	for (; param != NULL; param = param->next, arg = arg->next) {
	    SlashValue arg_value = eval(interpreter, (Expr *)arg->value);
	    var_define(interpreter->scope, (StrView *)param->value, &arg_value);
	}
    }

    SlashValue return_value = NoneSingleton;
    ExecResult result = exec_block_body(interpreter, function.body);
    if (result.type == RT_RETURN && result.return_expr != NULL)
	return_value = eval(interpreter, result.return_expr);
    interpreter->scope = function_scope->enclosing;
    scope_destroy(function_scope);
    return return_value;
}


/*
 * statement execution functions
 */
static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    if (stmt->expression->type == EXPR_CALL)
	return;

    TraitPrint trait_print = value.T->print;
    VERIFY_TRAIT_IMPL(print, value, "TODO");
    trait_print(interpreter, value);
    ///* edge case: if last char printed was a newline then we don't bother printing one */
    // if (value.type == SLASH_OBJ && value.obj->type == SLASH_OBJ_STR) {
    //     SlashStr *str = (SlashStr *)value.obj;
    //     if (slash_str_last_char(str) == '\n')
    //         return;
    // }
    SLASH_PRINT(&interpreter->stream_ctx, "\n");
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    /* Make sure variable is not defined already */
    ScopeAndValue current = var_get(interpreter->scope, &stmt->name);
    if (current.scope == interpreter->scope) {
	str_view_to_buf_cstr(stmt->name); // creates buf variable
	REPORT_RUNTIME_ERROR("Redefinition of '%s'", buf);
    }

    SlashValue value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name, &value);
}

static void exec_seq_var(Interpreter *interpreter, SeqVarStmt *stmt)
{
    SequenceExpr *initializer = (SequenceExpr *)stmt->initializer;
    if (initializer->type == EXPR_SEQUENCE) {
	if (stmt->names.size != initializer->seq.size)
	    REPORT_RUNTIME_ERROR("Unpacking only supported for collections of the same size");

	LLItem *l = stmt->names.head;
	LLItem *r = initializer->seq.head;
	for (size_t i = 0; i < stmt->names.size; i++) {
	    StrView *name = l->value;
	    ScopeAndValue current = var_get(interpreter->scope, name);
	    if (current.scope == interpreter->scope) {
		str_view_to_buf_cstr(*name); // creates buf variable
		REPORT_RUNTIME_ERROR("Redefinition of '%s'", buf);
	    }
	    SlashValue value = eval(interpreter, (Expr *)r->value);
	    var_define(interpreter->scope, name, &value);
	    l = l->next;
	    r = r->next;
	}
	return;
    }

    SlashValue initializer_value = eval(interpreter, stmt->initializer);
    if (!IS_TUPLE(initializer_value))
	REPORT_RUNTIME_ERROR("Multiple variable declaration only supported for tuples");
    SlashTuple *values = AS_TUPLE(initializer_value);

    LLItem *item;
    size_t i = 0;
    ARENA_LL_FOR_EACH(&stmt->names, item)
    {
	StrView *name = item->value;
	ScopeAndValue current = var_get(interpreter->scope, name);
	if (current.scope == interpreter->scope) {
	    str_view_to_buf_cstr(*name); // creates buf variable
	    REPORT_RUNTIME_ERROR("Redefinition of '%s'", buf);
	}
	var_define(interpreter->scope, name, &values->items[i]);
	i++;
    }
}

void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
    ScopeAndValue path =
	var_get_or_runtime_error(interpreter->scope, &(StrView){ .view = "PATH", .size = 4 });
    if (!IS_STR(*path.value))
	REPORT_RUNTIME_ERROR("PATH variable should be type '%s' not '%s'", str_type_info.name,
			     path.value->T->name);

    WhichResult which_result = which(stmt->cmd_name, AS_STR(*path.value)->str);
    if (which_result.type == WHICH_NOT_FOUND) {
	str_view_to_buf_cstr(stmt->cmd_name); // creates temporary buf variable
	REPORT_RUNTIME_ERROR("Command '%s' not found", buf);
    }

    if (which_result.type == WHICH_EXTERN)
	exec_program_stub(interpreter, which_result.path, stmt->arg_exprs);
    else
	which_result.builtin(interpreter, stmt->arg_exprs);
}

static void exec_if(Interpreter *interpreter, IfStmt *stmt)
{
    SlashValue r = eval(interpreter, stmt->condition);
    if (r.T->truthy(r))
	exec(interpreter, stmt->then_branch);
    else if (stmt->else_branch != NULL)
	exec(interpreter, stmt->else_branch);
}

/*
 * Should not be called in a loop.
 * For exec'ing blocks in a loop, see exec_loop and exec_block_body
 */
static void exec_block(Interpreter *interpreter, BlockStmt *stmt)
{
    Scope *block_scope = scope_alloc(interpreter->scope, sizeof(Scope));
    scope_init(block_scope, interpreter->scope);
    interpreter->scope = block_scope;

    /* Permeate any abrupt control flow */
    interpreter->exec_res_ctx = exec_block_body(interpreter, stmt);

    interpreter->scope = block_scope->enclosing;
    interpreter->scope->arena_tmp.arena->offset -= sizeof(Scope);
    scope_destroy(block_scope);
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

    ScopeAndValue current = var_get_or_runtime_error(interpreter->scope, &var_name);
    /* the underlying self who's index (access_index) we're trying to modify */
    SlashValue self = *current.value;

    if (stmt->assignment_op == t_equal) {
	VERIFY_TRAIT_IMPL(item_assign, self, "Item assignment not defined for type '%s'",
			  self.T->name);
	self.T->item_assign(interpreter, self, access_index, new_value);
	return;
    }

    // TODO: this is inefficient as we have to re-eval the access_index
    SlashValue current_item_value = eval_subscript(interpreter, subscript);
    new_value =
	eval_binary_operators(interpreter, current_item_value, new_value, stmt->assignment_op);
    VERIFY_TRAIT_IMPL(item_assign, self, "Item assignment not defined for type '%s'", self.T->name);
    self.T->item_assign(interpreter, self, access_index, new_value);
}

static void exec_assign_unpack(Interpreter *interpreter, AssignStmt *stmt)
{
    SequenceExpr *left = (SequenceExpr *)stmt->var;
    // TODO: add support for list and maps
    if (stmt->value->type != EXPR_SEQUENCE) {
	REPORT_RUNTIME_ERROR("Unpacking only supported for tuples");
	ASSERT_NOT_REACHED;
    }
    SequenceExpr *right = (SequenceExpr *)stmt->value;
    if (left->seq.size != right->seq.size) {
	REPORT_RUNTIME_ERROR("Unpacking only supported for collections of the same size");
	ASSERT_NOT_REACHED;
    }

    /* early eval all values on the right side of assignment */
    SlashValue values[right->seq.size];
    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(&right->seq, item)
    {
	// TODO: can have problems with GC?
	values[i++] = eval(interpreter, (Expr *)item->value);
    }

    i = 0;
    ARENA_LL_FOR_EACH(&left->seq, item)
    {
	AccessExpr *access = (AccessExpr *)item->value;
	if (access->type != EXPR_ACCESS)
	    REPORT_RUNTIME_ERROR("Can not assign to literal value");
	ScopeAndValue variable = var_get_or_runtime_error(interpreter->scope, &access->var_name);
	var_assign(&access->var_name, variable.scope, &values[i++]);
    }
}

static void exec_assign(Interpreter *interpreter, AssignStmt *stmt)
{
    if (stmt->var->type == EXPR_SUBSCRIPT) {
	exec_subscript_assign(interpreter, stmt);
	return;
    }
    if (stmt->var->type == EXPR_SEQUENCE) {
	exec_assign_unpack(interpreter, stmt);
	return;
    }
    if (stmt->var->type != EXPR_ACCESS) {
	REPORT_RUNTIME_ERROR("Can not assign to a literal");
	ASSERT_NOT_REACHED;
    }

    AccessExpr *access = (AccessExpr *)stmt->var;
    StrView var_name = access->var_name;
    ScopeAndValue variable = var_get_or_runtime_error(interpreter->scope, &var_name);
    SlashValue new_value = eval(interpreter, stmt->value);

    if (stmt->assignment_op == t_equal) {
	var_assign(&var_name, variable.scope, &new_value);
	return;
    }
    new_value = eval_binary_operators(interpreter, *variable.value, new_value, stmt->assignment_op);
    var_assign(&var_name, variable.scope, &new_value);
}

static void exec_pipeline(Interpreter *interpreter, PipelineStmt *stmt)
{
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = &interpreter->stream_ctx;
    /* store a copy of the final write file descriptor */
    int final_out_fd = stream_ctx->out_fd;

    stream_ctx->out_fd = fd[STREAM_WRITE_END];
    exec_cmd(interpreter, stmt->left);

    /* push fie descriptors onto active_fds list/stack */
    arraylist_append(&stream_ctx->active_fds, &fd[0]);
    arraylist_append(&stream_ctx->active_fds, &fd[1]);

    stream_ctx->out_fd = final_out_fd;
    stream_ctx->in_fd = fd[STREAM_READ_END];

    exec(interpreter, stmt->right);

    /* pop file descriptors from list/stack */
    arraylist_pop(&stream_ctx->active_fds);
    arraylist_pop(&stream_ctx->active_fds);
}

static void exec_assert(Interpreter *interpreter, AssertStmt *stmt)
{
    SlashValue result = eval(interpreter, stmt->expr);
    TraitTruthy truthy_func = result.T->truthy;
    if (!truthy_func(result))
	REPORT_RUNTIME_ERROR("Assertion failed");
}

static void exec_loop(Interpreter *interpreter, LoopStmt *stmt)
{
    Scope *block_scope = scope_alloc(interpreter->scope, sizeof(Scope));
    scope_init(block_scope, interpreter->scope);
    interpreter->scope = block_scope;

    SlashValue r = eval(interpreter, stmt->condition);
    TraitTruthy truthy_func = r.T->truthy;
    while (truthy_func(r)) {
	ExecResultType result_t = exec_block_body(interpreter, stmt->body_block).type;
	if (result_t == RT_BREAK)
	    break;
	if (result_t == RT_CONTINUE) {
	    scope_reset(block_scope);
	    continue;
	}
	r = eval(interpreter, stmt->condition);
	scope_reset(block_scope);
    }

    scope_destroy(block_scope);
}

static void exec_iter_loop_list(Interpreter *interpreter, IterLoopStmt *stmt, SlashList *iterable)
{
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue iterator_value;
    for (size_t i = 0; i < iterable->len; i++) {
	iterator_value = slash_list_impl_get(iterable, i);
	var_assign(&stmt->var_name, interpreter->scope, &iterator_value);
	ExecResultType result_t = exec_block_body(interpreter, stmt->body_block).type;
	scope_reset(interpreter->scope);
	if (result_t == RT_BREAK)
	    break;
    }
}

static void exec_iter_loop_tuple(Interpreter *interpreter, IterLoopStmt *stmt, SlashTuple *iterable)
{
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < iterable->len; i++) {
	iterator_value = &iterable->items[i];
	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
	ExecResultType result_t = exec_block_body(interpreter, stmt->body_block).type;
	scope_reset(interpreter->scope);
	if (result_t == RT_BREAK)
	    break;
    }
}

static void exec_iter_loop_map(Interpreter *interpreter, IterLoopStmt *stmt, SlashMap *iterable)
{
    if (iterable->len == 0)
	return;

    SlashValue keys[iterable->len];
    slash_map_impl_get_keys(iterable, keys);
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, NULL);

    SlashValue *iterator_value;
    for (size_t i = 0; i < iterable->len; i++) {
	iterator_value = &keys[i];
	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
	ExecResultType result_t = exec_block_body(interpreter, stmt->body_block).type;
	scope_reset(interpreter->scope);
	if (result_t == RT_BREAK)
	    break;
    }
}

static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, SlashStr *iterable)
{
    ScopeAndValue ifs_res =
	var_get_or_runtime_error(interpreter->scope, &(StrView){ .view = "IFS", .size = 3 });
    if (!IS_STR(*ifs_res.value))
	REPORT_RUNTIME_ERROR("$IFS has to be of type 'str', but got '%s'", ifs_res.value->T->name);

    SlashStr *ifs = AS_STR(*ifs_res.value);
    SlashList *substrings = slash_str_split(interpreter, iterable, ifs->str, true);
    gc_shadow_push(&interpreter->gc, &substrings->obj);
    exec_iter_loop_list(interpreter, stmt, substrings);
    gc_shadow_pop(&interpreter->gc);
}

static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange iterable)
{
    if (iterable.start >= iterable.end)
	return;

    SlashValue iterator_value = { .T = &num_type_info, .num = iterable.start };
    /* define the loop variable that holds the current iterator value */
    var_define(interpreter->scope, &stmt->var_name, &iterator_value);

    while (iterator_value.num != iterable.end) {
	ExecResultType result_t = exec_block_body(interpreter, stmt->body_block).type;
	scope_reset(interpreter->scope);
	iterator_value.num++;
	var_assign(&stmt->var_name, interpreter->scope, &iterator_value);
	if (result_t == RT_BREAK)
	    break;
    }
}

static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
{
    Scope loop_scope;
    scope_init(&loop_scope, interpreter->scope);
    interpreter->scope = &loop_scope;

    SlashValue underlying = eval(interpreter, stmt->underlying_iterable);
    if (IS_OBJ(underlying))
	gc_shadow_push(&interpreter->gc, underlying.obj);

    if (IS_RANGE(underlying))
	exec_iter_loop_range(interpreter, stmt, underlying.range);
    else if (IS_LIST(underlying))
	exec_iter_loop_list(interpreter, stmt, AS_LIST(underlying));
    else if (IS_TUPLE(underlying))
	exec_iter_loop_tuple(interpreter, stmt, AS_TUPLE(underlying));
    else if (IS_MAP(underlying))
	exec_iter_loop_map(interpreter, stmt, AS_MAP(underlying));
    else if (IS_STR(underlying))
	exec_iter_loop_str(interpreter, stmt, AS_STR(underlying));
    else
	REPORT_RUNTIME_ERROR("Type '%s' can not be iterated over", underlying.T->name);

    if (IS_OBJ(underlying))
	gc_shadow_pop(&interpreter->gc);
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
	TraitTruthy truthy_func = value.T->truthy;
	if (truthy_func == NULL)
	    REPORT_RUNTIME_ERROR("&& or || failed becuase truthy is not defined for type '%s'",
				 value.T->name);
	predicate = truthy_func(value);
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
    // TODO: here we assume the cmd_stmt can NOT mutate the stmt->right_expr, however, this may
    //       not always be guaranteed ?.
    SlashValue value = eval(interpreter, stmt->right_expr);
    TraitToStr to_str = value.T->to_str;
    if (to_str == NULL)
	REPORT_RUNTIME_ERROR("Redirection failed because to_str is not defined for type '%s'",
			     value.T->name);

    char *file_name = AS_STR(to_str(interpreter, value))->str;
    StreamCtx *stream_ctx = &interpreter->stream_ctx;
    int og_read = stream_ctx->in_fd;
    int og_write = stream_ctx->out_fd;

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
	REPORT_RUNTIME_ERROR("Could not open file '%s'", file_name);
	ASSERT_NOT_REACHED;
    }

    if (new_write_fd)
	stream_ctx->out_fd = fileno(file);
    else
	stream_ctx->in_fd = fileno(file);
    exec_cmd(interpreter, (CmdStmt *)stmt->left);
    fclose(file);
    stream_ctx->in_fd = og_read;
    stream_ctx->out_fd = og_write;
}

static void exec_binary(Interpreter *interpreter, BinaryStmt *stmt)
{
    if (stmt->operator_ == t_anp_anp || stmt->operator_ == t_pipe_pipe)
	exec_andor(interpreter, stmt);
    else
	exec_redirect(interpreter, stmt);
}

static void exec_abrupt_control_flow(Interpreter *interpreter, AbruptControlFlowStmt *stmt)
{
    ExecResult result = { .type = RT_BREAK };
    if (stmt->ctrlf_type == t_continue) {
	result.type = RT_CONTINUE;
    } else {
	result.type = RT_RETURN;
	result.return_expr = stmt->return_expr;
    }
    interpreter->exec_res_ctx = result;
}

void ast_ll_to_argv(Interpreter *interpreter, ArenaLL *ast_nodes, SlashValue *result)
{
    gc_barrier_start(&interpreter->gc);
    LLItem *item = ast_nodes->head;
    for (size_t i = 0; i < ast_nodes->size; i++) {
	SlashValue value = eval(interpreter, (Expr *)item->value);
	result[i] = value;
	item = item->next;
    }
    gc_barrier_end(&interpreter->gc);
}

static SlashValue eval(Interpreter *interpreter, Expr *expr)
{
    interpreter->source_line = expr->source_line;
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
    case EXPR_STR:
	return eval_str(interpreter, (StrExpr *)expr);
    case EXPR_LIST:
	return eval_list(interpreter, (ListExpr *)expr);
    case EXPR_FUNCTION:
	return eval_function(interpreter, (FunctionExpr *)expr);
    case EXPR_MAP:
	return eval_map(interpreter, (MapExpr *)expr);
	/// case EXPR_METHOD:
	///	return eval_method(interpreter, (MethodExpr *)expr);
    case EXPR_SEQUENCE:
	return eval_tuple(interpreter, (SequenceExpr *)expr);
    case EXPR_GROUPING:
	return eval_grouping(interpreter, (GroupingExpr *)expr);
    case EXPR_CAST:
	return eval_cast(interpreter, (CastExpr *)expr);
    case EXPR_CALL:
	return eval_call(interpreter, (CallExpr *)expr);
    default:
	REPORT_RUNTIME_ERROR("Internal error: expression type not recognized");
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
    case STMT_SEQ_VAR:
	exec_seq_var(interpreter, (SeqVarStmt *)stmt);
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
    case STMT_ABRUPT_CONTROL_FLOW:
	exec_abrupt_control_flow(interpreter, (AbruptControlFlowStmt *)stmt);
	break;
    default:
	REPORT_RUNTIME_ERROR("Internal error: Statement type not recognized");
    }
}

void interpreter_init(Interpreter *interpreter, int argc, char **argv)
{
    m_arena_init_dynamic(&interpreter->arena, 1, 16384);

    scope_init_globals(&interpreter->globals, &interpreter->arena, argc, argv);
    interpreter->scope = &interpreter->globals;

    gc_ctx_init(&interpreter->gc);

    hashmap_init(&interpreter->type_register);
    /* Populate type register with all the builtin types types found in Slash */
    hashmap_put(&interpreter->type_register, "bool", sizeof("bool") - 1, &bool_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "num", sizeof("num") - 1, &num_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "range", sizeof("range") - 1, &range_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "text_lit", sizeof("text_lit") - 1,
		&text_lit_type_info, sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "list", sizeof("list") - 1, &list_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "tuple", sizeof("tuple") - 1, &tuple_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "str", sizeof("str") - 1, &str_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "map", sizeof("map") - 1, &map_type_info,
		sizeof(SlashTypeInfo), false);
    hashmap_put(&interpreter->type_register, "none", sizeof("none") - 1, &none_type_info,
		sizeof(SlashTypeInfo), false);

    /* 'Truhty' and 'eq' traits must be implemented for each type */
    assert(bool_type_info.eq != NULL && bool_type_info.truthy != NULL);
    assert(num_type_info.eq != NULL && num_type_info.truthy != NULL);
    assert(range_type_info.eq != NULL && range_type_info.truthy != NULL);
    assert(list_type_info.eq != NULL && list_type_info.truthy != NULL);
    assert(tuple_type_info.eq != NULL && tuple_type_info.truthy != NULL);
    assert(str_type_info.eq != NULL && str_type_info.truthy != NULL);
    assert(map_type_info.eq != NULL && map_type_info.truthy != NULL);
    assert(none_type_info.eq != NULL && none_type_info.truthy != NULL);

    /* Init default StreamCtx */
    StreamCtx stream_ctx = { .in_fd = STDIN_FILENO,
			     .out_fd = STDOUT_FILENO,
			     .err_fd = STDERR_FILENO };
    arraylist_init(&stream_ctx.active_fds, sizeof(int));
    interpreter->stream_ctx = stream_ctx;

    interpreter->exec_res_ctx = EXEC_NORMAL;
    interpreter->source_line = -1;
}

void interpreter_free(Interpreter *interpreter)
{
    gc_collect_all(interpreter);
    gc_ctx_free(&interpreter->gc);
    scope_destroy(&interpreter->globals);
    hashmap_free(&interpreter->type_register);
    arraylist_free(&interpreter->stream_ctx.active_fds);
}

static void interpreter_reset_from_err(Interpreter *interpreter)
{
    /* Pop everything from shadow stack as they are no longer in use */
    interpreter->gc.shadow_stack.size = 0;
    interpreter->gc.barrier = 0;

    /* Free any old scopes */
    while (interpreter->scope != &interpreter->globals) {
	Scope *to_destroy = interpreter->scope;
	interpreter->scope = interpreter->scope->enclosing;
	scope_destroy(to_destroy);
    }

    /* Reset stream_ctx */
    arraylist_free(&interpreter->stream_ctx.active_fds);
    StreamCtx stream_ctx = { .in_fd = STDIN_FILENO,
			     .out_fd = STDOUT_FILENO,
			     .err_fd = STDERR_FILENO };
    arraylist_init(&stream_ctx.active_fds, sizeof(int));
    interpreter->stream_ctx = stream_ctx;

    interpreter->exec_res_ctx = EXEC_NORMAL;
    interpreter->source_line = -1;
}

int interpreter_run(Interpreter *interpreter, ArrayList *statements)
{
    if (setjmp(runtime_error_jmp) != RUNTIME_ERROR) {
	for (size_t i = 0; i < statements->size; i++)
	    exec(interpreter, *(Stmt **)arraylist_get(statements, i));
    } else {
	/* execution enters here on a runtime error, therefore we must "reset" the interpreter */
	interpreter_reset_from_err(interpreter);
	set_exit_code(interpreter, 1);
    }

    return interpreter->prev_exit_code;
}

int interpret(ArrayList *statements, int argc, char **argv)
{
    Interpreter interpreter = { 0 };
    interpreter_init(&interpreter, argc, argv);
    interpreter_run(&interpreter, statements);
    interpreter_free(&interpreter);
    return interpreter.prev_exit_code;
}
