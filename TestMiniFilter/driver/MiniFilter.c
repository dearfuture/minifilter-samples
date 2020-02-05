#include <ntifs.h>
#include <windef.h>
#include <stdio.h>
#include <fltkernel.h>
#include "MiniFilter.h"

BOOLEAN SpyFindSubString(PUNICODE_STRING String,PUNICODE_STRING SubString)
{
	ULONG Index;
	if(RtlEqualUnicodeString(String,SubString,TRUE))return TRUE;
	for(Index=0;Index+(SubString->Length/sizeof(WCHAR))<=(String->Length/sizeof(WCHAR));Index++)
	{
		if(_wcsnicmp(&String->Buffer[Index],SubString->Buffer,(SubString->Length/sizeof(WCHAR)))==0)return TRUE;
	}
	return FALSE;
}

FLT_PREOP_CALLBACK_STATUS TpMFlt_SetInformationPreCall(IN OUT PFLT_CALLBACK_DATA FilterCallbackData,IN PCFLT_RELATED_OBJECTS FilterRelatedObjects,OUT PVOID *CompletionContext OPTIONAL)
{
	PFLT_FILE_NAME_INFORMATION NameInfo;
	NTSTATUS st=FltGetFileNameInformation(FilterCallbackData,FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,&NameInfo);
	if(NT_SUCCESS(st) && FilterCallbackData->Iopb->MajorFunction==IRP_MJ_SET_INFORMATION)
	{
		st=FltParseFileNameInformation(NameInfo);
		if(NT_SUCCESS(st))
		{
			if(SpyFindSubString(&NameInfo->Name,&ProtectedFile))
			{
				FltReleaseFileNameInformation(NameInfo);
				return FLT_PREOP_DISALLOW_FASTIO;
			}
		}
	}
	if(NT_SUCCESS(st))FltReleaseFileNameInformation(NameInfo);
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS PrepMiniFilter(IN PUNICODE_STRING RegistryString,IN PWSTR Altitude)
{
	NTSTATUS st=STATUS_UNSUCCESSFUL;
	WCHAR TempStr[MAX_PATH]={0},Data[MAX_PATH]={0};
	PWSTR wPtr=wcsrchr(RegistryString->Buffer,L'\\');
	if(!MmIsAddressValid(wPtr))return STATUS_INVALID_PARAMETER;
	RtlZeroMemory(TempStr,MAX_PATH * sizeof(WCHAR));
	swprintf(TempStr,L"%ws\\Instances",wPtr);
	//RtlStringCbPrintfW(TempStr,NTSTRSAFE_MAX_CCH*sizeof(WCHAR),L"%ws\\Instances",wPtr);
	st=RtlCreateRegistryKey(RTL_REGISTRY_SERVICES,TempStr);
	if(NT_SUCCESS(st))
	{
		swprintf(Data,L"%ws Instance",&wPtr[1]);
		//RtlStringCbPrintfW(Data,NTSTRSAFE_MAX_CCH*sizeof(WCHAR),L"%ws Instance",&wPtr[1]);
		st=RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,TempStr,L"DefaultInstance",REG_SZ,Data,(ULONG)(wcslen(Data)*sizeof(WCHAR)+2));
		if(NT_SUCCESS(st))
		{
			RtlZeroMemory(TempStr,MAX_PATH*sizeof(WCHAR));
			swprintf(TempStr,L"%ws\\Instances%ws Instance",wPtr,wPtr);
			//RtlStringCbPrintfW(TempStr,NTSTRSAFE_MAX_CCH*sizeof(WCHAR),L"%ws\\Instances%ws Instance",wPtr,wPtr);
			st=RtlCreateRegistryKey(RTL_REGISTRY_SERVICES,TempStr);
			if(NT_SUCCESS(st))
			{
				st=RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,TempStr,L"Altitude",REG_SZ,Altitude,(ULONG)(wcslen(Altitude)*sizeof(WCHAR)+2));
				if(NT_SUCCESS(st))
				{
					ULONG dwData=0;
					st=RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,TempStr,L"Flags",REG_DWORD,&dwData,4);
				}
			}
		}
	}
	return st;
}

NTSTATUS FinalizeMiniFilterCallback(IN FLT_FILTER_UNLOAD_FLAGS Flags)
{
	FltUnregisterFilter(hFilter);
	Flags=FLTFL_FILTER_UNLOAD_MANDATORY;
	return STATUS_SUCCESS;
}

NTSTATUS InitializeMiniFilter(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS st=PrepMiniFilter(RegistryPath,L"321229");
	if(NT_SUCCESS(st))
	{
		FLT_REGISTRATION FltReg;
		FLT_OPERATION_REGISTRATION FltOpReg[2];
		RtlZeroMemory(&FltReg,sizeof(FltReg));
		FltReg.Size=sizeof(FLT_REGISTRATION);
		FltReg.Version=FLT_REGISTRATION_VERSION;
		FltReg.FilterUnloadCallback=FinalizeMiniFilterCallback;
		FltReg.OperationRegistration=FltOpReg;
		RtlZeroMemory(FltOpReg,sizeof(FLT_OPERATION_REGISTRATION)*2);
		FltOpReg[0].MajorFunction=IRP_MJ_SET_INFORMATION;
		FltOpReg[0].Flags=FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO;
		FltOpReg[0].PreOperation=TpMFlt_SetInformationPreCall;
		FltOpReg[1].MajorFunction=IRP_MJ_OPERATION_END;
		st=FltRegisterFilter(DriverObject,&FltReg,&hFilter);
		if(NT_SUCCESS(st))
		{
			st=FltStartFiltering(hFilter);
		}
	}
	return st;
}
