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

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_value.h"


/*
 * bool impl
 */
SlashValue bool_unary_not(SlashValue self)
{
    assert(IS_BOOL(self));
    return (SlashValue){ .T = &bool_type_info, .boolean = !self.boolean };
}

void bool_print(SlashValue self)
{
    assert(IS_BOOL(self));
    printf("%s", self.boolean == true ? "true" : "false");
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
	REPORT_RUNTIME_ERROR("Division by zero error");
    return (SlashValue){ .T = &num_type_info, .num = self.num / other.num };
}

SlashValue num_int_div(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    if (other.num == 0)
	REPORT_RUNTIME_ERROR("Division by zero error");
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
	REPORT_RUNTIME_ERROR("Modulo by zero error");
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
    assert(is_truthy != NULL);
    return (SlashValue){ .T = &bool_type_info, .boolean = !is_truthy(self) };
}

void num_print(SlashValue self)
{
    assert(IS_NUM(self));
    if (self.num == (int)self.num)
	printf("%d", (int)self.num);
    else
	printf("%f", self.num);
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
void range_print(SlashValue self)
{
    assert(IS_RANGE(self));
    printf("%d -> %d", self.range.start, self.range.end);
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
    SlashObj *str = gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_view(interpreter, (SlashStr *)str, &self.text_lit);
    return AS_VALUE(str);
}


/*
 * map impl
 */
SlashValue map_unary_not(SlashValue self)
{
    assert(IS_MAP(self));
    return (SlashValue){ .T = &bool_type_info, .boolean = !self.T->truthy(self) };
}

void map_print(SlashValue self)
{
    assert(IS_MAP(self));
    slash_map_impl_print(*AS_MAP(self));
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
    gc_shadow_push(&interpreter->gc, &new_list->obj);
    slash_list_impl_init(interpreter, new_list);

    SlashList *a = AS_LIST(self);
    for (size_t i = 0; i < a->len; i++)
	slash_list_impl_append(interpreter, new_list, a->items[i]);

    SlashList *b = AS_LIST(other);
    for (size_t i = 0; i < b->len; i++)
	slash_list_impl_append(interpreter, new_list, b->items[i]);

    gc_shadow_pop(&interpreter->gc);
    return AS_VALUE(new_list);
}

SlashValue list_unary_not(SlashValue self)
{
    assert(IS_LIST(self));
    return (SlashValue){ .T = &bool_type_info, .boolean = !self.T->truthy(self) };
}

void list_print(SlashValue self)
{
    assert(IS_LIST(self));
    SlashList *underlying = AS_LIST(self);
    putchar('[');
    for (size_t i = 0; i < underlying->len; i++) {
	SlashValue item = underlying->items[i];
	assert(item.T->print != NULL);
	item.T->print(item);
	if (i != underlying->len - 1)
	    printf(", ");
    }
    putchar(']');
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

void tuple_print(SlashValue self)
{
    assert(IS_TUPLE(self));
    SlashTuple *tuple = AS_TUPLE(self);
    putchar('(');
    for (size_t i = 0; i < tuple->len; i++) {
	SlashValue this = tuple->items[i];
	this.T->print(this);
	if (i != tuple->len - 1 || i == 0)
	    printf(", ");
    }
    putchar(')');
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

void str_print(SlashValue self)
{
    assert(IS_STR(self));
    printf("\"%s\"", AS_STR(self)->str);
}

bool str_truthy(SlashValue self)
{
    assert(IS_STR(self));
    return AS_STR(self)->len != 0;
}

bool str_eq(SlashValue self, SlashValue other)
{
    assert(IS_STR(self) && IS_STR(other));
    // TODO: we can do better
    SlashStr *a = AS_STR(self);
    SlashStr *b = AS_STR(other);
    return strcmp(a->str, b->str) == 0;
}

int str_hash(SlashValue self)
{
    assert(IS_STR(self));
    SlashStr *str = AS_STR(self);

    int A = 1327217885;
    int k = 5381;
    for (size_t i = 0; i < str->len; i++)
	k += ((k << 5) + k) + (str->str)[i];

    return k * A;
}


/*
 * none impl
 */
void none_print(SlashValue self)
{
    (void)self;
    assert(IS_NONE(self));
    printf("none");
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
				 .init = NULL,
				 .free = NULL,
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
				.init = NULL,
				.free = NULL,
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
				  .init = NULL,
				  .free = NULL,
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
				     .init = NULL,
				     .free = NULL,
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
				.init = NULL,
				.free = NULL,
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
				 .init = NULL,
				 .free = NULL,
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
				  .init = NULL,
				  .free = NULL,
				  .obj_size = sizeof(SlashTuple) };

SlashTypeInfo str_type_info = { .name = "str",
				.plus = NULL,
				.minus = NULL,
				.mul = NULL,
				.div = NULL,
				.int_div = NULL,
				.pow = NULL,
				.mod = NULL,
				.unary_minus = NULL,
				.unary_not = NULL,
				.print = str_print,
				.to_str = NULL,
				.item_get = NULL,
				.item_assign = NULL,
				.item_in = NULL,
				.truthy = str_truthy,
				.eq = str_eq,
				.cmp = NULL,
				.hash = str_hash,
				.init = NULL,
				.free = NULL,
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
				 .to_str = NULL,
				 .item_get = NULL,
				 .item_assign = NULL,
				 .item_in = NULL,
				 .truthy = none_truthy,
				 .eq = none_eq,
				 .cmp = NULL,
				 .hash = NULL,
				 .init = NULL,
				 .free = NULL,
				 .obj_size = 0 };


SlashValue NoneSingleton = { .T = &none_type_info };
