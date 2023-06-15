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
#include "common.h"
#include "interpreter/types/slash_str.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include "sac/sac.h"


void scope_init(Scope *scope, Scope *enclosing)
{
    // TODO: capacity could be larger
    if (enclosing == NULL) {
	m_arena_init_dynamic(&scope->value_arena, 1, 16384);
    } else {
	m_arena_init_dynamic(&scope->value_arena, 1, 4096);
    }

    scope->enclosing = enclosing;
    hashmap_init(&scope->values);
}

void scope_destroy(Scope *scope)
{
    m_arena_release(&scope->value_arena);
    hashmap_free(&scope->values);
}

void *scope_alloc(Scope *scope, size_t size)
{
    return m_arena_alloc(&scope->value_arena, size);
}

void var_define(Scope *scope, SlashStr *key, SlashValue *value)
{
    // TODO: YOLO conversion to uint32_t
    hashmap_put(&scope->values, key->p, (uint32_t)key->size, value, sizeof(SlashValue), true);
}

void var_undefine(Scope *scope, SlashStr *key)
{
    hashmap_rm(&scope->values, key->p, (uint32_t)key->size);
}

void var_assign(Scope *scope, SlashStr *key, SlashValue *value)
{
    do {
	SlashValue *current = hashmap_get(&scope->values, key->p, (uint32_t)key->size);
	if (current != NULL) {
	    hashmap_put(&scope->values, key->p, (uint32_t)key->size, value, sizeof(SlashValue),
			true);
	    return;
	}
	scope = scope->enclosing;
    } while (scope != NULL);

    slash_exit_interpreter_err("cannot assign a variable that is not defined");
}

Scope *get_scope_of_var(Scope *scope, SlashStr *key)
{
    do {
	SlashValue *current = hashmap_get(&scope->values, key->p, (uint32_t)key->size);
	if (current != NULL) {
	    return scope;
	}
	scope = scope->enclosing;
    } while (scope != NULL);

    return NULL;
}

ScopeAndValue var_get(Scope *scope, SlashStr *key)
{
    SlashValue *value = NULL;
    do {
	value = hashmap_get(&scope->values, key->p, (uint32_t)key->size);
	if (value != NULL)
	    return (ScopeAndValue){ .scope = scope, .value = value };
	scope = scope->enclosing;
    } while (value == NULL && scope != NULL);

    return (ScopeAndValue){ .scope = NULL, .value = NULL };
}
