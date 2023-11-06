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
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/gc.h"
#include "interpreter/scope.h"
#include "nicc/nicc.h"
#include "sac/sac.h"

#define STREAM_WRITE_END 1
#define STREAM_READ_END 0

typedef struct {
    int read_fd; // the file descriptor we are reading from, defaulted to fileno(STDIN)
    int write_fd; // the file descriptor we are writing to, defaulted to fileno(STDOUT)
    ArrayList active_fds; // list/stack of open file descriptors that need to be closed on fork()
} StreamCtx;

typedef struct interpreter_t {
    Arena arena;
    Scope globals;
    Scope *scope;
    GC gc;
    StreamCtx stream_ctx;
    int prev_exit_code;
} Interpreter;

void interpreter_init(Interpreter *interpreter, int argc, char **argv);
void interpreter_free(Interpreter *interpreter);
int interpreter_run(Interpreter *interpreter, ArrayList *statements);

int interpret(ArrayList *statements, int argc, char **argv);


#endif /* INTERPRETER_H */
