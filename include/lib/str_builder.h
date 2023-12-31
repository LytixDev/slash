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
#ifndef STR_BUILDER_H
#define STR_BUILDER_H

#include <stdlib.h>

#include "sac/sac.h"


/*
 * String builder that uses an arena for its backing memory.
 */
typedef struct {
    char *buffer;
    size_t len; // bytes in use
    size_t cap; // bytes allocated
    Arena *arena;
} StrBuilder;


/* 
 * Inits the StrBuilder and attaches the backing arena.
 * The backing arena must be manually released to get back the memory.
 *
 * Example use case:
 *  ArenaTmp tmp = m_arena_tmp_init(&arena);
 *  StrBuilder sb;
 *  str_builder_init(&sb, tmp.arena);
 *  str_builder_append(&sb, "Hello ", 6);
 *  str_builder_append(&sb, "World", 5);
 *  str_builder_complete(&sb);
 *  char *str = sb->str
 *  ...
 *  m_arena_tmp_release(tmp);
 */
void str_builder_init(StrBuilder *sb, Arena *arena);

void str_builder_append(StrBuilder *sb, char *cstr, size_t len);
void str_builder_append_char(StrBuilder *sb, char c);

/* Appends the null terminator. */
void str_builder_complete(StrBuilder *sb);


#endif /* STR_BUILDER_H */
