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

#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "interpreter/types/trait.h"
#include "nicc/nicc.h"


typedef struct {
    SlashObj obj;
    HashMap underlying;
} SlashMap;


void slash_map_init(SlashMap *map);


/*
 * traits
 */
/* O(n) */
void slash_map_print(SlashValue *value);
SlashValue slash_map_item_get(Scope *scope, SlashValue *self, SlashValue *index);
void slash_map_item_assign(SlashValue *self, SlashValue *index, SlashValue *new_value);
bool slash_map_item_in(SlashValue *self, SlashValue *item);


/*
 * methods
 */
SlashValue slash_map_get_keys(Interpreter *interpreter, SlashMap *map);


#endif /* SLASH_MAP_H */
