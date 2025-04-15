/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Switch to gets() if $TERM is something we can't support.
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - Completion?
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * CHA (Cursor Horizontal Absolute)
 *    Sequence: ESC [ n G
 *    Effect: moves cursor to column n
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward of n chars
 *
 * [eLua] code adapted to eLua by bogdanm
 *
 */

/*
 * This code has been modified by Analog Devices, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "shell.h"
#include "shell_string.h"
#include "term.h"
#include "linenoise.h"

#define LINENOISE_NON_BLOCK_RETURN          ( -3 )
#define LINENOISE_CTRL_C                    ( -2 )
#define LINENOISE_CTRL_Z                    ( -1 )
#define LINENOISE_DONT_PUSH_EMPTY           0
#define LINENOISE_PUSH_EMPTY                1

/* Make sure a default context always exists */
shell_context_t *defaultCxt = NULL;

static int linenoise_internal_addhistory( shell_context_t *ctx, const char *line, int force_empty );

void linenoise_cleanup( shell_context_t *ctx )
{
  int j;

  if( ctx->histories )
  {
    for( j = 0; j < ctx->num_histories; j++ )
      SHELL_FREE( ctx->histories[ j ] );
    SHELL_FREE( ctx->histories );
    ctx->histories = NULL;
  }

  ctx->num_histories = 0;
}

#define MAX_SEQ_LEN           32
static void refreshLine(shell_context_t *ctx, const char *prompt, char *buf, size_t len, size_t pos, size_t cols) {
    char seq[MAX_SEQ_LEN];
    size_t plen = strlen(prompt);

    while((plen+pos) >= cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen+len > cols) {
        len--;
    }

    /* Hide the text if required */
    if (ctx->hidden) {
       buf = SHELL_MALLOC(len + 1);
       memset(buf, '*', len);
       buf[len] = '\0';
    }

    /* Cursor to left edge */
    snprintf(seq,MAX_SEQ_LEN,"\r");
    term_putstr( &ctx->t, seq, strlen( seq ) );
    /* Write the prompt and the current buffer content */
    term_putstr( &ctx->t, prompt, strlen( prompt ) );
    term_putstr( &ctx->t, buf, len );
    /* Erase to right */
    snprintf(seq,MAX_SEQ_LEN,"\x1b[0K");
    term_putstr( &ctx->t, seq, strlen( seq ) );
    /* Move cursor to original position. */
    snprintf(seq,MAX_SEQ_LEN,"\r\x1b[%dC", (int)(pos+plen));
    term_putstr( &ctx->t, seq, strlen( seq ) );

    if (ctx->hidden) {
       SHELL_FREE(buf);
    }
}

static int linenoisePrompt(shell_context_t *ctx, char *buf, size_t buflen, const char *prompt) {

    /* Blocking mode always starts a new line */
    if (ctx->blocking == SHELL_MODE_BLOCKING) {
        ctx->new_line = 1;
    }

    /* Start a new line if necessary */
    if (ctx->new_line) {

        ctx->plen = strlen(prompt);
        ctx->pos = 0;
        ctx->len = 0;
        ctx->cols = SHELL_COLUMNS;
        ctx->history_index = 0;

        buf[0] = '\0';
        buflen--; /* Make sure there is always space for the nulterm */

        /* The latest history entry is always our current buffer, that
         * initially is just an empty string. */
        linenoise_internal_addhistory( ctx, "", LINENOISE_PUSH_EMPTY );

        term_putstr( &ctx->t, prompt, ctx->plen );

        ctx->new_line = false;
    }

    while(1) {

        int c;

        if (ctx->blocking == SHELL_MODE_BLOCKING) {
            c = term_getch( &ctx->t, TERM_INPUT_WAIT );
        } else {
            c = term_getch( &ctx->t, TERM_INPUT_DONT_WAIT );
            if (c == -1) {
                return(LINENOISE_NON_BLOCK_RETURN);
            }
        }

        switch(c)
        {
          case KC_ENTER:
          case KC_CTRL_C:
          case KC_CTRL_Z:
            if (ctx->num_histories > 0)
            {
              ctx->num_histories--;
              SHELL_FREE( ctx->histories[ctx->num_histories] );
            }
            ctx->new_line = true;
            if( c == KC_CTRL_C )
              return LINENOISE_CTRL_C;
            else if( c == KC_CTRL_Z )
              return LINENOISE_CTRL_Z;
            return ctx->len;

         case KC_BACKSPACE:
            if (ctx->pos > 0 && ctx->len > 0)
            {
              memmove(buf+ctx->pos-1,buf+ctx->pos,ctx->len-ctx->pos);
              ctx->pos--;
              ctx->len--;
              buf[ctx->len] = '\0';
              refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

         case KC_CTRL_T:
            /* ctrl-t */
            if (ctx->len > 1 && ctx->pos > 0 && ctx->pos <= ctx->len)
            {
                if (ctx->pos == ctx->len)
                {
                    ctx->pos--;
                }
                int aux = buf[ctx->pos-1];
                buf[ctx->pos-1] = buf[ctx->pos];
                buf[ctx->pos] = aux;
                ctx->pos++;
                refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

        case KC_LEFT:
            /* left arrow */
            if (ctx->pos > 0)
            {
              ctx->pos--;
              refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

        case KC_RIGHT:
            /* right arrow */
            if (ctx->pos != ctx->len)
            {
              ctx->pos++;
              refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

       case KC_UP:
       case KC_DOWN:
            /* up and down arrow: history */
            if (ctx->num_histories > 1)
            {
              /* Update the current history entry before to
               * overwrite it with tne next one. */
              SHELL_FREE(ctx->histories[ctx->num_histories-1-ctx->history_index]);
              ctx->histories[ctx->num_histories-1-ctx->history_index] = SHELL_STRDUP(buf);
              /* Show the new entry */
              ctx->history_index += (c == KC_UP) ? 1 : -1;
              if (ctx->history_index < 0)
              {
                ctx->history_index = 0;
                break;
              } else if (ctx->history_index >= ctx->num_histories)
              {
                ctx->history_index = ctx->num_histories-1;
                break;
              }
              strncpy(buf,ctx->histories[ctx->num_histories-1-ctx->history_index],buflen);
              buf[buflen] = '\0';
              ctx->len = ctx->pos = strlen(buf);
              refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

        case KC_DEL:
            /* delete */
            if (ctx->len > 0 && ctx->pos < ctx->len)
            {
              memmove(buf+ctx->pos,buf+ctx->pos+1,ctx->len-ctx->pos-1);
              ctx->len--;
              buf[ctx->len] = '\0';
              refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            }
            break;

        case KC_HOME: /* Ctrl+a, go to the start of the line */
            ctx->pos = 0;
            refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            break;

        case KC_END: /* ctrl+e, go to the end of the line */
            ctx->pos = ctx->len;
            refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            break;

        case KC_CTRL_U: /* Ctrl+u, delete the whole line. */
            buf[0] = '\0';
            ctx->pos = ctx->len = 0;
            refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            break;

        case KC_CTRL_K: /* Ctrl+k, delete from current to end of line. */
            buf[ctx->pos] = '\0';
            ctx->len = ctx->pos;
            refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
            break;

        case KC_UNKNOWN:
            break;

        default:
            if( isprint( c ) && ctx->len < buflen )
            {
              if(ctx->len == ctx->pos)
              {
                buf[ctx->pos] = c;
                ctx->pos++;
                ctx->len++;
                buf[ctx->len] = '\0';
                if (ctx->plen+ctx->len < ctx->cols)
                {
                  /* Avoid a full update of the line in the
                   * trivial case. */
                  if (ctx->hidden) {
                    term_putch( &ctx->t, '*' );
                  } else {
                    term_putch( &ctx->t, c );
                  }
                }
                else
                {
                  refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
                }
              }
              else
              {
                memmove(buf+ctx->pos+1,buf+ctx->pos,ctx->len-ctx->pos);
                buf[ctx->pos] = c;
                ctx->len++;
                ctx->pos++;
                buf[ctx->len] = '\0';
                refreshLine(ctx, prompt,buf,ctx->len,ctx->pos,ctx->cols);
              }
            }
            break;
        }
    }

    //return ctx->len;
}

int linenoise_getline( shell_context_t *ctx, char* buffer, int maxinput, const char* prompt )
{
  int count;

  if (ctx == NULL) {
      ctx = defaultCxt;
  }

  while( 1 )
  {
    count = linenoisePrompt( ctx, buffer, maxinput, prompt );
    if (count == LINENOISE_NON_BLOCK_RETURN) {
        return(LINENOISE_CONTINUE);
    }
    if( count == LINENOISE_CTRL_Z )
    {
        return LINENOISE_EOF;
    }
    else if (count == LINENOISE_CTRL_C)
    {
        term_putch(&ctx->t, '\n');
    }
    else
    {
      term_putch(&ctx->t, '\n');
      if( count > 0 && buffer[ count ] != '\0' )
        buffer[ count ] = '\0';
      return count;
    }
  }
}

static int linenoise_internal_addhistory( shell_context_t *ctx, const char *line, int force_empty )
{
  char *linecopy;
  const char *p;

  if( ctx->max_histories == 0 )
    return 0;

  if( ctx->histories == NULL )
  {
    if( ( ctx->histories = SHELL_MALLOC( sizeof( char* ) * ctx->max_histories ) ) == NULL )
    {
      fprintf( stderr, "out of memory in linenoise while trying to allocate history buffer.\n" );
      return 0;
    }
    memset( ctx->histories, 0, ( sizeof( char* ) * ctx->max_histories ) );
  }

  while( 1 )
  {
    if( ( p = strchr( line, '\n' ) ) == NULL )
      p = line + strlen( line );
    if( p > line || force_empty == LINENOISE_PUSH_EMPTY )
    {
      if( ( linecopy = SHELL_STRNDUP( line, p - line ) ) == NULL )
      {
        fprintf( stderr, "out of memory in linenoise while trying to add a line to history.\n" );
        return 0;
      }
      if( ctx->num_histories == ctx->max_histories )
      {
        SHELL_FREE( ctx->histories[ 0 ] );
        memmove( ctx->histories, ctx->histories + 1, sizeof( char* ) * ( ctx->max_histories - 1 ) );
        ctx->num_histories--;
      }
      ctx->histories[ctx->num_histories] = linecopy;
      ctx->num_histories++;
    }
    if( *p == 0 )
      break;
    line = p + 1;
    if( *line == 0 )
      break;
  }

  return 1;
}

int linenoise_addhistory( shell_context_t *ctx, const char *line )
{
  if (ctx == NULL) {
      ctx = defaultCxt;
  }

  return linenoise_internal_addhistory( ctx, line, LINENOISE_DONT_PUSH_EMPTY );
}

int linenoise_init(shell_context_t *ctx)
{
    /* Initialize the default context */
    if (defaultCxt == NULL) {
        defaultCxt = SHELL_MALLOC(sizeof(*defaultCxt));
        memset(defaultCxt, 0, sizeof(*defaultCxt));
        defaultCxt->max_histories = SHELL_MAX_HISTORIES;
        defaultCxt->new_line = 1;
        if (ctx) {
            defaultCxt->blocking = ctx->blocking;
        }
    }

    /* Initialize the given context */
    if (ctx) {
        ctx->max_histories = SHELL_MAX_HISTORIES;
        ctx->new_line = 1;
    }

    return 1;
}
