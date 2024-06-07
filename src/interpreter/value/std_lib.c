/*
 *  Copyright (C) 2024 Nicolai Brand (https://lytix.dev)
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
#include <string.h>

#include "interpreter/scope.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/std_lib.h"


static SlashValue impl_typeof(Interpreter *interpreter)
{
    // assume first parameters is called FIRST
    ScopeAndValue sav = var_get(interpreter->scope, &TO_STR_VIEW("FIRST"));
    assert(sav.value != NULL);

    SlashValue *v = sav.value;
    char *type_name = v->T->name;
    SlashStr *s = (SlashStr *)gc_new_T(interpreter, &str_type_info);
    slash_str_init_from_view(interpreter, s,
			     &(StrView){ .view = type_name, .size = strlen(type_name) });
    return AS_VALUE(s);
}

SlashValue lib_typeof(Interpreter *interpreter)
{
    ArenaLL params = { 0 };
    arena_ll_init(&interpreter->arena, &params);
    StrView *view = m_arena_alloc_struct(&interpreter->arena, StrView);
    *view = TO_STR_VIEW("FIRST");
    arena_ll_append(&params, view);
    SlashFunction func = { .params = params, .is_lib_function = true, .lib_func = impl_typeof };
    return (SlashValue){ .T = &function_type_info, .function = func };
}
