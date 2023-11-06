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
#ifndef SLASH_MAP_IMPL_H
#define SLASH_MAP_IMPL_H
#endif /* SLASH_MAP_IMPL_H */

#include <stdint.h>
#include <stdlib.h>

#include "interpreter/interpreter.h"
#include "interpreter/value/slash_value.h"

/*
 * Based on the HashMap implementation found in nicc (https://github.com/LytixDev/nicc)
 */

/* Should already be defined by the nicc dependency */
#ifndef NICC_HASHMAP_IMPLEMENTATION
#define _HM_FULL 1
#define _HM_OVERRIDE 2
#define _HM_SUCCESS 3
#endif /* NICC_HASHMAP_IMPLEMENTATION */

#define SLASH_MAP_STARTING_BUCKETS_LOG2 3
/*
 * sizeof(SlashValue) == 24.
 * 24 * 8 = 192, which is divisible by 32, meaning we have no extra padding.
 * */
#define SLASH_MAP_BUCKET_SIZE 8

#ifdef N_BUCKETS
#undef N_BUCKETS
#define N_BUCKETS(log2) (size_t)(1 << (log2))
#endif

#define SLASH_MAP_LOAD_FACTOR_THRESHOLD 0.75


typedef struct {
    SlashValue key;
    SlashValue value;
    uint8_t hash_extra; // used for faster comparison
    bool is_occupied;
} SlashMapEntry;

typedef struct {
    SlashMapEntry entries[SLASH_MAP_BUCKET_SIZE];
} SlashMapBucket;

typedef struct {
    SlashObj obj;
    SlashMapBucket *buckets;
    size_t total_buckets_log2;
    size_t len; // total items stored in the hashmap
} SlashMap;

/* Functions */
void slash_map_impl_init(Interpreter *interpreter, SlashMap *map);
void slash_map_impl_free(Interpreter *interpreter, SlashMap *map);

void slash_map_impl_put(Interpreter *interpreter, SlashMap *map, SlashValue key, SlashValue value);
SlashValue slash_map_impl_get(SlashMap *map, SlashValue key);

bool slash_map_impl_rm(SlashMap *map, SlashValue key);

void slash_map_impl_get_values(SlashMap *map, SlashValue *return_ptr);
void slash_map_impl_get_keys(SlashMap *map, SlashValue *return_ptr);

void slash_map_impl_print(SlashMap map);
