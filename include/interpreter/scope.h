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

#include "nicc/nicc.h"
#include "sac/sac.h"
#include "str_view.h"

typedef struct slash_value_t SlashValue; // Forward declaration of SlashValue

typedef struct scope_t Scope;
struct scope_t {
    Scope *enclosing; // if NULL then there is no enclosing scope and is the global scope
    size_t depth; // the amount of enclosing scopes
    ArenaTmp arena_tmp; // arena to put any temporary data on
    HashMap values; // key: StrView, value: SlashValue (actual objects, not pointers)
    LinkedList owning; // owning references to SlashValue's that must be freed on scope_destroy
};

typedef struct {
    Scope *scope;
    SlashValue *value;
} ScopeAndValue;


void scope_init_global(Scope *scope, Arena *arena);
void scope_init(Scope *scope, Scope *enclosing);
void scope_destroy(Scope *scope);
void scope_register_owning(Scope *scope, SlashValue *sv);
void *scope_alloc(Scope *scope, size_t size);

/*
 * copies the SlashValue object into the hashmap
 * if value is NULL then the variable is defined as SLASH_NONE
 */
void var_define(Scope *scope, StrView *key, SlashValue *value);
void var_undefine(Scope *scope, StrView *key);

/*
 * variable assignment for values that are stored in place, i.e non references.
 * a variable may be defined in an enclosing scope, meaning var_define is not sufficient.
 * var_assign attempts to find the scope of the variable before updating its value
 */
void var_assign_simple(Scope *scope, StrView *key, SlashValue *value);
/*
 * assigns the value to the variable who's key/name is var_name.
 * transfers the ownership of the reference value of the variable if neededed.
 * if not it calls var_assign_simple.
 */
void var_assign(StrView *var_name, Scope *var_owner, Scope *current, SlashValue *value);

/* returns the variable (if exists) with its owning scope */
ScopeAndValue var_get(Scope *scope, StrView *key);


#endif /* SCOPE_H */
