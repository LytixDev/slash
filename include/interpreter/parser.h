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

#include "nicc/nicc.h"
#include "sac/sac.h"

/* types */
typedef struct {
    Arena *ast_arena; // memory arena to put the all AST nodes on
    ArrayList *tokens;
    size_t token_pos; // index of current token being processed
} Parser;

/*
 * parses a list of tokens into a list of Stmt's.
 * the Stmt objects in the list are the first nodes in an AST.
 */
ArrayList parse(Arena *ast_arena, ArrayList *tokens);


#endif /* PARSER_H */
