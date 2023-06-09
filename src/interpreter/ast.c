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
#include "interpreter/ast.h"
#include "interpreter/lexer.h"
#include "interpreter/types/slash_value.h"
#include "sac/sac.h"
#include "str_view.h"


const size_t expr_size_table[] = {
    sizeof(UnaryExpr),	sizeof(BinaryExpr),   sizeof(LiteralExpr),
    sizeof(AccessExpr), sizeof(SubshellExpr), sizeof(ListExpr),
};

const size_t stmt_size_table[] = { sizeof(ExpressionStmt), sizeof(VarStmt),  sizeof(LoopStmt),
				   sizeof(IterLoopStmt),   sizeof(IfStmt),   sizeof(CmdStmt),
				   sizeof(AssignStmt),	   sizeof(BlockStmt) };

char *expr_type_str_map[EXPR_ENUM_COUNT] = {
    "EXPR_UNARY", "EXPR_BINARY", "EXPR_LITERAL", "EXPR_ACCESS", "EXPR_SUBSHELL", "EXPR_LIST",
};

char *stmt_type_str_map[STMT_ENUM_COUNT] = {
    "STMT_EXPRESSION", "STMT_VAR", "STMT_LOOP",	  "STMT_ITER_LOOP",
    "STMT_IF",	       "STMT_CMD", "STMT_ASSIGN", "STMT_BLOCK",
};

Expr *expr_alloc(Arena *ast_arena, ExprType type)
{
    size_t size = expr_size_table[type];
    Expr *expr = m_arena_alloc(ast_arena, size);
    expr->type = type;
    return expr;
}

Stmt *stmt_alloc(Arena *ast_arena, StmtType type)
{
    size_t size = stmt_size_table[type];
    Stmt *stmt = m_arena_alloc(ast_arena, size);
    stmt->type = type;
    return stmt;
}


/* arena */
void ast_arena_init(Arena *ast_arena)
{
    m_arena_init_dynamic(ast_arena, 2, 32);
}

void ast_arena_release(Arena *ast_arena)
{
    m_arena_release(ast_arena);
}


/* ast print */
static void ast_print_expr(Expr *expr);
static void ast_print_stmt(Stmt *stmt);


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

static void ast_print_range_literal(SlashRange *range)
{
    printf("range(%d -> %d)", range->start, range->end);
}

static void ast_print_literal(LiteralExpr *expr)
{
    switch (expr->value.type) {
    case SLASH_STR:
    case SLASH_SHLIT:
	str_view_print(expr->value.str);
	break;

    case SLASH_NUM:
	printf("%f", expr->value.num);
	break;

    case SLASH_BOOL:
	printf("%s", expr->value.boolean == true ? "true" : "false");
	break;

    case SLASH_RANGE:
	ast_print_range_literal(&expr->value.range);
	break;

    default:
	printf("bad literal type");
    }
}

static void ast_print_access(AccessExpr *expr)
{
    str_view_print(expr->var_name);
    if (expr->access_type == ACCESS_INDEX) {
	printf(".get(%d)", expr->index);
    } else if (expr->access_type == ACCESS_KEY) {
	printf(".get(");
	str_view_print(expr->key);
	printf(")");
    } else if (expr->access_type == ACCESS_RANGE) {
	printf(".get(%d..%d)", expr->range.start, expr->range.end);
    }
}

static void ast_print_list(ListExpr *expr)
{
    if (expr->exprs == NULL)
	return;

    LLItem *item;
    ARENA_LL_FOR_EACH(expr->exprs, item)
    {
	ast_print_expr(item->value);
	printf(", ");
    }
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
    ast_print_expr(stmt->access);
    if (stmt->assignment_op == t_equal)
	printf(" = ");
    else if (stmt->assignment_op == t_plus_equal)
	printf(" += ");
    else
	printf(" -= ");
    ast_print_expr(stmt->value);
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

    case EXPR_SUBSHELL:
	ast_print_stmt(((SubshellExpr *)expr)->stmt);
	break;

    case EXPR_LIST:
	ast_print_list((ListExpr *)expr);
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
