//
// Grimoire
// Copyright 2025 Wenting Zhang
//
// Original copyright information:
// Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
//
// This file is licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.  You may
// obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "shell.h"
#include "shell_string.h"
#include "shell_platform.h"
#include "shell_printf.h"
#include "linenoise.h"
#include "term.h"
#include "version.h"

#ifdef printf
#undef printf
#endif

#define printf(...) shell_printf(ctx, __VA_ARGS__)

// Shell alternate ' ' char
#define SHELL_ALT_SPACE    '\x07'

// Helper macros
#define SHELL_FUNC( func )      extern void func( shell_context_t *ctx, int argc, char **argv )
#define SHELL_HELP( cmd ) \
  extern const char shell_help_##cmd[]; \
  extern const char shell_help_summary_##cmd[]
#define SHELL_INFO( cmd ) { #cmd, shell_help_summary_##cmd, shell_help_##cmd }

// Command / handler pair structure
typedef struct
{
  const char* cmd;
  p_shell_handler handler_func;
} SHELL_COMMAND;

// Help data
typedef struct
{
  const char *cmd;
  const char *help_summary;
  const char *help_full;
} SHELL_HELP_DATA;

// Add additional shell commands here
SHELL_FUNC( shell_help );
SHELL_FUNC( shell_ver );
SHELL_FUNC( shell_syslog );
SHELL_FUNC( shell_stacks );
SHELL_FUNC( shell_test );
SHELL_FUNC( shell_i2c_probe );
SHELL_FUNC( shell_recv );
SHELL_FUNC( shell_send );
SHELL_FUNC( shell_df );
SHELL_FUNC( shell_format );
SHELL_FUNC( shell_fdump );
SHELL_FUNC( shell_rm );
SHELL_FUNC( shell_setvolt );
SHELL_FUNC( shell_setcfg );
SHELL_FUNC( shell_sensor );

SHELL_HELP( help );
SHELL_HELP( ver );
SHELL_HELP( syslog );
SHELL_HELP( stacks );
SHELL_HELP( test );
SHELL_HELP( i2c_probe );
SHELL_HELP( recv );
SHELL_HELP( send );
SHELL_HELP( df );
SHELL_HELP( format );
SHELL_HELP( fdump );
SHELL_HELP( rm );
SHELL_HELP( setvolt );
SHELL_HELP( setcfg );
SHELL_HELP( sensor );

//static const SHELL_COMMAND shell_commands[] =
const SHELL_COMMAND shell_commands[] =
{
  { "help", shell_help },
  { "ver", shell_ver },
  { "syslog", shell_syslog },
  { "stacks", shell_stacks },
  { "test", shell_test },
  { "i2c_probe", shell_i2c_probe },
  { "recv", shell_recv },
  { "send", shell_send },
  { "df", shell_df },
  { "format", shell_format },
  { "fdump", shell_fdump },
  { "rm", shell_rm },
  { "setvolt", shell_setvolt },
  { "setcfg", shell_setcfg },
  { "sensor", shell_sensor },
  { "exit", NULL },
  { NULL, NULL }
};

static const SHELL_HELP_DATA shell_help_data[] =
{
  SHELL_INFO( help ),
  SHELL_INFO( ver ),
  SHELL_INFO( syslog ),
  SHELL_INFO( stacks ),
  SHELL_INFO( test ),
  SHELL_INFO( i2c_probe ),
  SHELL_INFO( recv ),
  SHELL_INFO( send ),
  SHELL_INFO( df ),
  SHELL_INFO( format ),
  SHELL_INFO( fdump ),
  SHELL_INFO( rm ),
  SHELL_INFO( setvolt ),
  SHELL_INFO( setcfg ),
  SHELL_INFO( sensor ),
  { NULL, NULL, NULL }
};

// ****************************************************************************
// Built-in help functions
// ****************************************************************************

static void shell_alphabetize(shell_context_t *ctx)
{
    bool     bSwaped;
    bool     bSorted;
    uint16_t u16HelpTableIdx;
    uint16_t u16HelpTableSize;
    uint16_t u16HelpTableCurIdx;
    uint16_t u16IndicesTableIdx;
    uint16_t u16HelpTablePrevIdx;

    /* Local Inits */
    u16HelpTableIdx  = 1U;
    bSwaped          = false;
    bSorted          = false;
    u16HelpTableSize = sizeof(shell_help_data)/sizeof(SHELL_HELP_DATA);

    /* Allocate array based on the size of the help table */
    ctx->uHelpIndicies = SHELL_MALLOC(u16HelpTableSize * sizeof(*ctx->uHelpIndicies));

    if(ctx->uHelpIndicies != NULL)
    {
        /* Initialize array */
        for(u16IndicesTableIdx = 0U; u16IndicesTableIdx < u16HelpTableSize; u16IndicesTableIdx++)
        {
            ctx->uHelpIndicies[u16IndicesTableIdx] = u16IndicesTableIdx;
        }

        /* The last element in the table is intentionally NULL so exclude from alphabetization */
        u16HelpTableSize--;

        /* Perform alphabetization, sorting the indices in the array */
        do
        {
            /* Find our starting point */
            u16HelpTableCurIdx  = ctx->uHelpIndicies[u16HelpTableIdx];
            u16HelpTablePrevIdx = ctx->uHelpIndicies[u16HelpTableIdx - 1U];

            if(strcmp(shell_help_data[u16HelpTableCurIdx].cmd, shell_help_data[u16HelpTablePrevIdx].cmd) < 0)
            {
                ctx->uHelpIndicies[u16HelpTableIdx]      = u16HelpTablePrevIdx;
                ctx->uHelpIndicies[u16HelpTableIdx - 1U] = u16HelpTableCurIdx;
                bSwaped                                  = true;
            }

            u16HelpTableIdx++;

            /* Exit if we are done sorting - otherwise loop back around again */
            if(u16HelpTableIdx == u16HelpTableSize)
            {
                if(bSwaped == false)
                {
                   bSorted = true;
                }
                else
                {
                   /* Reset parameters */
                   bSwaped         = false;
                   u16HelpTableIdx = 1U;
                }
            }
        }while(bSorted == false);
    }
}

// 'Help' help data
const char shell_help_help[] = "[<command>]\n"
  "  [<command>] - the command to get help on.\n"
  "Without arguments it shows a summary of all the shell commands.\n";
const char shell_help_summary_help[] = "shell help";

void shell_help( shell_context_t *ctx, int argc, char **argv )
{
  const SHELL_HELP_DATA *ph;
  uint16_t u16CmdIdx;
  uint16_t u16TableIdx;

  if( argc > 2 )
  {
    printf( "Invalid arguments. Type help [<command>] for usage.\n" );
    return;
  }
  ph = shell_help_data;
  if( argc == 1 )
  {
    // List commands and their summary
    // It is assumed that a command with an empty summary does not
    // actually exist (helpful for conditional compilation)
    // Noting that if we could not alphabetize due to constrained memory,
    // just print the table as-is.
    u16CmdIdx = 0U;
    u16TableIdx = (ctx->uHelpIndicies == NULL) ? u16CmdIdx : ctx->uHelpIndicies[u16CmdIdx];
    printf( "Shell commands:\n" );
    while( 1 )
    {
        if( ph[u16TableIdx].cmd == NULL )
            break;
        if( strlen( ph[u16TableIdx].help_summary ) > 0 )
            printf( "  %-10s - %s\n", ph[u16TableIdx].cmd, ph[u16TableIdx].help_summary );
        u16CmdIdx++;
        u16TableIdx = (ctx->uHelpIndicies == NULL) ? u16CmdIdx : ctx->uHelpIndicies[u16CmdIdx];
    }
    printf( "For more information use 'help <command>'.\n" );
  }
  else
  {
    while( 1 )
    {
      if( ph->cmd == NULL )
        break;
      if( !strcmp( ph->cmd, argv[ 1 ] ) && strlen( ph->help_summary ) > 0 )
      {
        printf( "%s - %s", ph->cmd, ph->help_summary );
        printf( "\n");
        printf( "Usage: %s %s", ph->cmd, ph->help_full );
        return;
      }
      ph ++;
    }
    printf( "Unknown command '%s'.\n", argv[ 1 ] );
  }
}

// ****************************************************************************
// Built-in version function
// ****************************************************************************
const char shell_help_ver[] = "\n";
const char shell_help_summary_ver[] = "show version information";

void shell_ver( shell_context_t *ctx, int argc, char **argv )
{
  if( argc != 1 )
  {
    printf( "Invalid arguments. Type help [<command>] for usage.\n" );
    return;
  }
  printf( SHELL_WELCOMEMSG, STR_VERSION, __DATE__, __TIME__);
}

// ****************************************************************************
// Shell functions
// ****************************************************************************

// 'Not implemented' handler for shell comands
void shellh_not_implemented_handler( shell_context_t *ctx, int argc, char **argv )
{
  printf( SHELL_ERRMSG );
}

// Executes the given shell command
// 'interactive_mode' is 1 if invoked directly from the interactive shell,
// 0 otherwise
// Returns a pointer to the shell_command that was executed, NULL for error
const SHELL_COMMAND* shellh_execute_command( shell_context_t *ctx, char* cmd, int interactive_mode )
{
  char *p, *temp;
  const SHELL_COMMAND* pcmd;
  int i, inside_quotes;
  char quote_char;
  int argc;
  char *argv[ SHELL_MAX_ARGS ];

  if( strlen( cmd ) == 0 )
    return NULL;

  // Change '\r', '\n' and '\t' chars to ' ' to ease processing
  p = cmd;
  while( *p )
  {
    if( *p == '\r' || *p == '\n' || *p == '\t' )
      *p = ' ';
    p ++;
  }

  // Transform ' ' characters inside a '' or "" quoted string in
  // a 'special' char.
  for( i = 0, inside_quotes = 0, quote_char = '\0'; i < strlen( cmd ); i ++ )
    if( ( cmd[ i ] == '\'' ) || ( cmd[ i ] == '"' ) )
    {
      if( !inside_quotes )
      {
        inside_quotes = 1;
        quote_char = cmd[ i ];
      }
      else
      {
        if( cmd[ i ] == quote_char )
        {
          inside_quotes = 0;
          quote_char = '\0';
        }
      }
    }
    else if( ( cmd[ i ] == ' ' ) && inside_quotes )
      cmd[ i ] = SHELL_ALT_SPACE;
  if( inside_quotes )
  {
    printf( "Invalid quoted string\n" );
    return NULL;
  }

  // Transform consecutive sequences of spaces into a single space
  p = strchr( cmd, ' ' );
  while( p )
  {
    temp = p + 1;
    while( *temp && *temp == ' ' )
      memmove( temp, temp + 1, strlen( temp ) );
    p = strchr( p + 1, ' ' );
  }
  if( !strcmp( cmd, " " ) )
    return NULL;

  // Skip over the trailing space char if it exists
  p = cmd + strlen(cmd) - 1;
  if( *p == ' ' )
    *p = 0;

  // Skip over the initial space char if it exists
  p = cmd;
  if( *p == ' ' )
    p ++;

  // Compute argc/argv
  for( argc = 0; argc < SHELL_MAX_ARGS; argc ++ )
    argv[ argc ] = NULL;
  argc = 0;

  while( ( temp = strchr( p, ' ' ) ) != NULL )
  {
    if( argc < SHELL_MAX_ARGS)
    {
      *temp = 0;
      argv[ argc ++ ] = p;
      p = temp + 1;
    }
    else
    {
      break;
    }
  }
  if (argc < SHELL_MAX_ARGS)
  {
    argv[ argc ++ ] = p;
  }
  else
  {
    printf( "Error: too many arguments\n" );
    return NULL;
  }

  // Additional argument processing happens here
  for( i = 0; i < argc; i ++ )
  {
    p = argv[ i ];
    // Put back spaces if needed
    for( inside_quotes = 0; inside_quotes < strlen( argv[ i ] ); inside_quotes ++ )
    {
      if( p[ inside_quotes ] == SHELL_ALT_SPACE )
        argv[ i ][ inside_quotes ] = ' ';
    }
    // Remove quotes
    if( ( p[ 0 ] == '\'' || p [ 0 ] == '"' ) && ( p[ 0 ] == p[ strlen( p ) - 1 ] ) )
    {
      argv[ i ] = p + 1;
      p[ strlen( p ) - 1 ] = '\0';
    }
  }

  // Match user command with shell's commands
  i = 0;
  while( 1 )
  {
    pcmd = shell_commands + i;
    if( pcmd->cmd == NULL )
    {
      printf( SHELL_ERRMSG );
      break;
    }
    if( !strcmp( pcmd->cmd, argv[ 0 ] ) )
    {
      if( pcmd->handler_func )
        pcmd->handler_func( ctx, argc, argv );
      break;
    }
    i ++;
  }

  return pcmd;
}

void shell_exec( shell_context_t *ctx, const char *command )
{
    char *cmd;
    unsigned len;
    int interactive;

    // Make a copy of the command since it gets overwritten
    len = strlen(command) + 1;
    cmd = SHELL_MALLOC(len);
    memcpy(cmd, command, len);

    // Save interactive status; set non-interactive
    interactive = ctx->interactive;
    ctx->interactive = 0;

    // Execute the command
    shellh_execute_command(ctx, cmd, 0);

    // Restore interactive status
    ctx->interactive = interactive;

    // Free the copy memory
    SHELL_FREE(cmd);
}

void shell_start( shell_context_t *ctx )
{
  //term_reset_mode(&ctx->t);
  //term_sync_size(&ctx->t);
  printf("\n");
  shellh_execute_command(ctx, "ver", 0);
  shell_poll(ctx);
}

void shell_poll( shell_context_t *ctx )
{
  const SHELL_COMMAND *pcmd;
  int result;

  while( 1 )
  {
    do {
      result = linenoise_getline( ctx, ctx->cmd, SHELL_MAX_LINE_LEN - 1, SHELL_PROMPT );
      if (result == LINENOISE_CONTINUE) {
          return;
      }
      if (result == LINENOISE_EOF) {
          printf( "\n" );
          clearerr( stdin );
          break;
      }
    } while (result == LINENOISE_EOF);

    if( strlen( ctx->cmd ) == 0 )
      continue;
    linenoise_addhistory( ctx, ctx->cmd );
    pcmd = shellh_execute_command( ctx, ctx->cmd, 1 );
    // Check for 'exit' command
    if( pcmd && pcmd->cmd && !pcmd->handler_func )
      break;
  }
}

// Initialize the shell, returning 1 for OK and 0 for error
int shell_init( shell_context_t *ctx, p_term_out term_out, p_term_in term_in, int blocking, void *usr )
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->blocking = blocking;
    ctx->usr = usr;
    ctx->interactive = 1;
    shell_platform_init(ctx, term_out, term_in);
    linenoise_init(ctx);
    shell_alphabetize(ctx);
    return 1;
}

void shell_deinit( shell_context_t *ctx )
{
    if (ctx->uHelpIndicies) {
        SHELL_FREE(ctx->uHelpIndicies);
        ctx->uHelpIndicies = NULL;
    }
    linenoise_cleanup(ctx);
    shell_platform_deinit(ctx);
}
