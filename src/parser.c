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

// TODO: match multiple
// static bool match(Parser *parser, TokenType type)
//{
//     if (check(parser, type)) {
//         advance(parser);
//         return true;
//     }
//
//     return false;
// }


/* grammar functions */
static Stmt *declaration(Parser *parser);
static Stmt *var_decl(Parser *parser);
static Expr *expression(Parser *parser);
static Expr *primary(Parser *parser);

static Stmt *declaration(Parser *parser)
{
    return var_decl(parser);
}

static Stmt *var_decl(Parser *parser)
{
    // TODO: should not consume var token as var token will be advanced past when we enter here
    consume(parser, t_var, "lol");
    Token *name = consume(parser, t_identifier, "Expected variable name");
    consume(parser, t_equal, "Expected variable definition");
    Expr *initializer = expression(parser);
    consume(parser, t_newline, "Expected line ending after variable definition");

    VarStmt *stmt = (VarStmt *)stmt_alloc(parser->ast_arena, STMT_VAR);
    stmt->name = name;
    stmt->initializer = initializer;
    return (Stmt *)stmt;
}

static Expr *expression(Parser *parser)
{
    return primary(parser);
}

static Expr *primary(Parser *parser)
{
    Token *token = advance(parser);

    // TODO: fix this lol
    TokenType value_type = t_error;
    bool is_valid = false;
    TokenType valid[] = { t_true, t_false, dt_num, dt_str };
    for (int i = 0; i < 4; i++) {
	if (token->type == valid[i]) {
	    is_valid = true;
	    value_type = i > 1 ? valid[i] : dt_bool;
	    break;
	}
    }
    if (!is_valid)
	slash_exit_parse_err("not a valid primary type");

    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value_type = value_type;
    void *value;

    if (value_type == dt_bool) {
	value = m_arena_alloc(parser->ast_arena, sizeof(bool));
	*(bool *)value = token->type == t_true ? true : false;
    } else if (value_type == dt_num) {
	// TODO: here we need to figure out the base and a bunch of things
	value = m_arena_alloc(parser->ast_arena, sizeof(double));
	*(double *)value = 3.14;
    } else {
	value = &token->lexeme;
    }

    expr->value = value;
    return (Expr *)expr;
}

struct darr_t *parse(Arena *ast_arena, struct darr_t *tokens)
{
    Parser parser = { .ast_arena = ast_arena, .tokens = tokens, .token_pos = 0 };
    struct darr_t *statements = darr_malloc();
    while (!check(&parser, t_eof))
	darr_append(statements, declaration(&parser));
    return statements;
}
