/* $Id$
 *
 * reactos/subsys/csrss/api/handle.c
 *
 * CSRSS handle functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <csrss/csrss.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include "api.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static unsigned ObjectDefinitionsCount = 0;
static PCSRSS_OBJECT_DEFINITION ObjectDefinitions = NULL;

NTSTATUS FASTCALL
CsrRegisterObjectDefinitions(PCSRSS_OBJECT_DEFINITION NewDefinitions)
{
  unsigned NewCount;
  PCSRSS_OBJECT_DEFINITION Scan;
  PCSRSS_OBJECT_DEFINITION New;

  NewCount = 0;
  for (Scan = NewDefinitions; 0 != Scan->Type; Scan++)
    {
      NewCount++;
    }

  New = RtlAllocateHeap(CsrssApiHeap, 0,
                        (ObjectDefinitionsCount + NewCount)
                        * sizeof(CSRSS_OBJECT_DEFINITION));
  if (NULL == New)
    {
      DPRINT1("Unable to allocate memory\n");
      return STATUS_NO_MEMORY;
    }
  if (0 != ObjectDefinitionsCount)
    {
      RtlCopyMemory(New, ObjectDefinitions,
                    ObjectDefinitionsCount * sizeof(CSRSS_OBJECT_DEFINITION));
      RtlFreeHeap(CsrssApiHeap, 0, ObjectDefinitions);
    }
  RtlCopyMemory(New + ObjectDefinitionsCount, NewDefinitions,
                NewCount * sizeof(CSRSS_OBJECT_DEFINITION));
  ObjectDefinitions = New;
  ObjectDefinitionsCount += NewCount;

  return STATUS_SUCCESS;
}

NTSTATUS STDCALL CsrGetObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle, Object_t **Object )
{
  ULONG h = (((ULONG)Handle) >> 2) - 1;
  DPRINT("CsrGetObject, Object: %x, %x, %x\n", Object, Handle, ProcessData ? ProcessData->HandleTableSize : 0);

  if (ProcessData == NULL)
    {
      return STATUS_INVALID_PARAMETER;
    }
  if (ProcessData->HandleTableSize <= h)
    {
      DPRINT1("CsrGetObject returning invalid handle\n");
      return STATUS_INVALID_HANDLE;
    }
  *Object = ProcessData->HandleTable[h];
  //   DbgPrint( "CsrGetObject returning\n" );
  return *Object ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
}


NTSTATUS STDCALL
CsrReleaseObjectByPointer(Object_t *Object)
{
  BOOL Found;
  unsigned DefIndex;

  /* dec ref count */
  if (InterlockedDecrement(&Object->ReferenceCount) == 0)
    {
      Found = FALSE;
      for (DefIndex = 0; ! Found && DefIndex < ObjectDefinitionsCount; DefIndex++)
        {
          if (Object->Type == ObjectDefinitions[DefIndex].Type)
            {
              (ObjectDefinitions[DefIndex].CsrCleanupObjectProc)(Object);
              Found = TRUE;
            }
        }

      if (! Found)
        {
	  DPRINT1("CSR: Error: releaseing unknown object type 0x%x", Object->Type);
        }
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle)
{
  ULONG h = (((ULONG)Handle) >> 2) - 1;
  NTSTATUS Status;

  if (ProcessData == NULL)
    {
      return STATUS_INVALID_PARAMETER;
    }
  if (h >= ProcessData->HandleTableSize || ProcessData->HandleTable[h] == NULL)
    {
      return STATUS_INVALID_HANDLE;
    }

  Status = CsrReleaseObjectByPointer(ProcessData->HandleTable[h]);

  RtlEnterCriticalSection(&ProcessData->HandleTableLock);
  ProcessData->HandleTable[h] = 0;
  RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
  return Status;
}

NTSTATUS STDCALL CsrInsertObject( PCSRSS_PROCESS_DATA ProcessData, PHANDLE Handle, Object_t *Object )
{
   ULONG i;
   PVOID* NewBlock;

   if (ProcessData == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }

   RtlEnterCriticalSection(&ProcessData->HandleTableLock);

   for (i = 0; i < ProcessData->HandleTableSize; i++)
     {
	if (ProcessData->HandleTable[i] == NULL)
	  {
            break;
	  }
     }
   if (i >= ProcessData->HandleTableSize)
     {
       NewBlock = RtlAllocateHeap(CsrssApiHeap,
			          HEAP_ZERO_MEMORY,
			          (ProcessData->HandleTableSize + 64) * sizeof(HANDLE));
       if (NewBlock == NULL)
         {
           RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
	   return(STATUS_UNSUCCESSFUL);
         }
       RtlCopyMemory(NewBlock,
		     ProcessData->HandleTable,
		     ProcessData->HandleTableSize * sizeof(HANDLE));
       RtlFreeHeap( CsrssApiHeap, 0, ProcessData->HandleTable );
       ProcessData->HandleTable = (Object_t **)NewBlock;
       ProcessData->HandleTableSize += 64;
     }
   ProcessData->HandleTable[i] = Object;
   *Handle = (HANDLE)(((i + 1) << 2) | 0x3);
   InterlockedIncrement( &Object->ReferenceCount );
   RtlLeaveCriticalSection(&ProcessData->HandleTableLock);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL CsrVerifyObject( PCSRSS_PROCESS_DATA ProcessData, HANDLE Handle )
{
  ULONG h = (((ULONG)Handle) >> 2) - 1;

  if (ProcessData == NULL)
  {
      return STATUS_INVALID_PARAMETER;
  }
  if (h >= ProcessData->HandleTableSize)
    {
      return STATUS_INVALID_HANDLE;
    }

  return ProcessData->HandleTable[h] ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
}

/* EOF */
