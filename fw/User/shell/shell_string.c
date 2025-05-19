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

#include <string.h>
#include <stdlib.h>

#include "shell.h"

char *shell_strdup(const char *str)
{
   size_t size;
   char *copy;

   size = strlen(str) + 1;
   if ((copy = SHELL_MALLOC(size)) == NULL)
      return(NULL);

   (void)memcpy(copy, str, size);
   return(copy);
}

char *shell_strndup(const char *s, size_t n)
{
  char *result;
  size_t size = strlen(s);

  if (n < size)
    size = n;

  result = (char *)SHELL_MALLOC(size + 1);
  if (!result)
    return 0;

  result[size] = '\0';
  return ((char *)memcpy(result, s, size));
}
