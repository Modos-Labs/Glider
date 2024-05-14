#include <stdlib.h>
#include <string.h>

/*
 * Given a string which might contain unescaped newlines, split it up into
 * lines which do not contain unescaped newlines, returned as a
 * NULL-terminated array of malloc'd strings.
 */
char **split_on_unescaped_newlines(const char *txt) {
    const char *ptr, *lineStart;
    char **buf, **bptr;
    int fQuote, nLines;

    /* First pass: count how many lines we will need */
    for ( nLines = 1, ptr = txt, fQuote = 0; *ptr; ptr++ ) {
        if ( fQuote ) {
            if ( *ptr == '\"' ) {
                if ( ptr[1] == '\"' ) {
                    ptr++;
                    continue;
                }
                fQuote = 0;
            }
        } else if ( *ptr == '\"' ) {
            fQuote = 1;
        } else if ( *ptr == '\n' ) {
            nLines++;
        }
    }

    buf = malloc( sizeof(char*) * (nLines+1) );

    if ( !buf ) {
        return NULL;
    }

    /* Second pass: populate results */
    lineStart = txt;
    for ( bptr = buf, ptr = txt, fQuote = 0; ; ptr++ ) {
        if ( fQuote ) {
            if ( *ptr == '\"' ) {
                if ( ptr[1] == '\"' ) {
                    ptr++;
                    continue;
                }
                fQuote = 0;
                continue;
            } else if ( *ptr ) {
                continue;
            }
        }

        if ( *ptr == '\"' ) {
            fQuote = 1;
        } else if ( *ptr == '\n' || !*ptr ) {
            size_t len = ptr - lineStart;

            if ( len == 0 ) {
                *bptr = NULL;
                return buf;
            }

            *bptr = malloc( len + 1 );

            if ( !*bptr ) {
                for ( bptr--; bptr >= buf; bptr-- ) {
                    free( *bptr );
                }
                free( buf );
                return NULL;
            }

            memcpy( *bptr, lineStart, len );
            (*bptr)[len] = '\0';

            if ( *ptr ) {
                lineStart = ptr + 1;
                bptr++;
            } else {
                bptr[1] = NULL;
                return buf;
            }
        }
    }
}
