/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsystem/win32/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


SHORT
FASTCALL
IntGdiGetLanguageID(VOID)
{
  HANDLE KeyHandle;
  ULONG Size = sizeof(WCHAR) * (MAX_PATH + 12);
  OBJECT_ATTRIBUTES ObAttr;
//  http://support.microsoft.com/kb/324097
  ULONG Ret = 0x409; // English
  PVOID KeyInfo;
  UNICODE_STRING Language;
  
  RtlInitUnicodeString( &Language,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language");

  InitializeObjectAttributes( &ObAttr,
                            &Language,
                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

  if ( NT_SUCCESS(ZwOpenKey(&KeyHandle, KEY_READ, &ObAttr)))
  {
     KeyInfo = ExAllocatePoolWithTag(PagedPool, Size, TAG_STRING);
     if ( KeyInfo )
     {
        RtlInitUnicodeString(&Language, L"Default");

        if ( NT_SUCCESS(ZwQueryValueKey( KeyHandle,
                                         &Language,
                        KeyValuePartialInformation,
                                           KeyInfo,
                                              Size,
                                             &Size)) )
      {
        RtlInitUnicodeString(&Language, (PVOID)((char *)KeyInfo + 12));
        RtlUnicodeStringToInteger(&Language, 16, &Ret);
      }
      ExFreePoolWithTag(KeyInfo, TAG_STRING);
    }
    ZwClose(KeyHandle);
  }
  DPRINT("Language ID = %x\n",Ret);
  return (SHORT) Ret;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserGetThreadState(
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetThreadState\n");
   if (Routine != THREADSTATE_GETTHREADINFO)
   {
       UserEnterShared();
   }
   else
   {
       UserEnterExclusive();
   }

   switch (Routine)
   {
      case THREADSTATE_GETTHREADINFO:
         GetW32ThreadInfo();
         RETURN(0);

      case THREADSTATE_FOCUSWINDOW:
         RETURN( (DWORD)IntGetThreadFocusWindow());
      case THREADSTATE_CAPTUREWINDOW:
         /* FIXME should use UserEnterShared */
         RETURN( (DWORD)IntGetCapture());
      case THREADSTATE_PROGMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->pDeskInfo->hProgmanWindow);
      case THREADSTATE_TASKMANWINDOW:
         RETURN( (DWORD)GetW32ThreadInfo()->pDeskInfo->hTaskManWindow);
      case THREADSTATE_ACTIVEWINDOW:
         RETURN ( (DWORD)UserGetActiveWindow());
      case THREADSTATE_INSENDMESSAGE:
         {
           DWORD Ret = ISMEX_NOSEND;
           PUSER_MESSAGE_QUEUE MessageQueue = 
                ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->MessageQueue;
           DPRINT1("THREADSTATE_INSENDMESSAGE\n");

           if (!IsListEmpty(&MessageQueue->SentMessagesListHead))
           {
             Ret = ISMEX_SEND;
           }
           else if (!IsListEmpty(&MessageQueue->NotifyMessagesListHead))
           {
           /* FIXME Need to set message flag when in callback mode with notify */
             Ret = ISMEX_NOTIFY;
           }
           /* FIXME Need to set message flag if replied to or ReplyMessage */
           RETURN( Ret);           
         }
      case THREADSTATE_GETMESSAGETIME: 
         /* FIXME Needs more work! */
         RETURN( ((PTHREADINFO)PsGetCurrentThreadWin32Thread())->timeLast);

      case THREADSTATE_GETINPUTSTATE:
         RETURN( HIWORD(IntGetQueueStatus(FALSE)) & (QS_KEY | QS_MOUSEBUTTON));
   }
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserGetThreadState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


UINT
APIENTRY
NtUserGetDoubleClickTime(VOID)
{
   UINT Result;

   DPRINT("Enter NtUserGetDoubleClickTime\n");
   UserEnterShared();

   // FIXME: Check if this works on non-interactive winsta
   Result = gspv.iDblClickTime;

   DPRINT("Leave NtUserGetDoubleClickTime, ret=%i\n", Result);
   UserLeave();
   return Result;
}

BOOL
APIENTRY
NtUserGetGUIThreadInfo(
   DWORD idThread, /* if NULL use foreground thread */
   LPGUITHREADINFO lpgui)
{
   NTSTATUS Status;
   PTHRDCARETINFO CaretInfo;
   GUITHREADINFO SafeGui;
   PDESKTOP Desktop;
   PUSER_MESSAGE_QUEUE MsgQueue;
   PETHREAD Thread = NULL;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserGetGUIThreadInfo\n");
   UserEnterShared();

   Status = MmCopyFromCaller(&SafeGui, lpgui, sizeof(DWORD));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   if(SafeGui.cbSize != sizeof(GUITHREADINFO))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( FALSE);
   }

   if(idThread)
   {
      Status = PsLookupThreadByThreadId((HANDLE)idThread, &Thread);
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         RETURN( FALSE);
      }
      Desktop = ((PTHREADINFO)Thread->Tcb.Win32Thread)->Desktop;
   }
   else
   {
      /* get the foreground thread */
      PTHREADINFO W32Thread = (PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread;
      Desktop = W32Thread->Desktop;
      if(Desktop)
      {
         MsgQueue = Desktop->ActiveMessageQueue;
         if(MsgQueue)
         {
            Thread = MsgQueue->Thread;
         }
      }
   }

   if(!Thread || !Desktop)
   {
      if(idThread && Thread)
         ObDereferenceObject(Thread);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   MsgQueue = (PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue;
   CaretInfo = MsgQueue->CaretInfo;

   SafeGui.flags = (CaretInfo->Visible ? GUI_CARETBLINKING : 0);
   if(MsgQueue->MenuOwner)
      SafeGui.flags |= GUI_INMENUMODE | MsgQueue->MenuState;
   if(MsgQueue->MoveSize)
      SafeGui.flags |= GUI_INMOVESIZE;

   /* FIXME add flag GUI_16BITTASK */

   SafeGui.hwndActive = MsgQueue->ActiveWindow;
   SafeGui.hwndFocus = MsgQueue->FocusWindow;
   SafeGui.hwndCapture = MsgQueue->CaptureWindow;
   SafeGui.hwndMenuOwner = MsgQueue->MenuOwner;
   SafeGui.hwndMoveSize = MsgQueue->MoveSize;
   SafeGui.hwndCaret = CaretInfo->hWnd;

   SafeGui.rcCaret.left = CaretInfo->Pos.x;
   SafeGui.rcCaret.top = CaretInfo->Pos.y;
   SafeGui.rcCaret.right = SafeGui.rcCaret.left + CaretInfo->Size.cx;
   SafeGui.rcCaret.bottom = SafeGui.rcCaret.top + CaretInfo->Size.cy;

   if(idThread)
      ObDereferenceObject(Thread);

   Status = MmCopyToCaller(lpgui, &SafeGui, sizeof(GUITHREADINFO));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetGUIThreadInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
APIENTRY
NtUserGetGuiResources(
   HANDLE hProcess,
   DWORD uiFlags)
{
   PEPROCESS Process;
   PPROCESSINFO W32Process;
   NTSTATUS Status;
   DWORD Ret = 0;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetGuiResources\n");
   UserEnterShared();

   Status = ObReferenceObjectByHandle(hProcess,
                                      PROCESS_QUERY_INFORMATION,
                                      PsProcessType,
                                      ExGetPreviousMode(),
                                      (PVOID*)&Process,
                                      NULL);

   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   W32Process = (PPROCESSINFO)Process->Win32Process;
   if(!W32Process)
   {
      ObDereferenceObject(Process);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   switch(uiFlags)
   {
      case GR_GDIOBJECTS:
         {
            Ret = (DWORD)W32Process->GDIHandleCount;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserHandleCount;
            break;
         }
      default:
         {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            break;
         }
   }

   ObDereferenceObject(Process);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserGetGuiResources, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;
   Dest->MaximumLength = Dest->Length;

   if(Dest->Length > 0 && Src)
   {
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePoolWithTag(Dest->Buffer, TAG_STRING);
         Dest->Buffer = NULL;
         return Status;
      }


      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source)
{
   NTSTATUS Status;
   PWSTR Src;

   Status = MmCopyFromCaller(Dest, Source, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      return Status;
   }

   if(Dest->Length > 0x4000)
   {
      return STATUS_UNSUCCESSFUL;
   }

   Src = Dest->Buffer;
   Dest->Buffer = NULL;
   Dest->MaximumLength = 0;

   if(Dest->Length > 0 && Src)
   {
      Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePoolWithTag(Dest->Buffer, TAG_STRING);
         Dest->Buffer = NULL;
         return Status;
      }

      /* make sure the string is null-terminated */
      Src = (PWSTR)((PBYTE)Dest->Buffer + Dest->Length);
      *Src = L'\0';

      return STATUS_SUCCESS;
   }

   /* string is empty */
   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src)
{
   if (Src->Length + sizeof(WCHAR) <= Src->MaximumLength
         && L'\0' == Src->Buffer[Src->Length / sizeof(WCHAR)])
   {
      /* The unicode_string is already nul terminated. Just reuse it. */
      *Dest = Src->Buffer;
      return STATUS_SUCCESS;
   }

   *Dest = ExAllocatePoolWithTag(PagedPool, Src->Length + sizeof(WCHAR), TAG_STRING);
   if (NULL == *Dest)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(*Dest, Src->Buffer, Src->Length);
   (*Dest)[Src->Length / 2] = L'\0';

   return STATUS_SUCCESS;
}

void FASTCALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString)
{
   if (NullTerminated != UnicodeString->Buffer)
   {
      ExFreePool(NullTerminated);
   }
}

PPROCESSINFO
GetW32ProcessInfo(VOID)
{
    return (PPROCESSINFO)PsGetCurrentProcessWin32Process();
}

PTHREADINFO
GetW32ThreadInfo(VOID)
{
    PTEB Teb;
    PPROCESSINFO ppi;
    PCLIENTINFO pci;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti == NULL)
    {
        /* FIXME - temporary hack for system threads... */
        return NULL;
    }
    /* initialize it */
    pti->ppi = ppi = GetW32ProcessInfo();

    pti->pcti = &pti->cti; // FIXME Need to set it in desktop.c!

    if (pti->Desktop != NULL)
    {
       pti->pDeskInfo = pti->Desktop->DesktopInfo;
    }
    else
    {
       pti->pDeskInfo = NULL;
    }
    /* update the TEB */
    Teb = NtCurrentTeb();
    pci = GetWin32ClientInfo();
    pti->pClientInfo = pci;
    _SEH2_TRY
    {
        ProbeForWrite( Teb,
                       sizeof(TEB),
                       sizeof(ULONG));

        Teb->Win32ThreadInfo = (PW32THREAD) pti;

        pci->pClientThreadInfo = NULL; // FIXME Need to set it in desktop.c!
        pci->ppi = ppi;
        pci->fsHooks = pti->fsHooks;
        if (pti->KeyboardLayout) pci->hKL = pti->KeyboardLayout->hkl;
        pci->dwTIFlags = pti->TIF_flags;
        /* CI may not have been initialized. */
        if (!pci->pDeskInfo && pti->pDeskInfo)
        {
           if (!pci->ulClientDelta) pci->ulClientDelta = DesktopHeapGetUserDelta();

           pci->pDeskInfo = (PVOID)((ULONG_PTR)pti->pDeskInfo - pci->ulClientDelta);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return pti;
}


/* EOF */
