#ifndef __INCLUDE_DDK_PSTYPES_H
#define __INCLUDE_DDK_PSTYPES_H

#include <ntos/ps.h>

#include <ntos/tss.h>
#include <napi/teb.h>

#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 	(64)
#endif
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES		0xFFFFFFFF
#endif
#ifndef MAX_PATH
#define MAX_PATH 	(260)
#endif

struct _EPROCESS;
struct _KPROCESS;
struct _ETHREAD;
struct _KTHREAD;
struct _EJOB;

typedef struct _EJOB *PEJOB;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;

typedef struct _IMAGE_INFO {
   union {
      ULONG Properties;
      struct {
         ULONG ImageAddressingMode : 8;
         ULONG SystemModeImage : 1;
         ULONG ImageMappedToAllPids : 1;
         ULONG Reserved : 22;
      };
   };
   PVOID ImageBase;
   ULONG ImageSelector;
   ULONG ImageSize;
   ULONG ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

typedef VOID STDCALL_FUNC
(*PKSTART_ROUTINE)(PVOID StartContext);

typedef VOID STDCALL_FUNC
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(HANDLE ParentId,
				  HANDLE ProcessId,
				  BOOLEAN Create);

typedef VOID STDCALL_FUNC
(*PCREATE_THREAD_NOTIFY_ROUTINE)(HANDLE ProcessId,
				 HANDLE ThreadId,
				 BOOLEAN Create);

typedef VOID STDCALL_FUNC
(*PLOAD_IMAGE_NOTIFY_ROUTINE)(PUNICODE_STRING FullImageName,
                              HANDLE ProcessId,
                              PIMAGE_INFO ImageInfo);

typedef NTSTATUS STDCALL_FUNC
(*PW32_PROCESS_CALLBACK)(struct _EPROCESS *Process,
			 BOOLEAN Create);

typedef NTSTATUS STDCALL_FUNC
(*PW32_THREAD_CALLBACK)(struct _ETHREAD *Thread,
			BOOLEAN Create);
            
/* 
 * Callbacks used for Win32 objects... this define won't be needed after the Object Manager
 * rewrite -- Alex
 */
typedef NTSTATUS STDCALL_FUNC
(*OBJECT_CREATE_ROUTINE)(PVOID ObjectBody,
                         PVOID Parent,
                         PWSTR RemainingPath,
                         struct _OBJECT_ATTRIBUTES* ObjectAttributes);

typedef NTSTATUS STDCALL_FUNC
(*OBJECT_PARSE_ROUTINE)(PVOID Object,
                        PVOID *NextObject,
                        PUNICODE_STRING FullPath,
                        PWSTR *Path,
                        ULONG Attributes);
                        
typedef VOID STDCALL_FUNC
(*OBJECT_DELETE_ROUTINE)(PVOID DeletedObject);

typedef PVOID STDCALL_FUNC
(*OBJECT_FIND_ROUTINE)(PVOID WinStaObject,
                       PWSTR Name,
                       ULONG Attributes);
                       
typedef struct _W32_OBJECT_CALLBACK {
    OBJECT_CREATE_ROUTINE WinStaCreate;
    OBJECT_PARSE_ROUTINE WinStaParse;
    OBJECT_DELETE_ROUTINE WinStaDelete;
    OBJECT_FIND_ROUTINE WinStaFind;
    OBJECT_CREATE_ROUTINE DesktopCreate;
    OBJECT_DELETE_ROUTINE DesktopDelete;    
} W32_OBJECT_CALLBACK, *PW32_OBJECT_CALLBACK;

typedef struct _STACK_INFORMATION
{
  PVOID BaseAddress;
  PVOID UpperAddress;
} STACK_INFORMATION, *PSTACK_INFORMATION;

typedef ULONG THREADINFOCLASS;
typedef ULONG PROCESSINFOCLASS;

struct _KPROCESS;

#define LOW_PRIORITY (0)
#define LOW_REALTIME_PRIORITY (16)
#define HIGH_PRIORITY (31)
#define MAXIMUM_PRIORITY (32)

#define IMAGE_ADDRESSING_MODE_32BIT (3)

#endif /* __INCLUDE_DDK_PSTYPES_H */
