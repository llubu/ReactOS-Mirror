/* $Id: pci.c,v 1.6 2002/12/09 23:15:57 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/pci.c
 * PURPOSE:         Interfaces to the PCI bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  05/06/1998: Created
 *                  17/08/2000: Added preliminary pci bus scanner
 *                  13/06/2001: Implemented access to pci configuration space
 */

/*
 * NOTES: Sections copied from the Linux pci support
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <bus.h>

#define NDEBUG
#include <internal/debug.h>


/* MACROS ******************************************************************/

/* access type 1 macros */
#define CONFIG_CMD(bus, dev_fn, where) \
	(0x80000000 | (((ULONG)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))

/* access type 2 macros */
#define IOADDR(dev_fn, where) \
	(0xC000 | (((dev_fn) & 0x1F) << 8) | (where))
#define FUNC(dev_fn) \
	((((dev_fn) & 0xE0) >> 4) | 0xf0)


/* GLOBALS ******************************************************************/

static ULONG BusConfigType = 0;  /* undetermined config type */
static KSPIN_LOCK PciLock;

/* FUNCTIONS ****************************************************************/

static NTSTATUS
ReadPciConfigUchar(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PUCHAR Value)
{
   KIRQL oldIrql;

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_UCHAR((PUCHAR)0xCFC + (Offset & 3));
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_UCHAR((PUCHAR)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
ReadPciConfigUshort(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    PUSHORT Value)
{
   KIRQL oldIrql;

   if ((Offset & 1) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_USHORT((PUSHORT)0xCFC + (Offset & 1));
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
ReadPciConfigUlong(UCHAR Bus,
		   UCHAR Slot,
		   UCHAR Offset,
		   PULONG Value)
{
   KIRQL oldIrql;

   if ((Offset & 3) != 0)
     {
	return STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	*Value = READ_PORT_ULONG((PULONG)0xCFC);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	*Value = READ_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)));
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUchar(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    UCHAR Value)
{
   KIRQL oldIrql;

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_UCHAR((PUCHAR)0xCFC + (Offset&3), Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_UCHAR((PUCHAR)(IOADDR(Slot,Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUshort(UCHAR Bus,
		     UCHAR Slot,
		     UCHAR Offset,
		     USHORT Value)
{
   KIRQL oldIrql;

   if ((Offset & 1) != 0)
     {
	return  STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_USHORT((PUSHORT)0xCFC + (Offset & 1), Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_USHORT((PUSHORT)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
WritePciConfigUlong(UCHAR Bus,
		    UCHAR Slot,
		    UCHAR Offset,
		    ULONG Value)
{
   KIRQL oldIrql;

   if ((Offset & 3) != 0)
     {
	return  STATUS_INVALID_PARAMETER;
     }

   switch (BusConfigType)
     {
     case 1:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus, Slot, Offset));
	WRITE_PORT_ULONG((PULONG)0xCFC, Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;

     case 2:
        KeAcquireSpinLock(&PciLock, &oldIrql);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(Slot));
	WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
	WRITE_PORT_ULONG((PULONG)(IOADDR(Slot, Offset)), Value);
	WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);
	KeReleaseSpinLock(&PciLock, oldIrql);
	return STATUS_SUCCESS;
     }
   return STATUS_UNSUCCESSFUL;
}


static ULONG STDCALL
HalpGetPciData(PBUS_HANDLER BusHandler,
	       ULONG BusNumber,
	       ULONG SlotNumber,
	       PVOID Buffer,
	       ULONG Offset,
	       ULONG Length)
{
   PVOID Ptr = Buffer;
   ULONG Address = Offset;
   ULONG Len = Length;
   ULONG Vendor;
   UCHAR HeaderType;

   DPRINT("HalpGetPciData() called.\n");
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);

   if ((Length == 0) || (BusConfigType == 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber & 0x1F,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
   {
     if (BusNumber == 0 && Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }

   /* 0E=PCI_HEADER_TYPE */
   ReadPciConfigUchar(BusNumber,
		      SlotNumber & 0x1F,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & PCI_MULTIFUNCTION) == 0) && ((SlotNumber & 0xE0) != 0))
   {
     if (Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }
   ReadPciConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
   {
     if (BusNumber == 0 && Offset == 0 && Length >= 2)
     {
	*(PUSHORT)Buffer = PCI_INVALID_VENDORID;
	return 2;
     }
     return 0;
   }

   if ((Address & 1) && (Len >= 1))
     {
	ReadPciConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	ReadPciConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	ReadPciConfigUlong(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	ReadPciConfigUshort(BusNumber,
			    SlotNumber,
			    Address,
			    Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	ReadPciConfigUchar(BusNumber,
			   SlotNumber,
			   Address,
			   Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


static ULONG STDCALL
HalpSetPciData(PBUS_HANDLER BusHandler,
	       ULONG BusNumber,
	       ULONG SlotNumber,
	       PVOID Buffer,
	       ULONG Offset,
	       ULONG Length)
{
   PVOID Ptr = Buffer;
   ULONG Address = Offset;
   ULONG Len = Length;
   ULONG Vendor;
   UCHAR HeaderType;

   DPRINT("HalpSetPciData() called.\n");
   DPRINT("  BusNumber %lu\n", BusNumber);
   DPRINT("  SlotNumber %lu\n", SlotNumber);
   DPRINT("  Offset 0x%lx\n", Offset);
   DPRINT("  Length 0x%lx\n", Length);

   if ((Length == 0) || (BusConfigType == 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber & 0x1F,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;


   /* 0E=PCI_HEADER_TYPE */
   ReadPciConfigUchar(BusNumber,
		      SlotNumber & 0x1F,
		      0x0E,
		      &HeaderType);
   if (((HeaderType & PCI_MULTIFUNCTION) == 0) && ((SlotNumber & 0xE0) != 0))
     return 0;

   ReadPciConfigUlong(BusNumber,
		      SlotNumber,
		      0x00,
		      &Vendor);
   /* some broken boards return 0 if a slot is empty: */
   if (Vendor == 0xFFFFFFFF || Vendor == 0)
     return 0;

   if ((Address & 1) && (Len >= 1))
     {
	WritePciConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   if ((Address & 2) && (Len >= 2))
     {
	WritePciConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   while (Len >= 4)
     {
	WritePciConfigUlong(BusNumber,
			    SlotNumber,
			    Address,
			    *(PULONG)Ptr);
	Ptr = Ptr + 4;
	Address += 4;
	Len -= 4;
     }

   if (Len >= 2)
     {
	WritePciConfigUshort(BusNumber,
			     SlotNumber,
			     Address,
			     *(PUSHORT)Ptr);
	Ptr = Ptr + 2;
	Address += 2;
	Len -= 2;
     }

   if (Len >= 1)
     {
	WritePciConfigUchar(BusNumber,
			    SlotNumber,
			    Address,
			    *(PUCHAR)Ptr);
	Ptr = Ptr + 1;
	Address++;
	Len--;
     }

   return Length - Len;
}


static ULONG
GetBusConfigType(VOID)
{
   ULONG Value;
   KIRQL oldIrql;

   DPRINT("GetBusConfigType() called\n");

   KeAcquireSpinLock(&PciLock, &oldIrql);
 
   DPRINT("Checking configuration type 1:");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x01);
   Value = READ_PORT_ULONG((PULONG)0xCF8);
   WRITE_PORT_ULONG((PULONG)0xCF8, 0x80000000);
   if (READ_PORT_ULONG((PULONG)0xCF8) == 0x80000000)
     {
	WRITE_PORT_ULONG((PULONG)0xCF8, Value);
	KeReleaseSpinLock(&PciLock, oldIrql);
	DPRINT("  Success!\n");
	return 1;
     }
   WRITE_PORT_ULONG((PULONG)0xCF8, Value);
   DPRINT("  Unsuccessful!\n");

   DPRINT("Checking configuration type 2:");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, 0x00);
   if (READ_PORT_UCHAR((PUCHAR)0xCF8) == 0x00 &&
       READ_PORT_UCHAR((PUCHAR)0xCFB) == 0x00)
     {
	KeReleaseSpinLock(&PciLock, oldIrql);
	DPRINT("  Success!\n");
	return 2;
     }
   KeReleaseSpinLock(&PciLock, oldIrql);
   DPRINT("  Unsuccessful!\n");

   DPRINT("No pci bus found!\n");
   return 0;
}


static ULONG STDCALL
HalpGetPciInterruptVector(PVOID BusHandler,
			  ULONG BusNumber,
			  ULONG BusInterruptLevel,
			  ULONG BusInterruptVector,
			  PKIRQL Irql,
			  PKAFFINITY Affinity)
{
  *Irql = PROFILE_LEVEL - BusInterruptVector;
  *Affinity = 0xFFFFFFFF;
  return BusInterruptVector;
}

static BOOLEAN STDCALL
HalpTranslatePciAddress(PBUS_HANDLER BusHandler,
			ULONG BusNumber,
			PHYSICAL_ADDRESS BusAddress,
			PULONG AddressSpace,
			PPHYSICAL_ADDRESS TranslatedAddress)
{
   if (*AddressSpace == 0)
     {
	/* memory space */

     }
   else if (*AddressSpace == 1)
     {
	/* io space */

     }
   else
     {
	/* other */
	return FALSE;
     }

   TranslatedAddress->QuadPart = BusAddress.QuadPart;

   return TRUE;
}


VOID
HalpInitPciBus(VOID)
{
  PBUS_HANDLER BusHandler;

  DPRINT("HalpInitPciBus() called.\n");

  KeInitializeSpinLock (&PciLock);

  BusConfigType = GetBusConfigType();
  if (BusConfigType == 0)
    return;

  DPRINT("Bus configuration %lu used\n", BusConfigType);

  /* pci bus (bus 0) handler */
  BusHandler = HalpAllocateBusHandler(PCIBus,
				      PCIConfiguration,
				      0);
  BusHandler->GetBusData = (pGetSetBusData)HalpGetPciData;
  BusHandler->SetBusData = (pGetSetBusData)HalpSetPciData;
  BusHandler->GetInterruptVector =
    (pGetInterruptVector)HalpGetPciInterruptVector;
  BusHandler->TranslateBusAddress = 
    (pTranslateBusAddress)HalpTranslatePciAddress;
//	BusHandler->AdjustResourceList =
//		(pGetSetBusData)HalpAdjustPciResourceList;
//	BusHandler->AssignSlotResources =
//		(pGetSetBusData)HalpAssignPciSlotResources;


  /* agp bus (bus 1) handler */
  BusHandler = HalpAllocateBusHandler(PCIBus,
				      PCIConfiguration,
				      1);
  BusHandler->GetBusData = (pGetSetBusData)HalpGetPciData;
  BusHandler->SetBusData = (pGetSetBusData)HalpSetPciData;
  BusHandler->GetInterruptVector =
    (pGetInterruptVector)HalpGetPciInterruptVector;
  BusHandler->TranslateBusAddress = 
    (pTranslateBusAddress)HalpTranslatePciAddress;
//	BusHandler->AdjustResourceList =
//		(pGetSetBusData)HalpAdjustPciResourceList;
//	BusHandler->AssignSlotResources =
//		(pGetSetBusData)HalpAssignPciSlotResources;

  DPRINT("HalpInitPciBus() finished.\n");
}

/* EOF */
