#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "nicc/nicc.h"
#include "slash_str.h"

#define STATE_FN(___state_fn) \
    (StateFn)                 \
    {                         \
	.fn = ___state_fn     \
    }

typedef enum {
    /* keywords */
    t_var = 0,
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
    // t_star,
    t_dollar,

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

#define keywords_len (t_bool + 1)

extern char *token_type_str_map[t_enum_count];

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
    bool command_mode;
    struct darr_t *tokens;
    struct hashmap_t *keywords;
} Lexer;


typedef struct func_wrap {
    struct func_wrap (*fn)(Lexer *);
} StateFn;


/* functions */
struct darr_t *lex(char *input);

void tokens_print(struct darr_t *tokens);


#endif /* LEXER_H */
