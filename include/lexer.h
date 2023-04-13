#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#define NICC_DARR_IMPLEMENTATION
#include "nicc/nicc.h"

typedef enum {
    /* keywords */
    t_var,
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
    t_str,
    t_num,
    t_bool,

    /* single-character tokens */
    t_lparen,
    t_rparen,
    t_lbrace,
    t_rbrace,
    t_star,
    t_dollar,

    /* one or two character tokens */
    t_anp,
    // ...

    /* data types */
    dt_str,
    dt_num,
    dt_range,
    dt_bool,
    dt_word,

    t_identifier,
    t_error,
    t_eof,
    t_enum_count,
} TokenType;

typedef struct {
    TokenType type;
    union {
        char *lexeme;
        char *error;
    };
} Token;

typedef struct {
    char *input;        // the input string being scanned.
    size_t start;       // start position of this token.
    size_t pos;         // current position in the input.
    struct darr_t *tokens;
} Lexer;


typedef struct func_wrap {
    struct func_wrap (*fn)(Lexer *);
} StateFn;



#endif /* LEXER_H */
