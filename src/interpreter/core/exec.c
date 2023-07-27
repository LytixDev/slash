/*
 *  Copyright (C) 2022-2023 Nicolai Brand, originally a part of the Valery project.
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
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "interpreter/interpreter.h"
#include "nicc/nicc.h"

extern char **environ;


static void close_active_fds(ArrayList *active_fds)
{
    for (size_t i = 0; i < active_fds->size; i++) {
	int *fd = arraylist_get(active_fds, i);
	close(*fd);
    }
}

int exec_program(StreamCtx *stream_ctx, char **argv)
{
#ifdef EXEC_DEBUG
#include <stdio.h>
    char **argv_cpy = argv;
    for (char *argv_cur = *argv_cpy; argv_cur != NULL;) {
	printf("'%s'\n", argv_cur);
	argv_cpy++;
	argv_cur = *argv_cpy;
    }
#endif /* EXEC_DEBUG */

    int status;
    int return_code = 0;

    pid_t new_pid = fork();
    if (new_pid == 0) {
	if (stream_ctx->read_fd != STDIN_FILENO) {
	    dup2(stream_ctx->read_fd, STDIN_FILENO);
	}
	if (stream_ctx->write_fd != STDOUT_FILENO) {
	    dup2(stream_ctx->write_fd, STDOUT_FILENO);
	}

	close_active_fds(&stream_ctx->active_fds);
	return_code = execve(argv[0], argv, environ);
    }

    close_active_fds(&stream_ctx->active_fds);
    waitpid(new_pid, &status, 0);
    return return_code;
}
