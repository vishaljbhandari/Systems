/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   srd
* @file      recovery/srd/srdutil.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     /
*
* @since     23.05.01    lukaj   Original Coding
*
* @remarks   /
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/srdutil.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|   include files
 ---------------------------------------------------------------------------*/
#include "lib/cmn/common.h"
#include "srd.h"
#include "srdcmn.h"
#include "srdutil.h"
#if TARGET_WIN32
    #include <initguid.h>
    #include <diskguid.h>
    #include <objbase.h>
#endif

/* ---------------------------------------------------------------------------
|   conditional compilation
 ---------------------------------------------------------------------------*/
#define PERFORM_REAL_WRITE                   1

#if TARGET_WIN32
R_RESULT
SrdDiskOpen(PHANDLE pOut, R_ULONG ulNum)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_TCHAR     pDrive[MAX_PATH];
    HANDLE      hDrive = INVALID_HANDLE_VALUE;

    sprintf(pDrive, _T ("\\\\.\\PHYSICALDRIVE%u"), ulNum);

    hDrive = CreateFile (
        pDrive,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );

    if(hDrive != INVALID_HANDLE_VALUE)
    {
        *pOut = hDrive;
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

R_VOID
SrdDiskClose(HANDLE hDisk)
{
    CloseHandle(hDisk);
}

R_RESULT
SrdVolumeOpen(PHANDLE pOut, R_ULONG ulDriveLetter)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_TCHAR     pDrive[MAX_PATH];
    HANDLE      hVolume = INVALID_HANDLE_VALUE;

    sprintf(pDrive, _T ("\\\\.\\%C:"), (R_TCHAR)ulDriveLetter);

    hVolume = CreateFile (
        pDrive,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );

    if(hVolume != INVALID_HANDLE_VALUE)
    {
        *pOut = hVolume;
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

/* ---------------------------------------------------------------------------
|   SRD open volume extended
---------------------------------------------------------------------------*/
R_RESULT
SrdVolumeOpenEx(PHANDLE pOut, R_TCHAR *volumeName)
{
    R_RESULT    returnVal = SRDERR_ERROR;
    R_TCHAR     pDrive[MAX_PATH];
    HANDLE      hVolume = INVALID_HANDLE_VALUE;

    /**
     * If Volume GUID
     */
    if (strstr(volumeName, _T("Volume{")) != NULL)
    {
        sprintf(pDrive, _T ("%s"), volumeName);
    }
    else
    {
        sprintf(pDrive, _T ("\\\\.\\%s:"), volumeName);
    }

    hVolume = CreateFile (
        pDrive,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );

    if (hVolume != INVALID_HANDLE_VALUE)
    {
        *pOut = hVolume;
        returnVal = SRDERR_SUCCESS;
    }

    return(returnVal);
}

R_VOID
SrdVolumeClose(HANDLE hVolume)
{
    CloseHandle(hVolume);
}

R_RESULT
SrdGetDiskGeometry(HANDLE hDisk, PDiskGeometry pGeo)
{
    R_RESULT        Return = SRDERR_ERROR;
    R_ULONG         ulBytes = 0;
    DISK_GEOMETRY   DiskGeo = { 0 };

    if (
        pGeo != NULL &&
        DeviceIoControl (hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeo, sizeof(DISK_GEOMETRY), &ulBytes, NULL)
    )
    {
        pGeo->Cylinders = DiskGeo.Cylinders.QuadPart;
        pGeo->MediaType = DiskGeo.MediaType;
        pGeo->TracksPerCylinder = DiskGeo.TracksPerCylinder;
        pGeo->SectorsPerTrack = DiskGeo.SectorsPerTrack;
        pGeo->BytesPerSector = DiskGeo.BytesPerSector;

        Return = SRDERR_SUCCESS;
    }

    return Return;
}
#endif
R_VOID
SrdFreeDiskLayout(PDriveLayout pLay)
{
    R_ULONG ii = 0;

    if(pLay != NULL)
    {
        for(ii = 0; ii < pLay->PartitionCount; ii++)
        {
            if(pLay->PartitionEntry[ii] != NULL)
                FREE(pLay->PartitionEntry[ii]);
        }

        FREE(pLay);
    }
}
#if TARGET_WIN32
#define BUFFER_SIZE     4096
R_RESULT
SrdGetDiskLayout(HANDLE hDisk, PDriveLayout *ppOut, R_ULONG* dynamic)
{
    R_RESULT    Return = SRDERR_ERROR;
    UCHAR *Buffer = NULL;
    int ret = 0;
    int bytes = BUFFER_SIZE;
    R_ULONG     ulBytes = 0;
    R_ULONG     platform = OS_OTHER;
    R_ULONG     isDynamic = 0;

    *ppOut = NULL;

    _TRY_
        Buffer = (UCHAR *)MALLOC(bytes);
        if(Buffer == NULL)
        {
            DbgPlain(10,_T("SrdGetDiskLayout: MALLOC failed. Cannot get disk layout."));
           _LEAVE_;
        }

        if(GetPlatformInfo(&platform, NULL, NULL) != SRDERR_SUCCESS)
        {
            DbgPlain(10, _T("GetPlatformInfo() failed. Cannot get disk layout."));
            _LEAVE_;
        }

        if (platform == OS_W2K)
        {
            R_ULONG ulSize = 0;
            PDRIVE_LAYOUT_INFORMATION pOrig = NULL;
            PDriveLayout pLay = NULL;

            do 
            {
                if(DeviceIoControl (hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, Buffer, bytes, &ulBytes, NULL))
                {
                    ulSize = DRIVE_LAYOUT_SIZE_W(Buffer);
                    pOrig = (PDRIVE_LAYOUT_INFORMATION)Buffer;
                    pLay = MALLOC(ulSize);

                    break;
                }

                ret = GetLastError();

                if(ret == ERROR_INSUFFICIENT_BUFFER)
                {
                    bytes += BUFFER_SIZE;
                    DbgPlain(10, _T("DeviceIOControl() failed (ERROR_INSUFFICIENT_BUFFER). %d. Trying to REALLOC %d bytes"), ret, bytes);
                    Buffer = (UCHAR *)REALLOC(Buffer, bytes);
                    if(Buffer == NULL)
                    {
                        DbgPlain(10,_T("SrdGetDiskLayout: REALLOC failed. Cannot get disk layout."));
                        _LEAVE_;
                    }
                }
                else
                {
                    DbgPlain(10, _T("DeviceIOControl() failed (Error: %d)."), ret);
                    _LEAVE_;
                }
            } while(ret == ERROR_INSUFFICIENT_BUFFER);

            if(pLay != NULL)
            {
                R_ULONG     ii = 0;

                memset(pLay, 0, ulSize);

                pLay->PartitionCount = pOrig->PartitionCount;
                sprintf(pLay->Signature, _T("%08X"), pOrig->Signature);

                for(ii = 0; ii < pOrig->PartitionCount; ii++)
                {
                    pLay->PartitionEntry[ii] = MALLOC(sizeof(PartitionInformation));

                    if(pLay->PartitionEntry[ii] == NULL)
                        break;

                    pLay->PartitionEntry[ii]->StartingOffset = pOrig->PartitionEntry[ii].StartingOffset.QuadPart;
                    pLay->PartitionEntry[ii]->PartitionLength = pOrig->PartitionEntry[ii].PartitionLength.QuadPart;
                    pLay->PartitionEntry[ii]->HiddenSectors = pOrig->PartitionEntry[ii].HiddenSectors;
                    pLay->PartitionEntry[ii]->PartitionNumber = pOrig->PartitionEntry[ii].PartitionNumber;
                    pLay->PartitionEntry[ii]->PartitionType = pOrig->PartitionEntry[ii].PartitionType;
                    pLay->PartitionEntry[ii]->BootIndicator = pOrig->PartitionEntry[ii].BootIndicator;
                    pLay->PartitionEntry[ii]->RecognizedPartition = pOrig->PartitionEntry[ii].RecognizedPartition;
                    pLay->PartitionEntry[ii]->RewritePartition = pOrig->PartitionEntry[ii].RewritePartition;
                }

                if(ii >= pOrig->PartitionCount)
                {
                    *ppOut = pLay;

                    Return = SRDERR_SUCCESS;
                }
                else
                    SrdFreeDiskLayout(pLay);
            }
        }
        else
        {
            R_ULONG ulSize = 0;
            PDRIVE_LAYOUT_INFORMATION_EX pOrig = NULL;
            PDriveLayout pLay = NULL;

            do 
            {
                if(DeviceIoControl (hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, Buffer, bytes, &ulBytes, NULL))
                {
                    ulSize = DRIVE_LAYOUT_SIZE_EX_W(Buffer);
                    pOrig = (PDRIVE_LAYOUT_INFORMATION_EX)Buffer;
                    pLay = MALLOC(ulSize);

                    break;
                }

                ret = GetLastError();

                if(ret == ERROR_INSUFFICIENT_BUFFER)
                {
                    bytes += BUFFER_SIZE;
                    DbgPlain(10, _T("DeviceIOControl() failed (ERROR_INSUFFICIENT_BUFFER). %d. Trying to REALLOC %d bytes"), ret, bytes);
                    Buffer = (UCHAR *)REALLOC(Buffer, bytes);
                    if (Buffer == NULL)
                    {
                        DbgPlain(10,_T("SrdGetDiskLayout: REALLOC failed. Cannot get disk layout."));
                        _LEAVE_;
                    }
                }
                else
                {
                        DbgPlain(10, _T("DeviceIOControl() failed (Error: %d)."), ret);
                        _LEAVE_;
                }
            } while(ret == ERROR_INSUFFICIENT_BUFFER);

            if(pLay != NULL)
            {
                R_ULONG ii = 0;

                memset(pLay, 0, ulSize);

                pLay->PartitionCount = pOrig->PartitionCount;
                if (pOrig->PartitionStyle == PARTITION_STYLE_MBR ||
                    pOrig->PartitionStyle == PARTITION_STYLE_RAW)
                {
                    sprintf(pLay->Signature, _T("%08X"), pOrig->Mbr.Signature);
                }
                else /* pOrig->PartitionStyle == PARTITION_STYLE_GPT */
                {
                    wchar_t str_guid[64];

                    HRESULT hr = StringFromGUID2(&pOrig->Gpt.DiskId, str_guid, 64);
                    if (FAILED(hr))
                    {
                        DbgPlain(70, _T("StringFromGUID2 failed. [%08X]."), hr);
                        return Return;
                    }
                    strcpy(pLay->Signature, W2T(str_guid));
                    strupr(pLay->Signature);
                }

                for(ii = 0; ii < pOrig->PartitionCount; ii++)
                {
                    pLay->PartitionEntry[ii] = MALLOC(sizeof(PartitionInformation));

                    if(pLay->PartitionEntry[ii] == NULL)
                        break;

                    pLay->PartitionEntry[ii]->StartingOffset = pOrig->PartitionEntry[ii].StartingOffset.QuadPart;
                    pLay->PartitionEntry[ii]->PartitionLength = pOrig->PartitionEntry[ii].PartitionLength.QuadPart;
                    pLay->PartitionEntry[ii]->PartitionNumber = pOrig->PartitionEntry[ii].PartitionNumber;
                    pLay->PartitionEntry[ii]->RewritePartition = pOrig->PartitionEntry[ii].RewritePartition;

                    switch(pOrig->PartitionEntry[ii].PartitionStyle)
                    {
                    case PARTITION_STYLE_MBR:
                        pLay->PartitionEntry[ii]->HiddenSectors = pOrig->PartitionEntry[ii].Mbr.HiddenSectors;
                        pLay->PartitionEntry[ii]->PartitionType = pOrig->PartitionEntry[ii].Mbr.PartitionType;
                        pLay->PartitionEntry[ii]->BootIndicator = pOrig->PartitionEntry[ii].Mbr.BootIndicator;
                        pLay->PartitionEntry[ii]->RecognizedPartition = pOrig->PartitionEntry[ii].Mbr.RecognizedPartition;
                        break;
                    case PARTITION_STYLE_GPT:
                        pLay->PartitionEntry[ii]->HiddenSectors = 0;
                        pLay->PartitionEntry[ii]->PartitionType = 0;
                        if (IsEqualGUID(&pOrig->PartitionEntry[ii].Gpt.PartitionType, &PARTITION_LDM_DATA_GUID) ||
                            IsEqualGUID(&pOrig->PartitionEntry[ii].Gpt.PartitionType, &PARTITION_LDM_METADATA_GUID))
                            pLay->PartitionEntry[ii]->PartitionType = PARTITION_LDM;
                        pLay->PartitionEntry[ii]->BootIndicator = 0;
                        pLay->PartitionEntry[ii]->RecognizedPartition = 0;
                        break;
                    default:
                        pLay->PartitionEntry[ii]->HiddenSectors = 0;
                        pLay->PartitionEntry[ii]->PartitionType = 0;
                        pLay->PartitionEntry[ii]->BootIndicator = 0;
                        pLay->PartitionEntry[ii]->RecognizedPartition = 0;
                        break;
                    }

                    if (isDynamic == 0 && pLay->PartitionEntry[ii]->PartitionType == PARTITION_LDM)
                    {
                        isDynamic = 1;
                        *dynamic = isDynamic;
                    }
                }

                if(ii >= pOrig->PartitionCount)
                {
                    *ppOut = pLay;

                    Return = SRDERR_SUCCESS;
                }
                else
                    SrdFreeDiskLayout(pLay);
            }
        }

    _FINALLY_
        if (Buffer != NULL)
            FREE(Buffer);
    _ENDTRY_

    return Return;
}

R_RESULT
SrdReadDiskSector(HANDLE hDisk, R_UINT64 Offset, R_ULONG SectorSize, R_UCHAR *pOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_ULONG     ulReadSize = SectorSize;
    R_ULONG     ulHighPart = HIGHPART64(Offset);

    if(SetFilePointer(hDisk, LOWPART64(Offset), &ulHighPart, FILE_BEGIN) != 0xffffffff)
    {
        R_ULONG ulBytes = 0;

        if(
            ReadFile(hDisk, pOut, ulReadSize, &ulBytes, NULL) == TRUE &&
            ulBytes == ulReadSize
        )
        {
            Return = SRDERR_SUCCESS;
        }
    }

    return Return;
}

R_RESULT
SrdWriteDiskSector(HANDLE hDisk, R_UINT64 Offset, R_ULONG SectorSize, R_UCHAR *pSector)
{
    R_RESULT    Return = SRDERR_ERROR;
#if PERFORM_REAL_WRITE
    R_ULONG     ulWriteSize = SectorSize;
    R_ULONG     ulHighPart = HIGHPART64(Offset);

    if(SetFilePointer(hDisk, LOWPART64(Offset), &ulHighPart, FILE_BEGIN) != 0xffffffff)
    {
        R_ULONG ulBytes = 0;

        DbgPlain(70, _T("ulHighPart = %lu."), ulHighPart);
        DbgPlain(70, _T("ulLowPart = %lu."), LOWPART64(Offset));
        DbgPlain(70, _T("ulWriteSize = %lu."), ulWriteSize);

        if(
            WriteFile(hDisk, pSector, ulWriteSize, &ulBytes, NULL) == TRUE &&
            ulBytes == ulWriteSize
        )
        {
            DbgPlain(70, _T("ulBytes = %lu."), ulBytes);
            Return = SRDERR_SUCCESS;
        }
    }
#else
    Return = SRDERR_SUCCESS;
#endif
    return Return;
}
#endif
