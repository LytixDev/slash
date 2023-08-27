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

#include "nicc/nicc.h"
#include "str_view.h"

#define STATE_FN(___state_fn) \
    (StateFn)                 \
    {                         \
	.fn = ___state_fn     \
    }

/*
 * X macro for the token types.
 * All thats needed to sucesfully add a new token type is to add it somewhere on this list.
 * The identifiers for the TokenType enum will be t_`token` where `token` is the X macro param.
 *
 * hmm, is this too much voodoo?
 */
#define KEYWORD_TOKENS \
    X(var)             \
    X(func)            \
    X(return)          \
    X(if)              \
    X(elif)            \
    X(else)            \
    X(loop)            \
    X(in)              \
    X(true)            \
    X(false)           \
    X(as)              \
    X(and)             \
    X(or)              \
    X(not )            \
    X(str)             \
    X(num)             \
    X(bool)            \
    X(none)            \
    X(assert)          \
    X(break)           \
    X(continue)
#define SINGLE_CHAR_TOKENS \
    X(lparen)              \
    X(rparen)              \
    X(lbrace)              \
    X(rbrace)              \
    X(lbracket)            \
    X(rbracket)            \
    X(tilde)               \
    X(backslash)           \
    X(comma)               \
    X(colon)               \
    X(semicolon)           \
    X(qoute)
#define ONE_OR_TWO_CHAR_TOKENS \
    X(anp)                     \
    X(anp_anp)                 \
    X(equal)                   \
    X(equal_equal)             \
    X(pipe)                    \
    X(pipe_pipe)               \
    X(bang)                    \
    X(bang_equal)              \
    X(greater)                 \
    X(greater_equal)           \
    X(less)                    \
    X(less_equal)              \
    X(dot)                     \
    X(dot_dot)                 \
    X(plus)                    \
    X(plus_equal)              \
    X(minus)                   \
    X(minus_equal)             \
    X(at)                      \
    X(at_lbracket)             \
    X(slash)                   \
    X(slash_slash)             \
    X(slash_equal)             \
    X(slash_slash_equal)       \
    X(star)                    \
    X(star_star)               \
    X(star_equal)              \
    X(star_star_equal)         \
    X(percent)                 \
    X(percent_equal)
#define DATA_TYPE_TOKENS \
    X(dt_str)            \
    X(dt_num) X(dt_range) X(dt_bool) X(dt_shident) X(dt_list) X(dt_tuple) X(dt_map) X(dt_none)
#define REST_TOKENS \
    X(access)       \
    X(ident)        \
    X(newline)      \
    X(eof)          \
    X(error)

#define SLASH_ALL_TOKENS   \
    KEYWORD_TOKENS         \
    SINGLE_CHAR_TOKENS     \
    ONE_OR_TWO_CHAR_TOKENS \
    DATA_TYPE_TOKENS       \
    REST_TOKENS

#define X(token) t_##token,
typedef enum {
    SLASH_ALL_TOKENS t_enum_count,
} TokenType;
#undef X

extern char *token_type_str_map[t_enum_count];

typedef struct {
    TokenType type;
    StrView lexeme;
    size_t line;
    size_t start; // position in line of first char of lexeme
    size_t end; // position in line of final char of lexeme
} Token;

typedef struct {
    bool had_error;
    char *input; // the input string being scanned.
    size_t input_size; // size of input in bytes.
    size_t start; // start position of this token.
    size_t pos; // current position in the input.

    size_t line_count; // what line in the input we are on.
    size_t pos_in_line;

    ArrayList tokens;
    HashMap keywords;
} Lexer;


typedef struct func_wrap {
    struct func_wrap (*fn)(Lexer *);
} StateFn;


/* functions */
Lexer lex(char *input, size_t input_size);

void tokens_print(ArrayList *tokens);


#endif /* LEXER_H */
