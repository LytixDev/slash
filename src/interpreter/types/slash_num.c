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
#include "interpreter/types/slash_value.h"
#include <stdio.h>

void slash_num_print(SlashValue *value)
{
    if (value->num == (int)value->num)
	printf("%d", (int)value->num);
    else
	printf("%f", value->num);
}

int slash_num_cmp(const void *a, const void *b)
{
    SlashValue *A = (SlashValue *)a;
    SlashValue *B = (SlashValue *)b;

    double sum = A->num - B->num;

    if (sum < 1 && sum > -1) {
	if (A->num > B->num)
	    return 1;
	else if (A->num < B->num)
	    return -1;
	else
	    return 0;
    }

    return (int)(sum);
}