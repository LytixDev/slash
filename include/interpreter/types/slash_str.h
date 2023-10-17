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

/*
 * The String type for Slash.
 * Null terminated.
 * Mutable.
 */
typedef struct {
    SlashObj obj;
    char *p; // Null terminated
    size_t len; // Length of string: includes null terminator. So "hi" has length 3
    size_t cap;
} SlashStr;


/*
 * Init and GC allocate based on a view
 */
void slash_str_init_from_view(SlashStr *str, StrView *view);
/*
 * Does not GC allocate the cstr
 */
void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr);
/*
 * Init and GC allocate `size` bytes
 */
void slash_str_init_from_slice(SlashStr *str, char *cstr, size_t size);

/*
 * Returns the last char of the string.
 */
char slash_str_last_char(SlashStr *str);

SlashList *slash_str_internal_split(Interpreter *interpreter, SlashStr *str, char *separator,
				    bool split_any);

void slash_str_internal_concat(SlashStr *self, SlashStr *other);

/*
 * traits
 */
void slash_str_print(SlashValue *value);
SlashValue slash_str_to_str(Interpreter *interpreter, SlashValue *value);
SlashValue slash_str_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index);
void slash_str_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value);
bool slash_str_item_in(SlashValue *self, SlashValue *item);
bool slash_str_truthy(SlashValue *self);
bool slash_str_eq(SlashValue *a, SlashValue *b);
int slash_str_cmp(SlashValue *self, SlashValue *other);
size_t slash_str_hash(SlashValue *self);

/*
 * methods
 */
#define SLASH_STR_METHODS_COUNT 3
extern SlashMethod slash_str_methods[SLASH_STR_METHODS_COUNT];

SlashValue slash_str_split(Interpreter *interpreter, SlashValue *self, size_t argc,
			   SlashValue *argv);
SlashValue slash_str_len(Interpreter *interpreter, SlashValue *self, size_t argc, SlashValue *argv);
SlashValue slash_str_concat(Interpreter *interpreter, SlashValue *self, size_t argc,
			    SlashValue *argv);


#endif /* SLASH_STR_H */
