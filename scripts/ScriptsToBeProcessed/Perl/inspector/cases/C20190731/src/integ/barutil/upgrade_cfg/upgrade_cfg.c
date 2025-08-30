/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Option Parser (Opt)
* @file      integ/barutil/upgrade_cfg/upgrade_cfg.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     upgrade_cfg utility - upgrade utility for integrations configuration files
*
* @since     26.10.2000   antonk          Initial Coding
*/

#include "lib/cmn/target.h"

#ifndef lint
    static tchar rcsId[] = _T("$Header: /integ/barutil/upgrade_cfg/upgrade_cfg.c $Rev$ $Date::                      $:") ;
#endif

#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "cs/csa/csa.h"
#include "integ/barutil/ob2util.h"
#include "integ/barutil/upgrade_cfg/upgrade_cfg.h"
#include "integ/barutil/upgrade_cfg/optmgr.h"
#include <errno.h>

const tchar AppProgname[STRLEN_FILE+1] = _T("UPGRADE_CFG");

#if TARGET_WIN32
#   ifndef F_OK
#       define F_OK    01       /* NT: check only for existance */
#   endif
#   ifndef X_OK
#       define X_OK    04       /* NT: check only read perms */
#   endif
#   if UNICODE
#       undef  access
#       define access   _taccess
#   else
#       error TODO: Eventually should fix this also for non-unicode
#   endif
#endif

#if TARGET_WIN32
  #define  SLASH   _T('\\')
#else
  #define  SLASH   _T('/')
#endif

#define INSTANCE_LIST                   _T("INSTANCE_LIST")

BP_Opt      config;
tchar       *CSHost=NULL;
tchar       outputBuffer[STRLEN_16K+1];
tchar       logFile[STRLEN_FILE];
int         needUpgrade=1;

#if TARGET_WIN32
short RMAN_cmd_idx = 2;
tchar *RMAN_list[] =
{
    _T("rman80.exe"),
    _T("rman.exe"),
    NULL
};
#else
short RMAN_cmd_idx = 1;
tchar *RMAN_list[] = {
    _T("rman"),
    NULL
};
#endif

tchar *RMAN_cmd = NULL;

struct oracle_versionTag
{
    int    relMaj;
    int    relMid;
    int    relMin;
    tchar *resto;
} oracle_version;


static int
ErhReportHook (tchar *s)
{   
    PrintMessage(s);
    return(0);
}

static int
ErhConsoleHook (tchar *s)
{
    ConPrint(s);
    return(0);
}

static int
ErhBackintHook (tchar *msg)
{
    int     ver;

    if (sscanf(msg, _T("HP OpenView OmniBack II A.%02d"), &ver)!=1)
        return(0);

    if (ver>=4) needUpgrade=0;
    
    return(0);
}


static int VersionGrepperHook (tchar *msg)
{
    ERH_FUNCTION(_T("VersionGrepperHook"));
    int _rel1, _rel2, _rel3;
    tchar *tmpPtr;

    DbgPlain(DBG_OB2BAR , _T("[%s] Got :%s"), __FCN__, NS(msg) );
    if ( msg && (tmpPtr = StrStr(msg, _T("Release "))) )
    {
        if ( sscanf(tmpPtr, _T("Release %d.%d.%d"), &_rel1, &_rel2, &_rel3) == 3 )
        {
            oracle_version.relMaj = _rel1;
            oracle_version.relMid = _rel2;
            oracle_version.relMin = _rel3;
            DbgPlain( DBG_OB2BAR, _T("[%s] Got version: %d.%d.%d"), __FCN__, _rel1, _rel2, _rel3 );
        }
    }
    return 0;
}

static int
BitVersionGrepperHook (
    tchar *msg
)
{
    ERH_FUNCTION(_T("BitVersionGrepperHook"));
    tchar *tmpPtr;
    
    DbgPlain( DBG_OB2BAR, _T("[%s] Got :%s"), __FCN__, NS(msg) );
    if ( msg && 
             ( 
               (tmpPtr=StrStr(msg,_T("ELF-64"))) ||
               (tmpPtr=StrStr(msg,_T("64-bit")))
             )
       )
    {
            /* Prepare to add 64bit in Oracle version */
            oracle_version.resto=_T("64bit");
            DbgPlain( DBG_OB2BAR, _T("[%s] Got 64-bit version: %s"), __FCN__, oracle_version.resto );
    }

    return(0);
}

static int
getOracleVersion (
    tchar *ora_home,
    tchar *rman_cmd
)
{
    ERH_FUNCTION(_T("getOracleVersion"));
    int    loc_argc = 0;
    int    dummy_retVal;
    tchar *loc_argv[2];
    tchar  tmpPath[STRLEN_PATH+1];

    DbgFcnIn();

    if ( ora_home == NULL ) goto ret_get_version;
    if ( rman_cmd == NULL ) goto ret_get_version;

    loc_argc    = 1;
    loc_argv[0] = rman_cmd;
    loc_argv[1] = NULL;

    PathConcat(tmpPath, ora_home, _T("bin"), STRLEN_PATH);

    (void) memset(&oracle_version, 0, sizeof(struct oracle_versionTag));

    ErhSetHooks( VersionGrepperHook, VersionGrepperHook );
    if ( CmnRunScriptEx(tmpPath, loc_argc, loc_argv, _T("exit"), 0, 0, &dummy_retVal) != RUN_SCRIPT_OK )
    {
        /* Will re-use loc_argc variable */
        loc_argc = 0;
    }
    ErhSetHooks( ErhReportHook, ErhConsoleHook );

ret_get_version:
    DbgFcnOut();
    return --loc_argc;
}

static int
getBitOracleVersion (
    tchar *ora_home    
)
{
    ERH_FUNCTION(_T("getBitOracleVersion"));
    int    retVal = 0;
    int    loc_argc = 0;
    int    dummy_retVal;
    tchar *loc_argv[3];
    tchar  tmpPath[STRLEN_PATH+1];    
    tchar  buff[STRLEN_2K+1];

    DbgFcnIn();

    if ( ora_home == NULL )
    {
        retVal = -1;
        goto ret_get_version;
    }

    PathConcat(tmpPath, ora_home, _T("bin"), STRLEN_PATH);    
    sprintf(buff, _T("%s/oracle"), tmpPath);

    loc_argc    = 2;
    loc_argv[0] = _T("file");
    loc_argv[1] = buff;
    loc_argv[2] = NULL;

    ErhSetHooks( BitVersionGrepperHook, BitVersionGrepperHook );
    if ( CmnRunScriptEx(_T("/usr/bin"), loc_argc, loc_argv, NULL, 0, 0, &dummy_retVal) != RUN_SCRIPT_OK )
    {
        retVal = -1;
    }
    ErhSetHooks( ErhReportHook, ErhConsoleHook );

ret_get_version:
    DbgFcnOut();
    return retVal;
}


static void OptParseDebug(tchar *ob2opts)
{
    OB2_DebugOpts dbg = {0};
    if (!OB2_DebugOptsFromStr (&dbg, ob2opts))
        return;

    opt.debugRanges  = StrNewCopy(dbg.ranges);
    opt.debugSession = StrNewCopy(dbg.postfix);
    opt.debugSelect  = StrNewCopy(dbg.select);
}


static int PutConfig(tchar *sid, tchar *integration, tchar *hostname, BP_Opt *confOpt)
{
    ERH_FUNCTION(_T("PutConfig"));

    int   result=RESULT_FAILURE;

    DbgFcnIn();

    if (StrIsEmpty(integration))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Integration name is missing."), 
            __FCN__);
        goto exitPutConfig;
    }
    
    if (StrIsEmpty(sid))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Instance name is missing."), 
            __FCN__);
        goto exitPutConfig;
    }

    DbgPlain(DBG_OB2BAR, 
        _T("[%s] Storing configuration on Cell Server \"%s\" with integration \"%s\" and instance \"%s\""),
        __FCN__,CSHost,integration, sid);

    if (BU_PutConfig(CSHost, hostname, integration, sid, *confOpt)!=BP_NOERROR)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, 
            _T("[%s] Can not put configuration to Cell Server \"%s\" with integration \"%s\" and instance \"%s\""),
            __FCN__,CSHost,integration, sid);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Error: %d, %s"), __FCN__, ErhErrno(), ErhErrnoToText());
        ErhClearError();
        goto exitPutConfig;
    }
    result=RESULT_SUCCESS;

exitPutConfig:
    DbgFcnOut();
    return(result); /* result is initialized with RESULT_FAILURE, so this is the default value */
}

static int GetConfig(tchar *sid, tchar *integration, BP_Opt *confOpt)
{
    ERH_FUNCTION(_T("GetConfig"));

    int   result=RESULT_FAILURE;

    DbgFcnIn();

    if (StrIsEmpty(integration))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Integration name is missing."), 
            __FCN__);
        goto exitGetConfig;
    }
    
    if (StrIsEmpty(sid))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Instance name is missing."), 
            __FCN__);
        goto exitGetConfig;
    }

    DbgPlain(DBG_OB2BAR, 
        _T("[%s] Reading configuration from Cell Server \"%s\" with integration \"%s\" and instance \"%s\""),
        __FCN__,CSHost,integration, sid);

    if (BU_GetConfig(CSHost, opt.appHost, integration, sid, confOpt)!=BP_NOERROR)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, 
            _T("[%s] Can not read configuration from Cell Server \"%s\" with integration \"%s\" and instance \"%s\""),
            __FCN__, CSHost, integration, sid);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Error: %d, %s"), __FCN__, ErhErrno(), ErhErrnoToText());
        ErhClearError();
        goto exitGetConfig;
    }
    result=RESULT_SUCCESS;

exitGetConfig:
    DbgFcnOut();
    return(result); /* result is initialized with RESULT_FAILURE, so this is the default value */
}

/* 
   Adds variable var with value val to configuration conf 
   In case of errors uses errorlevel prefix and instance while reporting an error

   Return value: RESULT_SUCCESS/RESULT_FAILURE
*/

static int AddValueToNewEnv(BP_Opt *conf, tchar *var, tchar *val,tchar *errorlevel, tchar *instance)
{
    if (val && strcmp(val,_T("")))
    {
        if (BP_AddStr(conf,var,val)!=BP_NOERROR)
            return(RESULT_FAILURE);
    }
    else
    {
        if (!errorlevel) errorlevel=ERROR_MINOR;
        DbgLogPlain(logFile, 
            _T("%s%s is not set in the environment for instance %s"),
            errorlevel,
            var,
            instance);
        DbgStamp(DBG_OB2BAR);
        DbgPlain(DBG_OB2BAR,
            _T("%s%s is not set in the environment for instance %s"),
            errorlevel,
            var,
            instance);
        return(RESULT_FAILURE);
    }
    
    return(RESULT_SUCCESS);
}

static void RenameFile(tchar *oldpath, tchar *tmppath, tchar *newsubpath)
{
    ERH_FUNCTION(_T("RenameFile"));

    tchar  newpath[STRLEN_PATH+1];
    DbgFcnIn();

    if (*tmppath)
    {
        sprintf(newpath,_T("%s/%s"),tmppath,newsubpath);
        if (OB2_RenameFile(oldpath,newpath))
        {
            if (errno==EXDEV) /* Cross-device link, i.e. trying to move between partitions */
            {
                tchar   *buf=NULL;
                size_t   size=0;

                errno=0;
                if (!(buf=CmnGetFile(oldpath,&size)) && errno)
                  {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not read file %s. Error: %d, %s"),
                             __FCN__, oldpath, errno, ErhSysErrnoToText(errno));
                    DbgLogPlain(logFile,_T("Can not read file %s. %s"),
                                oldpath, ErhSysErrnoToText(errno));
                  } else
                if (CmnPutFile(newpath,buf,size)!=0)
                  {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not write file %s. Error: %d, %s"),
                             __FCN__, oldpath, errno, ErhSysErrnoToText(errno));
                    DbgLogPlain(logFile,_T("Can not write file %s. %s"),
                                oldpath, ErhSysErrnoToText(errno));
                  } else
                if (OB2_DeleteFile(oldpath))
                  {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not delete file %s. Error: %d, %s"),
                             __FCN__, oldpath, errno, ErhSysErrnoToText(errno));

                    DbgLogPlain(logFile,_T("Can not delete file %s. %s"),
                                oldpath, ErhSysErrnoToText(errno));
                  }
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not move file %s to %s. Error: %d, %s"),
                         __FCN__, oldpath, newpath, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile,_T("Can not move file %s to %s. %s"),
                            oldpath, newpath, ErhSysErrnoToText(errno));
            }
        }
        else
        {
            DbgLogPlain(logFile, _T("File %s moved to %s"), oldpath, newpath);
        }
    }

    DbgFcnOut();
}

typedef struct sappar {
    tchar   sid[STRLEN_FILE+1],
            TGTConn[STRLEN_FILE+1],
            BRTools[STRLEN_PATH+1],
            OracleHome[STRLEN_PATH+1],
            SapArch[STRLEN_PATH+1],
            SapBackup[STRLEN_PATH+1],
            SapCheck[STRLEN_PATH+1],
            SapData_Home[STRLEN_PATH+1],
            SapLocalHost[STRLEN_PATH+1],
            SapReorg[STRLEN_PATH+1],
            SapStat[STRLEN_PATH+1],
            SapTrace[STRLEN_PATH+1],
            (*barlists)[STRLEN_FILE+1];
    int     blAllocated,blCount;
} SAPPAR;

#if TARGET_WIN32
int GetStrFromReg(HKEY hKeyEnv, tchar *src, tchar *dst, int dst_len)
{
    int cbName2;
    int err;
    tchar Name2[STRLEN_PATH+1];

    cbName2 = (dst_len>STRLEN_PATH)? STRLEN_PATH : dst_len;
    if ((err=RegQueryValueEx(
        hKeyEnv,
        src,
        NULL,
        NULL,
        (LPBYTE)Name2,
        &cbName2
        )) == ERROR_SUCCESS)
    {
        StrCopy(dst,Name2,dst_len);
    }
    else
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            err,
            0,
            Name2,
            STRLEN_PATH,
            0);
        
        DbgLogPlain(logFile, 
            _T("%sCan not get %s from registry. %s"),
            ERROR_WARNING,
            src,
            Name2);
    }
    return(err);
}
#endif

static int UpgradeSAP(tchar *CSHost)
{
    SAPPAR  *sapPar;        

    int     count=0, allocated=0;
    int     i;
    int     bWasConfigured=0;
    int     retVal=RESULT_SUCCESS; /* default return value */
    FILE    *F;

    tchar   path[STRLEN_PATH+1],
            tmppath[STRLEN_PATH+1],
            newpath[STRLEN_PATH+1];

    ERH_FUNCTION(_T("UpgradeSAP"));

    DbgFcnIn();

    sapPar = (SAPPAR *) MALLOC(10*sizeof(SAPPAR));
    memset(sapPar,0,10*sizeof(SAPPAR));
    allocated=10;

    DbgLogFull(__FLNPT__, logFile, _T("Starting SAP R/3 configuration upgrade"));

#if TARGET_WIN32
    sprintf(tmppath,_T("%ssap"),cmnPanTmp);
#else
    sprintf(tmppath,_T("/tmp/sap"));
#endif

    if (OB2_CreateDirectory(tmppath,0777) && 
#if TARGET_WIN32
        GetLastError()!=ERROR_ALREADY_EXISTS
#else
        errno!=EEXIST
#endif
        )
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
            __FCN__, tmppath);
        DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
            tmppath, ErhSysErrnoToText(errno));
        *tmppath=_T('\0');
    }
    else
    {
        DbgLogPlain(logFile, 
            _T("Directory %s created. Old configuration will be moved there."), tmppath);
    }

#if TARGET_WIN32
    {
        HKEY    hKey,hKeyEnv;
        LONG    err;
        DWORD   dwIndex;
        DWORD   cbName,cbClass,cbName2;
        tchar   Name[STRLEN_PATH+1];
        tchar   Class[STRLEN_PATH+1];
        tchar   Name2[STRLEN_PATH+1];
        tchar   EnvSubkey[STRLEN_PATH+1];
        FILETIME ft;
        
        if ((err=RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            SAP_REG,
            0,
            KEY_READ,
            &hKey
            )
            ) == ERROR_SUCCESS)
        {
            dwIndex = 0;
            cbName = cbClass = STRLEN_PATH;     /* set enough space for first query */
            
            while ((err=RegEnumKeyEx(
                hKey,
                dwIndex,
                Name,
                &cbName,
                NULL,
                Class,
                &cbClass,
                &ft
                )) != ERROR_NO_MORE_ITEMS)
            {
                dwIndex++;
                if (err == ERROR_SUCCESS)
                {
                    DbgPlain(DBG_OB2BAR,_T("SAP subkey found: %s"),Name);
                    DbgLogPlain(logFile, _T("SAP R/3 configuration found for instance %s"),Name);
                    sprintf(EnvSubkey,_T("%s\\%s"),Name,REG_ENV);   /* <sid>\Environment */
                    
                    if ((err=RegOpenKeyEx(
                        hKey,
                        EnvSubkey,
                        0,
                        KEY_READ,
                        &hKeyEnv
                        )) == ERROR_SUCCESS)
                    {
                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            ORACLE_SID,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                            
                            if (!strcmp(Name,Name2))
                            {
                                tchar   szKey[STRLEN_PATH+1],
                                        *lpszName;

                                if (count==allocated)
                                {
                                    sapPar = (SAPPAR *) REALLOC(sapPar, (10+allocated)*sizeof(SAPPAR));
                                    memset(sapPar+allocated,0,10*sizeof(SAPPAR));
                                    allocated+=10;
                                }
                                StrCopy(sapPar[count].sid,Name,STRLEN_FILE);

                                GetStrFromReg(hKeyEnv, SAPARCH,   sapPar[count].SapArch,   STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPBACKUP, sapPar[count].SapBackup, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPCHECK, sapPar[count].SapCheck, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPDATA_HOME, sapPar[count].SapData_Home, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPLOCALHOST, sapPar[count].SapLocalHost, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPREORG, sapPar[count].SapReorg, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPSTAT, sapPar[count].SapStat, STRLEN_PATH);
                                GetStrFromReg(hKeyEnv, SAPTRACE, sapPar[count].SapTrace, STRLEN_PATH);

                                /* REGSAP is defined on Omniback level ! */
                                sprintf(szKey,_T("%s\\%s"), REGSAP, Name); 
                                if ((lpszName=RegGetString(
                                                 HKEY_LOCAL_MACHINE, 
                                                 szKey, 
                                                 REGSAP_BRDIR))!=NULL
                                                 )
                                {
                                    StrCopy(sapPar[count].BRTools,lpszName,STRLEN_PATH);
                                    FREE(lpszName);
                                }

                                if ((lpszName=RegGetString(
                                                 HKEY_LOCAL_MACHINE, 
                                                 szKey, 
                                                 REGSAP_LOGIN))!=NULL
                                                 )
                                {
                                    StrCopy(sapPar[count].TGTConn,lpszName,STRLEN_PATH);
                                    FREE(lpszName);
                                }

                                if ((lpszName=RegGetString(
                                                 HKEY_LOCAL_MACHINE, 
                                                 szKey, 
                                                 ORACLE_HOME))!=NULL
                                                 )
                                {
                                    StrCopy(sapPar[count].OracleHome,lpszName,STRLEN_PATH);
                                    FREE(lpszName);
                                }

                                count++;
                                RegCloseKey(hKeyEnv);
                            }
                    }
                    
                    cbName = cbClass = STRLEN_PATH;     /* set enough space for next query */
                }
                
            }
            if (count!=0)
            {
                bWasConfigured=1;
            }
            RegCloseKey(hKey);
        }
    }
#else
    {
        tchar str[STRLEN_PATH+1];

        sprintf(path,_T("%s/sap/data_servers"),cmnPanConfigBase);

        if ((F=fopen(path,_T("r")))!=NULL)
        {
            while (fgets(str,STRLEN_FILE,F))
            {
                if (count==allocated)
                {
                    DbgPlain(DBG_OB2BAR, _T("Current size=%d, New size=%d"), 
                             (allocated*sizeof(SAPPAR)), (10+allocated)*sizeof(SAPPAR));
                    sapPar = (SAPPAR *) REALLOC(sapPar, (10+allocated)*sizeof(SAPPAR));
                    memset(sapPar+allocated,0,10*sizeof(SAPPAR));
                    allocated+=10;
                }
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                  str[strlen(str)-1]=0;
                DbgLogPlain(logFile, _T("Found SID %s"), str);
                strcpy(sapPar[count++].sid,str);
            }
            fclose(F);
            RenameFile(path,tmppath,_T("data_servers"));
            if (count!=0)
            {
                bWasConfigured=1;
            }
        }
        else
        {
            DbgStamp(DBG_OB2BAR);
            DbgPlain(DBG_OB2BAR, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile,  
                _T("Can not open data_servers file at %s. Error: %d, %s"),
                path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile,
                _T("%sCan not read configuration"),ERROR_FAILURE);
            return RESULT_FAILURE;
        }

        for (i=0;i<count;i++)
        {
            sprintf(newpath,_T("%s/%s"),tmppath,sapPar[i].sid);
            if (OB2_CreateDirectory(newpath,0777))
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
                    __FCN__, newpath);
                DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
                    newpath, ErhSysErrnoToText(errno));
            }
            else
            {
                DbgLogPlain(logFile, 
                    _T("Directory %s created."),newpath);
            }
            sprintf(path,_T("%s/sap/%s/oracle_home"),cmnPanConfigBase,sapPar[i].sid);
            if ((F=fopen(path,_T("r")))!=NULL)
            {
                if (fgets(str,STRLEN_PATH,F))
                {
                    if (str[0] && str[strlen(str)-1]==_T('\n'))
                        str[strlen(str)-1]=0;
                    strcpy(sapPar[i].OracleHome,str);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                        __FCN__, path);
                    DbgLogPlain(logFile, _T("Can not read oracle_home from file %s. %s"),
                        path, ErhSysErrnoToText(errno));
                }
                fclose(F);
                sprintf(newpath,_T("%s/oracle_home"),sapPar[i].sid);
                RenameFile(path,tmppath,newpath);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                    __FCN__, path, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile, _T("Can not open oracle_home file at %s. %s"),
                    path, ErhSysErrnoToText(errno));
            }

            sprintf(path,_T("%s/sap/%s/tgtconn"),cmnPanConfigBase,sapPar[i].sid);
            if ((F=fopen(path,_T("r")))!=NULL)
            {
                if (fgets(str,STRLEN_PATH,F))
                {
                    if (str[0] && str[strlen(str)-1]==_T('\n'))
                        str[strlen(str)-1]=0;
                    strcpy(sapPar[i].TGTConn,str);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                        __FCN__, path);
                    DbgLogPlain(logFile, _T("Can not read tgtconn from file %s. %s"),
                        path, ErhSysErrnoToText(errno));
                }
                fclose(F);
                sprintf(newpath,_T("%s/tgtconn"),sapPar[i].sid);
                RenameFile(path,tmppath,newpath);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                    __FCN__, path, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile, _T("Can not open tgtconn file at %s. %s"),
                    path, ErhSysErrnoToText(errno));
            }

            sprintf(path,_T("%s/sap/%s/conf"),cmnPanConfigBase,sapPar[i].sid);
            if ((F=fopen(path,_T("r")))!=NULL)
            {
                if (fscanf(F,_T("PATH %s"),str)==1)
                {
                    strcpy(sapPar[i].BRTools,str);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                        __FCN__, path);
                    DbgLogPlain(logFile, _T("Can not read conf from file %s. %s"),
                        path, ErhSysErrnoToText(errno));
                }
                fclose(F);
                sprintf(newpath,_T("%s/conf"),sapPar[i].sid);
                RenameFile(path,tmppath,newpath);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                    __FCN__, path, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile, _T("%sCan not open conf file at %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
            }

            sprintf(path,_T("%s/omnisap.exe.tmp"),cmnPanLBin);
            if ((F=fopen(path,_T("r")))!=NULL)
            {
                tchar str[STRLEN_PATH+1];
                while (fgets(str,STRLEN_PATH,F))
                {
                    tchar   *c,
                            *c2,
                            *var;

                    if (str[0] && str[strlen(str)-1]==_T('\n'))
                        str[strlen(str)-1]=0;
                    if ((c=strchr(str,_T('#')))==str) /* if the whole line is commented */
                        continue;
                    if (!c) c=str+strlen(str);
                    

                    if ((var=strstr(str,SAPARCH)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))  /* if there is no value specified */
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapArch,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPBACKUP)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapBackup,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPCHECK)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapCheck,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPDATA_HOME)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapData_Home,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPLOCALHOST)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapLocalHost,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPREORG)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapReorg,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPSTAT)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapStat,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPTRACE)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapTrace,c2,c-c2);
                        continue;
                    }
                }
                fclose(F);
            }
            else
            {
                DbgStamp(DBG_OB2BAR);
                DbgPlain(DBG_OB2BAR, _T("[%s] Can not open file %s. Error: %d, %s"),
                    __FCN__, path, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile, _T("Can not open omnisap.exe file at %s. %s"),
                    path, ErhSysErrnoToText(errno));
            }

            sprintf(path,_T("%s/sap/%s/.profile"),cmnPanConfigBase,sapPar[i].sid);
            if ((F=fopen(path,_T("r")))!=NULL)
            {
                tchar str[STRLEN_PATH+1];
                while (fgets(str,STRLEN_PATH,F))
                {
                    tchar   *c,
                            *c2,
                            *var;

                    if (str[0] && str[strlen(str)-1]==_T('\n'))
                        str[strlen(str)-1]=0;
                    if ((c=strchr(str,_T('#')))==str) /* if the whole line is commented */
                        continue;
                    if (!c) c=str+strlen(str);
                    

                    if ((var=strstr(str,SAPARCH)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))  /* if there is no value specified */
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapArch,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPBACKUP)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapBackup,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPCHECK)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapCheck,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPDATA_HOME)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapData_Home,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPLOCALHOST)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapLocalHost,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPREORG)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapReorg,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPSTAT)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapStat,c2,c-c2);
                        continue;
                    }
                    if ((var=strstr(str,SAPTRACE)) && var<c)
                    {
                        if (!(c2=strchr(var,_T('='))+1))
                            continue;
                        while (*c2==_T(' ') || *c2==_T('\t')) c2++;
                        strncpy(sapPar[i].SapTrace,c2,c-c2);
                        continue;
                    }
                }
                fclose(F);
                sprintf(newpath,_T("%s/.profile"),sapPar[i].sid);
                RenameFile(path,tmppath,newpath);
            }
            else
            {
                DbgStamp(DBG_OB2BAR);
                DbgPlain(DBG_OB2BAR, _T("[%s] Can not open file %s. Error: %d, %s"),
                    __FCN__, path, errno, ErhSysErrnoToText(errno));
                DbgLogPlain(logFile, _T("Can not open .profile file at %s. %s"),
                    path, ErhSysErrnoToText(errno));
            }
        }
    }
#endif
    for (i=0;i<count;i++)
    {
        DbgLogPlain(logFile, _T("%sConfiguration for instance %s"),ERROR_MINOR,sapPar[i].sid);
        DbgLogPlain(logFile, _T("Got the following SAP R/3 configuration parameters:"));
        DbgLogPlain(logFile, _T("\tORACLE_SID=%s"),sapPar[i].sid);
        DbgLogPlain(logFile, _T("\tORACLE_HOME=%s"),sapPar[i].OracleHome);
        DbgLogPlain(logFile, _T("\tConnection string=%s"),sapPar[i].TGTConn);
        DbgLogPlain(logFile, _T("\tBRTools directory=%s"),sapPar[i].BRTools);
        DbgLogPlain(logFile, _T("\tSAPDATA_HOME=%s"),sapPar[i].SapData_Home);
        DbgLogPlain(logFile, _T("\tSAPARCH=%s"),sapPar[i].SapArch);
        DbgLogPlain(logFile, _T("\tSAPBACKUP=%s"),sapPar[i].SapBackup);
        DbgLogPlain(logFile, _T("\tSAPCHECK=%s"),sapPar[i].SapCheck);
        DbgLogPlain(logFile, _T("\tSAPLOCALHOST=%s"),sapPar[i].SapLocalHost);
        DbgLogPlain(logFile, _T("\tSAPREORG=%s"),sapPar[i].SapReorg);
        DbgLogPlain(logFile, _T("\tSAPSTAT=%s"),sapPar[i].SapStat);
        DbgLogPlain(logFile, _T("\tSAPTRACE=%s"),sapPar[i].SapTrace);

        DbgPlain(DBG_OB2BAR,_T("%sConfiguration for instance %s"),ERROR_MINOR,sapPar[i].sid);
        DbgPlain(DBG_OB2BAR,_T("Got the following SAP R/3 configuration parameters:"));
        DbgPlain(DBG_OB2BAR,_T("\tORACLE_SID=%s"),sapPar[i].sid);
        DbgPlain(DBG_OB2BAR,_T("\tORACLE_HOME=%s"),sapPar[i].OracleHome);
        DbgPlain(DBG_OB2BAR,_T("\tConnection string=%s"),sapPar[i].TGTConn);
        DbgPlain(DBG_OB2BAR,_T("\tBRTools directory=%s"),sapPar[i].BRTools);
        DbgPlain(DBG_OB2BAR,_T("\tSAPDATA_HOME=%s"),sapPar[i].SapData_Home);
        DbgPlain(DBG_OB2BAR,_T("\tSAPARCH=%s"),sapPar[i].SapArch);
        DbgPlain(DBG_OB2BAR,_T("\tSAPBACKUP=%s"),sapPar[i].SapBackup);
        DbgPlain(DBG_OB2BAR,_T("\tSAPCHECK=%s"),sapPar[i].SapCheck);
        DbgPlain(DBG_OB2BAR,_T("\tSAPLOCALHOST=%s"),sapPar[i].SapLocalHost);
        DbgPlain(DBG_OB2BAR,_T("\tSAPREORG=%s"),sapPar[i].SapReorg);
        DbgPlain(DBG_OB2BAR,_T("\tSAPSTAT=%s"),sapPar[i].SapStat);
        DbgPlain(DBG_OB2BAR,_T("\tSAPTRACE=%s"),sapPar[i].SapTrace);
    }

    /* Now let's fill in barlists structure, i.e. let's find .par files for all barlists defined */
    if (count) /* Just in case there were no SIDs found... */
    {
        int     b=-1;
        int     msg;
        int     barlist_allocated=0,barlist_count=0;
        tchar   (*Barlist)[STRLEN_FILE+1];
        tchar   path[STRLEN_PATH+1];
        tchar   *data=NULL;
        tchar   *f1,*f2,*f3,*f4,*f5,*f6,*f7,*f8,*f9,*f10;

        /* Get all SAP barlists names */
        Barlist=(tchar (*)[STRLEN_FILE+1]) MALLOC(10*(STRLEN_FILE+1)*sizeof(tchar));
        memset(Barlist,0,10*(STRLEN_FILE+1)*sizeof(tchar));
        barlist_allocated=10;
        b=MCsaBindServer(CSHost);

        if (!MCsaGetDatalists(b,&data))
        {
            msg=StrParseStart(data);
            if (msg==MSG_DL_LIST)
            {
                while (!StrParseNNext(10,&f1,&f2,&f3,&f4,&f5,
                                         &f6,&f7,&f8,&f9,&f10))
                {
                    if (!strncmp(f1,OB2_APP_SAP,3))
                    {
                        if (barlist_allocated==barlist_count)
                        {
                            Barlist= (tchar (*)[STRLEN_FILE+1]) REALLOC(Barlist,
                                      (barlist_allocated+10)*(STRLEN_FILE+1)*sizeof(tchar));
                            memset(Barlist+barlist_allocated,0,10*(STRLEN_FILE+1)*sizeof(tchar));
                            barlist_allocated+=10;
                        }
                        StrCopy(Barlist[barlist_count++],f1+4,STRLEN_FILE);
                        DbgLogPlain(logFile, _T("Found SAP R/3 barlist %s"),f1+4);
                    }
                }

                if (data)
                {
                    FREE(data);
                    data=NULL;
                }


                for (i=0;i<10;i++)
                {
                    sapPar[i].barlists=(tchar (*)[STRLEN_FILE+1]) MALLOC (10*(STRLEN_FILE+1)*sizeof(tchar));
                    memset(sapPar[i].barlists,0,10*(STRLEN_FILE+1)*sizeof(tchar));
                    sapPar[i].blAllocated=10;
                }
                /* Get contents of each SAP barlist and if this is for our host - add 
                   barlist name to corresponding sid */
                for (i=0;i<barlist_count;i++)
                {
                    sprintf(path,_T("%s/%s"),G_SAPLIST_PATH,Barlist[i]);
                    if (!MCsaGetCustomFile(b,CMN_PAN_CONFIG,path,&data,READ_ONLY))
                    {
                        tchar *client;
                        tchar sid[STRLEN_FILE+1],host[STRLEN_PATH+1];

                        /* searched for string looks like: CLIENT "<SID>" <HOSTNAME> */
                        if ((client=strstr(data,_T("CLIENT")))!=NULL &&
                             sscanf(client+8,_T("%s %s"),sid,host)==2 &&
                             !stricmp(host,opt.appHost))
                        {
                            int j;

                            sid[strlen(sid)-1]=_T('\0'); /* Cut off the last \" */
                            for (j=0;j<count;j++)
                                if (!stricmp(sapPar[j].sid,sid))
                                {
                                    if (sapPar[j].blAllocated==sapPar[j].blCount)
                                    {
                                        sapPar[j].barlists=(tchar (*)[STRLEN_FILE+1]) 
                                            REALLOC (sapPar[j].barlists,
                                                     (sapPar[j].blAllocated+10)*(STRLEN_FILE+1)*sizeof(tchar)
                                                     );
                                        memset(sapPar[j].barlists+sapPar[j].blAllocated,
                                               0,
                                               10*(STRLEN_FILE+1)*sizeof(tchar));
                                        sapPar[j].blAllocated+=10;
                                    }
                                    StrCopy(sapPar[j].barlists[sapPar[j].blCount++],
                                            Barlist[i],
                                            STRLEN_FILE);
                                    DbgLogPlain(logFile, _T("Using barlist %s. Instance name %s hostname %s"),
                                        Barlist[i],sid,host);
                                }
                        }
                        if (data)
                            FREE(data);
                    } /* if MCsaGetCustomFile succeeded*/
                    else
                    {
                        if (retVal!=RESULT_FAILURE)
                            retVal=RESULT_SOSO;
                        DbgStamp(DBG_UNEXPECTED);
                        DbgPlain(DBG_UNEXPECTED,_T("Can not get file %s from the Cell Server"), path);
                        DbgLogPlain(logFile, _T("Can not get file %s from the Cell Server. Error: %d, %s"),
                            path, ErhErrno(), ErhErrnoToText());
                    }
                }/* for all barlist names found */
            } /* if got correct MSG i.e. barlists */
        } /* if MCsaGetDatalists succeeded*/
        else
        {
            DbgLogPlain(logFile, _T("Can not get list of backup specifications. Upgrade may be incomplete"));
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
        }

        if (Barlist)
            FREE(Barlist);
        if (data)
            FREE(data);
    } /* end of filling barlist fields, if (count) */
    else
    {
        DbgLogPlain(logFile, _T("%sNo configured SAP R/3 instances found"), ERROR_WARNING);
        if (bWasConfigured)
        {
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
        }
    }

    /* Let's create manual_balance_files directory where we will move manual balancing sets */

    sprintf(newpath,_T("%s%cmanual_balance_files"),tmppath,SLASH);
    if (OB2_CreateDirectory(newpath,0777))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
            __FCN__, newpath);
        DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
            newpath, ErhSysErrnoToText(errno));
    }
    else
    {
        DbgLogPlain(logFile, 
            _T("Directory %s created."),newpath);
    }

    /* Now we can proceed with creating new configuration itself */

    for (i=0;i<count;i++)
    {
        BP_Opt  config,
               *listOpt,
               *manOpt;
        int     j;

        DbgLogPlain(logFile, _T("\nCreating configuration for instance %s"),sapPar[i].sid);

        BP_InitAsList(&config,sapPar[i].sid);
        /* First add 3 major variables */
        if (AddValueToNewEnv(&config,ORACLE_HOME,sapPar[i].OracleHome,ERROR_FAILURE,sapPar[i].sid) != RESULT_SUCCESS)
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;

        if (AddValueToNewEnv(&config,REGSAP_LOGIN,sapPar[i].TGTConn,ERROR_FAILURE,sapPar[i].sid) != RESULT_SUCCESS)
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;

        if (AddValueToNewEnv(&config,REGSAP_BRDIR,sapPar[i].BRTools,ERROR_FAILURE,sapPar[i].sid) != RESULT_SUCCESS)
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;

        /* SAPDATA_HOME should be from now on in the top list */
        AddValueToNewEnv(&config,SAPDATA_HOME,sapPar[i].SapData_Home,ERROR_MINOR,sapPar[i].sid);

        /* Next fill in Environment sub-list */
        BP_AddList(&config,REG_ENV,&listOpt);

        AddValueToNewEnv(listOpt,SAPARCH,sapPar[i].SapArch,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPBACKUP,sapPar[i].SapBackup,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPCHECK,sapPar[i].SapCheck,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPLOCALHOST,sapPar[i].SapLocalHost,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPREORG,sapPar[i].SapReorg,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPSTAT,sapPar[i].SapStat,ERROR_MINOR,sapPar[i].sid);
        AddValueToNewEnv(listOpt,SAPTRACE,sapPar[i].SapTrace,ERROR_MINOR,sapPar[i].sid);

        /* Next get all .par files and add them to their own list */
        if (sapPar[i].blCount)
        {
            BP_AddList(&config,_T("SAP_Parameters"),&listOpt);
            for (j=0;j<sapPar[i].blCount;j++)
            {
                sprintf(path,_T("%s/sap/%s.par"),cmnPanConfigBase,sapPar[i].barlists[j]);
                if ((F=fopen(path,_T("rb")))!=NULL)
                {
                    tchar str[STRLEN_PATH+1];
                    
                    BP_AddStrList(listOpt,sapPar[i].barlists[j]);

                    DbgLogPlain(logFile, _T("Adding backup parameters for barlist %s"),
                        sapPar[i].barlists[j]);
                    
                    while (fgets(str,STRLEN_PATH,F))
                    {
                        if (str[0] && str[strlen(str)-1]==_T('\n'))
                            str[strlen(str)-1]=_T('\0'); /* Cut off the last \n */
                        BP_ChangeStrListAdd(listOpt,sapPar[i].barlists[j],str);
                    }
                    fclose(F);
                    sprintf(newpath,_T("%s.par"),sapPar[i].barlists[j]);
                    RenameFile(path,tmppath,newpath);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("%sFile %s not found"),ERROR_WARNING,path);
                    DbgLogPlain(logFile, 
                        _T("Can not open SAP R/3 parameter file %s corresponding to barlist %s. Error: %d, %s"),
                        path,sapPar[i].barlists[j], errno, ErhSysErrnoToText(errno));
                    if (retVal!=RESULT_FAILURE)
                        retVal=RESULT_SOSO;
                }
            }
        }

        /* Finally get speed.log and ratio files and add them to speed and xxx lists */
        sprintf(path,_T("%s/sap/sap_speed.log"),cmnPanConfigBase);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            tchar str[STRLEN_PATH+STRLEN_INT+2];

            BP_AddList(&config,_T("speed"),&listOpt);
            DbgLogPlain(logFile, _T("Adding backup times from the file %s to section speed"), path);

            while (fgets(str,STRLEN_PATH+STRLEN_INT+1,F))
            {
                tchar   fname[STRLEN_PATH+1];
                long    speed=0;

                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=_T('\0'); /* Cut off the last \n */
                /* CHECKME!!! if !TARGET_WIN32, on WIN32 the delimiter is #*/
#if !TARGET_WIN32
                if ((sscanf(str,_T("%[^:]:%ld"),fname,&speed))==2)
#else
                if ((sscanf(str,_T("%[^#]#%ld"),fname,&speed))==2)
#endif
                {
                    BP_AddInt(listOpt,fname,speed);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,
                        _T("%sCan not parse line from speed.log\n%s"),
                        ERROR_WARNING,str);
                    DbgLogPlain(logFile, _T("Can not parse line from speed.log\n%s"),
                        str);
                    if (retVal!=RESULT_FAILURE)
                        retVal=RESULT_SOSO;
                }

            }
            fclose(F);
            RenameFile(path,tmppath,_T("sap_speed.log"));
        }
        else
        {
            DbgLogPlain(logFile, _T("Can not open file %s. %s"),
                path, ErhSysErrnoToText(errno));
        }

        /* Let's add manual balancing files */

        BP_AddList(&config,_T("manual_balance"),&manOpt);
        for (j=0;j<sapPar[i].blCount;j++)
        {
            int k=0;

            while (1)
            {
#if !TARGET_WIN32
                sprintf(path,_T("%s/BAL_%s:%d"),cmnPanTmp,sapPar[i].barlists[j],k);
#else
                sprintf(path,_T("%sBAL_%s.%d"),cmnPanTmp,sapPar[i].barlists[j],k);
#endif
                if ((F=fopen(path,_T("r")))!=NULL)
                {
                    tchar str[STRLEN_PATH+1];
                    
                    DbgLogPlain(logFile, 
                        _T("Adding files manually set to be backed up in backup specification %s to set %d"),
                        sapPar[i].barlists[j],k);

                    if (BP_GetListByName(*manOpt, sapPar[i].barlists[j], &listOpt)!=BP_NOERROR)
                        BP_AddList(manOpt, sapPar[i].barlists[j], &listOpt);
                    while (fgets(str,STRLEN_PATH,F))
                    {
                        if (strlen(str)<2)
                            continue;
                        if (str[strlen(str)-1]==_T('\n'))
                            str[strlen(str)-1]=0;
                        BP_AddInt(listOpt,str,k);
                    }
                    fclose(F);

#if !TARGET_WIN32
                    sprintf(newpath, _T("/manual_balance_files/BAL_%s:%d"),
#else
                    sprintf(newpath, _T("\\manual_balance_files\\BAL_%s.%d"),
#endif
                        sapPar[i].barlists[j], k);


                    RenameFile(path,tmppath,newpath);
                }
                else
                    break;
                k++;
            }
        }

        /* Now on WIN32 we will check if backint exists at BRTOOLS location
           if it does and it's version is >=4.XX we don't need to upgrade anything */
#if TARGET_WIN32
        {
            tchar               backint[STRLEN_PATH+1];
            int                 ret;

            if (StrCmp(sapPar[i].BRTools, _T("")))
            {
                sprintf(backint, _T("%s\\backint.exe"), sapPar[i].BRTools);
                if (0==OB2_StatFile(backint, NULL))
                {
                    tchar *args[1];
                    args[0] = backint;

                    ErhSetHooks (ErhBackintHook, ErhBackintHook);

                    if (!CmnRunScriptEx(NULL, 1, args, NULL, 0, 0,&ret) & !ret)
                    {
                        if (!needUpgrade)
                        {
                            DbgLogPlain(logFile, 
                                _T("%sbackint.exe of version 4.00 or later is found. No upgrade needed."),
                                ERROR_MINOR);
                            BP_Free(&config);
                            ErhSetHooks (ErhReportHook, ErhConsoleHook);
                            continue;
                        }
                    }
                    ErhSetHooks (ErhReportHook, ErhConsoleHook);
                }
            }
            else
            {
                DbgLogPlain(logFile, 
                    _T("%sEither incorrect configuration or not upgrading from Omniback II 3.xx."),
                    ERROR_MINOR);
                DbgLogPlain(logFile, 
                    _T("%sBRTOOLS location was not specified. No upgrade needed."),
                    ERROR_MINOR);
                BP_Free(&config);
                continue;
            }
        }
#endif

        /* Now we are ready to store the configuration */

        if (PutConfig(sapPar[i].sid,OB2_APP_SAP,opt.appHost,&config)!=RESULT_SUCCESS)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("Storing SAP R/3 configuration for instance %s on host %s failed!"),
                sapPar[i].sid,
                opt.appHost);
            DbgLogFull(__FLNPT__, logFile, _T("%sStoring SAP R/3 configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,
                sapPar[i].sid,
                opt.appHost);
        }
        else
        {
            DbgLogFull(__FLNPT__, logFile, _T("%sSAP R/3 configuration for instance %s on host %s stored successfully."),
                ERROR_MINOR,sapPar[i].sid, opt.appHost);
            if (retVal!=RESULT_SUCCESS)
                DbgLogPlain(logFile, 
                _T("%sSome errors occured during SAP R/3 configuration upgrade. Please check on the CM if the configuration was moved as expected."),
                ERROR_WARNING);

            /* On NT we should also copy backint.exe, libdc.dll, libde.dll 
               from omniback/bin to BRTools directory */
#if TARGET_WIN32
            if (cmnPanBin && sapPar[i].BRTools[0]!=_T('\0'))
            {
                tchar orig[STRLEN_PATH+1], copy[STRLEN_PATH+1];


                sprintf(orig, _T("%s\\backint.exe"),cmnPanBin);
                sprintf(copy, _T("%s\\backint.exe"),sapPar[i].BRTools);
                DbgLogPlain(logFile, _T("Copying %s to %s"), orig, copy);

                if (!CopyFile(orig, copy, FALSE))
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("%sCopying backint.exe from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                    DbgLogFull(__FLNPT__, logFile, 
                        _T("%sCopying backint.exe from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                }
                else
                    DbgLogPlain(logFile, _T("Successfully copied %s to %s"), orig, copy);
                
                /* DLL copy for Win32 platform....LIBDC and LIBDE */
                sprintf(orig, _T("%s%clibdc.dll"), cmnPanBin, SLASH);
                sprintf(copy, _T("%s%clibdc.dll"), sapPar[i].BRTools, SLASH);
                DbgLogPlain(logFile, _T("Copying %s to %s"), orig, copy);

                if (!CopyFile(orig, copy, FALSE))
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("%sCopying libdc.dll from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                    DbgLogFull(__FLNPT__, logFile, 
                        _T("%sCopying libdc.dll from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                }
                else
                    DbgLogPlain(logFile, _T("Successfully copied %s to %s"), orig, copy);
                
                sprintf(orig, _T("%s%clibde.dll"), cmnPanBin, SLASH);
                sprintf(copy, _T("%s%clibde.dll"), sapPar[i].BRTools, SLASH);
                DbgLogPlain(logFile, _T("Copying %s to %s"), orig, copy);

                if (!CopyFile(orig, copy, FALSE))
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, _T("%sCopying libde.dll from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                    DbgLogFull(__FLNPT__, logFile, 
                        _T("%sCopying libde.dll from %s to %s failed! Error: %d. %s"),
                        ERROR_FAILURE,
                        cmnPanBin,
                        sapPar[i].BRTools,
                        GetLastError(),
                        ErhSysErrnoToText(GetLastError())
                        );
                }
                else
                    DbgLogPlain(logFile, _T("Successfully copied %s to %s"), orig, copy);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, 
                    _T("[%s] We will NOT copy backint/libdc/libde. cmnPanBin=%s, sapPar[%d].BRTools=%s"),
                    __FCN__, cmnPanBin, i, sapPar[i].BRTools);
            }   

#endif
        }
        BP_Free(&config);
    }

    /* Now we let's create INSTANCE_LIST */

    if (count>0)
    {
        BP_Opt  config;
        int     i;

        DbgLogPlain(logFile, _T("\nCreating INSTANCE_LIST"));

        BP_InitAsList(&config, INSTANCE_LIST);

        if (BP_AddStrList(&config, INSTANCE_LIST)!=BP_NOERROR)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, 
                _T("[%s] Can not add option %s to configuration"),
                __FCN__,INSTANCE_LIST);
            DbgLogPlain(logFile, 
                _T("%sCan not add option %s to configuration"),
                ERROR_MINOR,INSTANCE_LIST);
            ErhClearError();
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
        }
        else
        {
            for (i=0;i<count;i++)
            {
                if(BP_ChangeStrListAdd(&config, INSTANCE_LIST, sapPar[i].sid)!=BP_NOERROR)
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED, 
                             _T("[%s] Can not add string %s to list %s in configuration"),
                             __FCN__, sapPar[i].sid?sapPar[i].sid:_T("<NULL>"), INSTANCE_LIST);
                    DbgLogPlain(logFile, 
                                _T("%sCan not add string %s to list %s in configuration"),
                                ERROR_MINOR, sapPar[i].sid?sapPar[i].sid:_T("<NULL>"), INSTANCE_LIST);

                    ErhClearError();
                    if (retVal!=RESULT_FAILURE)
                      retVal=RESULT_SOSO;
                }
            }
        }
        
        if (BU_PutConfig(CSHost,opt.appHost,OB2_APP_SAP,INSTANCE_LIST,config)!=BP_NOERROR)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                     _T("[%s] Can not save instance list on the Cell Server \"%s\" with instance \"%s\""),
                     __FCN__,CSHost,INSTANCE_LIST);
            DbgLogPlain(logFile,
                     _T("%sCan not save instance list on the Cell Server \"%s\" with instance \"%s\""),
                     ERROR_WARNING, CSHost,INSTANCE_LIST);
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
        }
        else
            DbgLogPlain(logFile, _T("Instance list stored successfully"));
        BP_Free(&config);   
    }

    for (i=0;i<count;i++)
        if (sapPar[i].barlists)
            FREE(sapPar[i].barlists)
    if (sapPar)
        FREE(sapPar);
    DbgFcnOut();
    return retVal;
}

static int UpgradeSAPRatio(tchar *CSHost)
{
    ERH_FUNCTION(_T("UpgradeSAPRatio"));

    BP_Opt   config,
            *listOpt;
    FILE    *F;
    int      retVal=RESULT_SUCCESS;
    tchar    path[STRLEN_PATH+1];

    DbgFcnIn();

    if (!opt.sid || !opt.sid[0]==_T('\0'))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sInstance name is missing."), 
            ERROR_FAILURE);
        DbgLogFull(__FLNPT__, logFile, _T("%sInstance name is missing."),
            ERROR_FAILURE);
        retVal=RESULT_FAILURE;
        goto exitUpgradeSAPRatio;
    }

    if (GetConfig(opt.sid,OB2_APP_SAP,&config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,
            _T("Reading SAP R/3 configuration for instance %s on host %s failed!"),
            opt.sid, opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sReading SAP R/3 configuration for instance %s on host %s failed!"),
            ERROR_FAILURE,
            opt.sid,
            opt.appHost);
        retVal=RESULT_FAILURE;
        goto exitUpgradeSAPRatio;
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sSAP R/3 configuration for instance %s on host %s read successfully."),
            ERROR_MINOR,opt.sid, opt.appHost);
    }
    
    if ((F=fopen(opt.optFile,_T("r")))!=NULL)
    {
        tchar str[STRLEN_PATH+STRLEN_INT+2];
        
        BP_AddList(&config,_T("compression"),&listOpt);
        
        DbgLogPlain(logFile, 
            _T("Adding compression ratios from file %s to section compression"),
            path);
        
        while (fgets(str,STRLEN_PATH+STRLEN_INT+1,F))
        {
            tchar   fname[STRLEN_PATH+1];
            long    speed=0;
            
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=_T('\0'); /* Cut off the last \n */
#if !TARGET_WIN32
            if ((sscanf(str,_T("%[^:]:%ld"),fname,&speed))==2)
#else
                if ((sscanf(str,_T("%[^#]#%ld"),fname,&speed))==2)
#endif
                {
                    BP_AddInt(listOpt,fname,speed);
                }
                else
                {
                    DbgLogPlain(logFile, _T("%sCan not parse line from %s\n%s"),
                        ERROR_WARNING,opt.optFile,str);
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,
                        _T("%sCan not parse line from %s\n%s"),
                        ERROR_WARNING,opt.optFile,str);
                    retVal=RESULT_SOSO;
                }
                
        }
    }
    else
    {
        DbgLogPlain(logFile, _T("%sCan not open file %s. Error: %d, %s"),
            ERROR_FAILURE, opt.optFile, errno, ErhSysErrnoToText(errno));
        retVal=RESULT_FAILURE;
        goto exitUpgradeSAPRatio;
    }
    
    if (PutConfig(opt.sid,OB2_APP_SAP,opt.appHost,&config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sStoring SAP R/3 configuration for instance %s on host %s failed!"),
            ERROR_FAILURE,
            opt.sid,
            opt.appHost);
        DbgLogFull(__FLNPT__, logFile, _T("%sStoring SAP R/3 configuration for instance %s on host %s failed!"),
            ERROR_FAILURE,
            opt.sid,
            opt.appHost);
        retVal=RESULT_FAILURE;
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sSAP R/3 configuration for instance %s on host %s stored successfully."),
            ERROR_FAILURE,opt.sid, opt.appHost);
        if (retVal!=RESULT_SUCCESS)
            DbgLogPlain(logFile, 
            _T("%sSome errors occured during SAP R/3 configuration upgrade. Please check on the CM if the configuration was moved as expected."),
            ERROR_WARNING);
    }

exitUpgradeSAPRatio:
    DbgFcnOut();
    return(retVal);
}

static int UpgradeSQL()
{
    ERH_FUNCTION(_T("UpgradeSQL"));

    int     retVal = RESULT_FAILURE;
#if TARGET_WIN32
    HKEY    hKey;
    DWORD   cbUser=STRLEN_PATH;
#endif
    LONG    err;
    tchar   saUser[STRLEN_PATH+1];
    tchar   saPassword[STRLEN_PATH+1];
    tchar   saServer[STRLEN_PATH+1];

    BP_Opt  config;
    memset(&config, 0, sizeof(BP_Opt)); 
    
    DbgFcnIn();
    
    DbgLogFull(__FLNPT__, logFile, _T("Starting MS SQL 7.0 configuration upgrade"));

    strcpy(saUser,_T(""));
    strcpy(saPassword,_T(""));


#if TARGET_WIN32
    retVal = RESULT_SUCCESS;
    if ((err=RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        SQL_REG,
        0,
        KEY_READ,
        &hKey
        )
        ) == ERROR_SUCCESS)
    {
        if ((err=RegQueryValueEx(
            hKey,
            SQL_USER,
            NULL,
            NULL,
            (LPBYTE) saUser,
            &cbUser
            )) != ERROR_SUCCESS)
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,err,0,saUser,STRLEN_PATH,0);

            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not read registry value HKLM\\%s\\%s"),
                __FCN__,SQL_REG,SQL_USER);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] %s%d, %s"),__FCN__,ERROR_WARNING,err,saUser);
            DbgLogPlain(logFile, _T("%sCan not read registry value HKLM\\%s\\%s"),
                ERROR_WARNING,SQL_REG,SQL_USER);
            DbgLogPlain(logFile, _T("%s%s"),ERROR_WARNING,saUser);
            saUser[0]=_T('\0');

            retVal = RESULT_FAILURE;
        }else
        if (saUser[0]==_T('\0'))
        {
            /*retVal=RESULT_FAILURE; HSLco26659 */
            DbgLogPlain(logFile, _T("%sHKLM\\%s\\%s is empty"), 
                ERROR_WARNING, SQL_REG, SQL_USER);
        }

        cbUser=STRLEN_PATH;
        if ((err=RegQueryValueEx(
            hKey,
            SQL_PASSWORD,
            NULL,
            NULL,
            (LPBYTE) saPassword,
            &cbUser
            )) != ERROR_SUCCESS)
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,err,0,saPassword,STRLEN_PATH,0);

            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not read registry value HKLM\\%s\\%s"),
                __FCN__,SQL_REG,SQL_PASSWORD);
            DbgLogPlain(logFile, _T("%sCan not read registry value HKLM\\%s\\%s"),
                ERROR_WARNING,SQL_REG,SQL_PASSWORD);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] %s%d, %s"),__FCN__,ERROR_WARNING,err,saPassword);
            DbgLogPlain(logFile, _T("%s%s"),ERROR_WARNING,saPassword);
            saPassword[0]=_T('\0');
            if (retVal!=RESULT_FAILURE)
                retVal = RESULT_SOSO;
        }

        cbUser=STRLEN_PATH;
        if ((err=RegQueryValueEx(
            hKey,
            SQL_SERVER,
            NULL,
            NULL,
            (LPBYTE) saServer,
            &cbUser
            )) != ERROR_SUCCESS)
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,err,0,saServer,STRLEN_PATH,0);

            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not read registry value HKLM\\%s\\%s"),
                __FCN__,SQL_REG,SQL_SERVER);
            DbgLogPlain(logFile, _T("%sCan not read registry value HKLM\\%s\\%s"),
                ERROR_MINOR,SQL_REG,SQL_SERVER);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] %s%d, %s"),__FCN__,ERROR_MINOR,err,saServer);
            DbgLogPlain(logFile, _T("%s%s"),ERROR_MINOR,saServer);

            strcpy(saServer,opt.appHost);
        }
    }
    else
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,err,0,saPassword,STRLEN_PATH,0);

        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not open registry key HKLM\\%s"),__FCN__,SQL_REG);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] %s%d, %s"),__FCN__,ERROR_FAILURE,err,saPassword);
        DbgLogPlain(logFile, _T("%sCan not open registry key HKLM\\%s"),ERROR_FAILURE,SQL_REG);
        DbgLogPlain(logFile, _T("%s%s\n"),ERROR_FAILURE,saPassword);
        saPassword[0]=_T('\0');
        
        retVal = RESULT_FAILURE;
        goto exit_SQL;
    }



    if (*saPassword || *saUser)
    {
        DbgLogPlain(logFile, 
            _T("Found configuration for instance \"(DEFAULT)\""),ERROR_MINOR);
        DbgLogPlain(logFile, 
            _T("Got the following MS SQL configuration parameters:"));
        DbgLogPlain(logFile, _T("\tsaUser=%s"),saUser);
        DbgLogPlain(logFile, _T("\tsaPassword=%s"),saPassword);
            
            BP_InitAsList(&config,_T("(DEFAULT)"));
        
        BP_AddStr(&config,SQL_NEW_USER,saUser);
        BP_AddStr(&config,SQL_NEW_PASS,saPassword);
        
        /* Now we are ready to store the configuration */
        
        if (PutConfig(_T("(DEFAULT)"),OB2_APP_MSSQL,saServer,&config)!=RESULT_SUCCESS)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,
                _T("%sStoring MS SQL 7.0 configuration for instance (DEFAULT) on host %s failed!"),
                ERROR_FAILURE, opt.appHost);
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sStoring MS SQL 7.0 configuration for instance (DEFAULT) on host %s failed!"),
                ERROR_FAILURE, opt.appHost);
            retVal=RESULT_FAILURE;
        }
        else
        {
            DbgLogFull(__FLNPT__, logFile, 
                _T("%s MS SQL 7.0 configuration for instance (DEFAULT) on host %s stored successfully."),
                ERROR_MINOR, opt.appHost);
            if (retVal!=RESULT_SUCCESS)
                DbgLogPlain(logFile, 
                _T("%sSome errors occured during MS SQL 7.0 configuration upgrade. Please check on the CM if the configuration was moved as expected.")
                ERROR_WARNING);
         }
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, _T("%sNo configured MS SQL 7.0 instances found"), ERROR_FAILURE);
        retVal=RESULT_FAILURE;
    }
    

    BP_Free(&config);
exit_SQL:
    RegCloseKey(hKey);
#endif
    DbgFcnOut();
    return retVal;
}

static int UpgradeOracle()
{
    ERH_FUNCTION(_T("UpgradeOracle"));

    int       retVal = RESULT_SUCCESS;
    BP_Opt    config,*temp;
    tchar     str[STRLEN_PATH+1] = {0},
              path[STRLEN_PATH+1] = {0},
              tmppath[STRLEN_PATH+1] = {0};
#if !TARGET_WIN32
    tchar     newpath[STRLEN_PATH+1]; /* #if-ed not to be removed accidentally when compiling on NT */
#endif
    FILE      *F;

    int       count=0,allocated=0,i,j;
    tchar     **sid=NULL;

    DbgFcnIn();

    DbgLogFull(__FLNPT__, logFile, _T("Starting Oracle 8 configuration upgrade"));

#if TARGET_WIN32
    sprintf(tmppath,_T("%soracle8"),cmnPanTmp);
#else
    sprintf(tmppath,_T("/tmp/oracle8"));
#endif

    if (OB2_CreateDirectory(tmppath,0777) && 
#if TARGET_WIN32
        GetLastError()!=ERROR_ALREADY_EXISTS
#else
        errno!=EEXIST
#endif
        )
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
            __FCN__, tmppath);
        DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
            tmppath, ErhSysErrnoToText(errno));
        *tmppath=_T('\0');
    }
    else
    {
        DbgLogPlain(logFile, 
            _T("Directory %s created. Old configuration will be moved there."), tmppath);
    }

#if TARGET_WIN32
    {
        HKEY    hKey,hKeyEnv;
        LONG    err;
        DWORD   dwIndex;
        DWORD   cbName,cbClass,cbName2;
        tchar   Name[STRLEN_PATH+1];
        tchar   Class[STRLEN_PATH+1];
        tchar   Name2[STRLEN_PATH+1];
        FILETIME ft;
        
        if ((err=RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGORACLE8,
            0,
            KEY_READ,
            &hKey
            )
            ) == ERROR_SUCCESS)
        {
            dwIndex = 0;
            cbName = cbClass = STRLEN_PATH;     /* set enough space for first query */
            
            while ((err=RegEnumKeyEx(
                hKey,
                dwIndex,
                Name,
                &cbName,
                NULL,
                Class,
                &cbClass,
                &ft
                )) != ERROR_NO_MORE_ITEMS)
            {
                dwIndex++;
                if (err == ERROR_SUCCESS)
                {
                    DbgPlain(DBG_OB2BAR,_T("Found configuration for Oracle8 instance %s"),Name);
                    DbgLogPlain(logFile, _T("Found configuration for Oracle8 instance %s"),Name);

                    if ((err=RegOpenKeyEx(
                        hKey,
                        Name,
                        0,
                        KEY_READ,
                        &hKeyEnv
                        )) == ERROR_SUCCESS)
                    {
                        if (count==allocated)
                        {
                            sid = (tchar **)REALLOC(sid, 
                                (10+allocated)*sizeof(tchar *));
                            memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                            allocated+=10;
                        }
                        sid[count]=StrNewCopy(Name);
                        i=count;
                        BP_InitAsList(&config,sid[count++]);

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            ORACLE_HOME,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,ORACLE_HOME,Name2);
                            DbgLogPlain(logFile, _T("\tORACLE_HOME=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get ORACLE_HOME from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        /* Let's find out our executable and get the version */                        

                        /* Find suitable RMAN binary */
                        for (j=0; RMAN_list[j]; j++)
                        {
                            tchar tmpPath[STRLEN_PATH+1] = {0};
                            tchar tmpPath2[STRLEN_PATH+1] = {0};

                            sprintf(tmpPath, _T("ORACLE_SID=%s"), sid[i]);
                            PutEnv(tmpPath);
                            if (strlen(str) > 0)
                            {
                                sprintf(tmpPath, _T("ORACLE_HOME=%s"), str);
                                PutEnv(tmpPath);
                            }

                            tmpPath[0] = 0;

                            PathConcat(tmpPath, Name2, _T("bin"), STRLEN_PATH);
                            PathConcat(tmpPath2, tmpPath, RMAN_list[j], STRLEN_PATH);
                            if ( access(tmpPath2, X_OK) >= 0 )
                            {
                                RMAN_cmd_idx = j;
                                RMAN_cmd = StrNewCopy(tmpPath2);
                                break;
                            }
                        }
                        if ( getOracleVersion(Name2, RMAN_list[RMAN_cmd_idx]) >= 0 )
                        {
                            tchar tmpPath[STRLEN_PATH+1];
                            
                            sprintf( tmpPath, _T("%d.%d.%d"),
                                oracle_version.relMaj > 0 ? oracle_version.relMaj : 8,
                                oracle_version.relMid,
                                oracle_version.relMin );
                            if (oracle_version.resto)
                            {
                                StrCatEx( tmpPath, _T(" "),              STRLEN_PATH, NULL );
                                StrCatEx( tmpPath, oracle_version.resto, STRLEN_PATH, NULL );
                            }
                            
                            /* VERSION */
                            BP_AddStr(&config, ORA_VERSION, tmpPath);
                            DbgLogPlain(logFile, _T("\t%s=%s"), ORA_VERSION, tmpPath);
                        }
                        
                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            ORA_TGTLOGIN,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,ORA_TGTLOGIN,Name2);
                            DbgLogPlain(logFile, _T("\tORA_TGTLOGIN=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get ORA_TGTLOGIN from registry"),ERROR_WARNING);
                                
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            ORA_RCVLOGIN,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,ORA_RCVLOGIN,Name2);
                            DbgLogPlain(logFile, _T("\tORA_RCVLOGIN=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get ORA_RCVLOGIN from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        BP_AddList(&config,REG_ENV,&temp);

                        if (PutConfig(Name,OB2_APP_ORACLE8,opt.appHost,&config)!=RESULT_SUCCESS)
                        {
                            DbgStamp(DBG_UNEXPECTED);
                            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Oracle8 configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            DbgLogPlain(logFile, 
                                _T("%sStoring Oracle8 configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            retVal=RESULT_FAILURE;
                        }
                        else
                        {
                            DbgLogFull(__FLNPT__, logFile, 
                                _T("%sOracle8 Configuration for instance %s on host %s stored successfully."),
                                ERROR_MINOR,Name,opt.appHost);
                            if (retVal!=RESULT_SUCCESS)
                                DbgLogPlain(logFile, 
                                _T("%sSome errors occured during Oracle8 configuration upgrade. Please check on the CM if the configuration was moved as expected.")
                                ERROR_WARNING);
                        }

                    }

                    cbName = cbClass = STRLEN_PATH;     /* set enough space for next query */
                }
                
            }
            
            RegCloseKey(hKey);
        }

    }
#else
    sprintf(path,_T("%s/oracle8/data_servers"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_FILE,F))
        {
            DbgPlain(DBG_OB2BAR, _T("count==%d, allocated==%d"), count, allocated);
            if (count==allocated)
            {
                DbgStamp(DBG_OB2BAR);
                sid = (tchar **)REALLOC(sid, 
                    (10+allocated)*sizeof(tchar *));
                DbgStamp(DBG_OB2BAR);
                memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                allocated+=10;
            }
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=0;
            sid[count++]=StrNewCopy(str);
            DbgLogPlain(logFile, _T("Found configuration for Oracle8 instance %s"),str);
            DbgPlain(DBG_OB2BAR, _T("Found configuration for Oracle8 instance %s"),str);
        }
        DbgStamp(DBG_OB2BAR);
        fclose(F);
        RenameFile(path, tmppath, _T("data_servers"));
    }
    else
    {
        DbgStamp(DBG_OB2BAR);
        DbgPlain(DBG_OB2BAR, _T("[%s] Can not open file %s. Error: %d, %s"),
            __FCN__, path, errno, ErhSysErrnoToText(errno));
        DbgLogPlain(logFile, _T("%sCan not open data_servers file at %s. Error: %d, %s"),
            ERROR_FAILURE, path, errno, ErhSysErrnoToText(errno));
        retVal=RESULT_FAILURE;
    }
    
    DbgStamp(DBG_OB2BAR);
    for (i=0;i<count;i++)
    {
        BP_InitAsList(&config,OB2_APP_ORACLE8);
        BP_AddStr(&config,ORA_VERSION,_T("8.0.0"));

        sprintf(newpath,_T("%s/%s"),tmppath, sid[i]);
        if (OB2_CreateDirectory(newpath,0777))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
                __FCN__, newpath);
            DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
                newpath, ErhSysErrnoToText(errno));
        }
        else
        {
            DbgLogPlain(logFile, 
                _T("Directory %s created."),newpath);
        }
        
        sprintf(path,_T("%s/oracle8/%s/oracle_home"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config, ORACLE_HOME, str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, _T("%sCan not read oracle_home from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
                str[0] = 0;
            }
            fclose(F);
            sprintf(newpath, _T("%s/oracle_home"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open oracle_home file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sContinuing to another instance because the oracle_home file is missing for this instance."),
                ERROR_WARNING);
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
            continue;
        }
        
        /* Let's find out our executable and get the version */                        
        
        /* Find suitable RMAN binary */
        for (j=0; RMAN_list[j]; j++)
        {
            tchar tmpPath[STRLEN_PATH+1] = {0};
            tchar tmpPath2[STRLEN_PATH+1] = {0};
            
            sprintf(tmpPath, _T("ORACLE_SID=%s"), sid[i]);
            PutEnv(tmpPath);
            if (strlen(str) > 0)
            {
                sprintf(tmpPath, _T("ORACLE_HOME=%s"), str);
                PutEnv(tmpPath);
            }

            tmpPath[0] = 0;
            
            PathConcat(tmpPath, str, _T("bin"), STRLEN_PATH);
            PathConcat(tmpPath2, tmpPath, RMAN_list[j], STRLEN_PATH);
            if ( access(tmpPath2, X_OK) >= 0 )
            {
                RMAN_cmd_idx = j;
                RMAN_cmd = StrNewCopy(tmpPath2);
                break;
            }
        }
        if ( 
               ( getOracleVersion(str, RMAN_list[RMAN_cmd_idx]) >= 0 ) &&
               ( getBitOracleVersion(str) >= 0 )
           )
        {
            tchar tmpPath[STRLEN_PATH+1];
            
            sprintf( tmpPath, _T("%d.%d.%d"),
                oracle_version.relMaj>0 ? oracle_version.relMaj : 8,
                oracle_version.relMid,
                oracle_version.relMin ); 
            
            if (oracle_version.resto)
            {
                StrCatEx( tmpPath, _T(" "),              STRLEN_PATH, NULL );
                StrCatEx( tmpPath, oracle_version.resto, STRLEN_PATH, NULL );
            }
            
            /* VERSION */
            BP_ChangeStr(&config, ORA_VERSION, tmpPath);
            DbgLogPlain(logFile, _T("\t%s=%s"), ORA_VERSION, tmpPath);
        }
        else
        {
            BP_ChangeStr(&config, ORA_VERSION, _T("8.0.0"));
        }
        
        sprintf(path,_T("%s/oracle8/%s/tgtconn"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,ORA_TGTLOGIN,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read target connection string from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
            }
            fclose(F);
            sprintf(newpath, _T("%s/tgtconn"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open target connection file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }
        
        sprintf(path,_T("%s/oracle8/%s/catconn"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,ORA_RCVLOGIN,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/catconn"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }

        sprintf(path,_T("%s/oracle8/%s/location"),cmnPanConfigBase,sid[i]); /* SMB control file location - optional! */
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,ORA_SMB_LOC,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/location"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else if (errno!=ENOENT)
        {
            DbgLogPlain(logFile, 
                _T("%sCan not read location of control file for Split mirror backup from file %s. %s"),
                ERROR_MINOR, path, ErhSysErrnoToText(errno)); 
        }

        sprintf(path,_T("%s/oracle8/%s/init_bckp.ora"),cmnPanConfigBase,sid[i]); /* init.ora file used for starting DB on R2 - optional! */
        DbgPlain(DBG_UNEXPECTED, _T("path=%s"), path);
        {
            struct stat tmp_buf;
            int         handle=-1;
            tchar       filename[STRLEN_FILE+1];

            if (!stat(path, &tmp_buf))
            {
                tchar  *init_ora_Buf = NULL;
                int    group = G_TOP_INTEG_CNF;

                init_ora_Buf = CmnGetAsciiFile(path);

                sprintf(filename,_T("config/%s/%s%%init%s_bckp.ora"), OB2_APP_ORACLE8, opt.appHost, sid[i]);

                DbgPlain(DBG_UNEXPECTED, _T("filename=%s"), filename);
                if ((handle = MCsaBindServer(CSHost))<0)
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not bind to Cell Server %s. Error: %d, %s"),
                             __FCN__,CSHost,ErhErrno(),ErhSysErrorStr());
                    DbgLogPlain(logFile, _T("%sinit_bckp.ora file transfer failed"), ERROR_WARNING);
                    if (retVal!=RESULT_FAILURE)
                        retVal=RESULT_SOSO;
                }
                else
                if (MCsaPutGroupItem(handle, group, filename, init_ora_Buf, MODIFY)<0)
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not put file %s to the Cell Server. Error: %d, %s"),
                             __FCN__,filename,ErhErrno(),ErhSysErrorStr());
                    DbgLogPlain(logFile, _T("%sinit_bckp.ora file transfer failed"), ERROR_WARNING);
                    if (retVal!=RESULT_FAILURE)
                        retVal=RESULT_SOSO;
                }
        else
        {
            sprintf(newpath, _T("%s/init_bckp.ora"),sid[i]);
                RenameFile(path, tmppath, newpath);
        }
                FREE(init_ora_Buf);
            }
        else if (errno!=ENOENT)
        {
        DbgLogPlain(logFile, _T("%sCan not read location of control file for Split mirror backup from file %s. %s"),
            ERROR_MINOR, path, ErhSysErrnoToText(errno));

        }
        }

        if (PutConfig(sid[i],OB2_APP_ORACLE8,opt.appHost,&config)!=RESULT_SUCCESS)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Oracle8 configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sStoring Oracle8 configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            retVal=RESULT_FAILURE;
        }
        else
        {
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sOracle8 Configuration for instance %s on host %s stored successfully."),
                ERROR_MINOR,sid[i],opt.appHost);
            if (retVal!=RESULT_SUCCESS)
                DbgLogPlain(logFile, 
                    _T("%sSome errors occured during Oracle8 configuration upgrade. Please check on the CM if the configuration was moved as expected."),
                    ERROR_WARNING);
        }
        
        BP_Free(&config);
    }

#endif

    if (GetConfig(STR_GLOBAL, OB2_APP_ORACLE8, &config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sLoading global Oracle8 configuration for host %s failed!"),
            ERROR_MINOR,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sCan not load global Oracle8 configuration for host %s. Creating a new one!"),
            ERROR_MINOR,opt.appHost);
        BP_InitAsList(&config,STR_GLOBAL);
        ErhClearError();
    }
    
    if (count)
    {
        BP_StrListOpt   instances;

        memset(&instances, 0, sizeof(BP_StrListOpt));
        if (BP_GetStrListByName(config, STR_INST_LIST, &instances) != BP_NOERROR)
        {
            BP_AddStrList(&config,STR_INST_LIST);
            BP_GetStrListByName(config, STR_INST_LIST, &instances);
        }
        
        for (i=0;i<count;i++)
        {
            int j, found=0;
            for (j=0;j<BP_StrListCount(&instances); j++)
            {
                if (!StrCmp(BP_StrListAt(&instances,j), sid[i]))
                {
                    found=1;
                    break;
                }
            }
            if (!found)
                BP_ChangeStrListAdd(&config,STR_INST_LIST,sid[i]);
        }
    }
    
    if ((temp=BP_GetOptByName(config, REG_ENV)) == NULL) 
        BP_AddList(&config,REG_ENV,&temp);
    
    sprintf(path,_T("%s/oracle8/oracle8.conf"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_PATH,F))
        {
            tchar *c1,*c2;
            
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=_T('\0');
            c2=strchr(str,_T('='));
            if (c2 != NULL)
            {
                c1=c2-1;
                while (*c1==_T(' ') || *c1==_T('\t'))
                    c1--;
                *(c1+1)=_T('\0');
                c2++;
                while (*c2==_T(' ') || *c2==_T('\t'))
                    c2--;
                BP_RemoveOptByName(temp, str);
                BP_AddStr(temp,str,c2);
                DbgLogPlain(logFile, _T("Adding option %s=%s to global environment section"),
                    str,c2);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
            }
        }
        fclose(F);
        RenameFile(path, tmppath, _T("oracle8.conf"));
    }
    else
    {
        DbgLogPlain(logFile, _T("%sCan not open Oracle 8 environment file %s. %s"),
            ERROR_WARNING, path, ErhSysErrnoToText(errno));
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sCan not open file %s"),ERROR_WARNING,path);
        if (retVal!=RESULT_FAILURE)
            retVal=RESULT_SOSO;
    }
    
    if (PutConfig(STR_GLOBAL,OB2_APP_ORACLE8,opt.appHost,&config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sStoring global Oracle8 configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sStoring global Oracle8 configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        retVal=RESULT_FAILURE;
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sGlobal Oracle8 configuration on host %s stored successfully."),
            ERROR_MINOR,opt.appHost);
        if (retVal!=RESULT_SUCCESS)
            DbgLogPlain(logFile, 
            _T("%sSome errors occured during Oracle8 configuration upgrade. Please check on the CM if the configuration was moved as expected."),
            ERROR_WARNING);
    }
    
    if (sid)
        FREE(sid);
    BP_Free(&config);
    
    DbgFcnOut();
    return retVal;
}



static int UpgradeInformix()
{

    ERH_FUNCTION(_T("UpgradeInformix"));

    int       retVal = RESULT_SUCCESS;
    BP_Opt    config,*temp;
    tchar     str[STRLEN_PATH+1],
              path[STRLEN_PATH+1],
              tmppath[STRLEN_PATH+1];
#if !TARGET_WIN32
    tchar     newpath[STRLEN_PATH+1]; /* #if-ed not to be removed accidentally when compiling on NT */
#endif

    int       count=0,allocated=0,i;
    tchar     **sid=NULL;

    FILE      *F;

    DbgFcnIn();

    DbgLogFull(__FLNPT__, logFile, _T("Starting Informix configuration upgrade"));

#if TARGET_WIN32
    sprintf(tmppath,_T("%sinformix"),cmnPanTmp);
#else
    sprintf(tmppath,_T("/tmp/informix"));
#endif

    if (OB2_CreateDirectory(tmppath,0777) && 
#if TARGET_WIN32
        GetLastError()!=ERROR_ALREADY_EXISTS
#else
        errno!=EEXIST
#endif
        )
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
            __FCN__, tmppath);
        DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
            tmppath, ErhSysErrnoToText(errno));
        *tmppath=_T('\0');
    }
    else
    {
        DbgLogPlain(logFile, 
            _T("Directory %s created. Old configuration will be moved there."), tmppath);
    }

#if TARGET_WIN32
    {
        HKEY    hKey,hKeyEnv;
        LONG    err;
        DWORD   dwIndex;
        DWORD   cbName,cbClass,cbName2;
        tchar   Name[STRLEN_PATH+1];
        tchar   Class[STRLEN_PATH+1];
        tchar   Name2[STRLEN_PATH+1];
        FILETIME ft;
        
        if ((err=RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGINFORMIX,
            0,
            KEY_READ,
            &hKey
            )
            ) == ERROR_SUCCESS)
        {
            dwIndex = 0;
            cbName = cbClass = STRLEN_PATH;     /* set enough space for first query */
            
            while ((err=RegEnumKeyEx(
                hKey,
                dwIndex,
                Name,
                &cbName,
                NULL,
                Class,
                &cbClass,
                &ft
                )) != ERROR_NO_MORE_ITEMS)
            {
                dwIndex++;
                if (err == ERROR_SUCCESS)
                {
                    DbgPlain(DBG_OB2BAR,_T("Found configuration for Informix instance %s"),Name);
                    DbgLogPlain(logFile, _T("Found configuration for Informix instance %s"),Name);

                    if ((err=RegOpenKeyEx(
                        hKey,
                        Name,
                        0,
                        KEY_READ,
                        &hKeyEnv
                        )) == ERROR_SUCCESS)
                    {
                        if (count==allocated)
                        {
                            sid = (tchar **)REALLOC(sid, 
                                (10+allocated)*sizeof(tchar *));
                            memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                            allocated+=10;
                        }
                        sid[count]=StrNewCopy(Name);
                        i=count;
                        BP_InitAsList(&config,sid[count++]);

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            INFORMIX_HOME,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,INFORMIX_HOME,Name2);
                            DbgLogPlain(logFile, _T("\tINFORMIX_HOME=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get INFORMIX_HOME from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }
                      
                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            INFORMIX_SQLHOSTS,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,INFORMIX_SQLHOSTS,Name2);
                            DbgLogPlain(logFile, _T("\tINFORMIX_SQLHOSTS=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get INFORMIX_SQLHOSTS from registry"),ERROR_WARNING);
                                
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            INFORMIX_ONCONFIG,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,INFORMIX_ONCONFIG,Name2);
                            DbgLogPlain(logFile, _T("\tINFORMIX_ONCONFIG=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get INFORMIX_ONCONFIG from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        BP_AddList(&config,REG_ENV,&temp);

                        sprintf(path,_T("%s/informix/%s.profile"),cmnPanConfigBase,sid[i]);
    
                        if ((F=fopen(path,_T("r")))!=NULL)
                        {
                            tchar local_profile[STRLEN_PATH+1]={0};

                            while (fgets(str,STRLEN_PATH,F))
                            {
                                tchar *c1,*c2;
            
                                if (str[0] && str[strlen(str)-1]==_T('\n'))
                                    str[strlen(str)-1]=_T('\0');
                                c2=strchr(str,_T('='));
                                if (c2 != NULL)
                                {
                                    c1=c2-1;
                                    while (*c1==_T(' ') || *c1==_T('\t'))
                                        c1--;
                                    *(c1+1)=_T('\0');
                                    c2++;
                                    while (*c2==_T(' ') || *c2==_T('\t'))
                                        c2--;
                                    BP_RemoveOptByName(temp, str);
                                    BP_AddStr(temp,str,c2);
                                    DbgLogPlain(logFile, _T("Adding option %s=%s to local environment section"),
                                        str,c2);
                                }
                                else
                                {
                                    DbgStamp(DBG_UNEXPECTED);
                                    DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                                    DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
                                }
                            }
                            fclose(F);
                            sprintf(local_profile,_T("%s.profile"),sid[i]);
                            RenameFile(path, tmppath, local_profile);
                        }
                        else
                        {
                            DbgStamp(DBG_OB2BAR);
                            DbgPlain(DBG_OB2BAR,_T("Could not open %s"),path); 
                        }
        
                        if (PutConfig(Name,OB2_APP_INFORMIX,opt.appHost,&config)!=RESULT_SUCCESS)
                        {
                            DbgStamp(DBG_UNEXPECTED);
                            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Informix configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            DbgLogPlain(logFile, 
                                _T("%sStoring Informix configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            retVal=RESULT_FAILURE;
                        }
                        else
                        {
                            DbgLogFull(__FLNPT__, logFile, 
                                _T("%sInformix Configuration for instance %s on host %s stored successfully."),
                                ERROR_MINOR,Name,opt.appHost);
                            if (retVal!=RESULT_SUCCESS)
                                DbgLogPlain(logFile, 
                                _T("%sSome errors occured during Informix configuration upgrade. Please check on the CM if the configuration was moved as expected.")
                                ERROR_WARNING);
                        }

                    }

                    cbName = cbClass = STRLEN_PATH;     /* set enough space for next query */
                }
                
            }
            
            RegCloseKey(hKey);
        }

    }
#else
    sprintf(path,_T("%s/informix/data_servers"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_FILE,F))
        {
            DbgPlain(DBG_OB2BAR, _T("count==%d, allocated==%d"), count, allocated);
            if (count==allocated)
            {
                DbgStamp(DBG_OB2BAR);
                sid = (tchar **)REALLOC(sid, 
                    (10+allocated)*sizeof(tchar *));
                DbgStamp(DBG_OB2BAR);
                memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                allocated+=10;
            }
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=0;
            sid[count++]=StrNewCopy(str);
            DbgLogPlain(logFile, _T("Found configuration for Informix instance %s"),str);
            DbgPlain(DBG_OB2BAR, _T("Found configuration for Informix instance %s"),str);
        }
        DbgStamp(DBG_OB2BAR);
        fclose(F);
        RenameFile(path, tmppath, _T("data_servers"));
    }
    else
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
            __FCN__, path, errno, ErhSysErrnoToText(errno));
        DbgLogPlain(logFile, _T("%sCan not open data_servers file at %s. Error: %d, %s"),
            ERROR_FAILURE, path, errno, ErhSysErrnoToText(errno));
        retVal=RESULT_FAILURE;
    }
    
    DbgStamp(DBG_OB2BAR);
    for (i=0;i<count;i++)
    {
        BP_InitAsList(&config,OB2_APP_INFORMIX);

        sprintf(newpath,_T("%s/%s"),tmppath, sid[i]);
        if (OB2_CreateDirectory(newpath,0777))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
                __FCN__, newpath);
            DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
                newpath, ErhSysErrnoToText(errno));
        }
        else
        {
            DbgLogPlain(logFile, 
                _T("Directory %s created."),newpath);
        }
        
        sprintf(path,_T("%s/informix/%s/informixdir"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config, INFORMIX_HOME, str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, _T("%sCan not read oracle_home from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
                str[0] = 0;
            }
            fclose(F);
            sprintf(newpath, _T("%s/informixdir"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open oracle_home file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sContinuing to another instance because the oracle_home file is missing for this instance."),
                ERROR_WARNING);
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
            continue;
        }     
        
        sprintf(path,_T("%s/informix/%s/informixsqlhosts"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,INFORMIX_SQLHOSTS,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read target connection string from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
            }
            fclose(F);
            sprintf(newpath, _T("%s/informixsqlhosts"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open target connection file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }
        
        sprintf(path,_T("%s/informix/%s/onconfig"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,INFORMIX_ONCONFIG,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/onconfig"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }

        BP_AddList(&config,REG_ENV,&temp);
        sprintf(path,_T("%s/informix/%s/.profile"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            tchar local_profile[STRLEN_PATH+1]={0};

            while (fgets(str,STRLEN_PATH,F))
            {
                tchar *c1,*c2;
            
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=_T('\0');
                c2=strchr(str,_T('='));
                if (c2 != NULL)
                {
                    c1=c2-1;
                    while (*c1==_T(' ') || *c1==_T('\t'))
                        c1--;
                    *(c1+1)=_T('\0');
                    c2++;
                    while (*c2==_T(' ') || *c2==_T('\t'))
                        c2--;
                    BP_RemoveOptByName(temp, str);
                    BP_AddStr(temp,str,c2);
                    DbgLogPlain(logFile, _T("Adding option %s=%s to local environment section"),
                        str,c2);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                    DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
                }
            }
            fclose(F);
            sprintf(local_profile,_T("%s/.profile"),sid[i]);
            RenameFile(path, tmppath, local_profile);
        }
        else
        {
            DbgStamp(DBG_OB2BAR);
            DbgPlain(DBG_OB2BAR,_T("Could not open %s"),path); 
        }
    
        if (PutConfig(sid[i],OB2_APP_INFORMIX,opt.appHost,&config)!=RESULT_SUCCESS)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Informix configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sStoring Informix configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            retVal=RESULT_FAILURE;
        }
        else
        {
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sInformix Configuration for instance %s on host %s stored successfully."),
                ERROR_MINOR,sid[i],opt.appHost);
            if (retVal!=RESULT_SUCCESS)
                DbgLogPlain(logFile, 
                    _T("%sSome errors occured during Informix configuration upgrade. Please check on the CM if the configuration was moved as expected."),
                    ERROR_WARNING);
        }
        
        BP_Free(&config);
    }

#endif

    if (GetConfig(STR_GLOBAL, OB2_APP_INFORMIX, &config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sLoading global Informix configuration for host %s failed!"),
            ERROR_MINOR,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sCan not load global Informix configuration for host %s. Creating a new one!"),
            ERROR_MINOR,opt.appHost);
        BP_InitAsList(&config,STR_GLOBAL);
        ErhClearError();
    }
    
    if (count)
    {
        BP_StrListOpt   instances;

        memset(&instances, 0, sizeof(BP_StrListOpt));
        if (BP_GetStrListByName(config, STR_INST_LIST, &instances) != BP_NOERROR)
        {
            BP_AddStrList(&config,STR_INST_LIST);
            BP_GetStrListByName(config, STR_INST_LIST, &instances);
        }
        
        for (i=0;i<count;i++)
        {
            int j, found=0;
            for (j=0;j<BP_StrListCount(&instances);j++)
            {
                if (!StrCmp(BP_StrListAt(&instances,j), sid[i]))
                {
                    found=1;
                    break;
                }
            }
            if (!found)
                BP_ChangeStrListAdd(&config,STR_INST_LIST,sid[i]);
        }
    }
    
    if ((temp=BP_GetOptByName(config, REG_ENV)) == NULL) 
        BP_AddList(&config,REG_ENV,&temp);
      
    sprintf(path,_T("%s/informix/informix.conf"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_PATH,F))
        {
            tchar *c1,*c2;
            
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=_T('\0');
            c2=strchr(str,_T('='));
            if (c2 != NULL)
            {
                c1=c2-1;
                while (*c1==_T(' ') || *c1==_T('\t'))
                    c1--;
                *(c1+1)=_T('\0');
                c2++;
                while (*c2==_T(' ') || *c2==_T('\t'))
                    c2--;
                BP_RemoveOptByName(temp, str);
                BP_AddStr(temp,str,c2);
                DbgLogPlain(logFile, _T("Adding option %s=%s to global environment section"),
                    str,c2);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
            }
        }
        fclose(F);
        RenameFile(path, tmppath, _T("informix.conf"));
    }
    else
    {
        DbgLogPlain(logFile, _T("%sCan not open Informix environment file %s. %s"),
            ERROR_WARNING, path, ErhSysErrnoToText(errno));
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sCan not open file %s"),ERROR_WARNING,path);
        if (retVal!=RESULT_FAILURE)
            retVal=RESULT_SOSO;
    }
    
    if (PutConfig(STR_GLOBAL,OB2_APP_INFORMIX,opt.appHost,&config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sStoring global Informix configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sStoring global Informix configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        retVal=RESULT_FAILURE;
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sGlobal Informix configuration on host %s stored successfully."),
            ERROR_MINOR,opt.appHost);
        if (retVal!=RESULT_SUCCESS)
            DbgLogPlain(logFile, 
            _T("%sSome errors occured during Informix configuration upgrade. Please check on the CM if the configuration was moved as expected."),
            ERROR_WARNING);
    }
    
    if (sid)
        FREE(sid);
    BP_Free(&config);
    
    DbgFcnOut();
    return retVal;
}


static int UpgradeSybase()
{
    ERH_FUNCTION(_T("UpgradeSybase"));

    int       retVal = RESULT_SUCCESS;
    BP_Opt    config,*temp;
    tchar     str[STRLEN_PATH+1],
              path[STRLEN_PATH+1],
              tmppath[STRLEN_PATH+1];
#if !TARGET_WIN32
    tchar     newpath[STRLEN_PATH+1]; /* #if-ed not to be removed accidentally when compiling on NT */
#endif

    int       count=0,allocated=0,i;
    tchar     **sid=NULL;

    FILE      *F;

    DbgFcnIn();

    DbgLogFull(__FLNPT__, logFile, _T("Starting Sybase configuration upgrade"));

#if TARGET_WIN32
    sprintf(tmppath,_T("%ssybase"),cmnPanTmp);
#else
    sprintf(tmppath,_T("/tmp/sybase"));
#endif

    if (OB2_CreateDirectory(tmppath,0777) && 
#if TARGET_WIN32
        GetLastError()!=ERROR_ALREADY_EXISTS
#else
        errno!=EEXIST
#endif
        )
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
            __FCN__, tmppath);
        DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
            tmppath, ErhSysErrnoToText(errno));
        *tmppath=_T('\0');
    }
    else
    {
        DbgLogPlain(logFile, 
            _T("Directory %s created. Old configuration will be moved there."), tmppath);
    }

#if TARGET_WIN32
    {
        HKEY    hKey,hKeyEnv;
        LONG    err;
        DWORD   dwIndex;
        DWORD   cbName,cbClass,cbName2;
        tchar   Name[STRLEN_PATH+1];
        tchar   Class[STRLEN_PATH+1];
        tchar   Name2[STRLEN_PATH+1];
        FILETIME ft;
        
        if ((err=RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGSYBASE,
            0,
            KEY_READ,
            &hKey
            )
            ) == ERROR_SUCCESS)
        {
            dwIndex = 0;
            cbName = cbClass = STRLEN_PATH;     /* set enough space for first query */
            
            while ((err=RegEnumKeyEx(
                hKey,
                dwIndex,
                Name,
                &cbName,
                NULL,
                Class,
                &cbClass,
                &ft
                )) != ERROR_NO_MORE_ITEMS)
            {
                dwIndex++;
                if (err == ERROR_SUCCESS)
                {
                    tchar   sybaseLibPath[STRLEN_PATH+1]={0};

                    DbgPlain(DBG_OB2BAR,_T("Found configuration for Sybase instance %s"),Name);
                    DbgLogPlain(logFile, _T("Found configuration for Sybase instance %s"),Name);

                    if ((err=RegOpenKeyEx(
                        hKey,
                        Name,
                        0,
                        KEY_READ,
                        &hKeyEnv
                        )) == ERROR_SUCCESS)
                    {
                        if (count==allocated)
                        {
                            sid = (tchar **)REALLOC(sid, 
                                (10+allocated)*sizeof(tchar *));
                            memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                            allocated+=10;
                        }
                        sid[count]=StrNewCopy(Name);
                        i=count;
                        BP_InitAsList(&config,sid[count++]);

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_HOME_DIR,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_HOME_DIR,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_HOME_DIR=%s"),Name2);
                            strcpy(sybaseLibPath,Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_HOME_DIR from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }
                      
                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_ISQL_PATH,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_ISQL_PATH,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_ISQL_PATH=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_ISQL_PATH from registry"),ERROR_WARNING);
                                
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_SA_USER,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_SA_USER,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_SA_USER=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_SA_USER from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_SA_PASSWORD,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_SA_PASSWORD,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_SA_PASSWORD=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_SA_PASSWORD from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_SYBASE_ASE,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_SYBASE_ASE,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_SYBASE_ASE=%s"),Name2);

                            if (strcmp(Name2,_T("")))
                            {
                                strcat(sybaseLibPath,_T("\\"));
                                strcat(sybaseLibPath,Name2);
                            }
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_SYBASE_ASE from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }

                        cbName2 = STRLEN_PATH;      /* set enough space for next query */
                        if ((err=RegQueryValueEx(
                            hKeyEnv,
                            SYBASE_SYBASE_OCS,
                            NULL,
                            NULL,
                            (LPBYTE)Name2,
                            &cbName2
                            )) == ERROR_SUCCESS)
                        {                            
                            BP_AddStr(&config,SYBASE_SYBASE_OCS,Name2);
                            DbgLogPlain(logFile, _T("\tSYBASE_SYBASE_OCS=%s"),Name2);
                        }
                        else
                        {
                            DbgLogPlain(logFile, 
                                _T("%sCan not get SYBASE_SYBASE_OCS from registry"),ERROR_WARNING);
                            if (retVal!=RESULT_FAILURE)
                                retVal=RESULT_SOSO;
                        }
                        
                        BP_AddList(&config,REG_ENV,&temp);

                        sprintf(path,_T("%s/sybase/%s.profile"),cmnPanConfigBase,sid[i]);
    
                        if ((F=fopen(path,_T("r")))!=NULL)
                        {
                            tchar local_profile[STRLEN_PATH+1]={0};

                            while (fgets(str,STRLEN_PATH,F))
                            {
                                tchar *c1,*c2;
            
                                if (str[0] && str[strlen(str)-1]==_T('\n'))
                                    str[strlen(str)-1]=_T('\0');
                                c2=strchr(str,_T('='));
                                if (c2 != NULL)
                                {
                                    c1=c2-1;
                                    while (*c1==_T(' ') || *c1==_T('\t'))
                                        c1--;
                                    *(c1+1)=_T('\0');
                                    c2++;
                                    while (*c2==_T(' ') || *c2==_T('\t'))
                                        c2--;
                                    BP_RemoveOptByName(temp, str);
                                    BP_AddStr(temp,str,c2);
                                    DbgLogPlain(logFile, _T("Adding option %s=%s to local environment section"),
                                        str,c2);
                                }
                                else
                                {
                                    DbgStamp(DBG_UNEXPECTED);
                                    DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                                    DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
                                }
                            }
                            fclose(F);
                            sprintf(local_profile,_T("%s.profile"),sid[i]);
                            RenameFile(path, tmppath, local_profile);
                        }
                        else
                        {
                            DbgStamp(DBG_OB2BAR);
                            DbgPlain(DBG_OB2BAR,_T("Could not open %s"),path); 
                        }

                        if (PutConfig(Name,OB2_APP_SYBASE,opt.appHost,&config)!=RESULT_SUCCESS)
                        {
                            DbgStamp(DBG_UNEXPECTED);
                            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Sybase configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            DbgLogPlain(logFile, 
                                _T("%sStoring Sybase configuration for instance %s on host %s failed!"),
                                ERROR_FAILURE,Name,opt.appHost);
                            retVal=RESULT_FAILURE;
                        }
                        else
                        {
                            DbgLogFull(__FLNPT__, logFile, 
                                _T("%sSybase Configuration for instance %s on host %s stored successfully."),
                                ERROR_MINOR,Name,opt.appHost);
                            if (retVal!=RESULT_SUCCESS)
                                DbgLogPlain(logFile, 
                                _T("%sSome errors occured during Sybase configuration upgrade. Please check on the CM if the configuration was moved as expected.")
                                ERROR_WARNING);
                        }

                    }

                    cbName = cbClass = STRLEN_PATH;     /* set enough space for next query */

                    {
                        tchar orig[STRLEN_PATH+1]={0}, copy[STRLEN_PATH+1]={0};

                        strcat(sybaseLibPath,_T("\\lib"));

                        sprintf(orig, _T("%s\\libob2syb.dll"),cmnPanBin);
                        sprintf(copy, _T("%s\\libob2syb.dll"),sybaseLibPath);

                        DbgLogPlain(logFile, _T("Copying %s to %s"), orig, copy);
                        if (!CopyFile(orig, copy, FALSE))
                        {
                            DbgStamp(DBG_UNEXPECTED);
                            DbgPlain(DBG_UNEXPECTED, _T("%sCopying libob2syb.dll from %s to %s failed! Error: %d. %s"),
                                ERROR_FAILURE,
                                cmnPanBin,
                                sybaseLibPath,
                                GetLastError(),
                                ErhSysErrnoToText(GetLastError())
                            );

                            DbgLogFull(__FLNPT__, logFile, 
                                _T("%sCopying libob2syb.dll from %s to %s failed! Error: %d. %s"),
                                ERROR_FAILURE,
                                cmnPanBin,
                                sybaseLibPath,
                                GetLastError(),
                                ErhSysErrnoToText(GetLastError())
                            );
  
                            retVal=RESULT_FAILURE;
                        }
                    }
                }                
            }
            
            RegCloseKey(hKey);
        }
    }
#else
    sprintf(path,_T("%s/sybase/SERVERLIST"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_FILE,F))
        {
            DbgPlain(DBG_OB2BAR, _T("count==%d, allocated==%d"), count, allocated);
            if (count==allocated)
            {
                DbgStamp(DBG_OB2BAR);
                sid = (tchar **)REALLOC(sid, 
                    (10+allocated)*sizeof(tchar *));
                DbgStamp(DBG_OB2BAR);
                memset(sid+allocated*sizeof(tchar *),0,10*sizeof(tchar *));
                allocated+=10;
            }
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=0;
            sid[count++]=StrNewCopy(str);
            DbgLogPlain(logFile, _T("Found configuration for Sybase instance %s"),str);
            DbgPlain(DBG_OB2BAR, _T("Found configuration for Sybase instance %s"),str);
        }
        DbgStamp(DBG_OB2BAR);
        fclose(F);
        RenameFile(path, tmppath, _T("SERVERLIST"));
    }
    else
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
            __FCN__, path, errno, ErhSysErrnoToText(errno));
        DbgLogPlain(logFile, _T("%sCan not open data_servers file at %s. Error: %d, %s"),
            ERROR_FAILURE, path, errno, ErhSysErrnoToText(errno));
        retVal=RESULT_FAILURE;
    }
    
    DbgStamp(DBG_OB2BAR);
    for (i=0;i<count;i++)
    {
        BP_InitAsList(&config,OB2_APP_SYBASE);

        sprintf(newpath,_T("%s/%s"),tmppath, sid[i]);
        if (OB2_CreateDirectory(newpath,0777))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Can not create directory %s"),
                __FCN__, newpath);
            DbgLogPlain(logFile, _T("Can not create directory %s. %s"),
                newpath, ErhSysErrnoToText(errno));
        }
        else
        {
            DbgLogPlain(logFile, 
                _T("Directory %s created."),newpath);
        }
        
        sprintf(path,_T("%s/sybase/%s/SYBASE"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config, SYBASE_HOME_DIR, str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, _T("%sCan not read oracle_home from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
                str[0] = 0;
            }
            fclose(F);
            sprintf(newpath, _T("%s/SYBASE"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open SYBASE file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sContinuing to another instance because the SYBASE file is missing for this instance."),
                ERROR_WARNING);
            if (retVal!=RESULT_FAILURE)
                retVal=RESULT_SOSO;
            continue;
        }     
        
        sprintf(path,_T("%s/sybase/%s/ISQL_PATH"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,SYBASE_ISQL_PATH,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read target connection string from file %s. %s"),
                    ERROR_WARNING, path, ErhSysErrnoToText(errno));
                if (retVal!=RESULT_FAILURE)
                    retVal=RESULT_SOSO;
            }
            fclose(F);
            sprintf(newpath, _T("%s/ISQL_PATH"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, _T("%sCan not open target connection file at %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }
        
        sprintf(path,_T("%s/sybase/%s/SA_USER"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,SYBASE_SA_USER,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/SA_USER"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }

        sprintf(path,_T("%s/sybase/%s/SA_PASSWORD"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,SYBASE_SA_PASSWORD,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/SA_PASSWORD"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }
        
        sprintf(path,_T("%s/sybase/%s/SYBASE_ASE"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,SYBASE_SYBASE_ASE,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/SYBASE_ASE"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }        
        
        sprintf(path,_T("%s/sybase/%s/SYBASE_OCS"),cmnPanConfigBase,sid[i]);
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            if (fgets(str,STRLEN_PATH,F))
            {
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=0;
                BP_AddStr(&config,SYBASE_SYBASE_OCS,str);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not read %s file."),
                    __FCN__, path);
                DbgLogPlain(logFile, 
                    _T("%sCan not read recovery catalog connection string from file %s. %s"),
                    ERROR_WARNING,path, ErhSysErrnoToText(errno));
            }
            fclose(F);
            sprintf(newpath, _T("%s/SYBASE_OCS"),sid[i]);
            RenameFile(path, tmppath, newpath);
        }
        else
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not open file %s. Error: %d, %s"),
                __FCN__, path, errno, ErhSysErrnoToText(errno));
            DbgLogPlain(logFile, 
                _T("%sCan not read recovery catalog connection string from file %s. %s"),
                ERROR_WARNING, path, ErhSysErrnoToText(errno)); 
        }        
        
        BP_AddList(&config,REG_ENV,&temp);

        sprintf(path,_T("%s/sybase/%s/.profile"),cmnPanConfigBase,sid[i]);
    
        if ((F=fopen(path,_T("r")))!=NULL)
        {
            tchar local_profile[STRLEN_PATH+1]={0};

            while (fgets(str,STRLEN_PATH,F))
            {
                tchar *c1,*c2;
            
                if (str[0] && str[strlen(str)-1]==_T('\n'))
                    str[strlen(str)-1]=_T('\0');
                c2=strchr(str,_T('='));
                if (c2 != NULL)
                {
                    c1=c2-1;
                    while (*c1==_T(' ') || *c1==_T('\t'))
                        c1--;
                    *(c1+1)=_T('\0');
                    c2++;
                    while (*c2==_T(' ') || *c2==_T('\t'))
                        c2--;
                    BP_RemoveOptByName(temp, str);
                    BP_AddStr(temp,str,c2);
                    DbgLogPlain(logFile, _T("Adding option %s=%s to local environment section"),
                        str,c2);
                }
                else
                {
                    DbgStamp(DBG_UNEXPECTED);
                    DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                    DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
                }
            }
            fclose(F);
            sprintf(local_profile,_T("%s/.profile"),sid[i]);
            RenameFile(path, tmppath, local_profile);
        }
        else
        {
            DbgStamp(DBG_OB2BAR);
            DbgPlain(DBG_OB2BAR,_T("Could not open %s"),path); 
        }
        
        if (PutConfig(sid[i],OB2_APP_SYBASE,opt.appHost,&config)!=RESULT_SUCCESS)
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("%sStoring Sybase configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sStoring Sybase configuration for instance %s on host %s failed!"),
                ERROR_FAILURE,sid[i],opt.appHost);
            retVal=RESULT_FAILURE;
        }
        else
        {
            DbgLogFull(__FLNPT__, logFile, 
                _T("%sSybase Configuration for instance %s on host %s stored successfully."),
                ERROR_MINOR,sid[i],opt.appHost);
            if (retVal!=RESULT_SUCCESS)
                DbgLogPlain(logFile, 
                    _T("%sSome errors occured during Sybase configuration upgrade. Please check on the CM if the configuration was moved as expected."),
                    ERROR_WARNING);
        }
        
        BP_Free(&config);

        {
            tchar szExecutable[STRLEN_PATH+1]={0};
            tchar szParams[STRLEN_PATH+1]={0};
            tchar **argv=NULL;
            int argc=0;
            int ret=0;

            strcpy(szExecutable,_T("cp"));
            sprintf(szParams,_T("%s/sybackup.sh.temp %s/sybackup_%s.sh"),cmnPanLBin, cmnPanLBin, sid[i]);
            StrToArg(szExecutable, szParams, &argc, &argv);

            DbgLogPlain(logFile, _T("Copying %s/sybackup.sh.temp to %s/sybackup_%s.sh"), cmnPanLBin, cmnPanLBin, sid[i]);

            if ((CmnRunScriptEx(_T("/bin"),argc,argv,NULL,0,0,&ret)!=RUN_SCRIPT_OK)&&(ret!=0))
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("%sCopying from sybackup.sh.temp to sybackup_%s.sh failed!"),
                    ERROR_FAILURE,
                    sid[i]
                );

                DbgLogFull(__FLNPT__, logFile,
                    _T("%sCopying from sybackup.sh.temp to sybackup_%s.sh failed!"),
                    ERROR_FAILURE,
                    sid[i]
                );

                retVal=RESULT_FAILURE;
            }

            FreeStrToArg(&argc, &argv);
        }
    }

#endif

    if (GetConfig(STR_GLOBAL, OB2_APP_SYBASE, &config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sLoading global Sybase configuration for host %s failed!"),
            ERROR_MINOR,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sCan not load global Sybase configuration for host %s. Creating a new one!"),
            ERROR_MINOR,opt.appHost);
        BP_InitAsList(&config,STR_GLOBAL);
        ErhClearError();
    }
    
    if (count)
    {
        BP_StrListOpt   instances;

        memset(&instances, 0, sizeof(BP_StrListOpt));
        if (BP_GetStrListByName(config, STR_INST_LIST, &instances) != BP_NOERROR)
        {
            BP_AddStrList(&config,STR_INST_LIST);
            BP_GetStrListByName(config, STR_INST_LIST, &instances);
        }
        
        for (i=0;i<count;i++)
        {
            int j, found=0;
            for (j=0;j<BP_StrListCount(&instances);j++)
            {
                if (!StrCmp(BP_StrListAt(&instances,j),sid[i]))
                {
                    found=1;
                    break;
                }
            }
            if (!found)
                BP_ChangeStrListAdd(&config,STR_INST_LIST,sid[i]);
        }
    }
    
    if ((temp=BP_GetOptByName(config, REG_ENV)) == NULL) 
        BP_AddList(&config,REG_ENV,&temp);
      
    sprintf(path,_T("%s/sybase/sybase.conf"),cmnPanConfigBase);
    
    if ((F=fopen(path,_T("r")))!=NULL)
    {
        while (fgets(str,STRLEN_PATH,F))
        {
            tchar *c1,*c2;
            
            if (str[0] && str[strlen(str)-1]==_T('\n'))
                str[strlen(str)-1]=_T('\0');
            c2=strchr(str,_T('='));
            if (c2 != NULL)
            {
                c1=c2-1;
                while (*c1==_T(' ') || *c1==_T('\t'))
                    c1--;
                *(c1+1)=_T('\0');
                c2++;
                while (*c2==_T(' ') || *c2==_T('\t'))
                    c2--;
                BP_RemoveOptByName(temp, str);
                BP_AddStr(temp,str,c2);
                DbgLogPlain(logFile, _T("Adding option %s=%s to global environment section"),
                    str,c2);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("%s Option %s is not supported."), ERROR_WARNING, str);
                DbgLogPlain(logFile, _T("%s Option %s is not supported."), ERROR_WARNING, str);
            }
        }
        fclose(F);
        RenameFile(path, tmppath, _T("sybase.conf"));
    }
    else
    {
        DbgLogPlain(logFile, _T("%sCan not open Sybase environment file %s. %s"),
            ERROR_WARNING, path, ErhSysErrnoToText(errno));
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sCan not open file %s"),ERROR_WARNING,path);
        if (retVal!=RESULT_FAILURE)
            retVal=RESULT_SOSO;
    }
    
    if (PutConfig(STR_GLOBAL,OB2_APP_SYBASE,opt.appHost,&config)!=RESULT_SUCCESS)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("%sStoring global Sybase configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sStoring global Sybase configuration on host %s failed!"),
            ERROR_FAILURE,opt.appHost);
        retVal=RESULT_FAILURE;
    }
    else
    {
        DbgLogFull(__FLNPT__, logFile, 
            _T("%sGlobal Sybase configuration on host %s stored successfully."),
            ERROR_MINOR,opt.appHost);
        if (retVal!=RESULT_SUCCESS)
            DbgLogPlain(logFile, 
            _T("%sSome errors occured during Sybase configuration upgrade. Please check on the CM if the configuration was moved as expected."),
            ERROR_WARNING);
    }
    
    if (sid)
        FREE(sid);
    BP_Free(&config);
    
    DbgFcnOut();
    return retVal;
}


int main(int argc, tchar *argv[])
{
    ERH_FUNCTION(_T("main"));

    int     result;
    int     nRetry=0;

    CmnSetProgname(AppProgname);

    OptInit();

    if (OptParseOptions(argc,argv)==-1) return(RESULT_FAILURE);

    OptParseDebug(GetEnv(_T("OB2OPTS")));

    /* initialize common library */
    if (CmnInit (
        AppProgname,
        _T(""),
        opt.debugSession?opt.debugSession:_T(""),
        opt.debugRanges ?opt.debugRanges :_T(""),
        opt.debugSelect ?opt.debugSelect :_T(""),
        0
    ) < 0)
    {       
        return(RESULT_FAILURE);
    }


    DbgStamp(DBG_OB2BAR);

    IpcInit();

    DbgStamp(DBG_OB2BAR);

    switch (opt.integration)
    {
    case OB2_BAR_SAP:
        sprintf(logFile,_T("upgrade_cfg_sap.log"));
            break;
    case OB2_BAR_MSSQL:
        sprintf(logFile,_T("upgrade_cfg_mssql70.log"));
            break;
    case OB2_BAR_ORACLE8:
        sprintf(logFile,_T("upgrade_cfg_oracle8.log"));
            break;
    case OB2_BAR_INFORMIX:
        sprintf(logFile,_T("upgrade_cfg_informix.log"));
            break;
    case OB2_BAR_SYBASE:
        sprintf(logFile,_T("upgrade_cfg_sybase.log"));
            break;
    default:
        printf(_T("%s"), NlsGetMessage(NLS_SET_LIBBAR, NLS_UPG_INTEG_NAME_UNSPECIFIED, ERROR_FAILURE));
        result=RESULT_FAILURE;
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Integration name is not specified"),__FCN__);
        goto exit;
    }

    if (opt.server != NULL)
    {
        CSHost = StrNewCopy(opt.server);
    }
    else if (CliReadCSHost (&(CSHost))<0)
    {
        result=RESULT_SOSO;
        DbgLogFull(__FLNPT__, logFile, _T("Cell Sever is not defined!"));
        goto exit;   
    }

    if (opt.appHost == NULL)
    {
        opt.appHost = StrNewCopy(cmnHostname);
    }

    csaEmbedded = 1; /* Ipc calls are handled separately */

    do
    {
        CsaInit (CSHost, &result);

        if (result)
        {
            if (opt.forceRetry)
            {
                DbgPlain(DBG_OB2BAR,_T("[%s] Retrying to connect to cell manager"),__FCN__);
                ErhClearError();
                sleep(5);
            }
            else
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED,_T("[%s] CsaInit failed"),__FCN__);
                result=RESULT_FAILURE;
                goto exit;
            }
        }

        nRetry++;
    } while (result&&(nRetry<60));

    if (result)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED,_T("[%s] CsaInit failed"),__FCN__);
        result=RESULT_FAILURE;
        goto exit;
    }

    DbgStamp(DBG_OB2BAR);
    
    switch (opt.integration)
    {
    case OB2_BAR_SAP:
        if (opt.optFile)
            result=UpgradeSAPRatio(CSHost);
        else
            result=UpgradeSAP(CSHost);
        break;
    case OB2_BAR_MSSQL:
        result=UpgradeSQL();
        break;
    case OB2_BAR_ORACLE8:
        result=UpgradeOracle();
        break;
    case OB2_BAR_INFORMIX:
        result=UpgradeInformix();
        break;
    case OB2_BAR_SYBASE:
        result=UpgradeSybase();
        break;
    }
exit:

    switch(result)
    {
    case RESULT_FAILURE:
        DbgLogFull(__FLNPT__, logFile, _T("Configuration update FAILED!"));
        printf(_T("%s"), NlsGetMessage(NLS_SET_LIBBAR, NLS_UPG_UPGRADE_FAILED, ERROR_FAILURE));
        break;
    case RESULT_SOSO:
        DbgLogFull(__FLNPT__, logFile, _T("Configuration update finished with errors!"));
        printf(_T("%s"), NlsGetMessage(NLS_SET_LIBBAR, NLS_UPG_UPGRADE_WITH_ERRORS,
                       ERROR_WARNING, logFile));
        break;
    case RESULT_SUCCESS:
        DbgLogFull(__FLNPT__, logFile, _T("Configuration update successful!"));
        printf(_T("%s"), NlsGetMessage(NLS_SET_LIBBAR, NLS_UPG_UPGRADE_SUCCESS, ERROR_MINOR));
        break;
    }
    FREE(CSHost);

    DbgStamp(DBG_OB2BAR);
    CsaExit();
    IpcExit();
    CmnExit();
    OptExit();  

    return(result);
}
