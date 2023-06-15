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
#include "interpreter/types/slash_list.h"
#include "interpreter/types/slash_value.h"
#include "nicc/nicc.h"

void slash_list_init(SlashList *list)
{
    arraylist_init(&list->underlying, sizeof(SlashValue *));
    list->T = SVT_NONE;
}

bool slash_list_append(SlashList *list, SlashValue *val)
{
    return arraylist_append(&list->underlying, val);
}
