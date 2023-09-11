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
#include "nicc/nicc.h"

typedef struct slash_value_t SlashValue; // Forward declaration of SlashValue

/*
 * Tuples are ordered and unchangeable.
 */
typedef struct {
    size_t size; // can actually be stored in place since the tuple is unchangeable
    SlashValue *values;
} SlashTuple;


void slash_tuple_init(Scope *scope, SlashTuple *tuple, size_t size);
void slash_tuple_free(SlashTuple *tuple);

/* common slash value functions */

// void slash_tuple_print(SlashValue *value);
// size_t *slash_tuple_len(SlashValue *value);
// SlashValue slash_tuple_item_get(Scope *scope, SlashValue *self, SlashValue *index);
// bool slash_tuple_item_in(SlashValue *self, SlashValue *item);
// int slash_tuple_cmp(SlashTuple a, SlashTuple b);

#endif /* SLASH_TUPLE_H */
