/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   DEVBRA - Device browser Agent
* @file      ma/devtool/devlist_nt.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     DEVBRA - Generates a table of all devices on the system.
*
* @since     01.dec.2000    Drago Fiser    Initial Coding
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /ma/devtool/devlist_nt.c $Rev$ $Date::                      $:") ;
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#   if TARGET_WIN64
#       include <wxp/NTDDSCSI.H>
#       include <wxp/devioctl.h>
#   else
#       include <NTDDSCSI.H>
#       include <devioctl.h>
#   endif
#include <winioctl.h>

#include "lib/cmn/common.h"
#include "ma/xma/ma.h"

#include "ma/spt/sctl.h"

#if TARGET_HW_TAPE_ENCRYPTION
#include <ma/spt/sctl_common.h>
#endif

#include "ma/xma/scsitab.h"
#include "devtool.h"
#include "devlist.h"
#include "devlist_file.h"
#include "devlist_nt.h"

#if TARGET_HW_TAPE_ENCRYPTION
#include "ma/spt/sctl_common.h"
extern void CheckDeviceEncryptionSupport(int filedes,
                                 PhysicalDevice* phd);
#endif

/* ===========================================================================*/
int DevSCTL_GetScsiAddress( tchar *name, SCSI_ADDRESS *add )
{
    ERH_FUNCTION(_T("devNT_GetScsiAddress"));
    tchar inname[MAX_PATH];
    int result = 0;
    DBGFCNINLOW();

    sprintf(inname, _T("\\\\.\\%s"), name);
    inname[strlen(inname)-1]=_T('\0');

    result = SCTL_QueryScsiAddress(inname, &add->PortNumber, &add->PathId, &add->TargetId, &add->Lun);

    DBGFCNOUTLOW_DEC(result);
    return (result);
}

/********************************************************************************
| DevSCTL_GetSystemInquiryData
**********************************************************************Drago*****/
#define SYSTEM_INQUIRY_ALLOC    1024    /* system uses 64 bytes per device */
#define SYSTEM_INQUIRY_REALLOC  512
#define MAXCOMPRESSRETRY	      10
/********************************************************************************/

UCHAR *DevSCTL_GetSystemInquiryData(int fd)
{
    ERH_FUNCTION(_T("GetSystemInquiryData"));
    BOOL status;
    ULONG returned;
    UCHAR *buffer;
    ULONG buffersize;

    DBGFCNINLOW();

    buffer = MALLOC(SYSTEM_INQUIRY_ALLOC);
    buffersize = SYSTEM_INQUIRY_ALLOC;
    memset(buffer, 0, SYSTEM_INQUIRY_ALLOC);

    do
    {
        status = DeviceIoControl(
                    SCTL_GetScsiHandle(fd),
                    IOCTL_SCSI_GET_INQUIRY_DATA,
                    NULL,
                    0,
                    buffer,
                    buffersize,
                    &returned,
                    FALSE
                );

        if (!status)
        {
            ULONG errorCode = 0;

            errorCode = GetLastError();

            if ((errorCode == ERROR_INSUFFICIENT_BUFFER) ||
                (errorCode == ERROR_MORE_DATA)
                )
            {
                DbgPlain(DBG_UNEXPECTED, __FCN_FMT__ _T("DeviceIOControl failed [error = %d] insufficient buffer (%d bytes). enlarging..."), __FCN__, errorCode, buffersize);
                buffer = REALLOC(buffer, buffersize + SYSTEM_INQUIRY_REALLOC);
                buffersize += SYSTEM_INQUIRY_REALLOC;
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, __FCN_FMT__ _T("DeviceIOControl failed [error = %d]"), __FCN__, errorCode);
                DBGFCNOUTLOW_DEC(0);
                return (0);
            }
        }
    }
    while (!status);

    DbgPlain(30, __FCN_FMT__ _T("Read %d bytes of inquiry data Information."), __FCN__, returned);

    DBGFCNOUTLOW();
    return (buffer);
}

/********************************************************************************
| GetDriveLetter
**********************************************************************Drago*****/
int GetDriveLetter( tchar *name, int Port, int Bus, int Target, int Lun )
{
    ERH_FUNCTION(_T("ScanDriveLetters"));
    tchar buf[256];
    DWORD buflen = 256;
    DWORD length;
    UINT driveType;
    tchar driveTypeString[64];
    static size_t i;
    DBGFCNINLOW();

    DbgPlain(30, __FCN_FMT__ _T("ScanDriveLetters(): Checking connected drives:"), __FCN__ );

    length = GetLogicalDriveStrings( buflen, buf );

    for (i=0; strlen(&buf[i])>0; i+=1+strlen(&buf[i]) )
    {
        driveType = GetDriveType(&buf[i]);
        switch (driveType)
        {
          case DRIVE_REMOVABLE:
            {
              SCSI_ADDRESS add;

              if (DevSCTL_GetScsiAddress(&buf[i], &add) == 1)
              {
                sprintf(driveTypeString, _T("Removable drive - scsi disk %d:%d:%d:%d"),
                    add.PortNumber, add.PathId, add.TargetId, add.Lun
                );

                if (Port==add.PortNumber)
                 if (Bus==add.PathId)
                  if (Target==add.TargetId)
                   if (Lun==add.Lun)
                   {
                        strcpy(name, &buf[i]);
                        name[StrLen(name)-1]=_T('\0');
                        DbgPlain(30, _T("Found searched drive: %s (%s) [at %d; length=%d] Type=%d - %s"),
                        &buf[i], name, i, strlen(&buf[i]), driveType, driveTypeString
                        );
                        DBGFCNOUTLOW_DEC(1);
                        return (1);
                   }
              }
              else
              {
                sprintf(driveTypeString, _T("Removable drive - non-scsi"));
              }
            break;
            }
          case DRIVE_FIXED:
            sprintf(driveTypeString, _T("Fixed drive"));
            break;
          case DRIVE_CDROM:
            sprintf(driveTypeString, _T("CD-ROM"));
            break;
          case DRIVE_REMOTE:
            sprintf(driveTypeString, _T("Network drive"));
            break;
          case DRIVE_RAMDISK:
            sprintf(driveTypeString, _T("Ramdisk"));
            break;
          case DRIVE_UNKNOWN:
          case DRIVE_NO_ROOT_DIR:
            sprintf(driveTypeString, _T("One heck of a messed up drive."));
            break;
          default:
            sprintf(driveTypeString, _T("Got no idea what this drive is."));
            break;
        }
        DbgPlain(30, _T("Found drive: %s [at %d; length=%d] Type=%d - %s"),
            &buf[i], i, strlen(&buf[i]),  driveType, driveTypeString
        );
    }

    DBGFCNOUTLOW_DEC(0);
    return (0);
}

/********************************************************************************
|
**********************************************************************Drago*****/
/********************************************************************************
| GetHardwarePath
**********************************************************************Drago*****/
int GetHardwarePath(tchar *path, int port, PSCSI_INQUIRY_DATA inquiryData)
{
    ERH_FUNCTION(_T("GetHardwarePath"));
    union inquiry_data      *inqData;
    DBGFCNINLOW();

    inqData = (union inquiry_data *)inquiryData->InquiryData;

    if (inquiryData->DeviceClaimed)
    {
        if (inqData->inq1.dev_type == SCSI_TYPE_OPTICAL)        // X:n:n:n:n for MO
        {
    //      GetDriveLetter(path, port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);
            tchar dl[8];
            if(!GetDriveLetter(dl, port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun))
            {
                DBGFCNOUTLOW_DEC(0);
                return (0);
            }
            sprintf(path, _T("%s%d:%d:%d:%d"), dl, port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);
        }
        else
        {
            tchar cardRegKey[128], deviceName[32];

            memset(deviceName, 0, 32*sizeof(tchar)); memset(cardRegKey, 0, 128*sizeof(tchar));
            sprintf(cardRegKey,
                _T("HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d\\"),
                port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);

            if (RegReadString( HKEY_LOCAL_MACHINE, cardRegKey, _T("DeviceName"), deviceName, 255) == 0)
            {
                sprintf(path, _T("%s:%d:%d:%d:%d"), deviceName, port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);
    //          sprintf(path, _T("%s"), deviceName);
            }
            else
            {  /*** error reporting in RegReadString ***/
                sprintf(path, _T("scsi%d:%d:%d:%d"), port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);
            }
        }

    }
    else
    {   /*** device not claimed ***/
        sprintf(path, _T("scsi%d:%d:%d:%d"), port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun);
    }

    DBGFCNOUTLOW_DEC(0);
    return (1);
}


int DBD_flag = 1;

/********************************************************************************
| create new entry for physical device
**********************************************************************Drago*****/
PhysicalDevice *CreatePhysicalDeviceEntry(int port, PSCSI_INQUIRY_DATA inquiryData)
{
    ERH_FUNCTION(_T("CreatePhysicalDeviceEntry"));
    union inquiry_data      *inqData = NULL;
    PhysicalDevice          *phd = NULL;
    int                     fd = -1;
    unsigned int            fullDevIdx = 0;
    DBGFCNINLOW();

    inqData = (union inquiry_data *)inquiryData->InquiryData;

    /*** check if its a device that omniback supports ***/
    switch (inqData->inq1.dev_type)
    {
    case SCSI_TYPE_TAPE:
    case SCSI_TYPE_OPTICAL:
    case SCSI_TYPE_CLT:
        break;

    default:
        {
            unsigned int idx;
            /*** find description in ScsiTypeTab ***/
            ScsiTypeTab_FINDINDEX(inqData->inq1.dev_type, idx);

            DbgPlain(30, __FCN_FMT__ _T("device type is %s and ignored."), __FCN__, ScsiTypeTab[idx].description);
            DBGFCNOUTLOW_TEXT(_T("NULL"));
            return (NULL);
        }
    }

    /*** allocate space for structure ***/
    phd = NewPhysicalDevice();

    /*** init ***/
    phd->flags = 0;

    /*** get scsi port, path, target and logical unit number ***/
    phd->scsi_port      = port;
    phd->scsi_path      = inquiryData->PathId;
    phd->scsi_target    = inquiryData->TargetId;
    phd->scsi_LUN       = inquiryData->Lun;

    /*** get vendorID, productID and revision ***/
    StrCopyA2T(phd->vendorID, inqData->inq1.vendor_id, 8); StrCutTrailingSpacesInPlace(phd->vendorID);
    StrCopyA2T(phd->productID, inqData->inq1.product_id, 16); StrCutTrailingSpacesInPlace(phd->productID);
    StrCopyA2T(phd->revision, inqData->inq1.rev_num, 4); StrCutTrailingSpacesInPlace(phd->revision);

    /*** get description and flags for the device from internal table ***/
    DEVICETABLE_SEARCHINDEXWITHTYPE(phd->productID, phd->vendorID,
                                    inqData->inq1.dev_type == SCSI_TYPE_CLT ? DEVTYPE_LIB: DEVTYPE_TAPE, fullDevIdx);
    if (deviceTable[fullDevIdx].devFlags & CAP_BARCODE)     phd->flags |= DEV_SUPPORT_BARCODE;
    //if (deviceTable[fullDevIdx].devFlags & CAP_NOEJECT)   phd->flags |= DEV_NOEJECT;
/*  if (deviceTable[fullDevIdx].devFlags & CAP_CLEANDETECT) phd->flags |= DEV_CLEAN_DETECTION; */
    /* add new lines for new flags */
/* CM removal ******************/
#if 0
    if (deviceTable[fullDevIdx].devFlags & CAP_CM)          phd->flags |= DEV_SUPPORT_CM;
#endif
    if(!((MAENV->dbd == 1) && (DBD_flag == 1)))
    {
        DBD_flag = 0;

        if(deviceTable[fullDevIdx].devFlags & CAP_DBD)
        {
            MAENV->dbd = 1;
        }
        else
        {
            MAENV->dbd = 0;
        }

        DbgPlain(30, __FCN_FMT__ _T("DBD bit set: %d"),__FCN__, MAENV->dbd);
    }


    /*** set device type ***/
    phd->devType = deviceTable[fullDevIdx].devType; if (phd->devType == DEV_UNKNOWN) phd->devType = DEV_TAPE;

    /*** check if device is claimed ***/
    if (inquiryData->DeviceClaimed)
        phd->flags |= DEV_CLAIMED;

    /*** add hostname ***/
    StrCpy(phd->host, cmnHostname);

    /*** get hardware path ***/
    if(!GetHardwarePath(phd->hwPath, port, inquiryData))
    {
        FREE(phd);
        DBGFCNOUTLOW_TEXT(_T("NULL"));
        return (NULL);
    }

    /*** open file handle for querying device, doesnt matter if fails ***/
    fd = CreateOB2Device(phd->hwPath, 0);
    if (fd < 0)
    {
        DbgPlain(30, __FCN_FMT__ _T("Could not open device! some information may be missing"), __FCN__);
        ErhClearError();
    }

    /*** get serial number ***/
    if (fd >= 0)
    {   DevbraQuerySerialNumber(fd, phd->serial_num);
    }

    /*** get type specific data ***/
    if (inqData->inq1.dev_type == SCSI_TYPE_CLT)
    {   /*** robotics ***/
        phd->flags                      |= DEV_DETECT_CONTROL;
        phd->devdesc.picker.interface   = SCSI_SPT;

        if (fd >= 0)
        {
            /*** get slots information ***/
            DevbraGetPickerElementStatus(fd, &(phd->devdesc.picker));

            /*** get drives information ***/
            GetPickersDrivesSerialNumbers(fd, phd);
        }
    }
    else
    { /*** drive ***/
        int r = -1;

        phd->flags                       |= DEV_DETECT_TAPE;
        phd->devdesc.drive.dfType        = 0;
        phd->devdesc.drive.dfSubType     = 0;
        phd->devdesc.drive.bcsConnection = 0;
        if (fd >= 0)
        {
            int i;

            if ((MAENV->compressRetry < 1) || (MAENV->compressRetry > MAXCOMPRESSRETRY))
            {
                DbgPlain(30, __FCN_FMT__ _T("Setting Compression retry to default value 1"), __FCN__);
                MAENV->compressRetry = 1;
            }
            for (i = 0; i < MAENV->compressRetry; i++)
            {
                r = SCTL_GetCompression(fd);
                if (r >= 0)
                {
                    DbgPlain(30, __FCN_FMT__ _T("Compression supported! Status: %s"), __FCN__, r ? _T("Enabled") : _T("Disabled"));
                    strcat(phd->hwPath, _T("C"));
                    break;
                }
                else if (r == -2)
                {
                    DbgPlain(30, __FCN_FMT__ _T("Compression not supported!"), __FCN__);
                    break;
                }
                else if (r == -1)
                {
                    DbgPlain(30, __FCN_FMT__ _T("Retry %d"), __FCN__, i);
                    Sleep(1000);
                }
           }
        }
        if (r < 0)
        {
            DbgPlain(30, __FCN_FMT__ _T("Compression not supported or recognized!"), __FCN__);
        }

#if TARGET_HW_TAPE_ENCRYPTION
        CheckDeviceEncryptionSupport(fd, phd);
#endif
    }

    /*** close file handle ***/
    if (fd >= 0)
        CloseOB2Device(fd);

    {
        unsigned int idx;
        /*** dump info ***/
        ScsiTypeTab_FINDINDEX(inqData->inq1.dev_type, idx);
        DbgPlain(30, __FCN_FMT__ _T("Info about device: type: %s; port: %d; path: %d; target: %d; lun: %d; path: \"%s\""), __FCN__,
            ScsiTypeTab[idx].description, phd->scsi_port, phd->scsi_path, phd->scsi_target, phd->scsi_LUN, phd->hwPath
        );
        DbgPlain(30, __FCN_FMT__ _T("Vendor: \"%s\"; product: \"%s\" revision: \"%s\" serial: \"%s\""), __FCN__,
            phd->vendorID, phd->productID, phd->revision, phd->serial_num
        );
        DevDescTab_FINDINDEX(deviceTable[fullDevIdx].devType, idx);
        DbgPlain(30, __FCN_FMT__ _T("(%d) devType=%d (%s), flags=%d, Vendor=\"%s\"; Marketing name =\"%s\""), __FCN__,
            idx, deviceTable[fullDevIdx].devType, devDescTab[idx].description, deviceTable[fullDevIdx].devFlags, deviceTable[fullDevIdx].VendorID, deviceTable[fullDevIdx].MarketingName
        );
        if (inqData->inq1.dev_type == SCSI_TYPE_CLT)
        {
            unsigned int i;
            DbgPlain(30, __FCN_FMT__ _T("S1 = %d; Sn = %d; D1 = %d; Dn = %d; T1 = %d; Tn = %d; X1 = %d; Xn = %d"), __FCN__,
            phd->devdesc.picker.S1, phd->devdesc.picker.Sn, phd->devdesc.picker.D1, phd->devdesc.picker.Dn, phd->devdesc.picker.T1, phd->devdesc.picker.Tn, phd->devdesc.picker.X1, phd->devdesc.picker.Xn
            );
            for(i=0; i<phd->devdesc.picker.Dn; i++)
                DbgPlain(30, __FCN_FMT__ _T("Drive %d serial: \"%s\""), __FCN__, i, phd->devdesc.picker.driveSerials.argv[i]);
        }

    }
    DBGFCNOUTLOW();
    return (phd);
}

/********************************************************************************
| add all devices connected to a port to list
**********************************************************************Drago*****/
PhysicalDeviceList *AddPortToList(PCHAR sysInqInfo, int port, PhysicalDeviceList *phyList)
{
    ERH_FUNCTION(_T("AddPortToList"));
    USES_CONVERSION_A2T
    PhysicalDeviceList          *new_phdl = NULL, *first_phdl = NULL;
    PhysicalDevice              *phd;
    PSCSI_ADAPTER_BUS_INFO      adapterInfo;
    ULONG                       i = 0;
    DBGFCNINLOW();

    first_phdl = phyList;
    adapterInfo = (PSCSI_ADAPTER_BUS_INFO) sysInqInfo;

    /*** loop all buses ***/
    for (i = 0; i < adapterInfo->NumberOfBuses; i++)
    {
        PSCSI_INQUIRY_DATA      inquiryData = NULL;

        inquiryData = (PSCSI_INQUIRY_DATA)(sysInqInfo + adapterInfo->BusData[i].InquiryDataOffset);
        DbgPlain(30, __FCN_FMT__ _T("bus=%d; NumberOfLogicalUnits=%d; InitiatorBusId=%d"), __FCN__,
            i,
            adapterInfo->BusData[i].NumberOfLogicalUnits,
            adapterInfo->BusData[i].InitiatorBusId
        );
        /*** loop all inq data ***/
        while (adapterInfo->BusData[i].InquiryDataOffset)
        {
            DbgPlain(30, __FCN_FMT__ _T("port=%d; Path=%d; Target=%d; LUN=%d; Claimed=%s; ID=\"%.28s\" INQUIRY: %02X %02X %02X %02X %02X %02X %02X %02X"), __FCN__,
                port, inquiryData->PathId, inquiryData->TargetId, inquiryData->Lun,
                (inquiryData->DeviceClaimed)? _T("YES"): _T("NO"),
                A2T(&inquiryData->InquiryData[8]),
                inquiryData->InquiryData[0],inquiryData->InquiryData[1],inquiryData->InquiryData[2],inquiryData->InquiryData[3],inquiryData->InquiryData[4],inquiryData->InquiryData[5],inquiryData->InquiryData[6],inquiryData->InquiryData[7]
            );
            /*DbgDump(_T("dump"), inquiryData->InquiryData, inquiryData->InquiryDataLength);*/

            phd = CreatePhysicalDeviceEntry(port, inquiryData);

            if (phd)
            {   /*** add new device entry on top ***/
                new_phdl = MALLOC(sizeof(PhysicalDeviceList));
                new_phdl->phd = phd;
                new_phdl->next = first_phdl;
                first_phdl = new_phdl;
            }

            if (inquiryData->NextInquiryDataOffset == 0)
            {
                break;
            }
            DbgPlain(30, __FCN_FMT__ _T("Next offset = %d"), __FCN__, inquiryData->NextInquiryDataOffset);
            inquiryData = (PSCSI_INQUIRY_DATA)(sysInqInfo + inquiryData->NextInquiryDataOffset);

        } /*** logical unit ***/
    }/*** bus ***/

    DBGFCNOUTLOW();
    return (first_phdl);
}

/********************************************************************************
| Create PhysicalDevice list, returns pointer to the first device in list
**********************************************************************Drago*****/
PhysicalDeviceList *CreatePhysicalDeviceList()
{
    ERH_FUNCTION(_T("CreatePhysicalDeviceList_NT"));
    PhysicalDeviceList *phydev = NULL;
    tchar name[20] = _T("\0");
    int port = 0;
    int fd = -1;
    UCHAR *inquiry_buffer = NULL;
    tchar cardRegKey[256] = _T("\0");
    HKEY hKey = NULL;
    DBGFCNINLOW();

    /* get all devices */
    for (port=0; port<64; port++)
    {
        sprintf(cardRegKey, _T("HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d"), port);

        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            cardRegKey,
                            0,
                            KEY_QUERY_VALUE,
                            &hKey
                        ) == ERROR_SUCCESS
            )
        {
            RegCloseKey(hKey);

            sprintf(name, _T("scsi%d:0:0:0"), port);
            if ( (fd = CreateOB2Device(name, 0)) != -1)
            {
                inquiry_buffer = DevSCTL_GetSystemInquiryData(fd);

                CloseOB2Device(fd);

                if (inquiry_buffer == NULL)
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, __FCN_FMT__ _T("GetSystemInquiryData for %s failed"),__FCN__,name);
                }
                else
                    phydev = AddPortToList(inquiry_buffer, port, phydev);

                FREE(inquiry_buffer);
            }
            else
            {
                DbgPlain(30,__FCN_FMT__ _T("Bus %s: no HBA {%s}\n"), __FCN__, name, ErhErrnoToText());
                ErhClearError();
            } // if open
        }
        else
        {
            DbgPlain(30,__FCN_FMT__ _T("scsi port %d not found in registry %s"), __FCN__, port, cardRegKey);
        }
    } // for

    if (ErhErrno() > 0) ErhClearError();

    DBGFCNOUTLOW();
    return (phydev);
}


/******************************************************************************
|
| USB code (QXCR1000239106)
|
******************************************************************************/
#include <setupapi.h>
#include <ntddscsi.h>
#include <winioctl.h>
#if TARGET_WIN64
#   include <wxp/devioctl.h>
#   include <wxp/NTDDSCSI.H>
#else
#   include <devioctl.h>
#   include <NTDDSCSI.H>
#endif

PhysicalDevice *CreateUSBDeviceNode(PSP_DEVICE_INTERFACE_DETAIL_DATA
                            pInfDetail);
/* PhysicalDeviceList * AddUSBTapeToDeviceList(PhysicalDeviceList *phdlist); */

int GetUSBDATDeviceData(tchar *devNm, PhysicalDevice *phydevp);

/* dummy values of SCSI Path, Target and LUN */
#define USB_SCSI_BUS        128
#define USB_SCSI_TARGET        128
#define USB_SCSI_LUN        128

/* standard GUID for tape class devices */
static GUID TapeClassGUID = { 0x53F5630BL, 0xB6BF, 0x11D0,
                        {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B }};

/******************************************************************************
| AddUSBTapeToDeviceList, returns pointer to the first device in the list
| with HP USB tape devices if detected.
******************************************************************************/
PhysicalDeviceList *
AddUSBTapeToDeviceList(PhysicalDeviceList *phydevlist)
{
    ERH_FUNCTION(_T("AddUSBTapeToDeviceList"));

    PhysicalDevice    *phydev;
    HDEVINFO        hDevInfo;
    DWORD            memberIdx;
    BOOL            flagIf;
    SP_DEVICE_INTERFACE_DATA    devInterface;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    infDetail;

    DBGFCNINLOW();

    SetUSBProcessingFlag(1);
        /* set the flag */

    /* retrieve the tape class device interfaces */
    hDevInfo = SetupDiGetClassDevs(&TapeClassGUID, NULL,
                    NULL, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
    if (INVALID_HANDLE_VALUE == hDevInfo) {
        DbgPlain(81,
            __FCN_FMT__ _T("Unable to retrieve tape class device interfaces"), __FCN__);
        goto exit_label_2;
            /* no need to free HDEVINFO */
    }

    memberIdx = 0;
    memset(&devInterface, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
    devInterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    /* enumerate all the tape class device interfaces */
    while (TRUE == (flagIf = SetupDiEnumDeviceInterfaces(hDevInfo, NULL,
                                    &TapeClassGUID, memberIdx,
                                    &devInterface))) {
        DWORD    requiredSize, devDetail;
        BOOL    flagInfDetail;

        requiredSize = devDetail = 0;
        memberIdx++;

        /* obtain the required size of device interface detailed data */
        flagInfDetail = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                             &devInterface,
                            NULL, devDetail, &requiredSize, NULL);

        devDetail = requiredSize;

        infDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)MALLOC(requiredSize);
        infDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        /* obtain the actual device interface detailed data */
        flagInfDetail = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                            &devInterface,
                            infDetail, devDetail, &requiredSize, NULL);
        if (FALSE == flagInfDetail) {
            DbgPlain(81,
                __FCN_FMT__ _T("Unable to retrieve device interface detail"), __FCN__);
            goto exit_label_1;
                /* need to free up HDEVINFO */
        }

        phydev = CreateUSBDeviceNode(infDetail);
        if (NULL != phydev) {
            /* add it to the phydevlist */
            PhysicalDeviceList    *phydevnode;

            phydevnode = (PhysicalDeviceList *)
                            MALLOC(sizeof(PhysicalDeviceList));
            phydevnode->phd = phydev;
            phydevnode->next = phydevlist;
            phydevlist = phydevnode;
        }

        memset(&devInterface, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
        devInterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        /* free this memory - no further use in this iteration */
        FREE(infDetail);
    }

exit_label_1:

    if (!SetupDiDestroyDeviceInfoList(hDevInfo)) {
        DbgPlain(DBG_UNEXPECTED,
            __FCN_FMT__ _T("Unable to FREE Device Information Set"), __FCN__);
    }

exit_label_2:

    SetUSBProcessingFlag(0);
    /* reset the flag */
    DBGFCNOUTLOW();
    return phydevlist;
}


/******************************************************************************
| CreateUSBDeviceNode, creates device node for USB mass storage device and
| returns the pointer to the same
******************************************************************************/
PhysicalDevice *
CreateUSBDeviceNode(PSP_DEVICE_INTERFACE_DETAIL_DATA pInfDetail)
{
    ERH_FUNCTION(_T("CreateUSBDeviceNode"));
    HANDLE                 devHandle;
    DWORD                  bytesReturned = 0;
    STORAGE_DEVICE_NUMBER  devNumber;
    PhysicalDevice         *phydev;
    tchar                  deviceName[MAX_HWPATH_LEN + 1];
    int                    r = 1;

    DBGFCNINLOW();

    DbgPlain(81, __FCN_FMT__ _T("Device Path = [%s]"), __FCN__, pInfDetail->DevicePath);

    /* check if it is a USB Mass Storage device */
    if (!strstr(pInfDetail->DevicePath, _T("usbstor")))
    {
        DBGFCNOUTLOW_TEXT(_T("NULL"));
        return (NULL);
    }

    /* proceed with processing the USB mass storage device */
    devHandle = CreateFile(pInfDetail->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_OFFLINE | FILE_FLAG_WRITE_THROUGH,
                        NULL);

    if (INVALID_HANDLE_VALUE == devHandle)
    {
        DbgPlain(81, __FCN_FMT__ _T("Unable to open device path [%s]\n"), __FCN__, pInfDetail->DevicePath);
        DBGFCNOUTLOW_TEXT(_T("NULL"));
        return (NULL);
    }

    /* get the corresponding device number */
    if (!DeviceIoControl(devHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            NULL, 0, &devNumber,
                            sizeof(STORAGE_DEVICE_NUMBER),
                            &bytesReturned, NULL))
    {
        DbgPlain(81, __FCN_FMT__ _T("Unable to obtain STORAGE DEVICE NUMBER"), __FCN__);
        DBGFCNOUTLOW_TEXT(_T("NULL"));
        return NULL;
    }

    phydev = NewPhysicalDevice();

    /* init the PhysicalDevice structure */
    phydev->flags = 0;
    phydev->scsi_port = devNumber.DeviceNumber;
    phydev->scsi_path = USB_SCSI_BUS;
    phydev->scsi_target = USB_SCSI_TARGET;
    phydev->scsi_LUN = USB_SCSI_LUN;

    /* makeup a dummy device string identifier "\\.\tape<num>:B:T:L */
    sprintf(deviceName, _T("tape%d:%d:%d:%d"),
                    devNumber.DeviceNumber, USB_SCSI_BUS, USB_SCSI_TARGET,
                    USB_SCSI_LUN);

    /* issue the SCSI pass through commands to retrieve INQUIRY data */
    r = GetUSBDATDeviceData(deviceName, phydev);
    if (r == 0)
    {
        DbgPlain(81, __FCN_FMT__ _T("SCSI PT command failure for USB DAT device"), __FCN__);
        DBGFCNOUTLOW_TEXT(_T("NULL"));
        return NULL;
    }

    DBGFCNOUTLOW();
    return phydev;
}

/******************************************************************************
| GetUSBDATDeviceData, is function that will issue SCSI pass through commands
| and retrieve the vendor ID, product ID, revision and Serial Number
******************************************************************************/
int
GetUSBDATDeviceData(tchar *devName, PhysicalDevice *phydevp)
{
    ERH_FUNCTION(_T("GetUSBDATDeviceData"));
    int            fd, r;
    int            devIdx;

    DBGFCNINLOW();

    r = 1;
    if (-1 == (fd = CreateOB2Device(devName, 1)))
    {
        r = 0;
        goto exit_label;
    }

    r = GetOB2DeviceData(fd, phydevp->vendorID, phydevp->productID,
                    phydevp->revision, phydevp->hwPath);

    StrCutTrailingSpacesInPlace(phydevp->vendorID);
    StrCutTrailingSpacesInPlace(phydevp->productID);
    StrCutTrailingSpacesInPlace(phydevp->revision);
            /* remove unnecessary white spaces */

    /* query for the serial number */
    r = DevbraQuerySerialNumber(fd, phydevp->serial_num);
    if (!r)
    {
        DbgPlain(81, __FCN_FMT__ _T("Unable to obtain valid serial number"), __FCN__);
        r = 0;
    }

    DbgPlain(81, __FCN_FMT__ _T("vendor=%s; product=%s; revision=%s; hwPath=%s; serial_number=%s"),
            __FCN__, phydevp->vendorID, phydevp->productID,
            phydevp->revision, phydevp->hwPath, phydevp->serial_num);

    /* get description and flags for the device from internal table */

    DEVICETABLE_SEARCHINDEXWITHTYPE(phydevp->productID, phydevp->vendorID,
                DEVTYPE_TAPE, devIdx);

    /* set device type */
    phydevp->devType = deviceTable[devIdx].devType;
    if (phydevp->devType == DEV_UNKNOWN)
        phydevp->devType = DEV_TAPE;

    /* being a tape device */
    phydevp->flags |= DEV_DETECT_TAPE;
    phydevp->devdesc.drive.dfType = 0;
    phydevp->devdesc.drive.dfSubType = 0;
    phydevp->devdesc.drive.bcsConnection = 0;

    /* add hostname */
    StrCpy(phydevp->host, cmnHostname);

#if TARGET_HW_TAPE_ENCRYPTION
    CheckDeviceEncryptionSupport(fd, phydevp);
#endif

    if (CloseOB2Device(fd) != 0)
    {
        r = 0;
        DbgPlain(81, __FCN_FMT__ _T("failure of SCTL_CloseDevice (fd=%d)"), __FCN__, fd);
        goto exit_label;
    }

exit_label:
    DBGFCNOUTLOW_DEC(r);
    return (r);
}
