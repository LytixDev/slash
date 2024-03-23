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
#ifndef SLASH_TYPE_FUNC_H
#define SLASH_TYPE_FUNC_H

#include "interpreter/error.h"
// #include "interpreter/interpreter.h"


#define VERIFY_TRAIT_IMPL(trait_func_name, value, ...) \
    do {                                               \
	if ((value).T->trait_func_name == NULL)        \
	    REPORT_RUNTIME_ERROR(__VA_ARGS__);         \
    } while (0);


typedef struct slash_type_info_t SlashTypeInfo; // Forward decl
typedef struct slash_obj_t SlashObj; // Forward decl
typedef struct slash_value_t SlashValue; // Forward decl
typedef struct interpreter_t Interpreter; // Forward decl


/* Operators */
typedef SlashValue (*OpPlus)(Interpreter *interpreter, SlashValue self, SlashValue other);
typedef SlashValue (*OpMinus)(SlashValue self, SlashValue other);
typedef SlashValue (*OpMul)(Interpreter *interpreter, SlashValue self, SlashValue other);
typedef SlashValue (*OpDiv)(SlashValue self, SlashValue other);
typedef SlashValue (*OpIntDiv)(SlashValue self, SlashValue other);
typedef SlashValue (*OpPow)(SlashValue self, SlashValue other);
typedef SlashValue (*OpMod)(SlashValue self, SlashValue other);
typedef SlashValue (*OpUnaryMinus)(SlashValue self);
typedef SlashValue (*OpUnaryNot)(SlashValue self);

/* Traits */
typedef void (*TraitPrint)(Interpreter *interpreter, SlashValue self);
typedef SlashValue (*TraitToStr)(Interpreter *interpreter, SlashValue self);
typedef SlashValue (*TraitItemGet)(Interpreter *interpreter, SlashValue self, SlashValue other);
typedef void (*TraitItemAssign)(Interpreter *interpreter, SlashValue self, SlashValue index,
				SlashValue other);
/* Equality and hashing traits */
typedef bool (*TraitItemIn)(SlashValue self, SlashValue other);
typedef bool (*TraitTruthy)(SlashValue self);
typedef bool (*TraitEq)(SlashValue self, SlashValue other);
typedef int (*TraitCmp)(SlashValue self, SlashValue other);
typedef int (*TraitHash)(SlashValue self);

#endif /* SLASH_TYPE_FUNC_H */
