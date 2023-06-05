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

#include "lexer.h"

int main(void)
{
    // https://www.youtube.com/watch?v=HxaD_trXwRE
    char input[1024];
    FILE *fp = fopen("src/test.slash", "r");
    if (fp == NULL) {
	return -1;
    }

    int c;
    size_t counter = 0;
    do {
	c = fgetc(fp);
	input[counter++] = c;
	if (counter == 1024)
	    break;
    } while (c != EOF);

    input[--counter] = 0;

    struct darr_t *tokens = lex(input);
    tokens_print(tokens);
}
