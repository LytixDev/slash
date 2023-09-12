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
#ifndef SLASH_METHOD_H
#define SLASH_METHOD_H

#include <stdbool.h>
#include <stdlib.h> // size_t decl

#include "interpreter/types/slash_value.h"

typedef SlashValue (*MethodFunc)(SlashValue *self, size_t argc, SlashValue *argv);

typedef struct {
    char *name;
    MethodFunc fp;
} SlashMethod;

MethodFunc get_method(SlashValue *self, char *method_name);

/*
 * Slash supports method overloading.
 * This function returns if the provided arguments (argv) match the format of the given signature.
 * The signature is represented using a string.
 *
 * Every char denotes a type. (see slash_type_to_char in method.c)
 * a -> any
 * n -> num
 * s -> str
 * etc.
 * A whitespace is used to seperate each argument.
 *
 * Examples:
 *      ""      -> empty signature
 *      "a"     -> takes in one argument of any time
 *      "n n"   -> takes in two arguments, both of type SlashNum
 */
bool match_signature(char *signature, size_t argc, SlashValue *argv);

#endif /* SLASH_METHOD_H */
