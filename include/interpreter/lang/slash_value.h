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


typedef enum {
    SVT_BOOL = 0, // p = bool *
    SVT_STR, // p = SlashStr *
    SVT_NUM, // p = double *
    SVT_SHLIT, // p = SlashStr *
    SVT_RANGE, // p = SlashRange *
    SVT_NONE, // p = NULL
} SlashValueType;

typedef struct {
    void *p; // arena allocated (TODO: well, should be)
    SlashValueType type;
} SlashValue;


bool is_truthy(SlashValue *value);
// TODO: this is turbo ugly
SlashValue slash_value_cmp(Arena *arena, SlashValue a, SlashValue b, TokenType operator);
void slash_value_println(SlashValue sv);

#endif /* SLASH_VALUE_H */
