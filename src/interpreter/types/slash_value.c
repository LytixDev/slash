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

#include "interpreter/error.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_value.h"
#include "lib/str_view.h"
#include "sac/sac.h"

SlashValue slash_glob_none = { .type = SLASH_NONE };

char *slash_type_names[SLASH_TYPE_COUNT] = { "bool", "num", "shident", "range", "obj", "none" };

char *slash_obj_type_names[] = { "list", "tuple", "map", "str" };

int slash_cmp_precedence[SLASH_TYPE_COUNT] = {
    /* bool */
    0,
    /* num */
    1,
    /* shident */
    __INT_MAX__,
    /* range */
    __INT_MAX__,
    /* obj */
    __INT_MAX__,
    /* none */
    __INT_MAX__,
};


bool is_truthy(SlashValue *value)
{
    switch (value->type) {
    case SLASH_SHIDENT:
	return value->shident.size != 0;
    case SLASH_NUM:
	return value->num != 0;
    case SLASH_BOOL:
	return value->boolean;
    case SLASH_NONE:
	return false;
    case SLASH_OBJ:
	return value->obj->traits->truthy(value);

    default:
	fprintf(stderr, "truthy not defined for this type, returning false");
    }

    return false;
}

bool slash_value_eq(SlashValue *a, SlashValue *b)
{
    if (a->type != b->type)
	return false;

    switch (a->type) {
    case SLASH_NUM:
	return a->num == b->num;
    case SLASH_BOOL:
	return a->boolean == b->boolean;
    case SLASH_NONE:
	return false;
    case SLASH_OBJ:
	return a->obj->traits->equals(a, b);

    default:
	REPORT_RUNTIME_ERROR("Equality not defined for type '%s'", "x");
	ASSERT_NOT_REACHED;
    }

    return false;
}

int slash_value_cmp_stub(const void *a, const void *b)
{
    return slash_value_cmp((SlashValue *)a, (SlashValue *)b);
}

int slash_value_cmp_rev_stub(const void *a, const void *b)
{
    return slash_value_cmp((SlashValue *)b, (SlashValue *)a);
}

int slash_value_cmp(SlashValue *a, SlashValue *b)
{
    if (a->type != b->type)
	return slash_cmp_precedence[a->type] - slash_cmp_precedence[b->type];

    switch (a->type) {
    case SLASH_BOOL:
	return a->boolean - b->boolean;
    case SLASH_NUM:
	return a->num - b->num;
    default:
	REPORT_RUNTIME_ERROR("Compare not implented for type '%s'", "X");
    }

    ASSERT_NOT_REACHED;
    return 0;
}

char *slash_type_to_name(SlashValue *value)
{
    if (value->type == SLASH_OBJ)
	return slash_obj_type_names[value->obj->type];
    return slash_type_names[value->type];
}

void slash_bool_print(SlashValue *value)
{
    printf("%s", value->boolean == true ? "true" : "false");
}

void slash_num_print(SlashValue *value)
{
    if (value->num == (int)value->num)
	printf("%d", (int)value->num);
    else
	printf("%f", value->num);
}

void slash_range_print(SlashValue *value)
{
    printf("%d..%d", value->range.start, value->range.end);
}

void slash_none_print(void)
{
    printf("none");
}
