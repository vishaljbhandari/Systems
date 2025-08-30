/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   srd
* @file      recovery/srd/srdparsewin.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     The auxiliary SRD buffer parsing module that handles SRD file parsing specifics for
*            Win32 platforms.
*
* @since     July 2001   lukaj   Original Coding
*
* @remarks   /
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/srdparsewin.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|   include files
 ---------------------------------------------------------------------------*/
#include "lib/cmn/common.h"
#include "lib/parse/gen_parser.h"
#include "srd.h"
#include "srdcmn.h"
#include "srdparse.h"
#include "srdfree.h"

/* ---------------------------------------------------------------------------
|   THE parse function (calls every other parsing function)
 ---------------------------------------------------------------------------*/
R_RESULT
SrdParseWinSrdData(R_TCHAR *pBuffer, PSRDDATA *ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    SRDDATA     TmpData = { 0 };
    PSRDDATA    pSrdData = NULL;
    ParserList  *pParserList = NULL;
    R_TCHAR     *pNative = NULL;

    _TRY_
        if(pBuffer == NULL || ppOut == NULL)
            _LEAVE_;
        memset(&TmpData, 0, sizeof(SRDDATA));

        PROBE_ZERO_MALLOC(TmpData.Header, sizeof(SRDHEADER));
        if(SrdParseHeader((R_UCHAR*)pBuffer, &pNative, TmpData.Header, &pParserList) != SRDERR_SUCCESS)
            _LEAVE_;
        PROBE_ZERO_MALLOC(TmpData.SysInfo, sizeof(SYSINFO));
        if(SrdParseSysInfo(pParserList, TmpData.SysInfo, TmpData.Header) != SRDERR_SUCCESS)
            _LEAVE_;
        if(SrdParseClusInfo(pParserList, &TmpData.ClusInfo, TmpData.Header) != SRDERR_SUCCESS)
            _LEAVE_;
        if(SrdParseNetworkInfo(pParserList, &TmpData.NetworkInfo, TmpData.Header) != SRDERR_SUCCESS)
            _LEAVE_;
        PROBE_ZERO_MALLOC(TmpData.DiskInfo, sizeof(DISKINFO));
        if (!IS_LINUX(TmpData.Header->Platform))
            if(SrdParseDiskInfo(pParserList, TmpData.DiskInfo) != SRDERR_SUCCESS)
                _LEAVE_;

        if(SrdParseObjectInfo(pParserList, &TmpData.RestoreVolumes, TmpData.Header) != SRDERR_SUCCESS)
            _LEAVE_;

        PROBE_ZERO_MALLOC(pSrdData, sizeof(SRDDATA));

        pSrdData->Header = TmpData.Header;
        pSrdData->SysInfo = TmpData.SysInfo;
        pSrdData->ClusInfo = TmpData.ClusInfo;
        pSrdData->DiskInfo = TmpData.DiskInfo;
        pSrdData->RestoreVolumes = TmpData.RestoreVolumes;
        pSrdData->NetworkInfo = TmpData.NetworkInfo;

        *ppOut = pSrdData;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(Return != SRDERR_SUCCESS)
            FreeSrdData(&TmpData);
        if(pParserList != NULL)
            PLFree(pParserList);
        if(pNative != NULL)
            FREE(pNative);
    _ENDTRY_

    return Return;
}

/* ---------------------------------------------------------------------------
|   THE put function (calls every other putting function)
 ---------------------------------------------------------------------------*/
R_RESULT
SrdPutWinSrdData(PSRDDATA pSrdData, R_TCHAR **ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    ParserList  *pParserList = NULL;
    R_TCHAR     *pBuffer = NULL;

    *ppOut = NULL;

    _TRY_
        pParserList = PLNew();
        if(pParserList == NULL)
            _LEAVE_;
        pParserList->pt = drInfo;
        pParserList->ptSize = PTGetItemCount(drInfo);
#if defined(UNICODE)
        pParserList->flags |= PL_UNICODE;
#else
        pParserList->flags &= ~PL_UNICODE;
#endif

        if(pSrdData->Header == NULL)
            _LEAVE_;
        if(SrdPutHeader(pParserList, pSrdData->Header) != SRDERR_SUCCESS)
            _LEAVE_;
        if(pSrdData->SysInfo == NULL)
            _LEAVE_;
        if(SrdPutSysInfo(pParserList, pSrdData->SysInfo) != SRDERR_SUCCESS)
            _LEAVE_;
        if(SrdPutClusInfo(pParserList, pSrdData->ClusInfo) != SRDERR_SUCCESS)
            _LEAVE_;
        if(SrdPutNetworkInfo(pParserList, pSrdData->NetworkInfo) != SRDERR_SUCCESS)
            _LEAVE_;
        if(
            pSrdData->DiskInfo != NULL &&
            pSrdData->DiskInfo->DiskCount != 0
        )
        {
            if(SrdPutDiskInfo(pParserList, pSrdData->DiskInfo) != SRDERR_SUCCESS)
                _LEAVE_;
        }
        if(pSrdData->RestoreVolumes == NULL)
            _LEAVE_;
        if(SrdPutObjectInfo(pParserList, pSrdData->RestoreVolumes) != SRDERR_SUCCESS)
            _LEAVE_;

        if(PLGetBufferFromList(&pBuffer, pParserList) <= 0)
            _LEAVE_;

        if(NativeToBuffer((R_UCHAR**)&pBuffer) != SRDERR_SUCCESS)
            _LEAVE_;

        *ppOut = pBuffer;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(Return != SRDERR_SUCCESS)
        {
            if(pBuffer != NULL)
                FREE(pBuffer);
        }

        if(pParserList != NULL)
            PLFree(pParserList);
    _ENDTRY_

    return Return;
}

R_RESULT
SrdGetPotentialAsrBufferSize(PASRDATA pAsrData, R_ULONG *pSize)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_ULONG     ii = 0;
    R_ULONG     ulSize = sizeof(R_ULONG) * 4;

    _TRY_
        if(pAsrData == NULL || pAsrData->Header == NULL)
            _LEAVE_;
        for(ii = 0; ii < pAsrData->Count; ii++)
        {
            if (
                pAsrData->AsrFiles[ii] == NULL ||
                pAsrData->AsrFiles[ii]->Data == NULL
            )
                _LEAVE_;
            ulSize += sizeof(R_ULONG) * 2;
            ulSize += pAsrData->AsrFiles[ii]->Size;
        }

        if(pSize != NULL)
            *pSize = ulSize;

        Return = SRDERR_SUCCESS;
    _FINALLY_
    _ENDTRY_

    return Return;
}

R_RESULT
AsrParseHeader(R_UCHAR *pBuffer, PASRHEADER pHeader, R_UCHAR **ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;

    _TRY_
        R_ULONG ulSize = 0;
        R_UCHAR pFourBytes[4] = { 0 };
        R_UCHAR *pNext = pBuffer;
        R_UCHAR *dst = NULL;
        /* ---------------------------------------------------------------------------
        |   sanity checking.
         ---------------------------------------------------------------------------*/
        if(pBuffer == NULL || pHeader == NULL)
            _LEAVE_;
        /* ---------------------------------------------------------------------------
        |   verify the input buffer.
         ---------------------------------------------------------------------------*/
        if(BufferSize(pBuffer, SRDHT_ASRINFO) == 0xFFFFFFFF)
            _LEAVE_;
        /* ---------------------------------------------------------------------------
        |   read magic number.
         ---------------------------------------------------------------------------*/
        READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
        ASSIGN_ULONG(pHeader->Magic, pFourBytes, 4);
        /* ---------------------------------------------------------------------------
        |   read srd library version.
         ---------------------------------------------------------------------------*/
        READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
        ASSIGN_ULONG(pHeader->Version, pFourBytes, 4);
        /* ---------------------------------------------------------------------------
        |   read source platform.
         ---------------------------------------------------------------------------*/
        READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
        ASSIGN_ULONG(pHeader->Platform, pFourBytes, 4);

        if(ppOut != NULL)
            *ppOut = pNext;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        /* ---------------------------------------------------------------------------
        |   free all.
         ---------------------------------------------------------------------------*/
    _ENDTRY_

    return Return;
}

/* ---------------------------------------------------------------------------
|   THE parse function for ASR purposes.
 ---------------------------------------------------------------------------*/
R_RESULT
SrdParseAsrData(R_UCHAR *pBuffer, PASRDATA *ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    PASRDATA    pAsrData = NULL;

    _TRY_
        R_ULONG ii = 0, ulSize = 0;
        R_UCHAR *pNext = pBuffer;
        R_UCHAR pFourBytes[4] = { 0 };
        R_UCHAR *dst = NULL;

        /* ---------------------------------------------------------------------------
        |   sanity checking.
         ---------------------------------------------------------------------------*/
        if(pBuffer == NULL || ppOut == NULL)
            _LEAVE_;
        /* ---------------------------------------------------------------------------
        |   allocate ASRDATA structures.
         ---------------------------------------------------------------------------*/
        PROBE_ZERO_MALLOC(pAsrData, sizeof(ASRDATA));
        PROBE_ZERO_MALLOC(pAsrData->Header, sizeof(ASRHEADER));

        if(AsrParseHeader(pBuffer, pAsrData->Header, &pNext) != SRDERR_SUCCESS)
            _LEAVE_;
        /* ---------------------------------------------------------------------------
        |   read the number of files stored within the input buffer.
         ---------------------------------------------------------------------------*/
        READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
        ASSIGN_ULONG(pAsrData->Count, pFourBytes, 4);
        /* ---------------------------------------------------------------------------
        |   (re)allocate enough memory to accomodate all files.
         ---------------------------------------------------------------------------*/
        if(pAsrData->Count > 1)
        {
            R_ULONG ulAllocSize = sizeof(ASRDATA) + (pAsrData->Count - 1) * sizeof(PASRFILEBUF);
            pAsrData = REALLOC(pAsrData, ulAllocSize);
            if(pAsrData == NULL)
                _LEAVE_;
            memset(&pAsrData->AsrFiles, 0, sizeof(PASRFILEBUF) * pAsrData->Count);
        }

        for(ii = 0; ii < pAsrData->Count; ii++)
        {
            /* ---------------------------------------------------------------------------
            |   allocate file definition structure.
             ---------------------------------------------------------------------------*/
            PROBE_ZERO_MALLOC(pAsrData->AsrFiles[ii], sizeof(ASRFILEBUF));
            /* ---------------------------------------------------------------------------
            |   read file type.
             ---------------------------------------------------------------------------*/
            READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
            ASSIGN_ULONG(pAsrData->AsrFiles[ii]->Type, pFourBytes, 4);
            /* ---------------------------------------------------------------------------
            |   read file size.
             ---------------------------------------------------------------------------*/
            READ_BUFFER(pNext, 4, (dst = pFourBytes), ulSize); pNext += 4;
            ASSIGN_ULONG(pAsrData->AsrFiles[ii]->Size, pFourBytes, 4);
            /* ---------------------------------------------------------------------------
            |   allocate buffer for file data and read it.
             ---------------------------------------------------------------------------*/
            pAsrData->AsrFiles[ii]->Data = MALLOC(pAsrData->AsrFiles[ii]->Size);
            if(pAsrData->AsrFiles[ii]->Data == NULL)
                _LEAVE_;
            READ_BUFFER(pNext, pAsrData->AsrFiles[ii]->Size, pAsrData->AsrFiles[ii]->Data, ulSize); pNext += pAsrData->AsrFiles[ii]->Size;
        }

        *ppOut = pAsrData;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(Return != SRDERR_SUCCESS)
        {
            FreeAsrData(pAsrData);
            FREE(pAsrData);
        }
    _ENDTRY_

    return Return;
}

/* ---------------------------------------------------------------------------
|   THE put function for ASR purposes.
 ---------------------------------------------------------------------------*/
R_RESULT
SrdPutAsrData(PASRDATA pAsrData, R_UCHAR **ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_UCHAR     *pBuffer = NULL;

    *ppOut = NULL;

    _TRY_
        R_ULONG ii = 0, ulSize = 0;
        R_UCHAR *pNext = NULL;
        R_UCHAR pFourBytes[4] = { 0 };

        if(pAsrData == NULL || pAsrData->Header == NULL)
            _LEAVE_;
        if(SrdGetPotentialAsrBufferSize(pAsrData, &ulSize) != SRDERR_SUCCESS)
            _LEAVE_;
        PROBE_ZERO_MALLOC(pBuffer, ulSize);

        pNext = pBuffer;

        DEASSIGN_ULONG(pAsrData->Header->Magic, pFourBytes, 4);
        WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;
        DEASSIGN_ULONG(pAsrData->Header->Version, pFourBytes, 4);
        WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;
        DEASSIGN_ULONG(pAsrData->Header->Platform, pFourBytes, 4);
        WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;
        DEASSIGN_ULONG(pAsrData->Count, pFourBytes, 4);
        WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;

        for(ii = 0; ii < pAsrData->Count; ii++)
        {
            DEASSIGN_ULONG(pAsrData->AsrFiles[ii]->Type, pFourBytes, 4);
            WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;
            DEASSIGN_ULONG(pAsrData->AsrFiles[ii]->Size, pFourBytes, 4);
            WRITE_BUFFER(pFourBytes, 4, pNext); pNext += 4;
            WRITE_BUFFER(pAsrData->AsrFiles[ii]->Data, pAsrData->AsrFiles[ii]->Size, pNext); pNext += pAsrData->AsrFiles[ii]->Size;
        }

        *ppOut = pBuffer;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(Return != SRDERR_SUCCESS)
        {
            if(pBuffer != NULL)
                FREE(pBuffer);
        }
    _ENDTRY_

    return Return;
}
