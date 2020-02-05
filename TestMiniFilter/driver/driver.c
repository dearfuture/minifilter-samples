/*
 Driver Development Template
 Powered by tangptr@126.com
 Version: v2.0
 Start Date: 2016.12.03
*/
#include <ntddk.h>
#include <windef.h>
#include <ntimage.h>
#include "ioctl.h"

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{	
	UNICODE_STRING strLink=RTL_CONSTANT_STRING(LINK_NAME);
	if(MyName)ExFreePool(MyName);
	IoDeleteSymbolicLink(&strLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{
	pIrp->IoStatus.Status=STATUS_SUCCESS;
	pIrp->IoStatus.Information=0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{
	NTSTATUS st=STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION irpsp=IoGetCurrentIrpStackLocation(pIrp);
	ULONG uIoControlCode=irpsp->Parameters.DeviceIoControl.IoControlCode;
	PVOID InputBuffer=pIrp->AssociatedIrp.SystemBuffer;
	PVOID OutputBuffer=pIrp->UserBuffer;
	ULONG InputSize=irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputSize=irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	//
	switch(uIoControlCode)
	{
		case IOCTL_SetName:
			{
				__try
				{
					ExFreePool(MyName);
					MyName=ExAllocatePool(NonPagedPool,InputSize+2);
					ProtectedFile.Buffer=MyName;
					ProtectedFile.Length=(USHORT)InputSize;
					ProtectedFile.MaximumLength=(USHORT)InputSize+2;
					RtlZeroMemory(ProtectedFile.Buffer,InputSize+2);
					RtlCopyMemory(ProtectedFile.Buffer,InputBuffer,InputSize);
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					;
				}
				st=STATUS_SUCCESS;
				break;
			}
	}
	if(st==STATUS_SUCCESS)
		pIrp->IoStatus.Information=OutputSize;
	else
		pIrp->IoStatus.Information=0;
	pIrp->IoStatus.Status=st;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	return st;
}

void DriverReinitialize(IN PDRIVER_OBJECT DriverObject,IN PVOID Context OPTIONAL,IN ULONG Count)
{
	RtlInitUnicodeString(&ProtectedFile,L"fucker.txt");
	MyName=ExAllocatePool(NonPagedPool,20);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryString)
{
	NTSTATUS st=STATUS_SUCCESS;
	UNICODE_STRING uniDevName=RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING uniLinkName=RTL_CONSTANT_STRING(LINK_NAME);
	PDEVICE_OBJECT pDevObj=NULL;
	//
	DriverObject->MajorFunction[IRP_MJ_CREATE]=DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]=DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=DispatchIoctl;
	DriverObject->DriverUnload=DriverUnload;
	//
	IoRegisterDriverReinitialization(DriverObject,DriverReinitialize,NULL);
	st=InitializeMiniFilter(DriverObject,RegistryString);
	if(NT_SUCCESS(st))
	{
		st=IoCreateDevice(DriverObject,0,&uniDevName,FILE_DEVICE_UNKNOWN,0,FALSE,&pDevObj);
		if(!NT_SUCCESS(st))return st;
		st=IoCreateSymbolicLink(&uniLinkName,&uniDevName);
		if(!NT_SUCCESS(st))IoDeleteDevice(pDevObj);
	}
	return st;
}