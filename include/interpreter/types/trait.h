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
#ifndef SLASH_TRAIT_H
#define SLASH_TRAIT_H

#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value.h"

typedef void (*TraitPrint)(SlashValue *self);
typedef SlashValue (*TraitToStr)(Interpreter *interpreter, SlashValue *self);
typedef SlashValue (*TraitItemGet)(Interpreter *interpreter, SlashValue *self, SlashValue *);
typedef void (*TraitItemAssign)(SlashValue *self, SlashValue *, SlashValue *);
typedef bool (*TraitItemIn)(SlashValue *self, SlashValue *);
typedef bool (*TraitTruthy)(SlashValue *self);
typedef bool (*TraitEquals)(SlashValue *self, SlashValue *other);
typedef bool (*TraitCmp)(SlashValue *self, SlashValue *other);

typedef void (*ObjInit)(SlashObj *obj);
typedef void (*ObjFree)(SlashObj *obj);

typedef struct {
    TraitPrint print;
    TraitToStr to_str;
    TraitItemGet item_get;
    TraitItemAssign item_assign;
    TraitItemIn item_in;
    TraitTruthy truthy;
    TraitEquals equals;
    TraitCmp cmp;

    ObjInit init;
    ObjFree free;
} ObjTraits;

/*
 * Table containing function pointers implementing print for each type
 *
 * Example usage for printing a SlashValue:
 * SlashValue value = ...
 * print_func = slash_print[value.type]
 * print_func(&value)
 */
extern TraitPrint trait_print[SLASH_TYPE_COUNT];
extern TraitItemGet trait_item_get[SLASH_TYPE_COUNT];
extern TraitItemAssign trait_item_assign[SLASH_TYPE_COUNT];
extern TraitItemIn trait_item_in[SLASH_TYPE_COUNT];


#endif /* SLASH_TRAIT_H */
