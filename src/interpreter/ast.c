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

#include "interpreter/ast.h"
#include "interpreter/lexer.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"
#include "sac/sac.h"


const size_t expr_size_table[] = {
    sizeof(UnaryExpr),	   sizeof(BinaryExpr),	 sizeof(LiteralExpr), sizeof(AccessExpr),
    sizeof(SubscriptExpr), sizeof(SubshellExpr), sizeof(StrExpr),     sizeof(ListExpr),
    sizeof(FunctionExpr),  sizeof(MapExpr),	 sizeof(MethodExpr),  sizeof(SequenceExpr),
    sizeof(GroupingExpr),  sizeof(CastExpr),	 sizeof(CallExpr),
};

const size_t stmt_size_table[] = {
    sizeof(ExpressionStmt),
    sizeof(VarStmt),
    sizeof(SeqVarStmt),
    sizeof(LoopStmt),
    sizeof(IterLoopStmt),
    sizeof(IfStmt),
    sizeof(CmdStmt),
    sizeof(AssignStmt),
    sizeof(BlockStmt),
    sizeof(PipelineStmt),
    sizeof(AssignStmt),
    sizeof(BinaryStmt),
    sizeof(AbruptControlFlowStmt),
};

char *expr_type_str_map[EXPR_ENUM_COUNT] = {
    "EXPR_UNARY",    "EXPR_BINARY",   "EXPR_LITERAL",  "EXPR_ACCESS",	"EXPR_ITEM_ACCESS",
    "EXPR_SUBSHELL", "EXPR_STR",      "EXPR_LIST",     "EXPR_FUNCTION", "EXPR_MAP",
    "EXPR_METHOD",   "EXPR_SEQUENCE", "EXPR_GROUPING", "EXPR_CAST",	"EXPR_CALL",
};

char *stmt_type_str_map[STMT_ENUM_COUNT] = {
    "STMT_EXPRESSION",
    "STMT_VAR",
    "STMT_SEQ_VAR",
    "STMT_LOOP",
    "STMT_ITER_LOOP",
    "STMT_IF",
    "STMT_CMD",
    "STMT_ASSIGN",
    "STMT_BLOCK",
    "STMT_PIPELINE",
    "STMT_ASSERT",
    "STMT_BINARY",
    "STMT_ABRUPT_CONTROL_FLOW",
};


Expr *expr_alloc(Arena *ast_arena, ExprType type, int file_no)
{
    size_t size = expr_size_table[type];
    Expr *expr = m_arena_alloc(ast_arena, size);
    expr->type = type;
    expr->source_line = file_no;
    return expr;
}

Stmt *stmt_alloc(Arena *ast_arena, StmtType type)
{
    size_t size = stmt_size_table[type];
    Stmt *stmt = m_arena_alloc(ast_arena, size);
    stmt->type = type;
    return stmt;
}

Expr *expr_copy(Arena *arena, Expr *to_copy)
{
    if (to_copy == NULL)
	return NULL;

    Expr *copy = expr_alloc(arena, to_copy->type, to_copy->source_line);
    switch (to_copy->type) {
    case EXPR_UNARY: {
	((UnaryExpr *)copy)->operator_ = ((UnaryExpr *)to_copy)->operator_;
	((UnaryExpr *)copy)->right = expr_copy(arena, ((UnaryExpr *)to_copy)->right);
	break;
    }
    case EXPR_BINARY: {
	((BinaryExpr *)copy)->left = expr_copy(arena, ((BinaryExpr *)to_copy)->left);
	((BinaryExpr *)copy)->operator_ = ((BinaryExpr *)to_copy)->operator_;
	((BinaryExpr *)copy)->right = expr_copy(arena, ((BinaryExpr *)to_copy)->right);
	break;
    }
    case EXPR_LITERAL: {
	((LiteralExpr *)copy)->value = ((LiteralExpr *)to_copy)->value;
	break;
    }
    case EXPR_ACCESS: {
	((AccessExpr *)copy)->var_name =
	    str_view_arena_copy(arena, ((AccessExpr *)to_copy)->var_name);
	break;
    }
    case EXPR_SUBSCRIPT: {
	((SubscriptExpr *)copy)->expr = expr_copy(arena, ((SubscriptExpr *)to_copy)->expr);
	((SubscriptExpr *)copy)->access_value =
	    expr_copy(arena, ((SubscriptExpr *)to_copy)->access_value);
	break;
    }
    case EXPR_SUBSHELL: {
	((SubshellExpr *)copy)->stmt = stmt_copy(arena, ((SubshellExpr *)to_copy)->stmt);
	break;
    }
    case EXPR_STR: {
	((StrExpr *)copy)->view = str_view_arena_copy(arena, ((StrExpr *)to_copy)->view);
	break;
    }
    case EXPR_LIST: {
	((ListExpr *)copy)->exprs =
	    (SequenceExpr *)expr_copy(arena, (Expr *)((ListExpr *)to_copy)->exprs);
	break;
    }
    case EXPR_FUNCTION: {
	FunctionExpr *tc = (FunctionExpr *)to_copy;
	/* list of parameter names as pointers to StrView's */
	arena_ll_init(arena, &((FunctionExpr *)copy)->params);
	LLItem *item;
	ARENA_LL_FOR_EACH(&tc->params, item)
	{
	    StrView *view = item->value;
	    StrView *view_cpy = m_arena_alloc(arena, sizeof(StrView));
	    *view_cpy = str_view_arena_copy(arena, *view);
	    arena_ll_append(&((FunctionExpr *)copy)->params, view_cpy);
	}

	((FunctionExpr *)copy)->body = (BlockStmt *)stmt_copy(arena, (Stmt *)tc->body);
	break;
    }
    case EXPR_MAP: {
	MapExpr *tc = (MapExpr *)to_copy;
	/* list of KeyValuePairs */
	((MapExpr *)copy)->key_value_pairs = arena_ll_alloc(arena);
	LLItem *item;
	ARENA_LL_FOR_EACH(tc->key_value_pairs, item)
	{
	    KeyValuePair *pair = item->value;
	    KeyValuePair *pair_cpy = m_arena_alloc_struct(arena, KeyValuePair);
	    pair_cpy->key = expr_copy(arena, pair->key);
	    pair_cpy->value = expr_copy(arena, pair->value);
	    arena_ll_append(((MapExpr *)copy)->key_value_pairs, pair_cpy);
	}
	break;
    }
    case EXPR_SEQUENCE: {
	SequenceExpr *tc = (SequenceExpr *)to_copy;
	/* list of Exprs */
	arena_ll_init(arena, &((SequenceExpr *)copy)->seq);
	LLItem *item;
	ARENA_LL_FOR_EACH(&tc->seq, item)
	{
	    arena_ll_append(&((SequenceExpr *)copy)->seq, expr_copy(arena, item->value));
	}
	break;
    }
    case EXPR_GROUPING: {
	((GroupingExpr *)copy)->expr = expr_copy(arena, ((GroupingExpr *)to_copy)->expr);
	break;
    }
    case EXPR_CAST: {
	((CastExpr *)copy)->expr = expr_copy(arena, ((CastExpr *)to_copy)->expr);
	((CastExpr *)copy)->type_name =
	    str_view_arena_copy(arena, ((CastExpr *)to_copy)->type_name);
	break;
    }
    case EXPR_CALL: {
	((CallExpr *)copy)->callee = expr_copy(arena, ((CallExpr *)to_copy)->callee);
	((CallExpr *)copy)->args =
	    (SequenceExpr *)expr_copy(arena, (Expr *)((CallExpr *)to_copy)->args);
	break;
    }
    case EXPR_METHOD:
    case EXPR_ENUM_COUNT:
	break;
    }

    return copy;
}

Stmt *stmt_copy(Arena *arena, Stmt *to_copy)
{
    if (to_copy == NULL)
	return NULL;

    Stmt *copy = stmt_alloc(arena, to_copy->type);
    switch (to_copy->type) {
    case STMT_VAR: {
	((VarStmt *)copy)->name = str_view_arena_copy(arena, ((VarStmt *)to_copy)->name);
	((VarStmt *)copy)->initializer = expr_copy(arena, ((VarStmt *)to_copy)->initializer);
	break;
    }
    case STMT_SEQ_VAR: {
	SeqVarStmt *tc = (SeqVarStmt *)to_copy;
	/* list of parameter names as pointers to StrView's */
	arena_ll_init(arena, &((SeqVarStmt *)copy)->names);
	LLItem *item;
	ARENA_LL_FOR_EACH(&tc->names, item)
	{
	    StrView *view = item->value;
	    StrView *view_cpy = m_arena_alloc(arena, sizeof(StrView));
	    *view_cpy = str_view_arena_copy(arena, *view);
	    arena_ll_append(&((SeqVarStmt *)copy)->names, view_cpy);
	}

	((SeqVarStmt *)copy)->initializer = expr_copy(arena, tc->initializer);
	break;
    }
    case STMT_EXPRESSION: {
	((ExpressionStmt *)copy)->expression =
	    expr_copy(arena, ((ExpressionStmt *)to_copy)->expression);
	break;
    }
    case STMT_CMD: {
	((CmdStmt *)copy)->cmd_name = str_view_arena_copy(arena, ((CmdStmt *)to_copy)->cmd_name);
	((CmdStmt *)copy)->arg_exprs = arena_ll_alloc(arena);
	LLItem *item;
	ARENA_LL_FOR_EACH(((CmdStmt *)to_copy)->arg_exprs, item)
	{
	    arena_ll_append(((CmdStmt *)copy)->arg_exprs, expr_copy(arena, item->value));
	}
	break;
    }
    case STMT_LOOP: {
	((LoopStmt *)copy)->condition = expr_copy(arena, ((LoopStmt *)to_copy)->condition);
	((LoopStmt *)copy)->body_block =
	    (BlockStmt *)stmt_copy(arena, (Stmt *)((LoopStmt *)to_copy)->body_block);
	break;
    }
    case STMT_ITER_LOOP: {
	((IterLoopStmt *)copy)->var_name =
	    str_view_arena_copy(arena, ((IterLoopStmt *)to_copy)->var_name);
	((IterLoopStmt *)copy)->underlying_iterable =
	    expr_copy(arena, ((IterLoopStmt *)to_copy)->underlying_iterable);
	((IterLoopStmt *)copy)->body_block =
	    (BlockStmt *)stmt_copy(arena, (Stmt *)((IterLoopStmt *)to_copy)->body_block);
	break;
    }
    case STMT_IF: {
	((IfStmt *)copy)->condition = expr_copy(arena, ((IfStmt *)to_copy)->condition);
	((IfStmt *)copy)->then_branch = stmt_copy(arena, ((IfStmt *)to_copy)->then_branch);
	((IfStmt *)copy)->else_branch = stmt_copy(arena, ((IfStmt *)to_copy)->else_branch);
	break;
    }
    case STMT_BLOCK: {
	BlockStmt *tc = (BlockStmt *)to_copy;
	/* list of pointers to stmts */
	((BlockStmt *)copy)->statements = arena_ll_alloc(arena);
	LLItem *item;
	ARENA_LL_FOR_EACH(tc->statements, item)
	{
	    arena_ll_append(((BlockStmt *)copy)->statements, stmt_copy(arena, item->value));
	}
	break;
    }
    case STMT_ASSIGN: {
	((AssignStmt *)copy)->var = expr_copy(arena, ((AssignStmt *)to_copy)->var);
	((AssignStmt *)copy)->value = expr_copy(arena, ((AssignStmt *)to_copy)->value);
	((AssignStmt *)copy)->assignment_op = ((AssignStmt *)to_copy)->assignment_op;
	break;
    }
    case STMT_PIPELINE: {
	((PipelineStmt *)copy)->left =
	    (CmdStmt *)stmt_copy(arena, (Stmt *)((PipelineStmt *)to_copy)->left);
	((PipelineStmt *)copy)->right = stmt_copy(arena, ((PipelineStmt *)to_copy)->right);
	break;
    }
    case STMT_ASSERT: {
	((AssertStmt *)copy)->expr = expr_copy(arena, ((AssertStmt *)to_copy)->expr);
	break;
    }
    case STMT_BINARY: {
	((BinaryStmt *)copy)->left = stmt_copy(arena, ((BinaryStmt *)to_copy)->left);
	((BinaryStmt *)copy)->operator_ = ((BinaryStmt *)to_copy)->operator_;
	if (((BinaryStmt *)copy)->operator_ == t_greater)
	    ((BinaryStmt *)copy)->right_expr =
		expr_copy(arena, ((BinaryStmt *)to_copy)->right_expr);
	else
	    ((BinaryStmt *)copy)->right_stmt =
		stmt_copy(arena, ((BinaryStmt *)to_copy)->right_stmt);
	break;
    }
    case STMT_ABRUPT_CONTROL_FLOW: {
	((AbruptControlFlowStmt *)copy)->ctrlf_type =
	    ((AbruptControlFlowStmt *)to_copy)->ctrlf_type;
	if (((AbruptControlFlowStmt *)to_copy)->return_expr != NULL) {
	    ((AbruptControlFlowStmt *)copy)->return_expr =
		expr_copy(arena, ((AbruptControlFlowStmt *)to_copy)->return_expr);
	} else {
	    ((AbruptControlFlowStmt *)copy)->return_expr = NULL;
	}
	break;
    }
    case STMT_ENUM_COUNT:
	break;
    }
    return copy;
}


/* arena */
void ast_arena_init(Arena *ast_arena)
{
    m_arena_init_dynamic(ast_arena, 2, 512);
}

void ast_arena_release(Arena *ast_arena)
{
    m_arena_release(ast_arena);
}


/* ast print */
static void ast_print_expr(Expr *expr);
static void ast_print_stmt(Stmt *stmt);

static void ast_print_sequence(SequenceExpr *expr)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(&expr->seq, item)
    {
	ast_print_expr(item->value);
    }
}

static void ast_print_unary(UnaryExpr *expr)
{
    printf("%s ", token_type_str_map[expr->operator_]);
    ast_print_expr(expr->right);
}

static void ast_print_binary(BinaryExpr *expr)
{
    ast_print_expr(expr->left);
    printf(" %s ", token_type_str_map[expr->operator_]);
    ast_print_expr(expr->right);
}

static void ast_print_literal(LiteralExpr *expr)
{
    (void)expr;
    printf("literal");
    // switch (expr->value.type) {
    // case SLASH_SHIDENT:
    //     str_view_print(expr->value.shident);
    //     break;

    // case SLASH_NUM:
    //     printf("%f", expr->value.num);
    //     break;

    // case SLASH_BOOL:
    //     printf("%s", expr->value.boolean == true ? "true" : "false");
    //     break;

    // case SLASH_RANGE:
    //     ast_print_range_literal(&expr->value.range);
    //     break;

    // default:
    //     printf("bad literal type");
    // }
}

static void ast_print_access(AccessExpr *expr)
{
    str_view_print(expr->var_name);
}

static void ast_print_item_access(SubscriptExpr *expr)
{
    ast_print_expr(expr->expr);
    putchar('[');
    ast_print_expr(expr->access_value);
    putchar(']');
}

static void ast_print_list(ListExpr *expr)
{
    if (expr->exprs == NULL)
	return;

    ast_print_sequence(expr->exprs);
}

static void ast_print_map(MapExpr *expr)
{
    if (expr->key_value_pairs == NULL)
	return;

    LLItem *item;
    KeyValuePair *pair;
    ARENA_LL_FOR_EACH(expr->key_value_pairs, item)
    {
	pair = item->value;
	ast_print_expr(pair->key);
	printf(":");
	ast_print_expr(pair->value);
    }
}

static void ast_print_method(MethodExpr *expr)
{
    ast_print_expr(expr->obj);
    putchar('.');
    str_view_print(expr->method_name);
    putchar('(');

    if (expr->args != NULL) {
	LLItem *item;
	ARENA_LL_FOR_EACH(&expr->args->seq, item)
	{
	    ast_print_expr(item->value);
	}
    }
    putchar(')');
}

static void ast_print_grouping(GroupingExpr *expr)
{
    putchar('(');
    ast_print_expr(expr->expr);
    putchar(')');
}

static void ast_print_expression(ExpressionStmt *stmt)
{
    ast_print_expr(stmt->expression);
}

static void ast_print_var(VarStmt *stmt)
{
    str_view_print(stmt->name);
    printf(" = ");
    ast_print_expr(stmt->initializer);
}

static void ast_print_seq_var(SeqVarStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(&stmt->names, item)
    {
	str_view_print(*(StrView *)item->value);
	printf(", ");
    }
    printf(" = ");
    ast_print_expr(stmt->initializer);
}

static void ast_print_cmd(CmdStmt *stmt)
{
    str_view_print(stmt->cmd_name);
    if (stmt->arg_exprs == NULL)
	return;

    printf(", ");
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->arg_exprs, item)
    {
	ast_print_expr(item->value);
    }
}

static void ast_print_if(IfStmt *stmt)
{
    ast_print_expr(stmt->condition);

    printf(" then_branch ");

    ast_print_stmt(stmt->then_branch);
    if (stmt->else_branch != NULL) {
	printf(" else_branch ");
	ast_print_stmt(stmt->else_branch);
    }
}

static void ast_print_block(BlockStmt *stmt)
{
    LLItem *item;
    ARENA_LL_FOR_EACH(stmt->statements, item)
    {
	ast_print_stmt(item->value);
    }
}

static void ast_print_loop(LoopStmt *stmt)
{
    ast_print_expr(stmt->condition);
    ast_print_block(stmt->body_block);
}

static void ast_print_iter_loop(IterLoopStmt *stmt)
{
    str_view_print(stmt->var_name);
    printf(" = iter.");
    ast_print_expr(stmt->underlying_iterable);
    ast_print_block(stmt->body_block);
}

static void ast_print_assign(AssignStmt *stmt)
{
    ast_print_expr(stmt->var);
    if (stmt->assignment_op == t_equal)
	printf(" = ");
    else if (stmt->assignment_op == t_plus_equal)
	printf(" += ");
    else
	printf(" -= ");
    ast_print_expr(stmt->value);
}

static void ast_print_pipeline(PipelineStmt *stmt)
{
    ast_print_stmt((Stmt *)stmt->left);
    printf(" | ");
    ast_print_stmt((Stmt *)stmt->right);
}

static void ast_print_assert(AssertStmt *stmt)
{
    printf("ASSERT");
    ast_print_expr((Expr *)stmt->expr);
}

static void ast_print_binary_stmt(BinaryStmt *stmt)
{
    ast_print_stmt(stmt->left);
    printf(" %s ", token_type_str_map[stmt->operator_]);
    if (stmt->operator_ == t_anp_anp || stmt->operator_ == t_pipe_pipe)
	ast_print_stmt(stmt->right_stmt);
    else
	ast_print_expr(stmt->right_expr);
}

static void ast_print_expr(Expr *expr)
{
    printf("%s", expr_type_str_map[expr->type]);
    putchar('[');

    switch (expr->type) {
    case EXPR_UNARY:
	ast_print_unary((UnaryExpr *)expr);
	break;

    case EXPR_BINARY:
	ast_print_binary((BinaryExpr *)expr);
	break;

    case EXPR_LITERAL:
	ast_print_literal((LiteralExpr *)expr);
	break;

    case EXPR_ACCESS:
	ast_print_access((AccessExpr *)expr);
	break;

    case EXPR_SUBSCRIPT:
	ast_print_item_access((SubscriptExpr *)expr);
	break;

    case EXPR_SUBSHELL:
	ast_print_stmt(((SubshellExpr *)expr)->stmt);
	break;

    case EXPR_LIST:
	ast_print_list((ListExpr *)expr);
	break;

    case EXPR_MAP:
	ast_print_map((MapExpr *)expr);
	break;

    case EXPR_METHOD:
	ast_print_method((MethodExpr *)expr);
	break;

    case EXPR_SEQUENCE:
	ast_print_sequence((SequenceExpr *)expr);
	break;

    case EXPR_GROUPING:
	ast_print_grouping((GroupingExpr *)expr);
	break;

    default:
	printf("ast-expr type not handled");
    }

    putchar(']');
}

static void ast_print_stmt(Stmt *stmt)
{
    printf("%s", stmt_type_str_map[stmt->type]);
    putchar('{');

    switch (stmt->type) {
    case STMT_EXPRESSION:
	ast_print_expression((ExpressionStmt *)stmt);
	break;

    case STMT_VAR:
	ast_print_var((VarStmt *)stmt);
	break;

    case STMT_SEQ_VAR:
	ast_print_seq_var((SeqVarStmt *)stmt);
	break;

    case STMT_CMD:
	ast_print_cmd((CmdStmt *)stmt);
	break;

    case STMT_BLOCK:
	ast_print_block((BlockStmt *)stmt);
	break;

    case STMT_LOOP:
	ast_print_loop((LoopStmt *)stmt);
	break;

    case STMT_ITER_LOOP:
	ast_print_iter_loop((IterLoopStmt *)stmt);
	break;

    case STMT_IF:
	ast_print_if((IfStmt *)stmt);
	break;

    case STMT_ASSIGN:
	ast_print_assign((AssignStmt *)stmt);
	break;

    case STMT_PIPELINE:
	ast_print_pipeline((PipelineStmt *)stmt);
	break;

    case STMT_ASSERT:
	ast_print_assert((AssertStmt *)stmt);
	break;

    case STMT_BINARY:
	ast_print_binary_stmt((BinaryStmt *)stmt);
	break;

    default:
	printf("ast-stmt type not handled");
    }

    putchar('}');
}

void ast_print(ArrayList *ast_heads)
{
    printf("--- AST ---\n");

    size_t size = ast_heads->size;
    for (size_t i = 0; i < size; i++) {
	ast_print_stmt(*(Stmt **)arraylist_get(ast_heads, i));
	if (i != size - 1) {
	    printf("\n\n");
	} else {
	    printf("\n");
	}
    }
}
