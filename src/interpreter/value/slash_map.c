/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "interpreter/gc.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_value.h"
#include "interpreter/value/type_funcs.h"
#include "nicc/nicc.h"


static inline uint8_t map_hash_extra(int hash)
{
	return (uint8_t)(hash & ((1 << 8) - 1));
}

static SlashMapEntry *map_get_from_bucket(SlashMapBucket bucket, SlashValue key, uint8_t hash_extra)
{
	SlashMapEntry *entry = NULL;
	for (size_t i = 0; i < HM_BUCKET_SIZE; i++) {
		entry = &bucket.entries[i];
		if (entry->is_occupied && entry->hash_extra == hash_extra && TYPE_EQ(key, entry->key)) {
			if (key.T->eq(key, entry->key))
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
			if (key.T->eq(key, entry->key)) {
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

static void map_increase_capacity(Interpreter *interpreter, SlashMap *map)
{
	map->total_buckets_log2++;
	assert(map->total_buckets_log2 < 32);

	/*
	 * The strategy here is to create more buckets and move each entry into one of the new buckets.
	 * This causes a small freeze in execution. Could be improved by partially moving entries
	 * sequentially. So far the added complexity is not worth it.
	 */
	size_t n_buckets = N_BUCKETS(map->total_buckets_log2);
	SlashMapBucket *new_buckets = gc_alloc(interpreter, sizeof(SlashMapBucket) * n_buckets);
	/* Sets all entries' is_occupied field to false (0) */
	memset(new_buckets, 0, sizeof(SlashMapBucket) * n_buckets);

	/* Move all entries into the new buckets */
	size_t old_n_buckets = N_BUCKETS(map->total_buckets_log2 - 1);
	for (size_t i = 0; i < old_n_buckets; i++) {
		SlashMapBucket *bucket = &map->buckets[i];
		for (int j = 0; j < HM_BUCKET_SIZE; j++) {
			SlashMapEntry entry = bucket->entries[j];
			if (entry.is_occupied) {
				unsigned int hash = (unsigned int)entry.key.T->hash(entry.key);
				uint8_t hash_extra = map_hash_extra(hash);
				unsigned int bucket_idx = hash >> (32 - map->total_buckets_log2);
				map_insert(&new_buckets[bucket_idx], entry.key, entry.value, hash_extra);
			}
		}
	}

	gc_free(interpreter, map->buckets, sizeof(SlashMapBucket) * old_n_buckets);
	map->buckets = new_buckets;
}

void slash_map_impl_init(Interpreter *interpreter, SlashMap *map)
{
	map->len = 0;
	map->total_buckets_log2 = SLASH_MAP_STARTING_BUCKETS_LOG2;
	size_t n_buckets = N_BUCKETS(map->total_buckets_log2);
	map->buckets = gc_alloc(interpreter, sizeof(SlashMapBucket) * n_buckets);
	/*
	 * Since we memset each bucket and therefore each entry to 0 the is_occupied field on
	 * each entry will be set to false.
	 */
	memset(map->buckets, 0, sizeof(SlashMapBucket) * n_buckets);
}

void slash_map_impl_free(Interpreter *interpreter, SlashMap *map)
{
	gc_free(interpreter, map->buckets, N_BUCKETS(map->total_buckets_log2) * sizeof(SlashMapBucket));
}

void slash_map_impl_put(Interpreter *interpreter, SlashMap *map, SlashValue key, SlashValue value)
{
	double load_factor = (double)map->len / (N_BUCKETS(map->total_buckets_log2) * HM_BUCKET_SIZE);
	if (load_factor >= SLASH_MAP_LOAD_FACTOR_THRESHOLD)
		map_increase_capacity(interpreter, map);

	VERIFY_TRAIT_IMPL(hash, key, "Can not use type '%s' as key in map because type is unhashable.",
					  key.T->name);
	unsigned int hash = (unsigned int)key.T->hash(key);
	uint8_t hash_extra = map_hash_extra(hash);
	unsigned int bucket_idx = hash >> (32 - map->total_buckets_log2);

	int rc = map_insert(&map->buckets[bucket_idx], key, value, hash_extra);
	if (rc == _HM_FULL) {
		map_increase_capacity(interpreter, map);
		slash_map_impl_put(interpreter, map, key, value);
	}
	if (rc == _HM_SUCCESS)
		map->len++;
}

SlashValue slash_map_impl_get(SlashMap *map, SlashValue key)
{
	if (map->len == 0)
		return NoneSingleton;

	VERIFY_TRAIT_IMPL(hash, key, "Can not use type '%s' as key in map because type is unhashable.",
					  key.T->name);
	unsigned int hash = (unsigned int)key.T->hash(key);
	uint8_t hash_extra = map_hash_extra(hash);
	unsigned int bucket_idx = hash >> (32 - map->total_buckets_log2);

	SlashMapEntry *entry = map_get_from_bucket(map->buckets[bucket_idx], key, hash_extra);
	if (entry == NULL)
		return NoneSingleton;

	return entry->value;
}

void slash_map_impl_get_values(SlashMap *map, SlashValue *return_ptr)
{
	size_t count = 0;
	for (size_t i = 0; i < N_BUCKETS(map->total_buckets_log2); i++) {
		SlashMapBucket *bucket = &map->buckets[i];
		for (int j = 0; j < HM_BUCKET_SIZE; j++) {
			SlashMapEntry entry = bucket->entries[j];
			if (!entry.is_occupied)
				continue;

			return_ptr[count++] = entry.value;
			if (count == map->len)
				return;
		}
	}
}

void slash_map_impl_get_keys(SlashMap *map, SlashValue *return_ptr)
{
	size_t count = 0;
	for (size_t i = 0; i < N_BUCKETS(map->total_buckets_log2); i++) {
		SlashMapBucket *bucket = &map->buckets[i];
		for (int j = 0; j < HM_BUCKET_SIZE; j++) {
			SlashMapEntry entry = bucket->entries[j];
			if (!entry.is_occupied)
				continue;

			return_ptr[count++] = entry.key;
			if (count == map->len)
				return;
		}
	}
}

void slash_map_impl_print(Interpreter *interpreter, SlashMap map)
{
	SlashValue key, value;
	SlashMapBucket bucket;
	SlashMapEntry entry;
	size_t entries_found = 0;

	SLASH_PRINT(&interpreter->stream_ctx, "@[");
	for (size_t i = 0; i < N_BUCKETS(map.total_buckets_log2); i++) {
		bucket = map.buckets[i];
		for (size_t j = 0; j < HM_BUCKET_SIZE; j++) {
			entry = bucket.entries[j];
			if (!entry.is_occupied)
				continue;

			entries_found++;
			key = entry.key;
			value = entry.value;

			key.T->print(interpreter, key);
			SLASH_PRINT(&interpreter->stream_ctx, ": ");
			value.T->print(interpreter, value);
			if (entries_found == map.len)
				break;
			SLASH_PRINT(&interpreter->stream_ctx, ",");
		}
	}
	SLASH_PRINT(&interpreter->stream_ctx, "]");
}
