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
#include <stdio.h>

#include "interpreter/error.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"


ObjTraits tuple_traits = { .print = slash_tuple_print,
			   .item_get = slash_tuple_item_get,
			   .item_assign = NULL,
			   .item_in = slash_tuple_item_in,
			   .truthy = NULL,
			   .equals = NULL,
			   .cmp = NULL };


void slash_tuple_init(SlashTuple *tuple, size_t size)
{
    tuple->size = size;
    if (size == 0) {
	tuple->values = NULL;
    } else {
	tuple->values = malloc(sizeof(SlashValue) * size);
    }
    tuple->obj.traits = &tuple_traits;
}

void slash_tuple_print(SlashValue *value)
{
    assert(value->type == SLASH_OBJ);
    assert(value->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *tuple = (SlashTuple *)value->obj;
    SlashValue current;
    printf("'");
    for (size_t i = 0; i < tuple->size; i++) {
	current = tuple->values[i];
	/* call the print function directly */
	trait_print[current.type](&current);
	if (i != tuple->size - 1)
	    printf(", ");
    }
    printf("'");
}

SlashValue slash_tuple_item_get(Scope *scope, SlashValue *self, SlashValue *index)
{
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *tuple = (SlashTuple *)self->obj;
    if (index->type != SLASH_NUM) {
	report_runtime_error("List indices must be numbers");
	ASSERT_NOT_REACHED;
    }

    size_t idx = (size_t)index->num;
    if (idx > tuple->size) {
	report_runtime_error("List indices out of range");
	ASSERT_NOT_REACHED;
    }

    return tuple->values[idx];
}

bool slash_tuple_item_in(SlashValue *self, SlashValue *item)
{
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *tuple = (SlashTuple *)self->obj;
    for (size_t i = 0; i < tuple->size; i++) {
	if (slash_value_eq(&tuple->values[i], item))
	    return true;
    }

    return false;
}

// int slash_tuple_cmp(SlashTuple a, SlashTuple b)
//{
//     int result = 0;
//     size_t i = 0;
//     size_t min_size = a.size < b.size ? a.size : b.size;
//
//     while (i < min_size && result == 0) {
//	result = slash_value_cmp(&a.values[i], &b.values[i]);
//	i++;
//     }
//
//     return result;
// }
