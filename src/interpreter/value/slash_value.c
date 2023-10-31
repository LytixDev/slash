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
#include "interpreter/value/slash_value.h"


/*
 * bool impl
 */
SlashValue bool_unary_not(SlashValue self)
{
    assert(IS_BOOL(self));
    return (SlashValue){ .T_info = &bool_type_info, .boolean = !self.boolean };
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
SlashValue num_plus(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    // TODO: check for overflow and other undefined behaviour. Same for all arithmetic operators.
    return (SlashValue){ .T_info = &num_type_info, .num = self.num + other.num };
}

SlashValue num_minus(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    return (SlashValue){ .T_info = &num_type_info, .num = self.num - other.num };
}

SlashValue num_mul(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    return (SlashValue){ .T_info = &num_type_info, .num = self.num * other.num };
}

SlashValue num_div(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    if (other.num == 0)
	REPORT_RUNTIME_ERROR("Division by zero error");
    return (SlashValue){ .T_info = &num_type_info, .num = self.num / other.num };
}

SlashValue num_int_div(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    if (other.num == 0)
	REPORT_RUNTIME_ERROR("Division by zero error");
    return (SlashValue){ .T_info = &num_type_info, .num = (int)(self.num / other.num) };
}

SlashValue num_pow(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    return (SlashValue){ .T_info = &num_type_info, .num = pow(self.num, other.num) };
}

SlashValue num_mod(SlashValue self, SlashValue other)
{
    assert(IS_NUM(self) && IS_NUM(other));
    if (other.num == 0)
	REPORT_RUNTIME_ERROR("Modulo by zero error");
    double m = fmod(self.num, other.num);
    /* same behaviour as we tend to see in maths */
    m = m < 0 && other.num > 0 ? m + other.num : m;
    return (SlashValue){ .T_info = &num_type_info, .num = m };
}

SlashValue num_unary_minus(SlashValue self)
{
    assert(IS_NUM(self));
    return (SlashValue){ .T_info = &num_type_info, .num = -self.num };
}

SlashValue num_unary_not(SlashValue self)
{
    assert(IS_NUM(self));
    TraitTruthy is_truthy = self.T_info->truthy;
    assert(is_truthy != NULL);
    return (SlashValue){ .T_info = &bool_type_info, .boolean = !is_truthy(self) };
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
    return self.num > other.num;
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
	size_t range_size = self.range.start - self.range.end;
	if (idx >= range_size)
	    REPORT_RUNTIME_ERROR(
		"Range index out of range. Has size '%zu', tried to get item at index '%zu'",
		range_size, idx);

	int offset = self.range.start + idx;
	return (SlashValue){ .T_info = &num_type_info, .num = offset };
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

/*
 * list impl
 */

/*
 * tuple impl
 */

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

void str_print(SlashValue self)
{
    assert(IS_STR(self));
    printf("%s", AS_STR(self)->str);
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
				.unary_not = NULL,
				.print = NULL,
				.to_str = NULL,
				.item_get = NULL,
				.item_assign = NULL,
				.item_in = NULL,
				.truthy = NULL,
				.eq = NULL,
				.cmp = NULL,
				.hash = NULL,
				.init = NULL,
				.free = NULL,
				.obj_size = sizeof(SlashMap) };

SlashTypeInfo list_type_info = { .name = "list",
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
				 .to_str = NULL,
				 .item_get = NULL,
				 .item_assign = NULL,
				 .item_in = NULL,
				 .truthy = NULL,
				 .eq = NULL,
				 .cmp = NULL,
				 .hash = NULL,
				 .init = NULL,
				 .free = NULL,
				 .obj_size = sizeof(SlashList) };

SlashTypeInfo tuple_type_info = { .name = "tuple",
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
				  .to_str = NULL,
				  .item_get = NULL,
				  .item_assign = NULL,
				  .item_in = NULL,
				  .truthy = NULL,
				  .eq = NULL,
				  .cmp = NULL,
				  .hash = NULL,
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
				.truthy = NULL,
				.eq = NULL,
				.cmp = NULL,
				.hash = NULL,
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
				 .truthy = NULL,
				 .eq = NULL,
				 .cmp = NULL,
				 .hash = NULL,
				 .init = NULL,
				 .free = NULL,
				 .obj_size = 0 };


SlashValue NoneSingleton = { .T_info = &none_type_info };
