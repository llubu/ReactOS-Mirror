/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/hardware.c
 * PURPOSE:     Hardware related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   25 Aug 2003 Vizzini - NDIS4/5 and PnP additions
 *   3  Oct 2003 Vizzini - formatting and minor bugfixes
 *
 * TODO:
 *    - Fix HalGetBusDataByOffset() param 2 in most calls below
 */
#include <ndissys.h>
#include <miniport.h>


/*
 * @implemented
 */
ULONG
EXPORT
NdisImmediateReadPciSlotInformation(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  return HalGetBusDataByOffset (PCIConfiguration, 0, SlotNumber, Buffer, Offset, Length);
}


/*
 * @implemented
 */
ULONG 
EXPORT
NdisImmediateWritePciSlotInformation(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  return HalSetBusDataByOffset (PCIConfiguration, 0, SlotNumber, Buffer, Offset, Length);
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMPciAssignResources(
    IN  NDIS_HANDLE             MiniportHandle,
    IN  ULONG                   SlotNumber,
    OUT PNDIS_RESOURCE_LIST     *AssignedResources)
/*
 * NOTES:
 *     - I think this is fundamentally broken
 */
{
  PCM_RESOURCE_LIST ResourceList;
  NTSTATUS Status;
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportHandle;

  ResourceList = NULL;
  Status = HalAssignSlotResources (Adapter->Miniport->RegistryPath,
				   0,
				   Adapter->Miniport->DriverObject,
				   0,
				   PCIBus,
				   Adapter->BusNumber,
				   SlotNumber,
				   &ResourceList);
  if (!NT_SUCCESS (Status))
    {
      *AssignedResources = NULL;
      return NDIS_STATUS_FAILURE;
    }

  *AssignedResources = (PNDIS_RESOURCE_LIST)&ResourceList->List[0].PartialResourceList;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMQueryAdapterResources(
    OUT     PNDIS_STATUS        Status,
    IN      NDIS_HANDLE         WrapperConfigurationContext,
    OUT     PNDIS_RESOURCE_LIST ResourceList,
    IN OUT  PUINT               BufferSize)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisQueryMapRegisterCount(
    IN  NDIS_INTERFACE_TYPE BusType,
    OUT PUINT               MapRegisterCount)
/*
 * On X86 (and all other current hardware), map registers aren't real hardware,
 * and there is no real limit to the number that can be allocated.
 * As such, we do what microsoft does on the x86 hals and return as follows
 */
{
  return NDIS_STATUS_NOT_SUPPORTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReadEisaSlotInformation(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE			            WrapperConfigurationContext,
    OUT PUINT                           SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION EisaData)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReadEisaSlotInformationEx(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    OUT PUINT                           SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION *EisaData,
    OUT PUINT                           NumberOfFunctions)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisReadPciSlotInformation(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  return HalGetBusDataByOffset (PCIConfiguration, 0, SlotNumber, Buffer, Offset, Length);
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisWritePciSlotInformation(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  return HalSetBusDataByOffset (PCIConfiguration, 0, SlotNumber, Buffer, Offset, Length);
}

/* EOF */

