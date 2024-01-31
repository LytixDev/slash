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

#include "interpreter/lexer.h"
#include "interpreter/value/type_funcs.h"
#include "lib/arena_ll.h"
#include "sac/sac.h"


/* The Range type */
typedef struct {
    int32_t start;
    int32_t end;
} SlashRange;

typedef struct slash_block_stmt_t BlockStmt; // Forward decl.

/* The Function type */
typedef struct {
    StrView name;
    ArenaLL params;
    BlockStmt *body;
} SlashFunction;


typedef struct slash_type_info_t SlashTypeInfo;

/* The "head" of each Object type */
typedef struct slash_obj_t {
    SlashTypeInfo *T; // TODO: not ideal ...
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
    size_t len;
    SlashValue *items;
} SlashTuple;

typedef struct slash_list_impl_t SlashList; // Forward decl.

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
    TraitPrint print; // Must be implemented
    TraitToStr to_str;
    TraitItemGet item_get;
    TraitItemAssign item_assign;
    TraitItemIn item_in;

    /*
     * Equality and hashing functions.
     * If trait function takes multiple SlashValue arguments then each argument must be of the
     * same type as the `self` argument.
     */
    TraitTruthy truthy; // Must be implemented
    TraitEq eq; // Must be implemented
    TraitCmp cmp;
    TraitHash hash;

    /* Object params */
    ObjInit init;
    ObjFree free;
    size_t obj_size; // Size of the object in bytes
} SlashTypeInfo;

typedef struct slash_value_t {
    SlashTypeInfo *T;
    union {
	bool boolean;
	double num;
	SlashRange range;
	StrView text_lit;
	SlashFunction function;
	SlashObj *obj;
    };
} SlashValue;


extern SlashTypeInfo bool_type_info;
extern SlashTypeInfo num_type_info;
extern SlashTypeInfo range_type_info;
extern SlashTypeInfo text_lit_type_info;
extern SlashTypeInfo function_type_info;
extern SlashTypeInfo list_type_info;
extern SlashTypeInfo tuple_type_info;
extern SlashTypeInfo str_type_info;
extern SlashTypeInfo map_type_info;
extern SlashTypeInfo none_type_info;

extern SlashValue NoneSingleton;

#define IS_BOOL(value__) ((value__).T == &bool_type_info)
#define IS_NUM(value__) ((value__).T == &num_type_info)
#define IS_RANGE(value__) ((value__).T == &range_type_info)
#define IS_TEXT_LIT(value__) ((value__).T == &text_lit_type_info)
#define IS_FUNCTION(value__) ((value__).T == &function_type_info)
#define IS_MAP(value__) ((value__).T == &map_type_info)
#define IS_LIST(value__) ((value__).T == &list_type_info)
#define IS_TUPLE(value__) ((value__).T == &tuple_type_info)
#define IS_STR(value__) ((value__).T == &str_type_info)
#define IS_NONE(value__) ((value__).T == &none_type_info)
#define IS_OBJ(value__) \
    (IS_MAP((value__)) || IS_LIST((value__)) || IS_TUPLE((value__)) || IS_STR((value__)))

#define AS_MAP(value__) ((SlashMap *)(value__).obj)
#define AS_LIST(value__) ((SlashList *)(value__).obj)
#define AS_TUPLE(value__) ((SlashTuple *)(value__).obj)
#define AS_STR(value__) ((SlashStr *)(value__).obj)
#define AS_VALUE(obj__) ((SlashValue){ .T = ((SlashObj *)(obj__))->T, .obj = (SlashObj *)(obj__) })

#define TYPE_EQ(a, b) ((a).T == (b).T)
#define NUM_IS_INT(value_num__) ((value_num__).num == (int)(value_num__).num)


/* Tuple functions */
void slash_tuple_init(Interpreter *interpreter, SlashTuple *tuple, size_t size);

#endif /* SLASH_VALUE_H */
