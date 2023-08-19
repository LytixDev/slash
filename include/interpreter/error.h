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
#ifndef ERROR_H
#define ERROR_H

#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include <assert.h>


#define ASSERT_NOT_REACHED assert(false && "panic: unreachable code reached")


void slash_exit_lex_err(char *err_msg);
void slash_exit_parse_err(Parser *parser, char *err_msg);
void slash_exit_interpreter_err(char *err_msg);
void slash_exit_internal_err(char *err_msg);

/*
 * Here the idea is that maybe we would at some point in the future like to report
 * errors somewhere else than straight to stderr.
 */
#ifndef REPORT_PRINT_IMPL
#define REPORT_PRINT_IMPL(format, ...) fprintf(stderr, (format), __VA_ARGS__)
#endif

#define REPORT_LEX_ERR(lexer, fmt, ...)                         \
    do {                                                        \
	REPORT_PRINT_IMPL("[line %zu]: ", (lexer)->line_count); \
	REPORT_PRINT_IMPL(fmt, __VA_ARGS__);                    \
	(lexer)->had_error = true;                              \
    } while (0)

#endif /* ERROR_H */
