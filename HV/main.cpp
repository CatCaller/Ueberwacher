#include <ntddk.h>
#include "hv.h"
#include "wrappers.h"

#pragma section (".Ueberwacher", read)
#pragma comment(linker, "/merge:.Ueberwacher=.text")
extern "C" __declspec(allocate(".Ueberwacher")) int ajvisor_signature = 0x69420; 

void driver_unload(PDRIVER_OBJECT);

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object = nullptr, PUNICODE_STRING registry_path = nullptr)
{

	UNREFERENCED_PARAMETER(registry_path);

	if(driver_object)
		driver_object->DriverUnload = driver_unload;

	if (!check_svm_support())
		return STATUS_UNSUCCESSFUL;

	virtualize();

	return STATUS_SUCCESS;
}

void driver_unload(PDRIVER_OBJECT)
{
	print("Driver Unload \n");
	unvirtualize();
}
