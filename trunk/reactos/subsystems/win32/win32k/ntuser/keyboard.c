/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/keyboard.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* Directory to load key layouts from */
#define SYSTEMROOT_DIR L"\\SystemRoot\\System32\\"
/* Lock modifiers */
#define CAPITAL_BIT   0x80000000
#define NUMLOCK_BIT   0x40000000
#define MOD_BITS_MASK 0x3fffffff
#define MOD_KCTRL     0x02
/* Key States */
#define KS_DOWN_MASK     0xc0
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT    0x01
/* lParam bits */
#define LP_EXT_BIT       (1<<24)
/* From kbdxx.c -- Key changes with numlock */
#define KNUMP         0x400


BYTE gQueueKeyStateTable[256];


PKBDRVFILE KBLList = NULL; // Keyboard layout list.


/* FUNCTIONS *****************************************************************/

/* Initialization -- Right now, just zero the key state and init the lock */
NTSTATUS FASTCALL InitKeyboardImpl(VOID)
{
   RtlZeroMemory(&gQueueKeyStateTable,0x100);
   return STATUS_SUCCESS;
}

/*** Statics used by TranslateMessage ***/

/*** Shift state code was out of hand, sorry. --- arty */

static UINT DontDistinguishShifts( UINT ret )
{
   if( ret == VK_LSHIFT || ret == VK_RSHIFT )
      ret = VK_LSHIFT;
   if( ret == VK_LCONTROL || ret == VK_RCONTROL )
      ret = VK_LCONTROL;
   if( ret == VK_LMENU || ret == VK_RMENU )
      ret = VK_LMENU;
   return ret;
}

static VOID STDCALL SetKeyState(DWORD key, DWORD vk, DWORD ext, BOOL down)
{
   ASSERT(vk <= 0xff);

   /* Special handling for toggles like numpad and caps lock */
   if (vk == VK_CAPITAL || vk == VK_NUMLOCK)
   {
      if (down)
         gQueueKeyStateTable[vk] ^= KS_LOCK_BIT;
   }

   if (ext && vk == VK_LSHIFT)
      vk = VK_RSHIFT;
   if (ext && vk == VK_LCONTROL)
      vk = VK_RCONTROL;
   if (ext && vk == VK_LMENU)
      vk = VK_RMENU;

   if (down)
      gQueueKeyStateTable[vk] |= KS_DOWN_BIT;
   else
      gQueueKeyStateTable[vk] &= ~KS_DOWN_MASK;

   if (vk == VK_LSHIFT || vk == VK_RSHIFT)
   {
      if ((gQueueKeyStateTable[VK_LSHIFT] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RSHIFT] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_SHIFT] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_SHIFT] &= ~KS_DOWN_MASK;
      }
   }

   if (vk == VK_LCONTROL || vk == VK_RCONTROL)
   {
      if ((gQueueKeyStateTable[VK_LCONTROL] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RCONTROL] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_CONTROL] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_CONTROL] &= ~KS_DOWN_MASK;
      }
   }

   if (vk == VK_LMENU || vk == VK_RMENU)
   {
      if ((gQueueKeyStateTable[VK_LMENU] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RMENU] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_MENU] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_MENU] &= ~KS_DOWN_MASK;
      }
   }
}

VOID DumpKeyState( PBYTE KeyState )
{
   int i;

   DbgPrint( "KeyState { " );
   for( i = 0; i < 0x100; i++ )
   {
      if( KeyState[i] )
         DbgPrint( "%02x(%02x) ", i, KeyState[i] );
   }
   DbgPrint( "};\n" );
}

static BYTE KeysSet( PKBDTABLES pkKT, PBYTE KeyState,
                     int FakeModLeft, int FakeModRight )
{
   if( !KeyState || !pkKT )
      return 0;

   /* Search special codes first */
   if( FakeModLeft && KeyState[FakeModLeft] )
      return KeyState[FakeModLeft];
   else if( FakeModRight && KeyState[FakeModRight] )
      return KeyState[FakeModRight];

   return 0;
}

/* Search the keyboard layout modifiers table for the shift bit.  I don't
 * want to count on the shift bit not moving, because it can be specified
 * in the layout */

static DWORD FASTCALL GetShiftBit( PKBDTABLES pkKT, DWORD Vk )
{
   int i;

   for( i = 0; pkKT->pCharModifiers->pVkToBit[i].Vk; i++ )
      if( pkKT->pCharModifiers->pVkToBit[i].Vk == Vk )
         return pkKT->pCharModifiers->pVkToBit[i].ModBits;

   return 0;
}

static DWORD ModBits( PKBDTABLES pkKT, PBYTE KeyState )
{
   DWORD ModBits = 0;

   if( !KeyState )
      return 0;

   /* DumpKeyState( KeyState ); */

   if (KeysSet( pkKT, KeyState, VK_LSHIFT, VK_RSHIFT ) &
         KS_DOWN_BIT)
      ModBits |= GetShiftBit( pkKT, VK_SHIFT );

   if (KeysSet( pkKT, KeyState, VK_LCONTROL, VK_RCONTROL ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_CONTROL );

   if (KeysSet( pkKT, KeyState, VK_LMENU, VK_RMENU ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_MENU );

   /* Handle Alt+Gr */
   if (KeysSet( pkKT, KeyState, VK_RMENU, 0 ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_CONTROL );

   /* Deal with VK_CAPITAL */
   if (KeysSet( pkKT, KeyState, VK_CAPITAL, 0 ) & KS_LOCK_BIT)
   {
      ModBits |= CAPITAL_BIT;
   }

   /* Deal with VK_NUMLOCK */
   if (KeysSet( pkKT, KeyState, VK_NUMLOCK, 0 ) & KS_LOCK_BIT)
   {
      ModBits |= NUMLOCK_BIT;
   }

   DPRINT( "Current Mod Bits: %x\n", ModBits );

   return ModBits;
}

static BOOL TryToTranslateChar(WORD wVirtKey,
                               DWORD ModBits,
                               PBOOL pbDead,
                               PBOOL pbLigature,
                               PWCHAR pwcTranslatedChar,
                               PKBDTABLES keyLayout )
{
   PVK_TO_WCHAR_TABLE vtwTbl;
   PVK_TO_WCHARS10 vkPtr;
   size_t size_this_entry;
   int nMod;
   DWORD CapsMod = 0, CapsState = 0;

   CapsState = ModBits & ~MOD_BITS_MASK;
   ModBits = ModBits & MOD_BITS_MASK;

   DPRINT ( "TryToTranslate: %04x %x\n", wVirtKey, ModBits );

   if (ModBits > keyLayout->pCharModifiers->wMaxModBits)
   {
      return FALSE;
   }
   for (nMod = 0; keyLayout->pVkToWcharTable[nMod].nModifications; nMod++)
   {
      vtwTbl = &keyLayout->pVkToWcharTable[nMod];
      size_this_entry = vtwTbl->cbSize;
      vkPtr = (PVK_TO_WCHARS10)((BYTE *)vtwTbl->pVkToWchars);
      while(vkPtr->VirtualKey)
      {
         if( wVirtKey == (vkPtr->VirtualKey & 0xff) )
         {
            CapsMod = keyLayout->pCharModifiers->ModNumber
                      [ModBits ^
                       ((CapsState & CAPITAL_BIT) ? vkPtr->Attributes : 0)];

            if( CapsMod >= keyLayout->pVkToWcharTable[nMod].nModifications )
            {
               DWORD MaxBit = 1;
               while( MaxBit <
                      keyLayout->pVkToWcharTable[nMod].nModifications )
                  MaxBit <<= 1;

               CapsMod &= MaxBit - 1; /* Guarantee that CapsMod lies
                        in bounds. */
            }

            *pbDead = vkPtr->wch[CapsMod] == WCH_DEAD;
            *pbLigature = vkPtr->wch[CapsMod] == WCH_LGTR;
            *pwcTranslatedChar = vkPtr->wch[CapsMod];

            DPRINT("%d %04x: CapsMod %08x CapsState %08x Char %04x\n",
                   nMod, wVirtKey,
                   CapsMod, CapsState, *pwcTranslatedChar);

            if( *pbDead )
            {
               vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
               if( vkPtr->VirtualKey != 0xff )
               {
                  DPRINT( "Found dead key with no trailer in the table.\n" );
                  DPRINT( "VK: %04x, ADDR: %08x\n", wVirtKey, (int)vkPtr );
                  return FALSE;
               }
               *pwcTranslatedChar = vkPtr->wch[CapsMod];
            }
            return TRUE;
         }
         vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
      }
   }
   return FALSE;
}

static
int STDCALL
ToUnicodeInner(UINT wVirtKey,
               UINT wScanCode,
               PBYTE lpKeyState,
               LPWSTR pwszBuff,
               int cchBuff,
               UINT wFlags,
               PKBDTABLES pkKT)
{
   WCHAR wcTranslatedChar;
   BOOL bDead;
   BOOL bLigature;

   if( !pkKT )
      return 0;

   if( TryToTranslateChar( wVirtKey,
                           ModBits( pkKT, lpKeyState ),
                           &bDead,
                           &bLigature,
                           &wcTranslatedChar,
                           pkKT ) )
   {
      if( bLigature )
      {
         DPRINT("Not handling ligature (yet)\n" );
         return 0;
      }

      if( cchBuff > 0 )
         pwszBuff[0] = wcTranslatedChar;

      return bDead ? -1 : 1;
   }

   return 0;
}


DWORD FASTCALL UserGetKeyState(DWORD key)
{
   DWORD ret = 0;

   if( key < 0x100 )
   {
      ret = ((DWORD)(gQueueKeyStateTable[key] & KS_DOWN_BIT) << 8 ) |
            (gQueueKeyStateTable[key] & KS_LOCK_BIT);
   }

   return ret;
}


DWORD
STDCALL
NtUserGetKeyState(
   DWORD key)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetKeyState\n");
   UserEnterExclusive();

   RETURN(UserGetKeyState(key));

CLEANUP:
   DPRINT("Leave NtUserGetKeyState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



DWORD FASTCALL UserGetAsyncKeyState(DWORD key)
{
   DWORD ret = 0;

   if( key < 0x100 )
   {
      ret = ((DWORD)(gQueueKeyStateTable[key] & KS_DOWN_BIT) << 8 ) |
            (gQueueKeyStateTable[key] & KS_LOCK_BIT);
   }

   return ret;
}



DWORD
STDCALL
NtUserGetAsyncKeyState(
   DWORD key)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetAsyncKeyState\n");
   UserEnterExclusive();

   RETURN(UserGetAsyncKeyState(key));

CLEANUP:
   DPRINT("Leave NtUserGetAsyncKeyState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



int STDCALL ToUnicodeEx( UINT wVirtKey,
                         UINT wScanCode,
                         PBYTE lpKeyState,
                         LPWSTR pwszBuff,
                         int cchBuff,
                         UINT wFlags,
                         HKL dwhkl )
{
   int ToUnicodeResult = 0;

   if (0 == (lpKeyState[wVirtKey] & KS_DOWN_BIT))
   {
      ToUnicodeResult = 0;
   }
   else
   {
      ToUnicodeResult = ToUnicodeInner( wVirtKey,
                                        wScanCode,
                                        lpKeyState,
                                        pwszBuff,
                                        cchBuff,
                                        wFlags,
                                        PsGetCurrentThreadWin32Thread() ?
                                        PsGetCurrentThreadWin32Thread()->KeyboardLayout : 0 );
   }

   return ToUnicodeResult;
}

int STDCALL ToUnicode( UINT wVirtKey,
                       UINT wScanCode,
                       PBYTE lpKeyState,
                       LPWSTR pwszBuff,
                       int cchBuff,
                       UINT wFlags )
{
   return ToUnicodeEx( wVirtKey,
                       wScanCode,
                       gQueueKeyStateTable,
                       pwszBuff,
                       cchBuff,
                       wFlags,
                       0 );
}

/*
 * Utility to copy and append two unicode strings.
 *
 * IN OUT PUNICODE_STRING ResultFirst -> First string and result
 * IN     PUNICODE_STRING Second      -> Second string to append
 * IN     BOOL            Deallocate  -> TRUE: Deallocate First string before
 *                                       overwriting.
 *
 * Returns NTSTATUS.
 */

NTSTATUS NTAPI AppendUnicodeString(PUNICODE_STRING ResultFirst,
                                   PUNICODE_STRING Second,
                                   BOOL Deallocate)
{
   NTSTATUS Status;
   PWSTR new_string =
      ExAllocatePoolWithTag(PagedPool,
                            (ResultFirst->Length + Second->Length + sizeof(WCHAR)),
                            TAG_STRING);
   if( !new_string )
   {
      return STATUS_NO_MEMORY;
   }
   memcpy( new_string, ResultFirst->Buffer,
           ResultFirst->Length );
   memcpy( new_string + ResultFirst->Length / sizeof(WCHAR),
           Second->Buffer,
           Second->Length );
   if( Deallocate )
      RtlFreeUnicodeString(ResultFirst);
   ResultFirst->Length += Second->Length;
   ResultFirst->MaximumLength = ResultFirst->Length;
   new_string[ResultFirst->Length / sizeof(WCHAR)] = 0;
   Status = RtlCreateUnicodeString(ResultFirst,new_string) ?
            STATUS_SUCCESS : STATUS_NO_MEMORY;
   ExFreePool(new_string);
   return Status;
}

/*
 * Utility function to read a value from the registry more easily.
 *
 * IN  PUNICODE_STRING KeyName       -> Name of key to open
 * IN  PUNICODE_STRING ValueName     -> Name of value to open
 * OUT PUNICODE_STRING ReturnedValue -> String contained in registry
 *
 * Returns NTSTATUS
 */

static NTSTATUS NTAPI ReadRegistryValue( PUNICODE_STRING KeyName,
      PUNICODE_STRING ValueName,
      PUNICODE_STRING ReturnedValue )
{
   NTSTATUS Status;
   HANDLE KeyHandle;
   OBJECT_ATTRIBUTES KeyAttributes;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
   ULONG Length = 0;
   ULONG ResLength = 0;
   UNICODE_STRING Temp;

   InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE,
                              NULL, NULL);
   Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &KeyAttributes);
   if( !NT_SUCCESS(Status) )
   {
      return Status;
   }

   Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
                            0,
                            0,
                            &ResLength);

   if( Status != STATUS_BUFFER_TOO_SMALL )
   {
      NtClose(KeyHandle);
      return Status;
   }

   ResLength += sizeof( *KeyValuePartialInfo );
   KeyValuePartialInfo =
      ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
   Length = ResLength;

   if( !KeyValuePartialInfo )
   {
      NtClose(KeyHandle);
      return STATUS_NO_MEMORY;
   }

   Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
                            (PVOID)KeyValuePartialInfo,
                            Length,
                            &ResLength);

   if( !NT_SUCCESS(Status) )
   {
      NtClose(KeyHandle);
      ExFreePool(KeyValuePartialInfo);
      return Status;
   }

   Temp.Length = Temp.MaximumLength = KeyValuePartialInfo->DataLength;
   Temp.Buffer = (PWCHAR)KeyValuePartialInfo->Data;

   /* At this point, KeyValuePartialInfo->Data contains the key data */
   RtlInitUnicodeString(ReturnedValue,L"");
   AppendUnicodeString(ReturnedValue,&Temp,FALSE);

   ExFreePool(KeyValuePartialInfo);
   NtClose(KeyHandle);

   return Status;
}

typedef PVOID (*KbdLayerDescriptor)(VOID);
NTSTATUS STDCALL LdrGetProcedureAddress(PVOID module,
                                        PANSI_STRING import_name,
                                        DWORD flags,
                                        PVOID *func_addr);

void InitKbdLayout( PVOID *pkKeyboardLayout )
{
   WCHAR LocaleBuffer[16];
   UNICODE_STRING LayoutKeyName;
   UNICODE_STRING LayoutValueName;
   UNICODE_STRING DefaultLocale;
   UNICODE_STRING LayoutFile;
   UNICODE_STRING FullLayoutPath;
   LCID LocaleId;
   PWCHAR KeyboardLayoutWSTR;
   HMODULE kbModule = 0;
   NTSTATUS Status;
   ANSI_STRING kbdProcedureName;
   KbdLayerDescriptor layerDescGetFn;

#define XX_STATUS(x) if (!NT_SUCCESS(Status = (x))) continue;

   do
   {
      Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Could not get default locale (%08lx).\n", Status);
      }
      else
      {
         DPRINT("DefaultLocale = %lx\n", LocaleId);
         swprintf(LocaleBuffer, L"%08lx", LocaleId);
         DPRINT("DefaultLocale = %S\n", LocaleBuffer);
         RtlInitUnicodeString(&DefaultLocale, LocaleBuffer);

         RtlInitUnicodeString(&LayoutKeyName,
                              L"\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet"
                              L"\\Control\\KeyboardLayouts\\");

         AppendUnicodeString(&LayoutKeyName,&DefaultLocale,FALSE);

         RtlInitUnicodeString(&LayoutValueName,L"Layout File");

         Status = ReadRegistryValue(&LayoutKeyName,&LayoutValueName,&LayoutFile);

         RtlFreeUnicodeString(&LayoutKeyName);

         if( !NT_SUCCESS(Status) )
         {
            DPRINT1("Got default locale but not layout file. (%08lx)\n",
                    Status);
         }
         else
         {
            DPRINT("Read registry and got %wZ\n", &LayoutFile);


            RtlInitUnicodeString(&FullLayoutPath,SYSTEMROOT_DIR);
            AppendUnicodeString(&FullLayoutPath,&LayoutFile,FALSE);

            DPRINT("Loading Keyboard DLL %wZ\n", &FullLayoutPath);

            RtlFreeUnicodeString(&LayoutFile);

            KeyboardLayoutWSTR =
               ExAllocatePoolWithTag(PagedPool,
                                     FullLayoutPath.Length + sizeof(WCHAR),
                                     TAG_STRING);

            if( !KeyboardLayoutWSTR )
            {
               DPRINT1("Couldn't allocate a string for the keyboard layout name.\n");
               RtlFreeUnicodeString(&FullLayoutPath);
               return;
            }
            memcpy(KeyboardLayoutWSTR,FullLayoutPath.Buffer,
                   FullLayoutPath.Length);
            KeyboardLayoutWSTR[FullLayoutPath.Length / sizeof(WCHAR)] = 0;

            kbModule = EngLoadImage(KeyboardLayoutWSTR);
            DPRINT( "Load Keyboard Layout: %S\n", KeyboardLayoutWSTR );

            if( !kbModule )
               DPRINT1( "Load Keyboard Layout: No %wZ\n", &FullLayoutPath );

            ExFreePool(KeyboardLayoutWSTR);
            RtlFreeUnicodeString(&FullLayoutPath);
         }
      }

      if( !kbModule )
      {
         DPRINT1("Trying to load US Keyboard Layout\n");
         kbModule = EngLoadImage(L"\\SystemRoot\\system32\\kbdus.dll");

         if (!kbModule)
         {
            DPRINT1("Failed to load any Keyboard Layout\n");
            return;
         }
      }

      RtlInitAnsiString( &kbdProcedureName, "KbdLayerDescriptor" );

      LdrGetProcedureAddress((PVOID)kbModule,
                             &kbdProcedureName,
                             0,
                             (PVOID*)&layerDescGetFn);

      if( layerDescGetFn )
      {
         *pkKeyboardLayout = layerDescGetFn();
      }
   }
   while (FALSE);

   if( !*pkKeyboardLayout )
   {
      DPRINT1("Failed to load the keyboard layout.\n");
   }

#undef XX_STATUS
}

PKBDTABLES W32kGetDefaultKeyLayout(VOID)
{
   PKBDTABLES pkKeyboardLayout = 0;
   InitKbdLayout( (PVOID) &pkKeyboardLayout );
   return pkKeyboardLayout;
}

BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg,
                       HKL dwhkl)
{
   static INT dead_char = 0;
   LONG UState = 0;
   WCHAR wp[2] = { 0 };
   MSG NewMsg = { 0 };
   PKBDTABLES keyLayout;
   BOOL Result = FALSE;
   DWORD ScanCode = 0;


   keyLayout = PsGetCurrentThreadWin32Thread()->KeyboardLayout;
   if( !keyLayout )
      return FALSE;

   if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
      return FALSE;

   ScanCode = (lpMsg->lParam >> 16) & 0xff;

   /* All messages have to contain the cursor point. */
   IntGetCursorLocation(PsGetCurrentThreadWin32Thread()->Desktop->WindowStation,
                        &NewMsg.pt);

   UState = ToUnicodeInner(lpMsg->wParam, HIWORD(lpMsg->lParam) & 0xff,
                           gQueueKeyStateTable, wp, 2, 0,
                           keyLayout );

   if (UState == 1)
   {
      NewMsg.message = (lpMsg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
      if (dead_char)
      {
         ULONG i;
         WCHAR first, second;
         DPRINT("PREVIOUS DEAD CHAR: %c\n", dead_char);

         for( i = 0; keyLayout->pDeadKey[i].dwBoth; i++ )
         {
            first = keyLayout->pDeadKey[i].dwBoth >> 16;
            second = keyLayout->pDeadKey[i].dwBoth;
            if (first == dead_char && second == wp[0])
            {
               wp[0] = keyLayout->pDeadKey[i].wchComposed;
               dead_char = 0;
               break;
            }
         }

         DPRINT("FINAL CHAR: %c\n", wp[0]);
      }

      if (dead_char)
      {
         NewMsg.hwnd = lpMsg->hwnd;
         NewMsg.wParam = dead_char;
         NewMsg.lParam = lpMsg->lParam;
         dead_char = 0;
         MsqPostMessage(PsGetCurrentThreadWin32Thread()->MessageQueue, &NewMsg, FALSE, QS_KEY);
      }

      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      DPRINT( "CHAR='%c' %04x %08x\n", wp[0], wp[0], lpMsg->lParam );
      MsqPostMessage(PsGetCurrentThreadWin32Thread()->MessageQueue, &NewMsg, FALSE, QS_KEY);
      Result = TRUE;
   }
   else if (UState == -1)
   {
      NewMsg.message =
         (lpMsg->message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      dead_char = wp[0];
      MsqPostMessage(PsGetCurrentThreadWin32Thread()->MessageQueue, &NewMsg, FALSE, QS_KEY);
      Result = TRUE;
   }

   return Result;
}

DWORD
STDCALL
NtUserGetKeyboardState(
   LPBYTE lpKeyState)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(DWORD);
   
   DPRINT("Enter NtUserGetKeyboardState\n");
   UserEnterShared();
   
   if (lpKeyState)
   {
      if(!NT_SUCCESS(MmCopyToCaller(lpKeyState, gQueueKeyStateTable, 256)))
         Result = FALSE;
   }

   RETURN(Result);
   
CLEANUP:
   DPRINT("Leave NtUserGetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD
STDCALL
NtUserSetKeyboardState(LPBYTE lpKeyState)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(DWORD);
   
   DPRINT("Enter NtUserSetKeyboardState\n");
   UserEnterExclusive();

   if (lpKeyState)
   {
      if(! NT_SUCCESS(MmCopyFromCaller(gQueueKeyStateTable, lpKeyState, 256)))
         Result = FALSE;
   }
   
   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserSetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static UINT VkToScan( UINT Code, BOOL ExtCode, PKBDTABLES pkKT )
{
   int i;

   for( i = 0; i < pkKT->bMaxVSCtoVK; i++ )
   {
      if( pkKT->pusVSCtoVK[i] == Code )
      {
         return i;
      }
   }

   return 0;
}

UINT ScanToVk( UINT Code, BOOL ExtKey, PKBDTABLES pkKT )
{
   if( !pkKT )
   {
      DPRINT("ScanToVk: No layout\n");
      return 0;
   }

   if( ExtKey )
   {
      int i;

      for( i = 0; pkKT->pVSCtoVK_E0[i].Vsc; i++ )
      {
         if( pkKT->pVSCtoVK_E0[i].Vsc == Code )
            return pkKT->pVSCtoVK_E0[i].Vk & 0xff;
      }
      for( i = 0; pkKT->pVSCtoVK_E1[i].Vsc; i++ )
      {
         if( pkKT->pVSCtoVK_E1[i].Vsc == Code )
            return pkKT->pVSCtoVK_E1[i].Vk & 0xff;
      }

      return 0;
   }
   else
   {
      if( Code >= pkKT->bMaxVSCtoVK )
      {
         return 0;
      }
      return pkKT->pusVSCtoVK[Code] & 0xff;
   }
}

/*
 * Map a virtual key code, or virtual scan code, to a scan code, key code,
 * or unshifted unicode character.
 *
 * Code: See Below
 * Type:
 * 0 -- Code is a virtual key code that is converted into a virtual scan code
 *      that does not distinguish between left and right shift keys.
 * 1 -- Code is a virtual scan code that is converted into a virtual key code
 *      that does not distinguish between left and right shift keys.
 * 2 -- Code is a virtual key code that is converted into an unshifted unicode
 *      character.
 * 3 -- Code is a virtual scan code that is converted into a virtual key code
 *      that distinguishes left and right shift keys.
 * KeyLayout: Keyboard layout handle (currently, unused)
 *
 * @implemented
 */

static UINT IntMapVirtualKeyEx( UINT Code, UINT Type, PKBDTABLES keyLayout )
{
   UINT ret = 0;

   switch( Type )
   {
      case 0:
         if( Code == VK_RSHIFT )
            Code = VK_LSHIFT;
         if( Code == VK_RMENU )
            Code = VK_LMENU;
         if( Code == VK_RCONTROL )
            Code = VK_LCONTROL;
         ret = VkToScan( Code, FALSE, keyLayout );
         break;

      case 1:
         ret =
            DontDistinguishShifts
            (IntMapVirtualKeyEx( Code, 3, keyLayout ) );
         break;

      case 2:
         {
            WCHAR wp[2] = {0};

            ret = VkToScan( Code, FALSE, keyLayout );
            ToUnicodeInner( Code, ret, 0, wp, 2, 0, keyLayout );
            ret = wp[0];
         }
         break;

      case 3:

         ret = ScanToVk( Code, FALSE, keyLayout );
         break;
   }

   return ret;
}

UINT
STDCALL
NtUserMapVirtualKeyEx( UINT Code, UINT Type, DWORD keyboardId, HKL dwhkl )
{
   PKBDTABLES keyLayout;
   DECLARE_RETURN(UINT);
   
   DPRINT("Enter NtUserMapVirtualKeyEx\n");
   UserEnterExclusive();
   
   keyLayout = PsGetCurrentThreadWin32Thread() ? PsGetCurrentThreadWin32Thread()->KeyboardLayout : 0;

   if( !keyLayout )
      RETURN(0);

   RETURN(IntMapVirtualKeyEx( Code, Type, keyLayout ));
   
CLEANUP:
   DPRINT("Leave NtUserMapVirtualKeyEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;   
}


int
STDCALL
NtUserToUnicodeEx(
   UINT wVirtKey,
   UINT wScanCode,
   PBYTE lpKeyState,
   LPWSTR pwszBuff,
   int cchBuff,
   UINT wFlags,
   HKL dwhkl )
{
   BYTE KeyStateBuf[0x100];
   PWCHAR OutPwszBuff = 0;
   int ret = 0;
   DECLARE_RETURN(int);
   
   DPRINT("Enter NtUserSetKeyboardState\n");
   UserEnterShared();//faxme: this syscall doesnt seem to need any locking...


   if( !NT_SUCCESS(MmCopyFromCaller(KeyStateBuf,
                                    lpKeyState,
                                    sizeof(KeyStateBuf))) )
   {
      DPRINT1( "Couldn't copy key state from caller.\n" );
      RETURN(0);
   }
   OutPwszBuff = ExAllocatePoolWithTag(NonPagedPool,sizeof(WCHAR) * cchBuff, TAG_STRING);
   if( !OutPwszBuff )
   {
      DPRINT1( "ExAllocatePool(%d) failed\n", sizeof(WCHAR) * cchBuff);
      RETURN(0);
   }
   RtlZeroMemory( OutPwszBuff, sizeof( WCHAR ) * cchBuff );

   ret = ToUnicodeEx( wVirtKey,
                      wScanCode,
                      KeyStateBuf,
                      OutPwszBuff,
                      cchBuff,
                      wFlags,
                      dwhkl );

   MmCopyToCaller(pwszBuff,OutPwszBuff,sizeof(WCHAR)*cchBuff);
   ExFreePool(OutPwszBuff);

   RETURN(ret);
   
CLEANUP:
   DPRINT("Leave NtUserSetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;   
}

static int W32kSimpleToupper( int ch )
{
   if( ch >= 'a' && ch <= 'z' )
      ch = ch - 'a' + 'A';
   return ch;
}

DWORD
STDCALL
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize )
{
   int i;
   DWORD ret = 0;
   UINT CareVk = 0;
   UINT VkCode = 0;
   UINT ScanCode = (lParam >> 16) & 0xff;
   BOOL ExtKey = lParam & (1<<24) ? TRUE : FALSE;
   PKBDTABLES keyLayout;
   VSC_LPWSTR *KeyNames;
   DECLARE_RETURN(DWORD);
   
   DPRINT("Enter NtUserGetKeyNameText\n");
   UserEnterShared();
   
   keyLayout = PsGetCurrentThreadWin32Thread() ?
      PsGetCurrentThreadWin32Thread()->KeyboardLayout : 0;

   if( !keyLayout || nSize < 1 )
      RETURN(0);

   if( lParam & (1<<25) )
   {
      CareVk = VkCode = ScanToVk( ScanCode, ExtKey, keyLayout );
      if( VkCode == VK_LSHIFT || VkCode == VK_RSHIFT )
         VkCode = VK_LSHIFT;
      if( VkCode == VK_LCONTROL || VkCode == VK_RCONTROL )
         VkCode = VK_LCONTROL;
      if( VkCode == VK_LMENU || VkCode == VK_RMENU )
         VkCode = VK_LMENU;
   }
   else
   {
      VkCode = ScanToVk( ScanCode, ExtKey, keyLayout );
   }

   KeyNames = 0;

   if( CareVk != VkCode )
      ScanCode = VkToScan( VkCode, ExtKey, keyLayout );

   if( ExtKey )
      KeyNames = keyLayout->pKeyNamesExt;
   else
      KeyNames = keyLayout->pKeyNames;

   for( i = 0; KeyNames[i].pwsz; i++ )
   {
      if( KeyNames[i].vsc == ScanCode )
      {
         UINT StrLen = wcslen(KeyNames[i].pwsz);
         UINT StrMax = StrLen > (nSize - 1) ? (nSize - 1) : StrLen;
         WCHAR null_wc = 0;
         if( NT_SUCCESS( MmCopyToCaller( lpString,
                                         KeyNames[i].pwsz,
                                         StrMax * sizeof(WCHAR) ) ) &&
               NT_SUCCESS( MmCopyToCaller( lpString + StrMax,
                                           &null_wc,
                                           sizeof( WCHAR ) ) ) )
         {
            ret = StrMax;
            break;
         }
      }
   }

   if( ret == 0 )
   {
      WCHAR UCName[2];

      UCName[0] = W32kSimpleToupper(IntMapVirtualKeyEx( VkCode, 2, keyLayout ));
      UCName[1] = 0;
      ret = 1;

      if( !NT_SUCCESS(MmCopyToCaller( lpString, UCName, 2 * sizeof(WCHAR) )) )
         RETURN(0);
   }

   RETURN(ret);
   
CLEANUP:
   DPRINT("Leave NtUserGetKeyNameText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * Filter this message according to the current key layout, setting wParam
 * appropriately.
 */

VOID FASTCALL
W32kKeyProcessMessage(LPMSG Msg,
                      PKBDTABLES KeyboardLayout,
                      BYTE Prefix)
{
   DWORD ScanCode = 0, ModifierBits = 0;
   DWORD i = 0;
   DWORD BaseMapping = 0;
   DWORD RawVk = 0;
   static WORD NumpadConversion[][2] =
      { { VK_DELETE, VK_DECIMAL },
        { VK_INSERT, VK_NUMPAD0 },
        { VK_END,    VK_NUMPAD1 },
        { VK_DOWN,   VK_NUMPAD2 },
        { VK_NEXT,   VK_NUMPAD3 },
        { VK_LEFT,   VK_NUMPAD4 },
        { VK_CLEAR,  VK_NUMPAD5 },
        { VK_RIGHT,  VK_NUMPAD6 },
        { VK_HOME,   VK_NUMPAD7 },
        { VK_UP,     VK_NUMPAD8 },
        { VK_PRIOR,  VK_NUMPAD9 },
        { 0,0 } };
   PVSC_VK VscVkTable = NULL;

   if( !KeyboardLayout || !Msg ||
         (Msg->message != WM_KEYDOWN && Msg->message != WM_SYSKEYDOWN &&
          Msg->message != WM_KEYUP   && Msg->message != WM_SYSKEYUP) )
   {
      return;
   }

   /* arty -- handle numpad -- On real windows, the actual key produced
    * by the messaging layer is different based on the state of numlock. */
   ModifierBits = ModBits(KeyboardLayout,gQueueKeyStateTable);

   /* Get the raw scan code, so we can look up whether the key is a numpad
    * key
    *
    * Shift and the LP_EXT_BIT cancel. */
   ScanCode = (Msg->lParam >> 16) & 0xff;
   BaseMapping = Msg->wParam =
                    IntMapVirtualKeyEx( ScanCode, 1, KeyboardLayout );
   if( Prefix == 0 )
   {
      if( ScanCode >= KeyboardLayout->bMaxVSCtoVK )
         RawVk = 0xff;
      else
         RawVk = KeyboardLayout->pusVSCtoVK[ScanCode];
   }
   else
   {
      if( Prefix == 0xE0 )
      {
         /* ignore shift codes */
         if( ScanCode == 0x2A || ScanCode == 0x36 )
         {
            return;
         }
         VscVkTable = KeyboardLayout->pVSCtoVK_E0;
      }
      else if( Prefix == 0xE1 )
      {
         VscVkTable = KeyboardLayout->pVSCtoVK_E1;
      }

      RawVk = 0xff;
      while (VscVkTable->Vsc)
      {
         if( VscVkTable->Vsc == ScanCode )
         {
            RawVk = VscVkTable->Vk;
         }
         VscVkTable++;
      }
   }

   if ((ModifierBits & NUMLOCK_BIT) &&
         !(ModifierBits & GetShiftBit(KeyboardLayout, VK_SHIFT)) &&
         (RawVk & KNUMP) &&
         !(Msg->lParam & LP_EXT_BIT))
   {
      /* The key in question is a numpad key.  Search for a translation. */
      for (i = 0; NumpadConversion[i][0]; i++)
      {
         if ((BaseMapping & 0xff) == NumpadConversion[i][0]) /* RawVk? */
         {
            Msg->wParam = NumpadConversion[i][1];
            break;
         }
      }
   }

   DPRINT("Key: [%04x -> %04x]\n", BaseMapping, Msg->wParam);

   /* Now that we have the VK, we can set the keymap appropriately
    * This is a better place for this code, as it's guaranteed to be
    * run, unlike translate message. */
   if (Msg->message == WM_KEYDOWN || Msg->message == WM_SYSKEYDOWN)
   {
      SetKeyState( ScanCode, Msg->wParam, Msg->lParam & LP_EXT_BIT,
                   TRUE ); /* Strike key */
   }
   else if (Msg->message == WM_KEYUP || Msg->message == WM_SYSKEYUP)
   {
      SetKeyState( ScanCode, Msg->wParam, Msg->lParam & LP_EXT_BIT,
                   FALSE ); /* Release key */
   }

   /* We need to unset SYSKEYDOWN if the ALT key is an ALT+Gr */
   if( gQueueKeyStateTable[VK_RMENU] & KS_DOWN_BIT )
   {
      if( Msg->message == WM_SYSKEYDOWN )
         Msg->message = WM_KEYDOWN;
      else
         Msg->message = WM_KEYUP;
   }

}

DWORD
STDCALL
NtUserGetKeyboardLayoutList(
   DWORD Items,
   DWORD pHklBuff)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
STDCALL
NtUserGetKeyboardLayoutName(
   LPWSTR lpszName)
{
  BOOL ret = FALSE;
  LCID LocaleId;
  WCHAR LocaleBuffer[16];
  NTSTATUS Status;


  UserEnterExclusive();

   DPRINT("Enter NtUserGetKeyboardLayoutName\n");

   Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
   if (NT_SUCCESS(Status))
   {
        swprintf(LocaleBuffer, L"%08lx", LocaleId);
        DPRINT("LocaleId : %08lx\n",LocaleId);
        _SEH_TRY
        {
           ProbeForWrite(lpszName, 16, 1);
           RtlCopyMemory(lpszName,LocaleBuffer,16);
            ret = TRUE;
        }
        _SEH_HANDLE
        {
            SetLastNtError(_SEH_GetExceptionCode());
            ret = FALSE;
        }
        _SEH_END;
   }
   UserLeave();
   return ret;
}


HKL FASTCALL
UserGetKeyboardLayout(
   DWORD dwThreadId)
{
   NTSTATUS Status;
   PETHREAD Thread;
   PW32THREAD W32Thread;
   PKBDTABLES layout;

   if (!dwThreadId)
      W32Thread = PsGetCurrentThreadWin32Thread();
   else
   {
      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);//fixme: deref thread
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      W32Thread = Thread->Tcb.Win32Thread; /* Wrong, but returning the pointer to
                                              the table. */
   }
   layout = W32Thread->KeyboardLayout;
   if(!layout)
      return 0;
   return (HKL)layout;
}


HKL
STDCALL
NtUserGetKeyboardLayout(
   DWORD dwThreadId)
{
   DECLARE_RETURN(HKL);

   UserEnterShared();
   DPRINT("Enter NtUserGetKeyboardLayout\n");

   RETURN( UserGetKeyboardLayout(dwThreadId));

CLEANUP:
   DPRINT("Leave NtUserGetKeyboardLayout, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD FASTCALL
UserGetKeyboardType(
   DWORD TypeFlag)
{
   switch(TypeFlag)
   {
      case 0:        /* Keyboard type */
         return 4;    /* AT-101 */
      case 1:        /* Keyboard Subtype */
         return 0;    /* There are no defined subtypes */
      case 2:        /* Number of F-keys */
         return 12;   /* We're doing an 101 for now, so return 12 F-keys */
      default:
         DPRINT1("Unknown type!\n");
         return 0;    /* The book says 0 here, so 0 */
   }
}


HKL
STDCALL
NtUserLoadKeyboardLayoutEx( 
   IN HANDLE Handle,
   IN DWORD offTable,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN UINT KLayoutLangID,
   IN UINT Flags)
{
   UNIMPLEMENTED

   return 0;
}


/*
    Based on TryToTranslateChar, instead of processing VirtualKey match,
    look for wChar match.
 */
DWORD
STDCALL
NtUserVkKeyScanEx(
   DWORD wChar,
   DWORD KeyboardLayout,
   DWORD Unknown2)
{
/* FAXME: currently, this routine doesnt seem to need any locking */

   PKBDTABLES KeyLayout;
   PVK_TO_WCHAR_TABLE vtwTbl;
   PVK_TO_WCHARS10 vkPtr;
   size_t size_this_entry;
   int nMod;
   DWORD CapsMod = 0, CapsState = 0;

   if(!KeyboardLayout)
      return -1;
   KeyLayout = (PKBDTABLES) KeyboardLayout;

   for (nMod = 0; KeyLayout->pVkToWcharTable[nMod].nModifications; nMod++)
   {
      vtwTbl = &KeyLayout->pVkToWcharTable[nMod];
      size_this_entry = vtwTbl->cbSize;
      vkPtr = (PVK_TO_WCHARS10)((BYTE *)vtwTbl->pVkToWchars);

      while(vkPtr->VirtualKey)
      {
         /*
            0x01 Shift key
            0x02 Ctrl key
            0x04 Alt key
            Should have only 8 valid possibilities. Including zero.
          */
         for(CapsState = 0; CapsState < vtwTbl->nModifications; CapsState++)
         {
            if(vkPtr->wch[CapsState] == wChar)
            {
               CapsMod = KeyLayout->pCharModifiers->ModNumber[CapsState];
               DPRINT("nMod %d wC %04x: CapsMod %08x CapsState %08x MaxModBits %08x\n",
                      nMod, wChar, CapsMod, CapsState, KeyLayout->pCharModifiers->wMaxModBits);
               return ((CapsMod << 8)|(vkPtr->VirtualKey & 0xff));
            }
         }
         vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
      }
   }
   return -1;
}


/* EOF */
