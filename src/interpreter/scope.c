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
#include <assert.h>

#include "common.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include "sac/sac.h"
#include "str_view.h"

extern char **environ;


static int get_char_pos(char *str, char m)
{
    int i = 0;
    while (*str != 0) {
	if (*str == m)
	    return i;
	i++;
	str++;
    }
    return -1;
}

static void set_env_as_var(Scope *scope, char *env_entry)
{
    int pos = get_char_pos(env_entry, '=');
    StrView key = { .view = env_entry, .size = pos };
    SlashValue value = { .type = SLASH_STR,
			 .str = (StrView){ .view = env_entry + pos + 1,
					   .size = strlen(env_entry) - pos } };
    var_define(scope, &key, &value);
}

static void set_globals(Scope *scope)
{
    assert(scope->enclosing == NULL);

    char *item;
    char **environ_cpy = environ;
    for (item = *environ_cpy; item != NULL; item = *(++environ_cpy))
	set_env_as_var(scope, item);

    StrView ifs = { .view = "IFS", .size = 3 };
    SlashValue ifs_value = { .type = SLASH_STR, .str = (StrView){ .view = "\n\t ", .size = 3 } };
    var_define(scope, &ifs, &ifs_value);
}

void scope_init_global(Scope *scope, Arena *arena)
{
    scope->arena_tmp = m_arena_tmp_init(arena);
    scope->enclosing = NULL;
    scope->depth = 0;
    hashmap_init(&scope->values);
    arraylist_init(&scope->owning, sizeof(SlashValue));
    set_globals(scope);
}

void scope_init(Scope *scope, Scope *enclosing)
{
    scope->arena_tmp = m_arena_tmp_init(enclosing->arena_tmp.arena);
    scope->enclosing = enclosing;
    scope->depth = enclosing->depth + 1;
    hashmap_init(&scope->values);
    arraylist_init(&scope->owning, sizeof(SlashValue));
}

void scope_destroy(Scope *scope)
{
    /* free all owning references */
    struct linkedlist_item_t *item;
    for (size_t i = 0; i < scope->owning.size; i++) {
	SlashValue *sv = arraylist_get(&scope->owning, i);
	assert(SLASH_TYPE_DYNAMIC(sv->type));

	if (sv->type == SLASH_LIST)
	    slash_list_free(&sv->list);
	else if (sv->type == SLASH_MAP)
	    slash_map_free(&sv->map);
	else
	    slash_tuple_free(&sv->tuple);
    }
    arraylist_free(&scope->owning);
    hashmap_free(&scope->values);
    m_arena_tmp_release(scope->arena_tmp);
}

void scope_register_owning(Scope *scope, SlashValue *sv)
{
    arraylist_append(&scope->owning, sv);
}

void scope_transfer_owning(Scope *owner, Scope *new_owner, SlashValue *sv)
{
    /* remove sv from current owner */
    for (size_t i = 0; i < owner->owning.size; i++) {
	if (nicc_data_eq(arraylist_get(&owner->owning, i), sv, owner->owning.T_size)) {
	    arraylist_rm(&owner->owning, i);
	    break;
	}
    }

    scope_register_owning(new_owner, sv);
}

void *scope_alloc(Scope *scope, size_t size)
{
    return m_arena_alloc(scope->arena_tmp.arena, size);
}

void var_define(Scope *scope, StrView *key, SlashValue *value)
{
    /* define value to SLASH_NONE */
    if (value == NULL) {
	SlashValue none = { .type = SLASH_NONE };
	hashmap_put(&scope->values, key->view, (uint32_t)key->size, &none, sizeof(SlashValue),
		    true);
	return;
    }
    hashmap_put(&scope->values, key->view, (uint32_t)key->size, value, sizeof(SlashValue), true);
}

void var_undefine(Scope *scope, StrView *key)
{
    hashmap_rm(&scope->values, key->view, (uint32_t)key->size);
}

void var_assign_simple(Scope *current, StrView *var_name, SlashValue *value)
{
    do {
	SlashValue *current_value =
	    hashmap_get(&current->values, var_name->view, (uint32_t)var_name->size);
	if (current_value != NULL) {
	    hashmap_put(&current->values, var_name->view, (uint32_t)var_name->size, value,
			sizeof(SlashValue), true);
	    return;
	}
	current = current->enclosing;
    } while (current != NULL);

    slash_exit_interpreter_err("cannot assign a variable that is not defined");
}

void var_assign(StrView *var_name, Scope *var_owner, Scope *current, SlashValue *value)
{
    /*
     * only need to transfer ownership for dynamic variables whos new value is allocated at a
     * deeper scope loc
     */
    if (!(SLASH_TYPE_DYNAMIC(value->type) && current->depth > var_owner->depth)) {
	var_assign_simple(current, var_name, value);
	return;
    }

    scope_transfer_owning(current, var_owner, value);
    hashmap_put(&var_owner->values, var_name->view, (uint32_t)var_name->size, value,
		sizeof(SlashValue), true);
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
