/******************************************************************
 * Copyright (C) 2003 by SG Software.
 ******************************************************************/

/* FILE NAME   : eqwnd.c
 * PURPOSE     : SG MPFC. Equalizer window functions implementation.
 * PROGRAMMER  : Sergey Galanov
 * LAST UPDATE : 11.08.2003
 * NOTE        : Module prefix 'eqwnd'.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#include <stdlib.h>
#include "types.h"
#include "cfg.h"
#include "error.h"
#include "eqwnd.h"
#include "file_input.h"
#include "help_screen.h"
#include "player.h"
#include "window.h"
#include "util.h"

/* Create a new equalizer window */
eq_wnd_t *eqwnd_new( wnd_t *parent, int x, int y, int w, int h )
{
	eq_wnd_t *eq;

	/* Allocate memory */
	eq = (eq_wnd_t *)malloc(sizeof(eq_wnd_t));
	if (eq == NULL)
	{
		error_set_code(ERROR_NO_MEMORY);
		return NULL;
	}

	/* Initialize equalizer window */
	if (!eqwnd_init(eq, parent, x, y, w, h))
	{
		free(eq);
		return NULL;
	}
	return eq;
} /* End of 'eqwnd_new' function */

/* Initialize equalizer window */
bool_t eqwnd_init( eq_wnd_t *eq, wnd_t *parent, int x, int y, int w, int h )
{
	wnd_t *wnd = (wnd_t *)eq;

	/* Initialize window part */
	if (!wnd_init(wnd, parent, x, y, w, h))
		return FALSE;

	/* Register handlers */
	wnd_register_handler(wnd, WND_MSG_DISPLAY, eqwnd_display);
	wnd_register_handler(wnd, WND_MSG_KEYDOWN, eqwnd_handle_key);
	wnd_register_handler(wnd, WND_MSG_MOUSE_LEFT_CLICK, eqwnd_handle_mouse);

	/* Set fields */
	eq->m_pos = 0;
	WND_OBJ(eq)->m_flags |= (WND_INITIALIZED);
	return TRUE;
} /* End of 'eqwnd_init' function */

/* Destroy equalizer window */
void eqwnd_free( wnd_t *wnd )
{
	wnd_destroy(wnd);
} /* End of 'eqwnd_free' function */

/* Handle display message */
void eqwnd_display( wnd_t *wnd, dword data )
{
	char *title = _("MPFC Equalizer");
	eq_wnd_t *eq = (eq_wnd_t *)wnd;
	int i;
	int x;
	char *str[EQWND_NUM_BANDS] = {"PREAMP", "60HZ", "170HZ", "310HZ", "600HZ",
						"1KHZ", "3KHZ", "6KHZ", "12KHZ", "14KHZ", "16KHZ"};
	
	/* Print title */
	wnd_clear(wnd, FALSE);
	wnd_move(wnd, (wnd->m_width - strlen(title)) / 2, 0);
	wnd_set_attrib(wnd, A_BOLD);
	wnd_printf(wnd, "%s\n\n", title);
	wnd_set_attrib(wnd, A_NORMAL);

	/* Display bands sliders */
	for ( i = 0, x = 3; i < EQWND_NUM_BANDS; i ++ )
	{
		char name[20];
		float val;

		eqwnd_get_var_name(i, name);
		val = cfg_get_var_float(cfg_list, name);
		x += eqwnd_display_slider(wnd, x, 2, 22,
				(i == eq->m_pos), val, str[i]);
		if (i == 0)
		{
			wnd_move(wnd, x, 2);
			wnd_printf(wnd, "+20 dB");
			wnd_move(wnd, x, 12);
			wnd_printf(wnd, "0 dB");
			wnd_move(wnd, x, 22);
			wnd_printf(wnd, "-20 dB");
			x += 10;
		}
	}

	/* Remove cursor */
	wnd_move(wnd, wnd->m_width - 1, wnd->m_height - 1);
} /* End of 'eqwnd_display' function */

/* Handle key message */
void eqwnd_handle_key( wnd_t *wnd, dword data )
{
	eq_wnd_t *eq = (eq_wnd_t *)wnd;
	int key = (int)data;

	switch (key)
	{
	case 'q':
	case 27:
		/* Save parameters */
		eqwnd_save_params();

		/* Close window */
		wnd_send_msg(wnd, WND_MSG_CLOSE, 0);
		break;
	case 'h':
	case KEY_LEFT:
		eq->m_pos --;
		if (eq->m_pos < 0)
			eq->m_pos = 10;
		break;
	case 'l':
	case KEY_RIGHT:
		eq->m_pos ++;
		if (eq->m_pos > 10)
			eq->m_pos = 0;
		break;
	case 'j':
	case KEY_DOWN:
		eqwnd_set_var(eq->m_pos, -2., TRUE);
		player_eq_changed = TRUE;
		break;
	case 'k':
	case KEY_UP:
		eqwnd_set_var(eq->m_pos, 2., TRUE);
		player_eq_changed = TRUE;
		break;
	case 'p':
		eqwnd_load_eqf_dlg();
		break;
	case '?':
		eqwnd_help(eq);
		break;
	}
} /* End of 'eqwnd_handle_key' function */

/* Display slider */
int eqwnd_display_slider( wnd_t *wnd, int x, int start_y, int end_y,
							bool_t hl, float val, char *str )
{
	int pos, i;
	int h;
	
	h = end_y - start_y;
	pos = eqwnd_val2pos(val, h);
	for ( i = 0; i <= h; i ++ )
	{
		wnd_move(wnd, x, i + start_y);
		if (i == pos)
		{
			wnd_print_char(wnd, ACS_BLOCK);
			wnd_print_char(wnd, ACS_BLOCK);
		}
		else
		{
			wnd_print_char(wnd, ACS_VLINE);
			wnd_print_char(wnd, ACS_VLINE);
		}
	}
	wnd_move(wnd, x - 1, end_y + 1);
	if (hl)
		wnd_set_attrib(wnd, A_BOLD);
	wnd_printf(wnd, "%s", str);
	wnd_set_attrib(wnd, A_NORMAL);
	return 6;
} /* End of 'eqwnd_display_slider' function */

/* Set equalizer variable value */
void eqwnd_set_var( int pos, float val, bool_t rel )
{
	char str[20];
	float cur_val;

	/* Get variable name using slider position */
	eqwnd_get_var_name(pos, str);

	/* Update value */
	if (rel)
	{
		cur_val = cfg_get_var_float(cfg_list, str);
		cur_val += val;
	}
	else
		cur_val = val;
	if (cur_val < -20.)
		cur_val = -20.;
	else if (cur_val > 20.)
		cur_val = 20.;
	cfg_set_var_float(cfg_list, str, cur_val);
} /* End of 'eqwnd_set_var' function */

/* Get equalizer variable name */
void eqwnd_get_var_name( int pos, char *name )
{
	if (pos == 0)
		strcpy(name, "eq-preamp");
	else
		sprintf(name, "eq-band%i", pos);
} /* End of 'eqwnd_get_var_name' function */

/* Save equalizer parameters */
void eqwnd_save_params( void )
{
	char *str = "eq-preamp;eq-band1;eq-band2;eq-band3;eq-band4;eq-band5;"
				"eq-band6;eq-band7;eq-band8;eq-band9;eq-band10";
	player_save_cfg_vars(cfg_list, str);
} /* End of 'eqwnd_save_params' function */

/* Process load preset from EQF file dialog */
void eqwnd_load_eqf_dlg( void )
{
	file_input_box_t *fin;

	/* Create edit box for path input */
	fin = fin_new(wnd_root, 0, wnd_root->m_height - 1, 
			wnd_root->m_width, _("Load preset from a Winamp EQF file: "));
	if (fin != NULL)
	{
		/* Run message loop */
		wnd_run(fin);

		/* Add file if enter was pressed */
		if (fin->m_box.m_last_key == '\n')
			eqwnd_load_eqf(EBOX_TEXT(fin));

		/* Destroy edit box */
		wnd_destroy(fin);
	}
} /* End of 'eqwnd_load_eqf_dlg' function */

/* Load a Winamp EQF file */
void eqwnd_load_eqf( char *filename )
{
	FILE *fd;
	char header[31];
	byte bands[EQWND_NUM_BANDS];
	int i;

	/* Open file */
	fd = util_fopen(filename, "rb");
	if (fd == NULL)
		return;

	/* Read data */
	fread(header, 1, 31, fd);
	if (!strncmp(header, "Winamp EQ library file v1.1", 27))
	{
		if (fseek(fd, 257, SEEK_CUR) == -1)	/* Skip name */
		{
			fclose(fd);
			return;
		}
		if (fread(bands, 1, EQWND_NUM_BANDS, fd) != EQWND_NUM_BANDS)
		{
			fclose(fd);
			return;
		}

		cfg_set_var_float(cfg_list, "eq-preamp", 
				20.0 - ((bands[10] * 40.0) / 63.0));
		for ( i = 0; i < 10; i ++ )
		{
			char str[20];
			sprintf(str, "eq-band%i", i + 1);
			cfg_set_var_float(cfg_list, str, 
					20.0 - ((bands[i] * 40.0) / 64.0));
		}
	}

	/* Close file */
	fclose(fd);

	/* Report about changing */
	player_eq_changed = TRUE;
} /* End of 'eqwnd_load_eqf' function */

/* Handle mouse left button click */
void eqwnd_handle_mouse( wnd_t *wnd, dword data )
{
	int mx = WND_MOUSE_X(data), my = WND_MOUSE_Y(data), i;
	eq_wnd_t *eq = (eq_wnd_t *)wnd;

	/* Check if this point is inside any of the band sliders */
	for ( i = 0; i < EQWND_NUM_BANDS; i ++ )
	{
		int x, y, w, h;

		eqwnd_get_slider_pos(i, &x, &y, &w, &h);
		if (mx >= x && my >= y && mx <= x + w && my <= y + h)
		{
			eq->m_pos = i;
			eqwnd_set_var(i, eqwnd_pos2val(my - y, h), FALSE);
			break;
		}
	}
} /* End of 'eqwnd_handle_mouse' function */

/* Get equalizer band slider position */
void eqwnd_get_slider_pos( int band, int *x, int *y, int *w, int *h )
{
	*x = 2 + band * 6;
	if (band > 0)
		(*x) += 10;
	*y = 2;
	*w = 6;
	*h = 20;
} /* End of 'eqwnd_get_slider_pos' function */

/* Convert band value to position */
int eqwnd_val2pos( float val, int height )
{
	return ((-val) + 20.) * height / 40.;
} /* End of 'eqwnd_val2pos' function */

/* Convert position to band value */
float eqwnd_pos2val( int pos, int height )
{
	return 20. - (pos * 40. / height);
} /* End of 'eqwnd_pos2val' function */

/* Show help screen */
void eqwnd_help( eq_wnd_t *eq )
{
	help_screen_t *h;

	h = help_new(WND_OBJ(eq), HELP_EQWND, 0, 0, WND_WIDTH(eq), WND_HEIGHT(eq));
	wnd_run(h);
	wnd_destroy(h);
} /* End of 'eqwnd_help' function */

/* End of 'eqwnd.c' file */

