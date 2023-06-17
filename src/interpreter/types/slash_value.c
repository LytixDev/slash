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
#include "interpreter/types/slash_value.h"
#include "sac/sac.h"
#include "str_view.h"


SlashValue *slash_value_arena_alloc(Arena *arena, SlashValueType type)
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

    case SLASH_NONE:
	return false;

    default:
	fprintf(stderr, "truthy not defined for this type, returning false");
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

    default:
	fprintf(stderr, "printing not defined for this type");
    }
}
