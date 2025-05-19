// Terminal functions

#ifndef __TERM_H__
#define __TERM_H__

// ****************************************************************************
// Data types

typedef struct term_state_t term_state_t; // Break recursive definition

// Terminal output function
typedef void ( *p_term_out )( char, void * );
// Terminal input function
typedef int ( *p_term_in )( int, void * );
// Terminal translate input function
typedef int ( *p_term_translate )( term_state_t *, int, void * );

// Terminal input mode (parameter of p_term_in and term_getch())
#define TERM_INPUT_DONT_WAIT      0
#define TERM_INPUT_WAIT           1

// Terminal mode flags
#define TERM_MODE_NONE           0
#define TERM_MODE_COOKED         1

struct term_state_t {
    p_term_out term_out;             /**< Terminal output function pointer */
    p_term_in term_in;               /**< Terminal input function pointer */
    p_term_translate term_translate; /**< Terminal translate function pointer */
    unsigned term_num_lines;         /**< Terminal lines */
    unsigned term_num_cols;          /**< Terminal column */
    unsigned term_cx;                /**< Terminal current x */
    unsigned term_cy;                /**< Terminal current y */
    unsigned term_mode;              /**< Terminal character mode */
    void *usr;                       /**< User defined pointer */
};

// ****************************************************************************
// Exported functions

// Terminal initialization
void term_init( term_state_t *t, unsigned lines, unsigned cols, p_term_out term_out_func,
                p_term_in term_in_func, p_term_translate term_translate_func,
                void *usr );
void term_deinit( term_state_t *t );

// Terminal output functions
void term_clrscr(term_state_t *t);
void term_clreol(term_state_t *t);
void term_gotoxy( term_state_t *t, unsigned x, unsigned y );
void term_up( term_state_t *t, unsigned delta );
void term_down( term_state_t *t, unsigned delta );
void term_left( term_state_t *t, unsigned delta );
void term_right( term_state_t *t, unsigned delta );
unsigned term_get_lines(term_state_t *t);
unsigned term_get_cols(term_state_t *t);
void term_putch( term_state_t *t, char ch );
void term_putstr( term_state_t *t, const char* str, unsigned size );
unsigned term_get_cx(term_state_t *t);
unsigned term_get_cy(term_state_t *t);
int term_getch( term_state_t *t, int mode );
void term_reset_mode( term_state_t *t );
void term_sync_size( term_state_t *t );
void term_set_size( term_state_t *t, unsigned num_cols, unsigned num_lines );
void term_set_mode( term_state_t *t, int mode, int set );

#define TERM_KEYCODES\
  _D( KC_UP ),\
  _D( KC_DOWN ),\
  _D( KC_LEFT ),\
  _D( KC_RIGHT ),\
  _D( KC_HOME ),\
  _D( KC_END ),\
  _D( KC_PAGEUP ),\
  _D( KC_PAGEDOWN ),\
  _D( KC_ENTER ),\
  _D( KC_TAB ),\
  _D( KC_BACKSPACE ),\
  _D( KC_ESC ),\
  _D( KC_CTRL_Z ),\
  _D( KC_CTRL_A ),\
  _D( KC_CTRL_E ),\
  _D( KC_CTRL_C ),\
  _D( KC_CTRL_T ),\
  _D( KC_CTRL_U ),\
  _D( KC_CTRL_K ),\
  _D( KC_DEL ),\
  _D( KC_UNKNOWN )

// Terminal input functions
// Keyboard codes
#define _D( x ) x

enum
{
  term_dummy = 255,
  TERM_KEYCODES,
  TERM_FIRST_KEY = KC_UP,
  TERM_LAST_KEY = KC_UNKNOWN
};

#endif // #ifndef __TERM_H__
