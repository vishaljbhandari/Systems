/**
*
* (c) Copyright 1993-2014 Hewlett-Packard Development Company, L.P.
*
* @ingroup   libob2db2
* @file      integ/db2/libob2db2/logArchive.c
*
* @par Last Change:
* $Rev$
* $Date$
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /src/integ/db2/libob2db2/logArchive.c $Rev$ $Date$") ;
#endif

#include "libob2db2.h"
#include "integ/db2/common.h"

/*========================================================================*//**
*
* @ingroup   libob2db2
*
* @param     [in] ctx       the pointer to Context
* @param     [in] hostname  host name 
* @param     [in] instance  Instance name
* @param     [in] dbname    Database name
* @param     [in] barlist   Barlist name
*
* @retval    Gets the ID of last backup file +1
*
*//*=========================================================================*/
static int Vendor_GetNextID(OB2_CONTEXT ctx, tchar *hostname, tchar *instance, tchar *dbname, tchar *barlist)
{
    tchar       *cStr = NULL;
    tchar       *msg[SESSIONID_MSG_LEN] = {0};
    int         count = 0;
    int         setNumber = 0;
    int         ret = 0;
    int         csaInitPerformed = 0;

    OB2_ObjectAndVersion *arrObjAndVer = NULL;

    ERH_FUNCTION(_T("Vendor_GetNextID"));

    if ((ret = OB2_StartQueries(ctx)) != OBE_OK)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] OB2_StartQueries failed with error %d, %s"),
                 __FCN__, ret, ErhErrnoToText());
        goto ret_last;
    }

    if (!csaInitialized)
    { /* Initialization must be performed if CSA is not initialized */
        tchar   *CSHost=NULL;

        CliReadCSHost (&(CSHost));

        csaEmbedded=1;

        CsaInit(CSHost, &ret);
        if (ret != 0)
        {
            CliErhReport (strEmpty);
            FREE (cStr);
            goto ret_queries;
        }
        else
        {
            csaInitPerformed=1;
        }
        FREE(CSHost);
    }

    if (MCsaGetSessionOverview (0, &cStr) == -1)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] MCsaGetSessionOverview failed. [%d] %s"), 
                 __FCN__, ErhErrno(), ErhErrnoToText());
        FREE(cStr);
        goto ret_GetNextID;
    }

    if (StrParseStart (cStr) == -1)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s] StrParseStart failed. [%d] %s"), 
                 __FCN__, ErhErrno(), ErhErrnoToText());
        FREE(cStr);
        goto ret_GetNextID;
    }

    while (StrParseMessage(SESSIONID_MSG_LEN, msg) == 0)
    {
        tchar barl[STRLEN_FILE+1];

        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Checking session %s"), __FCN__, msg[6]); /* Session ID */
        if (atoi(msg[1]) != BACKUP) /* type */
            continue;
 
        sprintf(barl, _T("%s %s"), OB2_APP_DB2, barlist); /* msg[5] is like 'DB2 <barlist>' */
        if (StrCmp(msg[5], barl)) /* barlist */
        {            
            DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Session barlist = '%s', current barlist = '%s'"), 
                     __FCN__, msg[5], barlist);
            continue;
        }        
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Found session %s, barlist %s"), __FCN__, msg[6], msg[5]);
        break;
    }
    
    if (strlen(msg[6]) == 0)
    {        
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Session is the new one, no session ID yet, ignoring..."), __FCN__);

        goto ret_GetNextID;
    }
    if ( (ret = OB2_QueryObjectsOfSession(ctx, CliStrToSessionID(msg[6]), hostname, OB2_APP_DB2, &count, &arrObjAndVer)) == OBE_OK)
    {
        int i;

        for (i=0; i<count; i++)
        {
            tchar *inst,
                  *db,
                  *type,
                  *timestamp,
                  *set;
            inst = strtok(arrObjAndVer[i].object.name, _T(":"));
            db = strtok(NULL, _T(":"));
            type = strtok(NULL, _T(":"));
            timestamp = strtok(NULL, _T(":"));
            set = strtok(NULL, _T(":"));

            DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Checking object %s:%s:%s:%s:%s"),
                     __FCN__, inst, db, type, timestamp, set);

            if ((StrCmp(instance,inst)==0) &&
                (StrCmp(dbname, db)==0) &&
                atoi(timestamp) >= setNumber)
            {
                setNumber = atoi(timestamp)+1;
                DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] Setting setNumber to %d"), 
                         __FCN__, 
                         setNumber);
            }
            OB2_ObjectVersionFree(&arrObjAndVer[i]);
        }
        FREE(arrObjAndVer);
    }
    else
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] OB2_QueryObjectsOfSession exited with error [%d] %s"),
                  __FCN__,
                  ret,
                  ErhErrnoToText());
    }

ret_GetNextID:
    FREE(cStr);

    if (csaInitPerformed)
        CsaExit();

ret_queries:    
    OB2_EndQueries(ctx);

ret_last:
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Returning %d as the next set/timestamp number"),
             __FCN__, setNumber);
    return setNumber;
}

/*========================================================================*//**
*
* @ingroup   libob2db2
*
* @param     fName   File or path to convert from native format to IDB
*
* @retval    "xx"   converted path/filename string
*
* @brief     Function returns a path string converted from IDB format to
*            local format, without modifying the original. Subsequent calls
*            destroy the previous contents.
*
*//*=========================================================================*/
tchar *DB2_fNameFormatIDB(const tchar * const fName)
{
    ERH_FUNCTION(_T("DB2_fNameFormatIDB"));

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] fName == %s"), __FCN__, fName);

    if(StrLen(fName) > STRLEN_ITEMNAME)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s]: ERROR: Parameter too long!!"),
            __FCN__ );
        goto exit_err;

    }

    if(fName == NULL)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[%s]: ERROR: Parameter is NULL!!"),
            __FCN__ );
        goto exit_err;
    }

    StrCopy(convertedFileName, fName, STRLEN_ITEMNAME);

    PathToSlashes(convertedFileName);

    goto exit;

exit_err:
    convertedFileName[0]=_T('\0');
exit:

    return convertedFileName;
}

/*========================================================================*//**
*
* @ingroup   libob2db2
*
* @param     [out] config  configuration options
* @param     [in]  dbname  Database name
*
* @retval    the name of barlist
*
*//*=========================================================================*/
static tchar *GetArchiveBarListName(BP_Opt *config, tchar *dbname)
{
    BP_Opt *barListOpt = NULL;
    tchar  *barlist = NULL;
    int     ret;
    ERH_FUNCTION(_T("GetArchiveBarListName"));

    if (!config || !dbname)
    {
        DbgPlain(DBG_UNEXPECTED, _T("Wrong input parameters config=%p, dbname =%s"),            
            config, 
            NS(dbname)
        );
    }

    ret = BP_GetListByName(*config, USEREXIT_CONF, &barListOpt);
    if (BP_NOERROR != ret)
    {
        DbgPlain(DBG_UNEXPECTED, _T("BP_GetListByName(config, %s, &barListOpt) failed with error [%d], %s"),
            USEREXIT_CONF, ret, ErhErrnoToText());

        goto ret_last;
    }

    ret = BP_GetStrByName(*barListOpt, dbname, &barlist);
    if (BP_NOERROR != ret)
    {
        DbgPlain(DBG_UNEXPECTED, _T("BP_GetStrByName(*barListOpt, %s, &barlist) failed with error [%d], %s"),
            dbname, ret, ErhErrnoToText());

        barlist = NULL;
        goto ret_last;
    }

    barlist = StrNewCopy(barlist); /* otherwise BP_Free() will destroy it! */

ret_last:
    return barlist;
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

    ERH_FUNCTION(_T("GetVSArray"));
    *vsC=0;
    /*GetListOfVirtual servers for cmnHostName*/
    if(CliReadCSHost(&csHost)==-1)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] Unable to get CS host"), __FCN__);
        goto exit;
    }

    if (!CsaIsInitialized())
    {
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
    return retVal;
}

static BOOL configFileIsFound(tchar *vsName, tchar *instName)
{
    BOOL isFound=FALSE;
    tchar *csHost=NULL;
    BP_Opt              config = {0};
    ERH_FUNCTION(_T("configFileIsFound"));

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
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] configuration file fot virtual host %s and instance %s if found"), __FCN__, vsName, instName);
        isFound=TRUE;
    }

exit:
    return isFound;
}

static MALLOCED tchar * GetVirtualServerName(tchar *instName, OB2_CONTEXT ctx)
{
    tchar **vsArray=NULL;
    tchar *vsName=NULL;
    int vsCount=0;
    int i=0;
    ERH_FUNCTION(_T("GetVirtualServerName"));
    GetVSArray(&vsArray, &vsCount);
    for(i=0; i<vsCount;i++)
    {
        if(configFileIsFound(vsArray[i], instName))
        {
            vsName=StrNewCopy(vsArray[i]);
            break;
        }
    }
    return vsName;
}

/*========================================================================*//**
*
* @ingroup   libob2db2
*
* @param     in     input information
* @param     out    output information
* @param     ret    result
*
* @retval    SQLUV_OK            success
* @retval    SQLUV_INIT_FAILED   fail
*
* @brief     initialize archive logs Backup
*
*//*=========================================================================*/
int InitArchiveLogs(
    DB2_INPUT  *in,
    DB2_OUTPUT *out)
{
    DPLIB_CONTEXT      *libctx  = out->pVendorCB;
    OB2_BACKUP_OPTIONS  options = {0};
    OB2_Item            ob2item = {0};  
    OB2_CONTEXT         ob2ctx = {0};

    int    status = OBE_OK;
    tchar  objectName[STRLEN_OBJECTNAME+1] = {0};
    tchar  tree[STRLEN_PATH+SQL_ALIAS_SZ+SQL_ALIAS_SZ+3] = {0};
    tchar  logName[STRLEN_FILE+1] = {0};
    tchar  logfullname[STRLEN_PATH+1] = {0};
    tchar  *temp = NULL;
    tchar  *barlist = NULL;
    tchar  *CSHost = NULL;
    tchar  *instance = NULL;
    tchar  *buffer = NULL;
    tchar  *dbtree;

    BP_Opt config = {0};

    ERH_FUNCTION(_T("InitArchiveLogs"));
    DbgFcnIn();

    instance = StrNullCopy(GetEnv (DB2INSTENV));
    if (!instance)
        instance = StrNewCopy(_T("DB2"));

    ob2ctx = OB2_GetContext();

    { /* For cluster needs we parse variable OB2BARHOSTNAME. In case it is set it should be used as
        a hostname where we run instead of cmnHostname */
        tchar *barHostname = GetEnv(_T("OB2BARHOSTNAME"));
        if (!StrIsEmpty(barHostname))
        {
            DbgPlain(DBG_MAIN_ACTION, _T("OB2BARHOSTNAME variable is set to %s, using it as the hostname"), barHostname);
            StrCopy(cmnHostname, barHostname, STRLEN_HOSTNAME);
        }
        else
        {
            barHostname=GetVirtualServerName(instance, ob2ctx);
            if(barHostname)
            {
                DbgPlain(DBG_MAIN_ACTION, _T("cmnHostname is set to %s"), barHostname);
                StrCopy(cmnHostname, barHostname, STRLEN_HOSTNAME);
            }
        }
    }
    DbgPlain(DBG_MAIN_ACTION, _T("DB2INSTANCE=%s"), instance);

    status = CliReadCSHost(&CSHost);
    if (0 != status)
    {
        DbgPlain(DBG_UNEXPECTED, _T("CliReadCSHost() failed with error [%d] %s"), 
            status, ErhErrnoToText());

        status = SQLUV_INIT_FAILED;
        goto ret_backup_file;
    }

    if ( (status = BU_GetConfig(CSHost, 
        cmnHostname, 
        OB2_APP_DB2, 
        instance,
        &config)) != BP_NOERROR
        )
    {
        DbgPlain(DBG_UNEXPECTED, _T("BU_GetConfig(%s, %s, DB2, %s, config) failed with error [%d] %s"),
            CSHost, cmnHostname, instance, status, ErhErrnoToText());

        status = SQLUV_INIT_FAILED;
        goto ret_backup_file;
    }

    if ((barlist = GetArchiveBarListName(&config, libctx->dbname)) == NULL)
    {
        DbgPlain(DBG_UNEXPECTED, _T("GetArchiveBarListName(%s, %s) failed!"), instance, libctx->dbname);
        DbgPlain(DBG_UNEXPECTED, _T("Barlist for archive log not specified for Instance %s, Database %s. Aborting archived log backup."), instance, libctx->dbname);     
      
        status = SQLUV_INIT_FAILED;
        goto ret_backup_file;
    }
    libctx->barlist = barlist;

    if ( (status = BU_GetBarlist(CSHost, 
        OB2_APP_DB2, 
        barlist,
        &buffer)) != BP_NOERROR
        )
    {
        DbgPlain(DBG_UNEXPECTED, _T("BU_GetBarlist(%s, DB2, %s, config) failed with error [%d] %s"),
            CSHost, barlist, status, ErhErrnoToText());

        status = SQLUV_INIT_FAILED;
        goto ret_backup_file;
    }
    FREE(buffer);

    status = OB2_StartBackup(NULL, libctx->barlist);
    if (OBE_OK != status)
    {
        status = SQLUV_COMM_ERROR;
        goto ret_backup_file;
    }

    /* db2instance:db_alias:0:UNIQUE_ID:0 */
    sprintf(objectName, ARCH_OBJ_NAME_FORMAT, 
        libctx->instance, 
        libctx->dbname, 
        Vendor_GetNextID(ob2ctx, cmnHostname, libctx->instance, libctx->dbname, libctx->barlist)
    );

    // get archived log name
    temp = StrNewCopyS2T(in->DB2_session->filename);
    StrCopy(logName, temp, STRLEN_PATH);
    FREE(temp);

    sprintf(logfullname, _T("/%s/%s/NODE%04d/%s"),
        libctx->instance, 
        libctx->dbname,
        libctx->nodenum,
        logName);

    sprintf(tree, _T("/%s/%s/%s%s%c"), 
        libctx->instance, 
        libctx->dbname, 
        ARCHIVELOG_STRING, 
        logfullname,
        1);

    options.externalOptions = dbtree = StrFormat(_T("DBtree=%s"), tree);
    DbgPlain(DBG_MAIN_ACTION, _T("Set External Options:%s"), options.externalOptions );

    status = OB2_StartObjectBackupEx(NULL, BT_FULL, OB2_Objectname(objectName, NULL), NULL, &options);
    FREE(dbtree);

    if (OBE_OK != status)
    {
        status = SQLUV_COMM_ERROR;
        goto ret_backup_file;
    }

    ob2item.type = OBJ_FILE;
    sprintf (ob2item.name, _T("%.*s"), STRLEN_ITEMNAME,
        DB2_fNameFormatIDB(tree)); /* convert */

    status = OB2_StartItemBackup(NULL, &ob2item, NULL, 0, 0L);

    if (status != OBE_OK)
    {
        status = SQLUV_COMM_ERROR;
        goto ret_backup_file;
    }     

ret_backup_file:    
    FREE(barlist);
    
    RETURN_INT(status);
}

