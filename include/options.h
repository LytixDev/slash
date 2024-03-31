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

/* GC opts */
#define GC_HEAP_GROW_FACTOR 2
#define GC_MIN_RUN (2 << 24) // ̃~32mb

/* Misc. */
#define PROGRAM_PATH_MAX_LEN 512


/* Maintenance */
#define ASSERT_NOT_REACHED assert(0 && "panic: unreachable code reached")
#define TODO(txt) assert(0 && (txt))
#define TODO_LOG(txt) fprintf(stderr, "[TODO]: %s\n", txt)


#endif /* OPTIONS_H */
