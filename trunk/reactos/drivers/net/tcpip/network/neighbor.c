/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/neighbor.c
 * PURPOSE:     Neighbor address cache
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


NEIGHBOR_CACHE_TABLE NeighborCache[NB_HASHMASK + 1];


VOID FreeNCE(
  PVOID Object)
{
  ExFreePool(Object);
}

VOID NCETimeout(
  PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Neighbor cache entry timeout handler
 * NOTES:
 *   The neighbor cache lock must be held
 */
{
  PNDIS_PACKET NdisPacket;
  PNDIS_PACKET NextNdisPacket;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));

  TI_DbgPrint(DEBUG_NCACHE, ("NCE->State is (0x%X).\n", NCE->State));

  switch (NCE->State)
    {
      case NUD_INCOMPLETE:
        /* Retransmission timer expired */
        if (NCE->EventCount++ > MAX_MULTICAST_SOLICIT)
          {
            /* We have retransmitted too many times */

            /* Calling IPSendComplete with cache lock held is not
              a great thing to do. We don't get here very often
              so maybe it's not that big a problem */

            /* Flush packet queue */
            NdisPacket = NCE->WaitQueue;
            while (NdisPacket != NULL)
              {
                NextNdisPacket = (PNDIS_PACKET)PC(NdisPacket)->DLComplete;
                IPSendComplete((PVOID)NCE->Interface,
                  NdisPacket,
                  NDIS_STATUS_REQUEST_ABORTED);
                NdisPacket = NextNdisPacket;
              }
            NCE->WaitQueue = NULL;

            NCE->EventCount = 0;

            /* Remove route cache entries with references to this NCE.
              Remember that neighbor cache lock is acquired before the
              route cache lock */
            RouteInvalidateNCE(NCE);
          }
        else
          {
            /* Retransmit request */
            NBSendSolicit(NCE);
          }
        break;

      case NUD_DELAY:
        /* FIXME: Delayed state */
        TI_DbgPrint(DEBUG_NCACHE, ("NCE delay state.\n"));
        break;

      case NUD_PROBE:
        /* FIXME: Probe state */
        TI_DbgPrint(DEBUG_NCACHE, ("NCE probe state.\n"));
        break;

      default:
        /* Should not happen since the event timer is not used in the other states */
        TI_DbgPrint(MIN_TRACE, ("Invalid NCE state (%d).\n", NCE->State));
        break;
    }
}


VOID NBTimeout(
    VOID)
/*
 * FUNCTION: Neighbor address cache timeout handler
 * NOTES:
 *     This routine is called by IPTimeout to remove outdated cache
 *     entries.
 */
{
    UINT i;
    KIRQL OldIrql;
    PNEIGHBOR_CACHE_ENTRY NCE;

    for (i = 0; i <= NB_HASHMASK; i++) {
        KeAcquireSpinLock(&NeighborCache[i].Lock, &OldIrql);

        for (NCE = NeighborCache[i].Cache;
            NCE != NULL; NCE = NCE->Next) {
            /* Check if event timer is running */
            if (NCE->EventTimer > 0)  {
                NCE->EventTimer--;
                if (NCE->EventTimer == 0) {
                    /* Call timeout handler for NCE */
                    NCETimeout(NCE);
                }
            }
        }

        KeReleaseSpinLock(&NeighborCache[i].Lock, OldIrql);
    }
}

VOID NBStartup(
  VOID)
/*
 * FUNCTION: Starts the neighbor cache
 */
{
  UINT i;

  TI_DbgPrint(DEBUG_NCACHE, ("Called.\n"));

  for (i = 0; i <= NB_HASHMASK; i++)
    {
      NeighborCache[i].Cache = NULL;
      KeInitializeSpinLock(&NeighborCache[i].Lock);
    }
}

VOID NBShutdown(
  VOID)
/*
 * FUNCTION: Shuts down the neighbor cache
 */
{
  PNEIGHBOR_CACHE_ENTRY NextNCE;
  PNEIGHBOR_CACHE_ENTRY CurNCE;
  PNDIS_PACKET NextNdisPacket;
  PNDIS_PACKET NdisPacket;
  KIRQL OldIrql;
  UINT i;

  TI_DbgPrint(DEBUG_NCACHE, ("Called.\n"));

  /* Remove possible entries from the cache */
  for (i = 0; i <= NB_HASHMASK; i++)
    {
      KeAcquireSpinLock(&NeighborCache[i].Lock, &OldIrql);

      CurNCE = NeighborCache[i].Cache;
      while (CurNCE)
        {
          NextNCE = CurNCE->Next;

          /* Remove all references from route cache */
          RouteInvalidateNCE(CurNCE);

          /* Flush wait queue */
          NdisPacket = CurNCE->WaitQueue;
          while (NdisPacket)
            {
              NextNdisPacket = (PNDIS_PACKET)PC(NdisPacket)->DLComplete;
              FreeNdisPacket(NdisPacket);
              NdisPacket = NextNdisPacket;
            }

#ifdef DBG
          if (CurNCE->RefCount != 1)
            {
              TI_DbgPrint(DEBUG_REFCOUNT, ("NCE at (0x%X) has (%d) references (should be 1).\n", CurNCE, CurNCE->RefCount));
            }
#endif

        /* Remove reference for being alive */
        DereferenceObject(CurNCE);

        CurNCE = NextNCE;
      }

    NeighborCache[i].Cache = NULL;

    KeReleaseSpinLock(&NeighborCache[i].Lock, OldIrql);
  }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

VOID NBSendSolicit(
  PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Sends a neighbor solicitation message
 * ARGUMENTS:
 *   NCE = Pointer to NCE of neighbor to solicit
 * NOTES:
 *   May be called with lock held on NCE's table
 */
{
  PLIST_ENTRY CurrentEntry;
  PNET_TABLE_ENTRY NTE;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));

  if (NCE->State == NUD_INCOMPLETE)
    {
      /* This is the first solicitation of this neighbor. Broadcast
          a request for the neighbor */

      /* FIXME: Choose first NTE. We might want to give an NTE as argument */
      if (!NCE->Interface || !NCE->Interface->NTEListHead.Flink) {
	  TI_DbgPrint(MID_TRACE, 
		      ("NCE->Interface: %x, "
		       "NCE->Interface->NTEListHead.Flink %x\n",
		       NCE->Interface,
		       NCE->Interface ? NCE->Interface->NTEListHead.Flink : 0));
      }
      if (!IsListEmpty(&NCE->Interface->NTEListHead))
        {
	    CurrentEntry = NCE->Interface->NTEListHead.Flink;
	    NTE = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, 
				    IFListEntry);
	    ARPTransmit(&NCE->Address, NTE);
        }
      else
        {
	    TI_DbgPrint(MIN_TRACE, ("Interface at 0x%X has zero NTE.\n", 
				    NCE->Interface));
        }
    }
  else
    {
      /* FIXME: Unicast solicitation since we have a cached address */
      TI_DbgPrint(MIN_TRACE, ("Uninplemented unicast solicitation.\n"));
    }
}

PNEIGHBOR_CACHE_ENTRY NBAddNeighbor(
  PIP_INTERFACE Interface,
  PIP_ADDRESS Address,
  PVOID LinkAddress,
  UINT LinkAddressLength,
  UCHAR State)
/*
 * FUNCTION: Adds a neighbor to the neighbor cache
 * ARGUMENTS:
 *   Interface         = Pointer to interface
 *   Address           = Pointer to IP address
 *   LinkAddress       = Pointer to link address (may be NULL)
 *   LinkAddressLength = Length of link address
 *   State             = State of NCE
 * RETURNS:
 *   Pointer to NCE, NULL there is not enough free resources
 * NOTES:
 *   The NCE if referenced for the caller if created. The NCE retains
 *   a reference to the IP address if it is created, the caller is
 *   responsible for providing this reference
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;
  ULONG HashValue;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. Interface (0x%X)  Address (0x%X)  "
    "LinkAddress (0x%X)  LinkAddressLength (%d)  State (0x%X)\n",
    Interface, Address, LinkAddress, LinkAddressLength, State));

  ASSERT(Address->Type == IP_ADDRESS_V4);

  NCE = ExAllocatePool(NonPagedPool, sizeof(NEIGHBOR_CACHE_ENTRY) + LinkAddressLength);
  if (NCE == NULL)
    {
      TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return NULL;
    }

  INIT_TAG(NCE, TAG('N','C','E',' '));

  /* Initialize NCE free routine */
  NCE->Free = FreeNCE;

  /* Reference once for beeing alive and once for the caller */
  NCE->RefCount = 2;
  NCE->Interface = Interface;
  NCE->Address = *Address;
  NCE->LinkAddressLength = LinkAddressLength;
  NCE->LinkAddress = (PVOID)&NCE[1];
  if (LinkAddress != NULL)
    {
      RtlCopyMemory(NCE->LinkAddress, LinkAddress, LinkAddressLength);
    }
  NCE->State = State;
  NCE->EventTimer = 0; /* Not in use */
  NCE->WaitQueue = NULL;

  HashValue = *(PULONG)&Address->Address;
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  NCE->Table = &NeighborCache[HashValue];

  KeAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  NCE->Next = NeighborCache[HashValue].Cache;
  NeighborCache[HashValue].Cache = NCE;

  KeReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

  return NCE;
}

VOID NBUpdateNeighbor(
  PNEIGHBOR_CACHE_ENTRY NCE,
  PVOID LinkAddress,
  UCHAR State)
/*
 * FUNCTION: Update link address information in NCE
 * ARGUMENTS:
 *   NCE         = Pointer to NCE to update
 *   LinkAddress = Pointer to link address
 *   State       = State of NCE
 * NOTES:
 *   The link address and state is updated. Any waiting packets are sent
 */
{
  PNDIS_PACKET Current;
  PNDIS_PACKET Next;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X)  LinkAddress (0x%X)  State (0x%X).\n", NCE, LinkAddress, State));

  KeAcquireSpinLock(&NCE->Table->Lock, &OldIrql);

  RtlCopyMemory(NCE->LinkAddress, LinkAddress, NCE->LinkAddressLength);
  NCE->State = State;
  Current = NCE->WaitQueue;
  NCE->WaitQueue = NULL;

  KeReleaseSpinLock(&NCE->Table->Lock, OldIrql);

  /* Send any waiting packets */
  while (Current != NULL)
    {
      /* Our link to the next packet is broken by the
         datalink layer code so we must save it here */
      Next = (PNDIS_PACKET)PC(Current)->DLComplete;
      IPSendFragment(Current, NCE);
      Current = Next;
    }
}

PNEIGHBOR_CACHE_ENTRY NBLocateNeighbor(
  PIP_ADDRESS Address)
/*
 * FUNCTION: Locates a neighbor in the neighbor cache
 * ARGUMENTS:
 *   Address = Pointer to IP address
 * RETURNS:
 *   Pointer to NCE, NULL if not found
 * NOTES:
 *   If the NCE is found, it is referenced. The caller is
 *   responsible for dereferencing it again after use
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;
  UINT HashValue;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. Address (0x%X).\n", Address));

  HashValue = *(PULONG)&Address->Address;
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  KeAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  NCE = NeighborCache[HashValue].Cache;

  while ((NCE) && (!AddrIsEqual(Address, &NCE->Address)))
    {
      NCE = NCE->Next;
    }

  if (NCE)
    {
      ReferenceObject(NCE);
    }

  KeReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return NCE;
}

PNEIGHBOR_CACHE_ENTRY NBFindOrCreateNeighbor(
  PIP_INTERFACE Interface,
  PIP_ADDRESS Address)
/*
 * FUNCTION: Tries to find a neighbor and if unsuccesful, creates a new NCE
 * ARGUMENTS:
 *   Interface = Pointer to interface to use (in case NCE is not found)
 *   Address   = Pointer to IP address
 * RETURNS:
 *   Pointer to NCE, NULL if there is not enough free resources
 * NOTES:
 *   The NCE is referenced if found or created. The caller is
 *   responsible for dereferencing it again after use
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. Interface (0x%X)  Address (0x%X).\n", Interface, Address));

  NCE = NBLocateNeighbor(Address);
  if (NCE == NULL)
    {
      ReferenceObject(Address);
      NCE = NBAddNeighbor(Interface, Address, NULL, 
			  Interface->AddressLength, NUD_INCOMPLETE);
      NCE->EventTimer = 1;
      NCE->EventCount = 0;
    }

  return NCE;
}

BOOLEAN NBQueuePacket(
  PNEIGHBOR_CACHE_ENTRY NCE,
  PNDIS_PACKET NdisPacket)
/*
 * FUNCTION: Queues a packet on an NCE for later transmission
 * ARGUMENTS:
 *   NCE        = Pointer to NCE to queue packet on
 *   NdisPacket = Pointer to NDIS packet to queue
 * RETURNS:
 *   TRUE if the packet was successfully queued, FALSE if not
 */
{
  PKSPIN_LOCK Lock;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X)  NdisPacket (0x%X).\n", NCE, NdisPacket));

  /* FIXME: Should we limit the number of queued packets? */

  Lock = &NCE->Table->Lock;

  KeAcquireSpinLock(Lock, &OldIrql);

  /* Use data link level completion handler pointer to link
     queued packets together */
  PC(NdisPacket)->DLComplete = (PACKET_COMPLETION_ROUTINE)NCE->WaitQueue;
  NCE->WaitQueue = NdisPacket;

  KeReleaseSpinLock(Lock, OldIrql);

  return TRUE;
}


VOID NBRemoveNeighbor(
  PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Removes a neighbor from the neighbor cache
 * ARGUMENTS:
 *   NCE = Pointer to NCE to remove from cache
 * NOTES:
 *   The NCE must be in a safe state
 */
{
  PNEIGHBOR_CACHE_ENTRY *PrevNCE;
  PNEIGHBOR_CACHE_ENTRY CurNCE;
  PNDIS_PACKET NextNdisPacket;
  PNDIS_PACKET NdisPacket;
  ULONG HashValue;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));

  HashValue  = *(PULONG)(&NCE->Address.Address);
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  KeAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  /* Search the list and remove the NCE from the list if found */
  for (PrevNCE = &NeighborCache[HashValue].Cache;
    (CurNCE = *PrevNCE) != NULL;
    PrevNCE = &CurNCE->Next)
    {
      if (CurNCE == NCE)
        {
          /* Found it, now unlink it from the list */
          *PrevNCE = CurNCE->Next;

          /* Purge wait queue */
          NdisPacket = CurNCE->WaitQueue;
          while (NdisPacket != NULL)
            {
              NextNdisPacket = (PNDIS_PACKET)PC(NdisPacket)->DLComplete;
              FreeNdisPacket(NdisPacket);
              NdisPacket = NextNdisPacket;
            }

          /* Remove all references from route cache */
          RouteInvalidateNCE(CurNCE);

#ifdef DBG
          CurNCE->RefCount--;

          if (CurNCE->RefCount != 0)
            {
              TI_DbgPrint(DEBUG_REFCOUNT, ("NCE at (0x%X) has (%d) references (should be 0).\n",
                CurNCE, CurNCE->RefCount));
            }
#endif
          ExFreePool(CurNCE);

          KeReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

          return;
        }
    }

  KeReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);
}
