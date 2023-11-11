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

#include "interpreter/ast.h"
#include "interpreter/error.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "interpreter/value/slash_value.h"
#include "lib/arena_ll.h"
#include "lib/str_view.h"
#include "nicc/nicc.h"
#include "sac/sac.h"


/*
 * non-terminal grammar rule functions
 */

static void newline(Parser *parser);
static void expr_promotion(Parser *parser);
static Stmt *declaration(Parser *parser);
static Stmt *var_decl(Parser *parser);
static Stmt *and_or(Parser *parser);
/* stmts */
static Stmt *statement(Parser *parser);
static Stmt *loop_stmt(Parser *parser);
static Stmt *assert_stmt(Parser *parser);
static Stmt *if_stmt(Parser *parser);
static Stmt *pipeline_stmt(Parser *parser);
static Stmt *redirect_stmt(Parser *parser, Stmt *left);
static Stmt *cmd_stmt(Parser *parser);
static Stmt *block(Parser *parser);
static Stmt *assignment_stmt(Parser *parser);
/* exprs */
static Expr *top_level_expr(Parser *parser);
static Expr *expression(Parser *parser);
static SequenceExpr *sequence(Parser *parser, TokenType terminator);
static Expr *logical_or(Parser *parser);
static Expr *logical_and(Parser *parser);
static Expr *equality(Parser *parser);
static Expr *comparison(Parser *parser);
static Expr *term(Parser *parser);
static Expr *factor(Parser *parser);
static Expr *exponentiation(Parser *parser);
static Expr *unary(Parser *parser);
static Expr *single(Parser *parser);
static Expr *subshell(Parser *parser);
static Expr *subscript(Parser *parser);
static Expr *access(Parser *parser);
static Expr *primary(Parser *parser);
static Expr *bool_lit(Parser *parser);
static Expr *number(Parser *parser);
static Expr *range(Parser *parser);
static Expr *list(Parser *parser);
static Expr *map(Parser *parser);
static Expr *grouping(Parser *parser);


/* util/helper functions */
static Token *peek(Parser *parser)
{
    return arraylist_get(parser->tokens, parser->token_pos);
}

static Token *previous(Parser *parser)
{
    return arraylist_get(parser->tokens, parser->token_pos - 1);
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

static void backup(Parser *parser)
{
    if (parser->token_pos == 0)
	report_parse_err(parser, "Internal error: attempted to backup() at pos = 0");
    parser->token_pos--;
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
	if (check_single(parser, type, step)) {
	    va_end(args);
	    return true;
	}
    }

    va_end(args);
    return false;
}

#define VA_NUMBER_OF_ARGS(...) (sizeof((int[]){ __VA_ARGS__ }) / sizeof(int))
#define check(parser, ...) check_either(parser, 0, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)
#define check_ahead(parser, n, ...) \
    check_either(parser, n, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)
#define check_arg_end(parser)                                                                  \
    check(parser, t_newline, t_eof, t_pipe, t_pipe_pipe, t_greater, t_greater_greater, t_less, \
	  t_anp, t_anp_anp, t_rparen, t_rbrace)

static Token *consume(Parser *parser, TokenType expected, char *err_msg)
{
    if (!check_single(parser, expected, 0)) {
	report_parse_err(parser, err_msg);
	/* we need to "unconsume" the token */
	parser->token_pos--;
    }

    return advance(parser);
}

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
	if (match_single(parser, type)) {
	    va_end(args);
	    return true;
	}
    }

    va_end(args);
    return false;
}

#define match(parser, ...) match_either(parser, VA_NUMBER_OF_ARGS(__VA_ARGS__), __VA_ARGS__)


/* grammar functions */
static void newline(Parser *parser)
{
    consume(parser, t_newline, "Expected newline or semicolon");
    ignore(parser, t_newline);
}

static void expr_promotion(Parser *parser)
{
    if (check(parser, t_rbrace, t_anp_anp, t_pipe_pipe))
	return;
    newline(parser);
}

static Stmt *declaration(Parser *parser)
{
    ignore(parser, t_newline);
    Stmt *stmt = match(parser, t_var) ? var_decl(parser) : and_or(parser);
    ignore(parser, t_newline);
    return stmt;
}

static Stmt *var_decl(Parser *parser)
{
    SeqVarStmt *seq_var = NULL;
    /* came from 'var' */
    Token *name = consume(parser, t_ident, "Expected variable name");
    /* SeqVarStmt */
    if (match(parser, t_comma)) {
	seq_var = (SeqVarStmt *)stmt_alloc(parser->ast_arena, STMT_SEQ_VAR);
	arena_ll_init(parser->ast_arena, &seq_var->names);
	arena_ll_append(&seq_var->names, &name->lexeme);
	do {
	    name = consume(parser, t_ident, "Expected variable name");
	    arena_ll_append(&seq_var->names, &name->lexeme);
	} while (match(parser, t_comma));
    }

    consume(parser, t_equal, "Expected variable definition");
    Expr *initializer = top_level_expr(parser);
    expr_promotion(parser);
    if (seq_var != NULL) {
	seq_var->initializer = initializer;
	return (Stmt *)seq_var;
    }

    VarStmt *stmt = (VarStmt *)stmt_alloc(parser->ast_arena, STMT_VAR);
    stmt->name = name->lexeme;
    stmt->initializer = initializer;
    return (Stmt *)stmt;
}

static Stmt *and_or(Parser *parser)
{
    Stmt *left = statement(parser);
    while (match(parser, t_anp_anp, t_pipe_pipe)) {
	BinaryStmt *stmt = (BinaryStmt *)stmt_alloc(parser->ast_arena, STMT_BINARY);
	stmt->left = left;
	stmt->operator_ = previous(parser)->type;
	stmt->right_stmt = statement(parser);
	left = (Stmt *)stmt;
    }
    return left;
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

    if (match(parser, t_dt_text_lit))
	return pipeline_stmt(parser);

    if (match(parser, t_lbrace))
	return block(parser);

    return assignment_stmt(parser);
}

static Stmt *loop_stmt(Parser *parser)
{
    /* came from 'loop' */
    if (match(parser, t_ident)) {
	/* loop IDENTIFIER in expression { ... } */
	Token *var_name = previous(parser);
	consume(parser, t_in, "Expected 'in' keyword to continue loop statement");
	/* A runtime error will be thrown if expression can not be iterated over */
	Expr *iterable = top_level_expr(parser);

	IterLoopStmt *iter_loop = (IterLoopStmt *)stmt_alloc(parser->ast_arena, STMT_ITER_LOOP);
	iter_loop->var_name = var_name->lexeme;
	iter_loop->underlying_iterable = iterable;
	consume(parser, t_lbrace, "Expected block '{' after loop condition");
	iter_loop->body_block = (BlockStmt *)block(parser);
	return (Stmt *)iter_loop;
    }

    LoopStmt *stmt = (LoopStmt *)stmt_alloc(parser->ast_arena, STMT_LOOP);
    stmt->condition = expression(parser);
    consume(parser, t_lbrace, "Expected '{' after loop condition");
    stmt->body_block = (BlockStmt *)block(parser);
    return (Stmt *)stmt;
}

static Stmt *assert_stmt(Parser *parser)
{
    /* came from 'assert' */
    AssertStmt *stmt = (AssertStmt *)stmt_alloc(parser->ast_arena, STMT_ASSERT);
    stmt->expr = top_level_expr(parser);
    expr_promotion(parser);
    return (Stmt *)stmt;
}

static Stmt *if_stmt(Parser *parser)
{
    /* came from 'if' or 'elif' */
    IfStmt *stmt = (IfStmt *)stmt_alloc(parser->ast_arena, STMT_IF);
    stmt->condition = expression(parser);
    consume(parser, t_lbrace, "Expected '{' after if-statement");
    stmt->then_branch = block(parser);

    stmt->else_branch = NULL;
    ignore(parser, t_newline);
    if (match(parser, t_elif)) {
	stmt->else_branch = if_stmt(parser);
    } else if (match(parser, t_else)) {
	consume(parser, t_lbrace, "Expected '{' after else-statement");
	stmt->else_branch = block(parser);
    }

    return (Stmt *)stmt;
}

static Stmt *pipeline_stmt(Parser *parser)
{
    /* came from t_dt_text_lit */
    Stmt *left = cmd_stmt(parser);
    if (match(parser, t_greater, t_greater_greater, t_less))
	return redirect_stmt(parser, left);

    if (!match(parser, t_pipe))
	return left;

    consume(parser, t_dt_text_lit, "Expected shell command after pipe symbol");
    PipelineStmt *stmt = (PipelineStmt *)stmt_alloc(parser->ast_arena, STMT_PIPELINE);
    stmt->left = (CmdStmt *)left;
    stmt->right = pipeline_stmt(parser);
    return (Stmt *)stmt;
}

static Stmt *redirect_stmt(Parser *parser, Stmt *left)
{
    /* already consumed cmd_stmt and operand: '>' */
    BinaryStmt *stmt = (BinaryStmt *)stmt_alloc(parser->ast_arena, STMT_BINARY);
    stmt->left = left;
    stmt->operator_ = previous(parser)->type;
    stmt->right_expr = expression(parser);
    return (Stmt *)stmt;
}

static Stmt *cmd_stmt(Parser *parser)
{
    /* came from t_dt_text_lit */
    Token *cmd_name = previous(parser);

    CmdStmt *stmt = (CmdStmt *)stmt_alloc(parser->ast_arena, STMT_CMD);
    stmt->cmd_name = cmd_name->lexeme;
    /* edge case where there are no arguments */
    if (check_arg_end(parser)) {
	stmt->arg_exprs = NULL;
	return (Stmt *)stmt;
    }

    stmt->arg_exprs = arena_ll_alloc(parser->ast_arena);
    while (!check_arg_end(parser))
	arena_ll_append(stmt->arg_exprs, expression(parser));

    return (Stmt *)stmt;
}

static Stmt *block(Parser *parser)
{
    /* came from '{' */
    BlockStmt *stmt = (BlockStmt *)stmt_alloc(parser->ast_arena, STMT_BLOCK);
    stmt->statements = arena_ll_alloc(parser->ast_arena);
    /* ignore all leading newlines inside block */
    ignore(parser, t_newline);

    while (!check(parser, t_rbrace) && !is_at_end(parser))
	arena_ll_append(stmt->statements, declaration(parser));

    consume(parser, t_rbrace, "Expected '}' to terminate block");
    return (Stmt *)stmt;
}

static Stmt *assignment_stmt(Parser *parser)
{
    Expr *expr = top_level_expr(parser);
    if (!match(parser, t_equal, t_plus_equal, t_minus_equal, t_star_equal, t_star_star_equal,
	       t_slash_equal, t_slash_slash_equal, t_percent_equal)) {
	ExpressionStmt *stmt = (ExpressionStmt *)stmt_alloc(parser->ast_arena, STMT_EXPRESSION);
	stmt->expression = expr;
	expr_promotion(parser);
	return (Stmt *)stmt;
    }

    /* access part of assignment */
    Token *assignment_op = previous(parser);
    Expr *value = top_level_expr(parser);
    expr_promotion(parser);

    AssignStmt *stmt = (AssignStmt *)stmt_alloc(parser->ast_arena, STMT_ASSIGN);
    stmt->var = expr;
    stmt->assignment_op = assignment_op->type;
    stmt->value = value;
    return (Stmt *)stmt;
}

static Expr *top_level_expr(Parser *parser)
{
    Expr *expr = expression(parser);
    if (match(parser, t_comma)) {
	SequenceExpr *seq_expr = sequence(parser, t_newline);
	/* edge case: don't want top level call to sequence() to consume newline */
	if (previous(parser)->type == t_newline)
	    backup(parser);
	arena_ll_prepend(&seq_expr->seq, expr);
	return (Expr *)seq_expr;
    }
    return expr;
}

static Expr *expression(Parser *parser)
{
    return logical_or(parser);
}

static SequenceExpr *sequence(Parser *parser, TokenType terminator)
{
    SequenceExpr *expr = (SequenceExpr *)expr_alloc(parser->ast_arena, EXPR_SEQUENCE);
    arena_ll_init(parser->ast_arena, &expr->seq);
    do {
	if (match(parser, terminator))
	    break;
	ignore(parser, t_newline);
	arena_ll_append(&expr->seq, expression(parser));
	if (terminator != t_newline)
	    ignore(parser, t_newline);
    } while (!match(parser, terminator) && match(parser, t_comma));

    return expr;
}

static Expr *logical_or(Parser *parser)
{
    Expr *expr = logical_and(parser);

    while (match(parser, t_or)) {
	Expr *right = logical_and(parser);
	BinaryExpr *expr_bin = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	expr_bin->left = expr;
	expr_bin->operator_ = t_or;
	expr_bin->right = right;
	expr = (Expr *)expr_bin;
    }

    return expr;
}

static Expr *logical_and(Parser *parser)
{
    Expr *expr = equality(parser);

    while (match(parser, t_and)) {
	Expr *right = equality(parser);
	BinaryExpr *expr_bin = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	expr_bin->left = expr;
	expr_bin->operator_ = t_and;
	expr_bin->right = right;
	expr = (Expr *)expr_bin;
    }

    return expr;
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
    Expr *expr = exponentiation(parser);

    while (match(parser, t_slash, t_slash_slash, t_star, t_percent)) {
	Token *operator_ = previous(parser);
	Expr *right = exponentiation(parser);

	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = operator_->type;
	bin_expr->right = right;
	expr = (Expr *)bin_expr;
    }
    return expr;
}

static Expr *exponentiation(Parser *parser)
{
    Expr *expr = unary(parser);

    while (match(parser, t_star_star)) {
	Expr *right = unary(parser);
	BinaryExpr *bin_expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	bin_expr->left = expr;
	bin_expr->operator_ = t_star_star;
	bin_expr->right = right;
	expr = (Expr *)bin_expr;
    }
    return expr;
}

static Expr *unary(Parser *parser)
{
    if (!match(parser, t_not, t_minus))
	return single(parser);

    UnaryExpr *expr = (UnaryExpr *)expr_alloc(parser->ast_arena, EXPR_UNARY);
    expr->operator_ = previous(parser)->type;
    expr->right = unary(parser);
    return (Expr *)expr;
}

static Expr *single(Parser *parser)
{
    Expr *left;
    if (match(parser, t_lparen)) {
	if (check(parser, t_dt_text_lit)) {
	    left = subshell(parser);
	} else {
	    parser->token_pos--;
	    /* continue the "normal" recursive path */
	    left = subscript(parser);
	}
    } else {
	/* continue the "normal" recursive path */
	left = subscript(parser);
    }

    /* contains */
    if (match(parser, t_in)) {
	BinaryExpr *expr = (BinaryExpr *)expr_alloc(parser->ast_arena, EXPR_BINARY);
	expr->left = left;
	expr->operator_ = t_in;
	expr->right = expression(parser);
	return (Expr *)expr;
    }

    if (match(parser, t_as)) {
	CastExpr *expr = (CastExpr *)expr_alloc(parser->ast_arena, EXPR_CAST);
	expr->expr = left;
	// TODO: support more casts later
	if (!match(parser, t_ident)) {
	    report_parse_err(parser, "Expected identifier after cast");
	    /* move past token and continue as normal */
	    parser->token_pos++;
	    return NULL;
	}
	/// expr->as = SLASH_NUM;//token_type_to_slash_type(previous(parser)->type);
	return (Expr *)expr;
    }

    /* method call */
    if (match(parser, t_dot)) {
	MethodExpr *expr = (MethodExpr *)expr_alloc(parser->ast_arena, EXPR_METHOD);
	Token *method_name = consume(parser, t_ident, "Expected method name");
	expr->obj = left;
	expr->method_name = method_name->lexeme;
	consume(parser, t_lparen, "Expected left paren");
	if (!match(parser, t_rparen)) {
	    expr->args = sequence(parser, t_rparen);
	} else {
	    expr->args = NULL;
	}
	return (Expr *)expr;
    }

    /* neither contains nor method_call matched */
    return left;
}

static Expr *subshell(Parser *parser)
{
    /* came from '(' */
    consume(parser, t_dt_text_lit, "Expected shell literal after '('");
    SubshellExpr *expr = (SubshellExpr *)expr_alloc(parser->ast_arena, EXPR_SUBSHELL);
    expr->stmt = pipeline_stmt(parser);
    consume(parser, t_rparen, "Expected ')' after subshell");
    return (Expr *)expr;
}

static Expr *subscript(Parser *parser)
{
    Expr *expr = access(parser);
    while (match(parser, t_lbracket)) {
	SubscriptExpr *subscript_expr =
	    (SubscriptExpr *)expr_alloc(parser->ast_arena, EXPR_SUBSCRIPT);
	subscript_expr->expr = expr;
	subscript_expr->access_value = expression(parser);
	consume(parser, t_rbracket, "Expected ']' after variable subscript");
	expr = (Expr *)subscript_expr;
    }

    return expr;
}

static Expr *access(Parser *parser)
{
    if (!match(parser, t_access))
	return primary(parser);

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
    if (match(parser, t_dt_num)) {
	if (check(parser, t_dot_dot))
	    return range(parser);
	return number(parser);
    }
    if (match(parser, t_lbracket))
	return list(parser);
    if (match(parser, t_at_lbracket))
	return map(parser);
    if (match(parser, t_lparen))
	return grouping(parser);

    if (!match(parser, t_dt_str, t_dt_text_lit)) {
	report_parse_err(parser, "Not a valid primary type");
	return NULL;
    }

    Token *token = previous(parser);
    /* text_lit */
    if (token->type == t_dt_text_lit) {
	LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
	expr->value = (SlashValue){ .T = &text_lit_type_info, .text_lit = token->lexeme };
	return (Expr *)expr;
    }

    /* str */
    StrExpr *expr = (StrExpr *)expr_alloc(parser->ast_arena, EXPR_STR);
    expr->view = token->lexeme;
    return (Expr *)expr;
}

static Expr *bool_lit(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value =
	(SlashValue){ .T = &bool_type_info, .boolean = token->type == t_true ? true : false };
    return (Expr *)expr;
}

static Expr *number(Parser *parser)
{
    Token *token = previous(parser);
    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value = (SlashValue){ .T = &num_type_info, .num = str_view_to_double(token->lexeme) };
    return (Expr *)expr;
}

static Expr *range(Parser *parser)
{
    SlashRange range;
    Token *start_num_or_any = previous(parser);
    if (start_num_or_any == NULL || start_num_or_any->type != t_dt_num)
	range.start = 0;
    else
	range.start = str_view_to_int(start_num_or_any->lexeme);

    consume(parser, t_dot_dot, "This error should be unreachable");
    consume(parser, t_dt_num, "Expected second number after '..' in range expression");
    Token *end_num = previous(parser);
    range.end = str_view_to_int(end_num->lexeme);

    LiteralExpr *expr = (LiteralExpr *)expr_alloc(parser->ast_arena, EXPR_LITERAL);
    expr->value = (SlashValue){ .T = &range_type_info, .range = range };
    return (Expr *)expr;
}

static Expr *list(Parser *parser)
{
    /* came from '[' */
    ListExpr *expr = (ListExpr *)expr_alloc(parser->ast_arena, EXPR_LIST);

    /* if next is not ']', parse list initializer */
    if (!match(parser, t_rbracket)) {
	expr->exprs = sequence(parser, t_rbracket);
    } else {
	expr->exprs = NULL;
    }

    return (Expr *)expr;
}

static Expr *map(Parser *parser)
{
    /* came from '@[' */
    MapExpr *expr = (MapExpr *)expr_alloc(parser->ast_arena, EXPR_MAP);
    /* Case where initializer is empty */
    if (match(parser, t_rbracket)) {
	expr->key_value_pairs = NULL;
	return (Expr *)expr;
    }

    expr->key_value_pairs = arena_ll_alloc(parser->ast_arena);

    do {
	KeyValuePair *pair = m_arena_alloc_struct(parser->ast_arena, KeyValuePair);
	pair->key = expression(parser);
	consume(parser, t_colon, "Expected colon ':' to denote value for key in map expression");
	pair->value = expression(parser);
	arena_ll_append(expr->key_value_pairs, pair);
	if (!match(parser, t_comma))
	    break;
    } while (!check(parser, t_rbracket));

    consume(parser, t_rbracket, "Expected ']' to terminate map");
    return (Expr *)expr;
}

static Expr *grouping(Parser *parser)
{
    /* came from '(' */
    Expr *expr = expression(parser);
    if (match(parser, t_comma)) {
	SequenceExpr *seq_expr = sequence(parser, t_rparen);
	arena_ll_prepend(&seq_expr->seq, expr);
	return (Expr *)seq_expr;
    }
    GroupingExpr *grouping = (GroupingExpr *)expr_alloc(parser->ast_arena, EXPR_GROUPING);
    grouping->expr = expr;
    consume(parser, t_rparen, "Expected ')' after grouping expression");
    return (Expr *)grouping;
}

StmtsOrErr parse(Arena *ast_arena, ArrayList *tokens, char *input)
{
    Parser parser = {
	.ast_arena = ast_arena, .tokens = tokens, .token_pos = 0, .input = input, .had_error = false
    };
    ArrayList statements;
    arraylist_init(&statements, sizeof(Stmt *));

    /* edge case: empty source file */
    ignore(&parser, t_newline);
    if (check(&parser, t_eof))
	return (StmtsOrErr){ .had_error = parser.had_error, .stmts = statements };

    while (!check(&parser, t_eof)) {
	Stmt *stmt = declaration(&parser);
	arraylist_append(&statements, &stmt);
    }

    return (StmtsOrErr){ .had_error = parser.had_error, .stmts = statements };
}
