/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   SSEA_API
* @file      integ/smb/sse/sseapi.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Split Mirror Backup CLI Option Parser
*
* @since     ...
*/

#include "ssermlib.h" /* This should be included before target.h */
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/smb/sse/sseapi.c $Rev$ $Date::                      $:") ;
#endif 

/* System includes */

/* Application specific includes */

/* OB common includes */
#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "integ/smb/libsmb.h"
#include "integ/smb/smb.h"
#include "lib/smbdb/dbxp/libdbxp.h"
#include "../prepare.h"
#include "../smbcmn.h"
#include "../libsmbdbsys2.h"
#include "lib/xtra/scramble.h"

/* Module includes */
#include "sse.h"
#include "ssedef.h"
#include "ssedbxp.h"
#include "optmgr.h"
#include "sseapi.h"
#include "ssecmn.h"
#include "ssex.h"
#include "smbmsg.h"

#include "nls.h"
#include "debug.h"

USES_CONVERSION

extern optRec  opt;  
 
#define SMB_MSG_LINES 10
#define DEV_LIST_INCR 512
#define SSEA_ATTACH_RETRY 5
#define SSEA_ATTACH_SLEEP_TIME 10

#define SCSI_VENDOR_ID_HP            _T("HP")
#define SCSI_VENDOR_ID_HP_LEN            2
#define SCSI_VENDOR_ID_HITACHI        _T("HITACHI")
#define SCSI_VENDOR_ID_HITACHI_LEN      7

#define CheckVendorId(x)		(strncmp((x), SCSI_VENDOR_ID_HP, SCSI_VENDOR_ID_HP_LEN) == 0 || \
					strncmp((x), SCSI_VENDOR_ID_HITACHI, SCSI_VENDOR_ID_HITACHI_LEN) == 0)

#define RMLIB_CSIZE 15

#define SSEA_VERIFY(x)                                                                  \
    if (!(x))                                                                           \
    {                                                                                   \
        DbgStamp (DBG_UNEXPECTED);                                                      \
        DbgPlain (DBG_UNEXPECTED, _T("Verification failed: \"%s\"\n\n"), _T(#x));       \
        retVal = RETURN_ER;                                                             \
        goto end;                                                                       \
    }

int
SseGetCommandDevice (
    int    iSerialNum,
    xpDevice_t **device,
    int *numb
)
{
    int retVal = RETURN_OK;
    PSMB_RAW_DISK_T *pRdskList = NULL;
    int              iRdskCount = 0;
    PSMB_RAW_DISK_T *pRcdiskList = NULL;
    int              iRcdsikCount = 0;
    PSMB_RAW_DISK_T pRdsk;
    SSE_DEVICE_T Ldev;
    int i;
    int dbStatus = 0;
    int p = 0;
    int countDb = 0;
    xpDevice_t *device1 = NULL;
    xpDevice_t *device2 = NULL;
    tchar *name;
    int skip = -2;
    tchar* var;
    int iStaticCMD = 0;

    ERH_FUNCTION (_T("SseGetCommandDevice"));    
    
    DbgFcnInEx(__FLND__, _T("iSerialNum = %d"), iSerialNum);

    *device = NULL;
    *numb = 0;
    name = (tchar *) MALLOC(DEV_LIST_INCR);
    name[0] = _T('\0');

/*
    QXCR1000769604: at HP_UX 11.31, it is possible to disable legacy DSF (e.g. /dev/[r]dsk/c#t#d#),
    therefore command devices will only be searched among new DSF device files (e.g. /dev/[r]disk/disk###)
    Prerok: Refactored this part to remove the need for #if's. All HP-UX will try to search among agile
            DSFs but if nothing is returned then we fall back to legacy naming convention.
*/
#if TARGET_HPUX
    {
        int cdsf_retVal = GetAllRdsksEx (&pRcdiskList, &iRcdsikCount, 2);
        retVal = GetAllRdsksEx (&pRdskList, &iRdskCount, 1);


        if ( (RETURN_ER == retVal && RETURN_ER == cdsf_retVal) || (0 == iRdskCount && 0 == iRcdsikCount))
        {
            retVal = GetAllRdsksEx (&pRdskList, &iRdskCount, 0);

            if ( RETURN_ER == retVal || 0 == iRdskCount )
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, _T("[%s] GetAllRdsksEx failed!"), __FCN__);
                retVal = RETURN_ER;
                goto end;
            }
        }
    }
#else
    if((retVal = GetAllRdsks (&pRdskList, &iRdskCount)) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] GetAllRdsks failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }
#endif

    if ((var=(tchar*)GetEnv(_T("SSEA_QUERY_STORED_CMDDEVS")))!=NULL)
    {
        iStaticCMD = atoi (var);
    }

    /* check if cmddev is in dbxp */
    if ((retVal=dbXP_qryCmDevice (iSerialNum, opt.hostname, &device2, numb))!= RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_qryCmDevice failed!"), __FCN__);
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_DBXP_CMDEVICE_FAILED, ErhErrnoToText ()));
        
        ErhClearError();

        retVal = RETURN_ER;
        goto end;
    }
    if (*numb)
    {
        /* Cmd devices are in dbXP */
        dbStatus = 1;
    }
   
    /* Get dynamic CMD devs at all times except when the OMNIRC variable tells us not to.  Also 
       get dynamic devs when there are no IRDB entries. */
    if (!iStaticCMD || !dbStatus)
    {
        device1 = (xpDevice_t *) MALLOC( sizeof(xpDevice_t));

        for (i=0; i < (iRdskCount + iRcdsikCount); i++)
        {
            /* Getting raw disk in both new DSF and cDSF format */ 
            if (i < iRdskCount)
                pRdsk = pRdskList[i];
            else
                pRdsk = pRcdiskList[i - iRdskCount];

            DbgPrintRdsk (pRdsk);

            if (pRdsk->status != RDSK_OK)
                continue;

            if (CheckVendorId(pRdsk->tsVendorId) && 
                strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0 &&
                strstr(pRdsk->tsProductId, _T("-CM")) != NULL)
            {
                DbgStamp (DBG_SSE_3DAPI);
                if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
                {
                    /* Error */
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] ScsiInquiryXp failed for %s!"),
#if TARGET_SOLARIS
                        __FCN__, pRdsk->tsSliceName);
#else
                        __FCN__, pRdsk->tsRdskName);
#endif

                }
                if (Ldev.Dev.serial == iSerialNum)
                {
#if TARGET_SOLARIS
                    sprintf (device1->tsCmd, _T("/dev/rdsk/%s"), pRdsk->tsSliceName);
#elif TARGET_WIN32
                    sprintf (device1->tsCmd, _T("\\\\.\\%s"), pRdsk->tsRdskName);
#elif TARGET_HPUX
                    StrCpy (device1->tsCmd, pRdsk->tsRdskName);
#elif TARGET_LINUX
                    sprintf (device1->tsCmd, _T("/dev/%s"), pRdsk->tsRdskName);
#endif
                    device1->serial = iSerialNum;
                    device1->ldev = Ldev.Dev.ldevno;
                    device1->instance = -1;
                    StrCpy( device1->system, opt.hostname);
                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                    _T("[%s] Device %s (%s, %s, %d, %d ) was found to be a possible  command device for %u!"),
                    __FCN__, device1->tsCmd, pRdsk->tsVendorId,
                    pRdsk->tsProductId, *numb, device1->ldev, iSerialNum);

                    if ((retVal = dbXP_addCmDevice(device1)) != RETURN_OK)
                    {
                        /*ERROR HANDLING */

                        /*check if hostname already exist*/
                        if (retVal == skip)
                            continue;
                        
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                           _T("[%s] dbXP_addCmDevice failed!"), __FCN__);

                        retVal = RETURN_ER;
                        goto end;
                    }
                }
            }
        }
    }

    /* There are now either updated/dynamically added entries, or just the existing entries. */
    if ((retVal=dbXP_qryCmDevice (iSerialNum, opt.hostname, device, numb))!= RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_qryCmDevice failed!"), __FCN__);
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_DBXP_CMDEVICE_FAILED, ErhErrnoToText ()));
        
        ErhClearError();

        retVal = RETURN_ER;
        goto end;
    }
    if (*numb)
    {
        for ( countDb=0; countDb < *numb; countDb++)
            (*device)[countDb].tsCmd[0] = _T('\0');

        for (i=0; i<iRdskCount; i++)
        {
            pRdsk = pRdskList[i];

            DbgPrintRdsk (pRdsk);

            if (pRdsk->status != RDSK_OK)
                continue;

            if (CheckVendorId(pRdsk->tsVendorId) && 
                strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0 &&
                strstr(pRdsk->tsProductId, _T("-CM")) != NULL)
            {
                DbgStamp (DBG_SSE_3DAPI);
                if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
                {
                    /* Error */
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] ScsiInquiryXp failed for %s!"),
#if TARGET_SOLARIS
                        __FCN__, pRdsk->tsSliceName);
#else
                        __FCN__, pRdsk->tsRdskName);
#endif
                }
                if (Ldev.Dev.serial == iSerialNum)
                {
                    /* if cmd devices are in dbXP */
                    for ( countDb=0; countDb < *numb; countDb++)
                    {
                        if ( ((*device)[countDb].ldev == Ldev.Dev.ldevno) &&
                              ((*device)[countDb].tsCmd[0] == _T('\0')) )
                        {
#if TARGET_SOLARIS
                            sprintf ((*device)[countDb].tsCmd, _T("/dev/rdsk/%s"), pRdsk->tsSliceName);
#elif TARGET_WIN32
                            sprintf ((*device)[countDb].tsCmd, _T("\\\\.\\%s"), pRdsk->tsRdskName);
#elif TARGET_HPUX
                            StrCpy ((*device)[countDb].tsCmd, pRdsk->tsRdskName);
#elif TARGET_LINUX
                            sprintf ((*device)[countDb].tsCmd, _T("/dev/%s"), pRdsk->tsRdskName);
#endif
                            DbgStamp (DBG_SSE_3DAPI);
                            DbgPlain (DBG_SSE_3DAPI,
                               _T("[%s] Device %s (%s, %s, %d, %d ) was found to be a possible  command device for %u!"),
                               __FCN__, (*device)[countDb].tsCmd, pRdsk->tsVendorId,
                               pRdsk->tsProductId,  countDb, (*device)[countDb].ldev ,iSerialNum);
                            p++;
                            if (p == *numb)
                                goto end;
                            break;
                         }
                     }
                 }
            }
        }
    }
    else
    {
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE, NLS_SSE_FIND_COMMAND_DEVICE_FAILED));
        ErhClearError ();
        retVal = RETURN_ER;
        goto end;
    }
    
    for (i=0; i< *numb; i++)
    {
        DbgPlain (DBG_SSE_3DAPI,
            _T("[%s] Command device[%d]: %s!"), __FCN__, i, (*device)[i].tsCmd);

        if ( strlen(NS((*device)[i].tsCmd)) == 0 )
        {
            DbgStamp (DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI,
                      _T("[%s] Cannot find command device for LDev %d!"), __FCN__,(*device)[i].ldev);

            /* Commenting out the warning as this is no longer applicable.
             * In a cluster environment after a failover, 
             * we have command devices from 2 different nodes under one virtual name,
             * hence this warning would always be printed after a failover */
            /*ErhFullReport (__FCN__, ERH_WARNING, NlsGetMessage (NLS_SET_SSE,
                NLS_SSE_GET_COMMAND_DEVICE_FAILED,(*device)[i].ldev));
            ErhClearError ();*/

            /*retVal = RETURN_ER;
              goto end;*/
        }
    }
   
end:
    FREE(name);
    FREE(device1);
    FREE(device2);
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseGetCommandDevice */

int
SseMapRdskToLdev (
    PSMB_RAW_DISK_T     *pRdsk,
    PSSE_DEVICE_T       *pSseDevice,
    int                 iCount
)
{
    int    retVal = RETURN_OK;
    int    i=0;
   
    ERH_FUNCTION (_T("SseMapRdskToLdev"));    

    DbgFcnIn();

    if ((pRdsk == NULL || pSseDevice == NULL) && iCount > 0)
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    for (i = 0; i < iCount; i ++) 
    {
        do
        {
            if (pRdsk[i]->status == RDSK_OK)
            {
                pSseDevice[i]->iStatus = LDEV_OK;
                pSseDevice[i]->iIndex = i;


                if ((retVal=(ScsiInquiry (pRdsk[i]))) != RETURN_OK)
                {
                    tchar  *token; 
                    tchar  tsAlterPath [STRLEN_PATH + 1];
                                                            
                    strcpy(tsAlterPath,pRdsk[i]->alternate_paths);                       
                    
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                              _T("[%s] ScsiInquiry failed. Try with alternate_paths = %s!"),
                              __FCN__,
                              NS(tsAlterPath));
                    

                    token=strtok(tsAlterPath,_T(","));
                    
                    while((token!=NULL)&&(retVal!=RETURN_OK))
                    {
                        /*using alternate_paths*/
#if TARGET_SOLARIS 
                        strcpy(pRdsk[i]->tsSliceName,token);
#else
                        strcpy(pRdsk[i]->tsRdskName,token);
                        
#endif
                                                
                        if ((retVal=(ScsiInquiry (pRdsk[i]))) != RETURN_OK)
                        {
                            ErhClearError();
                            DbgStamp (DBG_SSE_ACTION);
                            DbgPlain (DBG_SSE_ACTION, 
                             _T("[%s] ScsiInquiry failed for alternate_path=%s so try next alternate_path...!"),
                                      __FCN__,
                                      token);
                        }
                        
                        token=strtok(NULL,_T(","));
                        
                    } /* end of while */
                    
                }

                if (retVal!=RETURN_OK)           
                {
                    pRdsk[i]->status = RDSK_ER;
                    pSseDevice[i]->iStatus = LDEV_INVALID;

                    /* Error already marked */
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] ScsiInquiry failed!"),
                        __FCN__);

                    break;
                }

#if TARGET_HPUX
				if (strstr(pRdsk[i]->tsProductId, _T("OPEN")) != NULL)
                {
#endif
					DbgPlain (DBG_MAIN_MSG, 
                              _T("[%s] Disk is HP OPEN (XP) - (tsVendorId = %s, tsProductId = %s) - running ScsiInquiryXp"),
                              __FCN__,
                              pRdsk[i]->tsVendorId, 
                              pRdsk[i]->tsProductId);
                    if (ScsiInquiryXp (pRdsk[i], pSseDevice[i]) != RETURN_OK)
                    {
                    pRdsk[i]->status = RDSK_ER;
                        pSseDevice[i]->iStatus = LDEV_INVALID;

                        /* Error already marked */
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                            _T("[%s] ScsiInquiryXp failed!"),
                            __FCN__);

                        break;
                    }
#if TARGET_HPUX
                } 
#endif
                    
                if ( (CheckVendorId(pRdsk[i]->tsVendorId) == 0) ||
                    (strncmp(pRdsk[i]->tsProductId, _T("OPEN"), 4) != 0))
                {
                    pRdsk[i]->status = RDSK_ER;
                    pSseDevice[i]->iStatus = LDEV_INVALID;

                    ErhMark (ERH_SSE_NOT_HP_OPEN, __FCN__, ERH_MAJOR);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] Disk %s is not HP OPEN (XP256) but it is in split mirror backup! (tsVendorId = %s, tsProductId = %s)"),
                        __FCN__, pRdsk[i]->tsRdskName, pRdsk[i]->tsVendorId, 
                        pRdsk[i]->tsProductId);
                    break;
                }

                if (strstr(pRdsk[i]->tsProductId, _T("-CM")) != NULL)
                {
                    pRdsk[i]->status = RDSK_ER;
                    pSseDevice[i]->iStatus = LDEV_INVALID;

                    ErhMark (ERH_SSE_HP_OPEN_CMD, __FCN__, ERH_MAJOR);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] Disk %s is XP256 command device but it is in split mirror backup! (tsProductId = %s)"),
                        __FCN__, pRdsk[i]->tsRdskName, pRdsk[i]->tsProductId);

                    /* ErhMark (ERH_SSE_NOT_HP_OPEN, __FCN__, ERH_MAJOR); */

                    break;
                }

                pSseDevice[i]->Dev.targid = pRdsk[i]->iTarget;
                pSseDevice[i]->Dev.lun = pRdsk[i]->iLun;
            }
        }
        while (0);
#if TARGET_HPUX
		if (strstr(pRdsk[i]->tsProductId, _T("OPEN")) != NULL)
        {
                DbgPlain (DBG_SSE_3DAPI, _T("[%s] XP:pRdsk[i]->tsProductId[%s]"), __FCN__,pRdsk[i]->tsProductId);
                if (pRdsk[i]->status != RDSK_OK)
                {
                        ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage (
                                NLS_SET_SSE, NLS_SSE_MAP_RDSK_2_LDEV_OBJ_FAILED,
                                pRdsk[i]->tsRdskName,
                                ErhErrnoToText ()));
                        ErhClearError();
                }
        }
        else{
                DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdsk[i]->tsProductId[%s]"), __FCN__,pRdsk[i]->tsProductId);
                ErhFullReport(__FCN__, ERH_WARNING, NlsGetMessage (
                                NLS_SET_SSE, NLS_SSE_MAP_RDSK_2_LDEV_OBJ_FAILED,
                                pRdsk[i]->tsRdskName,
                                ErhErrnoToText ()));
        }
#else
        if (pRdsk[i]->status != RDSK_OK)
        {
            ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_MAP_RDSK_2_LDEV_OBJ_FAILED,
                    pRdsk[i]->tsRdskName,
                    ErhErrnoToText ()));
            ErhClearError();
       }
#endif 
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseMapRdskToLdev */

int
SseResolveLdev (
    cmddevinfo         **pCmdDev,
    int                  iCmdCount,
    PSSE_DEVICE_T       *pSseDevice,
    int                 iCount
)
{
    int                 retVal = RETURN_OK;
    int                 i;
    int iCmd;

    ERH_FUNCTION (_T("SseResolveLdev"));    

    DbgFcnIn();

    if (iCmdCount < 0 || iCount < 0 ||
       (iCmdCount <= 0 && iCount > 0) || 
       ((pSseDevice == NULL || pCmdDev == NULL || 
       iCmdCount <= 0) && iCount > 0))
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    for (i = 0; i < iCount; i ++) 
    {
        if (pSseDevice[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSseDevice[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            /* Critical error */
            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSseDevice[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        pSseDevice[i]->iCmdIndex = iCmd;
        pSseDevice[i]->Dev.targid = -1;
        pSseDevice[i]->Dev.lun = -1;
    }

    if (SseXResolveLdevs(pCmdDev, pSseDevice, iCount) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("SsexResolveLdevs failed"));
        retVal = RETURN_ER;
        goto end;
    }
end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseResolveLdev */


int
SseResolveLdevRestore (
    cmddevinfo         **pCmdDev,
    int                  iCmdCount,
    PSSE_DEVICE_T       *pSseDevice,
    int                 iCount
)
{
    int                 retVal = RETURN_OK;
    int                 i;
    int iCmd;

    ERH_FUNCTION (_T("SseResolveLdevRestore"));    

    DbgFcnIn();
    
    if (iCmdCount < 0 || iCount < 0 ||
       (iCmdCount <= 0 && iCount > 0) || 
       ((pSseDevice == NULL || pCmdDev == NULL || 
       iCmdCount <= 0) && iCount > 0))
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    for (i = 0; i < iCount; i ++) 
    {
        if (pSseDevice[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSseDevice[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            /* Critical error */
            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSseDevice[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        pSseDevice[i]->iCmdIndex = iCmd;
        pSseDevice[i]->Dev.targid = -1;
        pSseDevice[i]->Dev.lun = -1;
    }

    if (SseXResolveLdevs(pCmdDev, pSseDevice, iCount) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("SsexResolveLdevs failed"));
        retVal = RETURN_ER;
        goto end;
    }
end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseResolveLdevRestore */

int
SseAttachCmdDev (
    PSSE_DEVICE_T *pSseList,
    int           iDevCount,
    cmddevinfo ***pCmdList,
    int          *piCmdCount
)
{
    int     retVal = RETURN_OK;
    int     iDev, iCmd;
    /*tchar   tsCmd[STRLEN_PATH+1];*/
    xpDevice_t *device = NULL;
    int numb = 0;
    int i = 0;
    cmddevinfo *pCmd;
    int attachRetry;
    int attachSleepTime;
    int k;
    tchar *var=NULL;
    tchar *decPassword = NULL;

    ERH_FUNCTION (_T("SseAttachCmdDev"));
    DbgFcnIn ();

    if (iDevCount < 0 || (pSseList == NULL && iDevCount > 0))
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    if ((var=(tchar*)GetEnv(_T("SSEA_ATTACH_RETRY")))!=NULL)
    {
        if ((attachRetry = atoi (var)) <= 0)
        {    
            DbgStamp (DBG_SSE_ACTION);
            DbgPlain (DBG_SSE_ACTION, 
                      _T("[%s]Invalid Value For SSEA_ATTACH_RETRY,resetting to 1"),
                      __FCN__);
            attachRetry = 1;
        } 
    }
    else
        attachRetry = SSEA_ATTACH_RETRY;
    if ((var=(tchar*)GetEnv(_T("SSEA_ATTACH_SLEEP_TIME")))!=NULL)
    {
        if (!(attachSleepTime = atoi (var)))
             attachSleepTime = SSEA_ATTACH_SLEEP_TIME;
    }
    else
        attachSleepTime = SSEA_ATTACH_SLEEP_TIME;

    DbgPlain (DBG_SSE_ACTION, _T("[%s] attach Retry = %d"),
             __FCN__, attachRetry);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] attach Sleep Time = %d"),
             __FCN__, attachSleepTime);

    *pCmdList = NULL;
    *piCmdCount = 0;

    srand ((unsigned int) time (NULL));

    for (iDev = 0; iDev < iDevCount; iDev++)
    {
        if (pSseList[iDev]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < *piCmdCount; iCmd++)
            if (pSseList[iDev]->Dev.serial == (*pCmdList)[iCmd]->serial)
                break;

        if (iCmd < *piCmdCount)
            continue;

        /* Find command device for new box */
        if ((retVal = SseGetCommandDevice (pSseList[iDev]->Dev.serial,
                      &device,&numb))!= RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] SseGetCommandDevice failed!"), __FCN__);
            ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_FIND_COMMAND_DEVICE_FAILED));
        
            ErhClearError();
            retVal = RETURN_ER;
            goto end;
        }

        /* Allocate memory for cmd structures */
        if ((pCmd = (cmddevinfo *) MALLOC (sizeof(cmddevinfo))) ==
            NULL || memset (pCmd, 0, sizeof(cmddevinfo)) != pCmd ||
            ExtendList ((void **)pCmdList, *piCmdCount, 
            sizeof (cmddevinfo *)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] Memory allocation failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }

        (*pCmdList)[*piCmdCount] = pCmd;
        (*piCmdCount)++;

        DbgPlain (DBG_SSE_3DAPI, _T("[%s] Cmd count %d!"), 
            __FCN__, *piCmdCount);
        
        /* Attach to command device */
      for (k=1; k<=attachRetry; k++)
      {
        for (i=0; i<numb ; i++)
        {
            tchar username[STRLEN_STD + 1] = {0};
            tchar password[STRLEN_STD + 1] = {0};

            if ( strlen(NS(device[i].tsCmd)) == 0 )
            {
                if ( i <  numb - 1)
                    continue;
                else
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                         _T("[%s] SseXAttachCmdDev failed!"), __FCN__);
                    ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
                                   NLS_SSE_FIND_COMMAND_DEVICE_FAILED));
                    ErhClearError ();
                    retVal = RETURN_ER;
                     goto end;
                }
            }
            
            if (SseDbXP_RetrieveUsrPasswd (device[i].serial, username, password) != RETURN_OK)
            {
                DbgStamp (DBG_SSE_ACTION);
                DbgPlain (DBG_SSE_ACTION, _T("[%s] Could not get user/password for %d. not using authentication!"),
                        __FCN__, device[i].serial);
            }
            else
            {
                DbgStamp (DBG_SSE_ACTION);
                DbgPlain (DBG_SSE_ACTION, _T("[%s] Authenticating with user %s"), __FCN__, username);
                decPassword = DecodePassword (password);
            }
            
            retVal = SseXAttachCmdDev (pCmd, device[i].tsCmd, device[i].instance, username, decPassword);
            if (retVal != RETURN_OK)
            {
                if ( i < numb )
                {
                    char sBuf[STRLEN_1K];
                    /*warning*/
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, _T("[%s] SseXAttachCmdDev failed!"), __FCN__);
                    ErhFullReport (__FCN__, ERH_WARNING, NlsGetMessage (NLS_SET_SSE, 
                        NLS_SSE_RMLIB_ATTACHCMDDEV_FAILED, device[i].tsCmd, A2T(getrmerrmsg(pCmd))));
                    getfmtmsg (pCmd, sBuf);
                    DbgPlain (DBG_UNEXPECTED, _T("%s"), A2T(sBuf));
                }
            }
            else
            {
                if (username == NULL || _T('\0') == username[0])
                {
                    ErhFullReport (__FCN__, ERH_NORMAL, NlsGetMessage (NLS_SET_SSE,
                                NLS_SSE_RMLIB_ATTACHCMDDEV_OK, device[i].tsCmd, pCmd->inst ));
                }
                else
                {
                    ErhFullReport (__FCN__, ERH_NORMAL, NlsGetMessage (NLS_SET_SSE,
                                NLS_SSE_RMLIB_ATTACHCMDDEV_AUTH_OK, device[i].tsCmd, username, pCmd->inst));
                }
                break;
            }
        }
       if (retVal == RETURN_OK)
           break;
       else if (k < attachRetry)    /* for retry */
       {
           DbgStamp (DBG_SSE_ACTION);
           DbgPlain (DBG_SSE_ACTION,
                    _T("[%s] Retrying! "), __FCN__ );
           ErhFullReport (__FCN__, ERH_WARNING, NlsGetMessage (NLS_SET_SSE,
                  NLS_SSE_RMLIB_ATTACHCMDDEV_RETRYING,attachSleepTime,ErhErrnoToText()));
           sleep(attachSleepTime);
       }
       else
       {
           DbgStamp (DBG_UNEXPECTED);
           DbgPlain (DBG_UNEXPECTED,
                     _T("[%s] SseXAttachCmdDev failed!"), __FCN__);
           ErhFullReport (__FCN__, ERH_CRITICAL,
                         NlsGetMessage (NLS_SET_SSE,
                         NLS_SSE_RMLIB_ATTACHCMDDEV_ERR));
           retVal = RETURN_ER;
           goto end;
       }
     }
        FREE (device);
        
    }

end:
    FREE (decPassword);
    if (device)
        FREE (device);
    
    DbgFcnOutRet (retVal);
    return (retVal);
}

int
SseDetachCmdDev (
    cmddevinfo ***pCmdList,
    int          *piCmdCount
)
{
    int     retVal = RETURN_OK;
    int     iCmd;

    ERH_FUNCTION (_T("SseDetachCmdDev"));   
    DbgFcnIn ();

    if (*piCmdCount < 0 || (pCmdList == NULL && *piCmdCount > 0))
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }


    for (iCmd = 0; iCmd < *piCmdCount; iCmd++)
    {
        /* Detach command device */
        if ((retVal = SseXDetachCmdDev ((*pCmdList)[iCmd]))
            != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] SseXDetachCmdDev failed!"), __FCN__);
            retVal = RETURN_ER;
        }

        if ((*pCmdList)[iCmd] != NULL)
            FREE ((*pCmdList)[iCmd]);
    }

    FREE (*pCmdList);

    *pCmdList = NULL;
    *piCmdCount = 0;

end:
    DbgFcnOutRet (retVal);
    return (retVal);
}

int
SseMirrorListCheck (
    tchar   *mirrors,
    int     mun,
    int     *status,
    int     *min,
    int     *max
)
{
    int     retVal = RETURN_OK;
    tchar   *token;
    tchar   *subtoken;
    tchar   *buffer;
    int     mirror_min=INT_MAX, mirror_max=0;

    ERH_FUNCTION (_T("SseMirrorListCheck"));

    DbgFcnInEx(__FLND__, _T("mirrors = %s, mun = %d, status = %p, min = %p, max = %p"), 
            mirrors, mun, status, min, max);

    errno = 0; /* Having some problems with that later on */

    if (status != NULL)
        *status = 0;

    if (min != NULL)
        *min = 0;

    if (max != NULL)
        *max = 0;

    if (mirrors == NULL)
    {
        goto end;
    }

    if ((buffer = (tchar *) MALLOC(strlen(mirrors) * sizeof (tchar) * 2)) == NULL)
    {
        /* Critical error */
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Memory allocation failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    strcpy (buffer, mirrors);

    token = strtok (buffer, _T(","));

    while (token != NULL)
    {
        if ((subtoken = strchr (token, _T('-'))) != NULL)
        {
            int     range_a, range_b;

            *subtoken = 0;
            subtoken++;
            
            range_a = atoi (token);
            if (range_a == 0 && errno !=0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Mirror list parsing failed! errno = %d"), 
                        __FCN__, errno);
                retVal = RETURN_ER;
                goto end;
            }

            range_b = atoi (subtoken);
            if (range_b == 0 && errno !=0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Mirror list parsing failed! errno = %d"), 
                        __FCN__, errno);
                retVal = RETURN_ER;
                goto end;
            }

            if (range_a > range_b)
            {
                int tmp;

                tmp = range_b;
                range_b = range_a;
                range_a = tmp;
            }

            if (range_a < mirror_min)
                mirror_min = range_a;

            if (range_b > mirror_max)
                mirror_max = range_b;

            if (status != NULL)
                if (mun >= range_a && mun <= range_b)
                {
                    *status = 1;
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION,
                        _T("[%s] Mun %d was found to be in list %s!"), __FCN__,
                        mun, mirrors);

                    if (min == NULL || max == NULL)
                        goto end;
                }
        }
        else
        {
            /* parsing single number */

            int     parse_mun;

            parse_mun = atoi (token);

            if (parse_mun < mirror_min)
                mirror_min = parse_mun;

            if (parse_mun > mirror_max)
                mirror_max = parse_mun;

            if (status != NULL)
                if (mun == parse_mun)
                {
                    *status = 1;
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION,
                        _T("[%s] Mun %d was found to be in list %s!"), __FCN__,
                        mun, mirrors);

                    if (min == NULL || max == NULL)
                        goto end;
                }
        }

        token = strtok (NULL, _T(","));
    }

end:
    if (min != NULL && max != NULL)
    {
        *min = mirror_min;
        *max = mirror_max;

        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION,
            _T("[%s] Mun range %d - %d!"), __FCN__, *min, *max);
    }
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseMirrorListCheck */

/*========================================================================*//**
*
* @param     pCmdDev       - pointer to list of command devices
*            iCmdCount     - number of command devices in list
*            pSseDev       - list of devices
*            iDevCount     - number of devices in list
*
* @retval    TRUE  All devices in list are part of a snapshot pair
* @retval    FALSE  At least one device in list is part of a mirror pair
*                   or something failed.
*
* @brief     Function checks whether all devices are part of a snapshot pair.
*            This is used to determine if option "Prepare next disks for backup"
*            is applicable.
*
*//*=========================================================================*/
BOOL
SseAreAllSnapShots(cmddevinfo     **pCmdDev, 
                   int            iCmdCount,
                   PSSE_DEVICE_T  *pSseDev,
                   int            iDevCount)
{
    int retVal = FALSE;
    int i = 0;
    int iCmd = 0;
    ERH_FUNCTION (_T("SseAreAllSnapShots"));

    DbgFcnIn();

    for (i = 0; i < iDevCount; i++)
    {
        targdev srcDevice = {0};
        pairstatinfo pairStat = {0};
        
        srcDevice.serial = pSseDev[i]->Dev.serial;
        srcDevice.targid = pSseDev[i]->Dev.targid;
        srcDevice.lun    = pSseDev[i]->Dev.lun;
        srcDevice.port   = pSseDev[i]->Dev.port;
        srcDevice.ldevno = pSseDev[i]->Dev.ldevno;
        srcDevice.mun    = pSseDev[i]->Dev.mun;
        srcDevice.mmode  = MD_MRCF;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
        {
            if (pCmdDev[iCmd]->serial == srcDevice.serial)
            {
                break;
            }
        }

        if (iCmd == iCmdCount)
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Command device not found! Breaking out..."), __FCN__);
            break;
        }

        DbgStamp (DBG_SSE_ACTION);
        if (0 != pairvolstat (pCmdDev[iCmd], &srcDevice, &pairStat))
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] pairvolstat failed at index %d"), __FCN__, i);
            break;
        }
        else if (MRCF_CM_SNAP == pairStat.copymode)
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Snapshot found on index %d"), __FCN__, i);
        }
        else
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Mirror found on index %d, breaking out"), __FCN__, i);
            break;
        }
    }

    if (i == iDevCount)
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] All devices are snapshots."), __FCN__);
        retVal = TRUE;
    }

    DbgFcnOut();
    return(retVal);
} /* SseAreAllSnapShots */

/*========================================================================*//**
*
* @param     pCmdDev       - pointer to command devices
*            device        - device to check
*
* @retval    TRUE  No snapshot was found among pairs, so a quick restore is valid
* @retval    FALSE  At least one pair is a snapshot pair, so only a normal restore is valid
*
* @brief     Function checks if there is a snapshot pair among any MU# on the device.
*
*//*=========================================================================*/
BOOL
SseValidQuickRestoreCheck(cmddevinfo     *pCmdDev, 
                          targdev        device)
{
    int retVal = TRUE;
    int mun = 0;
    dkc_limit limit = {0};
    ERH_FUNCTION (_T("SseValidQuickRestoreCheck"));

    DbgFcnIn();

    if (RETURN_OK != SseXGetDKClimit (pCmdDev, &limit))
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] SseXGetDKClimit failed"), __FCN__);
        goto end;
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Checking the following device:"), __FCN__);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.serial = %d"), __FCN__, device.serial);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.port = %d"), __FCN__, device.port);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.targetid = %d"), __FCN__, device.targid);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.lun = %d"), __FCN__, device.lun);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Maximum HOMRCF MU# is %d"), __FCN__, limit.maxmmun);

    for (mun = 0; mun < limit.maxmmun; mun++)
    {
        pairstatinfo pairStat = {0};
        
        device.mun    = mun;
        device.mmode  = MD_MRCF;

        if (0 != pairvolstat (pCmdDev, &device, &pairStat))
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] pairvolstat failed at MU# %d"), __FCN__, mun);
            continue;
        }
        
        if (MRCF_CM_SNAP == pairStat.copymode)
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Snapshot found on MU# %d"), __FCN__, mun);
            retVal = FALSE;
            break;
        }
    }

end:
    DbgFcnOut();
    return(retVal);
} /* SseValidQuickRestoreCheck */

/*========================================================================*//**
*
* @param     pSrcDevices pointer to list of source devices
*            iDevCount number of source devices
*            pCmdList pointer to list of command devices
*            iCmdCount number of command devices
*
* @retval    RETURN_OK Checking for a valid IR scenario succeeded
* @retval    RETURN_ER Checking for a valid IR scenario failed
*
* @brief     Function checks if there is an active CA link on any of the source devices.
*
*//*=========================================================================*/
int
SseValidIRCheck(PSSE_DEVICE_T   *pSrcDevices,
                int             iDevCount,
                cmddevinfo      **pCmdList,
                int             iCmdCount)
{
    int retVal = RETURN_OK;
    int i = 0;
    int iCmd = 0;
    ERH_FUNCTION (_T("SseValidIRCheck"));

    DbgFcnIn();

    for (i = 0; i < iDevCount; i++)
    {
        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
        {
            if (pCmdList[iCmd]->serial == pSrcDevices[i]->Dev.serial)
            {
                break;
            }
        }

        if (iCmd == iCmdCount)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Cannot find command device for serial number %d!"), 
                __FCN__, pSrcDevices[i]->Dev.serial);
            /* unexpected but try to continue anyway */
            continue;
        }

        if (RETURN_OK != SseCAStatusCheck (pCmdList[iCmd], pSrcDevices[i]->Dev))
        {
            DbgPlain (DBG_SSE_ACTION, 
                _T("[%s] At least one source disk has an active CA link. Aborting IR..."), __FCN__);

            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage(NLS_SET_SSE, NLS_SSE_IR_ACTIVE_CA_CHECK_FAILED));

            retVal = RETURN_ER;
            break;
        }
    }

    DbgFcnOut();
    return(retVal);
} /* SseValidIRCheck */

/*========================================================================*//**
*
* @param     pCmdDev pointer to command device
*            device  device to check
*
* @retval    RETURN_OK No active CA links on the device
* @retval    RETUNR_ER At least one device has an active CA link
*
* @brief     Function checks if there is an active CA link on any HORC MU# on the device.
*
*//*=========================================================================*/
int
SseCAStatusCheck(cmddevinfo     *pCmdDev,
                 targdev        device)
{
    int retVal = RETURN_OK;
    int mun = 0;
    dkc_limit limit = {0};
    ERH_FUNCTION (_T("SseCAStatusCheck"));

    DbgFcnIn();

    if (RETURN_OK != SseXGetDKClimit (pCmdDev, &limit))
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] SseXGetDKClimit failed"), __FCN__);
        goto end;
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Checking the following device:"), __FCN__);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.serial = %d"), __FCN__, device.serial);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.port = %d"), __FCN__, device.port);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.targetid = %d"), __FCN__, device.targid);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] device.lun = %d"), __FCN__, device.lun);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Maximum HORC MU# is %d"), __FCN__, limit.maxhmun);

    for (mun = 0; mun < limit.maxhmun; mun++)
    {
        pairstatinfo pairStat = {0};
        
        device.mun    = mun;
        device.mmode  = MD_HORC;

        if (0 != pairvolstat (pCmdDev, &device, &pairStat))
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] pairvolstat failed at MU# %d"), __FCN__, mun);
            continue;
        }
        
        if (STAT_SMPL == pairStat.status)
        {
            /* no CA link on this mun */
            continue;
        }
        else
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] CA link with status %s found on MU# %d"), 
                __FCN__, DbgPrintPairStat(pairStat.status), mun);
        }

        if (STAT_PSUS != pairStat.status &&
            STAT_PSUE != pairStat.status)
        {
            retVal = RETURN_ER;
            break;
        }
    }

end:
    DbgFcnOut();
    return(retVal);
} /* SseCAStatusCheck */

int
SseGetPair (
    cmddevinfo          **pCmdDev, 
    int                 iCmdCount,
    SSE_DEVICE_T        **pSrcDevice, 
    SSE_DEVICE_T        **pTgtDevice,
    int                 iDevCount,
    unsigned char       mmode,
    tchar               *mirrors
)    
{
    int     retVal = RETURN_OK;
    int     i, iCmd;
    int     mmin, mmax;
    int     mun, select_mun = -1;
    int             oldest_mun = -1;
    time_t          oldest_time = LONG_MAX;

    PTARGDEV_LIST_T pDevList = NULL;
    int             iDevListCount = 0;
    tchar locked_mirrors[STRLEN_PATH + 1];
    int p;
    int count;
    tchar tsBuf[STRLEN_PATH+1];
    

    ERH_FUNCTION (_T("SseGetPair")); 
    DbgFcnInEx(__FLND__,
        _T("mmode = %s [%d]"), DbgPrintMmode(mmode), mmode);

    if (iCmdCount < 0 || (pCmdDev == NULL && iCmdCount != 0) ||
        ((pSrcDevice == NULL || pTgtDevice == NULL) && iDevCount != 0))
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    if (SseMirrorListCheck (mirrors, 0, NULL, &mmin, &mmax) 
            != RETURN_OK)
    {
        /* Critical error */
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] SseMirrorListCheck failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }
    DbgStamp (DBG_SSE_ACTION);
            DbgPlain (DBG_SSE_ACTION, _T("[%s] !!!! opt.mun =  %d!\n"), __FCN__,opt.mun);
            

    locked_mirrors[0] =  _T('\0');
    p = 0;
    
    while (p <= (mmax - mmin))
    {
        int locked = 0;

        /* select_mun = oldest_mun = -1; */
        oldest_time = LONG_MAX;
        iDevListCount = 0;
        
        
        DbgPlain (DBG_SSE_ACTION, _T("\n\n\n[%s] !!!! p = %d!"), __FCN__, p);
        
        if (pDevList)
        {
            for (i=0; i<iDevListCount; i++)
                FREE (pDevList[i].pDev);

            FREE (pDevList);
        }

        
        for (mun = mmin; mun <= mmax; mun++)
        {
            int             mun_status;
            int             status;
            

            sprintf(tsBuf,_T("%d"),mun);

            if (NULL != strstr(locked_mirrors, tsBuf))
                continue;

            /*if (NULL != strstr(locked_mirrors, ltoa(mun)))
              continue;*/
            
            SseMirrorListCheck (mirrors, mun, &mun_status, NULL, NULL);
             
            if (mun_status == 0)
                continue;
            
            DbgStamp (DBG_SSE_ACTION);
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Checking mun %d!"), __FCN__, mun);
            
           
            if ((pDevList = (PTARGDEV_LIST_T) REALLOC (pDevList, ++iDevListCount * 
                                                      sizeof (TARGDEV_LIST_T)))==NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                  _T("[%s] Memory allocation failed!"), __FCN__);
                retVal = RETURN_ER;
            }
                
            pDevList [iDevListCount-1].mun = mun;
            
            if ((pDevList [iDevListCount-1].pDev = (targdev *) MALLOC (iDevCount * 
                                                                       sizeof (targdev)))==NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] Memory allocation failed!"), __FCN__);
                retVal = RETURN_ER;
            }

            memset(pDevList[iDevListCount-1].pDev, 0, (iDevCount * sizeof (targdev)));
            
            
            for (i = 0; i < iDevCount; i ++) 
            {
                
                if (pSrcDevice[i]->iStatus == LDEV_OK)
                {
                    targdev srcDev, *tgtDev;
                    int excluded = 0;

                                        
                    for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                        if (pCmdDev[iCmd]->serial == pSrcDevice[i]->Dev.serial)
                            break;
                    
                    if (iCmd == iCmdCount)
                    {
                        /* Critical error */
                        ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                                  _T("[%s] Cannot find command device for serial number %d!"),
                                  __FCN__, pSrcDevice[i]->Dev.serial);
                        
                            retVal = RETURN_ER;
                            goto end;
                    }
                    
                    memcpy (&srcDev, &(pSrcDevice[i]->Dev), sizeof (srcDev));
                    tgtDev = &((pDevList[iDevListCount-1]).pDev[i]);
                    
                    srcDev.mun = mun;
                    
                    if (SseXGetPair (pCmdDev[iCmd], &srcDev, tgtDev, mmode, 
                                     &status) != RETURN_OK)
                    {
                        pSrcDevice[i]->iStatus = LDEV_INVALID;
                        
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                                  _T("[%s] SseXGetPair failed!"), __FCN__);
                        
                        if (mmode == MD_HORC) 
                            /* CA */
                            ErhFullReport (__FCN__, ERH_WARNING, NlsGetMessage (
                                NLS_SET_SSE, NLS_SSE_RMLIB_GETPAIR_FAILED_CA, 
                                srcDev.ldevno, srcDev.serial, A2T(getrmerrmsg(pCmdDev[iCmd]))));
                        else
                                /* BC */
                            ErhFullReport (__FCN__, ERH_WARNING, NlsGetMessage (
                                NLS_SET_SSE, NLS_SSE_RMLIB_GETPAIR_FAILED_BC, 
                                srcDev.ldevno, srcDev.mun, srcDev.serial, A2T(getrmerrmsg(pCmdDev[iCmd]))));
                        
                        ErhClearError ();
                        tgtDev->serial = 0;                        
                        continue;
                    }
                    
                    /* TBD: Check exclude list */
                    SseDbXP_IsLdevExcluded (tgtDev, &excluded);
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION,_T("mun = %d    select mun = %d"), mun, select_mun);
                    
                    
                    if (excluded)
                    {
                        pSrcDevice[i]->iStatus = LDEV_INVALID;
                        ErhFullReport (__FCN__, ERH_MAJOR, NlsGetMessage (
                            NLS_SET_SSE, NLS_SSE_LDEV_EXCLUDED, 
                            tgtDev->ldevno, tgtDev->serial));
                        
                        /* if device is excluded, we don't have to lock this device */
                        
                        tgtDev->serial = 0;
                        continue;
                    }
                    
                    
                    if (select_mun == -1)
                    {
                        if ((status == STAT_PAIR) || (status == STAT_PFUL))
                        {
                            select_mun = mun;
                            DbgStamp (DBG_SSE_ACTION);
                            DbgPlain (DBG_SSE_ACTION, _T("status = STAT_PAIR/STAT_PFUL, select_mun = %d "), mun);
                        }
                        else
                        {
                            int status;
                            time_t dateTime;
                            
                            if ((SseDbXP_IsLdevLogged (tgtDev, &status, &dateTime))!=RETURN_OK)
                            {
                                DbgStamp (DBG_UNEXPECTED);
                                DbgPlain (DBG_UNEXPECTED,
                                  _T("[%s] SseDbXP_IsLdevLogged failed!"), __FCN__);
                                retVal = RETURN_ER;
                             }
                             
                            DbgPlain(DBG_SSE_ACTION, _T("[%s] status=%i dateTime=  %i  oldest_time= %i"), __FCN__, status, (int)dateTime, (int)oldest_time);                   
                            if (status == 0)
                            {
                                select_mun = mun;
                            }
                            else if (dateTime < oldest_time)
                            {
                                oldest_time = dateTime;
                                oldest_mun = mun;
                            }
                        }
                    }
                }
            }
        }
        
        DbgPlain (DBG_SSE_ACTION, _T("[%s] iDevListCount = %d"), 
                  __FCN__, iDevListCount);
        DbgPlain (DBG_SSE_ACTION, _T("[%s] oldest_mun = %d"), 
                  __FCN__, oldest_mun);
        
        if (select_mun == -1)
            select_mun = oldest_mun;
        
        if (select_mun == -1)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
        
        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION,_T("Selected mun is %d "), select_mun);
            
        /* request for mirror locking */
        if ((opt.mun != -1) && (opt.mun == select_mun))
            break;
        
        count = 0;
        while (count < iDevListCount && pDevList [count].mun != select_mun)
            count++;

        
        if (MsgMirrorLock( pDevList[count].pDev, iDevCount, &locked) != RETURN_OK)
        {
            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_MIRROR_LOCKING_FAILED));
            
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] MsgMirrorLock failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;                    
        }
        if (locked)
            break;
        
        sprintf(tsBuf,_T("%d"),select_mun);
        strcat(locked_mirrors, tsBuf);
        /* strcat (locked_mirrors, ltoa (select_mun)); */
        strcat (locked_mirrors, _T(" "));
        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION, _T("[%s] Selected mirrors are locked: %s!"), __FCN__, locked_mirrors);
        select_mun = oldest_mun = -1;               
        p++;
    }
    
    opt.mun = select_mun;

    if (select_mun != -1)
    {
        int     index = 0;

        while (index < iDevListCount && pDevList [index].mun != select_mun)
            index++;

        for (i = 0; i < iDevCount; i ++) 
        {
            pSrcDevice[i]->Dev.mun = select_mun;
            memcpy (&(pTgtDevice[i]->Dev), 
                    &((pDevList[index]).pDev[i]), 
                    sizeof (pTgtDevice[i]->Dev));
            pTgtDevice[i]->iStatus = LDEV_OK;
        }
    }
    else
    {
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_DISKS_ARE_ALREADY_LOCKED));
            
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] All mirror disks are locked!"), __FCN__);
        retVal = RETURN_ER;
        goto end;                    
    }

   
end:
    if (pDevList)
    {
        for (i=0; i<iDevListCount; i++)
            FREE (pDevList[i].pDev);
        
        FREE (pDevList);
    }


    DbgFcnOutRet (retVal);
    return (retVal);
}
/* SseGetPair */

int
SseExPairs (
    cmddevinfo **pCmdDev, int iCmdCount,
    SSE_DEVICE_T        **pSrcDevs, 
    int                 iDevCount,
    SSE_DEVICE_T        ***pExDevs,
    int                 *iExDevs,
    unsigned char       mmode
)
{
    int i, j, retVal = RETURN_OK;
    int iCmd;
    targdev dev;
    int     inv_count=0;
    tchar   *tsBuf = NULL;
    tchar tsBuf2[STRLEN_PATH+1];
    int    iBufLen = 0, iOldLen;
    tchar *newline = _T("\n\t");
    int newline_len = (int) strlen(newline);

    ERH_FUNCTION (_T("SseExPairs")); 
    DbgFcnIn ();

    if (pExDevs == NULL || *pExDevs != NULL)
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    *iExDevs = 0;
    for (i = 0; i < iDevCount; i ++) 
    {
        dkc_limit limit = {0};

        if (pSrcDevs[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSrcDevs[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            /* Critical error */

            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSrcDevs[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        if (RETURN_OK != SseXGetDKClimit (pCmdDev[iCmd], &limit))
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] SseXGetDKClimit failed"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }

        memcpy (&dev, &(pSrcDevs[i]->Dev), sizeof (dev));

        for (j = 0; j < limit.maxmmun; j++)
        {
            pairstatinfo  status;

            if (j == pSrcDevs[i]->Dev.mun)
                continue;

            dev.mun=j;

            if (SseXPairVolStat (pCmdDev[iCmd], &dev, 
                    &status, mmode, 0) != RETURN_OK)
                continue;

            if ((status.status == STAT_PSUS) || 
                (status.status == STAT_SMPL) || 
                (status.status == STAT_PFUS))
            {
                continue;
            }

            if ((status.status != STAT_PAIR) &&
                (status.status != STAT_PFUL))
            {
                /* Critical error */

                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Invalid status!"), __FCN__);

                retVal = RETURN_ER;

                sprintf (tsBuf2, 
                    _T("%-7d %04Xh (%4d)  %5s  %3d  %3d  %3d  %s  %-7d %04Xh (%4d)"),
                    pSrcDevs[i]->Dev.serial,
                    pSrcDevs[i]->Dev.ldevno,
                    pSrcDevs[i]->Dev.ldevno,
                    portno[pSrcDevs[i]->Dev.port],
                    pSrcDevs[i]->Dev.targid,
                    pSrcDevs[i]->Dev.lun,
                    pSrcDevs[i]->Dev.mun,
                    DbgPrintPairStat (status.status),
                    status.pserial,
                    status.pldev,
                    status.pldev
                );
                
                DbgPlain (DBG_SSE_ACTION,
                    _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

                iOldLen = iBufLen;
                iBufLen += ((int) strlen(tsBuf2)) + (inv_count > 0 ? newline_len : 0);
                if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                    sizeof (tchar))) == NULL)
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] REALLOC failed"), __FCN__);

                    retVal = RETURN_ER;
                    goto end;
                }
                tsBuf[iOldLen] = _T('\0');
                tsBuf[iBufLen] = _T('\0');

                if (inv_count>0)
                    strcat(tsBuf, newline);

                strcat (tsBuf, tsBuf2);

                inv_count++;

                if (inv_count == SMB_MSG_LINES - 1 && tsBuf != NULL)
                {
                    ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_BC_INVALID_PAIRS_REPORT, tsBuf));

                    FREE(tsBuf);
                    tsBuf = NULL;
                    iBufLen = iOldLen = 0;
                    inv_count = 0;
                }
                continue;
            }

            (*iExDevs)++;
            *pExDevs = (SSE_DEVICE_T **) REALLOC (*pExDevs, (*iExDevs) * 
                    sizeof (SSE_DEVICE_T *));

            if (*pExDevs == NULL)
            {
                /* Critical error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Memory allocation failed!"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }

            (*pExDevs)[*iExDevs - 1] = (SSE_DEVICE_T *) MALLOC (
                    sizeof (SSE_DEVICE_T));

            if ((*pExDevs)[*iExDevs - 1] == NULL)
            {
                /* Critical error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Memory allocation failed!"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }

            memset ((*pExDevs)[*iExDevs - 1], 0, sizeof ((*pExDevs)[0]));

            (*pExDevs)[*iExDevs - 1] -> iIndex = *iExDevs;
            (*pExDevs)[*iExDevs - 1] -> iStatus = LDEV_OK;
            memcpy ( &((*pExDevs)[*iExDevs - 1] -> Dev), &dev, 
                    sizeof ((*pExDevs)[0] -> Dev));
            
        }
    }

    if (tsBuf != NULL)
    {
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_BC_INVALID_PAIRS_REPORT, tsBuf));

        FREE(tsBuf);
        tsBuf = NULL;
    }

    /* List all devs */
    for (i=0; i<*iExDevs; i++)
        DbgPrintSseDev ((*pExDevs)[i]);

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} /* SseExPairs */

int
SseLockAllMirrors (
    cmddevinfo          **pCmdDev,
    int                 iCmdCount,
    SSE_DEVICE_T        **pSrcDevs, 
    int                 iDevCount,
    unsigned char       mmode
)
{
    int i, j,  count, locked;
    int retVal = RETURN_OK;
    int iCmd;
    targdev dev;
    targdev *tgtDev = NULL;
    
    

    ERH_FUNCTION (_T("SseLockAllMirrors")); 
    DbgFcnIn ();

    
    for (i = 0; i < iDevCount; i ++) 
    {
        dkc_limit limit = {0};
        
        if (pSrcDevs[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSrcDevs[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            /* Critical error */

            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSrcDevs[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        if (RETURN_OK != SseXGetDKClimit (pCmdDev[iCmd], &limit))
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] SseXGetDKClimit failed"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
        

        memcpy (&dev, &(pSrcDevs[i]->Dev), sizeof (dev));
        count = 0;
        locked = 0;

        for (j = 0; j < limit.maxmmun; j++)
        {
            pairstatinfo  status;

            dev.mun=j;

            if (SseXPairVolStat (pCmdDev[iCmd], &dev, 
                    &status, mmode, 0) != RETURN_OK)
                continue;

            if (status.status == STAT_SMPL)
                continue;

            
            tgtDev = (targdev *) REALLOC(tgtDev, (count+1) * sizeof(targdev));
            tgtDev[count].serial = status.pserial;
            tgtDev[count].ldevno = status.pldev;
       
            
            DbgStamp (DBG_SSE_ACTION);
            DbgPlain (DBG_SSE_ACTION,_T("serial =  %d ldev = %d\n"),tgtDev[count].serial,tgtDev[count].ldevno);
            count++;
            
        }
        if (!count)
            continue;


        if (MsgMirrorLock( tgtDev, count, &locked) != RETURN_OK)
        {
            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_MIRROR_LOCKING_FAILED));
            
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] MsgMirrorLock failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;                    
        }
        if (!locked)
        {
            /* CRITICAL */
            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_DISKS_ARE_ALREADY_LOCKED));
                        
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] Some of the mirrors are Locked!\n"), __FCN__);
            /* pSrcDevs[i]->iStatus = LDEV_INVALID; */
            retVal = RETURN_ER;
            goto end;                    
            
        }

        FREE(tgtDev);
        
    }
     
end:
    
    if (tgtDev)
        FREE(tgtDev)
            
    DbgFcnOutRet (retVal);
    return (retVal);
} /* SseLockAllMirrors */

/*========================================================================*//**
*
* @ingroup   SSEA_API
*
* @param     PSSE_DEVICE_T    - ptr to device array
*            PSMB_RAW_DISK_T  - ptr to disk array
*            int              - disk array entries count
*
* @retval    RETURN_OK or RETURN_ER indicating function succeeded
*            or failed
*
* @brief     Pass through to SseMapLdevToRdskEx
*
*//*=========================================================================*/
int
SseMapLdevToRdsk( PSSE_DEVICE_T       *pSseDevice,
                 PSMB_RAW_DISK_T     *pRdsks,
                 int                 iSseDeviceCount )
{
    return(SseMapLdevToRdskEx(pSseDevice, pRdsks, iSseDeviceCount, FALSE ));
}


#if TARGET_SOLARIS

int
SseMapLdevToRdskEx (
            PSSE_DEVICE_T       *pSseDevice,
            PSMB_RAW_DISK_T     *pRdsks,
            int                 iSseDeviceCount,
            BOOL                BackupHost)
{
    int retVal = RETURN_OK;
    PSMB_RAW_DISK_T *pRdskList = NULL;
    int              iRdskCount = 0;
    PSMB_RAW_DISK_T pRdsk;
    int             alter_path_count=0;
    SSE_DEVICE_T    Ldev;
    unsigned long   i, j;
    XP_RAW_DISK_T   *pRdskXP     = NULL,
                    *pRdskListXP = NULL;
    int             iRdskCountXP = 0;
    int             iSlice;
    tchar           *tsSPos = NULL;


    ERH_FUNCTION (_T("SseMapLdevToRdskEx"));    
    
    DbgFcnIn();

    if((retVal = GetAllRdsks (&pRdskList, &iRdskCount)) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] GetAllRdsks failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }
   
    /* Allocate space for all rdsks */
    if ( ((pRdskListXP = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCount)) == NULL) || 
         (memset (pRdskListXP, 0, sizeof(XP_RAW_DISK_T)*iRdskCount) != pRdskListXP) )
    {
        /* Critical error */
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }
   
    
    /* Collect SCSI Inq. info once for all rdsks */
    for (i=0; i<iRdskCount; i++)
    {
        pRdsk = pRdskList[i];

        if (pRdsk->status != RDSK_OK)
            continue;

        if (CheckVendorId(pRdsk->tsVendorId) && 
            strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0)
        {
            if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
            { /* Error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] ScsiInquiryXp failed for %s!"),
                    __FCN__, pRdsk->tsRdskName);
            }

            /* Store rdsk structure values */
            StrCopy (pRdskListXP[iRdskCountXP].tsSliceName, pRdsk->tsSliceName, 
                    strlen(pRdsk->tsSliceName));
            StrCopy (pRdskListXP[iRdskCountXP].tsVendorId, pRdsk->tsVendorId, 
                    strlen(pRdsk->tsVendorId));
            StrCopy (pRdskListXP[iRdskCountXP].tsProductId, pRdsk->tsProductId, 
                    strlen(pRdsk->tsProductId));
            StrCopy (pRdskListXP[iRdskCountXP].tsRevision, pRdsk->tsRevision, 
                    strlen(pRdsk->tsRevision));
            
            pRdskListXP[iRdskCountXP].SerialID   = Ldev.Dev.serial;
            pRdskListXP[iRdskCountXP].LDEVNumber = Ldev.Dev.ldevno;
            pRdskListXP[iRdskCountXP].port       = Ldev.Dev.port;
            pRdskListXP[iRdskCountXP].status     = pRdsk->status; 
                
            DbgStamp (DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdskListXP[%d] = \n{\n\t"
                                        " tsSliceName  = [%s]\n\t"
                                        " tsVendorID  = [%s]\n\t"
                                        " tsProductId = [%s]\n\t"
                                        " tsRevision  = [%s]\n\t"
                                        " SerialID    = %d\n\t"
                                        " LDEVNumber  = %d\n\t"
                                        " port        = %d\n\t"
                                        " status      = %d\n}"),  
                                        __FCN__, iRdskCountXP, 
                                        pRdskListXP [iRdskCountXP].tsSliceName, 
                                        pRdskListXP [iRdskCountXP].tsVendorId, 
                                        pRdskListXP [iRdskCountXP].tsProductId,
                                        pRdskListXP [iRdskCountXP].tsRevision, 
                                        pRdskListXP [iRdskCountXP].SerialID, 
                                        pRdskListXP [iRdskCountXP].LDEVNumber, 
                                        pRdskListXP [iRdskCountXP].port, 
                                        pRdskListXP [iRdskCountXP].status);
 
            iRdskCountXP++;
        }
    }


    for (j=0; j<iSseDeviceCount; j++)
    {
        DbgStamp (DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI,
            _T("[%s] LDEV %u on XP S/N %d"),
            __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);

        alter_path_count = 0;

        /*sscanf (pRdsks[j]->tsSliceName, _T("c%*dt%*dd%*ds%d"), 
                &iSlice);*/
        if ((tsSPos = strrchr (pRdsks[j]->tsSliceName, _T('s'))) == NULL)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] strrchr failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
        else
        {
            tsSPos = tsSPos + 1;
            iSlice = atoi(tsSPos);
            DbgPlain (DBG_SSE_3DAPI, _T("[%s] iSlice = %d"), 
                __FCN__, iSlice);
        }
        

        /* For each LDEV, try to find appropriate Rdsk */
        for (i=0; i<iRdskCountXP; i++)
        {
            /* Select new rdsk from XP rdsk list */
            pRdskXP = pRdskListXP + i;

            if (pRdskXP->status != RDSK_OK)
                continue;

            /* If the IDs of rdsk are appropriate  */
            if (CheckVendorId(pRdskXP->tsVendorId) && 
                strncmp(pRdskXP->tsProductId, _T("OPEN"), 4) == 0)
            {
                /* Check for Serial and Device number */ 
                if (pRdskXP->SerialID   == pSseDevice[j]->Dev.serial && 
                    pRdskXP->LDEVNumber == pSseDevice[j]->Dev.ldevno)
                {
                    tchar tsRdskName[STRLEN_PATH+1];
                    tchar *tsSPos=NULL;

                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                        _T("[%s] Device %s (%s, %s) was found to be LDEV %u on XP S/N %d and port %d!"),
                        __FCN__, pRdskXP->tsSliceName, pRdskXP->tsVendorId,
                        pRdskXP->tsProductId, pRdskXP->LDEVNumber, pRdskXP->SerialID, pRdskXP->port);

                     if (*(pRdskXP->tsSliceName) == _T('c'))
                     {
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] Device name in short notation"), __FCN__);
                        strcpy(pRdskXP->tsFullPath, _T("/dev/rdsk/"));
                        strncat (pRdskXP->tsFullPath, pRdskXP->tsSliceName, STRLEN_PATH);
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] pRdskXP->tsFullPath = %s"),
                                 __FCN__, NS(pRdskXP->tsFullPath));
                    }
                    else
                    {
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] Device name in long notation"), __FCN__);
                        strcpy(pRdskXP->tsFullPath, pRdskXP->tsSliceName);
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] pRdskXP->tsFullPath = %s"),
                                 __FCN__, NS(pRdskXP->tsFullPath));
                    }

                    strncpy (tsRdskName, pRdskXP->tsSliceName, STRLEN_PATH);
                    if ((tsSPos = strrchr (tsRdskName, _T('s'))) == NULL)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                                _T("[%s] Invalid disk name!"), __FCN__);
                        retVal = RETURN_ER;
                        goto end;
                    }
                    *tsSPos = _T('\0');

                    /*alternate_path*/
                    if (alter_path_count == 0)
                    {
                        tchar tsTmpSliceName[STRLEN_PATH+1];
                        tchar tsTmpFullPath[STRLEN_PATH+1];
                        pRdsks[j]->crc    = pSseDevice[j]->crc;
                        
                        DbgPlain (DBG_SSE_3DAPI, 
                                  _T("[%s] Current XP crc = 0x%08x"), 
                                  __FCN__, pRdsks[j]->crc);
                        /* getting the slice number and changing the slicename and rdskfullpath acoordingly*/
                        DbgPlain (DBG_SSE_3DAPI, 
                                  _T("[%s] iSlice = %d"), 
                                  __FCN__, iSlice);
                        sprintf(tsTmpSliceName, _T("%ss%d"),
                                tsRdskName,iSlice);
                        sprintf(tsTmpFullPath, _T("/dev/rdsk/%ss%d"),
                                tsRdskName,iSlice);
                        
                        /*strcpy(pRdsks[j]->tsFullPath,  pRdskXP->tsFullPath);*/
                        StrCopy(pRdsks[j]->tsFullPath,tsTmpFullPath,STRLEN_PATH);
                        strcpy(pRdsks[j]->tsFullBackName, pRdskXP->tsFullBackName);
                        strcpy(pRdsks[j]->tsBackName,  pRdskXP->tsBackName);
                        strcpy(pRdsks[j]->tsVendorId,  pRdskXP->tsVendorId);
                        strcpy(pRdsks[j]->tsProductId, pRdskXP->tsProductId);
                        strcpy(pRdsks[j]->tsRevision,  pRdskXP->tsRevision);
                        /*strcpy(pRdsks[j]->tsSliceName, pRdskXP->tsSliceName);*/
                        StrCopy(pRdsks[j]->tsSliceName,tsTmpSliceName,STRLEN_PATH);

                        pRdsks[j]->iLun    = pRdskXP->iLun;
                        pRdsks[j]->iTarget = pRdskXP->iTarget;
                        pRdsks[j]->status  = pRdskXP->status;
                        pRdsks[j]->iErrReported = pRdskXP->iErrReported;
                        /* pRdsks[j]->port = pRdskXP->port; */
                        strcpy (pRdsks[j]->alternate_paths, tsRdskName);
                        strcpy (pRdsks[j]->tsRdskName, tsRdskName);
                        DbgStamp (DBG_SSE_3DAPI);
                        DbgPlain (DBG_SSE_3DAPI,
                                  _T("[%s] alternate_paths = %s ,"
                                     "tsRdskName =%s,pRdsks[j]->tsRdskName=%s "),
                                  __FCN__, pRdsks[j]->alternate_paths, tsRdskName, pRdsks[j]->tsRdskName);
                        
                        pSseDevice[j]->Dev.port = pRdskXP->port;
                        
                    }
                    
                    else
                    {
                        if(strstr(pRdsks[j]->alternate_paths, tsRdskName)==NULL)
                        {
                            strcat (pRdsks[j]->alternate_paths, _T(","));
                            strcat (pRdsks[j]->alternate_paths, tsRdskName);
                        } 
                    }
                    
                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                              _T("[%s] alternate_paths = %s , alter_path_count = %d , tsSliceName[j] = %s , tsRdskName[j]=%s,tsRdskName=%s"),
                              __FCN__, pRdsks[j]->alternate_paths,alter_path_count, pRdsks[j]->tsSliceName,pRdsks[j]->tsRdskName,tsRdskName);
                    
                    alter_path_count++;
                }
            }
        }
        
        if (alter_path_count==0)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                      _T("[%s] Failed to find match for LDEV %lu on XP S/N %lu!"),
                      __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);
            pSseDevice[j]->iStatus = LDEV_INVALID;
            pRdsks[j]->status =  RDSK_ER;
        }
    }
    
end:
    FREE(pRdskListXP);

    DbgFcnOutRet (retVal);
    return (retVal);
    
} /* SseMapLdevToRdskEx */

#elif TARGET_HPUX
static int
CollectXPdiskSCSIinq (
            PSMB_RAW_DISK_T *pRdskList,
            int             iRdskCount,
            XP_RAW_DISK_T   *pRdskListXP,
            int             *iRdskCountXP  
) 
{
    int retVal = RETURN_OK;
    int i;
    PSMB_RAW_DISK_T pRdsk;
    SSE_DEVICE_T Ldev = {0};
    int iXPDskCount = 0;


    ERH_FUNCTION (_T("CollectXPdiskSCSIinq"));
    DbgFcnIn();    

    for (iXPDskCount=0, i=0; i<iRdskCount; i++)
    {
        pRdsk = pRdskList[i];

        if (pRdsk->status != RDSK_OK)
            continue;

        if (CheckVendorId(pRdsk->tsVendorId) && 
            strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0)
        {
            if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
            { /* Error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] ScsiI(pRdskListXP[inquiryXp failed for %s!"),
                    __FCN__, pRdsk->tsRdskName);
            }
            /* Store rdsk structure values */
            StrCopy (pRdskListXP[iXPDskCount].tsRdskName, pRdsk->tsRdskName, 
                    strlen(pRdsk->tsRdskName));
            StrCopy (pRdskListXP[iXPDskCount].tsVendorId, pRdsk->tsVendorId, 
                    strlen(pRdsk->tsVendorId));
            StrCopy (pRdskListXP[iXPDskCount].tsProductId, pRdsk->tsProductId, 
                    strlen(pRdsk->tsProductId));
            StrCopy (pRdskListXP[iXPDskCount].tsRevision, pRdsk->tsRevision, 
                    strlen(pRdsk->tsRevision));

            pRdskListXP[iXPDskCount].SerialID   = Ldev.Dev.serial;
            pRdskListXP[iXPDskCount].LDEVNumber = Ldev.Dev.ldevno;
            pRdskListXP[iXPDskCount].port       = Ldev.Dev.port;
            pRdskListXP[iXPDskCount].status     = pRdsk->status; 

            /* Copy struct pRdskXp to struct. list pRdskListXP */    
            /*memcpy( pRdskListXP [iXPDskCount], pRdskXP, sizeof(XP_RAW_DISK_T));*/

            DbgStamp (DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdskListXP[%d] = \n{\n\t"
                                        " tsRdskName  = [%s]\n\t"
                                        " tsVendorID  = [%s]\n\t"
                                        " tsProductId = [%s]\n\t"
                                        " tsRevision  = [%s]\n\t"
                                        " SerialID    = %d\n\t"
                                        " LDEVNumber  = %d\n\t"
                                        " port        = %d\n\t"
                                        " status      = %d\n}"),
                                        __FCN__, iXPDskCount,
                                        pRdskListXP [iXPDskCount].tsRdskName, 
                                        pRdskListXP [iXPDskCount].tsVendorId, 
                                        pRdskListXP [iXPDskCount].tsProductId,
                                        pRdskListXP [iXPDskCount].tsRevision, 
                                        pRdskListXP [iXPDskCount].SerialID, 
                                        pRdskListXP [iXPDskCount].LDEVNumber, 
                                        pRdskListXP [iXPDskCount].port, 
                                        pRdskListXP [iXPDskCount].status);

            iXPDskCount++;
       }
    }

    *iRdskCountXP = iXPDskCount;
    DbgFcnOutRet (retVal);
    return (retVal);
} /* CollectXPdiskSCSIinq */


static int
MapLdevToRdsk (
            PSSE_DEVICE_T   pSseDevice,
            PSMB_RAW_DISK_T pRdsks,
            XP_RAW_DISK_T   *pRdskListXP,
            int             iRdskCountXP
)
{
    int retVal = RETURN_OK;
    int i;
    XP_RAW_DISK_T *pRdskXP = NULL;
    int alter_path_count;

    ERH_FUNCTION (_T("MapLdevToRdsk"));
    DbgFcnIn();

    alter_path_count = 0;  
    memset(pRdsks->alternate_paths, 0, STRLEN_PATH);

    for (i=0; i<iRdskCountXP; i++)
    {
        pRdskXP = pRdskListXP + i;

        DbgPlain (DBG_SSE_3DAPI, _T("[%s] Checking against disk[%d]:\n\tVendor:%s\n\tProduct id: %s\n\tSerial: %d\n\tLDev: %d"),
                                        __FCN__,
                                        i,
                                        pRdskXP->tsVendorId,
                                        pRdskXP->tsProductId,
                                        pRdskXP->SerialID,
                                        pRdskXP->LDEVNumber);

        if (pRdskXP->status != RDSK_OK)
            continue;

        if (CheckVendorId(pRdskXP->tsVendorId) && 
            strncmp(pRdskXP->tsProductId, _T("OPEN"), 4) == 0)
        {
            /* Check for Serial and Device number */ 
            if (pRdskXP->SerialID   == pSseDevice->Dev.serial && 
                pRdskXP->LDEVNumber == pSseDevice->Dev.ldevno)
            {
               /* if (*(pRdskXP->tsRdskName) == _T('c') */ 
                if (*(pRdskXP->tsRdskName) != _T('/') )  
                {
                    DbgPlain(DBG_SSE_SYSTEM,
                             _T("[%s] Device name in short notation"), __FCN__);

                    if ( NULL != strstr(pRdskXP->tsRdskName, _T("disk")))
                    { 
                        strcpy(pRdskXP->tsFullPath, _T(NEW_RAW_DSF));
                    }
                    else
                    {
                        strcpy(pRdskXP->tsFullPath, _T(OLD_RAW_DSF));
                    }
                    /*strcpy(pRdskXP->tsFullPath, _T("/dev/rdsk/"));*/ 
                    strncat (pRdskXP->tsFullPath, pRdskXP->tsRdskName, STRLEN_PATH);
                    DbgPlain(DBG_SSE_SYSTEM,
                             _T("[%s] pRdskXP->tsFullPath = %s"),
                             __FCN__, NS(pRdskXP->tsFullPath));
                }
                else
                {
                    DbgPlain(DBG_SSE_SYSTEM,
                             _T("[%s] Device name in long notation"), __FCN__);
                    strcpy(pRdskXP->tsFullPath, pRdskXP->tsRdskName);
                    DbgPlain(DBG_SSE_SYSTEM,
                             _T("[%s] pRdskXP->tsFullPath = %s"),
                             __FCN__, NS(pRdskXP->tsFullPath));
                }

                /* device begins with cXdXtX */
                if (NULL != strstr(pRdskXP->tsRdskName, _T("disk")))
                    strcpy (pRdskXP->tsRdskName, strrchr(pRdskXP->tsRdskName, _T('d'))); 
                else 
                    strcpy (pRdskXP->tsRdskName,strchr(pRdskXP->tsRdskName, _T('c')));
    
                DbgPlain (DBG_SSE_3DAPI, _T("[%s] "),pRdskXP->tsRdskName);

                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI,
                    _T("[%s] Device %s (%s, %s) was found to be LDEV %u on XP S/N %d and port %d!"),
                    __FCN__, pRdskXP->tsRdskName, pRdskXP->tsVendorId,
                    pRdskXP->tsProductId, pRdskXP->LDEVNumber, pRdskXP->SerialID, pRdskXP->port);

                pSseDevice->Dev.port = pRdskXP->port;
                /*alternate_path*/
                if (alter_path_count == 0)
                {
                    pRdsks->crc    = pSseDevice->crc;
                    pRdskXP->iIndex = pRdsks->iIndex;

                    DbgPlain (DBG_SSE_3DAPI, 
                              _T("[%s] Current XP crc = 0x%08x"), 
                              __FCN__, pRdsks->crc);

                    /* Replace bottom memcpy */
                    /* memcpy (pRdsks, pRdsk, sizeof(SMB_RAW_DISK_T));*/
                    StrCopy(pRdsks->tsRdskName, pRdskXP->tsRdskName, 
                        strlen(pRdskXP->tsRdskName));
                    StrCopy(pRdsks->tsFullPath, pRdskXP->tsFullPath, 
                        strlen(pRdskXP->tsFullPath));
                    StrCopy(pRdsks->tsFullBackName, pRdskXP->tsFullBackName, 
                        strlen(pRdskXP->tsFullBackName));
                    StrCopy(pRdsks->tsBackName, pRdskXP->tsBackName, 
                         strlen(pRdskXP->tsBackName));
                    pRdsks->iLun    = pRdskXP->iLun;
                    pRdsks->iTarget = pRdskXP->iTarget;
                    pRdsks->status  = pRdskXP->status;
                    pRdsks->iErrReported = pRdskXP->iErrReported;
                    /*pRdsks->port    = pRdskXP->port;*/

                    /* device begins with cXdXtX */ 
                    if(strstr(pRdsks->tsRdskName, _T("disk"))) 
                        strcpy (pRdsks->alternate_paths, strrchr(pRdsks->tsRdskName, _T('d'))); 
                    else
                        strcpy (pRdsks->alternate_paths, strchr(pRdsks->tsRdskName, _T('c')));

                    DbgPlain (DBG_SSE_3DAPI, 
                           _T("[%s] alternate_paths = %s "), 
                           __FCN__, pRdsks->alternate_paths);
                }
                else
                {
                    strcat (pRdsks->alternate_paths, _T(",")); 
                    if (strchr(pRdskXP->tsRdskName, _T('i'))) 
                       strcat (pRdsks->alternate_paths, strrchr(pRdskXP->tsRdskName, _T('d'))); 
                    else 
                       strcat (pRdsks->alternate_paths, strchr(pRdskXP->tsRdskName, _T('c')));
                }

                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI,
                          _T("[%s] alternate_paths = %s , alter_path_count = %d"),
                          __FCN__, pRdsks->alternate_paths,alter_path_count);
                    
                alter_path_count++;
                    
                /* break;*/
            }
        }
    }

    if (alter_path_count==0)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Failed to find match for LDEV %u on XP S/N %u!"),
            __FCN__, pSseDevice->Dev.ldevno, pSseDevice->Dev.serial);
        pSseDevice->iStatus = LDEV_INVALID;
        pRdsks->status =  RDSK_ER;
    }

    DbgFcnOut();
    return (retVal);
}  /* MapLdevtoRdsk */

int
SseMapLdevToRdskEx (
            PSSE_DEVICE_T       *pSseDevice,
            PSMB_RAW_DISK_T     *pRdsks,
            int                 iSseDeviceCount,
            BOOL                BackupHost)
{
    int              retVal = RETURN_OK;
    PSMB_RAW_DISK_T  *pRdskListOldDSF = NULL;
    int              iRdskCountOldDSF = 0;
    unsigned long    j;
    XP_RAW_DISK_T    *pRdskListXPOldDSF = NULL;
    int              iRdskCountXPOldDSF = 0;
    PSMB_RAW_DISK_T  *pRdskListNewDSF = NULL;
    int              iRdskCountNewDSF = 0;
    XP_RAW_DISK_T    *pRdskListXPNewDSF = NULL;
    int              iRdskCountXPNewDSF = 0;
    PSMB_RAW_DISK_T  *pRdskListCDSF = NULL;
    int              iRdskCountCDSF = 0;
    XP_RAW_DISK_T    *pRdskListXPCDSF = NULL;
    int              iRdskCountXPCDSF = 0;

    ERH_FUNCTION (_T("SseMapLdevToRdskEx"));    
    
    DbgFcnIn();

    GetAllRdsksEx (&pRdskListOldDSF, &iRdskCountOldDSF, 0);
    GetAllRdsksEx (&pRdskListNewDSF, &iRdskCountNewDSF, 1);
    GetAllRdsksEx (&pRdskListCDSF, &iRdskCountCDSF, 2);

    if ( 0 == iRdskCountOldDSF && 0 == iRdskCountNewDSF && 0 == iRdskCountCDSF)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] GetAllRdsks failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    DbgStamp (DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, 
              _T("[%s] Allocating struct pRdskListXPOldDSF (iRdskCount=%d)!"), 
              __FCN__, iRdskCountOldDSF);

    /* Allocate space for all rdsks */
    if (iRdskCountOldDSF > 0)
    {
        if ( ((pRdskListXPOldDSF = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCountOldDSF)) == NULL) || 
             (memset (pRdskListXPOldDSF, 0, sizeof(XP_RAW_DISK_T)*iRdskCountOldDSF) != pRdskListXPOldDSF) )
        {
            /* Critical error */
            ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
     }

    DbgStamp (DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, _T("[%s] Allocating struct pRdskListXPNewDSF (iRdskCount=%d)!"), __FCN__, iRdskCountNewDSF);

    /* Allocate space for all rdsks */
    if (iRdskCountNewDSF > 0)
    {
        if ( ((pRdskListXPNewDSF = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCountNewDSF)) == NULL) || 
             (memset (pRdskListXPNewDSF, 0, sizeof(XP_RAW_DISK_T)*iRdskCountNewDSF) != pRdskListXPNewDSF) )
        {
            /* Critical error */
            ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
     }

     DbgStamp (DBG_SSE_3DAPI);
     DbgPlain (DBG_SSE_3DAPI, _T("[%s] Allocating struct pRdskListXPCDSF (iRdskCount=%d)!"), __FCN__, iRdskCountCDSF);

     /* Allocate space for all rdsks */
    if (iRdskCountCDSF > 0)
    {
        if ( ((pRdskListXPCDSF = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCountCDSF)) == NULL) ||
         (memset (pRdskListXPCDSF, 0, sizeof(XP_RAW_DISK_T)*iRdskCountCDSF) != pRdskListXPCDSF) )
        {
            /* Critical error */
            ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }
    }

    /* Collect SCSI Inquiry info once for all XP rdsks */
    if (NULL != pRdskListOldDSF)
    {
        CollectXPdiskSCSIinq (pRdskListOldDSF, iRdskCountOldDSF, pRdskListXPOldDSF, &iRdskCountXPOldDSF);
    }   
    if (NULL != pRdskListNewDSF)
    {
        CollectXPdiskSCSIinq (pRdskListNewDSF, iRdskCountNewDSF, pRdskListXPNewDSF, &iRdskCountXPNewDSF);
    }
    if (NULL != pRdskListCDSF)
    {
        CollectXPdiskSCSIinq (pRdskListCDSF, iRdskCountCDSF, pRdskListXPCDSF, &iRdskCountXPCDSF);
    }
 
    DbgStamp (DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, _T("[%s] Scsi Inquiry finished for all rdsks!"), __FCN__);

    for (j=0; j<iSseDeviceCount; j++)
    {
        DbgStamp (DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI,
            _T("[%s] rdskName %s (LDEV %u on XP S/N %d)"),
            __FCN__, pRdsks[j]->tsRdskName, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);

        /* Using new DSF if original was specified in new DSF or if there are no legacy files *
         * (customer may have disabled them on the backup host.                               */
        if (NULL != strstr(pRdsks[j]->tsRdskName, _T("disk")) || 0 == iRdskCountXPOldDSF)
        {
            if (NULL != strstr(pRdsks[j]->tsRdskName, _T("cdisk")) && (iRdskCountXPCDSF != 0))
            {
                MapLdevToRdsk (pSseDevice[j], pRdsks[j], pRdskListXPCDSF, iRdskCountXPCDSF);
                strcpy(DSF_RAW_PATH, RAW_CDSF);
                strcpy(DSF_BLOCK_PATH , BLOCK_CDSF );
                if (pRdsks[j]->status == RDSK_OK)
                    continue;
            }
            MapLdevToRdsk (pSseDevice[j], pRdsks[j], pRdskListXPNewDSF, iRdskCountXPNewDSF);
            strcpy(DSF_RAW_PATH, NEW_RAW_DSF);
            strcpy(DSF_BLOCK_PATH , NEW_BLOCK_DSF );
        }
        else
        {
            MapLdevToRdsk (pSseDevice[j], pRdsks[j], pRdskListXPOldDSF, iRdskCountXPOldDSF);
            strcpy(DSF_RAW_PATH, OLD_RAW_DSF );
            strcpy(DSF_BLOCK_PATH, OLD_BLOCK_DSF);
        }
    }

end:
    FREE(pRdskListXPOldDSF);
    FREE(pRdskListXPNewDSF);
    FREE(pRdskListXPCDSF);

    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseMapLdevToRdskEx */

#elif TARGET_WIN32


/*========================================================================*//**
*
* @ingroup   SSEA_API
*
* @param     PSSE_DEVICE_T    - ptr to device array
*            PSMB_RAW_DISK_T  - ptr to disk array
*            int              - disk array entries count
*            BOOL             - backup host specifics
*
* @retval    RETURN_OK or RETURN_ER indicating function succeeded
*            or failed
*
* @brief     Maps ldev to rdsk
*
*//*=========================================================================*/
int
SseMapLdevToRdskEx (PSSE_DEVICE_T       *pSseDevice,
                    PSMB_RAW_DISK_T     *pRdsks,
                    int                 iSseDeviceCount,
                    BOOL                BackupHost )

{
    int retVal = RETURN_OK;
    PSMB_RAW_DISK_T *pRdskList = NULL;
    int              iRdskCount = 0;
    PSMB_RAW_DISK_T pRdsk;
    SSE_DEVICE_T Ldev;
    long i, j;
    XP_RAW_DISK_T   *pRdskXP      = NULL,
                    *pRdskListXP = NULL;
    int             iRdskCountXP = 0;

    ERH_FUNCTION (_T("SseMapLdevToRdskEx"));    
    
    DbgFcnIn();

    if((retVal = GetAllRdsks (&pRdskList, &iRdskCount)) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] GetAllRdsks failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    /* Allocate space for all rdsks */
    if ( ((pRdskListXP = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCount)) == NULL) || 
         (memset (pRdskListXP, 0, sizeof(XP_RAW_DISK_T)*iRdskCount) != pRdskListXP) )
    {
        /* Critical error */
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }
    /* Collect SCSI Inquiry info once for all XP rdsks */
    for (i=0; i<iRdskCount; i++)
    {
        pRdsk = pRdskList[i];

        if (pRdsk->status != RDSK_OK)
            continue;

        if (CheckVendorId(pRdsk->tsVendorId) && 
            strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0)
        {
            if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
            { /* Error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] ScsiI(pRdskListXP[inquiryXp failed for %s!"),
                    __FCN__, pRdsk->tsRdskName);
            }
            /* Store rdsk structure values */
            StrCopy (pRdskListXP[iRdskCountXP].tsRdskName, pRdsk->tsRdskName, 
                    strlen(pRdsk->tsRdskName));
            StrCopy (pRdskListXP[iRdskCountXP].tsVendorId, pRdsk->tsVendorId, 
                    strlen(pRdsk->tsVendorId));
            StrCopy (pRdskListXP[iRdskCountXP].tsProductId, pRdsk->tsProductId, 
                    strlen(pRdsk->tsProductId));
            StrCopy (pRdskListXP[iRdskCountXP].tsRevision, pRdsk->tsRevision, 
                    strlen(pRdsk->tsRevision));

            pRdskListXP[iRdskCountXP].dwPDrive   = pRdsk->dwPDrive;
            pRdskListXP[iRdskCountXP].SerialID   = Ldev.Dev.serial;
            pRdskListXP[iRdskCountXP].LDEVNumber = Ldev.Dev.ldevno;
            pRdskListXP[iRdskCountXP].port       = Ldev.Dev.port;
            pRdskListXP[iRdskCountXP].status     = pRdsk->status; 
                
            /* Copy struct pRdskXp to struct. list pRdskListXP */    
            /*memcpy( pRdskListXP [iRdskCountXP], pRdskXP, sizeof(XP_RAW_DISK_T));*/
            
            DbgStamp (DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdskListXP[%d] = \n\n\t tsRdskName  = [%s]\n\t tsVendorID  = [%s]\n\t tsProductId = [%s]\n\t tsRevision  = [%s]\n\t SerialID    = %d\n\t LDEVNumber  = %d\n\t port        = %d\n\t status      = %d\n"),  
                                        __FCN__, iRdskCountXP, 
                                        pRdskListXP [iRdskCountXP].tsRdskName, 
                                        pRdskListXP [iRdskCountXP].tsVendorId, 
                                        pRdskListXP [iRdskCountXP].tsProductId,
                                        pRdskListXP [iRdskCountXP].tsRevision, 
                                        pRdskListXP [iRdskCountXP].SerialID, 
                                        pRdskListXP [iRdskCountXP].LDEVNumber, 
                                        pRdskListXP [iRdskCountXP].port, 
                                        pRdskListXP [iRdskCountXP].status);
        
            iRdskCountXP++;
       }
    }



    /* Collect SCSI Inq. info once for all rdsks */
    for (j=0; j<iSseDeviceCount; j++)
    {
        DbgStamp (DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI,
            _T("[%s] LDEV %u on XP S/N %d"),
            __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);

        for (i=0; i<iRdskCount; i++)
        {
            pRdskXP = pRdskListXP + i;

            if (pRdskXP->status != RDSK_OK)
                continue;

            if (CheckVendorId(pRdskXP->tsVendorId) && 
                strncmp(pRdskXP->tsProductId, _T("OPEN"), 4) == 0)
            {
                /* Check for Serial and Device number */ 
                if (pRdskXP->SerialID   == pSseDevice[j]->Dev.serial && 
                    pRdskXP->LDEVNumber == pSseDevice[j]->Dev.ldevno)
                {
                    /*tchar tsRdskName[STRLEN_PATH+1];
                    tchar *tsSPos;*/

                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                        _T("[%s] Device %s, %lu (%s, %s) was found to be LDEV %u on XP S/N %d and port %d!"),
                        __FCN__, pRdskXP->tsRdskName, pRdskXP->dwPDrive, pRdskXP->tsVendorId,
                        pRdskXP->tsProductId, pRdskXP->LDEVNumber, pRdskXP->SerialID, pRdskXP->port);

                    strcpy(pRdskXP->tsFullPath, pRdskXP->tsRdskName);
                    DbgPlain(DBG_SSE_SYSTEM,
                             _T("[%s] pRdskXP->tsFullPath = %s"),
                             __FCN__, NS(pRdskXP->tsFullPath));

                    pRdskXP->iIndex = pRdsks[j]->iIndex;
                    pRdsks[j]->crc    = pSseDevice[j]->crc;

                    DbgPlain (DBG_SSE_3DAPI, 
                              _T("[%s] Current XP crc = 0x%08x"), 
                              __FCN__, pRdsks[j]->crc);

                    /* Replace bottom memcpy */
                    /* memcpy (pRdsks[j], pRdsk, sizeof(SMB_RAW_DISK_T));*/
                    StrCopy(pRdsks[j]->tsRdskName, pRdskXP->tsRdskName, 
                        strlen(pRdskXP->tsRdskName));
                    StrCopy(pRdsks[j]->tsFullPath, pRdskXP->tsFullPath, 
                        strlen(pRdskXP->tsFullPath));
                    StrCopy(pRdsks[j]->tsBackName, pRdskXP->tsBackName, 
                        strlen(pRdskXP->tsBackName));
                    StrCopy(pRdsks[j]->tsFullBackName, pRdskXP->tsFullBackName, 
                        strlen(pRdskXP->tsFullBackName));
                    pRdsks[j]->dwPDrive= pRdskXP->dwPDrive;
                    pRdsks[j]->iLun    = pRdskXP->iLun;
                    pRdsks[j]->iTarget = pRdskXP->iTarget;
                    pRdsks[j]->status  = pRdskXP->status;
                    pRdsks[j]->port    = pRdskXP->port;
                    pRdsks[j]->iErrReported = pRdskXP->iErrReported;

                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdsks[%d] = \n{\n\t tsRdskName  = [%s]\n\t tsFullPath  = [%s]\n\t tsBackName  = [%s]\n\t tsFullBackName = [%s]\n\t dwPDrive       = [%d]\n\t iLun        = [%d]\n\t iTarget     = %d\n\t iErrReported= %d\n\t status      = %d\n}"),  
                              __FCN__, j, 
                              pRdsks[j]->tsRdskName, 
                              pRdsks[j]->tsFullPath, 
                              pRdsks[j]->tsBackName,
                              pRdsks[j]->tsFullBackName,
                              pRdsks[j]->dwPDrive, 
                              pRdsks[j]->iLun, 
                              pRdsks[j]->iTarget, 
                              pRdsks[j]->iErrReported, 
                              pRdsks[j]->status);
                    
                    break;
                }
            }
        }

        if (i==iRdskCount)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] Failed to find match for LDEV %lu on XP S/N %lu!"),
                __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);
            pSseDevice[j]->iStatus = LDEV_INVALID;
            pRdsks[j]->status =  RDSK_ER;
        }
    }

    if ( BackupHost ) 
    {
        DbgPlain(DBG_SSE_3DAPI, _T("[%s] This is R2, skipping cluster and disk sig. resolving"), __FCN__ );
    }
    else if ( retVal == RETURN_OK )
    {
        GetWinDiskSignatures(pRdsks, iSseDeviceCount);
        GetWinClusterResources(pRdsks, iSseDeviceCount);
    }

end:
    FREE(pRdskListXP);
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseMapLdevToRdskEx */
#elif TARGET_LINUX

int
SseMapLdevToRdskEx (
            PSSE_DEVICE_T       *pSseDevice,
            PSMB_RAW_DISK_T     *pRdsks,
            int                 iSseDeviceCount,
            BOOL                BackupHost )
{
    int retVal = RETURN_OK;
    PSMB_RAW_DISK_T *pRdskList = NULL;
    int              iRdskCount = 0;
    PSMB_RAW_DISK_T pRdsk;
    int             alter_path_count=0;
    SSE_DEVICE_T    Ldev;
    unsigned long   i, j;
    XP_RAW_DISK_T   *pRdskXP     = NULL,
                    *pRdskListXP = NULL;
    int             iRdskCountXP = 0;


    ERH_FUNCTION (_T("SseMapLdevToRdskEx"));    
    
    DbgFcnIn();

    if((retVal = GetAllRdsks (&pRdskList, &iRdskCount)) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] GetAllRdsks failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }
   
    /* Allocate space for all rdsks */
    if ( ((pRdskListXP = (XP_RAW_DISK_T *) MALLOC (sizeof(XP_RAW_DISK_T)*iRdskCount)) == NULL) || 
         (memset (pRdskListXP, 0, sizeof(XP_RAW_DISK_T)*iRdskCount) != pRdskListXP) )
    {
        /* Critical error */
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SMB_MALLOC_FAILED));
           
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Memory allocation failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }
   
    
    /* Collect SCSI Inq. info once for all rdsks */
    for (i=0; i<iRdskCount; i++)
    {
        pRdsk = pRdskList[i];

        if (pRdsk->status != RDSK_OK)
            continue;

        if (CheckVendorId(pRdsk->tsVendorId) && 
            strncmp(pRdsk->tsProductId, _T("OPEN"), 4) == 0)
        {
            if (ScsiInquiryXp (pRdsk, &Ldev) != RETURN_OK)
            { /* Error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] ScsiInquiryXp failed for %s!"),
                    __FCN__, pRdsk->tsRdskName);
            }

            /* Store rdsk structure values */
            StrCopy (pRdskListXP[iRdskCountXP].tsRdskName, pRdsk->tsRdskName, 
                    strlen(pRdsk->tsRdskName));
            StrCopy (pRdskListXP[iRdskCountXP].tsVendorId, pRdsk->tsVendorId, 
                    strlen(pRdsk->tsVendorId));
            StrCopy (pRdskListXP[iRdskCountXP].tsProductId, pRdsk->tsProductId, 
                    strlen(pRdsk->tsProductId));
            StrCopy (pRdskListXP[iRdskCountXP].tsRevision, pRdsk->tsRevision, 
                    strlen(pRdsk->tsRevision));
            
            pRdskListXP[iRdskCountXP].SerialID   = Ldev.Dev.serial;
            pRdskListXP[iRdskCountXP].LDEVNumber = Ldev.Dev.ldevno;
            pRdskListXP[iRdskCountXP].port       = Ldev.Dev.port;
            pRdskListXP[iRdskCountXP].status     = pRdsk->status; 
                
            DbgStamp (DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI, _T("[%s] pRdskListXP[%d] = \n{\n\t"
                                        " tsRdskName  = [%s]\n\t"
                                        " tsVendorID  = [%s]\n\t"
                                        " tsProductId = [%s]\n\t"
                                        " tsRevision  = [%s]\n\t"
                                        " SerialID    = %d\n\t"
                                        " LDEVNumber  = %d\n\t"
                                        " port        = %d\n\t"
                                        " status      = %d\n}"),  
                                        __FCN__, iRdskCountXP,
                                        pRdskListXP [iRdskCountXP].tsRdskName,
                                        pRdskListXP [iRdskCountXP].tsVendorId, 
                                        pRdskListXP [iRdskCountXP].tsProductId,
                                        pRdskListXP [iRdskCountXP].tsRevision, 
                                        pRdskListXP [iRdskCountXP].SerialID, 
                                        pRdskListXP [iRdskCountXP].LDEVNumber, 
                                        pRdskListXP [iRdskCountXP].port, 
                                        pRdskListXP [iRdskCountXP].status);
 
            iRdskCountXP++;
        }
    }


    for (j=0; j<iSseDeviceCount; j++)
    {
        DbgStamp (DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI,
            _T("[%s] LDEV %u on XP S/N %d"),
            __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);

        alter_path_count = 0;
        DbgPlain(DBG_SSE_3DAPI, _T("in loop:%s"), pRdsks[j]->tsRdskName);
        
        /* For each LDEV, try to find appropriate Rdsk */
        for (i=0; i<iRdskCountXP; i++)
        {
            /* Select new rdsk from XP rdsk list */
            pRdskXP = pRdskListXP + i;

            if (pRdskXP->status != RDSK_OK)
                continue;

            /* If the IDs of rdsk are appropriate  */
            if (CheckVendorId(pRdskXP->tsVendorId) && 
                strncmp(pRdskXP->tsProductId, _T("OPEN"), 4) == 0)
            {
                /* Check for Serial and Device number */ 
                if (pRdskXP->SerialID   == pSseDevice[j]->Dev.serial && 
                    pRdskXP->LDEVNumber == pSseDevice[j]->Dev.ldevno)
                {
                    tchar tsRdskName[STRLEN_PATH+1] = {0};
                    tchar *tsSPos=NULL;

                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                        _T("[%s] Device %s (%s, %s) was found to be LDEV %u on XP S/N %d and port %d!"),
                        __FCN__, pRdskXP->tsRdskName, pRdskXP->tsVendorId,
                        pRdskXP->tsProductId, pRdskXP->LDEVNumber, pRdskXP->SerialID, pRdskXP->port);

                    if (StrStrNS(pRdskXP->tsRdskName, _T("/dev")))
                    {
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] Device name in long notation"), __FCN__);
                        StrCopy(pRdskXP->tsFullPath, pRdskXP->tsRdskName, STRLEN_PATH);
                        
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] pRdskXP->tsFullPath = %s"),
                                 __FCN__, NS(pRdskXP->tsFullPath));
                    }
                    else
                    {
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] Device name in short notation"), __FCN__);
                        strcpy(pRdskXP->tsFullPath, _T("/dev/"));
                        strncat(pRdskXP->tsFullPath, pRdskXP->tsRdskName, STRLEN_PATH);
                        DbgPlain(DBG_SSE_SYSTEM,
                                 _T("[%s] pRdskXP->tsFullPath = %s"),
                                 __FCN__, NS(pRdskXP->tsFullPath));
                    }

                    strncpy (tsRdskName, pRdskXP->tsRdskName, STRLEN_PATH);
                    DbgPlain(10, _T("tsRdskName:%s pRdskXP->tsRdskName:%s"), tsRdskName, pRdskXP->tsRdskName);
                    if ((tsSPos = strrchr (tsRdskName, _T('s'))) == NULL)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                                _T("[%s] Invalid disk name!"), __FCN__);
                        retVal = RETURN_ER;
                        goto end;
                    }

                    /*alternate_path*/
                    if (alter_path_count == 0)
                    {
                        pRdsks[j]->crc    = pSseDevice[j]->crc;
                        
                        DbgPlain (DBG_SSE_3DAPI, 
                                  _T("[%s] Current XP crc = 0x%08x"), 
                                  __FCN__, pRdsks[j]->crc);
                                                
                        /*strcpy(pRdsks[j]->tsFullPath,  pRdskXP->tsFullPath);*/
                        StrCopy(pRdsks[j]->tsFullPath,pRdskXP->tsFullPath,STRLEN_PATH);
                        DbgPlain(10, _T("pRdsks[j]->tsFullPath:%s pRdskXP->tsFullPath:%s"), pRdsks[j]->tsFullPath, pRdskXP->tsFullPath);
                        strcpy(pRdsks[j]->tsFullBackName, pRdskXP->tsFullBackName);
                        strcpy(pRdsks[j]->tsBackName,  pRdskXP->tsBackName);
                        strcpy(pRdsks[j]->tsVendorId,  pRdskXP->tsVendorId);
                        strcpy(pRdsks[j]->tsProductId, pRdskXP->tsProductId);
                        strcpy(pRdsks[j]->tsRevision,  pRdskXP->tsRevision);
                        /*strcpy(pRdsks[j]->tsSliceName, pRdskXP->tsSliceName);*/

                        pRdsks[j]->iLun    = pRdskXP->iLun;
                        pRdsks[j]->iTarget = pRdskXP->iTarget;
                        pRdsks[j]->status  = pRdskXP->status;
                        pRdsks[j]->iErrReported = pRdskXP->iErrReported;
                        /* pRdsks[j]->port = pRdskXP->port; */
                        strcpy (pRdsks[j]->alternate_paths, tsRdskName);
                        strcpy (pRdsks[j]->tsRdskName, tsRdskName);
                        DbgStamp (DBG_SSE_3DAPI);
                        DbgPlain (DBG_SSE_3DAPI,
                                  _T("[%s] alternate_paths = %s ,"
                                     "tsRdskName =%s,pRdsks[j]->tsRdskName=%s "),
                                  __FCN__, pRdsks[j]->alternate_paths, tsRdskName, pRdsks[j]->tsRdskName);
                        
                        pSseDevice[j]->Dev.port = pRdskXP->port; 
                    } 
                    else
                    {
                        if(strstr(pRdsks[j]->alternate_paths, tsRdskName)==NULL)
                        {
                            strcat (pRdsks[j]->alternate_paths, _T(","));
                            strcat (pRdsks[j]->alternate_paths, tsRdskName);
                        } 
                    }
                    
                    DbgStamp (DBG_SSE_3DAPI);
                    DbgPlain (DBG_SSE_3DAPI,
                              _T("[%s] alternate_paths = %s , alter_path_count = %d , tsRdskName[j]=%s,tsRdskName=%s"),
                              __FCN__, pRdsks[j]->alternate_paths,alter_path_count, pRdsks[j]->tsRdskName,tsRdskName);
                    
                    alter_path_count++;
                }
            }
        }
        
        if (alter_path_count==0)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                      _T("[%s] Failed to find match for LDEV %u on XP S/N %u!"),
                      __FCN__, pSseDevice[j]->Dev.ldevno, pSseDevice[j]->Dev.serial);
            pSseDevice[j]->iStatus = LDEV_INVALID;
            pRdsks[j]->status =  RDSK_ER;
        }
    }
    
end:
    FREE(pRdskListXP);

    DbgFcnOutRet (retVal);
    return (retVal);
    
} /* SseMapLdevToRdskEx */
#endif

int
SseConsistencyCheck (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T       *pSseDev,
    int                 iCount,
    unsigned char       mmode
)
{
    int retVal = RETURN_OK;
    int i, iCmd; 
    cmddevinfo      *pCmd = {0};
    pairstatinfo    pstat;
    PSSE_DEVICE_T   pDev;


    ERH_FUNCTION (_T("SseConsistencyCheck"));   
    DbgFcnIn();
 
    for (i = 0; i < iCount; i ++) 
    {
        pDev = pSseDev[i];

        if (pDev->iStatus == LDEV_OK)
        {
            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                {
                    pCmd = pCmdDev[iCmd];
                    break;
                }

            while (1)
            {
                if (iCmd == iCmdCount)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] Cannot find command device for serial number %d!"),
                        __FCN__, pDev->Dev.serial);

                    break;
                }

                if (SseXPairVolStat (pCmd, &(pDev->Dev), &pstat, mmode, 1) 
                   != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairVolStat failed!"), __FCN__);

                    break;
                }

                if(pstat.smod == 1)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                        _T("[%s] Mirror disk %d,%d was modified!"), __FCN__, 
                        pDev->Dev.serial, pDev->Dev.ldevno);

                }

                break;
            }

            if (pDev->iStatus != LDEV_OK)
            {
                if (mmode == MD_MRCF)
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_SPLIT_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
                else
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_CA_SPLIT_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial));
            }
        }
    }

/*end:*/
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseConsistencyCheck */

int
SseSplit (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T       *pSseDev,
    int                 iCount,
    unsigned char       mmode
)
{
    int retVal = RETURN_OK;
    int iDev, iCmd; 
    int CTGIDCount = 0;
    int *CTGIDs = NULL;
    int *CTSerials = NULL;

    cmddevinfo      *pCmd;
    PSSE_DEVICE_T   pDev;

    ERH_FUNCTION (_T("SseSplit"));   
    DbgFcnIn();

    //Normal split
    if (opt.atomic_split == 0)
    {
        for (iDev = 0; iDev < iCount; iDev++) 
        {
            pDev = pSseDev[iDev];

            if (pDev->iStatus == LDEV_OK)
            {
                pCmd = NULL;
                for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                    if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                    {
                        pCmd = pCmdDev[iCmd];
                        break;
                    }

                while (1)
                {
                    if (pCmd == NULL)
                    {
                        pDev->iStatus = LDEV_INVALID;

                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                            _T("[%s] Cannot find command device for serial number %d!"),
                            __FCN__, pDev->Dev.serial);

                        break;
                    }
                    SseSingleSplit (pCmd, pDev, S_RDWT, mmode);

                    break;
                }

                if (pDev->iStatus != LDEV_OK)
                {
                    if (mmode == MD_MRCF)
                        ErhFullReport (__FCN__, ERH_MAJOR,
                            NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_SPLIT_FAILED,
                            pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
                    else
                        ErhFullReport (__FCN__, ERH_MAJOR,
                            NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_CA_SPLIT_FAILED,
                            pDev->Dev.ldevno, pDev->Dev.serial));
                }
            }
        }
    }
    //Atomic split
    else
    {
        int iCT = 0;

        GetCTGIDs (iCount, pSseDev, &CTGIDCount, &CTGIDs, &CTSerials);

        //Loop through all CTGIDs and mark errors.
        for (iCT = 0; iCT < CTGIDCount; iCT++) 
        {
            for (iDev = 0; iDev < iCount; iDev++)
            {
                if ((pSseDev[iDev]->ctgid == CTGIDs[iCT]) 
                    && (pSseDev[iDev]->Dev.serial == CTSerials[iCT])
                    && (pSseDev[iDev]->iStatus != LDEV_OK))
                {
                    MarkFailedCTGroup (CTGIDs[iCT], pSseDev[iDev]->Dev.serial, iCount, pSseDev);
                    break;
                }
            }
        }

        //Split all CT groups
        for (iCT = 0; iCT < CTGIDCount; iCT++) 
        {
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] Attempting to split a CT group with ctgid='%d' on array '%d'."),
                __FCN__, CTGIDs[iCT], CTSerials[iCT] );

            for (iDev = 0; iDev < iCount; iDev++)
            {
                if ((pSseDev[iDev]->ctgid == CTGIDs[iCT]) && (pSseDev[iDev]->Dev.serial == CTSerials[iCT]))
                {
                    pDev = pSseDev[iDev];

                    if (pDev->iStatus == LDEV_OK)
                    {
                        pCmd = NULL;

                        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                            if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                            {
                                pCmd = pCmdDev[iCmd];
                                break;
                            }

                        do
                        {
                            if (pCmd == NULL)
                            {
                                pDev->iStatus = LDEV_INVALID;

                                DbgStamp (DBG_UNEXPECTED);
                                DbgPlain (DBG_UNEXPECTED, 
                                    _T("[%s] Cannot find command device for serial number %d!"),
                                    __FCN__, pDev->Dev.serial);

                                break;
                            }

                            SseSingleSplit (pCmd, pDev, S_RDWT | S_GROUP, mmode);
                        }
                        while (0);

                        if (pDev->iStatus != LDEV_OK)
                        {
                            MarkFailedCTGroup (CTGIDs[iCT], pDev->Dev.serial, iCount, pSseDev);

                            ErhFullReport (__FCN__, ERH_MAJOR,
                                NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_SPLIT_FAILED,
                                pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
                        }
                        
                        //We've split this group.
                        break;
                    }
                }
            }
        }

        FREE (CTGIDs);
        FREE (CTSerials);

        //Split any leftover disks.
        for (iDev = 0; iDev < iCount; iDev++) 
        {
            pDev = pSseDev[iDev];

            if (pDev->ctgid != 0)
            {
                continue;
            }

            if (pDev->iStatus == LDEV_OK)
            {
                pCmd = NULL;
                for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                    if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                    {
                        pCmd = pCmdDev[iCmd];
                        break;
                    }

                do
                {
                    if (pCmd == NULL)
                    {
                        pDev->iStatus = LDEV_INVALID;

                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                            _T("[%s] Cannot find command device for serial number %d!"),
                            __FCN__, pDev->Dev.serial);

                        break;
                    }

                    SseSingleSplit (pCmd, pDev, S_RDWT, mmode);
                }
                while (0);

                if (pDev->iStatus != LDEV_OK)
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_SPLIT_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
            }
        }
    }
 
/*end:*/
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseSplit */


int
SseSingleSplit (
    cmddevinfo *pCmd, 
    PSSE_DEVICE_T pDev,
    int SPLIT_FLAGS,
    unsigned char mmode
)
{
    int retVal = RETURN_OK;
    pairstatinfo    pstat;

    ERH_FUNCTION (_T("SseSingleSplit"));   
    DbgFcnIn();

    if (SseXPairVolStat (pCmd, &(pDev->Dev), &pstat, mmode, 1) 
    != RETURN_OK)
    {
        pDev->iStatus = LDEV_INVALID;

        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] SseXPairVolStat failed!"), __FCN__);

        goto end;
    }

    if (SseXPairSplit (pCmd, &(pDev->Dev), SPLIT_FLAGS, mmode) 
    != RETURN_OK)
    {
        pDev->iStatus = LDEV_INVALID;

        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] SseXPairSplit failed!"), __FCN__);

        goto end;
    }

    if (SseXPairVolStat (pCmd, &(pDev->Dev), &pstat, mmode, 1) 
    != RETURN_OK)
    {
        pDev->iStatus = LDEV_INVALID;

        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] SseXPairVolStat failed!"), __FCN__);

        goto end;
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} 


int
SseResync (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDev,
    int            iCount,
    unsigned char  mmode
)
{
    int     retVal = RETURN_OK;
    int     i, iCmd;
    pairstatinfo   Pair;
    PSSE_DEVICE_T  pDev;
    
    ERH_FUNCTION (_T("SseResync"));   
    DbgFcnIn();
 
    DbgStamp(DBG_SSE_3DAPI);

    for (i = 0; i < iCount; i ++) 
    {
        pDev = pSseDev[i];

        if (pDev->iStatus == LDEV_OK)
        {
            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                    break;

            while (1)
            {
                if (iCmd == iCmdCount)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] Cannot find command device for serial number %d!"),
                        __FCN__, pDev->Dev.serial);

                    break;
                }

                if (SseXPairVolStat (pCmdDev[iCmd], &(pDev->Dev), &Pair, 
                    mmode, 1) != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairVolStat failed!"), __FCN__);

                    break;
                }

                if ((Pair.status == STAT_PAIR) ||
                    (Pair.status == STAT_PFUL))
                {
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                        _T("[%s] Ldev %u in %d already in sync!"), __FCN__,
                        pDev->Dev.ldevno, pDev->Dev.serial);

                    break;
                }

                if (Pair.status != STAT_PSUS &&
                    Pair.status != STAT_PFUS)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairResync failed!"), __FCN__);

                    break;
                }

                if (SseXPairResync (pCmdDev[iCmd], &(pDev->Dev), mmode)
                    != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairResync failed!"), __FCN__);

                    break;
                }

                if (SseXPairVolStat (pCmdDev[iCmd], &(pDev->Dev), &Pair, 
                    mmode, 1) != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairVolStat failed!"), __FCN__);

                    break;
                }

                break;
            }
        
            if (pDev->iStatus != LDEV_OK)
            {
                if (mmode == MD_MRCF)
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_RESYNC_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
                else
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_CA_RESYNC_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial));
            }
        }
    }

/*end:*/
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseResync */

int
SseRestore (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDev,
    int            iCount,
    unsigned char  mmode
)
{
    int     retVal = RETURN_OK;
    int i, iCmd;
    pairstatinfo Pair;
    PSSE_DEVICE_T  pDev;

    ERH_FUNCTION (_T("SseRestore"));   
    DbgFcnIn();
 
    if (mmode != MD_MRCF) /* only BC is supported*/
    {
        /* Invalid parameters */
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s)!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    for (i = 0; i < iCount; i ++) 
    {
        pDev = pSseDev[i];

        if (pDev->iStatus == LDEV_OK)
        {
            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pDev->Dev.serial)
                    break;

            while (1)
            {
                if (iCmd == iCmdCount)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] Cannot find command device for serial number %d!"),
                        __FCN__, pDev->Dev.serial);

                    break;
                }

                if (SseXPairVolStat (pCmdDev[iCmd], &(pDev->Dev), &Pair, 
                    mmode, 1) != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairResync failed!"), __FCN__);

                    break;
                }

                if ((Pair.status == STAT_PAIR) ||
                    (Pair.status == STAT_PFUL))
                {
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                        _T("[%s] Ldev %u in %d already in sync!"), __FCN__,
                        pDev->Dev.ldevno, pDev->Dev.serial);

                    break;
                }

                if ((Pair.status != STAT_PSUS) &&
                    (Pair.status != STAT_PFUS))
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairResync failed!"), __FCN__);

                    break;
                }
                

                if (SseXPairRestore (pCmdDev[iCmd], &(pDev->Dev), mmode)
                    != RETURN_OK)
                {
                    pDev->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairResync failed!"), __FCN__);

                    break;
                }

                break;
            }
        
            if (pDev->iStatus != LDEV_OK)
            {
                if (mmode == MD_MRCF)
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_BC_RESTORE_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial, pDev->Dev.mun));
                else
                    ErhFullReport (__FCN__, ERH_MAJOR,
                        NlsGetMessage (NLS_SET_SSE, NLS_SSE_RMLIB_CA_RESTORE_FAILED,
                        pDev->Dev.ldevno, pDev->Dev.serial));
            }
        }
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseRestore */


int
SseWaitStatus (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDevice,
    int            iCount,
    unsigned char ucPairStatus, /* same behaviour for PAIR and PFUL or PSUS and PFUS*/
    unsigned char  mmode
)
{
    int     retVal = RETURN_OK;
    int i, iCmd, iWait,iCopy;
    pairstatinfo Pair;
    int iIteration, iValidDisk, iProgress, iProc1, iProc2;

    ERH_FUNCTION (_T("SseWaitStatus"));   
    DbgFcnIn();
 
    DbgStamp(DBG_SSE_3DAPI);

    iIteration = 0;
    iProc1 = 0;
    iProc2 = 0;
    iCopy  = 0;
    do 
    {
        iProgress = 0;
        iValidDisk = 0;
        iWait = 0;
        iProc2 = iProc1;
        iProc1 = 0;
        for (i = 0; i < iCount; i ++) 
        {
            if (pSseDevice[i]->iStatus == LDEV_OK)
            {
                iValidDisk++;

                for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                    if (pCmdDev[iCmd]->serial == pSseDevice[i]->Dev.serial)
                        break;

                if (iCmd == iCmdCount)
                {
                    pSseDevice[i]->iStatus = LDEV_INVALID;

                    ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] Cannot find command device for serial number %d!"),
                        __FCN__, pSseDevice[i]->Dev.serial);

                    retVal = RETURN_ER;
                    goto end;
                }

                if (SseXPairVolStat (pCmdDev[iCmd], &(pSseDevice[i]->Dev), 
                            &Pair, mmode, 1) != RETURN_OK)
                {
                    pSseDevice[i]->iStatus = LDEV_INVALID;

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] SseXPairVolStat failed!"), __FCN__);

                    continue;
                }

                if (Pair.status == STAT_COPY)
                {
                    iWait = 1;
                    iCopy = 1;
                    iProgress += Pair.copyrate;

                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                      _T("[%s] Ldev %u in %d in STAT_COPY state, copyrate = %d%%"), 
                      __FCN__, pSseDevice[i]->Dev.ldevno, 
                      pSseDevice[i]->Dev.serial, Pair.copyrate);
                    if (ucPairStatus == STAT_PSUS)
                        if (iIteration%opt.splitReportRate == 0)
                            ErhFullReport (__FCN__, ERH_NORMAL,
                                NlsGetMessage (NLS_SET_SSE, NLS_MSG_SSE_SPLITING_PROGRESS,
                                pSseDevice[i]->Dev.ldevno,Pair.copyrate, NLS_PERCENT));
                    
                    continue;
                }

                if (Pair.status == STAT_RCPY)
                {
                    iWait = 1;
                    iProgress += Pair.copyrate;

                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION, 
                      _T("[%s] Ldev %u in %d in STAT_RCPY state, copyrate = %d%%"), 
                      __FCN__, pSseDevice[i]->Dev.ldevno, 
                      pSseDevice[i]->Dev.serial, Pair.copyrate);

                    continue;
                }

                if (ucPairStatus == STAT_PSUS)
                {
                    if ((Pair.status == STAT_PSUS) ||
                        (Pair.status == STAT_PFUS))
                    {
                        iProgress += 100;
                        continue;
                    }
                }
                else if (ucPairStatus == STAT_PAIR)
                {
                    if ((Pair.status == STAT_PAIR) ||
                        (Pair.status == STAT_PFUL))
                    {
                        iProgress += 100;
                        continue;
                    }

                }

                pSseDevice[i]->iStatus = LDEV_INVALID;

                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Ldev %u in %d is in unexpected status %s(%d)!"), 
                    __FCN__,
                    pSseDevice[i]->Dev.ldevno, pSseDevice[i]->Dev.serial,
                DbgPrintPairStat (Pair.status), Pair.status);
                retVal = RETURN_ER;
                goto end;
            }
        }
        
        if (iWait)
        {
            if (ucPairStatus == STAT_PAIR)
            {
                iProc1 = iProgress/iValidDisk;
                    if (iIteration%opt.syncReportRate == 0)
                        if (iProc1 - iProc2 > 0)
                            ErhFullReport (__FCN__, ERH_NORMAL,
                                NlsGetMessage (NLS_SET_SSE, NLS_MSG_SSE_SYNCHRONIZING,
                                iProgress/iValidDisk, NLS_PERCENT));
                    sleep (opt.syncSleepTime);
            }
            else if (ucPairStatus == STAT_PSUS)
            {
                 DbgStamp (DBG_SSE_ACTION);
                 DbgPlain (DBG_SSE_ACTION, 
                    _T("[%s] waiting for splitSleepTime =%d"),
                    __FCN__,opt.splitSleepTime);   
                /* if (iIteration%opt.splitReportRate == 0)
                    ErhFullReport (__FCN__, ERH_NORMAL,
                        NlsGetMessage (NLS_SET_SSE, NLS_MSG_SSE_SPLITING)); */
                    sleep (opt.splitSleepTime);
            }
            else
                sleep (1);
        }
        iIteration++;
    }
    while (iWait);
    if (( ucPairStatus == STAT_PSUS ) && iCopy)
    {
        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION, 
            _T("[%s] Splitting is completed"),__FCN__);
        ErhFullReport (__FCN__, ERH_NORMAL,
            NlsGetMessage (NLS_SET_SSE, NLS_SPLIT_BCKP_COMPLETED)); 
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseWaitStatus */

static int
SseCheckBCStatus (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDevice,
    int            iCount,
    unsigned char ucPairStatus /* same behaviour for PSUS and PFUS or PAIR and PFUL*/
)
{
    int     retVal = RETURN_OK;
    int i, iCmd;
    pairstatinfo status;
    int     inv_count=0;
    tchar   *tsBuf = NULL;
    tchar tsBuf2[STRLEN_PATH+1];
    int    iBufLen = 0, iOldLen;
    tchar *newline = _T("\n\t");
    int newline_len = (int) strlen(newline);

    ERH_FUNCTION (_T("SseCheckBCStatus"));   
    DbgFcnIn();
 
    for (i = 0; i < iCount; i ++) 
    {
        if (pSseDevice[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSseDevice[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            pSseDevice[i]->iStatus = LDEV_INVALID;

            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSseDevice[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        if (SseXPairVolStat (pCmdDev[iCmd], &(pSseDevice[i]->Dev), 
                    &status, MD_MRCF, 1) != RETURN_OK)
        {
            pSseDevice[i]->iStatus = LDEV_INVALID;

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] SseXPairVolStat failed!"), __FCN__);

            continue;
        }

        if ((ucPairStatus == STAT_PAIR && status.status == STAT_PAIR) || status.status == STAT_PFUL)
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Ldev %u in %d has expected status %s(%d)!"),
                      __FCN__, pSseDevice[i]->Dev.ldevno,
                      pSseDevice[i]->Dev.serial,
                      DbgPrintPairStat (status.status),
                      status.status);
        }
        else if ((ucPairStatus == STAT_PSUS && status.status == STAT_PSUS) || status.status == STAT_PFUS)
        {
            DbgPlain (DBG_SSE_ACTION, _T("[%s] Ldev %u in %d has expected status %s(%d)!"),
                      __FCN__, pSseDevice[i]->Dev.ldevno,
                      pSseDevice[i]->Dev.serial,
                      DbgPrintPairStat (status.status),
                      status.status);
        }
        else
        {

            pSseDevice[i]->iStatus = LDEV_INVALID;

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Ldev %u in %d has unexpected status %s(%d)!")
                , __FCN__, pSseDevice[i]->Dev.ldevno, 
                pSseDevice[i]->Dev.serial,
                DbgPrintPairStat (status.status), status.status);

            sprintf (tsBuf2, 
                _T("%-7d %04Xh (%4d)  %5s  %3d  %3d  %3d  %s  %-7d %04Xh (%4d)"),
                pSseDevice[i]->Dev.serial,
                pSseDevice[i]->Dev.ldevno,
                pSseDevice[i]->Dev.ldevno,
                portno[pSseDevice[i]->Dev.port],
                pSseDevice[i]->Dev.targid,
                pSseDevice[i]->Dev.lun,
                pSseDevice[i]->Dev.mun,
                DbgPrintPairStat (status.status),
                status.pserial,
                status.pldev,
                status.pldev
            );
                
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (inv_count > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (inv_count>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            inv_count++;

            if (inv_count == SMB_MSG_LINES - 1 && tsBuf != NULL)
            {
                ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_BC_INVALID_PAIRS_REPORT, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
                inv_count = 0;
            }
        }
    }

    if (tsBuf != NULL)
    {
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_BC_INVALID_PAIRS_REPORT, tsBuf));

        FREE(tsBuf);
        tsBuf = NULL;
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseCheckStatus */

static int
SseCheckCAStatus (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDevice,
    int            iCount,
    unsigned char ucPairStatus
)
{
    int     retVal = RETURN_OK;
    int i, iCmd;
    pairstatinfo status;
    int     inv_count=0;
    tchar   *tsBuf = NULL;

    ERH_FUNCTION (_T("SseCheckCAStatus"));   
    DbgFcnIn();
 
    for (i = 0; i < iCount; i ++) 
    {
        if (pSseDevice[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSseDevice[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            pSseDevice[i]->iStatus = LDEV_INVALID;

            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Cannot find command device for serial number %d!"),
                __FCN__, pSseDevice[i]->Dev.serial);

            retVal = RETURN_ER;
            goto end;
        }

        if (SseXPairVolStat (pCmdDev[iCmd], &(pSseDevice[i]->Dev), 
                    &status, MD_HORC, 1) != RETURN_OK)
        {
            pSseDevice[i]->iStatus = LDEV_INVALID;

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] SseXPairVolStat failed!"), __FCN__);

            continue;
        }

        if (status.status != ucPairStatus)
        {
            tchar tsBuf2[STRLEN_PATH+1];
            int    iBufLen = 0, iOldLen;
            tchar *newline = _T("\n\t");
            int newline_len = (int) strlen(newline);

            pSseDevice[i]->iStatus = LDEV_INVALID;

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Ldev %u in %d has unexpected status %s(%d)!")
                , __FCN__, pSseDevice[i]->Dev.ldevno, 
                pSseDevice[i]->Dev.serial,
                DbgPrintPairStat (status.status), status.status);

            sprintf (tsBuf2, 
                _T("%-7d %04Xh (%4d)  %5s  %3d  %3d  %s  %-7d %04Xh (%4d)"),
                pSseDevice[i]->Dev.serial,
                pSseDevice[i]->Dev.ldevno,
                pSseDevice[i]->Dev.ldevno,
                portno[pSseDevice[i]->Dev.port],
                pSseDevice[i]->Dev.targid,
                pSseDevice[i]->Dev.lun,
                DbgPrintPairStat (status.status),
                status.pserial,
                status.pldev,
                status.pldev
            );
                
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (inv_count > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (inv_count>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            inv_count++;

            if (inv_count == SMB_MSG_LINES - 1 && tsBuf != NULL)
            {
                ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_CA_INVALID_PAIRS_REPORT, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
                inv_count = 0;
            }
        }
    }

    if (tsBuf != NULL)
    {
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_CA_INVALID_PAIRS_REPORT, tsBuf));

        FREE(tsBuf);
        tsBuf = NULL;
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);
} 
/* SseCheckStatus */


int
SseCheckStatus (
    cmddevinfo **pCmdDev, int iCmdCount,
    PSSE_DEVICE_T  *pSseDevice,
    int            iCount,
    unsigned char ucPairStatus,
    unsigned char  mmode
)
{
    if (mmode == MD_MRCF)
        return SseCheckBCStatus (pCmdDev, iCmdCount, pSseDevice, iCount,
                ucPairStatus);
    else
        return SseCheckCAStatus (pCmdDev, iCmdCount, pSseDevice, iCount, 
                ucPairStatus);
} 
/* SseCheckStatus */


int 
SseRmlibVersion ()
{
    int retVal = RETURN_OK;
    int iVersion;

    ERH_FUNCTION (_T("SseRmlibVersion"));

    DbgFcnIn ();

    SseXRmlibVersion (&iVersion);

    DbgPlain (DBG_SSE_3DAPI, _T("[%s] RMLIB version = %02d.%02d.%02d"), 
        __FCN__, iVersion/10000, iVersion/100%100, iVersion%100);

/*end:*/
    DbgFcnOutRet (retVal);
    return (retVal);
}
void
DbgPrintSseDev (
    PSSE_DEVICE_T pSseDev
)
{
    if (pSseDev == NULL)
        return;
    
    DbgPlain (DBG_SSE_ACTION, _T("\n\tSseDev[%d]"), pSseDev->iIndex);
    DbgPlain (DBG_SSE_ACTION, _T("\tldevno = %u"), pSseDev->Dev.ldevno);
    DbgPlain (DBG_SSE_ACTION, _T("\tserial = %d"), pSseDev->Dev.serial);
    DbgPlain (DBG_SSE_ACTION, _T("\tport = %s (%d)"), 
            pSseDev->Dev.port >= 0 ? portno[pSseDev->Dev.port] : _T("?"),
            pSseDev->Dev.port);
    DbgPlain (DBG_SSE_ACTION, _T("\tmun = %d"), pSseDev->Dev.mun);
    DbgPlain (DBG_SSE_ACTION, _T("\tiStatus = %d"), pSseDev->iStatus);

}

void
DbgPrintTargDev ( targdev *dev )
{
    if (dev == NULL)
        return;
    
    DbgPlain (DBG_SSE_ACTION, _T("\tldevno = %u"), dev->ldevno);
    DbgPlain (DBG_SSE_ACTION, _T("\tserial = %d"), dev->serial);
    DbgPlain (DBG_SSE_ACTION, _T("\tport = %s (%d)"), 
            dev->port >= 0 ? portno[dev->port] : _T("?"), dev->port);
    DbgPlain (DBG_SSE_ACTION, _T("\ttargid = %d"), dev->targid);
    DbgPlain (DBG_SSE_ACTION, _T("\tlun = %d"), dev->lun);
    DbgPlain (DBG_SSE_ACTION, _T("\tmun = %d"), dev->mun);
    DbgPlain (DBG_SSE_ACTION, _T("\tfcalpa = %d\n"), dev->fcalpa);

}
/* DbgPrintSseDev */

#if 0
typedef struct targdev {
    unsigned char    mmode;    /* IN: mirror mode */
    unsigned char    ctgid;    /* IN: CT group ID for only HORC Async*/
    int    port;                /* IN: Following is Port number on the DKC */
						Port number on the DKC	*/


    /* 00:CL1-A 01:CL1-B 02:CL1-C 03:CL1-D */
    /* 04:CL1-E 05:CL1-F 06:CL1-G 07:CL1-H */
    /* 08:CL1-J 09:CL1-K 10:CL1-L 11:CL1-M */
    /* 12:CL1-N 13:CL1-P 14:CL1-Q 15:CL1-R */
    /* 16:CL2-A 17:CL2-B 18:CL2-C 19:CL2-D */
    /* 20:CL2-E 21:CL2-F 22:CL2-G 23:CL2-H */
    /* 24:CL2-J 25:CL2-K 26:CL2-L 27:CL2-M */
    /* 28:CL2-N 29:CL2-P 30:CL2-Q 31:CL2-R */

    /* static char *portno[32] = {
        "CL1-A", "CL1-B", "CL1-C", "CL1-D",
        "CL1-E", "CL1-F", "CL1-G", "CL1-H",
        "CL1-J", "CL1-K", "CL1-L", "CL1-M",
        "CL1-N", "CL1-P", "CL1-Q", "CL1-R",
        "CL2-A", "CL2-B", "CL2-C", "CL2-D",
        "CL2-E", "CL2-F", "CL2-G", "CL2-H",
        "CL2-J", "CL2-K", "CL2-L", "CL2-M",
        "CL2-N", "CL2-P", "CL2-Q", "CL2-R"
    }; */

    int    targid;     /* IN: targetID in the port */
    int    lun;        /* IN: lun in the targetID */
    int    mun;        /* IN: MU# for HOMRCF */
    int    serial;     /* IN/OUT:  serial number of DKC */
                /* IN: Case of NOT *Unknown_LDEV* for pair*() */
    unsigned short ssid;    /* IN/OUT:  ssid of DKC*/
                    /* IN: Case of NOT *Unknown_LDEV* for pair*() */
    unsigned short ldevno; /* IN/OUT:  LDEV number of LU */
                /* IN: Must be spcified *Unknown_LDEV* or LDEV# for pair*() */
    unsigned char fcalpa; /* OUT:  Fibre AL_PA/SCSI for identify of port*/
							identify of port*/
    unsigned short numldev;	/* OUT: Number of LDEV for LU */
    unsigned short ldev[128]; /* OUT: Mapped LDEV# for LU */
} targdev;

#endif

void
DbgPrintSseDevAll (
    PSSE_DEVICE_T *pSseDevList,
    int            iSseDevCount)
{
    int i;

    if (pSseDevList == NULL || iSseDevCount <= 0)
        return;

    DbgPlain(DBG_SSE_ACTION, _T("Sse devices [%d]"), iSseDevCount);

    for (i=0; i<iSseDevCount; i++)
        if (pSseDevList[i] != NULL)
            DbgPrintSseDev (pSseDevList[i]);
}
/* DbgPrintSseDevAll */


int
SseReportRdskToLdevMap (
    int iDevCount,
    PSMB_RAW_DISK_T *pRdsk,
    PSSE_DEVICE_T   *pSseDevices
)
{
    int retVal = RETURN_OK, i=0, j;
    int iDevOkCount = 0;
    int p;

    ERH_FUNCTION (_T("SseReportRdskToLdevMap"));

    DbgFcnInEx(__FLND__, _T("iDevCount = %d"), iDevCount);

    /* Check parameters */
    if (iDevCount > 0 && (pRdsk == NULL || pSseDevices == NULL))
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_SMB_INVPARAM, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s) iDevCount = %d, pRdsk = %p, pSseDevices = %p"),
            __FCN__, iDevCount, pRdsk, pSseDevices);
        retVal = RETURN_ER;
    }
    else 
    {
        /* Entering */     
        tchar *tsBuf = NULL;
        int    iBufLen = 0, iOldLen;
        tchar *newline = _T("\n\t");
        int newline_len = (int) strlen(newline);
        
        for (i=0; i<iDevCount; i++)
            if (pRdsk[i]->status != RDSK_OK && !pRdsk[i]->iErrReported)
            {
                /*
                ErhMark (pRdsk[i]->status, 
                    __FCN__, ERH_MAJOR);
                ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_MAP_RDSK_2_LDEV_OBJ_FAILED,
                    pRdsk[i]->tsRdskName,
                    ErhErrnoToText ()));
                ErhClearError(); 
                */
                pRdsk[i]->iErrReported = 1;
            }
        
        /* count pRdsk with ok status */
        for (i=0; i<iDevCount; i++)
            if (pRdsk[i]->status == RDSK_OK) 
                iDevOkCount++;

        i=0;
        /* Report what's found */
        p=0;
        for (j=0; j<iDevCount; j++)
        {
            tchar tsBuf2[STRLEN_PATH+1];

            if (pRdsk[j]->status != RDSK_OK)
                continue;
#if TARGET_WIN32

            snprintf (tsBuf2, STRLEN_PATH, _T("%-20s %3d %3d %-9d %s %04Xh (%5d)"), 
                pRdsk[j]->tsRdskName, pSseDevices[j]->Dev.targid, pSseDevices[j]->Dev.lun,
                pSseDevices[j]->Dev.serial, 
                portno[pSseDevices[j]->Dev.port], 
                pSseDevices[j]->Dev.ldevno, pSseDevices[j]->Dev.ldevno);
#else
            snprintf (tsBuf2, STRLEN_PATH, _T("%-28s %-9d %s %04Xh (%5d)"), 
                pRdsk[j]->tsRdskName,
                pSseDevices[j]->Dev.serial, 
                portno[pSseDevices[j]->Dev.port], 
                pSseDevices[j]->Dev.ldevno, pSseDevices[j]->Dev.ldevno);
#endif
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] [%d] tsBuf2=%s"), __FCN__, i, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (i > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (i>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            i++;
            p++;

            if ((i == SMB_MSG_LINES - 1 || p == iDevOkCount) &&
                *tsBuf != _T('\0'))
            {
                ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_RDSK_TO_LDEV_MAP_REPORT, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
                i = 0;
            }
        }
    
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseReportRdskToLdevMap */

int
SseReportLdevToRdskMap (
    int iDevCount,
    PSMB_RAW_DISK_T *pRdsk,
    PSSE_DEVICE_T   *pSseDevices
)
{
    int retVal = RETURN_OK, i=0, j;
    int iDevOkCount = 0;
    int p;

    ERH_FUNCTION (_T("SseReportLdevToRdskMap"));

    DbgFcnInEx(__FLND__, _T("iDevCount = %d"), iDevCount);

    /* Check parameters */
    if (iDevCount > 0 && (pRdsk == NULL || pSseDevices == NULL))
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_SMB_INVPARAM, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s) iDevCount = %d, pRdsk = %p, pSseDevices = %p"),
            __FCN__, iDevCount, pRdsk, pSseDevices);
        retVal = RETURN_ER;
    }
    else 
    {
        /* Entering */     
        tchar *tsBuf = NULL;
        int    iBufLen = 0, iOldLen;
        tchar *newline = _T("\n\t");
        int newline_len = (int) strlen(newline);

        
        for (i=0; i<iDevCount; i++)
            if (pRdsk[i]->status != RDSK_OK && !pRdsk[i]->iErrReported)
            {
                tchar tsBuf[STRLEN_PATH+1];

                sprintf (tsBuf, _T("%0Xh (%5d)"), pSseDevices[i]->Dev.ldevno,
                    pSseDevices[i]->Dev.ldevno);
                       
                ErhMark (pRdsk[i]->status, 
                    __FCN__, ERH_MAJOR);
                ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_MAP_LDEV_2_RDSK_OBJ_FAILED,
                    tsBuf, pSseDevices[i]->Dev.serial/*, ErhErrnoToText ()*/));
                ErhClearError();
                               
                pRdsk[i]->iErrReported = 1;
            }


        /* count pRdsk with ok status */
        for (i=0; i<iDevCount; i++)
            if (pRdsk[i]->status == RDSK_OK)
                iDevOkCount++;

        i=0;
        /* Report what's found */
        p=0;
        for (j=0; j<iDevCount; j++)
        {
            tchar tsBuf2[STRLEN_PATH+1];

            if (pRdsk[j]->status != RDSK_OK)
                continue;

            snprintf (tsBuf2, STRLEN_PATH, _T("%d %9s  %04Xh (%5d)    %3d %3d %-20s"),
                pSseDevices[j]->Dev.serial, 
                portno[pSseDevices[j]->Dev.port], 
                pSseDevices[j]->Dev.ldevno, pSseDevices[j]->Dev.ldevno,
                pSseDevices[j]->Dev.targid, pSseDevices[j]->Dev.lun, pRdsk[j]->tsRdskName);

            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (i > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (i>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            i++;
            p++;

            if ((i == SMB_MSG_LINES - 1 || p == iDevOkCount) && 
                *tsBuf != _T('\0'))
            {
                ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_LDEV_TO_RDSK_MAP_REPORT, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
                i = 0;
            }
        }
        
    }
end:
        
    DbgFcnOutRet (retVal);
    return (retVal);
} /* SseReportLdevToRdskMap */

int
SseReportPairs (
    cmddevinfo      **pCmdDev, 
    int             iCmdCount,
    PSSE_DEVICE_T   *pSrcDevices,
    int             iDevCount,
    int             mmode,
    SSE_PAIR_REPORTING_T reporting

)
{
    if (mmode == MD_MRCF)
         return SseReportBCPairs (pCmdDev, iCmdCount, pSrcDevices, iDevCount, reporting);
    else
        return SseReportCAPairs (pCmdDev, iCmdCount, pSrcDevices, iDevCount, reporting);
}

int
SseReportBCPairs (
    cmddevinfo      **pCmdDev, 
    int             iCmdCount,
    PSSE_DEVICE_T   *pSrcDevices,
    int             iDevCount,
    SSE_PAIR_REPORTING_T reporting
)
{
    int retVal = RETURN_OK, i;
    int iDevOkCount=0;
    int j;
    int dev_status_OK; 

    ERH_FUNCTION (_T("SseReportBCPairs"));

    DbgFcnIn();

    /* Check parameters */
    if (iDevCount > 0 && pSrcDevices == NULL)
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s)!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    {
        /* Entering */
        tchar *tsBuf = NULL;
        int    iBufLen = 0, iOldLen;
        tchar *newline = _T("\n\t");
        int newline_len = (int) strlen(newline);
        int iCmd;

        tsBuf = (tchar *) MALLOC (STRLEN_PATH * sizeof(tchar));
        tsBuf[0] = _T('\0');
        
        /* count pSrcDevices with status ok */
        for (i=0; i<iDevCount; i++)
            if (pSrcDevices[i]->iStatus == LDEV_OK)
                iDevOkCount++;
          
        /* Report what's found */
        dev_status_OK=-1; 
        j=0;
        for (i=0; i<iDevCount; i++)
        {
            pairstatinfo  status;
            tchar tsBuf2[STRLEN_PATH+1];

            if (pSrcDevices[i]->iStatus != LDEV_OK)
                continue;
            dev_status_OK++;

            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pSrcDevices[i]->Dev.serial)
                    break;

            if (iCmd == iCmdCount)
            {
                /* Critical error */
                ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Cannot find command device for serial number %d!"),
                    __FCN__, pSrcDevices[i]->Dev.serial);

                retVal = RETURN_ER;
                goto end;
            }

            if (SseXPairVolStat (pCmdDev[iCmd], &(pSrcDevices[i]->Dev), 
                        &status, MD_MRCF, 1) != RETURN_OK)
            {
                pSrcDevices[i]->iStatus = LDEV_INVALID;

                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] SseXPairVolStat failed!"), __FCN__);

                continue;
            }
            pSrcDevices[i]->PairStatus = status.status;

            sprintf (tsBuf2, 
                _T("%-7d %04Xh (%4d)  %5s  %3d  %3d  %3d  %s  %-7d %04Xh (%4d)"),
                pSrcDevices[i]->Dev.serial,
                pSrcDevices[i]->Dev.ldevno,
                pSrcDevices[i]->Dev.ldevno,
                portno[pSrcDevices[i]->Dev.port],
                pSrcDevices[i]->Dev.targid,
                pSrcDevices[i]->Dev.lun,
                pSrcDevices[i]->Dev.mun,
                DbgPrintPairStat (status.status),
                status.pserial,
                status.pldev,
                status.pldev
            );
            
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (dev_status_OK%SMB_MSG_LINES > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (dev_status_OK%SMB_MSG_LINES>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            j++;
            
            if ((dev_status_OK%SMB_MSG_LINES == SMB_MSG_LINES - 1 || j == iDevOkCount) &&
                *tsBuf != _T('\0'))
            {
                if (reporting == SSE_REPORT_CURRENT_SESS)
                {
                    /* At beggining we report to GUI,  which BC pairs 
                     * will be used in current session
                     * */
                    ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_BC_LDEV_TO_LDEV_MAP_REPORT, tsBuf));
                }
                else
                {
                    /* At end we report to GUI,  which BC pairs 
                     * will be used in next(!) session
                     * */
                    ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_BC_LDEV_TO_LDEV_NEXT_MAP_REPORT, tsBuf));
                }

                dev_status_OK=-1;
                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
            }
        }
        
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseReportBCPairs */

int
SseReportCAPairs (
    cmddevinfo      **pCmdDev, 
    int             iCmdCount,
    PSSE_DEVICE_T   *pSrcDevices,
    int             iDevCount,
    SSE_PAIR_REPORTING_T reporting
)
{
    int retVal = RETURN_OK, i;
    int iDevOkCount=0;
    int j; 

    ERH_FUNCTION (_T("SseReportCAPairs"));

    DbgFcnIn();

    /* Check parameters */
    if (iDevCount > 0 && pSrcDevices == NULL)
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s)!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    {
        /* Entering */
        tchar *tsBuf = NULL;
        int    iBufLen = 0, iOldLen;
        tchar *newline = _T("\n\t");
        int newline_len = (int) strlen(newline);
        int iCmd;

        tsBuf = (tchar *) MALLOC (STRLEN_PATH * sizeof(tchar));
        
        /* count pSrcDevices with status ok */
        for (i=0; i<iDevCount; i++)
            if (pSrcDevices[i]->iStatus == LDEV_OK)
                iDevOkCount++;
                 

        /* Report what's found */
        j=0;
        for (i=0; i<iDevCount; i++)
        {
            pairstatinfo  status;
            tchar tsBuf2[STRLEN_PATH+1];

            if (pSrcDevices[i]->iStatus != LDEV_OK)
                continue;

            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pSrcDevices[i]->Dev.serial)
                    break;

            if (iCmd == iCmdCount)
            {
                /* Critical error */
                ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Cannot find command device for serial number %d!"),
                    __FCN__, pSrcDevices[i]->Dev.serial);

                retVal = RETURN_ER;
                goto end;
            }

            if (SseXPairVolStat (pCmdDev[iCmd], &(pSrcDevices[i]->Dev), 
                        &status, MD_HORC, 1) != RETURN_OK)
            {
                pSrcDevices[i]->iStatus = LDEV_INVALID;

                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] SseXPairVolStat failed!"), __FCN__);

                continue;
            }
            pSrcDevices[i]->PairStatus = status.status;

            sprintf (tsBuf2, 
                _T("%-7d %04Xh (%4d)  %5s  %3d  %3d  %s  %-7d %04Xh (%4d)"),
                pSrcDevices[i]->Dev.serial,
                pSrcDevices[i]->Dev.ldevno,
                pSrcDevices[i]->Dev.ldevno,
                portno[pSrcDevices[i]->Dev.port],
                pSrcDevices[i]->Dev.targid,
                pSrcDevices[i]->Dev.lun,
                DbgPrintPairStat (status.status),
                status.pserial,
                status.pldev,
                status.pldev
            );
            
            iOldLen = iBufLen;
            iBufLen += ((int) strlen(tsBuf2)) + (i%SMB_MSG_LINES > 0 ? newline_len : 0);
            if ((tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * 
                sizeof (tchar))) == NULL)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] REALLOC failed"), __FCN__);

                retVal = RETURN_ER;
                goto end;
            }
            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            if (i%SMB_MSG_LINES>0)
                strcat(tsBuf, newline);

            strcat (tsBuf, tsBuf2);

            j++;
            
            if ((i%SMB_MSG_LINES == SMB_MSG_LINES - 1 || j == iDevOkCount) &&
                *tsBuf != _T('\0'))
            {
                if (reporting == SSE_REPORT_CURRENT_SESS)
                {
                    /* At beggining we report to GUI,  which BC pairs 
                     * will be used in current session
                     * */
                    ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_CA_LDEV_TO_LDEV_MAP_REPORT, tsBuf));
                }
                else
                {
                    /* At end we report to GUI,  which BC pairs 
                     * will be used in next(!) session
                     * */
                    ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_CA_LDEV_TO_LDEV_NEXT_MAP_REPORT, tsBuf));
                }


                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = iOldLen = 0;
            }
        }
        
    }

end:
    DbgFcnOutRet (retVal);
    return (retVal);

} /* SseReportCAPairs */

int
SseUpdateStatus (
    PSSE_DEVICE_T   *pSseDev,
    PSMB_DB_T       pSmbDb,
    int             iCount
)
{
    int i;
    int retVal = RETURN_OK;
    

    ERH_FUNCTION (_T("SseUpdateStatus"));
    DbgFcnIn();

    if (pSseDev == NULL || pSmbDb == NULL)
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Invalid parameter(s)!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    for (i=0; i<iCount; i++)
        if (pSseDev[i] != NULL && pSseDev[i]->iStatus != LDEV_OK)
            if (pSmbDb->pSysObjects->pRdsk[i] != NULL)
            {
                /*
                  DbgStamp (DBG_UNEXPECTED);
                 DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] pRdsk[i]->status = RDSK_ER!"), __FCN__); */

                pSmbDb->pSysObjects->pRdsk[i]->status = RDSK_ER;
            }

    if ((retVal = SmbUpdateStatus(pSmbDb)) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
                  _T("[%s] SmbUpdateStatus failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

end:
    DbgFcnOutRet(retVal);
    return (retVal);
    
}

/*========================================================================*//**
*
* @param     PSSE_DEVICE_T   *pSseDev - (in) array devices
*            PSMB_DB_T       pSmbDb   - (in) split mirror backup structure
*            int             iCount   - (in) number of devices
*
* @retval    RETURN_OK  Success
* @retval    RETURN_ER  Error
*
* @brief     Updates the statuses of given structure. This is the IR equivalent of
*            SseUpdateStatus.
*
*//*=========================================================================*/
int
SseUpdateStatusIR (PSSE_DEVICE_T   *pSseDev,
                   PSMB_DB_T       pSmbDb,
                   int             iCount)
{
    int i = 0;
    int retVal = RETURN_OK;
    

    ERH_FUNCTION (_T("SseUpdateStatusIR"));
    DbgFcnIn();

    if (pSseDev == NULL || pSmbDb == NULL)
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameter(s)!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    for (i = 0; i < iCount; i++)
    {
        if (pSseDev[i] != NULL && pSseDev[i]->iStatus != LDEV_OK)
        {
            if (pSmbDb->pSysObjects->pRdsk[i] != NULL)
            {
                pSmbDb->pSysObjects->pRdsk[i]->status = RDSK_ER;
            }
        }
    }

    retVal = SmbUpdateStatusIR(pSmbDb);

    if (retVal != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] SmbUpdateStatusIR failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

end:
    DbgFcnOutRet(retVal);
    return (retVal);
}

extern int GetLdevSessionIds  
(
    PSSE_DEVICE_T   *pSseDev,
    int             iCount,
    tchar           ***sessIdList, 
    int             *sessIdCntr
)
{
    int            retVal = RETURN_OK;
    int            i      = 0;
    xpDev_t        *devList[2];
   
    ERH_FUNCTION (_T("GetLdevSessionIds"));   
    DbgFcnIn();

    DbgStamp(DBG_SSE_3DAPI);
   
    devList[0] = (xpDev_t *) MALLOC (sizeof (xpDev_t));
    
    *sessIdList = NULL;
    *sessIdCntr  = 0;
    
    for (i = 0; i < iCount; i ++) 
    {
        if (pSseDev[i]->iStatus == LDEV_OK)
        {
            /* Search for SessID info for current LDEV */
            memset (devList[0], 0, sizeof (*(devList[0])));
            devList[0]->ldev = pSseDev[i]->Dev.ldevno;
            devList[0]->seq  = pSseDev[i]->Dev.serial;
            devList[1] = NULL;

            DbgPlain (DBG_SSE_MSG, 
                      _T("[%s] ldev = %d"),
                      __FCN__, devList[0]->ldev);

            if (dbXP_getDev (devList) != RETURN_OK)
            {
                /* If LDEV (next multimirror) is not in DBXP, who cares ... */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDev failed!"), __FCN__);
             }
             else
             {
                 DbgPlain (DBG_SSE_MSG, 
                           _T("[%s] devList[0]->sessId = %s"),
                           __FCN__, 
                           NS(devList[0]->sessId));

                 if (StrCmp(devList[0]->sessId, _T("-1")))
                 {
                     (*sessIdCntr)++;
     
                     DbgPlain (DBG_SSE_MSG, 
                              _T("[%s] sessIdCntr = %d"),
                              __FCN__, 
                              *sessIdCntr);
  
                     /* Store 'devList[0]->sessId' to sessID list */
                     *sessIdList = (tchar**) REALLOC (*sessIdList, 
                                            (*sessIdCntr) * sizeof(tchar *) );
                 
                    (*sessIdList)[(*sessIdCntr)-1] = StrNewCopy(devList[0]->sessId);
                 
                    DbgPlain (DBG_SSE_MSG, 
                              _T("[%s] Added sessID[%d]: %s, to sessIdList"),
                             __FCN__, 
                             *sessIdCntr, 
                             (*sessIdList)[(*sessIdCntr)-1]);
                 }
             }
        } /* If LDEV = OK */

    } /* For */

    FREE(devList[0]);

    DbgFcnOutRet (retVal);
    return retVal;

} /* GetLdevSessionIds */


/*========================================================================*//**
*
* @param     sessId -    (in) The session id of which the file system
*                        structures should be dismounted
*
* @retval    RETURN_OK  Success
* @retval    RETURN_ER  Error
*
* @brief     Dismounts the disks of a specific session.
*            This can happen on the backup host (at the end of session,
*            on rotation, or prior to IR).
*
*
*//*=========================================================================*/

/*
    QXCR1000739669: this function is caled at the BH only, therefore the appHost parameter was omitted and replaced by FALSE where applicable
    The function was renamed into more appropriate name SSEA_DisableBs as well.
*/
int SSEA_DisableBs ( tchar *sessId )
{
    int               retVal = RETURN_ER;
    SMB_DB_T          smbDb={0};
    SMB_STATE_T       origState = SMB_STATE_INIT;
    int               i = 0;
    
    ERH_FUNCTION (_T("SSEA_DisableBs"));
    DbgFcnInEx (__FLND__, _T("sessId = %s"), NSD(sessId));

    origState = inf.state;
    SSEA_VERIFY ( NULL != sessId );

    smbDb.pSysObjects = (PSMB_SYS_OBJECTS_T) CALLOC (1, sizeof (SMB_SYS_OBJECTS_T));
    SSEA_VERIFY ( NULL != smbDb.pSysObjects );

    if ( LIBSMB_OK != SmbDbSys_Query(sessId, FALSE, &smbDb) )
    {
        /* Could happen if we had failed to update the internal database */
        retVal = RETURN_OK;
        goto end;
    }

    /* 
        QXCR1000739669: instead of just dismounting filesystems the BS must be completely disabled (e.g. volume groups must be deactivated and 
        exported at HP-UX). Instead of "reinventing hot water"  we'll use already existing function SmbDisableBs.
    However this function requires inf.state to be SMB_STATE_RESOLVE_FS and sets it to SMB_STATE_DISABLE_BS. This would break the further session flow
    therefore we must remember the current inf.state and set it back after SmbDisableBs is done.
    */

    inf.state =  SMB_STATE_RESOLVE_BS;
    if (LIBSMB_OK != SmbDisableBsEx(&smbDb, 0))
    {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] SmbDisableBsEx failed! Session : %s could not be removed from the DbSys"), __FCN__, sessId);
            goto end;
    }
    SmbUpdateStatus(&smbDb);

    if (RETURN_OK != smbDb.iStatus)
    {
        goto end;
    }

    /* QXCR1000739616: check if all required filesystems were dismounted */
    for (i = 0; i<smbDb.pSysObjects->iFsCount; i++)
    {
        DbgPrintFs(smbDb.pSysObjects->pFs[i]);
        if (smbDb.pSysObjects->pFs[i]->iStatus == SMB_OS_ERROR)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, 
                     _T("[%s] All file systems are not dismounted.\n")
                     _T("Session : %s will not be removed from DbSys"), 
                     __FCN__,sessId);
            goto end;
        }
    }
    /* QXCR1000739616: if no goto end; was called, change retVal from the default RETURN_ER to RETURN_OK */
    retVal = RETURN_OK;

end:
    inf.state=origState;
    if (Free_PSMB_SYS_OBJECTS_T(smbDb.pSysObjects) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Free_PSMB_SYS_OBJECTS_T failed!"), __FCN__);
    }
    
    FREE(smbDb.pSysObjects);

    DbgFcnOutRet(retVal);
    return (retVal);
    
} /* SSEA_DisableBs */


int
SSEA_CheckDbSys (
    PSSE_DEVICE_T *pDev,
    int           iCount,
    BOOL          appHost
)
{
    int     retVal = RETURN_OK;
    int     i = 0;
    xpDev_t *devList[2];
    
    ERH_FUNCTION (_T("SSEA_CheckDbSys"));
    DbgFcnIn();

    /* Check input parameter */
    if (*pDev == NULL)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    devList[0] = (xpDev_t *) MALLOC (sizeof (xpDev_t));

    for (i = 0; i < iCount; i ++) 
    {
        if (pDev[i]->iStatus == LDEV_OK)
        {
            /* Search for SessID info for current LDEV */
            memset (devList[0], 0, sizeof (*(devList[0])));
            devList[0]->ldev = pDev[i]->Dev.ldevno;
            devList[0]->seq  = pDev[i]->Dev.serial;
            devList[1] = NULL;

            DbgPlain (DBG_SSE_MSG, 
                      _T("[%s] ldev = %d"),
                      __FCN__, devList[0]->ldev);

            if (dbXP_getDev (devList) != RETURN_OK)
            {
                /* If LDEV (next multimirror) is not in DBXP, who cares ... */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                          _T("[%s] dbXP_getDev failed!"), __FCN__);
             }
             else
             {
                 SMB_DB_T smbDb={0};

                 /* if sessId still exists in dbSys, status of 
                    dev will be set on LDEV_INVALID */

                 smbDb.pSysObjects = (PSMB_SYS_OBJECTS_T) MALLOC 
                                        (sizeof (SMB_SYS_OBJECTS_T));
                 
                 memset (smbDb.pSysObjects, 0, sizeof (SMB_SYS_OBJECTS_T));

                 if (SmbDbSys_Query(devList[0]->sessId, 
                                    appHost, 
                                    &smbDb) == LIBSMB_OK)
                 {
                     DbgStamp (DBG_SSE_MSG);
                     DbgPlain (DBG_SSE_MSG, 
                               _T("[%s] iFsCount = %d!"), 
                               __FCN__, 
                               smbDb.pSysObjects->iFsCount);

                     if (smbDb.pSysObjects->iFsCount != 0)
                     {
                         DbgStamp (DBG_UNEXPECTED);
                         DbgPlain (DBG_UNEXPECTED, 
                           _T("[%s] Session is not removed from dbSys!"), 
                           __FCN__);
                         pDev[i]->iStatus = LDEV_INVALID;
                     }
                 }

                 
                 if (Free_PSMB_SYS_OBJECTS_T(smbDb.pSysObjects) != RETURN_OK)
                 {
                     DbgStamp (DBG_UNEXPECTED);
                     DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] Free_PSMB_SYS_OBJECTS_T failed!"), __FCN__);
                 }

                 FREE(smbDb.pSysObjects);
             }
        } /* If LDEV = OK */
    
    } /* end of for */

    FREE(devList[0]);

end:
    DbgFcnOutRet(retVal);
    return (retVal);
    
} /* SSEA_CheckDbSys */

int ResolveAtomicSplit (
    cmddevinfo      **pCmdDev, 
    int             iCmdCount,
    PSSE_DEVICE_T   *pSrcDevice,
    int             iDevCount)
{
    int     retVal = RETURN_OK;
    int     i;
    int     j;
    int     iCT;
    int     iDev;
    int     iCmd;
    int     CTGIDCount = 0;
    int     *CTGIDs = NULL;
    int     *CTSerials = NULL;
    int     *CTLDEVs[3];
    int     iLDEVOutsideCTGroup = 0;
    int     iMessageLines = 0;
    tchar   *tsBuf = NULL;

    ERH_FUNCTION (_T("ResolveAtomicSplit"));
    DbgFcnIn();

    CTLDEVs[0] = NULL;
    CTLDEVs[1] = NULL;
    CTLDEVs[2] = NULL;

    /* Check the CTGID of the pairs.  Currently ctgid may be 1-xxx (xxx being the upper limit of 
     * valid CT groups on that platform).  This does not include 0, as this gives false positives 
     * for being  inside a CT group (call ID submitted .
     * The algorithm is:
     *   -query each pairvolstatus, for all mirrors, to generate a list of CTGIDs and the LDEVs 
     *       attached to each CTGID
     *   -query the CTGID to ensure that all of the LDEVs found are in that CTGID.  If there are 
     *       more LDEVs, then the backup objects do not include all of the expected disks

     * Using a lot of MALLOC (relatively).  Would rather use slightly more memory than bother 
     * with keeping counters, as is common usage in SSEA, and with REALLOC. */
    CTLDEVs[0] = (int*) MALLOC (sizeof (int) * iDevCount);
    CTLDEVs[1] = (int*) MALLOC (sizeof (int) * iDevCount);
    CTLDEVs[2] = (int*) MALLOC (sizeof (int) * iDevCount);

    /* The -1 is a placeholder.  When a CT group and a SVOL's LDEV is found, this is replaced. */
    memset(CTLDEVs[0], -1, (iDevCount * sizeof (int)));
    memset(CTLDEVs[1], -1, (iDevCount * sizeof (int)));
    memset(CTLDEVs[2], -1, (iDevCount * sizeof (int)));

    DbgStamp(DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, 
        _T("[%s] Beginning the resolve of CT groups."), __FCN__);

    for (i = 0; i < iDevCount; i ++) 
    {
        unsigned char mun;
        pairstatinfo statp;

        if (pSrcDevice[i]->iStatus != LDEV_OK)
            continue;

        for (iCmd = 0; iCmd < iCmdCount; iCmd++)
            if (pCmdDev[iCmd]->serial == pSrcDevice[i]->Dev.serial)
                break;

        if (iCmd == iCmdCount)
        {
            DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] ATOMIC SPLIT Cannot find command device for serial number %d!"),
                        __FCN__, pSrcDevice[i]->Dev.serial);
            continue;
        }

        DbgStamp(DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI, 
            _T("[%s] Looking at LDEV#%d"), __FCN__, i);

        /* Check each mu. */
        for (mun = 0; mun < 3; mun++)
        {
            targdev targDevice;
            char sBuf[1024];

            memset (&statp, 0, sizeof (pairstatinfo));
            memset (&targDevice, 0, sizeof (targdev));

            targDevice.targid = pSrcDevice[i]->Dev.targid;
            targDevice.lun    = pSrcDevice[i]->Dev.lun;
            targDevice.port   = pSrcDevice[i]->Dev.port;
            targDevice.ldevno = pSrcDevice[i]->Dev.ldevno;
            targDevice.mun    = mun;
            targDevice.mmode  = MD_TYPE(opt.conf);

            DbgStamp(DBG_SSE_3DAPI);
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] Checking for the CT group and a pair relationship on mu='%d'"), __FCN__, mun);
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] targDevice.targid = %d"), __FCN__, targDevice.targid );
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] targDevice.lun = %d"), __FCN__, targDevice.lun );
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] targDevice.port = %d"), __FCN__, targDevice.port );
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] targDevice.mun = %d"), __FCN__, targDevice.mun );
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] targDevice.ldevno = %d"), __FCN__, targDevice.ldevno );

            if (pairvolstat (pCmdDev[iCmd], &targDevice, &statp) == -1)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] pairvolstat failed!\n%s"), __FCN__,
                    A2T(getrmerrmsg(pCmdDev[iCmd])));
                getfmtmsg (pCmdDev[iCmd], sBuf);
                DbgPlain (DBG_UNEXPECTED, _T("%s"), A2T(sBuf));
                MarkFailedCTGroup (-1, 0, iDevCount, pSrcDevice);
                retVal = RETURN_ER;
                goto end;
            }

            if (statp.ctgid == 0)
            {
                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] A ctgid = 0 was found.  No CT group or usable CT group is found."), __FCN__);
                break;
            }
            else if (statp.status == STAT_SMPL)
            {
                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] No BC pair for MU '%d'."), __FCN__, mun);
            }
            else
            {
                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] Found a BC pair relationship for MU '%d'."), __FCN__, mun);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] CT group ID is '%d'"), __FCN__, statp.ctgid);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] SVOL ldev is '%d'"), __FCN__, statp.pldev);
                DbgPlain (DBG_SSE_3DAPI, 
                    _T("[%s] BC pair is %s(%d)"), __FCN__, DbgPrintPairStat(statp.status), statp.status);

                /* (Re)set the source device's CTGID. */
                pSrcDevice[i]->ctgid = statp.ctgid;

                /* Set the ldev number for this MU. */
                CTLDEVs[mun][i] = statp.pldev;
            }
        }
    }

    DbgStamp (DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, _T("[%s]Discovered the following CTGIDs and MUs."), __FCN__);
    DbgPlain (DBG_SSE_3DAPI, _T("[%s]\tArray\tPVOL\tCTGID\tMU 0\tMU 1\tMU 2"), __FCN__);
    for (i = 0; i < iDevCount; i ++) 
    {
        if (pSrcDevice[i]->iStatus != LDEV_OK)
        {
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s]\t%d\t%d/%xh is LDEV_INVALID"), __FCN__, 
                pSrcDevice[i]->Dev.serial,
                pSrcDevice[i]->Dev.ldevno, pSrcDevice[i]->Dev.ldevno);
        }
        else if (pSrcDevice[i]->ctgid == 0)
        {
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s]\t%d/%xh\tno valid ctgid"), __FCN__,
                pSrcDevice[i]->Dev.ldevno, pSrcDevice[i]->Dev.ldevno);
        }
        else
        {
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s]\t%d\t%d/%xh\t%d/%xh\t%d/%xh\t%d/%xh\t%d/%xh"), __FCN__,
                pSrcDevice[i]->Dev.serial,
                pSrcDevice[i]->Dev.ldevno, pSrcDevice[i]->Dev.ldevno,
                pSrcDevice[i]->ctgid, pSrcDevice[i]->ctgid,
                CTLDEVs[0][i], CTLDEVs[0][i],
                CTLDEVs[1][i], CTLDEVs[1][i],
                CTLDEVs[2][i], CTLDEVs[2][i]);
        }
    }

    /* Extract different CTGIDs. */
    GetCTGIDs (iDevCount, pSrcDevice, &CTGIDCount, &CTGIDs, &CTSerials);

    /* Display CTGID resolved groups.
     * Message content creation is cannibalized from existing message creations.  What ugly
     * message loops those are.  Were they working around an (old) max size of session messages? */
    for (iCT = 0; iCT < CTGIDCount; iCT++)
    {
        int     iBufLen = 0;
        int     iOldLen = 0;
        
        iMessageLines = 0;

        tsBuf = NULL;
        tsBuf = (tchar *) MALLOC (STRLEN_PATH * sizeof(tchar));
        tsBuf[0] = _T('\0');
        
        DbgStamp (DBG_SSE_3DAPI)
        DbgPlain (DBG_SSE_3DAPI, 
            _T("[%s] Building message string for CTGID = '%d'."), __FCN__, CTGIDs[iCT]);

        for (iDev = 0; iDev < iDevCount; iDev++)
        {
            tchar tsBuf2[STRLEN_PATH+1];

            if (pSrcDevice[iDev]->iStatus != LDEV_OK)
                continue;

            if ((pSrcDevice[iDev]->ctgid != CTGIDs[iCT]) || (pSrcDevice[iDev]->Dev.serial != CTSerials[iCT]))
                continue;

            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] Looking at device '%d'."), __FCN__, iDev);

            sprintf (tsBuf2, 
                _T("\t%-7d %04Xh (%4d)  %5s  %3d  %3d  %3d  %04Xh (%4d)\n"),
                pSrcDevice[iDev]->Dev.serial,
                pSrcDevice[iDev]->Dev.ldevno,
                pSrcDevice[iDev]->Dev.ldevno,
                portno[pSrcDevice[iDev]->Dev.port],
                pSrcDevice[iDev]->Dev.targid,
                pSrcDevice[iDev]->Dev.lun,
                pSrcDevice[iDev]->Dev.mun,
                pSrcDevice[iDev]->ctgid,
                pSrcDevice[iDev]->ctgid
            );
            
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += strlen(tsBuf2);

            tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * sizeof (tchar));

            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            strcat (tsBuf, tsBuf2);
            iMessageLines++;

            if (iMessageLines == SMB_MSG_LINES - 1)
            {
                ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_DISPLAY_CT_GROUP, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = 0;
                iOldLen = 0;
                iMessageLines = 0;
            }
        }

        if (iMessageLines != 0)
        {
            ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_DISPLAY_CT_GROUP, tsBuf));

            FREE(tsBuf);
            tsBuf = NULL;
        }
    }

    /* Display non-CT group disks inside the message. */
    for (iDev = 0; iDev < iDevCount; iDev++)
    {        
        if (pSrcDevice[iDev]->iStatus != LDEV_OK)
            continue;

        if (pSrcDevice[iDev]->ctgid == 0)
        {
            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] Device '%d' does not have a valid CT group."), __FCN__, iDev);

            iLDEVOutsideCTGroup = 1;
            break;
        }
    }

    if (iLDEVOutsideCTGroup == 1)
    {
        int     iBufLen = 0;
        int     iOldLen = 0;
        tchar   tsBuf2[STRLEN_PATH+1];

        iMessageLines = 0;

        tsBuf = NULL;
        tsBuf = (tchar *) MALLOC (STRLEN_PATH * sizeof(tchar));
        tsBuf[0] = _T('\0');


        for (iDev = 0; iDev < iDevCount; iDev++)
        {        
            if (pSrcDevice[iDev]->iStatus != LDEV_OK)
                continue;

            if (pSrcDevice[iDev]->ctgid != 0)
                continue;

            DbgPlain (DBG_SSE_3DAPI, 
                _T("[%s] Looking at device '%d'."), __FCN__, iDev);

            sprintf (tsBuf2, 
                _T("\t%-7d %04Xh (%4d)  %5s  %3d  %3d  %3d\n"),
                pSrcDevice[iDev]->Dev.serial,
                pSrcDevice[iDev]->Dev.ldevno,
                pSrcDevice[iDev]->Dev.ldevno,
                portno[pSrcDevice[iDev]->Dev.port],
                pSrcDevice[iDev]->Dev.targid,
                pSrcDevice[iDev]->Dev.lun,
                pSrcDevice[iDev]->Dev.mun
            );
            
            DbgPlain (DBG_SSE_ACTION,
                _T("[%s] tsBuf2=%s"), __FCN__, tsBuf2);

            iOldLen = iBufLen;
            iBufLen += strlen(tsBuf2);

            tsBuf = (tchar *) REALLOC (tsBuf, (iBufLen + 1) * sizeof (tchar));

            tsBuf[iOldLen] = _T('\0');
            tsBuf[iBufLen] = _T('\0');

            strcat (tsBuf, tsBuf2);
            iMessageLines++;

            if (iMessageLines == SMB_MSG_LINES - 1)
            {
                ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                    NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_LDEVS_OUTSIDE_OF_CT_GROUP, tsBuf));

                FREE(tsBuf);
                tsBuf = NULL;
                iBufLen = 0;
                iOldLen = 0;
                iMessageLines = 0;
            }
        }

        if (iMessageLines > 0)
        {
            ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_LDEVS_OUTSIDE_OF_CT_GROUP, tsBuf));

            FREE(tsBuf);
            tsBuf = NULL;
        }
        
        if (opt.atomic_split_mixed_config == 0)
        {
            /* Fail all objects if the ATOMIC_SPLIT_MIXED_CONFIG OMNIRC variable is not enabled. */
            ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_MIXED_CONFIG_FAILED));

            MarkFailedCTGroup (-1, 0, iDevCount, pSrcDevice);
            retVal = RETURN_ER;
            goto end;
        }
    }

    /* There are no configured CT groups at all, so we can turn off atomic split behaviour.
     * Note, if atomic split was enabled and mixed config was not enabled, this would have
     * failed previously. */
    if (CTGIDCount == 0)
    {
        ErhFullReport(__FCN__, ERH_WARNING, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_DISABLE_BEHAVIOUR));

        DbgPlain (DBG_SSE_3DAPI, 
            _T("[%s] Disabled atomic split behaviour because no CT groups were found."), __FCN__);

        opt.atomic_split = 0;
        goto end;
    }
    /* Also fail if the config is strict and there are more than 1 CT group */
    else if ((CTGIDCount > 1) && (opt.atomic_split_multiple_ctgroups == 0))
    {
        ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_MULTIPLE_GROUPS_FAILED));

        DbgPlain (DBG_SSE_3DAPI, 
            _T("[%s] The agent is running in CT STRICT mode (only 1 CT group is allowed in the backup)."), __FCN__);

        MarkFailedCTGroup (-1, 0, iDevCount, pSrcDevice);
        goto end;
    }

    /* So, we now have a valid CTGID for PVOLs and have recorded any mirrors for each array.  We can compare 
     * this information to a getrminfo query for the ctgid groups. */
    for (iCT = 0; iCT < CTGIDCount; iCT++)
    {
        rminfo rmInfo = {0};
        for (iDev = 0; iDev < iDevCount; iDev++)
        {
            int CTLDEVCount = 0;
            
            if (pSrcDevice[iDev]->iStatus != LDEV_OK)
                continue;
            if ((pSrcDevice[iDev]->ctgid != CTGIDs[iCT]) || (pSrcDevice[iDev]->Dev.serial != CTSerials[iCT]))
                continue;

            /* At this point, we are looking at a source LDEV which is in good state and which has 
             * the correct ctgid and is on the correct array.  We can query this for all of the LDEVS
             * in the CT group. */
            for (iCmd = 0; iCmd < iCmdCount; iCmd++)
                if (pCmdDev[iCmd]->serial == pSrcDevice[iDev]->Dev.serial)
                    break;

            if (iCmd == iCmdCount)
            {
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] ATOMIC SPLIT Cannot find command device for serial number %d!"),
                    __FCN__, pSrcDevice[iDev]->Dev.serial);

                MarkFailedCTGroup (CTGIDs[iCT], pSrcDevice[iDev]->Dev.serial, iDevCount, pSrcDevice);
                continue;
            }

            /* byte 11 = report the LDEVs assigned to CTGID for BC */
            rmInfo.buf[11] = 3;
            /* byte 12 = the CTGID to provide the report for */
            rmInfo.buf[12] = CTGIDs[iCT];

            if (getrminfo(pCmdDev[iCmd],LDEV_LIST,0,0,&rmInfo)==-1)
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("[%s] getrminfo() failed"),__FCN__);

                MarkFailedCTGroup (CTGIDs[iCT], pSrcDevice[iDev]->Dev.serial, iDevCount, pSrcDevice);
                continue;
            }

            /* Extract the number of LDEVS in the group
             *   -extract each LDEV
             *   -check LDEV in the entire PVOL/SVOL list */
            CTLDEVCount = (rmInfo.buf [28] << 24 ) + (rmInfo.buf [29] << 16) + (rmInfo.buf [30] << 8) + rmInfo.buf [31];
            
            DbgStamp(DBG_SSE_3DAPI);
            DbgPlain(DBG_SSE_3DAPI,_T("[%s] On array '%d', CTGID='%d' contains '%d' LDEVs."),__FCN__, CTSerials[iCT], CTGIDs[iCT], CTLDEVCount);
            DbgPlain(DBG_SSE_3DAPI,_T("[%s] The LDEVs are:"),__FCN__);

            for (i = 0;i < CTLDEVCount; i++)
            {
                int CurrentLDEV = (rmInfo.buf[32 + (2 * i)] << 8) + rmInfo.buf[33 + (2 * i)];
                DbgPlain(DBG_SSE_3DAPI,_T("[%s] LDEV#%d='%d/%xh'"),__FCN__, i, CurrentLDEV, CurrentLDEV);
            }


            /* Iterate through all of the LDEVs in the list. */
            for (i = 0;i < CTLDEVCount; i++)
            {
                int iFoundLDEV = 0;
                int CurrentLDEV = 0;

                CurrentLDEV = (rmInfo.buf[32 + (2 * i)] << 8) + rmInfo.buf[33 + (2 * i)];

                DbgStamp(DBG_SSE_3DAPI);
                DbgPlain(DBG_SSE_3DAPI,_T("[%s] CT group LDEV#%d='%d' (%x)"),__FCN__, i, CurrentLDEV, CurrentLDEV);
                
                for (j = 0; j < iDevCount; j++)
                {
                    /* Check if on the correct array. 'iDev' still holds the index to the PVOL that was used
                     * to query for rminfo. */
                    if (pSrcDevice[iDev]->Dev.serial != pSrcDevice[j]->Dev.serial)
                        continue;

                    if (CurrentLDEV == pSrcDevice[j]->Dev.ldevno)
                    {
                        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Matched on array '%d' with the PVOL with LDEV '%d/%xh'."),__FCN__,
                            pSrcDevice[iDev]->Dev.serial,
                            pSrcDevice[j]->Dev.ldevno, pSrcDevice[j]->Dev.ldevno);
                        iFoundLDEV = 1;
                        break;
                    }
                    else if (CurrentLDEV == CTLDEVs[0][j])
                    {
                        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Matched on array '%d' with MU 0 with LDEV '%d/%xh' of the PVOL '%d/%xh'."),__FCN__,
                            pSrcDevice[iDev]->Dev.serial,
                            CTLDEVs[0][j], CTLDEVs[0][j],
                            pSrcDevice[j]->Dev.ldevno, pSrcDevice[j]->Dev.ldevno);
                        iFoundLDEV = 1;
                        break;
                    }
                    else if (CurrentLDEV == CTLDEVs[1][j])
                    {
                        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Matched on array '%d' with MU 1 with LDEV '%d/%xh' of the PVOL '%d/%xh'."),__FCN__,
                            pSrcDevice[iDev]->Dev.serial,
                            CTLDEVs[1][j], CTLDEVs[1][j],
                            pSrcDevice[j]->Dev.ldevno, pSrcDevice[j]->Dev.ldevno);
                        iFoundLDEV = 1;
                        break;
                    }
                    else if (CurrentLDEV == CTLDEVs[2][j])
                    {
                        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Matched on array '%d' with MU 2 with LDEV '%d/%xh' of the PVOL '%d/%xh'."),__FCN__,
                            pSrcDevice[iDev]->Dev.serial,
                            CTLDEVs[2][j], CTLDEVs[2][j],
                            pSrcDevice[j]->Dev.ldevno, pSrcDevice[j]->Dev.ldevno);
                        iFoundLDEV = 1;
                        break;
                    }
                }

                if (iFoundLDEV == 0)
                {
                    /* LDEV not inside the backup! */
                    DbgPlain(DBG_SSE_3DAPI,_T("[%s] Did not find a match.  The CT group '%d/%xh' on array '%d' has LDEVs outside of the backup."),__FCN__,
                        CTGIDs[iCT], CTGIDs[iCT],
                        pSrcDevice[iDev]->Dev.serial);

                    MarkFailedCTGroup (CTGIDs[iCT], pSrcDevice[iDev]->Dev.serial, iDevCount, pSrcDevice);

                    ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage (
                        NLS_SET_SSE, NLS_SSE_ATOMIC_SPLIT_CT_GROUP_HAS_MORE_LDEVS,
                        CTGIDs[iCT]
                        ));

                    break;
                }
            }
            
            /* We've checked the rminfo for this CTGID, let's break out to move on to the next CTGID. */
            break;
        }
    }

end:

    FREE (CTGIDs);
    FREE (CTSerials);
    FREE (CTLDEVs[0]);
    FREE (CTLDEVs[1]);
    FREE (CTLDEVs[2]);

    DbgFcnOutRet(retVal);
    return (retVal);
}


int MarkFailedCTGroup (
    int CTGID,
    int SerialNumber,
    int iDevCount,
    PSSE_DEVICE_T   *pSrcDevice)
{
    int     retVal = RETURN_OK;
    int     i;

    ERH_FUNCTION (_T("MarkFailedCTGroup"));
    DbgFcnIn();

    DbgStamp(DBG_SSE_3DAPI);

    if (CTGID == -1)
        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Marking all LDEVs as failed."),__FCN__);
    else
        DbgPlain(DBG_SSE_3DAPI,_T("[%s] Marking all LDEVs with a ctgid of '%d' on XP with a serial number '%d' as failed."),__FCN__, CTGID, SerialNumber);

    for (i = 0; i < iDevCount; i++)
    {
        if (pSrcDevice[i]->iStatus != LDEV_OK)
            continue;

        if ((CTGID == -1) || ((pSrcDevice[i]->ctgid == CTGID) && pSrcDevice[i]->Dev.serial == SerialNumber))
        {
             DbgPlain(DBG_SSE_3DAPI,_T("[%s] Marking LDEV '%d/%xh' as failed."),__FCN__,
                 pSrcDevice[i]->Dev.ldevno, pSrcDevice[i]->Dev.ldevno);
             pSrcDevice[i]->iStatus = LDEV_INVALID;
        }
    }

    DbgFcnOutRet(retVal);
    return (retVal);
}


void GetCTGIDs (
    int iDevCount,
    PSSE_DEVICE_T   *pSrcDevice,
    int  *CTGIDCount,
    int **CTGIDs,
    int **CTSerials)
{
    int     i;
    int     j;
    int     iCount = 0;
    int     *CTs = NULL;
    int     *Serials = NULL;

    ERH_FUNCTION (_T("GetCTGIDs"));
    DbgFcnIn();

    if (NULL == CTGIDCount || NULL == CTGIDs || NULL == CTSerials)
    {
        /* Invalid parameters */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_CRITICAL);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        goto end;
    }

    
    for (i = 0; i < iDevCount; i++)
    {
        int iMatchFound = 0;

        /* Not in good status.  If this LDEV is from the CT group, this error is caught when a check 
         * for LDEVs in the CT group that are outside of the backup is performed. */
        if (pSrcDevice[i]->iStatus != LDEV_OK)
            continue;

        /* No CTGID for this LDEV. */
        if (pSrcDevice[i]->ctgid == 0)
            continue;

        /* Found something with a CTGID. Let's check if it's in the list. */
        for (j = 0; j < iCount; j++)
        {
            /* Found a match */
            if ((pSrcDevice[i]->ctgid == CTs[j]) && (pSrcDevice[i]->Dev.serial == Serials[j]))
            {
                DbgStamp (DBG_SSE_3DAPI);
                DbgPlain (DBG_SSE_3DAPI, _T("[%s] CTGID '%d' on array '%d' already in list at index '%d'."), __FCN__,
                    CTs[j], Serials[j], j);
                iMatchFound = 1;
                break;
            }
        }

        /* No need to insert, let's go on with the loop. */
        if (iMatchFound)
            continue;

        /* No match in the list, need to insert the new CTGID. */
        DbgStamp (DBG_SSE_3DAPI);
        DbgPlain (DBG_SSE_3DAPI, _T("[%s] Inserting CTGID '%d' on array '%d' into the list"), __FCN__,
            pSrcDevice[i]->ctgid,
            pSrcDevice[i]->Dev.serial);

        CTs = REALLOC (CTs, (iCount + 1) * sizeof (int));
        Serials = REALLOC (Serials, (iCount + 1) * sizeof (int));

        CTs[iCount] = pSrcDevice[i]->ctgid;
        Serials[iCount] = pSrcDevice[i]->Dev.serial;
        iCount++;
    }

    DbgStamp (DBG_SSE_3DAPI);
    DbgPlain (DBG_SSE_3DAPI, _T("[%s] Located a total of '%d' unique CTGIDs."), __FCN__, iCount);

    for (j = 0; j < iCount; j++)
    {
        DbgPlain (DBG_SSE_3DAPI, 
            _T("[%s] Array '%d' contains a CTGID of '%d'"), __FCN__, Serials[j], CTs[j]);
    }

    *CTGIDCount = iCount;
    *CTGIDs = CTs;
    *CTSerials = Serials;

end:
    DbgFcnOut();
    return;
}

/*========================================================================*//**
*
* @param     pSseArray     - pointer to SSE array structure
*
* @retval    void
*
* @brief     Function applicable only in case of snapshots. It checks and reports if any
*            snapshots are part of a pool which reached the configured capacity threshold.
*            This is 80% by default.
*
*//*=========================================================================*/
void 
SseCheckPoolStatus(PSSE_ARRAY_T pSseArray)
{           
    int i = 0;
    int idxPID = 0;
    int iCmd = 0;
    int idxUniqueID = 0;

    /* valid pid number is from 0 to 127 (on XP10000) */
    /* maximum unsigned char is 255 */
    unsigned char arrayPoolIDs[STRLEN_STD + 1] = {0};

    tchar *reportBuf = NULL;

    ERH_FUNCTION (_T("SseCheckPoolStatus"));
    DbgFcnIn();

    for (i = 0; i < pSseArray->iDevCount; i++)
    {
        if (pSseArray->pSrcDevices[i]->PairStatus == STAT_PFUL ||
            pSseArray->pSrcDevices[i]->PairStatus == STAT_PFUS)
        {
            pairstatinfo pairStat = {0};
            targdev srcDevice = {0};

            DbgPlain (DBG_SSE_ACTION, _T("[%s] Found device with status PFUL/PFUS, reporting pool stats."), 
                __FCN__);


            srcDevice.targid = pSseArray->pSrcDevices[i]->Dev.targid;
            srcDevice.lun    = pSseArray->pSrcDevices[i]->Dev.lun;
            srcDevice.port   = pSseArray->pSrcDevices[i]->Dev.port;
            srcDevice.ldevno = pSseArray->pSrcDevices[i]->Dev.ldevno;
            srcDevice.mun    = pSseArray->pSrcDevices[i]->Dev.mun;
            srcDevice.mmode  = MD_MRCF;

            for (iCmd = 0; iCmd < pSseArray->iCmdCount; iCmd++)
            {
                if (pSseArray->pCmdList[iCmd]->serial == pSseArray->pSrcDevices[i]->Dev.serial)
                {
                    break;
                }
            }

            if (iCmd == pSseArray->iCmdCount)
            {
                DbgPlain (DBG_SSE_ACTION, _T("[%s] Command device not found! Continue..."), __FCN__);
                continue;
            }

            /* pairvolstat to get pool ID */
            if (-1 == pairvolstat(pSseArray->pCmdList[iCmd], &srcDevice, &pairStat))
            {
                DbgPlain (DBG_SSE_ACTION, _T("[%s] pairvolstat failed at index %d"), __FCN__, i);
                continue;
            }
            if (MRCF_CM_SNAP != pairStat.copymode)
            {
                DbgPlain (DBG_SSE_ACTION, _T("[%s] Not a snapshot, continue..."), __FCN__);
                continue;
            }

            /* check if we already have this pid */
            for (idxPID = 0; idxPID < idxUniqueID; idxPID++)
            {
                if (arrayPoolIDs[idxPID] == pairStat.pid)
                {
                    DbgPlain (DBG_SSE_ACTION, _T("[%s] Pool id %d already resolved"), __FCN__, pairStat.pid);
                    break;
                }
            }

            DbgPlain (DBG_SSE_ACTION, 
                    _T("[%s] Checking index: index = %d, idxPID = %d"), __FCN__, idxUniqueID, idxPID);

            if (idxUniqueID == idxPID)
            {
                /* pool id not yet in array */
                poolinfo poolStat = {0};
                tchar tempBuf[STRLEN_1K] = {0};
                tchar poolStatStr[STRLEN_STD] = {0};
                int poolID = pairStat.pid;

                DbgPlain (DBG_SSE_ACTION, _T("[%s] Found pool with ID %d. Resolving it..."), __FCN__, pairStat.pid);
                if (-1 == GetPoolStat (pSseArray->pCmdList[iCmd], poolID, &poolStat))
                {
                    DbgPlain (DBG_SSE_3DAPI, _T("[%s] getpoolstat failed"), __FCN__);
                    continue;
                }
                DbgPrintPoolStat (poolStat.pstat, poolStatStr);

                sprintf (tempBuf,
                         _T("\n\t%-7d %-4d %s  %4d%% %6d %13d %13d"),
                         pSseArray->pSrcDevices[i]->Dev.serial, 
                         pairStat.pid,     /* Pool ID */
                         poolStatStr,      /* Pool status */
                         poolStat.prate,   /* Pool usage % */
                         poolStat.numsnap, /* number of snapshots in pool */
                         poolStat.premsz,  /* available capacity (MB) */
                         poolStat.pallsz); /* all capacity in pool (MB)*/

                /* StrAppend calls REALLOC which behaves as MALLOC in case source ptr is NULL */
                StrAppend (reportBuf, tempBuf);

                arrayPoolIDs[idxUniqueID] = pairStat.pid;
                idxUniqueID++;
            }
        }
    }
    if (NULL == reportBuf)
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] Nothing to report."), __FCN__);
    }
    else
    {
        ErhFullReport(__FCN__, ERH_WARNING, 
            NlsGetMessage (NLS_SET_SSE, NLS_SSE_BC_POOL_STATUS_REPORT, reportBuf));
        FREE (reportBuf);
    }

    DbgFcnOut();
    return;
}


