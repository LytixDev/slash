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
#include <assert.h>

#include "arena_ll.h"
#include "sac/sac.h"


void arena_ll_init(Arena *arena, ArenaLL *ll)
{
    assert(arena != NULL && ll != NULL);
    ll->arena = arena;
    ll->head = NULL;
    ll->tail = NULL;
    ll->size = 0;
}

ArenaLL *arena_ll_alloc(Arena *arena)
{
    assert(arena != NULL);

    ArenaLL *ll = m_arena_alloc_struct(arena, ArenaLL);
    arena_ll_init(arena, ll);
    return ll;
}

void arena_ll_append(ArenaLL *ll, void *p)
{
    ll->size++;
    LLItem *item = m_arena_alloc_struct(ll->arena, LLItem);
    item->value = p;
    item->next = NULL;

    /* base case */
    if (ll->head == NULL) {
	ll->head = item;
	ll->tail = ll->head;
	return;
    }

    assert(ll->tail != NULL);
    LLItem *tail = ll->tail;
    tail->next = item;
    ll->tail = item;
}
