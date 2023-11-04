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
#include <stdbool.h>
#include <string.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/type_funcs.h"

/// typedef struct {
///     SlashValue key;
///     SlashValue value;
///     bool hash_extra; // used for faster comparison
///     bool is_occupied;
/// } SlashMapEntry;

static inline uint8_t map_hash_extra(int hash)
{
    return (uint8_t)(hash & ((1 << 8) - 1));
}

static SlashMapEntry *get_from_bucket(SlashMapBucket bucket, SlashValue key, uint8_t hash_extra)
{
    SlashMapEntry *entry = NULL;
    for (size_t i = 0; i < HM_BUCKET_SIZE; i++) {
	entry = &bucket.entries[i];
	if (entry->is_occupied && entry->hash_extra == hash_extra && TYPE_EQ(key, entry->key)) {
	    if (key.T_info->eq(key, entry->key))
		return entry;
	}
    }

    return NULL;
}

static int map_insert(SlashMapBucket *bucket, SlashValue key, SlashValue value, uint8_t hash_extra)
{
    /*
     * Hashmap implementation does not allow duplcate keys.
     * Therefore, we cannot simply find the first empty entry and set the new
     * entry here, we have to make sure the key we are inserting does not already
     * exist somewhere else in the bucket. If we find a matching key, we simply
     * override. If we do not find a matching entry, we insert the new entry in
     * the first found empty entry.
     */
    SlashMapEntry *found = NULL;
    bool override = false;
    for (size_t i = 0; i < HM_BUCKET_SIZE; i++) {
	SlashMapEntry *entry = &bucket->entries[i];
	/* Check if entry's key is equal to new key */
	if (entry->is_occupied && entry->hash_extra == hash_extra && TYPE_EQ(entry->key, key)) {
	    if (key.T_info->eq(key, entry->key)) {
		found = entry;
		override = true;
		break;
	    }
	}

	if (!entry->is_occupied && found == NULL)
	    found = entry;
    }

    if (found == NULL)
	return _HM_FULL;

    SlashMapEntry new_entry = {
	.key = key, .value = value, .hash_extra = hash_extra, .is_occupied = true
    };
    *found = new_entry;
    return override ? _HM_OVERRIDE : _HM_SUCCESS;
}

void slash_map_init(Interpreter *interpreter, SlashMapImpl *map)
{
    map->len = 0;
    map->total_buckets_log2 = HM_STARTING_BUCKETS_LOG2;
    size_t n_buckets = N_BUCKETS(map->total_buckets_log2);
    // TODO: should we have a gc_calloc() function ?
    map->buckets = gc_alloc(interpreter, sizeof(SlashMapBucket) * n_buckets);
    /*
     * Since we memset each bucket and therefore each entry to 0 the is_occupied field on
     * each entry will be set to false.
     */
    memset(map->buckets, 0, sizeof(SlashMapBucket) * n_buckets);
}

void slash_map_free(Interpreter *interpreter, SlashMapImpl *map)
{
    gc_free(interpreter, map->buckets, N_BUCKETS(map->total_buckets_log2) * sizeof(SlashMapBucket));
}

void slash_map_put(Interpreter *interpreter, SlashMapImpl *map, SlashValue key, SlashValue value)
{
    double load_factor = (double)map->len / (N_BUCKETS(map->total_buckets_log2) * HM_BUCKET_SIZE);
    if (load_factor >= 0.75) {
	assert(NULL && "Implement increase() func for map");
    }

    TraitHash hash_func = key.T_info->hash;
    if (hash_func == NULL)
	REPORT_RUNTIME_ERROR("Can not use type '%s' as key in map because type is unhashable.",
			     key.T_info->name);

    int hash = hash_func(key);
    uint8_t hash_extra = map_hash_extra(hash);
    int bucket_idx = hash >> (32 - map->total_buckets_log2);

    int rc = map_insert(&map->buckets[bucket_idx], key, value, hash_extra);
    if (rc == _HM_FULL) {
	assert(NULL && "Implement increase() func for map");
	slash_map_put(interpreter, map, key, value);
    }
    if (rc == _HM_SUCCESS)
	map->len++;
}

SlashValue slash_map_get(SlashMapImpl *map, SlashValue key)
{
    if (map->len == 0)
	return NoneSingleton;

    TraitHash hash_func = key.T_info->hash;
    if (hash_func == NULL)
	REPORT_RUNTIME_ERROR("Can not use type '%s' as key in map because type is unhashable.",
			     key.T_info->name);

    int hash = hash_func(key);
    uint8_t hash_extra = map_hash_extra(hash);
    int bucket_idx = hash >> (32 - map->total_buckets_log2);

    SlashMapEntry *entry = get_from_bucket(map->buckets[bucket_idx], key, hash_extra);
    if (entry == NULL)
	return NoneSingleton;

    return entry->value;
}
