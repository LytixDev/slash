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

#include "interpreter/scope.h"
#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "nicc/nicc.h"


// TODO: arena
void scope_init(Scope *scope, Scope *enclosing)
{
    scope->enclosing = enclosing;
    hashmap_init(&scope->values);
}

// TODO: this is a hack because my original hashmap is not very flexible it turns out
//       anyways, it should be be in this file
//       creat environment abstraction
void var_set(Scope *scope, SlashStr *key, SlashValue *value)
{
    // YOLO conversion to uint32_t
    hashmap_put(&scope->values, key->p, (uint32_t)key->size, value, sizeof(SlashValue), true);
}

SlashValue var_get(Scope *scope, SlashStr *key)
{
    SlashValue *value = NULL;
    do {
	value = hashmap_get(&scope->values, key->p, (uint32_t)key->size);
	if (value != NULL)
	    return *value;
	scope = scope->enclosing;
    } while (value == NULL && scope != NULL);

    return (SlashValue){ .p = NULL, .type = SVT_NONE };
}
