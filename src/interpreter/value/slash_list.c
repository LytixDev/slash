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
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_value.h"


/*
 * Allocates more memory if len (total items) >= capacity.
 * This ensures we have space to add at least one new item after the function is called.
 */
static void ensure_capacity(Interpreter *interpreter, SlashList *list)
{
    (void)interpreter;
    if (list->len >= list->cap) {
	size_t old_size = sizeof(SlashValue) * list->cap;
	list->cap = SLASH_LIST_GROW_CAPACITY(list->cap);
	size_t new_size = sizeof(SlashValue) * list->cap;
	list->items = gc_realloc(interpreter, list->items, old_size, new_size);
    }
}

void slash_list_impl_init(Interpreter *interpreter, SlashList *list)
{
#ifdef SLASH_LIST_STARTING_CAP
    list->cap = SLASH_LIST_STARTING_CAP;
#elif
    list->cap = 8;
#endif
    list->len = 0;
    list->items = gc_alloc(interpreter, sizeof(SlashValue) * list->cap);
}

void slash_list_impl_free(Interpreter *interpreter, SlashList *list)
{
    gc_free(interpreter, list->items, sizeof(SlashValue) * list->cap);
}

bool slash_list_impl_set(Interpreter *interpreter, SlashList *list, SlashValue val, size_t idx)
{
    /* Not possible to set a value at a position greater than the current len */
    if (idx > list->len)
	return false;

    ensure_capacity(interpreter, list);
    list->items[idx] = val;
    /* Only increase the length when we do not overwrite an existing item */
    if (idx == list->len)
	list->len++;
    return true;
}

bool slash_list_impl_append(Interpreter *interpreter, SlashList *list, SlashValue val)
{
    return slash_list_impl_set(interpreter, list, val, list->len);
}

SlashValue slash_list_impl_get(SlashList *list, size_t idx)
{
    /* This should be checked and reported as a runtime error before this function is called */
    assert(idx < list->len);
    return list->items[idx];
}

size_t slash_list_impl_index_of(SlashList *list, SlashValue val)
{
    for (size_t i = 0; i < list->len; i++) {
	SlashValue other = list->items[i];
	if (TYPE_EQ(val, other) && val.T_info->eq(val, other))
	    return slash_list_impl_rm(list, i);
    }
    return SIZE_MAX;
}

bool slash_list_impl_rm(SlashList *list, size_t idx)
{
    if (idx > list->len)
	return false;

    /* Shift all items to the right of the removed item by one */
    for (size_t i = idx + 1; i < list->len; i++)
	list->items[i - 1] = list->items[i];

    list->len--;
    return true;
}

bool slash_list_impl_rmv(SlashList *list, SlashValue val)
{
    if (val.T_info->eq == NULL)
	REPORT_RUNTIME_ERROR(
	    "Can not remove type '%s' from list as equality is not defined on this type",
	    val.T_info->name);

    /* Find index of value */
    size_t idx = slash_list_impl_index_of(list, val);
    if (idx == SIZE_MAX)
	return false;
    return slash_list_impl_rm(list, idx);
}
