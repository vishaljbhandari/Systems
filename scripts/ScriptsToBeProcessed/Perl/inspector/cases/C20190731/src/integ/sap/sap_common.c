/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Data Protector - SAP R/3 BAR Integration
* @file      integ/sap/sap_common.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     SAP R/3 BAR common functions
*
* @since     22.09.2005     mrakm       Merged ux_common & w32_common
*/


#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/sap/sap_common.c $Rev$ $Date::                      $:") ;
#endif

#include "integ/sap/sap_common.h"

tchar   *szOracleNLS_orig = NULL;
tchar   *szOracleNLS_UTF  = NULL;
tchar   *oracle_output = NULL;      /* data returned by Oracle */
tchar   *sap_error_string = NULL;
int     script_status = OBE_NODATA; /* status of script, retval or sqlcode */
int     script_ConOutput = 0;       /* set to 1 if you want script output sent to console from parsing hooks */

#if TARGET_UNIX
static struct sigaction act = {0};
#endif


#if TARGET_SOLARIS || TARGET_LINUX
/*
   Linux and Solaris do not have environ declared in stdlib.h.
   Everyone else implicitly includes stdlib.h by including target.h.
   
   If someone finds the appropriate include file, add it and remove the ifdef.
*/
extern char **environ;
#endif

#if TARGET_WIN32

#include <winbase.h>
#include <sys/stat.h>
#include "integ/sap/commonmsg.h"

HANDLE          hMutex;

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     N/A
*
* @brief     Creates mutex (Win32)
*
*//*=========================================================================*/
int CreateSemOrMutex()
{
    tchar lpBrMutex[] = _T("brbackup_mutex");
    DWORD err=0;
    
    ERH_FUNCTION(_T("CreateSemOrMutex"));

    DbgFcnIn();

    hMutex = CreateMutex(NULL, FALSE, lpBrMutex);
    DbgPlain (DBG_SAP_INFO, 
        _T("[%s] CreateMutex LastError: %lu"), 
        __FCN__,
        GetLastError());
    
    if ((err = GetLastError()) == ERROR_INVALID_HANDLE)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] CreateMutex(NULL, FALSE, lpBrMutex) failed. Error: %s"),
            __FCN__, 
            ErhSysErrnoToText(GetLastError()));
        
        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            ErhSysErrnoToText(err) );

        DbgFcnOut();
        return (CC_FAILURE);
    }
    
    DbgPlain (DBG_SAP_FLOW, 
        _T("[%s] Brbackup mutex created\n"),
        __FCN__);

    DbgFcnOut();
    return(CC_SUCCESS);
}




/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
*//*=========================================================================*/
void RemoveSemOrMutex()
{
    ERH_FUNCTION(_T("RemoveSemOrMutex"));
    
    DbgFcnIn();
    
    CloseHandle(hMutex);
    
    DbgFcnOut();
}




/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     fname            file name
*
* @brief     Returns 1 if file is Directory, 0 otherwise, -1 on error
*
*//*=========================================================================*/
int fileType (tchar *fname)
{
    ERH_FUNCTION(_T("fileType"));

    struct _stati64    buf={0};
    int     ret=0;
    
    DbgFcnIn();

/*
    return( _tstati64 (fname, &buf) ? -1 :( 
        (buf.st_mode & S_IFDIR)?(DbgFcnOut(),1):(DbgFcnOut(),0)) 
        );
*/   
    ret=_tstati64(fname, &buf);

    if((ret==0)&&(buf.st_mode & S_IFDIR))
    {
      DbgPlain(DBG_MAIN_ACTION, _T("[%s] %s is directory"), __FCN__, fname);
      ret=1;
    }

    DbgFcnOut();
    return(ret);
}

#else

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sem.h>

#include "integ/bar2/libbar.h"

#include "backint.h"
#include "sapback.h"
#include "optmgr.h"
#include "commonmsg.h"

int interrupt = 0;

static void CtrlCHandler (int not_used)
{
    DbgStamp(DBG_SAP);
    DbgPlain(DBG_SAP, _T("CtrlCHandler\n"));

    interrupt = 1;
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     Semaphore id
*
* @retval    0          Success
* @retval    -1         Failure
*
* @brief     Performs P() operation on the semaphore (decrease)
*
*//*=========================================================================*/
int P_Sem (int semid)
{
  ERH_FUNCTION(_T("P_Sem"));
  struct    sembuf sops[1]={0};

  sops[0].sem_num =  0;
  sops[0].sem_op  = -1;
  sops[0].sem_flg =  SEM_UNDO;

  if (semop (semid, sops, 1) == -1 )
    {
      DbgStamp(DBG_UNEXPECTED);
      DbgPlain(DBG_UNEXPECTED,
               _T("[%s] semop(semid=%d, sops.sem_op=-1, 1) failed. Error: %s"),
               __FCN__, semid, ErhSysErrnoToText(errno));
      return (-1);
    }

  return (0);
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     Semaphore id
*
* @retval    0          Success
* @retval    -1         Failure
*
* @brief     Performs V() operation on the semaphore (increase)
*
*//*=========================================================================*/
int V_Sem (int semid)
{
  ERH_FUNCTION(_T("V_Sem"));
  struct    sembuf sops[1]={0};

  sops[0].sem_num =  0;
  sops[0].sem_op  =  1;
  sops[0].sem_flg =  SEM_UNDO;

  if (semop (semid, sops, 1) == -1 )
    {
      DbgStamp(DBG_UNEXPECTED);
      DbgPlain(DBG_UNEXPECTED,
               _T("[%s] semop(semid=%d, sops.sem_op=1, 1) failed. Error: %s"),
               __FCN__, semid, ErhSysErrnoToText(errno));
      return (-1);
    }
 
  return (0);
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     N/A
*
* @retval    N/A
*
* @brief     Sets Ctrl-C key handler
*
*//*=========================================================================*/
void SetCtrlCHandler()
{
    ERH_FUNCTION(_T("SetCtrlCHandler"));  
    
    act.sa_handler=CtrlCHandler;
    if (sigemptyset(&act.sa_mask) < 0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] sigemptyset() failed. Error: %s"),
            __FCN__,  ErhSysErrnoToText(errno));
    }
    
    act.sa_flags=0;
    
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] sigaction() failed. Error: %s"),
            __FCN__,  ErhSysErrnoToText(errno));
        
    }
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     N/A
*
* @retval    Success status (CC_SUCCESS/CC_FAILURE)
*
* @brief     Creates semaphore (UX)
*
* @remarks   Uses global variables defined in ux_common.h
*
*//*=========================================================================*/
int CreateSemOrMutex()
{
    ERH_FUNCTION(_T("CreateSemOrMutex"));
#if !TARGET_WIN32
    int oldmask = umask(S_IXUSR|S_IRWXG|S_IRWXO);;
#endif

    semFile = OB2_GetTempFileName(cmnPanTmp, _T("sem"), NULL);

    if (!(semFP = fopen (semFile, _T("w"))))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] fopen(semFile=%s, \"w\") failed. Error: %s"),
            __FCN__, semFile, ErhSysErrnoToText(errno));

        DbgPlain (
            DBG_SAP, 
            _T("[%s] \tError: Cannot open: (%s)\n"),
            __FCN__,
            ErhSysErrnoToText (errno)
            );

        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_OPEN_FILE_FAIL,
            ErhSysErrnoToText (errno))        
            );

      if (unlink (semFile))
      {
          DbgStamp(DBG_UNEXPECTED);
          DbgPlain(DBG_UNEXPECTED,
              _T("[%s] unlink(semFile=%s) failed. Error: %s"),
              __FCN__, semFile, ErhSysErrnoToText(errno));
      }

      FREE (semFile);

#if !TARGET_WIN32
        umask(oldmask);
#endif
      return (CC_FAILURE);
    }

#if !TARGET_WIN32
    umask(oldmask);
#endif
    if ((sem_key = ftok (semFile, SAP_ID)) == -1)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] ftok(semFile=%s, \"w\") failed. Error: %s"),
            __FCN__, semFile, ErhSysErrnoToText(errno));

        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_FTOK_ERR)
            );
        DbgPlain (DBG_SAP, _T("[%s] Error: ftok call returns error\n"), __FCN__);

        if (unlink (semFile))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] unlink(semFile=%s) failed. Error: %s"),
                __FCN__, semFile, ErhSysErrnoToText(errno));
        }

        FREE (semFile);

        return (CC_FAILURE);
    }
    act.sa_handler=CtrlCHandler;
    if (sigemptyset(&act.sa_mask) < 0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] sigemptyset() failed. Error: %s"),
            __FCN__,  ErhSysErrnoToText(errno));
    }
    act.sa_flags=0;

    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] sigaction() failed. Error: %s"),
            __FCN__,  ErhSysErrnoToText(errno));
    }

    if ((sem_id  = semget (sem_key, 1, IPC_CREAT | 0644)) == -1)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] semget(sem_key=%d, 1, IPC_CREAT | 0644) failed. Error: %s"),
            __FCN__, sem_key, ErhSysErrnoToText(errno));

        DbgPlain (
            DBG_SAP, 
            _T("[%s] \tError: (%s)\n"),
            __FCN__,
            ErhSysErrnoToText (errno)
            );

        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_MESSAGE,
            ErhSysErrnoToText (errno))
            );
      
        if (unlink (semFile))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] unlink(semFile=%s) failed. Error: %s"),
                __FCN__, semFile, ErhSysErrnoToText(errno));
        }
        FREE (semFile);

        return (CC_FAILURE);
    }

    DbgPlain (DBG_SAP, _T("[%s] Created semaphore file %s\n"), 
        __FCN__, semFile);
    DbgPlain (DBG_SAP, _T("[%s] Sem_key %d\n"), __FCN__, sem_key);
    DbgPlain (DBG_SAP, _T("[%s] Sem_id  %d\n"), __FCN__, sem_id);

    #if SEMCTL_USE_SEMUN
    value_arg.val=1;
    #else
    value_arg=1;
    #endif

    /* ---------------------------------------------------------------------------
    |       Initializing semaphore
    -------------------------------------------------------------------------- */
    if ((sem_ct = semctl (sem_id, 0, SETVAL, value_arg)) == -1)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] semctl(sem_id=%d, 0, SETVAL, 1) failed. Error: %s"),
            __FCN__, sem_id, ErhSysErrnoToText(errno));

        DbgPlain (
            DBG_SAP, 
            _T("[%s] \tError: (%s)\n"),
            __FCN__,
            ErhSysErrnoToText (errno)
            );

        ErhFullReport (
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage (
            NLS_SET_SAP,
            NLS_SAP_MESSAGE,
            ErhSysErrnoToText(errno))
            );

        DbgPlain (DBG_SAP, _T("[%s] Sem_id  %d\n"), __FCN__, sem_id);

        if (semctl (sem_id, 0, IPC_RMID) < 0)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] semctl(sem_id=%d, 0, IPC_RMID) failed. Error: %s"),
                __FCN__, sem_id, ErhSysErrnoToText(errno));
        }
        DbgPlain (DBG_SAP, _T("[%s] Sem_id  %d\n"), __FCN__, sem_id);
        if (unlink (semFile))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("[%s] unlink(semFile=%s) failed. Error: %s"),
                __FCN__, semFile, ErhSysErrnoToText(errno));
        }
        DbgPlain (DBG_SAP, _T("[%s] Sem_id  %d\n"), __FCN__, sem_id);

        FREE (semFile);

        return (CC_FAILURE);
    }

    DbgPlain (DBG_SAP, _T("[%s] Semaphore file initialized\n"),__FCN__);
    return(CC_SUCCESS);
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     N/A
*
* @retval    N/A
*
* @brief     Removes previously created semaphore
*
*//*=========================================================================*/
void RemoveSemOrMutex()
{
    ERH_FUNCTION(_T("RemoveSemOrMutex"));
    if (semctl (sem_id, 0, IPC_RMID) < 0)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] semctl(sem_id=%d) failed. Error: %s"),
            __FCN__,  sem_id, ErhSysErrnoToText(errno));
    }

    if (unlink (semFile))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("[%s] unlink(semFile=%s) failed. Error: %s"),
            __FCN__, semFile, ErhSysErrnoToText(errno));
    }
    FREE (semFile);  
}

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     fname            file name
*
* @brief     Returns 1 if file is Directory, 0 otherwise, -1 on error
*
*//*=========================================================================*/
int fileType (tchar *fname)
{
    ERH_FUNCTION(_T("fileType"));
    struct stat    buf={0};
    int retVal=0;
    
    DbgFcnIn();
/*
    return (stat (fname, &buf) ? -1 :( 
        (buf.st_mode & _S_IFDIR)?(DbgFcnOut(),1):(DbgFcnOut(),0)) 
        );
*/
    retVal=stat(fname, &buf);
    if((retVal==0)&&(buf.st_mode & _S_IFDIR))
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] %s is a directory"), __FCN__, fname);
        retVal=1;
    }
    DbgFcnOut();
    return(retVal);
}

#endif


/*  Common (non system specific functions */

/*========================================================================*//**
*
* @ingroup   SAP-Integration
*
* @param     oraHome         Oracle home of sid
* @param     oraSid          SID of the instance
*
* @brief     This function expands spfile path(if oracle returned path with
*            special symbols) to usable path. Input is global oracle_output.
*
*//*=========================================================================*/
tchar *ExpandSpfilePath(tchar **spfile, const tchar *sid, const tchar *orahome)
{
    ERH_FUNCTION(_T("ExpandSpfilePath"));
    Variant out = T2V(*spfile);

    DbgFcnInEx(__FLND__, _T("spfile:%s"), NSD(*spfile));

    #if TARGET_UNIX
        CmnVarReplace (&out, _T("?"), orahome);
        CmnVarReplace (&out, _T("@"), sid);
    #else
        out = CT2V(StrExpandEnvironmentStrings(V2T(&out)));
    #endif

    FREE (*spfile);
    *spfile = V2T(&out);

    RETURN_STR (*spfile);
}


MALLOCED tchar *BRDir_DefaultPath(tchar *dbsid)
{
    tchar path[STRLEN_PATH+1] = {0};
    sprintf(path, BRBIN_DEFAULT_PATH_FMT, dbsid);
    return( StrNewCopy(path) );
}


MALLOCED tchar *BRDir_UniformPath(tchar *path, tchar *host, tchar *dbsid)
{
    tchar  unc_path[STRLEN_PATH+1] = {0};
    tchar *systr = NULL, *dbstr = NULL, *uncp = NULL;

    if (
        !StrIsEmpty(BRBIN_UNC_PATH_FMT) &&
        StrNCmp(path, _T("\\\\"), StrLen(_T("\\\\")))
    )
    {
        dbstr = StrStr(path, dbsid);
        if ( dbstr )
        {
            systr = StrNewCopy(dbstr);
        }
        else
        {
#if TARGET_WIN32
            systr = StrNewCopy(_T("SYS\\exe\\run"));
#else
            systr = StrNewCopy(_T("SYS/exe/run"));
#endif
        }
        sprintf(unc_path, BRBIN_UNC_PATH_FMT, host, systr);
        FREE(systr);
        uncp = StrNewCopy(unc_path);
    }
    else
    {
        uncp = StrNewCopy(path);
    }

    return(uncp);
}



/*========================================================================*//**
*
* @brief     Dumps environment variables. Windows will not properly report
*            unicode.
*
*//*=========================================================================*/
void DumpEnv()
{
    int i;

    DbgStamp(DBG_SAP);
    DbgPlain(DBG_SAP, _T("  Environment list:"));
    DbgPlain(DBG_SAP, _T("  ================="));
    for (i=0; environ[i]; i++)
    {
#if TARGET_WIN32
        DbgPlain(DBG_SAP, _T("  ENV[%2d]> '%.2000hs'"), i, environ[i]);
#else
        DbgPlain(DBG_SAP, _T("  ENV[%2d]> '%.2000s'"), i, environ[i]);
#endif
    }
    DbgPlain(DBG_SAP, _T("  ================="));
}

/*========================================================================*//**
*
* @param     int   argc
*                number of arguments
* @param     tchar **argv
*                array of strings
*
* @brief     Dumps argc number of argvs.
*
*//*=========================================================================*/
void DumpArguments(int argc, tchar **argv)
{
    int i;

    DbgPlain(DBG_SAP, _T("Starting parameter(s) (%d)"), argc);
    for (i=0; i<argc; i++)
    {
        DbgPlain(DBG_SAP, _T(" %d: %s"), i, argv[i]);
    }
}

/*========================================================================*//**
*
* @param     buf  - content of temporary file
*
* @retval    NULL      - error
* @retval    otherwise - name of the file
*
* @brief     Function will generate temporary file with desired content.
*            If flags include RUN_SCRIPT_CP_UTF8_STDIN the content will be converted
*            to UTF8, otherwise it will be converted using ACP.
*
*//*=========================================================================*/
MALLOCED tchar *GenerateTmpFile(const tchar *buf, tchar *postfix, ULONG flags)
{
    ERH_FUNCTION(_T("GenerateTmpFile"));

    tchar      *path = NULL;
    char       *contents = NULL;
    int         size, written;
    FILEHANDLE  file = INVALID_HANDLE_VALUE;
    BOOL        ok   = FALSE;
#if !TARGET_WIN32
    int         oldmask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
#endif

    DbgFcnIn();

    if (!buf)
        goto clean_up;

    /* Create temporary INPUT file */
    path = OB2_GetTempFileName(cmnPanTmp, _T("OB"), postfix);
    if (StrIsEmpty(path))
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_CANNOT_CREATE_TEMPFILE,
                cmnPanTmp,
                GetLastError()
            )
        );
        goto clean_up;
    }

    DbgPlain (DBG_SAP,_T("[%s] Temporary file name is %s"), __FCN__, path);

    file = OB2_OpenFile(
        path, 
        1, /* Open for writing */
        1  /* Create file */
    );
    
    if (file == INVALID_HANDLE_VALUE)
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_TEST_CANNOT_OPEN_FILE,
                path,
                GetLastError()
            )
        );
        goto clean_up;
    }

    if (flags & RUN_SCRIPT_CP_UTF8_STDIN)
    {
        contents = StrNewCopyT2S(buf);
    }
    else
    {
        contents = StrNewCopyT2A(buf);
    }

    size = strlenA(contents);
    written = OB2_WriteFile(file, contents, size);
    if (written != size)
    {
        ErhFullReport(
            __FCN__,
            ERH_CRITICAL,
            NlsGetMessage(
                NLS_SET_SAP,
                NLS_SAP_CANNOT_WRITE_FILE,
                path,
                GetLastError()
            )
        );
        goto clean_up;
    }

    ok = TRUE;

clean_up:
    OB2_CloseFile(file);
    FREE (contents);
#if !TARGET_WIN32
    umask(oldmask);
#endif

    if (!ok)
    {
        ErhMarkMajor(ERR_ORA_INTERNAL);
        FREE (path);
    }

    RETURN_STR(path);
}


 
/*========================================================================*//**
*
* @param     tchar *characterset
*                charset to be set
*
* @retval    OBE_OK
*                variable set
* @retval    OBE_ERR
*                setting failed
*
* @brief     The function will set or change NLS_LANG to characterset.
*            (NLS_LANG=.characterset or NLS_LANG=xxx.characterset)
*            Original NLS_LANG is saved into szOracleNLS_orig.
*
*//*=========================================================================*/
int SetOraCP(tchar *characterset)
{
    tchar   *tmpPtr = NULL;
    tchar   tmp_str [STRLEN_STD+1]={0};
    int     tmp_len = 0;
    ERH_FUNCTION(_T("SetOraCP"));

    if (!szOracleNLS_orig)
    {   /* init - get original language */
        DbgStamp (DBG_MAIN_ACTION);
        DbgPlain (DBG_MAIN_ACTION, _T("[%s]: Getting NLS_LANG settings..."), __FCN__);
        szOracleNLS_orig = EnvGetString(_T("NLS_LANG"));
        szOracleNLS_UTF  = NULL;
    }
    DbgPlain (DBG_MAIN_ACTION, _T("[%s]: Original charSet is: %s"), __FCN__, NSD(szOracleNLS_orig));

    tmpPtr=StrChr(szOracleNLS_orig, _T('.'));
    {   /* charset specified, change it */
        if (tmpPtr)
        {
            tmp_len = tmpPtr - szOracleNLS_orig;
        }
        else
        {
            tmp_len = StrLen(szOracleNLS_orig); 
        } 
        sprintf(tmp_str, _T("NLS_LANG=%.*s.%s"), tmp_len, NS(szOracleNLS_orig), characterset);
        if (PutEnv(tmp_str))
        {
            DbgStamp (DBG_MAIN_ACTION);
            DbgPlain (DBG_MAIN_ACTION, _T("[%s]: PutEnv(%s) failed. Error: %d"), __FCN__, tmp_str, errno );
            return (OBE_ERR);
        }
    }
    return (OBE_OK);
}

/*========================================================================*//**
*
* @retval    OBE_OK
*                variable set
* @retval    OBE_ERR
*                setting failed or no old setting found
*
* @brief     The function will set or change NLS_LANG to previously saved
*            szOracleNLS_orig.
*
*//*=========================================================================*/
static int RevertOraCP()
{
    tchar *tmpPtr = NULL;
    ERH_FUNCTION(_T("RevertOraCP"));

    if (szOracleNLS_orig)
    {
        if ((tmpPtr = EnvGetString (_T("NLS_LANG"))) != NULL)
        {
            if (!StrCmp(tmpPtr, szOracleNLS_orig))
            {
                FREE(tmpPtr);
                return (OBE_OK);
            }
            FREE(tmpPtr);
        }
        if (PutEnv(szOracleNLS_orig))
        {
            DbgStamp (DBG_MAIN_ACTION);
            DbgPlain (DBG_MAIN_ACTION, _T("[%s]: PutEnv(%s) failed. Error: %d"), __FCN__, szOracleNLS_orig, errno );
            return (OBE_OK);
        }
        else
        {
            DbgPlain (DBG_SAP_INFO, _T("[%s] %s is set in the environment"), __FCN__, szOracleNLS_orig);
            return (OBE_ERR);
        }
    }
    else
    {
        DbgStamp (DBG_MAIN_ACTION);
        DbgPlain (DBG_MAIN_ACTION, _T("[%s]: No Oracle CP to be set!"), __FCN__);
        return (OBE_ERR);
    }
}


/*========================================================================*//**
*
* @param     tchar *s
*                string to be outputed
*
* @brief     Prints message to stdout.
*
*//*=========================================================================*/
int ErhReportHook (tchar *s)
{
    PrintMessage(s);
    return(0);
}

/*========================================================================*//**
*
* @param     tchar *s
*                string to be outputed
*
* @brief     Prints string to stdout.
*
*//*=========================================================================*/
int ErhConsoleHook (tchar *s)
{
    DbgPlain(10, _T("       output: %s"), NS(s));
    ConPrint(s);
    return(0);
}

/*========================================================================*//**
*
* @param     tchar *inputline
*                line from script
*
* @brief     Parses the output of script and sets script_status to the RETVAL
*            reported.
*            If global script_ConOutput is set the lines are printed to console.
*
*//*=========================================================================*/
int ErhRetvalHook (tchar *inputline)
{
    tchar *retval_num=NULL;

    if (script_ConOutput)
    {
        DbgPlain(10, _T("       output: %s"), NS(inputline));
        ConPrint(inputline);
    }
    else
        DbgPlain(10, _T("script output: %s"), NS(inputline));
    

    if (script_status == OBE_NODATA)
        script_status = OBE_OB2ERR;

    if ((retval_num=StrStr(inputline, _T("*RETVAL*"))) != NULL)
    {
        sscanf(retval_num, _T("*RETVAL*%d"), &script_status);
        if (script_status<0)
        {
            script_status = script_status * (-1);
        }
        DbgPlain(DBG_MAIN_ACTION, _T("RETVAL is %d"), script_status);
    }
    else
    {
        StrAppend(oracle_output, inputline);
    }
    return(0);
}


/*========================================================================*//**
*
* @param     tchar *inputline
*                line from sqlplus
*
* @brief     Function does the actual parsing of sqlplus output and returns status
*            into global script_status.
*            Error codes are received from ORA- or SP2- status reports or
*            from sqlcode.
*            Detected errror lines are appended to global sap_error_string.
*
*//*=========================================================================*/
static int SAP_ParseErrString (tchar *inputline)
{
    int status=0;
    tchar *retval_num=NULL;
    
    if ((retval_num=StrStr(inputline, _T("ORA-"))) != 0)
    {
        StrAppend(sap_error_string, inputline);
        sscanf(retval_num, _T("ORA-%d"), &status);
        if (script_status<=0)
        {
            DbgPlain(DBG_SAP_INFO, _T("Operation status is %d."), status);
            script_status = status;
        }
    }
    else if ((retval_num=StrStr(inputline, _T("SP2-"))) != 0)
    {
        StrAppend(sap_error_string, inputline);
        sscanf(retval_num, _T("SP2-%d"), &status);
        if (script_status<=0)
        {
            DbgPlain(DBG_SAP_INFO, _T("Operation status is %d."), status);
            script_status = status;
        }
    }
    else
    {
        if ((retval_num=StrStr(inputline, _T("sqlcode "))) != 0)
        {
            sscanf(retval_num, _T("sqlcode %d"), &status);
            if (status) StrAppend(sap_error_string, inputline);
            if (script_status<=0)
            {
                DbgPlain(DBG_SAP_INFO, _T("Operation status is %d."), status);
                script_status = status;
            }
        }
    }
    return(script_status);
}

/*========================================================================*//**
*
* @param     tchar *inputline
*                line from sqlplus
*
* @brief     Parses the output of sqlplus and returns the status into global
*            script_status.
*            Errors are checked in SAP_ParseErrString()
*            If global script_ConOutput is set the lines are printed to console.
*
*//*=========================================================================*/
int ErhOraStatusHook(tchar *inputline)
{
    if (script_ConOutput)
    {
        DbgPlain(10, _T("       output: %s"), NS(inputline));
        ConPrint(inputline);
    }
    else
        DbgPlain(10, _T("script output: %s"), NS(inputline));

    if (script_status == OBE_NODATA)
        script_status = OBE_OB2ERR;

    if (oracle_output)
    {
        StrAppend(oracle_output, inputline);
        StrAppend(oracle_output, _T("\n"));
    }

    /*Search for errors in inputline*/
    script_status=SAP_ParseErrString(inputline);
    
    return(0);
}



#define OUTPUT_STR(_str)                                        \
        if (oracle_output)                                      \
        {                                                       \
            StrAppend(oracle_output, _str);                     \
            if (bState == 1)                                    \
            {                                                   \
                StrAppend(oracle_output, _T("\n"));             \
            }                                                   \
        }                                                       \
        if ( script_ConOutput &&                                \
             !StrStr(_str, _T("sqlcode ")) &&                   \
             !StrStr(_str, _T("ORA-")) &&                       \
             !StrStr(_str, _T("SP2-")) )                        \
        {                                                       \
            DbgPlain(10, _T("       output: %s"), _str);        \
            ConPrintf(_T("%s"), _str);                          \
            if (bState == 1)                                    \
            {                                                   \
                ConPrintf(_T("\n"));                            \
            }                                                   \
        }

/*========================================================================*//**
*
* @param     tchar *inputline
*                line from sqlplus
*
* @brief     Checks the return of sqlplus query.
*            If script_ConOutput is set the result lines are printed to console.
*            If oracle_output is set the result lines are appended to oracle_output.
*            Errors are checked in SAP_ParseErrString()
*            Problems:
*            - lines of the exact len of ORA_LINESIZE will be merged with next
*            - if line contains UTF-8 string it can be splitted in the middle
*            of character, causing problems when converting strings
*
*//*=========================================================================*/
int ErhOraOutputHook(tchar *inputline)
{
    static  int bState=0;   /* 0 - skipping output, 1 - reading, 2 - reading long line */
    tchar   *line;
    int     line_len = 0;
    USES_CONVERSION_T2A;

    /* is it time to switch? */
    if (StrStr(inputline, ORA_PROMPT))
    {
        bState = (bState == 0 ? 1 : 0 );

        /* set status to indicate we have received something */
        if (script_status == OBE_NODATA)
            script_status = OBE_OB2ERR;

        /* add missing newline */
        if (bState == 2)
        {
            bState = 0;
            OUTPUT_STR(_T("\n"));
        }
        return (0);
    }

    /* read line */
    if (bState)
    {
        line = StrCutTrailingSpaces(inputline);

        if (!line)
            return(0);

        line_len = strlenA(NSA(T2S(line)));

        if (line_len == ORA_LINESIZE)
        {
            bState = 2;
        }
        else
        {
            bState = 1;
        }

        OUTPUT_STR(line);
        FREE(line);
    }
    /*Check for errors in inputline*/
    script_status=SAP_ParseErrString(inputline);

    return(0);
}


/*========================================================================*//**
*
* @param     BP_Opt  config
*                configuration from the CS
*
* @retval    OBE_OK
*                variables exported
* @retval    OBE_ERR
*                error occured during PutEnv
*
* @brief     Checks the environment section of the configuration and exports the
*            variables.
*
*//*=========================================================================*/
int ExportConfigEnvironment(BP_Opt config)
{
    ERH_FUNCTION(_T("ExportConfigEnvironment"));

    BP_Opt  *env;
    int      i;

    DbgFcnIn();

    env = BP_GetOptByName(config, CONF_ENVIRONMENT);
    if (!env)
        RETURN_INT (OBE_OK);

    for (i=0; i<BP_GetOptCount(*env); ++i)
    {
        tchar *key = BP_GetKeyAt(*env,i);
        tchar *val = NULL;
        int valInt = 0;

        if (BP_NOERROR == BP_GetStrAt(*env,i,&val))
        {
            val = StrExpandEnvironmentStrings(val);
        }
        else if (BP_NOERROR == BP_GetIntAt(*env,i,&valInt))
        {
            tchar tmp[STRLEN_INT]={0};
            sprintf(tmp, _T("%d"), valInt);
            val = StrNewCopy(tmp);
        }
        else
            continue;

        BU_PutEnv (key, val);
        FREE(val);
    }

    RETURN_INT (OBE_OK);
}



/*========================================================================*//**
*
* @param     tchar *lpszORACLEHome
*                Oracle home of SID to check configuration for
* @param     tchar *szExecutable
*                string where the executable name is copied to
*
* @retval    OBE_OK
*                binary found
* @retval    OBE_ERR
*                binary not found
*
* @brief     Checks if Oracle binary exists in the specified directory.
*
*//*=========================================================================*/
int StatOracleBinary(tchar *lpszORACLEHome, tchar *szExecutable)
{
    ERH_FUNCTION(_T("StatOracleBinary"));
    tchar               full_path[STRLEN_PATH+1]={0};

    /* Let's find our executable Oracle */
    DbgStamp(DBG_SAP);
    StrCopy(full_path, lpszORACLEHome, STRLEN_PATH);
    PathAppendComponent(full_path, _T("bin"));
    PathAppendComponent(full_path, SQLPLUS);

    DbgPlain(DBG_SAP,_T("[%s] testing executable %s"), __FCN__, full_path);
    if (!OB2_StatFile(full_path, NULL))
    {
        DbgPlain(DBG_SAP,_T("[%s] Found!"), __FCN__);
        StrCopy(szExecutable, full_path, STRLEN_PATH );        
        return(OBE_OK);
    }
    else
    {
        DbgPlain(DBG_SAP, _T("[%s] ORACLE executable not found."), __FCN__);
        ErhMarkMajor(ERR_SAP_NO_ORACLE);
        *szExecutable = _T('\0');
        return(OBE_ERR);
    }
}


/*========================================================================*//**
*
* @param     tchar *lpszORACLEHome
*                Oracle home
* @param     tchar *ora_query
*                query statement for Oracle
*
* @retval    OBE_OK
*                statement executed OK
* @retval    OBE_ERR
*                error in execution of sqlplus, error is marked
* @retval    OBE_OB2ERR
*                sqlplus returned error or no status in output
* @retval    <num>
*                OBE_ are negative, if number is positive it indicates
*                ORA- or sqlcode error number.
*
* @brief     Runs specified Oracle query.
*            Also uses:
*            oracle_output: if not NULL, the output is appended to oracle_output
*            script_ConOutput: if !0 the output is sent to console
*            script_status: contains the return code of Oracle
*            If hooks did not set script_status it will be OBE_NODATA and function
*            will return OBE_OK.
*
*//*=========================================================================*/
int RunOracleQuery(tchar *lpszORACLEHome, unsigned long flags, tchar *ora_query, ...)
{
    ERH_FUNCTION(_T("RunOracleQuery"));
    tchar   *lpszFile=NULL;
    tchar   szSTDIN[STRLEN_PATH+1]={0};
    tchar   szExecutable[STRLEN_PATH+1]={0};
    tchar   query_str[STRLEN_16K+1]={0};
    int     retVal = OBE_OK;
    int     scriptRetval=0;
    int     sql_argc=4;
    tchar   **sql_argv=NULL;

    va_list query_param={0};
    tchar   *query_full_str=NULL;
    DbgFcnIn();

    /* compose query */

    va_start(query_param, ora_query);
#   if TARGET_WIN32
	vsprintf(query_str, STRLEN_16K + 1, ora_query, query_param);
#   else
	vsprintf(query_str, ora_query, query_param);
#   endif  
    va_end(query_param);

    /* Let's find our executable Oracle */
    if (StatOracleBinary(lpszORACLEHome, szExecutable) != OBE_OK)
        RETURN_INT(OBE_ERR);

    /* compose full query */
    query_full_str = StrNewCopy(query_str);
    StrAppend (query_full_str, ORA_PROMPT_SQLCODE);
   
    /* Let's create temporary INPUT command file */
    if ((lpszFile= GenerateTmpFile(query_full_str, _T("sql"), flags)) == NULL)
    {
        FREE (query_full_str);
        RETURN_INT(OBE_ERR);
    }

    /* prepares stdin */
    strcpy(szSTDIN, _T(" -S /NOLOG"));
    strcat(szSTDIN, _T(" \"@"));
    strcat(szSTDIN, lpszFile);
    strcat(szSTDIN, _T("\""));


    DbgPlain(DBG_SAP, _T("[%s] Running StrToArg(szExecutable = %s, szSTDIN = %s"), __FCN__, szExecutable, szSTDIN);
    StrToArg(szExecutable, szSTDIN, &sql_argc, &sql_argv);

    if (flags & RUN_SCRIPT_CP_UTF8_OUT)
    {   /* sqlplus output will be in UTF, set the Oracle charset to UFT8 */
        SetOraCP(ENV_CHARSET_UTF8);
    }
    else
    {   /* set original codepage, output will be in that */
        RevertOraCP();
    }

    script_status = OBE_NODATA;
    /* run sqlplus */
    DbgPlain(DBG_SAP, _T("[%s] Running %s; query is\n%s"), __FCN__, sql_argv[0], ora_query);
    DbgStamp(DBG_MAIN_ACTION);

    if (CmnRunScriptEx(NULL, sql_argc, sql_argv, NULL, 0, flags, &scriptRetval) != RUN_SCRIPT_OK) 
    {
        DbgStamp(DBG_SAP);
        DbgPlain(DBG_SAP, _T("[%s] Script '%s' failed to start. Retval value: %d"), __FCN__, szExecutable, scriptRetval);
        ErhMarkMajor(ERR_ORA_INTERNAL);
        retVal = OBE_ERR;   /* error in CmnRunScript */
    }
    else
    {
        if (scriptRetval)
        {
            retVal = OBE_OB2ERR;    /* error in script run */
        }
        else
        {
            if (script_status == 32004) /* ignore the obsolete parameter specified */
            {
                script_status = 0;
            }
                      
            if (script_status == OBE_NODATA)
            {
                DbgPlain(DBG_SAP, _T("[%s] No status was set by query, no errors in launching - returning OK."), __FCN__);
                retVal = OBE_OK;
            }
            else
            {
                retVal = script_status;
            }
        }
    }
    DbgStamp(DBG_MAIN_ACTION);

    /* delete file */
    if (lpszFile)
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s]Delete file %s "), __FCN__, lpszFile);
        if (OB2_DeleteFile(lpszFile))
        {
            DbgStamp(DBG_SAP);
            DbgPlain(DBG_SAP, _T("[%s]: OB2_DeleteFile(%s) failed. Error: %s"), 
                        __FCN__, lpszFile, ErhSysErrnoToText(errno));
            DbgPlain(DBG_SAP, _T("Cannot delete TMP file '%s'"), lpszFile);
        }
    }
    FREE(lpszFile);
    FreeStrToArg(&sql_argc, &sql_argv);

    FREE(query_full_str);
    RETURN_INT(retVal);
}

/*========================================================================*//**
*
* @param     int retVal
*                retVal from RunOracleQuery
*
* @retval    0
*                no error was returned by RunOracleQuery & RunOracleQueryMarkRetval
* @retval    !=0
*                mapped error number returned
*
* @brief     Function will try to map a possible Oracle error,
*            otherwise will return generic ERH_OBE_APPERROR error.
*            If OBE_OK received it checks script_status and if it is ok
*            OBE_OK returned.
*
*//*=========================================================================*/
int RunOracleQueryMarkRetval(int retVal)
{
    ERH_FUNCTION(_T("RunOracleQueryMarkRetval"));
    int ret = OBE_OK;

    DbgStamp(DBG_SAP);
    DbgPlain(DBG_SAP, _T("[%s]: Mapping retVal:  '%d'"), __FCN__, retVal);

    switch (retVal)
    {
        case 1034:      /* ORA-01034: ORACLE not available */
            ret = ERR_SAP_NO_ORACLE;
            break;
        case 640:       /* SP2-640: Not connected */
        case 12500:     /* TNS:listener failed to start a dedicated server process */
            ret = ERR_SAP_BAD_CONNSTRING;
            break;
        case 12154:      /* ORA-12154: TNS:no listener */
            ret = ERR_ORA8_NO_LISTENER;
            break;
        case OBE_OK:     /* ok returned, but was it ok? */
            if (script_status == OBE_NODATA)  /* no output */
            {
                ret = ERR_ORA_INTERNAL;
            }
            break;
        case OBE_ERR:    /* error already marked */
            ret = ErhErrno();
            break;
        case OBE_OB2ERR: /* sqlplus set return code */
        default:
            ret = ERH_OBE_APPERROR;
            break;
    }

    DbgStamp(DBG_SAP);
    DbgPlain(DBG_SAP, _T("[%s]: Returning ret value: '%d'"), __FCN__, ret);

    return (ret);
}

/*========================================================================*//**
*
* @param     FILE *inFile,               -(IN) file with listof files
* @param     tchar ***outFileList,       -(OUT)list of files
* @param     int *outFileNum             -(OUT)number of files
*
* @retval    0

*
* @brief     Getlist of files from inFile
*
*//*=========================================================================*/

int GetFilesListFromFile(FILE *inFile, tchar ***outFileList, int *outFileNum)
{
    ERH_FUNCTION(_T("GetFilesListFromFile"));
    char        *buff=MALLOC((STRLEN_1K+1)*sizeof(char));
    int readChrs=0;
    char    *fileName=NULL;
    tchar **fileList=NULL;
    int     nFiles=0;
    int i=1;
    DbgFcnIn();
    
    {/* First fill char buffer from file*/
        size_t readChrsTmp=0;
        char        *readBuf;    

        memset(buff, 0, STRLEN_1K+1);
        readBuf=buff;
        while((readChrsTmp=fread((void *)readBuf, sizeof(BYTE), STRLEN_1K, inFile))==STRLEN_1K)
        {
            buff=(char *)REALLOC(buff,(++i)*STRLEN_1K*sizeof(char)+1);
            readChrs+=readChrsTmp;
            readBuf=buff+readChrs;
            memset(readBuf, 0, STRLEN_1K*sizeof(char)+1);
        }
        readChrs+=readChrsTmp;
    }
    /*Now Fill fileList and set nFiles*/
    fileName=&(buff[0]);
    for(i=0;i<(readChrs+1);i++)
    {
        switch(buff[i])
        {
        case '\n':
            buff[i]='\0';
        case '\0':
            {
                tchar *tmpFileName=StrNewCopyA2T(fileName);
                if(StrLen(tmpFileName))  
                {
                    fileList=(tchar **)REALLOC(fileList, (++nFiles)*sizeof(tchar *));
                    { tchar *c = NULL; c=StrrChr (tmpFileName, _T('\n')); if (c) *c = _T('\0'); }
                    fileList[nFiles-1]=tmpFileName;
                    DbgPlain(DBG_SAP_FLOW, _T("[%s] %d File %s added"), __FCN__, nFiles, tmpFileName);
                }
            }
            fileName=&(buff[i+1]);
        default: break;
        }
    }
    *outFileList=fileList;
    *outFileNum=nFiles;
    DbgFcnOut();
    return(0);
}

/*========================================================================*//**
*
* @retval    SAP_NO_ERROR
* @retval    ERR_SAP_COPY_BR_INTERFACE
*
*//*=========================================================================*/
int CopyFileToBrDir(
    IN const tchar *sourceFile, 
    IN const tchar *targetFile, 
    IN const tchar *targetDir)
{
    tchar  source[STRLEN_PATH+1]={0};
    tchar  target[STRLEN_PATH+1]={0};
    tchar *info = NULL;
    int    retval;

    PathCatR (source, cmnPanLBin, sourceFile);
    PathCatR (target, targetDir,  targetFile);
    
    retval = OB2_CopyFile (source,target,OB2_COPY_FSYMLINK|OB2_COPY_FOVERWRITE);
   
    if (retval != 0)
    {
#if TARGET_WIN32
        if (ErhSysErrno() == ERROR_SHARING_VIOLATION)
        {
            DbgPlain(DBG_SAP_INFO, _T("%s"), ErhSysErrorStr());
            return (0);
        }
#endif
        info = StrFormat (_T("Copying %s to %s failed"), source, target);
        ErhPromoteError(ERR_SAP_COPY_BR_INTERFACE);
        ErhSetErrorInfo(info);
        FREE (info);
        return ERR_SAP_COPY_BR_INTERFACE;
    }
    return 0;
}



DPCOM_STUB (CopyFileToBrDir) (
    Variant sourceFile, 
    Variant targetFile,
    Variant targetDir )
{
    int result = CopyFileToBrDir (V2T(&sourceFile), V2T(&targetFile), V2T(&targetDir));
    return I2V(result);
}


void SAP_DPComExports()
{
    DPComExport (CopyFileToBrDir);
}
