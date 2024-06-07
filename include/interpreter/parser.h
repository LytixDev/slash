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
#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "interpreter/lexer.h"
#include "nicc/nicc.h"
#include "sac/sac.h"


/* types */
typedef struct parse_error_t ParseError;
struct parse_error_t {
    char *msg;
    Token *failed; // the token that caused the error
    ParseError *next;
};

typedef struct {
    Arena *ast_arena; // memory arena to put where all AST nodes live on
    ArrayList *tokens; // stream of tokens from the lexer
    size_t token_pos; // index of current token being processed
    char *input; // handle to the source code
    int source_line;
    size_t n_errors;
    ParseError *perr_head; // linked list of parse errors
    ParseError *perr_tail; // final parse error node
} Parser;

typedef struct {
    size_t n_errors;
    ParseError *perr_head;
    ArrayList stmts;
} ParseResult;

/*
 * Parses a list of tokens into a list of statements: Arraylist<Stmt>
 * The Stmt objects in the list are the first nodes in an AST.
 */
ParseResult parse(Arena *ast_arena, ArrayList *tokens, char *input);


#endif /* PARSER_H */
