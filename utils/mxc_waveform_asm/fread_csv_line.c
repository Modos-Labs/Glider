#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "csv.h"

#define READ_BLOCK_SIZE 65536
#define QUICK_GETC( ch, fp )\
do\
{\
    if ( read_ptr == read_end ) {\
        fread_len = fread( read_buf, sizeof(char), READ_BLOCK_SIZE, fp );\
        if ( fread_len < READ_BLOCK_SIZE ) {\
            read_buf[fread_len] = '\0';\
        }\
        read_ptr = read_buf;\
    }\
    ch = *read_ptr++;\
}\
while(0)

/*
 * Given a file pointer, read a CSV line from that file.
 * File may include newlines escaped with "double quotes".
 *
 * Warning: This function is optimized for the use case where
 *   you repeatedly call it until the file is exhausted.  It is
 *   very suboptimal for the use case of just grabbing one single
 *   line of CSV and stopping.  Also, this function advances the
 *   file position (in the fseek/ftell sense) unpredictably.  You
 *   should not change the file position between calls to
 *   fread_csv_line (e.g., don't use "getc" on the file in between
 *   calls to fread_csv_line).
 *
 * Other arguments:
 * size_t max_line_size: Maximum line size, in bytes.
 * int *done: Pointer to an int that will be set to 1 when file is exhausted.
 * int *err: Pointer to an int where error code will be written.
 *
 * Warning: Calling this function on an exhausted file (as indicated by the
 *   'done' flag) is undefined behavior.
 *
 * See csv.h for definitions of error codes.
 */
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err, int rst) {
    static FILE *bookmark;
    static char read_buf[READ_BLOCK_SIZE], *read_ptr, *read_end;
    static int fread_len, prev_max_line_size = -1;
    static char *buf;
    char *bptr, *limit;
    char ch;
    int fQuote;

    if ( max_line_size > prev_max_line_size ) {
        if ( prev_max_line_size != -1 ) {
            free( buf );
        }
        buf = malloc( max_line_size + 1 );
        if ( !buf ) {
            *err = CSV_ERR_NO_MEMORY;
            prev_max_line_size = -1;
            return NULL;
        }
        prev_max_line_size = max_line_size;
    }
    bptr = buf;
    limit = buf + max_line_size;

    if (( bookmark != fp ) || ( rst )) {
        read_ptr = read_end = read_buf + READ_BLOCK_SIZE;
        bookmark = fp;
    }

    for ( fQuote = 0; ; ) {
        QUICK_GETC(ch, fp);

        if ( !ch || (ch == '\n' && !fQuote)) {
            break;
        }

        if ( bptr >= limit ) {
            free( buf );
            *err = CSV_ERR_LONGLINE;
            return NULL;
        }
        *bptr++ = ch;

        if ( fQuote ) {
            if ( ch == '\"' ) {
                QUICK_GETC(ch, fp);

                if ( ch != '\"' ) {
                    if ( !ch || ch == '\n' ) {
                        break;
                    }
                    fQuote = 0;
                }
                *bptr++ = ch;
            }
        } else if ( ch == '\"' ) {
            fQuote = 1;
        }
    }

    *done = !ch;
    *bptr = '\0';
    return strdup(buf);
}
