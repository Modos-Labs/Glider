/*!
 * @mainpage Shell Index Page
 * @brief  Small command-line shell for embedded systems
 *
 *    This shell supports:
 *      - Command-line history
 *      - Command-line editing
 *      - Blocking and non-blocking operation
 *      - FreeRTOS and main-loop friendly
 *
 * @file      shell.h
 * @version   1.0.0
 * @sa        shell_cfg.h
 *
*/

#ifndef __SHELL_H__
#define __SHELL_H__

#include "shell_cfg.h"
#include "term.h"

/*!****************************************************************
 * @brief  The number of rows/lines in the display.
 ******************************************************************/
#ifndef SHELL_LINES
#define SHELL_LINES             (25)
#endif

/*!****************************************************************
 * @brief  The number of columns in the display.
 ******************************************************************/
#ifndef SHELL_COLUMNS
#define SHELL_COLUMNS           (80)
#endif

/*!****************************************************************
 * @brief  The function used to allocate memory for the shell.
 *
 * This defaults to the standard C library malloc if not defined
 * otherwise.
 ******************************************************************/
#ifndef SHELL_MALLOC
#define SHELL_MALLOC            malloc
#endif

/*!****************************************************************
 * @brief  The function used to allocate memory for the shell.
 *
 * This defaults to the standard C library malloc if not defined
 * otherwise.
 ******************************************************************/
#ifndef SHELL_CALLOC
#define SHELL_CALLOC            calloc
#endif

/*!****************************************************************
 * @brief  The function used to allocate memory for the shell.
 *
 * This defaults to the standard C library malloc if not defined
 * otherwise.
 ******************************************************************/
#ifndef SHELL_REALLOC
#define SHELL_REALLOC           realloc
#endif

/*!****************************************************************
 * @brief  The function used to free memory.
 *
 * This defaults to the standard C library free if not defined
 * otherwise.
 ******************************************************************/
#ifndef SHELL_FREE
#define SHELL_FREE              free
#endif

/*!****************************************************************
 * @brief  The depth of the command line history buffer.
 *
 * The heap allocate/free routines are not called when this is set
 * to zero.
 ******************************************************************/
#ifndef SHELL_MAX_HISTORIES
#define SHELL_MAX_HISTORIES     (25)
#endif

/*!****************************************************************
 * @brief  The function used for strdup.
 *
 * This defaults to a built-in function that allocates memory using
 * the SHELL_MALLOC allocation routine.
 ******************************************************************/
#ifndef SHELL_STRDUP
#define SHELL_STRDUP            shell_strdup
#endif

/*!****************************************************************
 * @brief  The function used for strndup.
 *
 * This defaults to a built-in function that allocates memory using
 * the SHELL_MALLOC allocation routine.
 ******************************************************************/
#ifndef SHELL_STRNDUP
#define SHELL_STRNDUP           shell_strndup
#endif

/*!****************************************************************
 * @brief  Maximum number of command line arguments
 ******************************************************************/
#ifndef SHELL_MAX_ARGS
#define SHELL_MAX_ARGS          10
#endif

/*!****************************************************************
 * @brief  Shell welcome message string.
 *
 * The %s will be substituted with a version string and the line
 * must end with a \n terminator.
 ******************************************************************/
#ifndef SHELL_WELCOMEMSG
#define SHELL_WELCOMEMSG        "Shell %s\n"
#endif

/*!****************************************************************
 * @brief  Shell command line prompt string.
 ******************************************************************/
#ifndef SHELL_PROMPT
#define SHELL_PROMPT            "# "
#endif

/*!****************************************************************
 * @brief  Shell error message string string.
 *
 * The string must terminate with a newline
 ******************************************************************/
#ifndef SHELL_ERRMSG
#define SHELL_ERRMSG            "Invalid command, type 'help' for help\n"
#endif

/*!****************************************************************
 * @brief  The maximum length of a command line string with all
 *         options.
 ******************************************************************/
#ifndef SHELL_MAX_LINE_LEN
#define SHELL_MAX_LINE_LEN      (SHELL_COLUMNS - 1)
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*!****************************************************************
 * @brief Blocking modes.
 ******************************************************************/
enum BLOCKING_MODES {
    SHELL_MODE_BLOCKING = 0,   /**< Blocking mode */
    SHELL_MODE_NON_BLOCKING    /**< Non-blocking mode */
};

/*!****************************************************************
 * @brief Shell context block
 *
 * This structure holds the shell context between calls to
 * shell_poll().  The application should not set any of these members.
 * The documentation for this structure is to aid in debugging.
 ******************************************************************/
typedef struct {
    char cmd[SHELL_MAX_LINE_LEN + 1];  /**< Buffer to hold the the current command line */
    int blocking;                      /**< Blocking mode.  This is set in shell_init() */
    int history_index;                 /**< Index into the array of history pointers */
    int max_histories;                 /**< Maximum number of history entries.
                                            This is set to SHELL_MAX_HISTORIES */
    int num_histories;                 /**< Number of entries in the history pointer array */
    char **histories;                  /**< The array of history pointers */
    int new_line;                      /**< Indicates a new line must be initialized */
    size_t plen;                       /**< Linenoise line plen */
    size_t pos;                        /**< Linenoise line position */
    size_t len;                        /**< Linenoise current line length */
    size_t cols;                       /**< Linenoise terminal line length */
    int hidden;                        /**< Hide text */
    term_state_t t;                      /**< Terminal state */
    void *usr;                         /**< User data pointer */
    int interactive;                   /**< Interactive shell */
    unsigned *uHelpIndicies;           /**< Alphabetized help indicies */
} shell_context_t;

/*!****************************************************************
 * @brief Typedef of a shell command handler
 *
 * Use this when adding new commands to the shell
 ******************************************************************/
typedef void( *p_shell_handler )( shell_context_t *ctx, int argc, char **argv );

/*!****************************************************************
 *  @brief Shell initialization routine.
 *
 * This function initializes the shell context and sets the
 * working mode to blocking or non-blocking.

 * @param [in]  ctx        Pointer to an application allocated context
 * @param [in]  term_out   Pointer to function to output characters
 * @param [in]  term_in    Pointer to function to input characters
 * @param [in]  mode       Blocking or non-blocking mode.
 *                         @sa BLOCKING_MODES
 * @param [in]  usr        User pointer for term character callback
 *                         functions.
 *
 * @return Returns 1 if successful, otherwise an error.
 ******************************************************************/
int shell_init( shell_context_t *ctx, p_term_out term_out, p_term_in term_in,
    int mode, void *usr );

/*!****************************************************************
 *  @brief Shell de-initialization routine.
 *
 * This function de-initializes and releases all shell resources.

 * @param [in]  ctx        Pointer to an application allocated context
 ******************************************************************/
void shell_deinit( shell_context_t *ctx );

/*!****************************************************************
 *  @brief Shell start routine.
 *
 * This function starts the shell.  It calls shell_poll() once.
 * In blocking mode, this call never returns.
 *
 * @param [in]  ctx        Pointer to an application allocated context
 ******************************************************************/
void shell_start( shell_context_t *ctx );

/*!****************************************************************
 *  @brief Shell poll routine.
 *
 * This function polls for new characters and process completed
 * command lines.  This function should not be called directly in
 * blocking mode.
 *
 * @param [in]  ctx        Pointer to an application allocated context
 ******************************************************************/
void shell_poll( shell_context_t *ctx );

/*!****************************************************************
 *  @brief Shell execute routine.
 *
 * This function executes the given command and argument string as
 * if it were typed on the console.
 *
 * @param [in]  ctx        Pointer to an application allocated context
 * @param [in]  command    Command (and arguments) to execute
 ******************************************************************/
void shell_exec( shell_context_t *ctx, const char *command );

#ifdef __cplusplus
} // extern "C"
#endif

#endif
