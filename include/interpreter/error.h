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


#define ASSERT_NOT_REACHED assert(0 && "panic: unreachable code reached")

/*
 * Here the idea is that maybe we would at some point in the future like to report
 * errors somewhere else than straight to stderr.
 */
#ifndef REPORT_IMPL
#define REPORT_IMPL(...) fprintf(stderr, __VA_ARGS__)
#endif

void slash_exit_interpreter_err(char *err_msg);
void slash_exit_internal_err(char *err_msg);

void report_lex_err(Lexer *lexer, bool print_offending, char *msg);
void report_parse_err(Parser *parser, char *msg);

#endif /* ERROR_H */
