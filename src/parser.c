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

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "nicc/nicc.h"
#include "parser.h"
#include "sac/sac.h"
#include "slash_value.h"

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
static Stmt *var_decl(Parser *parser);
static Stmt *shell_cmd(Parser *parser);
static Expr *argument(Parser *parser);
static Expr *expression(Parser *parser);
static Expr *primary(Parser *parser);
static Expr *bool_lit(Parser *parser);
static Expr *number(Parser *parser);
static Expr *interpolation(Parser *parser);

static Stmt *declaration(Parser *parser)
{
    if (match(parser, t_var))
	return var_decl(parser);

    return shell_cmd(parser);
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

static Stmt *shell_cmd(Parser *parser)
{
    // TODO: should be previous since we will only enter here if we came from dt_shlit
    Token *cmd_name = consume(parser, dt_shlit, "Expected shell literal");

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
    return primary(parser);
}

static Expr *primary(Parser *parser)
{
    if (match(parser, t_true, t_false))
	return bool_lit(parser);

    if (match(parser, t_num))
	return number(parser);

    if (match(parser, t_interpolation))
	return interpolation(parser);

    /* str or shlit */
    if (!match(parser, dt_str, dt_shlit))
	slash_exit_parse_err("not a valid primary type");

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
    // TODO: different parsing based on number base
    value.p = m_arena_alloc(parser->ast_arena, sizeof(double));
    *(double *)value.p = 3.14;

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
	/* ignore all trailing newlines after statement */
	ignore(&parser, t_newline);
    }
    return statements;
}
