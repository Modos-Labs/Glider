/*
 * This code has been modified by Analog Devices, Inc.
 */

// Terminal function
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "shell.h"
#include "shell_platform.h"
#include "term.h"

// Maximum size on an ANSI sequence
#define TERM_MAX_ANSI_SIZE        14

// *****************************************************************************
// Terminal functions

static void term_ansi( term_state_t *t, const char* fmt, ... )
{
  char seq[ TERM_MAX_ANSI_SIZE + 1 ];
  va_list ap;

  seq[ TERM_MAX_ANSI_SIZE ] = '\0';
  seq[ 0 ] = '\x1B';
  seq[ 1 ] = '[';
  va_start( ap, fmt );
  vsnprintf( seq + 2, TERM_MAX_ANSI_SIZE - 2, fmt, ap );
  va_end( ap );
  term_putstr( t, seq, strlen( seq ) );
}

// Clear the screen
void term_clrscr(term_state_t *t)
{
  term_ansi( t, "2J" );
  t->term_cx = t->term_cy = 0;
}

// Clear to end of line
void term_clreol(term_state_t *t)
{
  term_ansi( t, "K" );
}

// Move cursor to (x, y)
void term_gotoxy( term_state_t *t, unsigned x, unsigned y )
{
  term_ansi( t, "%u;%uH", y, x );
  t->term_cx = x;
  t->term_cy = y;
}

// Move cursor up "delta" lines
void term_up( term_state_t *t, unsigned delta )
{
  term_ansi( t, "%uA", delta );
  t->term_cy -= delta;
}

// Move cursor down "delta" lines
void term_down( term_state_t *t, unsigned delta )
{
  term_ansi( t, "%uB", delta );
  t->term_cy += delta;
}

// Move cursor right "delta" chars
void term_right( term_state_t *t, unsigned delta )
{
  term_ansi( t, "%uC", delta );
  t->term_cx -= delta;
}

// Move cursor left "delta" chars
void term_left( term_state_t *t, unsigned delta )
{
  term_ansi( t, "%uD", delta );
  t->term_cx += delta;
}

// Reset the character mode
void term_reset_mode( term_state_t *t )
{
  term_ansi( t, "0m" );
}

// Request current size from the terminal
void term_sync_size( term_state_t *t )
{
  term_putstr( t, "\x1B" "7", 2 ); // Store cursor position
  term_ansi( t, "999;999H" );      // Move far away
  term_ansi( t, "6n" );            // Request position
  term_putstr( t, "\x1B" "8", 2 ); // Restore cursor position
}

// Set the terminal size
void term_set_size( term_state_t *t, unsigned num_cols, unsigned num_lines )
{
  t->term_num_lines = num_lines;
  t->term_num_cols = num_cols;
  term_ansi(t, "8;%u;%ut", num_lines, num_cols);
}

void term_set_mode( term_state_t *t, int mode, int set )
{
    if ( set ) {
        t->term_mode |= mode;
    } else {
        t->term_mode &= ~mode;
    }
}

// Return the number of terminal lines
unsigned term_get_lines(term_state_t *t)
{
  return t->term_num_lines;
}

// Return the number of terminal columns
unsigned term_get_cols(term_state_t *t)
{
  return t->term_num_cols;
}

// Write a character to the terminal
void term_putch( term_state_t *t, char ch )
{
  if( ( ch == '\n' ) && ( t->term_mode & TERM_MODE_COOKED ) )
  {
    if( t->term_cy < t->term_num_lines )
      t->term_cy ++;
    t->term_cx = 0;
    t->term_out( '\r', t->usr );
  }
  t->term_out( ch, t->usr );
}

// Write a string to the terminal
void term_putstr( term_state_t *t, const char* str, unsigned size )
{
  while( size )
  {
    term_putch( t, *str ++ );
    size --;
  }
}

// Write a string of

// Return the cursor "x" position
unsigned term_get_cx(term_state_t *t)
{
  return t->term_cx;
}

// Return the cursor "y" position
unsigned term_get_cy(term_state_t *t)
{
  return t->term_cy;
}

// Return a char read from the terminal
// If "mode" is TERM_INPUT_DONT_WAIT, return the char only if it is available,
// otherwise return -1
// Calls the translate function to translate the terminal's physical key codes
// to logical key codes (defined in the term.h header)
int term_getch( term_state_t *t, int mode )
{
  int ch;

  ch = t->term_in( mode, t->usr );

  if( ( ch != -1 ) && ( t->term_mode & TERM_MODE_COOKED ) )
    return t->term_translate( t, ch, t->usr );
  else
    return ch;
}

void term_init( term_state_t *t, unsigned lines, unsigned cols, p_term_out term_out_func,
                p_term_in term_in_func, p_term_translate term_translate_func,
                void *usr )
{
  t->term_num_lines = lines;
  t->term_num_cols = cols;
  t->term_out = term_out_func;
  t->term_in = term_in_func;
  t->term_translate = term_translate_func;
  t->usr = usr;
  t->term_cx = t->term_cy = 0;
  t->term_mode = TERM_MODE_COOKED;
}

void term_deinit( term_state_t *t )
{
}
