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
#include <stdio.h>

#include "common.h"
#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "interpreter/lexer.h" //TODO: do not use types from the lexer. Tight coupling is le bad!!


/* following are the three musketeers of soydev functions */
static bool svt_str_truthy(SlashStr *str)
{
    return str->size != 0;
}

static bool svt_bool_truthy(bool *p)
{
    return *p == true;
}

static bool svt_num_truthy(double *p)
{
    return *p != 0;
}

bool is_truthy(SlashValue *value)
{
    switch (value->type) {
    case SVT_STR:
	return svt_str_truthy(value->p);

    case SVT_BOOL:
	return svt_bool_truthy(value->p);

    case SVT_NUM:
	return svt_num_truthy(value->p);

    default:
	return false;
    }
}


// TODO: all of the following functions need reworking.
//       this is just temporary to get things going
//       turbo ugly
static SlashValue slash_value_plus(SlashValue a, SlashValue b)
{
    double *res = malloc(sizeof(double));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p + *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_NUM };
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NONE };
    }
}

static SlashValue slash_value_minus(SlashValue a, SlashValue b)
{
    double *res = malloc(sizeof(double));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p - *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_NUM };
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NONE };
    }
}

static SlashValue slash_value_greater(SlashValue a, SlashValue b)
{
    bool *res = malloc(sizeof(bool));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p > *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_BOOL };
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NONE };
    }
}

static SlashValue slash_value_equal(SlashValue a, SlashValue b)
{
    bool *res = malloc(sizeof(bool));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p == *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_BOOL };
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NONE };
    }
}

static SlashValue slash_value_not_equal(SlashValue a, SlashValue b)
{
    SlashValue value = slash_value_equal(a, b);
    if (value.p == NULL)
	return value;

    // TODO: lol
    bool *v = value.p;
    *v = !*v;
    *(bool *)value.p = *v;
    return value;
}

SlashValue slash_value_cmp(SlashValue a, SlashValue b, TokenType operator)
{
    // is this too conservative?
    // TODO: better error handling in cases like this
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NONE };

    if (operator== t_plus)
	return slash_value_plus(a, b);
    if (operator== t_minus)
	return slash_value_minus(a, b);
    if (operator== t_greater)
	return slash_value_greater(a, b);
    if (operator== t_equal_equal)
	return slash_value_equal(a, b);
    if (operator== t_bang_equal)
	return slash_value_not_equal(a, b);

    fprintf(stderr, "operator not supported, returning SVT_NONE");
    return (SlashValue){ .p = NULL, .type = SVT_NONE };
}

void slash_value_println(SlashValue sv)
{
    switch (sv.type) {
    case SVT_STR:
    case SVT_SHLIT:
	slash_str_println(*(SlashStr *)sv.p);
	break;

    case SVT_BOOL:
	printf("%s\n", *(bool *)sv.p ? "true" : "false");
	break;

    case SVT_NUM:
	printf("%f\n", *(double *)sv.p);
	break;

    default:
	break;
    }
}
