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
#ifndef ARENA_LL
#define ARENA_LL

#include "sac/sac.h"


/*
 * Linked list data structure for the sac memory arena
 */

typedef struct arena_ll_item_t LLItem;
struct arena_ll_item_t {
	void *value; // a view into some memory in the arena
	LLItem *next;
};

typedef struct {
	Arena *arena;
	LLItem *head; // always keep a pointer to the head for convenience
	LLItem *tail;
	size_t size; // for convenience
} ArenaLL;


/* Allocates the returning ArenaLL pointer on the arena */
ArenaLL *arena_ll_alloc(Arena *arena);

/*
 * Sets default initial state for the ll
 * arena_ll_alloc() calls this function.
 * The reason this is a seperate function is in the case where you want to control the allocation
 * of the the ArenaLL itself.
 */
void arena_ll_init(Arena *arena, ArenaLL *ll);

void arena_ll_prepend(ArenaLL *ll, void *p);

void arena_ll_append(ArenaLL *ll, void *p);

#define ARENA_LL_FOR_EACH(ll, item) for ((item) = (ll)->head; (item) != NULL; (item) = (item)->next)


#endif /* ARENA_LL */
