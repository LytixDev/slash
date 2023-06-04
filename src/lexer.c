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

#include "lexer.h"
#define SAC_IMPLEMENTATION
#define SAC_TYPEDEF
#include "sac/sac.h"

char *keywords[t_false + 1] = {
    "var",
    "if",
    "true",
    "false",
};

void slash_str_println(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    printf("'%s'\n", str);
}

void emit(Lexer *lexer, TokenType type);

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

static char prev(Lexer *lexer)
{
    if (lexer->pos == 0)
	return -1;
    return lexer->input[lexer->pos - 1];
}

static void ignore(Lexer *lexer)
{
    lexer->start = lexer->pos;
}

static void backup(Lexer *lexer)
{
    // NOTE: should we give err?
    if (lexer->pos == 0)
	return;

    lexer->pos--;
}

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
    while (1) {
	if (!consume(lexer, accept))
	    return;
    }
}

/*
 * semantic
 */
bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\v';
}

bool is_numeric(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_valid_identifier(char c)
{
    return is_numeric(c) || is_alpha(c) || c == '_';
}


/*
 * state functions
 */
StateFn lex_any(Lexer *lexer);
StateFn lex_end(Lexer *lexer);
StateFn lex_number(Lexer *lexer);
StateFn lex_identifier(Lexer *lexer);
StateFn lex_string(Lexer *lexer);

StateFn lex_any(Lexer *lexer)
{
    while (1) {
	char c = next(lexer);
	switch (c) {
	case ' ':
	case '\t':
	    ignore(lexer);
	    break;

	case '\n':
	    emit(lexer, t_newline);
	    break;

	// TODO: lex_block state function
	case '{':
	    emit(lexer, t_lbrace);
	    break;
	case '}':
	    emit(lexer, t_rbrace);
	    break;

	case EOF:
	    return STATE_FN(lex_end);

	case '"':
	    return STATE_FN(lex_string);

	default:
	    // if (is_alpha(c)) {
	    //     backup(lexer);
	    //     return STATE_FN(lex_number);
	    // }

	    if (is_valid_identifier(c)) {
		backup(lexer);
		return STATE_FN(lex_identifier);
	    }

	    goto error;
	}
    }

error:
    printf("error\n");
    return STATE_FN(NULL);
}

StateFn lex_end(Lexer *lexer)
{
    emit(lexer, t_eof);
    return (StateFn){ .fn = NULL };
}

StateFn lex_number(Lexer *lexer);

StateFn lex_identifier(Lexer *lexer)
{
    while (is_valid_identifier(next(lexer)))
	;
    backup(lexer);
    // TODO resolve what identifier it is
    emit(lexer, t_identifier);
    return STATE_FN(lex_any);
}

StateFn lex_string(Lexer *lexer)
{
    /* ignore starting qoute */
    ignore(lexer);

    char c;
    while ((c = next(lexer)) != '"') {
	if (c == EOF)
	    return STATE_FN(NULL);
    }

    /* backup final qoute */
    backup(lexer);
    emit(lexer, t_str);
    /* advance past final qoute and ignore it */
    next(lexer);
    ignore(lexer);

    return STATE_FN(lex_any);
}


/*
 * fsm
 */
void emit(Lexer *lexer, TokenType type)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->lexeme =
	(SlashStr){ .p = lexer->input + lexer->start, .size = lexer->pos - lexer->start };
    darr_append(lexer->tokens, token);

    lexer->start = lexer->pos;
}

void run(Lexer *lexer)
{
    StateFn start_state = lex_any(lexer);
    for (StateFn state = start_state; state.fn != NULL;) {
	state = state.fn(lexer);
    }
}

struct darr_t *lex(char *input)
{
    struct darr_t *tokens = darr_malloc();
    Lexer lexer = { .input = input, .pos = 0, .start = 0, .tokens = tokens };
    run(&lexer);
    return lexer.tokens;
}

int main(void)
{
    // https://www.youtube.com/watch?v=HxaD_trXwRE
    char input[1024];
    FILE *fp = fopen("src/test.sh", "r");
    if (fp == NULL) {
	return -1;
    }

    int c;
    size_t i = 0;
    do {
	c = fgetc(fp);
	input[i++] = c;
	if (i == 1024)
	    break;
    } while (c != EOF);

    input[--i] = 0;


    // Arena lex_arena;
    // m_arena_init_dynamic(&lex_arena, SAC_DEFAULT_CAPACITY, SAC_DEFAULT_COMMIT_SIZE);

    struct darr_t *tokens = lex(input);
    for (i = 0; i < tokens->size; i++) {
	// printf("%s\n", ((Token *)darr_get(tokens, i))->lexeme);
	Token *token = darr_get(tokens, i);
	printf("[%zu] ", i);
	slash_str_println(token->lexeme);
    }

    // m_arena_release(&lex_arena);
}
