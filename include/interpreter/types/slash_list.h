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

#include "interpreter/types/method.h"
#include "interpreter/types/slash_range.h"
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
    ArrayList *underlying;
    // SlashValueType underlying_T;
} SlashList;

void slash_list_init(SlashList *list);
// void slash_list_copy(SlashList *list);

bool slash_list_append(SlashList *list, SlashValue val);
void slash_list_append_list(SlashList *list, SlashList *to_append);

SlashValue *slash_list_get(SlashList *list, size_t idx);

bool slash_list_set(SlashList *list, SlashValue *val, size_t idx);

/* NOTE: function assumes ret_ptr is NOT initialized */
void slash_list_from_ranged_copy(SlashList *ret_ptr, SlashList *to_copy, SlashRange range);

bool slash_list_eq(SlashList *a, SlashList *b);

/* common slash value functions */
void slash_list_print(SlashValue *value);
size_t *slash_list_len(SlashValue *value);
SlashValue slash_list_item_get(SlashValue *self, SlashValue *index);
void slash_list_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value);
bool slash_list_item_in(SlashValue *self, SlashValue *item);

/* slash list methods */
#define SLASH_LIST_METHODS_COUNT 1
extern SlashMethod slash_list_methods[SLASH_LIST_METHODS_COUNT];

SlashValue slash_list_pop(SlashValue *self, size_t argc, SlashValue *argv);


#endif /* SLASH_LIST_H */
