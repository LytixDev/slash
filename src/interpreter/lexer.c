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

/* forward declarations */
static void lex_panic(Lexer *lexer, char *err_msg);
static void run_until(Lexer *lexer, StateFn start_state, StateFn end_state);
static void emit(Lexer *lexer, TokenType type);
StateFn lex_any(Lexer *lexer);
StateFn lex_end(Lexer *lexer);
StateFn lex_shell_arg_list(Lexer *lexer);
StateFn lex_number(Lexer *lexer);
StateFn lex_string(Lexer *lexer);
StateFn lex_identifier(Lexer *lexer);
StateFn lex_access(Lexer *lexer);
StateFn lex_comment(Lexer *lexer);
StateFn lex_lparen(Lexer *lexer);
StateFn lex_rparen(Lexer *lexer);

/*
 * How the lexer works:
 * The lexer is a finite state machine (FSM) where the `lex_` family of functions are the states and
 * functions calls are transition between states. The `lex_` functions return the next state rather
 * than calling the next state function directly. This keeps the code quite elegant and makes it
 * easy to mentally track what is going.
 * Inspired by 'Lexical Scanning in Go' by Rob Pike: https://www.youtube.com/watch?v=HxaD_trXwRE
 */

#define MACRO_TO_STR(a) #a
#define X(token) MACRO_TO_STR(t_##token),
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

void tokens_print(ArrayList *tokens)
{
    printf("--------------\n");
    printf("count\t| line, column\t| type\t\t| lexeme\n");
    printf("--- tokens ---\n");

    for (size_t i = 0; i < tokens->size; i++) {
	Token *token = arraylist_get(tokens, i);
	if (token->type == t_newline)
	    continue;
	printf("[%zu]\t| [%zu, %zu-%zu]\t| %s\t| ", i, token->line, token->start, token->end,
	       token_type_str_map[token->type]);
	str_view_print(token->lexeme);
	putchar('\n');
    }
}


/*
 * Lexer helper functions
 */
static void emit(Lexer *lexer, TokenType type)
{
    size_t token_length = lexer->pos - lexer->start;
    Token token = { .type = type,
		    .lexeme =
			(StrView){ .view = lexer->input + lexer->start, .size = token_length },
		    .line = lexer->line_count,
		    /* A single can not span across multiple lines, so this is fine . */
		    .start = lexer->pos_in_line - token_length,
		    .end = lexer->pos_in_line };
    arraylist_append(&lexer->tokens, &token);
    lexer->start = lexer->pos;
}

static TokenType *token_as_keyword(Lexer *lexer)
{
    return hashmap_get(&lexer->keywords, lexer->input + lexer->start, lexer->pos - lexer->start);
}

static char next(Lexer *lexer)
{
    if (lexer->input[lexer->pos] == 0)
	return EOF;

    lexer->pos_in_line++;
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

static void ignore(Lexer *lexer)
{
    lexer->start = lexer->pos;
}

static void backup(Lexer *lexer)
{
    if (lexer->pos == 0)
	lex_panic(lexer, "tried to backup at input position zero");

    /* This is fine because we never increment line_count and then backup() */
    lexer->pos_in_line--;
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

static void shell_arg_emit(Lexer *lexer)
{
    backup(lexer);
    if (lexer->start != lexer->pos)
	emit(lexer, t_dt_shident);
    next(lexer);
}

static void lex_panic(Lexer *lexer, char *err_msg)
{
    fprintf(stderr, "LEX PANIC!\n");
    fprintf(stderr, "--- lexer internal state --\n");
    fprintf(stderr, "line: %zu. column: %zu\ninput: %s", lexer->line_count, lexer->pos_in_line,
	    lexer->input);
    tokens_print(&lexer->tokens);
    slash_exit_lex_err(err_msg);
}


/*
 * Semantic utils
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
 * State functions
 */
StateFn lex_any(Lexer *lexer)
{
    while (true) {
	char c = next(lexer);
	switch (c) {
	case ' ':
	case '\t':
	case '\v':
	    ignore(lexer);
	    break;

	case ';':
	case '\n':
	    emit(lexer, t_newline);
	    lexer->line_count++;
	    lexer->pos_in_line = 0;
	    break;

	/* one character tokens */
	case '(':
	    /* returning lex_lparen would make no "practical" difference */
	    emit(lexer, t_lparen);
	    break;
	case ')':
	    return STATE_FN(lex_rparen);
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
	case ':':
	    emit(lexer, t_colon);
	    break;
	case '*':
	    emit(lexer, t_star);
	    break;
	case '~':
	    emit(lexer, t_tilde);
	    break;
	case '\\':
	    emit(lexer, t_backslash);
	    break;
	case '\'':
	    emit(lexer, t_qoute);
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
	case '@':
	    emit(lexer, match(lexer, '[') ? t_at_lbracket : t_at);
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
    lex_panic(lexer, "does not match anything");
    return STATE_FN(NULL);
}

StateFn lex_end(Lexer *lexer)
{
    emit(lexer, t_eof);
    return STATE_FN(NULL);
}

StateFn lex_shell_arg_list(Lexer *lexer)
{
    /*
     * NOTE: Previous token was a shell identifier.
     *       We are quite strict in that we require identifiers to be alphanumeric plus '-' and '_'.
     *       The POSIX parsing rules will f.ex allow ']', 'abc]' and much more to to be a valid
     *       token here. This may be a problem, alltough I have not found a single shell command
     *       that would not match our 'stricter' identifer requirements. The arguments to a shell
     *       command however need to be a lot more flexible.
     *
     * Shell command example: grep -rs --color=auto "some_string"
     *  Here we don't want to parse '-' as t_minus etc.
     *
     * Lexing rules for shell arg list:
     *  whitespace, tab         -> backup, emit, advance, ignore and continue
     *  $                       -> backup, emit, advance and lex access
     *  "                       -> backup, emit, advance and lex string
     *  (                       -> backup, emit, advance, lex any until rparen and continue
     *  )                       -> backup, emit, advance, stop and return lex rparen
     *  \n, ;, |, &, EOF        -> backup, emit, and stop
     *
     *  shell_arg_emit() is a helper function that will backup, emit and advance
     */
    while (1) {
	char c = next(lexer);
	switch (c) {
	case ' ':
	case '\t':
	case '\v':
	    shell_arg_emit(lexer);
	    accept_run(lexer, " \t\v");
	    ignore(lexer);
	    break;

	case '$':
	    shell_arg_emit(lexer);
	    lex_access(lexer);
	    break;
	case '"':
	    shell_arg_emit(lexer);
	    lex_string(lexer);
	    break;

	case '(':
	    shell_arg_emit(lexer);
	    lex_lparen(lexer);
	    break;
	case ')':
	    shell_arg_emit(lexer);
	    return STATE_FN(lex_rparen);

	case '\n':
	case ';':
	case '|':
	case '&':
	case EOF:
	    shell_arg_emit(lexer);
	    backup(lexer);
	    return STATE_FN(lex_any);
	}
    }
}

StateFn lex_number(Lexer *lexer)
{
    char *digits = "_0123456789";

    /* optional leading sign */
    if (accept(lexer, "+-")) {
	/* edge case: leading sign is not followed by a digit */
	if (!match_any(lexer, (char *)(digits + 1)))
	    lex_panic(lexer, "optional leading sign must be followed by a digit");
    }

    /* hex and binary */
    if (accept(lexer, "0")) {
	if (accept(lexer, "xX"))
	    digits = "_0123456789abcdefABCDEF";
	else if (accept(lexer, "bB"))
	    digits = "_01";
    }

    /* edge case: no valid digits */
    if (!match_any(lexer, (char *)(digits + 1)))
	lex_panic(lexer, "number must contain at least one valid digit");

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

    TokenType *tt = token_as_keyword(lexer);
    if (tt != NULL) {
	emit(lexer, *tt);
	return STATE_FN(lex_any);
    }

    TokenType previous = prev_token_type(lexer);
    if (previous == t_var || previous == t_loop || previous == t_func || previous == t_comma) {
	emit(lexer, t_ident);
	return STATE_FN(lex_any);
    }

    emit(lexer, t_dt_shident);
    return STATE_FN(lex_shell_arg_list);
}

StateFn lex_access(Lexer *lexer)
{
    /* came from '$' which we want to ignore */
    ignore(lexer);

    if (!is_valid_identifier(next(lexer)))
	lex_panic(lexer, "invalid identifier access");

    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);
    emit(lexer, t_access);

    return STATE_FN(lex_any);
}

StateFn lex_string(Lexer *lexer)
{
    /* ignore starting qoute */
    ignore(lexer);

    char c;
    while ((c = next(lexer)) != '"') {
	if (c == EOF)
	    lex_panic(lexer, "unterminated string");
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

StateFn lex_lparen(Lexer *lexer)
{
    emit(lexer, t_lparen);
    run_until(lexer, STATE_FN(lex_any), STATE_FN(lex_rparen));
    StateFn next = lex_rparen(lexer);
    return next;
}

StateFn lex_rparen(Lexer *lexer)
{
    emit(lexer, t_rparen);
    return STATE_FN(lex_any);
}


/*
 * FSM run functions
 */
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
		    .start = 0,
		    .line_count = 0,
		    .pos_in_line = 0 };
    keywords_init(&lexer.keywords);
    arraylist_init(&lexer.tokens, sizeof(Token));

    run(&lexer);

    hashmap_free(&lexer.keywords);
    return lexer.tokens;
}
