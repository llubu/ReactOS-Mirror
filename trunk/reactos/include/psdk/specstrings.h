/*
 * specstrings.h
 *
 * Standard Annotation Language (SAL) definitions
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#define SPECSTRINGS_H

#include <sal.h>
#include <driverspecs.h>

#define __field_bcount(size) __notnull __byte_writableTo(size)
#define __field_ecount(size) __notnull __elem_writableTo(size)

#define __deref_in
#define __deref_in_ecount(size)
#define __deref_in_bcount(size)
#define __deref_in_opt
#define __deref_in_ecount_opt(size)
#define __deref_in_bcount_opt(size)
#define __deref_opt_in
#define __deref_opt_in_ecount(size)
#define __deref_opt_in_bcount(size)
#define __deref_opt_in_opt
#define __deref_opt_in_ecount_opt(size)
#define __deref_opt_in_bcount_opt(size)
#define __out_awcount(expr,size)
#define __in_awcount(expr,size)
#define __nullnullterminated
#define __in_data_source(src_sym)
#define __analysis_noreturn
#define __kernel_entry

