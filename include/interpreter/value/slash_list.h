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
#ifndef SLASH_LIST_IMPL_H
#define SLASH_LIST_IMPL_H
#endif /* SLASH_LIST_IMPL_H */

#include <stdbool.h>
#include <stdlib.h>

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"

/*
 * Based on the ArrayList implementation found in nicc (https://github.com/LytixDev/nicc)
 */

#define SLASH_LIST_STARTING_CAP 8
#define SLASH_LIST_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

typedef struct slash_list_impl_t {
    SlashValue *items;
    size_t len;
    size_t cap;
} SlashListImpl;

typedef struct {
    SlashObj obj;
    SlashListImpl list;
} SlashList;


/* Functions */
void slash_list_impl_init(Interpreter *interpreter, SlashListImpl *list);
void slash_list_impl_free(Interpreter *interpreter, SlashListImpl *list);

bool slash_list_impl_set(Interpreter *interpreter, SlashListImpl *list, SlashValue val, size_t idx);
bool slash_list_impl_append(Interpreter *interpreter, SlashListImpl *list, SlashValue val);

SlashValue slash_list_impl_get(SlashListImpl *list, size_t idx);
/* Returns SIZE_MAX if value is not found in list */
size_t slash_list_impl_index_of(SlashListImpl *list, SlashValue val);

bool slash_list_impl_rm(SlashListImpl *list, size_t idx);
bool slash_list_impl_rmv(SlashListImpl *list, SlashValue val);

// TODO: implement
bool slash_list_impl_sort(SlashListImpl *list);
