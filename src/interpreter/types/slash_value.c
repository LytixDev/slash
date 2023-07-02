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

#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "sac/sac.h"
#include "str_view.h"


/*
 * print, len
 */
SlashOpFunc type_functions[SLASH_TYPE_COUNT][OP_COUNT] = {
    /* bool */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },

    /* str */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },

    /* num */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },

    /* shlit */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },

    /* range */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },

    /* list */
    { (SlashOpFunc)slash_list_print, (SlashOpFunc)slash_op_not_supported },

    /* tuple */
    { (SlashOpFunc)slash_tuple_print, (SlashOpFunc)slash_op_not_supported },

    /* map */
    { (SlashOpFunc)slash_map_print, (SlashOpFunc)slash_op_not_supported },

    /* none */
    { (SlashOpFunc)slash_op_not_supported, (SlashOpFunc)slash_op_not_supported },
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

void slash_value_print(SlashValue *sv)
{
    switch (sv->type) {
    case SLASH_STR:
    case SLASH_SHLIT:
	str_view_print(sv->str);
	break;

    case SLASH_NUM:
	printf("%f", sv->num);
	break;

    case SLASH_LIST:
	slash_list_print(&sv->list);
	break;

    case SLASH_TUPLE:
	slash_tuple_print(&sv->tuple);
	break;

    case SLASH_MAP:
	slash_map_print(&sv->map);
	break;

    case SLASH_BOOL:
	printf("%s", sv->boolean ? "true" : "false");
	break;


    default:
	fprintf(stderr, "printing not defined for this type");
    }
}

void slash_op_not_supported(void *arg)
{
    printf("Not supported\n");
}
