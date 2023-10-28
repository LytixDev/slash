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

typedef struct slash_type_info_t SlashTypeInfo; // Forward decl
typedef struct slash_obj_t SlashObj; // Forward dec


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

#endif /* SLASH_TRAIT_H */
