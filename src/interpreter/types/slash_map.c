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
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"
#include <stdio.h>

void slash_map_init(SlashMap *map)
{
    hashmap_init(&map->underlying);
}

void slash_map_put(SlashMap *map, SlashValue *key, SlashValue *value)
{
    hashmap_put(&map->underlying, key, sizeof(SlashValue), value, sizeof(SlashValue), true);
}

SlashValue *slash_map_get(SlashMap *map, SlashValue *key)
{
    return hashmap_get(&map->underlying, key, sizeof(SlashValue));
}

void slash_map_print(SlashValue *value)
{
    printf("[[");
    HashMap *m = &value->map.underlying;
    SlashValue *k, *v;
    struct hm_bucket_t *bucket;
    struct hm_entry_t entry;

    for (int i = 0; i < N_BUCKETS(m->size_log2); i++) {
	bucket = &m->buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    k = entry.key;
	    v = entry.value;
	    slash_print[k->type](k);
	    putchar(':');
	    slash_print[v->type](v);
	    printf(", ");
	}
    }
    printf("]]");
}

size_t *slash_map_len(SlashValue *value)
{
    return (size_t *)&value->map.underlying.len;
}
