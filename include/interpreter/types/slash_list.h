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
#ifndef SLASH_LIST_H
#define SLASH_LIST_H

// #include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"

typedef struct slash_value_t SlashValue; // Forward declaration of SlashValue

/*
 * The SlashList is just a wrapper over the dynamic ArrayList impl I wrote for nicc.
 * T stores the type of the items in the list. If there are items of different types, T is SVT_ANY.
 * If T is SVT_NUM or SVT_STR then we can provide default sorting functions.
 * TODO: If T is something else, maybe we should expose some big brain api that takes in a
 * compare-function.
 */
typedef struct {
    ArrayList underlying;
    // SlashValueType underlying_T;
} SlashList;

void slash_list_init(SlashList *list);
// void slash_list_copy(SlashList *list);

bool slash_list_append(SlashList *list, SlashValue val);
void slash_list_append_list(SlashList *list, SlashList *to_append);

SlashValue *slash_list_get(SlashList *list, size_t idx);

bool slash_list_set(SlashList *list, SlashValue val, size_t idx);

void slash_list_print(SlashList *list);

#endif /* SLASH_LIST_H */
