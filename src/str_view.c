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
#include <string.h>

#include "str_view.h"


void str_view_print(StrView s)
{
    char str[s.size + 1];
    memcpy(str, s.view, s.size);
    str[s.size] = 0;
    printf("%s", str);
}

void str_view_println(StrView s)
{
    char str[s.size + 1];
    memcpy(str, s.view, s.size);
    str[s.size] = 0;
    printf("%s\n", str);
}

// TODO: add base2 support
double str_view_to_double(StrView s)
{
    char str[s.size + 1];
    memcpy(str, s.view, s.size);
    str[s.size] = 0;
    return strtod(str, NULL);
}

int32_t str_view_to_int(StrView s)
{
    char str[s.size + 1];
    memcpy(str, s.view, s.size);
    str[s.size] = 0;
    return atoi(str);
}

bool str_view_eq(StrView a, StrView b)
{
    if (a.size != b.size)
	return false;

    char *A = a.view;
    char *B = b.view;
    for (size_t i = 0; i < a.size; i++) {
	if (A[i] != B[i])
	    return false;
    }

    return true;
}
