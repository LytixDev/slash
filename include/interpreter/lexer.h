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
#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "interpreter/types/slash_str.h"
#include "nicc/nicc.h"

#define STATE_FN(___state_fn) \
    (StateFn)                 \
    {                         \
	.fn = ___state_fn     \
    }

typedef enum {
    /* keywords */
    t_var = 0,
    t_func,
    t_return,
    t_if,
    t_elif,
    t_else,
    t_loop,
    t_in,
    t_true,
    t_false,
    t_as,
    t_and,
    t_or,
    t_not,
    t_str,
    t_num,
    t_bool,
    t_none,

    /* single-character tokens */
    t_lparen,
    t_rparen,
    t_lbrace,
    t_rbrace,
    t_lbracket,
    t_rbracket,
    t_star,
    t_tilde,
    t_backslash,
    t_comma,
    t_colon,

    /* one or two character tokens */
    t_anp,
    t_anp_anp,
    t_equal,
    t_equal_equal,
    t_pipe,
    t_pipe_pipe,
    t_bang,
    t_bang_equal,
    t_greater,
    t_greater_equal,
    t_less,
    t_less_equal,
    t_dot,
    t_dot_dot,
    t_plus,
    t_plus_equal,
    t_minus,
    t_minus_equal,

    /* data types */
    dt_str,
    dt_num,
    dt_range,
    dt_bool,
    dt_shlit,

    /* */
    t_interpolation,
    t_identifier,
    t_newline,
    t_eof,
    t_error,
    t_enum_count,
} TokenType;

#define keywords_len (t_none + 1)

extern char *token_type_str_map[t_enum_count];

typedef struct {
    TokenType type;
    SlashStr lexeme;
} Token;

typedef struct {
    char *input; // the input string being scanned.
    size_t input_size;
    size_t start; // start position of this token.
    size_t pos; // current position in the input.
    ArrayList *tokens;
    struct hashmap_t *keywords;
} Lexer;


typedef struct func_wrap {
    struct func_wrap (*fn)(Lexer *);
} StateFn;


/* functions */
ArrayList lex(char *input, size_t input_size);

void tokens_print(ArrayList *tokens);


#endif /* LEXER_H */
