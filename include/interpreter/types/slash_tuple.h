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
#ifndef SLASH_TUPLE_H
#define SLASH_TUPLE_H

#include <stdlib.h>

#include "interpreter/scope.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_value.h"
#include "interpreter/types/trait.h"
#include "nicc/nicc.h"

/*
 * Tuples are ordered and unchangeable.
 */
typedef struct {
    SlashObj obj;
    size_t size;
    SlashValue *values;
} SlashTuple;

void slash_tuple_init(SlashTuple *tuple, size_t size);

/*
 * traits
 */

void slash_tuple_print(SlashValue *value);
SlashValue slash_tuple_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index);
bool slash_tuple_item_in(SlashValue *self, SlashValue *item);
bool slash_tuple_truthy(SlashValue *self);
bool slash_tuple_eq(SlashValue *a, SlashValue *b);
// int slash_tuple_cmp(SlashTuple a, SlashTuple b);

#endif /* SLASH_TUPLE_H */
