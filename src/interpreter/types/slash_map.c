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

#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


ObjTraits map_traits = { .print = slash_map_print,
			 .to_str = NULL,
			 .item_get = slash_map_item_get,
			 .item_assign = slash_map_item_assign,
			 .item_in = slash_map_item_in,
			 .truthy = slash_map_truthy,
			 .equals = slash_map_eq,
			 .cmp = NULL,
			 .hash = NULL };


void slash_map_init(SlashMap *map)
{
    hashmap_init(&map->underlying);
    map->obj.traits = &map_traits;
}

static void slash_map_put(SlashMap *map, SlashValue *key, SlashValue *value)
{
    hashmap_put(&map->underlying, key, sizeof(SlashValue), value, sizeof(SlashValue), true);
}

static SlashValue *slash_map_get(SlashMap *map, SlashValue *key)
{
    SlashValue *value = hashmap_get(&map->underlying, key, sizeof(SlashValue));
    if (value == NULL)
	return &slash_glob_none;
    return value;
}


/*
 * traits
 */
void slash_map_print(SlashValue *value)
{
    assert(value->type == SLASH_OBJ);
    assert(value->obj->type == SLASH_OBJ_MAP);

    printf("@[");
    HashMap m = ((SlashMap *)value->obj)->underlying;
    SlashValue *k, *v;
    struct hm_bucket_t *bucket;
    struct hm_entry_t entry;

    bool first_print = true;

    for (int i = 0; i < N_BUCKETS(m.size_log2); i++) {
	bucket = &m.buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    if (!first_print)
		printf(", ");
	    first_print = false;

	    k = entry.key;
	    v = entry.value;
	    trait_print[k->type](k);
	    printf(": ");
	    trait_print[v->type](v);
	}
    }
    printf("]");
}

SlashValue slash_map_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *index)
{
    (void)interpreter;
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_MAP);

    SlashMap *map = (SlashMap *)self->obj;
    return *slash_map_get(map, index);
}

void slash_map_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value)
{
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_MAP);
    SlashMap *map = (SlashMap *)self->obj;
    slash_map_put(map, index, new_value);
}

bool slash_map_item_in(SlashValue *self, SlashValue *item)
{
    assert(self->type == SLASH_OBJ);
    assert(self->obj->type == SLASH_OBJ_MAP);
    /* "in" means that the item is a key in the dict */
    SlashMap *map = (SlashMap *)self->obj;
    SlashValue *value = hashmap_get(&map->underlying, item, sizeof(SlashValue));
    return value != NULL;
}

bool slash_map_truthy(SlashValue *self)
{
    SlashMap *map = (SlashMap *)self->obj;
    return map->underlying.len != 0;
}

bool slash_map_eq(SlashValue *a, SlashValue *b)
{
    assert(a->type == SLASH_OBJ);
    assert(a->obj->type == SLASH_OBJ_MAP);
    assert(b->type == SLASH_OBJ);
    assert(b->obj->type == SLASH_OBJ_MAP);

    HashMap *A = &((SlashMap *)a->obj)->underlying;
    HashMap *B = &((SlashMap *)b->obj)->underlying;
    if (A->len != B->len)
	return false;

    // TODO: loop over all keys and check if associated values are equal
    return true;
}


/*
 * methods
 */
SlashMethod slash_map_methods[SLASH_MAP_METHODS_COUNT] = {
    { .name = "keys", .fp = slash_map_get_keys_method_stub }
};

SlashValue slash_map_get_keys_method_stub(Interpreter *interpreter, SlashValue *self, size_t argc,
					  SlashValue *argv)
{
    (void)argc;
    (void)argv;
    return slash_map_get_keys(interpreter, (SlashMap *)self->obj);
}

SlashValue slash_map_get_keys(Interpreter *interpreter, SlashMap *map)
{
    SlashTuple *map_keys = (SlashTuple *)gc_alloc(interpreter, SLASH_OBJ_TUPLE);
    SlashValue value = { .type = SLASH_OBJ, .obj = (SlashObj *)map_keys };
    slash_tuple_init(map_keys, map->underlying.len);
    if (map_keys->size == 0)
	return value;

    HashMap *m = &map->underlying;
    struct hm_bucket_t *bucket;
    struct hm_entry_t entry;
    size_t counter = 0;

    for (int i = 0; i < N_BUCKETS(m->size_log2); i++) {
	bucket = &m->buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    map_keys->values[counter++] = *(SlashValue *)entry.key;
	}
    }

    return value;
}
