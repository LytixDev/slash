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

#include "interpreter/types/slash_value.h"

SlashTypeInfo bool_type_info = { .name = "bool" };
SlashTypeInfo num_type_info = { .name = "num" };
SlashTypeInfo range_type_info = { .name = "range" };
SlashTypeInfo map_type_info = { .name = "map" };
SlashTypeInfo list_type_info = { .name = "list" };
SlashTypeInfo tuple_type_info = { .name = "tuple" };
SlashTypeInfo str_type_info = { .name = "str" };
