/*
 *  (Originally code for the Valery project)
 *  Copyright (C) 2022-2023 Nicolai Brand (https://lytix.dev)
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
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "interactive/prompt.h"

#define cursor_right(n) printf("\033[%zuC", (n))
#define cursor_left(n) printf("\033[%zuD", (n))
#define cursor_goto(x) printf("\033[%zu", (x))
#define flush_line() printf("\33[2K\r")

typedef enum {
    KEY_INVALID = -1,
    KEY_ARROW_1 = 27,
    KEY_ARROW_2 = 91,
    KEY_ARROW_UP = 65,
    KEY_ARROW_DOWN = 66,
    KEY_ARROW_RIGHT = 67,
    KEY_ARROW_LEFT = 68,

    KEY_BACKSPACE = 127
} PromptKey;

static void termconf_begin(Prompt *prompt)
{
    tcgetattr(STDIN_FILENO, &prompt->termios_og);
    prompt->termios_new = prompt->termios_og;
    /* change so buffer don't require new line or similar to return */
    prompt->termios_new.c_lflag &= ~ICANON;
    prompt->termios_new.c_cc[VTIME] = 0;
    prompt->termios_new.c_cc[VMIN] = 1;
    /* turn of echo */
    prompt->termios_new.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &prompt->termios_new);
}

static void termconf_end(Prompt *prompt)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &prompt->termios_og);
}

static void prompt_show(Prompt *prompt)
{
    flush_line();
    printf("%s%s", prompt->ps1, prompt->buf);
    /* move the cursor to its corresponding position */
    if (prompt->cursor_pos != prompt->buf_len)
	cursor_left(prompt->buf_len - prompt->cursor_pos);
}

/* returns the type of arrow consumed from the terminal input buffer */
static PromptKey get_arrow_type(void)
{
    /*
       Arrow keys takes up three chars in the buffer.
       Only last char codes for up or down, so consume
       second char value.
     */
    if (getchar() == KEY_ARROW_2)
	return (PromptKey)getchar();

    /* consume and discard next char */
    getchar();
    return KEY_INVALID;
}

void prompt_run(Prompt *prompt)
{
    prompt_reset(prompt);
    prompt_show(prompt);
    char ch;
    while (EOF != (ch = getchar()) && ch != '\n') {
	switch (ch) {
	case KEY_BACKSPACE: {
	    if (prompt->cursor_pos != 0) {
		prompt->cursor_pos--;
		prompt->buf_len--;
	    }
	} break;

	case KEY_ARROW_1: {
	    PromptKey arrow = get_arrow_type();
	    if (arrow == KEY_ARROW_LEFT) {
		cursor_left((size_t)1);
		prompt->cursor_pos--;
	    } else if (arrow == KEY_ARROW_RIGHT) {
		cursor_right((size_t)1);
		prompt->cursor_pos++;
	    }
	} break;
	}

	prompt->buf[prompt->buf_len++] = ch;
	prompt->cursor_pos++;
	prompt_show(prompt);
    }

    putchar('\n');
    prompt->buf[prompt->buf_len++] = EOF;
}

void prompt_reset(Prompt *prompt)
{
    memset(prompt->buf, 0, prompt->buf_cap);
    prompt->cursor_pos = 0;
    prompt->buf_len = 0;
}


void prompt_init(Prompt *prompt, char *ps1)
{
    prompt->cursor_pos = 0;
    /* init buffer */
    prompt->buf_len = 0;
    prompt->buf_cap = 1024;
    prompt->buf = malloc(prompt->buf_cap);

    size_t ps1_len = 0;
    if (ps1 != NULL)
	ps1_len = strlen(ps1);
    if (ps1_len > 256 || ps1_len == 0) {
	strcpy(prompt->ps1, "->");
	ps1_len = 2;
    } else {
	strncpy(prompt->ps1, ps1, ps1_len);
    }
    prompt->ps1[ps1_len] = 0;

    termconf_begin(prompt);
}

void prompt_free(Prompt *prompt)
{
    free(prompt->buf);
    termconf_end(prompt);
}
