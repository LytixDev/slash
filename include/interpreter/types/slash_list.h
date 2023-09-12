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

#include "interpreter/scope.h"
#include "interpreter/types/method.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"

/*
 * The SlashList is just a wrapper over the dynamic ArrayList impl I wrote for nicc.
 */
typedef struct {
    SlashObj obj;
    ArrayList underlying;
} SlashList;


void slash_list_init(SlashList *list);
bool slash_list_append(SlashList *list, SlashValue val);
void slash_list_append_list(SlashList *list, SlashList *to_append);
SlashValue *slash_list_get(SlashList *list, size_t idx);
bool slash_list_set(SlashList *list, SlashValue *val, size_t idx);
/* NOTE: function assumes ret_ptr is NOT initialized */
void slash_list_from_ranged_copy(Scope *scope, SlashList *ret_ptr, SlashList *to_copy,
				 SlashRange range);


/*
 * traits
 */
void slash_list_print(SlashValue *value);
SlashValue slash_list_item_get(Scope *scope, SlashValue *self, SlashValue *index);
void slash_list_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value);
bool slash_list_item_in(SlashValue *self, SlashValue *item);
bool slash_list_truthy(SlashValue *self);
bool slash_list_eq(SlashValue *a, SlashValue *b);


/*
 * methods
 */
#define SLASH_LIST_METHODS_COUNT 3
extern SlashMethod slash_list_methods[SLASH_LIST_METHODS_COUNT];

SlashValue slash_list_pop(SlashValue *self, size_t argc, SlashValue *argv);
SlashValue slash_list_len(SlashValue *self, size_t argc, SlashValue *argv);
SlashValue slash_list_sort(SlashValue *self, size_t argc, SlashValue *argv);


#endif /* SLASH_LIST_H */
