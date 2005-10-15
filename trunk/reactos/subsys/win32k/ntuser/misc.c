/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsys/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* registered Logon process */
PW32PROCESS LogonProcess = NULL;

VOID W32kRegisterPrimitiveMessageQueue(VOID)
{
   extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
   if( !pmPrimitiveMessageQueue )
   {
      PW32THREAD pThread;
      pThread = PsGetWin32Thread();
      if( pThread && pThread->MessageQueue )
      {
         pmPrimitiveMessageQueue = pThread->MessageQueue;
         IntReferenceMessageQueue(pmPrimitiveMessageQueue);
         DPRINT( "Installed primitive input queue.\n" );
      }
   }
   else
   {
      DPRINT1( "Alert! Someone is trying to steal the primitive queue.\n" );
   }
}

VOID W32kUnregisterPrimitiveMessageQueue(VOID)
{
   extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
   IntDereferenceMessageQueue(pmPrimitiveMessageQueue);
   pmPrimitiveMessageQueue = NULL;
}

PUSER_MESSAGE_QUEUE W32kGetPrimitiveMessageQueue()
{
   extern PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue;
   return pmPrimitiveMessageQueue;
}

BOOL FASTCALL
co_IntRegisterLogonProcess(HANDLE ProcessId, BOOL Register)
{
   PEPROCESS Process;
   NTSTATUS Status;
   CSR_API_MESSAGE Request;

   Status = PsLookupProcessByProcessId(ProcessId,
                                       &Process);
   if (!NT_SUCCESS(Status))
   {
      SetLastWin32Error(RtlNtStatusToDosError(Status));
      return FALSE;
   }

   if (Register)
   {
      /* Register the logon process */
      if (LogonProcess != NULL)
      {
         ObDereferenceObject(Process);
         return FALSE;
      }

      LogonProcess = (PW32PROCESS)Process->Win32Process;
   }
   else
   {
      /* Deregister the logon process */
      if (LogonProcess != (PW32PROCESS)Process->Win32Process)
      {
         ObDereferenceObject(Process);
         return FALSE;
      }

      LogonProcess = NULL;
   }

   ObDereferenceObject(Process);

   Request.Type = MAKE_CSR_API(REGISTER_LOGON_PROCESS, CSR_GUI);
   Request.Data.RegisterLogonProcessRequest.ProcessId = ProcessId;
   Request.Data.RegisterLogonProcessRequest.Register = Register;

   Status = co_CsrNotify(&Request);
   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Failed to register logon process with CSRSS\n");
      return FALSE;
   }

   return TRUE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserCallNoParam(DWORD Routine)
{
   DWORD Result = 0;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallNoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case NOPARAM_ROUTINE_REGISTER_PRIMITIVE:
         W32kRegisterPrimitiveMessageQueue();
         Result = (DWORD)TRUE;
         break;

      case NOPARAM_ROUTINE_DESTROY_CARET:
         Result = (DWORD)co_IntDestroyCaret(PsGetCurrentThread()->Tcb.Win32Thread);
         break;

      case NOPARAM_ROUTINE_INIT_MESSAGE_PUMP:
         Result = (DWORD)IntInitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP:
         Result = (DWORD)IntUninitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO:
         Result = (DWORD)MsqGetMessageExtraInfo();
         break;

      case NOPARAM_ROUTINE_ANYPOPUP:
         Result = (DWORD)IntAnyPopup();
         break;

      case NOPARAM_ROUTINE_CSRSS_INITIALIZED:
         Result = (DWORD)CsrInit();
         break;

      case NOPARAM_ROUTINE_MSQCLEARWAKEMASK:
         RETURN( (DWORD)IntMsqClearWakeMask());

      default:
         DPRINT1("Calling invalid routine number 0x%x in NtUserCallNoParam\n", Routine);
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         break;
   }
   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserCallNoParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallOneParam(
   DWORD Param,
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallOneParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case ONEPARAM_ROUTINE_GETMENU:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)Window->IDMenu;

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            Window = UserGetWindowObject((HWND)Param);
            if(!Window)
            {
               RETURN( FALSE);
            }
            Result = Window->Unicode;
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_WINDOWFROMDC:
         RETURN( (DWORD)IntWindowFromDC((HDC)Param));

      case ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            Window = UserGetWindowObject((HWND)Param);
            if(!Window)
            {
               RETURN( FALSE);
            }

            Result = Window->ContextHelpId;

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
         {
            PWINSTATION_OBJECT WinSta;
            NTSTATUS Status;
            DWORD Result;

            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinSta);
            if (!NT_SUCCESS(Status))
               RETURN( (DWORD)FALSE);

            /* FIXME
            Result = (DWORD)IntSwapMouseButton(WinStaObject, (BOOL)Param); */
            Result = 0;

            ObDereferenceObject(WinSta);
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SWITCHCARETSHOWING:
         RETURN( (DWORD)IntSwitchCaretShowing((PVOID)Param));

      case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
         RETURN( (DWORD)IntSetCaretBlinkTime((UINT)Param));

      case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
         RETURN( (DWORD)IntEnumClipboardFormats((UINT)Param));

      case ONEPARAM_ROUTINE_GETWINDOWINSTANCE:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)Window->Instance;
            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO:
         RETURN( (DWORD)MsqSetMessageExtraInfo((LPARAM)Param));

      case ONEPARAM_ROUTINE_GETCURSORPOSITION:
         {
            PWINSTATION_OBJECT WinSta;
            NTSTATUS Status;
            POINT Pos;

            if(!Param)
               RETURN( (DWORD)FALSE);
            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinSta);
            if (!NT_SUCCESS(Status))
               RETURN( (DWORD)FALSE);

            /* FIXME - check if process has WINSTA_READATTRIBUTES */
            IntGetCursorLocation(WinSta, &Pos);

            Status = MmCopyToCaller((PPOINT)Param, &Pos, sizeof(POINT));
            if(!NT_SUCCESS(Status))
            {
               ObDereferenceObject(WinSta);
               SetLastNtError(Status);
               RETURN( FALSE);
            }

            ObDereferenceObject(WinSta);

            RETURN( (DWORD)TRUE);
         }

      case ONEPARAM_ROUTINE_ISWINDOWINDESTROY:
         {
            PWINDOW_OBJECT Window;
            DWORD Result;

            if(!(Window = UserGetWindowObject((HWND)Param)))
            {
               RETURN( FALSE);
            }

            Result = (DWORD)IntIsWindowInDestroy(Window);

            RETURN( Result);
         }

      case ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING:
         {
            BOOL Enable;
            PW32PROCESS Process = PsGetWin32Process();

            if(Process != NULL)
            {
               Enable = (BOOL)(Param != 0);

               if(Enable)
               {
                  Process->Flags &= ~W32PF_NOWINDOWGHOSTING;
               }
               else
               {
                  Process->Flags |= W32PF_NOWINDOWGHOSTING;
               }

               RETURN( TRUE);
            }

            RETURN( FALSE);
         }

      case ONEPARAM_ROUTINE_MSQSETWAKEMASK:
         RETURN( (DWORD)IntMsqSetWakeMask(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDTYPE:
         RETURN( UserGetKeyboardType(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT:
         RETURN( (DWORD)UserGetKeyboardLayout(Param));
   }
   DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam(), Param=0x%x\n",
           Routine, Param);
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserCallOneParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallTwoParam(
   DWORD Param1,
   DWORD Param2,
   DWORD Routine)
{
   NTSTATUS Status;
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCallTwoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case TWOPARAM_ROUTINE_SETDCPENCOLOR:
         {
            RETURN( (DWORD)IntSetDCColor((HDC)Param1, OBJ_PEN, (COLORREF)Param2));
         }
      case TWOPARAM_ROUTINE_SETDCBRUSHCOLOR:
         {
            RETURN( (DWORD)IntSetDCColor((HDC)Param1, OBJ_BRUSH, (COLORREF)Param2));
         }
      case TWOPARAM_ROUTINE_GETDCCOLOR:
         {
            RETURN( (DWORD)IntGetDCColor((HDC)Param1, (ULONG)Param2));
         }
      case TWOPARAM_ROUTINE_GETWINDOWRGNBOX:
         {
            DWORD Ret;
            RECT rcRect;
            PWINDOW_OBJECT Window = UserGetWindowObject((HWND)Param1);
            if (!Window) RETURN(ERROR);
            
            Ret = (DWORD)IntGetWindowRgnBox(Window, &rcRect);
            Status = MmCopyToCaller((PVOID)Param2, &rcRect, sizeof(RECT));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( ERROR);
            }
            RETURN( Ret);
         }
      case TWOPARAM_ROUTINE_GETWINDOWRGN:
         {
            PWINDOW_OBJECT Window = UserGetWindowObject((HWND)Param1);
            if (!Window) RETURN(ERROR);

            RETURN( (DWORD)IntGetWindowRgn(Window, (HRGN)Param2));
         }
      case TWOPARAM_ROUTINE_SETMENUBARHEIGHT:
         {
            DWORD Ret;
            PMENU_OBJECT MenuObject = IntGetMenuObject((HMENU)Param1);
            if(!MenuObject)
               RETURN( 0);

            if(Param2 > 0)
            {
               Ret = (MenuObject->MenuInfo.Height == (int)Param2);
               MenuObject->MenuInfo.Height = (int)Param2;
            }
            else
               Ret = (DWORD)MenuObject->MenuInfo.Height;
            IntReleaseMenuObject(MenuObject);
            RETURN( Ret);
         }
      case TWOPARAM_ROUTINE_SETMENUITEMRECT:
         {
            BOOL Ret;
            SETMENUITEMRECT smir;
            PMENU_OBJECT MenuObject = IntGetMenuObject((HMENU)Param1);
            if(!MenuObject)
               RETURN( 0);

            if(!NT_SUCCESS(MmCopyFromCaller(&smir, (PVOID)Param2, sizeof(SETMENUITEMRECT))))
            {
               IntReleaseMenuObject(MenuObject);
               RETURN( 0);
            }

            Ret = IntSetMenuItemRect(MenuObject, smir.uItem, smir.fByPosition, &smir.rcRect);

            IntReleaseMenuObject(MenuObject);
            RETURN( (DWORD)Ret);
         }

      case TWOPARAM_ROUTINE_SETGUITHRDHANDLE:
         {
            PUSER_MESSAGE_QUEUE MsgQueue = PsGetCurrentThread()->Tcb.Win32Thread->MessageQueue;

            ASSERT(MsgQueue);
            RETURN( (DWORD)MsqSetStateWindow(MsgQueue, (ULONG)Param1, (HWND)Param2));
         }

      case TWOPARAM_ROUTINE_ENABLEWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_UNKNOWN:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
      {
         PWINDOW_OBJECT Window = UserGetWindowObject((HWND)Param1);
         if (!Window) RETURN(0);
         
         RETURN( (DWORD)IntShowOwnedPopups(Window, (BOOL) Param2));
      }

      case TWOPARAM_ROUTINE_ROS_SHOWWINDOW:
         {
#define WIN_NEEDS_SHOW_OWNEDPOPUP (0x00000040)
            PWINDOW_OBJECT Window;
            DPRINT1("ROS_SHOWWINDOW\n");
            
            if (!(Window = UserGetWindowObject((HWND)Param1)))
            {
               RETURN( FALSE);
            }
            
            if (Param2)
            {
               if (!(Window->Flags & WIN_NEEDS_SHOW_OWNEDPOPUP))
               {
                  RETURN( TRUE);
               }
               Window->Flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
            }
            else
               Window->Flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;

            DPRINT1("ROS_SHOWWINDOW ---> 0x%x\n",Window->Flags);
            RETURN( TRUE);
         }
      case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
         UNIMPLEMENTED
         RETURN( 0);

      case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:
         
         if(!(Window = UserGetWindowObject((HWND)Param1)))
         {
            RETURN( (DWORD)FALSE);
         }

         Window->ContextHelpId = Param2;

         RETURN( (DWORD)TRUE);

      case TWOPARAM_ROUTINE_SETCARETPOS:
         RETURN( (DWORD)co_IntSetCaretPos((int)Param1, (int)Param2));

      case TWOPARAM_ROUTINE_GETWINDOWINFO:
         {
            WINDOWINFO wi;
            DWORD Ret;

            if(!(Window = UserGetWindowObject((HWND)Param1)))
            {
               RETURN( FALSE);
            }

#if 0
            /*
             * According to WINE, Windows' doesn't check the cbSize field
             */

            Status = MmCopyFromCaller(&wi.cbSize, (PVOID)Param2, sizeof(wi.cbSize));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( FALSE);
            }

            if(wi.cbSize != sizeof(WINDOWINFO))
            {
               SetLastWin32Error(ERROR_INVALID_PARAMETER);
               RETURN( FALSE);
            }
#endif

            if((Ret = (DWORD)IntGetWindowInfo(Window, &wi)))
            {
               Status = MmCopyToCaller((PVOID)Param2, &wi, sizeof(WINDOWINFO));
               if(!NT_SUCCESS(Status))
               {
                  SetLastNtError(Status);
                  RETURN( FALSE);
               }
            }

            RETURN( Ret);
         }

      case TWOPARAM_ROUTINE_REGISTERLOGONPROC:
         RETURN( (DWORD)co_IntRegisterLogonProcess((HANDLE)Param1, (BOOL)Param2));

      case TWOPARAM_ROUTINE_SETSYSCOLORS:
         {
            DWORD Ret = 0;
            PVOID Buffer;
            struct
            {
               INT *Elements;
               COLORREF *Colors;
            }
            ChangeSysColors;

            /* FIXME - we should make use of SEH here... */

            Status = MmCopyFromCaller(&ChangeSysColors, (PVOID)Param1, sizeof(ChangeSysColors));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               RETURN( 0);
            }

            Buffer = ExAllocatePool(PagedPool, (Param2 * sizeof(INT)) + (Param2 * sizeof(COLORREF)));
            if(Buffer != NULL)
            {
               INT *Elements = (INT*)Buffer;
               COLORREF *Colors = (COLORREF*)Buffer + Param2;

               Status = MmCopyFromCaller(Elements, ChangeSysColors.Elements, Param2 * sizeof(INT));
               if(NT_SUCCESS(Status))
               {
                  Status = MmCopyFromCaller(Colors, ChangeSysColors.Colors, Param2 * sizeof(COLORREF));
                  if(NT_SUCCESS(Status))
                  {
                     Ret = (DWORD)IntSetSysColors((UINT)Param2, Elements, Colors);
                  }
                  else
                     SetLastNtError(Status);
               }
               else
                  SetLastNtError(Status);

               ExFreePool(Buffer);
            }


            RETURN( Ret);
         }

      case TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES:
      case TWOPARAM_ROUTINE_GETSYSCOLORPENS:
      case TWOPARAM_ROUTINE_GETSYSCOLORS:
         {
            DWORD Ret = 0;
            union
            {
               PVOID Pointer;
               HBRUSH *Brushes;
               HPEN *Pens;
               COLORREF *Colors;
            } Buffer;

            /* FIXME - we should make use of SEH here... */

            Buffer.Pointer = ExAllocatePool(PagedPool, Param2 * sizeof(HANDLE));
            if(Buffer.Pointer != NULL)
            {
               switch(Routine)
               {
                  case TWOPARAM_ROUTINE_GETSYSCOLORBRUSHES:
                     Ret = (DWORD)IntGetSysColorBrushes(Buffer.Brushes, (UINT)Param2);
                     break;
                  case TWOPARAM_ROUTINE_GETSYSCOLORPENS:
                     Ret = (DWORD)IntGetSysColorPens(Buffer.Pens, (UINT)Param2);
                     break;
                  case TWOPARAM_ROUTINE_GETSYSCOLORS:
                     Ret = (DWORD)IntGetSysColors(Buffer.Colors, (UINT)Param2);
                     break;
                  default:
                     Ret = 0;
                     break;
               }

               if(Ret > 0)
               {
                  Status = MmCopyToCaller((PVOID)Param1, Buffer.Pointer, Param2 * sizeof(HANDLE));
                  if(!NT_SUCCESS(Status))
                  {
                     SetLastNtError(Status);
                     Ret = 0;
                  }
               }

               ExFreePool(Buffer.Pointer);
            }
            RETURN( Ret);
         }

   }
   DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam(), Param1=0x%x Parm2=0x%x\n",
           Routine, Param1, Param2);
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserCallTwoParam, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserCallHwndLock(
   HWND hWnd,
   DWORD Routine)
{
   BOOL Ret = 0;
   PWINDOW_OBJECT Window;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserCallHwndLock\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   /* FIXME: Routine can be 0x53 - 0x5E */
   switch (Routine)
   {
      case HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS:
         co_WinPosArrangeIconicWindows(Window);
         break;

      case HWNDLOCK_ROUTINE_DRAWMENUBAR:
         {
            PMENU_OBJECT Menu;
            DPRINT("HWNDLOCK_ROUTINE_DRAWMENUBAR\n");
            Ret = FALSE;
            if (!((Window->Style & (WS_CHILD | WS_POPUP)) != WS_CHILD))
               break;
            
            if(!(Menu = UserGetMenuObject((HMENU) Window->IDMenu)))
               break;
            
            Menu->MenuInfo.WndOwner = hWnd;
            Menu->MenuInfo.Height = 0;

            co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
            
            Ret = TRUE;
            break;
         }

      case HWNDLOCK_ROUTINE_REDRAWFRAME:
         /* FIXME */
         break;

      case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW:
         Ret = co_IntSetForegroundWindow(Window);
         break;

      case HWNDLOCK_ROUTINE_UPDATEWINDOW:
         /* FIXME */
         break;
   }

   UserDerefObjectCo(Window);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserCallHwndLock, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
HWND
STDCALL
NtUserCallHwndOpt(
   HWND Param,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWNDOPT_ROUTINE_SETPROGMANWINDOW:
         /*
          * FIXME 
          * Nothing too hard...validate the hWnd and save it in the Desktop Info
          */
         DPRINT1("HWNDOPT_ROUTINE_SETPROGMANWINDOW UNIMPLEMENTED\n");
         break;

      case HWNDOPT_ROUTINE_SETTASKMANWINDOW:
         /*
          * FIXME 
          * Nothing too hard...validate the hWnd and save it in the Desktop Info
          */
         DPRINT1("HWNDOPT_ROUTINE_SETTASKMANWINDOW UNIMPLEMENTED\n");
         break;
   }

   return Param;
}

/*
 * @unimplemented
 */
DWORD STDCALL
NtUserGetThreadState(
   DWORD Routine)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetThreadState\n");
   UserEnterShared();

   switch (Routine)
   {
      case 0:
         RETURN( (DWORD)IntGetThreadFocusWindow());
   }
   RETURN( 0);

CLEANUP:
   DPRINT("Leave NtUserGetThreadState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

VOID FASTCALL
IntGetFontMetricSetting(LPWSTR lpValueName, PLOGFONTW font)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   NTSTATUS Status;
   static LOGFONTW DefaultFont = {
                                    11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                    0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                    L"Bitstream Vera Sans"
                                 };

   RtlZeroMemory(&QueryTable, sizeof(QueryTable));

   QueryTable[0].Name = lpValueName;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].EntryContext = font;

   Status = RtlQueryRegistryValues(
               RTL_REGISTRY_USER,
               L"Control Panel\\Desktop\\WindowMetrics",
               QueryTable,
               NULL,
               NULL);

   if (!NT_SUCCESS(Status))
   {
      RtlCopyMemory(font, &DefaultFont, sizeof(LOGFONTW));
   }
}

ULONG FASTCALL
IntSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;

   static BOOL bInitialized = FALSE;
   static LOGFONTW IconFont;
   static NONCLIENTMETRICSW pMetrics;
   static BOOL GradientCaptions = TRUE;
   static UINT FocusBorderHeight = 1;
   static UINT FocusBorderWidth = 1;

   if (!bInitialized)
   {
      RtlZeroMemory(&IconFont, sizeof(LOGFONTW));
      RtlZeroMemory(&pMetrics, sizeof(NONCLIENTMETRICSW));

      IntGetFontMetricSetting(L"CaptionFont", &pMetrics.lfCaptionFont);
      IntGetFontMetricSetting(L"SmCaptionFont", &pMetrics.lfSmCaptionFont);
      IntGetFontMetricSetting(L"MenuFont", &pMetrics.lfMenuFont);
      IntGetFontMetricSetting(L"StatusFont", &pMetrics.lfStatusFont);
      IntGetFontMetricSetting(L"MessageFont", &pMetrics.lfMessageFont);
      IntGetFontMetricSetting(L"IconFont", &IconFont);

      pMetrics.iBorderWidth = 1;
      pMetrics.iScrollWidth = UserGetSystemMetrics(SM_CXVSCROLL);
      pMetrics.iScrollHeight = UserGetSystemMetrics(SM_CYHSCROLL);
      pMetrics.iCaptionWidth = UserGetSystemMetrics(SM_CXSIZE);
      pMetrics.iCaptionHeight = UserGetSystemMetrics(SM_CYSIZE);
      pMetrics.iSmCaptionWidth = UserGetSystemMetrics(SM_CXSMSIZE);
      pMetrics.iSmCaptionHeight = UserGetSystemMetrics(SM_CYSMSIZE);
      pMetrics.iMenuWidth = UserGetSystemMetrics(SM_CXMENUSIZE);
      pMetrics.iMenuHeight = UserGetSystemMetrics(SM_CYMENUSIZE);
      pMetrics.cbSize = sizeof(NONCLIENTMETRICSW);

      bInitialized = TRUE;
   }

   switch(uiAction)
   {
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETDESKWALLPAPER:
      case SPI_GETDESKWALLPAPER:
         {
            PSYSTEM_CURSORINFO CurInfo;

            Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                                    KernelMode,
                                                    0,
                                                    &WinStaObject);
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return (DWORD)FALSE;
            }

            switch(uiAction)
            {
               case SPI_SETDOUBLECLKWIDTH:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickWidth = uiParam;
                  break;
               case SPI_SETDOUBLECLKHEIGHT:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum value? */
                  CurInfo->DblClickHeight = uiParam;
                  break;
               case SPI_SETDOUBLECLICKTIME:
                  CurInfo = IntGetSysCursorInfo(WinStaObject);
                  /* FIXME limit the maximum time to 1000 ms? */
                  CurInfo->DblClickSpeed = uiParam;
                  break;
               case SPI_SETDESKWALLPAPER:
                  {
                     /* This function expects different parameters than the user mode version!

                        We let the user mode code load the bitmap, it passed the handle to
                        the bitmap. We'll change it's ownership to system and replace it with
                        the current wallpaper bitmap */
                     HBITMAP hOldBitmap, hNewBitmap;
                     ASSERT(pvParam);

                     hNewBitmap = *(HBITMAP*)pvParam;
                     if(hNewBitmap != NULL)
                     {
                        BITMAPOBJ *bmp;
                        /* try to get the size of the wallpaper */
                        if(!(bmp = BITMAPOBJ_LockBitmap(hNewBitmap)))
                        {
                           ObDereferenceObject(WinStaObject);
                           return FALSE;
                        }
                        WinStaObject->cxWallpaper = bmp->SurfObj.sizlBitmap.cx;
                        WinStaObject->cyWallpaper = bmp->SurfObj.sizlBitmap.cy;

                        BITMAPOBJ_UnlockBitmap(bmp);

                        /* change the bitmap's ownership */
                        GDIOBJ_SetOwnership(hNewBitmap, NULL);
                     }
                     hOldBitmap = (HBITMAP)InterlockedExchange((LONG*)&WinStaObject->hbmWallpaper, (LONG)hNewBitmap);
                     if(hOldBitmap != NULL)
                     {
                        /* delete the old wallpaper */
                        NtGdiDeleteObject(hOldBitmap);
                     }
                     break;
                  }
               case SPI_GETDESKWALLPAPER:
                  /* This function expects different parameters than the user mode version!

                     We basically return the current wallpaper handle - if any. The user
                     mode version should load the string from the registry and return it
                     without calling this function */
                  ASSERT(pvParam);
                  *(HBITMAP*)pvParam = (HBITMAP)WinStaObject->hbmWallpaper;
                  break;
            }

            /* FIXME save the value to the registry */

            ObDereferenceObject(WinStaObject);
            return TRUE;
         }
      case SPI_SETWORKAREA:
         {
            RECT *rc;
            PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            rc = (RECT*)pvParam;
            Desktop->WorkArea = *rc;

            return TRUE;
         }
      case SPI_GETWORKAREA:
         {
            PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;

            if(!Desktop)
            {
               /* FIXME - Set last error */
               return FALSE;
            }

            ASSERT(pvParam);
            IntGetDesktopWorkArea(Desktop, (PRECT)pvParam);

            return TRUE;
         }
      case SPI_SETGRADIENTCAPTIONS:
         {
            GradientCaptions = (pvParam != NULL);
            /* FIXME - should be checked if the color depth is higher than 8bpp? */
            return TRUE;
         }
      case SPI_GETGRADIENTCAPTIONS:
         {
            HDC hDC;
            BOOL Ret = GradientCaptions;

            hDC = IntGetScreenDC();
            if(hDC)
            {
               Ret = (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8) && Ret;

               ASSERT(pvParam);
               *((PBOOL)pvParam) = Ret;
               return TRUE;
            }
            return FALSE;
         }
      case SPI_SETFONTSMOOTHING:
         {
            IntEnableFontRendering(uiParam != 0);
            return TRUE;
         }
      case SPI_GETFONTSMOOTHING:
         {
            ASSERT(pvParam);
            *((BOOL*)pvParam) = IntIsFontRenderingEnabled();
            return TRUE;
         }
      case SPI_GETICONTITLELOGFONT:
         {
            ASSERT(pvParam);
            *((LOGFONTW*)pvParam) = IconFont;
            return TRUE;
         }
      case SPI_GETNONCLIENTMETRICS:
         {
            ASSERT(pvParam);
            *((NONCLIENTMETRICSW*)pvParam) = pMetrics;
            return TRUE;
         }
      case SPI_GETFOCUSBORDERHEIGHT:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderHeight;
            return TRUE;
         }
      case SPI_GETFOCUSBORDERWIDTH:
         {
            ASSERT(pvParam);
            *((UINT*)pvParam) = FocusBorderWidth;
            return TRUE;
         }
      case SPI_SETFOCUSBORDERHEIGHT:
         {
            FocusBorderHeight = (UINT)pvParam;
            return TRUE;
         }
      case SPI_SETFOCUSBORDERWIDTH:
         {
            FocusBorderWidth = (UINT)pvParam;
            return TRUE;
         }

      default:
         {
            DPRINT1("SystemParametersInfo: Unsupported Action 0x%x (uiParam: 0x%x, pvParam: 0x%x, fWinIni: 0x%x)\n",
                    uiAction, uiParam, pvParam, fWinIni);
            return FALSE;
         }
   }
   return FALSE;
}

/*
 * @implemented
 */
BOOL FASTCALL
UserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   NTSTATUS Status;

   switch(uiAction)
   {
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETGRADIENTCAPTIONS:
      case SPI_SETFONTSMOOTHING:
      case SPI_SETFOCUSBORDERHEIGHT:
      case SPI_SETFOCUSBORDERWIDTH:
         {
            return (DWORD)IntSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
         }
      case SPI_SETWORKAREA:
         {
            RECT rc;
            Status = MmCopyFromCaller(&rc, (PRECT)pvParam, sizeof(RECT));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( (DWORD)IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni));
         }
      case SPI_GETWORKAREA:
         {
            RECT rc;

            if(!IntSystemParametersInfo(uiAction, uiParam, &rc, fWinIni))
            {
               return( FALSE);
            }

            Status = MmCopyToCaller((PRECT)pvParam, &rc, sizeof(RECT));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( TRUE);
         }
      case SPI_GETFONTSMOOTHING:
      case SPI_GETGRADIENTCAPTIONS:
      case SPI_GETFOCUSBORDERHEIGHT:
      case SPI_GETFOCUSBORDERWIDTH:
         {
            BOOL Ret;

            if(!IntSystemParametersInfo(uiAction, uiParam, &Ret, fWinIni))
            {
               return( FALSE);
            }

            Status = MmCopyToCaller(pvParam, &Ret, sizeof(BOOL));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( TRUE);
         }
      case SPI_SETDESKWALLPAPER:
         {
            /* !!! As opposed to the user mode version this version accepts a handle
                   to the bitmap! */
            HBITMAP hbmWallpaper;

            Status = MmCopyFromCaller(&hbmWallpaper, pvParam, sizeof(HBITMAP));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( IntSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hbmWallpaper, fWinIni));
         }
      case SPI_GETDESKWALLPAPER:
         {
            /* !!! As opposed to the user mode version this version returns a handle
                   to the bitmap! */
            HBITMAP hbmWallpaper;
            BOOL Ret;

            Ret = IntSystemParametersInfo(SPI_GETDESKWALLPAPER, 0, &hbmWallpaper, fWinIni);

            Status = MmCopyToCaller(pvParam, &hbmWallpaper, sizeof(HBITMAP));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( Ret);
         }
      case SPI_GETICONTITLELOGFONT:
         {
            LOGFONTW IconFont;

            if(!IntSystemParametersInfo(uiAction, uiParam, &IconFont, fWinIni))
            {
               return( FALSE);
            }

            Status = MmCopyToCaller(pvParam, &IconFont, sizeof(LOGFONTW));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( TRUE);
         }
      case SPI_GETNONCLIENTMETRICS:
         {
            NONCLIENTMETRICSW metrics;

            Status = MmCopyFromCaller(&metrics.cbSize, pvParam, sizeof(UINT));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            if(metrics.cbSize != sizeof(NONCLIENTMETRICSW))
            {
               SetLastWin32Error(ERROR_INVALID_PARAMETER);
               return( FALSE);
            }

            if(!IntSystemParametersInfo(uiAction, uiParam, &metrics, fWinIni))
            {
               return( FALSE);
            }

            Status = MmCopyToCaller(pvParam, &metrics.cbSize, sizeof(NONCLIENTMETRICSW));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               return( FALSE);
            }
            return( TRUE);
         }
   }
   return( FALSE);
}




/*
 * @implemented
 */
BOOL
STDCALL
NtUserSystemParametersInfo(
   UINT uiAction,
   UINT uiParam,
   PVOID pvParam,
   UINT fWinIni)
{
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserSystemParametersInfo\n");
   UserEnterExclusive();

   RETURN( UserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni));

CLEANUP:
   DPRINT("Leave NtUserSystemParametersInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



UINT
STDCALL
NtUserGetDoubleClickTime(VOID)
{
   UINT Result;
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject;
   PSYSTEM_CURSORINFO CurInfo;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetDoubleClickTime\n");
   UserEnterShared();

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObject);
   if (!NT_SUCCESS(Status))
      RETURN( (DWORD)FALSE);

   CurInfo = IntGetSysCursorInfo(WinStaObject);
   Result = CurInfo->DblClickSpeed;

   ObDereferenceObject(WinStaObject);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserGetDoubleClickTime, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
STDCALL
NtUserGetGUIThreadInfo(
   DWORD idThread, /* if NULL use foreground thread */
   LPGUITHREADINFO lpgui)
{
   NTSTATUS Status;
   PTHRDCARETINFO CaretInfo;
   GUITHREADINFO SafeGui;
   PDESKTOP_OBJECT Desktop;
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
      Desktop = Thread->Tcb.Win32Thread->Desktop;
   }
   else
   {
      /* get the foreground thread */
      PW32THREAD W32Thread = PsGetCurrentThread()->Tcb.Win32Thread;
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
STDCALL
NtUserGetGuiResources(
   HANDLE hProcess,
   DWORD uiFlags)
{
   PEPROCESS Process;
   PW32PROCESS W32Process;
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

   W32Process = (PW32PROCESS)Process->Win32Process;
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
            Ret = (DWORD)W32Process->GDIObjects;
            break;
         }
      case GR_USEROBJECTS:
         {
            Ret = (DWORD)W32Process->UserObjects;
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

   if(Dest->Length > 0 && Src)
   {
      Dest->MaximumLength = Dest->Length;
      Dest->Buffer = ExAllocatePoolWithTag(PagedPool, Dest->MaximumLength, TAG_STRING);
      if(!Dest->Buffer)
      {
         return STATUS_NO_MEMORY;
      }

      Status = MmCopyFromCaller(Dest->Buffer, Src, Dest->Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(Dest->Buffer);
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
         ExFreePool(Dest->Buffer);
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

BOOL STDCALL
NtUserUpdatePerUserSystemParameters(
   DWORD dwReserved,
   BOOL bEnable)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserUpdatePerUserSystemParameters\n");
   UserEnterExclusive();

   Result &= IntDesktopUpdatePerUserSettings(bEnable);
   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserUpdatePerUserSystemParameters, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
