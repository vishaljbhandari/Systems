/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   OmniBack II SAP/R3 Remote Execution Server
* @file      integ/sap/cntrlbkp.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     This is the remote execution agent used for invoking
*            SAP/R3 utilities.
*
* @since     13.05.97   markus        Initial coding
*
* @remarks   |
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/sap/cntrlbkp.c $Rev$ $Date::                      $:") ;
#endif

#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "cs/csa/csa.h"
#include "inet/inet.h"
#include "integ/barutil/ob2util.h"
#include "integ/bar2/ob2common.h"
#include "integ/sap/commonmsg.h"
#include "lib/xtra/scramble.h"
#include "integ/bar2/ob2ddt.h"

#include "cntrlbkp.h"
#include "integ/sap/sap_common.h"
#include "integ/bar2/ob2backup.h"

#define BACKUP_CONTROLFILE  _T("-c -d util_file%s -m %s -u %s")
#define SAP_MODE_OFFLINE _T("")
#define SAP_MODE_ONLINE _T("_online")

#define CONTROLFILE_BACKUP_TIMEOUT  600
#define POLL_TIMEOUT 5

int fOracleState = ORA_STATE_SHUTDOWN;
int fReading = 0;
tchar  **cntrlList=NULL;
int cntrlListCount=0;
int everythingWasBackedUp=0;




/*========================================================================*//**
*
* @param     szProgramName   Program name
* @param     szOptions       Program options
* @param     argc            Number of program arguments
* @param     argv            Input arguments returned in an array
*
* @retval    0       ok
* @retval    -1      error
*
* @brief     Function transforms the parameters from string to an array.
*
* @remarks   Function does not display command arguments in debug files.
*
*//*=========================================================================*/
#define NO_QUOTE    0
#define D_QUOTE     1
#define S_QUOTE     2

int StrToArgNoDbg(tchar *szProgramName, tchar *szOptions, int *argc, tchar **argv[])
{
    int     iCount = 0;
    tchar   **szA = NULL;
    tchar   *szOneOpt;
    tchar   *pOpt = szOptions, *pOpt1;
    int     lQuote = NO_QUOTE;

    /* add program name */
    szA = (tchar **) REALLOC( szA, (++iCount) * sizeof(tchar *) );
    if (szProgramName)
        szA[iCount-1] = StrNewCopy(szProgramName);
    else
        szA[iCount-1] = NULL;

    while ( (pOpt) && (*pOpt) )
    {
        lQuote = NO_QUOTE;

        while ( (*pOpt) && ( (*pOpt) == _T(' ')) ) pOpt++;
        if (!(*pOpt)) break;

        if ( (*pOpt) == _T('\"') )
        {
            if (!(*(++pOpt))) break;
            lQuote = D_QUOTE;
        }
        else if ( (*pOpt) == _T('\'') )
        {
            if (!(*(++pOpt))) break;
            lQuote = S_QUOTE;
        }

        if (lQuote==D_QUOTE)
            pOpt1 = strchr(pOpt , _T('\"'));
        else if (lQuote==S_QUOTE)
            pOpt1 = strchr(pOpt , _T('\''));
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

    /* add sentinel at the end, but don't increment the counter */
    szA = REALLOC( szA, (iCount+1) * sizeof(tchar *) );
    szA[iCount] = NULL;

    *argc = iCount;
    *argv = szA;

    return(0);
}


/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     psBuffer       message to parse
*
* @retval    success code
*
* @brief     Parse orcale instatce state.
*
*//*=========================================================================*/
static int
ErhStateParseHook (tchar *psBuffer)
{
    tchar       *tmpPtr = NULL;

    /* skip spaces and tabs */
    tmpPtr = psBuffer;
    while ( isspace (*tmpPtr) )
    {
        tmpPtr++;
    }

    if ( strstr(tmpPtr, _T("ORA-01034:")) )
    {
        /* ORA-01034: ORACLE not available */
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Got ORA-01034, the database is not available."));
        fOracleState = ORA_STATE_SHUTDOWN;
    }
    if ( strstr(tmpPtr, _T("ORA-01109:")) )
    {
        /* ORA-01109: ORACLE not available */
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Got ORA-01109, the database is not open."));
        fOracleState = ORA_STATE_SHUTDOWN;
    }
    if ( strstr(tmpPtr, _T("ORA-03114:")) )
    {
        /* ORA-03114: not connected to ORACLE */
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Got ORA-03114, the database is not available."));
        fOracleState = ORA_STATE_SHUTDOWN;
    }
    if ( strstr(tmpPtr, _T("ORA-01031:")) )
    {
        /* ORA-01031: insufficient privileges */
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Got ORA-01031, insufficient privileges."));
        fOracleState = ORA_STATE_UNSUFFICIENT_PRIVILEGIES;
    }

    if(strstr(tmpPtr, _T("STARTED")))
    {
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Database is started in NOMOUNT mode."));
        fOracleState = ORA_STATE_NOMOUNT;
    }

    if(strstr(tmpPtr, _T("MOUNTED")))
    {
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Database is started in MOUNT mode."));
        fOracleState = ORA_STATE_MOUNT;
    }

    if(strstr(tmpPtr, _T("OPEN")))
    {
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Database is opened."));
        fOracleState = ORA_STATE_OPEN;
    }

    if(strstr(tmpPtr, _T("ORA-01089:")))
    {
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T(" Got ORA-01089: immediate shutdown in progress - no operations are permitted"));
        fOracleState = ORA_STATE_TRANS;
    }

    if(strstr(tmpPtr, _T("ORA-01033:")))
    {
        DbgStamp(DBG_SAP_INFO);
        DbgPlain(DBG_SAP_INFO, _T("Got ORA-01033:   ORACLE initialization or shutdown in progress"));
        fOracleState = ORA_STATE_TRANS;
    }

    return(SAP_OK);
}


/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     oraHome - ORACLE_HOME
* @param     connStrFixed  - database connection string
*
* @retval    ORA_STATE_
*
* @brief     The function checks the database state.
*
*//*=========================================================================*/
int GetDbStatus(tchar *oraHome, tchar *connStrFixed)
{
    SAP_ENTER (_T("GetDbStatus"));
    fOracleState = ORA_STATE_SHUTDOWN;

    ErhSetHooks(ErhStateParseHook,ErhStateParseHook);
    if (RunOracleQuery (oraHome, 0, QUERY_DBSTATE, connStrFixed) != OBE_OK)
    {
        ErhSetHooks(ErhReportHook, ErhConsoleHook);
        DbgStamp (DBG_ERROR);
        DbgPlain (DBG_ERROR, _T("[%s] Error while retrieving database status. ('%s' failed)"),
            __FCN__, QUERY_DBSTATE);

        fOracleState = ORA_STATE_ERROR;
    }
    ErhSetHooks(ErhReportHook, ErhConsoleHook);

    SAP_RETURN(fOracleState);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     lockDir - directory where brbackup puts its lock file,
*                           usualy it's SAPBACKUP or <SAPDATA_HOME>/sapbackup
*
* @retval    SAP_OK - if lock in case lock was not found
* @retval    SAP_ERR - lock file found
*
* @brief     The function looks for .lock.brb file - which indicates brbackup activity
*
*//*=========================================================================*/
static int IsBRBLock (const tchar *lockDir)
{
    tchar lock_file[STRLEN_PATH+1] = {0};

    PathConcat(lock_file, lockDir, _T(".lock.brb"), STRLEN_PATH);

    return (OB2_StatFile(lock_file, NULL)? SAP_OK : SAP_ERR);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     None
*
* @retval    Path to SAPBACKUP directory
*
* @brief     Function resolves SAPBACKUP path in this order:
*            SAPBACKUP path - if set in environment
*            <SAPDATA_HOME/sapbackup> - this is default location for SAPBACKUP
*            CmnPanTmp - if everything else failed
*
*//*=========================================================================*/
static tchar *GetSapbackupPath()
{
    tchar *sapBackupPath=EnvGetString(_T("SAPBACKUP"));
    SAP_ENTER(_T("GetSapbackupPath"));

    if(StrIsEmpty(sapBackupPath))
    {
        tchar newPath[STRLEN_PATH+1]={0};
        tchar *dataHome = EnvGetString(_T("SAPDATA_HOME"));

        DbgStamp(DBG_MAIN_ACTION);
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] SAPBACKUP variable not found in ENV"), __FCN__);
        if (StrIsEmpty(dataHome))
        {
            sapBackupPath=StrNewCopy(cmnPanTmp);
        }
        else
        {
            PathConcat(newPath, dataHome, _T("sapbackup"), STRLEN_PATH);
            FREE (dataHome);
            sapBackupPath = StrNewCopy(newPath);
        }
    }
    DbgStamp(DBG_MAIN_ACTION);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Sapbackup path is %s."), __FCN__, sapBackupPath);
    DbgFcnOut();
    return(sapBackupPath);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     szOracleHome
* @param     lpszConnStr - decrypted connection string
* @param     szSapbackup_dir - <SAPBACKUP> directory
* @param     OB2_CONTEXT ctx
*
* @retval    SAP_OK - if control file can be copied safely
* @retval    SAP_ERR - in case that som external backup process is active
*
* @brief     Function checks for external brbackup process activity. If detected, waits until
*            brbackup completes or aborts control file backup if ABORT message received
*
*//*=========================================================================*/
static int
MakeControlFileCopyEx(tchar *szOracleHome, tchar *lpszConnStr, tchar *szSapbackup_dir, OB2_CONTEXT ctx)
{
    int retVal = SAP_OK;
    int stop_wait = 0;
    tchar *barlist = NULL;
    int handle = -1;
    int status = GetDbStatus(szOracleHome, lpszConnStr);


    SAP_ENTER(_T("MakeControlFileCopyEx"));

    if ( ( status != ORA_STATE_OPEN ) && (status != ORA_STATE_MOUNT ) && IsBRBLock(szSapbackup_dir))
    {
        time_t start_time = time(NULL);
        BOOL wait_on_BRB_LOCK = 0;

        EnvReadInt(_T("OB2_WAIT_ON_BRBLOCK"),&wait_on_BRB_LOCK);

        barlist = BU_EnvGetStrValue (_T("OB2BARLIST"), _T(""));
        if (!StrLen(barlist))
        {
            DbgStamp (DBG_ERROR);
            DbgPlain (DBG_ERROR, _T("[%s] Got empty barlist, cannot connect to BSM."), __FCN__);
            retVal = SAP_ERR;
            goto exit;
        }

        ErhFullReport(
            __FCN__, ERH_NORMAL,
            NlsGetMessage(NLS_SET_SAP, NLS_SAP_CTLFILE_EXTERN_LOCK)
            );

        /* connect to BSM and wait for brbackup to stop or MSG_ABORT*/
        if (OB2_StartBackup(ctx, barlist) != OBE_OK)
        {
            DbgStamp (DBG_ERROR);
            DbgPlain (DBG_ERROR, _T("[%s] Got error while connecting to BSM."), __FCN__);
            retVal = SAP_ERR;
            goto exit;
        }
        handle = OB2_GetSMHandle(ctx);
        if (OBE_OB2WRONGCTX == handle)
        {
            DbgStamp (DBG_ERROR);
            DbgPlain (DBG_ERROR, _T("[%s] Incorrect handle passed from BSM."), __FCN__);

            retVal = SAP_ERR;
            goto exit;
        }

        while (!stop_wait)
        {
            StrMsgStruct msg = {0};
            int msgID = OB2_ReceiveMessageEx(ctx, POLL_TIMEOUT, &msg);

            if ((MSG_ABORT == msgID) ||
                ( !wait_on_BRB_LOCK && (time(NULL) - start_time > CONTROLFILE_BACKUP_TIMEOUT)))
            {
                stop_wait = 1;
                ErhFullReport(
                    __FCN__, ERH_WARNING,
                    NlsGetMessage(NLS_SET_SAP, NLS_SAP_CTLFILE_TIMEDOUT)
                    );

                retVal = SAP_ERR;
            }

            stop_wait = stop_wait ||
                (!IsBRBLock(szSapbackup_dir) && GetDbStatus(szOracleHome, lpszConnStr) != ORA_STATE_TRANS);
        }
        /*closing connection to BSM - brbackup completed or ABORT received*/
        OB2_EndBackup (ctx, OBE_OK);
    }

exit:
    FREE (barlist);
    SAP_RETURN (retVal);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @param     CSHost - cellserver
* @param     szOracleSID - Oracle SID
*
* @retval    0
*                success
* @retval    -1
*                error
*
* @brief     The function handles the offline case. If the database is it starts
*            database first.
*
*//*=========================================================================*/
static int
MakeControlFileCopy(tchar *CSHost, tchar *szOracleSID, OB2_CONTEXT ctx)
{
    int     retVal=SAP_OK;
    tchar   *connStr=NULL;
    tchar   *oraHome;
    tchar   connStrFixed[STRLEN_STD+1];
    tchar   cfCopyName[STRLEN_PATH+1];
    tchar   *sapBackupPath=NULL;
    tchar   controlFilename[STRLEN_PATH+1]={0};
    BP_Opt        config;

    SAP_ENTER (_T("MakeControlFileCopy"));

    { /* For cluster needs we parse variable OB2BARHOSTNAME. In case it is set it should be used as
        a hostname where we run instead of cmnHostname */
        tchar *barHostname;

        if ((barHostname=EnvGetString(_T("OB2BARHOSTNAME"))) != NULL)
        {
            DbgPlain(DBG_MAIN_ACTION, _T("OB2BARHOSTNAME variable is set to %s, using it as the hostname"), barHostname);
            StrCopy(cmnHostname, barHostname, STRLEN_HOSTNAME);
            FREE(barHostname);
        }
    }

    DbgPlain(DBG_SAP_INFO,
        _T("[%s] Getting configuration from Cell Server \"%s\" with instance \"%s\""),
        __FCN__,
        NSD(CSHost),
        NSD(szOracleSID));

    if (BU_GetConfig(CSHost, cmnHostname, OB2_APP_SAP, szOracleSID, &config) != BP_NOERROR)
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_GETCONFIG_FAILED,
                CSHost,
                szOracleSID) );

        retVal = -1;
        goto exit;
    }

    DbgStamp (DBG_SAP_STATUS);
    DbgPlain (DBG_SAP_STATUS,
        _T("[%s] Configuration read from Cell Manager."),
        __FCN__);

    /*Get the connection string*/
    if(BP_NOERROR != BP_GetStrByName(config, CONF_CONNECT_STR, &connStr))
    {
        const tchar *tmp = NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_CANNOT_GET_PARAMETERS,
            CONF_CONNECT_STR);
        ErhFullReport (__FCN__, ERH_CRITICAL, tmp );
        ErhClearError();
        retVal=SAP_ERR;
        goto exit;
    }

    connStr = DecodePassword(connStr);

    /*Get Oracle home directory*/

    if (BP_NOERROR != BP_GetStrByName(config, CONF_ORACLE_HOME, &oraHome))
    {
        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_CANNOT_GET_PARAMETERS,
                CONF_ORACLE_HOME)
            );
        retVal=SAP_ERR;
        goto exit;
    }

    StrCopy (connStrFixed, connStr, STRLEN_STD);
    if (!StrIsEmpty(connStr) && (connStr[strlen(connStr)-1] == _T('@')))
    {
        /* missing SID in config, current util_sap does not complain so we will not too */
        StrCat (connStrFixed, szOracleSID, STRLEN_STD);
        DbgPlain(DBG_SAP_STATUS, _T("[%s] Configuration string missing SID - fixed (%s added)."), __FCN__, szOracleSID);
    }

    sapBackupPath = GetSapbackupPath();

    retVal = MakeControlFileCopyEx(oraHome, connStr, sapBackupPath, ctx);
    if (retVal != SAP_OK)
        goto exit;

    ErhSetHooks(ErhStateParseHook,ErhStateParseHook);
    fOracleState=ORA_STATE_SHUTDOWN;
    if (RunOracleQuery (oraHome, 0, QUERY_DBSTATE, connStrFixed) != OBE_OK)
    {
        ErhSetHooks(ErhReportHook, ErhConsoleHook);
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
            NLS_SET_SAP,
            NLS_SAP_COMMAND_FAILED,
            QUERY_DBSTATE) );

        retVal=SAP_ERR;
        goto exit;
    }
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Database state is %d"), __FCN__, fOracleState);

    switch (fOracleState)
    {
        case ORA_STATE_UNSUFFICIENT_PRIVILEGIES:
            DbgPlain(DBG_ERROR, _T("[%s] Unsufficient privilegies. Can not connect to Oracle"), __FCN__);
            retVal=SAP_ERR;
            goto exit;
        case ORA_STATE_SHUTDOWN:
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Oracle is not started."), __FCN__);
            script_status=-1;
            ErhSetHooks(ErhOraStatusHook, ErhOraStatusHook);
            if ((RunOracleQuery (oraHome, 0, ORA_STARTUP_MOUNT, connStrFixed) != OBE_OK) || (script_status!=0))
            {
                if (script_status == 32004)
                {
                    /* The error ORA-32004 comes for "startup mount"
                     * but the instance gets mounted. So returning
                     * no error.
                     * ORA-32004: obsolete and/or deprecated parameter(s) specified 
                     */
                     DbgPlain(10, _T("script_status is 32004"));
                     script_status = 0;
                }
                else
                {
                ErhSetHooks(ErhReportHook, ErhConsoleHook);
                    
                ErhFullReport(
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                    NLS_SET_SAP,
                    NLS_SAP_COMMAND_FAILED,
                    ORA_STARTUP_MOUNT) );

                retVal=SAP_ERR;
                goto exit;
            }
            }
            break;
        case ORA_STATE_NOMOUNT:
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Oracle is not mounted."), __FCN__);
            script_status=-1;
            ErhSetHooks(ErhOraStatusHook, ErhOraStatusHook);
            if ((RunOracleQuery (oraHome, 0, ORA_ALTER_DATABASE_MOUNT, connStrFixed) != OBE_OK) || (script_status!=0))
            {
                ErhSetHooks(ErhReportHook, ErhConsoleHook);

                ErhFullReport(
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                    NLS_SET_SAP,
                    NLS_SAP_COMMAND_FAILED,
                    ORA_ALTER_DATABASE_MOUNT) );

                retVal=SAP_ERR;
                goto exit;
            }
            break;
        case ORA_STATE_MOUNT:
        case ORA_STATE_OPEN:
            break;
        default:
            DbgPlain(DBG_ERROR, _T("[%s] Unexpected database state"), __FCN__);
            retVal=SAP_ERR;
            goto exit;
    }
    sprintf(controlFilename,_T("ctrl%s.dbf"),szOracleSID);
    PathConcat(cfCopyName, sapBackupPath, controlFilename, STRLEN_PATH);

    script_status=-1;
    ErhSetHooks(ErhOraStatusHook,ErhOraStatusHook);
    if ((RunOracleQuery (oraHome, 0, ORA_COPY_CONTROLFILE, connStrFixed, cfCopyName) != OBE_OK) || (script_status!=0))
    {
        ErhSetHooks(ErhReportHook, ErhConsoleHook);

        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
            NLS_SET_SAP,
            NLS_SAP_COMMAND_FAILED,
            ORA_COPY_CONTROLFILE) );

        retVal=SAP_ERR;
    }

    if((fOracleState==ORA_STATE_SHUTDOWN)||(fOracleState==ORA_STATE_NOMOUNT))
    {
        script_status=-1;
        ErhSetHooks(ErhStateParseHook,ErhStateParseHook);
        if ((RunOracleQuery (oraHome, 0, ORA_SHUTDOWN, connStrFixed) != OBE_OK))
        {
            if (GetDbStatus(oraHome, connStrFixed) != ORA_STATE_SHUTDOWN)
            {
                ErhSetHooks(ErhReportHook, ErhConsoleHook);

                ErhFullReport(
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                    NLS_SET_SAP,
                    NLS_SAP_COMMAND_FAILED,
                    ORA_SHUTDOWN) );

                retVal=SAP_ERR;
            }
            else
            {
        ErhSetHooks(ErhReportHook, ErhConsoleHook);
                ErhFullReport(
                    __FCN__,
                    ERH_WARNING,
                    NlsGetMessage(NLS_SET_SAP, NLS_SAP_SHUTDOWN_ERROR)
                    );
            }
        }
    }

    if(fOracleState==ORA_STATE_NOMOUNT)
    {
        script_status=-1;
        if ((RunOracleQuery (oraHome, 0, ORA_STARTUP_NOMOUNT, connStrFixed) != OBE_OK) || (script_status!=0))
        {
            ErhSetHooks(ErhReportHook, ErhConsoleHook);

            ErhFullReport(
                __FCN__,
                ERH_CRITICAL,
                NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_COMMAND_FAILED,
                ORA_STARTUP_NOMOUNT) );

            retVal=SAP_ERR;
        }
    }

exit:
    ErhSetHooks(ErhReportHook, ErhConsoleHook);
    FREE(connStr);
    FREE(sapBackupPath);

    SAP_RETURN(retVal);
}
/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration
*
* @retval    0
*                success
* @retval    -1
*                error
*
* @brief     Performes the backup of oracle Controfile
*
*//*=========================================================================*/
int SAP_BackupControlFile(
    tchar *CSHost,
    tchar *szOracleSID,
    tchar *backintPath,
    OB2_CONTEXT ctx)
{
    int retVal=SAP_OK;
    tchar cfCopyName[STRLEN_PATH+1]={0};       /*Name Of the Copy of Controlfile*/
    tchar backintFullName[STRLEN_PATH+1]={0};
    tchar backintName[STRLEN_PATH+1]={0};
    tchar *sapBackupPath=NULL;
    tchar controlFilename[STRLEN_PATH+1]={0}, tempFilename[STRLEN_PATH+1]={0};
    tchar tmpFile[STRLEN_PATH+1]={0};
/*
    FILEHANDLE cfTmpFile=NULL;
*/
    FILE *cfTmpFile=NULL;

    SAP_ENTER(_T("SAP_BackupControlFile"));



    sapBackupPath = GetSapbackupPath();

#if !TARGET_WIN32
    PathConcat(backintFullName, backintPath, _T("backint"), STRLEN_PATH);
    StrCopy(backintName, _T("backint"), STRLEN_PATH);
#else
    PathConcat(backintFullName, backintPath, _T("backint.exe"), STRLEN_PATH);
    StrCopy(backintName, _T("backint.exe"), STRLEN_PATH);
#endif

    sprintf(controlFilename,_T("ctrl%s.dbf"),szOracleSID);
    PathConcat(cfCopyName, sapBackupPath, controlFilename, STRLEN_PATH);
    sprintf(tempFilename,_T("cftmpfile_%s"),szOracleSID);
    PathConcat(tmpFile, sapBackupPath, tempFilename, STRLEN_PATH);

/* At First make a copy of control file to cmnPanTmp directory*/

    if(OB2_StatFile(cfCopyName, NULL)==0)
    {
        DbgPlain(DBG_MAIN_ACTION,_T("[%s] %s file already exist. Delete it."), __FCN__, cfCopyName);
        OB2_DeleteFile(cfCopyName);
    }
    if(SAP_OK!=MakeControlFileCopy(CSHost, szOracleSID, ctx))
    {
        DbgStamp(DBG_ERROR);
        DbgPlain(DBG_ERROR, _T("Copy Of Controlfile is not done"));
        retVal=SAP_ERR;
        goto exit;
    }

/* Now we will create an input file for backint and put the name of the created
                         copy of controlfile inside it*/

    if(OB2_StatFile(tmpFile, NULL)==0)
    {
        DbgPlain(DBG_MAIN_ACTION,_T("[%s] %s file already exist. Delete it."), __FCN__, tmpFile);
        OB2_DeleteFile(tmpFile);
    }

    if(!(cfTmpFile = fopen(tmpFile, _T("w"))))
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
            NLS_SET_SAP,
            NLS_SAP_TEST_CANNOT_OPEN_FILE,
            tmpFile,
            GetLastError()
            )
            );
        retVal=SAP_ERR;
        goto exit;
    }
    fputs(cfCopyName, cfTmpFile);
    fclose(cfTmpFile);


/*           Now we will call backint                */

    {
        tchar backintArgs[STRLEN_PATH]={0};
        tchar   **argv=NULL;
        int     argc=0;
        sprintf(backintArgs,BACKINT_BACKUP_ARGS, szOracleSID, tmpFile);
        StrToArg(backintName, backintArgs, &argc, &argv);

        PutEnv(_T("BI_REQUEST=NEW"));

        if ( CmnRunScriptEx(
                 backintPath,
                 argc,
                 argv,
                 NULL,
                 0,
                 RUN_SCRIPT_HIDE_INPUT,
                 &retVal) != RUN_SCRIPT_OK )
        {

            ErhFullReport(
                __FCN__,
                ERH_CRITICAL,
                NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_COMMAND_FAILED,
                argv[0]) );

            retVal=SAP_ERR;
            goto exit;
        }
    }
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Backup of controlfile is completed successfully"),__FCN__);

/*  And finally temporary files must be deleted */

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Delete %s"), __FCN__, tmpFile);
   if(OB2_DeleteFile(tmpFile)!=SAP_OK)
    {
        DbgStamp(DBG_ERROR);
        DbgPlain(DBG_ERROR, _T("[%s] Cannot delete temporary file %s"), __FCN__, tmpFile);
    }
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Delete %s"), __FCN__, cfCopyName);
    if(OB2_DeleteFile(cfCopyName)!=SAP_OK)
    {
        DbgStamp(DBG_ERROR);
        DbgPlain(DBG_ERROR, _T("[%s] Cannot delete temporary file %s"), __FCN__, tmpFile);
    }

exit:
    FREE(sapBackupPath);
    SAP_RETURN(retVal);
}
