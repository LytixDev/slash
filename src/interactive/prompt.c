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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "interactive/prompt.h"


#define cursor_right(n) printf("\033[%zuC", (size_t)(n))
#define cursor_left(n) printf("\033[%zuD", (size_t)(n))
#define cursor_goto(x) printf("\033[%zu", (size_t)(x))
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
    /* in ASCII arrows are determined by three chars were the final determines the arrow type */
    if (getchar() == KEY_ARROW_2)
	return (PromptKey)getchar();

    /* consume and discard next char */
    getchar();
    return KEY_INVALID;
}

static void prompt_buf_ensure_capacity(Prompt *prompt)
{
    /* -1 because we always have to have space for the sentinel null byte */
    if (prompt->buf_len >= prompt->buf_cap - 1) {
	prompt->buf_cap *= 2;
	printf("\nREALLOC\n\n");
	prompt->buf = realloc(prompt->buf, prompt->buf_cap);
    }
}

static void prompt_buf_append(Prompt *prompt, char c)
{
    prompt_buf_ensure_capacity(prompt);
    prompt->buf[prompt->buf_len++] = c;
    prompt->buf[prompt->buf_len] = 0;
    prompt->cursor_pos++;
}

static void prompt_buf_remove_at_cursor(Prompt *prompt)
{
    char tmp[prompt->buf_len];
    strncpy(tmp, prompt->buf, prompt->cursor_pos - 1);
    strcpy(tmp + prompt->cursor_pos - 1, prompt->buf + prompt->cursor_pos);
    strcpy(prompt->buf, tmp);
}

static void prompt_buf_insert_at_cursor(Prompt *prompt, char c)
{
    /* ensure enough capacity for 'c' */
    prompt_buf_ensure_capacity(prompt);

    char tmp[prompt->buf_len];
    strncpy(tmp, prompt->buf, prompt->cursor_pos);
    tmp[prompt->cursor_pos] = c;
    strcpy(tmp + prompt->cursor_pos + 1, prompt->buf + prompt->cursor_pos);
    strcpy(prompt->buf, tmp);

    prompt->cursor_pos++;
    prompt->buf_len++;
    prompt_buf_ensure_capacity(prompt);
    prompt->buf[prompt->buf_len] = 0;
}

static void handle_backspace(Prompt *prompt)
{
    /* ignore backspace leftmost position */
    if (prompt->cursor_pos == 0)
	return;

    prompt_buf_remove_at_cursor(prompt);
    prompt->cursor_pos--;
    prompt->buf_len--;
    prompt->buf[prompt->buf_len] = 0;
}

static void handle_arrow(Prompt *prompt)
{
    PromptKey arrow = get_arrow_type();
    if (arrow == KEY_ARROW_LEFT && prompt->cursor_pos != 0) {
	cursor_left(1);
	prompt->cursor_pos--;
    } else if (arrow == KEY_ARROW_RIGHT && prompt->cursor_pos != prompt->buf_len) {
	cursor_right(1);
	prompt->cursor_pos++;
    }
}


bool prompt_run(Prompt *prompt)
{
    prompt_reset(prompt);
    prompt_show(prompt);
    char ch;
    while (EOF != (ch = getchar()) && ch != '\n') {
	switch (ch) {
	case KEY_BACKSPACE:
	    handle_backspace(prompt);
	    break;

	case KEY_ARROW_1:
	    handle_arrow(prompt);
	    break;

	default:
	    if (prompt->cursor_pos == prompt->buf_len)
		prompt_buf_append(prompt, ch);
	    else
		prompt_buf_insert_at_cursor(prompt, ch);
	}

	prompt_show(prompt);
    }

    // TODO: exit builtin means this branch is never taken
    if (strcmp(prompt->buf, "exit") == 0)
	return false;

    putchar('\n');
    prompt_buf_append(prompt, '\n');
    prompt_buf_append(prompt, EOF);
    return true;
}

void prompt_reset(Prompt *prompt)
{
    prompt->cursor_pos = 0;
    prompt->buf_len = 0;
    prompt->buf[0] = 0;
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
