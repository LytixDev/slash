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
#ifndef SLASH_STR_H
#define SLASH_STR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "nicc/nicc.h"

typedef struct {
    char *p; // pointer into some memory in an arena
    size_t size;
} SlashStr;


void slash_str_print(SlashStr s);
void slash_str_println(SlashStr s);

double slash_str_to_double(SlashStr s);

#endif /* SLASH_STR_H */
