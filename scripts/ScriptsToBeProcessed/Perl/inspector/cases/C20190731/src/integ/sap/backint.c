/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   OmniBack II - SAP R/3 BAR Integration
* @file      integ/sap/backint.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     OmniBack II - SAP R/3 BAR Integration Interface Program
*/
/* ---------------------------------------------------------------------------
CR    
CR   Code review for OB 4.0 SAP - Alenka Starc   
CR    
 -------------------------------------------------------------------------- */

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/sap/backint.c $Rev$ $Date::                      $:") ;
#endif

#include "lib/cmn/common.h"
#include "cs/csa/csa.h"
#include "integ/barutil/ob2util.h"
#include "lib/parse/cellinfo.h"
#include "lib/parse/gen_parser.h"
#include "integ/bar2/libbar.h"
#include "integ/bar2/ob2common.h"

#include "optmgr.h"
#include "backint.h"
#include "balance.h"
#include "commonmsg.h"
#include "dmaback.h"

#include "integ/sap/sap_common.h"

#if TARGET_WIN32
#   include <signal.h>
#endif

extern int reportHook(tchar *msg);
extern int consoleHook(tchar *msg);

FILE           *inFile, *outFile;
struct extFile *tmpFile;
int             argRet      = 0;
tchar          *prefix      = _T("BAL_");
enum workModeType {Normal = 0, SMB_DMA, SMB_DiskOnly} ;
enum workModeType    workMode;
static tchar  **verTab = NULL;
static ULONG    verNum = 0;
static ULONG    verLen = 0;
BP_Opt          BRdoc = {0};

OB2_CONTEXT context;

tchar      *semFile = _T("dummy"); /* ZQ: just to cheat sapback */
tchar      *barlist = NULL;
BOOL        aborted = FALSE;

extern tchar *NT2OB_FileName (tchar *);
extern tchar *OB2NT_FileName (tchar *);

static int SapFindBackIDsOfFile (tchar *backID, tchar *fileIDin);

static void
CmnVoidAbortHandler(int sig)
{
    aborted = TRUE;
}

/*========================================================================*//**
*
* @param     sessGuid - session Guid constructed from BSM pid, start session time, rnd
*
* @retval    -1   error
* @retval    >=0  next object id
*
*//*=========================================================================*/
typedef struct {
    FILEHANDLE lockfile;
    BOOL       locked;
    BOOL       read;
    
    struct {
        int   nObjects;
        tchar sGuid[STRLEN_GUID+1];
    } data;

} SAP_BACKUP_INFO;


static int SAP_UnlockBackupInfo (SAP_BACKUP_INFO *info)
{
    ERH_FUNCTION (_T("SAP_UnlockBackupInfo"));

    int status;
    int retval = 0;
    
    DbgFcnIn();

    if (info->lockfile == INVALID_HANDLE_VALUE)
        RETURN_INT (0);

    if (info->read)
    {
        OB2_SeekFile(info->lockfile, 0, SEEK_SET);

        DbgPlain (DBG_MAIN_ACTION, _T("[%s] nObjects:%d sGuid:%s"), __FCN__, info->data.nObjects, info->data.sGuid);
        status = OB2_WriteFile (info->lockfile, &info->data, sizeof(info->data));
        if (sizeof(info->data) != status)
            retval = -1;
        
        info->read = FALSE;
    }
    
    if (info->locked)
    {
        if (OB2_LockFile (info->lockfile, F_ULOCK, 0, 0) != 0)
            retval = -1;

        info->locked = FALSE;
    }

    OB2_CloseFile (info->lockfile);
    info->lockfile = INVALID_HANDLE_VALUE;

    RETURN_INT (retval);
}


static int SAP_LockBackupInfo (SAP_BACKUP_INFO *info)
{
    ERH_FUNCTION(_T("SAP_LockBackupInfo"));
    tchar *barList;

    int status;
    tchar      lockpath[STRLEN_PATH+1];
    DbgFcnIn();
    
    memset (info, 0, sizeof(*info));

    /* define per-barlist session info file name */
    barList = BU_GetEnv(_T("OB2BARLIST"));
    sprintf (lockpath, BACKINT_SESSINFO_FILE_FMT, cmnPanTmp, barList);
    FREE (barList);

    info->lockfile = OB2_OpenFile (lockpath, 1, 2);
    if (INVALID_HANDLE_VALUE == info->lockfile)
        RETURN_INT (-1);
    
    status = OB2_LockFile (info->lockfile, F_LOCK, 0, 0);
    if (status < 0)
    {
        SAP_UnlockBackupInfo(info);
        RETURN_INT (-1);
    }

    info->locked = TRUE;

    status = OB2_ReadFile (info->lockfile, &info->data, sizeof(info->data));
    if (status < 0)
    {
        SAP_UnlockBackupInfo(info);
        RETURN_INT (-1);
    }

    DbgPlain (DBG_MAIN_ACTION, _T("[%s] nObjects:%d sGuid:%s"), __FCN__, info->data.nObjects, info->data.sGuid);

    info->read = TRUE;

    RETURN_INT (0);
}


int NextSetNumber(const tchar *sessGuid)
{
    ERH_FUNCTION(_T("NextSetNumber"));
    SAP_BACKUP_INFO info = {0};
    int             retval;

    DbgFcnInEx (__FLND__, _T("sessGuid: %s"), NS(sessGuid));

    if (0 != SAP_LockBackupInfo(&info))
       RETURN_INT (-1);

    if (0!=StrICmp(info.data.sGuid, sessGuid))
        info.data.nObjects = 0;

    retval = info.data.nObjects;

    info.data.nObjects++;
    StrCopy (info.data.sGuid, sessGuid, STRLEN_GUID);

    if (0!=SAP_UnlockBackupInfo(&info))
        RETURN_INT (-1);

    RETURN_INT (retval);
}



/* ---------------------------------------------------------------------------

    1. Parse options and decide what function to perform

 -------------------------------------------------------------------------- */
/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     setVer[in]     name of the session
* @param     index[in]      number of set
* @param     backID[out]    SAP backID
*
* @retval    CC_SUCCESS
* @retval    CC_FAILURE
*
* @brief     The session ID is converted to sap backID.
*
* @remarks   If number of the session is bigger than 9999 than four digits are hex-a.
*            If number of sets is bigger than 99 (doesn't fit into 2 decimal digits)
*            than the last two digits are in hex-a.
*
*//*=========================================================================*/
int
SapSetVer2BackID(tchar *setVer,
                 int index,
                 tchar *backID)
{
    ERH_FUNCTION(_T("SapSetVer2BackID"));

    int    year   = 0;
    int    month  = 0;
    int    day    = 0;
    tchar  date[STRLEN_STD+1] = {0};
    tchar  tmpBackId[STRLEN_SESSIONID+1] = {0};
    int    formatCheck = 0;
    unsigned int count = 0;

    DbgFcnIn();

    DbgPlain (DBG_SAP_INFO, _T("Session name : %s"), setVer);
    DbgPlain (DBG_SAP_INFO, _T("Set number   : %d"), index);

    formatCheck = sscanf (setVer, FORMAT_SESSIONNAME_NEW, date, &count);
    formatCheck += sscanf (date, FORMAT_DATE, &year, &month, &day);
    if (formatCheck != 5)
    {
        ErhMark(ERR_SAP_WRONG_BACKID_FORMAT, __FCN__, ERH_CRITICAL);

        DbgFcnOut();
        return(CC_FAILURE);
    }

    if (count > 9999)
    {
        sprintf (tmpBackId, FORMAT_TMP_BACKID_NEW, year, month, day, count);
    }
    else
    {
        sprintf (tmpBackId, FORMAT_TMP_BACKID, year, month, day, count);
    }

    if (index > 99)
    {
        sprintf (backID, _T("%s:") FMT_SETID_NEW, tmpBackId, index);
    }
    else
    {
        sprintf (backID, _T("%s.") FMT_SETID, tmpBackId, index);
    }

    DbgPlain (DBG_SAP_INFO, _T("SAP BackID      : %s"), backID);

    DbgFcnOut();
    return(CC_SUCCESS);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param[in]    backID  SAP backID
* @param[out]   setVer  name of the session
* @param[out]   index   number of set
*
* @retval    CC_SUCCESS
* @retval    CC_FAILURE
*
* @brief     The sap backID is converted to Session ID.
*
* @remarks   If number of the session is bigger than 9999 than four digits are hex-a.
*            If number of sets is bigger than 99 (doesn't fit into 2 decimal digits)
*            than the last two digits are in hex-a.
*
*//*=========================================================================*/
int
SapBackID2SetVer(tchar *backID,
                 tchar *setVer,
                 int *index)
{
    ERH_FUNCTION(_T("SapBackID2SetVer"));

    int     year   = 0;
    int     month  = 0;
    int     day    = 0;
    tchar   date[STRLEN_STD+1] = {0};
    tchar   setVerTmp[STRLEN_SESSIONID+1] = {0};
    unsigned int count = 0;
    int formatCheck = 0;

    DbgFcnIn();

    DbgPlain (DBG_SAP_INFO, _T("SAP BackID     : %s"), backID);

    formatCheck = sscanf (backID, FORMAT_SAPBACKID, &year, &month, &day, &count, index);
    if (formatCheck != 5)
    {
        formatCheck = sscanf (backID, FORMAT_SAPBACKID_SETHEX, &year, &month, &day, &count, index);
    }
    if (formatCheck != 5)
    {
        formatCheck = sscanf (backID, FORMAT_SAPBACKID_SESSHEX, &year, &month, &day, &count, index);
    }
    if (formatCheck != 5)
    {
        formatCheck = sscanf (backID, FORMAT_SAPBACKID_ALLHEX, &year, &month, &day, &count, index);
    }

    if (formatCheck != 5)
    {
        ErhMark(ERR_SAP_WRONG_BACKID_FORMAT, __FCN__, ERH_CRITICAL);

        DbgFcnOut();
        return(CC_FAILURE);
    }

    sprintf (date, FORMAT_DATE, year, month, day);
    sprintf (setVerTmp, FORMAT_SESSIONNAME_NEW, date, count);
    StrCopy(setVer, CliStrToSessionID(setVerTmp), STRLEN_SESSIONID);

    DbgPlain (DBG_SAP_INFO, _T("Set number     : %d"), *index);
    DbgPlain (DBG_SAP_INFO, _T("Session name   : %s"), setVer);

    DbgFcnOut();
    return(CC_SUCCESS);
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     ind
*
* @retval    N/A
*
* @brief     Frees memory from the element of tmpFile array, which contains data
*            used for backup/restore of files. Each element corresponds to single
*            backup set.
*
*//*=========================================================================*/
void 
SapFreeMemory (int ind)
{
    ERH_FUNCTION(_T("SapFreeMemory"));

    int ii,jj;

    DbgFcnIn();
    for (ii=0; ii<ind; ii++)
    {
        for (jj=1; jj<tmpFile[ii].count; jj++)
        {
            FREE (tmpFile[ii].files[jj].name);
        }

        FREE (tmpFile[ii].name);
        FREE (tmpFile[ii].files);
        FREE (tmpFile[ii].barName);
        FREE (tmpFile[ii].backID);
    }
    
    FREE (tmpFile);
    DbgFcnOut();
}








/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     mode        backup mode
*
* @retval    success code
*
* @remarks   CR - split this function into several functions
*            CR - because it is too long...
*
*//*=========================================================================*/
static int
SapBackupFiles (int mode, tchar *request)
{
    ERH_FUNCTION(_T("SapBackupFiles"));
    
    int              i;
    int              todo = 0;
    int              done = 0;
    tchar            path[STRLEN_1K+1];
    int              totalFiles;
    speedListPtr     slp = NULL;
    tchar           *oraHome = NULL;
    int          totCount;
    double       totFsize;
    int          sapAdd = 0;

    DbgFcnIn();

    DbgPlain(DBG_SAP_INFO, 
        _T("[%s] mode is %s"),
        __FCN__,
        (mode==FILE_ONLINE)?_T("FILE_ONLINE"):_T("BACKUP"));


/* ---------------------------------------------------------------------------
CR    
CR   Comment is missing, I would also also Dbg statement at the level requested
CR   in the ERS for external variables   
CR    
 -------------------------------------------------------------------------- */
        if (!request)
        {       
            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage(
                    NLS_SET_SAP,
                    NLS_NOTSET_BIREQUEST) );
            
            /* seting default value...*/
            request = StrNewCopy(_T("NEW"));
        }
        
        DbgStamp (DBG_SAP_FLOW);
        DbgPlain (DBG_SAP_FLOW, _T("%s started!"), __FCN__);
        DbgPlain (DBG_SAP_INIT, _T("BI_REQUETST   : %s"), request);
        DbgPlain (DBG_SAP_INIT, _T("mode          : %d"), mode);
        
        if (mode == FILE_ONLINE)
        {            
/* ---------------------------------------------------------------------------
|  ORACLE_HOME (UX) / SAPBACKUP (NT) has to be exported,
|  This is the directory where semaphore file for BRBACKUP is stored
|  if we are running in a FILE_ONLINE mode
 -------------------------------------------------------------------------- */
            if (StrIsEmpty(oraHome = EnvGetString(SWITCH_DIR_VARIABLE)))
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] GetEnv(%s) failed. Error: %s"),
                    __FCN__, 
                    SWITCH_DIR_VARIABLE,
                    ErhSysErrnoToText(GetLastError()));
                
                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_SAP,
                        NLS_ENV_VAR_NOTSET,
                        _T("SAPBACKUP")) );

                DbgFcnOut();
                return (CC_FAILURE);
            }
        }



        DbgPlain(DBG_SAP_INIT, _T("Balancing type : %d"), sapOpt.balance);
        
/* ---------------------------------------------------------------------------
|  allocating file structure, used for balancing files to the backup devices
--------------------------------------------------------------------------- */ 
        tmpFile = CALLOC (sapOpt.conc, sizeof (struct extFile));

        OB2_StartBackup (OB2_GetContext(), BP_GetStr(&BRdoc, _T("OB2BARLIST")));

/* ---------------------------------------------------------------------------
| This part of code is for !TARGET_WIN32 ONLY
| Code to set balance type to SAP_DISK_BAL is commented out  
| in optmgr.c for TARGET_WIN32 
| See comment below.
 -------------------------------------------------------------------------- */
        if ((sapOpt.balance == SAP_DISK_BAL) && (StrCmp(request, _T("NEW")) == 0))
        {
            int  ret;
            DbgPlain(DBG_SAP_INIT, _T("[%s] Disk balancing selected"), __FCN__);

            if ((ret = BalanceDisk () ) != 0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Disk Balance failed. Error code : %d"),
                    __FCN__,
                    ret);

                ErhFullReport(
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_SAP,
                        NLS_SAP_DISK_BALANCE_FAILED,
                        DISK_BALANCE_SCRIPT,
                        sapOpt.inFile,
                        ret));

                ErhClearError();

                argRet = CC_FAILURE;
            }

            DbgPlain(DBG_MAIN_ACTION, _T("[%s] fileOP: CLOSE"), __FCN__);

            fclose (inFile);
            if (sapOpt.inFile)
            {
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] FileOP: OPEN"),__FCN__);
                if (!(inFile = fopen(sapOpt.inFile, _T("r+"))))
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] fopen(%s, \"r\") failed. Error: %s"),
                        __FCN__,
                        sapOpt.inFile,
                        ErhSysErrnoToText(errno));

                    ErhFullReport (
                        __FCN__,
                        ERH_CRITICAL,
                        NlsGetMessage (
                            NLS_SET_SAP,
                            NLS_SAP_OPEN_FILE_FAIL,
                            sapOpt.inFile) );

                    DbgFcnOut();
                    return (CC_FAILURE);
                }
            }
            else
            {
                inFile = stdin;
            }
            
            prefix = _T("DISK_");
        }
        else
        {
            /* So on NT this is ALWAYS executed */
            /* We DON'T support DiskBalancing on NT!!! */
            prefix = _T("BAL_");
        }

/* ---------------------------------------------------------------------------
|
|   Prepare tmpFile structure for backup
|   Create a unique file name for contents of each tmpFile element
|   If the file can not be created, exit with failure.
|
-------------------------------------------------------------------------- */
        for (i=0; i<sapOpt.conc; i++)
        {
            tmpFile[i].name    = OB2_GetTempFileName(cmnPanTmp, _T("sapb"), NULL);
            tmpFile[i].count   = 0;
            tmpFile[i].total   = 0;
            tmpFile[i].barName = NULL;
            tmpFile[i].backID  = NULL;
            tmpFile[i].files   = NULL;
        }
        
/* ---------------------------------------------------------------------
|      According to the balance type distribute files to backup devices
--------------------------------------------------------------------- */
        {
            int exit_v = 0;
            switch (sapOpt.balance) 
            {
            case SAP_LOAD_BAL:                 
                exit_v = BalanceLoad(
                    sapOpt.conc,
                    sapOpt.balance,
                    &totalFiles);
                break;

            case SAP_TIME_BAL:
                exit_v = BalanceLoad(
                    sapOpt.conc,
                    sapOpt.balance,
                    &totalFiles);
                break;

/* ---------------------------------------------------------------------------
|               The following line of code is unreachable on NT 
|               because we don't support this type of balancing 
 -------------------------------------------------------------------------- */
            case SAP_DISK_BAL:
            case SAP_MANUAL_BAL:
                exit_v = BalanceManual(
                    sapOpt.conc,
                    sapAdd,
                    barlist,
                    &totalFiles);
                break;
            default:
                exit_v = BalanceBasic(sapOpt.conc, &totalFiles);
                break;
            }
            
/* ---------------------------------------------------------------------------
|  do we have a balancing problem? 
 -------------------------------------------------------------------------- */
            if (exit_v) 
            {
                ErhFullReport(
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_SAP,
                        NLS_SAP_BALANCING_FAILED));

                ErhClearError();

                SapFreeMemory (sapOpt.conc);
                
                DbgFcnOut();
                return (exit_v);
            }
        } /*before switch*/

        DbgPlain (DBG_SAP_INIT, _T(" "));
        DbgPlain (DBG_SAP_INIT, _T("[%s] Files :"), __FCN__);
        totCount = 0;
        totFsize = 0.0;
        for (i=0; i<sapOpt.conc; i++)
        {
            totCount += tmpFile[i].count;
            totFsize += tmpFile[i].total;
            todo     += tmpFile[i].count;
            
            DbgPlain (DBG_SAP_INIT,
                _T("[%s] %d  %08d  %lld"),
                __FCN__,
                i,
                tmpFile[i].count,
                tmpFile[i].total);
        }
        DbgPlain (DBG_SAP_INIT, _T(" "));
        DbgPlain (DBG_SAP_INIT,
            _T("[%s] Files: %d, Sum: %ld, avgSize: %12.2f, avgPerDev: %10.2f"),
            __FCN__,
            totCount, 
            (unsigned long) totFsize,
            totFsize/totCount,
            totFsize/sapOpt.conc);

  
#if !TARGET_WIN32
    SetCtrlCHandler();
#endif
  
/* ---------------------------------------------------------------------------
|   Creating mutex on WIN32 or semaphore file on other platforms
 --------------------------------------------------------------------------- */
    if (mode == FILE_ONLINE)
    {
        DbgStamp (DBG_SAP_FLOW);
        DbgPlain (DBG_SAP_FLOW, _T("[%s] Creating mutex/semaphore..."), __FCN__);
        if (CreateSemOrMutex()) 
        {
            SapFreeMemory (sapOpt.conc);

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] Error: mutex/semaphore creation failed!"),
                __FCN__);

            DbgFcnOut();
            return (CC_FAILURE);
        }
    }
      
/* ---------------------------------------------------------------------------
|   If balance type is time balancing allocate speed_list_ptr
 --------------------------------------------------------------------------- */
    if (sapOpt.balance == SAP_TIME_BAL)
    {
        slp       = MALLOC (sizeof(speedList));
        slp->n    = 0;
        slp->ptr  = CALLOC(totalFiles+1, sizeof(speedRec));
    }
    
/* ----------------------------------------------------------------------
|    Initiate subprocesses and select() on fd's serving I/O events
 ---------------------------------------------------------------------- */
    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW, 
        _T("[%s] Initiating subprocesses and selecting on fd's serving I/O events\n"),
        __FCN__);
#if !TARGET_WIN32
    if (interrupt) /* impossible? on NT */
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] Backint interrupt!"), 
            __FCN__);

        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_BACKINT_INT) );

        if (mode == FILE_ONLINE)
        {
            RemoveSemOrMutex();
        }
        
        if (sapOpt.balance == SAP_TIME_BAL)
        {
            FREE (slp->ptr);
            FREE (slp);
        }
        
        SapFreeMemory (sapOpt.conc);

        DbgFcnOut();
        return (CC_FAILURE);
    }
#endif
    
    for (i=0; i<sapOpt.conc; i++)
    {
#if !TARGET_WIN32
        int oldmask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
#endif

        if (tmpFile[i].count > 0)
        {
/* -------------------------------------------------------
|         Form command line to start sapback 
 -------------------------------------------------------*/
            tchar    *cmnd;
            int       size;
            int       j;

            size  = StrLen(sapOpt.backProg);
            /* 2 chars will be counted for %s in SAPBACK_FMT */
            size += StrLen(sapOpt.userID)-2; 
            /* for setID we will allocate 2 chars by %d */
            size += StrLen(tmpFile[i].name)-2;
            size += StrLen(SAPBACK_FMT)+3;
            if (mode!=BACKUP)
            {
                size += StrLen(semFile) + 1;
            }

/* ---------------------------------------------------------------------------
|       Creating files contatining lists of files to be backed up
|       One tmpFile entry corresponds to one sapback process
 -------------------------------------------------------------------------- */
            /* Now let's create */
            DbgStamp (DBG_SAP_FLOW);
            DbgPlain (DBG_SAP_FLOW, 
                _T("[%s] Making filelist %s"), 
                __FCN__,
                tmpFile[i].name);

            if (!(tmpFile[i].F = fopen(tmpFile[i].name, _T("w+"))))
            {
                int j;

                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] fopen(\"%s\", \"w\") failed. Error: %s"),
                    __FCN__,
                    tmpFile[i].name,
                    ErhSysErrnoToText(errno) );

                for (j=0; j<=i; j++)
                {
                    fclose(tmpFile[i].F);
                    FREE (tmpFile[j].name);
                }

                FREE (tmpFile);

                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_OPEN_FILE_FAIL,
                        ErhSysErrnoToText (errno)) );

#if !TARGET_WIN32
                umask(oldmask);
#endif
                DbgFcnOut();
                return (CC_FAILURE);
            }

            for (j=0; j<tmpFile[i].count; j++)
            {                
                DbgPlain (DBG_SAP_INFO,
                    _T("[%s] Putting %s to file %s"),
                    __FCN__,
                    NS(tmpFile[i].files[j].name),
                    NS(tmpFile[i].name));

                if (tmpFile[i].files[j].name) 
                {
                    /* TODO: do we want T2A or T2S here? */
                    /* check SetConType() for details */
                    FPrintf (tmpFile[i].F, _T("%s\n"), tmpFile[i].files[j].name);
                }
            }

            fflush(tmpFile[i].F);
            fclose(tmpFile[i].F);
            DbgPlain (DBG_SAP_FLOW, _T("[%s] OK"), __FCN__);
/* ---------------------------------------------------------------------------
        Allocating space for command line to start sapback process 
 -------------------------------------------------------------------------- */
            DbgStamp (DBG_SAP_FLOW);
            DbgPlain (DBG_SAP_FLOW, _T("[%s] Creating command for sapback..."), __FCN__);

            cmnd = MALLOC (size * sizeof(tchar));
            
            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] Backup data mover is \"%s\""),
                __FCN__,
                sapOpt.backProg);
            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] Size of allocated space is %d"), 
                __FCN__, 
                size);

            tmpFile[i].setNumber = NextSetNumber(OB2_GetSessionGuid(NULL));
            sapAdd = tmpFile[0].setNumber;
            sprintf (
                cmnd, 
                SAPBACK_FMT,
                sapOpt.backProg, 
                sapOpt.userID, 
                tmpFile[i].setNumber,
                tmpFile[i].name,
                (mode!=BACKUP) ? semFile : _T(""));

            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] Target length should be %ld, while the allocated size is %d"),
                __FCN__,
                StrLen(cmnd),
                size );
            
            DbgPlain (DBG_SAP_FLOW, 
                _T("[%s] Starting %d: '%s'"), 
                __FCN__, 
                i+sapAdd, 
                cmnd);
            
            
            if (!(tmpFile[i].F = popen(cmnd, _T("r"))))
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] popen(cmnd=%s, \"r\") failed. Error: %s"),
                    __FCN__, 
                    cmnd, 
                    ErhSysErrnoToText(errno));
                                
                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_POPEN_FAIL,
                        ErhSysErrnoToText (errno)) );
            }

            DbgStamp (DBG_SAP_FLOW);
            DbgPlain (DBG_SAP_FLOW, _T("[%s] Agent started..."), __FCN__);

            tmpFile[i].fd = fileno(tmpFile[i].F);
            sleep (2);
            FREE (cmnd);
        }

#if !TARGET_WIN32
        umask(oldmask);
#endif
    }
    
    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW, _T("[%s] Entering wait loop"), __FCN__);
/* ---------------------------------------------------------------------------
CR    
CR  General remark - we could use IPC library here for communication 
CR  between processes. I would not change this at the moment since this 
CR  is planned to be changed for data mover 
CR   
 -------------------------------------------------------------------------- */
    {
#if TARGET_WIN32
        int doSleep = 1;
#endif
        long sumSpeed = 0;

        do
        {
#if !TARGET_WIN32
/* Wait for new data on UX */
            fd_set    rfds;
            
            FD_ZERO(&rfds);
            for (i=0; i<sapOpt.conc; i++)
            {
                if (tmpFile[i].F) 
                {
                    FD_SET(tmpFile[i].fd, &rfds);
                }
            }
                
            if (select (200, &rfds, NULL, NULL, 0) < 0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] select(200, &rfds, NULL, NULL, 0) failed. Error: %s"),
                    __FCN__, 
                    ErhSysErrnoToText(errno));
            }
#else
/* Simply sleep for 1 second on NT in case we didn't get any new data on the previous loop pass */
            if (doSleep) 
            {
                sleep(1); 
            }
#endif
                
            for (i=0; i<sapOpt.conc; i++)
            {
                if (!tmpFile[i].F) /* if file is [already] closed => skip it */
                {
                    continue;
                }
#if !TARGET_WIN32
                /* if file has new data => clear it's flag*/
                if (FD_ISSET(tmpFile[i].fd, &rfds))
                {
                    FD_CLR(tmpFile[i].fd, &rfds);
                }
                else /* skip it */
                {
                    continue;
                }
#endif
                
                if (!feof(tmpFile[i].F) && !ferror(tmpFile[i].F) && fgets (path, STRLEN_1K, tmpFile[i].F) && *path)
                {
                    tchar *c = StrrChr (path, _T('\n')); if (c) *c = _T('\0');

                    DbgStamp (DBG_SAP_FLOW);
                    DbgPlain (DBG_SAP_FLOW, 
                        _T("[%s] Got from subprocess %d: \"%s\""),__FCN__,
                        i,
                        path);
                    
                    switch (path[0])
                    {
                    case _T('s'):
                        /* Got session ID from sapback */
                        {
                            tchar    backID[STRLEN_SESSIONID+1];
                            
                            if (SapSetVer2BackID (
                                    path+2, 
                                    tmpFile[i].setNumber, 
                                    backID)!=CC_SUCCESS)
                            {
                                DbgStamp (DBG_UNEXPECTED);
                                DbgPlain (DBG_UNEXPECTED,
                                    _T("[%s] Conversion session name => backID failed"),
                                    __FCN__);

                                ErhFullReport (
                                    __FCN__,
                                    ERH_MAJOR,
                                    NlsGetMessage (
                                        NLS_SET_SAP,
                                        NLS_SAP_VER2BACKID_FAILED,
                                        path+2,
                                        i+sapAdd) );

                                ErhClearError();
                            }
                            
                            tmpFile[i].backID = StrNewCopy (backID);
                            DbgPlain(DBG_SAP_FLOW, 
                                _T("[%s] Parsed session name from %d => \"%s\""),
                                __FCN__,
                                i, 
                                path+2);
                            break;
                        }
                    case _T('b'):
                        /* Got object name from sapback */

                        tmpFile[i].barName = StrNewCopy (path+2);
                        DbgPlain(DBG_SAP_FLOW, 
                            _T("Parsed barname from %d => \"%s\""), 
                            i, 
                            path+2);
                        break;
                        
                    case _T('+'):
                        /* File, following '+' backed up successfully */
                        {
                            tchar *fileName;
                            tchar *fileSpeed;
                            
                            fileName  = StrTok(path+2, _T(" "));
                            fileSpeed = StrTok(NULL,   _T(" "));
                            
                            DbgPlain (DBG_SAP_INFO, _T("Filename : %s, FileSpeed : %s"), fileName, fileSpeed);

                            if (sapOpt.logFile)
                            {
                                FILE *fd = fopen(sapOpt.logFile, _T("a+"));
                                if (fd != NULL)
                                {
                                    fprintf(fd, _T("%s %s\n"), tmpFile[i].backID, path+2);
                                    fclose(fd);
                                }
                                else
                                {
                                    ErhFullReport (
                                        __FCN__,
                                        ERH_WARNING,
                                        NlsGetMessage (
                                            NLS_SET_ERRNO,
                                            ERP_LOGFAILED,
                                            sapOpt.logFile) );
                                }
                            }

/* ---------------------------------------------------------------------------
CR    
CR   This is actuall communication with BRTOOLS, I think it should be 
CR   documented. 
CR   
-------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------
AK
AK   It is documented in a separate document from SAP, complete documentation
AK   here would just mess up the code, and it's almost obvious what are these
AK   keywords stay for.
AK
-------------------------------------------------------------------------- */
                            fprintf (outFile,
                                _T("#SAVED %s %s\n"),
                                tmpFile[i].backID,
                                fileName);
                            fflush (outFile);

                            DbgPlain(DBG_SAP_FLOW,
                                _T("[%s] Parsed success and put the following to outFile:\n#SAVED %s %s"),
                                __FCN__,
                                tmpFile[i].backID,
                                fileName);
                            
                            done += 1;
                            
/* ---------------------------------------------------------------------------
    If we run a Time balanced backup, we have to save new information
    about time needed to backup the file, which will later be saved
    back to the configuration file on the CM
-------------------------------------------------------------------------- */
                            if (sapOpt.balance == SAP_TIME_BAL)
                            {
                                sumSpeed += AddSpeed(slp, fileName, atol(fileSpeed));
                            }
                            break;
                        }

                    case _T('-'):
                        /* Backup of the file following '-' failed */
                        fprintf (outFile, _T("#ERROR %s\n"), path+2);
                        fflush (outFile);

                        DbgPlain (DBG_SAP_FLOW,
                            _T("[%s] Parsed failure and put the following to outFile:\n#ERROR %s"), 
                            __FCN__,
                            path+2);
                        break;

                    case _T('e'):
                    case _T('v'):
                        /* Anything else we get from sapback should be treated
                           as "extended info", i.e. put to the output */

                        if (StrLen(path)<3) 
                        {
                            /* Sometimes sapback sends just empty lines with \n */
                            break;
                        }

                        fprintf (outFile, _T("%s\n"), path+2);
                        fflush (outFile);

                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed extended info and put the following to outFile:\n%s"),
                            __FCN__,
                            path+2);
                        break;

                    default:
                        if (StrLen(path)<3) 
                        {
                            /* Sometimes sapback sends just empty lines with \n */
                            break;
                        }

                        fprintf (outFile, _T("%s\n"), path);
                        fflush (outFile);

                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed extended info and put the following to outFile:\n%s"),
                            __FCN__,
                            path);
                        break;
                    }
#if TARGET_WIN32
                    doSleep = 0;
#endif
                }
#if TARGET_WIN32
                else
                {
                    doSleep = 1;
                }
#endif
                
                /* if something wrong with the comm file we close the pipe */
                if (feof(tmpFile[i].F) || ferror(tmpFile[i].F))
                {
                    if (pclose (tmpFile[i].F) == -1)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] Error closing pipe to process #%d. Handle : %p, Error : %s"),
                            __FCN__,
                            i,
                            tmpFile[i].F,
                            ErhSysErrnoToText(errno) );

                        ErhFullReport (
                            __FCN__,
                            ERH_WARNING,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_PCLOSE_FAILED,
                                ErhSysErrnoToText(errno)) );
                    }
                    tmpFile[i].F = NULL;
                }
            }

            /* Count how many of them completed */
            for (i=0; i<sapOpt.conc && !tmpFile[i].F; i++);
                
        } while (i < sapOpt.conc);

        if (sapOpt.balance == SAP_TIME_BAL)
        {
            slp->ptr[slp->n].name  = StrNewCopy(_T("AVERAGE"));
            slp->ptr[slp->n].speed = sumSpeed?(long) (sumSpeed/totalFiles):1;
            slp->n++;
        }
    }

#if !TARGET_WIN32
    if (interrupt) 
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Backint interrupt!"), __FCN__);
        
        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_BACKINT_INT) );

        if (mode == FILE_ONLINE)
        {
            RemoveSemOrMutex();
        }

        if (sapOpt.balance == SAP_TIME_BAL)
        {
            FREE (slp->ptr);
            FREE (slp);
        }            

        for (i=0; i<sapOpt.conc; i++)
        {
/* ---------------------------------------------------------------------------
CR    
CR  It is not consistent, now you use OB2_* function to remove file,
CR  before you used fopen function in order to create it 
 -------------------------------------------------------------------------- */
            if (tmpFile[i].name)
            {
                OB2_DeleteFile(tmpFile[i].name);
            }
        }

        SapFreeMemory (sapOpt.conc);
        
        DbgFcnOut();
        return (CC_FAILURE);
    }    
#endif

    if (sapOpt.balance == SAP_TIME_BAL && !StrICmp(request, _T("NEW")))
    {
        int ii;
        
        DbgStamp (DBG_SAP_FLOW);
        DbgPlain (DBG_SAP_FLOW, _T("[%s] Updating speed file..."), __FCN__);
        UpdateSpeedFile (slp);
/* ---------------------------------------------------------------------------
CR    
CR  This code should be put into function and called several times 
CR   
 -------------------------------------------------------------------------- */

        for (ii=0; ii<slp->n; ii++)
        {
            FREE (slp->ptr[ii].name);
        }
        FREE (slp->ptr);
        FREE (slp);
    }

    for (i=0; i<sapOpt.conc; i++)
    {
        if (tmpFile[i].name)
        {
            OB2_DeleteFile(tmpFile[i].name);
        }
    }

    if (mode == FILE_ONLINE)
    {
        RemoveSemOrMutex();
    }

    if (OB2_EndBackup(context,OB2_GetErrno(context)))
    {
       DbgPlain (DBG_UNEXPECTED,
            _T("[%s] EndBackup Failed"),
            __FCN__);
    }

    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW, _T("[%s] Done."), __FCN__);

    DbgFcnOut();
    return (done==todo ? CC_SUCCESS : (done>0 ? CC_SOANDSO : CC_FAILURE));
}

/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     conc        maximum possible concurrency (set number)
* @param     fileID      filename, which backup ID you're looking for
*
* @retval    BackupID    In the form YYYYMMDD.SSSS.NN
*                        where Y is year, M is month, D is day of backup
*                        S is session number, and N is object/set number
*
* @brief     Searches for the latest backup id of the file specified
*
*//*=========================================================================*/
static tchar *
SapFindLatestBackID (int conc, tchar *fileID)    
{
    ERH_FUNCTION(_T("SapFindLatestBackID"));

    static tchar          backID[STRLEN_SESSIONID+1];


    DbgFcnIn();

    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW, _T("[%s] File : '%s', conc : %d"), __FCN__, fileID, conc);
  
    if (SapFindBackIDsOfFile (_T("#NULL"), fileID)==CC_SUCCESS && verNum)
    {
        DbgPlain(DBG_SAP_FLOW, _T("[%s] Latest backID is %s"),__FCN__, verTab[0]);
        strcpy(backID, verTab[0]); /* The first version is exactly what we need - the latest one */
        verNum=0;
        FREE(verTab);
        DbgFcnOut();
        return(backID);
    }
    else
    {
        DbgPlain (DBG_SAP_FLOW, 
            _T("[%s] Not found!"), 
            __FCN__);

        DbgFcnOut();
        return (NULL);
    }

}



/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     i           list index (inside of tmpFile array)
* @param     fileName
* @param     targetName  Target file name for restore, can be NULL
*
* @retval    N/A
*
* @brief     Reallocates memory if needed and adds fileName to the tmpFile[i]
*            list of files (that will be backed up/restored by a single
*            sapback/saprest process)
*
*//*=========================================================================*/
void AddFileToList (int i, tchar *fileName, tchar *targetName)
{
    ERH_FUNCTION(_T("AddFileToList"));

    DbgFcnIn();

    DbgStamp (DBG_SAP_FLOW);
    
    if ((tmpFile[i].count%ALLOC_SIZE)==0)
    {
        
        DbgPlain (DBG_SAP_INFO, 
            _T("[%s] Expanding list for device %d new size %d elements\n"),
            __FCN__, 
            i, 
            tmpFile[i].count+ALLOC_SIZE);

        tmpFile[i].files = (filePtr)REALLOC(tmpFile[i].files, (tmpFile[i].count+ALLOC_SIZE)*sizeof(fileRec));
        memset(&tmpFile[i].files[tmpFile[i].count], 0, ALLOC_SIZE*sizeof(fileRec));
    }

    DbgPlain(DBG_SAP_FLOW,
        _T("[%s] Adding element %s to position %d of list %d"),
        __FCN__,
        fileName,
        tmpFile[i].count,
        i);

    tmpFile[i].files[tmpFile[i].count].name = StrNewCopy(fileName);
    if (targetName) 
        tmpFile[i].files[tmpFile[i].count].target = StrNewCopy(targetName);
    tmpFile[i].count++;
    DbgFcnOut();
}






/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     N/A
*
* @retval    succes code
*
* @brief     This function obtains all necessary data for restore of files
*            specified in inFile, starts saprest processes, communicates
*            with them and reports statuses to BRRESTORE.
*
*//*=========================================================================*/
static int
SapRestoreFiles (void)
{
    ERH_FUNCTION(_T("SapRestoreFiles"));
    int        dev;
    int        i, j;
    int        p, q;
    int        todo = 0;
    int        done = 0;
    tchar      path[STRLEN_1K+1];

    DbgFcnIn();

/* ---------------------------------------------------------------------------
|   Partition input filenames according to backID criteria
 -------------------------------------------------------------------------- */
    tmpFile = (struct extFile *)MALLOC(MAX_SAP_CONC * sizeof(struct extFile));
    memset(tmpFile,0,MAX_SAP_CONC * sizeof(struct extFile));
  
    for (dev=0; fgets (path, STRLEN_1K, inFile); )
    {
        tchar    *backID;
        tchar    *fileID;
        tchar    *target;
        tchar    *fileIDslashed;
      
        { tchar *c = StrrChr (path, _T('\n')); if (c) *c = _T('\0'); }
      
        DbgStamp (DBG_SAP_FLOW);
        DbgPlain (DBG_SAP_FLOW, _T("[%s] in: %s"), __FCN__, path);
      
        backID = StrTok (path, _T(" \t\n"));
        fileID = StrTok (NULL, _T(" \t\n"));
        target = StrTok (NULL, _T(" \t\n"));
      
        fileIDslashed = NT2OB_FileName (fileID);
        DbgPlain (DBG_SAP_INFO, 
            _T("backID=%s, fileID=%s, target=%s"),
            backID,
            fileID,
            (target ? target : _T("NULL")) );

/* ---------------------------------------------------------------------------
    If BackupID wasn't specified in inFile, we'll restore the latest version
    which BackupID we'll get from SapFindLatestBackID()
 -------------------------------------------------------------------------- */
        if (!StrICmp (backID, _T("#NULL")))
        {
            if ((backID=SapFindLatestBackID (MAX_SAP_CONC, fileID)) == NULL)
            {
                DbgPlain (DBG_SAP_INFO, 
                    _T("[%s] #NULL backID: %s => ignored !!"),
                    __FCN__,
                    fileID);

                fprintf (outFile, _T("#NOTFOUND %s\n"), fileID);
                fflush (outFile);
              
                FREE (fileIDslashed);
                continue;
            }
            else
            {
                DbgPlain(DBG_SAP_INFO, _T("[%s] Restoring from backID=%s"), __FCN__, backID);
            }
        }
        else
        {
            int      index;
            tchar    setVer[STRLEN_SESSIONID+1];

            if (SapBackID2SetVer (backID, setVer, &index)!=CC_SUCCESS)
            {
                DbgPlain (DBG_SAP_INFO, 
                    _T("[%s] #NULL backID: %s => ignored !!"),
                    __FCN__,
                    fileID);

                ErhFullReport (
                    __FCN__,
                    ERH_MAJOR,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_BACKID2VER_FAILED,
                        backID) );

                ErhClearError();

                fprintf (outFile, _T("#NOTFOUND %s\n"), fileID);
                fflush  (outFile);
                
                FREE (fileIDslashed);
                continue;
            }
        }

/* ---------------------------------------------------------------------------
CR    
CR  Comment missing 
CR   
 -------------------------------------------------------------------------- */
        for (i=0; i<dev; i++)
        {
            DbgPlain (DBG_SAP_INFO,
                _T("[%s] SapRF: i=%d, dev=%d, tmpFile[i].backID=%s"),
                __FCN__,
                i, 
                dev, 
                tmpFile[i].backID);
          
            if (!StrCmp (tmpFile[i].backID, backID))
            {
                break;
            }
        }
      
        if (i == dev)
        {
#if !TARGET_WIN32
            int oldmask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
#endif
            DbgPlain (DBG_SAP_INFO, 
                _T("[%s] New backID: %s, dev=%d"),
                __FCN__,
                backID, 
                dev+1);

            if ((i != 0) && (i % MAX_SAP_CONC == 0))
            {                
                DbgPlain (DBG_SAP_FLOW, 
                    _T("[%s] Reallocating tmpFile size %d"), 
                    __FCN__, 
                    (i/MAX_SAP_CONC)+1);

                tmpFile = (struct extFile *)REALLOC(tmpFile, ((i/MAX_SAP_CONC)+1)*MAX_SAP_CONC * sizeof(struct extFile));
            }
          
            tmpFile[dev].count = 0;
            tmpFile[dev].name  = OB2_GetTempFileName(cmnPanTmp, _T("sapr"), NULL);

            if (!(tmpFile[dev].F = fopen(tmpFile[dev].name, _T("w"))))
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] fopen(tmpFile[dev].name=%s, \"r\") failed. Error: %s"),
                    __FCN__, 
                    tmpFile[dev].name, 
                    ErhSysErrnoToText(errno));
                              
                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_OPEN_FILE_FAIL,
                        ErhSysErrnoToText (errno)) );

#if !TARGET_WIN32
                umask(oldmask);
#endif
                DbgFcnOut();
                return (CC_FAILURE);
              
            }
            fclose(tmpFile[dev].F);
            tmpFile[dev].backID    = StrNewCopy (backID);
            tmpFile[dev].barName   = NULL;
            dev += 1;

#if !TARGET_WIN32
            umask(oldmask);
#endif
        }
      
        DbgPlain (DBG_SAP_FLOW, 
            _T("[%s] out: %s %s -> %s"),
            __FCN__, 
            backID, 
            fileID, 
            (target ? target : _T("")) );
      
        AddFileToList(i, fileID, target);
    }
  
    for (i=0; i<dev; i++)
    {
        todo += tmpFile[i].count;
      
        DbgPlain (DBG_SAP_INFO, 
            _T("[%s] stat: index: %d  files: %04d"),
            __FCN__,
            i, 
            tmpFile[i].count);
    }
  
/* ---------------------------------------------------------------------------
|     We should serialize the individual restores to avoid session clashes.
|     The following algorithm is based on the assumption that the backup was
|     performed with concurrency=1 (no tape multiplexing).
 ---------------------------------------------------------------------------*/

/* --- Sort tmpFile table using the backID field as the sort key ---------- */

    DbgPlain (DBG_SAP_FLOW, 
        _T("[%s] Sorting tmpFile table ..."), 
        __FCN__);
  
    for (i=0; i<dev-1; i++)
    {
        for (j=i+1; j<dev; j++)
        {
            if (StrCmp(tmpFile[j].backID, tmpFile[i].backID) < 0)
            {
                struct extFile tmp;
                tmp        = tmpFile[i];
                tmpFile[i] = tmpFile[j];
                tmpFile[j] = tmp;                
            }
        }
    }
  
    DbgPlain (DBG_SAP_FLOW, 
        _T("[%s] ... sort completed, dev=%d"), 
        __FCN__, 
        dev);

    for (i=0; i<dev; i++)
    {
#if TARGET_WIN32
        tchar tmp_file_path[256] = {'\0'};
        FILE *stream;
        sprintf (tmp_file_path, _T("%s_%d"),tmpFile[i].name, atoi(strrchr(tmpFile[i].backID,_T('.'))+1 ));
        stream = fopen(tmp_file_path, _T("w"));
        fclose (stream);
#endif

        DbgPlain (DBG_SAP_INFO, 
            _T("[%s] tmpFile[%d] = %s"),
            __FCN__,
            i, 
            tmpFile[i].backID);
    }
  
    for (p=0; p<dev; p=q)
    {
#if TARGET_WIN32
        /* used for determining whether we have to sleep(1) during the next cycle */
        int doSleep = 1; 
#endif
      
/* ---------------------------------------------------------------------------
|  --- Start as many parallel saprest's (same backID) as possible --- 
 -------------------------------------------------------------------------- */

        for (
            q=p;
            q<dev && !BackIdCompare(tmpFile[p].backID, tmpFile[q].backID);
            q++)
            {
                if (tmpFile[q].count > 0)
                {
                    int      index;
                    tchar    setVer[STRLEN_SESSIONID+1];
                    tchar   *cmnd;
                    int      size;
              
                    if (SapBackID2SetVer (tmpFile[q].backID, setVer, &index)!=CC_SUCCESS)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] Conversion backID => session name failed"),
                            __FCN__);

                        tmpFile[q].F = NULL;

                        ErhFullReport (
                            __FCN__,
                            ERH_MAJOR,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_BACKID2VER_FAILED,
                                tmpFile[q].backID) );

                        ErhClearError();
                    
                        continue;
                    }
                
                    size = StrLen(sapOpt.restProg)-2 + /* all "-2" correspond to %s in SAPREST_FMT */
                           StrLen(sapOpt.userID)-2 +
                           StrLen(tmpFile[q].name)-2 +
                           StrLen(setVer)-2 +
                           StrLen(SAPREST_FMT) +1;
              
                    cmnd = (tchar*) MALLOC (size * sizeof(tchar));
                    sprintf (
                        cmnd, 
                        SAPREST_FMT,
                        sapOpt.restProg, 
                        sapOpt.userID, 
                        index, 
                        setVer, 
                        tmpFile[q].name);

                    DbgPlain (DBG_SAP_INFO,
                        _T("[%s] Length of actual string is %ld, allocated %d"),
                        __FCN__,
                        StrLen(cmnd),
                        size);
              
                    DbgStamp (DBG_SAP_FLOW);
                    DbgPlain (DBG_SAP_FLOW, 
                        _T("[%s] Starting %d: '%s'"), 
                        __FCN__, 
                        q, 
                        cmnd);

                    if ((tmpFile[q].F = fopen(tmpFile[q].name, _T("w"))) == NULL)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                            _T("[%s] fopen(\"%s\") failed"), 
                            __FCN__, 
                            tmpFile[q].name);
                    
                        ErhFullReport (
                            __FCN__,
                            ERH_CRITICAL,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_OPEN_FILE_FAIL,
                                tmpFile[q].name) );

                        break;
                    }

                    DbgStamp(DBG_SAP);

                    for (j=0; j<tmpFile[q].count; j++)
                    {
                        DbgPlain (DBG_SAP_INFO,
                                  _T("[%s] Putting %s as target \"%s\" to file %s"),
                                  __FCN__,
                                  NS(tmpFile[q].files[j].name),
                                  NS(tmpFile[q].files[j].target),
                                  NS(tmpFile[q].name));

                        if (tmpFile[q].files[j].name) 
                        {
                            fprintf(tmpFile[q].F, _T("%s"), tmpFile[q].files[j].name);
                            if (tmpFile[q].files[j].target)
                            {
                                fprintf(tmpFile[q].F, _T(" %s"), tmpFile[q].files[j].target);
                            }
                            fprintf(tmpFile[q].F, _T("\n"));
                        }
                    }
                
                    fclose(tmpFile[q].F);
                    DbgPlain (DBG_SAP_FLOW, _T("[%s] OK"), __FCN__);

                    DbgPlain (DBG_SAP_FLOW, 
                              _T("[%s] Starting %d: '%s'"), 
                              __FCN__, 
                              index, 
                              cmnd);

                    if (!(tmpFile[q].F  = popen(cmnd, _T("r"))))
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] popen(cmnd=%s, \"r\") failed. Error: %s"),
                            __FCN__,
                            cmnd, 
                            ErhSysErrnoToText(errno));
                                      
                        ErhFullReport (
                            __FCN__,
                            ERH_CRITICAL,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_POPEN_FAIL,
                                ErhSysErrnoToText (errno)) );
                    }
                  
                    DbgStamp (DBG_SAP_FLOW);
                    DbgPlain (DBG_SAP_FLOW, _T("[%s] Agent started..."), __FCN__);

                    tmpFile[q].fd = fileno(tmpFile[q].F);
                    sleep (2);
              
                    FREE (cmnd);
                }
            }
/* --------------------------------------------------------------------------
    Here we have number of processes started in variable q
    and tmpFile[i].F is an open pipe to a corresponding saprest process
 ---------------------------------------------------------------------------*/

/* --- Now serve subprocess' I/O requests until all finished -------------- */

        DbgStamp (DBG_SAP_FLOW);
        DbgPlain (DBG_SAP_FLOW, 
            _T("[%s] Entering select() loop ..."), 
            __FCN__);

        do
        {
#if !TARGET_WIN32
/* Wait for new data on UX */
            fd_set    rfds;
          
            FD_ZERO(&rfds);
            for (i=p; i<q; i++)
            {
                if (tmpFile[i].F) 
                {
                    FD_SET(tmpFile[i].fd, &rfds);
                }
            }
          
            if (select (ALLOC_SIZE, &rfds, NULL, NULL, 0) < 0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] select(ALLOC_SIZE, &rfds, NULL, NULL, 0) failed. Error: %s"),
                    __FCN__, 
                    ErhSysErrnoToText(errno));

                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_STATEMENT_FAILED,
                        _T("select()")) );

                break;
            }
#else
            /* Simply sleep for 1 second on NT*/
            if (doSleep)
            {
                sleep(1);
            }
#endif
            for (i=p; i<q; i++)
            {
                tchar *c = NULL;
                tchar tmp_file_path[256] = {'\0'};
                /* if file is [already] closed => skip it */
                if (!tmpFile[i].F) 
                {
                    continue;
                }
#if !TARGET_WIN32
                /* if file has new data => clear it's flag*/
                if (FD_ISSET(tmpFile[i].fd, &rfds))
                {
                    FD_CLR(tmpFile[i].fd, &rfds);
                }
                else /* skip it */
                {
                    continue;
                }
#endif

#if TARGET_WIN32
                sprintf (tmp_file_path, _T("%s_%d"),tmpFile[i].name, atoi(strrchr(tmpFile[i].backID,_T('.'))+1 ));               
                doSleep = 1;
#endif
                if (!feof(tmpFile[i].F) && !ferror(tmpFile[i].F) && fgets (path, STRLEN_1K, tmpFile[i].F) && *path)
                {

                    #if TARGET_WIN32
                    if (GetFileAttributes( tmp_file_path ) == INVALID_FILE_ATTRIBUTES)
                        doSleep = 0;
                    #endif
                  
                    c = StrrChr (path, _T('\n')); if (c) *c = _T('\0');

                    DbgPlain(DBG_SAP_FLOW,
                        _T("[%s] Got from subprocess %d: \"%s\""),
                        __FCN__,
                        i,
                        path);
/* ---------------------------------------------------------------------------
CR    
CR  Add comments which will describe the interface
CR   
-------------------------------------------------------------------------- */
                    switch (path[0])
                    {
                    case _T('s'): 
                        /* Got session ID from saprest */
                        DbgPlain(DBG_SAP_FLOW, 
                            _T("[%s] Parsed session name from %d => \"%s\""),
                            __FCN__,
                            i,
                            path+2);
                        break;
                    case _T('b'):
                        /* Got object name from sapback */
                        DbgPlain (DBG_SAP_FLOW, 
                            _T("[%s] Parsed barname from %d => \"%s\""),
                            __FCN__,
                            i,
                            path+2);
                        break;
                      
                    case _T('+'):
                        /* File, following '+' restored successfully */
                        fprintf (outFile,
                            _T("#RESTORED %s %s\n"),
                            tmpFile[i].backID,
                            path+2
                            );
                        fflush (outFile);
                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed success and put the following to outFile:\n#RESTORED %s %s"),
                            __FCN__,
                            tmpFile[i].backID,
                            path+2);
                        done += 1;
                        break;
                      
                    case _T('-'):
                        /* Restore of the file following '-' failed */
                        fprintf (outFile, _T("#ERROR %s\n"), path+2);
                        fflush (outFile);
                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed failure and put the following to outFile:\n#ERROR %s"),
                            __FCN__,
                            path+2);
                        break;
                      
                    case _T('?'):
                        fprintf (outFile, _T("#NOTFOUND %s\n"), path+2);
                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed 'not found' and put the following to outFile:\n#NOTFOUND %s"),
                            __FCN__,
                            path+2);
                        fflush (outFile);
                        break;

                    case _T('e'):
                    case _T('v'):
                        /* Anything else we get from sapback should be treated
                           as "extended info", i.e. put to the output */

                        if (StrLen(path)<3) 
                        {
                            /* Sometimes sapback sends just empty lines with \n */
                            break;
                        }

                        fprintf (outFile, _T("%s\n"), path+2);
                        fflush (outFile);

                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed extended info and put the following to outFile:\n%s"),
                            __FCN__,
                            path+2);
                        break;

                    default:
                        if (StrLen(path)<3) 
                        {
                            /* Sometimes sapback sends just empty lines with \n */
                            break;
                        }

                        fprintf (outFile, _T("%s\n"), path);
                        fflush (outFile);

                        DbgPlain(DBG_SAP_FLOW,
                            _T("[%s] Parsed extended info and put the following to outFile:\n%s"),
                            __FCN__,
                            path);
                        break;
                    }

                }

                if (feof(tmpFile[i].F) || ferror(tmpFile[i].F))
                {
                    if (pclose (tmpFile[i].F) == -1)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] Error closing pipe. Handle : %p, Error : %s"),
                            __FCN__,
                            tmpFile[i].F,
                            ErhSysErrnoToText(errno) );

                        ErhFullReport (
                            __FCN__,
                            ERH_WARNING,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_PCLOSE_FAILED,
                                ErhSysErrnoToText(errno)) );
                    }
                    tmpFile[i].F = NULL;
                }
            }
          
/* ----------------------------------------------------------------------------
    Count finished saprest processes. 
    q is total number of started saprest processes
 ----------------------------------------------------------------------------*/
            for (i=p; i<q && !tmpFile[i].F; i++);

        } while (i < q);
        /* while number of closed pipes is greater than number of started processes */
      
        DbgPlain (DBG_SAP_FLOW, 
            _T("[%s] Exiting select() loop ..."), 
            __FCN__);
    }
  
    DbgStamp (DBG_SAP_STATUS);
    DbgPlain (DBG_SAP_STATUS, 
        _T("[%s] All restore processing completed\n"),
        __FCN__);
  
    DbgStamp (DBG_SAP_INFO);
    DbgPlain (DBG_SAP_INFO, _T("[%s] Cleaning up ..."), __FCN__);
    for (i=0; i<dev; i++)
    {
        if (tmpFile[i].name) 
        {
            DbgPlain (DBG_SAP_INFO, 
                _T("[%s] Deleting %s..."), 
                __FCN__, 
                tmpFile[i].name);

            OB2_DeleteFile(tmpFile[i].name);
        }
    }

    SapFreeMemory (dev);
  
    DbgPlain (DBG_SAP_INFO, _T("[%s] OK"), __FCN__);

    DbgFcnOut();
    return (done==todo ? CC_SUCCESS : (done>0 ? CC_SOANDSO : CC_FAILURE));
}








/* ---------------------------------------------------------------------------
|    
|    INQUIRY
|    
 -------------------------------------------------------------------------- */
/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     N/A
*
* @retval    success status (CC_SUCCESS/CC_FAILURE)
*
* @brief     Prints all backupIDs available for restore.
*
*//*=========================================================================*/
static int
SapListBackIDs ()
{
    ERH_FUNCTION(_T("SapListBackIDs"));
  
    ULONG        i, j, 
                 barNum = 0, 
                 barLen = ALLOC_SIZE;
    ULONG        count;               
    tchar      **barNames;
    tchar        barName[STRLEN_BAROBJNAME+1];
    tchar        sapSetName[STRLEN_BAROBJNAME+1];
    OB2_Object  *arrObj = NULL;
    OB2_Version *arrVer = NULL;
    int          result;
    int          retVal = CC_SUCCESS;

    DbgFcnIn();

    barNames = (tchar **)MALLOC(barLen*sizeof(tchar *));

    verNum = 0;
    verLen = ALLOC_SIZE;
    verTab = (tchar **)MALLOC(verLen*sizeof(tchar *));    

    sprintf (sapSetName, _T("%s."), sapOpt.userID);

    if ( (result = OB2_QueryObjects(context, OT_OB2BAR, OB2_APP_SAP, &count, &arrObj)) == OBE_OK)
    {
        for (i=0; i<count; i++)
        {
            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] Object name [%d] = %s, host = %s\n"),
                __FCN__,
                i,
                arrObj[i].name, 
                arrObj[i].hostname);
          
            if (StrNCmp(arrObj[i].name, sapSetName,StrLen(sapSetName)) == 0)
            {
                StrToObjectName (
                    OT_OB2BAR,
                    arrObj[i].hostname,
                    arrObj[i].name,
                    OB2_APP_SAP,
                    barName);

                DbgPlain(DBG_SAP_INFO,
                    _T("[%s] barName=%s"),
                    __FCN__,
                    barName);

                if (barNum == barLen)
                {
                    barLen   += ALLOC_SIZE;
                    barNames  = (tchar **)REALLOC(barNames, barLen*sizeof(tchar *));
                }
                barNames[barNum++] = StrNewCopy(barName);
            }
        }        
    }
    else
    {
        switch (result)
        {
        default:
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] OB2_QueryObjects exited with errors. Error : %d"),
                __FCN__,
                result);

            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage (
                    NLS_SET_SAP,
                    NLS_SAP_FAILED,
                    _T("OB2_QueryObjects")) );

            ErhClearError();
            retVal = CC_FAILURE;
        }
    }
    FREE(arrObj);
    
/* ---------------------------------------------------------------------------
CR    
CR  Add commnent
CR   
 -------------------------------------------------------------------------- */
    for (j=0; j<barNum; j++)
    {
        if ( (result = OB2_QueryObjectVersions(context, barNames[j], 0, &count, &arrVer)) == OBE_OK) 
        {
            for(i=0; i<count; i++)
            {
                tchar     backID[STRLEN_SESSIONID+1];
                tchar    *objName;

                objName = StrChr(barNames[j], _T(':'))+1;
/* ---------------------------------------------------------------------------
CR    
CR  Add decription 
CR   
 -------------------------------------------------------------------------- */
                if (SapSetVer2BackID (
                        arrVer[i].backupID, 
                        atoi(objName+StrLen(sapSetName)), 
                        backID) != CC_SUCCESS) /* !!! CHECKME !!! */
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] Conversion session name => backID failed"),
                        __FCN__);

                    ErhFullReport (
                        __FCN__,
                        ERH_MAJOR,
                        NlsGetMessage (
                            NLS_SET_SAP,
                            NLS_SAP_VER2BACKID_FAILED,
                            arrVer[i].backupID,
                            atoi(objName+StrLen(sapSetName))) );

                    ErhClearError();

                    retVal = CC_FAILURE;

                    continue;
                }
                  
                DbgPlain (DBG_SAP_INFO,
                    _T("[%s] BackID=%s,setID=%d, Str is \"%s\""),
                    __FCN__,
                    backID,
                    atoi(objName+StrLen(sapSetName)),
                    objName+StrLen(sapSetName) );
                  
                if (verNum == verLen)
                {
                    verLen += ALLOC_SIZE;
                    verTab  = (tchar **)REALLOC(verTab, verLen*sizeof(tchar *));
                }
                verTab[verNum++] = StrNewCopy (backID);
            }            
        }
        else
        {
            switch (result)
            {
            default:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] OB2_QueryObjectVersions exited with errors. Error : %d"),
                    __FCN__,
                    result);

                ErhFullReport (
                    __FCN__,
                    ERH_WARNING,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_EXIT_WITH_ERRORS,
                        _T("OB2_QueryObjectVersions")) );

                ErhClearError();

                retVal = CC_FAILURE;
            }
        }
        OB2_FreeArrayOfVersions(count, &arrVer);
    }

/* ---------------------------------------------------------------------------
CR    
CR  Make a function here
CR   
 -------------------------------------------------------------------------- */
    for (j=0; j<barNum; j++)
    {
        FREE(barNames[j]);
    }
    FREE(barNames);
    barNames = NULL;

    DbgStamp(DBG_SAP_FLOW);

/* ---------------------------------------------------------------------------
|  Sort and output backID verTab
 -------------------------------------------------------------------------- */
    if (verNum)
      for (i=0; i<verNum-1; i++)
      {
          for (j=i+1; j<verNum; j++)
          {
              if (StrCmp (verTab[i], verTab[j]) < 0)
              {
                  tchar *tmp = verTab[i];
                  verTab[i]  = verTab[j];
                  verTab[j]  = tmp;
              }
          }
      }

    DbgStamp(DBG_SAP_FLOW);
/* ---------------------------------------------------------------------------
CR    
CR  Make a function
CR   
 -------------------------------------------------------------------------- */
    for (i=0; i<verNum; i++)
    {
        if (i==0 || StrCmp(verTab[i],verTab[i-1])) 
        {
            fprintf (outFile, _T("#BACKUP %s\n"), verTab[i]);
        }
    }
    fflush (outFile);

    DbgStamp(DBG_SAP_FLOW);
/* ---------------------------------------------------------------------------
CR    
CR  Make a function
CR   
 -------------------------------------------------------------------------- */
    DbgPlain(DBG_SAP_FLOW, _T("verNum=%d"), verNum);
    for (i=0; i<verNum; i++)
    {
        FREE(verTab[i]);
    }

    DbgPlain(DBG_SAP_FLOW, _T("verTab=%p"), verTab);
    FREE(verTab); verTab = NULL;
    DbgFcnOut();

    return(retVal);
}



/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     s       string to convert
*
* @brief     converts all uppercase characters to lovercase
*
*//*=========================================================================*/
void
lcase(tchar *s)
{
    while (*s)
    {
        if (isupper(*s))
            *s = tolower(*s);
        s++;
    }
}


/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     backID
* @param     fileIDin
*
* @retval    Success status
*
*//*=========================================================================*/
static int
SapFindBackIDsOfFile (tchar *backID, tchar *fileIDin)     
{
    ERH_FUNCTION(_T("SapFindBackIDsOfFile"));

    int         retVal      = CC_SUCCESS;
    ULONG       ovNum       = 0,
                ovLen       = ALLOC_SIZE,
                i,j;

    int         result;                 /* for OB2_xxx error handling */
    tchar       tree[STRLEN_PATH+1];
    tchar      *realName;
    tchar      *fileID;
    DPID       *ovIDs;
    int         count;
    OB2_Item   *arrItem;
    OB2_ObjectAndVersion arrObjAndVer;


    DbgFcnIn();

/* ---------------------------------------------------------------------------
CR    
CR  Make function for allocationg and free-ing 
CR   
 -------------------------------------------------------------------------- */
    ovIDs  = CALLOC(ovLen, sizeof(DPID));
    verNum = 0;
    verLen = ALLOC_SIZE;
    verTab = MALLOC(verLen*sizeof(tchar *));

    fileID = NT2OB_FileName (fileIDin);

#if TARGET_WIN32
        lcase(fileID);
#endif
    
    /* Here we deal with converted file name, SLASH is not needed */
    if ((realName = StrrChr(fileID, _T('/')))==NULL) 
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] No File Found : '%s'"), 
            __FCN__, fileID);

        retVal = CC_FAILURE;
        goto NO_FILE_FOUND;
    }

    realName++;
    if (realName != NULL) 
    {
        DbgPlain(DBG_SAP_INFO,
            _T("[%s] realName=%s"), 
            __FCN__, 
            realName);
    }

    StrCopy(tree, fileID, realName-fileID-1);

    if ( (result = OB2_QueryPatternSearchFast(context, OT_OB2BAR, OB2_APP_SAP, realName, cmnHostname, tree,
                                          0L,0L,0,&count,&arrItem)) == OBE_OK) 
    {
        for(i=0; i<count; i++)
        {
            DbgStamp(DBG_SAP_FLOW);
            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] arrItem[%d]=%s"),
                __FCN__, 
                i,
                arrItem[i].name );

            if (!StrCmp(arrItem[i].name, fileID))
            {
                if (ovNum == ovLen)
                {
/* ---------------------------------------------------------------------------
CR    
CR  Make a define and use it  - make separate functions for realloc
CR   
 -------------------------------------------------------------------------- */
                    ovLen +=ALLOC_SIZE;
                    ovIDs  = REALLOC(ovIDs,ovLen*sizeof(ULONG));
                }
                ovIDs[ovNum++] = arrItem[i].ovID;
            }
        }
        OB2_FreeArrayOfItems(count, &arrItem); 
    }
    else
    {
        switch (result)
        {
        default:
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] OB2_QueryPatternSearchFast exited with errors. Error : %d"),
                __FCN__,
                result);

            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage (
                    NLS_SET_SAP,
                    NLS_SAP_EXIT_WITH_ERRORS,
                    _T("OB2_QueryPatternSearchFast")) );

            ErhClearError();
            retVal = CC_FAILURE;
        }
    }

    DbgStamp(DBG_SAP_FLOW);

    for (i=0; i<ovNum; i++)
    {
        if ( (result = OB2_QueryGetObjectAndVersion(context, ovIDs[i], &arrObjAndVer)) == OBE_OK)
        {
            int setNum;
            tchar *p;
            DbgPlain(DBG_SAP_INFO,
                _T("[%s] objName is \"%s\""),
                __FCN__,
                arrObjAndVer.object.name);

            if ((p = strrchr(arrObjAndVer.object.name, _T('.'))) != NULL)
            {
                DbgPlain(DBG_SAP_INFO, 
                    _T("[%s] p==%s"), 
                    __FCN__, 
                    p);

                if (sscanf(++p, _T("%d"), &setNum) == 1)
                {
                    if (verNum == verLen)
                    {
                        verLen += ALLOC_SIZE;
                        verTab  = (tchar **)REALLOC(verTab, verLen*sizeof(tchar *));
                    }
                    verTab[verNum] = (tchar *)MALLOC((STRLEN_SESSIONNAME+1)*sizeof(tchar));
                    DbgPlain(DBG_SAP_INFO,
                        _T("[%s] arrObjAndVer.version.backupID=%s,setNum=%d"),
                        __FCN__,
                        arrObjAndVer.version.backupID,
                        setNum);

                    if (SapSetVer2BackID(
                            arrObjAndVer.version.backupID, 
                            setNum, 
                            verTab[verNum++]) != CC_SUCCESS)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] Conversion session name => backID failed"),
                            __FCN__);

                        ErhFullReport (
                            __FCN__,
                            ERH_MAJOR,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_VER2BACKID_FAILED,
                                arrObjAndVer.version.backupID,
                                setNum) );

                        ErhClearError();
                        retVal = CC_FAILURE;
                    }
                }
            }
            OB2_ObjectVersionFree(&arrObjAndVer);
        }
        else
        {
            switch (result)
            {
            default:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] OB2_QueryGetObjectAndVersion exited with errors. Error : %d"),
                    __FCN__,
                    result);

                ErhFullReport (
                    __FCN__,
                    ERH_WARNING,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_EXIT_WITH_ERRORS,
                        _T("OB2_QueryGetObjectAndVersion")) );

                ErhClearError();
                retVal = CC_FAILURE;
            }
        }
        OB2_ObjectVersionFree(&arrObjAndVer);
    }

    DbgStamp(DBG_SAP_FLOW);
    FREE (fileID);

/*---------------------------------------------------------------------------
|    Sort and output backID verTab
 -------------------------------------------------------------------------- */

    if (verNum)
        for (i=0; i<verNum-1; i++)
        {
            for (j=i+1; j<verNum; j++)
            {
                if (StrCmp (verTab[i], verTab[j]) < 0)
                {
                    tchar *tmp = verTab[i];
                    verTab[i]  = verTab[j];
                    verTab[j]  = tmp;
                }
            }
        }

NO_FILE_FOUND:

    FREE(ovIDs);
    
    DbgFcnOut();

    return(retVal);
}

static int
SapListBackIDsOfFile (tchar *backID, tchar *fileIDin)     
{
    ERH_FUNCTION(_T("SapListBackIDsOfFile"));

    int         retVal      = CC_SUCCESS;
    ULONG       i;

    DbgFcnIn();

    if ((retVal= SapFindBackIDsOfFile(backID, fileIDin))!=CC_SUCCESS)
    {
        if (verTab)
        {
            for (i=0;i<verNum;i++)
                FREE(verTab[i]);
            FREE(verTab);
        }
        verNum=0;
        DbgFcnOut();
        return(retVal);
    }

    if (!verNum)
    {
        fprintf (outFile, _T("#NOTFOUND %s\n"),fileIDin);
        fflush  (outFile);

        DbgPlain(DBG_SAP_INFO,
            _T("[%s] Output: #NOTFOUND %s"), 
            __FCN__,
            fileIDin);
    }
    else
    {
        int backIDFound=0;
        
        for (i=0; i<verNum; i++)
        {
            if (!StrICmp(backID, _T("#NULL")) || !StrCmp(backID, verTab[i]))
            {
                fprintf (outFile, _T("#BACKUP %s %s\n"), verTab[i], fileIDin);
                fflush  (outFile);

                backIDFound=1;

                DbgPlain(DBG_SAP_INFO,
                    _T("[%s] Output: #BACKUP %s %s"),
                    __FCN__,
                    verTab[i], 
                    fileIDin);

            }
            FREE(verTab[i]);
        }
        if (!backIDFound)
        {
            fprintf (outFile, _T("#NOTFOUND %s\n"), fileIDin);
            fflush  (outFile);
        }
    }

    FREE(verTab); 
    verTab = NULL;
    
    DbgFcnOut();

    return(retVal);
}






/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     backID
*
* @retval    success status
*
*//*=========================================================================*/
static int
SapListFilesInBackID (tchar *backID)    
{
    int         index;
    int         result; /* OB2_xxx error handling */
    tchar       setName[STRLEN_OBJECTNAME+1];
    tchar       setVer[STRLEN_SESSIONID+1];
    tchar      *p;
    ULONG       ovNum                   = 0,
                ovLen                   = ALLOC_SIZE,
                i,j,
                sidLen                  = 0,
                count;
    DPID       *ovIDs;

    OB2_ObjectAndVersion *arrObjAndVer  = NULL;
    OB2_Item             *arrItem       = NULL;

    int         retVal                  = CC_SUCCESS;

    ERH_FUNCTION(_T("SapListFilesInBackID"));

    DbgFcnIn();

    ovIDs = CALLOC(ovLen, sizeof *ovIDs);

/* ---------------------------------------------------------------------------
| Convert backID to session ID and index 
 -------------------------------------------------------------------------- */
    if (SapBackID2SetVer (backID, setVer, &index)!=CC_SUCCESS)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Conversion backID => session name failed"),
            __FCN__);

        ErhFullReport (
            __FCN__,
            ERH_MAJOR,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_BACKID2VER_FAILED,            
                backID) );

        ErhClearError();
        retVal = CC_FAILURE;

        DbgFcnOut();
        return retVal;
    }
      
    sprintf(setName, _T("%s.%d"), sapOpt.userID, index);
    sidLen = StrLen (sapOpt.userID) + 1;

/* ---------------------------------------------------------------------------
| Find ovID of userID.index/setVersion 
 -------------------------------------------------------------------------- */
    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW, 
        _T("[%s] Listing files in BackID; object name: %s"), 
        __FCN__, setName);
  
    if ( (result = OB2_QueryObjectsOfSession(context, setVer, cmnHostname, OB2_APP_SAP, &count, &arrObjAndVer)) == OBE_OK)
    {
        for(i=0; i<count; i++)
        {
            p=arrObjAndVer[i].object.name;

            DbgPlain (DBG_SAP_FLOW, _T("Received object name: %s (searching for: %s)"), NSD(p), setName);

            if ( (!StrNCmp(p, setName, sidLen)) && (StrLen (p) > sidLen) && (atoi(p + sidLen) == atoi(setName + sidLen)) ) 
            {
/* ---------------------------------------------------------------------------
CR    
CR  Make functions for allocating
CR   
 -------------------------------------------------------------------------- */
                DbgPlain (DBG_SAP_FLOW, _T(" .. match"));
                if (ovNum == ovLen)
                {
                    ovLen += ALLOC_SIZE;
                    ovIDs  = REALLOC(ovIDs, ovLen*sizeof(*ovIDs));
                }
                ovIDs[ovNum++] = arrObjAndVer[i].version.ovID;
            }
            OB2_ObjectVersionFree(&arrObjAndVer[i]);
        }
    }
    else
    {
        switch (result)
        {
        case EDB_INVSESSION:
            fprintf(stdout, _T("#NOTFOUND %s\n"), backID);
            fflush(stdout);

            ErhClearError();
            break;
        default:
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] OB2_QueryObjectsOfSession exited with errors. Error : %d"),
                __FCN__,
                result);

            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage (
                    NLS_SET_SAP,
                    NLS_SAP_EXIT_WITH_ERRORS,
                    _T("OB2_QueryObjectsOfSession")) );

            ErhClearError();
            retVal = CC_FAILURE;
        }
    }
    
    FREE(arrObjAndVer);
  
/* ---------------------------------------------------------------------------
|  List all Files inside the ovID 
 -------------------------------------------------------------------------- */

    for (i=0; i<ovNum; i++)
    {
/* ---------------------------------------------------------------------------
CR    
CR  What if not found
CR   
 -------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------
AK  
AK  This is normal situation, we should NOT print anything in this case
AK  and that's all. However, this is only if OB2_QueryXXX return OBE_OK,
AK  Otherwise we report an error.
AK
 -------------------------------------------------------------------------- */
        if ( (result = OB2_QueryCatalog(context, ovIDs[i], &count, &arrItem)) == OBE_OK) 
        {
            for (j=0; j<count; j++)
            {
                tchar *ntName;
              
                DbgPlain (DBG_SAP_INFO, _T(" "));
                DbgPlain (DBG_SAP_INFO, 
                    _T("[%s] arrItem[%d].name=%s"), 
                    __FCN__, 
                    j, 
                    arrItem[j].name);

                ntName = OB2NT_FileName (arrItem[j].name);
                fprintf (outFile, _T("#BACKUP %s %s\n"), backID, ntName);
                fflush (outFile);
                FREE (ntName);

                DbgPlain(DBG_SAP_INFO,
                    _T("[%s] backID=%s\nfileName=%s"),
                    __FCN__,
                    backID,
                    arrItem[j].name);
            }
            OB2_FreeArrayOfItems(count, &arrItem);
        }
        else
        {
            switch (result)
            {
            default:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] OB2_QueryCatalog exited with errors. Error : %d"),
                    __FCN__,
                    result);

                ErhFullReport (
                    __FCN__,
                    ERH_WARNING,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_EXIT_WITH_ERRORS,
                        _T("OB2_QueryCatalog")) );

                ErhClearError();
            }
        }
        OB2_FreeArrayOfItems(count, &arrItem);
    }

    FREE(ovIDs);
    DbgFcnOut();
    return retVal;
}




/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     N/A
*
* @retval    success status
*
* @brief     Function reads backIDs and/or filenames from the inFile and
*            starts necessary queries
*
*//*=========================================================================*/
static int
SapQueryFiles (void)
{
    ERH_FUNCTION(_T("SapQueryFiles"));

    int         dev                 = -1;
    int         retVal              = CC_SUCCESS;
    int         result;

    tchar       path[STRLEN_1K+1],
               *pom;

    ULONG       i,
                count;

    OB2_Object *arrObj;

    DbgFcnIn();

/* ---------------------------------------------------------------------------
|    First, find out what concurrency we used for the given sapOpt.userID
 -------------------------------------------------------------------------- */
    DbgStamp (DBG_SAP_FLOW);
    DbgPlain (DBG_SAP_FLOW,
        _T("[%s] Calling OB2_QueryObjectsPartName query to find out (and list) backups of this user"),
        __FCN__);

    if ( (result = OB2_QueryObjectsPartName(
                    context, 
                    OT_OB2BAR, 
                    OB2_APP_SAP, 
                    sapOpt.userID, 
                    &count, 
                    &arrObj)) == OBE_OK)
    {
        for(i=0; i<count; i++)
        {
            if((pom = StrChr(arrObj[i].name, _T('.')))==NULL)
            {
                continue;
            }
            
            *pom++;
            
            if (atoi(pom) > dev)
            {
                dev = atoi(pom);
            }
            
            DbgPlain(DBG_SAP_INFO, 
                _T("[%s] host = \"%s\", setname = \"%s\"\n"), 
                __FCN__,
                arrObj[i].hostname, 
                arrObj[i].name);
        }
        FREE(arrObj);
    }
    else
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] OB2_QueryObjectsPartName exited with errors. Error : %d"),
            __FCN__,
            result);
        
        ErhFullReport (
            __FCN__,
            ERH_WARNING,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_EXIT_WITH_ERRORS,
                _T("OB2_QueryObjectsPartName")) );

        ErhClearError();

        retVal = CC_FAILURE;
    }
    
    if (dev>-1)
    {
        dev++;
    }

    if (dev < 0)
    {
        ErhFullReport ( 
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_INV_SETVERSIONS) );

        DbgFcnOut();
        return (retVal);
    }

/* ---------------------------------------------------------------------------
|    Now read the inFile line by line and perform the appropriate queries
 -------------------------------------------------------------------------- */
    if (fgets (path, STRLEN_OBJNAME, inFile) == NULL)
    {
/* ---------------------------------------------------------------------------
|    CASE #0: Input file is empty ...
 -------------------------------------------------------------------------- */
        retVal = SapListBackIDs ();
    }
    else do
    {
        tchar    *backID = NULL, 
                 *fileID = NULL;

/* ---------------------------------------------------------------------------
|    CASE #1: Current line is empty ...
 -------------------------------------------------------------------------- */
        if (*path == '\0') 
        {
            continue;
        }

        backID = StrTok (path, _T(" \t\n"));
        fileID = StrTok (NULL, _T(" \t\n"));

        DbgPlain (DBG_SAP_INFO,
            _T("[%s] Input: backID=%s, fileID=%s"),
            __FCN__,
            (backID ? backID : _T("<null>")),
            (fileID ? fileID : _T("<null>")) );

/* ---------------------------------------------------------------------------
|    CASE #2: Both backID AND fileID are present and != #NULL
 -------------------------------------------------------------------------- */
        if (backID && StrICmp(backID, _T("#NULL")) &&
            fileID && StrICmp(fileID, _T("#NULL")))
        {
            retVal = SapListBackIDsOfFile (backID, fileID);
            continue;
        }

/* ---------------------------------------------------------------------------
|    CASE #3: backID is != #NULL, no fileID or fileID == #NULL
 -------------------------------------------------------------------------- */
        if (backID && StrICmp(backID, _T("#NULL")) &&
           (!fileID || (fileID && !StrICmp(fileID, _T("#NULL")))))
        {
            retVal = SapListFilesInBackID (backID);
            continue;
        }

/* ---------------------------------------------------------------------------
|    CASE #4: backID is #NULL and fileID is #NULL or unspecified
 -------------------------------------------------------------------------- */
        if (backID && !StrICmp(backID, _T("#NULL")) &&
            (!fileID || (fileID && !StrICmp(fileID, _T("#NULL")))))
        {
            retVal = SapListBackIDs ();
            continue;
        }

/* ---------------------------------------------------------------------------
|    CASE #5: backID is #NULL, fileID is there and != NULL
 -------------------------------------------------------------------------- */
        if (backID && !StrICmp(backID, _T("#NULL")) &&
            fileID && StrICmp(fileID, _T("#NULL")))
        {
            retVal = SapListBackIDsOfFile (backID, fileID);
            continue;
        }
    }
    while (fgets (path, STRLEN_OBJNAME, inFile));

    fprintf (outFile, _T("\n"));
    fflush  (outFile);

    DbgFcnOut();
    return (retVal);
}


/**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     backid1     SAP backID
* @param     backid2     SAP backID
*
* @retval    CC_SUCCESS
* @retval    CC_FAILURE
*
* @brief     Compare two backIds without index => compare sessionIds.  
*
*/
int 
BackIdCompare(tchar *backid1, 
              tchar *backid2)
{
    int retVal = 0;
    tchar sessId1[STRLEN_SESSIONID+1] = {0};
    tchar sessId2[STRLEN_SESSIONID+1] = {0};
    int index1 = 0;
    int index2 = 0;
    
    SapBackID2SetVer(backid1, sessId1, &index1);
    SapBackID2SetVer(backid2, sessId2, &index2);

    retVal = StrICmp(sessId1, sessId2);

    return(retVal);
}


/*========================================================================*//**
*
* @ingroup   OmniBack II - SAP R3 integration 
*
* @param     argc        argument count
* @param     argv        argument array
*
*//*=========================================================================*/
int
main (int argc, tchar *argv[])    
{
    
    ERH_FUNCTION(_T("main"));
    OB2_DebugOpts debug = {0};

    int         WaitTime = 0;
    int         i        = 0;
    int         isSMB    = 0;

    tchar       *caller   = NULL;
    tchar       *request  = NULL;
    tchar       *instance = NULL;
    tchar       *backup_system = NULL, *application_system = NULL;
    tchar       *tmpstr   = NULL;
    BP_Opt      *opt      = NULL;
    int         retVal    = CC_SUCCESS;

    signal(SIGABRT, CmnVoidAbortHandler);

    if ((argc<2)||(StrICmp(argv[1], _T("-ver"))==0))
    {
        tchar    verString[STRLEN_VERSION+1];

        CmnSetProgname(PROG_BACKINT);
        MakeVersionString (STRLEN_VERSION, verString);
        ConPrintf (_T("%s\n"), verString);

        return(retVal);
    }

#if TARGET_WIN32
    {
        tchar *env_path = BU_GetEnv(_T("PATH"));
        tchar *new_path = StrNewCopy(cmnPanLBin);

        StrAppend(new_path, _T(";"));
        StrAppend(new_path, env_path);
        BU_PutEnv(_T("PATH"), new_path);
        FREE(env_path);
        FREE(new_path);
    }
#endif

    OB2_DebugOptsFromEnv (&debug);
    OB2_DebugOptsFromArg (&debug, argc, argv);
    OB2_DebugOptsToEnv   (&debug);

/* ---------------------------------------------------------------------------
|    Init OB2 BAR Services
 -------------------------------------------------------------------------- */
    if ( OB2_Init(OB2_APP_SAP, _T("TARGET"), _T("BACKINT"), NULL, &context) )
    {
        instance = BU_EnvGetStrValue(_T("OB2APPNAME"), _T("DEFAULT"));

        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(
            DBG_UNEXPECTED,
            _T("[%s] OB2_Init(OB2_APP_SAP, appName=%s, errCallBack) failed. Error: %s"),
            __FCN__,
            instance,
            ErhSysErrnoToText(errno)
        );

        ConPrintf(
            _T("[%s] OB2_Init(OB2_APP_SAP, appName=%s, errCallBack) failed. Error: %s\n"),
            __FCN__,
            instance,
            ErhSysErrnoToText(errno)
        );

        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_OB2_INIT_FAILS
            )
        );

        ErhClearError();
        FREE(instance);

        return (CC_FAILURE);
    }

    SapOptInit (argc, argv); 
    SapOptParseCliOpts (argc-1, argv+1);

    instance = sapOpt.userID ? StrNewCopy (sapOpt.userID) : BU_EnvGetStrValue(_T("OB2APPNAME"), _T("DEFAULT"));
    DbgPlain (DBG_SAP_INIT, _T("[%s] User ID : %s, Instance: %s"), __FCN__, sapOpt.userID, instance);

    OB2_SetAppTypeAppName(context, OB2_APP_SAP, instance);

    caller  = EnvGetString(_T("BI_CALLER"));
    request = EnvGetString(_T("BI_REQUEST"));

    DbgPlain(
        DBG_SAP_INFO,
        _T("[%s] caller = '%s', request = '%s'"),
        __FCN__,
        caller,
        request
    );

    if ( !StrIsEmpty(sapOpt.cfgFile) )
    {
        BP_Opt doc = CmnVarReadFileContent(sapOpt.cfgFile, VAR_READ_DETECT_UNICODE);
        if ( !CmnVarDefined(&doc) )
        {
            DbgPlain (DBG_SAP_FLOW, _T("[%s] Failed to open BACKINT parameter file: %s"), __FCN__, sapOpt.cfgFile);
            ErhFullReport(
                __FCN__,
                ERH_CRITICAL,
                NlsGetMessage(
                    NLS_SET_SAP,
                    NLS_SAP_CANNOT_OPEN_FILE_FOR_READING,
                    sapOpt.cfgFile,
                    ErhErrnoToText()
                )
            );
            return(CC_FAILURE);
        }
        BU_ExportToEnv( &doc );

        if ( !instance )
        {
            instance = BP_GetStrCopy(&doc, _T("ORACLE_SID"));
            BU_PutEnv(_T("OB2APPNAME"), instance);
        }
    }

    {
        int    disk_only = 0;
        int    incCFOLF  = 0;
        int    isIR      = 0;
        tchar  *backup_type = NULL;

        BP_InitAsList(&BRdoc, _T("BRDOCUMENT"));
        BP_AddStr(&BRdoc, _T("ORACLE_SID"), instance);
        barlist = BU_EnvGetStrValue (_T("OB2BARLIST"), _T("SAP-R3"));
        BP_AddStr(&BRdoc, _T("OB2BARLIST"), barlist);
        backup_system = BU_EnvGetStrValue (_T("OB2BACKUPHOSTNAME"), cmnHostname);
        BP_AddStr(&BRdoc, _T("BACKUP_SYSTEM"), backup_system);
        application_system = BU_EnvGetStrValue (_T("OB2BARHOSTNAME"), cmnHostname);
        BP_AddStr(&BRdoc, _T("APPLICATION_SYSTEM"), application_system);
        BU_EnvGetIntValue(_T("OB2SMBIR"), &isIR);
        if ( isIR ) BP_AddInt(&BRdoc, _T("OB2SMBIR"), isIR);
        BU_EnvGetIntValue(_T("OB2DISKONLY"), &disk_only);
        if ( disk_only ) BP_AddInt(&BRdoc, _T("DISK_ONLY"), disk_only);
        BU_EnvGetIntValue(_T("ZDB_ORA_INCLUDE_CF_OLF"), &incCFOLF);
        if ( incCFOLF ) BP_AddInt(&BRdoc, _T("ZDB_ORA_INCLUDE_CF_OLF"), incCFOLF);
        BP_AddList(&BRdoc, _T("BRBOPTS"), &opt);
        backup_type = BU_EnvGetStrValue(_T("SAPBACKUP_TYPE"), _T("ONLINE"));
        if ( !StrICmp(backup_type, _T("ONLINE")) )
            BP_AddStr(opt, _T("TYPE"), _T("online_"));
        else
            BP_AddStr(opt, _T("TYPE"), _T("offline_"));

        BU_EnvGetIntValue(_T("OB2SMB"), &isSMB);
    }

    if ( isSMB && !StrCmp(application_system, backup_system) )
    {
        tchar errdesc[STRLEN_ERHMSG+1] = {0};
        sprintf( errdesc, _T("(OB2BACKUPHOSTNAME = OB2BARHOSTNAME = %s)"), application_system );
        ErhFullReport( __FCN__, ERH_CRITICAL,
            NlsGetMessage( NLS_SET_SAP, NLS_SAP_CANNOT_INIT, errdesc ) );
        return(CC_FAILURE);
    }

    if ( StrCmp(cmnHostname, backup_system) )
        StrCopy(cmnHostname, application_system, STRLEN_HOSTNAME);

    if ( !isSMB && StrCmp(cmnHostname, application_system) )
        StrCopy(cmnHostname, application_system, STRLEN_HOSTNAME);

    tmpstr = BU_EnvGetStrValue(_T("OB2APPNAME"), _T(""));
    if ( StrIsEmpty(tmpstr) ) BU_PutEnv(_T("OB2APPNAME"), instance);
    FREE(tmpstr);

    tmpstr = BU_EnvGetStrValue(_T("OB2BARTYPE"), _T(""));
    if ( StrIsEmpty(tmpstr) ) BU_PutEnv(_T("OB2BARTYPE"), OB2_APP_SAP);
    FREE(tmpstr);

    tmpstr = BP_Dump(BRdoc);
    DbgPlain(DBG_SAP_INFO, _T("[%s] BACKINT:\n%s"), __FCN__, NSD(tmpstr));
    FREE(tmpstr);

    DbgStamp (DBG_SAP_STATUS);
    DbgPlain (DBG_SAP_STATUS, _T("[%s] Parsing options"), __FCN__);

    SapOptInitFilenames();
    SapOptParseCfgOpts(application_system, instance, barlist);
    SapOptCheckOpts();

    sapOpt.userID = instance;
    
    if (sapOpt.inFile)
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] fileOP: OPEN"), __FCN__);
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] file we are attempting to open"), sapOpt.inFile);

        if (!(inFile = fopen(sapOpt.inFile, _T("a+"))))
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] fopen(%s, \"r\") failed. Error: %s"),
                __FCN__, 
                sapOpt.inFile, 
                ErhSysErrnoToText(errno));

            ErhFullReport (
                __FCN__,
                ERH_CRITICAL,
                NlsGetMessage (
                    NLS_SET_SAP,
                    NLS_SAP_OPEN_FILE_FAIL,
                    sapOpt.inFile) );

            goto EXIT;
        }
    }
    else
    {
         inFile = stdin;
    }
    

    if (sapOpt.outFile)
    {
        if (!(outFile = fopen(sapOpt.outFile, _T("w"))))
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] fopen(%s, \"w\") failed. Error: %s"),
                __FCN__, 
                sapOpt.outFile, 
                ErhSysErrnoToText(errno));

            ErhFullReport (
                __FCN__,
                ERH_CRITICAL,
                NlsGetMessage (
                    NLS_SET_SAP,
                    NLS_SAP_OPEN_FILE_FAIL,
                    sapOpt.outFile) );

            goto EXIT;
        }
    }
    else
    {
        outFile = stdout;
    }

    if ( isSMB && !StrCmp(caller, _T("BRBACKUP")) && !StrCmp(request, _T("NEW")))
    {
        DbgPlain (DBG_SAP_INIT, _T("[%s] Split Mirror (DMA) mode   : ON"), __FCN__);    
        workMode = SMB_DMA;
    }
    else
    {
        DbgPlain (DBG_SAP_INIT, _T("[%s] Split Mirror (DMA)   : OFF"), __FCN__);
        workMode = Normal; 
    }
    
/* ---------------------------------------------------------------------------
    Printing backint call arguments
 -------------------------------------------------------------------------- */
    {
        tchar msg[STRLEN_MESSAGE+1] = _T("");

        for (i=0; i<argc; i++)
        {
            sprintf(msg, _T("%s %s"), msg, argv[i]);
        }

        ErhFullReport (
            __FCN__,
            ERH_NORMAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_BACKINT_ARG,
                msg) );
    }

    OB2_StartQueries(context);

    if (!inFile) 
    {
        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_NO_INPUT));

        argRet = CC_FAILURE;
    }    
    else if (!StrICmp (sapOpt.func, _T("backup")))
/* ---------------------------------------------------------------------------
|    BACKUP
 -------------------------------------------------------------------------- */ 
    {
        DbgStamp (DBG_SAP_STATUS);
        DbgPlain (DBG_SAP_STATUS, _T("[%s] Starting backup"), __FCN__);
        switch (workMode)
        {
        case SMB_DMA:
            argRet = DmaBackupMode(&BRdoc);
            break;
        default:
            if (!StrICmp (sapOpt.type, _T("file")))
            {
                argRet = SapBackupFiles (BACKUP, request);
            }
            else 
            {
                if (!StrICmp (sapOpt.type, _T("file_online")))
                {
                    argRet = SapBackupFiles (FILE_ONLINE, request);
                }
                else 
                {
                    if (!StrICmp (sapOpt.type, _T("volume")))
                    {
                        ErhFullReport (
                            __FCN__,
                            ERH_CRITICAL,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_NO_VOLUME_SUPP) );

                        argRet = CC_FAILURE;
                    }
                    else
                    {
                        ErhFullReport (
                            __FCN__,
                            ERH_CRITICAL,
                            NlsGetMessage (
                                NLS_SET_SAP,
                                NLS_SAP_INV_TYPE) );

                        argRet = CC_FAILURE;
                    }
                }
            }
        }
    }
/* ---------------------------------------------------------------------------
|    RESTORE
 -------------------------------------------------------------------------- */
    else if (!StrICmp (sapOpt.func, _T("restore")))
    {
        DbgStamp (DBG_SAP_STATUS);
        DbgPlain (DBG_SAP_STATUS, _T("[%s] Starting restore"), __FCN__);

        if (!StrICmp (sapOpt.type, _T("file")))
        {
            argRet = SapRestoreFiles ();
        }
        else 
        {
            if (!StrICmp (sapOpt.type, _T("volume")))
            {
                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_NO_VOLUME_SUPP) );

                argRet = CC_FAILURE;
            }
            else
            {
                ErhFullReport (
                    __FCN__,
                    ERH_CRITICAL,
                    NlsGetMessage (
                        NLS_SET_SAP,
                        NLS_SAP_INV_TYPE) );

                argRet = CC_FAILURE;
            }
        }

        WaitTime = 60;
        EnvReadInt(_T("OB2WAITTIME"), &WaitTime);
        if (WaitTime)
        {
            DbgPlain(DBG_SAP_FLOW, 
                _T("[%s] Entering sleep(%d)"), 
                __FCN__,
                WaitTime);

            sleep(WaitTime);
        }
        else
        {
            DbgPlain(DBG_SAP_FLOW, _T("[%s] No sleep."), __FCN__);
        }

    }
/* ---------------------------------------------------------------------------
|    INQUIRY
 -------------------------------------------------------------------------- */
    else if (!StrICmp (sapOpt.func, _T("inquire"))
         ||  !StrICmp (sapOpt.func, _T("inquiry")))
    {
        DbgStamp (DBG_SAP_STATUS);
        DbgPlain (DBG_SAP_STATUS, _T("[%s] Starting inquiry"), __FCN__);

        argRet = SapQueryFiles ();
    }

/* ---------------------------------------------------------------------------
|    ???????
 -------------------------------------------------------------------------- */
    else
    {
        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_INV_FUNCTION, 
                sapOpt.func) );

        argRet = CC_FAILURE;
    }
    
    OB2_EndQueries(context);

    ErhFullReport(
        __FCN__,
        ERH_NORMAL,
        NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_BACKINT_EXIT,
            argRet) );

/* ---------------------------------------------------------------------------
|    Exit backint
 -------------------------------------------------------------------------- */

    DbgStamp (DBG_SAP_STATUS);
    DbgPlain (DBG_SAP_STATUS, _T("[%s] Exiting"), __FCN__);

EXIT:
    if (aborted)
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
                NLS_SET_SAP,
                NLS_SAP_ABORTED) );
    }
    OB2_Exit (context);

    return (argRet);
}
