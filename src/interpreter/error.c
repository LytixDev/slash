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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "nicc/nicc.h"

typedef struct {
    char *alloced_buffer;
    size_t left_offset;
} ErrBuf;


static char *offending_line(char *src, size_t line)
{
    size_t count = 0;
    while (*src != 0) {
	if (*src == '\n')
	    count++;
	if (count == line)
	    return src;

	src++;
    }
    return NULL;
}

static ErrBuf offending_line_from_offset(char *input, size_t offset)
{
    /* first char after previous newline */
    char *start;
    for (start = input + offset; start - input >= 0; start--) {
	if (*start == '\n') {
	    start++;
	    break;
	}
    }
    if (start == input - 1)
	start++;

    /* first char before previous newline */
    char *end;
    for (end = input + offset; *end != EOF; end++) {
	if (*end == '\n') {
	    break;
	}
    }

    // TODO: we can define a max len just for safety
    assert(end > start);
    size_t len = end - start;
    char *buffer = malloc(sizeof(char) * (len + 1));
    memcpy(buffer, start, len);
    buffer[len] = 0;
    return (ErrBuf){ .alloced_buffer = buffer, .left_offset = (input + offset) - start };
}

void report_lex_err(Lexer *lexer, bool print_offending, char *msg)
{
    REPORT_IMPL("[line %zu]: %s\n", lexer->line_count + 1, msg);
    if (print_offending) {
	ErrBuf bf = offending_line_from_offset(lexer->input, lexer->start);
	REPORT_IMPL(">%s\n ", bf.alloced_buffer);
	free(bf.alloced_buffer);

	// TODO: a lot of wasted calls to REPORT_IMPL
	for (size_t i = 0; i < bf.left_offset; i++)
	    REPORT_IMPL(" ");
	for (size_t i = 0; i < lexer->pos - lexer->start; i++)
	    REPORT_IMPL("^");
	REPORT_IMPL("\n");
    }

    lexer->had_error = true;
}

void report_parse_err(Parser *parser, char *msg)
{
    Token *failed = arraylist_get(parser->tokens, parser->token_pos);
    REPORT_IMPL("[line %zu]: Error during parsing: %s\n", failed->line + 1, msg);

    char *line = offending_line(parser->input, failed->line);
    ErrBuf bf = offending_line_from_offset(line, failed->start);
    REPORT_IMPL(">%s\n ", bf.alloced_buffer);
    free(bf.alloced_buffer);

    for (size_t i = 0; i < failed->start; i++)
	REPORT_IMPL(" ");
    for (size_t i = 0; i < failed->end - failed->start; i++)
	REPORT_IMPL("^");
    REPORT_IMPL("\n");

    parser->had_error = true;
}

void report_runtime_error(char *msg)
{
    REPORT_IMPL("Error during runtime: %s\n", msg);
    exit(1);
}
