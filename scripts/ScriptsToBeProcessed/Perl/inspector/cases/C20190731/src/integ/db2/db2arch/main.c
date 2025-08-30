/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   OmniBack II - IBM DB2 BAR Integration, db2arch
* @file      integ/db2/db2arch/main.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     DB2 db2arch - userexit program
*
* @since     21.10.2002         antonk       Initial Coding
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/db2/db2arch/main.c $Rev$ $Date::                      $:") ;
#endif

#include "integ/db2/common.h"

#include "integ/bar2/libbar.h"
#include "integ/barutil/ob2util.h"
#include "lib/parse/gen_parser.h"
#include "cs/csa/csa.h"


#include "db2arch.h"

const tchar AppProgname[STRLEN_FILE+1] = _T("DB2ARCH");



#define OPT_OS      0
#define OPT_RL      1
#define OPT_RQ      2
#define OPT_DB      3
#define OPT_NN      4
#define OPT_LP      5
#define OPT_LN      6
#define OPT_VERSION 7

#define DB2_LEVEL_10 10

static CLTab optTab[] = {
{  OPT_OS,           _T("os"),            0                },
{  OPT_RL,           _T("rl"),            0                },
{  OPT_RQ,           _T("rq"),            0                },
{  OPT_DB,           _T("db"),            0                },
{  OPT_NN,           _T("nn"),            0                },
{  OPT_LP,           _T("lp"),            0                },
{  OPT_LN,           _T("ln"),            0                },
{  OPT_VERSION,      _T("ver[sion"),      0                }
};

const tchar logFile[] = _T("db2.log");

static int db2Level = 0;

/*=========================================================================*//**
*
* @ingroup   db2bar
*
* @param     str   string which is one line from the output of 
*                  db2level command
*
* @retval    0     always
*
* @brief     This function is a hook which will be called once
*            for each line in the output of the command db2level.
*            This function determines whether the DB2 instance is
*            of level 10 or not.
*
*//*=========================================================================*/
static int ErhDb2LevelHook(tchar *str)
{
    if (str && (str[0] != _T('\n')) && (db2Level != DB2_LEVEL_10) 
        && (StrStrNS(str, _T("DB2 v1"))))
    {
        db2Level = DB2_LEVEL_10;
    }
    return 0;
}

/*=========================================================================*//**
*
* @ingroup   db2bar
*
* @param     config   pointer to a structure which contains all 
*                     information from DB2 config files written util_db2.
*
* @retval    void
*
* @brief     This function executes db2level command and provides
*            the output to ErhDb2LevelHook.
*
*//*=========================================================================*/
static void setDb2Lvl(BP_Opt* config)
{
    ERH_FUNCTION (_T("setDb2Lvl"));
    
    tchar *installDir = NULL;
	tchar installDirFull[STRLEN_PATH + 1] = {0};
    int retVal = 0;
	tchar *argv[] = {_T("db2level")};
	int argc = 1;    

    DbgFcnIn();
    ErhAssert(__FCN__, NULL != config);

    /*Get install dir */
    if (BP_GetStrByName(*config, DB2_KEY_INSTALLDIR, &installDir) != BP_NOERROR)
    {
        DbgStamp(DBG_DB2_INFO);
        DbgPlain(DBG_DB2_INFO, _T("[%s] Can not get install dir. from the configuration"), __FCN__);
        goto exit_lvl;
    }
    
    PathConcat(installDirFull, installDir,  _T("bin"), STRLEN_PATH);

    ErhSetHooks (ErhDb2LevelHook, ErhDb2LevelHook);
    if (CmnRunScriptEx(installDirFull, argc, argv, NULL, 0, 0, &retVal))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Can not start db2level. Error: [%d] %s"), __FCN__, retVal, ErhErrnoToText());
    }
    ErhSetHooks (BAR2ReportHook, BAR2ConsoleHook);
    FREE(installDir);

exit_lvl:
    DbgFcnOut();
}

static void InitDebugging(const Variant *config)
{
    ERH_FUNCTION(_T("InitDebugging"));

    tchar *debugStr;

    OB2_DebugOpts debug = {0};
    
    DbgFcnIn();

    if (NULL != DbgGetFd())
        RETURNV;

    debugStr = VarGetPathText(config, _T("Environment/OB2OPTS"));
    
    OB2_DebugOptsFromStr(&debug, debugStr);
    OB2_DebugOptsToEnv(&debug);

    DbgInit(debug.postfix,debug.ranges,debug.select);

    RETURNV;
}


/*========================================================================*//**
*
* @ingroup   db2arch
*
*//*=========================================================================*/
static USEREXIT_PARMS_T *UE_ParseOpts(int argc, tchar *argv[])
{
    USEREXIT_PARMS_T *opt = CALLOC (1, sizeof(USEREXIT_PARMS_T));
    tchar    verString[STRLEN_VERSION+1];
    int        i;
    int        neg;
    int        rc;

    DB2_ENTER(_T("UE_ParseOpts"));

    for (i=0;i<argc; )
    {
        tchar key[3] = {0};
        tchar *arg = argv[i];

        if ( arg[0] != _T('-') )
        {
            i++;
            continue;
        }

        rc = CliKwLookup(arg+1, optTab, CL_TAB_LEN(optTab), &neg);
        if (OPT_VERSION == rc)
        {
            MakeVersionString (STRLEN_VERSION, verString);
            printf (_T("%s\n"), verString);
            CmnExitProcess (0);
        }

        strncpy(key, arg+1, 2);
        rc = CliKwLookup(key, optTab, CL_TAB_LEN(optTab), &neg);

        switch (rc)
        {
        case OPT_OS:
            opt->os = StrNewCopy(argv[i++]+3);
            DbgPlain (DBG_DB2_INIT, _T("OS = %s"), opt->os); 
            break;

        case OPT_RL:
            opt->db2rel = StrNewCopy(argv[i++]+3);
            DbgPlain (DBG_DB2_INIT, _T("RL = %s"), opt->db2rel); 
            break;

        case OPT_RQ:
            if (!strncmp(argv[i]+3, ACTION_ARCHIVE, strlen(ACTION_ARCHIVE)))
            {
                opt->request = ACT_ARCHIVE;
            } else if (!strncmp(argv[i]+3, ACTION_RETRIEVE, strlen(ACTION_RETRIEVE)))
            {
                opt->request = ACT_RETRIEVE;
            } else
            {
                opt->request = ACT_UNDEFINED;
            }
            i++;
            DbgPlain (DBG_DB2_INIT, _T("RQ = %d"), opt->request);
            break;

        case OPT_DB:
            opt->dbname = StrNewCopy(argv[i++]+3);
            DbgPlain (DBG_DB2_INIT, _T("DB = %s"), opt->dbname); 
            break;

        case OPT_NN:
            opt->nodenum = StrNewCopy(argv[i++]+3);
            DbgPlain (DBG_DB2_INIT, _T("NN = %s"), opt->nodenum); 
            break;

        case OPT_LP:
            opt->logPath = StrNullCopy(StrCleanPathname(argv[i++]+3, 0));     
            DbgPlain (DBG_DB2_INIT, _T("LP = %s"), opt->logPath); 
            break;

        case OPT_LN:
            opt->logName = StrNewCopy(argv[i++]+3);
            DbgPlain (DBG_DB2_INIT, _T("LN = %s"), opt->logName); 
            break;

        default:
            i++;
            break;
        }
    }

    RETURN_PTR(opt);
}


static BOOL configFileIsFound(tchar *vsName, tchar *instName)
{
    BOOL isFound=FALSE;
    tchar *csHost=NULL;
    BP_Opt              config = {0};
    DB2_ENTER(_T("configFileIsFound"));

    if(CliReadCSHost(&csHost)==-1)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Unable to get CS host"), __FCN__);
        goto exit;
    }
    if ( !(BU_GetConfig(csHost, 
                             vsName, 
                             OB2_APP_DB2, 
                             instName,
                             &config)) != BP_NOERROR
       )
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] configuration file fot virtual host %s and instance %s if found"), __FCN__, vsName, instName);
        isFound=TRUE;
    }

exit:
    DB2_RETURN(isFound);
}

static int GetVSArray(tchar ***vsA, int *vsC)
{
    tchar **vsArray=NULL;
    tchar vsNumber=0;
    int retVal=OBE_ERR;
    int csHandle=0;
    tchar *csHost=NULL;
    tchar *buffer=NULL;
    tchar *cellFormat=NULL;
    ParserList   *pl=NULL;
    tchar *physName=StrNewCopy(cmnHostname);

    DB2_ENTER(_T("GetVSArray"));
    *vsC=0;
    /*GetListOfVirtual servers for cmnHostName*/
    if(CliReadCSHost(&csHost)==-1)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Unable to get CS host"), __FCN__);
        goto exit;
    }

    if (!CsaIsInitialized())
    {
        /* QXCR1000403707: Cell server Host is initialized */
        if (MCsaInit(csHost))
        {
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Unable to init CSA"), __FCN__);
            goto exit;        
        }
    }

    if(-1==(csHandle=MCsaBindServer(csHost)))
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Unable to bind CS host"), __FCN__);
        goto exit;    
    }
      if (-1 == (MCsaGetSingleFile(csHandle, S_CELL, &buffer, READ_ONLY)))
    {
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Unable to read cell info file."), __FCN__);
        goto exit;
    }
    if (-1 == (MCsaGetCustomFile (csHandle, CMN_PAN_CONFIG, S_CELLFORMAT_PATH, &cellFormat, READ_ONLY)))
    {
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Unable to read cell format file."), __FCN__);
        goto exit;    
    }
    pl=PLParse(cellFormat, buffer);
    if(!pl)
    {
        DbgPlain(DBG_UNEXPECTED,_T("[%s] Unable to parse received buffer."), __FCN__);
        goto exit;        
    }
    for (pl->current=pl->start; pl->current!=pl->end; pl->current=pl->current->child)
    {
        PObj    *obj=pl->current;
        tchar *name = PLGetValue(obj, PLCIK_HOSTNAME);
        if(!StrCmp(name, physName))
        {
            tchar *vsName=PLGetValue(obj, PLCIK_VIRTUAL_HOST);
            if(!vsName) vsName=PLGetValue(obj, PLCIK_CLUS);
            if(vsName)
            {
                vsNumber++;
                vsArray=REALLOC(vsArray, sizeof(tchar)*vsNumber);
                vsArray[vsNumber-1]=StrNewCopy(vsName);
            }
        }
    }
    retVal=OBE_OK;
exit:
    FREE(physName);
    *vsC=vsNumber;
    *vsA=vsArray;
    DB2_RETURN(retVal);
}


static MALLOCED tchar * GetVirtualServerName(tchar *instName, OB2_CONTEXT ctx)
{
    tchar **vsArray=NULL;
    tchar *vsName=NULL;
    int vsCount=0;
    int i=0;
    DB2_ENTER(_T("GetVirtualServerName"));
    GetVSArray(&vsArray, &vsCount);
    for(i=0; i<vsCount;i++)
    {
        if(configFileIsFound(vsArray[i], instName))
        {
            vsName=StrNewCopy(vsArray[i]);
            break;
        }
    }
    RETURN_STR(vsName);
}



/*========================================================================*//**
*
* @ingroup   db2arch
*
*//*=========================================================================*/
int main(int argc, tchar *argv[])
{
    ERH_FUNCTION(_T("main"));

    int                 retVal = UE_RET_SUCCESS;

    USEREXIT_PARMS_T    *opt = NULL;
    tchar               *barHostname = NULL;
    tchar               *instance = NULL;
    tchar               *logFileName = NULL;
    BP_Opt              config = {0};

    OB2_CONTEXT         ctx = {0};

    CmnSetProgname(AppProgname);

    instance = StrNullCopy(GetEnv (DB2INSTENV));
    if (!instance)
        instance = StrNewCopy(_T("DB2"));

    opt = UE_ParseOpts(argc-1, argv+1);

    if (OB2_Init(OB2_APP_DB2, instance, AppProgname, NULL, &ctx)!=OBE_OK)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("OB2_Init failed for instance %s!"), instance);

        retVal = UE_RET_ERROR;
        goto exit;
    }

    barHostname = GetEnv(_T("OB2BARHOSTNAME")) ? GetEnv(_T("OB2BARHOSTNAME")) : GetVirtualServerName(instance, ctx);
        if (!StrIsEmpty(barHostname))
        {
        OB2_SetHostname(barHostname);
        }
    DbgPlain(DBG_DB2_FLOW, _T("DB2INSTANCE=%s"), instance);

    if (BP_NOERROR != BU_GetConfig(OB2_CellServer(), cmnHostname, OB2_APP_DB2, instance, &config))
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] BU_GetConfig failed with error %s"),
            __FCN__, ErhErrnoToText());
        retVal = UE_RET_ERROR;
        goto exit;
    }

    InitDebugging(&config);
    
    logFileName =  StrFormat(_T("%s/%s"), opt->logPath, opt->logName);

    switch (opt->request)
    {
    case ACT_ARCHIVE:
        retVal = UE_BackupFile(ctx, &config, instance, opt->dbname, logFileName);
        break;
    case ACT_RETRIEVE:
        setDb2Lvl(&config);
        if (db2Level == DB2_LEVEL_10)
        {
            /*Truncating the second LOGSTREAMxxxx appearing
              in the full name of the log file*/
            FREE (logFileName);
            logFileName = StrFormat(_T("%s/%s"), StrDirname(opt->logPath), opt->logName);
        }

        retVal = UE_RestoreFile(ctx, instance, opt->dbname, logFileName, NULL, NULL, NULL, NULL);

        break;
    default:
        DbgLogPlain(logFile, _T("Got incorrect request"));
        retVal = UE_RET_ERROR;
    }
    if (retVal == UE_RET_SUCCESS)
    {
        DbgPlain(DBG_DB2_FLOW, _T("Log %s for %s/%s %s successfully"),
                    logFileName,
                    instance, 
                    opt->dbname,
                    (opt->request == ACT_ARCHIVE) ? _T("archived") : _T("retrieved")
                   );    
        DbgLogPlain(logFile, _T("Log %s for %s/%s %s successfully"),
                    logFileName,
                    instance, 
                    opt->dbname,
                    (opt->request == ACT_ARCHIVE) ? _T("archived") : _T("retrieved")
                   );    
    }
    else
    {
        DbgPlain(DBG_DB2_FLOW, _T("Log %s for %s/%s %s with error %d"),
                    logFileName,
                    instance, 
                    opt->dbname,
                    (opt->request == ACT_ARCHIVE) ? _T("archived") : _T("retrieved"),
                    retVal
                   );
        DbgLogPlain(logFile, _T("Log %s for %s/%s %s with error %d"),
                    logFileName,
                    instance, 
                    opt->dbname,
                    (opt->request == ACT_ARCHIVE) ? _T("archived") : _T("retrieved"),
                    retVal
                   );
    }

exit:
    OB2_Exit(ctx);

    return(retVal);
}


