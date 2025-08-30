/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   SSEA
* @file      integ/smb/sse/ssedbxp.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Interface to libdbxp.
*
* @since     ...
*/

#include "ssermlib.h" /* This should be included before target.h */
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/smb/sse/ssedbxp.c $Rev$ $Date::                      $:") ;
#endif 

/* System includes */

/* Application specific includes */

/* OB common includes */
#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "integ/smb/libsmb.h"
#include "integ/smb/libsmbdbsys2.h"
#include "lib/smbdb/dbxp/libdbxp_defs.h"
#include "lib/smbdb/dbxp/libdbxp.h"

/* Module includes */
#include "sse.h"
#include "ssedef.h"
#include "optmgr.h"
#include "sseapi.h"
#include "debug.h"

USES_CONVERSION

extern optRec  opt;  

#include "ssex.h"
#include "ssedbxp.h"

int
SseDbXP_AddSession (
        PSSE_DEVICE_T   *pSrcDev,
        PSSE_DEVICE_T   *pTgtDev,
        PSMB_RAW_DISK_T *pRdsk,
        int             iCount,
        int             instant_restore,
        int             mirror_type,
        int             mun,
        tchar           *app_system,
        tchar           *bckp_system)
{
    int retVal = RETURN_OK;
/* #if !TARGET_SOLARIS*/
    int i;

    sessInfo_t  sesInf;
    xpDev_t     *devList = NULL;
    int         devCount = 0;

    ERH_FUNCTION (_T("SseDbXP_AddSession"));    
    
    DbgFcnIn ();

    if (iCount<=0)
        goto end;

    memset (&sesInf, 0, sizeof (sesInf));

    strncpy (sesInf.sessId, opt.sessID, STRLEN_SESSIONNAME);
    sesInf.sessId [STRLEN_SESSIONNAME] = 0;
    sesInf.ir=instant_restore;
    strcpy (sesInf.appSystem, app_system);
    strcpy (sesInf.bckpSystem, bckp_system);

    for (i=0; i<iCount; i++)
    {
        if (pSrcDev[i]->iStatus != LDEV_OK ||
            pTgtDev[i]->iStatus != LDEV_OK)
            continue;

        ++devCount;

        if ((devList = (xpDev_t *) REALLOC (devList, (devCount) * 
                sizeof (xpDev_t)))==NULL)
        {
             DbgStamp (DBG_UNEXPECTED);
             DbgPlain (DBG_UNEXPECTED,
                 _T("[%s] Memory allocation failed!"), __FCN__);
             retVal = RETURN_ER;
        }

        memset (devList + devCount - 1, 0, sizeof (*devList));


        devList[devCount-1].p_ldev = pSrcDev[i]->Dev.ldevno;
        devList[devCount-1].p_seq  = pSrcDev[i]->Dev.serial;
        devList[devCount-1].p_port  = pSrcDev[i]->Dev.port;

        devList[devCount-1].ldev = pTgtDev[i]->Dev.ldevno;
        devList[devCount-1].seq  = pTgtDev[i]->Dev.serial;

        devList[devCount-1].p_tid = pSrcDev[i]->Dev.targid;
        devList[devCount-1].p_lun = pSrcDev[i]->Dev.lun;

        devList[devCount-1].ir=instant_restore;
        devList[devCount-1].m_type=mirror_type;
        devList[devCount-1].mu=mun;

        devList[devCount-1].crc=pRdsk[i]->crc;

        strcpy (devList[devCount-1].appSystem, app_system);
        strcpy (devList[devCount-1].bckpSystem, bckp_system);
    }

    ErhClearError ();
	if (dbXP_updtSess (&sesInf, devList, devCount) != RET_OKAY)
	{
		DbgStamp (DBG_UNEXPECTED);
		DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_updtSess failed!"), __FCN__);

        ErhClearError ();

        retVal = RETURN_ER;
		goto end;
	}

end:
    DbgFcnOutRet (retVal);
/* #endif  */ /* !TARGET_SOLARIS */
    return (retVal);

} /* SseDbXP_AddSession */

int
SseDbXP_RmSession (tchar *sessId)
{
    int retVal = RETURN_OK;
    sessInfo_t  sesInf;
    xpDev_t     *devList;
    int         i;
    int         devCount;
    
    ERH_FUNCTION (_T("SseDbXP_RmSession"));    
    
    DbgFcnInEx(__FLND__, _T("sessId = %s"), sessId);

    strcpy (sesInf.sessId, sessId);
    sesInf.ir = -1;

    ErhClearError ();
    if (dbXP_getDevs(&sesInf, &devList, &devCount) != RET_OKAY)
	{
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDevs failed!"), __FCN__);

        ErhClearError ();

        retVal = RETURN_ER;
        goto end;
    }

    for (i=0; i<devCount; i++)
    {
        ErhClearError ();
		if (dbXP_rmDev(&(devList[i])) != RET_OKAY)
		{
			DbgStamp (DBG_UNEXPECTED);
			DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_rmDev failed!"), __FCN__);

            ErhClearError ();

            retVal = RETURN_ER;
			goto end;
		}
    }
    
    if (SmbDbSys_Remove (sessId) != SMBDBSYS_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] SmbDbSys_Remove failed!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }
    
end:
    FREE (devList);
    DbgFcnOutRet (retVal);
    return (retVal);
} /* SseDbXP_RmSession */

int
SseDbXP_RmLdevs (
        PSSE_DEVICE_T   *pDev,
        int             iCount)
{
    int retVal = RETURN_OK;
/* #if !TARGET_SOLARIS  */
    xpDev_t     *devList[2];
    int			i;

    ERH_FUNCTION (_T("SseDbXP_RmLdevs"));    
    
    DbgFcnIn ();

    if (iCount<=0)
        goto end;

    devList[0] = (xpDev_t *) MALLOC (sizeof (xpDev_t));
    for (i=0; i<iCount; i++)
    {
        if (pDev[i]->iStatus != LDEV_OK)
            continue;

        memset (devList[0], 0, sizeof (*(devList[0])));

        devList[1] = NULL;

        devList[0]->ldev = pDev[i]->Dev.ldevno;
        devList[0]->seq  = pDev[i]->Dev.serial;

        ErhClearError ();
        if (dbXP_getDev (devList) != RET_OKAY)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDev failed!"), __FCN__);

            ErhClearError ();

            retVal = RETURN_ER;
            goto end;
        }

        if (SmbDbSys_Remove (devList[0]->sessId) != SMBDBSYS_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] SmbDbSys_Remove failed!"), 
                __FCN__);

            retVal = RETURN_ER;
            goto end;
        }
    
        ErhClearError ();
		if (dbXP_rmDev(devList[0]) != RET_OKAY)
		{
			DbgStamp (DBG_UNEXPECTED);
			DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_rmDev failed!"), __FCN__);

            ErhClearError ();

            retVal = RETURN_ER;
			goto end;
		}
    }
    FREE (devList[0]);

end:
    DbgFcnOutRet (retVal);
/* #endif */ /* !TARGET_SOLARIS */
    return (retVal);

} /* SseDbXP_RmLdevs */

int
SseDbXP_IsLdevLogged (
        targdev         *pDev,
        int             *status,
        time_t          *dateTime)
{
    int retVal = RETURN_OK;
/* #if !TARGET_SOLARIS */
    xpDev_t     *devList[2];

    ERH_FUNCTION (_T("SseDbXP_IsLdevLogged"));    
    
    DbgFcnIn ();


    if ((devList[0] = (xpDev_t *) MALLOC (sizeof (xpDev_t)))==NULL)
    {
         DbgStamp (DBG_UNEXPECTED);
         DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Memory allocation failed!"), __FCN__);
         retVal = RETURN_ER;
    }                        
    memset (devList[0], 0, sizeof (*(devList[0])));

    devList[1] = NULL;

    devList[0]->ldev = pDev->ldevno;
    devList[0]->seq  = pDev->serial;

    ErhClearError ();
    if (dbXP_getDev (devList) != RET_OKAY)
	{
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDev failed!"), __FCN__);

        ErhClearError ();

        retVal = RETURN_ER;
        goto end;
    }

    if (!strcmp (devList[0]->sessId, _T("-1")))
        *status = 0;
    else
    {
        *status = 1;
        *dateTime = SseDbXP_StrToTime (devList[0]->dateTime);
        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION, _T("dateTime = %ld"), *dateTime);
    }

end:

    FREE (devList[0]);
    DbgFcnOutRet (retVal);
/* #endif */ /* !TARGET_SOLARIS */
    return (retVal);

} /* SseDbXP_IsLdevLogged*/

time_t
SseDbXP_StrToTime (const tchar *str)
{
    int year, month, day, hour, min, sec, days_since_70;
    time_t seconds;
	struct tm *timeptr;
    time_t retVal;

    ERH_FUNCTION (_T("SseDbXP_StrToTime"));    
    DbgFcnIn();
    DbgPlain (DBG_SSE_ACTION+100, _T("[%s] Converting string '%s'"), __FCN__, str);
    
    DbgPlain(DBG_SSE_ACTION, _T("[%s] Input time string: \"%s\""), __FCN__, str);
    year = month = day = hour = min = sec = 0;
    sscanf (
        str,
        _T("%d/%d/%d %d:%d:%d"),
        &month,
        &day,
        &year,
        &hour,
        &min,
        &sec
    );
    
    DbgPlain(DBG_SSE_ACTION, _T("[%s] Year %d  Month %d  Day %d  Hour %d  Minute %d  Second %d"), __FCN__,
        year, month, day, hour, min, sec);
    
    /*-------- if not short or long form reject -------------------------*/

    /*
    if ( (n!=3) && (n!=6) ) return OB2_TIME_T_OUT_OF_RANGE;
    */


    /*-------- expand year number if necessary --------------------------*/

    if (year >= 0 && year < 69) 
    {
        year += 2000;
    } else if (year >= 70 && year < 100) 
    {
        year += 1900;
    }
    DbgPlain(DBG_SSE_ACTION, _T("[%s] Year=%d"), __FCN__, year);

    if (year < 1970)
    {
        DbgPlain(DBG_SSE_ACTION, _T("[%s] Dates before 1970 are not supported"), __FCN__);
        retVal = (time_t)(-1);
        goto end;
    }
    if (year > 2037)
    {
        DbgPlain(DBG_SSE_ACTION, _T("[%s] System time functions do not support dates beyond 2038"), __FCN__);
        retVal = (time_t)(-1);
        goto end;
    }
    /*
    if (year < 1970) return OB2_TIME_T_OUT_OF_RANGE;
    if (year > 2037) return OB2_TIME_T_OUT_OF_RANGE;
    */

    /* --------------------------------------------------------------------
    |   We will consider March  to be the first month so that our formula
    |   remains simple for leap and non-leap years ( Feb being the odd month
    |   out).
     -------------------------------------------------------------------- */

    year = year - 1971 + (month + 9)/12;
    month = (month + 9) % 12;


    /* --------------------------------------------------------------------
    |
    |  The MAGIC formula.
    |
     -------------------------------------------------------------------- */

    days_since_70 = 365*year + (year-2)/4 +
                    30*month + ((month%5)+1)/2 + 3*(month/5) + 59 + day;


    /* --------------------------------------------------------------------
    |   Correction for 1970 thru Feb. 1972.  This is here because
    |   T is the number of days since the beginning of time, not
    |   including the present day.  The formula adds the present day
    |   though, but after 1972 this is compensated for by the correction
    |   for leap years being one less than it should be.
     -------------------------------------------------------------------- */

    if (year < 2) 
    {
        days_since_70 -= 1;
    }


    /*----------------- convert to seconds and we are done ---------------*/

    seconds = ((24*days_since_70 + hour) * 60 + min)*60 + sec;


	timeptr=gmtime(&seconds);
        
    /*
    if (timeptr==NULL) return OB2_TIME_T_OUT_OF_RANGE;
    */

    /*
        Dates beyond year 2038 may not be handled correctly by 32-bit systems
    */
    if (timeptr==NULL) 
    {
        DbgPlain(DBG_SSE_ACTION, _T("[%s] gmtime returned NULL"), __FCN__);
        retVal = (time_t)(-1);
        goto end;
    }

    DbgPlain(DBG_SSE_ACTION, _T("[%s] gmtime returned the following values:"), __FCN__);
    DbgPlain(DBG_SSE_ACTION, _T("Year %d  Month %d  Day %d  Hour %d  Minute %d  Second %d"),
        timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	timeptr->tm_isdst=-1;
    retVal = mktime(timeptr);

end:
    DbgPlain(DBG_SSE_ACTION, _T("[%s] retVal = %ld"), __FCN__, retVal);
    DbgFcnOut();
    return (retVal);

/*
CHECKME: Converting to seconds and then to tm using gmtime() is unnecessary.
We should just declare a tm struct, fill it in and proceed with mktime().
*/
    /*
    seconds = mktime(&tm_struct);
    if (seconds == -1) return OB2_TIME_T_OUT_OF_RANGE;
    return (seconds);
    */
}

int
SseDbXP_IsLdevExcluded (
        targdev         *pDev,
        int             *status)
{
    int retVal = RETURN_OK;
/* #if !TARGET_SOLARIS */

    ERH_FUNCTION (_T("SseDbXP_IsLdevExcluded"));    
    
    DbgFcnIn ();

    ErhClearError ();
    *status = dbXP_ldevCheck (pDev->serial, pDev->ldevno);
    ErhClearError ();

    DbgFcnOut ();
/* #endif  */ /*!TARGET_SOLARIS */
    return (retVal);

} /* SseDbXP_RmLdevs */

int
SseDbXP_DevList (
        tchar           *sessId,
        PSSE_DEVICE_T   **srcDevices,
        PSSE_DEVICE_T   **tgtDevices,
        int             *devCount,
        int             *mirrorConf,
        int             *mun)
{
    int retVal = RETURN_OK;
/*#if !TARGET_SOLARIS */
    sessInfo_t  sesInf;
    xpDev_t     *devList;
    int         i;
    
    ERH_FUNCTION (_T("SseDbXP_DevList"));    
    
    DbgFcnInEx(__FLND__, _T("sessId = %s"), sessId);

    strcpy (sesInf.sessId, sessId);
    sesInf.ir = -1;

    ErhClearError ();
    if (dbXP_getDevs(&sesInf, &devList, devCount) != RET_OKAY)
	{
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDevs failed!"), __FCN__);

        ErhClearError ();

        retVal = RETURN_ER;
        goto end;
    }

    if (*devCount <=0)
	{
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] No data in DBXP for session %s!"), 
                __FCN__, sessId);

        retVal = RETURN_ER;
        goto end;
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] devCount = %d"), __FCN__,
            *devCount);

    if ((*srcDevices = (PSSE_DEVICE_T*) MALLOC
            (*devCount * sizeof (PSSE_DEVICE_T))) == NULL ||

        memset (*srcDevices, 0, *devCount * 
            sizeof (PSSE_DEVICE_T)) != *srcDevices ||

        ((*srcDevices)[0] = (PSSE_DEVICE_T) MALLOC
            (*devCount * sizeof (SSE_DEVICE_T))) == NULL ||

        memset ((*srcDevices)[0], 0, *devCount *
            sizeof (SSE_DEVICE_T)) != (*srcDevices)[0] ||

        (*tgtDevices = (PSSE_DEVICE_T*) MALLOC
            (*devCount * sizeof (PSSE_DEVICE_T))) == NULL ||

        memset (*tgtDevices, 0, *devCount *
             sizeof (PSSE_DEVICE_T)) != *tgtDevices ||

        ((*tgtDevices)[0] = (PSSE_DEVICE_T) MALLOC
             (*devCount * sizeof (SSE_DEVICE_T))) == NULL ||

        memset ((*tgtDevices)[0], 0, *devCount *
              sizeof (SSE_DEVICE_T)) != (*tgtDevices)[0]
    )
    {
        /* Critical error */
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Memory allocation failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Memory allocation OK"), __FCN__);

    for (i=1; i<*devCount; i++)
        (*srcDevices)[i] = (*srcDevices)[0]+i;
    for (i=1; i<*devCount; i++)
        (*tgtDevices)[i] = (*tgtDevices)[0]+i;

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Structure initialization OK"), __FCN__);

    /* Now fill the structures */
    *mirrorConf = -1;
    *mun = -1;
    for (i=0; i<*devCount; i++)
    {

        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION, _T("%2d: sessId = %s, ir = %d, ldev = %d, seq = %d, p_ldev = %d, p_seq = %d, m_type = %d, mu = %d, crc = %d"),
            i, devList[i].sessId, devList[i].ir, devList[i].ldev, devList[i].seq, devList[i].p_ldev, devList[i].p_seq, devList[i].m_type, devList[i].mu, devList[i].crc);

        if (strcmp (devList[i].sessId, sessId))
        {
            /* Critical error */
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] Session ID mismatch! (%s, %s)"), __FCN__, 
                devList[i].sessId, sessId);

            retVal = RETURN_ER;
            goto end;
        }

        if (devList[i].ir != 1)
        {
            /* Critical error */
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] IR flag is not set for device (%d, %d)"), __FCN__,
                devList[i].seq, devList[i].ldev);

            retVal = RETURN_ER;
            goto end;
        }

        if (*mirrorConf == -1)
            *mirrorConf = devList[i].m_type;
        else
            if (*mirrorConf != devList[i].m_type)
            {
                /* Critical error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Mirror configuration not the same for all devices in session! (%d, %d)"), 
                    __FCN__, *mirrorConf, devList[i].m_type);

                retVal = RETURN_ER;
                goto end;
            }

        if (*mun == -1)
            *mun = devList[i].mu;
        else
            if (*mun != devList[i].mu)
            {
                /* Critical error */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Mirror number not the same for all devices in session! (%d, %d)"), 
                    __FCN__, *mun, devList[i].mu);

                retVal = RETURN_ER;
                goto end;
            }

        (*srcDevices)[i]->Dev.ldevno = devList[i].p_ldev;
        (*srcDevices)[i]->Dev.serial = devList[i].p_seq;
        /*(*srcDevices)[i]->Dev.port = devList[i].p_port;*/
        (*srcDevices)[i]->Dev.port = 0;
        (*srcDevices)[i]->Dev.mun = devList[i].mu;
        (*srcDevices)[i]->Dev.lun = -1;
        (*srcDevices)[i]->Dev.targid = -1;

        (*tgtDevices)[i]->Dev.ldevno = devList[i].ldev;
        (*tgtDevices)[i]->Dev.serial = devList[i].seq;
        (*tgtDevices)[i]->crc = devList[i].crc;
        (*tgtDevices)[i]->Dev.port = -1;
        (*tgtDevices)[i]->Dev.lun = -1;
        (*tgtDevices)[i]->Dev.targid = -1;

        (*srcDevices)[i]->iStatus = LDEV_OK;
        (*tgtDevices)[i]->iStatus = LDEV_OK;

        /* Problem with checking. We don't have RDSK objects to requery */

        DbgPrintSseDev ((*srcDevices)[i]);
        DbgPrintSseDev ((*tgtDevices)[i]);
    }

    
    FREE (devList);

end:
    DbgFcnOutRet (retVal);
/* #endif  */ /*!TARGET_SOLARIS */
    return (retVal);

} /* SseDbXP_DevList */


int
SseDbXP_SessionList (
    PSSE_DEVICE_T *pDev,
    int           iCount,
    tchar         **list)
{
    int     retVal = RETURN_OK;
    xpDev_t **devList = NULL;
    int     i = 0;
    tchar   *inlist = NULL;
    int     list_len = STRLEN_STD;
    int     numberOfElements=0;

    ERH_FUNCTION (_T("SseDbXP_SessionList"));    
    DbgFcnIn ();

    /* Check parameters */
    if (pDev == NULL || list == NULL)
    {
        ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
                  _T("[%s] Invalid parameter(s) pDev = %p, list = %p"),
                  __FCN__, pDev, list);

        retVal = RETURN_ER;
        goto end;
    }

    if (iCount<=0)
        goto end;

    

    for (i=0; i<iCount; i++)
    {
        if (pDev[i]->iStatus == LDEV_OK)
            numberOfElements++;
    }


    if (numberOfElements<=0)
        goto end;

    
    DbgPlain (DBG_SSE_ACTION, 
              _T("[%s] Number of pDev elements with OK status = %d"),
              __FCN__, numberOfElements);
    

    /* Allocat memory */
    devList = (xpDev_t**) MALLOC( (numberOfElements + 1) * sizeof(xpDev_t*) );
    inlist = (tchar *) MALLOC (STRLEN_STD * sizeof(tchar));
    
    DbgPlain (DBG_SSE_ACTION, 
              _T("[%s] MALLOC passed"),
              __FCN__);
    

    for (i=0; i<numberOfElements; i++)
    {
        DbgPlain (DBG_SSE_ACTION, 
              _T("[%s] current number i = %d"),
              __FCN__, i);
                
        devList[i] = (xpDev_t *) MALLOC (sizeof (xpDev_t));
        memset (devList[i], 0, sizeof (xpDev_t));

        
        devList[i]->ldev = pDev[i]->Dev.ldevno;
        devList[i]->seq  = pDev[i]->Dev.serial;
        

    }

    devList[numberOfElements] = NULL;

        

    /* Initialize memory */
    inlist[0] = _T('\0');

    ErhClearError ();
        
    if (dbXP_getDev (devList) != RET_OKAY)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbXP_getDev failed!"), __FCN__);

        ErhClearError ();

        retVal = RETURN_ER;
        goto end;
    }
    
    for (i=0; i<numberOfElements; i++)
    {
        tchar sess_name [STRLEN_STD];
        int   maxLen=0;
        
        
        /* Add session name to session list */
        strcpy (sess_name, devList[i]->sessId);

        maxLen = (int) (strlen (inlist) + strlen (sess_name) + 1); 
        
        DbgPlain (DBG_SSE_ACTION, 
                  _T("[%s] maxLen = %d, list_len = %d"),
                  __FCN__, maxLen, list_len);

        if (maxLen >= list_len)
        {
                        
            list_len += STRLEN_STD;

            DbgPlain (DBG_SSE_ACTION, 
                _T("[%s] list_len = %d"),
                __FCN__, list_len);
                        
            inlist = (tchar *) REALLOC (inlist, list_len * sizeof(tchar));
            
        } 
        
        if (inlist[0] != _T('\0'))
            strcat (inlist, _T(","));
        
        strcat (inlist, sess_name);

        DbgPlain (DBG_SSE_ACTION, 
                _T("[%s] strlen (inlist) = %d"),
                __FCN__, (int)strlen (inlist));

    } 


    FREE(*list);
    
    if (inlist[0] != _T('\0'))
    {
        *list = StrNewCopy(inlist);
    }


    DbgPlain (DBG_SSE_ACTION, _T("[%s] Session removal list: [%s]"),
              __FCN__, NS(*list));
    
end:
    FREE(inlist);

    if ( devList != NULL)
    {
        for (i=0; i<numberOfElements; i++)
        {
            FREE (devList[i]);

        }
        
        FREE (devList);
    }
    

    DbgFcnOutRet (retVal);
    return (retVal);
} /* SseDbXP_SessionList */



int SseDbXP_RetrieveLdevs(
    PSSE_DEVICE_T       *pSseDevice,
    int                 iCount
    )
{
    int         retVal = RETURN_OK;
    int         i;
    int         iDevIndex;
    int         iDisks = 0;
    int         iPvols;
    xpDev_t     *pPvols = NULL;
    xpDev_t     *pInputPvols = NULL;

    ERH_FUNCTION (_T("SseDbXP_RetrieveLdevs"));
    DbgFcnIn ();

    DbgPlain (DBG_SSE_ACTION, _T("[%s] Input parameters: pSseDevice=%p, iCount=%u"), __FCN__, pSseDevice, iCount);
    DbgPlain (DBG_SSE_ACTION, _T("[%s] Trying to obtain disks' data from the dbXP first"), __FCN__);

	/* How much memory should be allocated for the list of pInputPvols? */
    for (i=0, iDisks=0; i<iCount; i++)
    {
        if (LDEV_OK == pSseDevice[i]->iStatus)
        {
            iDisks++;
        }
    }

    DbgPlain(DBG_SSE_ACTION, _T("[%s] %u disks to be resolved"), __FCN__, iDisks);
    pInputPvols = (xpDev_t*) CALLOC(iDisks, sizeof(xpDev_t));
    memset(pInputPvols, 0, iDisks*sizeof(xpDev_t));

    for (iDevIndex=0, i=0; i<iDisks && iDevIndex<iCount; iDevIndex++)
    {
        if (LDEV_OK == pSseDevice[iDevIndex]->iStatus)
        {
            pInputPvols[i].p_seq  = pSseDevice[iDevIndex]->Dev.serial;
            pInputPvols[i].p_ldev = pSseDevice[iDevIndex]->Dev.ldevno;
            DbgPlain(DBG_SSE_ACTION, _T("[%s] The following data will be sent to dbXP_getLdevList as field nr. %u: S/N: %u  LDEV: %u"), __FCN__,
                i,
                pInputPvols[i].p_seq,
                pInputPvols[i].p_ldev);
            i++;
        }
    }

    /* Obtain a list of all all consistent PVOLs from the DBXP first */
    if (RETURN_OK != dbXP_getLdevList(pInputPvols, iDisks, &pPvols, &iPvols))
    {
        DbgPlain(DBG_SSE_ACTION, _T("[%s] Retrieving of entries from DBXP failed, skipping this phase."), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    /* Nothing to do if an empty list is returned */
    if (iPvols<=0 || NULL==pPvols)
    {
        DbgPlain(DBG_SSE_ACTION, _T("[%s] No entries were fond in the DBXP"), __FCN__);
        goto end;
    }

    /* 
        Then check if any LDEV to be resolved is listed among them.
        Each LDEV may have several (more precise, up to 3, i.e. number of possible pairs) entries.
        In normal circumstances the same TID and LUN should be contained by all such entries, therefore
        data from the first entry with a valid XP serial number and LDEV will be taken.

        At this point the consistent TID and LUN will only be set to PSSE_DEVICE_T structure,
        their validity will be checked later.
    */

    for (iDevIndex=0; iDevIndex<iCount; iDevIndex++)
    {
        if (pSseDevice[iDevIndex]->iStatus != LDEV_OK)
        {
            DbgPlain(DBG_SSE_ACTION, _T("[%s] Status of disk nr. %u is not marked OK, therefore it will not be resolved"), __FCN__, iDevIndex);
            continue;  /* for (iDevIndex) */
        }

        /*
            If another LDEV resolve method is ever inserted before this one in future, an extra check whether a disk is already resolved must be inserted.
            Otherwise any previous methods do not make much sense.
        */

        for (i=0; i<iPvols; i++)
        {
            DbgPlain(DBG_SSE_ACTION, _T("[%s] Checking data file nr. %u: S/N=%u, LDEV=%u, port=%u"), __FCN__, i,
                pPvols[i].p_seq,
                pPvols[i].p_ldev,
                pPvols[i].p_port);
            if ( pPvols[i].p_tid >=0 && 
                    pPvols[i].p_lun >=0 && 
                    pSseDevice[iDevIndex]->Dev.serial == pPvols[i].p_seq &&
                    pSseDevice[iDevIndex]->Dev.ldevno == pPvols[i].p_ldev &&
                    pSseDevice[iDevIndex]->Dev.port == pPvols[i].p_port )
            {
                pSseDevice[iDevIndex]->Dev.targid = pPvols[i].p_tid;
                pSseDevice[iDevIndex]->Dev.lun = pPvols[i].p_lun;
                DbgPlain(DBG_SSE_ACTION, _T("[%s] Valid data for the disk nr. %u were found in the file nr. %u. TID=%u and LUN=%u"), __FCN__, iDevIndex, i, pPvols[i].p_tid, pPvols[i].p_lun);
                break; /* out of for (i) */
            } /* if */
        } /* for (i) */
    } /* for (iDevIndex) */
    DbgPlain(DBG_SSE_ACTION, _T("[%s] Retrieving disks' data from the dbXP done."), __FCN__);

end:
    free(pPvols);
    free(pInputPvols);
    DbgFcnOut();
    return (retVal);
}

/*========================================================================*//**
*
* @param[in]   serial   XP serial number
* @param[out]  username retrieved username
* @param[out]  password retrieved password
*
* @retval    RETURN_OK success
* @retval    RETURN_ER failure
*
* @brief     Wrapper aroud dbXP_getUsrPwd
*//*=========================================================================*/
int
SseDbXP_RetrieveUsrPasswd(int   serial,
                          tchar *username,
                          tchar *password)
{
    int retVal = RETURN_OK;

    ERH_FUNCTION (_T("SseDbXP_RetrieveUsrPasswd"));
    
    DbgFcnIn ();

    retVal = dbXP_getUsrPwd (serial, username, password);

    DbgFcnOut ();

    return (retVal);
}
