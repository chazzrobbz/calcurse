/*
 * Calcurse - text-based organizer
 *
 * Copyright (c) 2004-2022 calcurse Development Team <misc@calcurse.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer in the documentation and/or other
 *        materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Send your feedback or comments to : misc@calcurse.org
 * Calcurse home page : http://calcurse.org
 *
 */

#include "calcurse.h"

static void listbox_fix_sel(struct listbox *, int);
static void listbox_sel_in_view(struct listbox *);

void listbox_init(struct listbox *lb, int y, int x, int h, int w,
		  const char *label, listbox_fn_item_type_t fn_type,
		  listbox_fn_item_height_t fn_height,
		  listbox_fn_draw_item_t fn_draw)
{
	EXIT_IF(lb == NULL, "null pointer");
	wins_scrollwin_init(&(lb->sw), y, x, h, w, label);
	lb->item_count = lb->item_sel = 0;
	lb->fn_type = fn_type;
	lb->type = NULL;
	lb->fn_height = fn_height;
	lb->ch = NULL;
	lb->fn_draw = fn_draw;
	lb->cb_data = NULL;
}

void listbox_delete(struct listbox *lb)
{
	EXIT_IF(lb == NULL, "null pointer");
	wins_scrollwin_delete(&(lb->sw));
	mem_free(lb->type);
	mem_free(lb->ch);
}

void listbox_resize(struct listbox *lb, int y, int x, int h, int w)
{
	EXIT_IF(lb == NULL, "null pointer");
	wins_scrollwin_resize(&(lb->sw), y, x, h, w);

	if (lb->item_sel < 0)
		return;

	listbox_sel_in_view(lb);
}

void listbox_set_cb_data(struct listbox *lb, void *cb_data)
{
	lb->cb_data = cb_data;
}

void listbox_load_items(struct listbox *lb, int item_count)
{
	int i, ch;

	lb->item_count = item_count;
	if (item_count == 0) {
		lb->item_sel = -1;
		return;
	}

	mem_free(lb->type);
	mem_free(lb->ch);
	lb->type = mem_malloc(item_count * sizeof(unsigned));
	lb->ch = mem_malloc((item_count + 1) * sizeof(unsigned));
	for (i = 0, ch = 0; i < item_count; i++) {
		lb->type[i] = lb->fn_type(i, lb->cb_data);
		lb->ch[i] = ch;
		ch += lb->fn_height(i, lb->cb_data);
	}
	lb->ch[item_count] = ch;

	/* The pad lines: 0,.., ch - 1. */
	wins_scrollwin_set_pad(&(lb->sw), ch);

	/* Adjust the selection. */
	listbox_fix_sel(lb, 1);
	listbox_sel_in_view(lb);
}

void listbox_draw_deco(struct listbox *lb, int hilt)
{
	wins_scrollwin_draw_deco(&(lb->sw), hilt);
}

void listbox_display(struct listbox *lb, int hilt)
{
	int i;

	werase(lb->sw.inner);

	for (i = 0; i < lb->item_count; i++) {
		int is_sel = (i == lb->item_sel);
		lb->fn_draw(i, lb->sw.inner, lb->ch[i], is_sel, lb->cb_data);
	}

	wins_scrollwin_display(&(lb->sw), hilt);
}

int listbox_get_sel(struct listbox *lb)
{
	return lb->item_sel;
}

/* Avoid non-text items. */
static void listbox_fix_sel(struct listbox *lb, int direction)
{
	int did_flip = 0;

	if (lb->item_count == 0 || direction == 0)
		return;

	direction = direction > 0 ? 1 : -1;

	if (lb->item_sel < 0)
		lb->item_sel = 0;
	else if (lb->item_sel >= lb->item_count)
		lb->item_sel = lb->item_count - 1;

	while (lb->type[lb->item_sel] != LISTBOX_ROW_TEXT) {
		if ((direction == -1 && lb->item_sel == 0) ||
		    (direction == 1 && lb->item_sel == lb->item_count - 1)) {
			if (did_flip) {
				lb->item_sel = -1;
				return;
			}
			direction = -direction;
			did_flip = 1;
		} else {
			lb->item_sel += direction;
		}
	}
}

void listbox_set_sel(struct listbox *lb, unsigned pos)
{
	lb->item_sel = pos;
	listbox_fix_sel(lb, 1);
	if (lb->item_sel < 0)
		return;

	listbox_sel_in_view(lb);
}

void listbox_item_in_view(struct listbox *lb, int item)
{
	int first_line = lb->ch[item];
	int last_line = lb->ch[item + 1] - 1;

	wins_scrollwin_in_view(&(lb->sw), first_line);
	if (last_line != first_line)
		wins_scrollwin_in_view(&(lb->sw), last_line);
}

/* Keep the selection in view. */
static void listbox_sel_in_view(struct listbox *lb)
{
	listbox_item_in_view(lb, lb->item_sel);
}

/* Returns true if the move succeeded. */
int listbox_sel_move(struct listbox *lb, int delta)
{

	int saved_sel = lb->item_sel;

	if (lb->item_count == 0)
		return 0;

	lb->item_sel += delta;
	listbox_fix_sel(lb, delta);
	if (lb->item_sel < 0)
		return 0;

	listbox_sel_in_view(lb);

	return lb->item_sel != saved_sel;
}
