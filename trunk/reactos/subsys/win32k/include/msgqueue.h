#ifndef _WIN32K_MSGQUEUE_H
#define _WIN32K_MSGQUEUE_H

#include <internal/ex.h>
#include <windows.h>
#include "caret.h"
#include "hook.h"

#define MSQ_HUNG        5000

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  BOOLEAN FreeLParam;
  MSG Msg;
} USER_MESSAGE, *PUSER_MESSAGE;

struct _USER_MESSAGE_QUEUE;

typedef struct _USER_SENT_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  PKEVENT CompletionEvent;
  LRESULT* Result;
  struct _USER_MESSAGE_QUEUE* SenderQueue;
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  /* entry in the dispatching list of the sender's message queue */
  LIST_ENTRY DispatchingListEntry;
} USER_SENT_MESSAGE, *PUSER_SENT_MESSAGE;

typedef struct _USER_SENT_MESSAGE_NOTIFY
{
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  LRESULT Result;
  HWND hWnd;
  UINT Msg;
  LIST_ENTRY ListEntry;
} USER_SENT_MESSAGE_NOTIFY, *PUSER_SENT_MESSAGE_NOTIFY;

typedef struct _USER_MESSAGE_QUEUE
{
  /* Reference counter, only access this variable with interlocked functions! */
  LONG References;
  
  /* Owner of the message queue */
  struct _ETHREAD *Thread;
  /* Queue of messages sent to the queue. */
  LIST_ENTRY SentMessagesListHead;
  /* Queue of messages posted to the queue. */
  LIST_ENTRY PostedMessagesListHead;
  /* Queue of sent-message notifies for the queue. */
  LIST_ENTRY NotifyMessagesListHead;
  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* Lock for the hardware message list. */
  KMUTEX HardwareLock;
  /* Lock for the queue. */
  FAST_MUTEX Lock;
  /* Pointer to the current WM_MOUSEMOVE message */
  PUSER_MESSAGE MouseMoveMsg;
  /* True if a WM_QUIT message is pending. */
  BOOLEAN QuitPosted;
  /* The quit exit code. */
  ULONG QuitExitCode;
  /* Set if there are new messages specified by WakeMask in any of the queues. */
  PKEVENT NewMessages;
  /* Handle for the above event (in the context of the process owning the queue). */
  HANDLE NewMessagesHandle;
  /* Last time PeekMessage() was called. */
  ULONG LastMsgRead;
  /* Current window with focus (ie. receives keyboard input) for this queue. */
  HWND FocusWindow;
  /* True if a window needs painting. */
  BOOLEAN PaintPosted;
  /* Count of paints pending. */
  ULONG PaintCount;
  /* Current active window for this queue. */
  HWND ActiveWindow;
  /* Current capture window for this queue. */
  HWND CaptureWindow;
  /* Current move/size window for this queue */
  HWND MoveSize;
  /* Current menu owner window for this queue */
  HWND MenuOwner;
  /* Identifes the menu state */
  BYTE MenuState;
  /* Caret information for this queue */
  PTHRDCARETINFO CaretInfo;
  
  /* Window hooks */
  PHOOKTABLE Hooks;

  /* queue state tracking */
  WORD WakeMask;
  WORD QueueBits;
  WORD ChangedBits;
  
  /* extra message information */
  LPARAM ExtraInfo;

  /* messages that are currently dispatched by other threads */
  LIST_ENTRY DispatchingMessagesHead;
  /* messages that are currently dispatched by this message queue, required for cleanup */
  LIST_ENTRY LocalDispatchingMessagesHead;
  
  /* Desktop that the message queue is attached to */
  struct _DESKTOP_OBJECT *Desktop;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

BOOL FASTCALL
MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
               UINT uTimeout, BOOL Block, ULONG_PTR *uResult);
PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg, BOOLEAN FreeLParam);
VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       MSG* Msg, BOOLEAN FreeLParam, DWORD MessageBits);
VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode);
BOOLEAN STDCALL
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message);
BOOLEAN FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread);
VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqGetHardwareMessageQueue(VOID);
NTSTATUS FASTCALL
MsqWaitForNewMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqInitializeImpl(VOID);
BOOLEAN FASTCALL
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage);
VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
LRESULT FASTCALL
IntSendMessage(HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam);
LRESULT FASTCALL
IntPostOrSendMessage(HWND hWnd,
		     UINT Msg,
		     WPARAM wParam,
		     LPARAM lParam);
LRESULT FASTCALL
IntSendMessageTimeout(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult);
LRESULT FASTCALL
IntDispatchMessage(MSG* Msg);
BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg, HKL dwhkl);

VOID FASTCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg);
BOOL FASTCALL
MsqIsDblClk(LPMSG Msg, BOOL Remove);
HWND FASTCALL
MsqSetStateWindow(PUSER_MESSAGE_QUEUE MessageQueue, ULONG Type, HWND hWnd);

inline BOOL MsqIsSignaled( PUSER_MESSAGE_QUEUE queue );
inline VOID MsqSetQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );
inline VOID MsqClearQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );
BOOL STDCALL IntInitMessagePumpHook();
BOOL STDCALL IntUninitMessagePumpHook();
#define MAKE_LONG(x, y) ((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF))

PHOOKTABLE FASTCALL MsqGetHooks(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL MsqSetHooks(PUSER_MESSAGE_QUEUE Queue, PHOOKTABLE Hooks);

LPARAM FASTCALL MsqSetMessageExtraInfo(LPARAM lParam);
LPARAM FASTCALL MsqGetMessageExtraInfo(VOID);
VOID STDCALL MsqRemoveWindowMessagesFromQueue(PVOID pWindow); /* F*(&$ headers, will be gone in the rewrite! */

#define IntLockMessageQueue(MsgQueue) \
  ExAcquireFastMutex(&(MsgQueue)->Lock)

#define IntUnLockMessageQueue(MsgQueue) \
  ExReleaseFastMutex(&(MsgQueue)->Lock)

#define IntLockHardwareMessageQueue(MsgQueue) \
  KeWaitForMutexObject(&(MsgQueue)->HardwareLock, UserRequest, KernelMode, FALSE, NULL)

#define IntUnLockHardwareMessageQueue(MsgQueue) \
  KeReleaseMutex(&(MsgQueue)->HardwareLock, FALSE)

#define IntReferenceMessageQueue(MsgQueue) \
  InterlockedIncrement(&(MsgQueue)->References)

#define IntDereferenceMessageQueue(MsgQueue) \
  do { \
    if(InterlockedDecrement(&(MsgQueue)->References) == 0) \
    { \
      DPRINT("Free message queue 0x%x\n", (MsgQueue)); \
      if ((MsgQueue)->NewMessagesHandle != NULL) \
        ZwClose((MsgQueue)->NewMessagesHandle); \
      ExFreePool((MsgQueue)); \
    } \
  } while(0)

#define IS_BTN_MESSAGE(message,code) \
  ((message) == WM_LBUTTON##code || \
   (message) == WM_MBUTTON##code || \
   (message) == WM_RBUTTON##code || \
   (message) == WM_XBUTTON##code || \
   (message) == WM_NCLBUTTON##code || \
   (message) == WM_NCMBUTTON##code || \
   (message) == WM_NCRBUTTON##code || \
   (message) == WM_NCXBUTTON##code )

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask);

BOOL FASTCALL
IntMsqClearWakeMask(VOID);

#endif /* _WIN32K_MSGQUEUE_H */

/* EOF */
