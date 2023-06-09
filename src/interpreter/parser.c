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
#include <stdarg.h>

#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "nicc/nicc.h"
#include "sac/sac.h"

/* util/helper functions */
static Token *peek(Parser *parser)
{
    return darr_get(parser->tokens, parser->token_pos);
}

static Token *previous(Parser *parser)
{
    return darr_get(parser->tokens, parser->token_pos - 1);
}

static bool is_at_end(Parser *parser)
{
    return peek(parser)->type == t_eof;
}

static Token *advance(Parser *parser)
{
    Token *token = peek(parser);
    if (!is_at_end(parser))
	parser->token_pos++;

    return token;
}

static bool check_single(Parser *parser, TokenType type, int step)
{
    /* darr_get returns NULL if index out of bounds */
    Token *token = darr_get(parser->tokens, parser->token_pos + step);
    if (token == NULL)
	return false;
    return token->type == type;
}

static bool check_either(Parser *parser, int step, unsigned int n, ...)
{
    TokenType type;
    va_list args;
    va_start(args, n);

    for (unsigned int i = 0; i < n; i++) {
	type = va_arg(args, TokenType);
	if (check_single(parser, type, step))
	    return true;
    }

    va_end(args);
    return false;
}

#define VA_NUMBER_OF_ARGS(...) (sizeof((int[]){ __VA_ARGS__ }) / sizeof(int))
#define check(parser, ...) check_either(parser, 0, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)
#define check_ahead(parser, n, ...) \
    check_either(parser, n, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)

static Token *consume(Parser *parser, TokenType expected, char *err_msg)
{
    if (!check_single(parser, expected, 0))
	slash_exit_parse_err(err_msg);

    return advance(parser);
}

// TODO: match multiple?
static void ignore(Parser *parser, TokenType type)
{
    while (check(parser, type))
	advance(parser);
}

static bool match_single(Parser *parser, TokenType type)
{
    if (check(parser, type)) {
	advance(parser);
	return true;
    }

    return false;
}

static bool match_either(Parser *parser, unsigned int n, ...)
{
    TokenType type;
    va_list args;
    va_start(args, n);

    for (unsigned int i = 0; i < n; i++) {
	type = va_arg(args, TokenType);
	if (match_single(parser, type))
	    return true;
    }

    va_end(args);
    return false;
}

#define match(parser, ...) match_either(parser, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)


/* grammar functions */
static Stmt *declaration(Parser *parser);
static Stmt *statement(Parser *parser);
static Stmt *var_decl(Parser *parser);
static Stmt *loop_stmt(Parser *parser);
static Stmt *if_stmt(Parser *parser);
static Stmt *cmd_stmt(Parser *parser);
static Stmt *assignment_stmt(Parser *parser);
static Stmt *expr_stmt(Parser *parser);
static Stmt *block(Parser *parser);

static Expr *argument(Parser *parser);
static Expr *expression(Parser *parser);
static Expr *equality(Parser *parser);
static Expr *comparison(Parser *parser);
static Expr *term(Parser *parser);
static Expr *factor(Parser *parser);
static Expr *unary(Parser *parser);
static Expr *primary(Parser *parser);
static Expr *bool_lit(Parser *parser);
static Expr *number(Parser *parser);
static Expr *interpolation(Parser *parser);

static Stmt *declaration(Parser *parser)
{
    Stmt *stmt;
    /* ignore all leading newlines */
    ignore(parser, t_newline);

    if (match(parser, t_var))
	stmt = var_decl(parser);
    else
	stmt = statement(parser);

    /* ignore all trailing newlines */
    ignore(parser, t_newline);

    return stmt;
}

static Stmt *statement(Parser *parser)
{
    if (match(parser, t_loop))
	return loop_stmt(parser);

    if (match(parser, t_if))
	return if_stmt(parser);

    if (match(parser, t_interpolation))
	return assignment_stmt(parser);

    if (match(parser, t_lbrace))
	return block(parser);

    if (match(parser, dt_shlit))
	return cmd_stmt(parser);

    return expr_stmt(parser);
}

static Stmt *expr_stmt(Parser *parser)
{
    Expr *expr = expression(parser);
    consume(parser, t_newline, "Expected newline after expression statement");

    ExpressionStmt *stmt = (ExpressionStmt *)stmt_alloc(parser->ast_arena, STMT_EXPRESSION);
    stmt->expression = expr;
    return (Stmt *)stmt;
}

static Stmt *var_decl(Parser *parser)
{
    Token *name = consume(parser, t_identifier, "Expected variable name");
    consume(parser, t_equal, "Expected variable definition");
    Expr *initializer = expression(parser);
    consume(parser, t_newline, "Expected line ending after variable definition");

    VarStmt *stmt = (VarStmt *)stmt_alloc(parser->ast_arena, STMT_VAR);
    stmt->name = name;
    stmt->initializer = initializer;
    return (Stmt *)stmt;
}

static Stmt *loop_stmt(Parser *parser)
{
    LoopStmt *stmt = (LoopStmt *)stmt_alloc(parser->ast_arena, STMT_LOOP);
    stmt->condition = expression(parser);
    if (match(parser, t_lbrace)) {
	stmt->body = block(parser);
    } else {
	// TODO: continue with 'in ITERABLE' syntax
	consume(parser, t_identifier, "Expected either an identifier or '{' after loop keyword");
    }
    return (Stmt *)stmt;
}

static Stmt *if_stmt(Parser *parser)
{
    /* came from 'if' or 'elif' */
    IfStmt *stmt = (IfStmt *)stmt_alloc(parser->ast_arena, STMT_IF);
    stmt->condition = expression(parser);
    consume(parser, t_lbrace, "Expression after if-statement must be succeded by '{'");
    stmt->then_branch = block(parser);

    // TODO: elif support
    stmt->else_branch = NULL;
    if (match(parser, t_elif)) {
	stmt->else_branch = if_stmt(parser);
    } else if (match(parser, t_else)) {
	consume(parser, t_lbrace, "expected '{' after else-statement");
	stmt->else_branch = block(parser);
    }

    return (Stmt *)stmt;
}

static Stmt *block(Parser *parser)
{
    /* came from '{' */
    BlockStmt *stmt = (BlockStmt *)stmt_alloc(parser->ast_arena, STMT_BLOCK);
    stmt->statements = darr_malloc();

    while (!check(parser, t_rbrace) && !is_at_end(parser))
	darr_append(stmt->statements, declaration(parser));

    consume(parser, t_rbrace, "Expected '}' after block");
    return (Stmt *)stmt;
}

static Stmt *assignment_stmt(Parser *parser)
{
    /* came from t_interpolation */
    Token *name = previous(parser);
    // TODO: consume_any
    if (!match(parser, t_equal, t_plus_equal, t_minus_equal)) {
	slash_exit_parse_err("Expected '=', '+=' or '-='");
    }

    Token *assignment_op = previous(parser);
    Expr *value = expression(parser);

    AssignStmt *stmt = (AssignStmt *)stmt_alloc(parser->ast_arena, STMT_ASSIGN);
    stmt->name = name;
    stmt->assignment_op = assignment_op;
    stmt->value = value;
    return (Stmt *)stmt;
}

static Stmt *cmd_stmt(Parser *parser)
{
    Token *cmd_name = previous(parser);

    CmdStmt *stmt = (CmdStmt *)stmt_alloc(parser->ast_arena, STMT_CMD);
    stmt->cmd_name = cmd_name;
    stmt->args_ll = (ArgExpr *)argument(parser);
    return (Stmt *)stmt;
}

static Expr *argument(Parser *parser)
{
    if (check(parser, t_newline, t_eof))
	return NULL;

    ArgExpr *args_ll = (ArgExpr *)expr_alloc(parser->ast_arena, EXPR_ARG);
    args_ll->next = NULL;

    // TODO: bruh kanke være nødvendig
    ArgExpr **arg = &args_ll;
    while (!check(parser, t_newline, t_eof)) {
	if (*arg == NULL) {
	    *arg = (ArgExpr *)expr_alloc(parser->ast_arena, EXPR_ARG);
	    (*arg)->next = NULL;
	}

	(*arg)->this = expression(parser);
	arg = &(*arg)->next;
    }

    return (Expr *)args_ll;
}

static Expr *expression(Parser *parser)
{
    return equality(parser);
}

static Expr *equality(Parser *parser)
{
    Expr *expr = comparison(parser);

    while (match(parser, t_equal_equal, t_bang_equal)) {
	Token *operator_ = previous(parser);
	Expr *right = factor(parser);

	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = operator_;
	bin_expr->right = right;
	expr = (Expr *)bin_expr;
    }

    return expr;
}

static Expr *comparison(Parser *parser)
{
    Expr *expr = term(parser);

    while (match(parser, t_greater, t_greater_equal, t_less, t_less_equal)) {
	Token *operator_ = previous(parser);
	Expr *right = factor(parser);

	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = operator_;
	bin_expr->right = right;
	expr = (Expr *)bin_expr;
    }

    return expr;
}

static Expr *term(Parser *parser)
{
    Expr *expr = factor(parser);

    while (match(parser, t_minus, t_plus)) {
	Token *operator_ = previous(parser);
	Expr *right = factor(parser);

	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = operator_;
	bin_expr->right = right;
	expr = (Expr *)bin_expr;
    }
    return expr;
}

static Expr *factor(Parser *parser)
{
    return unary(parser);
}

static Expr *unary(Parser *parser)
{
    return primary(parser);
}

static Expr *primary(Parser *parser)
{
    if (match(parser, t_true, t_false))
	return bool_lit(parser);

    if (match(parser, t_interpolation))
	return interpolation(parser);

    if (match(parser, dt_num))
	return number(parser);

    if (!match(parser, dt_str, dt_shlit))
	slash_exit_parse_err("not a valid primary type");

    /* str or shlit */
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value =
	(SlashValue){ .type = token->type == dt_str ? SVT_STR : SVT_SHLIT, .p = &token->lexeme };
    return (Expr *)expr;
}

static Expr *bool_lit(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);

    SlashValue value;
    value.type = SVT_BOOL;
    value.p = m_arena_alloc(parser->ast_arena, sizeof(bool));
    *(bool *)value.p = token->type == t_true ? true : false;

    expr->value = value;
    return (Expr *)expr;
}

static Expr *number(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);

    SlashValue value;
    value.type = SVT_NUM;
    value.p = m_arena_alloc(parser->ast_arena, sizeof(double));
    *(double *)value.p = slash_str_to_double(token->lexeme);

    expr->value = value;
    return (Expr *)expr;
}

static Expr *interpolation(Parser *parser)
{
    Token *token = previous(parser);
    InterpolationExpr *expr =
	(InterpolationExpr *)expr_alloc(parser->ast_arena, EXPR_INTERPOLATION);
    expr->var_name = token;
    return (Expr *)expr;
}

struct darr_t *parse(Arena *ast_arena, struct darr_t *tokens)
{
    Parser parser = { .ast_arena = ast_arena, .tokens = tokens, .token_pos = 0 };
    struct darr_t *statements = darr_malloc();
    while (!check(&parser, t_eof)) {
	darr_append(statements, declaration(&parser));
    }
    return statements;
}
