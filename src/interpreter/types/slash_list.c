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
#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


void slash_list_init(Scope *scope, SlashList *list)
{
    list->underlying = scope_alloc(scope, sizeof(ArrayList));
    arraylist_init(list->underlying, sizeof(SlashValue));
    // scope_register_owning(scope, list->underlying, arraylist_free);
    // MemObj *m = arraylist_get(&scope->owning, 0);
}

bool slash_list_append(SlashList *list, SlashValue val)
{
    return arraylist_append(list->underlying, &val);
}

void slash_list_append_list(SlashList *list, SlashList *to_append)
{
    for (size_t i = 0; i < to_append->underlying->size; i++)
	arraylist_append(list->underlying, slash_list_get(to_append, i));
}

bool slash_list_set(SlashList *list, SlashValue *val, size_t idx)
{
    return arraylist_set(list->underlying, val, idx);
}

SlashValue *slash_list_get(SlashList *list, size_t idx)
{
    return arraylist_get(list->underlying, idx);
}

void slash_list_from_ranged_copy(Scope *scope, SlashList *ret_ptr, SlashList *to_copy,
				 SlashRange range)
{
    slash_list_init(scope, ret_ptr);
    assert(range.end <= to_copy->underlying->size);
    for (int i = range.start; i < range.end; i++)
	arraylist_append(ret_ptr->underlying, arraylist_get(to_copy->underlying, i));
}

bool slash_list_eq(SlashList *a, SlashList *b)
{
    if (a->underlying->size != b->underlying->size)
	return false;

    SlashValue *A;
    SlashValue *B;
    for (size_t i = 0; i < a->underlying->size; i++) {
	A = slash_list_get(a, i);
	B = slash_list_get(b, i);
	if (!slash_value_eq(A, B))
	    return false;
    }
    return true;
}


/* common functions */

void slash_list_print(SlashValue *value)
{
    ArrayList *underlying = value->list.underlying;
    SlashValue *item;

    putchar('[');

    for (size_t i = 0; i < underlying->size; i++) {
	item = arraylist_get(underlying, i);
	slash_print[item->type](item);

	if (i != underlying->size - 1)
	    printf(", ");
    }
    putchar(']');
}

size_t *slash_list_len(SlashValue *value)
{
    return &value->list.underlying->size;
}

SlashValue slash_list_item_get(SlashValue *self, SlashValue *index)
{
    assert(self->type == SLASH_LIST);
    if (!(index->type == SLASH_NUM || index->type == SLASH_RANGE)) {
	slash_exit_interpreter_err("list indices must be number or range");
	ASSERT_NOT_REACHED;
    }

    if (index->type == SLASH_NUM) {
	size_t idx = (size_t)index->num;
	return *slash_list_get(&self->list, idx);
    }

    slash_exit_interpreter_err("todo");

    // SlashValue ranged_copy;
    // ranged_copy.type = SLASH_LIST;
    // slash_list_init(&ranged_copy.list);
    // slash_list_from_ranged_copy(&ranged_copy.list, &self->list, index->range);
    // return ranged_copy;
}

void slash_list_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value)
{
    assert(self->type == SLASH_LIST);

    if (index->type != SLASH_NUM) {
	slash_exit_interpreter_err("list indices must be number");
	ASSERT_NOT_REACHED;
    }

    // TODO: cursed double to index
    size_t idx = (size_t)index->num;
    slash_list_set(&self->list, new_value, idx);
}

bool slash_list_item_in(SlashValue *self, SlashValue *item)
{
    assert(self->type == SLASH_LIST);

    ArrayList *underlying = self->list.underlying;
    for (size_t i = 0; i < underlying->size; i++) {
	if (slash_value_eq(arraylist_get(underlying, i), item))
	    return true;
    }
    return false;
}


/* methods */
SlashMethod slash_list_methods[SLASH_LIST_METHODS_COUNT] = {
    (SlashMethod){ .name = "pop", .fp = slash_list_pop },
};

SlashValue slash_list_pop(SlashValue *self, size_t argc, SlashValue *argv)
{
    assert(self->type == SLASH_LIST);

    ArrayList *underlying = self->list.underlying;
    SlashValue popped_item;

    if (match_signature("", argc, argv)) {
	arraylist_pop_and_copy(underlying, &popped_item);
	return popped_item;
    }

    if (!match_signature("n", argc, argv)) {
	slash_exit_interpreter_err("bad method args");
    }

    /* argc must be 1 and type of argv must be num */
    SlashValue key = argv[0];
    size_t idx = (size_t)key.num;
    if (idx < 0 || idx > underlying->size - 1) {
	slash_exit_interpreter_err("index out of range");
    }
    arraylist_get_copy(underlying, idx, &popped_item);
    arraylist_rm(underlying, idx);
    return popped_item;
}
