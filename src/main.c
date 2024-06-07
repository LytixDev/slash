/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef DEBUG_PERF
#include <time.h>
#endif /* DEBUG_PERF */

#include "interactive/prompt.h"
#include "interpreter/ast.h"
#include "interpreter/error.h"
#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#define SAC_IMPLEMENTATION
#include "sac/sac.h"
#define NICC_IMPLEMENTATION
#include "nicc/nicc.h"


void interactive(int argc, char **argv)
{
    Interpreter interpreter = { 0 };
    interpreter_init(&interpreter, argc, argv);
    Arena ast_arena;
    ast_arena_init(&ast_arena);
    Prompt prompt;
    prompt_init(&prompt, "-> ");
    bool inside_block = false;

    while (true) {
	prompt_run(&prompt, inside_block);

	Lexer lex_result = lex(&ast_arena, prompt.buf, prompt.buf_len);
	if (lex_result.had_error) {
	    arraylist_free(&lex_result.tokens);
	    m_arena_clear(&ast_arena);
	    continue;
	}

	ParseResult parse_result = parse(&ast_arena, &lex_result.tokens, prompt.buf);
	if (parse_result.n_errors == 0) {
	    interpreter_run(&interpreter, &parse_result.stmts);
	} else if ((parse_result.n_errors == 1 || inside_block) &&
		   parse_result.perr_tail->err_type == PET_EXPECTED_RBRACE) {
            /* */
	    prompt_set_ps1(&prompt, ".. ");
	    inside_block = true;
	    prompt.buf[--prompt.buf_len] = 0; /* Pop EOF. We are continuing. */

	    arraylist_free(&lex_result.tokens);
	    arraylist_free(&parse_result.stmts);
	    m_arena_clear(&ast_arena);

	    continue;
	} else {
	    report_all_parse_errors(parse_result.perr_head, prompt.buf);
	}

	if (inside_block) {
	    prompt_set_ps1(&prompt, "-> ");
	    inside_block = false;
	}

	// TODO: these should maybe be reset (e.i. set size to 0), not freed
	arraylist_free(&lex_result.tokens);
	arraylist_free(&parse_result.stmts);
	m_arena_clear(&ast_arena);
    }

    ast_arena_release(&ast_arena);
    interpreter_free(&interpreter);
    prompt_free(&prompt);
}


int main(int argc, char **argv)
{
    if (argc == 1) {
	interactive(argc, argv);
	return 0;
    }

    int exit_code;
    size_t input_size;
    char *input;

    /* -c flag executes next argv as source code */
    if (strcmp(argv[1], "-c") == 0) {
	if (argc == 2) {
	    REPORT_IMPL("Argument expected for the -c flag\n");
	    return 2;
	}

	input_size = strlen(argv[2]);
	input = malloc(sizeof(char) * (input_size + 2));
	memcpy(input, argv[2], input_size);
	input[input_size] = '\n';
	input[input_size + 1] = 0;
    } else {
	/* Treat next agument as a filename */
	char *file_path = argv[1];

	struct stat st;
	if (stat(file_path, &st) != 0) {
	    REPORT_IMPL("Could not stat file '%s'\n", file_path);
	    return 1;
	}
	input_size = st.st_size;
	input = malloc(sizeof(char) * (input_size + 1));
	input[input_size] = 0;
	FILE *fp = fopen(file_path, "r");
	if (fp == NULL) {
	    REPORT_IMPL("Could not open file '%s'\n", file_path);
	    exit_code = 1;
	    goto defer_input;
	}
	if (fread(input, sizeof(char), st.st_size, fp) != input_size) {
	    REPORT_IMPL("Could not read file '%s'\n", file_path);
	    exit_code = 1;
	    goto defer_input;
	}
	fclose(fp);
    }


#ifdef DEBUG_PERF
    double lex_elapsed, parse_elapsed, interpret_elapsed;
    clock_t start_time, end_time;
    start_time = clock();
#endif /* DEBUG_PERF */

    /* lex */
    Arena ast_arena;
    ast_arena_init(&ast_arena);
    Lexer lex_result = lex(&ast_arena, input, input_size);
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
    ParseResult parse_result = parse(&ast_arena, &lex_result.tokens, input);
    if (parse_result.n_errors != 0) {
	report_all_parse_errors(parse_result.perr_head, input);
	exit_code = 1;
	goto defer_stms;
    }

#ifdef DEBUG_PERF
    end_time = clock();
    parse_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
#endif /* DEBUG_PERF */
#ifdef DEBUG
    ast_print(&parse_result.stmts);
#endif /* DEBUG */

#ifdef DEBUG
    printf("--- interpreter ---\n");
#endif /* DEBUG */
#ifdef DEBUG_PERF
    start_time = clock();
#endif /* DEBUG_PERF */

    /* interpret */
    exit_code = interpret(&parse_result.stmts, argc - 1, argv + 1);

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
    arraylist_free(&parse_result.stmts);
defer_tokens:
    ast_arena_release(&ast_arena);
    arraylist_free(&lex_result.tokens);

defer_input:
    free(input);

    return exit_code;
}
