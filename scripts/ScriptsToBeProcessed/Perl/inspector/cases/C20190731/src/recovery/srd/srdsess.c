/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   srd
* @file      recovery/srd/srdsess.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     This module handles the specifics of getting the required object data from OmniBack's
*            internal database. Esablishing of DB sessions is also handled here.
*
* @since     July 2001   lukaj   Original Coding
*
* @remarks   /
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/srdsess.c $Rev$ $Date::                      $:") ;
#endif

/* ---------------------------------------------------------------------------
|   include files
 ---------------------------------------------------------------------------*/
#include "srd.h"
#include "lib/cmn/common.h"
#include "cs/csa/csa.h"
#include "lib/parse/strtable.h"
#include "lib/parse/sdstruct.h"
#include "lib/parse/sdparse.h"
#include "srdcmn.h"
#include "srdsess.h"
#include "srdfree.h"
#include "integ/postgres/postgres_defines.h"

/* From srdaction.c */
R_RESULT
GetRawDiskSectionForMount(R_TCHAR *pObjectOptions,
                          R_TCHAR *mount,
                          R_TCHAR **output);

/* ===========================================================================
|
|   FUNCTION     SrdQueryEngine
|
 ========================================================================== */
R_RESULT
SrdQueryEngine(R_INT dbId, StPtr st)
{
    ERH_FUNCTION (_T("SrdQueryEngine"));

    R_RESULT    Return = SRDERR_ERROR;
    R_INT       iLoop = 1;
    R_TCHAR     *output = NULL, *ptr = NULL;

    SRD_ENTER_FCN;

    while (iLoop && (CsaRead (dbId, &output, 0) != -1))
    {
        R_INT msgNo = StrParseStart(output);

        switch(msgNo)
        {
            case MSG_RESULT:
                ErhUnpackMsg();

                Return = (ErhErrno() == 0 || ErhErrno() == EDB_NODATA)
                    ? SRDERR_SUCCESS : SRDERR_ERROR;

                if (Return == SRDERR_ERROR)
                {
                    DbgPlain (DBG_SRD_SESS, _T("DBSM error %s"), ErhErrnoToText());
                    ErhClearError ();
                }
                else if (ST_N(st) <= 0)
                {
                    if (ErhErrno() == EDB_NODATA)
                        ErhClearError();

                    DbgPlain (DBG_SRD_SESS, _T("Data along with MSG_RESULT received.\n."));

                    while ((ptr = StrParseNext()) != NULL)
                        StAdd(st, ptr);
                }

                iLoop = 0;

                break;

            default:
                DbgPlain (DBG_SRD_SESS, _T("Data along with message %d received.\n."), msgNo);

                while ((ptr = StrParseNext()) != NULL)
                    StAdd (st, ptr);
                break;
        }

        FREE (output);
    }

    SRD_RETURN_VAL(Return);
}


/* ===========================================================================
|
|   FUNCTION    IsObjectCopy
|
 ========================================================================== */

R_RESULT
IsObjectCopy(StPtr chain, 
             R_TCHAR* overId, 
             R_BOOL* iscopy)
{
    ERH_FUNCTION (_T("IsObjectCopy"));

    R_INT       ii = 0;
    R_INT       addIdx = 0;
    R_INT       pNrFld = NR_OV + 1 + addIdx;
    R_INT       NrSessions = ST_N (chain) / pNrFld;
    R_TCHAR     *ovOpt = NULL;

    SRD_ENTER_FCN;

    for (ii = 0; ii < NrSessions; ii++)
    {
        /* Skipp object copies from restore chain. */
        ovOpt = StrNewCopy(ST_I(chain, ii * pNrFld + addIdx + FLD_OV_ID));

        DbgPlain (DBG_SRD_SESS, _T("Comparing object: %s with %s."),
            ovOpt, overId);
        if (ovOpt == NULL)
            continue;

        if (strcmp(ovOpt, overId) == 0)
        {
            FREE(ovOpt);
            ovOpt = StrNewCopy(ST_I(chain, ii * pNrFld + addIdx + FLD_OV_FLAGS));
            if (ovOpt != NULL && (atoi(ovOpt) & OVF_COPY) != 0)
            {
                *iscopy = TRUE;
            }

            FREE(ovOpt);
            break;
        }
        FREE(ovOpt);
    }

    SRD_RETURN_VAL(TRUE);
}

/* ===========================================================================
|
|   FUNCTION    GetDeviceDetails
|
 ========================================================================== */

R_RESULT
GetDeviceDetails(R_INT dbId, R_TCHAR *pszName, PSrdSession pSess)
{
    ERH_FUNCTION (_T("GetDeviceDetails"));

    R_RESULT    Return = SRDERR_ERROR;
    const       R_DWORD buffLen = STRLEN_PATH + STRLEN_STD;
    R_TCHAR     buff[STRLEN_PATH + STRLEN_STD];
    StPtr       devInfo = NULL;
    LDev        LogicalDevice = { 0 };

    SRD_ENTER_FCN;

    InitLDev(&LogicalDevice);

    _TRY_

        StrMsgMake(buff, buffLen, FUN_GETDEVICE, pszName, NULL);
        if (CsaWrite (dbId, buff, 0) == -1)
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("CsaWrite() FAILED.")); }
            );
        devInfo = StInit(NULL);
        if (SrdQueryEngine(dbId, devInfo) == SRDERR_ERROR)
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("SrdQueryEngine() FAILED.")); }
            );
        if(ST_N(devInfo) <= 0)
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("No return vector (ST_N(devInfo) = %d)."), ST_N(devInfo)); }
            );


        if(LDevParse(&LogicalDevice, ST_I(devInfo, 0)) == -1)
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("LDevParse() FAILED.")); }
            );
        if(
            LogicalDevice.host == NULL
#if 0
            || CmnVecH(&LogicalDevice.drives) <= 0
#endif
        )
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("Verification FAILED (host: %p)."), LogicalDevice.host); }
            );

        pSess->DeviceType = LogicalDevice.mediaclass;
        DbgPlain (DBG_SRD_SESS, _T("Device type: %ld.\n"), pSess->DeviceType);

        pSess->DevicePolicy = LogicalDevice.policy;
        DbgPlain (DBG_SRD_SESS, _T("Device policy: %ld.\n"), pSess->DevicePolicy);

        pSess->MaHost = StrNewCopy(LogicalDevice.host);
        if(pSess->MaHost == NULL)
            _LEAVE_DBG_(
                TRUE,
                Return, SRDERR_ERROR,
                DBG_SRD_SESS,
                { DbgPlain(DBG_SRD_SESS, _T("Memory allocation FAILURE for MA host.")); }
            );
        DbgPlain (DBG_SRD_SESS, _T("MA host: %s.\n"), pSess->MaHost);

        if(CmnVecH(&LogicalDevice.drives) != 0)
        {
            pSess->DeviceAddress = StrNewCopy(CmnVecI(&LogicalDevice.drives, 0));
            if(pSess->DeviceAddress == NULL)
                _LEAVE_DBG_(
                    TRUE,
                    Return, SRDERR_ERROR,
                    DBG_SRD_SESS,
                    { DbgPlain(DBG_SRD_SESS, _T("Memory allocation FAILURE for device address.")); }
                );
            DbgPlain (DBG_SRD_SESS, _T("Device address: %s.\n"), pSess->DeviceAddress);

            if(LogicalDevice.library != NULL && CmnVecH(&LogicalDevice.drives) > 1)
            {
                pSess->DriveIndex = StrNewCopy(CmnVecI(&LogicalDevice.drives, 1));
                DbgPlain (DBG_SRD_SESS, _T("Drive index: %s.\n"), pSess->DriveIndex);
            }
            else
            {
                pSess->DriveIndex = NULL;
                DbgPlain (DBG_SRD_SESS, _T("Drive index: NULL.\n"));
            }
        }
        else
        {
            pSess->DeviceAddress = NULL;
            DbgPlain (DBG_SRD_SESS, _T("Device address: NULL.\n"));
            pSess->DriveIndex = NULL;
            DbgPlain (DBG_SRD_SESS, _T("Drive index: NULL.\n"));
        }

        if(LogicalDevice.serial != NULL && LogicalDevice.serial[0] != _T('\0'))
        {
            pSess->DeviceSerial = StrNewCopy(LogicalDevice.serial);
            DbgPlain (DBG_SRD_SESS, _T("Device serial: %s.\n"), pSess->DeviceSerial);
        }
        else
        {
            pSess->DeviceSerial = NULL;
            DbgPlain (DBG_SRD_SESS, _T("Device serial: NULL.\n"));
        }

        /* In case only hardware encryption is used we have to put aes256 string in SRD file!
           Put it there only if it is not already set. */
        DbgPlain (DBG_SRD_SESS, _T("Encrypt flag of current session: %s"), pSess->Encrypt);
        if (pSess->Encrypt == NULL && (LogicalDevice.flags & DF_DEVENCRYPT) != 0)
        {
            DbgPlain (DBG_SRD_SESS, _T("Setting encrypt flag of current session to aes256."));
            pSess->Encrypt = StrNewCopy(_T("aes256"));
        }

        Return = SRDERR_SUCCESS;
    _FINALLY_
        FreeLDev(&LogicalDevice);
        if(devInfo != NULL)
            StDestroy(devInfo);
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

/* ===========================================================================
|
|   FUNCTION    SrdGetObject
|
 ========================================================================== */

R_RESULT
SrdGetObject (
    R_INT       dbId,
    R_TCHAR     *objName,
    R_TCHAR     *overId,
    R_TCHAR     *volume,
    R_BOOL      fDaIDOnly,
    PSrdObject  *pObjOut
    )
{
    ERH_FUNCTION (_T("SrdGetObject"));
    R_RESULT    Return = SRDERR_ERROR;
    const       R_INT buffLen = STRLEN_PATH + STRLEN_STD;
    R_TCHAR     buff[STRLEN_PATH + STRLEN_STD];
    StPtr       chain = NULL, tapes = NULL, tapeInfo = NULL, storeInfo = NULL;
    R_TCHAR     host[STRLEN_HOSTNAME];
    R_TCHAR     label[STRLEN_DESCRIPTION];
    R_INT       type;
    PSrdObject  pObject = NULL;

    SRD_ENTER_FCN;
    DbgPlain (DBG_SRD_SESS, _T("Object name: %s."), objName);
    DbgPlain (DBG_SRD_SESS, _T("Object version ID: %s."), overId);
    DbgPlain (DBG_SRD_SESS, _T("Volume: %s."), volume);

    StrFromObjectName(&type, host, 0, label, objName);

    _TRY_
        StrMsgMake(buff, buffLen, FUN_FBLISTALLOVER, NULL);
        if (type == OT_OB2BAR)
        {
            StrMsgFAppend(_T("%s%ld%ld%ld%ld"), objName, DBA_FBMULTIPLE | DBA_FBTREES, 0, 0, 0);
        }
        else
        {
            StrMsgFAppend(_T("%s%d"), objName, 0);
        }

        if (CsaWrite(dbId, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbId, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        StDestroy(chain);
        chain = NULL;

        StrMsgMake(buff, buffLen, FUN_FBLISTFULLRESCHAIN, overId, _T(""), NULL);

        if (CsaWrite(dbId, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain(DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        pObject = MALLOC(sizeof(SrdObject));
        if(pObject == NULL)
            _LEAVE_;
        memset (pObject, 0, sizeof(SrdObject));
        pObject->ObjHost = StrNewCopy(host);
        pObject->Label = StrNewCopy(label);
        if(pObject->Label == NULL)
            _LEAVE_;

        chain = StInit (NULL);

        if (SrdQueryEngine (dbId, chain) != SRDERR_ERROR)
        {
            R_INT       ii = 0;
            R_INT       addIdx = 0;
            R_INT       NrFld = NR_OV + 1 + addIdx;
            R_INT       NrSessions = ST_N (chain) / NrFld;
            R_TCHAR     *ovOpt = NULL;
            R_BOOL      idbConfDone = FALSE;
            R_BOOL      iscopy = FALSE;

            IsObjectCopy(chain, overId, &iscopy);

            for (ii = 0; ii < NrSessions; ii++)
            {
                PSrdSession pSess = NULL;

                if (iscopy)
                {
                    /* We support only full object copies, so add only one
                       session info (that of the copy) and skip all others.
                    */
                    ovOpt = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_ID));
                    if (strcmp(ovOpt, overId) != 0)
                    {
                        FREE(ovOpt);
                        continue;
                    }

                    DbgPlain (DBG_SRD_SESS, _T("Object is a copy."));
                }
                else
                {
                    /* Skipp object copies from restore chain, we could maybe
                       updated the IDB query to skip them in the first place.
                    */
                    ovOpt = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_FLAGS));
                    if (ovOpt != NULL && (atoi(ovOpt) & OVF_COPY) != 0)
                    {
                        DbgPlain (DBG_SRD_SESS, _T("Skipping object copy version: %s."),
                            StrToUserSessionId(ST_I(chain, ii * NrFld + addIdx + NR_OV)));

                        FREE(ovOpt);
                        continue;
                    }
                }

                FREE(ovOpt);

                /* We only need the last inc session for IDB ConfigurationFiles */
                if (idbConfDone)
                {
                    /* To elaborate this some more. If we find out that this is CF object and
                       we only need to add the last incremental session since it contains everything.
                    */
                    break;
                }
                else if (strcmp(volume, POSTGRES_MPOINT_CONFFILES) == 0)
                {
                    idbConfDone = TRUE;
                }

                /* ---------------------------------------------------------------
                |   initialize session
                 ---------------------------------------------------------------*/
                if(pObject->NrSessions != 0)
                    pObject = REALLOC(pObject, sizeof(SrdObject) + pObject->NrSessions * sizeof(PSrdSession));

                if(pObject == NULL)
                    _LEAVE_;
                pObject->Sessions[pObject->NrSessions] = MALLOC(sizeof(SrdSession));
                if(pObject->Sessions[pObject->NrSessions] == NULL)
                    _LEAVE_;

                memset(pObject->Sessions[pObject->NrSessions], 0, sizeof(SrdSession));

                pSess = pObject->Sessions[pObject->NrSessions];
                pObject->NrSessions += 1;

                pSess->Id = StrNewCopy(StrToUserSessionId(ST_I(chain, ii * NrFld + addIdx + NR_OV)));
                if(pSess->Id == NULL)
                    _LEAVE_;
                DbgPlain (DBG_SRD_SESS, _T("Session ID: %s.\n"), pSess->Id);

                pSess->CopyId = (R_UINT64)0;
                if (iscopy)
                {
                    DPID id = {0};

                    StrToDPID(&id, ST_I(chain, ii * NrFld + addIdx + FLD_OV_ID));
                    pSess->CopyId = id.sequence;
                    DbgPlain (DBG_SRD_SESS, _T("Copy ID: %llu.\n"), pSess->CopyId);
                    pSess->CopyIdStr = StrNewCopy(StrToUserDPID(ST_I(chain, ii * NrFld + addIdx + FLD_OV_ID)));
                    DbgPlain (DBG_SRD_SESS, _T("Copy ID string: %s\n"), pSess->CopyIdStr);
               }

                pSess->Device = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_BACKUPDEVICE));
                if(pSess->Device == NULL)
                    _LEAVE_;
                DbgPlain (DBG_SRD_SESS, _T("Logical device: %s.\n"), pSess->Device);

                ovOpt = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_OPT));
                if(ovOpt != NULL)
                {
                    R_TCHAR *section = NULL;
                    R_TCHAR *enc = strstr(ovOpt, _T("-encode"));

                    DbgPlain (DBG_SRD_SESS, _T("Object version options: %s.\n"), ovOpt);
                    if (enc)
                    {
                        R_TCHAR *token = strtok(enc, _T(" "));
                        token = strtok(NULL, _T("\n"));

                        if(token)
                            pSess->Encrypt = StrNewCopy(token);
                    }

                    if (type == OT_RAWDISK)
                    {
                        if (GetRawDiskSectionForMount(ovOpt, volume, &section) != SRDERR_SUCCESS)
                        {
                            _LEAVE_;
                        }
                        else if (section != NULL && pObject->RawDiskSection == NULL)
                        {
                            pObject->RawDiskSection = section;
                            section = NULL;
                        }
                    }

                    if (section != NULL)
                    {
                        FREE(section);
                    }
                    FREE(ovOpt);
                }

                pSess->DaId = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_DISKAGENTID));
                if(pSess->DaId == NULL)
                    _LEAVE_;
                DbgPlain (DBG_SRD_SESS, _T("DA ID: %s.\n"), pSess->DaId);

                pSess->StartTime = StrNewCopy(ST_I(chain, ii * NrFld + addIdx + FLD_OV_STARTTIME));
                if (pSess->StartTime == NULL)
                    _LEAVE_;

                DbgPlain (DBG_SRD_SESS, _T("Object start time: %s.\n"), pSess->StartTime);

                if(TRUE == fDaIDOnly)
                    continue;

                if(GetDeviceDetails(dbId, pSess->Device, pSess) != SRDERR_SUCCESS)
                    _LEAVE_;
                /* ---------------------------------------------------------------
                |   get tapes
                 ---------------------------------------------------------------*/
                StrMsgMake(buff, buffLen, FUN_LISTOVERMPOS, ST_I(chain, ii * NrFld + addIdx), NULL);
                if (CsaWrite (dbId, buff, 0) != -1)
                {
                    PSrdTape    pTape = NULL;
                    Store       StoreLibrary = { 0 };

                    tapes = StInit(NULL);
                    InitStore (&StoreLibrary);

                    if (SrdQueryEngine (dbId, tapes) == SRDERR_ERROR)
                    {
                        SRD_STAMP(DBG_SRD_SESS);
                        DbgPlain(DBG_SRD_SESS, _T("Cannot query database for FUNLISTOVERMPOS"));
                        _LEAVE_;
                    }
                    else
                    {
                        R_INT       jj;
                        R_INT       NrTapeFld = 10;
                        R_INT       NrTapes = ST_N(tapes) / NrTapeFld;

                        if(NrTapes > 1)
                        {
                            pObject->Sessions[pObject->NrSessions - 1] = REALLOC(pObject->Sessions[pObject->NrSessions - 1],
                                sizeof(SrdSession) + sizeof(PSrdTape) * (NrTapes - 1));
                            if(pObject->Sessions[pObject->NrSessions - 1] == NULL)
                                _LEAVE_;
                            memset(pObject->Sessions[pObject->NrSessions - 1]->Tapes, 0, sizeof(PSrdTape));

                            pSess = pObject->Sessions[pObject->NrSessions - 1];
                        }

                        if(pSess == NULL)
                            _LEAVE_;

                        pSess->NrTapes = NrTapes;
                        DbgPlain (DBG_SRD_SESS, _T("Number of tapes: %d.\n"), pSess->NrTapes);
                        for (jj = 0; jj < NrTapes; jj++)
                        {
                            R_TCHAR *data = NULL;

                            pSess->Tapes[jj] = MALLOC(sizeof(SrdTape));
                            if(pSess->Tapes[jj] == NULL)
                                _LEAVE_;
                            memset(pSess->Tapes[jj], 0, sizeof(SrdTape));

                            pTape = pSess->Tapes[jj];

                            pTape->Id = StrNewCopy(ST_I(tapes, jj * NrTapeFld + 4));
                            if(pTape->Id == NULL)
                                _LEAVE_;
                            DbgPlain (DBG_SRD_SESS, _T("Medium ID: %s.\n"), pTape->Id);

                            data = ST_I(tapes, jj * NrTapeFld + 2);
                            if((pTape->Segment = atoi(data)) < 0)
                            {
                                DbgPlain (DBG_SRD_SESS, _T("Segment cannot be less than 0. Value set to 0."));
                                pTape->Segment = 0;
                            }
                            DbgPlain (DBG_SRD_SESS, _T("Tape segment: %d.\n"), (R_INT)pTape->Segment);

                            data = ST_I(tapes, jj * NrTapeFld + 3);
                            if ((pTape->Offset = atoi(data)) < 0)
                            {
                                DbgPlain (DBG_SRD_SESS, _T("Offset cannot be less than 0. Value set to 0."));
                                pTape->Offset = 0;
                            }
                            DbgPlain (DBG_SRD_SESS, _T("Tape offset: %d.\n"), pTape->Offset);

#if 1
                            /* ---------------------------------------------------
                            |   Query database to get tape's location
                             ---------------------------------------------------*/
                            tapeInfo = StInit(NULL);
                            StrMsgMake(buff, buffLen, FUN_FINDMEDIUM, pTape->Id, NULL);
                            if(CsaWrite(dbId, buff, 0) == -1)
                            {
                                SRD_STAMP(DBG_SRD_SESS);
                                DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed"));
                                _LEAVE_;
                            }
                            else if(SrdQueryEngine(dbId, tapeInfo) == SRDERR_ERROR)
                            {
                                SRD_STAMP(DBG_SRD_SESS);
                                DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed on FUN_FINDMEDIUM for %s"), pTape->Id);
                                _LEAVE_;
                            }
                            else
                            {
                                pTape->PhysicalLocation = StrNewCopy(ST_I (tapeInfo, FLD_CART_PHYLOCATION));
                                if(pTape->PhysicalLocation == NULL)
                                    _LEAVE_;
                                DbgPlain (DBG_SRD_SESS, _T("Physical location: %s."), pTape->PhysicalLocation);

                                pTape->LogicalLocation = StrNewCopy(ST_I (tapeInfo, FLD_CART_LOGLOCATION));
                                if(pTape->LogicalLocation == NULL)
                                    _LEAVE_;
                                DbgPlain (DBG_SRD_SESS, _T("Logical location: %s."), pTape->LogicalLocation);

                                pTape->StoreName = StrNewCopy(ST_I(tapeInfo, NR_CART + NR_MED + 2 - 1));
                                if(pTape->StoreName == NULL)
                                    _LEAVE_;
                                DbgPlain (DBG_SRD_SESS, _T("Store name: %s."), pTape->StoreName);
                            }

                            if ((strlen(pTape->StoreName)>0) && (pSess->DeviceIOCTL==NULL))
                            {
                                StrMsgMake(buff, buffLen, FUN_GETSTORE, pTape->StoreName, NULL);
                                storeInfo = StInit(NULL);

                                if(CsaWrite(dbId, buff, 0) == -1)
                                {
                                    SRD_STAMP(DBG_SRD_SESS);
                                    DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed"));
                                    _LEAVE_;
                                }
                                else if(SrdQueryEngine(dbId, storeInfo) == SRDERR_ERROR)
                                {
                                    SRD_STAMP(DBG_SRD_SESS);
                                    DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed on FUN_GETSTORE for %s"), pTape->StoreName);
                                    _LEAVE_;
                                }
                                else
                                {
                                    if(ST_N(storeInfo) <= 0)
                                        _LEAVE_DBG_(
                                            TRUE,
                                            Return, SRDERR_ERROR,
                                            DBG_SRD_SESS,
                                            { DbgPlain(DBG_SRD_SESS, _T("No return vector (ST_N(storeInfo) = %d)."), ST_N(storeInfo)); }
                                        );

                                    if(StoreParse(&StoreLibrary, ST_I(storeInfo, 0)) == -1)
                                        _LEAVE_DBG_(
                                            TRUE,
                                            Return, SRDERR_ERROR,
                                            DBG_SRD_SESS,
                                            { DbgPlain(DBG_SRD_SESS, _T("StoreParse() FAILED.")); }
                                        );

                                    if (pSess->DevicePolicy == POL_B2D &&
                                        pTape->PhysicalLocation != NULL
                                        )
                                    {
                                        R_TCHAR b2dPhysicalLocation[STRLEN_4K] = {0};

                                        B2DOptions *b2dOpt = 
                                            (B2DOptions *)CmnVecI(&(StoreLibrary.directory), 0);

                                        if (b2dOpt != NULL)
                                        {
                                            sprintf(b2dPhysicalLocation, _T("%s\\%s"), 
                                                b2dOpt->storeName, pTape->PhysicalLocation);

                                            FREE(pTape->PhysicalLocation);
                                            pTape->PhysicalLocation = StrNewCopy(b2dPhysicalLocation);

                                            DbgPlain (DBG_SRD_SESS, _T("BD2 physical location: %s."), pTape->PhysicalLocation);
                                        }
                                        else
                                        {
                                            DbgPlain (DBG_SRD_SESS, _T("Faild to get directory of B2D options."));
                                        }
                                    }

                                    if (StoreLibrary.control!=NULL)
                                    {
                                        pSess->DeviceIOCTL=StrNewCopy(StoreLibrary.control);
                                        DbgPlain (DBG_SRD_SESS, _T("Device IOCTL: %s.\n"), pSess->DeviceIOCTL);
                                    }
                                }
                                StDestroy (storeInfo);
                                storeInfo = NULL;
                            }

                            StDestroy (tapeInfo);
                            tapeInfo = NULL;
#endif
                        }
                    }

                    StDestroy (tapes);
                    tapes = NULL;
                    FreeStore(&StoreLibrary);
                }
                else
                {
                    SRD_STAMP(DBG_SRD_SESS);
                    DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
                    _LEAVE_;
                }
            }

            StDestroy (chain);
            chain = NULL;
        }
        else
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed for FUN_FBLISTFULLRESCHAIN || FUN_GETOVER -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        *pObjOut = pObject;

        /*---------------------------------------------------------------------------
        |   One and for all time sort sessions by their IDs from oldest to latest.
        |   In DP8.0 sessions are listed from latest to oldest. Since lot of existing
        |   code depends on the sessions being sorted from oldest to latest we sort
        |   the sessions, at parse time, in this order (QCCR2A40820). 
        ----------------------------------------------------------------------------*/

        qsort(
            &pObject->Sessions, pObject->NrSessions,
            sizeof(PSrdSession), CompareSessionStartTime
            );

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if (Return != SRDERR_SUCCESS)
            FreeSrdObject(pObject);

        if (chain)
            StDestroy(chain);
        if (tapes)
            StDestroy(tapes);
        if (tapeInfo)
            StDestroy(tapeInfo);
        if (storeInfo)
            StDestroy (storeInfo);
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

/* ===========================================================================
|
|   FUNCTION    SrdListObjects
|
 ========================================================================== */

R_RESULT
SrdListObjects(
    R_INT  dbid,
    R_LONG type,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdListObjects"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    DbgPlain (DBG_SRD_SESS, _T("Type filter: %ld."), type);

    _TRY_
        StrMsgMake(buff, buff_size, FUN_LISTOBJECTS, NULL);
        StrMsgFAppend(_T("%ld"), type);

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        {
            R_INT i = 0;

            R_INT obj_count = ST_N(chain) / NR_OBJ;
            DbgPlain (DBG_SRD_SESS, _T("Number of objects found: %d."), obj_count);
            for (i = 0; i < obj_count; i++)
            {
                R_TCHAR* obj_name = ST_I(chain, i * NR_OBJ + FLD_OBJ_OBJECTNAME);
                DbgPlain (DBG_SRD_SESS, _T("Object[%d]: \"%s\"."), i, obj_name);
            }
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdListObjectVersions
|
 ========================================================================== */

R_RESULT
SrdListObjectVersions(
    R_INT  dbid,
    R_TCHAR* name,
    R_INT  type,
    R_INT  valid,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdListObjectVersions"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    DbgPlain (DBG_SRD_SESS, _T("Name: %s."), name);
    DbgPlain (DBG_SRD_SESS, _T("Type: %d."), type);
    DbgPlain (DBG_SRD_SESS, _T("Valid: %d."), valid);

    _TRY_
        StrMsgMake(buff, buff_size, FUN_LISTVEROFOBJECT, NULL);
        StrMsgFAppend(_T("%s%d%d"), name, type, valid);

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdListAllObjectVersions
|
 ========================================================================== */

R_RESULT
SrdListAllObjectVersions(
    R_INT dbid,
    R_TCHAR* name,
    R_LONG flags,
    R_LONG start_time,
    R_LONG end_time,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdListAllObjectVersions"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    DbgPlain (DBG_SRD_SESS, _T("Name: %s."), name);
    DbgPlain (DBG_SRD_SESS, _T("Flags: %ld."), flags);
    DbgPlain (DBG_SRD_SESS, _T("Start: %ld."), start_time);
    DbgPlain (DBG_SRD_SESS, _T("End:   %ld."), end_time);

    _TRY_
        StrMsgMake(buff, buff_size, FUN_FBLISTALLOVER, NULL);
        StrMsgFAppend(_T("%s%ld%ld%ld%ld"), name, flags, 0, start_time, end_time);

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdListDirectory
|
 ========================================================================== */

R_RESULT
SrdListDirectory(
    R_INT  dbid,
    R_INT  level,
    R_TCHAR* dir,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdListDirectory"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    DbgPlain (DBG_SRD_SESS, _T("Level: %d."), level);
    DbgPlain (DBG_SRD_SESS, _T("Dir:   %s."), dir);

    _TRY_
        StrMsgMake(buff, buff_size, FUN_FBLISTDIR, NULL);
        StrMsgFAppend(_T("%d%s"), level, dir);

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdGetVersionsOfFile
|
 ========================================================================== */

R_RESULT
SrdGetVersionsOfFile(
    R_INT  dbid,
    R_INT  level,
    R_TCHAR* file,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdGetVersionsOfFile"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    DbgPlain (DBG_SRD_SESS, _T("Level: %d."), level);
    DbgPlain (DBG_SRD_SESS, _T("File:  %s."), file);

    _TRY_
        StrMsgMake(buff, buff_size, FUN_FBLISTFILEVERS, NULL);
        StrMsgFAppend(_T("%d%s"), level, file);

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdGetDeviceData
|
========================================================================== */
static R_RESULT
SrdGetDeviceData(
    R_INT dbid,
    R_INT func,
    StPtr output
    )
{
    ERH_FUNCTION (_T("SrdGetDeviceData"));

    R_RESULT result = SRDERR_ERROR;
    StPtr    chain = NULL;
    R_DWORD  buff_size = STRLEN_PATH + STRLEN_STD;
    R_TCHAR  buff[STRLEN_PATH + STRLEN_STD];
    R_INT ii = 0;

    SRD_ENTER_FCN;

    _TRY_
        StrMsgMake(buff, buff_size, func, NULL);
        switch (func)
        {
        case FUN_LISTSTORES:
            StrMsgAppend(_T("-1"));
            /* Fall through expected */
        default:
            StrMsgAppend(_T("-1"));
            break;
        }

        if (CsaWrite(dbid, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        chain = StInit(NULL);

        if (SrdQueryEngine(dbid, chain) == SRDERR_ERROR)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        for (ii = 0; ii < StLen(chain); ii++)
        {
            StAdd(output, StGet(chain, ii));
        }

        result = SRDERR_SUCCESS;
    _FINALLY_
        if (chain)
            StDestroy(chain);
    _ENDTRY_

    return result;
}

/* ===========================================================================
|
|   FUNCTION    SrdGetDevices
|
 ========================================================================== */

R_RESULT
SrdGetDevices(
    R_INT  dbid,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdGetDevices"));

    SRD_ENTER_FCN;

    return SrdGetDeviceData(dbid, FUN_LISTDEVICES, output);
}

/* ===========================================================================
|
|   FUNCTION    SrdGetStores
|
 ========================================================================== */

R_RESULT
SrdGetStores(
    R_INT  dbid,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdGetStores"));

    SRD_ENTER_FCN;

    return SrdGetDeviceData(dbid, FUN_LISTSTORES, output);
}

/* ===========================================================================
|
|   FUNCTION    SrdGetPools
|
 ========================================================================== */

R_RESULT
SrdGetPools(
    R_INT  dbid,
    StPtr  output
    )
{
    ERH_FUNCTION (_T("SrdGetPools"));

    SRD_ENTER_FCN;

    return SrdGetDeviceData(dbid, FUN_LISTPOOLS, output);
}


/* ===========================================================================
|
|   FUNCTION    FindIDBSessionObjects
|
 ========================================================================== */
R_RESULT
SrdFindIDBSessionObjects(R_INT dbId,
                         StPtr queryOutput,
                         R_HANDLE hData,
                         CmnVec *sessionIds)
{
    ERH_FUNCTION (_T("FindIDBSessionObjects"));

    R_RESULT    retVal = SRDERR_ERROR;
    const       R_INT buffLen = STRLEN_PATH + STRLEN_STD;
    R_TCHAR     buff[STRLEN_PATH + STRLEN_STD];
    StPtr       chain = NULL;
    R_TCHAR     host[STRLEN_HOSTNAME];
    R_TCHAR     volume[STRLEN_PATH];
    R_TCHAR     label[STRLEN_DESCRIPTION];
    R_INT       type = 0;
    PSrdObject  pObject = NULL;
    R_TCHAR     *objVersion = NULL;
    R_TCHAR     *objName = NULL;
    R_INT       jj = 0;
    R_ULONG     nrObjectFields = NR_OBJ + NR_OV;
    R_INT       loopCount = ST_N(queryOutput) / nrObjectFields;
    PSRDDATA    pSrdData = (PSRDDATA)hData;
    R_INT       confVolumeFound = FALSE;

    SRD_ENTER_FCN;

    /* Find object called ConfigurationFiles that and add it to SRD */
    for (jj = 0; jj < loopCount; jj++)
    {
        objName = ST_I(queryOutput, jj * (R_INT)nrObjectFields);

        StrFromObjectName (&type, host, volume, label, objName);

        if (strcmp(volume, POSTGRES_MPOINT_CONFFILES) == 0)
        {
            objVersion = StrNewCopy(ST_I(queryOutput, jj * (int) nrObjectFields + NR_OBJ + FLD_OV_ID));
            AddRestVol(&pSrdData->RestoreVolumes, volume, OB2DB_FILES, OT_OB2BAR);
            confVolumeFound = TRUE;
            
            break;
        }
    }

    if (!confVolumeFound)
    {
        SRD_STAMP(DBG_SRD_SESS);
        DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed to find IDB objects (IDB-Conf: %d)."), confVolumeFound);
        retVal = SRDERR_ERROR;
        _LEAVE_;
    }

    if (objVersion == NULL)
    {
        SRD_STAMP(DBG_SRD_SESS);
        DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed for FUN_FBLISTFULLRESCHAIN -> [%s]."), ErhErrnoToText());
        retVal = SRDERR_ERROR;
        _LEAVE_;
    }

    DbgPlain (DBG_SRD_SESS, _T("Object name: %s."), objName);
    DbgPlain (DBG_SRD_SESS, _T("Object version ID: %s."), objVersion);

    StrFromObjectName(&type, host, volume, label, objName);

    _TRY_
        if (type == OT_OB2BAR)
        {
            StrMsgMake(buff, buffLen, FUN_FBLISTALLOVER, NULL);
            StrMsgFAppend(_T("%s%ld%ld%ld%ld"), objName, DBA_FBMULTIPLE | DBA_FBTREES, 0, 0, 0);

            if (CsaWrite(dbId, buff, 0) == -1)
            {
                SRD_STAMP(DBG_SRD_SESS);
                DbgPlain (DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
                _LEAVE_;
            }

            chain = StInit(NULL);

            if (SrdQueryEngine(dbId, chain) == SRDERR_ERROR)
            {
                SRD_STAMP(DBG_SRD_SESS);
                DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed -> [%s]."), ErhErrnoToText());
                _LEAVE_;
            }

            StDestroy(chain);
            chain = NULL;

            StrMsgMake(buff, buffLen, FUN_FBLISTFULLRESCHAIN, objVersion, _T(""), NULL);
        }

        if (CsaWrite(dbId, buff, 0) == -1)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain(DBG_SRD_SESS, _T("Write to CRS failed -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        pObject = MALLOC(sizeof(SrdObject));
        if (pObject == NULL)
        {
            _LEAVE_;
        }
        memset (pObject, 0, sizeof(SrdObject));
        pObject->ObjHost = StrNewCopy(host); /* parasoft-suppress  PB-07 "Same types. R_TCHAR" */
        pObject->Label = StrNewCopy(label); /* parasoft-suppress  PB-07 "Same types. R_TCHAR" */
        if (pObject->Label == NULL)
        {
            _LEAVE_;
        }

        chain = StInit (NULL);

        if (SrdQueryEngine (dbId, chain) != SRDERR_ERROR)
        {
            R_INT       ii = 0;
            R_INT       nrFld = NR_OV + 1;
            R_INT       nrSessions = ST_N (chain) / nrFld;

            R_INT noOfWalStreams = 0;
            R_INT noOfDCBFCounter = 0;
            R_INT noOfDPSPECCounter = 0;
            R_INT noOfSMBFCounter = 0;

            R_INT kk = 0;
            R_TCHAR pszWalNameBuff[STRLEN_PATH] = _T("");
            R_TCHAR pszDCBFNameBuff[STRLEN_PATH] = _T("");
            R_TCHAR pszDPSPECNameBuff[STRLEN_PATH] = _T("");
            R_TCHAR pszSMBFNameBuff[STRLEN_PATH] = _T("");
            for (ii = 0; ii < nrSessions; ii++)
            {
                R_TCHAR     queryBuffer[STRLEN_STD + 1] = _T("");
                PSrdSession pSess          = NULL;
                StPtr       tmpQueryOutput = StInit(NULL);
                R_INT       tmpLoopCount   = 0;
                R_INT       tmpWalCounter  = 0;
                R_INT       tmpDCBFCounter = 0;
                R_INT       tmpDPSPECCounter = 0;
                R_INT       tmpSMBFCounter = 0;

                /* ---------------------------------------------------------------
                |   initialize session
                 ---------------------------------------------------------------*/
                if (pObject->NrSessions != 0)
                {
                    pObject = REALLOC(pObject, sizeof(SrdObject) + pObject->NrSessions * sizeof(PSrdSession));
                }

                if (pObject == NULL)
                {
                    _LEAVE_;
                }
                pObject->Sessions[pObject->NrSessions] = MALLOC(sizeof(SrdSession));
                if (pObject->Sessions[pObject->NrSessions] == NULL)
                {
                    _LEAVE_;
                }

                memset(pObject->Sessions[pObject->NrSessions], 0, sizeof(SrdSession));
                pSess = pObject->Sessions[pObject->NrSessions];
                pObject->NrSessions += 1;

                pSess->Id = (ST_I(chain, ii * nrFld + NR_OV));
                if (pSess->Id == NULL)
                {
                    _LEAVE_;
                }
                DbgPlain (DBG_SRD_SESS, _T("Session ID: %s.\n"), pSess->Id);

                StrMsgMake(queryBuffer, STRLEN_STD, FUN_LISTOVEROFSESSION, pSess->Id, NULL);
                if (CsaWrite (dbId, queryBuffer, 0) == -1)
                {
                    SRD_STAMP(DBG_SRD_SESS);
                    retVal = SRDERR_ERROR;
                    DbgPlain (DBG_SRD_SESS, _T("Cannot write to DBSM, error description:\n%s"), ErhErrnoToText());
                    _LEAVE_;
                }
                else if (SrdQueryEngine (dbId, tmpQueryOutput) == -1)
                {
                    SRD_STAMP(DBG_SRD_SESS);
                    retVal = SRDERR_ERROR;
                    DbgPlain (DBG_SRD_SESS, _T("Error querying database, error description:\n%s"), ErhErrnoToText());
                    _LEAVE_;
                }

                CmnVecAddStr(sessionIds, pSess->Id);

                tmpLoopCount = ST_N(tmpQueryOutput) / nrObjectFields;

                for (jj = 0; jj < tmpLoopCount; jj++)
                {
                    R_TCHAR *objFlag = NULL;

                    objName = ST_I(tmpQueryOutput, jj * (R_INT)nrObjectFields);

                    StrFromObjectName(&type, host, volume, label, objName);

                    objFlag = StrNewCopy(ST_I(tmpQueryOutput, jj * (int) nrObjectFields + NR_OBJ + FLD_OV_FLAGS));
                    if (objFlag != NULL && (atoi(objFlag) & OVF_COPY) != 0)
                    {
                        FREE(objFlag);
                        continue;
                    }

                    /* Wee need to determine max no ID in all incremental and 
                       then add them into SRD */

                    if (StrStr(volume, POSTGRES_MPOINT_WALS) != NULL)
                    {
                        tmpWalCounter++;
                    }
                    else if (StrStr(volume, POSTGRES_MPOINT_DATAFILES) != NULL)
                    {
                        AddRestVol(&pSrdData->RestoreVolumes, volume, OB2DB_FILES, OT_OB2BAR);
                    }
                    else if (StrStr(volume, POSTGRES_MPOINT_DCBFS) != NULL)
                    {
                        tmpDCBFCounter++;
                    }
                    else if (StrStr(volume, POSTGRES_MPOINT_DPSPEC) != NULL)
                    {
                        tmpDPSPECCounter++;
                    }
                    else if (StrStr(volume, POSTGRES_MPOINT_SMBFS) != NULL)
                    {
                        tmpSMBFCounter++;
                    }
                }

                if (tmpWalCounter > noOfWalStreams)
                {
                    /* If current session has more wals then we need to save that info */
                    noOfWalStreams = tmpWalCounter;
                }

                if (tmpDCBFCounter > noOfDCBFCounter)
                {
                    /* If current session has more DCBFs then we need to save that info */
                    noOfDCBFCounter = tmpDCBFCounter;
                }

                if (tmpDPSPECCounter > noOfDPSPECCounter)
                {
                    /* If current session has more DPCPECs then we need to save that info */
                    noOfDPSPECCounter = tmpDPSPECCounter;
                }

                if (tmpSMBFCounter > noOfSMBFCounter)
                {
                    /* If current session has more SMBFs then we need to save that info */
                    noOfSMBFCounter = tmpSMBFCounter;
                }

                DbgPlain (DBG_SRD_SESS, _T("Current no of WALS is: %d."), noOfWalStreams);
                DbgPlain (DBG_SRD_SESS, _T("Current no of DCBF is: %d."), noOfDCBFCounter);
                DbgPlain (DBG_SRD_SESS, _T("Current no of DPSPEC is: %d."), noOfDPSPECCounter);
                DbgPlain (DBG_SRD_SESS, _T("Current no of SMBF is: %d."), noOfSMBFCounter);
            }

            DbgPlain (DBG_SRD_SESS, _T("Adding %d of WALS."), noOfWalStreams);
            DbgPlain (DBG_SRD_SESS, _T("Adding %d of DCBF."), noOfDCBFCounter);
            DbgPlain (DBG_SRD_SESS, _T("Adding %d of DPSPEC."), noOfDPSPECCounter);
            DbgPlain (DBG_SRD_SESS, _T("Adding %d of SMBF."), noOfSMBFCounter);

            for (kk = 0; kk < noOfWalStreams; kk++)
            {
                sprintf(pszWalNameBuff, _T("%s:%d"), POSTGRES_MPOINT_WALS, kk);
                AddRestVol(&pSrdData->RestoreVolumes, pszWalNameBuff, OB2DB_FILES, OT_OB2BAR);
            }

            for (kk = 0; kk < noOfDCBFCounter; kk++)
            {
                sprintf(pszDCBFNameBuff, _T("%s:%d"), POSTGRES_MPOINT_DCBFS, kk);
                AddRestVol(&pSrdData->RestoreVolumes, pszDCBFNameBuff, OB2DB_FILES, OT_OB2BAR);
            }

            for (kk = 0; kk < noOfDPSPECCounter; kk++)
            {
                sprintf(pszDPSPECNameBuff, _T("%s:%d"), POSTGRES_MPOINT_DPSPEC, kk);
                AddRestVol(&pSrdData->RestoreVolumes, pszDPSPECNameBuff, OB2DB_FILES, OT_OB2BAR);
            }

            for (kk = 0; kk < noOfSMBFCounter; kk++)
            {
                sprintf(pszSMBFNameBuff, _T("%s:%d"), POSTGRES_MPOINT_SMBFS, kk);
                AddRestVol(&pSrdData->RestoreVolumes, pszSMBFNameBuff, OB2DB_FILES, OT_OB2BAR);
            }

            StDestroy (chain);
            chain = NULL;
        }
        else
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed for FUN_FBLISTFULLRESCHAIN -> [%s]."), ErhErrnoToText());
            _LEAVE_;
        }

        retVal = SRDERR_SUCCESS;

        _FINALLY_
        if (chain)
        {
            StDestroy(chain);
        }
    _ENDTRY_

    SRD_RETURN_VAL(retVal); /* parasoft-suppress  NAMING-33_duplicated_1 "Old code." */ /* parasoft-suppress  NAMING-05_duplicated_1 "Old code." */
}

R_RESULT
SrdMarkIDBVolumes(StPtr queryOutput, R_HANDLE hData)
{
    ERH_FUNCTION (_T("SrdMarkIDBVolumes"));

    R_RESULT    retVal = SRDERR_ERROR;
    R_TCHAR     host[STRLEN_HOSTNAME];
    R_TCHAR     volume[STRLEN_PATH];
    R_TCHAR     label[STRLEN_DESCRIPTION];
    R_INT       type = 0;
    R_TCHAR     *objName = NULL;
    R_INT       jj = 0;
    R_ULONG     nrObjectFields = NR_OBJ + NR_OV;
    R_INT       loopCount = ST_N(queryOutput) / nrObjectFields;
    PSRDDATA    pSrdData = (PSRDDATA)hData;
    R_INT       idbVolumeFound = FALSE;

    SRD_ENTER_FCN;

    _TRY_
        if (IS_LINUX(pSrdData->Header->Platform))
        {
            DbgPlain (DBG_SRD_SESS, _T("Skipping IDB volume marking for Linux CM.\n"));
            retVal = SRDERR_SUCCESS;
            _LEAVE_;
        }

        /* Find object called ConfigurationFiles that and add it to SRD */
        for (jj = 0; jj < loopCount; jj++)
        {
            objName = ST_I(queryOutput, jj * (R_INT)nrObjectFields);

            StrFromObjectName (&type, host, volume, label, objName);

            /* During update we need to set volume purpose for the IDB volume. 
               We get this information from external options for the specified IDB
               session. POSTGRES_MPOINT_DATAFILES
            */

            if (StrStr(volume, POSTGRES_MPOINT_DATAFILES) != NULL)
            {
                R_TCHAR     *ovOpt = NULL;
                DbgPlain (DBG_SRD_SESS, _T("Searching for the volume where IDB is located.\n"));
                ovOpt = StrNewCopy(ST_I(queryOutput, jj * (int) nrObjectFields + NR_OBJ + FLD_OV_OPT));

                if(ovOpt != NULL)
                {
                    R_TCHAR *baropt = strstr(ovOpt, _T("-baropt"));

                    DbgPlain (DBG_SRD_SESS, _T("Object version options: %s.\n"), ovOpt);
                    if (baropt != NULL)
                    {
                        R_TCHAR dataPathMount[STRLEN_2K] = {0};
                        R_TCHAR *token = NULL;
                        R_TCHAR *token_end = NULL;

                        token = strstr(baropt, _T("/"));
                        if (token != NULL)
                        {
                            R_UINT ii = 0;
                            R_UINT vol_len = 0;
                            R_INT vol_idx = -1;

                            // Take care of drive letters and mount points
                            for (ii = 0; ii < pSrdData->RestoreVolumes->RestVolCount; ii++)
                            {
                                R_UINT lenght = (R_UINT)strlen(pSrdData->RestoreVolumes->RestoreVolumes[ii]->VolumeName);
                                if (strncmp(pSrdData->RestoreVolumes->RestoreVolumes[ii]->VolumeName, token, lenght) == 0 &&
                                    lenght > vol_len)
                                {
                                    vol_len = lenght;
                                    vol_idx = ii;
                                }
                            }

                            if (vol_idx >= 0)
                            {
                                DbgPlain (DBG_SRD_SESS, _T("IDB volume is: %s.\n"),
                                    pSrdData->RestoreVolumes->RestoreVolumes[vol_idx]->VolumeName);

                                memset(dataPathMount, 0, sizeof(R_TCHAR) * STRLEN_2K);
                                strncpy(dataPathMount, pSrdData->RestoreVolumes->RestoreVolumes[vol_idx]->VolumeName,
                                    strlen(pSrdData->RestoreVolumes->RestoreVolumes[vol_idx]->VolumeName));

                                AddRestVol(&pSrdData->RestoreVolumes, dataPathMount, 
                                    OB2DB_VOLUME, pSrdData->RestoreVolumes->RestoreVolumes[vol_idx]->VolumeType);

                                token_end = strstr(token, _T("'"));
                                if (token_end != NULL)
                                {
                                    strncpy(dataPathMount, token + 1, token_end - token - 1);
                                    dataPathMount[token_end - token - 1] = _T('\0');

                                    pSrdData->SysInfo->ObIDB = StrNewCopy(dataPathMount);
                                }
                                else
                                {
                                    DbgPlain (DBG_SRD_SESS, _T("Failed to parse IDB directory\n."));
                                }

                                idbVolumeFound = TRUE;
                            }
                        }
                    }
                    FREE(ovOpt);
                }
            }

            if (idbVolumeFound)
            {
                break;
            }
        }

        if (!idbVolumeFound)
        {
            SRD_STAMP(DBG_SRD_SESS);
            DbgPlain (DBG_SRD_SESS, _T("SrdQueryEngine failed to find IDB object."));
            retVal = SRDERR_ERROR;
            _LEAVE_;
        }

        retVal = SRDERR_SUCCESS;
    _FINALLY_

    _ENDTRY_

    SRD_RETURN_VAL(retVal);
}
