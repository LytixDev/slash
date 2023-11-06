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


typedef struct slash_type_info_t SlashTypeInfo;

/* The "head" of each Object type */
typedef struct slash_obj_t {
    SlashTypeInfo *T_info; // TODO: not ideal ...
    bool gc_marked;
    bool gc_managed;
} SlashObj;

/*
 * Slash Objects.
 * Each object has the Object "head" meaning we can pass around pointers to objects
 * and achieve some sort of polymorphism similar to Expr and Stmt in the AST.
 *
 * SlashMap defined in ./slash_map.h and SlashList in ./slash_list.h
 */
typedef struct {
    SlashObj obj;
    size_t size;
    SlashValue *items;
} SlashTuple;

typedef struct {
    SlashObj obj;
    char *str; // Null terminated
    size_t len; // Length of string: does not includes null terminator. So "hi" has length 2
} SlashStr;


typedef struct slash_type_info_t {
    char *name;

    /*
     * Operator functions.
     * If operator takes multiple operand then all operands must be of the same type.
     */
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

    /*
     * Equality and hashing functions.
     * If trait function takes multiple SlashValue arguments then each argument must be of the
     * same type as the `self` argument.
     */
    TraitTruthy truthy;
    TraitEq eq;
    TraitCmp cmp;
    TraitHash hash;

    /* Object params */
    ObjInit init;
    ObjFree free;
    size_t obj_size; // Size of the object in bytes
} SlashTypeInfo;

typedef struct slash_value_t {
    SlashTypeInfo *T_info;
    union {
	bool boolean;
	double num;
	SlashRange range;
	StrView text_lit;
	SlashObj *obj;
    };
} SlashValue;


extern SlashTypeInfo bool_type_info;
extern SlashTypeInfo num_type_info;
extern SlashTypeInfo range_type_info;
extern SlashTypeInfo text_lit_type_info;
extern SlashTypeInfo list_type_info;
extern SlashTypeInfo tuple_type_info;
extern SlashTypeInfo str_type_info;
extern SlashTypeInfo map_type_info;
extern SlashTypeInfo none_type_info;

extern SlashValue NoneSingleton;

#define IS_BOOL(value__) ((value__).T_info == &bool_type_info)
#define IS_NUM(value__) ((value__).T_info == &num_type_info)
#define IS_RANGE(value__) ((value__).T_info == &range_type_info)
#define IS_TEXT_LIT(value__) ((value__).T_info == &text_lit_type_info)
#define IS_MAP(value__) ((value__).T_info == &map_type_info)
#define IS_LIST(value__) ((value__).T_info == &list_type_info)
#define IS_TUPLE(value__) ((value__).T_info == &tuple_type_info)
#define IS_STR(value__) ((value__).T_info == &str_type_info)
#define IS_NONE(value__) ((value__).T_info == &none_type_info)
#define IS_OBJ(value__) (!(IS_BOOL((value__)) && IS_NUM((value__)) && IS_RANGE((value__))))

#define AS_MAP(value__) ((SlashMap *)(value__).obj)
#define AS_LIST(value__) ((SlashList *)(value__).obj)
#define AS_TUPLE(value__) ((SlashTuple *)(value__).obj)
#define AS_STR(value__) ((SlashStr *)(value__).obj)
#define AS_VALUE(obj__) \
    ((SlashValue){ .T_info = ((SlashObj *)(obj__))->T_info, .obj = (SlashObj *)(obj__) })

#define TYPE_EQ(a, b) ((a).T_info == (b).T_info)
#define NUM_IS_INT(value_num__) ((value_num__).num == (int)(value_num__).num)


/* Init functions */
void slash_tuple_init(Interpreter *interpreter, SlashTuple *tuple, size_t size);

void slash_str_init_from_view(Interpreter *interpreter, SlashStr *str, StrView *view);
void slash_str_init_from_slice(Interpreter *interpreter, SlashStr *str, char *cstr, size_t size);
void slash_str_init_from_alloced_cstr(SlashStr *str, char *cstr);

#endif /* SLASH_VALUE_H */
