/*
 * This code has been modified by Analog Devices, Inc.
 * ANSI Mode Control processing:
 *   https://vt100.net/docs/vt100-ug/chapter3.html
 */

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "shell_platform.h"
#include "term.h"

#define ANSI_CTRL_PARAM_SIZE      16
#define ANSI_CTRL_MAX_PARAMS      4

static int term_translate( term_state_t *t, int data, void *usr )
{
  static int escape = 0;

  static char param[ANSI_CTRL_MAX_PARAMS][ANSI_CTRL_PARAM_SIZE];
  static int param_num;
  static int param_idx;

  int i;

  if (escape)
  {
   switch (escape)
   {
      case 1:
         if (data == '[')
            escape = 2;
         else if (data == 0x1B) {
            escape = 0;
            return(KC_ESC);
         }
         else
            escape = 0;
         break;
      case 2:
         if( data >= 'A' && data <= 'D' )
         {
            escape = 0;
            switch( data )
            {
               case 'A':
                  return KC_UP;
               case 'B':
                  return KC_DOWN;
               case 'C':
                  return KC_RIGHT;
               case 'D':
                  return KC_LEFT;
            }
         }
         else if( data >= '0' && data <= '9' )
         {
            memset(param, 0, sizeof(param));
            param_num = 0; param_idx = 0;
            param[param_num][param_idx++] = (char)data;
            escape = 3;
         }
         break;
      case 3:
         if( data >= '0' && data <= '9' )
         {
            if( param_idx < (ANSI_CTRL_PARAM_SIZE - 1))
            {
               param[param_num][param_idx++] = (char)data;
            }
         } else {
            switch( data )
            {
               case '~':
                  escape = 0;
                  for ( i = 0; i <= param_num; i++ )
                  {
                     data = atoi(param[i]);
                     switch( data )
                     {
                        case '1':
                           return KC_HOME;
                        case '4':
                           return KC_END;
                        case '5':
                           return KC_PAGEUP;
                        case '6':
                           return KC_PAGEDOWN;
                        default:
                           break;
                     }
                  }
                  break;
               case ';':
                  if( param_num < ANSI_CTRL_MAX_PARAMS )
                  {
                     param_num++; param_idx = 0;
                  }
                  break;
               case 'R':
                  escape = 0;
                  if( param_num >= 1 )
                  {
                     t->term_num_lines = atoi(param[0]);
                     t->term_num_cols = atoi(param[1]);
                  }
                  break;
            }
         }
         break;
      default:
         break;
   }
   return KC_UNKNOWN;
  }
  else if( isprint( data ) )
    return data;
  else if( data == 0x1B ) // escape sequence
  {
     escape = 1;
  }
  else if( data == 0x0D )
  {
    return KC_ENTER;
  }
  else
  {
    switch( data )
    {
      case 0x09:
        return KC_TAB;
      case 0x7F:
        //return KC_DEL;
        return KC_BACKSPACE;
      case 0x08:
        return KC_BACKSPACE;
      case 26:
        return KC_CTRL_Z;
      case 1:
        return KC_CTRL_A;
      case 5:
        return KC_CTRL_E;
      case 3:
        return KC_CTRL_C;
      case 20:
        return KC_CTRL_T;
      case 21:
        return KC_CTRL_U;
      case 11:
        return KC_CTRL_K;
    }
  }
  return KC_UNKNOWN;
}

void shell_platform_init(shell_context_t *ctx, p_term_out term_out, p_term_in term_in)
{
    term_init(&ctx->t, SHELL_LINES, SHELL_COLUMNS, term_out, term_in, term_translate, ctx->usr);
}

void shell_platform_deinit(shell_context_t *ctx)
{
    term_deinit(&ctx->t);
}
