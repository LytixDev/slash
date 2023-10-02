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
#include <unistd.h>

#include "interpreter/error.h"
#include "interpreter/interpreter.h"
#include "interpreter/types/slash_value.h"


int builtin_cd(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    (void)interpreter;
    if (argc == 0) {
    }
    // if (dir_name == NULL) {
    //     fprintf(stderr, "cd: no argument provided\n");
    //     return 1;
    // }
    // return chdir(dir_name);
    return 0;
}
