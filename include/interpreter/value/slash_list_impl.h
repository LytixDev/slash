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

#include <stdlib.h>

#include "interpreter/value/slash_value.h"

/*
 * Based on the ArrayList implementation found in nicc (https://github.com/LytixDev/nicc)
 */

typedef struct {
    SlashValue *items;
    size_t size;
    size_t cap;
} SlashListImpl;


void slash_list_init(SlashListImpl *list);
void slash_list_free(SlashListImpl *list);

bool slash_list_set(SlashListImpl *list, SlashValue val, size_t idx);
bool slash_list_append(SlashListImpl *list, SlashValue *val);

bool slash_list_rm(SlashListImpl *list, size_t idx);
bool slash_list_rmv(SlashListImpl *list, SlashValue val);

SlashValue slash_list_get(SlashListImpl *list, size_t idx);
size_t slash_list_index_of(SlashListImpl *list, SlashValue val);

bool slash_list_sort(SlashListImpl *list);
