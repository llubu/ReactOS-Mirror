#ifndef _WIN32K_FOCUS_H
#define _WIN32K_FOCUS_H

/*
 * These functions take the window handles from current message queue.
 */
HWND FASTCALL
IntGetCaptureWindow();
HWND FASTCALL
IntGetFocusWindow();

/*
 * These functions take the window handles from current thread queue.
 */
HWND FASTCALL
IntGetThreadFocusWindow();

BOOL FASTCALL
IntSetForegroundWindow(PWINDOW_OBJECT Window);

#endif /* _WIN32K_FOCUS_H */
