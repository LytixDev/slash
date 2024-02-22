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
#ifndef GC_H
#define GC_H

#include "nicc/nicc.h"


// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC
#define GC_HEAP_GROW_FACTOR 2


typedef struct slash_type_info_t SlashTypeInfo; // Forward decl
typedef struct slash_obj_t SlashObj; // Forward decl
typedef struct slash_value_t SlashValue; // Forward decl
typedef struct interpreter_t Interpreter; // Forward decl

typedef struct {
    LinkedList gc_objs; // objects managed by the GC
    ArrayList gray_stack;
    size_t bytes_managing;
    size_t next_run; // how many bytes allocated until we run the GC again

    ArrayList shadow_stack; // list of objects that are always always marked during gc run.
    unsigned int barrier; // value > 0 means we are in a GC barrier. Value signifies barrier depth.
    size_t shadow_stack_len_pre_barrier; // elements in shadow_stack before barrier
} GC;


void gc_ctx_init(GC *gc);
void gc_ctx_free(GC *gc);
void *gc_alloc(Interpreter *interpreter, size_t size);
void *gc_realloc(Interpreter *interpreter, void *p, size_t old_size, size_t new_size);
void gc_free(Interpreter *interpreter, void *data, size_t size_freed);
SlashObj *gc_new_T(Interpreter *interpreter, SlashTypeInfo *T);
/*
 * Runs the garbage collector.
 * Finds all unreachable objects and frees them.
 */
void gc_run(Interpreter *interpreter);
/*
 * Free all uncollected objects regardless of if they are reachable or not.
 * Used on exit.
 */
void gc_collect_all(Interpreter *interpreter);
/*
 * Objects residing in the shadow stack can not collected.
 */
void gc_shadow_push(GC *gc, SlashObj *obj);
void gc_shadow_pop(GC *gc);
/*
 * GC objects allocated inside a barrier are guaranteed not to be collected before the barrier ends.
 * Example usage:
 * gc_barrier_start(gc);
 * ... some allocations ...
 * gc_barrier_end(gc);
 */
void gc_barrier_start(GC *gc);
void gc_barrier_end(GC *gc);

#endif /* GC_H */
