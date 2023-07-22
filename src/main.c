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

#include "interpreter/ast.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#define SAC_IMPLEMENTATION
#include "sac/sac.h"
#define NICC_IMPLEMENTATION
#include "nicc/nicc.h"

#define MAX_INPUT_SIZE 4096


int main(int argc, char **argv)
{
    char *file_path = "src/test.slash";
    if (argc > 1)
	file_path = argv[1];

    char input[MAX_INPUT_SIZE];
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
	fprintf(stderr, "error opening file %s\n", file_path);
	return -1;
    }

    int c;
    size_t counter = 0;
    do {
	c = fgetc(fp);
	input[counter++] = c;
	if (counter == MAX_INPUT_SIZE)
	    break;
    } while (c != EOF);

    input[--counter] = 0;
    fclose(fp);

    /* lex */
    ArrayList tokens = lex(input, counter + 1);
#ifdef DEBUG
    tokens_print(&tokens);
#endif

    /* parse */
    Arena ast_arena;
    ast_arena_init(&ast_arena);
    ArrayList stmts = parse(&ast_arena, &tokens);
#ifdef DEBUG
    ast_print(&stmts);
#endif

    /* interpret */
#ifdef DEBUG
    printf("--- interpreter ---\n");
#endif
    interpret(&stmts);

    /* clean up */
    ast_arena_release(&ast_arena);
    arraylist_free(&tokens);
    arraylist_free(&stmts);
}
