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
///#include "interpreter/error.h"
///#include "interpreter/interpreter.h"
///#include "interpreter/scope.h"
///#include "interpreter/types/slash_obj.h"
///#include "interpreter/types/slash_str.h"
///#include "interpreter/types/slash_value.h"
///
///
///SlashValue dynamic_cast(Interpreter *interpreter, SlashValue value, SlashType new_type)
///{
///    (void)interpreter;
///    if (value.type == new_type)
///	return value;
///    /* str -> num */
///    if (value.type == SLASH_OBJ && value.obj->type == SLASH_OBJ_STR && new_type == SLASH_NUM) {
///	SlashStr *str = (SlashStr *)value.obj;
///	return (SlashValue){ .type = SLASH_NUM, .num = strtod(str->p, NULL) };
///    }
///
///    REPORT_RUNTIME_ERROR("Cast not supported");
///    return (SlashValue){ 0 };
///}
