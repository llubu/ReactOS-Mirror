/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/free.c
 * PURPOSE:     DNSAPI functions built on the ADNS library.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include <windows.h>
#include <WinError.h>
#include <WinDNS.h>
#include <internal/windns.h>

VOID WINAPI DnsFree
( PVOID Data, DNS_FREE_TYPE FreeType ) {
  switch( FreeType ) {
  case DnsFreeFlat:
    RtlFreeHeap( RtlGetProcessHeap(), 0, Data );
    break;

  case DnsFreeRecordList:
    DnsIntFreeRecordList( (PDNS_RECORDA)Data );
    break;

  case DnsFreeParsedMessageFields:
    /* assert( FALSE ); XXX arty not yet implemented. */
    break;
  }
}

VOID WINAPI DnsRecordListFree
( PVOID Data, DNS_FREE_TYPE FreeType ) {
  DnsFree( Data, FreeType );
}
