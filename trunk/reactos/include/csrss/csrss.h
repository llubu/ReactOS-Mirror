#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

#define CSRSS_CREATE_PROCESS            (0x1)
#define CSRSS_TERMINATE_PROCESS         (0x2)
#define CSRSS_WRITE_CONSOLE             (0x3)
#define CSRSS_READ_CONSOLE              (0x4)
#define CSRSS_ALLOC_CONSO               (0x5)
#define CSRSS_FREE_CONSOLE              (0x6)
#define CSRSS_CONNECT_PROCESS           (0x7)

typedef struct
{
   ULONG Type;
   BYTE Data[0x12C];
} CSRSS_API_REQUEST, *PCSRSS_API_REQUEST;

typedef struct
{
   NTSTATUS Status;
   ULONG Count;
   HANDLE Handle;
} CSRSS_API_REPLY, *PCSRSS_API_REPLY;

#endif
