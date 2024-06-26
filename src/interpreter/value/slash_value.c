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

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/type_funcs.h"
#include "lib/str_builder.h"
#include "lib/str_view.h"


/*
 * bool impl
 */
SlashValue bool_unary_not(SlashValue self)
{
	assert(IS_BOOL(self));
	return (SlashValue){ .T = &bool_type_info, .boolean = !self.boolean };
}

void bool_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_BOOL(self));
	SLASH_PRINT(&interpreter->stream_ctx, "%s", self.boolean == true ? "true" : "false");
}

SlashValue bool_to_str(Interpreter *interpreter, SlashValue self)
{
	assert(IS_BOOL(self));
	SlashObj *str = gc_new_T(interpreter, &str_type_info);
	if (self.boolean)
		slash_str_init_from_slice(interpreter, (SlashStr *)str, "true", 4);
	else
		slash_str_init_from_slice(interpreter, (SlashStr *)str, "false", 5);

	return AS_VALUE(str);
}


bool bool_truthy(SlashValue self)
{
	assert(IS_BOOL(self));
	return self.boolean;
}

bool bool_eq(SlashValue self, SlashValue other)
{
	assert(IS_BOOL(self) && IS_BOOL(other));
	return self.boolean == other.boolean;
}

int bool_cmp(SlashValue self, SlashValue other)
{
	assert(IS_BOOL(self) && IS_BOOL(other));
	return self.boolean > other.boolean;
}

int bool_hash(SlashValue self)
{
	return self.boolean;
}


/*
 * num impl
 */
SlashValue num_plus(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_NUM(self) && IS_NUM(other));
	// TODO: check for overflow and other undefined behaviour. Same for all arithmetic operators.
	return (SlashValue){ .T = &num_type_info, .num = self.num + other.num };
}

SlashValue num_minus(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	return (SlashValue){ .T = &num_type_info, .num = self.num - other.num };
}

SlashValue num_mul(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_NUM(self) && IS_NUM(other));
	return (SlashValue){ .T = &num_type_info, .num = self.num * other.num };
}

SlashValue num_div(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	if (other.num == 0)
		REPORT_RUNTIME_ERROR_OPAQUE("Division by zero error");
	return (SlashValue){ .T = &num_type_info, .num = self.num / other.num };
}

SlashValue num_int_div(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	if (other.num == 0)
		REPORT_RUNTIME_ERROR_OPAQUE("Division by zero error");
	return (SlashValue){ .T = &num_type_info, .num = (int)(self.num / other.num) };
}

SlashValue num_pow(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	return (SlashValue){ .T = &num_type_info, .num = pow(self.num, other.num) };
}

SlashValue num_mod(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	if (other.num == 0)
		REPORT_RUNTIME_ERROR_OPAQUE("Modulo by zero error");
	double m = fmod(self.num, other.num);
	/* same behaviour as we tend to see in maths */
	m = m < 0 && other.num > 0 ? m + other.num : m;
	return (SlashValue){ .T = &num_type_info, .num = m };
}

SlashValue num_unary_minus(SlashValue self)
{
	assert(IS_NUM(self));
	return (SlashValue){ .T = &num_type_info, .num = -self.num };
}

SlashValue num_unary_not(SlashValue self)
{
	assert(IS_NUM(self));
	TraitTruthy is_truthy = self.T->truthy;
	return (SlashValue){ .T = &bool_type_info, .boolean = !is_truthy(self) };
}

void num_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_NUM(self));
	if (self.num == (int)self.num)
		SLASH_PRINT(&interpreter->stream_ctx, "%d", (int)self.num);
	else
		SLASH_PRINT(&interpreter->stream_ctx, "%f", self.num);
}

SlashValue num_to_str(Interpreter *interpreter, SlashValue self)
{
	assert(IS_NUM(self));
	SlashObj *str = gc_new_T(interpreter, &str_type_info);
	char buffer[256];
	int len = sprintf(buffer, "%f", self.num);
	slash_str_init_from_slice(interpreter, (SlashStr *)str, buffer, len);
	return AS_VALUE(str);
}

bool num_truthy(SlashValue self)
{
	assert(IS_NUM(self));
	return self.num != 0;
}

bool num_eq(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	return self.num == other.num;
}

int num_cmp(SlashValue self, SlashValue other)
{
	assert(IS_NUM(self) && IS_NUM(other));
	if (self.num > other.num)
		return 1;
	if (self.num < other.num)
		return -1;
	return 0;
}

int num_hash(SlashValue self)
{
	assert(IS_NUM(self));
	if (self.num == (int)self.num)
		return self.num;
	/* cursed */
	union long_or_d {
		long l;
		double d;
	};
	union long_or_d iod = { .d = self.num };
	return (int)iod.l;
}


/*
 * range impl
 */
void range_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_RANGE(self));
	SLASH_PRINT(&interpreter->stream_ctx, "%d -> %d", self.range.start, self.range.end);
}

SlashValue range_to_str(Interpreter *interpreter, SlashValue self)
{
	assert(IS_RANGE(self));
	SlashObj *str = gc_new_T(interpreter, &str_type_info);
	char buffer[512];
	int len = sprintf(buffer, "%d -> %d", self.range.start, self.range.end);
	slash_str_init_from_slice(interpreter, (SlashStr *)str, buffer, len);
	return AS_VALUE(str);
}

SlashValue range_item_get(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_RANGE(self));
	if (IS_NUM(other)) {
		if (!NUM_IS_INT(other))
			REPORT_RUNTIME_ERROR("Range index can not be a floating point number: '%f", other.num);
		size_t idx = other.num;
		size_t range_size = self.range.start > self.range.end ? self.range.start - self.range.end
															  : self.range.end - self.range.start;
		if (idx >= range_size)
			REPORT_RUNTIME_ERROR(
				"Range index out of range. Has size '%zu', tried to get item at index '%zu'",
				range_size, idx);

		int offset;
		if (self.range.end > self.range.start)
			offset = self.range.start + idx;
		else
			offset = self.range.start - idx;
		return (SlashValue){ .T = &num_type_info, .num = offset };
	}

	REPORT_RUNTIME_ERROR("TODO: implement item get on range for type range");
}

bool range_item_in(SlashValue self, SlashValue other)
{
	// TODO: can we implement range_a in range_b ? Would check if a is "subset" of b.
	assert(IS_RANGE(self));
	if (!IS_NUM(other))
		return false;
	if (!NUM_IS_INT(other))
		return false;

	int offset = self.range.start + other.num;
	return offset < self.range.end;
}

bool range_truthy(SlashValue self)
{
	(void)self;
	return true;
}

bool range_eq(SlashValue self, SlashValue other)
{
	assert(IS_RANGE(self) && IS_RANGE(other));
	return self.range.start == other.range.start && self.range.end == other.range.end;
}


/*
 * text_lit impl
 */
SlashValue text_lit_to_str(Interpreter *interpreter, SlashValue self)
{
	assert(IS_TEXT_LIT(self));
	StrView tl = self.text_lit;
	ArenaTmp tmp = m_arena_tmp_init(&interpreter->arena);
	StrBuilder sb;
	str_builder_init(&sb, tmp.arena);
	/* Eval tilde */
	for (size_t i = 0; i < tl.size; i++) {
		char c = tl.view[i];
		if (c == '~') {
			ScopeAndValue sv = var_get(interpreter->scope, &(StrView){ .view = "HOME", .size = 4 });
			if (sv.scope != NULL && IS_STR(*sv.value)) {
				SlashStr *home_val = AS_STR(*sv.value);
				str_builder_append(&sb, home_val->str, home_val->len);
				continue;
			}
		}

		/* TODO: Wasteful to call append_char on every iteration. */
		str_builder_append_char(&sb, c);
	}

	SlashObj *str = gc_new_T(interpreter, &str_type_info);
	StrView built = str_builder_complete(&sb);
	slash_str_init_from_view(interpreter, (SlashStr *)str, &built);
	m_arena_tmp_release(tmp);

	return AS_VALUE(str);
}


/*
 * function impl
 */
void function_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_FUNCTION(self));
	(void)self;
	SLASH_PRINT(&interpreter->stream_ctx, "<function>");
}


/*
 * map impl
 */
SlashValue map_unary_not(SlashValue self)
{
	assert(IS_MAP(self));
	return (SlashValue){ .T = &bool_type_info, .boolean = !self.T->truthy(self) };
}

void map_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_MAP(self));
	slash_map_impl_print(interpreter, *AS_MAP(self));
}

typedef SlashValue (*TraitToStr)(Interpreter *interpreter, SlashValue self);

SlashValue map_item_get(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_MAP(self));
	return slash_map_impl_get(AS_MAP(self), other);
}

void map_item_assign(Interpreter *interpreter, SlashValue self, SlashValue index, SlashValue other)
{
	assert(IS_MAP(self));
	slash_map_impl_put(interpreter, AS_MAP(self), index, other);
}

bool map_item_in(SlashValue self, SlashValue other)
{
	assert(IS_MAP(self));
	return slash_map_impl_get(AS_MAP(self), other).T != &none_type_info;
}

bool map_truthy(SlashValue self)
{
	assert(IS_MAP(self));
	return AS_MAP(self)->len != 0;
}

bool map_eq(SlashValue self, SlashValue other)
{
	assert(IS_MAP(self) && IS_MAP(other));
	SlashMap *a = AS_MAP(self);
	SlashMap *b = AS_MAP(other);
	if (a->len != b->len)
		return false;

	// TODO: VLA bad
	SlashValue keys[a->len];
	slash_map_impl_get_keys(a, keys);
	for (size_t i = 0; i < a->len; i++) {
		SlashValue entry_a = slash_map_impl_get(a, keys[i]);
		SlashValue entry_b = slash_map_impl_get(b, keys[i]);
		if (!TYPE_EQ(entry_a, entry_b))
			return false;
		if (!entry_a.T->eq(entry_a, entry_b))
			return false;
	}

	return true;
}


/*
 * list impl
 */
SlashValue list_plus(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	// TODO: 1. We can prealloc the memory since we know the size
	//       2. We can use something like memcpy to copy all data in one batch
	assert(IS_LIST(self) && IS_LIST(self));
	SlashList *new_list = (SlashList *)gc_new_T(interpreter, &list_type_info);
	gc_barrier_start(&interpreter->gc);
	slash_list_impl_init(interpreter, new_list);

	SlashList *a = AS_LIST(self);
	for (size_t i = 0; i < a->len; i++)
		slash_list_impl_append(interpreter, new_list, a->items[i]);

	SlashList *b = AS_LIST(other);
	for (size_t i = 0; i < b->len; i++)
		slash_list_impl_append(interpreter, new_list, b->items[i]);

	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(new_list);
}

SlashValue list_unary_not(SlashValue self)
{
	assert(IS_LIST(self));
	return (SlashValue){ .T = &bool_type_info, .boolean = !self.T->truthy(self) };
}

void list_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_LIST(self));
	SlashList *underlying = AS_LIST(self);
	SLASH_PRINT(&interpreter->stream_ctx, "[");
	for (size_t i = 0; i < underlying->len; i++) {
		SlashValue item = underlying->items[i];
		assert(item.T->print != NULL);
		item.T->print(interpreter, item);
		if (i != underlying->len - 1)
			SLASH_PRINT(&interpreter->stream_ctx, ", ");
	}
	SLASH_PRINT(&interpreter->stream_ctx, "]");
}

// TODO: we have no easy mechanism to build a string rn.
typedef SlashValue (*TraitToStr)(Interpreter *interpreter, SlashValue self);

SlashValue list_item_get(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_LIST(self));
	assert(IS_NUM(other)); // TODO: implement for range
	if (!NUM_IS_INT(other))
		REPORT_RUNTIME_ERROR("List index can not be a floating point number: '%f'", other.num);

	SlashList *list = AS_LIST(self);
	int index = (int)other.num;
	if (index < 0 || (size_t)index >= list->len)
		REPORT_RUNTIME_ERROR("List index '%d' out of range for list with len '%zu'", index,
							 list->len);

	/* Know the index is valid */
	return slash_list_impl_get(list, index);
}

void list_item_assign(Interpreter *interpreter, SlashValue self, SlashValue index, SlashValue other)
{
	assert(IS_LIST(self));
	assert(IS_NUM(index)); // TODO: implement for range
	if (!NUM_IS_INT(index))
		REPORT_RUNTIME_ERROR("List index can not be a floating point number: '%f'", index.num);

	SlashList *list = AS_LIST(self);
	int idx = (int)index.num;
	if (idx < 0 || (size_t)idx >= list->len)
		REPORT_RUNTIME_ERROR("List index '%d' out of range for list with len '%zu'", idx,
							 list->len);

	/* Know the index is valid */
	slash_list_impl_set(interpreter, list, other, idx);
}

bool list_item_in(SlashValue self, SlashValue other)
{
	assert(IS_LIST(self));
	return slash_list_impl_index_of(AS_LIST(self), other) != SIZE_MAX;
}

bool list_truthy(SlashValue self)
{
	assert(IS_LIST(self));
	return AS_LIST(self)->len != 0;
}

bool list_eq(SlashValue self, SlashValue other)
{
	assert(IS_LIST(self) && IS_LIST(other));
	SlashList *a = AS_LIST(self);
	SlashList *b = AS_LIST(other);
	if (a->len != b->len)
		return false;

	/* Know the two lists have the same length */
	for (size_t i = 0; i < a->len; i++) {
		SlashValue A = slash_list_impl_get(a, i);
		SlashValue B = slash_list_impl_get(b, i);
		if (!TYPE_EQ(A, B))
			return false;
		/* Know A and B have the same types */
		TraitEq item_eq = A.T->eq;
		if (!item_eq(A, B))
			return false;
	}

	return true;
}


/*
 * tuple impl
 */
void slash_tuple_init(Interpreter *interpreter, SlashTuple *tuple, size_t size)
{
	tuple->len = size;
	if (size == 0)
		tuple->items = NULL;
	else
		tuple->items = gc_alloc(interpreter, sizeof(SlashValue) * size);
}

SlashValue tuple_plus(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	assert(IS_TUPLE(self) && IS_TUPLE(other));
	SlashTuple *a = AS_TUPLE(self);
	SlashTuple *b = AS_TUPLE(other);
	SlashTuple *new_tuple = (SlashTuple *)gc_new_T(interpreter, &tuple_type_info);
	slash_tuple_init(interpreter, new_tuple, a->len + b->len);

	for (size_t i = 0; i < a->len; i++)
		new_tuple->items[i] = a->items[i];

	for (size_t i = 0; i < b->len; i++)
		new_tuple->items[a->len + i] = b->items[i];

	return AS_VALUE(new_tuple);
}

SlashValue tuple_unary_not(SlashValue self)
{
	assert(IS_TUPLE(self));
	return (SlashValue){ .T = &bool_type_info, .boolean = !self.T->truthy(self) };
}

void tuple_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_TUPLE(self));
	SlashTuple *tuple = AS_TUPLE(self);
	SLASH_PRINT(&interpreter->stream_ctx, "(");
	for (size_t i = 0; i < tuple->len; i++) {
		SlashValue this = tuple->items[i];
		this.T->print(interpreter, this);
		if (i != tuple->len - 1 || i == 0)
			SLASH_PRINT(&interpreter->stream_ctx, ",");
	}
	SLASH_PRINT(&interpreter->stream_ctx, "(");
}

typedef SlashValue (*TraitToStr)(Interpreter *interpreter, SlashValue self);

SlashValue tuple_item_get(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	(void)interpreter;
	assert(IS_TUPLE(self));
	assert(IS_NUM(other)); // TODO: implement for range
	if (!NUM_IS_INT(other))
		REPORT_RUNTIME_ERROR("Tuple index can not be a floating point number: '%f'", other.num);

	SlashTuple *tuple = AS_TUPLE(self);
	int index = (int)other.num;
	if (index < 0 || (size_t)index >= tuple->len)
		REPORT_RUNTIME_ERROR("Tuple index '%d' out of range for list with len '%zu'", index,
							 tuple->len);

	/* Know the index is valid */
	return tuple->items[index];
}

bool tuple_item_in(SlashValue self, SlashValue other)
{
	assert(IS_TUPLE(self));
	SlashTuple *tuple = AS_TUPLE(self);
	for (size_t i = 0; i < tuple->len; i++) {
		SlashValue this = tuple->items[i];
		if (!TYPE_EQ(this, other))
			continue;
		if (this.T->eq(this, other))
			return true;
	}
	return false;
}

bool tuple_truthy(SlashValue self)
{
	assert(IS_TUPLE(self));
	return AS_TUPLE(self)->len != 0;
}

bool tuple_eq(SlashValue self, SlashValue other)
{
	assert(IS_TUPLE(self) && IS_TUPLE(other));
	SlashTuple *a = AS_TUPLE(self);
	SlashTuple *b = AS_TUPLE(other);
	if (a->len != b->len)
		return false;

	for (size_t i = 0; i < a->len; i++) {
		if (!TYPE_EQ(a->items[i], b->items[i]))
			return false;
		if (!(a->items[i].T->eq(a->items[i], b->items[i])))
			return false;
	}

	return true;
}

int tuple_hash(SlashValue self)
{
	assert(IS_TUPLE(self));
	SlashTuple *tuple = AS_TUPLE(self);
	int hash = 5381;
	for (size_t i = 0; i < tuple->len; i++) {
		SlashValue this = tuple->items[i];
		VERIFY_TRAIT_IMPL(hash, this, "Unhashable type '%s'", this.T->name);
		hash += ((hash << 5) + hash) + this.T->hash(this);
	}

	return hash;
}


/*
 * str impl
 */
SlashValue str_plus(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	assert(IS_STR(self) && IS_STR(other));
	SlashStr *new = (SlashStr *)gc_new_T(interpreter, &str_type_info);
	gc_barrier_start(&interpreter->gc);
	slash_str_init_from_concat(interpreter, new, AS_STR(self), AS_STR(other));
	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(new);
}

SlashValue str_unary_not(SlashValue self)
{
	assert(IS_STR(self));
	TraitTruthy is_truthy = self.T->truthy;
	return (SlashValue){ .T = &bool_type_info, .boolean = !is_truthy(self) };
}

void str_print(Interpreter *interpreter, SlashValue self)
{
	assert(IS_STR(self));
	SLASH_PRINT(&interpreter->stream_ctx, "\"%s\"", AS_STR(self)->str);
}

SlashValue str_to_str(Interpreter *interpreter, SlashValue self)
{
	(void)interpreter;
	return self;
}

SlashValue str_item_get(Interpreter *interpreter, SlashValue self, SlashValue other)
{
	assert(IS_STR(self));
	SlashStr *str = AS_STR(self);

	size_t start = 0;
	size_t end = 0;

	if (IS_NUM(other)) {
		if (!NUM_IS_INT(other))
			REPORT_RUNTIME_ERROR("Index can not be a floating point number: '%f", other.num);
		start = other.num;
		end = start + 1;
		if (start >= str->len)
			REPORT_RUNTIME_ERROR(
				"Index out of range. String has len '%zu', tried to get item at index '%zu'",
				str->len, start);
	} else if (IS_RANGE(other)) {
		start = other.range.start;
		end = other.range.end;
		if (start > end)
			REPORT_RUNTIME_ERROR("Reversed range can not be used to get item from string");
	} else {
		REPORT_RUNTIME_ERROR("Can not use '%s' as an index", other.T->name);
	}

	SlashStr *new = (SlashStr *)gc_new_T(interpreter, &str_type_info);
	gc_barrier_start(&interpreter->gc);
	slash_str_init_from_slice(interpreter, new, str->str + start, end - start);
	gc_barrier_end(&interpreter->gc);
	return AS_VALUE(new);
}

void str_item_assign(Interpreter *interpreter, SlashValue self, SlashValue index, SlashValue other)
{
	assert(IS_STR(self));
	assert(IS_STR(other));
	assert(IS_NUM(index)); // TODO: implement for range
	if (!NUM_IS_INT(index))
		REPORT_RUNTIME_ERROR("Str index can not be a floating point number: '%f'", index.num);

	SlashStr *str = AS_STR(self);
	/* Ensure the index is valid */
	int idx = (int)index.num;
	if (idx < 0 || (size_t)idx >= str->len)
		REPORT_RUNTIME_ERROR("Str index '%d' out of range for str with len '%zu'", idx, str->len);

	SlashStr *str_other = AS_STR(other);
	if (str_other->len != 1)
		REPORT_RUNTIME_ERROR("Can only assign a string of length one, not length.");

	str->str[idx] = str_other->str[0];
}

bool str_item_in(SlashValue self, SlashValue other)
{
	assert(IS_STR(self) && IS_STR(other));
	char *haystack = AS_STR(self)->str;
	char *needle = AS_STR(other)->str;
	char *end_ptr = strstr(haystack, needle);
	return end_ptr != NULL;
}

bool str_truthy(SlashValue self)
{
	assert(IS_STR(self));
	return AS_STR(self)->len != 0;
}

int str_cmp(SlashValue self, SlashValue other)
{
	assert(IS_STR(self) && IS_STR(other));
	return strcmp(AS_STR(self)->str, AS_STR(other)->str);
}

bool str_eq(SlashValue self, SlashValue other)
{
	assert(IS_STR(self) && IS_STR(other));
	// NOTE(Nicolai): Easy potential optimisation would be a comparison that returns early
	//                on the first character that is now the same.
	return str_cmp(self, other) == 0;
}

int str_hash(SlashValue self)
{
	assert(IS_STR(self));
	SlashStr *str = AS_STR(self);

	size_t A = 1327217885;
	size_t k = 5381;
	for (size_t i = 0; i < str->len; i++)
		k += ((k << 5) + k) + (str->str)[i];

	return (int)(k * A);
}


/*
 * none impl
 */
void none_print(Interpreter *interpreter, SlashValue self)
{
	(void)self;
	assert(IS_NONE(self));
	SLASH_PRINT(&interpreter->stream_ctx, "none");
}

SlashValue none_to_str(Interpreter *interpreter, SlashValue self)
{
	(void)interpreter;
	(void)self;
	assert(IS_NONE(self));

	SlashObj *str = gc_new_T(interpreter, &str_type_info);
	slash_str_init_from_slice(interpreter, (SlashStr *)str, "none", 4);
	return AS_VALUE(str);
}

bool none_truthy(SlashValue self)
{
	(void)self;
	assert(IS_NONE(self));
	return false;
}

bool none_eq(SlashValue self, SlashValue other)
{
	(void)self;
	(void)other;
	assert(IS_NONE(self) && IS_NONE(other));
	return true;
}

/*
 * type infos
 */
SlashTypeInfo bool_type_info = { .name = "bool",
								 .plus = NULL,
								 .minus = NULL,
								 .mul = NULL,
								 .div = NULL,
								 .int_div = NULL,
								 .pow = NULL,
								 .mod = NULL,
								 .unary_minus = NULL,
								 .unary_not = bool_unary_not,
								 .print = bool_print,
								 .to_str = bool_to_str,
								 .item_get = NULL,
								 .item_assign = NULL,
								 .item_in = NULL,
								 .truthy = bool_truthy,
								 .eq = bool_eq,
								 .cmp = bool_cmp,
								 .hash = bool_hash,
								 .obj_size = 0 };

SlashTypeInfo num_type_info = { .name = "num",
								.plus = num_plus,
								.minus = num_minus,
								.mul = num_mul,
								.div = num_div,
								.int_div = num_int_div,
								.pow = num_pow,
								.mod = num_mod,
								.unary_minus = num_unary_minus,
								.unary_not = num_unary_not,
								.print = num_print,
								.to_str = num_to_str,
								.item_get = NULL,
								.item_assign = NULL,
								.item_in = NULL,
								.truthy = num_truthy,
								.eq = num_eq,
								.cmp = num_cmp,
								.hash = num_hash,
								.obj_size = 0 };

SlashTypeInfo range_type_info = { .name = "range",
								  .plus = NULL,
								  .minus = NULL,
								  .mul = NULL,
								  .div = NULL,
								  .int_div = NULL,
								  .pow = NULL,
								  .mod = NULL,
								  .unary_minus = NULL,
								  .unary_not = NULL,
								  .print = range_print,
								  .to_str = range_to_str,
								  .item_get = range_item_get,
								  .item_assign = NULL,
								  .item_in = range_item_in,
								  .truthy = range_truthy,
								  .eq = range_eq,
								  .cmp = NULL,
								  .hash = NULL,
								  .obj_size = 0 };

SlashTypeInfo text_lit_type_info = { .name = "text",
									 .plus = NULL,
									 .minus = NULL,
									 .mul = NULL,
									 .div = NULL,
									 .int_div = NULL,
									 .pow = NULL,
									 .mod = NULL,
									 .unary_minus = NULL,
									 .unary_not = NULL,
									 .print = NULL,
									 .to_str = text_lit_to_str,
									 .item_get = NULL,
									 .item_assign = NULL,
									 .item_in = NULL,
									 .truthy = NULL,
									 .eq = NULL,
									 .cmp = NULL,
									 .hash = NULL,
									 .obj_size = 0 };

SlashTypeInfo function_type_info = { .name = "function",
									 .plus = NULL,
									 .minus = NULL,
									 .mul = NULL,
									 .div = NULL,
									 .int_div = NULL,
									 .pow = NULL,
									 .mod = NULL,
									 .unary_minus = NULL,
									 .unary_not = NULL,
									 .print = function_print,
									 .to_str = NULL,
									 .item_get = NULL,
									 .item_assign = NULL,
									 .item_in = NULL,
									 .truthy = NULL,
									 .eq = NULL,
									 .cmp = NULL,
									 .hash = NULL,
									 .obj_size = 0 };

SlashTypeInfo map_type_info = { .name = "map",
								.plus = NULL,
								.minus = NULL,
								.mul = NULL,
								.div = NULL,
								.int_div = NULL,
								.pow = NULL,
								.mod = NULL,
								.unary_minus = NULL,
								.unary_not = map_unary_not,
								.print = map_print,
								.to_str = NULL,
								.item_get = map_item_get,
								.item_assign = map_item_assign,
								.item_in = map_item_in,
								.truthy = map_truthy,
								.eq = map_eq,
								.cmp = NULL,
								.hash = NULL,
								.obj_size = sizeof(SlashMap) };

SlashTypeInfo list_type_info = { .name = "list",
								 .plus = list_plus,
								 .minus = NULL,
								 .mul = NULL,
								 .div = NULL,
								 .int_div = NULL,
								 .pow = NULL,
								 .mod = NULL,
								 .unary_minus = NULL,
								 .unary_not = list_unary_not,
								 .print = list_print,
								 .to_str = NULL,
								 .item_get = list_item_get,
								 .item_assign = list_item_assign,
								 .item_in = list_item_in,
								 .truthy = list_truthy,
								 .eq = list_eq,
								 .cmp = NULL,
								 .hash = NULL,
								 .obj_size = sizeof(SlashList) };

SlashTypeInfo tuple_type_info = { .name = "tuple",
								  .plus = tuple_plus,
								  .minus = NULL,
								  .mul = NULL,
								  .div = NULL,
								  .int_div = NULL,
								  .pow = NULL,
								  .mod = NULL,
								  .unary_minus = NULL,
								  .unary_not = tuple_unary_not,
								  .print = tuple_print,
								  .to_str = NULL,
								  .item_get = tuple_item_get,
								  .item_assign = NULL,
								  .item_in = tuple_item_in,
								  .truthy = tuple_truthy,
								  .eq = tuple_eq,
								  .cmp = NULL,
								  .hash = tuple_hash,
								  .obj_size = sizeof(SlashTuple) };

SlashTypeInfo str_type_info = { .name = "str",
								.plus = str_plus,
								.minus = NULL,
								.mul = NULL,
								.div = NULL,
								.int_div = NULL,
								.pow = NULL,
								.mod = NULL,
								.unary_minus = NULL,
								.unary_not = str_unary_not,
								.print = str_print,
								.to_str = str_to_str,
								.item_get = str_item_get,
								.item_assign = str_item_assign,
								.item_in = str_item_in,
								.truthy = str_truthy,
								.eq = str_eq,
								.cmp = str_cmp,
								.hash = str_hash,
								.obj_size = sizeof(SlashStr) };

SlashTypeInfo none_type_info = { .name = "none",
								 .plus = NULL,
								 .minus = NULL,
								 .mul = NULL,
								 .div = NULL,
								 .int_div = NULL,
								 .pow = NULL,
								 .mod = NULL,
								 .unary_minus = NULL,
								 .unary_not = NULL,
								 .print = none_print,
								 .to_str = none_to_str,
								 .item_get = NULL,
								 .item_assign = NULL,
								 .item_in = NULL,
								 .truthy = none_truthy,
								 .eq = none_eq,
								 .cmp = NULL,
								 .hash = NULL,
								 .obj_size = 0 };


SlashValue NoneSingleton = { .T = &none_type_info };
