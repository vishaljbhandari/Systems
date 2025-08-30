/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   drivelib
* @file      lib/drivelib/drivelib.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     FileDescription
*
* @since     dd.mm.yy	Emir	Original Coding
*
* @remarks   /
*/
#include "lib\cmn\target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/drivelib/drivelib.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/
#include "lib/cmn/common.h"
#include "lib/cmn/containers.h"
#include <winioctl.h>
#include <winbase.h>
#include <dbt.h>
#include <winuser.h>
#include "lib\drivelib\drivelib.h"
#include "recovery\rid\ridoscmn.h"
#if (!TARGET_WIN64)

/* VS 2008 MIGRATION
#include <largeint.h>
*/
#endif

#define	DBG_DL	(32)
#define	STAMP	DbgStamp (DBG_DL)

#if 0
#define DBG_ENTER_FCN { DbgStamp (DBG_DL); DbgPlain (DBG_DL, _T("-> Entering function %s"), __FCN__); }
#define RETURN_VALUE(_value) { DbgStamp (DBG_DL); DbgPlain (DBG_DL, _T("<- Function %s RETURN_VALUE %d"), __FCN__, _value); return (_value); }
#define RETURN_VOID { DbgStamp (DBG_DL); DbgPlain (DBG_DL, _T("<- Function %s RETURN_VALUE (void)"), __FCN__); return; }
#define RETURN_STRING(_value) { DbgStamp (DBG_DL); DbgPlain (DBG_DL, _T("<- Function %s RETURN_VALUE string \"%s\""), __FCN__, _value); return (_value); }
#endif

BOOL	_bDynamic = FALSE;
#define DEFAULT_CHUNK_SIZE 512 /* parasoft-suppress  CODSTA-37 "Adoption to the old code" */
/* ===========================================================================
|
|	FUNCTION	DlOpenPhysicalDrive
|
 ========================================================================== */	

HANDLE
DlOpenPhysicalDrive (DWORD driveNr)
{
	ERH_FUNCTION (_T("DlOpenPhysicalDrive"));

	tchar			drive[MAX_PATH+1];
	HANDLE			handle = INVALID_HANDLE_VALUE;

	STAMP;
	_stprintf (drive, _T ("\\\\.\\PHYSICALDRIVE%d"), driveNr);
	DbgPlain (DBG_DL, _T("DlOpenPhysicalDrive  %s entered"), drive);
	
	handle = CreateFile (
		drive,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL
		);

	DbgPlain (DBG_DL, 
		_T("DlOpenPhysicaldrive %s -> %s\n"), 
		drive, handle == INVALID_HANDLE_VALUE ? _T("FAILED") : _T("OK")
		);
	if (handle == INVALID_HANDLE_VALUE)
	{
		ErhMarkSys (NTDLERR_CANNOT_OPEN_PHYDRIVE, GetLastError (), __FCN__, ERH_NORMAL);
	}

	return handle;
}

/* ===========================================================================
|
|    FUNCTION    DlOpenStorageDeviceNumber
|
 ========================================================================== */

HANDLE
DlOpenStorageDeviceNumber(const tchar *volumeGUID)
{
    ERH_FUNCTION (_T("DlOpenStorageDeviceNumber"));

    tchar   *path, *clean;
    HANDLE  handle = INVALID_HANDLE_VALUE;

    DbgFcnIn();

    /* -----------------------------------------------------------------------
    |   Instead of volumeGUID format: \\?\Volume{....}\
    |   handle needs to be open without trailing backslash
     -----------------------------------------------------------------------*/
    clean = StrNewCopy(volumeGUID);
    PathCutTrailSlashes(clean, 0);
    path = StrFormat(_T("\\\\?\\%s"), PathGetLeaf(clean));
    FREE(clean);

    handle = CreateFile (
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );

    DbgPlain (DBG_DL, _T("[%s] %s -> %s\n"), 
                __FCN__, path, handle == INVALID_HANDLE_VALUE ? _T("FAILED") : _T("OK")
             );

    if (handle == INVALID_HANDLE_VALUE)
    {
        ErhMarkSys (NTDLERR_CANNOT_OPEN_STORAGEDEV, GetLastError (), __FCN__, ERH_NORMAL);
    }

    FREE(path);

    RETURN_PTR(handle);
}



/* ===========================================================================
|
|   FUNCTION   DlGetPhysicalDriveGeometry
|
 ========================================================================== */

int
DlGetPhysicalDriveGeometry (HANDLE handle, PDisk_Geometry pDG)
{
	ERH_FUNCTION (_T("DlGetPhysicalDriveGeomtry"));

	int				ret = -1;
	DISK_GEOMETRY	ntDG;
	DWORD			bytesRet = 0;

	DBG_ENTER_FCN;

	if (DeviceIoControl (
			handle,
			IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, 
			&ntDG, sizeof (ntDG),
			&bytesRet,
			NULL)
		)
	{
		memset (pDG, 0, sizeof (Disk_Geometry));	/*	because of BemTiJeTerMa	*/

		memcpy (&pDG->Cylinders, &ntDG.Cylinders, sizeof (LARGE_INTEGER));
		pDG->MediaType = ntDG.MediaType;
		pDG->BytesPerSector = ntDG.BytesPerSector;
		pDG->SectorsPerTrack = ntDG.SectorsPerTrack;
		pDG->TracksPerCylinder = ntDG.TracksPerCylinder;
		
		ret = 0;
	}
	else
	{
		ErhMarkSys (NTDLERR_CANNOT_GET_GEOMETRY, GetLastError (), __FCN__, ERH_NORMAL);
	}

	DbgPlain (DBG_DL, _T("DlGetPhysicalDriveGeometry %s\n[SYSTEM %d]\n"), 
		ret == -1 ? _T("FAILED") : _T("OK"), GetLastError ()
		);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION	DlGetPhysicalDriveLayout
|
 ========================================================================== */	

int
DlGetPhysicalDriveLayout (HANDLE driveHandle, PDrive_Layout *layout)
{
	ERH_FUNCTION (_T("DlGetPhysicalDriveLayout"));

	int				ret = -1;
	BOOL			status;
	DWORD			bytesNeeded = 0;
	const	DWORD	chunk = 512;
	DWORD			bufferSz = chunk;
	DWORD			lastError = 0;
	BYTE			*buffer = (BYTE *) MALLOC (bufferSz);

	DBG_ENTER_FCN;

	*layout = NULL;

	/* -----------------------------------------------------------------------
	|	Initial size of DriveLayout is 512 bytes, because it is big enough for
	|	physical drive with 4 partition entries in partition table.
	|	If it is not big enough for this particular drive, resize it
	 -----------------------------------------------------------------------*/
	do {

		status = DeviceIoControl (
			driveHandle,
			IOCTL_DISK_GET_DRIVE_LAYOUT,
			NULL, 
			0, 
			buffer, 
			bufferSz, 
			&bytesNeeded, 
			NULL
			);

		if (status)
			ret = 0;

		if (!status && (lastError = GetLastError ()) == ERROR_INSUFFICIENT_BUFFER)
		{
			buffer = (BYTE *) REALLOC (buffer, (bufferSz += chunk));
		}

	} while (!status && lastError == ERROR_INSUFFICIENT_BUFFER);

	if (ret == 0)
	{
		*layout = (PDrive_Layout) buffer;
	}
	else
	{
		ErhMarkSys (NTDLERR_CANNOT_GET_LAYOUT, GetLastError (), __FCN__, ERH_NORMAL);
		if (*layout)
			FREE (*layout);
		*layout = NULL;
	}

	DbgPlain (DBG_DL, 
		_T("DlGetPhysicalDriveLayout %s\n[SYSTEM %d]\n"), 
		ret == -1 ? _T("FAILED") : _T("OK"), GetLastError ()
		);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|    FUNCTION    DlGetStorageDeviceNumber
|
 ========================================================================== */
int
DlGetStorageDeviceNumber(HANDLE deviceHandle, PStorageDevice_Number *sDevNum)
{
    ERH_FUNCTION (_T("DlGetStorageDeviceNumber"));

    int                ret = -1;
    BOOL               status = FALSE;
    DWORD              bytesNeeded = 0;
    const    DWORD     chunk = DEFAULT_CHUNK_SIZE;
    DWORD              bufferSz = chunk;
    DWORD              lastError = 0;
    BYTE               *buffer = (BYTE *) MALLOC (bufferSz);

    DBG_ENTER_FCN;

    /* -----------------------------------------------------------------------
    |    Initial size of StorageDeviceNumber is 512 bytes. If it's not big
    |   enough, resize it.
     -----------------------------------------------------------------------*/
    do
    {
        status = DeviceIoControl (
            deviceHandle,
            (unsigned long)IOCTL_STORAGE_GET_DEVICE_NUMBER, /* parasoft-suppress  CODSTA-12 "Standard API's" */
            NULL, 
            (unsigned long)0, 
            (void *)buffer, 
            bufferSz, 
            &bytesNeeded, 
            NULL
            );

        if (status)
        {
            ret = 0;
        }

        lastError = GetLastError ();
        if (!status && (lastError == ERROR_INSUFFICIENT_BUFFER))
        {
            buffer = (BYTE *) REALLOC (buffer, (bufferSz += chunk));
        }

    } 
    while (!status && lastError == ERROR_INSUFFICIENT_BUFFER);

    if (ret == 0)
    {
        *sDevNum = (PStorageDevice_Number) buffer;
    }
    else
    {
        ErhMarkSys (NTDLERR_CANNOT_GET_STORAGEDEV, GetLastError (), __FCN__, ERH_NORMAL);
        if (*sDevNum)
            FREE (*sDevNum);
        *sDevNum = NULL;
    }

    DbgPlain (DBG_DL, _T("[%s] %s\n[SYSTEM %d]\n"), 
            __FCN__, ret == -1 ? _T("FAILED") : _T("OK"), GetLastError ()
        );

    RETURN_VALUE (ret);
}

/* ===========================================================================
|
|   FUNCTION    DlGetLayouts
|
 ========================================================================== */

static int
DlGetLayouts (DWORD *NrDrives, PDrive_Layout **pLayout)
{
	ERH_FUNCTION (_T("DlGetLayouts"));

	int				ret = 0, errNr = 0;
	DWORD			ii = 0;
	HANDLE			drvHandle = INVALID_HANDLE_VALUE;
	PDrive_Layout	getLayout = NULL;

	DBG_ENTER_FCN;

	*NrDrives = 0;
	*pLayout = NULL;

	do {

		if ((drvHandle = DlOpenPhysicalDrive (ii)) != INVALID_HANDLE_VALUE)
		{
			ii++;

			if (DlGetPhysicalDriveLayout (drvHandle, &getLayout) == -1)
			{
				errNr = ErhErrno ();
			}
			else
			{
				*pLayout = (PDrive_Layout *) REALLOC (
					*pLayout, (*NrDrives + 1) * sizeof (PDrive_Layout)
					);
				(*pLayout)[(*NrDrives)++] = getLayout;
				getLayout = NULL;
			}

			CloseHandle (drvHandle);
		}

	} while (drvHandle != INVALID_HANDLE_VALUE && !errNr);

	if (drvHandle == INVALID_HANDLE_VALUE && !errNr)
	{
		STAMP;
		DbgPlain (DBG_DL, _T("Clear error on cannot open physical drive"));
		ErhClearError ();
	}

	if (errNr)
	{
		ret = -1;

		for (ii = 0; ii < *NrDrives; ii++)
			FREE ((*pLayout)[ii]);
		FREE (*pLayout);
		*pLayout = NULL;
		*NrDrives = 0;
	}

	DbgPlain (DBG_DL, 
		_T("DlGetLayouts -> %s\n[SYSTEM %d]\n"), 
		ret != -1 ? _T("OK") : _T("FAILED"), errNr
		);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION	DlGetDiskKey
|
 ========================================================================== */	

int
DlGetDiskKey (PDkHeader *pDk)
{
	ERH_FUNCTION (_T("DlGetDiskKey"));

	int		ret = -1, errNr = 0;
	DWORD	ii = 0;
	HKEY	dadKey = HKEY_LOCAL_MACHINE;
	HKEY	hKey = INVALID_HANDLE_VALUE;
	tchar	*infoValue = _T("Information");
	tchar	*keys[3] = 
	{
		_T("SYSTEM"), _T("DISK"), NULL
	};


	DBG_ENTER_FCN;

	*pDk = NULL;

	/* -----------------------------------------------------------------------
	|	when this loop is finished, handle to opened HKLM\SYSTEM\DISK is 
	|	stored in dadKey
	 -----------------------------------------------------------------------*/
	do {

		if (RegOpenKey (dadKey, keys[ii], &hKey) != ERROR_SUCCESS)
		{
			errNr = GetLastError ();
		}

		if (dadKey != HKEY_LOCAL_MACHINE)
			RegCloseKey (dadKey);
		dadKey = hKey;
		hKey = INVALID_HANDLE_VALUE;
		ii++;

	} while (!errNr && keys[ii] != NULL && dadKey != INVALID_HANDLE_VALUE);

	/* -----------------------------------------------------------------------
	|	search through values to find Information value
	 -----------------------------------------------------------------------*/
	if (!errNr)
	{
		DWORD	ncClassSize = 64;
		TCHAR	ncClass[64];
		DWORD	nSubKeys, maxSubKeyNameLen, maxSubKeyClassLen;
		DWORD	nValues, maxValueNameLen, maxValueDataLen;

		if (RegQueryInfoKey (dadKey, ncClass, &ncClassSize, NULL,
				&nSubKeys, 
				&maxSubKeyNameLen, 
				&maxSubKeyClassLen, 
				&nValues, 
				&maxValueNameLen, 
				&maxValueDataLen,
				NULL, 
				NULL
				) == ERROR_SUCCESS
			)
		{
			ret = 0;
			if (nValues > 0)
			{
				DWORD	nameLen, dataLen, type;
				TCHAR	*valueName;
				BYTE	*valueData;

				valueName = (TCHAR *) MALLOC (
					(maxValueNameLen + 1) * sizeof (TCHAR)
					);
				valueData = (BYTE *) MALLOC (
					maxValueDataLen * sizeof (BYTE)
					);
		
				ii = 0;
				while (ii < nValues)
				{
					nameLen = maxValueNameLen + 1;
					dataLen = maxValueDataLen;
					if (RegEnumValue (
							dadKey, ii,
							valueName, &nameLen,
							NULL, &type,
							valueData, &dataLen
							) == ERROR_SUCCESS
						)
					{
						if (strcmp (valueName, infoValue) == 0)
						{
							*pDk = (PDkHeader) MALLOC (dataLen);
							memcpy (*pDk, valueData, dataLen);
							break;
						}
					}
					ii++;
				}

				FREE (valueName);
				FREE (valueData);
			}
		}
		else
		{
			ErhMarkSys (NTDLERR_CANNOT_READ_DISK_KEY, GetLastError (), __FCN__, ERH_NORMAL);
		}
	}

	if (dadKey != HKEY_LOCAL_MACHINE && dadKey != INVALID_HANDLE_VALUE)
		RegCloseKey (dadKey);

	DbgPlain (DBG_DL, 
		_T("DlGetDiskKey -> %s\n[SYSTEM %d]\n"), 
		ret != -1 ? _T("OK") : _T("FAILED"), errNr
		);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION	DlGetDrives
|
 ========================================================================== */	

DWORD
DlGetDrives (UCHAR type)
{
	ERH_FUNCTION (_T("DlGetDrives"));

	UCHAR	subst = 0;
	DWORD	ii, DWordBitSz = sizeof (DWORD) * 8;
	DWORD	drives = GetLogicalDrives ();
	tchar	drive[8], dosNameSpace[8];
	tchar	fullPath[MAX_PATH];

	STAMP;
	DbgPlain (DBG_DL, 
		_T("DlGetDrives enetered for %s drives"), 
		type == FIXED_DRIVES ? _T("FIXED") : _T("REMOVABLE")
		);

	for (ii = 0; ii < DWordBitSz; ii++)
	{
		if (!(drives & (1 << ii)))
			continue;

		sprintf (drive, _T("%c:\\"), _T('A') + ii);
		sprintf (dosNameSpace, _T("%c:"), _T('A') + ii);
		subst = 0;
		/* -------------------------------------------------------------------
		|	QueryDosDevice is used to determine if this drive is really 
		|	partition on physical drive or is it just an SUBST created link
		 -------------------------------------------------------------------*/
		if (QueryDosDevice (dosNameSpace, fullPath, MAX_PATH))
		{
			if (strncmp (fullPath, _T("\\??"), 3) == 0)
				subst = 1;
		}

		switch (GetDriveType (drive))
		{
		case DRIVE_FIXED:
			if ((type == NON_FIXED_DRIVES && !subst) ||
				(type == FIXED_DRIVES && subst)
			   )
			{
				drives ^= (1 << ii);
			}
			break;

		case DRIVE_UNKNOWN:
		case DRIVE_NO_ROOT_DIR:
		case DRIVE_REMOVABLE:
		case DRIVE_REMOTE:
		case DRIVE_CDROM:
		case DRIVE_RAMDISK:
			if (type == FIXED_DRIVES)
				drives ^= (1 << ii);
			break;
		default:
			break;
		}
	}

	if (type == NON_FIXED_DRIVES)
	{
		if (!(drives & 1))	drives |= 1;	/* A */
		if (!(drives & 2))	drives |= 2;	/* B */
	}

	DbgPlain (DBG_DL, _T("%s drives bits %d"), 
		type == FIXED_DRIVES ? _T("FIXED") : _T("REMOVABLE"), drives
		);

	RETURN_VALUE (drives);
}

/* ===========================================================================
|
|	FUNCTION	DlFindLogicalDrive
|
 ========================================================================== */	

#define	CLEANUP											\
{														\
	DWORD	ii;											\
	if (pDiskKey)	FREE (pDiskKey);					\
	for (ii = 0; layouts && ii < NrPhysDrives; ii++)	\
		FREE (layouts[ii]);								\
	if (layouts)	FREE (layouts);						\
}

int
DlFindLogicalDrive (
	UCHAR					driveLetter,
	DWORD					*NrPartitions,
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	*pInfo,
	PDkPartitionInfo		*pDkInfo
	)
{
	ERH_FUNCTION (_T("DlFindLogicalDrive"));

	int						ret = 0, errNr = 0;
	DWORD					ii;
	DWORD					NrPhysDrives = 0;
	DWORD					nonFixedDrives = 0;
	PDrive_Layout			*layouts = NULL;
	PDkHeader				pDiskKey = NULL;
	PPartition_Information	*pInfoRet = NULL;
	PDkPartitionInfo		*pDkInfoRet = NULL;


	DBG_ENTER_FCN;

	/* -----------------------------------------------------------------------
	|	initialize RETURN_VALUE values
	 -----------------------------------------------------------------------*/
	*NrPartitions = 0;
	*PhysDrive = NULL;
	*PhysDriveSign = NULL;
	*pInfo = NULL;
	*pDkInfo = NULL;

	/* -----------------------------------------------------------------------
	|	Get logical volumes that are not on local physical drives 
	|	(shared disks, floppy disks, ...)
	 -----------------------------------------------------------------------*/
	nonFixedDrives = DlGetDrives (NON_FIXED_DRIVES);

	/* -----------------------------------------------------------------------
	|	get number of physical drives & drive layout for each one
	 -----------------------------------------------------------------------*/
	if (DlGetLayouts (&NrPhysDrives, &layouts) == -1)
	{
		errNr = ErhErrno ();
		CLEANUP;
		DbgPlain (DBG_DL, _T("Exiting DlFindLogicalDrive -> Cannot get layouts"));
		RETURN_VALUE (-1);
	}

	/* -----------------------------------------------------------------------
	|	if there is HKEY_LOCAL_MACHINE\SYSTEM\Disk key value "Information"
	|	get it 
	 -----------------------------------------------------------------------*/
	if (DlGetDiskKey (&pDiskKey) == -1)
	{
		STAMP;
		DbgPlain (DBG_DL, _T("Cannot find <DISK key>/<Information value> in registry"));
		ErhClearError ();
	}

	/* -----------------------------------------------------------------------
	|	do the real job in call to DlFindDriveEngine
	|	Q: why is this so ??
	|	A: same engine is used in DlLogicalDrivesOnPhysDrive
	 -----------------------------------------------------------------------*/
	ret = DlFindDriveEngine (
		driveLetter,		/*	input	*/
		NrPhysDrives,
		layouts, 
		pDiskKey,
		nonFixedDrives,
		NrPartitions,		/*	output	*/
		PhysDrive,
		PhysDriveSign,
		&pInfoRet,
		&pDkInfoRet,
		OB2_400_NT4_SIGNATURE
		);

	if (ret != -1)
	{
		*pInfo = (PPartition_Information) MALLOC (
			*NrPartitions * sizeof (Partition_Information)
			);
		if (pDkInfoRet)
		{
			*pDkInfo = (PDkPartitionInfo) MALLOC (
				*NrPartitions * sizeof (DkPartitionInfo)
				);
		}
		for (ii = 0; ii < *NrPartitions; ii++)
		{
			(*pInfo)[ii] = *(pInfoRet[ii]);
			if (pDkInfoRet)
				(*pDkInfo)[ii] = *(pDkInfoRet[ii]);
		}

		FREE (pInfoRet);
		if (pDkInfoRet)
			FREE (pDkInfoRet);
	}
	
	CLEANUP;

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION	DlLogicalDrivesOnPhysDrive
|
 ========================================================================== */	

int
DlLogicalDrivesOnPhysDrive (DWORD physDriveNr, DWORD *result)
{
	ERH_FUNCTION (_T("DlLogicalDrivesOnPhysDrive"));

	int						ret = 0, errNr = 0;
	DWORD					ii = 0, jj = 0, NrPhysDrives = 0;
	DWORD					DWordBitSz = sizeof (DWORD) * 8;
	DWORD					nonFixedDrives = 0, fixedDrives = 0;
	PDrive_Layout			*layouts = NULL;
	PDkHeader				pDiskKey = NULL;

	DBG_ENTER_FCN;

	/* -----------------------------------------------------------------------
	|	initialize RETURN_VALUE values
	 -----------------------------------------------------------------------*/
	*result = 0;

	/* -----------------------------------------------------------------------
	|	Get logical volumes that are not on local physical drives 
	|	(shared disks, floppy disks, ...)
	 -----------------------------------------------------------------------*/
	nonFixedDrives = DlGetDrives (NON_FIXED_DRIVES);
	fixedDrives = DlGetDrives (FIXED_DRIVES);

	/* -----------------------------------------------------------------------
	|	get number of physical drives & drive layout for each one
	 -----------------------------------------------------------------------*/
	if (DlGetLayouts (&NrPhysDrives, &layouts) == -1)
	{
		errNr = ErhErrno ();
		CLEANUP;
		DbgPlain (DBG_DL, _T("Exiting DlLogicalDrivesOnPhysDrive -> Cannot get layouts"));
		RETURN_VALUE (-1);
	}

	/* -----------------------------------------------------------------------
	|	if there is HKEY_LOCAL_MACHINE\SYSTEM\Disk key value "Information"
	|	get it 
	 -----------------------------------------------------------------------*/
	if (DlGetDiskKey (&pDiskKey) == -1)
	{
		errNr = ErhErrno ();
		CLEANUP;
		DbgPlain (DBG_DL, _T("Exiting DlLogicalDrivesOnPhysDrive -> Cannot get DISK key"));
		RETURN_VALUE (-1);
	}

	/* -----------------------------------------------------------------------
	|	get only logical drives that are on hard disks, for each of them find
	|	out if it is on physical drive passed as argument
	 -----------------------------------------------------------------------*/
	for (ii = 0; ret != -1 && ii < DWordBitSz; ii++)
	{
		DWORD					NrPartitions = 0;
		DWORD					*PhysDrive = NULL;
		DWORD					*PhysDriveSign = NULL;
		PPartition_Information	*pInfo = NULL;
		PDkPartitionInfo		*pDkInfo = NULL;

		if (!(fixedDrives & (1 << ii)))
			continue;

		ret = DlFindDriveEngine (
			(CHAR) (_T('A') + (CHAR ) ii),
			NrPhysDrives,
			layouts, 
			pDiskKey,
			nonFixedDrives,
			&NrPartitions,
			&PhysDrive,
			&PhysDriveSign,
			&pInfo,
			&pDkInfo,
			OB2_400_NT4_SIGNATURE
			);

		if (ret != -1)
		{
			for (jj = 0; jj < NrPartitions; jj++)
			{
				if (PhysDrive[jj] == physDriveNr)
					*result |= (1 << ii);
			}
		}

		NrPartitions = 0;
		FREE (PhysDrive);
		FREE (PhysDriveSign);
		FREE (pInfo);
		FREE (pDkInfo);
	}

	CLEANUP;

	if (ret == -1)
		*result = 0;

	RETURN_VALUE (ret);
}


/* ===========================================================================
|
|	FUNCTION	DlFindLogicalDrivesOnPhysDrive
|
 ========================================================================== */	

int
DlFindLogicalDrivesOnPhysDrive (DWORD physDriveNr, DWORD *result)
{
    ERH_FUNCTION (_T("DlFindLogicalDrivesOnPhysDrive"));

    tchar       drives[STRLEN_STD+2] = {0};
    tchar       *p; 
    int         ret = 0;


    *result = 0;

    if(GetLogicalDriveStrings(STRLEN_STD, drives)==0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, 
            _T("[%s] GetLogicalDriveStrings() failed! Error[%lu]"), 
            __FCN__, GetLastError());
        return -1;
    }
    p = drives;

    while(*p != 0)
    {
        tchar    dosNameSpace[8] = {0};
        tchar    devicePath[STRLEN_PATH+1] = {0};
        DWORD    drivetype;

        /* -------------------------------------------------------------------
        |   QueryDosDevice is used to determine if this drive is really
        |   partition on physical drive or is it just an SUBST created link
         -------------------------------------------------------------------*/
        sprintf (dosNameSpace, _T("%c:"), p[0]);
        if (QueryDosDevice (dosNameSpace, devicePath, STRLEN_PATH))
        {
            if (strncmp (devicePath, _T("\\??"), 3) == 0)
                goto next_lbl;
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] QueryDosDevice() failed for drive %s, err=%d"),
                __FCN__, dosNameSpace, GetLastError());
            return -1;
        }

        drivetype = GetDriveType(p);

        if (drivetype == DRIVE_UNKNOWN)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] GetDriveType() for drive %s returns DRIVE_UNKNOWN"),
                __FCN__, p
            );
            goto next_lbl;
        }
        else if (drivetype == DRIVE_FIXED)
        {
            tchar       logDrive[STRLEN_PATH+1] = {0};
            HANDLE      hDevice;               // handle to the drive to be examined 
            DWORD       dwBufferSize = sizeof(VOLUME_DISK_EXTENTS);
            BYTE        *lpOutBuffer = NULL;
            int         k = 0;
            DWORD       lpBytesReturned;
            BOOL        bRetVal = 0;
            int         dwErr = 0;
            PVOLUME_DISK_EXTENTS    VExts;

            strcpy(logDrive, _T("\\\\?\\"));
            strcat(logDrive, dosNameSpace);

            hDevice = CreateFile(logDrive,  // drive to open
                0,                // no access to the drive
                FILE_SHARE_READ | // share mode
                FILE_SHARE_WRITE, 
                NULL,             // default security attributes
                OPEN_EXISTING,    // disposition
                0,                // file attributes
                NULL);            // do not copy file attributes

            if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
            {
                dwErr = GetLastError();
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] CreateFile failed! Error[%lu]"),
                    __FCN__, dwErr);
                goto next_lbl;
            }

            lpOutBuffer = (BYTE *) MALLOC (dwBufferSize * sizeof (BYTE));

            for (k=0; k<2; k++)
            {
                bRetVal = DeviceIoControl(hDevice,
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL, 0, lpOutBuffer, dwBufferSize,
                            &lpBytesReturned, NULL);

                dwErr = GetLastError();
                if (bRetVal  || dwErr  != ERROR_MORE_DATA)
                {
                    /*QXCR1000888321- DevIoControl also fails with "ERROR_NOT_READY"*/
                    /*This error actually means that drive exists but is unavailable.*/
                    /*If we have an un-initialized disk existing on a system, we should not give up*/
                    /*and keep finding disks.*/
                    bRetVal = 1;

                    break;
                }
                /* if DeviceIoControl failed and
                    last error == ERROR_MORE_DATA, do REALLOC of lpOutBuffer
                    We are doing that only twice. */

                dwBufferSize += sizeof(DISK_EXTENT);
                lpOutBuffer = (BYTE *) REALLOC (lpOutBuffer,
                                        dwBufferSize * sizeof (BYTE));
            }
            
            CloseHandle(hDevice);

            if (!bRetVal)
            {
                dwErr = GetLastError();
                FREE(lpOutBuffer);
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,
                    _T("[%s] DeviceIoControl failed! Error[%lu]"), 
                    __FCN__, dwErr);
                return -1;
            }
            VExts = (PVOLUME_DISK_EXTENTS)lpOutBuffer;

            if (physDriveNr == VExts->Extents[0].DiskNumber)
            {
                *result |= (1 << (int)((int)p[0]-_T('A')));
            }

            FREE(lpOutBuffer);
        }
        /*else if (drivetype == DRIVE_REMOVABLE)
        {
            *result |= (1 << ((int)p[0]-'A'));
        }*/
next_lbl:
        for (p+=1; *p != 0; p++);
        p+=1;
    }
    return ret;
}


/* ===========================================================================
|
|	FUNCTION	DlGetFreeDriveLetter
|
 ========================================================================== */	

CHAR
DlGetFreeDriveLetter (DWORD drives)
{
	ERH_FUNCTION (_T("DlGetFreeDriveLetter"));

	CHAR	ii;
	CHAR	ret = 'A';

	DBG_ENTER_FCN;

	for (ii = 0; ii < sizeof (drives) * 8; ii++)
	{
		if (!(drives & 1))
		{
			ret += ii;
			break;
		}

		drives >>= 1;
	}

	RETURN_VALUE (ret);
}

#ifdef _NEW_DRIVE_ENGINE_

#define PARTITION_PRIMARY			1		/* primary (set) vs. logical (reset) */
#define PARTITION_BOOT				2		/* boot  */
#define PARTITION_NONBOOT			4		/* non-boot */
#define PARTITION_REVERSE			8		/* reverse order (set) or normal order (reset) */
#define PARTITION_FORCE				16		/* reverse order (set) or normal order (reset) */

static int 
KnownPartition(
		BYTE bType
)
{
	return (!IsContainerPartition(bType) && IsRecognizedPartition(bType) ? 1 : 0);
}

static int 
AssignDriveLetter(
		BOOL bBootPartition,
		PPartition_Information pPart
)
{
	int		nRet = 0;

	if(bBootPartition ||
		(KnownPartition(pPart->PartitionType) && !(pPart->PartitionType & PARTITION_NTFT)))
		nRet = 1;
	
	return nRet;
}

static int
GetSig(
		DWORD dwDisk, 
		PDrive_Layout *Layouts
)
{
	PDrive_Layout pLay = Layouts[dwDisk];

	return pLay->Signature;
}

static int 
GetPartition(
		int nType, 
		LPVOID lpData,
		int nDisk, 
		PDrive_Layout *Layouts, 
		PPartition_Information *ppOut
)
{
	int		nRet = -1, nPart = 0, nLimit = 0, nAdd = 1;
	int		nLowest = 0, nHighest = 256;
	PDrive_Layout	pLay = Layouts[nDisk];
	PPartition_Information pCur = NULL, pRet = NULL, pPart = NULL;
	size_t nCurrent = 0, nSearched = 0;
	
	if(nType & PARTITION_PRIMARY)
	{
		nSearched = (size_t)lpData;
		nCurrent = 0;
		/* Primary partition */
		if(nType & PARTITION_REVERSE)
		{
			nPart = 3; nLimit = 0; nAdd = -1;
		}
		else
		{
			nPart = 0; nLimit = 3; nAdd = 1;
		}
	}
	else
	{
		/* Logical partition */
		nPart = 4;
		nLimit = (int)pLay->PartitionCount-1;
		nAdd = 1;
		pPart = (PPartition_Information)lpData;
		nLowest = (int)(pPart == NULL ? 0 : pPart->PartitionNumber);	
	}
	
	for(; (nType & PARTITION_REVERSE ? nPart >= (int)nLimit : nPart <= (int)nLimit); nPart+=nAdd)
	{
		pCur = &pLay->PartitionEntry[nPart];

		if(pCur->PartitionType && !IsContainerPartition(pCur->PartitionType))
		{
			if((nType & PARTITION_BOOT) && !(nType & PARTITION_NONBOOT) && pCur->BootIndicator == FALSE)
				continue;
			if((nType & PARTITION_NONBOOT) && !(nType & PARTITION_BOOT) && pCur->BootIndicator)
				continue;

			if(nType & PARTITION_PRIMARY)
			{
				if(!(nType & PARTITION_FORCE))
				{
					if(pCur->BemTiJeTerMa == 0 && nCurrent++ == nSearched)
					{
						pRet = pCur;
						pRet->BemTiJeTerMa = 1;
						break;
					}
				}
				else
				{
					if(nCurrent++ == nSearched)
					{
						pRet = pCur;
						break;
					}
				}
			}
			else
			{
				if(pCur->PartitionNumber > (DWORD)nLowest && 
					pCur->PartitionNumber < (DWORD)nHighest &&
					pCur->BemTiJeTerMa == 0)
				{
					nHighest = pCur->PartitionNumber;
					pRet = pCur;
				}
			}
		}
	}

	if((nType & PARTITION_PRIMARY) == 0 && pRet != NULL)
	{
		/* for logical partitions */
		pRet->BemTiJeTerMa = 1;
	}

	*ppOut = pRet;
	nRet = (pRet ? 0 : nRet);
	
	return nRet;
}

static int
GetDiskFromSignature(
	DWORD dwSig, 
	PDkHeader pDiskKey, 
	PDrive_Layout *Layouts, 
	DWORD NrDrives,
	BOOL bFromLayout
)
{
	int			nRet = -1;
	int			nDisk = 0;
	DWORD		dkSize = 0;
	PDkDisks			dkDisks = NULL;
	PDkDiskInfo			dkDisk = 0;
	PDkPartitionHeader	dkPHdr = NULL;

	if(dwSig != 0)
	{
		if(bFromLayout)
		{
			for(nDisk = 0; nDisk < (int)NrDrives; nDisk++)
			{
				if(dwSig == Layouts[nDisk]->Signature)
				{
					nRet = nDisk;
					break;
				}
			}
		}
		else
		{
			dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
			for (nDisk = 0; nDisk < (int)dkDisks->NrDisks; nDisk++)
			{
				dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
				if (dkDisk->NrPartitions > 0)
				{
					dkPHdr = (PDkPartitionHeader) 
						(char *) &dkDisk->PartitionHeader;
					if(dwSig == dkPHdr->Signature)
					{
						nRet = nDisk;
						break;
					}
				}
				dkSize += sizeof (DkPartitionHeader);
				dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
			}
		}
	}

	return nRet;
}

int ResidesInDiskKey(
		PDkHeader pDiskKey,
		DWORD dwDisk,
		DWORD dwSig,
		LARGE_INTEGER liOffset,
		LARGE_INTEGER liLength,
		PDkPartitionInfo *ppPart
)
{
	int					nRet = -1, nD = -1;
	int					i = 0, j = 0;
	DWORD				dkSize = 0;
	PDkDisks			dkDisks = NULL;
	PDkDiskInfo			dkDisk = 0;
	PDkPartitionHeader	dkPHdr = NULL;
	PDkPartitionInfo	dkPInfo = NULL;

	if(pDiskKey)
	{
		nD = GetDiskFromSignature(dwSig, pDiskKey, NULL, 0, FALSE);
		if(nD != -1)
		{
			dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
			for (i = 0; i < dkDisks->NrDisks && -1 == nRet; i++)
			{
				dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
				if (dkDisk->NrPartitions > 0)
				{
					dkPHdr = (PDkPartitionHeader) 
						(char *) &dkDisk->PartitionHeader;
					for (j = 0; j < dkDisk->NrPartitions && -1 == nRet; j++)
					{
						dkPInfo = (PDkPartitionInfo) 
							((char *) &dkPHdr->Partitions +
								j * sizeof (DkPartitionInfo)
							);
						if(i != nD)
							continue;
						if(memcmp(&dkPInfo->StartingOffset, &liOffset, sizeof(LARGE_INTEGER)) != 0)
							continue;
						if(memcmp(&dkPInfo->Length, &liLength, sizeof(LARGE_INTEGER)) != 0)
							continue;
						
						*ppPart = dkPInfo;
						nRet = 0;
					}
				}
				dkSize += sizeof (DkPartitionHeader);
				dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
			}
		}
	}

	return nRet;
}

void RemoveRestoreDriveLetters(
		PDkHeader pDiskKey,
		PDrive_Layout *Layouts,
		DWORD NrDrives,
		BOOL bRemove
)
{
	int					nDisk = -1;
	int					i = 0, j = 0, k = 0;
	BOOL				bAssign = FALSE;
	DWORD				dkSize = 0;
	PDkDisks			dkDisks = NULL;
	PDkDiskInfo			dkDisk = 0;
	PDkPartitionHeader	dkPHdr = NULL;
	PDkPartitionInfo	dkPInfo = NULL;
	PDrive_Layout		pLay = NULL;
	PPartition_Information	pPart = NULL;

	if(pDiskKey)
	{
		dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
		for (i = 0; i < dkDisks->NrDisks; i++)
		{
			dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
			if (dkDisk->NrPartitions > 0)
			{
				dkPHdr = (PDkPartitionHeader) 
					(char *) &dkDisk->PartitionHeader;
				for (j = 0; j < dkDisk->NrPartitions; j++)
				{
					dkPInfo = (PDkPartitionInfo) 
						((char *) &dkPHdr->Partitions +
							j * sizeof (DkPartitionInfo)
						);

					bAssign = FALSE;
					/* get disk number based on DISK key signature (dkPHdr->Signature) from current layout */
					nDisk = GetDiskFromSignature(dkPHdr->Signature, NULL, Layouts, NrDrives, TRUE);

					if(nDisk != -1)
					{
//						if(nDisk != i)
//							nDisk = i;

						if(nDisk >= (int)NrDrives)
							continue;

						pLay = Layouts[nDisk];
						
						for(k = 0; k < (int)pLay->PartitionCount; k++)
						{
							pPart = &pLay->PartitionEntry[k];

							if(memcmp(&dkPInfo->StartingOffset, &pPart->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
								continue;
							if(memcmp(&dkPInfo->Length, &pPart->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
								continue;
							bAssign = TRUE;
							break; 
						}
					}
					
					if(!bAssign)
					{
						if(bRemove)
							dkPInfo->DriveLetter |= 0x80;
						else
							dkPInfo->DriveLetter &= 0x7F;
					}
					
				}
			}
			dkSize += sizeof (DkPartitionHeader);
			dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
		}
	}
}

static void 
RemoveRestoreHandledPartitions(
		PDrive_Layout *Layouts,
		DWORD NrDrives
)
{
	int						nPart = 0, nDisk = 0;
	PDrive_Layout			pLay = NULL;
	PPartition_Information	pPart = NULL;

	for(nDisk = 0; nDisk < (int)NrDrives; nDisk++)
	{
		pLay = Layouts[nDisk];

		for(nPart = 0; nPart < (int)pLay->PartitionCount; nPart++)
		{
			pPart = &pLay->PartitionEntry[nPart];
			pPart->BemTiJeTerMa = 0;
		}
	}
}

int IsFaultTolerantSet(PartitionType Type)
{
	BOOL	bSet = FALSE;

	switch(Type)
	{
		case PT_MIRROR_SET:
		case PT_STRIPE_SET:
		case PT_PARITY_STRIPE:
		case PT_VOLUME_SET:
			bSet = TRUE;
			break;
		default:
			break;
	}
	return bSet;
}

static int 
IsValidFaultTolerantMember(
	PDkPartitionInfo pPart, 
	int nDisk, 
	DWORD dwSig,
	PDrive_Layout *Layouts,
	DWORD NrDrives,
	PPartition_Information *ppInfo
)
{
	int				nRet = -1, nPart = 0, nD = 0;
	PDrive_Layout	pLay = NULL;
	PPartition_Information pInfo = NULL;

	if(nDisk < (int)NrDrives)
	{
		nD = GetDiskFromSignature(dwSig, NULL, Layouts, NrDrives, TRUE);
		if(nD != -1)
		{
			if(nD != nDisk)
				nDisk = nD;

			pLay = Layouts[nDisk];

			for(nPart = 0; nPart < (int)pLay->PartitionCount && nRet == -1; nPart++)
			{
				pInfo = &pLay->PartitionEntry[nPart];

				if(pInfo->PartitionType)
				{
					if(memcmp(&pPart->StartingOffset, &pInfo->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
						continue;
					if(memcmp(&pPart->Length, &pInfo->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
						continue;

					*ppInfo = pInfo;

					nRet = 0;
				}
			}
		}
	}

	return nRet;
}

int
HandleFaultTolerantSet(
	PDkHeader pDiskKey, 
	PDrive_Layout *Layouts,
	DWORD NrDrives,
	PDkPartitionInfo pPart, 
	CHAR *pLetter
)
{
	int					nRet = -1;
	int					i = 0, j = 0;
	DWORD				dkSize = 0;
	PDkDisks			dkDisks = NULL;
	PDkDiskInfo			dkDisk = 0;
	PDkPartitionHeader	dkPHdr = NULL;
	PDkPartitionInfo	dkPInfo = NULL;
	PPartition_Information pInfo = NULL;

	if(pDiskKey)
	{
		if(IsFaultTolerantSet(pPart->Type))
		{
			*pLetter = 0;

			dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
			for (i = 0; i < dkDisks->NrDisks && nRet == -1; i++)
			{
				dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
				if (dkDisk->NrPartitions > 0)
				{
					dkPHdr = (PDkPartitionHeader) 
						(char *) &dkDisk->PartitionHeader;
					for (j = 0; j < dkDisk->NrPartitions && nRet == -1; j++)
					{
						dkPInfo = (PDkPartitionInfo) 
							((char *) &dkPHdr->Partitions +
								j * sizeof (DkPartitionInfo)
							);

						pInfo = NULL;

						if(	dkPInfo->Type == pPart->Type && 
							dkPInfo->FtGroup == pPart->FtGroup &&
							dkPInfo->FtMember == 0 &&
							IsValidFaultTolerantMember(dkPInfo, i, dkPHdr->Signature, Layouts, NrDrives, &pInfo) == 0 &&
							(dkPInfo->DriveLetter & 0x7F) != 0
						)
						{
							*pLetter = (dkPInfo->DriveLetter & 0x7F);
							nRet = 0;
						}
					}
				}
				dkSize += sizeof (DkPartitionHeader);
				dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
			}
		}
	}

	return nRet;
}

static int
AddFaultTolerantMembers(
			PDkPartitionInfo pDkPart,
			DWORD NrDrives,
			PDrive_Layout *Layouts,
			PDkHeader pDiskKey,
			DWORD *pNrPart,		/*	output	*/
			DWORD *pNrDkPart,		/*	output	*/
			DWORD **PhysDrive,
			DWORD **PhysDriveSign,
			PPartition_Information **pInfo,
			PDkPartitionInfo **pDkInfo
)
{
	int		nRet = -1;
	int		nDisk = 0, nPart = 0;
	DWORD	dkSize = 0;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	PDkPartitionHeader		dkPHdr = NULL;
	PDkPartitionInfo		dkPInfo = NULL;
	PPartition_Information	pPartInfo = NULL;

	dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
	for (nDisk = 0; nDisk < dkDisks->NrDisks && nRet == -1; nDisk++)
	{
		dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
		if (dkDisk->NrPartitions > 0)
		{
			dkPHdr = (PDkPartitionHeader) 
				(char *) &dkDisk->PartitionHeader;
			for (nPart = 0; nPart < dkDisk->NrPartitions && nRet == -1; nPart++)
			{
				dkPInfo = (PDkPartitionInfo) 
					((char *) &dkPHdr->Partitions +
						nPart * sizeof (DkPartitionInfo)
					);

				pPartInfo = NULL;

				if(dkPInfo->Type == pDkPart->Type &&
					dkPInfo->FtGroup == pDkPart->FtGroup &&
					dkPInfo->FtMember != pDkPart->FtMember &&
					IsValidFaultTolerantMember(dkPInfo, nDisk, dkPHdr->Signature, Layouts, NrDrives, &pPartInfo) == 0
				)
				{
					*pInfo = (PPartition_Information *) REALLOC (
						*pInfo, (*pNrPart + 1) * sizeof (PPartition_Information)
						);
					(*pInfo)[*pNrPart] = pPartInfo;

					*PhysDrive = (DWORD *) REALLOC (
						*PhysDrive, (*pNrPart + 1) * sizeof (DWORD)
						);
					(*PhysDrive)[*pNrPart] = (DWORD)nDisk;

					*PhysDriveSign = (DWORD *) REALLOC (
						*PhysDriveSign, (*pNrPart + 1) * sizeof (DWORD)
						);
					(*PhysDriveSign)[*pNrPart] = Layouts[nDisk]->Signature;

					*pDkInfo = (PDkPartitionInfo *) REALLOC (
						*pDkInfo, 
						(*pNrDkPart + 1) * sizeof (PDkPartitionInfo)
						);
					(*pDkInfo)[*pNrDkPart] = dkPInfo;

					(*pNrPart)++;
					(*pNrDkPart)++;
				}
			}
		}
	}
	return nRet;
}

static int
FaultTolerantPresent(DWORD NrDisks, PDrive_Layout *Layouts, DWORD dwDisk)
{
	int				nRet = 0;
	int				nPart = 0;
	PDrive_Layout	pLay = Layouts[dwDisk];
	PPartition_Information pPart = NULL;

	for(nPart = 0; nPart < 4; nPart++)
	{
		pPart = &pLay->PartitionEntry[nPart];

		if(pPart->PartitionType && (pPart->PartitionType & PARTITION_NTFT) != 0)
		{
			nRet = 1;
			break;
		}
	}

	return nRet;
}

static int
GetBootDisk(
		DWORD NrDrives,
		PDrive_Layout *Layouts,
		PPartition_Information *ppPart
)
{
	int				nRet = -1;
	int				nDisk = 0, nPart = 0;
	PPartition_Information pPart = NULL;

	for(nDisk = 0; nDisk < (int)NrDrives; nDisk++)
	{
		for(nPart = 0; nPart < 4; nPart++)
		{
			pPart = &Layouts[nDisk]->PartitionEntry[nPart];

			if(pPart->PartitionType && pPart->BootIndicator)
			{
				nRet = nDisk;
				*ppPart = pPart;
				break;
			}
		}
	}

	return nRet;
}

int GetDriveLetter(
		DWORD dwDisk,
		LARGE_INTEGER liOffset,
		LARGE_INTEGER liLength,
		DWORD dwTotalDisks,
		PDrive_Layout *Layouts,
		PDkHeader pDiskKey,
		DWORD dwAssigned,
		PDkPartitionInfo *pPartInfo,
		CHAR *pLetter
)
{
	int						nRet = -1, i = 0;
	int						nBootDisk = -1; /* nType = PARTITION_PRIMARY; */
	int						DiskIdxs[32];
	CHAR					curLetter = 0;
	PDkPartitionInfo 		pPart = NULL;
	PPartition_Information	pCur = NULL, pBoot = NULL;
	DWORD					dwLetters = dwAssigned;
	BOOL					bContinue = FALSE;

	/* Find boot disk */
	nBootDisk = GetBootDisk(dwTotalDisks, Layouts, &pCur);

	/* Assign disk evaluation path */
	if(nBootDisk != -1)
		DiskIdxs[0] = nBootDisk;
	for(i = 0; i < (int)dwTotalDisks; i++)	
	{
		if(i != nBootDisk)
		{
			if(i < nBootDisk)
				DiskIdxs[i+1] = i;
			else
				DiskIdxs[i] = i;
		}
	}
	
	/* Remove drive letters from all those partitions existing in disk key, but not */
	/* being part of current configuration 											*/
	RemoveRestoreDriveLetters(pDiskKey, Layouts, dwTotalDisks, TRUE);
	/* The same goes for all partition in layouts. */
	RemoveRestoreHandledPartitions(Layouts, dwTotalDisks);
	
	/* The following loop handles all primary partitions */
	for(
		i = 0; 
		i < (int)dwTotalDisks && 
		GetPartition(
			(i == 0 && nBootDisk != -1 ?								/* If boot disk exists and it is the current disk */
				PARTITION_PRIMARY | PARTITION_BOOT :					/* then fetch boot partition, otherwise fetch the */
				PARTITION_PRIMARY | PARTITION_BOOT | PARTITION_NONBOOT), /* first normal primary partition.				  */
			(LPVOID)0, 
			DiskIdxs[i], 
			Layouts, 
			&pCur) != -1;
		i++
	)
	{
		curLetter = 0;
		pPart = NULL;
		
		if(ResidesInDiskKey(pDiskKey, DiskIdxs[i], GetSig(DiskIdxs[i], Layouts), 
			pCur->StartingOffset, pCur->PartitionLength, &pPart) != -1)
		{
		/* Partition with such an offset resides within disk key. */
			if(pPart->AssignDriveLetter == 0)
			{
				/* Drive letter must not be assigned unless we are talking about fault tolerant volume. */
				if(!IsFaultTolerantSet(pPart->Type))
					continue;
				HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
			}
			else if((pPart->DriveLetter & 0x80) == 0 && (pPart->DriveLetter & 0x7F) != 0)
			{
			/* Drive letter must be assigned and is specified */
				curLetter = (pPart->DriveLetter & 0x7F);
			}
			else
			{
			/* Drive letter must be assigned and is not specified  */
			/* This is certainly not one of sets, because sets     */
			/* must have drive letters specified. This can in fact */
			/* be one of sets - a broken set member.               */
				HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
			}
		}
		
		/* If drive letter hasn't been obtained by means of Disk key  */
		/* I should obtain it by using our internal function          */
		if(curLetter == 0 && AssignDriveLetter(pBoot == pCur, pCur))
			curLetter = DlGetFreeDriveLetter(dwLetters);

		if(curLetter)
			dwLetters |= (1 << (curLetter - 'A'));
		
		/* Validation that current partition sizes match and that partition */
		/* resides on the required disk 				    */
		if(dwDisk != (DWORD)DiskIdxs[i])
			continue;
		if(memcmp(&liOffset, &pCur->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
			continue;
		if(memcmp(&liLength, &pCur->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
			continue;
		if(curLetter == 0)
			continue;
			
		/* This is the partition */
		*pLetter = curLetter;
		if(pPartInfo)
			*pPartInfo = pPart;
		nRet = 0;
		
		break;
	}
	
	/* The following loop handles all logical partitions on each disk */
	for(
		i = 0; 
		i < (int)dwTotalDisks && nRet == -1;
		i++
	)
	{
		if(FaultTolerantPresent(dwTotalDisks, Layouts, DiskIdxs[i]))
		{
			bContinue = TRUE;

			while(GetPartition(
				PARTITION_PRIMARY | PARTITION_BOOT | 
				PARTITION_NONBOOT | PARTITION_REVERSE | PARTITION_FORCE, 
				(LPVOID)0, DiskIdxs[i], Layouts, &pCur) != -1 &&
				bContinue == TRUE
			)
			{
				/* Only one loop is required */
				bContinue = FALSE;
				curLetter = 0;
				pPart = NULL;				
				
				/* If already handled, just skip everything else */
				if(pCur->BemTiJeTerMa)
					continue;

				pCur->BemTiJeTerMa = 1;

				if(ResidesInDiskKey(pDiskKey, DiskIdxs[i], GetSig(DiskIdxs[i], Layouts), 
					pCur->StartingOffset, pCur->PartitionLength, &pPart) != -1)
				{
				/* Partition with such an offset resides within disk key. */
					if(pPart->AssignDriveLetter == 0)
					{
						/* Drive letter must not be assigned unless we are talking about fault tolerant volume. */
						if(!IsFaultTolerantSet(pPart->Type))
							continue;
						HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
					}
					else if((pPart->DriveLetter & 0x80) == 0 && (pPart->DriveLetter & 0x7F) != 0)
					{
					/* Drive letter must be assigned and is specified */
						curLetter = (pPart->DriveLetter & 0x7F);
					}
					else
					{
					/* Drive letter must be assigned and is not specified  */
					/* This is certainly not one of sets, because sets     */
					/* must have drive letters specified. This can in fact */
					/* be one of sets - a broken set member.               */
						HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
					}
				}
				
				/* If drive letter hasn't been obtained by means of Disk key  */
				/* I should obtain it by using our internal function          */
				if(curLetter == 0 && AssignDriveLetter(FALSE, pCur))
					curLetter = DlGetFreeDriveLetter(dwLetters);
		
				if(curLetter)
					dwLetters |= (1 << (curLetter - 'A'));
				
				/* Validation that current partition sizes match and that partition */
				/* resides on the required disk 				    */
				if(dwDisk != (DWORD)DiskIdxs[i])
					continue;
				if(memcmp(&liOffset, &pCur->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
					continue;
				if(memcmp(&liLength, &pCur->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
					continue;
				if(curLetter == 0)
					continue;
					
				/* This is the partition */
				*pLetter = curLetter;
				if(pPartInfo)
					*pPartInfo = pPart;
				nRet = 0;
				
				break;
			}
		}

		pCur = NULL;
		
		while(
			GetPartition(PARTITION_BOOT | PARTITION_NONBOOT, pCur, DiskIdxs[i], Layouts, &pCur) != -1 &&
			nRet == -1
		)
		{
			curLetter = 0;
			pPart = NULL;
			
			if(ResidesInDiskKey(pDiskKey, DiskIdxs[i], GetSig(DiskIdxs[i], Layouts), 
				pCur->StartingOffset, pCur->PartitionLength, &pPart) != -1)
			{
			/* Partition with such an offset resides within disk key. */
				if(pPart->AssignDriveLetter == 0)
				{
					/* Drive letter must not be assigned unless we are talking about fault tolerant volume. */
					if(!IsFaultTolerantSet(pPart->Type))
						continue;
					HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
				}
				else if((pPart->DriveLetter & 0x80) == 0 && (pPart->DriveLetter & 0x7F) != 0)
				{
				/* Drive letter must be assigned and is specified */
					curLetter = (pPart->DriveLetter & 0x7F);
				}
				else
				{
				/* Drive letter must be assigned and is not specified  */
				/* This is certainly not one of sets, because sets     */
				/* must have drive letters specified. This can in fact */
				/* be one of sets - a broken set member.               */
					HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
				}
			}
			
			/* If drive letter hasn't been obtained by means of Disk key  */
			/* I should obtain it by using our internal function          */
			if(curLetter == 0 && AssignDriveLetter(FALSE, pCur))
				curLetter = DlGetFreeDriveLetter(dwLetters);
	
			if(curLetter)
				dwLetters |= (1 << (curLetter - 'A'));
			
			/* Validation that current partition sizes match and that partition */
			/* resides on the required disk 				    */
			if(dwDisk != (DWORD)DiskIdxs[i])
				continue;
			if(memcmp(&liOffset, &pCur->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
				continue;
			if(memcmp(&liLength, &pCur->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
				continue;
			if(curLetter == 0)
				continue;
				
			/* This is the partition */
			*pLetter = curLetter;
			if(pPartInfo)
				*pPartInfo = pPart;
			nRet = 0;
			
			break;
		}
	}
	
	/* The following loop handles the rest of primary partitions on each disk */
	for(
		i = 0; 
		i < (int)dwTotalDisks && nRet == -1;
		i++
	)
	{
		pCur = NULL;
		
		while(
			GetPartition(
				(
				i == 0 && nBootDisk != -1 ? 
					PARTITION_PRIMARY | PARTITION_NONBOOT :
					PARTITION_PRIMARY | PARTITION_BOOT | PARTITION_NONBOOT
				), 
				(LPVOID)0, 
				DiskIdxs[i], 
				Layouts, 
				&pCur) != -1
		)
		{
			curLetter = 0;
			pPart = NULL;
			
			if(ResidesInDiskKey(pDiskKey, DiskIdxs[i], GetSig(DiskIdxs[i], Layouts), 
				pCur->StartingOffset, pCur->PartitionLength, &pPart) != -1)
			{
			/* Partition with such an offset resides within disk key. */
				if(pPart->AssignDriveLetter == 0)
				{
					/* Drive letter must not be assigned unless we are talking about fault tolerant volume. */
					if(!IsFaultTolerantSet(pPart->Type))
						continue;
					HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
				}
				else if((pPart->DriveLetter & 0x80) == 0 && (pPart->DriveLetter & 0x7F) != 0)
				{
				/* Drive letter must be assigned and is specified */
					curLetter = (pPart->DriveLetter & 0x7F);
				}
				else
				{
				/* Drive letter must be assigned and is not specified  */
				/* This is certainly not one of sets, because sets     */
				/* must have drive letters specified. This can in fact */
				/* be one of sets - a broken set member.               */
					HandleFaultTolerantSet(pDiskKey, Layouts, dwTotalDisks, pPart, &curLetter);
				}
			}
			
			/* If drive letter hasn't been obtained by means of Disk key  */
			/* I should obtain it by using our internal function          */
			if(curLetter == 0 && AssignDriveLetter(FALSE, pCur))
				curLetter = DlGetFreeDriveLetter(dwLetters);
	
			if(curLetter)
				dwLetters |= (1 << (curLetter - 'A'));
			
			/* Validation that current partition sizes match and that partition */
			/* resides on the required disk 				    */
			if(dwDisk != (DWORD)DiskIdxs[i])
				continue;
			if(memcmp(&liOffset, &pCur->StartingOffset, sizeof(LARGE_INTEGER)) != 0)
				continue;
			if(memcmp(&liLength, &pCur->PartitionLength, sizeof(LARGE_INTEGER)) != 0)
				continue;
			if(curLetter == 0)
				continue;
				
			/* This is the partition */
			*pLetter = curLetter;
			if(pPartInfo)
				*pPartInfo = pPart;
			nRet = 0;
			
			break;
		}
	}

	/* Restor drive letters to all those partitions existing in disk key, but not */
	/* being part of current configuration.										  */
	RemoveRestoreDriveLetters(pDiskKey, Layouts, dwTotalDisks, FALSE);
	/* The same goes for all partition in layouts. */
	RemoveRestoreHandledPartitions(Layouts, dwTotalDisks);
	
	return nRet;
}

/*========================================================================*//**
*
* @note      that other routines like DlFindLogicalDrive or
*            DlLogicalDrivesOnPhysDrive are mallocating space and coping
*            data from these pointers
*
*//*=========================================================================*/
int
DlFindDriveEngineStatic (
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*layouts, 
	PDkHeader				pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo
	)
{
	ERH_FUNCTION (_T("DlFindDriveEngineStatic"));

	int						ret = -1;
	USHORT					upperCase = (int) driveLetter;
	DWORD					ii = 0, jj = 0;
	DWORD					drives = 0;
	DWORD					NrPart = 0, NrDkPart = 0;
	PPartition_Information	part;
	PDkPartitionInfo		pDkPart = NULL;
	CHAR					Letter = 0;

	STAMP;
	driveLetter = (CHAR) toupper (upperCase);
	DbgPlain (DBG_DL, _T("DlFindDriveEngineStatic entered for drive %c"), driveLetter);

	drives = nonFixedDrives;

	for (ii = 0; ii < NrPhysDrives; ii++)
	{
		for (jj = 0; jj < layouts[ii]->PartitionCount; jj++)
		{
			part = DlPInformation (layouts[ii], jj);

			if(GetDriveLetter(
				ii, 
				part->StartingOffset,
				part->PartitionLength,
				NrPhysDrives,
				layouts,
				pDiskKey,
				drives,
				&pDkPart,
				&Letter
			) != -1)
			{
//				drives |= (1 << (Letter - 'A'));

				if(driveLetter == Letter)
				{
					*pInfo = (PPartition_Information *) REALLOC (
						*pInfo, (NrPart + 1) * sizeof (PPartition_Information)
						);
					(*pInfo)[NrPart] = part;

					*PhysDrive = (DWORD *) REALLOC (
						*PhysDrive, (NrPart + 1) * sizeof (DWORD)
						);
					(*PhysDrive)[NrPart] = ii;

					*PhysDriveSign = (DWORD *) REALLOC (
						*PhysDriveSign, (NrPart + 1) * sizeof (DWORD)
						);
					(*PhysDriveSign)[NrPart] = layouts[ii]->Signature;

					if(pDkPart)
					{
						*pDkInfo = (PDkPartitionInfo *) REALLOC (
							*pDkInfo, 
							(NrDkPart + 1) * sizeof (PDkPartitionInfo)
							);
						(*pDkInfo)[NrDkPart] = pDkPart;

						NrDkPart++;
					}

					NrPart++;
				}
			}
		}
	}

	*NrPartitions = NrPart;

	if(NrPart > 0)
		ret = 0;

	if (ret == -1)
	{
		*NrPartitions = 0;
		FREE (*PhysDrive);		*PhysDrive = NULL;
		FREE (*PhysDriveSign);	*PhysDriveSign = NULL;
		FREE (*pInfo);			*pInfo = NULL;
		FREE (*pDkInfo);		*pDkInfo = NULL;
		DbgStamp (DBG_DL);
		DbgPlain (DBG_DL, _T("DlFindDriveEngineStatic FAILED for drive letter %c"), driveLetter);
	}

	RETURN_VALUE (ret);
}

int
DlFindDriveEngineDynamic(
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*Layouts, 
	PDkHeader				pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo
	)
{
	ERH_FUNCTION (_T("DlFindDriveEngineDynamic"));

	int		nRet = -1;
	int		nDisk = 0, nPart = 0, nP = 0;
	DWORD	NrPart = 0, NrDkPart = 0;
	TCHAR	lpszDOS[5], lpszNT[128];
	PDrive_Layout			pLay = NULL;
	PPartition_Information	pPartInfo = NULL;
	PDkPartitionInfo		pDkPart = NULL;

	DBG_ENTER_FCN;

	_stprintf(lpszDOS, _T("%c:"), (TCHAR)driveLetter);

	if(QueryDosDevice(lpszDOS, lpszNT, 128) != 0)
	{
		STAMP;
		DbgPlain (DBG_DL, _T("Windows NT path '%s' matches DOS path '%s'."), lpszNT, lpszDOS);
	
		if(_tcsstr(lpszNT, _T("Harddisk")) != NULL && 
			_tcsstr(lpszNT, _T("Partition")) != NULL
		)
		{
			STAMP;
			DbgPlain (DBG_DL, _T("Harddisk and Partition strings found."));
		
			if(_stscanf(lpszNT, _T("\\Device\\Harddisk%d\\Partition%d"), &nDisk, &nPart) == 2)
			{
				STAMP;
				DbgPlain (DBG_DL, _T("Successfully converted disk (%d) and partition numbers (%d)."), nDisk, nPart);
	
				if(nDisk < (int)NrPhysDrives)
				{
					STAMP;
					DbgPlain (DBG_DL, _T("Disk number (%d) is valid. It is less than total disk number (%d)."), nDisk, NrPhysDrives);
	
					pLay = Layouts[nDisk];

					STAMP;
					DbgPlain (DBG_DL, _T("Partition count for disk %d is %d."), nDisk, pLay->PartitionCount);

					for(nP = 0; nP < (int)pLay->PartitionCount; nP++)
					{
						pPartInfo = &pLay->PartitionEntry[nP];

						/* ---------------------------------------------------------------------------
						|   First verify that partition is a valid partition and at the same time
						|	not an extended partition. It was noticed that even extended partition
						|	can have (in certain circumstances) its partition number set to a value
						|	that corresponds to a value already hold by another valid partition.
						 ---------------------------------------------------------------------------*/
						if(pPartInfo->PartitionType && !IsContainerPartition(pPartInfo->PartitionType))
						{
							if(pPartInfo->PartitionNumber == (DWORD)nPart)
								break;
						}
					}
					
					if(nP < (int)pLay->PartitionCount)
					{
						*pInfo = (PPartition_Information *) REALLOC (
							*pInfo, (NrPart + 1) * sizeof (PPartition_Information)
							);
						(*pInfo)[NrPart] = pPartInfo;

						*PhysDrive = (DWORD *) REALLOC (
							*PhysDrive, (NrPart + 1) * sizeof (DWORD)
							);
						(*PhysDrive)[NrPart] = (DWORD)nDisk;

						*PhysDriveSign = (DWORD *) REALLOC (
							*PhysDriveSign, (NrPart + 1) * sizeof (DWORD)
							);
						(*PhysDriveSign)[NrPart] = pLay->Signature;

						NrPart++;

						if(pDiskKey)
						{
							pDkPart = NULL;

							if(ResidesInDiskKey(
									pDiskKey, 
									nDisk,
									GetSig(nDisk, Layouts),
									pPartInfo->StartingOffset,
									pPartInfo->PartitionLength,
									&pDkPart
							) != -1)
							{
								*pDkInfo = (PDkPartitionInfo *) REALLOC (
									*pDkInfo, 
									(NrDkPart + 1) * sizeof (PDkPartitionInfo)
									);
								(*pDkInfo)[NrDkPart] = pDkPart;

								NrDkPart++;

								if(IsFaultTolerantSet(pDkPart->Type))
								{
									nRet = AddFaultTolerantMembers(
											pDkPart, 
											NrPhysDrives, 
											Layouts, 
											pDiskKey,
											&NrPart,
											&NrDkPart,		/*	output	*/
											PhysDrive,
											PhysDriveSign,
											pInfo,
											pDkInfo
									);
								}
								else
									nRet = 0;
							}
							else
								nRet = 0;
						}
						else
						{
							STAMP;
							DbgPlain (DBG_DL, _T("No disk key is present in current system's registry."));
							nRet = 0;
						}
					}
					else
					{
						STAMP;
						DbgPlain (DBG_DL, _T("Could not find disk partition %d withing current disk layout."), nPart);
						nRet = -1;
					}
				}
				else
				{
					STAMP;
					DbgPlain (DBG_DL, _T("Current disk is >= to number of physical drives present on the system."));
				}
			}
			else
			{
				STAMP;
				DbgPlain (DBG_DL, _T("_stscanf failed. Cannot extract disk and partition numbers."));
			}
		}
		else
		{
			STAMP;
			DbgPlain (DBG_DL, _T("Cannot find Harddisk and Partition strings within NT native path: '%s'."), lpszNT);
		}
	}
	else
	{
		STAMP;
		DbgPlain (DBG_DL, _T("QueryDosDevice function failed for drive letter %s."), lpszDOS);
	}

	*NrPartitions = NrPart;

	if(NrPart > 0)
		nRet = 0;

	if (nRet == -1)
	{
		*NrPartitions = 0;
		FREE (*PhysDrive);		*PhysDrive = NULL;
		FREE (*PhysDriveSign);	*PhysDriveSign = NULL;
		FREE (*pInfo);			*pInfo = NULL;
		FREE (*pDkInfo);		*pDkInfo = NULL;
		STAMP;
		DbgPlain (DBG_DL, _T("DlFindDriveEngineDynamic FAILED for drive letter %c"), driveLetter);
	}

	RETURN_VALUE (nRet);
}

static int
DlFindDriveEngineNT4(
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*layouts, 
	PDkHeader				pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo
	)
{
	ERH_FUNCTION (_T("DlFindDriveEngine"));

	int						ret = -1;

	STAMP;
	DbgPlain (DBG_DL, _T("DlFindDriveEngine entered for drive %c"), driveLetter);

	if(_bDynamic != 0)
	{
		ret = DlFindDriveEngineDynamic(driveLetter, NrPhysDrives, layouts, pDiskKey,
				nonFixedDrives, NrPartitions, PhysDrive, PhysDriveSign, pInfo, pDkInfo);
		_bDynamic = 0;
	}
	else
	{
		ret = DlFindDriveEngineStatic(driveLetter, NrPhysDrives, layouts, pDiskKey,
				nonFixedDrives, NrPartitions, PhysDrive, PhysDriveSign, pInfo, pDkInfo);
	}

	RETURN_VALUE(ret);
}

#else /* _NEW_DRIVE_ENGINE_*/

int
DlGetAssignedLetters(PDkHeader pDiskKey, DWORD *pAssigned)
{
	ERH_FUNCTION (_T("DlGetAssignedLetters"));
	
	DWORD					nRet = 0, i = 0, j = 0, dkSize = 0;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	PDkPartitionHeader		dkPHdr = NULL;
	PDkPartitionInfo		dkPInfo = NULL;

	*pAssigned = 0x00000003;

	dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
	for (i = 0; i < dkDisks->NrDisks; i++)
	{
		dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
		if (dkDisk->NrPartitions > 0)
		{
			dkPHdr = (PDkPartitionHeader) 
				(char *) &dkDisk->PartitionHeader;
			for (j = 0; j < dkDisk->NrPartitions; j++)
			{
				dkPInfo = (PDkPartitionInfo) 
					((char *) &dkPHdr->Partitions +
						j * sizeof (DkPartitionInfo)
					);
				if(dkPInfo->DriveLetter)
				{
					*pAssigned |= (1 << (dkPInfo->DriveLetter-(CHAR)'A'));
					DbgPlain (DBG_DL, _T("DISK key disk: %d partition %d driveletter %c assigned."), i, j, dkPInfo->DriveLetter);
				}
			}
		}
		dkSize += sizeof (DkPartitionHeader);
		dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
	}
	RETURN_VALUE(nRet);
}

int 
DlGetBootVolume(DWORD NrPhysDrives, 
				PDrive_Layout *layouts, 
				PDkHeader pDiskKey, 
				PDkPartitionInfo *ppInfo,
				int *pPos)
{
	ERH_FUNCTION (_T("DlGetBootVolume"));
	
	DWORD					nRet = -1, i = 0, j = 0, dkSize = 0;
	int						nBootDisk = -1, nBootPart = -1;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	PDkPartitionHeader		dkPHdr = NULL;
	PDkPartitionInfo		dkPInfo = NULL;
	PPartition_Information	pi = NULL;

	STAMP;

	DbgPlain (DBG_DL, _T("Number of physical disks as found in drive layout:\t%d."), NrPhysDrives);

	for(i = 0; i < NrPhysDrives; i++)
	{
		for(j = 0; j < layouts[i]->PartitionCount; j++)
		{
			pi = DlPInformation (layouts[i], j);

			if(pi->PartitionType && pi->BootIndicator)
			{
				DbgPlain (DBG_DL, _T("Boot partition found on drive: %d, partition number: %d."), i, pi->PartitionNumber);
				nBootDisk = (int)i;
				nBootPart = (int)j;
			}
		}
	}

	if(nBootDisk >= 0 && nBootPart >= 0)
	{
		DbgPlain (DBG_DL, _T("Valid disk and partition entry."));
		*ppInfo = NULL;

		pi = &layouts[nBootDisk]->PartitionEntry[nBootPart];

		dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
    	DbgPlain (DBG_DL, _T("[%s] dkDisks->NrDisks = %lu"), __FCN__, dkDisks->NrDisks);
		if(NrPhysDrives == dkDisks->NrDisks)
		{
    		DbgPlain (DBG_DL, _T("[%s] NrPhysDisks[%d] == dkDisks->NrDisks[%d]"), __FCN__, 
				NrPhysDrives, dkDisks->NrDisks);

			for (i = 0; i < dkDisks->NrDisks; i++)
			{
				DbgPlain (DBG_DL, _T("[%s] Disk %d is a boot partition disk."), __FCN__, i);

				dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);

				if(i == (DWORD)nBootDisk)
				{
					if (dkDisk->NrPartitions > 0)
					{
						dkPHdr = (PDkPartitionHeader) 
							(char *) &dkDisk->PartitionHeader;

						for (j = 0; j < dkDisk->NrPartitions; j++)
						{
							DbgPlain (DBG_DL, _T("[%s] j = %lu"), __FCN__, j);
		
							dkPInfo = (PDkPartitionInfo) 
								((char *) &dkPHdr->Partitions +
									j * sizeof (DkPartitionInfo)
								);
							
							DbgPlain (DBG_DL, 
								_T("[%s] dkPInfo->DriveLetter = %c\n")
								_T("\tOffset:\tlow = %lu\thigh = %lu\n")
								_T("\tLength:\tlow = %lu\thigh = %lu"),
								__FCN__, dkPInfo->DriveLetter,
								dkPInfo->StartingOffset.LowPart, dkPInfo->StartingOffset.HighPart,
								dkPInfo->Length.LowPart, dkPInfo->Length.HighPart
								);

							if(!(*ppInfo))
							{
								if(memcmp(&dkPInfo->StartingOffset, &pi->StartingOffset, sizeof(LARGE_INTEGER)))
									continue;
								if(memcmp(&dkPInfo->Length, &pi->PartitionLength, sizeof(LARGE_INTEGER)))
									continue;
								
								*ppInfo = dkPInfo;
								*pPos = j;
								DbgPlain (DBG_DL, _T("Found boot partition entry in DISK key. Position %d."), *pPos);

								nRet = 0;
							}
						}
					}
					else
						DbgPlain (DBG_DL, _T("dkDisk->NrPartitions:\t\t%d."), dkDisk->NrPartitions);
				}
				else
					DbgPlain (DBG_DL, _T("Disk %d is not a boot partition disk."), i);
				
				dkSize += sizeof (DkPartitionHeader);
				dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
			}
		}
		else
			DbgPlain (DBG_DL, _T("FAILURE: NrPhysDisks(%d) != dkDisks->NrDisks(%d)"), NrPhysDrives, dkDisks->NrDisks);
	}
	else
		DbgPlain (DBG_DL, _T("No boot partition found. Something wrong!!!"));

	RETURN_VALUE(nRet);
}

int
DlGetVolumeInformation(PDkHeader pDiskKey, BOOL *pSets)
{
	ERH_FUNCTION (_T("DlGetVolumeInformation"));
	
	DWORD					i = 0, j = 0, dkSize = 0;
	int						nRet = 0;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	PDkPartitionHeader		dkPHdr = NULL;
	PDkPartitionInfo		dkPInfo = NULL;

	*pSets = FALSE;

	dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
	for (i = 0; i < dkDisks->NrDisks; i++)
	{
		dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
		if (dkDisk->NrPartitions > 0)
		{
			dkPHdr = (PDkPartitionHeader) 
				(char *) &dkDisk->PartitionHeader;
			for (j = 0; j < dkDisk->NrPartitions; j++)
			{
				dkPInfo = (PDkPartitionInfo) 
					((char *) &dkPHdr->Partitions +
						j * sizeof (DkPartitionInfo)
					);
				switch(dkPInfo->Type)
				{
					case NON_FT:	
					case WHOLE_DISK:
						break;
					default:
						*pSets = TRUE;
				}
			}
		}
		dkSize += sizeof (DkPartitionHeader);
		dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
	}

	RETURN_VALUE(nRet);
}

int
DlMarkVolumes(DWORD NrPhysDrives, PDrive_Layout *layouts, PDkHeader pDiskKey, DWORD dwNonFixed)
{
	ERH_FUNCTION (_T("DlMarkVolumes"));
	
	BOOL					bSets = FALSE;
	DWORD					i = 0, j = 0, dkSize = 0, dwAssigned = 0;
	int						nRet = -1, nBootPos = -1;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	PDkPartitionHeader		dkPHdr = NULL;
	PDkPartitionInfo		dkPInfo = NULL, dkPBoot = NULL;

	STAMP;
	DlGetVolumeInformation(pDiskKey, &bSets);

	if(!bSets)
	{
		DbgPlain (DBG_DL, _T("There are no *sets present in current disk layout"));
		if(!DlGetBootVolume(NrPhysDrives, layouts, pDiskKey, &dkPBoot, &nBootPos))
		{
			DbgPlain (DBG_DL, _T("Boot volume recognized. Position in DISK key: %d"), nBootPos);
			if(nBootPos != 0)
			{
				DlGetAssignedLetters(pDiskKey, &dwAssigned);

				dwAssigned |= dwNonFixed;
				DbgPlain (DBG_DL, _T("All currently assigned letters(fixed + nonfixed): %d"), dwAssigned);

				if(!dkPBoot->DriveLetter && !(dwAssigned & 0x00000004))
				{
					DbgPlain (DBG_DL, _T("Boot drive is not yet assigned a drive letter. Assigning it to C."), dwAssigned);
					dkPBoot->DriveLetter = (CHAR)'C';
					dwAssigned |= (1 << ((CHAR)'C' - (CHAR)'A'));
					DbgPlain (DBG_DL, _T("All currently assigned letters(fixed + nonfixed): %d"), dwAssigned);
				}

				dkDisks = (PDkDisks) &((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
				for (i = 0; i < dkDisks->NrDisks; i++)
				{
					dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
					if (dkDisk->NrPartitions > 0)
					{
						dkPHdr = (PDkPartitionHeader) 
							(char *) &dkDisk->PartitionHeader;
						for (j = 0; j < dkDisk->NrPartitions; j++)
						{
							dkPInfo = (PDkPartitionInfo) 
								((char *) &dkPHdr->Partitions +
									j * sizeof (DkPartitionInfo)
								);
							if(!dkPInfo->DriveLetter && dkPInfo->AssignDriveLetter)
							{
								DbgPlain (DBG_DL, _T("DISK key disk: %d partition: %d is not assigned a drive letter."), i, j);
								dkPInfo->DriveLetter = DlGetFreeDriveLetter(dwAssigned);
								DbgPlain (DBG_DL, _T("Assigning drive letter %c."), dkPInfo->DriveLetter);
								dwAssigned |= (1 << (dkPInfo->DriveLetter - (CHAR)'A'));
								DbgPlain (DBG_DL, _T("All currently assigned letters(fixed + nonfixed): %d"), dwAssigned);
							}
						}
					}
					dkSize += sizeof (DkPartitionHeader);
					dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
				}
			}
			nRet = 0;
		}
	}
	else
		nRet = 0;

	RETURN_VALUE (nRet);
}

/*========================================================================*//**
*
* @note      that other routines like DlFindLogicalDrive or
*            DlLogicalDrivesOnPhysDrive are mallocating space and coping
*            data from these pointers
*
*//*=========================================================================*/
int
DlFindDriveEngineNT4(
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*layouts, 
	PVOID					pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo
	WORD					wDkSig
	)
{
	ERH_FUNCTION (_T("DlFindDriveEngine"));

	int						ret = 0;
	USHORT					upperCase = (int) driveLetter;
	CHAR					dl = 0;
	DWORD					ii = 0, jj = 0, kk = 0;
	DWORD					drives = 0;
	DWORD					NrDkPart = 0, NrPart = 0;
	PDkDisks				dkDisks = NULL;
	PDkDiskInfo				dkDisk = 0;
	DWORD					dkSize = 0;
	PDkPartitionHeader		dkPHdr;
	PDkPartitionInfo		dkPInfo;
	PPartition_Information	part;
	BOOL					foundFakedPartition = FALSE;
	DWORD					dwTooLargePartitionNumber = 0xFFFFFFFF, 
							dwPartitionNumber = dwTooLargePartitionNumber;

	STAMP;
	driveLetter = (CHAR) toupper (upperCase);
	DbgPlain (DBG_DL, _T("DlFindDriveEngine entered for drive %c"), driveLetter);

	/* -----------------------------------------------------------------------
	|	now do the real job
	|	(i)		if disk key value exists, find drive letter passed in
	|			argument list, and store all partitions and physical drive 
	|			numbers with same drive letter
	|	(ii)	
	|		a)	disk key exists - compare partitions with information from disk
	|			key (physical drive number & offset & length) to find drive
	|		b)	disk key doesn't exist - do loop in partitions while argument
	|			drive letter is not reached
	 -----------------------------------------------------------------------*/
	drives = nonFixedDrives;
	if (pDiskKey)
	{
		DlMarkVolumes(NrPhysDrives, layouts, pDiskKey, nonFixedDrives);
		
		dkDisks = (PDkDisks) 
			&((char *) pDiskKey)[pDiskKey->DiskInformationOffset];
    	DbgPlain (DBG_DL, _T("[%s] dkDisks->NrDisks = %lu"), __FCN__, dkDisks->NrDisks);
		for (ii = 0; ii < dkDisks->NrDisks; ii++)
		{
			DbgPlain (DBG_DL, _T("[%s] ii = %lu"), __FCN__, ii);
			dkDisk = (PDkDiskInfo) ((char *) &dkDisks->Disks[0] + dkSize);
			if (dkDisk->NrPartitions > 0)
			{
				dkPHdr = (PDkPartitionHeader) 
					(char *) &dkDisk->PartitionHeader;
				DbgPlain (DBG_DL, 
					_T("[%s] dkDisk->NrPartitions = %lu (Signature = %lu)"), 
					__FCN__, dkDisk->NrPartitions, dkPHdr->Signature
					);
				for (jj = 0; jj < dkDisk->NrPartitions; jj++)
				{
					DbgPlain (DBG_DL, _T("[%s] jj = %lu"), __FCN__, jj);
					dkPInfo = (PDkPartitionInfo) 
						((char *) &dkPHdr->Partitions +
							jj * sizeof (DkPartitionInfo)
						);

					DbgPlain (DBG_DL, 
						_T("[%s] dkPInfo->DriveLetter = %c\n")
						_T("\tOffset:\tlow = %lu\thigh = %lu\n")
						_T("\tLength:\tlow = %lu\thigh = %lu"),
						__FCN__, dkPInfo->DriveLetter,
						dkPInfo->StartingOffset.LowPart, dkPInfo->StartingOffset.HighPart,
						dkPInfo->Length.LowPart, dkPInfo->Length.HighPart
						);
					dl = dkPInfo->DriveLetter ? 
						dkPInfo->DriveLetter : DlGetFreeDriveLetter (drives);

					if (dkPInfo->AssignDriveLetter)
						drives += (1 << (dl - 'A'));

					if (dl == driveLetter)
					{
						if (dwPartitionNumber == dwTooLargePartitionNumber)
						{
							dwPartitionNumber = dkPInfo->PartitionNumber;
						}

						*pDkInfo = (PDkPartitionInfo *) REALLOC (
							*pDkInfo, 
							(NrDkPart + 1) * sizeof (PDkPartitionInfo)
							);
						(*pDkInfo)[NrDkPart] = dkPInfo;

						*PhysDrive = (DWORD *) REALLOC (
							*PhysDrive, (NrDkPart + 1) * sizeof (DWORD)
							);
						(*PhysDrive)[NrDkPart] = ii;

						NrDkPart++;

						//if (!dkPInfo->PartitionNumber)
						//	foundFakedPartition = TRUE;
					}
				}
			}
			dkSize += sizeof (DkPartitionHeader);
			dkSize += dkDisk->NrPartitions * sizeof (DkPartitionInfo);
		}
	}

	/* -----------------------------------------------------------------------
	|	!!! NOTE !!!
	|
	|	There is really very strange behavior while working with partitions that
	|	are created on physical drive which has been offline and is turned on 
	|	in real time.
	|	In this case, partitions are created partitions are created on faked 
	|	disk geometry, which means that not original BIOS geometry has been
	|	used, but the disk geometry of the physical drive which was online in
	|	loading Windows NT operating system.
	|
	|	Each partition created on such a physical drive is represented in 
	|	Disk Key with two partition information entries. One is with assigned
	|	partition number, while other has partition number set to 0.
	|	Partition entry with partition number set to 0, points to physical 
	|	location of partition using faked disk geometry, while the other 
	|	one, with assigned partition number is using correct physical location, 
	|	using original disk geometry.
	|	
	|	Calling DeviceIoControl with IOCTL_GET_DRIVE_LAYOUT in this case
	|	RETURN_VALUEs layout with faked partition entries.
	|
	|	Was this the comment or was this the comment ?
	|
	 -----------------------------------------------------------------------*/
	if (foundFakedPartition)
	{
		if (NrDkPart == 2)
		{
			/* ---------------------------------------------------------------
			|	Drive is NOT Fault Tolerant (Volume, Stripe, Mirror Set)
			|	1st entry has assigned partition number, but should be ignored
			|	because IOCTL_GET_DRIVE_LAYOUT points to 
			 ---------------------------------------------------------------*/
			NrDkPart--;
			(*pDkInfo)[0] = (*pDkInfo)[1];
			(*pDkInfo)[1] = NULL;
		}
		else
		{
			for (ii = 0; ii < NrDkPart; ii++)
			{
				if ((*pDkInfo)[ii]->PartitionNumber)
				{
				}
			}
		}
	}
	
	drives = nonFixedDrives;
	for (ii = 0; ii < NrPhysDrives; ii++)
	{
		for (jj = 0; jj < layouts[ii]->PartitionCount; jj++)
		{
			part = DlPInformation (layouts[ii], jj);

			if (!part->RecognizedPartition || part->PartitionType == 18)
				continue;

			if (pDiskKey && kk < NrDkPart && 
				((
				  memcmp (&(*pDkInfo)[kk]->StartingOffset, &part->StartingOffset, sizeof (LARGE_INTEGER)) == 0 &&
				  memcmp (&(*pDkInfo)[kk]->Length, &part->PartitionLength, sizeof (LARGE_INTEGER)) == 0
				 ) ||
				 (
				  part->PartitionNumber == dwPartitionNumber/*(*pDkInfo)[kk]->PartitionNumber*/
				 )
				) &&
				(*PhysDrive)[kk] == ii
			   )
			{
				*pInfo = (PPartition_Information *) REALLOC (
					*pInfo, (NrPart + 1) * sizeof (PPartition_Information)
					);
				(*pInfo)[kk] = part;

				*PhysDriveSign = (DWORD *) REALLOC (
					*PhysDriveSign, (NrPart + 1) * sizeof (DWORD)
					);
				(*PhysDriveSign)[NrPart] = layouts[ii]->Signature;

				kk++;
				NrPart++;
			}
			else if (!pDiskKey)
			{
				dl = DlGetFreeDriveLetter (drives);
				drives += (1 << (dl - 'A'));
	DbgPlain (DBG_DL, _T("[DlFindDriveEngine] dl = %c, drives = %ul"), dl, drives);
				if (dl == driveLetter)
				{
					*pInfo = (PPartition_Information *) REALLOC (
						*pInfo, (NrPart + 1) * sizeof (PPartition_Information)
						);
					(*pInfo)[NrPart] = part;

					*PhysDrive = (DWORD *) REALLOC (
						*PhysDrive, (NrPart + 1) * sizeof (DWORD)
						);
					(*PhysDrive)[NrDkPart] = ii;

					*PhysDriveSign = (DWORD *) REALLOC (
						*PhysDriveSign, (NrPart + 1) * sizeof (DWORD)
						);
					(*PhysDriveSign)[NrDkPart] = layouts[ii]->Signature;

					kk++;
					NrPart++;
					break;
				}
			}
		}
	}

	if (pDiskKey)
	{
		if (NrDkPart != NrPart)
			ret = -1;
		else
			*NrPartitions = NrPart;
	}
	else
	{
		if (NrPart <= 0)
			ret = -1;
		else
			*NrPartitions = NrPart;
	}

	if (ret != -1 && *NrPartitions <= 0)
		ret = -1;

	if (ret == -1)
	{
		*NrPartitions = 0;
		FREE (*PhysDrive);		*PhysDrive = NULL;
		FREE (*PhysDriveSign);	*PhysDriveSign = NULL;
		FREE (*pInfo);			*pInfo = NULL;
		FREE (*pDkInfo);		*pDkInfo = NULL;
		DbgStamp (DBG_DL);
		DbgPlain (DBG_DL, _T("DlFindDriveEngine FAILED for drive letter %c"), driveLetter);
	}

	RETURN_VALUE (ret);
}

#endif /* _NEW_DRIVE_ENGINE_*/

int
DlFindDriveEngineW2K(
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*layouts, 
	PDkHeader				pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo
	)
{
	ERH_FUNCTION (_T("DlFindDriveEngine"));

	int			ret = 0;
	UINT		ii = 0;
	USHORT		upperCase = (USHORT)driveLetter;
	PVOLUME_INFO	lpVol = (PVOLUME_INFO)pDiskKey;

	STAMP;
	driveLetter = (CHAR) toupper (upperCase);
	DbgPlain (DBG_DL, _T("DlFindDriveEngine entered for drive %c"), driveLetter);

	*NrPartitions = 0;

	if(lpVol != NULL)
	{
		for(ii = 0; ii < lpVol->VolumeCount; ii++)
		{
			if(lpVol->Volumes[ii].DriveLetter == (ULONG)driveLetter)
			{
				ULONG	jj = 0;
				ULONG	ulDisk = lpVol->Volumes[ii].DiskNumber;
				PPartition_Information pPart = NULL;

				for(jj = 0; jj < layouts[ulDisk]->PartitionCount; jj++)
				{
					if(layouts[ulDisk]->PartitionEntry[jj].StartingOffset.QuadPart == 
						lpVol->Volumes[ii].PartInfo.StartingOffset.QuadPart)
					{
						pPart = &layouts[ulDisk]->PartitionEntry[jj];
						break;
					}
				}

				if(pPart != NULL)
				{
					*pInfo = (PPartition_Information*) REALLOC (
						*pInfo, (*NrPartitions + 1) * sizeof (PPartition_Information)
						);
					(*pInfo)[*NrPartitions] = pPart;

					*PhysDrive = (DWORD*)REALLOC (
						*PhysDrive, (*NrPartitions + 1) * sizeof (DWORD)
						);
					(*PhysDrive)[*NrPartitions] = lpVol->Volumes[ii].DiskNumber;

					*PhysDriveSign = (DWORD *) REALLOC (
						*PhysDriveSign, (*NrPartitions + 1) * sizeof (DWORD)
						);
					(*PhysDriveSign)[*NrPartitions] = lpVol->Volumes[ii].DiskSignature;

					*pDkInfo = (PDkPartitionInfo *) REALLOC (
						*pDkInfo, (*NrPartitions + 1) * sizeof (PDkPartitionInfo)
						);
					(*pDkInfo)[*NrPartitions] = &lpVol->Volumes[ii].PartInfo;

					*NrPartitions += 1;
				}
			}
		}
	}

	if(*NrPartitions <= 0)
		ret = -1;

	RETURN_VALUE (ret);
}

int
DlFindDriveEngine(
	CHAR					driveLetter,		/*	input	*/
	DWORD					NrPhysDrives,
	PDrive_Layout			*layouts, 
	PVOID					pDiskKey,
	DWORD					nonFixedDrives,
	DWORD					*NrPartitions,		/*	output	*/
	DWORD					**PhysDrive,
	DWORD					**PhysDriveSign,
	PPartition_Information	**pInfo,
	PDkPartitionInfo		**pDkInfo,
	WORD					wDkSig
	)
{
	int	iRet = -1;

	if(wDkSig != OB2_400_W2K_SIGNATURE)
		iRet = DlFindDriveEngineNT4(driveLetter, NrPhysDrives, layouts, 
			pDiskKey ? ((PDkCast)pDiskKey)->header : NULL, 
			nonFixedDrives, NrPartitions, PhysDrive, PhysDriveSign,
			pInfo, pDkInfo);
	else
		iRet = DlFindDriveEngineW2K(driveLetter, NrPhysDrives, layouts, 
			(PDkHeader)pDiskKey, nonFixedDrives, NrPartitions, PhysDrive, PhysDriveSign,
			pInfo, pDkInfo);

	return iRet;
}

/* ===========================================================================
|
|	FUNCTION	DlCvtPInfoToPEntry
|
 ========================================================================== */	

extern	PPartitionEntry
DlCvtPInfoToPEntry (PPartition_Information ppi, PDisk_Geometry pdg)
{
	ERH_FUNCTION (_T("DlCvtPInfoToPEntry"));

	static PartitionEntry	_PartitionEntry;
	LARGE_INTEGER			c;		/* Cylinders			*/
	DWORD					tpc;	/* TracksPerCylinder	*/
	DWORD					spt;	/* SectorsPerTrack		*/
	DWORD					bps;	/* BytesPerSector		*/
	LARGE_INTEGER			offset;
	LARGE_INTEGER			length;
	BYTE					bitPlace = 0;
	DWORD					ii, offsetInSectors, lengthInSectors;

	DBG_ENTER_FCN;

	memset (&_PartitionEntry, 0, sizeof (_PartitionEntry));
	if (ppi->PartitionType == 0)
		return (&_PartitionEntry);

	c = pdg->Cylinders;
	tpc = pdg->TracksPerCylinder;
	spt = pdg->SectorsPerTrack;
	bps = pdg->BytesPerSector;

	memcpy (&offset, &ppi->StartingOffset, sizeof (LARGE_INTEGER));
	memcpy (&length, &ppi->PartitionLength, sizeof (LARGE_INTEGER));

	for (bps = pdg->BytesPerSector; bps != 1; bps >>= 1, bitPlace++);
/* VS 2008 MIGRATION
#if (TARGET_WIN64)
*/
	offset.QuadPart >>= bitPlace;
	length.QuadPart >>= bitPlace;
	/*
#else
	offset = LargeIntegerShiftRight (offset, bitPlace);
	length = LargeIntegerShiftRight (length, bitPlace);
#endif /*TARGET_WIN64*/

	offsetInSectors = offset.LowPart;
	lengthInSectors = length.LowPart;
	_PartitionEntry.EndingSector = 1;
	for (ii = 1; ii < offsetInSectors + lengthInSectors; ii++)
	{
		if (ii == offsetInSectors + 1)
		{
			_PartitionEntry.StartingCylinder = _PartitionEntry.EndingCylinder;
			_PartitionEntry.StartingHead = _PartitionEntry.EndingHead;
			_PartitionEntry.StartingSector = _PartitionEntry.EndingSector;
		}

		_PartitionEntry.EndingSector++;
		if (_PartitionEntry.EndingSector > spt)
		{
			_PartitionEntry.EndingSector = 1;
			_PartitionEntry.EndingHead++;
			if (_PartitionEntry.EndingHead >= tpc)
			{
				_PartitionEntry.EndingHead = 0;
				_PartitionEntry.EndingCylinder++;
			}
		}
	}

	_PartitionEntry.BootIndicator = ppi->BootIndicator;
	_PartitionEntry.SystemId = ppi->PartitionType;
	_PartitionEntry.RelativeSector = offsetInSectors;
	_PartitionEntry.TotalSectors = lengthInSectors;

	return (&_PartitionEntry);
}

/* ===========================================================================
|
|	FUNCTION	DlCvtPInfoToPEntry
|
 ========================================================================== */	

extern	PPartition_Information
DlCvtPEntryToPInfo (PPartitionEntry ppe, PDisk_Geometry pdg)
{
	ERH_FUNCTION (_T("DlCvtPEntryToPInfo"));

	static Partition_Information		_PartitionInformation;
	LARGE_INTEGER					c;		/* Cylinders			*/
	DWORD							tpc;	/* TracksPerCylinder	*/
	DWORD							spt;	/* SectorsPerTrack		*/
	DWORD							bps;	/* BytesPerSector		*/
	LARGE_INTEGER					offset = {0};
	LARGE_INTEGER					length = {0};
	BYTE							bitPlace = 0;

	DBG_ENTER_FCN;

	memset (&_PartitionInformation, 0, sizeof (Partition_Information));
	if (ppe->SystemId == 0)
		return (&_PartitionInformation);

	c = pdg->Cylinders;
	tpc = pdg->TracksPerCylinder;
	spt = pdg->SectorsPerTrack;
	bps = pdg->BytesPerSector;

	for (bps = pdg->BytesPerSector; bps != 1; bps >>= 1, bitPlace++);

	offset.LowPart = ppe->RelativeSector;


/* VS 2008 MIGRATION 
#if (TARGET_WIN64)
*/
	_PartitionInformation.StartingOffset.QuadPart = offset.QuadPart>>bitPlace;
/*
#else
	_PartitionInformation.StartingOffset = LargeIntegerShiftLeft (offset, bitPlace);
#endif /*TARGET_WIN64*/
	_PartitionInformation.HiddenSectors = ppe->RelativeSector;

	length.LowPart = ppe->TotalSectors;
/*
#if (TARGET_WIN64)
*/
	_PartitionInformation.PartitionLength.QuadPart = length.QuadPart>>bitPlace;
/*
#else
	_PartitionInformation.PartitionLength = LargeIntegerShiftLeft (length, bitPlace);
#endif /*TARGET_WIN64*/

	_PartitionInformation.BootIndicator = ppe->BootIndicator;
	_PartitionInformation.PartitionType = ppe->SystemId;

	return (&_PartitionInformation);
}

/* ===========================================================================
|
|	FUNCTION	DlReadPartitionEntry
|
 ========================================================================== */	

PPartitionEntry
DlReadPartitionEntry (char *peBuffer)
{
	ERH_FUNCTION (_T("DlReadPartitionEntry"));

	static	PartitionEntry	_partition;

	DBG_ENTER_FCN;

	/*	boot indicator		*/
	_partition.BootIndicator = (((BYTE) peBuffer[0] & 0xFF) == 0x80);

	/*	starting head		*/
	_partition.StartingHead = peBuffer[1];

	/*	starting sector		*/
	_partition.StartingSector = peBuffer[2] & 0x3F;

	/*	starting cylinder	*/
	_partition.StartingCylinder = 
		(((WORD) (peBuffer[2] & 0xC0))<<2) | 
		((WORD) peBuffer[3]) & 
		0xFF;

	/*	system ID			*/
	_partition.SystemId = peBuffer[4];

	/*	ending head			*/
	_partition.EndingHead = peBuffer[5];

	/*	endingSector		*/
	_partition.EndingSector = peBuffer[6] & 0x3F;

	/*	ending cylinder		*/
	_partition.EndingCylinder = 
		(((WORD) (peBuffer[6] & 0xC0))<<2) | 
		((WORD) peBuffer[7]) & 
		0xFF;

	/*	relative sector		*/
	memcpy (&_partition.RelativeSector, &peBuffer[8], sizeof (DWORD));

	/*	total sectors		*/
	memcpy (&_partition.TotalSectors, &peBuffer[12], sizeof (DWORD));


	return (&_partition);
}

/* ===========================================================================
|
|	FUNCTION	DlDumpPartitionEntry
|
 ========================================================================== */	

#define	LowSixBits	0x3F
#define	HighTwoBits	0x30
#define	AllBits		0xFF

BYTE *
DlDumpPartitionEntry (PPartitionEntry _PartitionEntry)
{
	ERH_FUNCTION (_T("DlDumpPartitionEntry"));

	static	BYTE	_DumpBuffer[PARTITION_ENTRY_SIZE];

	DBG_ENTER_FCN;

	memset (_DumpBuffer, 0, PARTITION_ENTRY_SIZE);

	if (!_PartitionEntry->SystemId)
		return (_DumpBuffer);

	/*	boot indicator		*/
	_DumpBuffer[0] = _PartitionEntry->BootIndicator ? 0x80 : 0;

	/*	starting head		*/
	_DumpBuffer[1] = _PartitionEntry->StartingHead;

	/*	starting sector		*/
	_DumpBuffer[2] = _PartitionEntry->StartingSector & LowSixBits;

	/*	starting cylinder	*/
	_DumpBuffer[2] |= ((_PartitionEntry->StartingCylinder & 0x0300) >> 2)/* & HighTwoBits*/;
	_DumpBuffer[3] = _PartitionEntry->StartingCylinder & (WORD) 0xFF;

	/*	system ID			*/
	_DumpBuffer[4] = _PartitionEntry->SystemId;

	/*	ending head			*/
	_DumpBuffer[5] = _PartitionEntry->EndingHead;

	/*	endingSector		*/
	_DumpBuffer[6] = _PartitionEntry->EndingSector & LowSixBits;

	/*	ending cylinder		*/
	_DumpBuffer[6] |= ((_PartitionEntry->EndingCylinder & 0x0300) >> 2)/* & HighTwoBits*/;
	_DumpBuffer[7] = _PartitionEntry->EndingCylinder & (WORD) 0xFF;

	/*	relative sector		*/
	memcpy (&_DumpBuffer[8], &_PartitionEntry->RelativeSector, sizeof (DWORD));

	/*	total sectors		*/
	memcpy (&_DumpBuffer[12], &_PartitionEntry->TotalSectors, sizeof (DWORD));


	return (_DumpBuffer);
}

/* ===========================================================================
|
|	FUNCTION	DlCreateDiskKey
|
 ========================================================================== */	

extern int
DlCreateDiskKey (
	DWORD			NrDrives, 
	PDrive_Layout	*layouts, 
	PDkHeader		*pDiskKey, 
	DWORD			*diskKeySz
	)
{
	ERH_FUNCTION (_T("DlCreateDiskKey"));

	int						ret = 0;
	DWORD					ii, jj, kk, sz = 0, diskSz = 0;
	BYTE					*aptr = NULL;
	PDkHeader				pdkh = NULL;
	DWORD					dkHeaderSz = sizeof (DkHeader);
	DWORD					dkDisksSz = sizeof (DkDisks);
	DWORD					dkDiskInfoSz = sizeof (DkDiskInfo);
	DWORD					dkPartHdrSz = sizeof (DkPartitionHeader);
	DWORD					dkPartInfoSz = sizeof (DkPartitionInfo);
	DWORD					pvoidSz = sizeof (PVOID);
	PPartition_Information	ppi;
	PDkDisks				disks;
	PDkDiskInfo				pdi;
	PDkPartitionHeader		pph;
	PDkPartitionInfo		pdki;
	CHAR					driveLetter;
	/* DWORD					fixedDrives = DlGetDrives (FIXED_DRIVES); */
	DWORD					nonFixedDrives = DlGetDrives (NON_FIXED_DRIVES);
	DWORD					drives = nonFixedDrives;

	DBG_ENTER_FCN;

	/* -----------------------------------------------------------------------
	|	!!! NOTE !!!
	|
	|	DlCreateDiskKey DOES NOT create any entries for fault tolerant
	|	drives (Ft)
	|	For creating entry for fault tolerant drives, argument list should
	|	extended with some additional informations on how you want to
	|	organize Ft volumes, what can be rather complicated
	|
	 -----------------------------------------------------------------------*/

	/* -----------------------------------------------------------------------
	|	1)	calculate size of disk key, and allocate buffer for it
	|		this is nesseccary beucase of reallocations (there is no way
	|		to remap reallocated pointers)
	 -----------------------------------------------------------------------*/
	sz += dkHeaderSz;
	sz += dkDisksSz - dkDiskInfoSz;
	for (ii = 0; ii < NrDrives; ii++)
	{
		sz += dkDiskInfoSz - pvoidSz;
		sz += dkPartHdrSz - pvoidSz;

		for (jj = 0, kk = 0; jj < layouts[ii]->PartitionCount; jj++)
		{
			ppi = (PPartition_Information) (
				(BYTE *) layouts[ii]->PartitionEntry +
				jj * sizeof (Partition_Information)
				);
			if (ppi->RecognizedPartition)
				kk++;
		}

		if (kk > 0)
			sz += kk * dkPartInfoSz;
	}
	aptr = (BYTE *) MALLOC (sz);
	memset (aptr, 0, sz);

	pdkh = (PDkHeader) aptr;

	/* -----------------------------------------------------------------------
	|	2)	set values to disk key
	|		- values are read from layouts
	 -----------------------------------------------------------------------*/
	pdkh->Version = 0x03;
	pdkh->DiskInformationOffset = dkHeaderSz;
	pdkh->DiskInformationSize = sz - dkHeaderSz;
	pdkh->FtInformationOffset = sz;
	disks = (PDkDisks) &aptr[dkHeaderSz];
	disks->NrDisks = (USHORT) NrDrives;
	for (ii = 0, diskSz = 0; ii < NrDrives; ii++)
	{
		pdi = (PDkDiskInfo) ((BYTE *) disks->Disks + diskSz);
		pph = (PDkPartitionHeader) &pdi->PartitionHeader;
		pph->Signature = layouts[ii]->Signature;
		for (jj = 0, kk = 0; jj < layouts[ii]->PartitionCount; jj++)
		{
			ppi = (PPartition_Information) (
				(BYTE *) layouts[ii]->PartitionEntry +
				jj * sizeof (Partition_Information)
				);
			if (!ppi->RecognizedPartition)
				continue;

			driveLetter = DlGetFreeDriveLetter (drives);
			drives |= (1 << (driveLetter - 'A'));

			pdki = (PDkPartitionInfo) (
				(BYTE *) (&pph->Partitions) + kk * dkPartInfoSz
				);

			pdki->Type = PT_NON_FT;
			memcpy (
				&pdki->StartingOffset, &ppi->StartingOffset, 
				sizeof (LARGE_INTEGER)
				);
			memcpy (
				&pdki->Length, &ppi->PartitionLength, 
				sizeof (LARGE_INTEGER)
				);
			pdki->DriveLetter = driveLetter;
			pdki->AssignDriveLetter = 1;
			pdki->PartitionNumber = 1;
			pdki->FtGroup = 0xFFFF;
			pdki->Modified = 1;

			kk++;
		}
		pdi->NrPartitions = (USHORT) kk;

		diskSz = dkDiskInfoSz + dkPartHdrSz - 2 * pvoidSz + kk * dkPartInfoSz;
	}

	*pDiskKey = (PDkHeader) aptr;
	*diskKeySz = sz;
	
	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION	DlCheckLocation
|
 ========================================================================== */	

PiLocation
DlCheckExtendedLocation (
	PPartition_Information	partInfo,
	PDisk_Geometry			geometry,
	PDrive_Layout			layout,
	DWORD					*chainCount,
	PPartition_Information	**chain
	)
{
	ERH_FUNCTION (_T("DlCheckExtendedLocation"));

	PiLocation				ret = PI_LOCATION_NOTFOUND;
	DWORD					ii;
	DWORD					PartTableSize = 4;
	PPartition_Information	pi, epi;
	BOOL					gotExtended = FALSE;

	DBG_ENTER_FCN;

	*chain = NULL;
	*chainCount = 0;

	/* -----------------------------------------------------------------------
	|	DriveLayout is variable length structure with informations about
	|	physical partitions.
	|	Since organization of DriveLayout's PartitionEntry table is similar 
	|	to DOS partition tables (always 4 partition entries, no matter if
	|	thay are used or not) it is not visible from one single partition 
	|	entry if it has been created as primary partition or on extended
	|	partition.
	|	This routine is responsible to figure out location (type) of 
	|	partition entered as partInfo
	|
	|	Partition Table (in MBR or PBS)
	|	|	16b		|	16b		|	16b		|	16b		|
	|
	|	If we find partition extended partition in MBR then we have to seek
	|	to startingOffset of that partition, and read PBS to get extended
	|	partition's partition table.
	|	If extended partition has more then 1 entry, then 2nd entry points
	|	to actual location of that partition.
	 -----------------------------------------------------------------------*/
	for (ii = 0; ii < PartTableSize; ii++)
	{
		pi = DlPInformation (layout, ii);

		if (memcmp (partInfo, pi, sizeof (Partition_Information)) == 0)
		{
			/* ---------------------------------------------------------------
			|	nice, this partition is not part of extended partition
			 ---------------------------------------------------------------*/
			if (*chain)	FREE (*chain); *chain = NULL;
			*chainCount = 0;
			RETURN_VALUE (PI_LOCATION_PRIMARY);
		}
		else if (IsContainerPartition (pi->PartitionType))
		{
			*chain = (PPartition_Information *) MALLOC (
				sizeof (PPartition_Information)
				);
			(*chain)[*chainCount] = pi;
			(*chainCount)++;

			gotExtended = TRUE;
		}
	}

	/* -----------------------------------------------------------------------
	|	Oooo, this is going to be boring ...
	 -----------------------------------------------------------------------*/
	if (!gotExtended)
	{
		DbgPlain (DBG_DL, 
			_T("Nor Extended partition or requested partition found\n")
			);
		RETURN_VALUE (ret);
	}

	/* -----------------------------------------------------------------------
	|	find  if partInfo is in extended partition, otherwise it's error
   	 -----------------------------------------------------------------------*/
	for (ii = PartTableSize; ii < layout->PartitionCount; ii += PartTableSize)
	{
		pi = DlPInformation (layout, ii);
		epi = DlPInformation (layout, ii + 1);

		if (memcmp (pi, partInfo, sizeof (Partition_Information)) == 0)
		{
			ret = PI_LOCATION_EXTENDED;
			break;
		}
		else if (IsContainerPartition (pi->PartitionType))
		{
			*chain = (PPartition_Information *) REALLOC (
				*chain,	(*chainCount + 1) * sizeof (PPartition_Information)
				);
			(*chain)[*chainCount] = pi;
			(*chainCount)++;
		}
		else if (IsContainerPartition (epi->PartitionType))
		{
			*chain = (PPartition_Information *) REALLOC (
				*chain,	(*chainCount + 1) * sizeof (PPartition_Information)
				);
			(*chain)[*chainCount] = epi;
			(*chainCount)++;
		}
		else
		{
			break;
		}
	}

	if (ret == PI_LOCATION_NOTFOUND)
	{
		DbgPlain (DBG_DL, _T("Partition not found in given drive layout"));
		if (*chain)	FREE (*chain);	*chain = NULL;
		*chainCount = 0;
	}
	
	RETURN_VALUE (ret);
}

/* ---------------------------------------------------------------------------
|
|	ATENTION !
|
|	Emir Hodzic (hodza@hermes.si)
|
|	Release:		OBII 3.1
|	Module:			EISA Utility Partition recognition/mounting
|	Desrription:	
|		EISA Utility Partition (found on HP NetServer and Compaq ProLine Server
|		is partition which is not visible on Windows NT systems. However, such 
|		partitions are DOS formatted partition and can be seen if partition type 
|		is changed and mount as normal filesystem.
|		Listed below are routines which are responsible for detecting and mounting
|		mentioned partitions so that OBII can backup them.
|
 ---------------------------------------------------------------------------*/

/*========================================================================*//**
*
* @ingroup   Dl
*
*//*=========================================================================*/
BOOL
DlUtilityPartitionExists (DWORD *physicalDriveNumber, PPartition_Information pUtilPartition)
{
	ERH_FUNCTION (_T("DlUtilityPartitionExists"));

	BOOL			ret = FALSE;
	DWORD			physDriveCntr = 0;
	DWORD			partCntr = 0;
	HANDLE			noHandle = INVALID_HANDLE_VALUE, hPhysDrive = noHandle;
	PDrive_Layout	pLayout = NULL;

	DBG_ENTER_FCN;

	/* ---------------------------------------------------------------------------
	|	Loop through physical drives and check if any of the partition has 
	|	PartitionType 0x12 (18 dec)
	 ---------------------------------------------------------------------------*/
	do 
	{
		hPhysDrive = DlOpenPhysicalDrive (physDriveCntr);
		if (hPhysDrive == noHandle)
		{
			if (GetLastError()==ERROR_FILE_NOT_FOUND)
			{
				DbgStamp (DBG_DL);
				DbgPlain (DBG_DL, _T("No more physical drives"));
				ErhClearError ();
				break;
			}
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, _T("Cannot open physical drive #%lu"), physDriveCntr);
			continue;
		}

		if (DlGetPhysicalDriveLayout (hPhysDrive, &pLayout) != -1)
		{
			for (partCntr = 0; !ret && partCntr < pLayout->PartitionCount; partCntr++)
			{
				Partition_Information	pi = *(DlPInformation (pLayout, partCntr));

				if (IsEisaUtilityPartition (pi.PartitionType))
				{
					DbgStamp (DBG_DL);
					DbgPlain (DBG_DL, 
						_T("Found EISA Utility Partition on physical drive #%lu:\n")
						_T("\toffset:      low %lu high %lu\n")
						_T("\tlength:      low %lu high %lu\n")
						_T("\ttype:        %d\n"),
						physDriveCntr,
						pi.StartingOffset.LowPart,
						pi.StartingOffset.HighPart,
						pi.PartitionLength.LowPart,
						pi.PartitionLength.HighPart,
						pi.PartitionType
						);
					if (physicalDriveNumber)
						*physicalDriveNumber = physDriveCntr;
					if (pUtilPartition)
						*pUtilPartition = pi;
					ret = TRUE;
				}
			}

			FREE (pLayout);
		}

		CloseHandle (hPhysDrive);
		physDriveCntr++;

	} while (hPhysDrive != noHandle && !ret);

	RETURN_VALUE (ret);
}

/*========================================================================*//**
*
* @ingroup   Dl
*
*//*=========================================================================*/
int
DlMountUtilityPartition (
	DWORD					physicalDriveNumber, 
	PPartition_Information	ppiUP,
	CHAR					*driveLetter
	)
{
	ERH_FUNCTION (_T("DlMountUtilityPartition"));

	int							ret = -1;
	HANDLE						hNoHandle = INVALID_HANDLE_VALUE, 
								hPhysDrive = hNoHandle, 
								hNewVolume = hNoHandle;
	DWORD						dwLogicalDrives = DlGetDrives (FIXED_DRIVES) | DlGetDrives (NON_FIXED_DRIVES);
	CHAR						freeDriveLetter = 0;
	tchar						nameSpace[STRLEN_STD], 
								symLink[16],
								newVolPath[STRLEN_STD];
	PDrive_Layout				pLayout = NULL;

	DBG_ENTER_FCN;

	__try
	{
		/* -------------------------------------------------------------------
		|	0)	Open physical drive
		 -------------------------------------------------------------------*/
		DbgPlain (DBG_DL, _T("Opening physical drive #%lu"), physicalDriveNumber);
		hPhysDrive = DlOpenPhysicalDrive (physicalDriveNumber);
		if (hPhysDrive == INVALID_HANDLE_VALUE)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Unable to open physical drive #%lu\n[SYSTEM: %d]"), 
				physicalDriveNumber, GetLastError  ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#if 0
		/* -------------------------------------------------------------------
		|	1)	Set partition type to FAT12
		 -------------------------------------------------------------------*/
		if (DlGetPhysicalDriveLayout (hPhysDrive, &pLayout) == -1)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Unable to get layout for physical drive #%lu\n[SYSTEM: %d]"), 
				physicalDriveNumber, GetLastError  ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}

		for (dwPartCntr = 0; dwPartCntr < pLayout->PartitionCount; dwPartCntr++)
		{
			if ((DlPInformation (pLayout, dwPartCntr))->PartitionNumber == ppiUP->PartitionNumber)
			{
				pUtilityPartition = DlPInformation (pLayout, dwPartCntr);
				pUtilityPartition->PartitionType = PARTITION_FAT_12;
			}
		}
		if (!DeviceIoControl (
				hPhysDrive, 
				IOCTL_DISK_SET_DRIVE_LAYOUT, 
				pLayout,
				DriveLayoutSize (pLayout),
				NULL, 0,
				&dwBytesReturned,
				NULL
				)
			)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Unable to set drive layout/partition type to FAT12 partition !\n[SYSTEM: %d]"), 
				GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#endif
		/* -------------------------------------------------------------------
		|	2.1)	Get drive letter for new partition
		 -------------------------------------------------------------------*/
		freeDriveLetter = DlGetFreeDriveLetter (dwLogicalDrives);

		/* -------------------------------------------------------------------
		|	2.2)	Create symbolic link for new partition
		 -------------------------------------------------------------------*/
		sprintf (nameSpace, 
			_T("\\Device\\Harddisk%d\\Partition%d"), 
			physicalDriveNumber, ppiUP->PartitionNumber
			);
		sprintf (symLink, _T("%c:"), freeDriveLetter);
		if (!DefineDosDevice (DDD_RAW_TARGET_PATH, symLink, nameSpace))
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Failed to create symbolic link %s to name space %s\n[SYSTEM: %d]"),
				symLink, nameSpace, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#if 0
		/* -------------------------------------------------------------------
		|	3)	Broadcast message that new baby is in town to system
		 -------------------------------------------------------------------*/
		devBcVolume.dbcv_size = sizeof (devBcVolume);
		devBcVolume.dbcv_devicetype = DBT_DEVTYP_VOLUME;
		devBcVolume.dbcv_unitmask = (DWORD) (1 << (freeDriveLetter - 'A'));
		lBcResult = BroadcastSystemMessage (
			BSF_QUERY,//BSF_POSTMESSAGE,
			&dwBcMsgRecipients,
			WM_DEVICECHANGE,
			DBT_DEVICEARRIVAL,
			(DWORD) &devBcVolume
			);
		if (lBcResult <= 0)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Failed to broadcast message about new volume %c:\n[SYSTEM: %d]"),
				freeDriveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}

		/* -------------------------------------------------------------------
		|	4)	Set partition type back to Utility Partition
		 -------------------------------------------------------------------*/
		pUtilityPartition->PartitionType = btOrigUpPartitionType;
		if (!DeviceIoControl (
				hPhysDrive, 
				IOCTL_DISK_SET_DRIVE_LAYOUT, 
				pLayout,
				DriveLayoutSize (pLayout),
				NULL, 0,
				&dwBytesReturned,
				NULL
				)
			)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Unable to set partition type back to utility partition !\n[SYSTEM: %d]"), 
				physicalDriveNumber, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#endif
		/* -------------------------------------------------------------------
		|	5)	Open new logical volume
		 -------------------------------------------------------------------*/
		sprintf (newVolPath, _T("\\\\.\\%c:"), freeDriveLetter);
		hNewVolume = CreateFile (
			newVolPath, 
			GENERIC_READ, 
			FILE_SHARE_READ, 
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL
			);
		if (hNewVolume == hNoHandle)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Cannot open logical drive %c: !\n[SYSTEM: %d]"), 
				freeDriveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#if 0
		/* -------------------------------------------------------------------
		|	6)	Lock opened logical volume
		 -------------------------------------------------------------------*/
		if (!DeviceIoControl (
				hNewVolume, 
				FSCTL_LOCK_VOLUME, 
				NULL, 0, 
				NULL, 0,
				&dwBytesReturned,
				NULL
				)
			)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Cannot lock logical drive %c: !\n[SYSTEM: %d]"), 
				freeDriveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#endif

		ret = 0;
		*driveLetter = freeDriveLetter;
	}
	__finally
	{
		if (hPhysDrive != hNoHandle)
			CloseHandle (hPhysDrive);
		if (hNewVolume != hNoHandle)
			CloseHandle (hNewVolume);
		if (pLayout)
			FREE (pLayout);
	}

	RETURN_VALUE (ret);
}

/*========================================================================*//**
*
* @ingroup   Dl
*
*//*=========================================================================*/
int
DlUnmountVolume (CHAR driveLetter)
{
	ERH_FUNCTION (_T("DlUnmountVolume"));

	int						ret = -1;
	tchar					symLink[16];
	HANDLE					hNoHandle = INVALID_HANDLE_VALUE, hVolume = hNoHandle;

	DBG_ENTER_FCN;

	__try
	{
#if 0
		sprintf (volPath, _T("\\\\.\\%c:"), driveLetter);
		DbgPlain (DBG_DL, _T("Opening volume path %s"), volPath);
		hVolume = CreateFile (
			volPath, 
			GENERIC_READ, 
			FILE_SHARE_READ, 
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL
			);
		if (hVolume == hNoHandle)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Cannot open logical drive %c: !\n[SYSTEM: %d]"), 
				driveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}

		if (!DeviceIoControl (
				hVolume, 
				FSCTL_UNLOCK_VOLUME,
				NULL, 0,
				NULL, 0, 
				&dwBytesReturned,
				NULL
				)
			)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Cannot unlock logical drive %c: !\n[SYSTEM: %d]"), 
				driveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}

		if (!DeviceIoControl (
				hVolume, 
				FSCTL_DISMOUNT_VOLUME,
				NULL, 0,
				NULL, 0, 
				&dwBytesReturned,
				NULL
				)
			)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Cannot dismount logical drive %c: !\n[SYSTEM: %d]"), 
				driveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#endif

		sprintf (symLink, _T("%c:"), driveLetter);
		if (!DefineDosDevice (DDD_REMOVE_DEFINITION, symLink, NULL))
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Failed to remove symbolic link %s\n[SYSTEM: %d]"),
				symLink, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#if 0
		devBcVolume.dbcv_size = sizeof (devBcVolume);
		devBcVolume.dbcv_devicetype = DBT_DEVTYP_VOLUME;
		devBcVolume.dbcv_unitmask = (DWORD) (1 << (driveLetter - 'A'));
		lBcResult = BroadcastSystemMessage (
			BSF_QUERY,//BSF_POSTMESSAGE,
			&dwBcMsgRecipients,
			WM_DEVICECHANGE,
			DBT_DEVICEQUERYREMOVE,
			(DWORD) &devBcVolume
			);
		if (lBcResult <= 0)
		{
			DbgStamp (DBG_DL);
			DbgPlain (DBG_DL, 
				_T("Failed to broadcast message about dismounted volume %c:\n[SYSTEM: %d]"),
				driveLetter, GetLastError ()
				);
			ErhMarkSys (ERH_SYSERR, GetLastError (), __FCN__, ERH_NORMAL);
			__leave;
		}
#endif
		ret = 0;
	}
	__finally
	{
		if (hVolume != hNoHandle)
			CloseHandle (hVolume);
	}

	RETURN_VALUE (ret);
}
