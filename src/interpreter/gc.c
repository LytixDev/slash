/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "interpreter/error.h"
#include "interpreter/gc.h"
#include "interpreter/scope.h"
#include "interpreter/value/slash_list.h"
#include "interpreter/value/slash_map.h"
#include "interpreter/value/slash_str.h"
#include "interpreter/value/slash_value.h"
#include "nicc/nicc.h"
#include "options.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif /* DEBUG_LOG_GC */


static void gc_sweep_obj(Interpreter *interpreter, SlashObj *obj)
{
	SlashValue value = AS_VALUE(obj);
	if (IS_MAP(value)) {
		slash_map_impl_free(interpreter, AS_MAP(value));
	} else if (IS_LIST(value)) {
		slash_list_impl_free(interpreter, AS_LIST(value));
	} else if (IS_TUPLE(value)) {
		SlashTuple *tuple = AS_TUPLE(value);
		gc_free(interpreter, tuple->items, tuple->len * sizeof(SlashValue));
	} else if (IS_STR(value)) {
		SlashStr *str = AS_STR(value);
		gc_free(interpreter, str->str, str->len + 1);
	} else {
		REPORT_RUNTIME_ERROR("Sweep not implemented for this obj");
	}

	gc_free(interpreter, obj, value.T->obj_size);
}

static void gc_sweep(Interpreter *interpreter)
{
	LinkedListItem *to_remove = NULL;
	LinkedListItem *current = interpreter->gc.gc_objs.head;

	while (current != NULL) {
		SlashObj *obj = current->data;
		if (!obj->gc_marked && obj->gc_managed) {
#ifdef DEBUG_LOG_GC
			printf("%p sweep ", (void *)obj);
			TraitPrint print_func = obj->T->print;
			assert(print_func != NULL);
			SlashValue value = AS_VALUE(obj);
			print_func(interpreter, value);
			putchar('\n');
#endif
			gc_sweep_obj(interpreter, obj);
			to_remove = current;
			current = current->next;
			linkedlist_remove_item(&interpreter->gc.gc_objs, to_remove);
			continue;
		}

		current = current->next;
	}
}

static void gc_reset(GC *gc)
{
	gc->next_run = gc->bytes_managing * GC_HEAP_GROW_FACTOR;
	if (GC_MIN_RUN > gc->next_run)
		gc->next_run = GC_MIN_RUN;

	for (LinkedListItem *item = gc->gc_objs.head; item != NULL; item = item->next) {
		SlashObj *obj = item->data;
		obj->gc_marked = false;
	}
}

static void gc_visit_obj(Interpreter *interpreter, SlashObj *obj)
{
	assert(obj != NULL);
	if (obj->gc_marked)
		return;
	obj->gc_marked = true;
	arraylist_append(&interpreter->gc.gray_stack, &obj);

#ifdef DEBUG_LOG_GC
	printf("%p mark ", (void *)obj);
	TraitPrint print_func = obj->T->print;
	assert(print_func != NULL);
	SlashValue value = AS_VALUE(obj);
	print_func(interpreter, value);
	putchar('\n');
#endif
}

static void gc_visit_value(Interpreter *interpreter, SlashValue *value)
{
	if (IS_OBJ(*value) && value->obj->gc_managed)
		gc_visit_obj(interpreter, value->obj);
}

static void gc_blacken_obj(Interpreter *interpreter, SlashObj *obj)
{
	SlashValue value = AS_VALUE(obj);
	(void)value;

#ifdef DEBUG_LOG_GC
	printf("%p blacken ", (void *)obj);
	TraitPrint print_func = obj->T->print;
	assert(print_func != NULL);
	print_func(interpreter, value);
	putchar('\n');
#endif

	if (IS_MAP(value)) {
		SlashMap *map = AS_MAP(value);
		if (map->len == 0)
			return;
		/* TODO: VLA bad ?! */
		SlashValue keys[map->len];
		slash_map_impl_get_keys(map, keys);
		for (size_t i = 0; i < map->len; i++) {
			gc_visit_value(interpreter, &keys[i]);
			SlashValue v = slash_map_impl_get(map, keys[i]);
			gc_visit_value(interpreter, &v);
		}
	} else if (IS_LIST(value)) {
		SlashList *list = AS_LIST(value);
		for (size_t i = 0; i < list->len; i++) {
			SlashValue v = slash_list_impl_get(list, i);
			gc_visit_value(interpreter, &v);
		}
	} else if (IS_TUPLE(value)) {
		SlashTuple *tuple = AS_TUPLE(value);
		for (size_t i = 0; i < tuple->len; i++)
			gc_visit_value(interpreter, &tuple->items[i]);
	} else if (IS_STR(value)) {
		return;
	} else {
		REPORT_RUNTIME_ERROR("gc blacken not implemented for this object type");
	}
}

static void gc_mark_roots(Interpreter *interpreter)
{
	GC *gc = &interpreter->gc;
	/* Mark all objects in shadow stack */
	for (size_t i = 0; i < gc->shadow_stack.size; i++)
		gc_visit_obj(interpreter, *(SlashObj **)arraylist_get(&gc->shadow_stack, i));

	/* mark all reachable objects */
	for (Scope *scope = interpreter->scope; scope != NULL; scope = scope->enclosing) {
		/* loop over all values */
		if (scope->values.len == 0)
			continue;
		// TODO: VLA bad
		SlashValue *values[scope->values.len];
		hashmap_get_values(&scope->values, (void **)values);

		for (size_t i = 0; i < scope->values.len; i++)
			gc_visit_value(interpreter, values[i]);
	}
}

static void gc_trace_references(Interpreter *interpreter)
{
	SlashObj *obj;
	while (interpreter->gc.gray_stack.size != 0) {
		arraylist_pop_and_copy(&interpreter->gc.gray_stack, &obj);
		gc_blacken_obj(interpreter, obj);
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
	gc->next_run = GC_MIN_RUN;
	gc->barrier = 0;
}

void gc_ctx_free(GC *gc)
{
	linkedlist_free(&gc->gc_objs);
	arraylist_free(&gc->shadow_stack);
	arraylist_free(&gc->gray_stack);
}

void *gc_alloc(Interpreter *interpreter, size_t size)
{
#ifdef DEBUG_STRESS_GC
	gc_run(interpreter);
#endif
	interpreter->gc.bytes_managing += size;
	if (interpreter->gc.bytes_managing > interpreter->gc.next_run)
		gc_run(interpreter);
#ifdef DEBUG_LOG_GC
	printf("gc_alloc %zu bytes\n", size);
	printf("barrier state %d \n", interpreter->gc.barrier);
#endif
	return malloc(size);
}

void *gc_realloc(Interpreter *interpreter, void *p, size_t old_size, size_t new_size)
{
#ifdef DEBUG_LOG_GC
	printf("gc_realloc diff of %zu bytes\n", new_size - old_size);
#endif
	interpreter->gc.bytes_managing += new_size - old_size;
	if (interpreter->gc.bytes_managing > interpreter->gc.next_run)
		gc_run(interpreter);
	return realloc(p, new_size);
}

void gc_free(Interpreter *interpreter, void *data, size_t size_freed)
{
	interpreter->gc.bytes_managing -= size_freed;
	free(data);
}

SlashObj *gc_new_T(Interpreter *interpreter, SlashTypeInfo *T)
{
#ifdef DEBUG_LOG_GC
	printf("GC new %s\n", T->name);
#endif
	SlashObj *obj = gc_alloc(interpreter, T->obj_size);
	obj->T = T;
	obj->gc_marked = true;
	obj->gc_managed = true;
	gc_register(&interpreter->gc, obj);
	if (interpreter->gc.barrier)
		gc_shadow_push(&interpreter->gc, obj);
	return obj;
}

void gc_run(Interpreter *interpreter)
{
#ifdef DEBUG_LOG_GC
	size_t pre = interpreter->gc.bytes_managing;
	printf("-- gc begin\n");
#endif
	gc_mark_roots(interpreter);
	gc_trace_references(interpreter);
#ifdef DEBUG_LOG_GC
	printf("-- gc sweep\n");
#endif
	gc_sweep(interpreter);
	gc_reset(&interpreter->gc);

#ifdef DEBUG_LOG_GC
	printf("gc freed %zu bytes\n", pre - interpreter->gc.bytes_managing);
	printf("-- gc end\n");
#endif
}

void gc_collect_all(Interpreter *interpreter)
{
#ifdef DEBUG_LOG_GC
	size_t pre = interpreter->gc.bytes_managing;
	printf("-- gc collect all begin\n");
#endif
	for (LinkedListItem *item = interpreter->gc.gc_objs.head; item != NULL; item = item->next) {
		SlashObj *obj = item->data;
		gc_sweep_obj(interpreter, obj);
	}
#ifdef DEBUG_LOG_GC
	printf("gc freed %zu bytes\n", pre - interpreter->gc.bytes_managing);
	printf("gc bytes managing: %zu bytes\n", interpreter->gc.bytes_managing);
	printf("gc barrier state: %d \n", interpreter->gc.barrier);
	printf("-- gc collect all end\n");
#endif
}

void gc_shadow_push(GC *gc, SlashObj *obj)
{
#ifdef DEBUG_LOG_GC
	printf("%p pushed to shadow\n", (void *)obj);
#endif /* DEBUG_LOG_GC */
	arraylist_append(&gc->shadow_stack, &obj);
}

void gc_shadow_pop(GC *gc)
{
#ifdef DEBUG_LOG_GC
	SlashObj **obj_ptr = arraylist_get(&gc->shadow_stack, gc->shadow_stack.size - 1);
	SlashObj *obj = *obj_ptr;
	printf("%p popped from shadow stack ->", (void *)obj);
	TraitPrint print_func = obj->T->print;
	assert(print_func != NULL);
	SlashValue value = AS_VALUE(obj);
	print_func(interpreter, value);
	putchar('\n');
#endif /* DEBUG_LOG_GC */
	/* Since we are using the ArrayList as a stack we simply decrement its size */
	assert(gc->shadow_stack.size != 0);
	gc->shadow_stack.size--;
}

void gc_barrier_start(GC *gc)
{
	gc->barrier++;
	if (gc->barrier == 1)
		gc->shadow_stack_len_pre_barrier = gc->shadow_stack.size;
}

void gc_barrier_end(GC *gc)
{
	gc->barrier--;
	if (gc->barrier == 0)
		gc->shadow_stack.size = gc->shadow_stack_len_pre_barrier;
}
