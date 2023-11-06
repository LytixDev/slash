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
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_value.h"
#include "nicc/nicc.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif /* DEBUG_LOG_GC */


static void gc_sweep_obj(SlashObj *obj)
{
    /// SlashValue value = AS_VALUE(obj);
    /// if (IS_MAP(value)) {
    ///     // slash_map_impl_free(AS_MAP(value)->map);
    /// } else if (IS_LIST(value)) {
    ///     // arraylist_free(&AS_LIST(value)->list);
    /// } else if (IS_TUPLE(value)) {
    ///     // free(AS_TUPLE(value)->tuple);
    /// } else if (IS_STR(value)) {
    ///     free(AS_STR(value)->str);
    /// } else {
    ///     REPORT_RUNTIME_ERROR("Sweep not implemented for this obj");
    /// }

    free(obj);
}

static void gc_sweep(GC *gc)
{
    LinkedListItem *to_remove = NULL;
    LinkedListItem *current = gc->gc_objs.head;

    while (current != NULL) {
	SlashObj *obj = current->data;
	if (!obj->gc_marked && obj->gc_managed) {
#ifdef DEBUG_LOG_GC
	    printf("%p sweep ", (void *)obj);
	    TraitPrint print_func = obj->T->print;
	    assert(print_func != NULL);
	    SlashValue value = AS_VALUE(obj);
	    print_func(value);
	    putchar('\n');
#endif
	    gc_sweep_obj(obj);
	    to_remove = current;
	    current = current->next;
	    linkedlist_remove_item(&gc->gc_objs, to_remove);
	    continue;
	}

	current = current->next;
    }
}

static void gc_reset(GC *gc)
{
    for (LinkedListItem *item = gc->gc_objs.head; item != NULL; item = item->next) {
	SlashObj *obj = item->data;
	obj->gc_marked = false;
    }
}

static void gc_visit_obj(GC *gc, SlashObj *obj)
{
    assert(obj != NULL);
    if (obj->gc_marked)
	return;
    obj->gc_marked = true;
    arraylist_append(&gc->gray_stack, &obj);

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)obj);
    TraitPrint print_func = obj->T->print;
    assert(print_func != NULL);
    SlashValue value = AS_VALUE(obj);
    print_func(value);
    putchar('\n');
#endif
}

static void gc_visit_value(GC *gc, SlashValue *value)
{
    if (IS_OBJ(*value) && value->obj->gc_managed)
	gc_visit_obj(gc, value->obj);
}

static void gc_blacken_obj(GC *gc, SlashObj *obj)
{
    SlashValue value = AS_VALUE(obj);
    (void)gc;
    (void)value;

#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)obj);
    TraitPrint print_func = obj->T->print;
    assert(print_func != NULL);
    print_func(value);
    putchar('\n');
#endif

    /// if (IS_MAP(value)) {
    /// } else if (IS_LIST(value)) {
    /// } else if (IS_TUPLE(value)) {
    /// } else if (IS_STR(value)) {
    /// } else {
    ///     REPORT_RUNTIME_ERROR("gc blacken not implemented for this object type");
    /// }
}

static void gc_mark_roots(Interpreter *interpreter)
{
    GC *gc = &interpreter->gc;
    /* Mark all objects in shadow stack */
    for (size_t i = 0; i < gc->shadow_stack.size; i++)
	gc_visit_obj(gc, *(SlashObj **)arraylist_get(&gc->shadow_stack, i));

    /* mark all reachable objects */
    for (Scope *scope = interpreter->scope; scope != NULL; scope = scope->enclosing) {
	/* loop over all values */
	if (scope->values.len == 0)
	    continue;
	// TODO: VLA bad
	SlashValue *values[scope->values.len];
	hashmap_get_values(&scope->values, (void **)values);

	for (size_t i = 0; i < scope->values.len; i++)
	    gc_visit_value(gc, values[i]);
    }
}

static void gc_trace_references(GC *gc)
{
    SlashObj *obj;
    while (gc->gray_stack.size != 0) {
	arraylist_pop_and_copy(&gc->gray_stack, &obj);
	gc_blacken_obj(gc, obj);
    }
}

static void gc_register(GC *gc, SlashObj *obj)
{
    linkedlist_append(&gc->gc_objs, obj);
}

void gc_ctx_init(GC *gc)
{
    linkedlist_init(&gc->gc_objs, sizeof(SlashObj *));
    arraylist_init(&gc->gray_stack, sizeof(SlashObj *));
    arraylist_init(&gc->shadow_stack, sizeof(SlashObj **));

    gc->bytes_managing = 0;
    gc->next_run = 0;
}

void gc_ctx_free(GC *gc)
{
    linkedlist_free(&gc->gc_objs);
    arraylist_free(&gc->shadow_stack);
    arraylist_free(&gc->gray_stack);
}

void *gc_alloc(GC *gc, size_t size)
{
    gc->bytes_managing += size;
    return malloc(size);
}

void *gc_realloc(GC *gc, void *p, size_t old_size, size_t new_size)
{
    gc->bytes_managing += new_size - old_size;
    return realloc(p, new_size);
}

void gc_free(GC *gc, void *data, size_t size_freed)
{
    gc->bytes_managing -= size_freed;
    free(data);
}

SlashObj *gc_new_T(GC *gc, SlashTypeInfo *T)
{
    SlashObj *obj = gc_alloc(gc, T->obj_size);
    obj->T = T;
    obj->gc_marked = true;
    obj->gc_managed = true;
    gc_register(gc, obj);
    return obj;
}

void gc_run(Interpreter *interpreter)
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif
    gc_mark_roots(interpreter);
    gc_trace_references(&interpreter->gc);
#ifdef DEBUG_LOG_GC
    printf("-- gc sweep\n");
#endif
    gc_sweep(&interpreter->gc);
    gc_reset(&interpreter->gc);

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
}

void gc_collect_all(Interpreter *interpreter)
{
    for (LinkedListItem *item = interpreter->gc.gc_objs.head; item != NULL; item = item->next) {
	SlashObj *obj = item->data;
	gc_sweep_obj(obj);
    }
}

void gc_shadow_push(GC *gc, SlashObj *obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p push to shadow stack ", (void *)obj);
    TraitPrint print_func = obj->T->print;
    assert(print_func != NULL);
    SlashValue value = AS_VALUE(obj);
    print_func(value);
    putchar('\n');
#endif /* DEBUG_LOG_GC */
    arraylist_append(&gc->shadow_stack, &obj);
}

void gc_shadow_pop(GC *gc)
{
#ifdef DEBUG_LOG_GC
    SlashObj **obj_ptr = arraylist_get(&gc->shadow_stack, gc->shadow_stack.size - 1);
    SlashObj *obj = *obj_ptr;
    printf("%p pop shadow stack", (void *)obj);
    TraitPrint print_func = obj->T->print;
    assert(print_func != NULL);
    SlashValue value = AS_VALUE(obj);
    print_func(value);
    putchar('\n');
#endif /* DEBUG_LOG_GC */
    arraylist_rm(&gc->shadow_stack, gc->shadow_stack.size - 1);
}
