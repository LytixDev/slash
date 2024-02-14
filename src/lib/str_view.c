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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lib/str_view.h"


#define HEX_UNDERSCORE_IGNORE 16

static const int hex_lookup[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,	1,  2,	3,  4,	5,  6,	7,  8,	9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 16,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static double strview_binary_to_double(StrView a)
{
    char *bin_str = a.view;
    long result = 0;
    char ch;

    while ((size_t)(bin_str - a.view) < a.size) {
	ch = *bin_str++;
	if (ch == '_')
	    continue;
	if (!((ch == '0') | (ch == '1')))
	    break;
	result = (result << 1) | (ch - '0');
    }

    return (double)result;
}

static double strview_hex_to_double(StrView a)
{
    char *hex_str = a.view;
    double result = 0.0;
    int digit;

    while ((size_t)(hex_str - a.view) < a.size) {
	if ((digit = hex_lookup[(int)*hex_str++]) >= 0) {
	    if (digit == HEX_UNDERSCORE_IGNORE) {
		continue;
	    }
	    result = result * 16.0 + (double)digit;
	} else {
	    break;
	}
    }

    return result;
}

double str_view_to_double(StrView a)
{
    double result = 0.0;
    char *str = a.view;
    if (a.size == 0)
	return NAN;

    /* check sign */
    double sign = 1.0;
    if (*str == '-') {
	sign = -1.0;
	str++;
    } else if (*str == '+') {
	str++;
    }

    if ((size_t)(str - a.view) == a.size)
	return NAN;

    /* check optional base identifier */
    if (*str == '0') {
	if ((size_t)(str - a.view) + 2 > a.size)
	    goto strtod_base10;
	if (str[1] == 'X' || str[1] == 'x')
	    return sign * strview_hex_to_double(
			      (StrView){ .view = str + 2, .size = a.size - (str - a.view) - 2 });

	if (str[1] == 'b' || str[1] == 'B')
	    return sign * strview_binary_to_double(
			      (StrView){ .view = str + 2, .size = a.size - (str - a.view) - 2 });
    }

strtod_base10:
    while ((*str >= '0' && *str <= '9') || *str == '_') {
	if ((size_t)(str - a.view) == a.size)
	    goto strtod_done;
	/* ignore '_' part */
	if (*str == '_') {
	    str++;
	    continue;
	}
	result = result * 10.0 + (*str - '0');
	str++;
    }

    /* fraction */
    if (*str == '.') {
	str++;
	double fraction = 0.1;
	while (*str >= '0' && *str <= '9') {
	    if ((size_t)(str - a.view) == a.size)
		break;
	    result += (*str - '0') * fraction;
	    fraction *= 0.1;
	    str++;
	}
    }

strtod_done:
    return result * sign;
}

void str_view_print(StrView s)
{
    char str[s.size + 1];
    memcpy(str, s.view, s.size);
    str[s.size] = 0;
    printf("%s", str);
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

int str_view_cmp(StrView a, StrView b)
{
    char *A = a.view;
    char *B = b.view;
    while (*A && (*A == *B)) {
	A++;
	B++;
    }
    return *(const unsigned char *)A - *(const unsigned char *)B;
}

void str_view_to_cstr(StrView view, char *cstr)
{
    memcpy(cstr, view.view, view.size);
    cstr[view.size] = 0;
}

StrView str_view_arena_copy(Arena *arena, StrView to_copy)
{
    // TODO: since we are now copying we also have the opportunity to null terminate
    char *view_cpy = m_arena_alloc(arena, sizeof(char) * to_copy.size);
    memcpy(view_cpy, to_copy.view, to_copy.size);
    return (StrView){ .view = view_cpy, .size = to_copy.size };
}
