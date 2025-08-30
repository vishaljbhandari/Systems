/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   srdnfow2k
* @file      recovery/srd/SysNfo/SysNfoW2K/srdw2k.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     /
*
* @since     23.05.01	lukaj	Original Coding
*
* @remarks   /
*/
#include "lib\cmn\target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/SysNfo/SysNfoW2K/srdw2k.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|	TBD items:
|
|   + Verify that vital volumes are verified if they are actually not volume
|     mountpoints.
|   + Finish the ADS info collecting functionality.
|   + Ntft volume handling the NT4 way. If the system is upgraded from NT4 to
|     W2K and there were FT volumes present on the original system, there can
|     still be the DISK key in the registry. The disk remains basic, though.
 ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/
#include "lib/cmn/common.h"
#include "da/win32/ntfrsapi.h"
#include "../../srd.h"
#include "../../srdutil.h"
#include "../../srdfree.h"
#include "../sysnfocmn.h"
#include "../sysnfonet.h"
#include "srdw2k.h"
#include "integ/postgres/postgres_defines.h"

#include <cfgmgr32.h>
#include <mountmgr.h>
#include <ntddstor.h>
#include <setupapi.h>
#include <objbase.h>
#include <initguid.h>
#include <shlobj.h>
#include <certbcli.h>
#ifndef COBJMACROS
	#define COBJMACROS
#endif
#include <netcfgx.h>
#include <devguid.h>

/* --------------------------------------------------------------------
|	DDK GENERAL DEFINES / CANNOT INCLUDE BOTH NTDDK.H AND WINDOWS.H
----------------------------------------------------------------------*/
typedef LONG NTSTATUS;

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_BUFFER_TOO_SMALL					((NTSTATUS)0xC0000023L)
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010

typedef struct _UNICODE_STRING {
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
    USHORT Length; //String length /* parasoft-suppress  NAMING-07 "Old structure this will not be changed" */
    USHORT MaximumLength; //String maximum length /* parasoft-suppress  NAMING-07 "Old structure this will not be changed" */
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(*PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

DEFINE_GUID(CLSID_NetCfg, 0x5B035261,0x40F9,0x11D1,0xAA,0xEC,0x00,0x80,0x5F,0xC1,0x27,0x0E);

typedef R_DWORD (WINAPI *getFirmwareEnvironmentVariable)(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize);

/* --------------------------------------------------------------------
|	NTDLL.DLL function typedefs
----------------------------------------------------------------------*/
typedef NTSTATUS (__stdcall 
*RtlCreateUnicodeStringProc)(PUNICODE_STRING, LPCWSTR);

typedef VOID (__stdcall 
*RtlFreeUnicodeStringProc)(PUNICODE_STRING);

typedef NTSTATUS (__stdcall 
*NtOpenFileProc)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

typedef NTSTATUS (__stdcall 
*NtDeviceIoControlFileProc)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, 
					  ULONG, PVOID, ULONG, PVOID, ULONG);
typedef NTSTATUS (__stdcall 
*NtFsControlFileProc)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, ULONG, 
					PVOID, ULONG, PVOID, ULONG);

typedef VOID (__stdcall 
*NtCloseProc)(HANDLE);

/* --------------------------------------------------------------------
|	NTDLL.DLL function declarations
----------------------------------------------------------------------*/
static HMODULE hNtdll = NULL;
static RtlCreateUnicodeStringProc RtlCreateUnicodeString = NULL;
static RtlFreeUnicodeStringProc RtlFreeUnicodeString = NULL;
static NtOpenFileProc NtOpenFile = NULL;
static NtDeviceIoControlFileProc NtDeviceIoControlFile = NULL;
static NtFsControlFileProc NtFsControlFile = NULL;
static NtCloseProc NtClose = NULL;

/* --------------------------------------------------------------------
|	NTDSBCLI.DLL function typedefs and defines
----------------------------------------------------------------------*/
typedef HRESULT (_stdcall *DsIsNTDSOnlineProcA)(LPCSTR, BOOL*);
typedef HRESULT (_stdcall *DsIsNTDSOnlineProcW)(LPCWSTR, BOOL*);

#if defined(UNICODE) || defined(_UNICODE)
    #define DsIsNTDSOnlineProc          DsIsNTDSOnlineProcW
    #define DsIsNTDSOnlineName          "DsIsNTDSOnlineW"
#else
    #define DsIsNTDSOnlineProc          DsIsNTDSOnlineProcA
    #define DsIsNTDSOnlineName          "DsIsNTDSOnlineA"
#endif
#define NTDS_DLLNAME             _T("ntdsbcli.dll")
#define NTDS_KEY                 _T("SYSTEM\\CurrentControlSet\\Services\\NTDS\\Parameters")
#define NTDS_DB_VAL              _T("DSA Database file")
#define NTDS_LOG_VAL             _T("Database log files path")

/* --------------------------------------------------------------------
|	CERTADM.DLL defines
----------------------------------------------------------------------*/
#define CertSrvIsServerOnlineName   "CertSrvIsServerOnlineW"
#define CERTSVC_DLLNAME             _T("certadm.dll")
#define CERTSVC_KEY                 _T("SYSTEM\\CurrentControlSet\\Services\\CertSvc\\Configuration")
#define CERTSVC_DB_VAL              _T("DBDirectory")
#define CERTSVC_LOG_VAL             _T("DBLogDirectory")

/* --------------------------------------------------------------------
|	Discards data collected for a W2K specific disk
----------------------------------------------------------------------*/
static R_VOID
DiscardDisk(PDISKW2K pDisk)
{
	if(pDisk != NULL)
	{
		if(pDisk->BootSectorCode != NULL)
			FREE(pDisk->BootSectorCode);
		if(pDisk->MBRs != NULL)
			FREE(pDisk->MBRs);
		if(pDisk->Layout != NULL)
			SrdFreeDiskLayout(pDisk->Layout);
	}
}

/* --------------------------------------------------------------------
|	Discards entire W2K specific disk information
----------------------------------------------------------------------*/
R_VOID
DiscardDisks(PDISKINFOW2K pDiskInfo)
{
	R_ULONG		ii = 0;

	if(pDiskInfo != NULL)
	{
		for(ii = 0; ii < pDiskInfo->DiskCount; ii++)
			DiscardDisk(&pDiskInfo->Disks[ii]);

		if(pDiskInfo->DOSDevices != NULL)
			FREE(pDiskInfo->DOSDevices);
		if(pDiskInfo->DiskDevices != NULL)
			FREE(pDiskInfo->DiskDevices);
		if(pDiskInfo->EUPBoot != NULL)
			FREE(pDiskInfo->EUPBoot);
		if(pDiskInfo->VolMtInfo != NULL)
			FreeVolumeMountpointInfo(pDiskInfo->VolMtInfo);
		if(pDiskInfo->VolumeInfo != NULL)
			FREE(pDiskInfo->VolumeInfo);
	}
}

/* --------------------------------------------------------------------
|	Disconnects from the ntdll.dll library.
----------------------------------------------------------------------*/
R_VOID
FreeNtdllProcedures(R_VOID)
{
	if (hNtdll != NULL)
	{
		FreeLibrary(hNtdll);

		RtlCreateUnicodeString = NULL;
		RtlFreeUnicodeString = NULL;
		NtOpenFile = NULL;
		NtDeviceIoControlFile = NULL;
		NtFsControlFile = NULL;
		NtClose = NULL;
	}
}

/* --------------------------------------------------------------------
|	Connects to the ntdll.dll library and initializes function pointers
|   to the subsequently used ntdll.dll procedures.
----------------------------------------------------------------------*/
R_RESULT
InitializeNtdllProcedures(R_VOID)
{
	R_RESULT	Return = SRDERR_ERROR;
	
	hNtdll = LoadLibrary(_T("ntdll.dll"));

	if(hNtdll != NULL)
	{
		RtlCreateUnicodeString =	(RtlCreateUnicodeStringProc)GetProcAddress(hNtdll, "RtlCreateUnicodeString");
		RtlFreeUnicodeString =		(RtlFreeUnicodeStringProc)GetProcAddress(hNtdll, "RtlFreeUnicodeString");
		NtOpenFile =				(NtOpenFileProc)GetProcAddress(hNtdll, "NtOpenFile");
		NtDeviceIoControlFile =		(NtDeviceIoControlFileProc)GetProcAddress(hNtdll, "NtDeviceIoControlFile");
		NtFsControlFile =			(NtFsControlFileProc)GetProcAddress(hNtdll, "NtFsControlFile");
		NtClose =					(NtCloseProc)GetProcAddress(hNtdll, "NtClose");

		if(
			RtlCreateUnicodeString && 
			RtlFreeUnicodeString && 
			NtOpenFile && 
			NtDeviceIoControlFile &&
			NtFsControlFile && 
			NtClose
		)
		{
			Return = SRDERR_SUCCESS;
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Similar to CreateFile() WIN32 API, but more advanced. Some obscure
|   device / disk paths can be opened by using this function that are
|   not supported by the standard WIN32 API call. At least W2K disk
|   management works this way...
----------------------------------------------------------------------*/
static R_RESULT
NativeOpenFile(PHANDLE lpOut, R_DWORD dwAccess, tchar *pszPath)
{
	R_RESULT	Return = SRDERR_ERROR;
	NTSTATUS	ntRes = STATUS_SUCCESS;
	UNICODE_STRING		usPath;
	OBJECT_ATTRIBUTES	ObjAttr;
	IO_STATUS_BLOCK		IoStatus;
	LPWSTR				pszPathU = NULL;
	R_BOOL				bReplace = FALSE;
#if !defined (UNICODE) && !defined (_UNICODE)
	WCHAR				pszBuffer[MAX_PATH];
#endif

	DbgPlain(DBG_SYSNFOW2K, _T("Opening path: %s."), pszPath);

	if(strncmp(pszPath, _T("\\\\?\\"), 4) == 0)
	{
		pszPath[1] = _T('?');
		bReplace = TRUE;
	}

	memset(&usPath, 0, sizeof(UNICODE_STRING));
	memset(&ObjAttr, 0, sizeof(OBJECT_ATTRIBUTES));
	memset(&IoStatus, 0, sizeof(IO_STATUS_BLOCK));

#if defined (UNICODE) || defined (_UNICODE)
	pszPathU = pszPath;
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszPath, -1, pszBuffer, MAX_PATH);
	pszPathU = pszBuffer;
#endif

	RtlCreateUnicodeString(&usPath, pszPathU);
	
	InitializeObjectAttributes(&ObjAttr, &usPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

	DbgPlain(DBG_SYSNFOW2K, _T("  dwAccess: %#lx."), dwAccess);

	ntRes = NtOpenFile(lpOut, dwAccess, &ObjAttr, &IoStatus, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);

	if(ntRes == STATUS_SUCCESS)
		Return = SRDERR_SUCCESS;
	else
		DbgPlain(DBG_SYSNFOW2K, _T("Error status: %#lx."), ntRes);

	RtlFreeUnicodeString(&usPath);

	if(bReplace)
		pszPath[1] = _T('\\');

	return Return;
}

/* --------------------------------------------------------------------
|	Just a wrapper around the configuration manager API. Locates a 
|   device node under the plug'n'play device tree. Use with disk
|   management.
----------------------------------------------------------------------*/
static R_RESULT
LocateDeviceNode(tchar *pszID, R_DWORD *pOut)
{
	R_RESULT	Return = SRDERR_ERROR;

	if(CM_Locate_DevNode(pOut, (DEVINSTID)pszID, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
	{
		Return = SRDERR_SUCCESS;
	}
	
	return Return;
}

/* --------------------------------------------------------------------
|	Just a wrapper around the configuration manager API. Fetches the
|   status of a (disk) device.
----------------------------------------------------------------------*/
static R_RESULT
GetDeviceStatus(DEVINST DevInst, R_ULONG *pStatus, R_ULONG *pProblem)
{
	R_RESULT Return = SRDERR_ERROR;

	if(CM_Get_DevNode_Status_Ex(pStatus, pProblem, DevInst, 0, NULL) == CR_SUCCESS)
	{
		Return = SRDERR_SUCCESS;
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Just a wrapper around the configuration manager API. Returns the
|   parent of the current child node inside the plug'n'play device tree.
----------------------------------------------------------------------*/
static R_RESULT
GetParentID(DEVINST hDevInst, DEVINST *pParent)
{
	R_RESULT	Return = SRDERR_ERROR;
	CONFIGRET	crRet = CR_SUCCESS;
	DEVINST		hParent = 0;

	crRet = CM_Get_Parent_Ex(&hParent, hDevInst, 0, NULL);

	if(crRet == CR_SUCCESS)
	{
		*pParent = hParent;

		Return = SRDERR_SUCCESS;
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Just a wrapper around the configuration manager API. Returns a
|   string representation of a property held by a given device node
|   (e.g. description, etc.)
----------------------------------------------------------------------*/
static R_RESULT
GetDeviceStringProperty(DEVINST hDevInst, R_ULONG ulProp, R_TCHAR *pszBuffer, R_ULONG ulSize)
{
	R_RESULT	Return = SRDERR_ERROR;
	CONFIGRET	crRet = CR_SUCCESS;
	R_ULONG		ulLen = 0, ulType = 0;

	crRet = CM_Get_DevNode_Registry_Property_Ex(hDevInst, ulProp, &ulType, NULL, &ulLen, 0, NULL);
	if(
		crRet == CR_SUCCESS ||
		crRet == CR_BUFFER_SMALL
	)
	{
		if(ulLen <= ulSize * sizeof(WCHAR))
		{
			CM_Get_DevNode_Registry_Property_Ex(hDevInst, ulProp, &ulType, pszBuffer, &ulLen, 0, NULL);

			Return = SRDERR_SUCCESS;
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Used to enumerate devices by its sequential number. All devices
|   enumerated start with 0 and continue to, let's say n - 1.
----------------------------------------------------------------------*/
static R_RESULT
GetDeviceNumber(tchar *pszPath, R_ULONG *pDisk, R_ULONG *pPart)
{
	R_RESULT	Return = SRDERR_ERROR;
	HANDLE		hDevice = NULL;

	DbgPlain(DBG_SYSNFOW2K, _T("Getting device number for path: %s."), pszPath);

	if(NativeOpenFile(&hDevice, SYNCHRONIZE, pszPath) == SRDERR_SUCCESS)
	{
		R_DWORD					dwBytes = 0;
		STORAGE_DEVICE_NUMBER	sdn;

		memset(&sdn, 0, sizeof(STORAGE_DEVICE_NUMBER));

		if(
			DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, 
				sizeof(STORAGE_DEVICE_NUMBER), &dwBytes, NULL) == TRUE &&
			dwBytes == sizeof(STORAGE_DEVICE_NUMBER)
		)
		{
			*pDisk = sdn.DeviceNumber;
			if(pPart != NULL)
				*pPart = sdn.PartitionNumber;

			Return = SRDERR_SUCCESS;
		}
		else
			DbgPlain(DBG_SYSNFOW2K, _T("DeviceIoControl(IOCTL_STORAGE_GET_DEVICE_NUMBER) failed."));

		NtClose(hDevice);
	}
	else
		DbgPlain(DBG_SYSNFOW2K, _T("NativeOpenFile failed."));

	return Return;
}

/* --------------------------------------------------------------------
|	Higher level function that collects various strings connected to the
|   operation of the disk devices (device description, device ID used
|   by some function later in the information collection process, disk\
|   controller name, etc.)
----------------------------------------------------------------------*/
static R_RESULT
GetDiskStrings(DEVINST DevInst, tchar *pszPath, tchar *pszID, PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ulCount = lpDisk->DiskCount;
	tchar		pszDescr[1024];
	static int	countNoDescr = 0;
	DEVINST		DevParent = 0;

	if(GetDeviceStringProperty(DevInst, CM_DRP_FRIENDLYNAME, 
		pszDescr, 1024 * sizeof(tchar)) != SRDERR_SUCCESS)
	{
		sprintf(pszDescr, _T("NoNameDevice%d"), countNoDescr);
		countNoDescr++;
	}

	if(GetParentID(DevInst, &DevParent) == SRDERR_SUCCESS)
	{
		tchar	pszParent[1024];

		if(GetDeviceStringProperty(DevParent, CM_DRP_DEVICEDESC, 
			pszParent, 1024 * sizeof(tchar)) == SRDERR_SUCCESS)
		{
			strcpy(lpDisk->Disks[ulCount].DeviceID, pszID);
			strcpy(lpDisk->Disks[ulCount].Description, pszDescr);
			strcpy(lpDisk->Disks[ulCount].Controller, pszParent);

			Return = SRDERR_SUCCESS;
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Higher level function that collects some of the disk characteristics
|   (sequential number, geometry, layout, etc.).
----------------------------------------------------------------------*/
static R_RESULT
GetDiskCharacteristics(tchar *pszPath, PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ulCount = lpDisk->DiskCount;
	R_ULONG		ulDisk = 0;
	R_ULONG		ulPart = 0;

	DbgPlain(DBG_SYSNFOW2K, _T("Characteristics for path: %s."), pszPath);

	if(GetDeviceNumber(pszPath, &ulDisk, &ulPart) == SRDERR_SUCCESS)
	{
		HANDLE	hDevice = NULL;
		tchar	pszDevPath[MAX_PATH];
        R_ULONG dynamic = 0;

		lpDisk->Disks[ulCount].Number = ulDisk;

		sprintf(pszDevPath, _T("\\Device\\Harddisk%d\\Partition%d"), ulDisk, ulPart);

		if(NativeOpenFile(&hDevice, GENERIC_READ | SYNCHRONIZE, pszDevPath) == SRDERR_SUCCESS)
		{
			if(
				SrdGetDiskGeometry(hDevice, &lpDisk->Disks[ulCount].Geometry) == SRDERR_SUCCESS &&
				SrdGetDiskLayout(hDevice, &lpDisk->Disks[ulCount].Layout, &dynamic) == SRDERR_SUCCESS
			)
			{
                lpDisk->Disks[ulCount].Dynamic = dynamic;
                DbgPlain(DBG_MAIN_GLOBAL, _T("Disk (%ul) dynamic flag (%ul)."), ulDisk, dynamic);

				Return = SRDERR_SUCCESS;
			}

			NtClose(hDevice);
		}
		else
			DbgPlain(DBG_SYSNFOW2K, _T("NativeOpenFile failed."));
	}
	else
		DbgPlain(DBG_SYSNFOW2K, _T("GetDeviceNumber failed."));

	return Return;
}

#if !TARGET_WIN64
/* --------------------------------------------------------------------
|	Higher level function that collects all the important disk sectors.
|   This includes the boot sector of the boot partition, the MBR of all
|   disks attached and the boot sector of the Eisa Utility Partition (if
|   one exists).
----------------------------------------------------------------------*/
static R_RESULT
GetDiskSectors(tchar *pszPath, PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ulCount = lpDisk->DiskCount;

	if(lpDisk->Disks[ulCount].Layout != NULL)
	{
        R_ULONG	ii = 0;
        HANDLE	hDevice = INVALID_HANDLE_VALUE;
        R_BYTE	*Sector = MALLOC(lpDisk->Disks[ulCount].Geometry.BytesPerSector);

        if(Sector && NativeOpenFile(&hDevice, GENERIC_READ | SYNCHRONIZE, pszPath) == SRDERR_SUCCESS)
        {
			R_UINT64	Offset = 0;

			if(SrdReadDiskSector(hDevice, Offset, lpDisk->Disks[ulCount].Geometry.BytesPerSector, Sector) == SRDERR_SUCCESS)
			{
				lpDisk->Disks[ulCount].MBRs = MALLOC(lpDisk->Disks[ulCount].Geometry.BytesPerSector);

				if(lpDisk->Disks[ulCount].MBRs != NULL)
				{
					memcpy(lpDisk->Disks[ulCount].MBRs, Sector, lpDisk->Disks[ulCount].Geometry.BytesPerSector);
					lpDisk->Disks[ulCount].MBRsSize = lpDisk->Disks[ulCount].Geometry.BytesPerSector;

					Return = SRDERR_SUCCESS;

					for(ii = 0; Return == SRDERR_SUCCESS && ii < lpDisk->Disks[ulCount].Layout->PartitionCount; ii++)
					{
						PPartitionInformation	lpPart = lpDisk->Disks[ulCount].Layout->PartitionEntry[ii];
						R_BYTE					bPartType = (lpPart->PartitionType & (~VALID_NTFT));

						Return = SRDERR_ERROR;

						if(bPartType == 0x12)
						{
							if(lpDisk->EUPBoot == NULL)
							{
								Offset = lpPart->StartingOffset;

								if(SrdReadDiskSector(hDevice, Offset, lpDisk->Disks[ulCount].Geometry.BytesPerSector, Sector) == SRDERR_SUCCESS)
								{
									lpDisk->EUPBoot = MALLOC(lpDisk->Disks[ulCount].Geometry.BytesPerSector);

									if(lpDisk->EUPBoot != NULL)
									{
										memcpy(lpDisk->EUPBoot, Sector, lpDisk->Disks[ulCount].Geometry.BytesPerSector);
										lpDisk->EUPSize = lpDisk->Disks[ulCount].Geometry.BytesPerSector;

										Return = SRDERR_SUCCESS;
									}
								}
							}
							else
								Return = SRDERR_SUCCESS;
						}
						else if(lpPart->BootIndicator)
						{
							if(lpDisk->Disks[ulCount].BootSectorCode == NULL)
							{
								Offset = lpPart->StartingOffset;

								if(SrdReadDiskSector(hDevice, Offset, lpDisk->Disks[ulCount].Geometry.BytesPerSector, Sector) == SRDERR_SUCCESS)
								{
									lpDisk->Disks[ulCount].BootSectorCode = MALLOC(lpDisk->Disks[ulCount].Geometry.BytesPerSector);

									if(lpDisk->Disks[ulCount].BootSectorCode != NULL)
									{
										memcpy(lpDisk->Disks[ulCount].BootSectorCode, Sector, lpDisk->Disks[ulCount].Geometry.BytesPerSector);

										Return = SRDERR_SUCCESS;
									}
								}
							}
							else
								Return = SRDERR_SUCCESS;
						}
						else
							Return = SRDERR_SUCCESS;
					}
				}
			}
			NtClose(hDevice);
		}
        FREE(Sector);
	}

	return Return;
}

#endif

/* --------------------------------------------------------------------
|	Higher level function that collects all the important information
|   about the disk identified by the device path and ID.
----------------------------------------------------------------------*/
static R_RESULT
GetDiskDetails(tchar *pszPath, tchar *pszID, PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ulCount = lpDisk->DiskCount;
	R_ULONG		ulStatus = 0, ulProblem = 0;
	DEVINST		DevInst = 0;

	DbgPlain (DBG_SYSNFOW2K, _T("pszPath: %s."), pszPath);
	DbgPlain (DBG_SYSNFOW2K, _T("pszID: %s."), pszID);

	if(
		LocateDeviceNode(pszID, &DevInst) == SRDERR_SUCCESS &&
		GetDeviceStatus(DevInst, &ulStatus, &ulProblem) == SRDERR_SUCCESS &&
		((ulStatus & DN_STARTED) != 0 && (ulStatus & DN_DRIVER_LOADED) != 0)
	)
	{
		if(GetDiskStrings(DevInst, pszPath, pszID, lpDisk) == SRDERR_SUCCESS)
		{
			if(GetDiskCharacteristics(pszPath, lpDisk) == SRDERR_SUCCESS)
			{
#if TARGET_WIN64
				lpDisk->DiskCount += 1;

				Return = SRDERR_SUCCESS;
#else
				if(GetDiskSectors(pszPath, lpDisk) == SRDERR_SUCCESS)
				{
					lpDisk->DiskCount += 1;

					Return = SRDERR_SUCCESS;
				}
				else
					DbgPlain (DBG_SYSNFOW2K, _T("Error getting disk sectors."));
#endif
			}
			else
				DbgPlain (DBG_SYSNFOW2K, _T("Error getting disk characteristics."));
		}
		else
			DbgPlain (DBG_SYSNFOW2K, _T("Error getting disk strings."));
	}
	else
		DbgPlain (DBG_SYSNFOW2K, _T("Cannot locate device node or device status bad or device not started/driver not loaded."));

	return Return;
}

/* --------------------------------------------------------------------
|	A utility function that collects the names of all W2K volumes and
|   maps them to their respective drive letters. (C: -> \Harddisk0\Volume1).
----------------------------------------------------------------------*/
static R_RESULT
QueryDOSVolumes(PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_SUCCESS;
	R_ULONG		ulNext = 0;
	R_ULONG		ii = 0;

	lpDisk->DOSDevices = MALLOC(sizeof(DOSDevices));

	if(lpDisk->DOSDevices != NULL)
	{
		memset(lpDisk->DOSDevices, 0, sizeof(DOSDevices));

		for(ii = 0; ii < MAX_DRIVE_LETTERS; ii++)
		{
			tchar	pszDrive[16];
			tchar	pszVolume[MAX_PATH];

			sprintf(pszDrive, _T("%c:"), _T('A') + (tchar)ii);

			if(QueryDosDevice(pszDrive, pszVolume, MAX_PATH) != 0)
			{
				strcpy(lpDisk->DOSDevices->Devices[ulNext].VolumeName, pszVolume);
				lpDisk->DOSDevices->Devices[ulNext].DriveLetter = _T('A') + (R_TCHAR)ii;

				ulNext += 1;

				lpDisk->DOSDevices->DeviceCount = ulNext;
			}
		}
	}
	else
		DbgPlain (DBG_SYSNFOW2K, _T("Memory allocation failed."));

	DbgPlain (DBG_SYSNFOW2K, _T("Collected #%u DOS devices."), lpDisk->DOSDevices->DeviceCount);

	return Return;
}

/* --------------------------------------------------------------------
|	A utility function that connect drive letters to disk partitions in
|   a similar fashion as QueryDOSVolumes(), but works (with some limitations)
|   also on boot disks that were upgraded to dynamic disks. The limitations
|   of the function: it only connect the first partition of the complex
|   volume to the drive letter and it is only able to acomplish this if
|   a partition table(s) are present on the disk (not only a single LDM
|   partition table),
----------------------------------------------------------------------*/
static R_RESULT
QueryDiskVolumes(PDISKINFOW2K lpDisk)
{
    R_RESULT    Return = SRDERR_SUCCESS;
    R_ULONG     ii = 0, ulNext = 0;

    lpDisk->DiskDevices = MALLOC(sizeof(DISKDEVICES));

    if(lpDisk->DiskDevices != NULL)
    {
		memset(lpDisk->DiskDevices, 0, sizeof(DISKDEVICES));

        for(ii = 0; ii < MAX_DRIVE_LETTERS; ii++)
        {
            HANDLE  hVolume = INVALID_HANDLE_VALUE;

            DbgPlain(DBG_MAIN_GLOBAL, _T("Drive Letter: %c"), _T('A') + ii);
            if(SrdVolumeOpen(&hVolume, (R_ULONG)_T('A') + ii) == SRDERR_SUCCESS)
            {
                _TRY_
                    DWORD bytes;
                    R_UCHAR buffer[4096];
                    PVOLUME_DISK_EXTENTS de = (PVOLUME_DISK_EXTENTS)buffer;

                    memset(buffer, 0, 4096);
                    if(!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, de, 
                        4096, &bytes, NULL))
                    {
                        DbgPlain(DBG_MAIN_GLOBAL, 
                            _T("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed for volume %c"), _T('A') + ii);
                        _LEAVE_;
                    }
                    if (!de->NumberOfDiskExtents)
                    {
                        DbgPlain(DBG_MAIN_GLOBAL, _T("Invalid number of disk extents for volume %c"), _T('A') + ii);
                        _LEAVE_;
                    }

                    if (de->NumberOfDiskExtents > 1)
                    {
                        DbgPlain(DBG_MAIN_GLOBAL, 
                            _T("Warning: LDM volume %c detected. Only the first extent will be used."), _T('A') + ii);
                    }

                    lpDisk->DiskDevices->Devices[ulNext].DiskNumber = de->Extents[0].DiskNumber;
                    lpDisk->DiskDevices->Devices[ulNext].DriveLetter = (R_TCHAR)(_T('A') + ii);
                    lpDisk->DiskDevices->Devices[ulNext].StartingOffset = de->Extents[0].StartingOffset.QuadPart;
    
                    DbgPlain(DBG_MAIN_GLOBAL, _T("Volume data for volume %c:"), _T('A') + ii);
                    DbgPlain(DBG_MAIN_GLOBAL, _T("  Disk number: %u"), lpDisk->DiskDevices->Devices[ulNext].DiskNumber);
                    DbgPlain(DBG_MAIN_GLOBAL, _T("  Starting offset: %I64d"),
                        lpDisk->DiskDevices->Devices[ulNext].StartingOffset);

                    ulNext++;

                    lpDisk->DiskDevices->DeviceCount = ulNext;
                    
                _FINALLY_
                    if (hVolume != INVALID_HANDLE_VALUE)
                        SrdVolumeClose(hVolume);
                _ENDTRY_
            }
        }
    }

    return Return;
}

/* --------------------------------------------------------------------
|	Manufactures a buffer that will be used in a query of W2K mount
|   manager using a devices symbolic name.
----------------------------------------------------------------------*/
static R_VOID
CreateSymbolicMountPoint(R_BYTE *pBuffer, R_ULONG *pMountSize, tchar *pszSymbolic)
{
	PMOUNTMGR_MOUNT_POINT	pMountPoint = (PMOUNTMGR_MOUNT_POINT)pBuffer;
	R_USHORT				ulSymLen = (R_USHORT)strlen(pszSymbolic) * sizeof(WCHAR);

	memset(pMountPoint, 0, sizeof(MOUNTMGR_MOUNT_POINT));

	pMountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
	pMountPoint->SymbolicLinkNameLength = ulSymLen;

#if defined (UNICODE) || defined (_UNICODE)
	strncpy((LPWSTR)(pBuffer + pMountPoint->SymbolicLinkNameOffset), pszSymbolic, ulSymLen);
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszSymbolic, ulSymLen / sizeof(WCHAR), 
		(LPWSTR)(pBuffer + pMountPoint->SymbolicLinkNameOffset), ulSymLen);
#endif

	*pMountSize = sizeof(MOUNTMGR_MOUNT_POINT) + ulSymLen;
}

/* --------------------------------------------------------------------
|	Give a device path (\Harddisk0\Volume1) return the drive letter.
|   It is basically a walk through the array of volume to drive letter
|   mappings produces by the QueryDOSVolumes() function.
----------------------------------------------------------------------*/
static R_RESULT
GetVolumeDriveLetter(PDISKINFOW2K lpDisk, tchar *pszDeviceName, tchar *lpDriveLetter)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ii = 0; 
	
	for(ii = 0; ii < lpDisk->DOSDevices->DeviceCount; ii++)
	{
		if(strcmp(lpDisk->DOSDevices->Devices[ii].VolumeName, pszDeviceName) == 0)
		{
			*lpDriveLetter = lpDisk->DOSDevices->Devices[ii].DriveLetter;

			Return = SRDERR_SUCCESS;

			break;
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	A workhorse function for filling the W2K volume structure that is
|   subsequently used in various places.
----------------------------------------------------------------------*/
static R_VOID
FillVolume(PDISKINFOW2K lpDisk, R_ULONG ulCount, R_ULONG iDisk, R_ULONG iPart, R_TCHAR *pszDeviceName, R_TCHAR *device_guid)
{
	R_TCHAR	tchDL = 0;

    memset(&lpDisk->VolumeInfo->Volumes[ulCount].PartInfo, 0, sizeof(DkPartitionInfo));

	lpDisk->VolumeInfo->Volumes[ulCount].DiskNumber = lpDisk->Disks[iDisk].Number;
    strcpy(lpDisk->VolumeInfo->Volumes[ulCount].DiskSignature, lpDisk->Disks[iDisk].Layout->Signature);

	DbgPlain (DBG_SYSNFOW2K, _T(" DiskNumber: %#lx."), lpDisk->VolumeInfo->Volumes[ulCount].DiskNumber);
	DbgPlain (DBG_SYSNFOW2K, _T(" DiskSignature: %s."), lpDisk->VolumeInfo->Volumes[ulCount].DiskSignature);

#if defined (UNICODE) || defined (_UNICODE)
	strcpy(lpDisk->VolumeInfo->Volumes[ulCount].DeviceName, pszDeviceName);
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszDeviceName, -1, 
		lpDisk->VolumeInfo->Volumes[ulCount].DeviceName, MAX_PATH * sizeof(WCHAR));
#endif

	DbgPlain (DBG_SYSNFOW2K, _T(" DeviceName: '%ws'."), lpDisk->VolumeInfo->Volumes[ulCount].DeviceName);
        strcpy(lpDisk->VolumeInfo->Volumes[ulCount].DeviceGuid, device_guid);
        DbgPlain (DBG_SYSNFOW2K, _T(" Device Guid: '%s'."), lpDisk->VolumeInfo->Volumes[ulCount].DeviceGuid);

	if(GetVolumeDriveLetter(lpDisk, pszDeviceName, &tchDL) == SRDERR_SUCCESS)
		lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter = tchDL;
	else
		lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter = 0;

	DbgPlain (DBG_SYSNFOW2K, _T(" DriveLetter: '%C'."), lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter);

	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.StartingOffset = 
		lpDisk->Disks[iDisk].Layout->PartitionEntry[iPart]->StartingOffset;
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.FtLength =
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.Length = 
		lpDisk->Disks[iDisk].Layout->PartitionEntry[iPart]->PartitionLength;
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.Type = NON_FT;
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.FtState = HEALTHY;
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.DriveLetter = 
		(R_CHAR)lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter;
	if(lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter != 0)
		lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.AssignDriveLetter = TRUE;
	else
		lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.AssignDriveLetter = FALSE;
	lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.PartitionNumber = (R_USHORT)
		lpDisk->Disks[iDisk].Layout->PartitionEntry[iPart]->PartitionNumber;

	lpDisk->VolumeInfo->Volumes[ulCount].SetMember = 0;
	lpDisk->VolumeInfo->Volumes[ulCount].SetGroup = 0;
	lpDisk->VolumeInfo->Volumes[ulCount].SetType = NON_FT;

	DbgPlain (DBG_SYSNFOW2K, _T(" StartingOffset: %I64u."), lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.StartingOffset);
	DbgPlain (DBG_SYSNFOW2K, _T(" Length: %I64u."), lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.Length);
}

/* --------------------------------------------------------------------
|	A workhorse function for adding a volume to the array of volumes
|   present on the current system.
----------------------------------------------------------------------*/
static R_RESULT
AddVolume(PDISKINFOW2K lpDisk, R_ULONG iDisk, R_ULONG iPart, R_TCHAR *device_name, R_TCHAR *device_guid)
{
	R_RESULT	Return = SRDERR_SUCCESS;

    if(lpDisk->VolumeInfo == NULL)
	{
		lpDisk->VolumeInfo = MALLOC(sizeof(VOLUMEINFOW2K));
		memset(lpDisk->VolumeInfo, 0, sizeof(VOLUMEINFOW2K));
	}
	else
		lpDisk->VolumeInfo = REALLOC(lpDisk->VolumeInfo, sizeof(VOLUMEINFOW2K) + 
			(lpDisk->VolumeInfo->VolumeCount + 1) * sizeof(VOLUMEPARTW2K));

	if(lpDisk->VolumeInfo != NULL)
	{
        R_ULONG ii;
        for (ii = 0; ii < lpDisk->VolumeInfo->VolumeCount; ii++)
        {
            if (stricmp(device_name, lpDisk->VolumeInfo->Volumes[ii].DeviceName) == 0)
            {
                DbgPlain(DBG_SYSNFOW2K, _T("Skipping volume with device name (%s)."), device_name);
                return Return;
            }
        }

        if (lpDisk->Disks[iDisk].Layout->PartitionEntry[iPart]->PartitionType == PARTITION_LDM)
        {
            R_TCHAR tchDL = 0;
            if (GetVolumeDriveLetter(lpDisk, device_name, &tchDL) == SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFOW2K, _T("Detected LDM partition with drive letter (%c)."), tchDL);
            }
            else
            {
                DbgPlain(DBG_SYSNFOW2K, _T("Detected LDM partition with device name (%s)."), device_name);
            }
        }

        FillVolume(lpDisk, lpDisk->VolumeInfo->VolumeCount, iDisk, iPart, device_name, device_guid);

		lpDisk->VolumeInfo->VolumeCount += 1;
		
		Return = SRDERR_SUCCESS;
	}
	else
		DbgPlain (DBG_SYSNFOW2K, _T("Memory allocation/reallocation failed."));

	return Return;
}
/* Get data volume */
static R_RESULT
GetVolumeData(R_ULONG DiskNumber, R_UINT64 StartingOffset, R_TCHAR *DeviceName, R_TCHAR *VolumeGuid)
{
    R_RESULT Return = SRDERR_ERROR;

    TCHAR volume_name[MAX_PATH] = { _T('\0') };
    TCHAR volume_guid[MAX_PATH] = { _T('\0') };

    HANDLE find = FindFirstVolume(volume_name, MAX_PATH);
    if (INVALID_HANDLE_VALUE != find)
    {
        do 
        {
            HANDLE vol = NULL;
            R_TCHAR device_name[MAX_PATH] = { _T('\0') };

            /* TODO: Find some triming function in libcmn to do trimming of last backslash. */
            strcpy(volume_guid, volume_name);
            if (volume_name[strlen(volume_name)-1] == _T('\\'))
                volume_guid[strlen(volume_guid)-1] = _T('\0');
            strcpy(volume_name, volume_guid + 4);

            DbgPlain(DBG_MAIN_GLOBAL, _T("Handling volume (%s) for disk (%d) and offset(%I64d)."),
                            volume_guid, DiskNumber, StartingOffset);

            vol = CreateFile(volume_guid, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (INVALID_HANDLE_VALUE == vol)
            {
                DbgPlain(DBG_MAIN_GLOBAL, _T("The opening of volume (%s) failed (%d)."), volume_guid, GetLastError());
                continue;
            }

            _TRY_
                ULONG ii = 0;
                DWORD bytes = 0;
                R_UCHAR buffer[STRLEN_1K] = { _T('\0') };
                PVOLUME_DISK_EXTENTS de = (PVOLUME_DISK_EXTENTS)buffer;

                memset(buffer, 0, STRLEN_1K);
                if (!DeviceIoControl(vol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, de,  /* parasoft-suppress  CODSTA-12 "This is a Microsoft enum value" */
                    STRLEN_1K, &bytes, NULL))
                {
                    DbgPlain(DBG_MAIN_GLOBAL, _T("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed (%d) for volume (%s)."),
                                    GetLastError(), volume_guid);
                    _LEAVE_;
                }
                if (!de->NumberOfDiskExtents)
                {
                    DbgPlain(DBG_MAIN_GLOBAL, _T("Invalid number of disk extents (0) for volume (%s)."), volume_guid);
                    _LEAVE_;
                }

                for (ii = 0; ii < de->NumberOfDiskExtents; ++ii)
                {
                    if (de->Extents[ii].DiskNumber == DiskNumber &&
                        de->Extents[ii].StartingOffset.QuadPart == StartingOffset)
                    {
                        break;
                    }
                }
                if (ii >= de->NumberOfDiskExtents)
                {
                    _LEAVE_;
                }

                if (!QueryDosDevice(volume_name, device_name, MAX_PATH))
                {
                    DbgPlain(DBG_MAIN_GLOBAL, 
                        _T("QueryDosDevice failure (%d) for volume (%s)."), GetLastError(), volume_guid);
                    _LEAVE_;
                }

                strcpy(DeviceName, device_name);
                strcpy(VolumeGuid, volume_guid);

            _FINALLY_
                CloseHandle(vol);
            _ENDTRY_

            if (device_name[0] != _T('\0'))
            {
                Return = SRDERR_SUCCESS;
                break;
            }

        }
        while (FindNextVolume(find, volume_name, MAX_PATH));
        
        FindVolumeClose(find);
    }

    return(Return);
}
/* --------------------------------------------------------------------
|	A high level function that establishes the volume information key
|   that holds information about the volumes on the system and their
|   connection the underlying disk partitions.
----------------------------------------------------------------------*/
static R_RESULT
CreateVolumeInformationKey(PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_SUCCESS;
	R_ULONG		iDisk = 0, iPart = 0;

	for(iDisk = 0; Return == SRDERR_SUCCESS && iDisk < lpDisk->DiskCount; iDisk++)
	{
		for(iPart = 0; Return == SRDERR_SUCCESS && iPart < lpDisk->Disks[iDisk].Layout->PartitionCount; iPart++)
		{
			PPartitionInformation	lpPart = lpDisk->Disks[iDisk].Layout->PartitionEntry[iPart];
            R_TCHAR device_name[MAX_PATH] = {_T('\0')};
            R_TCHAR device_guid[MAX_PATH] = {_T('\0')};

			Return = SRDERR_ERROR;

            if (lpPart == NULL)
            {
                DbgPlain (DBG_SYSNFOW2K, _T("PartitionEntry for partition [%d] is empty!!!"), iPart);
            }

			DbgPlain (DBG_SYSNFOW2K, _T("Querying signature [%s] and starting offset [%I64u]."), lpDisk->Disks[iDisk].Layout->Signature, 
                lpPart->StartingOffset);

            DbgPlain (DBG_SYSNFOW2K, _T(" Partition Info:"));
            DbgPlain (DBG_SYSNFOW2K, _T("  Disk Number [%lu]."),
                lpDisk->Disks[iDisk].Number);
            if (lpPart->BootIndicator)
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Boot Indicator [true]."));
            }
            else
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Boot Indicator [false]."));
            }
            DbgPlain (DBG_SYSNFOW2K, _T("  Hidden Sectors [%u]."),
                lpPart->HiddenSectors);
            DbgPlain (DBG_SYSNFOW2K, _T("  Partition Length [%I64u]."),
                lpPart->PartitionLength);
            DbgPlain (DBG_SYSNFOW2K, _T("  Partition Number [%u]."),
                lpPart->PartitionNumber);
            DbgPlain (DBG_SYSNFOW2K, _T("  Partition Type [%#x]."),
                (R_UINT)lpPart->PartitionType);
            if (lpPart->RecognizedPartition)
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Recognized Partition [true]."));
            }
            else
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Recognized Partition [false]."));
            }
            if (lpPart->RewritePartition)
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Rewrite Partition [true]."));
            }
            else
            {
                DbgPlain (DBG_SYSNFOW2K, _T("  Rewrite Partition [false]."));
            }
            DbgPlain (DBG_SYSNFOW2K, _T("  Starting Offset [%I64u]."),
                lpPart->StartingOffset);

            if (GetVolumeData(lpDisk->Disks[iDisk].Number, lpPart->StartingOffset, device_name, device_guid) == SRDERR_SUCCESS)
			{
				DbgPlain (DBG_SYSNFOW2K, _T("Calling AddVolume() [iDisk: %lu, iPart: %lu]."), iDisk, iPart);

                if(AddVolume(lpDisk, iDisk, iPart, device_name, device_guid) == SRDERR_SUCCESS)
					Return = SRDERR_SUCCESS;
				else
					DbgPlain (DBG_SYSNFOW2K, _T("AddVolume failed."));
			}
            else
			{
                DbgPlain (DBG_SYSNFOW2K, _T("GetVolumeData() did not discover device name for disk (%d), partition (%d)."), iDisk, iPart, device_name);
				
				Return = SRDERR_SUCCESS;
			}
		}
	}

	DbgPlain (DBG_SYSNFOW2K, _T("Evaluated all volumes, hopefully successfully."));

	return Return;
}

/* --------------------------------------------------------------------
|	A high level function that updates the volume information by using
|   the data collected by the QueryDiskVolumes() function. This function
|   updates only those volumes that are not already part of the volume
|   information key. This happens mostly with LDM upgraded disks.
----------------------------------------------------------------------*/
static R_RESULT
UpdateVolumeInformationKey(PDISKINFOW2K lpDisk)
{
    R_RESULT    Return = SRDERR_SUCCESS;
    R_ULONG     ii = 0, jj = 0, kk = 0, ll = 0;
    R_BOOL      bVolumeFound = FALSE, bVolumeFilled = FALSE;

    if(lpDisk->DiskDevices != NULL)
    {
        for(ii = 0; ii < MAX_DRIVE_LETTERS; ii++)
        {
            bVolumeFound = FALSE; bVolumeFilled = FALSE;

            if(lpDisk->VolumeInfo != NULL)
            {
                for(jj = 0; jj < lpDisk->VolumeInfo->VolumeCount; jj++)
                {
                    if(lpDisk->VolumeInfo->Volumes[jj].DriveLetter == (R_ULONG)_T('A') + ii)
                        bVolumeFound = TRUE;
                }
            }

            if(bVolumeFound == FALSE)
            {
                for(jj = 0; jj < lpDisk->DiskDevices->DeviceCount; jj++)
                {
                    if(lpDisk->DiskDevices->Devices[jj].DriveLetter == (R_TCHAR)(_T('A') + ii))
                    {
                        if(lpDisk->VolumeInfo == NULL)
		                {
			                lpDisk->VolumeInfo = MALLOC(sizeof(VOLUMEINFOW2K));
			                memset(lpDisk->VolumeInfo, 0, sizeof(VOLUMEINFOW2K));
		                }
		                else
			                lpDisk->VolumeInfo = REALLOC(lpDisk->VolumeInfo, sizeof(VOLUMEINFOW2K) + 
				                (lpDisk->VolumeInfo->VolumeCount + 1) * sizeof(VOLUMEPARTW2K));

		                if(lpDisk->VolumeInfo != NULL)
		                {
                            for(kk = 0; bVolumeFilled == FALSE && kk < lpDisk->DiskCount; kk++)
                            {
                                if(lpDisk->Disks[kk].Number == lpDisk->DiskDevices->Devices[jj].DiskNumber)
                                {
                                    for(ll = 0; bVolumeFilled == FALSE && ll < lpDisk->Disks[kk].Layout->PartitionCount; ll++)
                                    {
                                        if(
                                            lpDisk->Disks[kk].Layout->PartitionEntry[ll]->StartingOffset == 
                                            lpDisk->DiskDevices->Devices[jj].StartingOffset
                                        )
                                        {
                                            R_ULONG ulCount = lpDisk->VolumeInfo->VolumeCount;

                                            if (lpDisk->Disks[kk].Layout->PartitionEntry[ll]->PartitionType == 
                                                PARTITION_LDM)
                                            {
                                                DbgPlain(
                                                    DBG_SYSNFOW2K,
                                                    _T("Detected LDM partition with drive letter (%c)."),
                                                    lpDisk->DiskDevices->Devices[jj].DriveLetter);
                                            }

                                            FillVolume(lpDisk, ulCount, kk, ll, _T(""), _T(""));

                                            lpDisk->VolumeInfo->Volumes[ulCount].DriveLetter = lpDisk->DiskDevices->Devices[jj].DriveLetter;
                                            lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.DriveLetter = (R_CHAR)lpDisk->DiskDevices->Devices[jj].DriveLetter;
                                            lpDisk->VolumeInfo->Volumes[ulCount].PartInfo.AssignDriveLetter = TRUE;

                                            lpDisk->VolumeInfo->VolumeCount += 1;

                                            bVolumeFilled = TRUE;
                                        }
                                    }
                                }
                            }
                        }

                        break;
                    }
                }
            }
        }
    }

    return Return;
}

/* --------------------------------------------------------------------
|	GetVolumeMtpointDosName() function searches for a DOS name (drive
|   letter) that is related to the given volume mountpoint name (if
|   related at all).
----------------------------------------------------------------------*/
static R_RESULT
GetVolumeMtpointDosName(PDISKINFOW2K lpDisk, R_TCHAR *pszVol, R_TCHAR *pszOut)
{
	R_RESULT	Return = SRDERR_ERROR;
	HANDLE		hMount = INVALID_HANDLE_VALUE;

	hMount = CreateFile(_T("\\\\.\\MountPointManager"), GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		INVALID_HANDLE_VALUE);

	if(hMount != INVALID_HANDLE_VALUE)
	{
		R_BYTE	pMountPoint[2048];
		R_BYTE	pOutput[8192];
		R_ULONG	ulMountSize = 0;
		R_DWORD	dwBytes = 0;

		CreateSymbolicMountPoint(pMountPoint, &ulMountSize, pszVol);

		if(DeviceIoControl(hMount, IOCTL_MOUNTMGR_QUERY_POINTS, pMountPoint, ulMountSize, pOutput,
				8192, &dwBytes, NULL) == TRUE)
		{
			R_ULONG					ii = 0;
			PMOUNTMGR_MOUNT_POINTS	pPoints = (PMOUNTMGR_MOUNT_POINTS)pOutput;

			for(ii = 0; Return == SRDERR_ERROR && ii < pPoints->NumberOfMountPoints; ii++)
			{
				R_INT					iLen = 0;
				PMOUNTMGR_MOUNT_POINT	pPoint = (PMOUNTMGR_MOUNT_POINT)&pPoints->MountPoints[ii];

				if(
					pPoint->SymbolicLinkNameOffset != 0 &&
					pPoint->SymbolicLinkNameLength != 0
				)
				{
					if(
						strlen(pszVol) == pPoint->SymbolicLinkNameLength / sizeof(tchar) &&
						strnicmp(
							(tchar*)((R_UCHAR*)pPoints + pPoint->SymbolicLinkNameOffset),
							pszVol,
							pPoint->SymbolicLinkNameLength / sizeof(tchar)
						) == 0
					)
					{
						tchar	tchDl = 0;
						tchar	pszDevName[MAX_PATH];

						memcpy(pszDevName, (R_UCHAR*)pPoints + pPoint->DeviceNameOffset, pPoint->DeviceNameLength);
						pszDevName[pPoint->DeviceNameLength / sizeof(tchar)] = _T('\0');

						if(GetVolumeDriveLetter(lpDisk, pszDevName, &tchDl) == SRDERR_SUCCESS)
						{
							sprintf(pszOut, _T("%C:\\"), tchDl);

							Return = SRDERR_SUCCESS;
						}
					}
				}
			}
		}
		else
		{
			DbgPlain (DBG_SYSNFOW2K, _T("DeviceIoControl failed (%#lx)."), GetLastError());
			
			Return = SRDERR_ERROR;
		}

		CloseHandle(hMount);
	}
	else
		DbgPlain (DBG_SYSNFOW2K, _T("Creation of MountManager object failed."));

	return Return;
}

/* --------------------------------------------------------------------
|	A workhorse function for adding a volume mountpoint information
|   structure to the array of structures describing volume mountpoint
|   information.
----------------------------------------------------------------------*/
static R_RESULT
AddVolumeMountPoint(PVOLMTINFOPARENT *ppParent, R_TCHAR *pszVol, R_TCHAR *pszVolMt, R_TCHAR *pszPointsTo)
{
	R_RESULT			Return = SRDERR_ERROR;
	PVOLMTINFOPARENT	pNew = *ppParent;

	if(pNew->ulCount > 0)
	{
		R_ULONG	ulSize = sizeof(VOLMTINFOPARENT) + pNew->ulCount * sizeof(PVOLMTINFOCHILD);
		pNew = REALLOC(pNew, ulSize);

		if(pNew != NULL)
			pNew->MountPoints[pNew->ulCount] = NULL;
	}
	else
		pNew->pszRoot = StrNewCopy(pszVol);

	if(
		pNew != NULL &&
		pNew->pszRoot != NULL
	)
	{
		pNew->MountPoints[pNew->ulCount] = MALLOC(sizeof(VOLMTINFOCHILD));
		if(pNew->MountPoints[pNew->ulCount] != NULL)
		{
			pNew->MountPoints[pNew->ulCount]->pszName = StrNewCopy(pszVolMt);
			pNew->MountPoints[pNew->ulCount]->pszPointsTo = StrNewCopy(pszPointsTo);

			if(
				pNew->MountPoints[pNew->ulCount]->pszName != NULL &&
				pNew->MountPoints[pNew->ulCount]->pszPointsTo != NULL
			)
			{
				pNew->ulCount += 1;
				*ppParent = pNew;

				Return = SRDERR_SUCCESS;
			}
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	A highlevel function that collects all the information about the
|   volume mountpoints that exist on the system. It loops through all
|   named volumes (volumes that has drive letters assigned) and searches
|   for the directories on these volumes that are actually a reparse
|   points that point to some other (possibly unnamed) volumes. The
|   limitation of this function is that it does not bother very much
|   to look for nested volume mountpoints (volume mountpoints that
|   are actually part of unnamed volumes and point to some other volumes).
----------------------------------------------------------------------*/
static R_RESULT
CollectVolumeMtpointInfo(PDISKINFOW2K lpDisk)
{
    R_RESULT	Return = SRDERR_ERROR;
    PVOLMTINFO	pVolMt = NULL;
    HANDLE		hFind = INVALID_HANDLE_VALUE;

    __try
    {
        R_ULONG	ii = 0;

        for(ii = 0; ii < MAX_DRIVE_LETTERS; ii++)
        {
            R_TCHAR	pszVol[16];
            R_TCHAR	pszVolMt[MAX_PATH];

            sprintf(pszVol, _T("%c:\\"), _T('A') + ii);

            if(GetDriveType(pszVol) == DRIVE_FIXED)
            {
                hFind = FindFirstVolumeMountPoint(pszVol, pszVolMt, MAX_PATH);
                if(hFind != INVALID_HANDLE_VALUE)
                {
                    R_ULONG		ulSize = 0, ulCurrent = (pVolMt ? pVolMt->ulCount : 0);

                    if(pVolMt == NULL)
                    {
                        ulSize = sizeof(VOLMTINFO);
                        pVolMt = MALLOC(ulSize);
                        if(pVolMt == NULL)
                            __leave;
                        memset(pVolMt, 0, ulSize);
                    }
                    else
                    {
                        ulSize = sizeof(VOLMTINFO) + ulCurrent * sizeof(PVOLMTINFOPARENT);
                        pVolMt = REALLOC(pVolMt, ulSize);
                        if(pVolMt == NULL)
                            __leave;
                        memset(&pVolMt->VolMtInfos[ulCurrent], 0, sizeof(PVOLMTINFOPARENT));
                    }

                    pVolMt->VolMtInfos[ulCurrent] = MALLOC(sizeof(VOLMTINFOPARENT));
                    if(pVolMt->VolMtInfos[ulCurrent] == NULL)
                        __leave;
                    memset(pVolMt->VolMtInfos[ulCurrent], 0, sizeof(VOLMTINFOPARENT));
                    pVolMt->ulCount += 1;

                    do {
                        R_TCHAR	pszVolLet[MAX_PATH];
                        R_TCHAR	pszSrcPath[MAX_PATH];
                        R_TCHAR	pszName[MAX_PATH];

                        *pszVolLet = _T('\0');

                        sprintf(pszSrcPath, _T("%s%s"), pszVol, pszVolMt);
                        if(GetVolumeNameForVolumeMountPoint(pszSrcPath, pszName, MAX_PATH) != 0)
                        {
                            pszName[1] = _T('?');
                            pszName[strlen(pszName) - 1] = _T('\0');

                            GetVolumeMtpointDosName(lpDisk, pszName, pszVolLet);
                            if (pszVolLet[0] == _T('\0'))
                            {
                                pszName[1] = _T('\\');
                                strcpy(pszVolLet, pszName);
                            }
                        }

                        if(AddVolumeMountPoint(&pVolMt->VolMtInfos[ulCurrent], pszVol, pszVolMt, pszVolLet) != SRDERR_SUCCESS)
                            __leave;
                    } while(FindNextVolumeMountPoint(hFind, pszVolMt, MAX_PATH));

                    FindVolumeMountPointClose(hFind);
                    hFind = INVALID_HANDLE_VALUE;
                }
            }
        }

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(Return != SRDERR_SUCCESS)
            FreeVolumeMountpointInfo(pVolMt);
        else
            lpDisk->VolMtInfo = pVolMt;

        if(hFind != INVALID_HANDLE_VALUE)
            FindVolumeMountPointClose(hFind);
    }

    return Return;
}

/* --------------------------------------------------------------------
|	The highest level function that loops through all the disks using the
|   W2K configuration manager facility. The information collected is
|   then fed to other post processing functions (as CreateVolumeInformationKey(),
|   etc.) to obtain the final version of the user system's disks.
----------------------------------------------------------------------*/
R_RESULT
EnumDisks(PDISKINFOW2K lpDisk)
{
	R_RESULT	Return = SRDERR_ERROR;
	R_ULONG		ulSize = 0;

    if(CM_Get_Device_Interface_List_Size(&ulSize, (LPGUID)&DiskClassGuid, NULL, 0) == CR_SUCCESS)
    {
		tchar	*pszIfcList = (LPTSTR)MALLOC(ulSize * sizeof(TCHAR));

		if(pszIfcList != NULL)
		{
			if(CM_Get_Device_Interface_List_Ex((LPGUID)&DiskClassGuid, NULL, pszIfcList, 
				ulSize, CM_GET_DEVICE_INTERFACE_LIST_PRESENT, NULL) == CR_SUCCESS)
			{
				tchar	*pszCur = pszIfcList;
				
				Return = SRDERR_SUCCESS;

				while(
					Return == SRDERR_SUCCESS &&
					*pszCur != _T('\0')
				)
				{
					HDEVINFO	hInfoList = NULL;

					Return = SRDERR_SUCCESS;

					hInfoList = SetupDiCreateDeviceInfoList(NULL, NULL);
					if(hInfoList != INVALID_HANDLE_VALUE)
					{
						SP_DEVICE_INTERFACE_DATA DevIfcData;

						memset(&DevIfcData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
						DevIfcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

						if(SetupDiOpenDeviceInterface(hInfoList, pszCur, 0, &DevIfcData) == TRUE)
						{
							R_BOOL			bResult = FALSE;
							R_DWORD			dwRequiredSize = 0;
							SP_DEVINFO_DATA	DevInfoData;

							memset(&DevInfoData, 0, sizeof(SP_DEVINFO_DATA));
							DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

							bResult = SetupDiGetDeviceInterfaceDetail(hInfoList, &DevIfcData, NULL, 0, &dwRequiredSize, &DevInfoData);

							if(
								bResult == TRUE ||
								GetLastError() == ERROR_INSUFFICIENT_BUFFER
							)
							{
								ULONG	ulIdSize = 0;

								if(CM_Get_Device_ID_Size_Ex(&ulIdSize, DevInfoData.DevInst, 0L, NULL) == CR_SUCCESS)
								{
									tchar	*pszDevId = MALLOC((ulIdSize + 1) * sizeof(TCHAR));

									if(pszDevId != NULL)
									{
										if(CM_Get_Device_ID(DevInfoData.DevInst, pszDevId, ulIdSize + 1, 0L) == CR_SUCCESS)
										{
											pszDevId[ulIdSize] = L'\0';

											if(GetDiskDetails(pszCur, pszDevId, lpDisk) != SRDERR_SUCCESS)
											{
												Return = SRDERR_SUCCESS;
											}
										}
										else
											DbgPlain (DBG_SYSNFOW2K, _T("CMGet_Device_ID failed."));
										
										FREE(pszDevId);
									}
									else
										DbgPlain (DBG_SYSNFOW2K, _T("Memory allocation failure."));
								}
								else
									DbgPlain (DBG_SYSNFOW2K, _T("CMGet_Device_ID_Size_Ex failed."));
							}
							else
								DbgPlain (DBG_SYSNFOW2K, _T("SpDiGetDeviceInterfaceDetail failed."));
						}
						else
							DbgPlain (DBG_SYSNFOW2K, _T("SpDiOpenDeviceInterface failed."));

						SetupDiDestroyDeviceInfoList(hInfoList);
					}
					else
						DbgPlain (DBG_SYSNFOW2K, _T("SpDiCreateDeviceInfoList failed."));

					pszCur += strlen(pszCur) + 1;
				}
				
				if(
					Return == SRDERR_SUCCESS &&
					lpDisk->DiskCount < 1
				)
				{
					DbgPlain (DBG_SYSNFOW2K, _T("There has been less than 1 disk enumerated. Cannot proceed."));
					
					Return = SRDERR_ERROR;
				}
				/* --------------------------------------------------------------------
				|	All disks have been successfully stored.
				----------------------------------------------------------------------*/
				if(Return == SRDERR_SUCCESS)
				{
					DOSDevices	DOS;

					Return = SRDERR_ERROR;

					memset(&DOS, 0, sizeof(DOSDevices));

					if(
						QueryDOSVolumes(lpDisk) == SRDERR_SUCCESS &&
                        QueryDiskVolumes(lpDisk) == SRDERR_SUCCESS &&
						CreateVolumeInformationKey(lpDisk) == SRDERR_SUCCESS &&
                        UpdateVolumeInformationKey(lpDisk) == SRDERR_SUCCESS
					)
					{
						if(CollectVolumeMtpointInfo(lpDisk) == SRDERR_SUCCESS)
						{
							Return = SRDERR_SUCCESS;
						}
					}
					else
						DbgPlain (DBG_SYSNFOW2K, _T("QueryDOSVolumes/QueryDiskVolumes/CreateVolumeInformationKey failed."));
				}
				else
					DbgPlain (DBG_SYSNFOW2K, _T("Something wrong while enumerating disk devices."));
			}
			else
				DbgPlain (DBG_SYSNFOW2K, _T("CMGet_Device_Interface_List_Ex failed."));

			FREE(pszIfcList);
		}
		else
			DbgPlain (DBG_SYSNFOW2K, _T("Memory allocation failure."));
	}
	else
		DbgPlain (DBG_SYSNFOW2K, _T("CMGet_Device_Interface_List_Size failed."));

	return Return;
}

/* --------------------------------------------------------------------
|	The ConvertDisks() function is neccessary because srdw2k.c module
|   uses its internal structures that helps it to consistently handle
|   all the system queries. Only after all the information is available
|   is it possible to transfer it to the common form used by the SRD
|   library functions.
----------------------------------------------------------------------*/
R_RESULT
ConvertDisks(PDISKINFOW2K pDiskInfoW2k, PDISKINFO *ppDiskInfo)
{
	R_RESULT	Return = SRDERR_ERROR;
	PDISKINFO	pDi = MALLOC(sizeof(DISKINFO));

	if(pDi != NULL)
	{
		R_ULONG		ii = 0;

		pDi->DiskCount = pDiskInfoW2k->DiskCount;

		for(ii = 0; ii < pDi->DiskCount; ii++)
		{
			R_ULONG		jj = 0;

			pDi->Disks[ii] = MALLOC(sizeof(DISK));
			if(pDi == NULL)
				break;

			/* GENERAL */
			pDi->Disks[ii]->General = MALLOC(sizeof(GENERAL_DISK_INFORMATION));
			if(pDi->Disks[ii]->General == NULL)
				break;
			pDi->Disks[ii]->General->Number = pDiskInfoW2k->Disks[ii].Number;
			strcpy(pDi->Disks[ii]->General->Description, pDiskInfoW2k->Disks[ii].Description);
			pDi->Disks[ii]->General->Address = 0;
			pDi->Disks[ii]->General->Size = 0;

			/* DRIVE LAYOUT */
			pDi->Disks[ii]->Layout = pDiskInfoW2k->Disks[ii].Layout;
			pDiskInfoW2k->Disks[ii].Layout = NULL;

			/* DISK_GEOMETRY */
			pDi->Disks[ii]->Geometry = MALLOC(sizeof(DiskGeometry));
			if(pDi->Disks[ii]->Geometry == NULL)
				break;

			memcpy(pDi->Disks[ii]->Geometry, &pDiskInfoW2k->Disks[ii].Geometry, sizeof(DiskGeometry));

			/* BOOT_SECTOR_CODE */

			if(pDiskInfoW2k->Disks[ii].BootSectorCode != NULL)
			{
				pDi->Disks[ii]->BootSectorCode = pDiskInfoW2k->Disks[ii].BootSectorCode;
				pDiskInfoW2k->Disks[ii].BootSectorCode = NULL;
			}
			else
				pDi->Disks[ii]->BootSectorCode = NULL;

			/* MBR CODE */
			if(pDiskInfoW2k->Disks[ii].MBRs != NULL)
			{
				pDi->Disks[ii]->MBR = pDiskInfoW2k->Disks[ii].MBRs;
				pDiskInfoW2k->Disks[ii].MBRs = NULL;
			}
			else
				pDi->Disks[ii]->MBR = NULL;
		}

		if(ii >= pDi->DiskCount)
		{
			Return = SRDERR_SUCCESS;

			/* EUP_CODE */
			if(pDiskInfoW2k->EUPBoot != NULL)
			{
				pDi->EUPBoot = pDiskInfoW2k->EUPBoot;
				pDi->EUPBootSize = pDiskInfoW2k->EUPSize;

				pDiskInfoW2k->EUPBoot = NULL; pDiskInfoW2k->EUPSize = 0;
			}
			else
			{
				pDi->EUPBoot = NULL;
				pDi->EUPBootSize = 0;
			}

			/* VOLUME_INFORMATION */
			if(
				Return == SRDERR_SUCCESS &&
				pDiskInfoW2k->VolumeInfo != NULL
			)
			{
				R_ULONG		ulSize = sizeof(VOLUMEINFO) + (pDiskInfoW2k->VolumeInfo->VolumeCount - 1) * sizeof(PVOLUMEPART);
				
				pDi->VolumeInfo = MALLOC(ulSize);
				if(pDi->VolumeInfo != NULL)
				{
					pDi->VolumeInfo->VolumeCount = pDiskInfoW2k->VolumeInfo->VolumeCount;

					for(ii = 0; ii < pDi->VolumeInfo->VolumeCount; ii++)
					{
						pDi->VolumeInfo->Volumes[ii] = MALLOC(sizeof(VOLUMEPART));
						if(pDi->VolumeInfo->Volumes[ii] == NULL)
							break;
						
						pDi->VolumeInfo->Volumes[ii]->DiskNumber = pDiskInfoW2k->VolumeInfo->Volumes[ii].DiskNumber;
                                                strcpy(pDi->VolumeInfo->Volumes[ii]->DiskSignature, pDiskInfoW2k->VolumeInfo->Volumes[ii].DiskSignature);
						pDi->VolumeInfo->Volumes[ii]->DriveLetter = pDiskInfoW2k->VolumeInfo->Volumes[ii].DriveLetter;
                        strcpy(pDi->VolumeInfo->Volumes[ii]->deviceGuid,
                            pDiskInfoW2k->VolumeInfo->Volumes[ii].DeviceGuid);
						pDi->VolumeInfo->Volumes[ii]->StartingOffset = pDiskInfoW2k->VolumeInfo->Volumes[ii].PartInfo.StartingOffset;
						pDi->VolumeInfo->Volumes[ii]->Length = pDiskInfoW2k->VolumeInfo->Volumes[ii].PartInfo.Length;
						pDi->VolumeInfo->Volumes[ii]->FtType = pDiskInfoW2k->VolumeInfo->Volumes[ii].SetType;
						pDi->VolumeInfo->Volumes[ii]->FtGroup = (R_USHORT)pDiskInfoW2k->VolumeInfo->Volumes[ii].SetGroup;
						pDi->VolumeInfo->Volumes[ii]->FtMember = (R_USHORT)pDiskInfoW2k->VolumeInfo->Volumes[ii].SetMember;
					}

					if(ii < pDi->VolumeInfo->VolumeCount)
						Return = SRDERR_ERROR;
				}
				else
					Return = SRDERR_ERROR;
			}
			else
				pDi->VolumeInfo = NULL;

			/* VOLUME_MOUNTPOINT_INFORMATION */
			if(
				Return == SRDERR_SUCCESS &&
				pDiskInfoW2k->VolMtInfo != NULL
			)
			{
				pDi->VolumeMountpointInfo = pDiskInfoW2k->VolMtInfo;
				pDiskInfoW2k->VolMtInfo = NULL;
			}
			else
				pDi->VolumeMountpointInfo = NULL;

		}	
	}

	if(Return == SRDERR_SUCCESS)
		*ppDiskInfo = pDi;
	else
	{
		FreeDiskInfo(pDi);
		FREE(pDi);
	}

	DiscardDisks(pDiskInfoW2k);

	return Return;
}

static R_RESULT
FindMountpointPathByVolumeGuid(PVOLMTINFO pVolMt, R_TCHAR *pszGuid, R_TCHAR *pszOut)
{
    R_RESULT    Return = SRDERR_ERROR;

    if(pVolMt != NULL && pszGuid != NULL && pszOut != NULL)
    {
        R_ULONG ii = 0;
        R_TCHAR pszMountpoint[MAX_PATH] = { _T('\0') };

        for(ii = 0; Return == SRDERR_ERROR && ii < pVolMt->ulCount; ii++)
        {
            R_ULONG jj = 0;

            for(jj = 0; Return == SRDERR_ERROR && jj < pVolMt->VolMtInfos[ii]->ulCount; jj++)
            {
                strcpy(pszMountpoint, pVolMt->VolMtInfos[ii]->pszRoot);
                PathCutTrailSlashes(pszMountpoint, 0);
                strcat(pszMountpoint, _T("\\"));

                strcat(pszMountpoint, pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszName);
                PathCutTrailSlashes(pszMountpoint, 0);

                if(NULL != pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo &&
                   stricmp(pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo, pszGuid) == 0)
                {
                    strcpy(pszOut, pszMountpoint);

                    Return = SRDERR_SUCCESS;
                }
            }
        }
    }

    return Return;
}

/* --------------------------------------------------------------------
|	The GetMountpointPath() searches for a volume mountpoint presence
|   in a given (pszFullPath) path. If such a path is found it is
|   returned along with indication of success. This is important only
|   on >= W2K systems that may have some important volumes (for DR
|   purposes) mounted as directories on other volumes (e.g. OmniBack
|   installed on a volume mountpoint).
----------------------------------------------------------------------*/
static R_RESULT
GetMountpointPath(PVOLMTINFO pVolMt, R_TCHAR *pszFullPath, R_TCHAR *pszOut)
{
    R_RESULT    Return = SRDERR_ERROR;

    if(pVolMt != NULL && pszFullPath != NULL && pszOut != NULL)
    {
        R_ULONG ii = 0;
        R_TCHAR pszMountpoint[MAX_PATH] = { _T('\0') };

        for(ii = 0; Return == SRDERR_ERROR && ii < pVolMt->ulCount; ii++)
        {
            R_ULONG jj = 0;

            for(jj = 0; Return == SRDERR_ERROR && jj < pVolMt->VolMtInfos[ii]->ulCount; jj++)
            {
                strcpy(pszMountpoint, pVolMt->VolMtInfos[ii]->pszRoot);
                PathCutTrailSlashes(pszMountpoint, 0);
                strcat(pszMountpoint, _T("\\"));

                strcat(pszMountpoint, pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszName);
                PathCutTrailSlashes(pszMountpoint, 0);

                if(strnicmp(pszFullPath, pszMountpoint, strlen(pszMountpoint)) == 0)
                {
#if 0
                    strcpy(pszOut, pszMountpoint);
#else
                    /* --------------------------------------------------------------------
                    |	if volume mountpoint points to a target drive, the target drive is
                    |   returned not the volume mountpoint (this is the default OmniBack's
                    |   BDF behavior (see ntinet.c). if volume mountpoint points to an
                    |   unnamed volume, than volume mountpoint is returned.
                    ----------------------------------------------------------------------*/
                    if(
                        pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo != NULL &&
                        *pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo != _T('\0')
                    )
                    {
                        R_TCHAR *pszTmp = pVolMt->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo;
                        if (strlen(pszTmp) > 1 && pszTmp[1] != _T('\\'))
                        {
                            strcpy(pszMountpoint, pszTmp);
                            pszMountpoint[1] = _T('\0');
                        }
                        PathCutTrailSlashes(pszMountpoint, 0);
                    }
                    strcpy(pszOut, pszMountpoint);
#endif

                    Return = SRDERR_SUCCESS;
                }
            }
        }
    }

    return Return;
}
/* Check if we have UEFI loader
    This function is implemented if at any time we need logic that will detect what
    firmware env variable do we have.*/
R_BOOL
IsFirmwareVariableUefi()
{
    ERH_FUNCTION (_T("IsFirmwareVariableUefi"));

    R_RESULT retVal = FALSE;
    HMODULE handle = NULL;
    getFirmwareEnvironmentVariable uefiVersion = NULL;

    SRD_ENTER_FCN;

    handle = LoadLibrary(_T("kernel32.dll"));
    if (NULL == handle || INVALID_HANDLE_VALUE == handle)
    {
        DbgPlain (DBG_SYSNFOW2K, _T("LoadLibrary (Kernel Module Library) failed"));
        return FALSE;
    }

    uefiVersion = (getFirmwareEnvironmentVariable)GetProcAddress(handle, "GetFirmwareEnvironmentVariableA");

    if (NULL == uefiVersion)
    {
        DbgPlain(DBG_SYSNFOW2K,
            _T("Failed to get function address for 'GetFirmwareEnvironmentVariableA' (err=%d)."),
             GetLastError());

        return FALSE;
    }

    uefiVersion("", "{00000000-0000-0000-0000-000000000000}", NULL, 0);

    DbgPlain(DBG_SYSNFOW2K,
        _T("Err value (err=%d)."),
        GetLastError());

    if(GetLastError() != ERROR_INVALID_FUNCTION)
    {
        retVal = TRUE;
    }

    DbgPlain(DBG_SYSNFOW2K,
        _T("Returning value (%d)."),
        retVal);

    SRD_EXIT_FCN;

    return(retVal);

}
/* --------------------------------------------------------------------
|	The GetBootVolume() function returns the current systems boot volume
|   by searching through the disk(s) layout list. Once such a partition
|   is found it is connected to a certain drive letter. The limitation
|   of the function: if the user has more than one disk partition that
|   holds the ACTIVE signature, the function will choose the first one
|   found as the actual boot partition (even though this might not be
|   the real boot partition). If a boot partition is not found or it
|   cannot be connected to the drive letter (a possibility with an LDM
|   boot disk) the C drive letter is assumed to be the boot partitions
|   drive letter.
----------------------------------------------------------------------*/
#if !TARGET_WIN64 || defined(_AMD64_)

/* --------------------------------------------------------------------
|   Get boot volume function.
----------------------------------------------------------------------*/
static R_RESULT
GetBootVolume(PDISKINFOW2K pDiskInfo,
              R_TCHAR *pszVolumePath) /* parasoft-suppress  OPT-03 "Old Code" */
{
    ERH_FUNCTION (_T("GetBootVolume"));

    R_RESULT retVal = SRDERR_ERROR;
    R_ULONG ii = 0;
    R_ULONG jj = 0;
    R_ULONG ulLetter = 0;

    SRD_ENTER_FCN;

	DbgPlain(DBG_SYSNFOW2K, _T("Executing real GetBootVolume() function (not returning only system volume).\n"));
	for (ii = 0L; retVal == SRDERR_ERROR && ii < pDiskInfo->DiskCount; ii++)
	{
		PDISKW2K pD = &pDiskInfo->Disks[ii];

		for (jj = 0L; retVal == SRDERR_ERROR && jj < pD->Layout->PartitionCount; jj++)
		{
			PPartitionInformation pPart = pD->Layout->PartitionEntry[jj];

			if (pPart->BootIndicator)
			{
                R_ULONG kk = 0;

                DbgPlain(DBG_MAIN_GLOBAL,
                                _T("Boot indicator found for partition \
                                   (disk = %d partition offset = %I64d partition length = %I64d)"),
                                ii,
                                pPart->StartingOffset,
                                pPart->PartitionLength);
                for (kk = 0L; retVal == SRDERR_ERROR && pDiskInfo->VolumeInfo &&
                    kk < pDiskInfo->VolumeInfo->VolumeCount; kk++)
				{
                    DbgPlain(DBG_MAIN_GLOBAL,
                                    _T("  Searching volume \
                                       (offset = %I64d length = %I64d signature = %s guid = %s letter = %c)"),
                                    pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.StartingOffset,
                                    pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.Length,
                                    pDiskInfo->VolumeInfo->Volumes[kk].DiskSignature,
                                    pDiskInfo->VolumeInfo->Volumes[kk].DeviceGuid,
                                    pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.DriveLetter);
					if (
						pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.StartingOffset == pPart->StartingOffset &&
						pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.Length == pPart->PartitionLength &&
                        stricmp(pDiskInfo->VolumeInfo->Volumes[kk].DiskSignature, pD->Layout->Signature) == 0
					)
					{
                        DbgPlain(DBG_MAIN_GLOBAL, _T("  Found volume."));
                        /**
                         * No Drive Letter
                         * Check for mount point or GUID
                         */
                        if (pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.DriveLetter == _T('\0'))
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            if (pDiskInfo->VolumeInfo->Volumes[kk].DeviceGuid[0] != _T('\0'))
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                R_TCHAR pszVolumeMountPoint[MAX_PATH];
                                if (FindMountpointPathByVolumeGuid(
                                        pDiskInfo->VolMtInfo,
                                        pDiskInfo->VolumeInfo->Volumes[kk].DeviceGuid, 
                                        pszVolumeMountPoint
                                ) != SRDERR_SUCCESS)
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    if (IsWin7OrHigher())
                                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                        sprintf(pszVolumePath, _T("%s"), pDiskInfo->VolumeInfo->Volumes[kk].DeviceGuid);
                                        PathToSlashes(pszVolumePath);
                                        PathCutLeaderSlash(pszVolumePath, 1);
                                        retVal = SRDERR_SUCCESS;
                                    }
                                    else
                                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                        DbgPlain(DBG_SYSNFOW2K, _T("  No mountpoint for GUID: %s"),
                                            pDiskInfo->VolumeInfo->Volumes[kk].DeviceGuid);
                                        continue;
                                    }
                                }
                                else
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    sprintf(pszVolumePath, _T("/%s"), pszVolumeMountPoint);
                                    PathToSlashes(pszVolumePath);
                                    retVal = SRDERR_SUCCESS;
                                }
                            }
                            else
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_SYSNFOW2K, _T("  No drive letter and no volume GUID \
                                                            for volume on disk (%d) - offset (%I64d MB). Skipping."),
                                    pDiskInfo->VolumeInfo->Volumes[kk].DiskNumber,
                                    pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.StartingOffset);
                                continue;
                            }
                        }
                        /**
                         * Drive Letter exist
                         */
                        else
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            DbgPlain(DBG_MAIN_GLOBAL,_T("Adding the drive letter as [%C]"),pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.DriveLetter );
                            if (EnvGetBool(_T("OB2_USE_SYSTEM_BOOT_VOL")))
                            {
                                if(GetSystemVolume(&ulLetter, NULL) == 0)
                                {
                                    sprintf(pszVolumePath, _T("/%C"), UpcaseCharacter(ulLetter));
                                    DbgPlain(DBG_MAIN_GLOBAL,_T("Added System volume as boot volume =[%C] "),ulLetter);
                                    retVal = SRDERR_SUCCESS;
                                }
                                else
                                    retVal = SRDERR_ERROR;
                            }
                            else
                            {
                                sprintf(pszVolumePath, _T("/%C"), pDiskInfo->VolumeInfo->Volumes[kk].PartInfo.DriveLetter);
                                retVal = SRDERR_SUCCESS;
                            }
                        }
					}
				}
			}
		}
	}

    if (IsWin7OrHigher())
    {
#if 0 /** Remove this comment if disabled support for GUID*/
        DbgPlain(DBG_MAIN_GLOBAL, _T("Boot volume not found. Ask DRM module for boot volume."));

        *pLetter = (R_ULONG)_T('@');
#endif
    }

    if (retVal != SRDERR_SUCCESS)
    {
        if (GetSystemVolume(&ulLetter, NULL) == 0)
        {
            sprintf(pszVolumePath, _T("/%C"), UpcaseCharacter(ulLetter));
            retVal = SRDERR_SUCCESS;
        }

        DbgPlain(DBG_MAIN_GLOBAL,
            _T("Boot volume not found. Using default (system volume) drive letter (%s)."),
            pszVolumePath);
    }

    SRD_RETURN_VAL(retVal);
}

#else /* !defined(TARGET_WIN64) || defined(_AMD64_) */

static R_RESULT
GetBootVolume(PDISKINFOW2K pDiskInfo, R_TCHAR *pLetter)
{
    R_RESULT Return = SRDERR_ERROR;
    R_ULONG ulLetter = 0;

    DbgPlain(DBG_SYSNFOW2K, _T("Executing GetBootVolume() function (returning only system volume).\n"));

    if (GetSystemVolume(&ulLetter, NULL) == 0)
    {
        sprintf(pLetter, _T("/%C"), UpcaseCharacter(ulLetter));
        Return = SRDERR_SUCCESS;
    }

    return(Return);
}

#endif

/* --------------------------------------------------------------------
|	The GetCSVolumes() scans the current system configuration for the
|   presence of the Certificate Services and provides information about the
|   CS Database and Log directories (that doesn't have to be identical).
----------------------------------------------------------------------*/
static R_RESULT
GetCSVolumes(PVOLMTINFO pVolMt, R_TCHAR *pszCSDb, R_TCHAR *pszCSLogs)
{
	R_RESULT	Return = SRDERR_SUCCESS;
    HKEY        hKey = NULL;
    HMODULE     hCert = NULL;
    
	*pszCSDb = *pszCSLogs = _T('\0');

    __try
    {
        R_DWORD dwSize = MAX_PATH;
    	R_BOOL  bOnLine = FALSE;
        R_TCHAR   *pDot = NULL, pszHostName[MAX_PATH];
        R_TCHAR pszDB[MAX_PATH], pszLogs[MAX_PATH];
        R_TCHAR pszMountpoint[MAX_PATH];
        FNCERTSRVISSERVERONLINEW *fnOnline = NULL;
#if !defined(UNICODE) && !defined(_UNICODE)
        USES_CONVERSION;
#endif
        hCert = LoadLibrary(CERTSVC_DLLNAME);
        if(hCert == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot load Certificate Server Admin dynamic library."));}
            );

        fnOnline = (FNCERTSRVISSERVERONLINEW*)GetProcAddress(hCert, CertSrvIsServerOnlineName);
        if(fnOnline == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot get CertSrvIsServerOnlineW() function pointer."));}
            );

        sprintf(pszHostName, _T("\\\\%s"), cmnHostname);
        pDot = strchr(pszHostName, _T('.'));
        if(pDot != NULL)
            *pDot = L'\0';
        if(
            fnOnline(T2W(pszHostName), &bOnLine) != ERROR_SUCCESS ||
            bOnLine != TRUE
	    )
            SRD_W32_LEAVE(
                {
                    DbgPlain(
                        DBG_SYSNFOW2K,
                        _T("Certificate Service is not online or an error occured in the process. error: %d"),
                        GetLastError());
                }
            );
    	
        DbgPlain(DBG_SYSNFOW2K, _T("Certificate Service is online."));
    
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, CERTSVC_KEY, 
            0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("No CertSvc\\Configuration key in the registry."));}
            );
        dwSize = MAX_PATH;
        if(RegQueryValueEx(hKey, CERTSVC_DB_VAL, NULL, NULL, (R_UCHAR*)pszDB, &dwSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot query 'DBDirectory' value or value does not exist."));}
            );
        dwSize = MAX_PATH;
        if(RegQueryValueEx(hKey, CERTSVC_LOG_VAL, NULL, NULL, (R_UCHAR*)pszLogs, &dwSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot query 'DBLogDirectory' value or value does not exist."));}
            );

        UpcaseVolumeLetter(pszDB);
        UpcaseVolumeLetter(pszLogs);

        DbgPlain(DBG_SYSNFOW2K, _T("CS Database path [registry]: %s."), pszDB);
        DbgPlain(DBG_SYSNFOW2K, _T("CS Log path [registry]: %s."), pszLogs);
    
        if(GetMountpointPath(pVolMt, pszDB, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);
            sprintf(pszCSDb, _T("/%s"), pszMountpoint);
            PathToSlashes(pszCSDb);
        }
        else
    		sprintf(pszCSDb, _T("/%C"), UpcaseCharacter(*pszDB));

        if(GetMountpointPath(pVolMt, pszLogs, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);
            sprintf(pszCSLogs, _T("/%s"), pszMountpoint);
            PathToSlashes(pszCSLogs);
        }
        else
    		sprintf(pszCSLogs, _T("/%C"), UpcaseCharacter(*pszLogs));

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(hKey != NULL)
            RegCloseKey(hKey);
        if(hCert != NULL)
            FreeLibrary(hCert);
    }

	return Return;
}

/* --------------------------------------------------------------------
|	The GetADSVolumes() scans the current system configuration for the
|   presence of the Active Directory and provides information about the
|   AD Database and Log directories (that doesn't have to be identical).
----------------------------------------------------------------------*/
static R_RESULT
GetADSVolumes(PVOLMTINFO pVolMt, R_TCHAR *pszADSDb, R_TCHAR *pszADSLogs)
{
	R_RESULT	Return = SRDERR_SUCCESS;
    HKEY        hKey = NULL;
    HMODULE     hNtds = NULL;

	*pszADSDb = *pszADSLogs = _T('\0');

    __try
    {
        R_DWORD dwSize = MAX_PATH;
    	R_BOOL  bOnLine = FALSE;
 	    R_TCHAR *pDot = NULL, pszHostName[MAX_PATH];
        R_TCHAR pszDB[MAX_PATH], pszLogs[MAX_PATH];
        R_TCHAR pszMountpoint[MAX_PATH];
        DsIsNTDSOnlineProc fnOnline = NULL;

        hNtds = LoadLibrary(NTDS_DLLNAME);
        if(hNtds == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot load ntdsbcli library."));}
            );

        fnOnline = (DsIsNTDSOnlineProc)GetProcAddress(hNtds, DsIsNTDSOnlineName);
        if(fnOnline == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot get DsIsNTDSOnline() function pointer."));}
            );

        sprintf(pszHostName, _T("\\\\%s"), cmnHostname);
	    pDot = strchr(pszHostName, _T('.'));
	    if(pDot != NULL)
		    *pDot = _T('\0');
	    if(
		    fnOnline(pszHostName, &bOnLine) != ERROR_SUCCESS ||
		    bOnLine != TRUE
	    )
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("ADS is not online or an error occured in the process."));}
            );
    	
        DbgPlain(DBG_SYSNFOW2K, _T("ADS is online."));
    
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, NTDS_KEY, 
            0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("No NTDS\\Parameters key in the registry."));}
            );
        dwSize = MAX_PATH;
        if(RegQueryValueEx(hKey, NTDS_DB_VAL, NULL, NULL, (R_UCHAR*)pszDB, &dwSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot query 'DSA Database file' value or value does not exist."));}
            );
        dwSize = MAX_PATH;
        if(RegQueryValueEx(hKey, NTDS_LOG_VAL, NULL, NULL, (R_UCHAR*)pszLogs, &dwSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Cannot query 'Database log files path' value or value does not exist."));}
            );

        UpcaseVolumeLetter(pszDB);
        UpcaseVolumeLetter(pszLogs);

        DbgPlain(DBG_SYSNFOW2K, _T("Database path [registry]: %s."), pszDB);
        DbgPlain(DBG_SYSNFOW2K, _T("Log path [registry]: %s."), pszLogs);
    
        if(GetMountpointPath(pVolMt, pszDB, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);
            sprintf(pszADSDb, _T("/%s"), pszMountpoint);
            PathToSlashes(pszADSDb);
        }
        else
    		sprintf(pszADSDb, _T("/%C"), UpcaseCharacter(*pszDB));

        if(GetMountpointPath(pVolMt, pszLogs, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);
            sprintf(pszADSLogs, _T("/%s"), pszMountpoint);
            PathToSlashes(pszADSLogs);
        }
        else
    		sprintf(pszADSLogs, _T("/%C"), UpcaseCharacter(*pszLogs));

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(hKey != NULL)
            RegCloseKey(hKey);
        if(hNtds != NULL)
            FreeLibrary(hNtds);
    }

	return Return;
}

typedef DWORD (WINAPI *NtfrsInit)(DWORD, DWORD, PVOID*);
typedef DWORD (WINAPI *NtfrsDestroy)(PVOID*, DWORD, HKEY*, DWORD, PWCHAR);
typedef DWORD (WINAPI *NtfrsGetSets)(PVOID);
typedef DWORD (WINAPI *NtfrsEnum)(PVOID, DWORD, PVOID*);
typedef DWORD (WINAPI *NtfrsIsSysVol)(PVOID, PVOID, BOOL*);
typedef DWORD (WINAPI *NtfrsGetDir)(PVOID, PVOID, DWORD*, PWCHAR);

R_RESULT
EnumSysvolPath(R_TCHAR *sysVolPath)
{
    ERH_FUNCTION (_T("EnumSysvolPath"));

    R_RESULT    Return = SRDERR_ERROR;
    HMODULE     hMod = NULL;
    PVOID       context = NULL;
    NtfrsDestroy    fnDestroy = NULL;

    SRD_ENTER_FCN;

    __try
    {
        R_DWORD dwStatus = 0;
        R_DWORD dwIdx = 0;
        R_VOID  *burSet = NULL;
        R_BOOL  isSysVol = FALSE;
        R_TCHAR dirPathT[MAX_PATH] = { 0 };
        NtfrsInit       fnInit = NULL;
        NtfrsGetSets    fnSets = NULL;
        NtfrsEnum       fnEnum = NULL;
        NtfrsIsSysVol   fnIsSysVol = NULL;
        NtfrsGetDir     fnGetDir = NULL;
        USES_CONVERSION;
        UNREF_CONV;

        hMod = LoadLibrary(_T("ntfrsapi.dll"));
        if(NULL == hMod)
        {
        	DbgPlain(DBG_SYSNFOW2K, _T("Error loading ntfrsapi library."));
            __leave;
        }

        fnInit = (NtfrsInit)GetProcAddress(hMod, "NtFrsApiInitializeBackupRestore");
        fnDestroy = (NtfrsDestroy)GetProcAddress(hMod, "NtFrsApiDestroyBackupRestore");
        fnSets = (NtfrsGetSets)GetProcAddress(hMod, "NtFrsApiGetBackupRestoreSets");
        fnEnum = (NtfrsEnum)GetProcAddress(hMod, "NtFrsApiEnumBackupRestoreSets");
        fnIsSysVol = (NtfrsIsSysVol)GetProcAddress(hMod, "NtFrsApiIsBackupRestoreSetASysvol");
        fnGetDir = (NtfrsGetDir)GetProcAddress(hMod, "NtFrsApiGetBackupRestoreSetDirectory");

        if(
            NULL == fnInit ||
            NULL == fnDestroy ||
            NULL == fnSets ||
            NULL == fnEnum ||
            NULL == fnIsSysVol ||
            NULL == fnGetDir
        )
        {
        	DbgPlain(DBG_SYSNFOW2K, _T("Error binding to ntfrsapi library exports."));
            __leave;
        }

        dwStatus = fnInit(0, NTFRSAPI_BUR_FLAGS_SUPPORTED_BACKUP, &context);
        if (dwStatus != ERROR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("Error initializing ntfrs backup/restore context [%d]."), dwStatus);
            __leave;
        }

        if(fnSets(context) != ERROR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("Error getting ntfrs backup/restore sets."));
            __leave;
        }

        dwStatus = fnEnum(context, dwIdx, &burSet);
        dwIdx++;
        while (dwStatus == ERROR_SUCCESS)
        {
            if(fnIsSysVol(context, burSet, &isSysVol) != ERROR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFOW2K, _T("Error querying for sysvol."));
                __leave;
            }

            if(TRUE == isSysVol)
            {
                WCHAR   dirPathW[MAX_PATH] = { 0 };
                DWORD   dwSize = MAX_PATH * sizeof(WCHAR);

            	DbgPlain(DBG_SYSNFOW2K, _T("Replication set %d is SYSVOL."), dwIdx);
                if(fnGetDir(context, burSet, &dwSize, dirPathW) != ERROR_SUCCESS)
                {
                    DbgPlain(DBG_SYSNFOW2K, _T("Error getting directory path for SYSVOL."));
                    __leave;
                }

                strcpy(dirPathT, W2T(dirPathW));
                break;
            }
            dwStatus = fnEnum(context, dwIdx, &burSet);
            dwIdx++;
        }

        if(
            dwStatus != ERROR_SUCCESS &&
            dwStatus != ERROR_NO_MORE_ITEMS
        )
        {
            DbgPlain(DBG_SYSNFOW2K, _T("Error enumerating replication sets."));
            __leave;
        }

        if(dirPathT[0] == '\0')
        {
            DbgPlain(DBG_SYSNFOW2K, _T("No SYSVOL path available."));
            __leave;
        }

        strcpy(sysVolPath, dirPathT);
        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(NULL != context && NULL != fnDestroy)
            fnDestroy(&context, NTFRSAPI_BUR_FLAGS_NONE, NULL, 0, NULL);
        if(NULL != hMod)
            FreeLibrary(hMod);
    }

    SRD_RETURN_VAL(Return);
}
/* --------------------------------------------------------------------
|	The GetProfilesVolume() returns the information about the location
|   of the \Documents and Settings volume. This can actually reside
|   on a volume that is different than the system volume (if the
|   installation was performed using an unattended script file).
----------------------------------------------------------------------*/
static R_RESULT
GetProfilesVolume(R_ULONG *pProf, R_TCHAR *pszProfilesPath)
{
	R_RESULT	Return = SRDERR_ERROR;

	if(
		pszProfilesPath != NULL &&
		strlen(pszProfilesPath) > 2 &&
		pszProfilesPath[1] == _T(':')
	)
	{
		*pProf = (R_ULONG)*pszProfilesPath;

		Return = SRDERR_SUCCESS;
	}

	return Return;
}

/* --------------------------------------------------------------------
|	The GetSYSVOLVolume() returns the information about the location
|   of the ADS SYSVOL volume.
----------------------------------------------------------------------*/
static R_RESULT
GetSYSVOLVolume(PVOLMTINFO pVolMt, R_TCHAR *pszSYSVOLPath, R_TCHAR *pszOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_ULONG     ulSysVol = 0;
    R_TCHAR     pszPath[MAX_PATH] = { 0 };

    pszOut[0] = _T('\0');

    if(pszSYSVOLPath[0] != _T('\0'))
    {
        R_TCHAR pszMountpoint[MAX_PATH];

        strcpy(pszPath, pszSYSVOLPath);
        ulSysVol = pszPath[0];

        UpcaseVolumeLetter(pszPath);
        if(GetMountpointPath(pVolMt, pszPath, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);

            sprintf(pszOut, _T("/%s"), pszMountpoint);
            PathToSlashes(pszOut);
        }
        else
    		sprintf(pszOut, _T("/%C"), UpcaseCharacter(ulSysVol));

        Return = SRDERR_SUCCESS;
    }
    else
        Return = SRDERR_SUCCESS;

    return Return;
}

/* --------------------------------------------------------------------
|   The GetOmniBackVolumeW2K() is a W2K specific function for obtaining
|   the location of the OmniBack directory. It takes into account the
|   fact that OmniBack can be installed on a volume mountpoint.
----------------------------------------------------------------------*/
static R_RESULT
GetOmniBackVolumeW2K(PVOLMTINFO pVolMt, R_TCHAR *pszOmni, R_TCHAR *pszOmniData)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_ULONG     ulOmni = 0, ulData = 0;
    R_TCHAR     pszPath[MAX_PATH] = { _T('\0') };
    R_TCHAR     pszData[MAX_PATH] = { _T('\0') };

    *pszOmni = _T('\0');
    *pszOmniData = _T('\0');

    if(GetOmniBackVolume(&ulOmni, pszPath, &ulData, pszData) == SRDERR_SUCCESS)
    {
        R_TCHAR pszMountpoint[MAX_PATH];

        UpcaseVolumeLetter(pszPath);
        if(GetMountpointPath(pVolMt, pszPath, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);

            sprintf(pszOmni, _T("/%s"), pszMountpoint);
            PathToSlashes(pszOmni);
        }
        else
            sprintf(pszOmni, _T("/%C"), UpcaseCharacter(ulOmni));

        UpcaseVolumeLetter(pszData);
        if(GetMountpointPath(pVolMt, pszData, pszMountpoint) == SRDERR_SUCCESS)
        {
            PathCutTrailSlashes(pszMountpoint, 0);

            sprintf(pszOmniData, _T("/%s"), pszMountpoint);
            PathToSlashes(pszOmniData);
        }
        else
            sprintf(pszOmniData, _T("/%C"), UpcaseCharacter(ulData));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

/* --------------------------------------------------------------------
|	The GetQuorumVolume() is a W2K specific function for obtaining
|   the location of the quorum volume. It takes into account the
|   fact that quorum volume can be installed as a volume mountpoint.
----------------------------------------------------------------------*/
static R_RESULT
GetQuorumVolume(PVOLMTINFO pVolMt, PCLUSINFO pClusInfo, R_TCHAR *pszOut)
{
    ERH_FUNCTION (_T("GetQuorumVolume"));

    R_RESULT    Return = SRDERR_SUCCESS;
    R_TCHAR     pszPath[MAX_PATH] = { _T('\0') };

    DbgFcnIn();

    *pszOut = _T('\0');

    if (pClusInfo != NULL &&
        GetClusterQuorumVolume(pszPath) == SRDERR_SUCCESS
    )
    {
        R_TCHAR pszMountpoint[MAX_PATH];

        /*
         * if Volume GUID
         */
        if (strstr(pszPath, _T("Volume{")) != NULL)
        {
            PathToSlashes(pszPath);
            PathCutLeaderSlash(pszPath, 1);
            PathCutTrailSlashes(pszPath, 0);
            strcpy(pszOut, pszPath);
        }
        else
        {
            UpcaseVolumeLetter(pszPath);
            if (GetMountpointPath(pVolMt, pszPath, pszMountpoint) == SRDERR_SUCCESS)
            {
                PathCutTrailSlashes(pszMountpoint, 0);

                sprintf(pszOut, _T("/%s"), pszMountpoint);
                PathToSlashes(pszOut);
            }
            else
                sprintf(pszOut, _T("/%C"), UpcaseCharacter(*pszPath));
        }
    }

    DbgFcnOut();

    return(Return);
}

/* --------------------------------------------------------------------
 | On Windows Server 2008 R2 share volumes on server side and node side does not
 | contain same Volume GUID. Probably Windows bug. In this section we skip volume from
 | node side
 -------------------------------------------------------------------- */
static R_VOID
SkipVolumeGUIDs(R_TCHAR *supUnsupVolume,
                PDISKINFOW2K pDiskInfo,
                R_ULONG volumeIndex,
                R_BOOL volumeType)
{

    ERH_FUNCTION (_T("SkipVolumeGUIDs"));

    R_TCHAR *volume = NULL;
    R_TCHAR uv[STRLEN_48K+1] = { _T('\0') };
    HANDLE  hVolume = INVALID_HANDLE_VALUE;

    DbgFcnIn();

    strcpy(uv, supUnsupVolume);

    if (pDiskInfo->VolumeInfo->Volumes[volumeIndex].PartInfo.DriveLetter == _T('\0'))
    {
        if (pDiskInfo->VolumeInfo->Volumes[volumeIndex].DeviceGuid[0] != _T('\0'))
        {
            R_TCHAR pszVolumeMountPoint[MAX_PATH] = { _T('\0') };

            if (FindMountpointPathByVolumeGuid(
                    pDiskInfo->VolMtInfo,
                    pDiskInfo->VolumeInfo->Volumes[volumeIndex].DeviceGuid, 
                    pszVolumeMountPoint
            ) != SRDERR_SUCCESS)
            {
                volume = strtok(uv, _T(","));
                while (volume)
                {
                    DbgPlain(DBG_MAIN_GLOBAL, _T(" volume, %s"), volume);

                    if (strstr(volume, _T("Volume{")) != NULL)
                    {
                        if (SrdVolumeOpenEx(&hVolume, volume) == SRDERR_SUCCESS)
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            DWORD bytes = 0;
                            R_UCHAR buffer[STRLEN_16K];
                            PVOLUME_DISK_EXTENTS de = (PVOLUME_DISK_EXTENTS)buffer;

                            memset(buffer, 0, STRLEN_16K);
                            if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, de,  /* parasoft-suppress  CODSTA-12 "Windows enum value" */
                                16384, &bytes, NULL))
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_MAIN_GLOBAL, 
                                    _T("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed for volume '%s'"), volume);
                            }

                            if (!de->NumberOfDiskExtents)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_MAIN_GLOBAL, _T("Invalid number of disk extents for volume '%s'"), volume);
                            }

                            if (de->NumberOfDiskExtents > 1)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_MAIN_GLOBAL,
                                    _T("Warning: LDM volume '%s' detected. Only the first extent will be used."),
                                    volume);
                            }

                            if ((de->Extents[0].DiskNumber == pDiskInfo->VolumeInfo->Volumes[volumeIndex].DiskNumber) &&
                                (de->Extents[0].StartingOffset.QuadPart ==
                                pDiskInfo->VolumeInfo->Volumes[volumeIndex].PartInfo.StartingOffset) &&
                                stricmp(pDiskInfo->VolumeInfo->Volumes[volumeIndex].DeviceGuid, volume) != 0
                                )
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                /*
                                 * Change Volume GUID on node side with Volume GUID from cluster side
                                 */
                                strcpy(pDiskInfo->VolumeInfo->Volumes[volumeIndex].DeviceGuid, volume);
                            }

                            if (hVolume != INVALID_HANDLE_VALUE)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                SrdVolumeClose(hVolume);
                            }
                        }
                    }
                    /*
                    * Move to next mount point
                    */
                    volume = strtok(NULL, _T(","));

                    if (volumeType == TRUE)
                    {
                        volume = strtok(NULL, _T(","));
                    }
                }
            }
        }
    }
    DbgFcnOut();
}

/* --------------------------------------------------------------------
 | This function will go trough all volumes on the system and check
 | do they belong to disk that is dynamic. If so it will return it back
 | as a data volume. Exceptions are volumes that are on shared disks
 | and system and boot volume.
 -------------------------------------------------------------------- */
static R_VOID
GetDynamicVolumes(PDISKINFOW2K pDiskInfo, PCLUSINFO clus, const R_TCHAR *bootVolume, const R_TCHAR *systemVolume, R_TCHAR *pszOut)
{
    ERH_FUNCTION (_T("GetDynamicVolumes"));

    TCHAR volume_name[MAX_PATH] = { _T('\0') };
    TCHAR volume_guid[MAX_PATH] = { _T('\0') };
    R_TCHAR pLetter[MAX_PATH] = { _T('\0') };
    R_ULONG iDiskIterator = 0;
    HANDLE find = INVALID_HANDLE_VALUE;

    find = FindFirstVolume(volume_name, MAX_PATH);
    if (INVALID_HANDLE_VALUE != find)
    {
        do
        {
            R_BOOL isDynamicDisk = FALSE;
            HANDLE vol = NULL;
            R_TCHAR device_name[MAX_PATH] = { _T('\0') };
            R_TCHAR volume[MAX_PATH] = { _T('\0') };
            R_BOOL volumeBelongsToCluster = FALSE;

            strcpy(volume_guid, volume_name);
            if (volume_name[strlen(volume_name)-1] == _T('\\'))
                volume_guid[strlen(volume_guid)-1] = _T('\0');
            strcpy(volume_name, volume_guid + 4);

            DbgPlain(DBG_MAIN_GLOBAL, _T("Handling volume (%s)."), volume_guid);

            if (SrdVolumeOpenEx(&vol, volume_guid) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_MAIN_GLOBAL, _T("The opening of volume (%s) failed (%d)."), volume_guid, GetLastError());
                _LEAVE_;
            }

            _TRY_
            DWORD bytes = 0;
            R_UCHAR buffer[STRLEN_8K] = { _T('\0') };
            PVOLUME_DISK_EXTENTS de = (PVOLUME_DISK_EXTENTS)buffer;

            memset(buffer, 0, STRLEN_8K);
            if (!DeviceIoControl(vol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, de,
                STRLEN_8K, &bytes, NULL))
            {
                DbgPlain(DBG_MAIN_GLOBAL, _T("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed (%d) for volume (%s)."),
                    GetLastError(), volume_guid);
                _LEAVE_;
            }

            for (iDiskIterator = 0; iDiskIterator < pDiskInfo->DiskCount; iDiskIterator++)
            {
                if (pDiskInfo->Disks[iDiskIterator].Number == de->Extents->DiskNumber)
                {
                    // Volume must belong to dynamic disk
                    if ( pDiskInfo->Disks[iDiskIterator].Dynamic == 1)
                    {
                        isDynamicDisk = TRUE;
                        break;
                    }
                }
            }

            if (isDynamicDisk == FALSE)
            {
                continue;
            }

            if (!QueryDosDevice(volume_name, device_name, MAX_PATH))
            {
                DbgPlain(DBG_MAIN_GLOBAL, 
                    _T("QueryDosDevice failure (%d) for volume (%s)."), GetLastError(), volume_guid);
                _LEAVE_;
            }

            // Get the drive letter if there is one if not use GUID (it is a hidden volume)
            // After that convert it to SRD friendly format
            if (GetVolumeDriveLetter(pDiskInfo, device_name, volume) == SRDERR_SUCCESS)
            {
                sprintf(volume, _T("/%C"), UpcaseCharacter(*volume));
            }
            else
            {
                PathToSlashes(volume_guid);
                PathCutLeaderSlash(volume_guid, 1);
                PathCutTrailSlashes(volume_guid, 0);
                strcpy(volume, volume_guid);
            }

            // According to MSDN system and boot volumes will be properly returned by IOCTL_DISK_GET_PARTITION_INFO_EX 
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa365180(v=vs.85).aspx
            // The IOCTL_DISK_GET_PARTITION_INFO_EX control code is supported on basic disks. 
            // It is only supported on dynamic disks that are boot or system disks ...

            // So we dont want to add DATA purpose for system volumes or boot volumes so we are skipping them
            if (strcmp(volume, systemVolume) == 0 || strcmp(volume, bootVolume) == 0)
            {
                DbgPlain(DBG_MAIN_GLOBAL, _T("This is a boot or system volume skipping. Volume: (%s)"), volume);
                continue;
            }

            // If we have a cluster we should not add volumes that belong on any cluster disk (supported or unsupported)
            if (clus != NULL)
            {
                R_TCHAR *pszClusterTmp = NULL;

                if(clus->SupportedVols != NULL && clus->SupportedVols[0] != 0)
                {
                    pszClusterTmp = strtok(clus->SupportedVols, _T(","));
                    while (pszClusterTmp != NULL)
                    {
                        R_TCHAR pszClusterComverted[MAX_PATH] = { _T('\0') };
                        if (strstr(pszClusterTmp, _T("Volume{")) != NULL)
                        {
                            PathToSlashes(pszClusterTmp);
                            PathCutLeaderSlash(pszClusterTmp, 1);
                            PathCutTrailSlashes(pszClusterTmp, 0);
                            strcpy(pszClusterComverted, pszClusterTmp);
                        }
                        else
                        {
                            sprintf(pszClusterComverted, _T("/%C"), UpcaseCharacter(*pszClusterTmp));
                        }

                        if (strcmp (pszClusterComverted, volume) == 0)
                        {
                            volumeBelongsToCluster = TRUE;
                            break;
                        }
                        pszClusterTmp = strtok (NULL, _T(","));
                    }

                    if(volumeBelongsToCluster == FALSE && 
                        clus->UnsupportedVols != NULL &&
                        clus->UnsupportedVols[0] != 0)
                    {
                        pszClusterTmp = strtok(clus->UnsupportedVols, _T(","));
                        while (pszClusterTmp != NULL)
                        {
                            R_TCHAR pszClusterComverted[MAX_PATH] = { _T('\0') };
                            if (strstr(pszClusterTmp, _T("Volume{")) != NULL)
                            {
                                PathToSlashes(pszClusterTmp);
                                PathCutLeaderSlash(pszClusterTmp, 1);
                                PathCutTrailSlashes(pszClusterTmp, 0);
                                strcpy(pszClusterComverted, pszClusterTmp);
                            }
                            else
                            {
                                sprintf(pszClusterComverted, _T("/%C"), UpcaseCharacter(*pszClusterTmp));
                            }

                            if (strcmp (pszClusterComverted, volume) == 0)
                            {
                                volumeBelongsToCluster = TRUE;
                                break;
                            }
                            pszClusterTmp = strtok (NULL, _T(","));
                        }
                    }
                }
            }

            if (volumeBelongsToCluster == TRUE)
            {
                DbgPlain(DBG_MAIN_GLOBAL, _T("Volume (%s) is part of cluster disks. Skip..."), volume);
                continue;
            }

            if (pDiskInfo->VolMtInfo != NULL)
            {
                // If Volume points to some other volume we need to skip it
                R_BOOL volumePointsTo = FALSE;
                R_ULONG ii = 0;
                R_TCHAR pszMountpoint[MAX_PATH] = { _T('\0') };

                for(ii = 0;  ii < pDiskInfo->VolMtInfo->ulCount; ii++)
                {
                    R_ULONG jj = 0;

                    for(jj = 0; jj < pDiskInfo->VolMtInfo->VolMtInfos[ii]->ulCount; jj++)
                    {
                        if (NULL != pDiskInfo->VolMtInfo->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo)
                        {
                            strcpy(pszMountpoint, pDiskInfo->VolMtInfo->VolMtInfos[ii]->MountPoints[jj]->pszPointsTo);
                            PathToSlashes(pszMountpoint);
                            PathCutLeaderSlash(pszMountpoint, 1);
                            PathCutTrailSlashes(pszMountpoint, 0);

                            if (strstr(pszMountpoint, _T("Volume{")) != NULL && stricmp(pszMountpoint, volume) == 0)
                            {
                                DbgPlain(DBG_MAIN_GLOBAL, _T("Volume (%s) points to some other mountpoint. Skip..."), volume);
                                volumePointsTo = TRUE;
                                break;
                            }
                        }
                    }
                }

                if (volumePointsTo == TRUE)
                {
                    continue;
                }
            }

            DbgPlain(DBG_MAIN_GLOBAL, _T("Will add (%s) as a data volume."), volume);
            sprintf(pLetter, _T(";%s"), volume);
            strcat(pszOut, pLetter);

            _FINALLY_
                CloseHandle(vol);
            _ENDTRY_
        }
        while (FindNextVolume(find, volume_name, MAX_PATH));

        FindVolumeClose(find);
    }
}

/* --------------------------------------------------------------------
|   The GetDataVolumes() is specific function for obtaining
|   the location of the data volumes. ONLY IN VOLUMES.
----------------------------------------------------------------------*/
static R_VOID
GetDataVolumes(PDISKINFOW2K pDiskInfo, PRESTOREVOLUMES pVols, PCLUSINFO clus, R_TCHAR *pszOut)
{
    ERH_FUNCTION (_T("GetDataVolumes"));

    R_ULONG ii = 0;
    R_TCHAR pLetter[MAX_PATH] = { _T('\0') };

    DbgFcnIn();

    pszOut[0] = _T('\0');

    for (ii = 0L; pDiskInfo->VolumeInfo && ii < pDiskInfo->VolumeInfo->VolumeCount; ii++)
    {
        R_ULONG	jj = 0;
        R_ULONG mb = 1024 * 1024;
        R_TCHAR tmpVar[STRLEN_STD] = { _T('\0') };
        R_TCHAR volName[STRLEN_STD] = { _T('\0') };
        R_TCHAR fsName[STRLEN_STD] = { _T('\0') };
        R_TCHAR pszVolumePath[STRLEN_STD] = { _T('\0') };
        R_TCHAR pszVolumeRoot[STRLEN_STD] = { _T('\0') };

        if (clus)
        {
            if (clus->SupportedVols != NULL && clus->SupportedVols[0] != _T('\0'))
            {
                SkipVolumeGUIDs(clus->SupportedVols, pDiskInfo, ii, TRUE);
            }
            if (clus->UnsupportedVols != NULL && clus->UnsupportedVols[0] != _T('\0'))
            {
                SkipVolumeGUIDs(clus->UnsupportedVols, pDiskInfo, ii, FALSE);
            }
        }

        if (clus && clus->UnsupportedVols && clus->UnsupportedVols[0] != _T('\0'))
        {
            R_TCHAR uv[STRLEN_48K] = { _T('\0') };
            R_TCHAR *volUnsupp = NULL;

            strcpy(uv, clus->UnsupportedVols);
            if (pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter == _T('\0'))
            {
                if (pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid[0] != _T('\0'))
                {
                    R_TCHAR pszVolumeMountPoint[MAX_PATH] = { _T('\0') };
                    if (FindMountpointPathByVolumeGuid(
                            pDiskInfo->VolMtInfo,
                            pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid, 
                            pszVolumeMountPoint
                    ) != SRDERR_SUCCESS)
                    {
                        sprintf(tmpVar, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                        PathToBackslashes(tmpVar);
                    }
                    else
                    {
                        sprintf(tmpVar, _T("%s"), pszVolumeMountPoint);
                        PathToBackslashes(tmpVar);
                    }
                }
            }
            else
            {
                sprintf(tmpVar, _T("%C:"), pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter);
            }
            volUnsupp = strtok(uv, _T(","));
            while (volUnsupp)
            {
                if (!stricmp(volUnsupp, tmpVar))
                {
                    DbgPlain(DBG_MAIN_GLOBAL, _T("  Unsupported cluster volume '%s' found on the system."), tmpVar);
                    break;
                }
                volUnsupp = strtok(NULL, _T(","));
            }

            if (volUnsupp != NULL)
            {
                continue;
            }
        }

        if (pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter == _T('\0'))
        {
            if (pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid[0] != _T('\0'))
            {
                R_TCHAR pszVolumeMountPoint[MAX_PATH] = { _T('\0') };
                if (FindMountpointPathByVolumeGuid(
                        pDiskInfo->VolMtInfo,
                        pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid, 
                        pszVolumeMountPoint
                ) != SRDERR_SUCCESS)
                {
                    if (IsWin7OrHigher())
                    {
                        sprintf(pszVolumePath, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                        PathToSlashes(pszVolumePath);
                        PathCutLeaderSlash(pszVolumePath, 1);
                        sprintf(pszVolumeRoot, _T("%s\\"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                    }
                    else
                    {
                        DbgPlain(DBG_SYSNFOW2K, _T("  No mountpoint for GUID: %s"),
                            pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                        continue;
                    }
                }
                else
                {
                    sprintf(pszVolumePath, _T("/%s"), pszVolumeMountPoint);
                    PathToSlashes(pszVolumePath);
                    sprintf(pszVolumeRoot, _T("%s\\"), pszVolumeMountPoint);
                }
            }
            else
            {
                DbgPlain(DBG_SYSNFOW2K, _T("  No drive letter and no volume GUID for volume on disk (%d) - offset (%I64d MB). Skipping."),
                    pDiskInfo->VolumeInfo->Volumes[ii].DiskNumber,
                    pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.StartingOffset);
                continue;
            }
        }
        else
        {
            sprintf(pszVolumePath, _T("/%C"), pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter);
            sprintf(pszVolumeRoot, _T("%C:\\"), pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter);
        }

        for (jj = 0L; jj < pVols->RestVolCount; jj++)
        {
            if (strcmp(pszVolumePath, pVols->RestoreVolumes[jj]->VolumeName) == 0)
            {
                break;
            }
        }

        if (jj < pVols->RestVolCount)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("  Volume '%s' already among the restore volumes. Skipping."), pszVolumePath);
            continue;
        }

        if (GetDriveType(pszVolumeRoot) != DRIVE_FIXED)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("  Non-fixed volume '%s'. Skipping."), tmpVar);
            continue;
        }

        fsName[0] = _T('\0');
        if (!GetVolumeInformation(pszVolumeRoot, volName, STRLEN_STD, NULL, NULL, NULL, fsName, STRLEN_STD))
        {
            DbgPlain(DBG_SYSNFOW2K, _T("  Failed to obtain information for volume '%s' (%d). Skipping."),
                tmpVar, GetLastError());
            continue;
        }
        if (fsName[0] == _T('\0'))
        {
            DbgPlain(DBG_SYSNFOW2K, _T("  Invalid/Non-existent filesystem on volume '%s'. Skipping."),
                pszVolumeRoot);
            continue;
        }
        /*
        * On Windows 8, CSV volumes are available to Volume enumeration API (with an 'CSVFS' FS identifier)
        * For now we do not support backup of CSV volumes directly, so we skip them.
        * NOTE: Once support is implemented (inside DA) remove this entry.
        */
        if (strcmp(fsName, _T("CSVFS")) == 0)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("  CSVFS filesystem on volume '%s'. Skipping."),
                pszVolumeRoot);
            continue;
        }

        DbgPlain(DBG_SYSNFOW2K, _T("  Adding data volume '%s'."), pszVolumeRoot);

        sprintf(pLetter, _T(";%s"), pszVolumePath);
        strcat(pszOut, pLetter);
    }

    DbgFcnOut();
}

/** \fn GetStorageProperty(R_TCHAR cbDrive, PVIRTUALDISKINFO *ppVirtualDiskInfo)
    \brief Get Property for selected storage
    \param cbDrive Volume GUID
    \param ppVirtualDiskInfo Return Structure with propery of storage
    \return SRDERR_SUCCES or SRDERR_ERROR
 */
static R_RESULT
GetStorageProperty(R_TCHAR *cbDrive, PVIRTUALDISKINFO *ppVirtualDiskInfo)
{
    ERH_FUNCTION (_T("GetStorageProperty"));
    R_RESULT retVal = SRDERR_ERROR;

#ifdef IOCTL_STORAGE_QUERY_PROPERTY
    R_TCHAR                 pDrive[MAX_PATH] = { _T('\0') };
    HANDLE                  hDrive = INVALID_HANDLE_VALUE;
    R_DWORD                 cbBytesReturned = 0;
    STORAGE_PROPERTY_QUERY  sQuery;
    STORAGE_DEVICE_DESCRIPTOR   *pDescriptor = NULL;
    STORAGE_PROPERTY_QUERY  query;
    R_TCHAR                 buffer[MAX_PATH];
    R_INT                   ret = 0;

    DbgFcnIn();

    _TRY_
        PathToBackslashes(cbDrive);
        sprintf(pDrive, _T ("\\%s"), cbDrive);

        DbgPlain(DBG_SYSNFOW2K, _T("Create file on drive %s"), pDrive);
        hDrive = CreateFile(pDrive,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                            NULL
                            );
        if (hDrive == INVALID_HANDLE_VALUE)
        {
            if (strstr(pDrive, _T("Volume{")) == NULL)
            {
                DbgPlain(DBG_SYSNFOW2K, _T("Cannot create file on drive %s"), pDrive);
                _LEAVE_;
            }
        }

        sQuery.PropertyId = StorageDeviceProperty;
        sQuery.QueryType = PropertyStandardQuery;
        sQuery.AdditionalParameters[0] = 0;

        memset((void *) & query, 0, sizeof(query));
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        memset(&buffer, 0, sizeof(buffer)); /* parasoft-suppress  CODSTA-12 "This is a Microsoft enum value" */

        ret = DeviceIoControl(hDrive, IOCTL_STORAGE_QUERY_PROPERTY, /* parasoft-suppress  CODSTA-12 "Windows enum value" */
                                &query,
                                sizeof(query),
                                &buffer,
                                sizeof(buffer),
                                &cbBytesReturned, NULL);
        CloseHandle(hDrive);
        if (ret == FALSE)
        {
            DbgPlain(DBG_SYSNFOW2K, _T("IO error for IOCTL_STORAGE_QUERY_PROPERTY"));
            _LEAVE_;
        }

        pDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        (*ppVirtualDiskInfo)->enumBusType = pDescriptor->BusType;

        retVal = SRDERR_SUCCESS;
        _FINALLY_
        /**
         * Destroy
         */

    _ENDTRY_

    DbgFcnOut();
#endif /* IOCTL_STORAGE_QUERY_PROPERTY*/

    return(retVal);
}
/* parasoft-suppress  CODSTA-03 "This is copied from SDK can not be changed" */
#ifndef BusTypeVirtual
#define BusTypeVirtual  15  /* parasoft-suppress  CODSTA-37 "Define the storage bus types for VHD - WinIoCtl.h (SDK\Win2008R2)*/ /* parasoft-suppress  NAMING-01 "Define the storage bus types for VHD - WinIoCtl.h (SDK\Win2008R2), This is copied from SDK can not be changed" */ /* parasoft-suppress  METRICS-26_duplicated_1 "Parasoft suppression comment" */
#endif

/* --------------------------------------------------------------------
|    The GetVolumesFromVHD() is specific function for obtaining
|   the location of the volumes on VHD (Virtual Hard Drive).
----------------------------------------------------------------------*/
static R_VOID
GetVolumesFromVHD(PDISKINFOW2K pDiskInfo, /* parasoft-suppress  METRICS-28 "The complexity is up for only one than it is recomended so there is no need to refactore this now." */ /* parasoft-suppress  METRICS-26_duplicated_1 "Parasoft suppression comment" */
                  PRESTOREVOLUMES pVols,
                  R_TCHAR *pszOut)
{

    ERH_FUNCTION (_T("GetVolumesFromVHD"));

    R_ULONG ii = 0;
    R_ULONG jj = 0;
    R_ULONG kk = 0;
    R_TCHAR tmpVar[STRLEN_4K] = { _T('\0') };
    PVIRTUALDISKINFO    virtualDiskInfo = NULL;

    DbgFcnIn();

    pszOut[0] = _T('\0');
    _TRY_
    virtualDiskInfo = MALLOC(sizeof(VIRTUALDISKINFO));
    if (virtualDiskInfo == NULL)
    {
        DbgPlain(DBG_SYSNFO, _T("Memory allocation error - VirtualDiskInfo."));
        _LEAVE_;
    }
    memset(virtualDiskInfo , 0, sizeof(VIRTUALDISKINFO));

    /**
     * Found VHD folumes in list of restore volumes
     */
    for (jj = 0L; jj < pVols->RestVolCount; jj++)
    {
        R_TCHAR pszVolumePath[STRLEN_STD] = { _T('\0') };
        R_TCHAR pszVolumeGUID[STRLEN_STD] = { _T('\0') };

        /**
         * For get storage property we have volume GUID
         * Volume GUID for all volumes are stored in DiskInfo structure
         */
        for (ii = 0L; pDiskInfo->VolumeInfo && ii < pDiskInfo->VolumeInfo->VolumeCount; ii++)
        {
            /**
             * If volume does not have Drive Letter
             * Try get mountpoint
             */
            if (pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter == _T('\0'))
            {
                /** 
                 * Volume must have volume GUID
                 */
                if (pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid[0] != _T('\0'))
                {
                    R_TCHAR pszVolumeMountPoint[MAX_PATH] = { _T('\0') };
                    /**
                     * Try get mountpoint for volume
                     */
                    if (FindMountpointPathByVolumeGuid(
                            pDiskInfo->VolMtInfo,
                            pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid, 
                            pszVolumeMountPoint
                    ) != SRDERR_SUCCESS)
                    {
                        /**
                         * If volume does not have drive letter or mountpoint
                         * we use volume GUID for compare and check volume for VHD type
                         */
                        sprintf(pszVolumePath, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                        PathToSlashes(pszVolumePath);
                        PathCutLeaderSlash(pszVolumePath, 1);
                        if (StrCmp(pszVolumePath, pVols->RestoreVolumes[jj]->VolumeName) == 0)
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            sprintf(pszVolumeGUID, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                            PathToSlashes(pszVolumeGUID);
                            PathCutLeaderSlash(pszVolumeGUID, 1);
                            if (GetStorageProperty(pszVolumeGUID, &virtualDiskInfo) != SRDERR_SUCCESS)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_SYSNFOW2K, _T("FAILED - GetStorageProperty()"));
                                _LEAVE_;
                            }

                            /**
                             * If volume have VHD type, add to VHD volume list
                             */
                            if (virtualDiskInfo->enumBusType == BusTypeVirtual)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_SYSNFOW2K, _T("  Adding virtual volumes '%s'."), 
                                                        pVols->RestoreVolumes[jj]->VolumeName);

                                if (kk > 0)
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    sprintf(tmpVar, _T("%s;%s"), tmpVar, pVols->RestoreVolumes[jj]->VolumeName);
                                }
                                else
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    sprintf(tmpVar, _T("%s"), pVols->RestoreVolumes[jj]->VolumeName);
                                }
                                kk++;
                            }
                        }
                    }
                    /**
                     * If volume have a mountpoint
                     */
                    else
                    {
                        sprintf(pszVolumePath, _T("/%s"), pszVolumeMountPoint);
                        PathToSlashes(pszVolumePath);
                        if (StrCmp(pszVolumePath, pVols->RestoreVolumes[jj]->VolumeName) == 0)
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            sprintf(pszVolumeGUID, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                            PathToSlashes(pszVolumeGUID);
                            PathCutLeaderSlash(pszVolumeGUID, 1);
                            if (GetStorageProperty(pszVolumeGUID, &virtualDiskInfo) != SRDERR_SUCCESS)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_SYSNFOW2K, _T("FAILED - GetStorageProperty()"));
                                _LEAVE_;
                            }

                            /**
                             * If volume have VHD type, add to VHD volume list
                             */
                            if (virtualDiskInfo->enumBusType == BusTypeVirtual)
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                DbgPlain(DBG_SYSNFOW2K, _T("  Adding virtual volumes '%s'."), 
                                                        pVols->RestoreVolumes[jj]->VolumeName);

                                if (kk > 0)
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    sprintf(tmpVar, _T("%s;%s"), tmpVar, pVols->RestoreVolumes[jj]->VolumeName);
                                }
                                else
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    sprintf(tmpVar, _T("%s"), pVols->RestoreVolumes[jj]->VolumeName);
                                }
                                kk++;
                            }
                        }
                    }
                }
            }
            /**
             * If volume have Drive letter
             */
            else
            {
                sprintf(pszVolumePath, _T("/%C"), pDiskInfo->VolumeInfo->Volumes[ii].PartInfo.DriveLetter);
                if (StrCmp(pszVolumePath, pVols->RestoreVolumes[jj]->VolumeName) == 0)
                {
                    sprintf(pszVolumeGUID, _T("%s"), pDiskInfo->VolumeInfo->Volumes[ii].DeviceGuid);
                    PathToSlashes(pszVolumeGUID);
                    PathCutLeaderSlash(pszVolumeGUID, 1);
                    if (GetStorageProperty(pszVolumeGUID, &virtualDiskInfo) != SRDERR_SUCCESS)
                    {
                        DbgPlain(DBG_SYSNFOW2K, _T("FAILED - GetStorageProperty()"));
                        _LEAVE_;
                    }

                    /**
                     * If volume have VHD type, add to VHD volume list
                     */
                    if (virtualDiskInfo->enumBusType == BusTypeVirtual)
                    {
                        DbgPlain(DBG_SYSNFOW2K, _T("  Adding virtual volumes '%s'."), 
                                                pVols->RestoreVolumes[jj]->VolumeName);

                        if (kk > 0)
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            sprintf(tmpVar, _T("%s;%s"), tmpVar, pVols->RestoreVolumes[jj]->VolumeName);
                        }
                        else
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            sprintf(tmpVar, _T("%s"), pVols->RestoreVolumes[jj]->VolumeName);
                        }
                        kk++;
                    }
                }
            }
        }
    }
    _FINALLY_
        /**
         * Destroy allocated memory
         */
        if (virtualDiskInfo != NULL)
        {
            FREE(virtualDiskInfo);
        }
    _ENDTRY_

    /**
     * Return list VHD volumes
     */
    strcat(pszOut, tmpVar);

    DbgFcnOut();
}

/* --------------------------------------------------------------------
|	The AddOmniBackDatabaseVolumes() adds all detected OmniBack DB
|   volumes to the array of critical restore volumes.
----------------------------------------------------------------------*/
static R_RESULT
AddOmniBackDatabaseVolumes(PRESTOREVOLUMES *ppVols, PVOLMTINFO pVolMt, PCLUSINFO pClusInfo, R_TCHAR **ppDbLocs, R_ULONG ulCount)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_ULONG     ii = 0;

    __try
    {
        if(ppDbLocs == NULL || ulCount == 0)
            __leave;

        for(ii = 0; ii < ulCount; ii++)
        {
            R_TCHAR pszOmniDb[MAX_PATH];
            R_TCHAR pszMountpoint[MAX_PATH];

            UpcaseVolumeLetter(ppDbLocs[ii]);
            if(GetMountpointPath(pVolMt, ppDbLocs[ii], pszMountpoint) == SRDERR_SUCCESS)
            {
                PathCutTrailSlashes(pszMountpoint, 0);

                sprintf(pszOmniDb, _T("/%s"), pszMountpoint);
                PathToSlashes(pszOmniDb);
            }
            else
    		    sprintf(pszOmniDb, _T("/%C"), UpcaseCharacter(ppDbLocs[ii][0]));

            if(AddRestVol(
                ppVols, 
                pszOmniDb, 
                IsClusterCellManager(pClusInfo) == SRDERR_SUCCESS ? OB2DB_CONFIG : OB2DB_VOLUME, 
                OT_WINFS) != SRDERR_SUCCESS
            )
				__leave;
        }

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(Return != SRDERR_SUCCESS)
        	DbgPlain(DBG_SYSNFOW2K, _T("AddOmniBackDatabaseVolumes() failed - cannot add omniback database volumes."));
    }

    return Return;
}

/* --------------------------------------------------------------------
|	The highest level function that enumerates all the volumes that are
|   vital for a DR restore. This include the boot, system, OmniBack,
|   OmniBack database, profiles, ADS, etc. volumes.
----------------------------------------------------------------------*/
R_RESULT
EnumRestoreVolumes(PDISKINFOW2K pDiskInfo, /* parasoft-suppress  METRICS-15 "This is an odd function but it is modified for this project and for that reason this can not be fixed" */ /* parasoft-suppress  METRICS-26_duplicated_1 "Parasoft suppression comment" */ /* parasoft-suppress  METRICS-28 "This function complexity has been increased by adding new debug messages moustly and the rest is old code. The function did not have almoust any debug messages. To split this is not easy and since this is very important function for recovery proces spliting it in this stage can cause many complications that should be avoided. Refactorying can be made leater when more time is avalible for testing." */
                   PRESTOREVOLUMES *ppOut,
                   PCLUSINFO pClusInfo,
                   R_TCHAR *pszProfilesPath,
                   R_TCHAR *pszSYSVOLPath,
                   R_BOOL bCellManager)
{
    R_RESULT    retVal = SRDERR_ERROR;
    R_ULONG     ulSystem = 0;
    R_ULONG     ulProfiles = 0;
    R_TCHAR     pszBoot[STRLEN_4K] = { _T('\0') };
    R_TCHAR     pszOmni[STRLEN_4K] = { _T('\0') };
    R_TCHAR     pszOmniData[STRLEN_4K] = { _T('\0') };
    R_TCHAR     pszDeviceGuid[STRLEN_4K] = { _T('\0') };
    R_TCHAR     pzsSystemVolume[STRLEN_4K] = { _T('\0') };
#if 1 /* disable this to switch off Full Recovery */
    /*QCCR2A35643 -Buffer size increased to handle large no of vols*/
    R_TCHAR     pszData[STRLEN_48K] = { _T('\0') }, *pszDataTmp;
    R_TCHAR     pszDynamic[STRLEN_16K] = {_T('\0')};
    R_TCHAR     *pszDynamicTmp = NULL;
#endif
    R_TCHAR     pszVirtual[STRLEN_4K] = { _T('\0') }, *pszVirtualTmp;
    R_TCHAR     pszADSDb[MAX_PATH] = { _T('\0') }, pszADSLogs[MAX_PATH] = { _T('\0') };
    R_TCHAR     pszCSDb[MAX_PATH] = { _T('\0') }, pszCSLogs[MAX_PATH] = { _T('\0') };
    R_TCHAR     pszQuorumPath[MAX_PATH] = { 0 }, sysVolPath[MAX_PATH] = { 0 };
    PRESTOREVOLUMES pVols = NULL;

    __try 
    {
        R_TCHAR	pszVolName[MAX_RESTORE_VOLUME_NAME] = { _T('\0') };

        if (GetBootVolume(pDiskInfo, pszBoot) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetBootVolume."));
                }
            );
        }

        if (GetSystemVolume(&ulSystem, NULL) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetSystemVolume."));
                }
            );
        }

        if (GetOmniBackVolumeW2K(pDiskInfo->VolMtInfo, pszOmni, pszOmniData) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetOmniBackVolumeW2K."));
                }
            );
        }

        if (GetADSVolumes(pDiskInfo->VolMtInfo, pszADSDb, pszADSLogs) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetADSVolumes."));
                }
            );
        }

        if (GetCSVolumes(pDiskInfo->VolMtInfo, pszCSDb, pszCSLogs) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetCSVolumes."));
                }
            );
        }

        if (GetProfilesVolume(&ulProfiles, pszProfilesPath) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetProfilesVolume."));
                }
            );
        }

        if (GetSYSVOLVolume(pDiskInfo->VolMtInfo, pszSYSVOLPath, sysVolPath) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetSYSVOLVolume."));
                }
            );
        }

        if (GetQuorumVolume(pDiskInfo->VolMtInfo, pClusInfo, pszQuorumPath) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - GetQuorumVolume."));
                }
            );
        }

        if (IsWin7OrHigher())
        {
            /**
             * If no Drive Letter
             */
            if (pszBoot[0] == _T('\0'))
            {
                if (AddRestVol(&pVols, pszDeviceGuid, BOOT_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                {
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(BOOT_VOLUME by GUID)."));
                        }
                    );
                }
            }
#if 0
            /**
             * Detect boot volume by DR module
             */
            else if (pszBoot[0] == _T('@'))
            {                
                sprintf(pszVolName, _T("%C"), UpcaseCharacter(ulBoot));
                if (AddRestVol(&pVols, pszVolName, BOOT_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                {
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(BOOT_VOLUME by @)."));
                        }
                    );
                }
            }
#endif
            /**
             * Drive Letter exists
             */
            else
            {
                if (AddRestVol(&pVols, pszBoot, BOOT_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                {
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(BOOT_VOLUME by Drive Letter)."));
                        }
                    );
                }
            }
        }
        else
        {
            if (AddRestVol(&pVols, pszBoot, BOOT_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(BOOT_VOLUME)."));
                    }
                );
            }
        }

        sprintf(pszVolName, _T("/%C"), UpcaseCharacter(ulSystem));
        sprintf(pzsSystemVolume, _T("/%C"), UpcaseCharacter(ulSystem));
        if (AddRestVol(&pVols, pszVolName, SYSTEM_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(SYSTEM_VOLUME)."));
                }
            );
        }
        sprintf(pszVolName, _T("/%C"), UpcaseCharacter(ulProfiles));
        if (AddRestVol(&pVols, pszVolName, PROFILES_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(PROFILES_VOLUME)."));
                }
            );
        }
        if (*sysVolPath != _T('\0'))
        {
            if (AddRestVol(&pVols, sysVolPath, ADS_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(ADS_VOLUME)."));
                    }
                );
            }
        }
        if (*pszADSDb != _T('\0') && *pszADSLogs != _T('\0'))
        {
            if (AddRestVol(&pVols, pszADSDb, ADS_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(ADS_VOLUME)."));
                    }
                );
            }
            if (AddRestVol(&pVols, pszADSLogs, ADS_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(ADS_VOLUME)."));
                    }
                );
            }
        }
        if (*pszCSDb != _T('\0') && *pszCSLogs != _T('\0'))
        {
            if (AddRestVol(&pVols, pszCSDb, CS_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(CS_VOLUME)."));
                    }
                );
            }
            if (AddRestVol(&pVols, pszCSLogs, CS_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(CS_VOLUME)."));
                    }
                );
            }
        }
        if((pClusInfo) &&
            (pClusInfo->clusterType != CLUS_TYPE_MNS) &&
            (*pszQuorumPath != _T('\0')) &&
            pClusInfo->clusterType !=  CLUS_TYPE_MNFS)
        {
            R_TCHAR uv[STRLEN_48K];
            R_TCHAR *volSupp;

            if (AddRestVol(&pVols, pszQuorumPath, QUORUM_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(QUORUM_VOLUME)."));
                    }
                );
            }
            if (pClusInfo->SupportedVols != NULL)
            {
                strcpy(uv, pClusInfo->SupportedVols);

                volSupp = strtok(uv, _T(","));
                while(volSupp)
                {
                    /*
                     * If Volume GUID
                     */
                    if (strstr(volSupp, _T("Volume{")) != NULL)
                    {
                        strcpy(pszVolName, volSupp);
                        PathToSlashes(pszVolName);
                        PathCutLeaderSlash(pszVolName, 1);
                    }
                    else
                    {
                        if (strlen(volSupp) > 3)
                        {
                            // Mount Point
                            sprintf(pszVolName, _T("/%s"), volSupp);
                        }
                        else
                        {
                            // Drive Letter
                            sprintf(pszVolName, _T("/%C"), UpcaseCharacter(volSupp[0]));
                        }
                    }

                    if (strcmp(pszVolName, pszQuorumPath))
                    {
                        if (AddRestVol(&pVols, pszVolName, DATA_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            SRD_W32_LEAVE(
                                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(DATA_VOLUME)."));
                                }
                            );
                        }
                    }
                    volSupp = strtok(NULL, _T(","));
                    volSupp = strtok(NULL, _T(","));
                }
            } /*if SupportedVols ! = NULL*/
        }

        if (AddRestVol(&pVols, pszOmni, OB2_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(OB2_VOLUME)."));
                }
            );
        }
        if (AddRestVol(&pVols, pszOmniData, OB2_DATA_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(OB2_DATA_VOLUME)."));
                }
            );
        }

        if (bCellManager)
        {
            /* Cell Manager specific */
            sprintf(pszVolName, POSTGRES_MPOINT_CONFFILES);
            if (AddRestVol(&pVols, pszVolName, OB2DB_FILES, OT_OB2BAR) != SRDERR_SUCCESS)
            {
                SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(OB2DB_FILES)."));
                }
                );
            }
        }

        sprintf(pszVolName, _T("/CONFIGURATION"));
        if (AddRestVol(&pVols, pszVolName, CFG_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
        {
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(CFG_VOLUME)."));
                }
            );
        }
#if 1 /* disable this to switch off Full Recovery */
        /* --> Add DATA Volumes */
        GetDataVolumes(pDiskInfo, pVols, pClusInfo, pszData);

        if(*pszData != _T('\0'))
        {
            pszDataTmp = strtok(pszData, _T(";"));
            while (pszDataTmp != NULL)
            {
                if (AddRestVol(&pVols, pszDataTmp, DATA_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                {
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(DATA_VOLUME)."));
                        }
                    );
                }
                pszDataTmp = strtok (NULL, _T(";"));
            }
        }

        if (IsVistaOrHigher())
        {
            GetDynamicVolumes(pDiskInfo, pClusInfo, pszBoot, pzsSystemVolume, pszDynamic);

            if(*pszDynamic != _T('\0'))
            {
                pszDynamicTmp = strtok(pszDynamic, _T(";"));
                while (pszDynamicTmp != NULL)
                {
                    if (AddRestVol(&pVols, pszDynamicTmp, DATA_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                    {
                        SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(DATA_VOLUME)."));
                        }
                        );
                    }
                    pszDynamicTmp = strtok (NULL, _T(";"));
                }
            }
        }

        /* <-- Add DATA Volumes */
#endif
        /* --> Add Virtual Volumes */
        GetVolumesFromVHD(pDiskInfo, pVols, pszVirtual);

        if (*pszVirtual != _T('\0'))
        {
            pszVirtualTmp = strtok(pszVirtual, _T(";"));
            while (pszVirtualTmp != NULL)
            {
                if (AddRestVol(&pVols, pszVirtualTmp, VHD_VOLUME, OT_WINFS) != SRDERR_SUCCESS)
                {
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOWXP, _T("FAILURE - AddRestVol(VHD_VOLUME)."));
                        }
                    );
                }
                pszVirtualTmp = strtok (NULL, _T(";"));
            }
        }
        /* <-- Add Virtual Volumes */

        *ppOut = pVols;

        retVal = SRDERR_SUCCESS;
    }
    __finally
    {
        if (retVal != SRDERR_SUCCESS)
        {
            FreeRestoreVolumes(pVols);
            *ppOut = NULL;
        }
    }

    return(retVal);
}

/* --------------------------------------------------------------------
|   Gets the path to the \Documents and Settings directory by using
|   the Shell API.
----------------------------------------------------------------------*/
R_RESULT
EnumProfilesPath(tchar *pszProfilesPath)
{
	R_RESULT		Return = SRDERR_ERROR;
	R_TCHAR			*pszRegPath = NULL, pszPath[MAX_PATH];
	
	*pszProfilesPath = _T('\0');

	__try
	{
		R_TCHAR *pszStart = NULL, *pszEnd = NULL;

        pszRegPath = RegGetString(
            HKEY_LOCAL_MACHINE, 
            _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), 
            _T("ProfilesDirectory"));
        if (!pszRegPath)
            __leave;
        strcpy(pszPath, pszRegPath);
		pszStart = strchr(pszPath, _T('\\'));
		if(pszStart == NULL)
			__leave;
		pszEnd = strchr(pszStart + 1, _T('\\'));
		if(pszEnd == NULL)
			__leave;
		*pszEnd = _T('\0');

        UpcaseVolumeLetter(pszPath);
		strcpy(pszProfilesPath, pszPath);

		Return = SRDERR_SUCCESS;
	}
	__finally
	{
        if (pszRegPath)
            FREE(pszRegPath);

		if(Return != ERROR_SUCCESS)
		{
            R_ULONG platform;
            if(GetPlatformInfo(&platform, NULL, NULL) != SRDERR_SUCCESS)
                platform = OS_W2K;

			GetWindowsDirectory(pszPath, MAX_PATH);
            UpcaseVolumeLetter(pszPath);

            if (IsWinVistaOrHigher(platform))
            {
                strcpy(pszPath + 3, _T("Users"));
            }
            else
            {
			    strcpy(pszPath + 3, _T("Documents and Settings"));
            }
			strcpy(pszProfilesPath, pszPath);

			Return = SRDERR_SUCCESS;
		}
	}

	return Return;
}

/* --------------------------------------------------------------------
|	Network specific section.
----------------------------------------------------------------------*/
R_RESULT
EnumNetworkInformation(PNETWORKINFO *lpInfo)
{
    ERH_FUNCTION (_T("EnumNetworkInformation"));

	R_RESULT	rResult = SRDERR_ERROR;
	PNETWORKINFO	pNetInfo = NULL;

    SRD_ENTER_FCN;

	*lpInfo = NULL;

	__try
	{
        R_TCHAR pszName[MAX_PATH] = { 0 };
        R_TCHAR pszDomain[MAX_PATH] = { 0 };
        R_DWORD dwSize = MAX_PATH;

        if(!InitNetworkModule(NULL, NULL))
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("Could not initialize networking module."));}
            );
		pNetInfo = MALLOC(sizeof(NETWORKINFO));
		if(NULL == pNetInfo)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFOW2K, _T("FAILURE - memory allocation."));}
            );
		memset(pNetInfo, 0, sizeof(NETWORKINFO));

        if(GetComputerName(pszName, &dwSize) != 0)
            pNetInfo->computerName = StrNewCopy(pszName);

        if(GetMachineDomain(pszDomain) == SRDERR_SUCCESS)
            pNetInfo->domainName = StrNewCopy(pszDomain);

        if(!GetNetworkInfo(pNetInfo))
            SRD_W32_LEAVE(
               { DbgPlain(DBG_SYSNFOW2K, _T("GetNetworkInfo() failed."));}
            );

		*lpInfo = pNetInfo;

		rResult = SRDERR_SUCCESS;
	}
	__finally
	{
		if(rResult != SRDERR_SUCCESS)
		{
			FreeNetworkInfo(pNetInfo);
			FREE(pNetInfo);
		}
		FreeNetworkModule();
	}

	SRD_RETURN_VAL(rResult);
}

