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
#ifndef SLASH_STR_H
#define SLASH_STR_H

#include <stdbool.h>
#include <stdlib.h>

#include "interpreter/interpreter.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_obj.h"
#include "lib/str_view.h"

typedef struct {
    SlashObj obj;
    bool gc_managed;
    char *p;
    size_t len; // length of string: includes null terminator. So "hi" has length 3
    size_t cap;
} SlashStr;


void slash_str_init_from_view(SlashStr *str, StrView *view);
void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr);
void slash_str_init_from_slice(SlashStr *str, char *cstr, size_t size);

char slash_str_last_char(SlashStr *str);
SlashList *slash_str_internal_split(Interpreter *interpreter, SlashStr *str, char *splitter);
SlashList *slash_str_internal_split_any_char(Interpreter *interpreter, SlashStr *str, char *chars);
// void slash_list_init(SlashList *list);
// bool slash_list_append(SlashList *list, SlashValue val);
// void slash_list_append_list(SlashList *list, SlashList *to_append);
// SlashValue *slash_list_get(SlashList *list, size_t idx);
// bool slash_list_set(SlashList *list, SlashValue *val, size_t idx);


/*
 * traits
 */
void slash_str_print(SlashValue *value);
SlashValue slash_str_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index);
void slash_str_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value);
bool slash_str_truthy(SlashValue *self);
bool slash_str_eq(SlashValue *a, SlashValue *b);


/*
 * methods
 */
#define SLASH_STR_METHODS_COUNT 1
extern SlashMethod slash_str_methods[SLASH_STR_METHODS_COUNT];

SlashValue slash_str_split(Interpreter *interpreter, SlashValue *self, size_t argc,
			   SlashValue *argv);


#endif /* SLASH_STR_H */
