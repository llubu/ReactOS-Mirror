/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include "precomp.h"
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>
#include <msvcrt/string.h>

#undef putchar


/*
 * @implemented
 */
int puts(const char *s)
{
    int c;

    while ((c = *s++))
        putchar(c);
    return putchar('\n');

}
