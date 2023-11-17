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
#ifndef SLASH_STR_IMPL_H
#define SLASH_STR_IMPL_H
#endif /* SLASH_STR_IMPL_H */

#include <stdbool.h>
#include <stdlib.h>

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"


typedef struct {
    SlashObj obj;
    char *str; // Null terminated
    size_t len; // Length of string: does not includes null terminator. So "hi" has length 2
} SlashStr;


/* functions */
void slash_str_init_from_view(Interpreter *interpreter, SlashStr *str, StrView *view);
void slash_str_init_from_slice(Interpreter *interpreter, SlashStr *str, char *cstr, size_t size);
void slash_str_init_from_concat(Interpreter *interpreter, SlashStr *str, SlashStr *a, SlashStr *b);
void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr);
SlashList *slash_str_split(Interpreter *interpreter, SlashStr *str, char *separator,
			   bool split_any);
