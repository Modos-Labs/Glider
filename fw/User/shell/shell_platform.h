#ifndef _SHELL_PLATFORM_H
#define _SHELL_PLATFORM_H

#include "shell.h"

void shell_platform_init(shell_context_t *ctx, p_term_out term_out, p_term_in term_in);
void shell_platform_deinit(shell_context_t *ctx);

#endif
