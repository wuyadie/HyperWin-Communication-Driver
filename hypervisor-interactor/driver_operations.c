#include "driver_operations.h"
#include "hyperwin_structs.h"
#include "utils.h"
#include "x86_64.h"

NTSTATUS HyperWinCreate(IN PDEVICE_OBJECT pDeviceObj, IN PIRP Irp)
{
	//
	// When a create call is being used, we need to make sure that the communication block
	// is mapped in to the kernel's virtual memory
	//
	UNREFERENCED_PARAMETER(Irp);
	PHYPERWIN_MAIN_DATA pData = (PHYPERWIN_MAIN_DATA)pDeviceObj->DeviceExtension;
	if (!(pData->IsMapped))
	{
		KIRQL OldIrql;
		KeAcquireSpinLock(&(pData->OperationSpinLock), &OldIrql);
		int Values[4];
		__cpuidex(Values, 0x40020020, 0);
		//
		// The hypervisor will store the result in EDX:EAX
		//
		DWORD64 PhysicalAddress = ((DWORD64)Values[3] << 32) | (Values[0]);
		hvPrint("Got physical address: %llx\n", PhysicalAddress);
		pData->PhysicalCommunicationBaseAddress = PhysicalAddress;
		pData->CommunicationBlockSize = LARGE_PAGE_SIZE;
		//
		// Map the memory to the kernel's virtual address
		//
		PHYSICAL_ADDRESS pa;
		pa.QuadPart = PhysicalAddress;
		pData->VirtualCommunicationBlockAddress = MmMapIoSpace(pa, LARGE_PAGE_SIZE, MmCached);
		hvPrint("Kernel virtual address: %llx\n", (DWORD64)pData->VirtualCommunicationBlockAddress);
		KeReleaseSpinLock(&(pData->OperationSpinLock), OldIrql);
	}

	return STATUS_SUCCESS;
}