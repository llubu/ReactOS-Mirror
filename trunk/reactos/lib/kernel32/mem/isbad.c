#include <windows.h>

WINBOOL 
STDCALL
IsBadReadPtr(  CONST VOID *lp,  UINT ucb     )
{	
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
		return FALSE;

	VirtualQuery(  lp, &MemoryInformation,  sizeof(MEMORY_BASIC_INFORMATION) );
	
	if ( MemoryInformation.State != MEM_COMMIT )
		return FALSE;
		
	if ( MemoryInformation.RegionSize < ucb )
		return FALSE;
		
	if ( MemoryInformation.Protect == PAGE_EXECUTE )
		return FALSE;
		
	if ( MemoryInformation.Protect == PAGE_NOACCESS )
		return FALSE;	
		
	return TRUE;
			
}
WINBOOL 
STDCALL
IsBadHugeReadPtr(  CONST VOID *lp,  UINT ucb     )
{
	return IsBadReadPtr(lp,ucb);
}

WINBOOL 
STDCALL
IsBadCodePtr(  FARPROC lpfn  )
{
	MEMORY_BASIC_INFORMATION MemoryInformation;


	VirtualQuery(  lpfn, &MemoryInformation,  sizeof(MEMORY_BASIC_INFORMATION) );
	
	if ( MemoryInformation.State != MEM_COMMIT )
		return FALSE;
		
			
	if ( MemoryInformation.Protect == PAGE_EXECUTE || MemoryInformation.Protect == PAGE_EXECUTE_READ)
		return TRUE;
		
		
	return FALSE;
}

WINBOOL
STDCALL IsBadWritePtr(  LPVOID lp,  UINT ucb   )
{
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
		return FALSE;

	VirtualQuery(  lp, &MemoryInformation,  sizeof(MEMORY_BASIC_INFORMATION) );
	
	if ( MemoryInformation.State != MEM_COMMIT )
		return FALSE;
		
		
	if ( MemoryInformation.RegionSize < ucb )
		return FALSE;
		
		
	if ( MemoryInformation.Protect == PAGE_READONLY)
		return FALSE;	
		
	if ( MemoryInformation.Protect == PAGE_EXECUTE || MemoryInformation.Protect == PAGE_EXECUTE_READ)
		return FALSE;
		
	if ( MemoryInformation.Protect == PAGE_NOACCESS )
		return FALSE;	
		
	return TRUE;
}

WINBOOL
STDCALL
IsBadHugeWritePtr(
		  LPVOID lp,
		  UINT ucb
		  )
{
	return IsBadWritePtr(lp,ucb);
}


WINBOOL
STDCALL
IsBadStringPtrW(  LPCWSTR lpsz,  UINT ucchMax   )
{
	UINT Len = wcsnlen(lpsz+1,ucchMax>>1);
	return IsBadReadPtr(lpsz,Len<<1);
}

WINBOOL 
STDCALL
IsBadStringPtrA(  LPCSTR lpsz,  UINT ucchMax   )
{
	UINT Len = strnlen(lpsz+1,ucchMax);
	return IsBadReadPtr(lpsz,Len);
}
