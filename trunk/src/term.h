/*
	term: terminal control

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "config.h"


#ifndef _MPG123_TERM_H_
#define _MPG123_TERM_H_

#ifdef HAVE_TERMIOS

#define LOOP_CYCLES	0.500000	/* Loop time in sec */

/* 
 * Defines the keybindings in term.c - change to your 
 * own preferences.
 */

#define HELP_KEY	'h'
#define BACK_KEY	'b'
#define NEXT_KEY	'f'
#define PAUSE_KEY	'p'
#define QUIT_KEY	'q'
/* space bar is alias for that */
#define STOP_KEY	's'
#define REWIND_KEY	','
#define FORWARD_KEY	'.'
/* This is convenient on QWERTZ-keyboards. */
#define FAST_REWIND_KEY ';'
#define FAST_FORWARD_KEY ':'
#define FINE_REWIND_KEY '<'
#define FINE_FORWARD_KEY '>'
/* You probably want to use the following bindings instead
 * on a standard QWERTY-keyboard:
 */
 
/* #define FAST_REWIND_KEY '<' */
/* #define FAST_FORWARD_KEY '>' */
/* #define FINE_REWIND_KEY ';' */
/* #define FINE_FORWARD_KEY ':' */

#define VOL_UP_KEY '+'
#define VOL_DOWN_KEY '-'
#define VERBOSE_KEY 'v'
#define RVA_KEY 'r'
#define PLAYLIST_KEY 'l'
#define PREV_KEY 'd'
/* this counts as "undocumented" and can disappear */
#define FRAME_INDEX_KEY 'i'

#define PAUSED_STRING	"Paused. \b\b\b\b\b\b\b\b"
#define STOPPED_STRING	"Stopped.\b\b\b\b\b\b\b\b"
#define EMPTY_STRING	"        \b\b\b\b\b\b\b\b"

void term_init(void);
off_t term_control(mpg123_handle *mh);
void term_restore(void);

#endif

#endif
