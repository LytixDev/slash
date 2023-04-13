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

// temporary as f
static char *str_slice(char *s, size_t start, size_t end)
{
    size_t len = end - start + 2;
    char *str = malloc(sizeof(char) * len);

    memcpy(str, s + start, len - 1);
    str[len - 1] = 0;

    return str;
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
    //NOTE: should we give err?
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

static void consumeRun(Lexer *lexer, char *accept)
{
    while (1) {
        if (!consume(lexer, accept))
            return;
    }
}


/* 
 * state functions 
 */

static void *lex_error(Lexer *lexer, char *err_str)
{
    Token *token = malloc(sizeof(Token));
    token->type = t_error;
    token->error = err_str;
    darr_append(lexer->tokens, token);
    return NULL;
}

StateFn lex_any(Lexer *lexer)
{
    return (StateFn){ .fn = NULL};
}

StateFn lex_number(Lexer *lexer); 

StateFn lex_identifier(Lexer *lexer);

StateFn lex_string(Lexer *lexer);

/* 
 * fsm
 */

void emit(Lexer *lexer, TokenType type)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->lexeme = str_slice(lexer->input, lexer->start, lexer->pos);
    darr_append(lexer->tokens, token);

    lexer->start = lexer->pos;
}

void run(Lexer *lexer)
{
    StateFn start_state = lex_any(lexer);
    for (StateFn state = start_state; state.fn != NULL; ) {
        state = state.fn(lexer);
    }

//https://www.youtube.com/watch?v=HxaD_trXwRE

}

//list<Token> *lex(char *input)
struct darr_t *lex(char *input)
{
    struct darr_t *tokens = darr_malloc();
    Lexer lexer = { .input = input, .pos = 0, .start = 0, .tokens = tokens };
    run(&lexer);
    return lexer.tokens;
}

int main(void)
{
    char *input = "if true { echo \"Hello\" }";
    struct darr_t *tokens = lex(input);
    for (size_t i = 0; i< tokens->size; i++) {
        printf("%s\n", ((Token *)darr_get(tokens, i))->lexeme);

    }
}
