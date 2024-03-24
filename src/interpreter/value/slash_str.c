/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "lib/str_view.h"


void slash_str_init_from_view(Interpreter *interpreter, SlashStr *str, StrView *view)
{
    str->len = view->size;
    str->str = gc_alloc(interpreter, str->len + 1);
    memcpy(str->str, view->view, str->len);
    str->str[str->len] = 0;
}

void slash_str_init_from_slice(Interpreter *interpreter, SlashStr *str, char *cstr, size_t size)
{
    slash_str_init_from_view(interpreter, str, &(StrView){ .view = cstr, .size = size });
}

void slash_str_init_from_concat(Interpreter *interpreter, SlashStr *str, SlashStr *a, SlashStr *b)
{
    str->len = a->len + b->len;
    str->str = gc_alloc(interpreter, str->len + 1);
    memcpy(str->str, a->str, a->len);
    memcpy(str->str + a->len, b->str, b->len);
    str->str[str->len] = 0;
}

void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr)
{
    str->len = strlen(cstr);
    str->str = cstr;
    str->obj.gc_managed = false;
}

static char *split_single(char *str, char *chars)
{
    size_t offset = 0;
    size_t split_chars = strlen(chars);
    while (str[offset] != 0) {
	for (size_t i = 0; i < split_chars; i++) {
	    if (str[offset] == chars[i])
		return str + offset;
	}
	offset++;
    }

    return NULL;
}

SlashList *slash_str_split(Interpreter *interpreter, SlashStr *str, char *separator, bool split_any)
{
    SlashList *list = (SlashList *)gc_new_T(interpreter, &list_type_info);
    gc_shadow_push(&interpreter->gc, &list->obj);
    slash_list_impl_init(interpreter, list);

    size_t separator_len = strlen(separator);
    char *start_ptr = str->str;
    char *end_ptr = split_any ? split_single(start_ptr, separator) : strstr(start_ptr, separator);
    while (end_ptr != NULL) {
	/* save substr */
	SlashStr *substr = (SlashStr *)gc_new_T(interpreter, &str_type_info);
	slash_str_init_from_slice(interpreter, substr, start_ptr, end_ptr - start_ptr);
	slash_list_impl_append(interpreter, list, AS_VALUE(substr));
	/* continue */
	start_ptr = end_ptr + (split_any ? 1 : separator_len);
	end_ptr = split_any ? split_single(start_ptr, separator) : strstr(start_ptr, separator);
    }

    /* final substr */
    size_t final_size = (str->str + str->len) - start_ptr;
    if (final_size != 0) {
	SlashStr *substr = (SlashStr *)gc_new_T(interpreter, &str_type_info);
	slash_str_init_from_slice(interpreter, substr, start_ptr, final_size);
	slash_list_impl_append(interpreter, list, AS_VALUE(substr));
    }

    gc_shadow_pop(&interpreter->gc);
    return list;
}
