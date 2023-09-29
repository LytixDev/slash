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

#include "interpreter/interpreter.h"
#include "interpreter/scope.h"
#include "interpreter/types/slash_value.h"
#include "interpreter/types/trait.h"
#include "nicc/nicc.h"


int builtin_vars(Interpreter *interpreter)
{
    for (Scope *scope = interpreter->scope; scope != NULL; scope = scope->enclosing) {
        HashMap map = scope->values;
        char *vars[map.len];
        SlashValue *values[map.len];
        hashmap_get_keys(&map, (void **)vars);
        hashmap_get_values(&map, (void **)values);

        for (size_t i = 0; i < map.len; i++) {
            char *var = vars[i];
            SlashValue *value = values[i];
            putchar('=');
            TraitPrint print_func = trait_print[value->type];
            print_func(value);
            putchar('\n');
        }
    }

    return 0;
}
