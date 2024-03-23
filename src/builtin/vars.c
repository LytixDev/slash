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
#include "interpreter/value/slash_value.h"
#include "nicc/nicc.h"


static void hashmap_get_keys_as_str_views(HashMap *map, StrView *return_ptr)
{
    size_t count = 0;
    for (int i = 0; i < N_BUCKETS(map->size_log2); i++) {
	struct hm_bucket_t *bucket = &map->buckets[i];
	for (int j = 0; j < HM_BUCKET_SIZE; j++) {
	    struct hm_entry_t entry = bucket->entries[j];
	    if (entry.key == NULL)
		continue;

	    return_ptr[count++] = (StrView){ .view = entry.key, .size = entry.key_size };
	    if (count == map->len)
		return;
	}
    }
}

int builtin_vars(Interpreter *interpreter, ArenaLL *ast_nodes)
{
    (void)ast_nodes;
    for (Scope *scope = interpreter->scope; scope != NULL; scope = scope->enclosing) {
	HashMap map = scope->values;
	StrView keys[map.len];
	SlashValue *values[map.len];
	hashmap_get_keys_as_str_views(&map, keys);
	hashmap_get_values(&map, (void **)values);

	for (size_t i = 0; i < map.len; i++) {
	    StrView key = keys[i];
	    SlashValue *value = values[i];
	    str_view_to_buf_cstr(key); // creates temporary buf variable
	    SLASH_PRINT(&interpreter->stream_ctx, "%s", buf);
	    putchar('=');
	    VERIFY_TRAIT_IMPL(print, *value, "print not defined for type '%s'", value->T->name);
	    value->T->print(interpreter, *value);
	    putchar('\n');
	}
    }

    return 0;
}
