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
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_map.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif /* DEBUG_LOG_GC */

SlashObj *gc_alloc(LinkedList *gc_objs, SlashObjType type)
{
    SlashObj *obj = NULL;
    size_t size = 0;
    switch (type) {
    case SLASH_OBJ_LIST:
	size = sizeof(SlashList);
	break;
    case SLASH_OBJ_MAP:
	size = sizeof(SlashMap);
	break;
    case SLASH_OBJ_TUPLE:
	size = sizeof(SlashTuple);
	break;
    default:
	report_runtime_error("Slash obj not implemented");
	ASSERT_NOT_REACHED;
    }

    obj = malloc(size);
    obj->type = type;
    obj->gc_marked = false;
    obj->traits = NULL;

    gc_register(gc_objs, obj);
    return obj;
}

void gc_register(LinkedList *gc_objs, SlashObj *obj)
{
    linkedlist_append(gc_objs, obj);
}

void gc_collect(void)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}
