/*
 *  Copyright (C) 2024 Nicolai Brand (https://lytix.dev)
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
#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * This file contains global options/flags and macros.
 */

#include <assert.h>
#include <stdio.h>

/* Debug options */
// #define DEBUG_LEXER
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

/* Error messageing options */
#ifndef REPORT_FILE
#define REPORT_FILE stderr
#endif
#define ERROR_BUF_MAX_LEN 2048 // Max characters to print of a line

/* Parser options */
#define MAX_PARSE_ERRORS 64

/* GC options */
#define GC_HEAP_GROW_FACTOR 2
#define GC_MIN_RUN (2 << 24) // ̃~32mb

/* Misc. */
#define PROGRAM_PATH_MAX_LEN 512


/* Maintenance */
#define ASSERT_NOT_REACHED assert(0 && "panic: unreachable code reached")
#define TODO(txt) assert(0 && (txt))
#define TODO_LOG(txt) fprintf(stderr, "[TODO] @ %s:%d: %s\n", __FILE__, __LINE__, txt)

/* ANSI codes */
#define ANSI_COLOR_START_BLACK "\033[30m"
#define ANSI_COLOR_START_RED "\033[31m"
#define ANSI_COLOR_START_GREEN "\033[32m"
#define ANSI_COLOR_START_YELLOW "\033[33m"
#define ANSI_COLOR_START_BLUE "\033[34m"
#define ANSI_COLOR_START_MAGENTA "\033[35m"
#define ANSI_COLOR_START_CYAN "\033[36m"
#define ANSI_COLOR_START_WHITE "\033[37m"
#define ANSI_COLOR_START_DEFAULT "\033[39m"
#define ANSI_COLOR_END "\033[0m"
#define ANSI_BOLD_START "\033[1m"
#define ANSI_BOLD_END "\033[22m"


#endif /* OPTIONS_H */
