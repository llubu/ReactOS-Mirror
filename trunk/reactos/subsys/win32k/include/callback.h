#ifndef _WIN32K_CALLBACK_H
#define _WIN32K_CALLBACK_H

LRESULT STDCALL
IntCallWindowProc(WNDPROC Proc,
                  BOOLEAN IsAnsiProc,
                  HWND Wnd,
                  UINT Message,
                  WPARAM wParam,
                  LPARAM lParam,
                  INT lParamBufferSize);

VOID STDCALL
IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
			    HWND hWnd,
			    UINT Msg,
			    ULONG_PTR CompletionCallbackContext,
			    LRESULT Result);


HMENU STDCALL
IntLoadSysMenuTemplate();

BOOL STDCALL
IntLoadDefaultCursors(BOOL SetDefault);

LRESULT STDCALL
IntCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

#endif /* _WIN32K_CALLBACK_H */
