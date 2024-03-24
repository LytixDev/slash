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
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "lib/str_view.h"


SlashValue dynamic_cast(Interpreter *interpreter, SlashValue value, StrView type_name)
{
    (void)interpreter;
    SlashTypeInfo *new_T = hashmap_get(&interpreter->type_register, type_name.view, type_name.size);
    /* Casting to the same type as value already is does nothing */
    if (new_T == value.T)
	return value;

    if (new_T == &str_type_info) {
	VERIFY_TRAIT_IMPL(
	    to_str, value,
	    "Could not cast to 'str' because type '%s' does not implement the to_str trait",
	    value.T->name);
	return value.T->to_str(interpreter, value);
    }
    if (new_T == &num_type_info) {
	if (value.T != &str_type_info)
	    REPORT_RUNTIME_ERROR("Cast from '%s' to num is not supported ... yet! Please help :-)",
				 value.T->name);
	return (SlashValue){ .T = &num_type_info, .num = strtod(AS_STR(value)->str, NULL) };
    }

    REPORT_RUNTIME_ERROR("Cast not supported ... yet! Please help :-)");
    return (SlashValue){ 0 };
}
