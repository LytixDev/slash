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


int exec_program(char **argv)
{
    int status;
    int return_code = 0;

    pid_t new_pid = fork();
    if (new_pid == 0)
	return_code = execve(argv[0], argv, NULL);

    waitpid(new_pid, &status, 0);
    return return_code;
}

int exec_capture(char **argv, char buffer[1024])
{
    int status;
    int return_code = 0;

    int pipefd[2];
    pipe(pipefd);

    pid_t new_pid = fork();
    if (new_pid == 0) {
	close(pipefd[0]);
	dup2(pipefd[1], 1);
	dup2(pipefd[1], 2);
	close(pipefd[1]);
	return_code = execve(argv[0], argv, NULL);
    }

    waitpid(new_pid, &status, 0);
    close(pipefd[1]);
    read(pipefd[0], buffer, 1024);
    return return_code;
}
