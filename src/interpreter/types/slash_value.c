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
#include <stdbool.h>
#include <stdio.h>

// #include "common.h"

#include "interpreter/types/slash_value_all.h"
#include "sac/sac.h"
#include "str_view.h"


const size_t slash_value_size_table[] = {
    sizeof(SlashBool),	sizeof(SlashStr),  sizeof(SlashBool),  sizeof(SlashSHlit),
    sizeof(SlashRange), sizeof(SlashList), sizeof(SlashValue), sizeof(SlashValue),
};

SlashValue *slash_value_arena_alloc(Arena *arena, SlashValueType type)
{
    SlashValue *v = m_arena_alloc(arena, slash_value_size_table[type]);
    v->type = type;
    return v;
}

bool is_truthy(SlashValue *value)
{
    switch (value->type) {
    case SLASH_STR:
    case SLASH_SHLIT:
	return ((SlashStr *)value)->str.size != 0;

    case SLASH_NUM:
	return ((SlashNum *)value)->num != 0;

    case SLASH_BOOL:
	return ((SlashBool *)value)->value;

    default:
	fprintf(stderr, "truthy not defined for this type, returning true");
    }
    return true;
}

void slash_value_print(SlashValue *value)
{
    switch (value->type) {
    case SLASH_STR:
    case SLASH_SHLIT:
	str_view_print(((SlashStr *)value)->str);
	break;

    case SLASH_NUM:
	printf("%f", ((SlashNum *)value)->num);
	break;

    default:
	fprintf(stderr, "printing not defined for this type");
    }
}


/* following are the three musketeers of soydev functions */
// static bool svt_str_truthy(StrView *str)
//{
//     return str->size != 0;
// }
//
// static bool svt_bool_truthy(bool *p)
//{
//     return *p == true;
// }
//
// static bool svt_num_truthy(double *p)
//{
//     return *p != 0;
// }
//
// bool is_truthy(SlashValue *value)
//{
//     switch (value->type) {
//     case SVT_STR:
//	return svt_str_truthy(value->value);
//
//     case SVT_BOOL:
//	return svt_bool_truthy(value->value);
//
//     case SVT_NUM:
//	return svt_num_truthy(value->value);
//
//     default:
//	return false;
//     }
// }
//
//
//// TODO: all of the following functions need reworking.
////       this is just temporary to get things going
////       turbo ugly
// static SlashValue slash_value_plus(Arena *arena, SlashValue a, SlashValue b)
//{
//     double *res = m_arena_alloc(arena, sizeof(double));
//     switch (a.type) {
//     case SVT_NUM:
//	*res = *(double *)a.value + *(double *)b.value;
//	return (SlashValue){ .value = res, .type = SVT_NUM };
//     default:
//	return (SlashValue){ .value = NULL, .type = SVT_NONE };
//     }
// }
//
// static SlashValue slash_value_minus(Arena *arena, SlashValue a, SlashValue b)
//{
//     double *res = m_arena_alloc(arena, sizeof(double));
//     switch (a.type) {
//     case SVT_NUM:
//	*res = *(double *)a.value - *(double *)b.value;
//	return (SlashValue){ .value = res, .type = SVT_NUM };
//     default:
//	return (SlashValue){ .value = NULL, .type = SVT_NONE };
//     }
// }
//
// static SlashValue slash_value_greater(Arena *arena, SlashValue a, SlashValue b)
//{
//     bool *res = m_arena_alloc(arena, sizeof(bool));
//     switch (a.type) {
//     case SVT_NUM:
//	*res = *(double *)a.value > *(double *)b.value;
//	return (SlashValue){ .value = res, .type = SVT_BOOL };
//     default:
//	return (SlashValue){ .value = NULL, .type = SVT_NONE };
//     }
// }
//
// static SlashValue slash_value_equal(Arena *arena, SlashValue a, SlashValue b)
//{
//     bool *res = m_arena_alloc(arena, sizeof(bool));
//     switch (a.type) {
//     case SVT_NUM:
//	*res = *(double *)a.value == *(double *)b.value;
//	return (SlashValue){ .value = res, .type = SVT_BOOL };
//     default:
//	return (SlashValue){ .value = NULL, .type = SVT_NONE };
//     }
// }
//
// static SlashValue slash_value_not_equal(Arena *arena, SlashValue a, SlashValue b)
//{
//     SlashValue value = slash_value_equal(arena, a, b);
//     if (value.value == NULL)
//	return value;
//
//     // TODO: lol
//     bool *v = value.value;
//     *v = !*v;
//     *(bool *)value.value = *v;
//     return value;
// }
//
// SlashValue slash_value_cmp(Arena *arena, SlashValue a, SlashValue b, TokenType operator)
//{
//     // is this too conservative?
//     // TODO: better error handling in cases like this
//     if (a.type != b.type)
//	return (SlashValue){ .value = NULL, .type = SVT_NONE };
//
//     if (operator== t_plus)
//	return slash_value_plus(arena, a, b);
//     if (operator== t_minus)
//	return slash_value_minus(arena, a, b);
//     if (operator== t_greater)
//	return slash_value_greater(arena, a, b);
//     if (operator== t_equal_equal)
//	return slash_value_equal(arena, a, b);
//     if (operator== t_bang_equal)
//	return slash_value_not_equal(arena, a, b);
//
//     fprintf(stderr, "operator not supported, returning SVT_NONE");
//     return (SlashValue){ .value = NULL, .type = SVT_NONE };
// }
//
// void slash_value_print(SlashValue sv)
//{
//     switch (sv.type) {
//     case SVT_STR:
//     case SVT_SHLIT:
//	slash_str_print(*(StrView *)sv.value);
//	break;
//
//     case SVT_BOOL:
//	printf("%s", *(bool *)sv.value ? "true" : "false");
//	break;
//
//     case SVT_NUM:
//	printf("%f", *(double *)sv.value);
//	break;
//
//     default:
//	break;
//     }
// }
//
// void slash_value_println(SlashValue sv)
//{
//     switch (sv.type) {
//     case SVT_STR:
//     case SVT_SHLIT:
//	slash_str_println(*(StrView *)sv.value);
//	break;
//
//     case SVT_BOOL:
//	printf("%s\n", *(bool *)sv.value ? "true" : "false");
//	break;
//
//     case SVT_NUM:
//	printf("%f\n", *(double *)sv.value);
//	break;
//
//     default:
//	break;
//     }
// }
