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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/lang/slash_str.h"
#include "interpreter/lang/slash_value.h"
#include "nicc/nicc.h"

void slash_str_print(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    printf("%s", str);
}

void slash_str_println(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    printf("%s\n", str);
}

// TODO: currently assume all numbers are base10 or base16 and are not prefixed with + or -
double slash_str_to_double(SlashStr s)
{
    char str[s.size + 1];
    memcpy(str, s.p, s.size);
    str[s.size] = 0;
    return strtod(str, NULL);
}
