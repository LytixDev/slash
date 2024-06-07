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
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/error.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "nicc/nicc.h"
#include "options.h"


typedef struct {
    char *alloced_buffer;
    size_t left_offset;
} ErrBuf;

jmp_buf runtime_error_jmp = { 0 };


static char *offending_line(char *src, size_t line_no)
{
    if (line_no == 0)
	return src;

    size_t count = 0;
    while (*src != 0) {
	if (*src == '\n')
	    count++;
	if (count == line_no)
	    return src + 1;

	src++;
    }
    return NULL;
}

/*
 * Creates an ErrBuf where the buffer is the entire line where the error occured.
 */
static ErrBuf offending_line_from_offset(char *input_start, size_t offset)
{
    /* Edge case where the error was the last character in the input */
    if (*(input_start + offset + 1) == 0)
	offset--;
    char *line_start = input_start + offset;
    char *line_end = input_start + offset;
    for (; line_start != input_start; line_start--) {
	if (*line_start == '\n') {
	    line_start++;
	    break;
	}
    }
    for (; *line_end != EOF; line_end++) {
	if (*line_end == '\n') {
	    break;
	}
    }

    assert(line_end - line_start > 0);
    size_t len = line_end - line_start;
    if (len > ERROR_BUF_MAX_LEN)
	len = ERROR_BUF_MAX_LEN;
    char *buffer = malloc(sizeof(char) * (len + 1));
    memcpy(buffer, line_start, len);
    buffer[len] = 0;
    return (ErrBuf){ .alloced_buffer = buffer, .left_offset = (input_start + offset) - line_start };
}

static void err_buf_print(ErrBuf bf, size_t err_token_len)
{
    REPORT_IMPL(">%s\n ", bf.alloced_buffer);
    // NOTE(Nicolai): A lot of calls to REPORT_IMPL.
    //                Could be optimized, however reporting errors fast is not super critical.
    for (size_t i = 0; i < bf.left_offset; i++)
	REPORT_IMPL(" ");

    REPORT_IMPL(ANSI_COLOR_START_RED);
    for (size_t i = 0; i < err_token_len; i++)
	REPORT_IMPL("^");
    REPORT_IMPL(ANSI_COLOR_END);
    REPORT_IMPL("\n");
}

void report_lex_err(Lexer *lexer, bool print_offending, char *msg)
{
    lexer->had_error = true;
#ifdef DEBUG_LEXER
    REPORT_IMPL("Current token dump:\n");
    tokens_print(&lexer->tokens);
#endif /* DEBUG_LEXER */

    REPORT_IMPL("%s[line %zu]:%s %s\n", ANSI_BOLD_START, lexer->line_count + 1, ANSI_BOLD_END, msg);
    REPORT_IMPL(ANSI_BOLD_END);
    size_t err_token_len = lexer->pos - lexer->start;
    if (!print_offending || err_token_len < 1)
	return;

    ErrBuf bf = offending_line_from_offset(lexer->input, lexer->start);
    err_buf_print(bf, err_token_len);
    free(bf.alloced_buffer);
}

void report_all_parse_errors(ParseError *head, char *full_input)
{
    for (ParseError *error = head; error != NULL; error = error->next)
	report_parse_err(error, full_input);
}

void report_parse_err(ParseError *error, char *full_input)
{
    REPORT_IMPL("%s[line %zu]%s: Error during parsing: %s\n", ANSI_BOLD_START,
		error->failed->line + 1, ANSI_BOLD_END, error->msg);

    char *line = offending_line(full_input, error->failed->line);
    if (line == NULL) {
	REPORT_IMPL("Internal error: could not find line where parse error occured");
	return;
    }

    ErrBuf bf = offending_line_from_offset(line, error->failed->start);
    err_buf_print(bf, error->failed->end - error->failed->start);
    free(bf.alloced_buffer);
}
