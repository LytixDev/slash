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
#ifndef SLASH_VALUE_H
#define SLASH_VALUE_H

#include <stdbool.h>

#include "interpreter/interpreter.h"
#include "interpreter/lexer.h"
#include "interpreter/value/type_funcs.h"
#include "sac/sac.h"


/* The Range type */
typedef struct {
    int32_t start;
    int32_t end;
} SlashRange;


/* The "head" of each Object type */
typedef struct slash_obj_t {
    bool gc_marked;
    bool gc_managed;
} SlashObj;

/*
 * Slash Objects.
 * Each object has the Object "head" meaning we can pass around pointers to objects
 * and achieve some sort of polymorphism similar to Expr and Stmt in the AST.
 */
typedef struct {
    SlashObj obj;
    ArrayList list;
} SlashList;

typedef struct {
    SlashObj obj;
    HashMap map;
} SlashMap;

typedef struct {
    SlashObj obj;
    size_t size;
    SlashValue *tuple;
} SlashTuple;

typedef struct {
    SlashObj obj;
    char *str; // Null terminated
    size_t len; // Length of string: does not includes null terminator. So "hi" has length 2
} SlashStr;


typedef struct slash_type_info_t {
    char *name;

    /* Operator functions */
    OpPlus plus;
    OpMinus minus;
    OpMul mul;
    OpDiv div;
    OpIntDiv int_div;
    OpPow pow;
    OpMod mod;
    OpUnaryMinus unary_minus;
    OpUnaryNot unary_not;

    /* Trait functions */
    TraitPrint print;
    TraitToStr to_str;
    TraitItemGet item_get;
    TraitItemAssign item_assign;
    TraitItemIn item_in;

    /* Equality and hashing functions */
    TraitTruthy truthy;
    TraitEq eq;
    TraitCmp cmp;
    TraitHash hash;

    /* Object lifetime functions */
    ObjInit init;
    ObjFree free;
} SlashTypeInfo;

typedef struct slash_value_t {
    SlashTypeInfo *T_info;
    union {
	bool boolean;
	double num;
	StrView shident;
	SlashRange range;
	SlashObj *obj;
    };
} SlashValue;


extern SlashTypeInfo bool_type_info;
extern SlashTypeInfo num_type_info;
extern SlashTypeInfo range_type_info;
extern SlashTypeInfo map_type_info;
extern SlashTypeInfo list_type_info;
extern SlashTypeInfo tuple_type_info;
extern SlashTypeInfo str_type_info;

#define IS_BOOL(value) ((value).T_info == &bool_type_info)
#define IS_NUM(value) ((value).T_info == &num_type_info)
#define IS_RANGE(value) ((value).T_info == &range_type_info)
#define IS_MAP(value) ((value).T_info == &map_type_info)
#define IS_LIST(value) ((value).T_info == &list_type_info)
#define IS_TUPLE(value) ((value).T_info == &tuple_type_info)
#define IS_STR(value) ((value).T_info == &str_type_info)
#define IS_OBJ(value) (!(IS_BOOL((value)) && IS_NUM((value)) && IS_RANGE((value))))

#define AS_MAP(value) ((SlashMap *)(value).obj)
#define AS_LIST(value) ((SlashList *)(value).obj)
#define AS_TUPLE(value) ((SlashTuple *)(value).obj)
#define AS_STR(value) ((SlashStr *)(value).obj)

#define TYPE_EQ(a, b) ((a).T_info == (b).T_info)

#endif /* SLASH_VALUE_H */
