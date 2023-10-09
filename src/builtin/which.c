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
#include "interpreter/types/slash_str.h"
#include "interpreter/types/trait.h"
#include "lib/str_view.h"


char *builtin_names[] = { "cd", "vars", "which" };
#define BUILTINS_COUNT (sizeof((builtin_names)) / sizeof((builtin_names[0])))


static void which_internal(WhichResult *mutable_result, char *PATH, char *command)
{
    // TODO: lol
    PATH = PATH + 5;
    size_t PATH_len = strlen(PATH) + 1;
    char *path_cpy = malloc(PATH_len);
    memcpy(path_cpy, PATH, PATH_len);

    char *single_path = strtok(path_cpy, ":");
    while (single_path != NULL) {
	DIR *d = opendir(single_path);
	if (d == NULL)
	    goto builtin_internal_next;

	struct dirent *dir_entry;
	struct stat sb;
	while ((dir_entry = readdir(d)) != NULL) {
	    /* check for name equality */
	    if (strcmp(command, dir_entry->d_name) == 0) {
		snprintf(mutable_result->path, 512, "%s/%s", single_path, command);
		/* check if executable bit is on */
		if (stat(mutable_result->path, &sb) == 0 && sb.st_mode & S_IXUSR &&
		    !S_ISDIR(sb.st_mode)) {
		    mutable_result->type = WHICH_EXTERN;
		    return;
		}
		/* program can not be in this directory as the correct name is not executable */
		closedir(d);
		goto builtin_internal_next;
	    }
	}

	closedir(d);
builtin_internal_next:
	single_path = strtok(NULL, ":");
    }

    mutable_result->type = WHICH_NOT_FOUND;
}

int builtin_which(Interpreter *interpreter, size_t argc, SlashValue *argv)
{
    if (argc == 0) {
	fprintf(stderr, "which: no argument received");
	return 1;
    }

    SlashValue param = argv[0];
    TraitToStr to_str = trait_to_str[param.type];
    SlashStr *param_str = (SlashStr *)to_str(interpreter, &param).obj;
    ScopeAndValue path = var_get(interpreter->scope, &(StrView){ .view = "PATH", .size = 4 });
    if (!(path.value->type == SLASH_OBJ && path.value->obj->type == SLASH_OBJ_STR)) {
	fprintf(stderr, "PATH variable should be type 'str' not '%s'",
		SLASH_TYPE_TO_STR(path.value));
	return 1;
    }

    WhichResult which_result =
	which((StrView){ .view = param_str->p, .size = strlen(param_str->p) },
	      ((SlashStr *)path.value->obj)->p);
    switch (which_result.type) {
    case WHICH_BUILTIN:
	printf("%s: slash builtin\n", param_str->p);
	break;
    case WHICH_EXTERN:
	printf("%s\n", which_result.path);
	break;
    case WHICH_NOT_FOUND:
	printf("%s not found\n", param_str->p);
	return 1;
    }
    return 0;
}

WhichResult which(StrView cmd, char *PATH)
{
    // TODO: this function is not memory safe

    char command[cmd.size + 1];
    str_view_to_cstr(cmd, command);

    WhichResult result = { 0 };
    /* edge case: command is a path */
    if (command[0] == '/') {
	result.type = WHICH_EXTERN;
	memcpy(result.path, command, cmd.size + 1);
	return result;
    }

    // TODO: is there a faster solution than a linear search?
    for (size_t i = 0; i < BUILTINS_COUNT; i++) {
	char *builtin_name = builtin_names[i];
	if (strcmp(builtin_name, command) == 0) {
	    result.type = WHICH_BUILTIN;
	    // TODO: X macro would work nicely here I think
	    /* set correct function pointer */
	    if (strcmp(command, "cd") == 0) {
		result.builtin = builtin_cd;
	    } else if (strcmp(command, "which") == 0) {
		result.builtin = builtin_which;
	    } else {
		result.builtin = builtin_vars;
	    }
	    return result;
	}
    }

    /* execution enters here means cmd is not a builtin */
    which_internal(&result, PATH, command);
    return result;
}
