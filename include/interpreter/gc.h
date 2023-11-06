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

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#include "interpreter/value/slash_value.h"
#include "nicc/nicc.h"

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
void gc_collect_all(LinkedList *gc_objs);

/*
 * Useful when allocating a collection that can not be marked until every element of the
 * initializtion is allocated.
 */
void gc_shadow_push(ArrayList *gc_shadow_stack, SlashObj *obj);
void gc_shadow_pop(ArrayList *gc_shadow_stack);

#endif /* GC_H */
