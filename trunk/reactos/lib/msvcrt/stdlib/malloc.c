/*
 * msvcrt.dll heap functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/malloc.h>

extern HANDLE hHeap;

/*
 * @implemented
 */
void* malloc(size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _size);
}

/*
 * @implemented
 */
void free(void* _ptr)
{
   HeapFree(hHeap,0,_ptr);
}

/*
 * @implemented
 */
void* calloc(size_t _nmemb, size_t _size)
{
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _nmemb*_size);
}

/*
 * @implemented
 */
void* realloc(void* _ptr, size_t _size)
{
   return HeapReAlloc(hHeap, 0, _ptr, _size);
}

/*
 * @implemented
 */
void* _expand(void* _ptr, size_t _size)
{
   return HeapReAlloc(hHeap, HEAP_REALLOC_IN_PLACE_ONLY, _ptr, _size);
}

/*
 * @implemented
 */
size_t _msize(void* _ptr)
{
   return HeapSize(hHeap, 0, _ptr);
}

/*
 * @implemented
 */
int	_heapchk(void)
{
	if (!HeapValidate(hHeap, 0, NULL))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapmin(void)
{
	if (!HeapCompact(hHeap, 0))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapset(unsigned int unFill)
{
	if (_heapchk() == -1)
		return -1;
	return 0;

}

/*
 * @implemented
 */
int _heapwalk(struct _heapinfo* entry)
{
	return 0;
}

