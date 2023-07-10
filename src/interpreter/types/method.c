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
#include "interpreter/types/method.h"
#include "common.h"
#include "interpreter/types/slash_value.h"
#include <string.h>

MethodFunc get_method(SlashValue *self, char *method_name)
{
    size_t methods_count;
    SlashMethod *methods;

    // TODO: this could be a table
    switch (self->type) {
    case SLASH_LIST:
	methods_count = SLASH_LIST_METHODS_COUNT;
	methods = slash_list_methods;
	break;

    default:
	return NULL;
    }

    /* linear search */
    SlashMethod current;
    for (size_t i = 0; i < methods_count; i++) {
	current = methods[i];
	// TODO: strcmp does not exit early
	if (strcmp(current.name, method_name) == 0)
	    return current.fp;
    }
    return NULL;
}
