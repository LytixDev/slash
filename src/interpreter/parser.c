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

#include "arena_ll.h"
#include "common.h"
#include "interpreter/ast.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include "sac/sac.h"
#include "str_view.h"


/* grammar functions */
static Stmt *declaration(Parser *parser);
static Stmt *statement(Parser *parser);
static Stmt *var_decl(Parser *parser);
static Stmt *loop_stmt(Parser *parser);
static Stmt *assert_stmt(Parser *parser);
static Stmt *if_stmt(Parser *parser);
static Stmt *pipeline_stmt(Parser *parser);
static Stmt *cmd_stmt(Parser *parser);
static Stmt *assignment_stmt(Parser *parser);
static Stmt *expr_stmt(Parser *parser);
static Stmt *block(Parser *parser);

static Expr *argument(Parser *parser);
static Expr *expression(Parser *parser);
static Expr *subshell(Parser *parser);
static Expr *equality(Parser *parser);
static Expr *comparison(Parser *parser);
static Expr *term(Parser *parser);
static Expr *factor(Parser *parser);
static Expr *unary(Parser *parser);
static Expr *access(Parser *parser);
static Expr *item_access(Parser *parser);
static Expr *primary(Parser *parser);
static Expr *bool_lit(Parser *parser);
static Expr *number(Parser *parser);
static Expr *range(Parser *parser);
static Expr *map(Parser *parser);
static Expr *list_or_tuple_or_map(Parser *parser, bool is_list);


/* util/helper functions */
static Token *peek(Parser *parser)
{
    return arraylist_get(parser->tokens, parser->token_pos);
}

static Token *previous(Parser *parser)
{
    return arraylist_get(parser->tokens, parser->token_pos - 1);
}

/* lol */
static Token *previous_previous(Parser *parser)
{
    return arraylist_get(parser->tokens, parser->token_pos - 2);
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
    /* arraylist_get returns NULL if index out of bounds */
    Token *token = arraylist_get(parser->tokens, parser->token_pos + step);
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
#define check_arg_end(parser) \
    check(parser, t_newline, t_eof, t_pipe, t_pipe_pipe, t_anp, t_anp_anp, t_rparen)

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

ArenaLL *comma_sep_exprs(Parser *parser)
{
    /*
     * this function is only called when we expect comma seperated expression
     */

    ArenaLL *args = arena_ll_alloc(parser->ast_arena);
    do {
	ignore(parser, t_newline);
	arena_ll_append(args, expression(parser));
	/* if no comma then we exit */
	if (!match(parser, t_comma))
	    break;
    } while (!check(parser, t_eof));

    ignore(parser, t_newline);
    return args;
}


/* grammar functions */

static Stmt *declaration(Parser *parser)
{
    Stmt *stmt;
    /* ignore all leading newlines */
    ignore(parser, t_newline);

    stmt = statement(parser);

    /* ignore all trailing newlines */
    ignore(parser, t_newline);

    return stmt;
}

static Stmt *statement(Parser *parser)
{
    if (match(parser, t_var))
	return var_decl(parser);

    if (match(parser, t_loop))
	return loop_stmt(parser);

    if (match(parser, t_assert))
	return assert_stmt(parser);

    if (match(parser, t_if))
	return if_stmt(parser);

    if (match(parser, t_lbrace))
	return block(parser);

    if (match(parser, dt_shlit))
	return pipeline_stmt(parser);

    /*
     * TODO:
     * this can't be a match due to the BNF.
     * we have to call item_access() later that does not assume we came from
     * t_access. This is bad and should be fixed.
     */
    if (check(parser, t_access))
	return assignment_stmt(parser);

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

static Stmt *assignment_stmt(Parser *parser)
{
    /* know next is t_access */

    // TODO: this is a hack
    size_t pos = parser->token_pos;

    // TODO: throw parse error if not AccessExpr or ItemAccessExpr ?
    Expr *var = item_access(parser);
    if (!match(parser, t_equal, t_plus_equal, t_minus_equal)) {
	/* if next token after access is not an assignment op, then ignore everything we just did */
	// TODO: can we just return the `var` as an ExpressionStmt ?
	parser->token_pos = pos;
	return expr_stmt(parser);
    }

    /* access part of assignment */
    Token *assignment_op = previous(parser);
    Expr *value = expression(parser);

    AssignStmt *stmt = (AssignStmt *)stmt_alloc(parser->ast_arena, STMT_ASSIGN);
    stmt->var = var;
    stmt->assignment_op = assignment_op->type;
    stmt->value = value;
    return (Stmt *)stmt;
}

static Stmt *var_decl(Parser *parser)
{
    Token *name = consume(parser, t_identifier, "Expected variable name");
    consume(parser, t_equal, "Expected variable definition");
    Expr *initializer = expression(parser);
    consume(parser, t_newline, "Expected line ending after variable definition");

    VarStmt *stmt = (VarStmt *)stmt_alloc(parser->ast_arena, STMT_VAR);
    stmt->name = name->lexeme;
    stmt->initializer = initializer;
    return (Stmt *)stmt;
}

static Stmt *loop_stmt(Parser *parser)
{
    /* came from 'loop */
    if (match(parser, t_identifier)) {
	/* loop IDENTIFIER in iterable { ... } */
	Token *var_name = previous(parser);
	consume(parser, t_in, "Expected 'in' keyword in loop");
	/* Throw runtime error if expression can not be iterated over */
	Expr *iterable = expression(parser);

	IterLoopStmt *iter_loop = (IterLoopStmt *)stmt_alloc(parser->ast_arena, STMT_ITER_LOOP);
	iter_loop->var_name = var_name->lexeme;
	iter_loop->underlying_iterable = iterable;
	consume(parser, t_lbrace, "expected '{' after loop condition");
	iter_loop->body_block = (BlockStmt *)block(parser);

	return (Stmt *)iter_loop;
    }

    LoopStmt *stmt = (LoopStmt *)stmt_alloc(parser->ast_arena, STMT_LOOP);
    stmt->condition = expression(parser);
    consume(parser, t_lbrace, "expected '{' after loop condition");
    stmt->body_block = (BlockStmt *)block(parser);
    return (Stmt *)stmt;
}

static Stmt *assert_stmt(Parser *parser)
{
    /* came from 'assert' */
    AssertStmt *stmt = (AssertStmt *)stmt_alloc(parser->ast_arena, STMT_ASSERT);
    stmt->expr = expression(parser);
    return (Stmt *)stmt;
}

static Stmt *if_stmt(Parser *parser)
{
    /* came from 'if' or 'elif' */
    IfStmt *stmt = (IfStmt *)stmt_alloc(parser->ast_arena, STMT_IF);
    stmt->condition = expression(parser);
    consume(parser, t_lbrace, "Expression after if-statement must be succeded by '{'");
    stmt->then_branch = block(parser);

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
    stmt->statements = arena_ll_alloc(parser->ast_arena);
    while (!check(parser, t_rbrace) && !is_at_end(parser))
	arena_ll_append(stmt->statements, declaration(parser));

    consume(parser, t_rbrace, "Expected '}' after block");
    return (Stmt *)stmt;
}

static Stmt *pipeline_stmt(Parser *parser)
{
    /* came from dt_shlit */
    Stmt *left = cmd_stmt(parser);
    if (!match(parser, t_pipe))
	return left;

    consume(parser, dt_shlit, "expected shell literal after pipe symbol");
    Stmt *right = pipeline_stmt(parser);

    PipelineStmt *stmt = (PipelineStmt *)stmt_alloc(parser->ast_arena, STMT_PIPELINE);
    stmt->left = (CmdStmt *)left;
    stmt->right = right;
    return (Stmt *)stmt;
}

static Stmt *cmd_stmt(Parser *parser)
{
    /* came from dt_shlit */
    Token *cmd_name = previous(parser);

    CmdStmt *stmt = (CmdStmt *)stmt_alloc(parser->ast_arena, STMT_CMD);
    stmt->cmd_name = cmd_name->lexeme;
    if (check_arg_end(parser)) {
	stmt->arg_exprs = NULL;
	return (Stmt *)stmt;
    }

    stmt->arg_exprs = arena_ll_alloc(parser->ast_arena);
    while (!check_arg_end(parser)) {
	arena_ll_append(stmt->arg_exprs, argument(parser));
    }
    return (Stmt *)stmt;
}

static Expr *argument(Parser *parser)
{
    return expression(parser);
}

static Expr *expression(Parser *parser)
{
    return equality(parser);
}

static Expr *subshell(Parser *parser)
{
    /* came from '(' */
    consume(parser, dt_shlit, "Expected shell literal after '('");
    SubshellExpr *expr = (SubshellExpr *)expr_alloc(parser->ast_arena, EXPR_SUBSHELL);
    expr->stmt = pipeline_stmt(parser);
    consume(parser, t_rparen, "Expected ')' after subshell");
    return (Expr *)expr;
}

static Expr *equality(Parser *parser)
{
    Expr *expr = comparison(parser);

    while (match(parser, t_equal_equal, t_bang_equal)) {
	Token *operator_ = previous(parser);
	Expr *right = factor(parser);

	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = operator_->type;
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
	bin_expr->operator_ = operator_->type;
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
	bin_expr->operator_ = operator_->type;
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
    // TODO: need to figure out if '(' should be parsed as a tuple, subshell or grouping (TODO)
    Expr *left;
    if (!match(parser, t_lparen))
	/* continue the "normal" recursive path */
	left = item_access(parser);
    else if (match(parser, t_lparen))
	left = list_or_tuple_or_map(parser, true);
    else
	left = subshell(parser);

    if (match(parser, t_in)) {
	BinaryExpr *expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	expr->left = left;
	expr->operator_ = t_in;
	expr->right = expression(parser);
	return (Expr *)expr;
    }

    if (match(parser, t_dot)) {
	MethodExpr *expr = (MethodExpr *)expr_alloc(parser->ast_arena, EXPR_METHOD);
	Token *method_name = consume(parser, t_identifier, "expected method name");
	expr->obj = left;
	expr->method_name = method_name->lexeme;
	consume(parser, t_lparen, "expected left paren");
	if (!match(parser, t_rparen)) {
	    expr->arg_exprs = comma_sep_exprs(parser);
	    consume(parser, t_rparen, "expected right paren");
	} else {
	    expr->arg_exprs = NULL;
	}
	return (Expr *)expr;
    }

    /* no suitable match */
    return left;
}

static Expr *item_access(Parser *parser)
{
    if (!match(parser, t_access))
	return primary(parser);

    if (!match(parser, t_lbracket))
	return access(parser);

    Token *variable_name = previous_previous(parser);
    Expr *access_value = expression(parser);
    consume(parser, t_rbracket, "expected ']' after variable item access");
    ItemAccessExpr *expr = (ItemAccessExpr *)expr_alloc(parser->ast_arena, EXPR_ITEM_ACCESS);
    expr->var_name = variable_name->lexeme;
    expr->access_value = access_value;
    return (Expr *)expr;
}

static Expr *access(Parser *parser)
{
    /* came from t_access */
    Token *variable_name = previous(parser);
    AccessExpr *expr = (AccessExpr *)expr_alloc(parser->ast_arena, EXPR_ACCESS);
    expr->var_name = variable_name->lexeme;
    return (Expr *)expr;
}

static Expr *primary(Parser *parser)
{
    if (match(parser, t_true, t_false))
	return bool_lit(parser);

    if (check(parser, t_dot_dot))
	return range(parser);

    if (match(parser, dt_num)) {
	if (check(parser, t_dot_dot))
	    return range(parser);
	return number(parser);
    }

    if (match(parser, t_lbracket))
	return list_or_tuple_or_map(parser, false);

    if (!match(parser, dt_str, dt_shlit))
	slash_exit_parse_err("not a valid primary type");

    /* str or shlit */
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value = (SlashValue){ .type = token->type == dt_str ? SLASH_STR : SLASH_SHLIT,
				.str = token->lexeme };
    return (Expr *)expr;
}

static Expr *bool_lit(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value =
	(SlashValue){ .type = SLASH_BOOL, .boolean = token->type == t_true ? true : false };
    return (Expr *)expr;
}

static Expr *number(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value = (SlashValue){ .type = SLASH_NUM, .num = str_view_to_double(token->lexeme) };
    return (Expr *)expr;
}

static Expr *range(Parser *parser)
{
    SlashRange range;
    Token *start_num_or_any = previous(parser);
    if (start_num_or_any->type != dt_num)
	range.start = 0;
    else
	range.start = str_view_to_int(start_num_or_any->lexeme);

    consume(parser, t_dot_dot, "unreachable");
    consume(parser, dt_num, "Expected end number in range expression");
    Token *end_num = previous(parser);
    range.end = str_view_to_int(end_num->lexeme);

    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value = (SlashValue){ .type = SLASH_RANGE, .range = range };
    return (Expr *)expr;
}

static Expr *map(Parser *parser)
{
    /* came from '[[' */
    MapExpr *expr = (MapExpr *)expr_alloc(parser->ast_arena, EXPR_MAP);
    expr->key_value_pairs = arena_ll_alloc(parser->ast_arena);

    do {
	KeyValuePair *pair = m_arena_alloc_struct(parser->ast_arena, KeyValuePair);
	pair->key = expression(parser);
	consume(parser, t_colon, "expected colon ':' to denote value for key in map expression");
	pair->value = expression(parser);
	arena_ll_append(expr->key_value_pairs, pair);
	if (!match(parser, t_comma))
	    break;
    } while (!check(parser, t_rbracket));

    consume(parser, t_rbracket, "Expected ']]' to terminate map");
    consume(parser, t_rbracket, "Expected ']]' to terminate map");
    return (Expr *)expr;
}

static Expr *list_or_tuple_or_map(Parser *parser, bool is_tuple)
{
    /* came from '[' or '((' */
    if (match(parser, t_lbracket))
	return map(parser);

    ListExpr *expr = (ListExpr *)expr_alloc(parser->ast_arena, EXPR_LIST);
    expr->list_type = is_tuple ? SLASH_TUPLE : SLASH_LIST;
    TokenType terminator = is_tuple ? t_rparen : t_rbracket;

    /* edge case: empty list */
    if (match(parser, terminator)) {
	if (is_tuple)
	    consume(parser, t_rparen, "Expected '))' to terminate tuple");
	expr->exprs = NULL;
	return (Expr *)expr;
    }

    /* if next is not terminator, parse list initializer */
    if (!match(parser, terminator)) {
	expr->exprs = comma_sep_exprs(parser);
	consume(parser, terminator, "Expected list terminator after arg list");
    } else {
	expr->exprs = NULL;
    }

    if (is_tuple)
	consume(parser, t_rparen, "Expected '))' to terminate tuple");
    return (Expr *)expr;
}

ArrayList parse(Arena *ast_arena, ArrayList *tokens)
{
    Parser parser = { .ast_arena = ast_arena, .tokens = tokens, .token_pos = 0 };
    ArrayList statements;
    arraylist_init(&statements, sizeof(Stmt *));

    while (!check(&parser, t_eof)) {
	Stmt *stmt = declaration(&parser);
	arraylist_append(&statements, &stmt);
    }

    return statements;
}
