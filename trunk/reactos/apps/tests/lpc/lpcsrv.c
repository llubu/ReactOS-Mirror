/* $Id: lpcsrv.c,v 1.4 2000/01/22 22:22:48 ea Exp $
 *
 * DESCRIPTION: Simple LPC Server
 * PROGRAMMER:  David Welch
 */
#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void debug_printf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


int main(int argc, char* argv[])
{
   UNICODE_STRING PortName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   HANDLE NamedPortHandle;
   HANDLE PortHandle;
   LPCMESSAGE ConnectMsg;
   
   printf("(lpcsrv.exe) Lpc test server\n");
   
   RtlInitUnicodeString(&PortName, L"\\TestPort");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   
   printf("(lpcsrv.exe) Creating port\n");
   Status = NtCreatePort(&NamedPortHandle,
			 &ObjectAttributes,
			 0,
			 0,
			 0);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to create port (Status = 0x%08X)\n", Status);
	return EXIT_FAILURE;
     }
   
   
   printf("(lpcsrv.exe) Listening for connections\n");
   Status = NtListenPort(NamedPortHandle,
			 &ConnectMsg);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to listen for connections (Status = 0x%08X)\n", Status);
	return EXIT_FAILURE;
     }
   
   printf("(lpcsrv.exe) Accepting connections\n");
   Status = NtAcceptConnectPort(&PortHandle,
				NamedPortHandle,
				NULL,
				1,
				0,
				NULL);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to accept connection (Status = 0x%08X)\n", Status);
	return EXIT_FAILURE;
     }   
   
   printf("(lpcsrv.exe) Completing connection\n");
   Status = NtCompleteConnectPort(PortHandle);
   if (!NT_SUCCESS(Status))
     {
	printf("(lpcsrv.exe) Failed to complete connection (Status = 0x%08X)\n", Status);
	return EXIT_FAILURE;
     }
   
   for(;;)
     {
	LPCMESSAGE Request;
	
	Status = NtReplyWaitReceivePort(PortHandle,
					0,
					NULL,
					&Request);
	if (!NT_SUCCESS(Status))
	  {
	     printf("(lpcsrv.exe) Failed to receive request (Status = 0x%08X)\n", Status);
             return EXIT_FAILURE;
	  }
	
	printf("(lpcsrv.exe) Message contents are <%s>\n", Request.MessageData);
     }
   return EXIT_SUCCESS;
}


/* EOF */
