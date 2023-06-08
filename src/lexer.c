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
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "nicc/nicc.h"
#include "slash_str.h"

static void lex_panic(Lexer *lexer, char *err_msg);
static void run_until(Lexer *lexer, StateFn start_state, StateFn end_state);
static void emit(Lexer *lexer, TokenType type);
StateFn lex_any(Lexer *lexer);
StateFn lex_end(Lexer *lexer);
StateFn lex_cmd(Lexer *lexer);
StateFn lex_number(Lexer *lexer);
StateFn lex_string(Lexer *lexer);
StateFn lex_identifier(Lexer *lexer);
StateFn lex_interpolation(Lexer *lexer);
StateFn lex_comment(Lexer *lexer);
StateFn lex_subshell_start(Lexer *lexer);
StateFn lex_subshell_end(Lexer *lexer);


char *token_type_str_map[t_enum_count] = {
    /* keywords */
    "t_var",
    "t_if",
    "t_elif",
    "t_else",
    "t_loop",
    "t_in",
    "t_true",
    "t_false",
    "t_as",
    "t_and",
    "t_or",
    "t_not",
    "t_str",
    "t_num",
    "t_bool",

    /* single-character tokens */
    "t_lparen",
    "t_rparen",
    "t_lbrace",
    "t_rbrace",
    "t_star",
    "t_tilde",
    "t_backslash",

    /* one or two character tokens */
    "t_anp",
    "t_anp_anp",
    "t_equal",
    "t_equal_equal",
    "t_pipe",
    "t_pipe_pipe",
    "t_bang",
    "t_bang_equal",
    "t_greater",
    "t_greater_equal",
    "t_less",
    "t_less_equal",
    "t_dot",
    "t_dot_dot",

    /* data types */
    "dt_str",
    "dt_num",
    "dt_range",
    "dt_bool",
    "dt_shlit",

    "t_interpolation",
    "t_identifier",
    "t_newline",
    "t_eof",
    "t_error",
};

static void keywords_init(struct hashmap_t *keywords)
{
    /* init and populate hashmap with keyword strings */
    hashmap_init(keywords);

    char *keywords_str[keywords_len] = {
	"var", "if",  "elif", "else", "loop", "in",  "true", "false",
	"as",  "and", "or",   "not",  "str",  "num", "bool",
    };

    TokenType keywords_val[keywords_len] = {
	t_var, t_if,  t_elif, t_else, t_loop, t_in,  t_true, t_false,
	t_as,  t_and, t_or,   t_not,  t_str,  t_num, t_bool,
    };

    for (int i = 0; i < keywords_len; i++)
	hashmap_sput(keywords, keywords_str[i], &keywords_val[i], sizeof(TokenType), true);
}


void tokens_print(struct darr_t *tokens)
{
    printf("--- tokens ---\n");

    for (size_t i = 0; i < tokens->size; i++) {
	Token *token = darr_get(tokens, i);
	printf("[%zu] (%s) ", i, token_type_str_map[token->type]);
	if (token->type != t_newline)
	    slash_str_print(token->lexeme);
	putchar('\n');
    }
}

/*
 * helper functions
 */
static char next(Lexer *lexer)
{
    if (lexer->input[lexer->pos] == 0)
	return EOF;

    return lexer->input[lexer->pos++];
}

static char peek(Lexer *lexer)
{
    return lexer->input[lexer->pos];
}

static char peek_ahead(Lexer *lexer, int step)
{
    size_t idx = lexer->pos + step;
    if (idx >= lexer->input_size)
	return EOF;

    return lexer->input[idx];
}

// static char prev(Lexer *lexer)
//{
//     if (lexer->pos == 0)
//	return -1;
//     return lexer->input[lexer->pos - 1];
// }

static void ignore(Lexer *lexer)
{
    lexer->start = lexer->pos;
}

static void backup(Lexer *lexer)
{
    if (lexer->pos == 0)
	lex_panic(lexer, "tried to backup at input position zero");

    lexer->pos--;
}

static bool match(Lexer *lexer, char expected)
{
    if (peek(lexer) == expected) {
	next(lexer);
	return true;
    }
    return false;
}

static bool match_any(Lexer *lexer, char *expected)
{
    do {
	if (match(lexer, *expected))
	    return true;
    } while (*expected++ != 0);

    return false;
}

// static bool consume_single(Lexer *lexer, char accept)
//{
//     if (peek(lexer) == accept) {
//	next(lexer);
//	return true;
//     }
//     return false;
// }

static bool consume(Lexer *lexer, char *accept)
{
    char c = next(lexer);
    do {
	if (*accept == c)
	    return true;
    } while (*accept++ != 0);

    backup(lexer);
    return false;
}

static void consume_run(Lexer *lexer, char *accept)
{
    while (consume(lexer, accept))
	;
}

static TokenType prev_token_type(Lexer *lexer)
{
    size_t size = lexer->tokens->size;
    if (size == 0)
	return t_error;
    Token *token = darr_get(lexer->tokens, size - 1);
    return token->type;
}

static void shlit_seperate(Lexer *lexer)
{
    backup(lexer);
    if (lexer->start != lexer->pos)
	emit(lexer, dt_shlit);
}

static void lex_panic(Lexer *lexer, char *err_msg)
{
    printf("--- lex internal state --\n");
    printf("start: %zu. pos: %zu\n", lexer->start, lexer->pos);
    tokens_print(lexer->tokens);
    slash_exit_lex_err(err_msg);
}


/*
 * semantic
 */
static bool is_numeric(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_valid_identifier(char c)
{
    return is_numeric(c) || is_alpha(c) || c == '_';
}


/*
 * state functions
 */
StateFn lex_any(Lexer *lexer)
{
    while (1) {
	char c = next(lexer);
	switch (c) {
	case ' ':
	case '\t':
	case '\v':
	    ignore(lexer);
	    break;

	case '\n':
	    emit(lexer, t_newline);
	    break;

	/* one character tokens */
	case '(':
	    return STATE_FN(lex_subshell_start);
	case ')':
	    return STATE_FN(lex_subshell_end);
	case '{':
	    emit(lexer, t_lbrace);
	    break;
	case '}':
	    emit(lexer, t_rbrace);
	    break;
	// TODO: handle these better
	case '*':
	    emit(lexer, t_star);
	    break;
	case '~':
	    emit(lexer, t_tilde);
	    break;
	case '\\':
	    emit(lexer, t_backslash);
	    break;

	/* one or two character tokens */
	case '=':
	    emit(lexer, match(lexer, '=') ? t_equal_equal : t_equal);
	    break;
	case '&':
	    emit(lexer, match(lexer, '&') ? t_anp_anp : t_anp);
	    break;
	case '|':
	    emit(lexer, match(lexer, '|') ? t_pipe_pipe : t_pipe);
	    break;
	case '!':
	    emit(lexer, match(lexer, '=') ? t_bang_equal : t_bang);
	    break;
	case '>':
	    emit(lexer, match(lexer, '=') ? t_greater_equal : t_greater);
	    break;
	case '<':
	    emit(lexer, match(lexer, '=') ? t_less_equal : t_less);
	    break;
	case '.':
	    emit(lexer, match(lexer, '.') ? t_dot_dot : t_dot);
	    break;

	case '$':
	    return STATE_FN(lex_interpolation);

	case '"':
	    return STATE_FN(lex_string);

	case '#':
	    return STATE_FN(lex_comment);

	case EOF:
	    return STATE_FN(lex_end);

	default:
	    if (is_numeric(c) || c == '+' || c == '-') {
		backup(lexer);
		return STATE_FN(lex_number);
	    }

	    if (is_valid_identifier(c)) {
		backup(lexer);
		return STATE_FN(lex_identifier);
	    }

	    goto panic;
	}
    }

panic:
    lex_panic(lexer, "lex_any unsuccesfull");
    return STATE_FN(NULL);
}

StateFn lex_end(Lexer *lexer)
{
    emit(lexer, t_eof);
    return STATE_FN(NULL);
}

StateFn lex_cmd(Lexer *lexer)
{
    /* previous token was a shell literal */

    while (1) {
	char c = next(lexer);
	switch (c) {
	/* space and tab is treated as seperators when parsing arguments to a shell literal */
	case ' ':
	case '\t':
	case '\v':
	    shlit_seperate(lexer);
	    consume_run(lexer, " \t\v");
	    ignore(lexer);
	    break;

	case '(':
	    shlit_seperate(lexer);
	    next(lexer);
	    lex_subshell_start(lexer);
	    break;

	case '$':
	    shlit_seperate(lexer);
	    next(lexer);
	    lex_interpolation(lexer);
	    break;

	case '"':
	    shlit_seperate(lexer);
	    next(lexer);
	    lex_string(lexer);
	    break;

	/* terminating characters */
	case ')':
	    shlit_seperate(lexer);
	    next(lexer);
	    return STATE_FN(lex_subshell_end);
	case EOF:
	case '&':
	case '|':
	case '\n':
	    shlit_seperate(lexer);
	    return STATE_FN(lex_any);
	}
    }
}

// TODO: octal support?
StateFn lex_number(Lexer *lexer)
{
    char *digits = "0123456789";

    /* optional leading sign */
    if (consume(lexer, "+-")) {
	/* edge case where leading sign is not followed by a digit */
	if (!match_any(lexer, digits))
	    lex_panic(lexer, "optional leading sign must be followed by a digit");
    }

    /* hex and binary */
    if (consume(lexer, "0")) {
	if (consume(lexer, "xX"))
	    digits = "0123456789abcdefABCDEF";
	else if (consume(lexer, "bB"))
	    digits = "01";
    }

    consume_run(lexer, digits);

    /* treating two '.' as a seperate token */
    if (peek_ahead(lexer, 1) != '.') {
	/*
	 * here we say any number can have a decimal part, of course, this is not
	 * the case, and the parser will deal with this later
	 */
	if (consume(lexer, "."))
	    consume_run(lexer, digits);
    }

    emit(lexer, dt_num);
    return STATE_FN(lex_any);
}

StateFn lex_identifier(Lexer *lexer)
{
    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);

    /* figure out what identifier we are dealing with */
    // TODO: better use of string
    size_t size = lexer->pos - lexer->start + 1;
    char identifier[size];
    memcpy(identifier, lexer->input + lexer->start, size - 1);
    identifier[size - 1] = 0;

    TokenType *tt = hashmap_sget(lexer->keywords, identifier);
    TokenType previous = prev_token_type(lexer);
    if (tt == NULL && (previous == t_var || previous == t_loop)) {
	emit(lexer, t_identifier);
    } else if (tt == NULL) {
	emit(lexer, dt_shlit);
	return STATE_FN(lex_cmd);
    } else {
	emit(lexer, *tt);
    }

    return STATE_FN(lex_any);
}

StateFn lex_interpolation(Lexer *lexer)
{
    /* came from '$' which we want to ignore */
    ignore(lexer);

    // TODO: make sure there is at least one char here?
    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);
    emit(lexer, t_interpolation);

    return STATE_FN(lex_any);
}

StateFn lex_string(Lexer *lexer)
{
    /* ignore starting qoute */
    ignore(lexer);

    char c;
    while ((c = next(lexer)) != '"') {
	if (c == EOF)
	    lex_panic(lexer, "String not terminated");
    }

    /* backup final qoute */
    backup(lexer);
    emit(lexer, dt_str);
    /* advance past final qoute and ignore it */
    next(lexer);
    ignore(lexer);

    return STATE_FN(lex_any);
}

StateFn lex_comment(Lexer *lexer)
{
    /* came from '#' */
    char c;
    while ((c = next(lexer)) != '\n') {
	if (c == EOF)
	    return STATE_FN(lex_end);
    }
    backup(lexer);
    ignore(lexer);
    return STATE_FN(lex_any);
}

StateFn lex_subshell_start(Lexer *lexer)
{
    emit(lexer, t_lparen);
    run_until(lexer, STATE_FN(lex_any), STATE_FN(lex_subshell_end));
    StateFn next = lex_subshell_end(lexer);
    return next;
}

StateFn lex_subshell_end(Lexer *lexer)
{
    emit(lexer, t_rparen);
    return STATE_FN(lex_any);
}


/*
 * fsm
 */
static void emit(Lexer *lexer, TokenType type)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->lexeme =
	(SlashStr){ .p = lexer->input + lexer->start, .size = lexer->pos - lexer->start };
    darr_append(lexer->tokens, token);

    lexer->start = lexer->pos;
}

static void run_until(Lexer *lexer, StateFn start_state, StateFn end_state)
{
    for (StateFn state = start_state; state.fn != end_state.fn;) {
	if (state.fn == NULL)
	    lex_panic(lexer, "expected end_state not reached");
	state = state.fn(lexer);
    }
}

static void run(Lexer *lexer)
{
    StateFn start_state = lex_any(lexer);
    for (StateFn state = start_state; state.fn != NULL;)
	state = state.fn(lexer);
}

struct darr_t *lex(char *input, size_t input_size)
{
    struct hashmap_t keywords;
    keywords_init(&keywords);

    Lexer lexer = { .input = input,
		    .input_size = input_size,
		    .pos = 0,
		    .start = 0,
		    .tokens = darr_malloc(),
		    .keywords = &keywords };
    run(&lexer);

    hashmap_free(&keywords);
    return lexer.tokens;
}
