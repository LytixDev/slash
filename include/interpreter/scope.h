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
#ifndef SCOPE_H
#define SCOPE_H

#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "nicc/nicc.h"


typedef struct scope_t Scope;
struct scope_t {
    Scope *enclosing;
    struct hashmap_t values;
    // arena;
};

void scope_init(Scope *scope, Scope *enclosing);
void var_set(Scope *scope, SlashStr *key, SlashValue *value);
SlashValue var_get(Scope *scope, SlashStr *key);


#endif /* SCOPE_H */
