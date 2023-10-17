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
#include "interpreter/interpreter.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"


ObjTraits tuple_traits = {
    .print = slash_tuple_print,
    .to_str = NULL,
    .item_get = slash_tuple_item_get,
    .item_assign = NULL,
    .item_in = slash_tuple_item_in,
    .truthy = slash_tuple_truthy,
    .equals = slash_tuple_eq,
    .cmp = NULL,
    .hash = NULL,
};


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
    printf("(");
    for (size_t i = 0; i < tuple->size; i++) {
	current = tuple->values[i];
	/* call the print function directly */
	trait_print[current.type](&current);
	if (i != tuple->size - 1 || i == 0) // always print comma after first element
	    printf(", ");
    }
    printf(")");
}

SlashValue slash_tuple_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index)
{
    (void)interpreter;
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *tuple = (SlashTuple *)self->obj;
    if (index->type != SLASH_NUM) {
	REPORT_RUNTIME_ERROR("List indices must be numbers");
	ASSERT_NOT_REACHED;
    }

    size_t idx = (size_t)index->num;
    if (idx > tuple->size) {
	REPORT_RUNTIME_ERROR("List indices out of range");
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

bool slash_tuple_truthy(SlashValue *self)
{
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *tuple = (SlashTuple *)self->obj;
    return tuple->size != 0;
}

bool slash_tuple_eq(SlashValue *a, SlashValue *b)
{
    assert(a->type == SLASH_OBJ);
    assert(a->obj->type == SLASH_OBJ_TUPLE);
    assert(b->type == SLASH_OBJ);
    assert(b->obj->type == SLASH_OBJ_TUPLE);

    SlashTuple *A = (SlashTuple *)a->obj;
    SlashTuple *B = (SlashTuple *)b->obj;
    if (A->size != B->size)
	return false;

    for (size_t i = 0; i < A->size; i++) {
	SlashValue AV = A->values[i];
	SlashValue BV = B->values[i];
	if (!slash_value_eq(&AV, &BV))
	    return false;
    }
    return true;
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
