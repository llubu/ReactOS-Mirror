/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __STDLIB_H
#define __STDLIB_H

#include <freeldr.h>

///////////////////////////////////////////////////////////////////////////////////////
//
// Memory Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		memcmp(const void *buf1, const void *buf2, size_t count);
void *	memcpy(void *to, const void *from, size_t count);
void *	memmove(void *dest, const void *src, size_t count);
void *	memset(void *src, int val, size_t count);

#define RtlCompareMemory(Source1, Source2, Length)	memcmp(Source1, Source2, Length)
#define RtlCopyMemory(Destination, Source, Length)	memcpy(Destination, Source, Length)
#define RtlFillMemory(Destination, Length, Fill)	memset(Destination, Fill, Length)
#define RtlZeroMemory(Destination, Length)			memset(Destination, 0, Length)

///////////////////////////////////////////////////////////////////////////////////////
//
// Standard Library Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		atoi(char *string);
char *	itoa(int value, char *string, int radix);
int		toupper(int c);
int		tolower(int c);

int		isspace(int c);
int		isdigit(int c);
int		isxdigit(int c);

char *	convert_to_ascii(char *buf, int c, ...);
char *	convert_i64_to_ascii(char *buf, int c, ...);

void	beep(void);
void	delay(unsigned msec);
void	sound(int freq);

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define UINT64_C(val) val##ULL

///////////////////////////////////////////////////////////////////////////////////////
//
// Screen Output Functions
//
///////////////////////////////////////////////////////////////////////////////////////
void	print(char *str);
void	printf(char *fmt, ...);
void	sprintf(char *buffer, char *format, ...);

///////////////////////////////////////////////////////////////////////////////////////
//
// List Functions
//
///////////////////////////////////////////////////////////////////////////////////////

typedef struct _LIST_ITEM
{
	struct _LIST_ITEM*	ListPrev;
	struct _LIST_ITEM*	ListNext;

} LIST_ITEM, *PLIST_ITEM;

VOID		RtlListInitializeHead(PLIST_ITEM ListHead);							// Initializes a doubly linked list
VOID		RtlListInsertHead(PLIST_ITEM ListHead, PLIST_ITEM Entry);			// Inserts an entry at the head of the list
VOID		RtlListInsertTail(PLIST_ITEM ListHead, PLIST_ITEM Entry);			// Inserts an entry at the tail of the list
PLIST_ITEM	RtlListRemoveHead(PLIST_ITEM ListHead);								// Removes the entry at the head of the list
PLIST_ITEM	RtlListRemoveTail(PLIST_ITEM ListHead);								// Removes the entry at the tail of the list
PLIST_ITEM	RtlListGetHead(PLIST_ITEM ListHead);								// Returns the entry at the head of the list
PLIST_ITEM	RtlListGetTail(PLIST_ITEM ListHead);								// Returns the entry at the tail of the list
BOOL		RtlListIsEmpty(PLIST_ITEM ListHead);								// Indicates whether a doubly linked list is empty
ULONG		RtlListCountEntries(PLIST_ITEM ListHead);							// Counts the entries in a doubly linked list
PLIST_ITEM	RtlListGetPrevious(PLIST_ITEM ListEntry);							// Returns the previous item in the list
PLIST_ITEM	RtlListGetNext(PLIST_ITEM ListEntry);								// Returns the next item in the list
PLIST_ITEM	RtlListRemoveEntry(PLIST_ITEM ListEntry);							// Removes the entry from the list
VOID		RtlListInsertEntry(PLIST_ITEM InsertAfter, PLIST_ITEM ListEntry);	// Inserts a new list entry right after the specified one
VOID		RtlListMoveEntryPrevious(PLIST_ITEM ListEntry);						// Moves the list entry to before the previous entry
VOID		RtlListMoveEntryNext(PLIST_ITEM ListEntry);							// Moves the list entry to after the next entry


#endif  // defined __STDLIB_H
