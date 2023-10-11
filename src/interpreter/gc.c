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
#include "interpreter/types/slash_str.h"
#include "interpreter/types/slash_tuple.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif /* DEBUG_LOG_GC */


static void gc_sweep_obj(SlashObj *obj)
{
    switch (obj->type) {
    case SLASH_OBJ_LIST: {
	SlashList *list = (SlashList *)obj;
	arraylist_free(&list->underlying);
	break;
    }
    case SLASH_OBJ_MAP: {
	SlashMap *map = (SlashMap *)obj;
	hashmap_free(&map->underlying);
	break;
    }
    case SLASH_OBJ_TUPLE: {
	SlashTuple *tuple = (SlashTuple *)obj;
	free(tuple->values);
	break;
    }
    case SLASH_OBJ_STR: {
	SlashStr *str = (SlashStr *)obj;
	free(str->p);
	break;
    }
    default:
	REPORT_RUNTIME_ERROR("Sweep not implemented for this obj");
    }
    free(obj);
}

static void gc_sweep(LinkedList *gc_objs)
{
    LinkedListItem *to_remove = NULL;
    LinkedListItem *current = gc_objs->head;

    while (current != NULL) {
	SlashObj *obj = current->data;
	if (!obj->gc_marked && obj->gc_managed) {
#ifdef DEBUG_LOG_GC
	    printf("%p sweep ", (void *)obj);
	    TraitPrint print_func = trait_print[SLASH_OBJ];
	    SlashValue value = { .type = SLASH_OBJ, .obj = obj };
	    print_func(&value);
	    putchar('\n');
#endif
	    gc_sweep_obj(obj);
	    to_remove = current;
	    current = current->next;
	    linkedlist_remove_item(gc_objs, to_remove);
	    continue;
	}

	current = current->next;
    }
}

static void gc_reset(Interpreter *interpreter)
{
    for (LinkedListItem *item = interpreter->gc_objs.head; item != NULL; item = item->next) {
	SlashObj *obj = item->data;
	obj->gc_marked = false;
    }

    interpreter->obj_alloced_since_next_gc = 0;
}

static void gc_visit_obj(Interpreter *interpreter, SlashObj *obj)
{
    assert(obj != NULL);
    if (obj->gc_marked)
	return;
    obj->gc_marked = true;
    arraylist_append(&interpreter->gc_gray_stack, &obj);


#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)obj);
    TraitPrint print_func = trait_print[SLASH_OBJ];
    SlashValue value = { .type = SLASH_OBJ, .obj = obj };
    print_func(&value);
    putchar('\n');
#endif
}

static void gc_visit_value(Interpreter *interpreter, SlashValue *value)
{
    if (IS_OBJ(value->type) && value->obj->gc_managed)
	gc_visit_obj(interpreter, value->obj);
}

static void gc_blacken_obj(Interpreter *interpreter, SlashObj *obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)obj);
    TraitPrint print_func = trait_print[SLASH_OBJ];
    SlashValue value = { .type = SLASH_OBJ, .obj = obj };
    print_func(&value);
    putchar('\n');
#endif

    switch (obj->type) {
    case SLASH_OBJ_LIST: {
	SlashList *list = (SlashList *)obj;
	for (size_t i = 0; i < list->underlying.size; i++) {
	    SlashValue *v = arraylist_get(&list->underlying, i);
	    gc_visit_value(interpreter, v);
	}
	break;
    }
    case SLASH_OBJ_TUPLE: {
	SlashTuple *tuple = (SlashTuple *)obj;
	for (size_t i = 0; i < tuple->size; i++)
	    gc_visit_value(interpreter, &tuple->values[i]);
	break;
    }
    case SLASH_OBJ_MAP: {
	SlashMap *map = (SlashMap *)obj;
	if (map->underlying.len == 0)
	    break;
	SlashValue *keys[map->underlying.len];
	hashmap_get_keys(&map->underlying, (void **)keys);
	for (size_t i = 0; i < map->underlying.len; i++) {
	    gc_visit_value(interpreter, keys[i]);
	    SlashValue *v = hashmap_get(&map->underlying, keys[i], sizeof(SlashValue));
	    assert(v != NULL);
	    gc_visit_value(interpreter, v);
	}
	break;
    }
    case SLASH_OBJ_STR:
	break;

    default:
	REPORT_RUNTIME_ERROR("gc blacken not implemented for this object type");
    }
}


static void gc_mark_roots(Interpreter *interpreter)
{
    for (size_t i = 0; i < interpreter->gc_shadow_stack.size; i++) {
	gc_visit_obj(interpreter, *(SlashObj **)arraylist_get(&interpreter->gc_shadow_stack, i));
    }

    /* mark all reachable objects */
    for (Scope *scope = interpreter->scope; scope != NULL; scope = scope->enclosing) {
	/* loop over all values */
	// TODO: can we get a stack overflow if the amount of values grow too large?
	if (scope->values.len == 0)
	    continue;
	SlashValue *values[scope->values.len];
	hashmap_get_values(&scope->values, (void **)values);

	for (size_t i = 0; i < scope->values.len; i++)
	    gc_visit_value(interpreter, values[i]);
    }
}

static void gc_trace_references(Interpreter *interpreter)
{
    SlashObj *obj;
    while (interpreter->gc_gray_stack.size != 0) {
	arraylist_pop_and_copy(&interpreter->gc_gray_stack, &obj);
	gc_blacken_obj(interpreter, obj);
    }
}

static void gc_register(LinkedList *gc_objs, SlashObj *obj)
{
    linkedlist_append(gc_objs, obj);
}

void gc_run(Interpreter *interpreter)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    gc_mark_roots(interpreter);
    gc_trace_references(interpreter);
#ifdef DEBUG_LOG_GC
    printf("-- gc sweep\n");
#endif
    gc_sweep(&interpreter->gc_objs);
    gc_reset(interpreter);

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

void gc_collect_all(LinkedList *gc_objs)
{
    for (LinkedListItem *item = gc_objs->head; item != NULL; item = item->next) {
	SlashObj *obj = item->data;
	gc_sweep_obj(obj);
    }
}

SlashObj *gc_alloc(Interpreter *interpreter, SlashObjType type)
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
    case SLASH_OBJ_STR:
	size = sizeof(SlashStr);
	break;
    default:
	REPORT_RUNTIME_ERROR("Slash obj not implemented");
	ASSERT_NOT_REACHED;
    }

    obj = malloc(size);
    obj->type = type;
    obj->gc_marked = false;
    obj->gc_managed = true;
    obj->traits = NULL;

    // TODO: this is a lousy strategy
    //       a proper solution should track how many bytes has been allocated since the last
    //       time the gc ran.
#ifndef DEBUG_STRESS_GC
    interpreter->obj_alloced_since_next_gc++;
    if (interpreter->obj_alloced_since_next_gc > 10)
	gc_run(interpreter);
#else
    gc_run(interpreter);
#endif


    gc_register(&interpreter->gc_objs, obj);
    return obj;
}

void gc_shadow_push(ArrayList *gc_shadow_stack, SlashObj *obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p push to shadow stack ", (void *)obj);
    TraitPrint print_func = trait_print[SLASH_OBJ];
    SlashValue value = { .type = SLASH_OBJ, .obj = obj };
    print_func(&value);
    putchar('\n');
#endif /* DEBUG_LOG_GC */
    arraylist_append(gc_shadow_stack, &obj);
}

void gc_shadow_pop(ArrayList *gc_shadow_stack)
{
#ifdef DEBUG_LOG_GC
    SlashObj **obj = arraylist_get(gc_shadow_stack, gc_shadow_stack->size - 1);
    printf("%p pop shadow stack", (void *)*obj);
    TraitPrint print_func = trait_print[SLASH_OBJ];
    SlashValue value = { .type = SLASH_OBJ, .obj = *obj };
    print_func(&value);
    putchar('\n');
#endif /* DEBUG_LOG_GC */
    arraylist_rm(gc_shadow_stack, gc_shadow_stack->size - 1);
}
