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
#include "interpreter/types/slash_value.h"
#include "sac/sac.h"
#include "str_view.h"


void slash_print_none(void)
{
}

void slash_print_not_defined(SlashValue *value)
{
    printf("Print not defined for %d", value->type);
}

void slash_item_get_not_defined(void)
{
    slash_exit_interpreter_err("item no get");
}

void slash_item_assign_not_defined(void)
{
    slash_exit_interpreter_err("item assignment nt defined for this type");
}

SlashPrintFunc slash_print[SLASH_TYPE_COUNT] = {
    /* bool */
    (SlashPrintFunc)slash_bool_print,
    /* str */
    (SlashPrintFunc)slash_str_print,
    /* num */
    (SlashPrintFunc)slash_num_print,
    /* shlit */
    (SlashPrintFunc)slash_print_not_defined,
    /* range */
    (SlashPrintFunc)slash_range_print,
    /* list */
    (SlashPrintFunc)slash_list_print,
    /* tuple */
    (SlashPrintFunc)slash_tuple_print,
    /* map */
    (SlashPrintFunc)slash_map_print,
    /* none */
    (SlashPrintFunc)slash_print_none,
};

SlashItemGetFunc slash_item_get[SLASH_TYPE_COUNT] = {
    /* bool */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* str */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* num */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* shlit */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* range */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* list */
    (SlashItemGetFunc)slash_list_item_get,
    /* tuple */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* map */
    (SlashItemGetFunc)slash_item_get_not_defined,
    /* none */
    (SlashItemGetFunc)slash_item_get_not_defined,
};

SlashItemAssignFunc slash_item_assign[SLASH_TYPE_COUNT] = {
    /* bool */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* str */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* num */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* shlit */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* range */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* list */
    (SlashItemAssignFunc)slash_list_item_assign,
    /* tuple */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* map */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
    /* none */
    (SlashItemAssignFunc)slash_item_assign_not_defined,
};


SlashValue *slash_value_arena_alloc(Arena *arena, SlashType type)
{
    SlashValue *sv = m_arena_alloc_struct(arena, SlashValue);
    sv->type = type;
    return sv;
}

bool is_truthy(SlashValue *sv)
{
    switch (sv->type) {
    case SLASH_STR:
    case SLASH_SHLIT:
	return sv->str.size != 0;

    case SLASH_NUM:
	return sv->num != 0;

    case SLASH_BOOL:
	return sv->boolean;

    case SLASH_LIST:
	return sv->list.underlying.size != 0;

    case SLASH_TUPLE:
	return sv->tuple.size != 0;

    case SLASH_NONE:
	return false;

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
    case SLASH_STR:
    case SLASH_SHLIT:
	return str_view_eq(a->str, b->str);

    case SLASH_NUM:
	return a->num == b->num;

    case SLASH_BOOL:
	return a->boolean == b->boolean;

    case SLASH_LIST:
	return slash_list_eq(&a->list, &b->list);

    case SLASH_NONE:
	return false;

    default:
	fprintf(stderr, "equality not defined for this type, returning false");
    }

    return false;
}
