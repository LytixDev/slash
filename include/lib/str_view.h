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
#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


/*
 * the main string implementation used throughout the interpreter
 * the char *view is a view into some memory and will most likely not be null terminated,
 */
typedef struct {
    char *view; // pointer into some memory in an arena
    size_t size;
} StrView;


void str_view_print(StrView s);
double str_view_to_double(StrView s);
int32_t str_view_to_int(StrView s);
bool str_view_eq(StrView a, StrView b);
int str_view_cmp(StrView a, StrView b);
void str_view_to_cstr(StrView view, char *cstr);

#endif /* STR_VIEW_H */
