#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <lsass/lsass.h>

#define NDEBUG
#include <debug.h>

#include <ntsecapi.h>
#include <secext.h>


BOOLEAN
WINAPI
GetComputerObjectNameA (
	EXTENDED_NAME_FORMAT extended_name_format,
	LPSTR lpstr,
	PULONG pulong
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOLEAN
WINAPI
GetComputerObjectNameW (
	EXTENDED_NAME_FORMAT extended_name_format,
	LPWSTR lpstr,
	PULONG pulong
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOLEAN
WINAPI
GetUserNameExA (
	EXTENDED_NAME_FORMAT extended_exe_format,
	LPSTR lpstr,
	PULONG pulong
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOLEAN
WINAPI
GetUserNameExW (
	EXTENDED_NAME_FORMAT extended_exe_format,
	LPWSTR lpstr,
	PULONG pulong
	)
{
	DPRINT1("%s() not implemented!\n", __FUNCTION__);
	return ERROR_CALL_NOT_IMPLEMENTED;
}
