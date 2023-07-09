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
#include "interpreter/types/slash_bool.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_num.h"
#include "interpreter/types/slash_range.h"
#include "interpreter/types/slash_str.h"
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

bool is_truthy(SlashValue *value);

bool slash_value_eq(SlashValue *a, SlashValue *b);

typedef void (*SlashPrintFunc)(SlashValue *);
typedef size_t (*SlashLenFunc)(SlashValue *);
typedef SlashValue *(*SlashItemGetFunc)(SlashValue *, SlashValue *);
typedef void (*SlashItemAssignFunc)(SlashValue *, SlashValue *, SlashValue *);

/*
 * Table containing function pointers implementing print for each type
 *
 * Example usage for printing a SlashValue:
 * SlashValue value = ...
 * print_func = slash_print[value.type]
 * print_func(&value)
 */
extern SlashPrintFunc slash_print[SLASH_TYPE_COUNT];

extern SlashLenFunc slash_len[SLASH_TYPE_COUNT];

extern SlashItemGetFunc slash_item_get[SLASH_TYPE_COUNT];

extern SlashItemAssignFunc slash_item_assign[SLASH_TYPE_COUNT];


#endif /* SLASH_VALUE_H */
