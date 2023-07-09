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
#include <stdio.h>

#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"


void slash_tuple_print(SlashValue *value)
{
    SlashTuple tuple = value->tuple;
    SlashValue current;
    printf("((");
    for (size_t i = 0; i < tuple.size; i++) {
	current = tuple.values[i];
	/* call the print function directly */
	slash_print[current.type](&current);
	if (i != tuple.size - 1)
	    printf(", ");
    }
    printf("))");
}

size_t *slash_tuple_len(SlashValue *value)
{
    return &value->tuple.size;
}
