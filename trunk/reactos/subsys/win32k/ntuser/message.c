/* $Id: message.c,v 1.3 2002/01/13 22:52:08 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
W32kInitMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS
W32kCleanupMessageImpl(VOID)
{
  return(STATUS_SUCCESS);
}


LRESULT STDCALL
NtUserDispatchMessage(LPMSG lpmsg)
{
  UNIMPLEMENTED;
    
  return 0;
}

BOOL STDCALL
NtUserGetMessage(LPMSG lpMsg,
		 HWND hWnd,
		 UINT wMsgFilterMin,
		 UINT wMsgFilterMax)
/*
 * FUNCTION: Get a message from the calling thread's message queue.
 * ARGUMENTS:
 *      lpMsg - Pointer to the structure which receives the returned message.
 *      hWnd - Window whose messages are to be retrieved.
 *      wMsgFilterMin - Integer value of the lowest message value to be
 *                      retrieved.
 *      wMsgFilterMax - Integer value of the highest message value to be
 *                      retrieved.
 */
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  BOOLEAN Present;
  PUSER_MESSAGE Message;
  NTSTATUS Status;

  W32kGuiCheck();

  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

  do
    {
      /* FIXME: Dispatch sent messages here. */
      
      /* Now look for a quit message. */
      /* FIXME: WINE checks the message number filter here. */
      if (ThreadQueue->QuitPosted)
	{
	  lpMsg->hwnd = hWnd;
	  lpMsg->message = WM_QUIT;
	  lpMsg->wParam = ThreadQueue->QuitExitCode;
	  lpMsg->lParam = 0;
	  ThreadQueue->QuitPosted = FALSE;
	  return(FALSE);
	}

      /* Now check for normal messages. */
      Present = MsqFindMessage(ThreadQueue,
			       FALSE,
			       TRUE,
			       hWnd,
			       wMsgFilterMin,
			       wMsgFilterMax,
			       &Message);
      if (Present)
	{
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
	}

      /* Check for hardware events. */
      Present = MsqFindMessage(ThreadQueue,
			       TRUE,
			       TRUE,
			       hWnd,
			       wMsgFilterMin,
			       wMsgFilterMax,
			       &Message);
      if (Present)
	{
	  RtlCopyMemory(lpMsg, &Message->Msg, sizeof(MSG));
	  ExFreePool(Message);
	  return(TRUE);
	}

      /* FIXME: Check for sent messages again. */

      /* FIXME: Check for paint messages. */

      /* Nothing found so far. Wait for new messages. */
      Status = MsqWaitForNewMessages(ThreadQueue);
    }
  while (Status == STATUS_WAIT_0);
  return((BOOLEAN)(-1));
}

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserPeekMessage(LPMSG lpMsg,
		  HWND hWnd,
		  UINT wMsgFilterMin,
		  UINT wMsgFilterMax,
		  UINT wRemoveMsg)
{
  UNIMPLEMENTED;
    
  return 0;
}

BOOL STDCALL
NtUserPostMessage(HWND hWnd,
		  UINT Msg,
		  WPARAM wParam,
		  LPARAM lParam)
{
  UNIMPLEMENTED;
    
  return 0;
}

BOOL STDCALL
NtUserPostThreadMessage(DWORD idThread,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

DWORD STDCALL
NtUserQuerySendMessage(DWORD Unknown0)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserSendMessageCallback(HWND hWnd,
			  UINT Msg,
			  WPARAM wParam,
			  LPARAM lParam,
			  SENDASYNCPROC lpCallBack,
			  ULONG_PTR dwData)
{
  UNIMPLEMENTED;
  
  return 0;
}

BOOL STDCALL
NtUserSendNotifyMessage(HWND hWnd,
			UINT Msg,
			WPARAM wParam,
			LPARAM lParam)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserTranslateMessage(LPMSG lpMsg,
		       DWORD Unknown1)
{
  UNIMPLEMENTED;

  return 0;
}

BOOL STDCALL
NtUserWaitMessage(VOID)
{
  UNIMPLEMENTED;

  return 0;
}

/* EOF */
