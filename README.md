# minifilter-samples
https://github.com/dearfuture/minifilter-samples/blob/master/README.md

0x01
安装注册minifilter(4种选择)
1) 通过inf-file安装:
   右键inf-file, 安装(或InfDefaultInstall.exe inf-file), execute the DefaultInstall and DefaultInstall.Services  
   or    RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 path-to-inf\infname.inf
   or    InstallHinfSection(NULL,NULL,TEXT("DefaultInstall 132 path-to-inf\infname.inf"),0); 
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/using-an-inf-file-to-install-a-file-system-filter-driver?redirectedfrom=MSDN
inf内容
[NullFilter.Service]
DisplayName      = %ServiceName%
Description      = %ServiceDescription%
ServiceBinary    = %12%\%DriverName%.sys    ;%windir%\system32\drivers\
Dependencies     = "FltMgr"							<---------------------------
ServiceType      = 2                        ;SERVICE_FILE_SYSTEM_DRIVER
StartType        = 3                        ;SERVICE_DEMAND_START
ErrorControl     = 1                        ;SERVICE_ERROR_NORMAL
LoadOrderGroup   = "FSFilter Activity Monitor"					<---------------------------
AddReg           = NullFilter.AddRegistry

;
; Registry Modifications
;

[NullFilter.AddRegistry]
HKR,,"SupportedFeatures",0x00010001,0x3
HKR,"Instances","DefaultInstance",0x00000000,%DefaultInstance%			<---------------------------
HKR,"Instances\"%Instance1.Name%,"Altitude",0x00000000,%Instance1.Altitude%	<---------------------------
HKR,"Instances\"%Instance1.Name%,"Flags",0x00010001,%Instance1.Flags%		<---------------------------

2) 先CreateService类似于注册普通驱动, 再通过RegCreateKeyEx和RegSetValueEx写注册表HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance...
hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
CreateServiceA(hServiceMgr,
		lpszDriverName,             // 驱动程序的在注册表中的名字
		lpszDriverName,             // 注册表驱动程序的DisplayName 值
		SERVICE_ALL_ACCESS,         // 加载驱动程序的访问权限
		SERVICE_FILE_SYSTEM_DRIVER, // 表示加载的服务是文件系统驱动程序
		SERVICE_DEMAND_START,       // 注册表驱动程序的Start 值
		SERVICE_ERROR_IGNORE,       // 注册表驱动程序的ErrorControl 值
		szDriverImagePath,          // 注册表驱动程序的ImagePath 值
		"FSFilter Activity Monitor",// 注册表驱动程序的Group 值
		NULL,
		"FltMgr",                   // 注册表驱动程序的DependOnService 值
		NULL,
		NULL);
//then RegCreateKeyEx and RegSetValueEx for HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance...

3）sc create XXX，再注册minifilter所需的额外注册表项HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance...

本质: 创建了NullFilter服务(CreateService)
而相对于创建普通服务/安装注册普通驱动, minifilter还要进一步在注册表SYSTEM\\CurrentControlSet\\Services\\NullFilter中
创建SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance
填写HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\InstanceDefaultInstance -->(实例名)
创建HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance\\实例名
填写HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance\\实例名\\Altitude 和 HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance\\实例名\\Flags

4）当作普通驱动注册和加载，在调用FltRegisterFilter将驱动自身注册为minifilter之前，先通过RtlCreateRegistryKey和RtlWriteRegistryValue写注册表SYSTEM\\CurrentControlSet\\Services\\NullFilter\\Instance...
http://www.m5home.com/bbs/thread-8922-1-1.html
http://www.m5home.com/bbs/thread-8943-1-1.html
https://github.com/dearfuture/minifilter-samples/blob/master/TestMiniFilter/driver/MiniFilter.c

0x02
注销minifilter
sc delete NullFilter
本质: 删除了NullFilter服务(DeleteService)，和注销普通驱动类似

0x03
加载minifilter(4种选择)
1) sc start NullFilter
2) net start NullFilter
3) fltmc load NullFilter 或 调用FilterLoad
本质: 启动了NullFilter服务(StartService)，和加载普通驱动类似
4) 普通驱动调用FltLoadFilter加载其它minifilter (类似NtLoadDriver)

0x04
卸载minifilter(4种选择)
1) sc stop NullFilter
2) net stop NullFilter
3) fltmc unload NullFilter 或 调用FilterUnload
本质: 停止了NullFilter服务(CloseService)，和卸载普通驱动类似
4) 普通驱动调用FltUnloadFilter卸载其它minifilter (类似NtUnLoadDriver)

minifilter保证自身能被卸载：除了指定实现Driver->DriverUnload以外，还必须在调用FltRegisterFilter将驱动自身注册为minifilter之前，指定实现FilterRegistration.FilterUnloadCallback，否则minifilter无法真正卸载
(类比: 导出驱动不实现DllUnload就不会自动卸载，普通驱动不指定实现Driver->DriverUnload卸载时会蓝屏)
FLT_REGISTRATION FilterRegistration;
FilterRegistration.FilterUnloadCallback = NullUnload ;
NTSTATUS
NullUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    FltUnregisterFilter( NullFilterData.FilterHandle );
    return STATUS_SUCCESS;
}

0x05
minifilter开启过滤的一般流程
NTSTATUS FLTAPI FltRegisterFilter(
  PDRIVER_OBJECT         Driver,
  const FLT_REGISTRATION *Registration,
  PFLT_FILTER            *RetFilter
);
VOID FLTAPI FltUnregisterFilter(
  PFLT_FILTER Filter
);
NTSTATUS FLTAPI FltStartFiltering(
  PFLT_FILTER Filter
);
FltRegisterFilter和 FltUnregisterFilter都只能对驱动自身使用
DriverEntry调用FltRegisterFilter之后，一般调用FltStartFiltering开始过滤
一般在FilterRegistration.FilterUnloadCallback中或FltStartFiltering失败时调用FltUnregisterFilter

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &NullFilterData.FilterHandle );
    if (NT_SUCCESS( status )) {
        status = FltStartFiltering( NullFilterData.FilterHandle );
        if (!NT_SUCCESS( status )) {
            FltUnregisterFilter( NullFilterData.FilterHandle );
        }
    }
    return status;
}

0x06
FLTMC Command和Flt manage API (r0/r3)
https://ss64.com/nt/fltmc.html
https://docs.microsoft.com/en-us/windows/win32/api/fltuser/
https://docs.microsoft.com/zh-cn/windows-hardware/drivers/ddi/fltkernel/
https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/file-system-minifilter-drivers

FLTMC.exe Manage MiniFilter drivers. 
Load a Filter driver, Unload a Filter driver, 
List filter information, List all instances or the instances associated with a Filter or Volume, List all volumes (including the network redirectors), 
Attach or Detach a filter from a Volume.

FLTMC load [ driverName ]        
FLTMC unload [ driverName ]								
FLTMC filters								
FLTMC instances [-f filterName ]|[-v volumeName ]
FLTMC volumes
FLTMC attach [ filterName ] [ volumeName ] [[-i instanceName ][-a altitude]]
FLTMC detach [ filterName ] [ volumeName ] [ instanceName ]

1)
FLTMC load [ driverName ]         
Load a Filter driver  FltLoadFilter     
           
NTSTATUS FLTAPI FltLoadFilter(
  PCUNICODE_STRING FilterName
);

HRESULT FilterLoad(
  LPCWSTR lpFilterName
);

2)
FLTMC unload [ driverName ]		Unload a Filter driver		

NTSTATUS FLTAPI FltUnloadFilter(
  PCUNICODE_STRING FilterName
);

HRESULT FilterUnload(
  LPCWSTR lpFilterName
);

3)
FLTMC filters					

NTSTATUS FLTAPI FltEnumerateFilters(
  PFLT_FILTER *FilterList,
  ULONG       FilterListSize,
  PULONG      NumberFiltersReturned
);
NTSTATUS FLTAPI FltEnumerateFilterInformation(
  ULONG                    Index,
  FILTER_INFORMATION_CLASS InformationClass,
  PVOID                    Buffer,
  ULONG                    BufferSize,
  PULONG                   BytesReturned
);

HRESULT FilterFindFirst(
  FILTER_INFORMATION_CLASS dwInformationClass,
  LPVOID                   lpBuffer,
  DWORD                    dwBufferSize,
  LPDWORD                  lpBytesReturned,
  LPHANDLE                 lpFilterFind
);
HRESULT FilterFindNext(
  HANDLE                   hFilterFind,
  FILTER_INFORMATION_CLASS dwInformationClass,
  LPVOID                   lpBuffer,
  DWORD                    dwBufferSize,
  LPDWORD                  lpBytesReturned
);

4)
FLTMC instances [-f filterName ]|[-v volumeName ]		
NTSTATUS FLTAPI FltEnumerateInstances(
  PFLT_VOLUME   Volume,
  PFLT_FILTER   Filter,
  PFLT_INSTANCE *InstanceList,
  ULONG         InstanceListSize,
  PULONG        NumberInstancesReturned
);
NTSTATUS FLTAPI FltEnumerateInstanceInformationByFilter(
  PFLT_FILTER                Filter,
  ULONG                      Index,
  INSTANCE_INFORMATION_CLASS InformationClass,
  PVOID                      Buffer,
  ULONG                      BufferSize,
  PULONG                     BytesReturned
);
NTSTATUS FLTAPI FltEnumerateInstanceInformationByVolumeName(
  PUNICODE_STRING            VolumeName,
  ULONG                      Index,
  INSTANCE_INFORMATION_CLASS InformationClass,
  PVOID                      Buffer,
  ULONG                      BufferSize,
  PULONG                     BytesReturned
);

HRESULT FilterInstanceFindFirst(
  LPCWSTR                    lpFilterName,
  INSTANCE_INFORMATION_CLASS dwInformationClass,
  LPVOID                     lpBuffer,
  DWORD                      dwBufferSize,
  LPDWORD                    lpBytesReturned,
  LPHANDLE                   lpFilterInstanceFind
);

5)
FLTMC volumes						

NTSTATUS FLTAPI FltEnumerateVolumes(
  PFLT_FILTER Filter,
  PFLT_VOLUME *VolumeList,
  ULONG       VolumeListSize,
  PULONG      NumberVolumesReturned
);
NTSTATUS FLTAPI FltEnumerateVolumeInformation(
  PFLT_FILTER                     Filter,
  ULONG                           Index,
  FILTER_VOLUME_INFORMATION_CLASS InformationClass,
  PVOID                           Buffer,
  ULONG                           BufferSize,
  PULONG                          BytesReturned
);

HRESULT FilterVolumeFindFirst(
  FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
  LPVOID                          lpBuffer,
  DWORD                           dwBufferSize,
  LPDWORD                         lpBytesReturned,
  PHANDLE                         lpVolumeFind
);
HRESULT FilterVolumeFindNext(
  HANDLE                          hVolumeFind,
  FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
  LPVOID                          lpBuffer,
  DWORD                           dwBufferSize,
  LPDWORD                         lpBytesReturned
);

6)
FLTMC attach [ filterName ] [ volumeName ] [[-i instanceName ][-a altitude]]		

NTSTATUS FLTAPI FltAttachVolume(
  PFLT_FILTER      Filter,
  PFLT_VOLUME      Volume,
  PCUNICODE_STRING InstanceName,
  PFLT_INSTANCE    *RetInstance
);
NTSTATUS FLTAPI FltAttachVolumeAtAltitude(
  PFLT_FILTER      Filter,
  PFLT_VOLUME      Volume,
  PCUNICODE_STRING Altitude,
  PCUNICODE_STRING InstanceName,
  PFLT_INSTANCE    *RetInstance
);

HRESULT FilterAttach(
  LPCWSTR lpFilterName,
  LPCWSTR lpVolumeName,
  LPCWSTR lpInstanceName,
  DWORD   dwCreatedInstanceNameLength,
  LPWSTR  lpCreatedInstanceName
);
HRESULT FilterAttachAtAltitude(
  LPCWSTR lpFilterName,
  LPCWSTR lpVolumeName,
  LPCWSTR lpAltitude,
  LPCWSTR lpInstanceName,
  DWORD   dwCreatedInstanceNameLength,
  LPWSTR  lpCreatedInstanceName
);

7)
FLTMC detach [ filterName ] [ volumeName ] [ instanceName ]						

NTSTATUS FLTAPI FltDetachVolume(
  PFLT_FILTER      Filter,
  PFLT_VOLUME      Volume,
  PCUNICODE_STRING InstanceName
); 

HRESULT FilterDetach(
  LPCWSTR lpFilterName,
  LPCWSTR lpVolumeName,
  LPCWSTR lpInstanceName
);

0x07
FLT_REGISTRATION --> PFLT_OPERATION_REGISTRATION (OperationRegistration) --> PFLT_PRE_OPERATION_CALLBACK (PreOperation & PostOperation)  
typedef struct _FLT_REGISTRATION {
  USHORT                                      Size;							//sizeof(FLT_REGISTRATION)
  USHORT                                      Version;						//FLT_REGISTRATION_VERSION
  FLT_REGISTRATION_FLAGS                      Flags;
  const FLT_CONTEXT_REGISTRATION              *ContextRegistration;
  const FLT_OPERATION_REGISTRATION            *OperationRegistration;			<---------------------------
  PFLT_FILTER_UNLOAD_CALLBACK                 FilterUnloadCallback;				<---------------------------
  PFLT_INSTANCE_SETUP_CALLBACK                InstanceSetupCallback;
  PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK       InstanceQueryTeardownCallback;
  PFLT_INSTANCE_TEARDOWN_CALLBACK             InstanceTeardownStartCallback;
  PFLT_INSTANCE_TEARDOWN_CALLBACK             InstanceTeardownCompleteCallback;
  PFLT_GENERATE_FILE_NAME                     GenerateFileNameCallback;
  PFLT_NORMALIZE_NAME_COMPONENT               NormalizeNameComponentCallback;
  PFLT_NORMALIZE_CONTEXT_CLEANUP              NormalizeContextCleanupCallback;
  PFLT_TRANSACTION_NOTIFICATION_CALLBACK      TransactionNotificationCallback;
  PFLT_NORMALIZE_NAME_COMPONENT_EX            NormalizeNameComponentExCallback;
  PFLT_SECTION_CONFLICT_NOTIFICATION_CALLBACK SectionNotificationCallback;
} FLT_REGISTRATION, *PFLT_REGISTRATION;

typedef struct _FLT_OPERATION_REGISTRATION {
  UCHAR                            MajorFunction;			<---------------------------IRP_MJ_SET_INFORMATION/...
  FLT_OPERATION_REGISTRATION_FLAGS Flags;					<---------------------------FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO / FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO / FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO	
  PFLT_PRE_OPERATION_CALLBACK      PreOperation;			<---------------------------
  PFLT_POST_OPERATION_CALLBACK     PostOperation;			<---------------------------
  PVOID                            Reserved1;
} FLT_OPERATION_REGISTRATION, *PFLT_OPERATION_REGISTRATION;

PFLT_PRE_OPERATION_CALLBACK PfltPreOperationCallback;
FLT_PREOP_CALLBACK_STATUS PfltPreOperationCallback(
  PFLT_CALLBACK_DATA Data,									<---------------------------Data->Iopb->MajorFunction==IRP_MJ_SET_INFORMATION ...
  PCFLT_RELATED_OBJECTS FltObjects,
  PVOID *CompletionContext
)
{...}														

PFLT_POST_OPERATION_CALLBACK PfltPostOperationCallback;
FLT_POSTOP_CALLBACK_STATUS PfltPostOperationCallback(
  PFLT_CALLBACK_DATA Data,									<---------------------------Data->Iopb->MajorFunction==IRP_MJ_SET_INFORMATION ...
  PCFLT_RELATED_OBJECTS FltObjects,
  PVOID CompletionContext,
  FLT_POST_OPERATION_FLAGS Flags							<---------------------------
)
{...}

typedef struct _FLT_CALLBACK_DATA {
  FLT_CALLBACK_DATA_FLAGS     Flags;
  PETHREAD                    Thread;
  PFLT_IO_PARAMETER_BLOCK     Iopb;							<---------------------------
  IO_STATUS_BLOCK             IoStatus;
  struct _FLT_TAG_DATA_BUFFER *TagData;
  union {
    struct {
      LIST_ENTRY QueueLinks;
      PVOID      QueueContext[2];
    };
    PVOID FilterContext[4];
  };
  KPROCESSOR_MODE             RequestorMode;
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

typedef struct _FLT_IO_PARAMETER_BLOCK {
  ULONG          IrpFlags;
  UCHAR          MajorFunction;								<---------------------------
  UCHAR          MinorFunction;
  UCHAR          OperationFlags;
  UCHAR          Reserved;
  PFILE_OBJECT   TargetFileObject;
  PFLT_INSTANCE  TargetInstance;
  FLT_PARAMETERS Parameters;
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;


0x08 
minifilter通信

R3
HRESULT FilterConnectCommunicationPort(
  LPCWSTR               lpPortName,
  DWORD                 dwOptions,
  LPCVOID               lpContext,
  WORD                  wSizeOfContext,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  HANDLE                *hPort
);

HRESULT FilterSendMessage(
  HANDLE  hPort,
  LPVOID  lpInBuffer,
  DWORD   dwInBufferSize,
  LPVOID  lpOutBuffer,
  DWORD   dwOutBufferSize,
  LPDWORD lpBytesReturned
);
HRESULT FilterGetMessage(
  HANDLE                 hPort,
  PFILTER_MESSAGE_HEADER lpMessageBuffer,
  DWORD                  dwMessageBufferSize,
  LPOVERLAPPED           lpOverlapped
);


R0
NTSTATUS FLTAPI FltCreateCommunicationPort(
  PFLT_FILTER            Filter,
  PFLT_PORT              *ServerPort,
  POBJECT_ATTRIBUTES     ObjectAttributes,
  PVOID                  ServerPortCookie,
  PFLT_CONNECT_NOTIFY    ConnectNotifyCallback,
  PFLT_DISCONNECT_NOTIFY DisconnectNotifyCallback,		
  PFLT_MESSAGE_NOTIFY    MessageNotifyCallback,			<---------------------------  handle FilterSendMessage. if NULL, FilterSendMessage will receive an error 
  LONG                   MaxConnections
);

typedef NTSTATUS
(*PFLT_MESSAGE_NOTIFY) (
      IN PVOID PortCookie,							
      IN PVOID InputBuffer OPTIONAL,					<---------------------------  R3 Buffer from FilterSendMessage 
      IN ULONG InputBufferLength,						<---------------------------
      OUT PVOID OutputBuffer OPTIONAL,					<---------------------------  FilterGetMessage 
      IN ULONG OutputBufferLength,						<--------------------------- 
      OUT PULONG ReturnOutputBufferLength				<---------------------------  
      );

VOID FLTAPI FltCloseCommunicationPort(
  PFLT_PORT ServerPort
);
	  
0x09 
Other Samples
http://assurefiles.com/Forums_Files/MiniFilterExample.htm
......

0x0A
Advanced Topics
......