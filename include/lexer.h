#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#define NICC_DARR_IMPLEMENTATION
#include "nicc/nicc.h"

#define STATE_FN(___state_fn) \
    (StateFn)                 \
    {                         \
	.fn = ___state_fn     \
    }

typedef enum {
    /* keywords */
    t_var = 0,
    t_if,
    // t_elif,
    // t_else,
    // t_loop,
    // t_in,
    t_true,
    t_false,
    // t_as,
    // t_and,
    // t_or,

    t_str,
    // t_num,
    // t_bool,

    /* single-character tokens */
    // t_lparen,
    // t_rparen,
    t_lbrace,
    t_rbrace,
    // t_star,
    // t_dollar,
    t_newline,

    /* one or two character tokens */
    // t_anp,
    //  ...

    /* data types */
    // dt_str,
    // dt_num,
    // dt_range,
    // dt_bool,
    // dt_shlit,

    t_identifier,
    t_eof,
    t_error,
    t_enum_count,
} TokenType;

extern char *keywords[t_false + 1];

/* arena string */
typedef struct {
    char *p;
    size_t size;
} SlashStr;

typedef struct {
    TokenType type;
    SlashStr lexeme;
    // union {
    //     char *lexeme;
    //     char *error;
    // };
} Token;

typedef struct {
    char *input; // the input string being scanned.
    size_t start; // start position of this token.
    size_t pos; // current position in the input.
    struct darr_t *tokens;
} Lexer;


typedef struct func_wrap {
    struct func_wrap (*fn)(Lexer *);
} StateFn;


#endif /* LEXER_H */
