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
    return (SlashValue){ .T_info = &num_type_info, .boolean = !is_truthy(self) };
}

void num_print(SlashValue self)
{
    assert(IS_NUM(self));
    if (self.num == (int)self.num)
	printf("%d", (int)self.num);
    else
	printf("%f", self.num);
}

/*
 * range impl
 */

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
				 .to_str = NULL,
				 .item_get = NULL,
				 .item_assign = NULL,
				 .item_in = NULL,
				 .truthy = bool_truthy,
				 .eq = bool_eq,
				 .cmp = bool_cmp,
				 .hash = bool_hash,
				 .init = NULL,
				 .free = NULL };

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
				.to_str = NULL,
				.item_get = NULL,
				.item_assign = NULL,
				.item_in = NULL,
				.truthy = NULL,
				.eq = NULL,
				.cmp = NULL,
				.hash = NULL,
				.init = NULL,
				.free = NULL };

SlashTypeInfo range_type_info = { .name = "range" };
SlashTypeInfo map_type_info = { .name = "map" };
SlashTypeInfo list_type_info = { .name = "list" };
SlashTypeInfo tuple_type_info = { .name = "tuple" };
SlashTypeInfo str_type_info = { .name = "str" };
