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
///
///#include <unistd.h>
///
///#include "interpreter/error.h"
///#include "interpreter/interpreter.h"
///#include "interpreter/types/slash_str.h"
///#include "interpreter/types/slash_value.h"
///#include "interpreter/types/trait.h"
///
///
///int builtin_cd(Interpreter *interpreter, size_t argc, SlashValue *argv)
///{
///    if (argc == 0) {
///	fprintf(stderr, "cd: no argument received");
///	return 1;
///    }
///
///    SlashValue param = argv[0];
///    TraitToStr to_str = trait_to_str[param.type];
///    SlashStr *param_str = (SlashStr *)to_str(interpreter, &param).obj;
///    return chdir(param_str->p);
///}
