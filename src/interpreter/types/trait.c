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
#include <stdio.h>

#include "interpreter/error.h"
#include "interpreter/interpreter.h"
#include "interpreter/types/slash_obj.h"
#include "interpreter/types/slash_value.h"


void slash_print_not_defined(SlashValue *value)
{
    printf("Print not defined for %d", value->type);
}

void slash_item_get_not_defined(void)
{
    REPORT_RUNTIME_ERROR("Subscript not defined for this type");
}

void slash_item_assign_not_defined(void)
{
    REPORT_RUNTIME_ERROR("Item assignment not defined for this type");
}

void slash_item_in_not_defined(void)
{
    REPORT_RUNTIME_ERROR("Item in not defined for this type");
}

void slash_obj_print(SlashValue *value)
{
    SlashObj *obj = value->obj;
    if (obj->traits->print == NULL)
	REPORT_RUNTIME_ERROR("Printing not defined for this type");
    obj->traits->print(value);
}


SlashValue slash_obj_item_get(Interpreter *interpreter, SlashValue *self, SlashValue *idx)
{
    SlashObj *obj = self->obj;
    if (obj->traits->item_get == NULL)
	REPORT_RUNTIME_ERROR("Get not defined for this type");
    return obj->traits->item_get(interpreter, self, idx);
}

void slash_obj_item_assign(SlashValue *self, SlashValue *a, SlashValue *b)
{
    SlashObj *obj = self->obj;
    if (obj->traits->item_assign == NULL)
	REPORT_RUNTIME_ERROR("Assign not defined for this type");
    obj->traits->item_assign(self, a, b);
}

bool slash_obj_item_in(SlashValue *self, SlashValue *a)
{
    SlashObj *obj = self->obj;
    if (obj->traits->item_in == NULL)
	REPORT_RUNTIME_ERROR("In not defined for this type");
    return obj->traits->item_in(self, a);
}

TraitPrint trait_print[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitPrint)slash_bool_print,
    /* str */
    (TraitPrint)slash_str_print,
    /* num */
    (TraitPrint)slash_num_print,
    /* shident */
    (TraitPrint)slash_print_not_defined,
    /* range */
    (TraitPrint)slash_range_print,
    /* obj */
    (TraitPrint)slash_obj_print,
    /* none */
    (TraitPrint)slash_none_print,
};

TraitItemGet trait_item_get[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitItemGet)slash_item_get_not_defined,
    /* str */
    (TraitItemGet)slash_item_get_not_defined,
    /* num */
    (TraitItemGet)slash_item_get_not_defined,
    /* shident */
    (TraitItemGet)slash_item_get_not_defined,
    /* range */
    (TraitItemGet)slash_item_get_not_defined,
    /* obj */
    (TraitItemGet)slash_obj_item_get,
    /* none */
    (TraitItemGet)slash_item_get_not_defined,
};

TraitItemAssign trait_item_assign[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitItemAssign)slash_item_assign_not_defined,
    /* str */
    (TraitItemAssign)slash_item_assign_not_defined,
    /* num */
    (TraitItemAssign)slash_item_assign_not_defined,
    /* shident */
    (TraitItemAssign)slash_item_assign_not_defined,
    /* range */
    (TraitItemAssign)slash_item_assign_not_defined,
    /* obj */
    (TraitItemAssign)slash_obj_item_assign,
    /* none */
    (TraitItemAssign)slash_item_assign_not_defined,
};

TraitItemIn trait_item_in[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitItemIn)slash_item_in_not_defined,
    /* str */
    (TraitItemIn)slash_item_in_not_defined,
    /* num */
    (TraitItemIn)slash_item_in_not_defined,
    /* shident */
    (TraitItemIn)slash_item_in_not_defined,
    /* range */
    (TraitItemIn)slash_item_in_not_defined,
    /* obj */
    (TraitItemIn)slash_obj_item_in,
    /* none */
    (TraitItemIn)slash_item_in_not_defined,
};
