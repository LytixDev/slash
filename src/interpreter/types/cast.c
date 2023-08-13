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

#include "common.h"
#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value.h"


// TODO: support more casts and maybe rework the logic
SlashValue dynamic_cast(Interpreter *interpreter, SlashValue value, SlashType new_type)
{
    if (value.type == new_type)
	return value;
    /* str -> num */
    if (value.type == SLASH_STR && new_type == SLASH_NUM)
	return (SlashValue){ .type = SLASH_NUM, .num = str_view_to_double(value.str) };
    /* num -> str */
    if (value.type == SLASH_NUM && new_type == SLASH_STR) {
	StrView view = { .view = scope_alloc(interpreter->scope, 64), .size = 64 };
	snprintf(view.view, 64, "%f", value.num);
	return (SlashValue){ .type = SLASH_STR, .str = view };
    }

    slash_exit_interpreter_err("cast not supported");
    return (SlashValue){ 0 };
}
