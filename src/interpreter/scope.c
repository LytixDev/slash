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
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include "sac/sac.h"
#include "str_view.h"


void scope_init_global(Scope *scope, Arena *arena)
{
    scope->arena_tmp = m_arena_tmp_init(arena);
    scope->enclosing = NULL;
    hashmap_init(&scope->values);
}

void scope_init(Scope *scope, Scope *enclosing)
{
    scope->arena_tmp = m_arena_tmp_init(enclosing->arena_tmp.arena);
    scope->enclosing = enclosing;
    hashmap_init(&scope->values);
}

void scope_destroy(Scope *scope)
{
    m_arena_tmp_release(scope->arena_tmp);
    hashmap_free(&scope->values);
}

void *scope_alloc(Scope *scope, size_t size)
{
    return m_arena_alloc(scope->arena_tmp.arena, size);
}

void var_define(Scope *scope, StrView *key, SlashValue *value)
{
    hashmap_put(&scope->values, key->view, (uint32_t)key->size, value, sizeof(SlashValue), true);
}

void var_undefine(Scope *scope, StrView *key)
{
    hashmap_rm(&scope->values, key->view, (uint32_t)key->size);
}

void var_assign(Scope *scope, StrView *key, SlashValue *value)
{
    do {
	SlashValue *current = hashmap_get(&scope->values, key->view, (uint32_t)key->size);
	if (current != NULL) {
	    hashmap_put(&scope->values, key->view, (uint32_t)key->size, value, sizeof(SlashValue),
			true);
	    return;
	}
	scope = scope->enclosing;
    } while (scope != NULL);

    slash_exit_interpreter_err("cannot assign a variable that is not defined");
}

Scope *get_scope_of_var(Scope *scope, StrView *key)
{
    do {
	SlashValue *current = hashmap_get(&scope->values, key->view, (uint32_t)key->size);
	if (current != NULL) {
	    return scope;
	}
	scope = scope->enclosing;
    } while (scope != NULL);

    return NULL;
}

ScopeAndValue var_get(Scope *scope, StrView *key)
{
    SlashValue *value = NULL;
    do {
	value = hashmap_get(&scope->values, key->view, (uint32_t)key->size);
	if (value != NULL)
	    return (ScopeAndValue){ .scope = scope, .value = value };
	scope = scope->enclosing;
    } while (value == NULL && scope != NULL);

    return (ScopeAndValue){ .scope = NULL, .value = NULL };
}
