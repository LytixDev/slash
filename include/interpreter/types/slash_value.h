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
#include "interpreter/types/slash_range.h"
#include "sac/sac.h"


typedef enum {
    SLASH_BOOL = 0,
    SLASH_STR,
    SLASH_NUM,
    SLASH_SHLIT,
    SLASH_RANGE,
    SLASH_LIST,
    SLASH_NONE,
    SLASH_ANY,
} SlashValueType;


typedef struct slash_value_t {
    SlashValueType type;
    union {
	StrView str;
	bool boolean;
	double num;
	SlashRange range;
	SlashList list;
    };
} SlashValue;


SlashValue *slash_value_arena_alloc(Arena *arena, SlashValueType type);

bool is_truthy(SlashValue *value);

bool slash_value_eq(SlashValue *a, SlashValue *b);

void slash_value_print(SlashValue *value);

#endif /* SLASH_VALUE_H */
