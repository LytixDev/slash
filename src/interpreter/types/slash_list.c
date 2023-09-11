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

#include "interpreter/error.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


ObjTraits list_traits = { .print = slash_list_print,
			  .item_get = slash_list_item_get,
			  .item_assign = slash_list_item_assign,
			  .item_in = slash_list_item_in,
			  .truthy = slash_list_truthy,
			  .equals = slash_list_eq,
			  .cmp = NULL };


void slash_list_init(SlashList *list)
{
    arraylist_init(&list->underlying, sizeof(SlashValue));
    list->obj.traits = &list_traits;
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

bool slash_list_set(SlashList *list, SlashValue *val, size_t idx)
{
    return arraylist_set(&list->underlying, val, idx);
}

SlashValue *slash_list_get(SlashList *list, size_t idx)
{
    return arraylist_get(&list->underlying, idx);
}

void slash_list_from_ranged_copy(Scope *scope, SlashList *ret_ptr, SlashList *to_copy,
				 SlashRange range)
{
    // slash_list_init(scope, ret_ptr);
    // assert((size_t)range.end <= to_copy->underlying->size);
    for (int i = range.start; i < range.end; i++)
	arraylist_append(&ret_ptr->underlying, arraylist_get(&to_copy->underlying, i));
}


/* 
 * traits 
 */

void slash_list_print(SlashValue *value)
{
    SlashList *list = (SlashList *)value->obj;
    ArrayList *underlying = &list->underlying;
    SlashValue *item;

    putchar('[');

    for (size_t i = 0; i < underlying->size; i++) {
	item = arraylist_get(underlying, i);
	trait_print[item->type](item);

	if (i != underlying->size - 1)
	    printf(", ");
    }
    putchar(']');
}

SlashValue slash_list_item_get(Scope *scope, SlashValue *self, SlashValue *index)
{
    SlashList *list = (SlashList *)self->obj;
    if (!(index->type == SLASH_NUM || index->type == SLASH_RANGE)) {
	report_runtime_error("List indices must be number or range");
	ASSERT_NOT_REACHED;
    }

    if (index->type == SLASH_NUM) {
	size_t idx = (size_t)index->num;
	return *slash_list_get(list, idx);
    }

    SlashList ranged_copy;
    slash_list_from_ranged_copy(scope, &ranged_copy, list, index->range);
    return (SlashValue){ 0 };
    // return (SlashValue){ .type = SLASH_LIST, .list = ranged_copy };
}

void slash_list_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value)
{
    SlashList *list = (SlashList *)self->obj;

    if (index->type != SLASH_NUM) {
	report_runtime_error("List indices must be number");
	ASSERT_NOT_REACHED;
    }

    // TODO: cursed double to index
    size_t idx = (size_t)index->num;
    slash_list_set(list, new_value, idx);
}

bool slash_list_item_in(SlashValue *self, SlashValue *item)
{
    SlashList *list = (SlashList *)self->obj;

    ArrayList underlying = list->underlying;
    for (size_t i = 0; i < underlying.size; i++) {
	if (slash_value_eq(arraylist_get(&underlying, i), item))
	    return true;
    }
    return false;
}

bool slash_list_truthy(SlashValue *self)
{
    SlashList *list = (SlashList *)self->obj;
    return list->underlying.size != 0;
}

bool slash_list_eq(SlashValue *a, SlashValue *b)
{
    SlashList *list_a = (SlashList *)a->obj;
    SlashList *list_b = (SlashList *)b->obj;
    if (list_a->underlying.size != list_b->underlying.size)
	return false;

    SlashValue *A;
    SlashValue *B;
    for (size_t i = 0; i < list_a->underlying.size; i++) {
	A = slash_list_get(list_a, i);
	B = slash_list_get(list_b, i);
	if (!slash_value_eq(A, B))
	    return false;
    }
    return true;
}


/* 
 * methods 
 */
// SlashMethod slash_list_methods[SLASH_LIST_METHODS_COUNT] = {
//     { .name = "pop", .fp = slash_list_pop },
//     { .name = "len", .fp = slash_list_len },
//     { .name = "sort", .fp = slash_list_sort },
// };
//
// SlashValue slash_list_pop(SlashValue *self, size_t argc, SlashValue *argv)
//{
//     assert(self->type == SLASH_LIST);
//
//     ArrayList *underlying = self->list.underlying;
//     SlashValue popped_item;
//
//     if (match_signature("", argc, argv)) {
//	arraylist_pop_and_copy(underlying, &popped_item);
//	return popped_item;
//     }
//
//     if (!match_signature("n", argc, argv)) {
//	report_runtime_error("Bad method args");
//	ASSERT_NOT_REACHED;
//     }
//
//     /* argc must be 1 and type of argv must be num */
//     SlashValue key = argv[0];
//     size_t idx = (size_t)key.num;
//     if (key.num < 0 || idx > underlying->size - 1) {
//	report_runtime_error("Index out of range");
//	ASSERT_NOT_REACHED;
//     }
//     arraylist_get_copy(underlying, idx, &popped_item);
//     arraylist_rm(underlying, idx);
//     return popped_item;
// }
//
// SlashValue slash_list_len(SlashValue *self, size_t argc, SlashValue *argv)
//{
//     assert(self->type == SLASH_LIST);
//
//     ArrayList *underlying = self->list.underlying;
//
//     if (!match_signature("", argc, argv)) {
//	report_runtime_error("Bad method args");
//	ASSERT_NOT_REACHED;
//     }
//
//     SlashValue len = { .type = SLASH_NUM, .num = (double)underlying->size };
//     return len;
// }
//
// SlashValue slash_list_sort(SlashValue *self, size_t argc, SlashValue *argv)
//{
//     assert(self->type == SLASH_LIST);
//
//     ArrayList *underlying = self->list.underlying;
//
//     if (match_signature("", argc, argv)) {
//	arraylist_sort(underlying, slash_value_cmp_stub);
//     } else if (match_signature("b", argc, argv)) {
//	arraylist_sort(underlying,
//		       argv[0].boolean ? slash_value_cmp_rev_stub : slash_value_cmp_stub);
//     } else {
//	report_runtime_error("Bad method args, expected no args or a single boolean.");
//	ASSERT_NOT_REACHED;
//     }
//
//     return (SlashValue){ .type = SLASH_NONE };
// }
