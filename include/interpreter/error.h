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

#include <assert.h>
#include <setjmp.h>
#include <stdio.h> // stderr

#include "interpreter/lexer.h"
#include "interpreter/parser.h"


#define ASSERT_NOT_REACHED assert(0 && "panic: unreachable code reached")

#ifndef REPORT_FILE
#define REPORT_FILE stderr
#endif

#define REPORT_IMPL(...) fprintf(REPORT_FILE, __VA_ARGS__)

#define RUNTIME_ERROR 1

extern jmp_buf runtime_error_jmp;


void report_lex_err(Lexer *lexer, bool print_offending, char *msg);
void report_parse_err(Parser *parser, char *msg);

#define REPORT_RUNTIME_ERROR(...)                  \
    do {                                           \
	REPORT_IMPL("[Slash Runtime Error]: ");    \
	REPORT_IMPL(__VA_ARGS__);                  \
	REPORT_IMPL("\n");                         \
	longjmp(runtime_error_jmp, RUNTIME_ERROR); \
    } while (0);

#endif /* ERROR_H */
