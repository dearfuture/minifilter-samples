#include <ntddk.h>
#include <windef.h>

#define	DEVICE_NAME			L"\\Device\\TestMiniFilter"
#define LINK_NAME			L"\\DosDevices\\TestMiniFilter"

#define CTL_CODE_GEN(i)		CTL_CODE(FILE_DEVICE_UNKNOWN, i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SetName		CTL_CODE_GEN(0x801)

NTSTATUS InitializeMiniFilter(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath);
void FinalizeMiniFilter();
extern UNICODE_STRING ProtectedFile;
PWSTR MyName;