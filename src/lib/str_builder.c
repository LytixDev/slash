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
#include <stdio.h>
#include <string.h>

#include "lib/str_builder.h"
#include "lib/str_view.h"
#include "sac/sac.h"


void str_builder_init(StrBuilder *sb, Arena *arena)
{
    sb->arena = arena;
    sb->buffer = (char *)(arena->memory + arena->offset);
    sb->len = 0;
    sb->cap = 32;
    m_arena_alloc(sb->arena, sb->cap);
}

void str_builder_append(StrBuilder *sb, char *cstr, size_t len)
{
    while (sb->len + len >= sb->cap) {
	m_arena_alloc(sb->arena, sb->cap);
	sb->cap += sb->cap;
    }
    memcpy(sb->buffer + sb->len, cstr, len);
    sb->len += len;
}

void str_builder_append_char(StrBuilder *sb, char c)
{
    if (sb->len >= sb->cap) {
	m_arena_alloc(sb->arena, sb->cap);
	sb->cap += sb->cap;
    }
    sb->buffer[sb->len] = c;
    sb->len++;
}

char str_builder_peek(StrBuilder *sb)
{
    if (sb->len == 0)
	return EOF;
    return sb->buffer[sb->len - 1];
}

StrView str_builder_complete(StrBuilder *sb)
{
    str_builder_append_char(sb, '\0');
    return (StrView){ .view = sb->buffer, .size = sb->len };
}
