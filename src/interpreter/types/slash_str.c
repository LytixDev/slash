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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/types/slash_str.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


void slash_str_print(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    printf("%s", str);
}

void slash_str_println(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    printf("%s\n", str);
}

// TODO: add base2 support
double slash_str_to_double(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    return strtod(str, NULL);
}

int32_t slash_str_to_int(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    return atoi(str);
}

bool slash_str_eq(SlashStr a, SlashStr b)
{
    if (a.size != b.size)
	return false;

    char *A = a.p;
    char *B = b.p;
    for (size_t i = 0; i < a.size; i++) {
	if (A[i] != B[i])
	    return false;
    }

    return true;
}
