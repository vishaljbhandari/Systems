/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   scsi operations
* @file      recovery/drw/scsiop.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     FileDescription
*
* @since     dd.mm.yy   Emir        Original Coding
*
* @remarks   /
*/
#include "lib\cmn\target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/drw/scsiop.c $Rev$ $Date::                      $:") ;
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/
#define	DISABLE_RTLIB_MAPPINGS
#include <stddef.h>
#include <devioctl.h>
#include <NTDDSCSI.H>
#include "lib\cmn\common.h"
#include "recovery\drw\scsiop.h"
#include "recovery\rid\rid.h"

/* ---------------------------------------------------------------------------
|	static variables
 ---------------------------------------------------------------------------*/
#define	BUS_INFO_SZ		2048

/* ===========================================================================
|
|	FUNCTION     OpenScsiCard
|
 ========================================================================== */	

HANDLE
OpenScsiCard (BYTE cardNum)
{
	ERH_FUNCTION (_T("OpenScsiCard"));

	tchar	scsiCard[16];
	HANDLE	hScsi = INVALID_HANDLE_VALUE;

	DBG_ENTER_FCN;

	_stprintf (scsiCard, _T ("\\\\.\\scsi%d:"), cardNum);

#if defined(UNICODE) || defined(_UNICODE)
	hScsi = CreateFileW(
		scsiCard,
		GENERIC_READ | GENERIC_WRITE,
		0, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL
		);
#else
	hScsi = CreateFile(
		scsiCard,
		GENERIC_READ | GENERIC_WRITE,
		0, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL
		);
#endif

	DbgPlain (DBG_DRSCSI, 
		_T("OpenScsiCard %s -> %s"), 
		scsiCard, hScsi == INVALID_HANDLE_VALUE ? _T("FAILED") : _T("OK")
		);
	DbgPlain (DBG_DRSCSI, _T("%d\n"), GetLastError ());

	return (hScsi);
}

/* ===========================================================================
|
|	FUNCTION     ScsiAddressToString
|
 ========================================================================== */	

tchar *
ScsiAddressToString (PScsiAddress pScsi)
{
	ERH_FUNCTION (_T("ScsiAddressToString"));

	static	tchar	_ScsiString[STRLEN_STD+1];
	tchar			*ret = NULL;

	DBG_ENTER_FCN;

	if (pScsi)
	{
		StrNPrintf(_ScsiString, STRLEN_STD, _T("%d:%d:%d:%d"),
			pScsi->CardNum, pScsi->PathId, pScsi->TargetId, pScsi->Lun
			);
		ret = _ScsiString;
	}

	RETURN_STRING (ret);
}

/* ===========================================================================
|
|	FUNCTION     FunctionName
|
 ========================================================================== */	

int		
GetOccupiedScsiAddresses (
	BYTE			cardNum,
	PVOID			userData,
	ValidateScsiAddressHk	validate
	)
{
	ERH_FUNCTION (_T("GetOccupiedScsiAddresses"));

	int						ret = 0, ii;
	PSCSI_ADAPTER_BUS_INFO	busInfo;
	PSCSI_INQUIRY_DATA		inquiryData;
	InquiryData				inquiryRec;
	BYTE					busData[BUS_INFO_SZ];
	DWORD					bytesRead = 0;
	ScsiAddress				scsiAddress;
	HANDLE					hScsiCard;

	DBG_ENTER_FCN;

	if ((hScsiCard = OpenScsiCard (cardNum)) == INVALID_HANDLE_VALUE)
	{
		ret = -1;
	}
	else if (DeviceIoControl (
				hScsiCard,
				IOCTL_SCSI_GET_INQUIRY_DATA,
				NULL,
				0,
				busData,
				BUS_INFO_SZ,
				&bytesRead,
				FALSE
				) == FALSE
			)
	{
		ret = -1;
	}
	else
	{
		busInfo = (PSCSI_ADAPTER_BUS_INFO) busData;

		for (ii = 0; ii < busInfo->NumberOfBuses; ii++)
		{
			inquiryData = (PSCSI_INQUIRY_DATA) 
				(busData + busInfo->BusData[ii].InquiryDataOffset);

			do {

				memcpy (
					&inquiryRec, inquiryData->InquiryData, 
					sizeof (InquiryData)
					);

				scsiAddress.CardNum = cardNum;
				scsiAddress.PathId = inquiryData->PathId;
				scsiAddress.TargetId = inquiryData->TargetId;
				scsiAddress.Lun = inquiryData->Lun;

				if ((*validate) (&scsiAddress, &inquiryRec, userData) != -1)
				{
				}

				inquiryData = inquiryData->NextInquiryDataOffset ?
					((PSCSI_INQUIRY_DATA) 
						(busData + inquiryData->NextInquiryDataOffset)
					) :
					NULL;

			} while (inquiryData && ret != -1);
		}
	}

	if (hScsiCard != INVALID_HANDLE_VALUE)
		CloseHandle (hScsiCard);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION     FunctionName
|
 ========================================================================== */	

typedef struct _ScsiPassThroughDirect
{
	SCSI_PASS_THROUGH_DIRECT	spt;
	DWORD						thisMustBe;
	BYTE						senseBuffer[MAX_TRANSFER_LENGTH];

} ScsiPassThroughDirect, *PScsiPassThroughDirect;

static	ScsiPassThroughDirect	ScsiOp;

#define	SCSI_PTD_SZ		sizeof (ScsiPassThroughDirect)
#define	INIT_SCSI_OP(p)	(memset (p, 0, SCSI_PTD_SZ))
#define	SENSE_OFFSET	(offsetof (ScsiPassThroughDirect, senseBuffer))

int
ScsiCommand (
	HANDLE			opened,
	PScsiAddress	pScsiAddress,
	PCDB			pCDB,
	PVOID			dataBuffer,
	DWORD			dataLength
	)
{
	ERH_FUNCTION (_T("ScsiCommand"));

	int		ret = 0;
	DWORD	bytesRet = 0;
	HANDLE	handle;

	DBG_ENTER_FCN;

	if (opened != INVALID_HANDLE_VALUE)
	{
		handle = opened;
	}
	else if ((handle = OpenScsiCard (
				pScsiAddress->CardNum)
				) == INVALID_HANDLE_VALUE
			)
	{
		RETURN_VALUE(-1);
	}
	

	INIT_SCSI_OP (&ScsiOp);

	ScsiOp.spt.Length = sizeof (SCSI_PASS_THROUGH_DIRECT);
	ScsiOp.spt.ScsiStatus = 0;
	ScsiOp.spt.PathId = pScsiAddress->PathId;
	ScsiOp.spt.TargetId = pScsiAddress->TargetId;
	ScsiOp.spt.Lun = pScsiAddress->Lun;
	ScsiOp.spt.CdbLength = CDBLEN (pCDB->OpCode);
	ScsiOp.spt.SenseInfoLength = MAX_TRANSFER_LENGTH;
	ScsiOp.spt.DataIn = SCSI_IOCTL_DATA_IN;
	ScsiOp.spt.DataTransferLength = dataLength;
	ScsiOp.spt.TimeOutValue = 240;
	ScsiOp.spt.DataBuffer = dataBuffer;
	ScsiOp.spt.SenseInfoOffset = (ULONG)(ScsiOp.senseBuffer - (BYTE *) &ScsiOp);
	memcpy (&ScsiOp.spt.Cdb, pCDB, CDBLEN (pCDB->OpCode));

	if (!DeviceIoControl (
			handle,
			IOCTL_SCSI_PASS_THROUGH_DIRECT,
			&ScsiOp,
			SCSI_PTD_SZ,
			&ScsiOp,
			SCSI_PTD_SZ,
			&bytesRet,
			FALSE
			)
		)
	{
		ret = -1;
	}

	DbgPlain (DBG_DRSCSI,
		_T("ScsiCommand 0x%x for %d:%d:%d:%d %s. \nSystem error %d"),
		pCDB->OpCode, 
		pScsiAddress->CardNum,
		pScsiAddress->PathId,
		pScsiAddress->TargetId,
		pScsiAddress->Lun,
		(ret == -1) ? _T("FAILED") : _T("OK"),
		(ret == -1) ? GetLastError () : 0
		);

	if (opened == INVALID_HANDLE_VALUE)
		CloseHandle (handle);

	RETURN_VALUE (ret);
}

/* ===========================================================================
|
|	FUNCTION     FunctionName
|
 ========================================================================== */	

int
ScsiCmdInquiry (PScsiAddress pScsi, BYTE *buffer, BYTE length)
{
	ERH_FUNCTION (_T("ScsiCmdInquiry"));

	int		ret = 0;
	CDB		cdb;

	DBG_ENTER_FCN;

	memset (&cdb, 0, sizeof (cdb));
	cdb.OpCode = SCSICMD_INQUIRY;
	cdb.LB.Inquiry.LUN = pScsi->Lun;
	cdb.TransferLength = length;

	if (ScsiCommand (
			INVALID_HANDLE_VALUE, pScsi, &cdb, buffer, length
			) == -1
		)
	{
		ret = -1;
	}

	RETURN_VALUE(ret);
}

/* ===========================================================================
|
|	FUNCTION     ScsiCmdModeSense
|
 ========================================================================== */	

int
ScsiCmdModeSense (PScsiAddress pScsi, BYTE page, BYTE *buffer, BYTE length)
{
	ERH_FUNCTION (_T("ScsiCmdModeSense"));

	int		ret = 0;
	CDB		cdb;

	DBG_ENTER_FCN;

	memset (&cdb, 0, sizeof (cdb));
	cdb.OpCode = SCSICMD_MODE_SENSE_6;
	cdb.LB.ModeSense.LUN = pScsi->Lun;
	cdb.LB.ModeSense.page = page;
	cdb.TransferLength = length;

	if (ScsiCommand (
			INVALID_HANDLE_VALUE, pScsi, &cdb, buffer, length
			) == -1
		)
	{
		ret = -1;
	}

	RETURN_VALUE(ret);
}

/* ===========================================================================
|
|	FUNCTION	ScsiGetBusInfo
|
 ========================================================================== */	

int
ScsiGetBusInfo (BYTE scsiAdapter, PSCSI_ADAPTER_BUS_INFO *pScsiAdapterBusInfo)
{
	ERH_FUNCTION (_T("ScsiGetBusInfo"));

	int						ret = 0;
	HANDLE					hScsiAdapter = INVALID_HANDLE_VALUE;
	PSCSI_ADAPTER_BUS_INFO	pBusInfo = NULL;
	DWORD					busInfoSz = 10 * 1024;//sizeof (SCSI_ADAPTER_BUS_INFO);
	DWORD					bytesRet = 0;
	BOOL					ioRet = FALSE, err = FALSE;

	DBG_ENTER_FCN;

	__try {

		pBusInfo = (PSCSI_ADAPTER_BUS_INFO) MALLOC (busInfoSz);

		if ((hScsiAdapter = OpenScsiCard (scsiAdapter)) == INVALID_HANDLE_VALUE)
		{
			DbgStamp (DBG_DRSCSI);
			DbgPlain (DBG_DRSCSI, _T("Cannot open SCSI card %d"), scsiAdapter);
			__leave;
		}

		while (!err && !ioRet)
		{
			if ((ioRet = DeviceIoControl (
					hScsiAdapter, 
					IOCTL_SCSI_GET_INQUIRY_DATA, 
					NULL,
					0,
					pBusInfo,
					busInfoSz,
					&bytesRet,
					NULL)
					) != 0
				)
			{
				ioRet = TRUE;
			}
			else
			{
				DbgPlain (DBG_DRSCSI, _T("IoCtl (IOCTL_SCSI_GET_INQUIRY_DATA) FAILED\n%d"), GetLastError ());
				if (GetLastError () == ERROR_INSUFFICIENT_BUFFER)
				{
					pBusInfo = (PSCSI_ADAPTER_BUS_INFO) REALLOC (pBusInfo, (busInfoSz += sizeof (SCSI_BUS_DATA)));
					continue;
				}
				err = TRUE;
			}
		}


	} __finally {

		if (hScsiAdapter != INVALID_HANDLE_VALUE)
			CloseHandle (hScsiAdapter);
		if (err)
		{
			FREE (pBusInfo);
			pBusInfo = NULL;
		}
	}

	*pScsiAdapterBusInfo = pBusInfo;

	RETURN_VALUE (err ? -1 : 0);
}

int
DeviceStringToScsiAddress(tchar *deviceString, PScsiAddress pAddr)
{
	int				ret = -1;
	SCSI_ADDRESS	si = { 0 };
	tchar			addrString[MAX_PATH] = { 0 };
	ULONG			bytes = 0;

	_tcscpy(addrString, deviceString);
	_tcslwr(addrString);
    
    if(_tcsnicmp(addrString, _T("tape"), 4) == 0)
	{
		HANDLE	hTape = NULL;
		tchar	tapeName[MAX_PATH] = { 0 };

        _stprintf(tapeName, _T("\\\\.\\%s"), addrString);
		tapeName[9] = _T('\0');
        _tcsupr(tapeName);

#if defined(UNICODE) || defined(_UNICODE)
		hTape = CreateFileW(
#else
		hTape = CreateFileA(
#endif
			tapeName,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
			NULL
		);
		if(INVALID_HANDLE_VALUE != hTape)
		{
			si.Length = sizeof(SCSI_ADDRESS);
			if(
				DeviceIoControl(hTape, IOCTL_SCSI_GET_ADDRESS, NULL, 0, &si, sizeof(SCSI_ADDRESS), &bytes, NULL) != FALSE &&
				bytes == sizeof(SCSI_ADDRESS)
			)
			{
				pAddr->CardNum = si.PortNumber;
				pAddr->PathId = si.PathId;
				pAddr->TargetId = si.TargetId;
				pAddr->Lun = si.Lun;

				ret = 0;
			}

			CloseHandle(hTape);
		}

	}
	else if(_tcsnicmp(addrString, _T("scsi"), 4) == 0)
	{
		if(_stscanf (addrString, 
				_T("scsi%d:%d:%d:%d"),
				&si.PortNumber,
				&si.PathId,
				&si.TargetId,
				&si.Lun
				) == 4
			)
		{
			pAddr->CardNum = si.PortNumber;
			pAddr->PathId = si.PathId;
			pAddr->TargetId = si.TargetId;
			pAddr->Lun = si.Lun;

			ret = 0;
		}
	}
    else if(_tcsnicmp(addrString, _T("\\\\.\\tape"), 8) == 0)
    {
        // This happens when we have USB tape device.
        HANDLE	hTape = NULL;
		tchar	tapeName[MAX_PATH] = { 0 };

        _tcscpy(tapeName, addrString);
        _tcsupr(tapeName);

        pAddr->CardNum = 0;
        pAddr->PathId = 0;
        pAddr->TargetId = 0;
        pAddr->Lun = 0;

        ret = 0;
    }
	else
		ret = -1;

	return ret;
}

/*========================================================================*//**
*
* @brief     ScsiCmdInquiry for OBDR - Defect:  HSLco42138
*
*//*=========================================================================*/
int
ScsiCmdInquiry_OBDR (PScsiAddress pScsi, BYTE *buffer, BYTE length, tchar *tapeName)
{
	ERH_FUNCTION (_T("ScsiCmdInquiry_OBDR"));

	int		ret = 0;
	CDB		cdb;
	HANDLE	hScsi = INVALID_HANDLE_VALUE;
	DWORD	bytesRet = 0;

	DBG_ENTER_FCN;

	memset (&cdb, 0, sizeof (cdb));
	cdb.OpCode = SCSICMD_INQUIRY;
	cdb.LB.Inquiry.LUN = pScsi->Lun;
	cdb.TransferLength = length;


	// RUN SCSI COMMAND
	// first open scsi card

#if defined(UNICODE) || defined(_UNICODE)
	hScsi = CreateFileW(
		tapeName,
		GENERIC_READ | GENERIC_WRITE,
		0, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL
		);
#else
	hScsi = CreateFile(
		tapeName,
		GENERIC_READ | GENERIC_WRITE,
		0, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL
		);
#endif
    if (hScsi == INVALID_HANDLE_VALUE)
    {
        DbgPlain (60, _T("CreateFile() for tape device %s failed (error = %d)."), tapeName, GetLastError());
        return -1;
    }

	DbgPlain (60, _T("Opened tape device %s."), tapeName);

	INIT_SCSI_OP (&ScsiOp);

	ScsiOp.spt.Length = sizeof (SCSI_PASS_THROUGH_DIRECT);
	ScsiOp.spt.ScsiStatus = 0;
    ScsiOp.spt.PathId = pScsi->PathId;
	ScsiOp.spt.TargetId = pScsi->TargetId;
	ScsiOp.spt.Lun = pScsi->Lun;
	ScsiOp.spt.CdbLength = CDBLEN (cdb.OpCode);
	ScsiOp.spt.SenseInfoLength = MAX_TRANSFER_LENGTH;
	ScsiOp.spt.DataIn = SCSI_IOCTL_DATA_IN;
	ScsiOp.spt.DataTransferLength = length;
	ScsiOp.spt.TimeOutValue = 240;
	ScsiOp.spt.DataBuffer = buffer;
	ScsiOp.spt.SenseInfoOffset = (ULONG)(ScsiOp.senseBuffer - (BYTE *) &ScsiOp);
	memcpy (&ScsiOp.spt.Cdb, &cdb, CDBLEN (cdb.OpCode));

	if (!DeviceIoControl (
			hScsi,
			IOCTL_SCSI_PASS_THROUGH_DIRECT,
			&ScsiOp,
			SCSI_PTD_SZ,
			&ScsiOp,
			SCSI_PTD_SZ,
			&bytesRet,
			FALSE
			)
		)
	{
        DbgPlain (60, _T("DeviceIoControl(IOCTL_SCSI_PASS_THROUGH_DIRECT) for ")
            _T("tape device %s failed (error = %d)."), tapeName, GetLastError());
        CloseHandle (hScsi);
		return -1;
	}

	// RUN SCSI COMMAND FINISHED
	DbgPlain (60,
		_T("ScsiCommand 0x%x for %d:%d:%d:%d"),
		cdb.OpCode, 
		pScsi->CardNum,
		pScsi->PathId,
		pScsi->TargetId,
		pScsi->Lun);

	CloseHandle (hScsi);

	RETURN_VALUE(ret);
}