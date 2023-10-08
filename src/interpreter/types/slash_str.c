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
			 .item_get = slash_str_item_get,
			 .item_assign = slash_str_item_assign,
			 .item_in = slash_str_item_in,
			 .truthy = slash_str_truthy,
			 .equals = slash_str_eq,
			 .cmp = NULL };


void slash_str_init_from_view(SlashStr *str, StrView *view)
{
    // TODO: make string buffer dynamic similar to list
    str->len = view->size + 1;
    str->cap = str->len;
    str->p = malloc(str->cap);
    memcpy(str->p, view->view, str->len - 1);
    str->p[str->len - 1] = 0;

    str->obj.traits = &str_traits;
}

void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr)
{
    str->len = strlen(cstr) + 1;
    str->cap = str->len;
    str->p = cstr;
    str->obj.traits = &str_traits;
}

void slash_str_init_from_slice(SlashStr *str, char *cstr, size_t size)
{
    slash_str_init_from_view(str, &(StrView){ .view = cstr, .size = size });
}

char slash_str_last_char(SlashStr *str)
{
    /* empty str: "" */
    if (str->len <= 1)
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

SlashValue slash_str_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index)
{
    SlashStr *str = (SlashStr *)self->obj;
    if (!(index->type == SLASH_NUM || index->type == SLASH_RANGE)) {
	REPORT_RUNTIME_ERROR("List indices must be number or range");
	ASSERT_NOT_REACHED;
    }

    size_t offset = (size_t)index->num;
    size_t size = 1;
    if (index->type == SLASH_RANGE) {
	offset = index->range.start;
	size = index->range.end;
    }
    SlashStr *new_str = (SlashStr *)gc_alloc(interpreter, SLASH_OBJ_STR);
    slash_str_init_from_slice(new_str, str->p + offset, size);
    return (SlashValue){ .type = SLASH_OBJ, .obj = (SlashObj *)new_str };
}

void slash_str_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value)
{
    SlashStr *str = (SlashStr *)self->obj;

    if (index->type != SLASH_NUM) {
	REPORT_RUNTIME_ERROR("List indices must be number");
	ASSERT_NOT_REACHED;
    }
    if (!(new_value->type == SLASH_OBJ && new_value->obj->type == SLASH_OBJ_STR)) {
	REPORT_RUNTIME_ERROR("Can not assign value of type '%s' to a string",
			     SLASH_TYPE_TO_STR(new_value));
	ASSERT_NOT_REACHED;
    }
    SlashStr *ch = (SlashStr *)new_value->obj;
    if (ch->len - 1 != 1) {
	REPORT_RUNTIME_ERROR("Can only instert string of len 1, not len '%zu'", ch->len - 1);
	ASSERT_NOT_REACHED;
    }

    size_t idx = (size_t)index->num;
    if (idx >= str->len - 1) {
	REPORT_RUNTIME_ERROR("Index %zu out of range. String has len %zu", idx, str->len - 1);
	ASSERT_NOT_REACHED;
    }

    str->p[idx] = ch->p[0];
}

bool slash_str_item_in(SlashValue *self, SlashValue *item)
{
    if (!(item->type == SLASH_OBJ && item->obj->type == SLASH_OBJ_STR)) {
	REPORT_RUNTIME_ERROR("Can not check if a string contains a '%s'.", SLASH_TYPE_TO_STR(item));
	ASSERT_NOT_REACHED;
    }
    char *str = ((SlashStr *)self->obj)->p;
    char *substr = ((SlashStr *)item->obj)->p;

    return strstr(str, substr) != NULL;
}

bool slash_str_truthy(SlashValue *self)
{
    SlashStr *str = (SlashStr *)self->obj;
    return str->len > 1;
}

bool slash_str_eq(SlashValue *a, SlashValue *b)
{
    SlashStr *A = (SlashStr *)a->obj;
    SlashStr *B = (SlashStr *)b->obj;

    if (A->len != B->len)
	return false;

    return strcmp(A->p, B->p) == 0;
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
