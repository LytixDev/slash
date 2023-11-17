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
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "builtin/builtin.h"
#include "interpreter/interpreter.h"
#include "interpreter/value/slash_str.h"
#include "lib/str_view.h"


Builtin builtins[] = { { .name = "cd", .func = builtin_cd },
		       { .name = "vars", .func = builtin_vars },
		       { .name = "which", .func = builtin_which },
		       { .name = "exit", .func = builtin_exit } };


static void which_internal(WhichResult *mutable_result, char *PATH, char *command)
{
    /* PATH environment variable is often prefixed with 'PATH=' */
    size_t PATH_len = strlen(PATH) + 1;
    char *path_cpy = malloc(PATH_len);
    memcpy(path_cpy, PATH, PATH_len);

    char *single_path = strtok(path_cpy, ":");
    while (single_path != NULL) {
	DIR *path_dir = opendir(single_path);

	struct dirent *candidate_program;
	while (path_dir != NULL && (candidate_program = readdir(path_dir)) != NULL) {
	    struct stat sb;
	    /* check for name equality */
	    if (strcmp(command, candidate_program->d_name) == 0)
		continue;

	    snprintf(mutable_result->path, PROGRAM_PATH_MAX_LEN, "%s/%s", single_path, command);
	    /* check if executable bit is on */
	    if (stat(mutable_result->path, &sb) == 0 && sb.st_mode & S_IXUSR &&
		!S_ISDIR(sb.st_mode)) {
		mutable_result->type = WHICH_EXTERN;
		free(path_cpy);
		closedir(path_dir);
		return;
	    }
	    /* can not be in this path_dir as file with matching name is not an executable */
	    break;
	}

	if (path_dir != NULL)
	    closedir(path_dir);
	single_path = strtok(NULL, ":");
    }

    free(path_cpy);
    mutable_result->type = WHICH_NOT_FOUND;
}

int builtin_which(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    if (argc == 0) {
	fprintf(stderr, "which: no argument received");
	return 1;
    }

    SlashValue param = argv[0];
    TraitToStr to_str = param.T->to_str;
    if (to_str == NULL) {
	fprintf(stderr, "which: could not take to_str of type '%s'", param.T->name);
	return 1;
    }
    SlashStr *param_str = AS_STR(to_str(interpreter, param));
    ScopeAndValue path = var_get(interpreter->scope, &(StrView){ .view = "PATH", .size = 4 });
    if (!IS_STR(*path.value)) {
	fprintf(stderr, "which: PATH variable should be type '%s' not '%s'", str_type_info.name,
		path.value->T->name);
	return 1;
    }

    WhichResult which_result = which((StrView){ .view = param_str->str, .size = param_str->len },
				     AS_STR(*path.value)->str);

    int return_code = 0;
    switch (which_result.type) {
    case WHICH_BUILTIN:
	printf("%s: slash builtin\n", param_str->str);
	break;
    case WHICH_EXTERN:
	printf("%s\n", which_result.path);
	break;
    case WHICH_NOT_FOUND:
	printf("%s not found\n", param_str->str);
	return_code = 1;
	break;
    }

    return return_code;
}

WhichResult which(StrView cmd, char *PATH)
{
    char command[cmd.size + 1];
    str_view_to_cstr(cmd, command);

    WhichResult result = { 0 };
    /* edge case: command is a path */
    if (command[0] == '/') {
	result.type = WHICH_EXTERN;
	memcpy(result.path, command,
	       cmd.size + 1 > PROGRAM_PATH_MAX_LEN ? PROGRAM_PATH_MAX_LEN : cmd.size + 1);
	return result;
    }

    size_t builtins_count = sizeof(builtins) / sizeof(builtins[0]);
    for (size_t i = 0; i < builtins_count; i++) {
	Builtin builtin = builtins[i];
	if (strcmp(builtin.name, command) == 0) {
	    result.type = WHICH_BUILTIN;
	    result.builtin = builtin.func;
	    return result;
	}
    }

    /* execution enters here means cmd is not a builtin */
    which_internal(&result, PATH, command);
    return result;
}
