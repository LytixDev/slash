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

#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


void slash_list_init(SlashList *list)
{
    arraylist_init(&list->underlying, sizeof(SlashValue));
    // list->underlying_T = SLASH_NONE;
}

bool slash_list_append(SlashList *list, SlashValue val)
{
    return arraylist_append(&list->underlying, &val);
}

void slash_list_append_list(SlashList *list, SlashList *to_append)
{
    for (size_t i = 0; i < to_append->underlying.size; i++)
	arraylist_append(&list->underlying, slash_list_get(to_append, i));
}

bool slash_list_set(SlashList *list, SlashValue val, size_t idx)
{
    return arraylist_set(&list->underlying, &val, idx);
}

SlashValue *slash_list_get(SlashList *list, size_t idx)
{
    return arraylist_get(&list->underlying, idx);
}

void slash_list_print(SlashList *list)
{
    putchar('[');
    for (size_t i = 0; i < list->underlying.size; i++) {
	slash_value_print(arraylist_get(&list->underlying, i));
	if (i != list->underlying.size - 1)
	    printf(", ");
    }
    putchar(']');
}
