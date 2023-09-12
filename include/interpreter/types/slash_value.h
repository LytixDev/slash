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
#include "sac/sac.h"


typedef struct slash_obj_t SlashObj; // Forward decl

typedef enum {
    SLASH_BOOL = 0,
    SLASH_STR,
    SLASH_NUM,
    SLASH_SHIDENT,
    SLASH_RANGE,
    SLASH_OBJ,
    SLASH_NONE,
    SLASH_TYPE_COUNT,
} SlashType;

#define IS_OBJ(slash_type) (slash_type == SLASH_OBJ)

typedef struct {
    int32_t start;
    int32_t end;
} SlashRange;

typedef struct slash_value_t {
    SlashType type;
    union {
	/* stored in place */
	StrView str;
	bool boolean;
	double num;
	SlashRange range;
	/* pointers to heap allocated data */
	SlashObj *obj;
    };
} SlashValue;


bool is_truthy(SlashValue *value);
bool slash_value_eq(SlashValue *a, SlashValue *b);
int slash_value_cmp_stub(const void *a, const void *b);
int slash_value_cmp_rev_stub(const void *a, const void *b);
/* returns positive if a > b */
int slash_value_cmp(SlashValue *a, SlashValue *b);

/* */
void slash_bool_print(SlashValue *value);
void slash_num_print(SlashValue *value);
void slash_str_print(SlashValue *value);
void slash_range_print(SlashValue *value);
void slash_none_print(void);


extern int slash_cmp_precedence[SLASH_TYPE_COUNT];

extern SlashValue slash_glob_none;

#endif /* SLASH_VALUE_H */
