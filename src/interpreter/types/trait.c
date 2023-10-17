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


void slash_trait_not_defined(SlashValue *value)
{
    printf("Requested trait not defined for '%s'", SLASH_TYPE_TO_STR(value));
}

void slash_print_not_defined(SlashValue *value)
{
    printf("Print not defined for '%s'", SLASH_TYPE_TO_STR(value));
}

void slash_to_str_not_defined(SlashValue *value)
{
    printf("To string not defined for '%s'", SLASH_TYPE_TO_STR(value));
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

SlashValue slash_obj_to_str(Interpreter *interpreter, SlashValue *value)
{
    SlashObj *obj = value->obj;
    if (obj->traits->to_str == NULL)
	REPORT_RUNTIME_ERROR("To string not defined for type '%s'", SLASH_TYPE_TO_STR(value));
    return obj->traits->to_str(interpreter, value);
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

size_t slash_obj_hash(SlashValue *self)
{
    SlashObj *obj = self->obj;
    if (obj->traits->hash == NULL)
	REPORT_RUNTIME_ERROR("Hash defined for this type");
    return obj->traits->hash(self);
}

TraitPrint trait_print[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitPrint)slash_bool_print,
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

TraitToStr trait_to_str[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitToStr)slash_bool_to_str,
    /* num */
    (TraitToStr)slash_num_to_str,
    /* shident */
    (TraitToStr)slash_shident_to_str,
    /* range */
    (TraitToStr)slash_range_to_str,
    /* obj */
    (TraitToStr)slash_obj_to_str,
    /* none */
    (TraitToStr)slash_none_to_str,
};

TraitItemGet trait_item_get[SLASH_TYPE_COUNT] = {
    /* bool */
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

TraitHash trait_hash[SLASH_TYPE_COUNT] = {
    /* bool */
    (TraitHash)NULL,
    /* num */
    (TraitHash)NULL,
    /* shident */
    (TraitHash)NULL,
    /* range */
    (TraitHash)NULL,
    /* obj */
    (TraitHash)slash_obj_hash,
    /* none */
    (TraitHash)NULL,
};
