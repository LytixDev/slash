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
#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>

#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "options.h" // REPORT_FILE


#define REPORT_IMPL(...) fprintf(REPORT_FILE, __VA_ARGS__)

#define RUNTIME_ERROR 1

extern jmp_buf runtime_error_jmp;


void report_lex_err(Lexer *lexer, bool print_offending, char *msg);
void report_all_parse_errors(ParseError *head, char *full_input);
void report_parse_err(ParseError *error, char *full_input);

#ifdef DEBUG
#define REPORT_RUNTIME_ERROR(...)                                                               \
	do {                                                                                        \
		REPORT_IMPL("%s[Slash Runtime Error @ %s:%d]:%s ", ANSI_BOLD_START, __FILE__, __LINE__, \
					ANSI_BOLD_END);                                                             \
		REPORT_IMPL(__VA_ARGS__);                                                               \
		REPORT_IMPL("\n");                                                                      \
		longjmp(runtime_error_jmp, RUNTIME_ERROR);                                              \
		REPORT_IMPL("%d\n", (interpreter)->source_line);                                        \
	} while (0);
#else
#define REPORT_RUNTIME_ERROR(...)                                              \
	do {                                                                       \
		REPORT_IMPL("%s[Slash Runtime Error at line %d]:%s ", ANSI_BOLD_START, \
					(interpreter)->source_line, ANSI_BOLD_END);                \
		REPORT_IMPL(__VA_ARGS__);                                              \
		REPORT_IMPL("\n");                                                     \
		longjmp(runtime_error_jmp, RUNTIME_ERROR);                             \
	} while (0)
#endif /* DEBUG */

/*
 * Same as REPORT_RUNTIME_ERROR, but no interpreter access
 * NOTE: This is a botch.
 */
#define REPORT_RUNTIME_ERROR_OPAQUE(...)                                            \
	do {                                                                            \
		REPORT_IMPL("%s[Slash Runtime Error]:%s ", ANSI_BOLD_START, ANSI_BOLD_END); \
		REPORT_IMPL(__VA_ARGS__);                                                   \
		REPORT_IMPL("\n");                                                          \
		longjmp(runtime_error_jmp, RUNTIME_ERROR);                                  \
	} while (0)

#endif /* ERROR_H */
