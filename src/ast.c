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

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "sac/sac.h"
#include "slash_str.h"


// TODO: table lookup faster
Expr *expr_alloc(Arena *ast_arena, ExprType type)
{
    size_t size = 0;
    switch (type) {
    case EXPR_UNARY:
	size = sizeof(UnaryExpr);
	break;
    case EXPR_BINARY:
	size = sizeof(BinaryExpr);
	break;
    case EXPR_LITERAL:
	size = sizeof(LiteralExpr);
	break;
    default:
	slash_exit_internal_err("expr type not known");
	break;
    }

    Expr *expr = m_arena_alloc(ast_arena, size);
    expr->type = type;
    return expr;
}

Stmt *stmt_alloc(Arena *ast_arena, StmtType type)
{
    size_t size = 0;
    switch (type) {
    case STMT_EXPRESSION:
	size = sizeof(ExpressionStmt);
	break;
    case STMT_VAR:
	size = sizeof(VarStmt);
	break;
    default:
	slash_exit_internal_err("stmt type not known");
	break;
    }

    Stmt *stmt = m_arena_alloc(ast_arena, size);
    stmt->type = type;
    return stmt;
}


void ast_arena_init(Arena *ast_arena)
{
    m_arena_init_dynamic(ast_arena, SAC_DEFAULT_CAPACITY, SAC_DEFAULT_COMMIT_SIZE);
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
    printf("%s ", token_type_str_map[expr->operator_->type]);
    ast_print_expr(expr->right);
}

static void ast_print_binary(BinaryExpr *expr)
{
    ast_print_expr(expr->left);
    printf(" %s ", token_type_str_map[expr->operator_->type]);
    ast_print_expr(expr->right);
}

static void ast_print_literal(LiteralExpr *expr)
{
    switch (expr->value_type) {
    case dt_str:
	slash_str_print(*(SlashStr *)expr->value);
	break;

    case dt_num:
	printf("%f", *(double *)expr->value);
	break;

    case dt_bool:
	printf("%s", *(bool *)expr->value == true ? "true" : "false");
	break;

    default:
	printf("bad literal type");
    }
}


static void ast_print_expression(ExpressionStmt *stmt)
{
    ast_print_expr(stmt->expression);
}

static void ast_print_var(VarStmt *stmt)
{
    slash_str_print(stmt->name->lexeme);
    printf(" = ");
    ast_print_expr(stmt->initializer);
}

static void ast_print_expr(Expr *expr)
{
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

    default:
	printf("ast type not handled");
    }

    putchar(']');
}

static void ast_print_stmt(Stmt *stmt)
{
    putchar('{');

    switch (stmt->type) {
    case STMT_EXPRESSION:
	ast_print_expression((ExpressionStmt *)stmt);
	break;

    case STMT_VAR:
	ast_print_var((VarStmt *)stmt);
	break;

    default:
	printf("ast type not handled");
    }

    putchar('}');
}

void ast_print(struct darr_t *ast_heads)
{
    printf("--- AST ---\n");

    size_t size = ast_heads->size;
    for (size_t i = 0; i < size; i++) {
	ast_print_stmt(darr_get(ast_heads, i));
	if (i != size)
	    putchar('\n');
    }
}
