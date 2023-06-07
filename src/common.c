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
#include <stdio.h>
#include <stdlib.h>


void slash_exit_lex_err(char *err_msg)
{
    fprintf(stderr, "Error during lexing: %s\n", err_msg);
    exit(1);
}

void slash_exit_parse_err(char *err_msg)
{
    fprintf(stderr, "Error during parsing: %s\n", err_msg);
    exit(1);
}

void slash_exit_internal_err(char *err_msg)
{
    fprintf(stderr, "Internal error: %s\n", err_msg);
    exit(1);
}
