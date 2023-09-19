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
#include <sys/stat.h>
#ifdef DEBUG_PERF
#include <time.h>
#endif /* DEBUG_PERF */

#include "interpreter/ast.h"
#include "interpreter/error.h"
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
    int exit_code;
    char *file_path = "src/test.slash";
    if (argc > 1)
	file_path = argv[1];

    struct stat st;
    if (stat(file_path, &st) != 0) {
	REPORT_IMPL("Could not stat file '%s'\n", file_path);
	return 1;
    }
    size_t file_size = st.st_size;

    char *input = malloc(sizeof(char) * (file_size + 1));
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
	REPORT_IMPL("Could not open file '%s'\n", file_path);
	exit_code = 1;
	goto defer_input;
    }
    if (fread(input, sizeof(char), st.st_size, fp) != file_size) {
	REPORT_IMPL("Could not read file '%s'\n", file_path);
	exit_code = 1;
	goto defer_input;
    }
    input[file_size] = 0;

#ifdef DEBUG_PERF
    double lex_elapsed, parse_elapsed, interpret_elapsed;
    clock_t start_time, end_time;
    start_time = clock();
#endif /* DEBUG_PERF */

    /* lex */
    Lexer lex_result = lex(input, file_size);
    if (lex_result.had_error) {
	exit_code = 1;
	goto defer_tokens;
    }

#ifdef DEBUG_PERF
    end_time = clock();
    lex_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
#endif /* DEBUG_PERF */
#ifdef DEBUG
    tokens_print(&lex_result.tokens);
#endif /* DEBUG */

#ifdef DEBUG_PERF
    start_time = clock();
#endif /* DEBUG_PERF */

    /* parse */
    Arena ast_arena;
    ast_arena_init(&ast_arena);
    StmtsOrErr stmts = parse(&ast_arena, &lex_result.tokens, input);
    if (stmts.had_error) {
	exit_code = 1;
	goto defer_stms;
    }

#ifdef DEBUG_PERF
    end_time = clock();
    parse_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
#endif /* DEBUG_PERF */
#ifdef DEBUG
    ast_print(&stmts.stmts);
#endif /* DEBUG */

#ifdef DEBUG
    printf("--- interpreter ---\n");
#endif /* DEBUG */
#ifdef DEBUG_PERF
    start_time = clock();
#endif /* DEBUG_PERF */

    /* interpret */
    exit_code = interpret(&stmts.stmts);

#ifdef DEBUG_PERF
    end_time = clock();
    interpret_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("--- perf ---\n");
    printf("lex time:\t%f seconds.\n", lex_elapsed);
    printf("parse time:\t%f seconds.\n", parse_elapsed);
    printf("interpret time:\t%f seconds.\n", interpret_elapsed);
    printf("lex+parse:\t%11.1fK l/s.\n",
	   ((lex_result.line_count + 1) / (lex_elapsed + parse_elapsed)) / 1000);
#endif /* DEBUG_PERF */

    /* clean up */
defer_stms:
    ast_arena_release(&ast_arena);
    arraylist_free(&stmts.stmts);
defer_tokens:
    arraylist_free(&lex_result.tokens);
defer_input:
    free(input);
    fclose(fp);

    return exit_code;
}
