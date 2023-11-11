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
#include <setjmp.h>
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
/// #include "interpreter/value/cast.h"
/// #include "interpreter/value/method.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"
#include "nicc/nicc.h"


static SlashValue eval(Interpreter *interpreter, Expr *expr);
static void exec(Interpreter *interpreter, Stmt *stmt);

/*
 * helpers
 */
static void set_exit_code(Interpreter *interpreter, int exit_code)
{
    interpreter->prev_exit_code = exit_code;
    SlashValue value = { .T = &num_type_info, .num = interpreter->prev_exit_code };
    var_assign(&(StrView){ .view = "?", .size = 1 }, &interpreter->globals, &value);
}

static void exec_block_body(Interpreter *interpreter, BlockStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->statements, item)
    {
	exec(interpreter, item->value);
    }
}

static void exec_program_stub(Interpreter *interpreter, CmdStmt *stmt, char *program_path)
{
    size_t argc = 1;
    if (stmt->arg_exprs != NULL)
	argc += stmt->arg_exprs->size;
    char *argv[argc + 1]; // + 1 because last element is NULL
    argv[0] = program_path;

    size_t i = 1;
    if (stmt->arg_exprs != NULL) {
	LLItem *item;
	ARENA_LL_FOR_EACH(stmt->arg_exprs, item)
	{
	    SlashValue value = eval(interpreter, item->value);
	    VERIFY_TRAIT_IMPL(to_str, value, "Could not take 'to_str' of type '%s'", value.T->name);
	    SlashValue value_str_repr = value.T->to_str(interpreter, value);
	    gc_shadow_push(&interpreter->gc, value_str_repr.obj);
	    argv[i++] = AS_STR(value_str_repr)->str;
	}
    }

    for (size_t n = 0; n < argc - 1; n++)
	gc_shadow_pop(&interpreter->gc);

    argv[i] = NULL;
    int exit_code = exec_program(&interpreter->stream_ctx, argv);
    set_exit_code(interpreter, exit_code);
}

static SlashValue eval_binary_operators(Interpreter *interpreter, SlashValue left, SlashValue right,
					TokenType op)
{
    /* Binary operators */
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
	VERIFY_TRAIT_IMPL(plus, left, "'+' operator not defined for type '%s'", right.T->name);
	return left.T->plus(interpreter, left, right);
    case t_minus:
	VERIFY_TRAIT_IMPL(minus, left, "'-' operator not defined for type '%s'", right.T->name);
	return left.T->minus(left, right);
    case t_slash:
	VERIFY_TRAIT_IMPL(div, left, "'/' operator not defined for type '%s'", right.T->name);
	return left.T->div(left, right);
    case t_slash_slash:
	VERIFY_TRAIT_IMPL(int_div, left, "'//' operator not defined for type '%s'", right.T->name);
	return left.T->int_div(left, right);
    case t_percent:
	VERIFY_TRAIT_IMPL(mod, left, "'%%' operator not defined for type '%s'", right.T->name);
	return left.T->mod(left, right);
    case t_star:
	VERIFY_TRAIT_IMPL(mul, left, "'*' operator not defined for type '%s'", right.T->name);
	return left.T->mul(interpreter, left, right);
    case t_star_star:
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
    SlashValue left = eval(interpreter, expr->left);
    if (IS_OBJ(left))
	gc_shadow_push(&interpreter->gc, left.obj);
    SlashValue right;
    SlashValue return_value;

    /* logical operators */
    if (expr->operator_ == t_and) {
	if (!left.T->truthy(left)) {
	    return_value = (SlashValue){ .T = &bool_type_info, .boolean = false };
	    goto defer_shadow_pop;
	}
	right = eval(interpreter, expr->right);
	return_value = (SlashValue){ .T = &bool_type_info, .boolean = right.T->truthy(right) };
	goto defer_shadow_pop;
    }
    right = eval(interpreter, expr->right);
    if (expr->operator_ == t_or) {
	bool truthy = left.T->truthy(left) || right.T->truthy(right);
	return_value = (SlashValue){ .T = &bool_type_info, .boolean = truthy };
	goto defer_shadow_pop;
    }

    /* binary operators */
    if (expr->operator_ != t_in) {
	return_value = eval_binary_operators(interpreter, left, right, expr->operator_);
	goto defer_shadow_pop;
    }

    /* left "IN" right */
    VERIFY_TRAIT_IMPL(item_in, right, "'in' operator not defined for type '%s'", right.T->name);
    bool rc = right.T->item_in(right, left);
    return_value = (SlashValue){ .T = &bool_type_info, .boolean = rc };


defer_shadow_pop:
    if (IS_OBJ(left))
	gc_shadow_pop(&interpreter->gc);
    return return_value;
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
    /* pipe output will be written to */
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = &interpreter->stream_ctx;
    int original_write_fd = stream_ctx->write_fd;
    /* set the write fd to the newly created pipe */
    stream_ctx->write_fd = fd[STREAM_WRITE_END];

    exec(interpreter, expr->stmt);
    close(fd[1]);
    /* restore original write fd */
    stream_ctx->write_fd = original_write_fd;

    // TODO: dynamic buffer
    char buffer[4096] = { 0 };
    size_t size = read(fd[0], buffer, 4096);
    if (buffer[size - 1] == '\n')
	size--;
    close(fd[0]);

    SlashStr *str = (SlashStr *)gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_slice(interpreter, str, buffer, size);
    return AS_VALUE(str);
}

static SlashValue eval_tuple(Interpreter *interpreter, SequenceExpr *expr)
{
    SlashTuple *tuple = (SlashTuple *)gc_new_T(interpreter, &tuple_type_info);
    gc_shadow_push(&interpreter->gc, &tuple->obj);
    slash_tuple_init(interpreter, tuple, expr->seq.size);
    if (expr->seq.size == 0) {
	gc_shadow_pop(&interpreter->gc);
	return AS_VALUE(tuple);
    }

    size_t i = 0;
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	tuple->items[i++] = element_value;
    }

    gc_shadow_pop(&interpreter->gc);
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
    SlashList *list = (SlashList *)gc_new_T(interpreter, &list_type_info);
    gc_shadow_push(&interpreter->gc, &list->obj);
    slash_list_impl_init(interpreter, list);
    if (expr->exprs == NULL) {
	gc_shadow_pop(&interpreter->gc);
	return AS_VALUE(list);
    }

    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->exprs->seq, item)
    {
	SlashValue element_value = eval(interpreter, item->value);
	slash_list_impl_append(interpreter, list, element_value);
    }

    gc_shadow_pop(&interpreter->gc);
    return AS_VALUE(list);
}

static SlashValue eval_map(Interpreter *interpreter, MapExpr *expr)
{
    SlashMap *map = (SlashMap *)gc_new_T(interpreter, &map_type_info);
    gc_shadow_push(&interpreter->gc, &map->obj);
    slash_map_impl_init(interpreter, map);
    if (expr->key_value_pairs == NULL) {
	gc_shadow_pop(&interpreter->gc);
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

    gc_shadow_pop(&interpreter->gc);
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
///
/// static SlashValue eval_cast(Interpreter *interpreter, CastExpr *expr)
///{
///     SlashValue value = eval(interpreter, expr->expr);
///     /*
///      * When LHS is a subshell and RHS is boolean then the final exit code of the subshell
///      * expression determines the boolean value.
///      */
///     if (expr->as == SLASH_BOOL && expr->expr->type == EXPR_SUBSHELL)
///	return (SlashValue){ .type = SLASH_BOOL,
///			     .boolean = interpreter->prev_exit_code == 0 ? true : false };
///     return dynamic_cast(interpreter, value, expr->as);
/// }
///
///
////*
///  * statement execution functions
///  */
static void exec_expr(Interpreter *interpreter, ExpressionStmt *stmt)
{
    SlashValue value = eval(interpreter, stmt->expression);
    TraitPrint trait_print = value.T->print;
    assert(trait_print != NULL);
    trait_print(value);
    ///* edge case: if last char printed was a newline then we don't bother printing one */
    // if (value.type == SLASH_OBJ && value.obj->type == SLASH_OBJ_STR) {
    //     SlashStr *str = (SlashStr *)value.obj;
    //     if (slash_str_last_char(str) == '\n')
    //         return;
    // }
    putchar('\n');
}

static void exec_var(Interpreter *interpreter, VarStmt *stmt)
{
    /* Make sure variable is not defined already */
    ScopeAndValue current = var_get(interpreter->scope, &stmt->name);
    if (current.scope == interpreter->scope)
	REPORT_RUNTIME_ERROR("Redefinition of '%s'", "X");

    SlashValue value = eval(interpreter, stmt->initializer);
    var_define(interpreter->scope, &stmt->name, &value);
}

static void exec_seq_var(Interpreter *interpreter, SeqVarStmt *stmt)
{
    SequenceExpr *initializer = (SequenceExpr *)stmt->initializer;
    if (initializer->type != EXPR_SEQUENCE)
	REPORT_RUNTIME_ERROR("Multiple variable declaration only supported for tuples");

    if (stmt->names.size != initializer->seq.size)
	REPORT_RUNTIME_ERROR("Unpacking only supported for collections of the same size");

    LLItem *l = stmt->names.head;
    LLItem *r = initializer->seq.head;
    for (size_t i = 0; i < stmt->names.size; i++) {
	StrView *name = l->value;
	ScopeAndValue current = var_get(interpreter->scope, name);
	if (current.scope == interpreter->scope)
	    REPORT_RUNTIME_ERROR("Redefinition of '%s'", "X");
	SlashValue value = eval(interpreter, (Expr *)r->value);
	var_define(interpreter->scope, name, &value);
	l = l->next;
	r = r->next;
    }
}

static void exec_cmd(Interpreter *interpreter, CmdStmt *stmt)
{
    ScopeAndValue path = var_get(interpreter->scope, &(StrView){ .view = "PATH", .size = 4 });
    if (!IS_STR(*path.value))
	REPORT_RUNTIME_ERROR("PATH variable should be type '%s' not '%s'", str_type_info.name,
			     path.value->T->name);

    WhichResult which_result = which(stmt->cmd_name, AS_STR(*path.value)->str);
    if (which_result.type == WHICH_NOT_FOUND)
	REPORT_RUNTIME_ERROR("Command not found");

    if (which_result.type == WHICH_EXTERN) {
	exec_program_stub(interpreter, stmt, which_result.path);
    } else {
	/* builtin */
	if (stmt->arg_exprs == NULL) {
	    which_result.builtin(interpreter, 0, NULL);
	    return;
	}

	size_t argc = stmt->arg_exprs->size;
	SlashValue argv[argc];
	LLItem *item = stmt->arg_exprs->head;
	for (size_t i = 0; i < argc; i++) {
	    SlashValue value = eval(interpreter, (Expr *)item->value);
	    argv[i] = value;
	    item = item->next;
	}
	which_result.builtin(interpreter, argc, argv);
    }
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

    exec_block_body(interpreter, stmt);

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

    ScopeAndValue current = var_get(interpreter->scope, &var_name);
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
    /* convert from += to + and -= to - */
    TokenType operator= stmt->assignment_op == t_plus_equal ? t_plus : t_minus;
    /* += means we eval + and then assign */
    if (operator== t_plus_equal) {
	VERIFY_TRAIT_IMPL(plus, current_item_value, "'+' operator not defined for type '%s'",
			  current_item_value.T->name);
	new_value = current_item_value.T->plus(interpreter, current_item_value, new_value);
    } else {
	REPORT_RUNTIME_ERROR("TODO: FIX SUBSCRIPT ASSSIGN");
    }

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

    LLItem *l = left->seq.head;
    LLItem *r = right->seq.head;
    for (size_t i = 0; i < left->seq.size; i++) {
	AccessExpr *access = (AccessExpr *)l->value;
	if (access->type != EXPR_ACCESS)
	    REPORT_RUNTIME_ERROR("Can not assign to literal value");
	SlashValue value = eval(interpreter, (Expr *)r->value);
	var_assign(&access->var_name, interpreter->scope, &value);
	l = l->next;
	r = r->next;
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
	REPORT_RUNTIME_ERROR("Internal error: bad expr type");
	ASSERT_NOT_REACHED;
    }

    AccessExpr *access = (AccessExpr *)stmt->var;
    StrView var_name = access->var_name;
    ScopeAndValue variable = var_get(interpreter->scope, &var_name);
    if (variable.value == NULL)
	REPORT_RUNTIME_ERROR("Cannot modify undefined variable '%s'", "X");

    SlashValue new_value = eval(interpreter, stmt->value);

    if (stmt->assignment_op == t_equal) {
	var_assign(&var_name, variable.scope, &new_value);
	return;
    }

    REPORT_RUNTIME_ERROR("TODO: FIX ASSIGN");
    ///    /* convert from assignment operator to correct binary operator */
    ///    TokenType operator_;
    ///    switch (stmt->assignment_op) {
    ///    case t_plus_equal:
    ///	operator_ = t_plus;
    ///	break;
    ///    case t_minus_equal:
    ///	operator_ = t_minus;
    ///	break;
    ///    case t_star_equal:
    ///	operator_ = t_star;
    ///	break;
    ///    case t_star_star_equal:
    ///	operator_ = t_star_star;
    ///	break;
    ///    case t_slash_equal:
    ///	operator_ = t_slash;
    ///	break;
    ///    case t_slash_slash_equal:
    ///	operator_ = t_slash_slash;
    ///	break;
    ///    case t_percent_equal:
    ///	operator_ = t_percent;
    ///	break;
    ///    default:
    ///	REPORT_RUNTIME_ERROR("Unrecognized assignment operator");
    ///	ASSERT_NOT_REACHED;
    ///    }
    ///    // new_value = cmp_binary_values(interpreter, *variable.value, new_value, operator_);
    ///    /*
    ///     * For dynamic types the binary compare will update the underlying object.
    ///     * Therefore, we only assign if the variable type is not dynamic.
    ///     */
    ///    if (!IS_OBJ(variable.value->type))
    ///	var_assign(&var_name, variable.scope, &new_value);
}

static void exec_pipeline(Interpreter *interpreter, PipelineStmt *stmt)
{
    int fd[2];
    pipe(fd);

    StreamCtx *stream_ctx = &interpreter->stream_ctx;
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
	exec_block_body(interpreter, stmt->body_block);
	r = eval(interpreter, stmt->condition);
	scope_reset(block_scope);
    }

    scope_destroy(block_scope);
}

/// static void exec_iter_loop_list(Interpreter *interpreter, IterLoopStmt *stmt, SlashList
/// *iterable)
///{
///    /* define the loop variable that holds the current iterator value */
///    var_define(interpreter->scope, &stmt->var_name, NULL);
///
///    SlashValue *iterator_value;
///    for (size_t i = 0; i < iterable->underlying.size; i++) {
///	iterator_value = slash_list_get(iterable, i);
///	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
///	exec_block_body(interpreter, stmt->body_block);
///	scope_reset(interpreter->scope);
///    }
///}
///
/// static void exec_iter_loop_tuple(Interpreter *interpreter, IterLoopStmt *stmt, SlashTuple
/// *iterable)
///{
///    /* define the loop variable that holds the current iterator value */
///    var_define(interpreter->scope, &stmt->var_name, NULL);
///
///    SlashValue *iterator_value;
///    for (size_t i = 0; i < iterable->size; i++) {
///	iterator_value = &iterable->values[i];
///	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
///	exec_block_body(interpreter, stmt->body_block);
///	scope_reset(interpreter->scope);
///    }
///}
///
/// static void exec_iter_loop_map(Interpreter *interpreter, IterLoopStmt *stmt, SlashMap *iterable)
///{
///    if (iterable->underlying.len == 0)
///	return;
///
///    SlashValue keys = slash_map_get_keys(interpreter, iterable);
///    SlashTuple *keys_tuple = (SlashTuple *)keys.obj;
///    /* define the loop variable that holds the current iterator value */
///    var_define(interpreter->scope, &stmt->var_name, NULL);
///
///    SlashValue *iterator_value;
///    for (size_t i = 0; i < keys_tuple->size; i++) {
///	iterator_value = &keys_tuple->values[i];
///	var_assign(&stmt->var_name, interpreter->scope, iterator_value);
///	exec_block_body(interpreter, stmt->body_block);
///	scope_reset(interpreter->scope);
///    }
///}
///
/// static void exec_iter_loop_str(Interpreter *interpreter, IterLoopStmt *stmt, SlashStr *iterable)
///{
///    ScopeAndValue ifs_res = var_get(interpreter->scope, &(StrView){ .view = "IFS", .size = 3 });
///    if (ifs_res.value == NULL) {
///	REPORT_RUNTIME_ERROR("No IFS variable found");
///    }
///    if (!(ifs_res.value->type == SLASH_OBJ && ifs_res.value->obj->type == SLASH_OBJ_STR)) {
///	REPORT_RUNTIME_ERROR("Expeted IFS to be of type 'str' but got '%s'",
///			     SLASH_TYPE_TO_STR(ifs_res.value));
///    }
///
///    SlashStr *ifs = (SlashStr *)ifs_res.value->obj;
///    SlashList *substrings = slash_str_internal_split(interpreter, iterable, ifs->p, true);
///    gc_shadow_push(&interpreter->gc_shadow_stack, &substrings->obj);
///    exec_iter_loop_list(interpreter, stmt, substrings);
///    gc_shadow_pop(&interpreter->gc_shadow_stack);
///}
///
/// static void exec_iter_loop_range(Interpreter *interpreter, IterLoopStmt *stmt, SlashRange
/// iterable)
///{
///    if (!slash_range_is_nonzero(iterable))
///	return;
///    SlashValue iterator_value = { .type = SLASH_NUM, .num = iterable.start };
///
///    /* define the loop variable that holds the current iterator value */
///    var_define(interpreter->scope, &stmt->var_name, &iterator_value);
///
///    while (iterator_value.num != iterable.end) {
///	exec_block_body(interpreter, stmt->body_block);
///	scope_reset(interpreter->scope);
///	iterator_value.num++;
///	var_assign(&stmt->var_name, interpreter->scope, &iterator_value);
///    }
///}
///
/// static void exec_iter_loop(Interpreter *interpreter, IterLoopStmt *stmt)
///{
///    Scope loop_scope;
///    scope_init(&loop_scope, interpreter->scope);
///    interpreter->scope = &loop_scope;
///
///    SlashValue underlying = eval(interpreter, stmt->underlying_iterable);
///    if (IS_OBJ(underlying.type))
///	gc_shadow_push(&interpreter->gc_shadow_stack, underlying.obj);
///
///    switch (underlying.type) {
///    case SLASH_RANGE:
///	exec_iter_loop_range(interpreter, stmt, underlying.range);
///	break;
///    case SLASH_OBJ: {
///	switch (underlying.obj->type) {
///	case SLASH_OBJ_LIST:
///	    exec_iter_loop_list(interpreter, stmt, (SlashList *)underlying.obj);
///	    break;
///	case SLASH_OBJ_TUPLE:
///	    exec_iter_loop_tuple(interpreter, stmt, (SlashTuple *)underlying.obj);
///	    break;
///	case SLASH_OBJ_MAP:
///	    exec_iter_loop_map(interpreter, stmt, (SlashMap *)underlying.obj);
///	    break;
///	case SLASH_OBJ_STR:
///	    exec_iter_loop_str(interpreter, stmt, (SlashStr *)underlying.obj);
///	    break;
///	default:
///	    REPORT_RUNTIME_ERROR("Object type '%s' cannot be iterated over",
///				 SLASH_TYPE_TO_STR(&underlying));
///	    ASSERT_NOT_REACHED;
///	}
///
///    } break;
///    default:
///	REPORT_RUNTIME_ERROR("Type '%s' cannot be iterated over", SLASH_TYPE_TO_STR(&underlying));
///	ASSERT_NOT_REACHED;
///    }
///
///    if (IS_OBJ(underlying.type))
///	gc_shadow_pop(&interpreter->gc_shadow_stack);
///    interpreter->scope = loop_scope.enclosing;
///    scope_destroy(&loop_scope);
///}
///
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
	REPORT_RUNTIME_ERROR("Could not open file '%s'", file_name);
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
    case EXPR_STR:
	return eval_str(interpreter, (StrExpr *)expr);
    case EXPR_LIST:
	return eval_list(interpreter, (ListExpr *)expr);
    case EXPR_MAP:
	return eval_map(interpreter, (MapExpr *)expr);
	/// case EXPR_METHOD:
	///	return eval_method(interpreter, (MethodExpr *)expr);
    case EXPR_SEQUENCE:
	return eval_tuple(interpreter, (SequenceExpr *)expr);
    case EXPR_GROUPING:
	return eval_grouping(interpreter, (GroupingExpr *)expr);
	///    case EXPR_CAST:
	///	return eval_cast(interpreter, (CastExpr *)expr);
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
	///    case STMT_ITER_LOOP:
	///	exec_iter_loop(interpreter, (IterLoopStmt *)stmt);
	///	break;
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
	REPORT_RUNTIME_ERROR("Internal error: Statement type not recognized");
    }
}

void interpreter_init(Interpreter *interpreter, int argc, char **argv)
{
    m_arena_init_dynamic(&interpreter->arena, 1, 16384);

    scope_init_globals(&interpreter->globals, &interpreter->arena, argc, argv);
    interpreter->scope = &interpreter->globals;

    gc_ctx_init(&interpreter->gc);

    /* init default StreamCtx */
    StreamCtx stream_ctx = { .read_fd = STDIN_FILENO, .write_fd = STDOUT_FILENO };
    arraylist_init(&stream_ctx.active_fds, sizeof(int));
    interpreter->stream_ctx = stream_ctx;
}

void interpreter_free(Interpreter *interpreter)
{
    gc_collect_all(interpreter);
    gc_ctx_free(&interpreter->gc);
    scope_destroy(&interpreter->globals);
    arraylist_free(&interpreter->stream_ctx.active_fds);
}

static void interpreter_reset_from_err(Interpreter *interpreter)
{
    /* free any old scopes */
    while (interpreter->scope != &interpreter->globals) {
	Scope *to_destroy = interpreter->scope;
	interpreter->scope = interpreter->scope->enclosing;
	scope_destroy(to_destroy);
    }

    /* reset stream_ctx */
    arraylist_free(&interpreter->stream_ctx.active_fds);
    StreamCtx stream_ctx = { .read_fd = STDIN_FILENO, .write_fd = STDOUT_FILENO };
    arraylist_init(&stream_ctx.active_fds, sizeof(int));
    interpreter->stream_ctx = stream_ctx;
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
