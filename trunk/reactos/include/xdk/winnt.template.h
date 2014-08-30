/*
 * winnt.h
 *
 * Windows NT native definitions for user mode
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
#define _WINNT_

/* We require WDK / VS 2008 or newer */
#if defined(_MSC_VER) && (_MSC_VER < 1500)
#error Compiler too old!
#endif

/* HACK for wine code */
#if !defined(__ROS_LONG64__)
#ifdef __WINESRC__
#define __ROS_LONG64__
#endif
#endif

#include <ctype.h>
//#include <winapifamily.h>
#include <msvctarget.h>
#include <specstrings.h>
#include <kernelspecs.h>

#include <excpt.h>
#include <basetsd.h>
#include <guiddef.h>
#include <intrin.h>

#undef __need_wchar_t
#include <winerror.h>
#include <stddef.h>
#include <sdkddkver.h>
#ifndef RC_INVOKED
#include <string.h>
#endif

/* Silence some MSVC warnings */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif

#ifdef __cplusplus
extern "C" {
#endif

$define(_WINNT_)
$define(ULONG=DWORD)
$define(USHORT=WORD)
$define(UCHAR=BYTE)
$include(ntbasedef.h)
$include(winnt_old.h)

#ifdef __cplusplus
} // extern "C"
#endif
