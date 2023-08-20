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

#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"


void slash_map_init(Scope *scope, SlashMap *map)
{
    map->underlying = malloc(sizeof(HashMap));
    hashmap_init(map->underlying);
    scope_register_owning(scope, &(SlashValue){ .type = SLASH_MAP, .map = *map });
}

void slash_map_free(SlashMap *map)
{
    hashmap_free(map->underlying);
    free(map->underlying);
}

void slash_map_put(SlashMap *map, SlashValue *key, SlashValue *value)
{
    hashmap_put(map->underlying, key, sizeof(SlashValue), value, sizeof(SlashValue), true);
}

SlashValue *slash_map_get(SlashMap *map, SlashValue *key)
{
    SlashValue *value = hashmap_get(map->underlying, key, sizeof(SlashValue));
    if (value == NULL)
	return &slash_glob_none;
    return value;
}


/* common slash value functions */
void slash_map_print(SlashValue *value)
{
    printf("@[");
    HashMap *m = value->map.underlying;
    SlashValue *k, *v;
    struct hm_bucket_t *bucket;
    struct hm_entry_t entry;

    bool first_print = true;

    for (int i = 0; i < N_BUCKETS(m->size_log2); i++) {
	bucket = &m->buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    if (!first_print)
		printf(", ");
	    first_print = false;

	    k = entry.key;
	    v = entry.value;
	    slash_print[k->type](k);
	    printf(": ");
	    slash_print[v->type](v);
	}
    }
    printf("]");
}

size_t *slash_map_len(SlashValue *value)
{
    return (size_t *)&value->map.underlying->len;
}

SlashValue slash_map_item_get(Scope *scope, SlashValue *self, SlashValue *index)
{
    (void)scope;
    assert(self->type == SLASH_MAP);

    return *slash_map_get(&self->map, index);
}

void slash_map_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value)
{
    assert(self->type == SLASH_MAP);
    slash_map_put(&self->map, index, new_value);
}

bool slash_map_item_in(SlashValue *self, SlashValue *item)
{
    assert(self->type == SLASH_MAP);
    /* "in" means that the item is a key in the dict */

    SlashMap map = self->map;
    SlashValue *value = hashmap_get(map.underlying, item, sizeof(SlashValue));
    return value != NULL;
}


/* methods */
SlashTuple slash_map_get_keys(Scope *scope, SlashMap *map)
{
    SlashTuple map_keys;
    slash_tuple_init(scope, &map_keys, map->underlying->len);

    HashMap *m = map->underlying;
    struct hm_bucket_t *bucket;
    struct hm_entry_t entry;
    size_t counter = 0;

    for (int i = 0; i < N_BUCKETS(m->size_log2); i++) {
	bucket = &m->buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    map_keys.values[counter++] = *(SlashValue *)entry.key;
	}
    }

    return map_keys;
}
