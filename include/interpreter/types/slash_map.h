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
#ifndef SLASH_MAP_H
#define SLASH_MAP_H

#include "nicc/nicc.h"

typedef struct slash_value_t SlashValue; // Forward declaration of SlashValue

typedef struct {
    HashMap underlying;
} SlashMap;

void slash_map_init(SlashMap *map);

void slash_map_put(SlashMap *map, SlashValue *key, SlashValue *value);
/* returns NULL if key does not have an associated value */
SlashValue *slash_map_get(SlashMap *map, SlashValue *key);

/* O(n) */
void slash_map_print(SlashMap *map);

#endif /* SLASH_MAP_H */
