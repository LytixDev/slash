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

#include "common.h"
#include "slash_str.h"
#include "slash_value.h"

static bool svt_str_truthy(SlashStr *str)
{
    return str->size != 0;
}

// soidev function
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

SlashValue slash_plus(SlashValue a, SlashValue b)
{
    // TODO: return error
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NULL };

    // TODO: temporary
    double *res = malloc(sizeof(double));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p + *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_NUM };
    // TODO: handle
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NULL };
    }
}

SlashValue slash_minus(SlashValue a, SlashValue b)
{
    // TODO: return error
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NULL };

    // TODO: temporary
    double *res = malloc(sizeof(double));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p - *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_NUM };
    // TODO: handle
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NULL };
    }
}

SlashValue slash_greater(SlashValue a, SlashValue b)
{
    // TODO: return error
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NULL };

    // TODO: temporary
    bool *res = malloc(sizeof(bool));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p > *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_BOOL };
    // TODO: handle
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NULL };
    }
}

SlashValue slash_equal(SlashValue a, SlashValue b)
{
    // TODO: return error
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NULL };

    // TODO: temporary
    bool *res = malloc(sizeof(bool));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p == *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_BOOL };
    // TODO: handle
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NULL };
    }
}

SlashValue slash_not_equal(SlashValue a, SlashValue b)
{
    // TODO: return error
    if (a.type != b.type)
	return (SlashValue){ .p = NULL, .type = SVT_NULL };

    // TODO: temporary
    bool *res = malloc(sizeof(bool));
    switch (a.type) {
    case SVT_NUM:
	*res = *(double *)a.p != *(double *)b.p;
	return (SlashValue){ .p = res, .type = SVT_BOOL };
    // TODO: handle
    default:
	return (SlashValue){ .p = NULL, .type = SVT_NULL };
    }
}
