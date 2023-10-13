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
#include <stdio.h>

#include "interpreter/scope.h"
#include "interpreter/types/slash_str.h"
#include "interpreter/types/slash_value.h"
#include "lib/str_view.h"
#include "nicc/nicc.h"
#include "sac/sac.h"

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

static void define_scope_alloced_var(Scope *scope, StrView *key, char *cstr)
{
    SlashStr *str = scope_alloc(scope, sizeof(SlashStr));
    str->obj.gc_managed = false;
    str->obj.type = SLASH_OBJ_STR;
    slash_str_init_from_alloced_cstr(str, cstr);
    SlashValue value = { .type = SLASH_OBJ, .obj = (SlashObj *)str };
    var_define(scope, key, &value);
}


static void set_globals(Scope *scope)
{
    assert(scope->enclosing == NULL);

    char *env_entry;
    char **environ_cpy = environ;
    for (env_entry = *environ_cpy; env_entry != NULL; env_entry = *(++environ_cpy)) {
	int pos = get_char_pos(env_entry, '=');
	StrView key = { .view = env_entry, .size = pos };
	define_scope_alloced_var(scope, &key, env_entry + pos + 1);
    }

    StrView ifs_key = { .view = "IFS", .size = 3 };
    char *ifs_cstr = scope_alloc(scope, 3);
    memcpy(ifs_cstr, "\n\t ", 3);
    define_scope_alloced_var(scope, &ifs_key, ifs_cstr);
}

static void scope_init_argv(Scope *scope, int argc, char **argv)
{
    for (size_t i = 0; i < (size_t)argc; i++) {
	char *value = argv[i];
	char key[32];
	size_t size = sprintf(key, "%zu", i);
	define_scope_alloced_var(scope, &(StrView){ .view = key, .size = size }, value);
    }
}

void scope_init_globals(Scope *scope, Arena *arena, int argc, char **argv)
{
    scope->arena_tmp = m_arena_tmp_init(arena);
    scope->enclosing = NULL;
    scope->depth = 0;
    hashmap_init(&scope->values);
    set_globals(scope);
    scope_init_argv(scope, argc, argv);
    /* Define '$?' that holds the value of the previous exit code */
    SlashValue exit_code_value = { .type = SLASH_NUM, .num = 0 };
    var_define(scope, &(StrView){ .view = "?", .size = 1 }, &exit_code_value);
}

void scope_init(Scope *scope, Scope *enclosing)
{
    scope->arena_tmp = m_arena_tmp_init(enclosing->arena_tmp.arena);
    scope->enclosing = enclosing;
    scope->depth = enclosing->depth + 1;
    hashmap_init(&scope->values);
}

void scope_destroy(Scope *scope)
{
    hashmap_free(&scope->values);
    m_arena_tmp_release(scope->arena_tmp);
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

void var_assign(StrView *var_name, Scope *scope, SlashValue *value)
{
    hashmap_put(&scope->values, var_name->view, (uint32_t)var_name->size, value, sizeof(SlashValue),
		true);
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
