#include <ntifs.h>
#include <windef.h>
#include <fltkernel.h>

PFLT_FILTER		hFilter=NULL;
UNICODE_STRING	ProtectedFile;