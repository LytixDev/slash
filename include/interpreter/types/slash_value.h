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
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_range.h"
#include "interpreter/types/slash_tuple.h"
#include "sac/sac.h"


typedef enum {
    SLASH_BOOL = 0,
    SLASH_STR,
    SLASH_NUM,
    SLASH_SHLIT,
    SLASH_RANGE,
    SLASH_LIST,
    SLASH_TUPLE,
    SLASH_MAP,
    SLASH_NONE,
    SLASH_TYPE_COUNT,
} SlashType;

/*
 * Every SlashValue will have a table containing function pointers that implement these operations
 */
typedef enum {
    OP_PRINT = 0,
    OP_LEN,
    OP_COUNT,
} SlashTypeOp;


typedef struct slash_value_t {
    SlashType type;
    union {
	StrView str;
	bool boolean;
	double num;
	SlashRange range;
	SlashList list;
	SlashTuple tuple;
	SlashMap map;
    };
} SlashValue;


SlashValue *slash_value_arena_alloc(Arena *arena, SlashType type);

SlashTypeOp *slash_type_ops(SlashType type);

void slash_op_not_supported(void *);


bool is_truthy(SlashValue *value);

bool slash_value_eq(SlashValue *a, SlashValue *b);

void slash_value_print(SlashValue *value);

/*
 * Generic function pointer used for operations
 */
typedef void *(*SlashOpFunc)(void *);

/*
 * Table containing function pointers implementing operations for each type
 *
 * Example usage for printing a SlashValue:
 * SlashValue value = ...
 * print_func = op_table[value.type][OP_PRINT]
 * print_func(value)
 *
 * Example usage for getting the len of a SlashValue:
 * SlashValue value = ...
 * len_func = op_table[value.type][OP_LEN]
 * size_t *len = len_func(value)
 */
extern SlashOpFunc type_functions[SLASH_TYPE_COUNT][OP_COUNT];


#endif /* SLASH_VALUE_H */
