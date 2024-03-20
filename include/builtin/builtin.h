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
#ifndef BUILTIN_H
#define BUILTIN_H

#include "interpreter/interpreter.h"
#include "lib/arena_ll.h"

#define PROGRAM_PATH_MAX_LEN 512


typedef int (*BuiltinFunc)(Interpreter *interpreter, ArenaLL *ast_nodes);

typedef struct {
    char *name;
    BuiltinFunc func;
} Builtin;

typedef enum {
    WHICH_BUILTIN,
    WHICH_EXTERN,
    WHICH_NOT_FOUND,
} WhichResultType;

typedef struct {
    WhichResultType type;
    union {
	char path[PROGRAM_PATH_MAX_LEN];
	BuiltinFunc builtin;
    };
} WhichResult;


WhichResult which(StrView cmd, char *PATH);
int builtin_which(Interpreter *interpreter, ArenaLL *ast_nodes);
// int builtin_cd(Interpreter *interpreter, size_t argc, SlashValue *argv);
// int builtin_vars(Interpreter *interpreter, size_t argc, SlashValue *argv);
// int builtin_exit(Interpreter *interpreter, size_t argc, SlashValue *argv);
// int builtin_read(Interpreter *interpreter, size_t argc, SlashValue *argv);
// int builtin_dot(Interpreter *interpreter, size_t argc, SlashValue *argv);
int builtin_time(Interpreter *interpreter, ArenaLL *ast_nodes);


#endif /* BUILTIN_H */
