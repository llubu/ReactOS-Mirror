/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/interface.c
 * PURPOSE:     Convenient abstraction for getting and setting information
 *              in IP_INTERFACE.
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface, 
				  ULONG TargetType,
				  PULONG Address ) {
    switch( TargetType ) {
    case ADE_UNICAST:
	*Address = Interface->Unicast.Address.IPv4Address;
	break;

    case ADE_ADDRMASK:
	*Address = Interface->Netmask.Address.IPv4Address;
	break;

    case ADE_BROADCAST:
	*Address = 
	    Interface->Unicast.Address.IPv4Address | 
	    ~Interface->Netmask.Address.IPv4Address;
	break;

    case ADE_POINTOPOINT:
	*Address = Interface->PointToPoint.Address.IPv4Address;
	break;

    default:
	return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

UINT CountInterfaces() {
    DWORD Count = 0;
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    ForEachInterface(CurrentIF) {
	Count++;
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Count;
}

NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed ) {
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    
#ifdef __NTDRIVER__
    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          Speed,
                          sizeof(UINT));
#else
    (void)IF;
    NdisStatus = NDIS_STATUS_SUCCESS;
    *Speed = 10000;
#endif
    
    return 
	NdisStatus != NDIS_STATUS_SUCCESS ? 
	STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

NTSTATUS GetInterfaceName( PIP_INTERFACE Interface, 
			   PCHAR NameBuffer,
			   UINT Len ) {
    ULONG ResultSize = 0;
    NTSTATUS Status = 
	RtlUnicodeToMultiByteN( NameBuffer, 
				Len, 
				&ResultSize, 
				Interface->Name.Buffer,
				Interface->Name.Length );
    
    if( NT_SUCCESS(Status) ) 
	NameBuffer[ResultSize] = 0;
    else
	NameBuffer[0] = 0;

    return Status;
}

/*
 * FUNCTION: Locates and returns an address entry using IPv4 adress as argument
 * ARGUMENTS:
 *     Address = Raw IPv4 address
 * RETURNS:
 *     Pointer to address entry if found, NULL if not found
 * NOTES:
 *     Only unicast addresses are considered.
 *     If found, the address is referenced
 */
BOOLEAN AddrLocateADEv4(
    IPv4_RAW_ADDRESS MatchAddress, PIP_ADDRESS Address)
{
    KIRQL OldIrql;
    BOOLEAN Matched = FALSE;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    ForEachInterface(CurrentIF) {
	if( AddrIsEqualIPv4( &CurrentIF->Unicast, MatchAddress ) ) {
	    Address->Address.IPv4Address = MatchAddress;
	    Matched = TRUE; break;
	}
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Matched;
}

BOOLEAN IPGetDefaultAddress( PIP_ADDRESS Address ) {
    KIRQL OldIrql;
    BOOLEAN Matched = FALSE;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    /* Find the first 'real' interface */
    ForEachInterface(CurrentIF) {
	if( CurrentIF->Context ) {
	    *Address = CurrentIF->Unicast;
	    Matched = TRUE; break;
	}
    } EndFor(CurrentIF);

    /* Not matched, use the first one */
    if( !Matched ) {
	ForEachInterface(CurrentIF) {
	    *Address = CurrentIF->Unicast;
	    Matched = TRUE; break;
	} EndFor(CurrentIF);
    }

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Matched;
}

BOOLEAN HasPrefix(
    PIP_ADDRESS Address,
    PIP_ADDRESS Prefix,
    UINT Length)
/*
 * FUNCTION: Determines wether an address has an given prefix
 * ARGUMENTS:
 *     Address = Pointer to address to use
 *     Prefix  = Pointer to prefix to check for
 *     Length  = Length of prefix
 * RETURNS:
 *     TRUE if the address has the prefix, FALSE if not
 * NOTES:
 *     The two addresses must be of the same type
 */
{
    PUCHAR pAddress = (PUCHAR)&Address->Address;
    PUCHAR pPrefix  = (PUCHAR)&Prefix->Address;

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Address (0x%X)  Prefix (0x%X)  Length (%d).\n", Address, Prefix, Length));

#if 0
    TI_DbgPrint(DEBUG_ROUTER, ("Address (%s)  Prefix (%s).\n",
        A2S(Address), A2S(Prefix)));
#endif

    /* Check that initial integral bytes match */
    while (Length > 8) {
        if (*pAddress++ != *pPrefix++)
            return FALSE;
        Length -= 8;
    } 

    /* Check any remaining bits */
    if ((Length > 0) && ((*pAddress >> (8 - Length)) != (*pPrefix >> (8 - Length))))
        return FALSE;

    return TRUE;
}

PIP_INTERFACE FindOnLinkInterface(PIP_ADDRESS Address)
/*
 * FUNCTION: Checks all on-link prefixes to find out if an address is on-link
 * ARGUMENTS:
 *     Address = Pointer to address to check
 * RETURNS:
 *     Pointer to interface if address is on-link, NULL if not
 */
{
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Address (0x%X)\n", Address));
    TI_DbgPrint(DEBUG_ROUTER, ("Address (%s)\n", A2S(Address)));

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    ForEachInterface(CurrentIF) {
        if (HasPrefix(Address, &CurrentIF->Unicast,
		      AddrCountPrefixBits(&CurrentIF->Netmask))) {
	    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);
            return CurrentIF;
	}
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return NULL;
}
