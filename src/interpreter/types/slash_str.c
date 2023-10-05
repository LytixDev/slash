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
    str->cap = view->size + 1;
    str->len = str->cap;
    str->p = malloc(str->cap);
    memcpy(str->p, view->view, str->cap - 1);
    str->p[str->cap] = 0;

    str->obj.traits = &str_traits;
}

/*
 * traits
 */
void slash_str_print(SlashValue *value)
{
    SlashStr *str = (SlashStr *)value->obj;
    printf("%s", str->p);
}
