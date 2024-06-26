/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "interpreter/error.h"
#include "interpreter/lexer.h"
#include "lib/str_builder.h"
#include "lib/str_view.h"
#include "nicc/nicc.h"

/* forward declarations */
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
	printf("Tokens:\n");
	printf("count\t| line, column\t| type\t\t| lexeme\n");

	for (size_t i = 0; i < tokens->size; i++) {
		Token *token = arraylist_get(tokens, i);
		if (token->type == t_newline)
			continue;
		printf("[%zu]\t| [%zu, %zu-%zu]\t| %-10s\t| ", i, token->line, token->start, token->end,
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
					/* A single token can not span across multiple lines, so this is fine . */
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
		ASSERT_NOT_REACHED;

	/* This is fine because we never increment line_count and then backup() */
	lexer->pos_in_line--;
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
		emit(lexer, t_dt_text_lit);
	next(lexer);
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
			/* returning lex_lparen would make no "practical" difference ... I think? */
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
		case '\\':
			emit(lexer, t_backslash);
			break;
		// case '\'':
		//     emit(lexer, t_qoute);
		//     break;

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
		case '>': {
			if (match(lexer, '='))
				emit(lexer, t_greater_equal);
			else
				emit(lexer, match(lexer, '>') ? t_greater_greater : t_greater);
			break;
		}
		case '<':
			emit(lexer, match(lexer, '=') ? t_less_equal : t_less);
			break;
		case '.': {
			/* TODO: botch, please fix */
			if (peek(lexer) == '/') {
				emit(lexer, t_dot);
				return STATE_FN(lex_shell_arg_list);
			}
			emit(lexer, match(lexer, '.') ? t_dot_dot : t_dot);
			break;
		}
		case '@':
			emit(lexer, match(lexer, '[') ? t_at_lbracket : t_at);
			break;
		case '+':
			emit(lexer, match(lexer, '=') ? t_plus_equal : t_plus);
			break;
		case '-':
			emit(lexer, match(lexer, '=') ? t_minus_equal : t_minus);
			break;
		case '%':
			emit(lexer, match(lexer, '=') ? t_percent_equal : t_percent);
			break;
		case '/': {
			/* '/', '/=', '//', '//=' */
			if (match(lexer, '='))
				emit(lexer, t_slash_equal);
			else if (match(lexer, '/'))
				emit(lexer, match(lexer, '=') ? t_slash_slash_equal : t_slash_slash);
			else
				emit(lexer, t_slash);
			break;
		}
		case '*': {
			/* '*', '*=', '**', '**=' */
			if (match(lexer, '='))
				emit(lexer, t_star_equal);
			else if (match(lexer, '*'))
				emit(lexer, match(lexer, '=') ? t_star_star_equal : t_star_star);
			else
				emit(lexer, t_star);
			break;
		}

		case '$':
			return STATE_FN(lex_access);

		case '"':
		case '\'':
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

			report_lex_err(lexer, true, "Unrecognized character");
		}
	}
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
	 *  whitespace, tab           -> backup, emit, advance, ignore and continue
	 *  $                         -> backup, emit, advance and lex access
	 *  ", '                      -> backup, emit, advance and lex string
	 *  (                         -> backup, emit, advance, lex any until rparen and continue
	 *  )                         -> backup, emit, advance, stop and return lex rparen
	 *  \n, }, ;, |, >, <, &, EOF -> backup, emit, and stop
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
		case '\'':
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
		case '}':
		case ';':
		case '|':
		case '>':
		case '<':
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

	/* hex and binary */
	if (accept(lexer, "0")) {
		bool changed_base = false;
		if (accept(lexer, "xX")) {
			digits = "_0123456789abcdefABCDEF";
			changed_base = true;
		} else if (accept(lexer, "bB")) {
			digits = "_01";
			changed_base = true;
		}
		/* edge case: no valid digits */
		if (changed_base && !match_any(lexer, (char *)(digits + 1))) {
			report_lex_err(lexer, true, "Number must contain at least one valid digit");
			return STATE_FN(lex_any);
		}
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

	TokenType *tt = token_as_keyword(lexer);
	if (tt != NULL) {
		emit(lexer, *tt);
		return STATE_FN(lex_any);
	}

	TokenType previous = prev_token_type(lexer);
	if (previous == t_var || previous == t_loop || previous == t_comma || previous == t_as ||
		previous == t_equal || previous == t_func) {
		emit(lexer, t_ident);
		return STATE_FN(lex_any);
	}

	emit(lexer, t_dt_text_lit);
	return STATE_FN(lex_shell_arg_list);
}

StateFn lex_access(Lexer *lexer)
{
	/* came from '$' which we want to ignore */
	ignore(lexer);

	/* edge case: $? is a valid access */
	if (match(lexer, '?')) {
		emit(lexer, t_access);
		return STATE_FN(lex_any);
	}

	if (!is_valid_identifier(next(lexer))) {
		report_lex_err(lexer, true, "Illegal identifier name");
		return STATE_FN(lex_any);
	}

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

	/* find which string type this is (' or ") */
	char str_type = peek_ahead(lexer, -1);
	size_t str_start = lexer->pos_in_line;
	size_t str_end;

	StrBuilder sb;
	str_builder_init(&sb, lexer->arena);
	char c;
continue_str_lexing:
	while ((c = next(lexer)) != str_type) {
		switch (c) {
		case EOF:
		case '\n':
			backup(lexer);
			report_lex_err(lexer, true, "Unterminated string literal");
			return STATE_FN(lex_any);
		case '\\': {
			/* For single qouted strings we do escaping */
			if (str_type == '\'') {
				str_builder_append_char(&sb, '\\');
				break;
			}

			char next_char = next(lexer);
			switch (next_char) {
			case '"':
				str_builder_append_char(&sb, next_char);
				break;
			case 'n':
				str_builder_append_char(&sb, '\n');
				break;
			case '\\':
				str_builder_append_char(&sb, '\\');
				break;
			default:
				report_lex_err(lexer, true, "Unknown escape sequence");
				/* Error occured. Continue parsing after string is terminated. */
				while ((c = next(lexer)) != '"')
					;
				ignore(lexer);
				return STATE_FN(lex_any);
			}

			break;
		}
		default:
			str_builder_append_char(&sb, c);
		}
	}

	str_end = lexer->pos_in_line - 1;

	/* Handle multiline string */
	accept_run(lexer, " \t\v");
	if (match(lexer, '\\')) {
		accept_run(lexer, " \t\v");
		if (!match(lexer, '\n')) {
			ignore(lexer);
			report_lex_err(lexer, true, "Unexpected character after string continuiation");
			return STATE_FN(lex_any);
		}

		/*
		 * We just matched a newline, therefore me must increment the line_count.
		 * All tokens, but these multine strings span across one line and one line only.
		 * As such, the token type assumes each token is on one single line. For multline strings,
		 * this becomes the final line.
		 */
		lexer->line_count++;
		lexer->pos_in_line = 0;
		/* Ignore any identation before the next string */
		accept_run(lexer, " \t\v");
		lexer->start = lexer->pos;

		if (!match(lexer, str_type)) {
			lexer->pos++; // Gives an underline where the double qoute should be in the error msg
			report_lex_err(lexer, true, "Expected another string after '\'.");
			ignore(lexer);
			return STATE_FN(lex_any);
		}
		goto continue_str_lexing;
	}

	StrView str = str_builder_complete(&sb);
	Token token = { .type = t_dt_str,
					.lexeme = str,
					.line = lexer->line_count,
					.start = str_start,
					.end = str_end };
	arraylist_append(&lexer->tokens, &token);

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
		if (state.fn == NULL) {
			report_lex_err(lexer, false, "Expected end_state not reached");
			break;
		}
		state = state.fn(lexer);
	}
}

static void run(Lexer *lexer)
{
	StateFn start_state = lex_any(lexer);
	for (StateFn state = start_state; state.fn != NULL;)
		state = state.fn(lexer);
}

Lexer lex(Arena *arena, char *input, size_t input_size)
{
	Lexer lexer = { .had_error = false,
					.input = input,
					.input_size = input_size,
					.pos = 0,
					.start = 0,
					.line_count = 0,
					.pos_in_line = 0,
					.arena = arena };
	keywords_init(&lexer.keywords);
	arraylist_init(&lexer.tokens, sizeof(Token));

	run(&lexer);

	hashmap_free(&lexer.keywords);
	return lexer;
}
