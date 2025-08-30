/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Backup Session Manager
* @file      sm/bsm2/restart.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Restart session functions.
*
* @since     dd.mm.yyyy who        description
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /sm/bsm2/restart.c $Rev$ $Date::                      $:");
#endif

#include <stdio.h>
#include <string.h>

#if TARGET_WIN32
#   include "sm/smcmn/win32.h"
#endif

#include "lib/cmn/common.h"
#include "lib/cmn/gen_io.h"
#include "lib/ipc/ipc.h"

#include "lib/parse/vec.h"
#include "lib/parse/sdstruct.h"
#include "lib/parse/sdparse.h"
#include "lib/parse/wlparse.h"
#include "lib/parse/wllist.h"
#include "lib/parse/AdminParsers.h"

#if _SUPP_PREDDEVICES
#include "lib/cmn/suppdevices.h"
#endif

#include "db/dbcmn.h"
#include "db/cdb/cdbsession.h"
#include "db/cdb/cdbsmapi.h"
#include "db/cdb/cdbadm.h"
#include "sm/smcmn/smcommon.h"
#include "sm/smcmn/catalog.h"
#include "brsm.h"
#include "csm.h"
#include "brsmerr.h"
#include "brsmstruct.h"
#include "brsmutil.h"
#include "bsmutil.h"
#include "brsmhooks.h"
#include "restart.h"
#include "prototypes.h"

#include "cs/csa/csa.h"


/* ----------------------------------------------------------------
|   Structures & other stuff for restart session capability
----------------------------------------------------------------- */

/*==========================================================================*
|
|   typedef struct {} RESTARTFILE_HEADER;
|
*==========================================================================*/
typedef struct{
    tchar szThis[STRLEN_SESSIONNAME+1];
    tchar szRestart[STRLEN_SESSIONNAME+1];
    tchar szFrom[STRLEN_SESSIONNAME+1];

    tchar szDummy[128];
} RESTARTFILE_HEADER;



static int FormatClusSession2Name(tchar *szName);

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      error
*
* @brief     Transforms the parameters from string to array.
*
*//*=========================================================================*/
static int
StringToArg(tchar *szOptions, int *argc, tchar **argv[])
{
    int     iCount = 0;
    tchar   **szA = NULL;
    tchar   *szOneOpt;
    tchar   *pOpt = szOptions, *pOpt1;
    int     lQuote = 0;

    /* add program name */
    szA = (tchar **) REALLOC( szA, (++iCount) * sizeof(tchar *) );
    szA[iCount-1] = _T("JUNK");

    while ( (pOpt) && (*pOpt) )
    {
        lQuote = 0;

        while ( (*pOpt) && ( (*pOpt) == _T(' ')) ) pOpt++;
        if (!(*pOpt)) break;

        if ( (*pOpt) == _T('\"') )
        {
            if (!(*(++pOpt))) break;
            lQuote = 1;
        }

        if (lQuote)
            pOpt1 = strchr(pOpt , _T('\"'));
        else
            pOpt1 = strchr(pOpt , _T(' '));

        if (pOpt1)
            szOneOpt = StrNNewCopy(pOpt, (pOpt1++) - pOpt);
        else
            szOneOpt = StrNewCopy(pOpt);

        szA = (tchar **) REALLOC( szA, (++iCount) * sizeof(tchar *) );
        szA[iCount-1] = szOneOpt;

        pOpt = pOpt1;
    }

    *argc = iCount;
    *argv = szA;

    return(0);
}

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      invalid parameter
*
* @brief     Initializes the data for the restart session.
*
*//*=========================================================================*/

int InitRestartData(RESTART_DATA *rdRestartData)
{
    memset(rdRestartData, 0, sizeof(RESTART_DATA));

    CmnVecInit(&(rdRestartData->cvFailedObjects), 0, NULL);
    rdRestartData->szWorklist = MALLOC(STRLEN_PATH+1);
    rdRestartData->szOptions = MALLOC(MAX_STRING_SIZE+1);

    rdRestartData->lInitialized = 1;

    return(0);
}

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      invalid parameter
*
* @brief     Initializes the data for the restart session.
*
*//*=========================================================================*/

int FreeRestartData(RESTART_DATA *rdRestartData)
{
    tchar *szWl;

    if (!rdRestartData->lInitialized)
        return(0);

    CmnVecFree(&(rdRestartData->cvFailedObjects));
    FREE(rdRestartData->szOptions);

    /* szWorklist is cleared by bsm */
    szWl = rdRestartData->szWorklist;

    /* clear all */
    memset(rdRestartData, 0, sizeof(RESTART_DATA));

    /* szWorklist is cleared by bsm */
    rdRestartData->szWorklist = szWl;

    return(0);
}

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      invalid parameter
*
* @brief     Formats the session name to a valid file name.
*            The function replaces all slashes (/) with minuses (-) and
*            all spaces ( ) with underlines (_).
*            The session name is converted from:
*            1998/01/01 0003 to
*            1998-01-01_0003
*
*//*=========================================================================*/

static int FormatClusSession2Name(tchar *szName)
{
    if (!szName) return(-1);

    while (*szName)
    {
        switch (*szName)
        {
            case _T(' '):
                *szName=_T('_');
                break;
            case _T('/'):
                *szName=_T('-');
                break;
        }

        szName++;
    }

    return(0);
}

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      error removing session
*
* @brief     Removes the current session from the running sessions directory list.
*
*//*=========================================================================*/

int RemoveClusSession(tchar *szOrigSession)
{
    ERH_FUNCTION (_T("RemoveClusSession"));
    FILEHANDLE          fhSession;
    tchar               szSession[STRLEN_PATH+1];
    tchar               szSessionName[STRLEN_PATH+1];
    RESTARTFILE_HEADER  rfHeader;

    DbgPlain(DBG_SMUTILS, _T("%s('%s') entered."), __FCN__, szOrigSession);

    /* Check whether the clustering is enabled (cmnlib) */
    if (!cmnClus)
    {
        DbgPlain(DBG_SMUTILS, _T("\tClustering not enabled => exiting"));
        return(0);
    }

    if (IS_SESSION_PREVIEW)
    {
        DbgPlain (DBG_SMUTILS, _T("\tSession is in preview mode => exiting"));
        return(0);
    }

    StrCopy(szSessionName, szOrigSession, STRLEN_PATH);
    FormatClusSession2Name(szSessionName);
    CreatePathFromParams(szSession, _T("%s/%s"), cmnPanSessions, szSessionName);

    DbgPlain(DBG_SMUTILS, _T("\tOpening session file '%s'..."), szSession);
    fhSession = OB2_OpenFile(szSession, 0, 0);
    if (fhSession == INVALID_HANDLE_VALUE)
    {
        DbgPlain(DBG_SMUTILS, _T("\tERROR!"));
        return(0);
    }
    DbgPlain(DBG_SMUTILS, _T("\tOK"));


    /* wait for 5 seconds before removing,            */
    /* failoverscript my not detect services down yet */
    /* it checks (currently) every 3 seconds          */
    /* needed for ServiceGuard (unix) only            */
    sleep(5);


    memset(&rfHeader, 0, sizeof(rfHeader));
    OB2_ReadFile(fhSession, &rfHeader, sizeof(rfHeader));

    OB2_CloseFile(fhSession);

    /* recursive delete */
    if (strcmp(rfHeader.szFrom, _T(""))!=0)
    {
        RemoveClusSession(rfHeader.szFrom);
    }

    DbgPlain(DBG_SMUTILS, _T("\tDeleting session file '%s'..."), szSession);
    OB2_DeleteFile(szSession);

    DbgPlain(DBG_SMUTILS, _T("%s() exiting"), __FCN__);
    return(0);
}

/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      error adding session
*
* @brief     Adds the current session in the running sessions directory list.
*
*//*=========================================================================*/

int AddClusSession(void)
{
    ERH_FUNCTION (_T("AddClusSession"));
    FILEHANDLE          fhSession;
    tchar               szSession[STRLEN_PATH+1];
    tchar               szSessionName[STRLEN_PATH+1];
    RESTARTFILE_HEADER  rfHeader;

    DbgPlain(DBG_SMUTILS, _T("%s() entered"), __FCN__);

    /* Check whether the clustering is enabled (cmnlib) */
    if (!cmnClus)
    {
        DbgPlain(DBG_SMUTILS, _T("\tClustering not enabled => exiting"));
        return(0);
    }

    /* Check whether the session has the clus_restart option. */
    if (ssm.wl.clus_restart == _CLUS_RESTART_NO)
    {
        DbgPlain(DBG_SMUTILS, _T("\tclus_restart not specified => exiting"));
        return(0);
    }

    if (IS_SESSION_PREVIEW)
    {
        DbgPlain (DBG_SMUTILS, _T("\tSession is in preview mode => exiting"));
        return(0);
    }

    DbgPlain(DBG_SMUTILS, _T("\tSession ID   = %s "), ss->ses.sessionName);
    DbgPlain(DBG_SMUTILS, _T("\tSession From = %s "),
                          (IS_SESSION_RESTARTED ? 
                           SESSION_TO_RESTART : _T("no restart")) );

    /* DbgPlain(DBG_SMUTILS, _T("\tClustering   = %d "), cmnClusteringEnabled); */

    sprintf(szSessionName, _T("%s"), ss->ses.sessionName);
    FormatClusSession2Name(szSessionName);

    CreatePathFromParams(szSession, _T("%s/%s"), cmnPanSessions, szSessionName);

    DbgPlain(DBG_SMUTILS, _T("\tCreating session file '%s'..."), szSession);
    fhSession = OB2_OpenFile(szSession, 1, 1);
    if (fhSession == INVALID_HANDLE_VALUE)
    {
        DbgPlain(DBG_SMUTILS, _T("\t\tERROR creating session file => exiting!"));
        return(-1);
    }

    memset(&rfHeader, 0, sizeof(rfHeader));

    StrCopy(rfHeader.szThis, ss->ses.sessionName, STRLEN_SESSIONNAME);
    if (IS_SESSION_RESTARTED)
    {
        DbgPlain(DBG_SMUTILS, _T("\tWriting session From..."));
        StrCopy(rfHeader.szFrom, SESSION_TO_RESTART, STRLEN_SESSIONNAME);
    }

    DbgPlain(DBG_SMUTILS, _T("\tWriting session data..."));
    OB2_WriteFile(fhSession, &rfHeader, sizeof(rfHeader));

    DbgPlain(DBG_SMUTILS, _T("\tClosing file..."));
    OB2_CloseFile(fhSession);

    if (IS_SESSION_RESTARTED)
    {
        sprintf(szSessionName, _T("%s"), SESSION_TO_RESTART);
        FormatClusSession2Name(szSessionName);

        CreatePathFromParams(szSession, _T("%s/%s"), cmnPanSessions, szSessionName);

        DbgPlain(DBG_SMUTILS, _T("\tReopening session file '%s'..."), szSession);
        fhSession = OB2_OpenFile(szSession, 0, 0);
        if (fhSession == INVALID_HANDLE_VALUE)
        {
            DbgPlain (DBG_SMUTILS, _T("ERROR reopening session file!"));
            return(-1);
        }

        DbgPlain(DBG_SMUTILS, _T("\tRetrieving session data..."));
        memset(&rfHeader, 0, sizeof(rfHeader));
        OB2_ReadFile(fhSession, &rfHeader, sizeof(rfHeader));

        OB2_CloseFile(fhSession);

        DbgPlain(DBG_SMUTILS, _T("\tRewriting session file '%s'..."), szSession);
        fhSession = OB2_OpenFile(szSession, 1, 1);
        if (fhSession == INVALID_HANDLE_VALUE)
        {
            DbgPlain (DBG_SMUTILS, _T("ERROR reopening session file!"));
            return(-1);
        }

        StrCopy(rfHeader.szRestart, ss->ses.sessionName, STRLEN_SESSIONNAME);

        DbgPlain(DBG_SMUTILS, _T("\tWriting session data..."));
        OB2_WriteFile(fhSession, &rfHeader, sizeof(rfHeader));

        DbgPlain(DBG_SMUTILS, _T("\tClosing file..."));
        OB2_CloseFile(fhSession);
    }

    DbgPlain(DBG_SMUTILS, _T("%s() exiting"), __FCN__);
    return(0);
}

static CmnVec *
FailedObjectsListPtr(void)
{
    return &ss->rdRestart.failedObjects;
}

static int
RestartBackup_IsRemovalAllowed()
{
    if (!(IS_SESSION_BACKUP && (IS_SESSION_RESTARTED || IS_SESSION_RESUMED_FSFAILEDOBJSONLY)))
    {
        DbgPlain(DBG_SMUTILS, _T("not restart or resume of backup session => not allowed\n"));
        return 0;
    }

    if (ssm.wl.clus_restart == _CLUS_RESTART_ALL)
    {
        DbgPlain(DBG_SMUTILS, _T("CLUS_RESTART=ALL => not allowed\n"));
        return 0;
    }

    if (IS_SESSION_BAR)
    {
        DbgPlain(DBG_SMUTILS, _T("bar restart => not allowed\n"));
        return 0;
    }

    return 1;
}

/*========================================================================*//**
*
* @retval    0 - no
* @retval    1 - yes
*
* @brief     Checks whether the object can be removed from the list (if it was
*            completed).
*
*//*=========================================================================*/
int
RestartBackup_CanRemoveObject(WlObject *obj)
{
    ERH_FUNCTION(_T("Restart_BackupCanRemoveObject"));

    int      idx;
    tchar    objectName[STRLEN_OBJECTNAME + 1] = {0};
    dptime_t now = CmnGetUnixTime();

    StrToObjectName(obj->type, obj->host, obj->mtpoint, obj->label, objectName);

    DbgFcnInEx(__FLN__, DBG_SMUTILS, _T("Object:%s"), objectName);

    /*****************************************************
        checks
    ******************************************************/
    if (!RestartBackup_IsRemovalAllowed())
    {
        RETURN_INT(0);
    }

    /* check type of input object */
    if ( 
            (obj->type != wl_rawdisk) &&
            (obj->type != wl_filesystem) &&
            (obj->type != wl_db) &&
            (obj->type != wl_vbfs) &&
            (obj->type != wl_client) &&
            (obj->type != wl_winfs) &&
            (obj->type != wl_netware)
        )
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] Object type mismatch => cannot remove"), __FCN__);

        RETURN_INT(0);
    }


    /* if no failed objects, just return yes */
    if (CmnVecH(FailedObjectsListPtr()) < 1)
    {
        if (IS_SESSION_RESUMED_FSFAILEDOBJSONLY)
        {
            /* continue anyway if filesystem session was resumed,
               just in case we couldn't get failed objects from the session */
            RETURN_INT(0);
        }
        DbgPlain (DBG_SMUTILS, _T("[%s] No failed objects => can remove"), __FCN__);
        RETURN_INT(1);
    }

    /* compose input object name */

    for (idx = 0; idx < CmnVecH(FailedObjectsListPtr()); idx++)
    {
        CtOVerRec *over = (CtOVerRec*)CmnVecI(FailedObjectsListPtr(), idx);

        tchar *failedObjectName = over->objectName;

        if (strcmp(objectName, failedObjectName) == 0)
        {
            if (IsOVerProtected(over) && (glResumeMatchProtection == 1))
            {
                DbgPlain(DBG_SMUTILS, _T("[%s] Matching protection of object to failed version..."), __FCN__);

                bsm_protectionOVerToWl(
                    &obj->options.protection, &obj->options.protectionParam,
                    over->protType, over->protTime,
                    now - over->endTime
                );

                bsm_protectionOVerToWl(
                    &obj->options.catProtection, &obj->options.catProtectionParam,
                    over->catProtType, over->catProtTime,
                    now - over->endTime
                );
            }

            // XXX: can't selectively skip unprotecteded objects here because it other places 
            // have assumption that all failed objects will be resumed.
            
            DbgPlain(DBG_SMUTILS, _T("[%s] Match found => cannot remove"), __FCN__);
            RETURN_INT(0);
        }
    }

    DbgPlain (DBG_SMUTILS, _T("[%s] No match found => can remove"), __FCN__);
    RETURN_INT(1);
}

/*========================================================================*//**
*
* @retval    0 - no
* @retval    1 - yes
*
* @brief     Checks whether the object can be removed from the list (if it was
*            completed).
*
*//*=========================================================================*/
int
RestartBackup_RemoveObjects(void)
{
    ERH_FUNCTION (_T("RestartBackup_RemoveObjects"));

    int         idx;
    WlObject    *obj;
    int         ret = OB2RET_OKAY;


    OB2_ENTRY_LEVEL(DBG_SMUTILS);


    if (!RestartBackup_IsRemovalAllowed())
    {
        OB2_EXIT(true, OB2RET_OKAY, ; );
    }

    /* remove failed objects */
    for (idx = 0; idx < CmnVecH(&(ssm.wl.objects)); idx++)
	{
		obj = (WlObject *)CmnVecI(&(ssm.wl.objects), idx);
		
        /* remove this object? */
        if (RestartBackup_CanRemoveObject(obj))
        {
            DbgPlain(DBG_SMUTILS, _T("\tmatch found @ %d => object removed"), idx);
            CmnVecRemove(&(ssm.wl.objects), idx);
            idx = -1;
        }
	}

    /*****************************************************
        exit & cleanup
    ******************************************************/
    exit:
    DbgPlain(DBG_SMUTILS, _T("Freeing restart data\n"));
    CmnVecFree(FailedObjectsListPtr());

    OB2RET_TO_DBGS_LEVEL(DBG_SMUTILS);

    return (ret);
}

/*========================================================================*//**
*
* @param     sessionType     type of restarted session (COPY | BACKUP)
* @param     sessionID       which session to restart
*
* @brief     Sets some variables that mark restart of session.
*
*//*=========================================================================*/
void
SignalRestartSession(int sessionType, tchar *sessionID)
{
    ss->ses.sessionType = sessionType;
    ss->rdRestart.restart = 1;
    ssm.interactive = 0;

    SetRestartResumeSessionID(sessionID);
}

/*========================================================================*//**
*
* @param     sessionType     type of resumed session (BACKUP)
* @param     sessionID       which session to resume
*
* @brief     Sets some variables that mark resume of session.
*
*//*=========================================================================*/
void
SignalResumeSession(int sessionType, tchar *sessionID)
{
    ss->ses.sessionType = sessionType;
    ss->rdRestart.resume = 1;
    ssm.interactive = 0;

    SetRestartResumeSessionID(sessionID);
}

/*========================================================================*//**
*
* @param     sessionID
*
* @brief     Sets session ID, which can be in internal DP format or in user format.
*
*//*=========================================================================*/
void
SetRestartResumeSessionID(tchar *sessionID)
{
    tchar *internalSessionID = NULL;

    /* do we already have internal session id ? */
    if (strchr(sessionID, _T(' ')) != NULL)
    {
        internalSessionID = sessionID;
    }
    else
    {
        internalSessionID = StrFromUserSessionId(sessionID);
        if (internalSessionID == NULL)
        {
            internalSessionID = sessionID;
        }
    }

    StrCopy(ss->rdRestart.session, internalSessionID , STRLEN_SESSIONNAME);
}

static void
ClusterLogFailedObjects(
    tchar   *specName, 
    tchar   *options, 
    CmnVec  *failedObjects
)
{
    if (cmnClus)
    {
        int     idx;

        DbgLogFull(
            __FLNPT__, 
            S_CLUSTER_FILE, 
            _T("Backup specification '%s' with options '%s' will be restarted. Failed objects found are:\n"),
            specName,
            (options != NULL) ? options : _T("(no options specified)")
        );

        for (idx = 0; idx < CmnVecH(failedObjects); idx++)
        {
            CtOVerRec *over = (CtOVerRec*)CmnVecI(failedObjects, idx);
            DbgLogPlain(
                S_CLUSTER_FILE, 
                _T("\tFailed object #%d: %s"),
                idx,
                NSD(over->objectName)
            );
        }
    }
}

static void
ParseBackupSessionOptions(tchar *options)
{
    if (IS_SESSION_BACKUP && options)
    {
        int     argc, i;
        tchar   **argv;

        DbgPlain(DBG_SMUTILS, _T("\tParsing session options string: '%s'\n"), options);

        StringToArg(options, &argc, &argv);
        for (i = 0; i < argc; i++)
        {
            DbgPlain(DBG_SMUTILS, _T("\t\t#%d = '%s'"), i, argv[i]);
        }
        
        DbgPlain(DBG_SMUTILS, _T("\tStarting parser..."));
        ParseOptions(argc, argv);
        DbgPlain(DBG_SMUTILS, _T("\t\tdone!"));
    }
}

/*========================================================================*//**
*
* @param     specNameAndType     spec name and type in the following format: 'TYPE NAME'
*
* @retval    1 - is barlist
* @retval    0 - otherwise
*
* @brief     Checks if specified spec is barlist based on type that is part of spec name.
*
*//*=========================================================================*/
static int
IsBarlist(tchar *specNameAndType)
{
    tchar   *p = NULL;
    int      i;

    if ((p = strchr(specNameAndType, _T(' '))) != NULL)
    {
        size_t specTypeLen = p - specNameAndType;
        const OB2IntegType *integ;
        for (i = 0; integ = cmnIntegs[i], integ != NULL; ++i)
        {
            /* check if spec type is in list of supported integrations */
            if (strncmp(integ->type, specNameAndType, specTypeLen) == 0
                && integ->groupDl != G_WORKLIST)
            {
                DbgPlain(DBG_SMUTILS, _T("BAR mode: %s"), integ->type);
                ssm.barapptype = StrNewCopy(integ->type);
                return 1;
            }
        }
    }

    return 0;
}

/*========================================================================*//**
*
* @param     specNameAndType     spec name and type in the following format: 'TYPE NAME'
*
* @retval    barlist name or NULL
*
*//*=========================================================================*/
static tchar *
ExtractBarlistName(tchar *specNameAndType)
{
    tchar *p = strchr(specNameAndType, _T(' '));

    if (p != NULL)
    {
        /* skip type */
        return (p + 1);
    }

    return NULL;
}

static void
OverrideBackupSessionData(tchar *specNameAndType, int backupType)
{
    if (IsBarlist(specNameAndType))
    {
        ss->sessmode |= SESSION_BAR;
        ssm.barstart = 1;
        sm.U.type = BAR;
        ssm.wl.name = StrNewCopy(ExtractBarlistName(specNameAndType));
    }
    else
    {
        ss->sessmode = BACKUP;
        ssm.wl.name = StrNewCopy(specNameAndType);
        sm.U.type = BACKUP;
    }

	ss->spec.bsm.mode  = backupType;
    ss->ses.backupType = backupType;
    ssm.interactive    = 0;

    /* override specification name */
    FREE(ssm.wldata);
    ssm.wldata = StrNewCopy(specNameAndType);
}

static int
IsRestartableVEPASession(tchar *specNameAndType)
{
    ERH_FUNCTION (_T("IsRestartableVEPASession"));

    tchar *readBuf = NULL;
    tchar *readBufsub = NULL;
    int ret = OB2RET_ERROR;
    int csahandle=0;

    csaEmbedded = 1;
    if (!csaInitialized)
    {
        if (MCsaInit(cmnHostname) == -1)
        {
            return ret;
        }
    }
    else
    {
        if ((csahandle=MCsaBindServer(cmnHostname)) == -1)
        {
            return ret;
        }
    }

    if((MCsaGetGroupItem(csahandle, G_VEPALIST, ExtractBarlistName(specNameAndType), &readBuf, READ_ONLY) == 0) && (readBuf != NULL) )
    {
        if((readBufsub = strstr(readBuf, VEPA_CI_APPSERVER_ENVIRONMENT)) != NULL)
        {
            if(strstr(readBufsub, VEPA_CI_APPSERVER_SUBTYPE_VMWARE) != NULL)
            {
                ret = OB2RET_OKAY;
            }
        }
    }

    /*****************************************************
        exit
    ******************************************************/
    if(ret == OB2RET_OKAY)
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] restart of backup session allowed\n"), __FCN__);
    }
    else
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] restart not allowed/supported\n"), __FCN__);
    }
    CsaExit();
    return (ret);
}

#if UNUSED_CODE
/*========================================================================*//**
*
* @param     specNameAndType     spec name and type in the following format: 'TYPE NAME'
*
* @retval    OB2RET_OKAY     session can be resumed
* @retval    OB2RET_ERROR    otherwise
*
*//*=========================================================================*/
static int
CheckIfSessionCanBeResumed(tchar *specNameAndType)
{
    ERH_FUNCTION (_T("CheckIfSessionCanBeResumed"));

    if (!IS_SESSION_RESUMED)
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] resume not signaled\n"), __FCN__);
        return OB2RET_ERROR;
    }

    if (IS_SESSION_BACKUP)
    {
        if (strncmp(specNameAndType, OB2_APP_ORACLE8, strlen(OB2_APP_ORACLE8)) == 0)
        {
            DbgPlain(DBG_SMUTILS, _T("[%s] resume of Oracle backup session allowed\n"), __FCN__);
            return OB2RET_OKAY;
        }
        if(IS_SESSION_RESUMED_FSFAILEDOBJSONLY)
        {
            DbgPlain(DBG_SMUTILS, _T("[%s] resume of file system backup session allowed\n"), __FCN__);
            return OB2RET_OKAY;
        }
    }

    DbgPlain(DBG_SMUTILS, _T("[%s] resume not supported\n"), __FCN__);
    return OB2RET_ERROR;
}
#endif // UNUSED_CODE


/*========================================================================*//**
*
* @param     specNameAndType     spec name and type in the following format: 'TYPE NAME'
*
* @retval    OB2RET_OKAY     session can be restarted
* @retval    OB2RET_ERROR    otherwise
*
*//*=========================================================================*/
static int
CheckIfSessionCanBeRestarted(tchar *specNameAndType)
{
    ERH_FUNCTION (_T("CheckIfSessionCanBeRestarted"));

    if (!IS_SESSION_RESTARTED)
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] restart not signaled\n"), __FCN__);
        return OB2RET_ERROR;
    }

    /* restart of object copy is supported */
    if (IS_SESSION_COPY)
    {
        DbgPlain(DBG_SMUTILS, _T("[%s] restart of copy session allowed\n"), __FCN__);
        return OB2RET_OKAY;
    }

    if (IS_SESSION_BACKUP)
    {
        /* Enable "Restart Failed Objects" option only for VEPA VMWare sessions */
        if (strncmp(specNameAndType, OB2_APP_VEPA, strlen(OB2_APP_VEPA)) == 0)
        {
            return IsRestartableVEPASession(specNameAndType);
        }
        DbgPlain(DBG_SMUTILS, _T("[%s] restart of backup session allowed\n"), __FCN__);
        return OB2RET_OKAY;
    }

    DbgPlain(DBG_SMUTILS, _T("[%s] restart not allowed/supported\n"), __FCN__);
    return OB2RET_ERROR;
}

/*========================================================================*//**
*
* @param     None.
*
* @retval    OB2RET_OKAY     if everything Ok
* @retval    OB2RET_ERROR    otherwise
*
* @brief     Retrieves list of all failed objects. Only failed objects are
*            resumed.
*
*//*=========================================================================*/
int
ResumeBackup_GetFailedObjects(void)
{
    ERH_FUNCTION (_T("ResumeBackup_GetFailedObjects"));

    tchar   *specName = NULL,
            *options = NULL;
    int32   backupType = 0;
    int     ret = OB2RET_OKAY;
    int     r = 0;


    OB2_ENTRY_LEVEL(DBG_SMUTILS);

    specName = (tchar *) MALLOC(STRLEN_PATH + 1);
    OB2_EXIT(specName == NULL, OB2RET_ERROR, ;);

    options = (tchar *) MALLOC(MAX_STRING_SIZE + 1);
    OB2_EXIT(options == NULL, OB2RET_ERROR, ;);

    memset(specName, 0, (STRLEN_PATH + 1));
    memset(options, 0, (MAX_STRING_SIZE + 1));

    /*****************************************************
        Get failed objects
    *****************************************************/
    r = DbaGetFailedObjects(
            ss->dbGl,                   /* [in]  db globals */
            SESSION_TO_RESUME,          /* [in]  session name specified for resume */
            specName,                   /* [out] worklist */
            options,                    /* [out] options */
            &backupType,                /* [out] backup type (full/incr) */
            FailedObjectsListPtr()      /* [out] list of failed objects */
        );

    if (r < 0)
    {
        /* Cannot get list of failed objects. Just continue, all objects will be resumed */
        CmnVecFree(FailedObjectsListPtr());
    }

exit:
    FREE(specName);
    FREE(options);

    OB2RET_TO_DBGS_LEVEL(DBG_SMUTILS);

    return (ret);
}

/*========================================================================*//**
*
* @param     None.
*
* @retval    OB2RET_OKAY     if everything Ok
* @retval    OB2RET_ERROR    otherwise
*
* @brief     In case of restart of backup session, it overrides current session
*            data with data from original session. It also retrieves list of all
*            failed objects from original session.
*
*//*=========================================================================*/
int
RestartBackup_OverrideSessionData(void)
{
    ERH_FUNCTION (_T("RestartBackup_OverrideSessionData"));

    tchar   *specName = NULL,
            *options = NULL;
    int32   backupType;
    int     ret = OB2RET_OKAY;
    int     r;


    OB2_ENTRY_LEVEL(DBG_SMUTILS);


    /*****************************************************
        init
    ******************************************************/
    if (!(IS_SESSION_BACKUP && IS_SESSION_RESTARTED))
    {
        OB2_EXIT(true, OB2RET_OKAY, ; );
    }

    specName = (tchar *) MALLOC(STRLEN_PATH + 1);
    OB2_EXIT(specName == NULL, OB2RET_ERROR, ;);

    options = (tchar *) MALLOC(MAX_STRING_SIZE + 1);
    OB2_EXIT(options == NULL, OB2RET_ERROR, ;);

    memset(specName, 0, (STRLEN_PATH + 1));
    memset(options, 0, (MAX_STRING_SIZE + 1));

    ReportRestartSession();


    /*****************************************************
        override session data
    *****************************************************/
    r = DbaGetFailedObjects(
            ss->dbGl,                   /* [in]  db globals */
            SESSION_TO_RESTART,         /* [in]  restart session name specified command line */
            specName,                   /* [out] worklist */
            options,                    /* [out] options */
            &backupType,                /* [out] backup type (full/incr) */
            FailedObjectsListPtr()      /* [out] list of failed objects */
        );

    OB2_EXIT(
        r < 0, 
        OB2RET_ERROR,
        {
            ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage(NLS_SET_PANBSM, EBSM_RESTART_DBAGET));
        }
    );

    DbgPlain(
        DBG_SMUTILS, 
        _T("DbaGetFailedObjects() OK!\n\t")
        _T("worklist = '%s'\n\t")
        _T("options  = '%s'\n\t")
        _T("type     = %d\n\t")
        _T("fobjects = %d\n\t"),
        specName,
        options,
        backupType,
        CmnVecH(FailedObjectsListPtr())
    );

    ClusterLogFailedObjects(specName, options, FailedObjectsListPtr());
    ParseBackupSessionOptions(options);

    OverrideBackupSessionData(specName, backupType);

    r = CheckIfSessionCanBeRestarted(specName);
    OB2_EXIT(
        r < 0, 
        OB2RET_ERROR,
        {
            ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage(NLS_SET_PANBSM, EBSM_RESTART_NOTSUPPORTED));
        }
    );

    /*****************************************************
        exit & cleanup
    ******************************************************/
    exit:
    FREE(specName);
    FREE(options);

    OB2RET_TO_DBGS_LEVEL(DBG_SMUTILS);

    return (ret);
}

/*========================================================================*//**
*
* @param     None.
*
* @retval    OB2RET_OKAY     if everything Ok
* @retval    OB2RET_ERROR    otherwise
*
* @brief     In case of restart of copy session, it overrides spec name, session type
*            and subtype with data from original session.
*
*//*=========================================================================*/
int
RestartCopy_OverrideSessionData(void)
{
    ERH_FUNCTION (_T("RestartCopy_OverrideSessionData"));

    CtSesRec    sesData;
    int         ret = OB2RET_OKAY;
    int         r;


    OB2_ENTRY_LEVEL(DBG_SMUTILS);


    /*****************************************************
        init
    ******************************************************/
    CtRecInit(ses, &sesData);


    /*****************************************************
        check if restart is possible and override
        session data with the data from original
    ******************************************************/
    if (IS_SESSION_COPY && IS_SESSION_RESTARTED)
    {
        /*****************************************************
            get session info
        ******************************************************/
        r = DbaFindSession(ss->dbGl, SESSION_TO_RESTART, &sesData);

        OB2_EXIT(
            r < 0, 
            OB2RET_ERROR,
            {
                ErhFullReport(
                    __FCN__,
                    ERH_MAJOR,
                    NlsGetMessage(NLS_SET_CSM, ECSM_SESSION_NOT_FOUND, StrToUserSessionId(SESSION_TO_RESTART))
                );
            }
        );

        DbgPlain(
            DBG_SMUTILS,
            _T("\tSession type: %d\n")
            _T("\tSession flags: %u\n"),
            sesData.sessionType,
            sesData.flags
        );


        /*****************************************************
            override session type
        ******************************************************/
        if (sesData.sessionType != COPY)
        {
            DbgPlain(DBG_SMUTILS, _T("[%s] not copy session: %s\n"), __FCN__, SESSION_TO_RESTART);

            OB2_EXIT(
                true,
                OB2RET_ERROR,
                {
                    ErhFullReport(
                        __FCN__,
                        ERH_MAJOR,
                        NlsGetMessage(NLS_SET_CSM, ECSM_COPYRESTART_DENIED, StrToUserSessionId(SESSION_TO_RESTART))
                    );
                }
            );
        }


        /*****************************************************
            override session subtype & flags
        ******************************************************/
        if (sesData.flags & SESF_SCHEDULED)
        {
            ss->sessionSubtype = SESSION_SUBTYPE_SCHEDULED;
            ss->ses.flags |= SESF_SCHEDULED;

        }
        else if(sesData.flags & SESF_POSTBACKUP)
        {
            ss->sessionSubtype = SESSION_SUBTYPE_POSTBACKUP;
            ss->ses.flags |= SESF_POSTBACKUP;
        }
        else
        {
            DbgPlain(DBG_SMUTILS, _T("[%s] not scheduled or postbackup copy session: %s\n"), __FCN__, SESSION_TO_RESTART);

            OB2_EXIT(
                true,
                OB2RET_ERROR,
                {
                    ErhFullReport(
                        __FCN__,
                        ERH_MAJOR,
                        NlsGetMessage(NLS_SET_CSM, ECSM_COPYRESTART_DENIED, StrToUserSessionId(SESSION_TO_RESTART))
                    );
                }
            );
        }

        /* override specification name */
        FREE(ssm.wldata);
        ssm.wldata = StrNewCopy(sesData.datalist.str);
    }


    /*****************************************************
        exit & cleanup
    ******************************************************/
    exit:
    CtRecFree(ses, &sesData);

    OB2RET_TO_DBGS_LEVEL(DBG_SMUTILS);

    return (ret);
}

/*========================================================================*//**
*
* @param     None.
*
* @brief     Sends message to monitor that session is being restarted or resumed.
*            In cluster mode, it also writes note of restart to cluster log file.
*
*//*=========================================================================*/
void
ReportRestartSession(void)
{
    ERH_FUNCTION (_T("ReportRestartSession"));

    if (IS_SESSION_RESTARTED || IS_SESSION_RESUMED_FSFAILEDOBJSONLY)
    {
        tchar *userSessionId = StrToUserSessionId(SESSION_TO_RESTART);

        if (userSessionId == NULL)
        {
            userSessionId = SESSION_TO_RESTART;
        }

        if (IS_SESSION_RESTARTED)
        {
            ErhFullReport(
                __FCN__,
                ERH_NORMAL,
                NlsGetMessage(
                    NLS_SET_PANBSM, 
                    EBSM_RESTART_START, 
                    userSessionId
                )
            );
        }
        else
        {
            ErhFullReport(
                __FCN__,
                ERH_NORMAL,
                NlsGetMessage(
                    NLS_SET_PANRSM,
                    ERSM_CKP_RESUMINGSESSION,
                    userSessionId
                )
            );
        }

        /* if this is restart of backup when cluster fails
           over then report it to cluster log file */
        if (IS_SESSION_BACKUP && cmnClus)
        {
            DbgLogFull(
                __FLNPT__, 
                S_CLUSTER_FILE, 
                _T("Session '%s' is being restarted\n"), 
                SESSION_TO_RESTART
            );
        }
    }
}



int
SmPutUpdateRestart (tchar *skey, SmUrec U)
{
    int sid=atoi(skey);

    int ret = SmPutUpdate(skey, U);

    if (ret == 0 && ss->rdRestart.lRestart)
    {
        StrCopy(smShm->t[sid].datalist,U.datalist,STRLEN_FILE);
    }

    if (!ss->rdRestart.lRestart)
    {
        return SmPutUpdate( skey, U );
    }

    if ((sid <=0 ) || (sid >= MAX_SM_COUNT)) return 0;

    if (smShm == NULL)
    {
        if (SmCreateTable(0) == -1) return -1;
    }

    smShm->t[sid].noDa=U.noDa;
    smShm->t[sid].noMa=U.noMa;
    StrCopy(smShm->t[sid].sessionID,U.sessionID,STRLEN_KID);
    smShm->t[sid].status=U.status;
    StrCopy(smShm->t[sid].eun,U.eun,STRLEN_USER);
    StrCopy(smShm->t[sid].egn,U.egn,STRLEN_GROUP);
    StrCopy(smShm->t[sid].hostname,U.hostname,STRLEN_HOSTNAME);

    StrCopy(smShm->t[sid].datalist,U.datalist,STRLEN_FILE);

    smShm->t[sid].type = U.type;

    return 0;
}

#if 0
/*========================================================================*//**
*
* @retval    0       ok
* @retval    -1      error overriding
*
* @brief     Overrides the worklist name with the one specified in the existing
*            session in the database.
*
*//*=========================================================================*/

int RestartOverrideWorklist(void)
{
    ERH_FUNCTION (_T("RestartOverrideWorklist"));
    int     i, iRet = 0;
    tchar   *p, *userSession;

    DbgPlain(DBG_SMUTILS, _T("%s() entered"), __FCN__);

    /* if there is no -restart override just exit */
    if (!ss->rdRestart.lRestart)
    {
        DbgPlain(DBG_SMUTILS, _T("!bRestart => exiting %s()\n"), __FCN__);
        return(0);
    }
    DbgPlain (DBG_SMUTILS, _T("bRestart => Overriding worklist data\n"));



    userSession = StrToUserSessionId (ss->rdRestart.szSession);
    ErhFullReport (__FCN__, ERH_NORMAL,
        NlsGetMessage (NLS_SET_PANBSM, EBSM_RESTART_START,
            userSession == NULL? ss->rdRestart.szSession : userSession));


    if ( cmnClus )
    {
        DbgLogFull( __FLNPT__, S_CLUSTER_FILE, _T("Session '%s' is being restarted\n"), ss->rdRestart.szSession );
    }

    /* initialize restart session data */
    InitRestartData(&rdRestartData);

    /* query the db for the session */
    DbgPlain (DBG_SMUTILS, _T("Invoking DbaGetFailedObjects()"));
    iRet = DbaGetFailedObjects(
             ss->dbGl,                        /* [in]  db globals */
             ss->rdRestart.szSession,         /* [in]  restart session name specified command line */
             rdRestartData.szWorklist,        /* [out] worklist */
             rdRestartData.szOptions,         /* [out] options */
             &(rdRestartData.lMode),          /* [out] backup type (full/incr) */
             &(rdRestartData.cvFailedObjects) /* [out] list of failed objects */
            );

    if (iRet < 0)
    {
        /* got error => abort session */
        DbgPlain (DBG_SMUTILS, _T("Got error (#%d)!\n"), iRet);

        ErhFullReport(__FCN__, ERH_MAJOR,
                  NlsGetMessage(NLS_SET_PANBSM, EBSM_RESTART_DBAGET));

        return(-1);
    }
    DbgPlain (DBG_SMUTILS,
              _T("DbaGetFailedObjects() OK!\n\t")
              _T("worklist \t= '%s'\n\t")
              _T("options  \t= '%s'\n\t")
              _T("type     \t= %d\n\t")
              _T("fobjects \t= %d\n\t"),
              rdRestartData.szWorklist,
              rdRestartData.szOptions,
              rdRestartData.lMode,
              CmnVecH(&(rdRestartData.cvFailedObjects))
             );

    if ( cmnClus )
    {
        int     iIndex;
        tchar   *szObjectName;

        DbgLogFull( __FLNPT__, S_CLUSTER_FILE,
                    _T("Backup specification '%s' with options '%s' will be restarted. Failed objects found are:\n"),
                    rdRestartData.szWorklist,
                    rdRestartData.szOptions ? rdRestartData.szOptions : _T("(no options specified)")
        );

        for ( iIndex = 0; iIndex < CmnVecH(&(rdRestartData.cvFailedObjects)); iIndex++ )
        {
            szObjectName = (tchar *)CmnVecI(&(rdRestartData.cvFailedObjects), iIndex);
            DbgLogPlain(S_CLUSTER_FILE,
                        _T("\tFailed object #%d: %s"),
                        iIndex,
                        szObjectName ? szObjectName : _T("(NULL)")
            );
        }
    }

    /* parse the options in szOptions string */
    if (rdRestartData.szOptions)
    {
        int     argc, i;
        tchar   **argv;

        DbgPlain (DBG_SMUTILS, _T("\tParsing szOptions string: '%s'\n"), rdRestartData.szOptions);

        StringToArg(rdRestartData.szOptions, &argc, &argv);
        for (i = 0; i < argc; i++)
            DbgPlain(DBG_SMUTILS, _T("\t\t#%d = '%s'"), i, argv[i]);

        DbgPlain (DBG_SMUTILS, _T("\tStarting parser..."));
        ParseOptions(argc, argv);
        DbgPlain (DBG_SMUTILS, _T("\t\tdone!"));
    }


    rdRestartData.lSessMode = BACKUP;
    ssm.wl.name = rdRestartData.szWorklist;

    /* check if bar mode and oveeride options */
    if ((p=strchr(rdRestartData.szWorklist, _T(' '))) != NULL)
    {
        const OB2IntegType *integ;
        for (i=0; integ = cmnIntegs[i], integ != NULL; i++)
        {
            /* check if dl type is in list of supported integrations */
            if (StrLen(integ->type) == (size_t)(p - rdRestartData.szWorklist)
                && strncmp(integ->type,
                            rdRestartData.szWorklist,
                            p - rdRestartData.szWorklist) == 0
                && integ->groupDl != G_WORKLIST) /* not reg DL */

            {
                DbgPlain(DBG_SMUTILS, _T("BAR mode: %s"), integ->type);
                rdRestartData.lSessMode = SESSION_BAR;
                ssm.wl.name = p+1; /* strip integ type from name */
            }
        }
    }
    DbgPlain (DBG_SMUTILS, _T("Session mode: %d\n"), rdRestartData.lSessMode);


    /* fill the session data */
    ss->sessmode        = rdRestartData.lSessMode;
    if ( ss->sessmode & SESSION_BAR )
    {
        DbgPlain (DBG_SMUTILS, _T("BAR restart => ssm.barstart = 1;\n"));
        ssm.barstart = 1;

        sm.U.type = BAR;
    }
    else
    {
        sm.U.type = BACKUP;
    }

    ss->spec.bsm.mode   = rdRestartData.lMode;
    ss->ses.backupType  = rdRestartData.lMode;
    ssm.interactive     = 0;
    ssm.wldata          = rdRestartData.szWorklist;

    DbgPlain (DBG_SMUTILS, _T("ssm.wl.name = %s\n"), ( ssm.wl.name ? ssm.wl.name : _T("'NULL'")) );

    ErhFullReport(__FCN__, ERH_NORMAL,
                  NlsGetMessage(NLS_SET_PANBSM, EBSM_RESTART_DATA,
                  rdRestartData.szWorklist));

    DbgPlain(DBG_SMUTILS, _T("%s() exiting\n"), __FCN__);
    return(0);
}

/*========================================================================*//**
*
* @retval    0       no
* @retval    1       yes
*
* @brief     Checks whether the object can be removed from the list
*            (if it was completed).
*
*//*=========================================================================*/

int CanRemoveThisObject(WlObject *wloObj)
{
    ERH_FUNCTION (_T("CanRemoveThisObject"));
    int         iIndex;
    tchar       szObjectName1[STRLEN_OBJECTNAME+1];
    tchar       *szObjectName;

    DbgPlain(DBG_SMUTILS, _T("%s(%p)\n"), __FCN__, wloObj);

    DbgPlain(DBG_SMUTILS, _T("\tObject:")
                          _T("\n\t\tType = %d")
                          _T("\n\t\tLab  = '%s'")
                          _T("\n\t\tHost = '%s'")
                          _T("\n\t\tMtpo = '%s'")
                          _T("\n\t\tSess = '%s'"),
                          wloObj->type, NS(wloObj->label),
                          NS(wloObj->host), NS(wloObj->mtpoint), NS(wloObj->session));

    /* if there is no -restart override just exit */
    if (!ss->rdRestart.lRestart)
    {
        DbgPlain(DBG_SMUTILS, _T("\t!bRestart => exiting %s() [false]\n"), __FCN__);
        return 0; /* no */
    }

    /* if there is no the CLUS_RESTART ALL | FAILED option => remove completed  */
    /* if CLUS_RESTART ALL specified => fail                                    */
    if (ssm.wl.clus_restart == _CLUS_RESTART_ALL)
    {
        DbgPlain(DBG_SMUTILS, _T("\tCLUS_RESTART ALL => exiting %s() [false]\n"), __FCN__);
        return 0; /* no */
    }

    /* check type of input object */
    if (
         (wloObj->type != wl_rawdisk) &&
         (wloObj->type != wl_filesystem) &&
         (wloObj->type != wl_db) &&
         (wloObj->type != wl_vbfs) &&
         (wloObj->type != wl_client) &&
         (wloObj->type != wl_winfs) &&
         (wloObj->type != wl_netware)
       )
    {
        DbgPlain(DBG_SMUTILS, _T("\tObject type mismatch => exiting %s() [false]\n"), __FCN__);
        return 0; /* no */
    }

    if (ss->sessmode & SESSION_BAR)
    {
        return 0;
    }

    /* if no failed objects, just return yes */
    if (CmnVecH(&(rdRestartData.cvFailedObjects)) < 1)
    {
        DbgPlain(DBG_SMUTILS, _T("\tNo failed objects => exiting %s() [true]\n"), __FCN__);
        return 1; /* no failed objects => yes */
    }

    /* clear input object name */
    StrClear(szObjectName1);

    /* compose input object name */
    StrToObjectName(wloObj->type,
                    wloObj->host,
                    wloObj->mtpoint,
                    wloObj->label,
                    szObjectName1);

    DbgPlain(DBG_SMUTILS, _T("[%s] szObjectName1 = '%s'\n"), __FCN__, szObjectName1);
    DbgPlain(DBG_SMUTILS, _T("[%s] CmnVecH(&(rdRestartData.cvFailedObjects)) = %d"), __FCN__, CmnVecH(&(rdRestartData.cvFailedObjects)));

    for (iIndex=0; iIndex < CmnVecH(&(rdRestartData.cvFailedObjects)); iIndex++)
    {
        szObjectName = (tchar *)CmnVecI(&(rdRestartData.cvFailedObjects), iIndex);

        /* StrFromObjetName(&type, host, mpoint, label, &wlObj) */

        DbgPlain(DBG_SMUTILS, _T("[%s] szObject = '%s'\n"), __FCN__, szObjectName);

        if (strcmp(szObjectName1, szObjectName) == 0)
        {
            DbgPlain(DBG_SMUTILS, _T("\tMatch found => exiting %s() [false]\n"), __FCN__);
            return 0; /* match found => no */
        }
    }

    /* no match found => yes */
    DbgPlain(DBG_SMUTILS, _T("\tNo match found => exiting %s() [true]\n"), __FCN__);
    return 1;
}

int RestartRemoveObjects(void)
{
    ERH_FUNCTION (_T("RestartRemoveObjects"));
    int             iIndex;
    WlObject        *wloObj;

    DbgPlain(DBG_SMUTILS, _T("%s() entered\n"), __FCN__);

    /* if there is no -restart override just exit */
    if (!ss->rdRestart.lRestart)
    {
        DbgPlain(DBG_SMUTILS, _T("!bRestart => exiting %s()\n"), __FCN__);
        return(0);
    }

    /* if there is no the CLUS_RESTART ALL | FAILED option => remove completed  */
    /* if CLUS_RESTART ALL specified => fail                                    */
    if (ssm.wl.clus_restart == _CLUS_RESTART_ALL)
    {
        DbgPlain(DBG_SMUTILS, _T("CLUS_RESTART ALL => exiting %s()\n"), __FCN__);
        return(0);
    }

    /* remove failed objects */
    for (iIndex=0; iIndex < CmnVecH(&(ssm.wl.objects)); iIndex++)
    {
        wloObj=(WlObject *)CmnVecI(&(ssm.wl.objects),iIndex);

        DbgPlain(DBG_SMUTILS, _T("\tItem #%d")
                              _T("\n\t\tType = %d")
                              _T("\n\t\tLab  = '%s'")
                              _T("\n\t\tHost = '%s'")
                              _T("\n\t\tMtpo = '%s'")
                              _T("\n\t\tSess = '%s'"),
                              iIndex, wloObj->type, NS(wloObj->label),
                              NS(wloObj->host), NS(wloObj->mtpoint), NS(wloObj->session));

        /* remove this object? */
        if (CanRemoveThisObject(wloObj) == 1)
        {
            DbgPlain(DBG_SMUTILS, _T("\t\tMATCH FOUND => REMOVING OBJECT!!!"));
            CmnVecRemove(&(ssm.wl.objects), iIndex);
            iIndex=-1;
        }
    }

    /* restart data not needed anymore => free them */
    DbgPlain(DBG_SMUTILS, _T("Freeing restart data\n"));
    FreeRestartData(&rdRestartData);

    DbgPlain(DBG_SMUTILS, _T("%s() exiting\n"), __FCN__);
    return(0);
}
#endif
