/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/layer/versionlie.inl
 * PURPOSE:         Version lie implementation helper
 * PROGRAMMER:      Mark Jansen
 */


#include <setup_shim.inl>

DWORD WINAPI SHIM_OBJ_NAME(APIHook_GetVersion)()
{
    return VERSION_INFO.FullVersion;
}

BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExA)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(0, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &VERSION_INFO);
    }
    return FALSE;
}

/* We do not care about the actual type, FakeVersion will correctly handle it either way */
BOOL WINAPI SHIM_OBJ_NAME(APIHook_GetVersionExW)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(1, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &VERSION_INFO);
    }
    return FALSE;
}

/* We do not care about the actual type, FakeVersion will correctly handle it either way */
BOOL WINAPI SHIM_OBJ_NAME(APIHook_RtlGetVersion)(LPOSVERSIONINFOEXA lpOsVersionInfo)
{
    if (CALL_SHIM(3, GETVERSIONEXAPROC)(lpOsVersionInfo))
    {
        return FakeVersion(lpOsVersionInfo, &VERSION_INFO);
    }
    return FALSE;
}

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason)
{
    if (fdwReason == SHIM_REASON_ATTACH && VERSION_INFO.wServicePackMajor)
    {
        static CONST WCHAR szServicePack[] = {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','%','u',0};
        HRESULT hr = StringCbPrintfA(VERSION_INFO.szCSDVersionA, sizeof(VERSION_INFO.szCSDVersionA),
            "Service Pack %u", VERSION_INFO.wServicePackMajor);
        if (FAILED(hr))
            return FALSE;
        StringCbPrintfW(VERSION_INFO.szCSDVersionW, sizeof(VERSION_INFO.szCSDVersionW),
            szServicePack, VERSION_INFO.wServicePackMajor);
        if (FAILED(hr))
            return FALSE;
    }
    return TRUE;
}


#define SHIM_NUM_HOOKS  4
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "GetVersionExA", SHIM_OBJ_NAME(APIHook_GetVersionExA)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "GetVersionExW", SHIM_OBJ_NAME(APIHook_GetVersionExW)) \
    SHIM_HOOK(2, "KERNEL32.DLL", "GetVersion", SHIM_OBJ_NAME(APIHook_GetVersion)) \
    SHIM_HOOK(3, "NTDLL.DLL", "RtlGetVersion", SHIM_OBJ_NAME(APIHook_RtlGetVersion))
#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

#include <implement_shim.inl>

#undef VERSION_INFO

