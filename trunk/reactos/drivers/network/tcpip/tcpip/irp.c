/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/dispatch.h
 * PURPOSE:     TDI dispatch routines
 * PROGRAMMERS: arty
 * REVISIONS:
 *   CSH 01/08-2000 Created
 * TODO:        Validate device object in all dispatch routines
 */

#include "precomp.h"

VOID IRPRemember( PIRP Irp, PCHAR File, UINT Line ) {
#ifdef MEMTRACK
    TrackWithTag( IRP_TAG, Irp, File, Line );
#endif
}

NTSTATUS IRPFinish( PIRP Irp, NTSTATUS Status ) {
    KIRQL Irql;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //DbgPrint("Called: Irp %x, Status %x Event %x\n", Irp, Status, Irp->UserEvent);

#ifdef MEMTRACK
    UntrackFL( __FILE__, __LINE__, Irp );
#endif

    (void)IoSetCancelRoutine( Irp, NULL );

    if( Status == STATUS_PENDING )
	IoMarkIrpPending( Irp );
    else {
	Irp->IoStatus.Status = Status;
	Irql = KeGetCurrentIrql();

	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	if (KeGetCurrentIrql() != Irql) {
	    DbgPrint("WARNING: IO COMPLETION RETURNED AT WRONG IRQL:\n");
	    DbgPrint("WARNING: IRP TYPE WAS %d\n", IrpSp->MajorFunction);
	}
	ASSERT(KeGetCurrentIrql() == Irql);
    }

    return Status;
}

