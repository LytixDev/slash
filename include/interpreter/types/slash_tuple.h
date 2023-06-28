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
#ifndef SLASH_TUPLE_H
#define SLASH_TUPLE_H

#include <stdlib.h>

typedef struct slash_value_t SlashValue; // Forward declaration of SlashValue

/*
 * Tuples are ordered and unchangeable.
 * Other than that they behave similarly, and are generally inspired by, tuples in Python.
 */
typedef struct {
    size_t size;
    SlashValue *values;
} SlashTuple;

void slash_tuple_print(SlashTuple *tuple);

#endif /* SLASH_TUPLE_H */
