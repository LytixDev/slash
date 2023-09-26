/*
 *  (Originally code for the Valery project)
 *  Copyright (C) 2022-2023 Nicolai Brand (https://lytix.dev)
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
#ifndef PROMPT_H
#define PROMPT_H

#include <stdio.h>
#include <termios.h>


typedef struct {
    char *buf;
    size_t buf_len;
    size_t buf_cap;
    size_t cursor_pos;
    char ps1[256];
    struct termios termios_og;
    struct termios termios_new;
} Prompt;


void prompt_init(Prompt *prompt, char *ps1);
void prompt_free(Prompt *prompt);

void prompt_run(Prompt *prompt);
void prompt_reset(Prompt *prompt);


#endif /* PROMPT_H */
