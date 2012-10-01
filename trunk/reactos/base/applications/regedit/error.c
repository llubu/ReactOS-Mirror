/*
 * Regedit errors, warnings, informations displaying
 *
 * Copyright (C) 2010 Adam Kachwalla <geekdundee@gmail.com>
 * Copyright (C) 2012 Hermès Bélusca - Maïto <hermes.belusca@sfr.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <regedit.h>

int ErrorMessageBox(HWND hWnd, LPCTSTR lpTitle, DWORD dwErrorCode, ...)
{
    int iRet = 0;
    LPTSTR lpMsgBuf = NULL;
    DWORD Status = 0;

    va_list args = NULL;
    va_start(args, dwErrorCode);

    Status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           dwErrorCode,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR)&lpMsgBuf,
                           0,
                           &args);

    va_end(args);

    iRet = MessageBox(hWnd, (Status && lpMsgBuf ? lpMsgBuf : TEXT("Error displaying error message.\n")), lpTitle, MB_OK | MB_ICONERROR);

    if (lpMsgBuf) LocalFree(lpMsgBuf);

    /* Return the MessageBox information */
    return iRet;
}

int InfoMessageBox(HWND hWnd, UINT uType, LPCTSTR lpTitle, LPCTSTR lpMessage, ...)
{
    int iRet = 0;
    LPTSTR lpMsgBuf = NULL;
    DWORD Status = 0;

    va_list args = NULL;
    va_start(args, lpMessage);

    Status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           lpMessage,
                           0,
                           0,
                           (LPTSTR)&lpMsgBuf,
                           0,
                           &args);

    va_end(args);

    iRet = MessageBox(hWnd, (Status && lpMsgBuf ? lpMsgBuf : TEXT("Error displaying error message.\n")), lpTitle, uType);

    if (lpMsgBuf) LocalFree(lpMsgBuf);

    /* Return the MessageBox information */
    return iRet;
}
