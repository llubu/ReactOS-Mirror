/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ob/handle.c
 * PURPOSE:            Managing handles
 * PROGRAMMER:         David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *                 17/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define HANDLE_BLOCK_ENTRIES ((PAGESIZE-sizeof(LIST_ENTRY))/sizeof(HANDLE_REP))

/*
 * PURPOSE: Defines a page's worth of handles
 */
typedef struct
{
   LIST_ENTRY entry;
   HANDLE_REP handles[HANDLE_BLOCK_ENTRIES];
} HANDLE_BLOCK;

/* FUNCTIONS ***************************************************************/

static PHANDLE_REP ObpGetObjectByHandle(PEPROCESS Process,
					HANDLE h)
/*
 * FUNCTION: Get the data structure for a handle
 * ARGUMENTS:
 *         Process = Process to get the handle for
 *         h = Handle
 * ARGUMENTS: A pointer to the information about the handle on success,
 *            NULL on failure
 */
{
   PLIST_ENTRY current;
   unsigned int handle = ((unsigned int)h) - 1;
   unsigned int count=handle/HANDLE_BLOCK_ENTRIES;
   HANDLE_BLOCK* blk = NULL;
   unsigned int i;
   
   DPRINT("ObpGetObjectByHandle(Process %x, h %x)\n",Process,h);
   
   current = Process->Pcb.HandleTable.ListHead.Flink;
   DPRINT("current %x\n",current);
   
   for (i=0;i<count;i++)
     {
	current = current->Flink;
	if (current == (&(Process->Pcb.HandleTable.ListHead)))
	  {
	     return(NULL);
	  }
     }
   
   blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);
   return(&(blk->handles[handle%HANDLE_BLOCK_ENTRIES]));
}

NTSTATUS
STDCALL
NtDuplicateObject (
	IN	HANDLE		SourceProcessHandle,
	IN	PHANDLE		SourceHandle,
	IN	HANDLE		TargetProcessHandle,
	OUT	PHANDLE		TargetHandle,
	IN	ACCESS_MASK	DesiredAccess,
	IN	BOOLEAN		InheritHandle,
		ULONG		Options
	)
/*
 * FUNCTION: Copies a handle from one process space to another
 * ARGUMENTS:
 *         SourceProcessHandle = The source process owning the handle. The 
 *                               source process should have opened
 *			         the SourceHandle with PROCESS_DUP_HANDLE 
 *                               access.
 *	   SourceHandle = The handle to the object.
 *	   TargetProcessHandle = The destination process owning the handle 
 *	   TargetHandle (OUT) = Caller should supply storage for the 
 *                              duplicated handle. 
 *	   DesiredAccess = The desired access to the handle.
 *	   InheritHandle = Indicates wheter the new handle will be inheritable
 *                         or not.
 *	   Options = Specifies special actions upon duplicating the handle. 
 *                   Can be one of the values DUPLICATE_CLOSE_SOURCE | 
 *                   DUPLICATE_SAME_ACCESS. DUPLICATE_CLOSE_SOURCE specifies 
 *                   that the source handle should be closed after duplicating. 
 *                   DUPLICATE_SAME_ACCESS specifies to ignore the 
 *                   DesiredAccess paramter and just grant the same access to 
 *                   the new handle.
 * RETURNS: Status
 * REMARKS: This function maps to the win32 DuplicateHandle.
 */
{
   PEPROCESS SourceProcess;
   PEPROCESS TargetProcess;
   PHANDLE_REP SourceHandleRep;

   ASSERT_IRQL(PASSIVE_LEVEL);
   
   ObReferenceObjectByHandle(SourceProcessHandle,
			     PROCESS_DUP_HANDLE,
			     NULL,
			     UserMode,
			     (PVOID*)&SourceProcess,
			     NULL);
   ObReferenceObjectByHandle(TargetProcessHandle,
			     PROCESS_DUP_HANDLE,
			     NULL,
			     UserMode,
			     (PVOID*)&TargetProcess,
			     NULL);
   
   SourceHandleRep = ObpGetObjectByHandle(SourceProcess,
					  *SourceHandle);
   
   if (Options & DUPLICATE_SAME_ACCESS)
     {
	DesiredAccess = SourceHandleRep->GrantedAccess;
     }
   
   ObCreateHandle(TargetProcess,
		  SourceHandleRep->ObjectBody,
		  DesiredAccess,
		  InheritHandle,
		  TargetHandle);
   
   if (Options & DUPLICATE_CLOSE_SOURCE)
     {
	ZwClose(*SourceHandle);
     }
   
   ObDereferenceObject(TargetProcess);
   ObDereferenceObject(SourceProcess);
   
   return(STATUS_SUCCESS);
}

VOID ObDeleteHandleTable(PEPROCESS Process)
/*
 * FUNCTION: Deletes the handle table associated with a process
 */
{
   PLIST_ENTRY current = NULL;
   ULONG i;
   PHANDLE_TABLE HandleTable = NULL;
   
   HandleTable = &Process->Pcb.HandleTable;
   current = RemoveHeadList(&HandleTable->ListHead);
   
   while (current!=NULL)
     {
	HANDLE_BLOCK* HandleBlock = CONTAINING_RECORD(current,
						      HANDLE_BLOCK,
						      entry);
	
	/*
	 * Deference every handle in block
	 */
	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
	     if (HandleBlock->handles[i].ObjectBody != NULL)
	       {
		  ObDereferenceObject(HandleBlock->handles[i].ObjectBody);
	       }
	  }
	
	ExFreePool(HandleBlock);
	
	current = RemoveHeadList(&HandleTable->ListHead);
     }
}

VOID ObCreateHandleTable(PEPROCESS Parent,
			 BOOLEAN Inherit,
			 PEPROCESS Process)
/*
 * FUNCTION: Creates a handle table for a process
 * ARGUMENTS:
 *       Parent = Parent process (or NULL if this is the first process)
 *       Inherit = True if the process should inherit its parent's handles
 *       Process = Process whose handle table is to be created
 */
{
   DPRINT("ObCreateHandleTable(Parent %x, Inherit %d, Process %x)\n",
	  Parent,Inherit,Process);
   
   InitializeListHead(&(Process->Pcb.HandleTable.ListHead));
   KeInitializeSpinLock(&(Process->Pcb.HandleTable.ListLock));
   
   if (Parent != NULL)
     {
     }
}

VOID ObDeleteHandle(HANDLE Handle)
{
   PHANDLE_REP Rep;
   
   DPRINT("ObDeleteHandle(Handle %x)\n",Handle);
   
   Rep = ObpGetObjectByHandle(PsGetCurrentProcess(),Handle);
   Rep->ObjectBody=NULL;
   DPRINT("Finished ObDeleteHandle()\n");
}

NTSTATUS ObCreateHandle(PEPROCESS Process,
			PVOID ObjectBody,
			ACCESS_MASK GrantedAccess,
			BOOLEAN Inherit,
			PHANDLE HandleReturn)
/*
 * FUNCTION: Add a handle referencing an object
 * ARGUMENTS:
 *         obj = Object body that the handle should refer to
 * RETURNS: The created handle
 * NOTE: THe handle is valid only in the context of the current process
 */
{
   LIST_ENTRY* current;
   unsigned int handle=1;
   unsigned int i;
   HANDLE_BLOCK* new_blk = NULL;
   PHANDLE_TABLE HandleTable;
   KIRQL oldlvl;
   
   DPRINT("ObAddHandle(Process %x, obj %x)\n",Process,ObjectBody);
   
   HandleTable = &Process->Pcb.HandleTable;
   
   KeAcquireSpinLock(&HandleTable->ListLock, &oldlvl);
   current = HandleTable->ListHead.Flink;
   
   /*
    * Scan through the currently allocated handle blocks looking for a free
    * slot
    */
   while (current != (&HandleTable->ListHead))
     {
	HANDLE_BLOCK* blk = CONTAINING_RECORD(current,HANDLE_BLOCK,entry);

        DPRINT("Current %x\n",current);

	for (i=0;i<HANDLE_BLOCK_ENTRIES;i++)
	  {
             DPRINT("Considering slot %d containing %x\n",i,blk->handles[i]);
	     if (blk->handles[i].ObjectBody==NULL)
	       {
		  blk->handles[i].ObjectBody = ObjectBody;
		  blk->handles[i].GrantedAccess = GrantedAccess;
		  blk->handles[i].Inherit = Inherit;
		  KeReleaseSpinLock(&HandleTable->ListLock, oldlvl);
		  *HandleReturn = (HANDLE)(handle + i);
		  return(STATUS_SUCCESS);
	       }
	  }
	
	handle = handle + HANDLE_BLOCK_ENTRIES;
	current = current->Flink;
     }
   
   /*
    * Add a new handle block to the end of the list
    */
   new_blk = (HANDLE_BLOCK *)ExAllocatePool(NonPagedPool,sizeof(HANDLE_BLOCK));
   memset(new_blk,0,sizeof(HANDLE_BLOCK));
   InsertTailList(&(Process->Pcb.HandleTable.ListHead),
		  &new_blk->entry);
   KeReleaseSpinLock(&HandleTable->ListLock, oldlvl);
   new_blk->handles[0].ObjectBody = ObjectBody;
   new_blk->handles[0].GrantedAccess = GrantedAccess;
   new_blk->handles[0].Inherit = Inherit;
   *HandleReturn = (HANDLE)handle;
   return(STATUS_SUCCESS);   
}


NTSTATUS ObReferenceObjectByHandle(HANDLE Handle,
				   ACCESS_MASK DesiredAccess,
				   POBJECT_TYPE ObjectType,
				   KPROCESSOR_MODE AccessMode,
				   PVOID* Object,
				   POBJECT_HANDLE_INFORMATION 
				           HandleInformationPtr)
/*
 * FUNCTION: Increments the reference count for an object and returns a 
 * pointer to its body
 * ARGUMENTS:
 *         Handle = Handle for the object
 *         DesiredAccess = Desired access to the object
 *         ObjectType
 *         AccessMode 
 *         Object (OUT) = Points to the object body on return
 *         HandleInformation (OUT) = Contains information about the handle 
 *                                   on return
 * RETURNS: Status
 */
{
   PHANDLE_REP HandleRep;
   POBJECT_HEADER ObjectHeader;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   DPRINT("ObReferenceObjectByHandle(Handle %x, DesiredAccess %x, "
	  "ObjectType %x, AccessMode %d, Object %x)\n",Handle,DesiredAccess,
	  ObjectType,AccessMode,Object);

   
   
   if (Handle == NtCurrentProcess() && 
       (ObjectType == PsProcessType || ObjectType == NULL))
     {
	BODY_TO_HEADER(PsGetCurrentProcess())->RefCount++;
	*Object = PsGetCurrentProcess();
	DPRINT("Referencing current process %x\n", PsGetCurrentProcess());
	return(STATUS_SUCCESS);
     }
   else if (Handle == NtCurrentProcess())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }
   if (Handle == NtCurrentThread() && 
       (ObjectType == PsThreadType || ObjectType == NULL))
     {
	BODY_TO_HEADER(PsGetCurrentThread())->RefCount++;
	*Object = PsGetCurrentThread();
	CHECKPOINT;
	return(STATUS_SUCCESS);
     }
   else if (Handle == NtCurrentThread())
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }
   
   HandleRep = ObpGetObjectByHandle(PsGetCurrentProcess(),
				    Handle);
   if (HandleRep == NULL || HandleRep->ObjectBody == NULL)
     {
	CHECKPOINT;
	return(STATUS_INVALID_HANDLE);
     }
   
   ObjectHeader = BODY_TO_HEADER(HandleRep->ObjectBody);

   if (ObjectType != NULL && ObjectType != ObjectHeader->ObjectType)
     {
	CHECKPOINT;
	return(STATUS_OBJECT_TYPE_MISMATCH);
     }   
   
   if (!(HandleRep->GrantedAccess & DesiredAccess))
     {
	CHECKPOINT;
	return(STATUS_ACCESS_DENIED);
     }
   
   ObjectHeader->RefCount++;

   *Object = HandleRep->ObjectBody;
   
   CHECKPOINT;
   return(STATUS_SUCCESS);
}

NTSTATUS NtClose(HANDLE Handle)
/*
 * FUNCTION: Closes a handle reference to an object
 * ARGUMENTS:
 *         Handle = handle to close
 * RETURNS: Status
 */
{
   PVOID ObjectBody;
   POBJECT_HEADER Header;
   PHANDLE_REP HandleRep;
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("NtClose(Handle %x)\n",Handle);
   
   HandleRep = ObpGetObjectByHandle(PsGetCurrentProcess(),
				    Handle);
   if (HandleRep == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   ObjectBody = HandleRep->ObjectBody;
   
   HandleRep->ObjectBody = NULL;
   
   Header = BODY_TO_HEADER(ObjectBody);
   
   Header->RefCount++;
   Header->HandleCount--;
   
   if (Header->ObjectType != NULL &&
       Header->ObjectType->Close != NULL)
     {
	Header->ObjectType->Close(ObjectBody, Header->HandleCount);
     }
   
   Header->RefCount--;
   
   ObPerformRetentionChecks(Header);
   
   return(STATUS_SUCCESS);
}
