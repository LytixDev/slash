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
#include "interpreter/lexer.h"
#include "nicc/nicc.h"
#include "str_view.h"

static void lex_panic(Lexer *lexer, char *err_msg);
static void run_until(Lexer *lexer, StateFn start_state, StateFn end_state);
static void emit(Lexer *lexer, TokenType type);
StateFn lex_any(Lexer *lexer);
StateFn lex_end(Lexer *lexer);
StateFn lex_argument(Lexer *lexer);
StateFn lex_number(Lexer *lexer);
StateFn lex_string(Lexer *lexer);
StateFn lex_identifier(Lexer *lexer);
StateFn lex_access_indices(Lexer *lexer);
StateFn lex_access(Lexer *lexer);
StateFn lex_comment(Lexer *lexer);
StateFn lex_subshell_start(Lexer *lexer);
StateFn lex_subshell_end(Lexer *lexer);

#define MACRO_TO_STR(a) #a
#define X(token) MACRO_TO_STR(t_##token)
char *token_type_str_map[t_enum_count] = { SLASH_ALL_TOKENS };
#undef X

static void keywords_init(HashMap *keywords)
{
    /* init and populate hashmap with keyword strings */
    hashmap_init(keywords);

#define X(token)                                                                   \
    do {                                                                           \
	StrView view = { .view = #token, .size = sizeof(#token) - 1 };             \
	TokenType tt = t_##token;                                                  \
	hashmap_put(keywords, view.view, view.size, &tt, sizeof(TokenType), true); \
    } while (0);

    /* populats the hashmap by 'calling' the X macro on all keywords */
    KEYWORD_TOKENS
#undef X
}

static TokenType *keyword_get_from_start(Lexer *lexer)
{
    return hashmap_get(&lexer->keywords, lexer->input + lexer->start, lexer->pos - lexer->start);
}

void tokens_print(ArrayList *tokens)
{
    printf("--- tokens ---\n");

    for (size_t i = 0; i < tokens->size; i++) {
	Token *token = arraylist_get(tokens, i);
	printf("[%zu] (%s) ", i, token_type_str_map[token->type]);
	if (token->type != t_newline)
	    str_view_print(token->lexeme);
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

static bool check(Lexer *lexer, char expected)
{
    return peek(lexer) == expected;
}

static bool check_any(Lexer *lexer, char *expected)
{
    do {
	if (check(lexer, *expected))
	    return true;
    } while (*expected++ != 0);

    return false;
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

static bool accept(Lexer *lexer, char *accept_list)
{
    char c = next(lexer);
    do {
	if (*accept_list == c)
	    return true;
    } while (*accept_list++ != 0);

    backup(lexer);
    return false;
}

static void accept_run(Lexer *lexer, char *accept_list)
{
    while (accept(lexer, accept_list))
	;
}

static TokenType prev_token_type(Lexer *lexer)
{
    size_t size = lexer->tokens.size;
    if (size == 0)
	return t_error;
    Token *token = arraylist_get(&lexer->tokens, size - 1);
    return token->type;
}

static void shlit_seperate(Lexer *lexer)
{
    backup(lexer);
    if (lexer->start != lexer->pos)
	emit(lexer, t_dt_shlit);
}

static void lex_panic(Lexer *lexer, char *err_msg)
{
    fprintf(stderr, "LEX PANIC!\n");
    printf("--- lexer internal state --\n");
    printf("start: %zu. pos: %zu\ninput: %s", lexer->start, lexer->pos, lexer->input);
    tokens_print(&lexer->tokens);
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
    return is_numeric(c) || is_alpha(c) || c == '_' || c == '-';
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
	    /* do not go into subshell parsing mode if tuple: came from '(' or next is '(' */
	    if (!(prev_token_type(lexer) == t_identifier || prev_token_type(lexer) == t_lparen ||
		  peek(lexer) == '('))
		return STATE_FN(lex_subshell_start);
	    emit(lexer, t_lparen);
	    break;
	case ')':
	    return STATE_FN(lex_subshell_end);
	// TODO: handling brackets should have its own state fns?
	case '[':
	    emit(lexer, t_lbracket);
	    break;
	case ']':
	    emit(lexer, t_rbracket);
	    break;
	case '{':
	    emit(lexer, t_lbrace);
	    break;
	case '}':
	    emit(lexer, t_rbrace);
	    break;
	case ',':
	    emit(lexer, t_comma);
	    break;
	// TODO: should have its own state fn?
	case ':':
	    emit(lexer, t_colon);
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

	case '+':
	    if (match(lexer, '=')) {
		emit(lexer, t_plus_equal);
		break;
	    } else if (!check_any(lexer, "0123456789")) {
		emit(lexer, t_plus);
		break;
	    }
	    backup(lexer);
	    return STATE_FN(lex_number);

	case '-':
	    if (match(lexer, '=')) {
		emit(lexer, t_minus_equal);
		break;
	    } else if (!check_any(lexer, "0123456789")) {
		emit(lexer, t_minus);
		break;
	    }
	    backup(lexer);
	    return STATE_FN(lex_number);

	case '$':
	    return STATE_FN(lex_access);

	case '"':
	    return STATE_FN(lex_string);

	case '#':
	    return STATE_FN(lex_comment);

	case EOF:
	    return STATE_FN(lex_end);

	default:
	    if (is_numeric(c)) {
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

StateFn lex_argument(Lexer *lexer)
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
	    accept_run(lexer, " \t\v");
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
	    lex_access(lexer);
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
    if (accept(lexer, "+-")) {
	/* edge case where leading sign is not followed by a digit */
	if (!match_any(lexer, digits))
	    lex_panic(lexer, "optional leading sign must be followed by a digit");
    }

    /* hex and binary */
    if (accept(lexer, "0")) {
	if (accept(lexer, "xX"))
	    digits = "0123456789abcdefABCDEF";
	else if (accept(lexer, "bB"))
	    digits = "01";
    }

    accept_run(lexer, digits);

    /* treating two '.' as a seperate token */
    if (peek_ahead(lexer, 1) != '.') {
	/*
	 * here we say any number can have a decimal part, of course, this is not
	 * the case, and the parser will deal with this later
	 */
	if (accept(lexer, "."))
	    accept_run(lexer, digits);
    }

    emit(lexer, t_dt_num);
    return STATE_FN(lex_any);
}

StateFn lex_identifier(Lexer *lexer)
{
    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);

    TokenType *tt = keyword_get_from_start(lexer);
    TokenType previous = prev_token_type(lexer);
    if (tt != NULL) {
	emit(lexer, *tt);
	return STATE_FN(lex_any);
    }

    if (previous == t_var || previous == t_loop || previous == t_func || previous == t_lparen ||
	previous == t_comma || peek(lexer) == '(') {
	emit(lexer, t_identifier);
	return STATE_FN(lex_any);
    }

    emit(lexer, t_dt_shlit);
    return STATE_FN(lex_argument);
}

StateFn lex_access_indices(Lexer *lexer)
{
    /* came from '[' */
    emit(lexer, t_lbracket);

    /* index as str */
    if (match(lexer, '"')) {
	lex_string(lexer);
	if (!match(lexer, ']'))
	    slash_exit_lex_err("expected ']' after variable access");
	emit(lexer, t_rbracket);
	return STATE_FN(lex_any);
    }

    /* index as num or range */
    bool some_indicy = false;
    if (is_numeric(peek(lexer))) {
	lex_number(lexer);
	some_indicy = true;
    }

    if (match(lexer, '.')) {
	if (!match(lexer, '.'))
	    slash_exit_lex_err("expected another '.' after ranged access");
	emit(lexer, t_dot_dot);

	if (!is_numeric(peek(lexer)))
	    slash_exit_lex_err("expected base10 number");
	lex_number(lexer);
	some_indicy = true;
    }

    if (!some_indicy)
	slash_exit_lex_err("expected a valid indicy after element access");

    if (!match(lexer, ']'))
	slash_exit_lex_err("expected ']' after variable access");
    emit(lexer, t_rbracket);

    return STATE_FN(lex_any);
}

StateFn lex_access(Lexer *lexer)
{
    /* came from '$' which we want to ignore */
    ignore(lexer);

    // TODO: make sure there is at least one char here?
    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);
    emit(lexer, t_access);

    /* optional element access */
    if (!match(lexer, '['))
	return STATE_FN(lex_any);

    lex_access_indices(lexer);

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
    emit(lexer, t_dt_str);
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
    run_until(lexer, STATE_FN(lex_argument), STATE_FN(lex_subshell_end));
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
    Token token = { .type = type,
		    .lexeme = (StrView){ .view = lexer->input + lexer->start,
					 .size = lexer->pos - lexer->start } };
    arraylist_append(&lexer->tokens, &token);
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

ArrayList lex(char *input, size_t input_size)
{
    Lexer lexer = { .input = input,
		    .input_size = input_size,
		    .pos = 0,
		    .start = 0};
    keywords_init(&lexer.keywords);
    arraylist_init(&lexer.tokens, sizeof(Token));

    run(&lexer);

    hashmap_free(&lexer.keywords);
    return lexer.tokens;
}
