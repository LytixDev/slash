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

#include <stdio.h>
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_str.h"
#include "interpreter/types/trait.h"
#include "lib/str_view.h"


ObjTraits str_traits = { .print = slash_str_print,
			 .item_get = NULL,
			 .item_assign = NULL,
			 .item_in = NULL,
			 .truthy = NULL,
			 .equals = NULL,
			 .cmp = NULL };


void slash_str_init_from_view(SlashStr *str, StrView *view)
{
    // TODO: dynamic
    str->len = view->size + 1;
    str->cap = str->len;
    str->p = malloc(str->cap);
    memcpy(str->p, view->view, str->len - 1);
    str->p[str->len - 1] = 0;

    str->obj.traits = &str_traits;
    str->gc_managed = true;
}

void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr)
{
    str->len = strlen(cstr) + 1;
    str->cap = str->len;
    str->p = cstr;

    str->obj.traits = &str_traits;
    str->gc_managed = false;
}

void slash_str_init_from_slice(SlashStr *str, char *cstr, size_t size)
{
    str->len = size + 1;
    str->cap = str->len;
    str->p = malloc(str->cap);
    memcpy(str->p, cstr, str->len - 1);
    str->p[str->len - 1] = 0;

    str->obj.traits = &str_traits;
    str->gc_managed = true;
}

char slash_str_last_char(SlashStr *str)
{
    if (str->len < 2)
	return EOF;
    return str->p[str->len - 2];
}

static char *split(char *str, char *separator, size_t separator_len)
{
    size_t offset = 0;
split_continue:
    while (str[offset] != 0) {
	for (size_t i = 0; i < separator_len; i++) {
	    if (str[i] == 0)
		goto split_end;
	    if (str[offset] != separator[i]) {
		offset++;
		goto split_continue;
	    }
	    // offset++;
	}

	/* if execution enters here then we just matched the separator */
	return str + offset - separator_len;
    }

split_end:
    return NULL;
}

static char *split_single_char(char *str, char *chars)
{
    size_t offset = 0;
    size_t split_chars = strlen(chars);
    while (str[offset] != 0) {
	for (size_t i = 0; i < split_chars; i++) {
	    if (str[offset] == chars[i])
		return str + offset - 1;
	}
	offset++;
    }

    return NULL;
}

SlashList *slash_str_internal_split(Interpreter *interpreter, SlashStr *str, char *separator)
{
    SlashList *list = (SlashList *)gc_alloc(interpreter, SLASH_OBJ_LIST);
    slash_list_init(list);

    size_t separator_len = strlen(separator);
    char *start_ptr = str->p;
    char *end_ptr = split(start_ptr, separator, separator_len);
    while (end_ptr != NULL) {
	/* save substr */
	SlashStr *substr = (SlashStr *)gc_alloc(interpreter, SLASH_OBJ_STR);
	slash_str_init_from_slice(substr, start_ptr, end_ptr - start_ptr + 1);
	slash_list_append(list, (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)substr });

	/* continue */
	start_ptr = end_ptr + separator_len + 1;
	end_ptr = split(start_ptr, separator, separator_len);
    }

    /* final substr */
    SlashStr *substr = (SlashStr *)gc_alloc(interpreter, SLASH_OBJ_STR);
    size_t final_size = (str->p + str->len - 1) - start_ptr;
    slash_str_init_from_slice(substr, start_ptr, final_size);
    slash_list_append(list, (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)substr });
    return list;
}

SlashList *slash_str_internal_split_any_char(Interpreter *interpreter, SlashStr *str, char *chars)
{
    SlashList *list = (SlashList *)gc_alloc(interpreter, SLASH_OBJ_LIST);
    slash_list_init(list);

    char *start_ptr = str->p;
    char *end_ptr = split_single_char(start_ptr, chars);
    while (end_ptr != NULL) {
	/* save substr */
	SlashStr *substr = (SlashStr *)gc_alloc(interpreter, SLASH_OBJ_STR);
	slash_str_init_from_slice(substr, start_ptr, end_ptr - start_ptr + 1);
	slash_list_append(list, (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)substr });

	/* continue */
	start_ptr = end_ptr + 2;
	end_ptr = split_single_char(start_ptr, chars);
    }

    /* final substr */
    SlashStr *substr = (SlashStr *)gc_alloc(interpreter, SLASH_OBJ_STR);
    size_t final_size = (str->p + str->len - 1) - start_ptr;
    slash_str_init_from_slice(substr, start_ptr, final_size);
    slash_list_append(list, (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)substr });
    return list;
}

/*
 * traits
 */
void slash_str_print(SlashValue *value)
{
    SlashStr *str = (SlashStr *)value->obj;
    printf("%s", str->p);
}

/*
 * methods
 */
SlashMethod slash_str_methods[SLASH_STR_METHODS_COUNT] = {
    { .name = "split", .fp = slash_str_split },
};

SlashValue slash_str_split(Interpreter *interpreter, SlashValue *self, size_t argc,
			   SlashValue *argv)
{
    SlashStr *str = (SlashStr *)self->obj;
    if (!match_signature("s", argc, argv)) {
	REPORT_RUNTIME_ERROR("Bad method args: Expected 'str'");
    }

    SlashStr *separator = (SlashStr *)argv[0].obj;

    SlashList *substrings = slash_str_internal_split(interpreter, str, separator->p);
    return (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)substrings };
}
