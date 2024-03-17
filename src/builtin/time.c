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

#include <stdio.h>
#include <sys/resource.h>
#include <time.h>

#include "interpreter/exec.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"


int builtin_time(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    struct rusage start_usage;
    getrusage(RUSAGE_SELF, &start_usage);
    clock_t start_time = clock();

    char *exec_argv[argc + 1]; // + 1 because last element is NULL
    exec_argv[argc] = NULL;
    gc_barrier_start(&interpreter->gc);

    for (size_t i = 0; i < argc; i++) {
	SlashValue arg = argv[i];
	VERIFY_TRAIT_IMPL(to_str, arg, "Could not take 'to_str' of type '%s'", arg.T->name);
	SlashValue value_str_repr = arg.T->to_str(interpreter, arg);
	exec_argv[i] = AS_STR(value_str_repr)->str;
    }

    gc_barrier_end(&interpreter->gc);
    int exit_code = exec_program(&interpreter->stream_ctx, exec_argv);

    struct rusage end_usage;
    getrusage(RUSAGE_CHILDREN, &end_usage);
    clock_t end_time = clock();

    double real_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    double user_time = (double)(end_usage.ru_utime.tv_sec + end_usage.ru_utime.tv_usec / 1000000.0);
    double sys_time = (double)(end_usage.ru_stime.tv_sec + end_usage.ru_stime.tv_usec / 1000000.0);
    printf("real\t%.3f\n", real_time);
    printf("user\t%.3f\n", user_time);
    printf("sys\t%.3f\n", sys_time);

    return exit_code;
}
