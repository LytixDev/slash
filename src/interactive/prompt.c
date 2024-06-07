/*
 *  (Originally code for the Valery project)
 *  Copyright (C) 2022-2024 Nicolai Brand (https://lytix.dev)
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


#define CURSOR_LEFT(n) printf("\033[%zuD", (size_t)(n))
#define FLUSH_LINE() printf("\33[2K\r")
#define PROMPT_ABS_POS(prompt) (prompt)->cursor_pos_in_line + (prompt)->prev_line_end


typedef enum {
    KEY_INVALID = -1,
    KEY_ARROW_1 = 27,
    KEY_ARROW_2 = 91,
    KEY_ARROW_UP = 65,
    KEY_ARROW_DOWN = 66,
    KEY_ARROW_RIGHT = 67,
    KEY_ARROW_LEFT = 68,

    KEY_BACKSPACE = 127,
    KEY_TAB = '\t',
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
    FLUSH_LINE();
    printf("%s%s", prompt->ps1, prompt->buf + prompt->prev_line_end);
    /* move the cursor to its corresponding position */
    if (PROMPT_ABS_POS(prompt) != prompt->buf_len) {
	ssize_t left_shift = PROMPT_ABS_POS(prompt) - prompt->buf_len;
	CURSOR_LEFT(-left_shift);
    }
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
	prompt->buf = realloc(prompt->buf, prompt->buf_cap);
    }
}

static void prompt_buf_append(Prompt *prompt, char c)
{
    prompt_buf_ensure_capacity(prompt);
    prompt->buf[prompt->buf_len++] = c;
    prompt->buf[prompt->buf_len] = 0;
    prompt->cursor_pos_in_line++;
}

static void prompt_buf_remove_at_cursor(Prompt *prompt)
{
    char tmp[prompt->buf_len];
    size_t current_pos = PROMPT_ABS_POS(prompt);
    strncpy(tmp, prompt->buf, current_pos - 1);
    strcpy(tmp + current_pos - 1, prompt->buf + current_pos);
    strcpy(prompt->buf, tmp);
}

static void prompt_buf_insert_at_cursor(Prompt *prompt, char c)
{
    /* ensure enough capacity for 'c' */
    prompt_buf_ensure_capacity(prompt);

    size_t current_pos = PROMPT_ABS_POS(prompt);
    char tmp[prompt->buf_len];
    strncpy(tmp, prompt->buf, current_pos);
    tmp[current_pos] = c;
    strcpy(tmp + current_pos + 1, prompt->buf + current_pos);
    strcpy(prompt->buf, tmp);

    prompt->cursor_pos_in_line++;
    prompt->buf_len++;
    prompt_buf_ensure_capacity(prompt);
    prompt->buf[prompt->buf_len] = 0;
}

static void handle_backspace(Prompt *prompt)
{
    /* ignore backspace leftmost position */
    if (prompt->cursor_pos_in_line == 0)
	return;

    prompt_buf_remove_at_cursor(prompt);
    prompt->cursor_pos_in_line--;
    prompt->buf_len--;
    prompt->buf[prompt->buf_len] = 0;
}

static void handle_arrow(Prompt *prompt)
{
    PromptKey arrow = get_arrow_type();
    if (arrow == KEY_ARROW_LEFT && prompt->cursor_pos_in_line != 0)
	prompt->cursor_pos_in_line--;
    else if (arrow == KEY_ARROW_RIGHT && PROMPT_ABS_POS(prompt) != prompt->buf_len)
	prompt->cursor_pos_in_line++;
}

static void prompt_reset(Prompt *prompt, bool continuation)
{
    prompt->cursor_pos_in_line = 0;
    if (!continuation) {
	prompt->prev_line_end = 0;
	prompt->buf_len = 0;
	prompt->buf[0] = 0;
    }
}

void prompt_run(Prompt *prompt, bool continuation)
{
    prompt_reset(prompt, continuation);
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

	case KEY_TAB:
	    for (size_t i = 0; i < 4; i++)
		prompt_buf_insert_at_cursor(prompt, ' ');
	    break;

	default:
	    if (PROMPT_ABS_POS(prompt) == prompt->buf_len)
		prompt_buf_append(prompt, ch);
	    else
		prompt_buf_insert_at_cursor(prompt, ch);
	}

	prompt_show(prompt);
    }

    prompt_buf_append(prompt, '\n');
    prompt_buf_append(prompt, EOF);
    prompt->prev_line_end = prompt->buf_len - 1; /* -1 because prev line ends before EOF */

    putchar('\n');
}

void prompt_set_ps1(Prompt *prompt, char *ps1)
{
    size_t ps1_len = 0;
    if (ps1 != NULL)
	ps1_len = strlen(ps1);
    if (ps1_len > 256 || ps1_len == 0) {
	strcpy(prompt->ps1, "-> ");
	ps1_len = 2;
    } else {
	strncpy(prompt->ps1, ps1, ps1_len);
    }
    prompt->ps1[ps1_len] = 0;
}

void prompt_init(Prompt *prompt, char *ps1)
{
    prompt->cursor_pos_in_line = 0;
    prompt->prev_line_end = 0;
    /* init buffer */
    prompt->buf_len = 0;
    prompt->buf_cap = 1024;
    prompt->buf = malloc(prompt->buf_cap);
    prompt_set_ps1(prompt, ps1);
    termconf_begin(prompt);
}

void prompt_free(Prompt *prompt)
{
    free(prompt->buf);
    termconf_end(prompt);
}
