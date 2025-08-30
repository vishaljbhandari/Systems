/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   BARGUI Common functions
* @file      lib/bargui/barguicommon.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @since     17.10.97 sauron     Initial Coding
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/bargui/barguicommon.c $Rev$ $Date::                      $:");
#endif

#include <stdio.h>

#include "lib/cmn/common.h"
#include "cs/csa/csa.h"
#include "integ/barutil/ob2util.h"
#include "lib/cmn/containers.h"
#include "integ/barutil/xtra/query.h"
#include "lib/cmn/getopt.h"
#include "bargui.h"
#include "barguicommon.h"


static BarGuiPtdStruct barGuiPtdProto = {0};

CmnContextType BarGuiPtd = {
    _T("BarGuiPtd"),
    &barGuiPtdProto,
    sizeof(barGuiPtdProto)
};


#define CPY(_member)  tgt->_member = StrDup (src->_member)




/*========================================================================*//**
*
* @retval    nothing
*
* @brief     Copies the SpecStruct structure.
*
*//*=========================================================================*/
static void OptionsCopy (int apptype, SpecStruct *tgt, SpecStruct *src)
{
    IInteg *self = IQuery(apptype);

    CPY (preexec);
    CPY (postexec);

    if (self && self->options.copy)
        self->options.copy (tgt, src);

}


/*========================================================================*//**
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*//*=========================================================================*/
void IntegObjCopy (IntegObj *tgt, IntegObj *src)
{
    memcpy (tgt, src, sizeof(*tgt));
    CPY (name);
    CPY (objectname);
    CPY (description);
    CPY (appname);
    CPY (hostname);
    CPY (realname);
    CPY (childrenUri);

    CmnVecInit (&tgt->objectnames, 0, NULL);
    OptionsCopy (src->apptype, &tgt->options, &src->options);
}


/* ===========================================================================
|  Unused functions. 
 =========================================================================== */
#if BARGUI_DEAD_CODE


/*========================================================================*//**
*
* @retval    NULL    something is wrong (allocation) => ErhMark()
* @retval    Integ*  new integration structure
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Allocates an integration structure.
*
*//*=========================================================================*/
static Integ *IntegAlloc(int apptype)
{
    ERH_FUNCTION(_T("IntegAlloc"));
    Integ  *integ;
    IInteg *self = IQuery(apptype);

    BG_DBGENTRY;

    if (!self)
    {
        ErhMark(ERH_OBE_NOTPLATSUP, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    integ = (Integ*)MALLOC(sizeof(Integ));

    if (IntegInit(integ, apptype)!=OBE_OK)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    RETURN_PTR (integ);
}


/*========================================================================*//**
*
* @retval    NULL    something went wrong => ErhMark()
* @retval    Integ  *a source copy
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Copy constructor for Integ
*
*//*=========================================================================*/
static Integ *IntegCopy(Integ *src, Integ *tgt)
{
    ERH_FUNCTION(_T("IntegCopy"));
    IInteg *self = src? IQuery(src->apptype) : NULL;

    BG_DBGENTRY;

    if (!src)
    {
        ErhMark(ERH_OBE_INVARG, __FCN__, ERH_WARNING);
        RETURN_PTR(NULL);
    }

    if (!self)
    {
        ErhMark(ERH_OBE_NOTPLATSUP, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    if (!tgt)
        tgt = IntegAlloc(src->apptype);

    if (!tgt)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    /* copy all structure data to the target integration */
    memcpy(tgt, src, sizeof(Integ));

    /* now allocate all string-relevant members */
    CPY (appname);
    CPY (hostname);
    CPY (user);
    CPY (group);
    CPY (description);
    CPY (backupMethod);

    CopyOptions (&tgt->options, &src->options);

    RETURN_PTR (tgt);
}

/*========================================================================*//**
*
* @retval    NULL        somethin is wrong (allocation) => ErhMark()
* @retval    IntegObj*   new integration structure
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Allocates an integration object structure.
*
*//*=========================================================================*/
static IntegObj *IntegObjAlloc(int apptype)
{
    ERH_FUNCTION(_T("IntegObjAlloc"));
    IInteg   *self = IQuery (apptype);
    IntegObj *obj;

    if (!self)
    {
        ErhMark(ERH_OBE_NOTPLATSUP, __FCN__, ERH_WARNING);
        return NULL;
    }

    obj = (IntegObj*)MALLOC(sizeof(IntegObj));

    if (IntegObjInit(obj, apptype)!=OBE_OK)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        return NULL;
    }

    return obj;
}


/*========================================================================*//**
*
* @retval    NULL       something went wrong => ErhMark()
* @retval    IntegConfig*  a source copy
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Copies an integration object Config structure.
*
*//*=========================================================================*/
static IntegConfig *IntegConfigCopy(IntegConfig *src, IntegConfig *tgt)
{
    ERH_FUNCTION(_T("IntegConfigCopy"));
    IInteg *self = src? IQuery (src->apptype) : NULL;

    BG_DBGENTRY;

    if (!src)
    {
        ErhMark(ERH_OBE_INVARG, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }
    
    if (!self)
    {
        ErhMark(ERH_OBE_NOTPLATSUP, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    if (!tgt)
        tgt = IntegConfigAlloc(src->apptype);

    if (!tgt)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    /* copy all structure data to the target integration */
    memcpy(tgt, src, sizeof(IntegConfig));

    switch (src->apptype)
    {
    case OB2_BAR_MSSQL:
        CPY (MSSQLConfig.login);
        CPY (MSSQLConfig.password);
        break;

    case OB2_BAR_INFORMIX:
        CPY (INFORMIXConfig.informixDir);
        CPY (INFORMIXConfig.sqlhosts);
        CPY (INFORMIXConfig.onconfig);
        break;

    case OB2_BAR_SYBASE:
        CPY (SYBASEConfig.sybaseDir);
        CPY (SYBASEConfig.isql_path);
        CPY (SYBASEConfig.sybaseUser);
        CPY (SYBASEConfig.sybaseUserPassword);
        CPY (SYBASEConfig.SYBASE_ASE);
        CPY (SYBASEConfig.SYBASE_OCS);
        break;

    case OB2_BAR_MBX:
        CPY (MBXConfig.admin);
        CPY (MBXConfig.password);
        CPY (MBXConfig.domain);
        break;

    case OB2_BAR_DB2:
        CPY (DB2Config.user);
        CPY (DB2Config.password);
        break;
    }

    RETURN_PTR (tgt);
}

#endif /* BARGUI_DEAD_CODE */



/* ===========================================================================
|  Memory handling of BarGUI structures:
|  constructors, destructors, copy constructors
 =========================================================================== */


/*========================================================================*//**
*
* @retval    0   ok
|      -1   ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Constructor for Integ
*
*//*=========================================================================*/
int IntegInit(Integ *integ, int apptype)
{
    ERH_FUNCTION(_T("IntegInit"));
    IInteg *self = IQuery(apptype);

    BG_DBGENTRY;

    BG_CheckArgs (integ);
    BG_CheckArgs (self);

    memset (integ, 0, sizeof(Integ));
    integ->apptype = apptype;

    /* Set default values for global options */
    switch (apptype)
    {
    case OB2_BAR_ORACLE8: {
        ORACLE8GLOBAL *options = &integ->options.ORACLE8Global;
        options->backup.online_backup  = 1;
        options->restore.recoverUntil  = RU_NOW;
        options->instrest.recoverUntil = RU_NOW;
        options->restore.openDatabase  = 1;
        }
        break;
    
    case OB2_BAR_VSS: {
        VSSGLOBAL *options = &integ->options.VSSGlobal;
        options->restore.irOptSelected.arrayWait = 1;
        }
        break;
    
    case OB2_BAR_VMWARE: 
    case OB2_BAR_VEPA:{
        VMWAREGLOBALRestore *restore = &integ->options.VMWAREGlobal.restore;
        restore->vcb.map = array_new(ARR_TYPE_COPY, sizeof(VMWAREVCBMAP), NULL);
        }
        break;
    
    case OB2_BAR_MSSQL: {
        MSSQLLOCALRestore *restore = &integ->options.MSSQLLocal.restore;
        restore->fileMap = array_new(ARR_TYPE_CONTIG, sizeof(MSSQLFileMap), MSSQL_FileMapFree);
        }
        break;

    case OB2_BAR_MSSPS2007:
        {
            MSSPS2007GLOBALBackup *backup = &integ->options.MSSPS2007GLOBAL.backup;
            MSSPS2007GLOBALRestore *restore = &integ->options.MSSPS2007GLOBAL.restore;
            backup->concurrency = 1;
            backup->offline     = FALSE;
            restore->replace    = FALSE;
        }
        break;

    case OB2_BAR_IDB:
        {
            IDBGLOBALBACKUP *backup = &integ->options.IDBGlobal.backup;
            backup->checkIDB = 1;
            backup->deleteArchLogs = 1;
        }
        break;

    case OB2_BAR_MYSQL:
        {
            MySQLGLOBALBackup *backup = &integ->options.MySQLGlobal.backup;
            backup->paralelism = 1;
            backup->compression = 0;
            backup->compressLevel = 1;
            backup->purgeBinLog = 0;
        }
        break;

    case OB2_BAR_POSTGRESQL:
        {
            PostgreSQLGLOBALBackup *backup = &integ->options.PostgreSQLGlobal.backup;
            backup->purgeArchiveLog = 1;
        }
        break;
    }

    BG_RETURN(OBE_OK);
}


/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Destructor for Integ
*
*//*=========================================================================*/
int IntegFree(Integ *integ)
{
    ERH_FUNCTION(_T("IntegFree"));
    IInteg *self = integ? IQuery (integ->apptype) : NULL;

    BG_DBGENTRY;
    BG_CheckArgs (integ);
    BG_CheckArgs (self);

    /* free integration specific members */
    FREE(integ->appname);
    FREE(integ->hostname);
    FREE(integ->user);
    FREE(integ->group);
    FREE(integ->description);
    FREE(integ->backupMethod);

    /* now we have to clear the options member */
    if (self->options.dtor)
        self->options.dtor (&integ->options);

    if (integ->integConfig)
    {
        IntegConfigFree(integ->integConfig);
        FREE (integ->integConfig);
    }

    if ((integ->apptype == OB2_BAR_VEPA) && (0 != integ->options.VEPAGlobal.backup.mntEsxVms.count))
    {
         array_free(&integ->options.VEPAGlobal.backup.mntEsxVms);
    }

    /* just clear it up */
    memset(integ, 0, sizeof(Integ));


    BG_RETURN(OBE_OK);
}


void IntegObjDump (int dbglevel, IntegObj *obj)
{
    DbgPlain (dbglevel, _T("IntegObj(%p) [%d] %-40s| %s"), obj, obj->level, obj->name, obj->objectname);
}


/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Initializes an integration object structure.
*
*//*=========================================================================*/
int IntegObjInit(IntegObj *obj, int apptype)
{
    ERH_FUNCTION(_T("IntegObjInit"));
    IInteg *self = IQuery (apptype);

    BG_DBGENTRY;
    BG_CheckArgs (obj);
    BG_CheckArgs (self);

    memset (obj, 0, sizeof(IntegObj));
    obj->apptype   = apptype;
    obj->delimeter = _T('/');
    CmnVecInit (&obj->objectnames, 0, NULL);

    if (self->options.ctor)
    {
        self->options.ctor (&obj->options);
    }

    else switch (apptype)
    {
    case OB2_BAR_MSEXCHANGE:
        obj->options.MSEXCHANGELocal.restore.pubmdb      = 1;
        obj->options.MSEXCHANGELocal.restore.privmdb     = 1;
        obj->options.MSEXCHANGEGlobal.restore.deletelogs = 1;
        break;

    case OB2_BAR_SYBASE:
        obj->options.SYBASELocal.backup.paral = 1;
        break;

    case OB2_BAR_DB2:
        obj->options.DB2Global.backup.backupType    = DataBase;
        obj->options.DB2Global.backup.parallelism   = 1;
        obj->options.DB2Global.restore.rollforward  = ToEndOfLogs;
        obj->options.DB2Local.backup.isOnline       = Online;
        obj->options.DB2Local.restore.versions      = NULL;
        obj->options.DB2Local.restore.versionsCount = 0;
        break;
    }

    BG_RETURN(OBE_OK);
}



int IntegObjInitEx (IntegObj *obj, Integ *integ)
{
    int result = IntegObjInit(obj, integ->apptype);
    if (OBE_OK != result)
        return result;

    obj->appname  = StrNewCopy(integ->appname);
    obj->hostname = StrNewCopy(integ->hostname);
    return OBE_OK;
}





/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Frees an integration object structure.
*
*//*=========================================================================*/
int IntegObjFree(IntegObj *obj)
{
    ERH_FUNCTION(_T("IntegObjFree"));
    IInteg *self = obj? IQuery (obj->apptype) : NULL;
    int i;

    BG_DBGENTRY;

    BG_CheckArgs(obj);
    BG_CheckArgs(self);

    FREE(obj->realname);
    FREE(obj->objectname);
    FREE(obj->appname);
    FREE(obj->hostname);
    FREE(obj->name);
    FREE(obj->description);
    FREE(obj->childrenUri);
    CmnVecFree(&obj->objectnames);

    /* free additional objects to restore */
    for (i=0; i<obj->addObjCount; i++)
    {
        FREE(obj->addObjName[i]);
    }
    FREE(obj->addObjName);

    /* now we have to free the options member */
    if (self->options.dtor)
        self->options.dtor (&obj->options);

    /* just clear it up */
    memset(obj, 0, sizeof(IntegObj));

    BG_RETURN(OBE_OK);
}



int IntegObjItemInit(IN IntegObjItem *item)
{
    memset(item,0,sizeof(IntegObjItem));
    return(OBE_OK);
}


int IntegObjItemUnpack(IN IntegObjItem *item, IN tchar **msg)
{
    StrCopy(item->name,msg[FLD_FILE_NAME],STRLEN_ITEMNAME);
    item->flags = atol(msg[FLD_FILE_FLAGS]);
    item->type  = atol(msg[FLD_FILE_TYPE]);
    return OBE_OK;
}



/*========================================================================*//**
*
* @retval    NULL        somethin is wrong (allocation) => ErhMark()
* @retval    IntegVer*   new integration version structure
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Allocates an integration object version structure.
*
*//*=========================================================================*/
IntegVer *IntegVerAlloc(void)
{
    ERH_FUNCTION(_T("IntegVerAlloc"));
    IntegVer *ver;

    BG_DBGENTRY;

    ver = (IntegVer*)MALLOC(sizeof(IntegVer));

    if (IntegVerInit(ver) != OBE_OK)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    RETURN_PTR (ver);
}


/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Initializes an integration object version structure.
*
*//*=========================================================================*/
int IntegVerInit(IntegVer *ver)
{
    ERH_FUNCTION(_T("IntegVerInit"));

    BG_DBGENTRY;
    BG_CheckArgs (ver);

    memset(ver, 0, sizeof(IntegVer));

    CmnVecInit (&ver->id, 0, NULL);
    CmnVecInit (&ver->backupdevice, 0, NULL);

    BG_RETURN(OBE_OK);
}


/*========================================================================*//**
*
* @retval    NULL       something went wrong => ErhMark()
* @retval    IntegVer*  a source copy
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Copies an integration object version structure.
*
*//*=========================================================================*/
IntegVer *IntegVerCopy(IntegVer *src, IntegVer *tgt)
{
    ERH_FUNCTION(_T("IntegVerCopy"));
    int i;

    BG_DBGENTRY;
    if (!src)
    {
        ErhMark(ERH_OBE_INVARG, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    if (!tgt)
        tgt = IntegVerAlloc();
    
    if (!tgt)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    /* copy all structure data to the target integration */
    memcpy(tgt, src, sizeof(IntegVer));

    /* copy strings */
    tgt->ovID  = StrNewCopy(src->ovID);
    tgt->relatedversion  = StrNewCopy(src->relatedversion);
    tgt->ovopt = StrNewCopy(src->ovopt);
    tgt->realsessionname = StrNewCopy(src->realsessionname);

    CmnVecInit(&tgt->id, 0, NULL );
    for (i=0; i < CmnVecH(&src->id); i++)
    {
        CmnVecAddStr (&tgt->id, CmnVecI(&src->id, i));
    }

    CmnVecInit(&tgt->backupdevice, 0, NULL );
    for (i=0; i < CmnVecH(&src->backupdevice); i++)
    {
        CmnVecAddStr (&tgt->backupdevice, CmnVecI(&src->backupdevice, i) );
    }

    RETURN_PTR (tgt);
}


/*========================================================================*//**
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Frees an integration object version structure.
*
*//*=========================================================================*/
void IntegVerFree(IntegVer *ver)
{
    if (!ver)
        return;

    FREE(ver->ovID);
    FREE(ver->relatedversion);
    FREE(ver->ovopt);
    FREE(ver->realsessionname);

    CmnVecFree(&ver->id );
    CmnVecFree(&ver->backupdevice);

    memset(ver, 0, sizeof(IntegVer));

}


void IntegSessionInit(IntegSession *sess)
{
    memset(sess, 0, sizeof(IntegSession));
}


void IntegSessionUnpack(IntegSession *sess, tchar **msg)
{
    StrCopy (sess->sessionName,     msg[FLD_SES_SESSIONNAME], STRLEN_SESSIONNAME);
    StrCopy (sess->longSessionName, msg[FLD_SES_LONGSESNAME], STRLEN_LONGSESNAME);
    StrCopy (sess->description,     msg[FLD_SES_DESCRIPTION], STRLEN_DESCRIPTION);
    StrCopy (sess->owner,           msg[FLD_SES_SOWNER],      STRLEN_OB2USER);
    StrCopy (sess->datalist,        msg[FLD_SES_DATALIST],    STRLEN_PATH);
    sess->sessionType   = atol(msg[FLD_SES_SESSIONTYPE]);
    sess->startTime     = atol(msg[FLD_SES_STARTTIME]);
    sess->endTime       = atol(msg[FLD_SES_ENDTIME]);
    sess->sessionStatus = atol(msg[FLD_SES_STATUS]);
    sess->sessionFlags  = atol(msg[FLD_SES_FLAGS]);
    sess->nErrors       = atol(msg[FLD_SES_NROFERRORS]);
    sess->nWarnings     = atol(msg[FLD_SES_NROFWARNINGS]);
    sess->backupType    = atol(msg[FLD_OV_BACKUPTYPE]);
    sess->queueTime     = atol(msg[FLD_SES_QUETIME]);
    sess->opt           = NULL;
}


void IntegSessionFree(IntegSession *sess)
{
    FREE(sess->opt);
}


/*========================================================================*//**
*
* @retval    NULL        something is wrong (allocation) => ErhMark()
* @retval    IntegConfig*   new integration version structure
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Allocates an integration object version structure.
*
*//*=========================================================================*/
IntegConfig *IntegConfigAlloc(int apptype)
{
    ERH_FUNCTION(_T("IntegConfigAlloc"));
    IntegConfig *conf;

    BG_DBGENTRY;

    conf = CALLOC(1, sizeof(IntegConfig));

    if (IntegConfigInit(conf,apptype)!=OBE_OK)
    {
        ErhMark(ERH_OBE_INTERNAL, __FCN__, ERH_WARNING);
        RETURN_PTR (NULL);
    }

    RETURN_PTR (conf);
}


/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Initializes an integration object version structure.
*
*//*=========================================================================*/
int IntegConfigInit(IntegConfig *conf, int apptype)
{
    ERH_FUNCTION(_T("IntegConfigInit"));

    BG_DBGENTRY;
    BG_CheckArgs(conf);

    memset(conf, 0, sizeof(IntegConfig));

    conf->apptype = apptype;

    BG_RETURN(OBE_OK);
}


/*========================================================================*//**
*
* @retval    0   ok
* @retval    -1  ErhMark() error
*
* @brief     TYPE (1) common  (x) backup (.) restore (.)
*            (2) filling (.) config (.) browse  (.) structure (x)
*
*            Frees an integration object Config structure.
*
*//*=========================================================================*/
int IntegConfigFree(IntegConfig *conf)
{
    ERH_FUNCTION(_T("IntegConfigFree"));
    IInteg *self = conf? IQuery (conf->apptype) : NULL;

    BG_DBGENTRY;
    BG_CheckArgs (conf);
    BG_CheckArgs (self);

    if (self->config.dtor)
        self->config.dtor (conf);

    /* just clear it up */
    memset(conf, 0, sizeof(*conf));

    BG_RETURN(OBE_OK);
}

/* ----------------------------------------------------------------------
|  Fill (ptr***, ULONG*) output pair of BarGUI functions from array_t
---------------------------------------------------------------------- */
void array_collect_ptrs (IN OUT array_t *a, OUT void ***ptrs, OUT int *count)
{
    int  i;
    int oldCount = *count;
    *count += a->count;
    *ptrs = *count?
        REALLOC (*ptrs, *count * sizeof(void*)) :
        NULL;
    for (i=0; i < a->count; ++i)
        (*ptrs)[oldCount+i] = array_get(a, i);

    /* in case we store pointers, delete pointers */
    if (a->type&ARR_TYPE_FLAG_PTR)
        FREE (a->data);
    memset (a, 0, sizeof(*a));
}


void IntegsCopy (IN array_t integs, OUT Integ ***integration, OUT int *integrationCount)
{
    int i;
    array_collect (integs, integration, integrationCount);
    for (i=0; i < integs.count; i++)
    {
        DbgPlain(BG_DBG_TRACE, _T("\t|Integ[%d] = %s, %s"),
            i,
            NSD((*integration)[i]->hostname),
            NSD((*integration)[i]->appname));
    }
}

void IntegObjsCopy (IN array_t objs, OUT IntegObj ***ptrs, OUT int *count)
{
    int i;
    array_collect (objs, ptrs, count);
    for (i=0; i < *count; ++i)
    {
        DbgPlain(BG_DBG_TRACE, _T("\tIntegObj[%d] = %s, %s"),
            i,
            NSD((*ptrs)[i]->name),
            NSD((*ptrs)[i]->objectname));
    }
}


IInteg *DMA_GetParams () {
    static IInteg i = {0};
    i.type = OB2_BAR_DMA;
    i.name = _T("DMA");
    return &i;
}


/*========================================================================*//**
*
* @brief     Get integration parameters from its type or name.
*            Each integration exposes function that returns integration interface
*            (IInteg) structure.
*            We are abusing the fact the integ ids (OB2_BAR*) are 0,1, ...
*            Order in _IntegTable_ MUST be appropriate!
*
*//*=========================================================================*/
IInteg *Enabler_GetParams(void);
IInteg *Lotus_GetParams(void);
IInteg *Sybase_GetParams(void);
IInteg *Informix_GetParams(void);
IInteg *DB2_GetParams(void);
IInteg *Oracle8_GetParams(void);
IInteg *SAP_GetParams(void);
IInteg *SAPDB_GetParams(void);
IInteg *MBX_GetParams(void);
IInteg *MSSQL_GetParams(void);
IInteg *MSESE_GetParams(void);
IInteg *MSExchange_GetParams(void);
IInteg *VSS_GetParams(void);
IInteg *MSSPS_GetParams(void);
IInteg *VMWare_GetParams(void);
IInteg *VEPA_GetParams(void);
IInteg *E2010_GetParams(void); /* parasoft-suppress  COMMENT-04 "comment for all such functions above" */
IInteg *MSSPS2007_GetParams(void); /* parasoft-suppress  COMMENT-04 "Commented in mssps2007.c" */
IInteg *IDB_GetParams(void); /* parasoft-suppress  COMMENT-04 "Commented in mssps2007.c" */
IInteg *SAPHANA_GetParams(void);
IInteg *MYSQL_GetParams(void);
IInteg *POSTGRESQL_GetParams(void);

typedef IInteg* (*iinteg_f)();

static iinteg_f _IntegTable_[] =
{
    //NULL,                // OB2_FILESYSTEM
    SAP_GetParams,       // OB2_BAR_SAP
    NULL,                // OB2_BAR_ORACLE: only oracle 8 and higher supported ()
    Oracle8_GetParams,   // OB2_BAR_ORACLE8
    MSSQL_GetParams,     // OB2_BAR_MSSQL
    MSExchange_GetParams,// OB2_BAR_MSEXCHANGE
    Enabler_GetParams,   // OB2_BAR_ENABLER
    Informix_GetParams,  // OB2_BAR_INFORMIX
    Sybase_GetParams,    // OB2_BAR_SYBASE
    MSESE_GetParams,     // OB2_BAR_MSESE
    Lotus_GetParams,     // OB2_BAR_LOTUS
    DB2_GetParams,       // OB2_BAR_DB2
    SAPDB_GetParams,     // OB2_BAR_SAPDB
    NULL,                // OB2_BAR_ORACLE8PC: just a hack for agent/SM
    VSS_GetParams,       // OB2_BAR_VSS
    MBX_GetParams,       // OB2_BAR_MBX
    MSSPS_GetParams,     // OB2_BAR_MSSPS
    DMA_GetParams,       // OB2_BAR_DMA: just a hack (for now) to detect hosts with DMA installed
    VMWare_GetParams,    // OB2_BAR_VMWARE
    E2010_GetParams,     // OB2_APP_E2010
    MSSPS2007_GetParams, // OB2_APP_MSSPS2007
    VEPA_GetParams,      // OB2_BAR_VEPA
    IDB_GetParams,       // OB2_BAR_IDB
    SAPHANA_GetParams,   // OB2_BAR_SAPHANA
    MYSQL_GetParams,     // OB2_BAR_MYSQL
    POSTGRESQL_GetParams // OB2_BAR_POSTGRESQL
};


IInteg *IQuery (unsigned type)
{
    iinteg_f f = type>=ARRAYSIZE(_IntegTable_)? NULL : _IntegTable_[type];
    return f? f() : NULL;
}


IInteg *IQueryByName (tchar *integName)
{
    int i,j;
    for (i=0; i<ARRAYSIZE(_IntegTable_); ++i)
    {
        iinteg_f func = _IntegTable_[i];
        IInteg *integ = func? func() : NULL;
        if (!integ)
            continue;
        if (0==StrICmp(integName, integ->name))
            return integ;
        for (j=0; j<ARRAYSIZE(integ->alias); ++j)
        {
            if (0==StrICmp(integName, integ->alias[j]))
                return integ;
        }
    }
    return NULL;
}




/* ===========================================================================
|   DBSM queries
=========================================================================== */
BOOL IntegUnpackObjectName (IN const tchar *objectName, OUT IntegObjectName *on)
{
    return StrFromObjectName (
        &on->type,
        on->hostName,
        on->mountPoint,
        on->label,
        objectName ) == 0;
}


BOOL IntegVerUnpackEx (OUT IntegVer *ver, IN tchar **msg, IN size_t recLen)
{
    CmnVecAddStr(&ver->id, msg[FLD_OV_ID]);
    CmnVecAddStr(&ver->backupdevice, msg[FLD_OV_BACKUPDEVICE]);

    ver->relatedversion = StrNewCopy(msg[FLD_OV_RELATEDVERSION]);
    ver->starttime      = atol (msg[FLD_OV_STARTTIME]);
    ver->endtime        = atol (msg[FLD_OV_ENDTIME]);
    ver->reltime        = atol (msg[FLD_OV_RELTIME]);
    ver->objectsize     = atol (msg[FLD_OV_OBJECTSIZE]);
    ver->objectsizehi   = atol (msg[FLD_OV_OBJECTSIZEHI]);
    ver->status         = atol (msg[FLD_OV_STATUS]);
    ver->flags          = atol (msg[FLD_OV_FLAGS]);
    ver->backuptype     = atol (msg[FLD_OV_BACKUPTYPE]);
    ver->prottype       = atol (msg[FLD_OV_PROTTYPE]);
    ver->prottime       = atol (msg[FLD_OV_PROTTIME]);
    ver->catprottype    = atol (msg[FLD_OV_CATPROTTYPE]);
    ver->catprottime    = atol (msg[FLD_OV_CATPROTTIME]);
    ver->accstype       = atol (msg[FLD_OV_ACCSTYPE]);
    ver->accsvalue      = atol (msg[FLD_OV_ACCSVALUE]);
    ver->nroffiles      = atol (msg[FLD_OV_NROFFILES]);
    ver->nrofwarnings   = atol (msg[FLD_OV_NROFWARNINGS]);
    ver->nroferrors     = atol (msg[FLD_OV_NROFERRORS]);
    ver->diskagentid    = atol (msg[FLD_OV_DISKAGENTID]);
    ver->vertype        = atol (msg[FLD_OV_VERTYPE]);
    ver->mediaclass     = atol (msg[FLD_OV_MEDIACLASS]);
    ver->ovopt          = StrNewCopy (msg[FLD_OV_OPT]);
    ver->ovID           = StrNewCopy (msg[FLD_OV_ID]);
    {
        const tchar *backupId = GetBackupId (ver->ovopt);
        const tchar *sessionId = recLen>NR_OV ? msg[NR_OV] : NULL;
        ver->realsessionname = StrNullCopy(sessionId);
        StrCopy(ver->sessionname, backupId ? backupId : NS(sessionId), STRLEN_OBJECTNAME);
    }
    return TRUE;
}


BOOL FilterObjectOfType (IN tchar *objectName, IN void *lParam)
{
    tchar  *wantType = (tchar*)lParam;
    tchar   host[STRLEN_HOSTNAME+1];
    tchar   mountPoint[STRLEN_PATH+1];
    tchar   appType[STRLEN_DESCRIPTION+1];
    int     type;

    if (StrFromObjectName (&type, host, mountPoint, appType, objectName) == -1)
        return FALSE;

    return NULL==wantType || 0==StrCmp(wantType, appType);
}


BOOL FilterObjectOfTypeAndMountpoint (IN tchar *objectName, IN void *lParam)
{
    TypeAndMountpoint *typeAndMountpoint = (TypeAndMountpoint *)lParam;
    tchar  *wantType = (typeAndMountpoint!=NULL) ? typeAndMountpoint->type : NULL;
    tchar  *wantMountpoint = (typeAndMountpoint!=NULL) ? typeAndMountpoint->mountpoint : NULL;
    tchar   host[STRLEN_HOSTNAME+1];
    tchar   mountPoint[STRLEN_PATH+1];
    tchar   appType[STRLEN_DESCRIPTION+1];
    int     type;

    if (StrFromObjectName (&type, host, mountPoint, appType, objectName) == -1)
        return FALSE;

    return (NULL==wantType && NULL==wantMountpoint) ||
           (0==StrCmp(wantType, appType) && 0==StrCmp(mountPoint,wantMountpoint));
}


BOOL FilterObjectOfTypeAndNotMountpoint (IN tchar *objectName, IN void *lParam)
{
    TypeAndMountpoint *param = (TypeAndMountpoint*)lParam;
    IntegObjectName    on    = {0};
    return
        !param ? TRUE :
        !IntegUnpackObjectName(objectName, &on) ? FALSE :
        0==StrCmp(param->type,on.label) && 0!=StrCmp(param->mountpoint, on.mountPoint);
}


BOOL FilterVersionCompleted (IN IntegVer *ver, IN void *lParam)
{
    return BackupImageComplete(ver->status, ver->flags);
}


/*========================================================================*//**
*
* @param     filter  - Callback to filter versions. The caller usually needs
*                      only "completed without errors" versions
* @param     userPtr - User supplied parameter to be passed to the filtering fn
*
* @retval    Return value of query API.
*
* @brief     Queries versions of given objects and creates a list of IntegVer
*            structures with unique sessionIDs.
*
*//*=========================================================================*/
int QueryVersionsOfObjects (
    IN  int      dbsmHandle,
    IN  BOOL   (*filter)(IntegVer*, void*),
    IN  void    *userptr,
    IN  int      flags,
    IN  array_t  objects,
    OUT array_t *versions )
{
    ERH_FUNCTION (_T("QueryVersionsOfObjects"));
    USES_BARGUI_PTD

    int    i;
    int    retval = 0;
    hash_t hash = hash_new(hashfn_tstring, hashcmp_tstring, 0);

    DbgFcnIn();

    for (i=0; i<objects.count; ++i)
    {
        query_t query = {0};
        tchar *objectName = array_get(&objects, i);
        BOOL onlyValid = (flags&QUERY_FLAG_TAPELESS) ? FALSE : TRUE;

        DbgPlain(BG_DBG_TRACE, _T("[%s] Versions of objectname: %s"), __FCN__, objectName);

        query_open (&query, dbsmHandle, FUN_LISTVEROFOBJECT,
            _T("%s%d%d%d"), objectName, BACKUP, onlyValid, ThisBarGui->queryObjectType);

        retval = query.status;

        if (0 != retval)
            goto out_of_here;

        /* for each object version */
        while (query_read(&query))
        {
            int      j, pos;
            IntegVer ver, *old;
            //long     id;
            tchar   *id;
            tchar   *backupdevice;

            IntegVerInit(&ver);
            IntegVerUnpack (&ver, query.record);

            DbgPlain (DBG_DETAIL_PROGTRACE, _T("[%s] Version: %s"), __FCN__, ver.sessionname);

            if (filter && !filter(&ver, userptr))
                goto next;

            pos = (int)hash_get(&hash, ver.sessionname);

            // new session
            if (!pos)
            {
                array_push (versions, &ver);
                hash_set (&hash, StrNewCopy(ver.sessionname), (void*)versions->count);
                continue;
            }

            old = array_get(versions, pos-1);
            backupdevice = CmnVecI(&ver.backupdevice, 0);
            id = CmnVecI(&ver.id, 0);

            // update devices list
            for (j=0; j<CmnVecH(&old->backupdevice); ++j)
            {
                if (0 == StrCmp(id, CmnVecI(&old->id, j)))
                    break;
                if (0 == StrCmp(backupdevice, CmnVecI(&old->backupdevice, j)) )
                    break;
            }

            if (j>=CmnVecH(&old->backupdevice)) /* device not found, add */
            {
                CmnVecAddStr(&old->id, id);
                CmnVecAddStr(&old->backupdevice, backupdevice);
            }

            old->flags &= ~OVF_COPY | ver.flags;

            next:
            IntegVerFree (&ver);
        } /* for each object version */

        retval = query.status;
        query_close (&query);

        if (!(flags&QUERY_FLAG_DONT_BREAK))
        {
            if (0!=retval) break;
        }
    } /* for each object name */

out_of_here:
    {
        hash_iterator_t it;
        hash_foreach (&hash, it)
            free (hash_iter_key(it));
        hash_free (&hash);
    }

    RETURN_INT(retval);
}


/*========================================================================*//**
*
* @param     filter  - Callback to filter versions. The caller usually needs
*                      only "completed without errors" versions
* @param     userPtr - User supplied parameter to be passed to the filtering fn
*
* @retval    Return value of query API.
*
* @brief     Queries versions of given objects and creates a list of IntegVer
*            structures with unique sessionIDs. Used only for browsing.
*
*//*=========================================================================*/
int QueryVersionsOfObjectsFB(
    IN int        dbsmHandle,
    IN BOOL       (*filter)(IntegVer*, void*),
    IN void       *userptr,
    IN int        flags,
    IN array_t    objects,
    OUT array_t   *versions,
    OUT BrowseOpt *browseOpt)
{
    ERH_FUNCTION (_T("QueryVersionsOfObjectsFB"));
    int i;
    int retval = 0;

    DbgFcnIn();

    for (i=0; i<objects.count; ++i)
    {
        query_t query = {0};
        DbgPlain(BG_DBG_TRACE, _T("[%s] Versions of integObj->objectname = %s"),
            __FCN__, (tchar*)array_get(&objects, i));

        query_open (&query, dbsmHandle, FUN_FBLISTALLOVER,
            _T("%s%d"), (tchar*)array_get(&objects,i), DBA_FB_FOCUSONFIRSTRESTORECHAIN);

        retval = query.status;

        if (0 != retval)
            RETURN_INT(retval);

        /* for each object version */
        while (query_read(&query))
        {
            IntegVer ver={0};

            IntegVerInit(&ver);
            IntegVerUnpack (&ver, query.record+NR_OBJ);

            if (filter && !filter(&ver, userptr))
                continue;

            if (versions)
                array_push(versions,&ver);
        } /* for each object version */

        if (!query_empty(&query))
        {
            if (browseOpt)
            {
                browseOpt->nrIncomplete=atol(query.record[0]);
                browseOpt->nrMissing=atol(query.record[1]);
                browseOpt->browseMode=atol(query.record[2]);
                browseOpt->level=atol(query.record[3]);
            }
        }

        retval = query.status;
        query_close (&query);

        if (!(flags&QUERY_FLAG_DONT_BREAK))
        {
            if (0!=retval) break;
        }
    } /* for each object name */

    RETURN_INT(retval);
}


/*========================================================================*//**
*
* @retval    Return value of query API.
*
* @brief     Queries restore chain of given object and creates a list of IntegVer
*            structures with unique sessionIDs.
*
*//*=========================================================================*/
int QueryRestoreChain (
    IN int        dbsmHandle,
    IN BOOL       (*filter)(IntegVer*, void*),
    IN void       *userptr,
    IN tchar      *ovID,
    IN tchar      *objectName,
    IN tchar      *sessionId,
    IN tchar      *dir,
    OUT array_t   *versions,
    OUT BrowseOpt *browseOpt)
{
    ERH_FUNCTION (_T("QueryRestoreChain"));
    int    retval = OBE_OK;

    query_t query = {0};

    DbgFcnIn();

    query_open (&query, dbsmHandle, FUN_FBLISTFULLRESCHAIN,
        _T("%s%s"), ovID, dir);

    retval = query.status;

    if (0 != retval)
        RETURN_INT(retval);

        /* for each object version */
    while (query_read(&query))
    {
        IntegVer ver={0};

        IntegVerInit(&ver);
        IntegVerUnpack (&ver, query.record);

        if (filter && !filter(&ver, userptr))
            continue;

        if (versions)
            array_push(versions,&ver);
    }

    if (!query_empty(&query))
    {
        if (browseOpt)
        {
            browseOpt->nrIncomplete=atol(query.record[0]);
            browseOpt->nrMissing=atol(query.record[1]);
            browseOpt->browseMode=atol(query.record[2]);
            browseOpt->level=atol(query.record[3]);
        }
    }

    retval = query.status;
    query_close (&query);

    RETURN_INT(retval);
}


/*========================================================================*//**
*
* @retval    Return value of query API.
*
* @brief     Returns a list of items (files) for a certain directory.
*            QueryVersionsOfObjectsFB needs to be
*            called before this function.
*
*//*=========================================================================*/
int QueryListDir (
    IN int      dbsmHandle,
    IN BOOL     (*filter)(tchar*, void*),
    IN void     *userPtr,
    IN tchar    *dir,
    IN int       level,
    IN int       flags,
    OUT array_t *items )
{
    ERH_FUNCTION(_T("QueryItems"));

    int retval, i, start, end;
    query_t query;

    DbgFcnIn();

    start = array_length(items);
    
    query_open (&query, dbsmHandle, FUN_FBLISTDIR,
        _T("%d%s"),
        level,
        dir
    );

    if (0 != query.status)
        RETURN_INT (ERH_OBE_DBSMCONNECT);

    while (query_read(&query))
    {
        IntegObjItem item={0};

        IntegObjItemInit(&item);
        IntegObjItemUnpack(&item,query.record);

        if (filter && !filter(item.name, userPtr))
            continue;

        if (items)
            array_push (items, &item);

    }
    end = array_length(items);

    retval = query.status;
    query_close (&query);
    
    /* recursive */
    if (flags & 0x1)
    {
        tchar path[STRLEN_PATH+1];
        for (i=start; i<end; ++i)
        {
            IntegObjItem *item = array_get(items,i);
            
            sprintf (path, _T("%s/%s"), dir, item->name);
            StrCopy (item->name, path, STRLEN_ITEMNAME);
            
            if (item->type == OBJ_DIR)
                QueryListDir (dbsmHandle, filter, userPtr, path, level, flags, items);
        }
    }

    RETURN_INT (retval);
}


int QueryItems (
    IN int      dbsmHandle,
    IN BOOL     (*filter)(tchar*, void*),
    IN void     *userPtr,
    IN tchar    *dir,
    IN int       level,
    OUT array_t *items )
{
    *items = array_new(ARR_TYPE_COPY,sizeof(IntegObjItem),NULL);
    return QueryListDir (dbsmHandle, filter, userPtr, dir, level, 0, items);
}


/*========================================================================*//**
*
* @retval    Return value of query API.
*
* @brief     Returns a list of versions (files) for a certain item.
*            QueryVersionsOfObjectsFB needs to be
*            called before this function.
*
*//*=========================================================================*/
int QueryItemVersions(
    IN int      dbsmHandle,
    IN BOOL     (*filter)(IntegVer*, void*),
    IN void     *userptr,
    IN tchar    *item,
    IN int      level,
    OUT array_t *versions )
{
    ERH_FUNCTION(_T("QueryItemVersions"));

    int retval;
    query_t query;

    DbgFcnIn();

    query_open (&query, dbsmHandle, FUN_FBLISTFILEVERS,
        _T("%d%s"),level,item);

    if (0 != query.status)
        RETURN_INT (ERH_OBE_DBSMCONNECT);

    while (query_read(&query))
    {
        IntegVer ver={0};

        IntegVerInit(&ver);
        IntegVerUnpack (&ver, query.record+NR_FILE);

        if (filter && !filter(&ver, userptr))
            continue;

        if (versions)
            array_push(versions,&ver);
    }

    retval = query.status;
    query_close (&query);
    RETURN_INT (retval);
}

/*========================================================================*//**
*
* @param     filter  - Callback to filter object names. The caller usually needs
*                      only objects of his integration.
* @param     userPtr - User supplied parameter to be passed to the filtering fn
*
* @retval    Array of full object names in <objects>.
*                 Return value of query API.
*
* @brief     Queries all OT_OB2BAR objects and adds those that match filtering
*            to a list.
*
*//*=========================================================================*/
int QueryObjects (
    IN   int      dbsmHandle,
    IN   BOOL   (*filter)(tchar*, void*),
    IN   void    *userPtr,
    OUT  array_t *objects)
{
    ERH_FUNCTION (_T("QueryObjects"));
    USES_BARGUI_PTD

    int     retVal = 0;
    query_t query = {0};

    DbgFcnIn();

    query_open (&query, dbsmHandle, FUN_LISTOBJECTS, _T("%d%d"), OT_OB2BAR, ThisBarGui->queryObjectType);
    if (0!=query.status)
        RETURN_INT (ERH_OBE_DBSMCONNECT);

    while (query_read(&query))
    {
        tchar *name = query.record[FLD_OBJ_OBJECTNAME];
        if (filter && !filter(name, userPtr))
        {
            continue;
        }
        array_push (objects, name);
        DbgPlain (DBG_MAIN_ACTION, _T("[%s] Added object `%s'"), __FCN__, name);
    }
    retVal = query.status;
    query_close (&query);
    RETURN_INT (retVal);
}



/*========================================================================*//**
*
* @retval    Array of full object names in <objects>
* @retval    Return value of query API as return value.
*
* @brief     Queries OT_OB2BAR objects matching partname.
*
*//*=========================================================================*/
int QueryObjectsPartName (
    IN  int      dbsmHandle,
    IN  BOOL   (*filter)(tchar*, void*),
    IN  void    *userPtr,
    IN  tchar   *partname,
    OUT array_t *objects )
{
    ERH_FUNCTION(_T("QueryObjectsPartName"));
    USES_BARGUI_PTD

    int     retval;
    query_t query;

    DbgFcnIn();

    query_open (&query, dbsmHandle, FUN_LISTPARTOFOBJECT,
        _T("%d%s%d"),
        OT_OB2BAR,
        partname,
        ThisBarGui->queryObjectType);

    if (0 != query.status)
        RETURN_INT (ERH_OBE_DBSMCONNECT);

    while (query_read(&query))
    {
        tchar *name = query.record[FLD_OBJ_OBJECTNAME];
        if (filter && !filter(name, userPtr))
            continue;
        DbgPlain (DBG_DETAIL_PROGTRACE, _T("[%s] Added objectname: %s"), __FCN__, name);
        array_push (objects, name);
    }

    retval = query.status;
    query_close (&query);
    RETURN_INT (retval);
}

/*========================================================================*//**
*
* @retval    Filled Owner structure
*
* @brief     Queries OB2BAR objects matching partname, results match instance name if
*            specified, to get newest backup session. Then queries the session to
*            get the backup owner.
*
*//*=========================================================================*/
int QueryBckpOwner (
    IN  int    dbsmhandle,
    IN  tchar *integname,
    IN  tchar *appname,
    IN  tchar *hostname,
    OUT Owner *owner )
{
    ERH_FUNCTION(_T("BarGui_GetBckpOwner"));

    int     iVersion = 0;
    int     retval   = 0;
    int     since    = 0;
    int     until    = 0;
    query_t query    = {0};
    tchar   partname[STRLEN_OBJECTNAME+1] = {0};
    array_t versions = array_new(ARR_TYPE_COPY,sizeof(IntegVer),NULL);

    BG_DBGENTRY;

    StrToObjectName(OT_OB2BAR, hostname, _T(""), integname, partname);
    until = CmnGetUnixTime();
    since = until - 3*SEC_PER_MONTH;

    DbgPlain(BG_DBG_TRACE, _T("[%s] Versions of object part name = %s, since %d until %d"),
        __FCN__, partname, since, until);

    query_open (&query, dbsmhandle, FUN_FBLISTALLOVER, _T("%s%d%d%d%d"), partname, DBA_FBMULTIPLE, 0, since, until);

    retval = query.status;
    if (0 != retval)
        RETURN_INT(retval);

    while (query_read(&query))
    {
        tchar *name = query.record[FLD_OBJ_OBJECTNAME];
        if ( StrIsEmpty(appname) || (StrStr(name, appname) != NULL) )
        {
            IntegVer ver = {0};
            IntegVerInit(&ver);
            IntegVerUnpack(&ver, query.record+NR_OBJ);

            array_push(&versions, &ver);
        }
    }

    for (iVersion = versions.count - 1; iVersion>=0; iVersion--)
    {
        IntegVer *version = array_get(&versions, iVersion);

        query_open (&query, dbsmhandle, FUN_FINDSESSION, _T("%s"), version->sessionname);
        retval = query.status;

        if (0 != retval)
            RETURN_INT(retval);

        query_read(&query);

        if (!query_empty(&query))
        {
            tchar ownerUser [STRLEN_USER +1]    = {0};
            tchar ownerGroup[STRLEN_GROUP+1]    = {0};
            tchar ownerHost [STRLEN_HOSTNAME+1] = {0};

            IntegSession sess = {0};
            IntegSessionInit(&sess);
            IntegSessionUnpack(&sess, query.record);

            sscanf(sess.owner, _T("%[^.].%[^@]@%s"), ownerUser, ownerGroup, ownerHost);
            if ( !StrIsEmpty(ownerUser) && !StrIsEmpty(ownerGroup) && !StrIsEmpty(ownerHost) )
            {
                owner->user  = StrNewCopy(ownerUser);
                owner->group = StrNewCopy(ownerGroup);
                owner->host  = StrNewCopy(ownerHost);
                goto exit;
            }
        }
    }

exit:
    query_close (&query);

    BG_RETURN(retval);
}


/*========================================================================*//**
*
* @brief     backward compatibility wrappers for Query* functions
*            o GetVerObjPartCMN - used only by VSS (FilterVersionCompleted).
*            Removed.
*            o GetVerFilterCMN - used by Informix and Lotus. Removed.
*            o GetVerObjCMN - used a lot. Will keep a wrapper.
*            integObj->objectname MUST contain mount point.
*
*//*=========================================================================*/
int GetVerObjCMN (
    IN  int             cellbind,
    IN  int             dbsmHandle,
    IN  const IntegObj *object,
    OUT array_t        *versions )
{
    ERH_FUNCTION(_T("GetVerObjCMN"));
    IInteg *integ = IQuery(object->apptype);
    tchar   objectname[STRLEN_OBJECTNAME+1]={0};
    array_t objectnames = array_new(ARR_TYPE_STR);
    int     i;
    int     retval;

    DbgFcnIn();
    StrToObjectName(OT_OB2BAR,
        object->hostname,
        object->objectname,
        integ->name,
        objectname);

    array_push (&objectnames, objectname);

    for (i=0; i<object->addObjCount; ++i)
    {
        StrToObjectName(OT_OB2BAR,
            object->hostname,
            object->objectname,
            integ->name,
            objectname);

        array_push (&objectnames, objectname);
    }

    retval = QueryVersionsOfObjects (
        dbsmHandle,
        FilterVersionCompleted,
        NULL,
        0,
        objectnames,
        versions );

    array_free (&objectnames);
    RETURN_INT (retval);
}



/*========================================================================*//**
*
* @retval    a) Value from *RETVAL* if there was one, else
* @retval    b) ErhErrno if query got ERH_OBE_MSG_RES, else
* @retval    c) error from underlying CSA API (e.g. ERH_OBE_CSAREAD)
*
* @brief     Execute FUN_ message with debugging options added.
*            The output of Util binary must be in the following format:
*            [<line>] ...           -> stored in <output> if it isn't NULL
*            stored in _bargui_util_stderr_ otherwise
*            *RETVAL*N              -> N returned as return value
*            [<additional lines>]   -> stored in the _bargui_util_stderr_
*
*            Note: inet sends stdout first, followed by stderr;
*            => stderr appears after *RETVAL*
*
*//*=========================================================================*/
int
UtilExecuteX(IN  int    dbsmHandle,
             IN  tchar *msg,
             IN  int    flag,
             OUT void  *output,
             IN  ...  )
{
    ERH_FUNCTION(_T("UtilExecuteX"));
    USES_BARGUI_PTD

    static ctchar *EOL      = TARGET_WIN32? _T("\r\n") : _T("\n");
    int           retval    = ERH_OBE_NORETVAL;
    query_t       util      = {0};
    BOOL          gotretval = FALSE;

    array_t *array   = output;
    Variant *variant = output;
    tchar   *text    = NULL;

    DbgFcnIn();

    FREE (ThisBarGui->stdErr);

    if (!StrIsEmpty(DbgGetRanges()))
    {
        StrMsgFAppend (
            _T("%s%s%s%s"),
            _T("-debug"),
            DbgGetRanges(),
            DbgGetPostfix(),
            DbgGetSelect()
        );
    }

    query_opent (&util, dbsmHandle, StrMsgGetGlobal());

    if (0 != util.status)
    {
        RETURN_INT(ERH_OBE_DBSMCONNECT);
    }

    while (query_read(&util))
    {
        tchar *line = StrCutTrailingSpaces(util.record[0]);
        DbgPlain (DBG_DETAIL_PROGTRACE, _T("[%s] Got '%s'"), __FCN__, NSD(line));

        if (StrIsEmpty(line))
            goto next;

        if (IsMsgRetVal(line))
        {
            int thisRetval = GetRetValFromMsg(line);
            if (!gotretval)
                retval = thisRetval;

            gotretval = TRUE;
            if (thisRetval != 0)
            {
                StrAppend (ThisBarGui->stdErr, NlsGetMessage(NLS_SET_ERRNO, GetRetValFromMsg(line)));
                StrAppend (ThisBarGui->stdErr, EOL);
            }

            goto next;
        }
        if (gotretval || !output)
        {
            StrAppend (ThisBarGui->stdErr, line);
            StrAppend (ThisBarGui->stdErr, EOL);
        }
        else
        {
            if (flag&UTIL_EXECUTE_VARIANT)
                StrAppend (text, line);
            else
                array_push (array, line);
        }

        next: FREE (line);
    }

    if ((util.status == 0) && (flag&UTIL_EXECUTE_VARIANT))
    {
        *variant = CmnVarUnpack(text);
        FREE (text);
    }


    if ((util.status == ERH_OBE_MSG_RES) && !StrIsEmpty(util.record[0]))
    {
        tchar *t;
        for (t=StrTok(util.record[0], _T("\r\n")); t; t=StrTok(NULL, _T("\r\n")))
        {
            StrAppend (ThisBarGui->stdErr, t);
            StrAppend (ThisBarGui->stdErr, EOL);
        }
    }

    if (retval==ERH_OBE_NORETVAL)
        switch (util.status)
        {
        case 0:
            break;
        case ERH_OBE_MSG_RES:
            retval = ErhErrno(); 
            break;
        default:
            retval = util.status;
        }

    ErhClearError();
    query_close (&util);
    RETURN_INT (retval);
}


int
UtilExecute(IN  int          dbsmHandle,
            OUT array_t     *output,
            IN  const tchar *hostname,
            IN  const tchar *utilname,
            IN  const tchar *firstArg, // we know we need at least one - let compiler detect errors.
            IN  ...)
{
    tchar   msg[MAX_STRING_SIZE+1] = {0};
    const tchar  *t;
    va_list va;
    va_start (va, firstArg);

    StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTIL, hostname, utilname, firstArg, NULL);
    while (NULL != (t = va_arg(va, const tchar*)))
        StrMsgAppend(t);
    va_end(va);
    return UtilExecuteT (dbsmHandle, output, msg);
}


int
UtilExecuteEx(IN  int          dbsmHandle,
              OUT array_t     *output,
              IN  BOOL         switchUser,
              IN  const tchar *hostname,
              IN  const tchar *username,
              IN  const tchar *groupname,
              IN  const tchar *utilname,
              IN  const tchar *firstArg,
              ...)
{
    tchar   msg[MAX_STRING_SIZE+1] = {0};
    const tchar *t;
    va_list va;
    va_start (va, firstArg);

    if (!switchUser || StrIsEmpty(username) || StrIsEmpty(groupname))
    {
        StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTIL, hostname, utilname, firstArg, NULL);
        while (NULL != (t = va_arg(va, const tchar*)))
            StrMsgAppend(t);
    }
    else
    {
        StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTILEX, hostname, username, groupname, utilname, firstArg, NULL);
        while (NULL != (t = va_arg(va, const tchar*)))
            StrMsgAppend(t);
    }
    va_end (va);
    return UtilExecuteT (dbsmHandle, output, msg);
}

int
UtilExecuteExV(IN  int           dbsmHandle,
               OUT Variant      *output,
               IN  BOOL          switchUser,
               IN  const tchar  *hostname,
               IN  const tchar  *username,
               IN  const tchar  *groupname,
               IN  const tchar  *utilname,
               IN  const tchar  *firstArg,
               ...)
{
    tchar   msg[MAX_STRING_SIZE+1] = {0};
    tchar  *t;
    va_list va;
    va_start (va, firstArg);

    if (!switchUser || StrIsEmpty(username) || StrIsEmpty(groupname))
    {
        StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTIL, hostname, utilname, firstArg, NULL);
        while (NULL != (t=va_arg(va, tchar*)))
            StrMsgAppend(t);
    }
    else
    {
        StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTILEX, hostname, username, groupname, utilname, firstArg, NULL);
        while (NULL != (t=va_arg(va, tchar*)))
            StrMsgAppend(t);
    }
    va_end (va);
    return UtilExecuteX (dbsmHandle, msg, UTIL_EXECUTE_VARIANT, output);
}

/* ----------------------------------------------------------------------
|  BG_ API helpers
 ---------------------------------------------------------------------- */
void CmnVecAddOpt (CmnVec *vec, const tchar *option, const tchar *value)
{
    if (!StrIsEmpty(option) && !StrIsEmpty(value))
    {
        tchar *buf = MALLOC ((StrLen(option) + StrLen(value) + 2)*sizeof(tchar));
        option[StrLen(option)-1]==_T(':')?
            sprintf (buf, _T("%s%s"),  option, value):
            sprintf (buf, _T("%s:%s"), option, value);
        CmnVecAddStr (vec, buf);
        FREE (buf);
    }
}

/* some integrations can not parse the ':' character as delimiter */
void CmnVecAddOptOld(CmnVec *vec, const tchar *option, const tchar *value)
{
    if (!StrIsEmpty(option) && !StrIsEmpty(value))
    {
        tchar *buf = MALLOC ((StrLen(option) + StrLen(value) + 2)*sizeof(tchar));
        sprintf (buf, _T("%s %s"), option, value);
        CmnVecAddStr (vec, buf);
        FREE (buf);
    }
}

void CmnVecAddIntOpt (CmnVec *vec, const tchar *option, int value)
{
    tchar v[STRLEN_INT+1];
    sprintf (v, _T("%d"), value);
    CmnVecAddOpt (vec, option, v);
}


void CmnVecAddInt (CmnVec *vec, int n)
{
    CmnVecAdd (vec, &n, sizeof(n));
}

void CmnVecToCmdline (IN CmnVec *vec, OUT MALLOCED tchar **cmdline)
{
    tchar *t = StrNewCopy(_T(""));
    int i;
    for (i=0; i<CmnVecH(vec); ++i)
    {
        StrAppend (t, _T(" \""));
        StrAppend (t, CmnVecI(vec, i));
        StrAppend (t, _T("\" "));
    }
    *cmdline = t;
}

void CmnVecToArgv (IN const CmnVec *vec, OUT int *pargc, OUT tchar ***pargv)
{
    int i;
    *pargc = CmnVecH(vec);
    *pargv = MALLOC (sizeof(tchar*) * (*pargc));
    for (i=0; i<*pargc; ++i)
        (*pargv)[i] = CmnVecI(vec, i);
}

void CmnVecDump (CmnVec *vec)
{
    int i;
    DbgPlain (BG_DBG_TRACE, _T("Dumping CmnVec %p"), vec);
    for (i=0; i<CmnVecH(vec); ++i)
        DbgPlain (BG_DBG_TRACE, _T("\t  |%s"), CmnVecI(vec, i));
}


tchar *StrMap (CStrPair *list, tchar *first)
{
    for (; list && list->first; ++list)
        if (0==StrICmp(list->first, first))
            return list->second;
    return 0;
}

const tchar *StrExtName (const tchar *path)
{
    const tchar *basename = StrBasename(path);
    tchar *dot = basename? strrchr(basename, _T('.')) : NULL;
    return dot? dot+1 : NULL;
}


tchar *GetBackupId (IN tchar *opt)
{
    USES_BARGUI_PTD
    tchar *backupId = ThisBarGui->backupId;

    tok_t t;
    backupId[0] = _T('\0');
    t = tok_init (opt);
    if (0==StrCmp(tok_get(&t), _T("-backupId")))
        StrCopy (backupId, tok_get(&t), STRLEN_SESSIONNAME);
    tok_free(&t);
    return StrIsEmpty(backupId)? NULL : backupId;
}


/*========================================================================*//**
*
* @retval    PlatformID          on OK
* @retval    ERH_OBE_INTERNAL    on error
*
* @brief     Returns the platform id of the specified host.
*
*//*=========================================================================*/
int GetPlatformID(int bind, tchar *host)
{
    ERH_FUNCTION(_T("GetPlatformID"));
    tchar     *cStr=NULL;
    THostList *phl, *p;
    int platformId = 0;

    BG_DBGENTRY;

    DbgStamp(10);
    DbgPlain(10, _T("%s -> Getting nodes info file \n"), __FCN__);

    if (MCsaGetSingleFile(bind, S_CELL, &cStr, READ_ONLY) < 0)
    {
        DbgStamp(10);
        DbgPlain(10, _T("CsaGetSingleFile-> ERROR \n"));
        BG_RETURN(ERH_OBE_INTERNAL);
    }
    DbgStamp(10);
    DbgPlain(10, _T("%s -> Parse nodes list \n"), __FCN__);

    phl=CellPparseHosts(cStr);

    DbgStamp(10);
    DbgPlain(10, _T("%s -> Going through nodes list \n"), __FCN__);

    for (p=phl; p; p=p->next)
    {
        if (StrCmp(p->name, host) == 0)
        {
            platformId = p->platformId;
            break;
        }
    }

    CellPdeleteHostList(phl);
    FREE(cStr);

    RETURN_INT(platformId);
}


/*========================================================================*//**
*
* @retval    0   message is not a return value
* @retval    1   message is a return value
*
* @brief     Verifies if a message is a return value.
*
*//*=========================================================================*/
int IsMsgRetVal(tchar *text)
{
    return StrStr(text, _T("*RETVAL*")) != NULL;
}


/*========================================================================*//**
*
* @retval    retval
*
* @brief     Returns the return value from a string.
*
*//*=========================================================================*/
int GetRetValFromMsg(tchar *szMsg)
{
    ERH_FUNCTION(_T("GetRetValFromMsg"));
    int retval=-1;

    DbgFcnIn();

    if (!IsMsgRetVal(szMsg))
        RETURN_INT(0);

    sscanf(szMsg, _T("*RETVAL*%d"), &retval);

    if (retval != OBE_OK)
    {
        if (ErhErrno() == -1)
            ErhClearError();

        if (ErhErrno() == 0)
            ErhMark(retval, __FCN__, ERH_WARNING);
    }
    RETURN_INT(retval);
}


int IntegObjCmpName (const void *a, const void *b)
{
    const IntegObj *x = a;
    const IntegObj *y = b;
    return StrCmp (x->name, y->name);
}


int IntegObjCmpTypeAndName (const void *a, const void *b)
{
    const IntegObj *x = a;
    const IntegObj *y = b;

    return 
        x->tag > y->tag? -1 : 
        x->tag < y->tag?  1 :
        StrICmp (x->name, y->name);
}


tchar *IntegObjGetName (const void *a)
{
    return a? ((IntegObj*)a)->name : NULL;
}


tchar *IntegObjGetObjectName (const void *a)
{
    return a? ((IntegObj*)a)->objectname : NULL;
}


tchar *IntegVerGetSession (const void *a)
{
    return a? ((IntegVer*)a)->sessionname : NULL;
}


int IntegVerCmpSessionName (const void *a, const void *b)
{
    const IntegVer *x = a;
    const IntegVer *y = b;

    return StrCmp (x->sessionname, y->sessionname);
}


int IntegInstalled (THostList *host, int integType)
{
    switch (integType)
    {
    #define CASE(TYPE,CONDITION) case TYPE: return (CONDITION)!=0;
    CASE (OB2_BAR_SAP,       host->SAP);
    CASE (OB2_BAR_SAPDB,     host->SAPDB);
    CASE (OB2_BAR_ORACLE,    host->Oracle);
    CASE (OB2_BAR_ORACLE8,   host->Oracle8);
    CASE (OB2_BAR_MSSQL,     host->MSSQL || host->MSSQL70);
    CASE (OB2_BAR_MSEXCHANGE,host->MSExchange);
    CASE (OB2_BAR_MSESE,     host->MSESE);
    CASE (OB2_BAR_ENABLER,   host->OMS);
    CASE (OB2_BAR_INFORMIX,  host->Informix);
    CASE (OB2_BAR_SYBASE,    host->Sybase);
    CASE (OB2_BAR_VSS,       host->VSS);
    CASE (OB2_BAR_MBX,       host->MBX);
    CASE (OB2_BAR_DB2,       host->DB2);
    CASE (OB2_BAR_LOTUS,     host->Lotus);
    CASE (OB2_BAR_MSSPS,     host->MSSPS);
/*  CASE (OB2_BAR_VEPA,      host->VEPA || host->VMware); */
    /* even machines without the actual VEP agent are considered for VEPA */
    /* thus distinction is made elsewhere */
    CASE (OB2_BAR_VEPA,      TRUE);
    CASE (OB2_BAR_VMWARE,    host->VMware);
    CASE (OB2_BAR_EXCH2010,  host->E2010);
    CASE (OB2_BAR_MSSPS2007, host->MSSPS2007);
    CASE (OB2_BAR_SAPHANA,   host->SAPHANA);
    CASE (OB2_BAR_MYSQL,     host->MySQL);
    CASE (OB2_BAR_POSTGRESQL,     host->PostgreSQL);
    // DMA is part of ts_core package
    CASE (OB2_BAR_DMA,       host->TSCORE != NULL ? 1 : 0);
    // Internal Database backup is always available
    CASE (OB2_BAR_IDB,       1);

    default:
        return 0;
    #undef CASE
    }
}

/*========================================================================*//**
*
* @retval   0
* @input    hostinformation, integType vepa 
* @brief    Will check if vepa component is installed or not.
*
*//*=========================================================================*/
int VepaInstalled (THostList *host, int integType)
{
    switch (integType)
    {
    #define CASE(TYPE,CONDITION) case TYPE: return (CONDITION)!=0;
    CASE (OB2_BAR_SAP,       host->SAP);
    CASE (OB2_BAR_SAPDB,     host->SAPDB);
    CASE (OB2_BAR_ORACLE,    host->Oracle);
    CASE (OB2_BAR_ORACLE8,   host->Oracle8);
    CASE (OB2_BAR_MSSQL,     host->MSSQL || host->MSSQL70);
    CASE (OB2_BAR_MSEXCHANGE,host->MSExchange);
    CASE (OB2_BAR_MSESE,     host->MSESE);
    CASE (OB2_BAR_ENABLER,   host->OMS);
    CASE (OB2_BAR_INFORMIX,  host->Informix);
    CASE (OB2_BAR_SYBASE,    host->Sybase);
    CASE (OB2_BAR_VSS,       host->VSS);
    CASE (OB2_BAR_MBX,       host->MBX);
    CASE (OB2_BAR_DB2,       host->DB2);
    CASE (OB2_BAR_LOTUS,     host->Lotus);
    CASE (OB2_BAR_MSSPS,     host->MSSPS);
/*  CASE (OB2_BAR_VEPA,      host->VEPA || host->VMware); */
    /* even machines without the actual VEP agent are considered for VEPA */
    /* thus distinction is made elsewhere */
    CASE (OB2_BAR_VEPA,      host->VEPA);
    CASE (OB2_BAR_VMWARE,    host->VMware);
    CASE (OB2_BAR_EXCH2010,  host->E2010);
    CASE (OB2_BAR_MSSPS2007, host->MSSPS2007);
    CASE (OB2_BAR_SAPHANA,   host->SAPHANA);
    CASE (OB2_BAR_MYSQL,     host->MySQL);
    CASE (OB2_BAR_POSTGRESQL,     host->PostgreSQL);
    // DMA is part of ts_core package
    CASE (OB2_BAR_DMA,       host->TSCORE != NULL ? 1 : 0);
    // Internal Database backup is always available
    CASE (OB2_BAR_IDB,       1);

    default:
        return 0;
    #undef CASE
    }
}

array_t CellEnumHosts (
    int cellbind,
    int integType )
{
    ERH_FUNCTION(_T("CellEnumHosts"));
    THostList *hostlist, *hostinfo;
    IInteg    *integ = IQuery(integType);
    tchar     *buffer;
    array_t    hosts = array_new(ARR_TYPE_STR);
    DbgFcnIn();

    if (!integ)
    {
        RETURN_STRUCT (hosts);
    }

    DbgPlain (BG_DBG_TRACE, _T("[%s] Integration=%s"), __FCN__, integ->name);

    // cmnPanConfig/cell/cell_info
    if (MCsaGetSingleFile(cellbind, S_CELL, &buffer, READ_ONLY) < 0)
    {
        if (0==ErhErrno())
        {
            ErhMark(ERH_OBE_CELLINFO, __FCN__, ERH_WARNING);
        }
        RETURN_STRUCT (hosts);
    }

    hostlist = CellPparseHosts(buffer);
    FREE (buffer);

    for (hostinfo = hostlist; hostinfo; hostinfo=hostinfo->next)
    {
        if (!IntegInstalled(hostinfo, integType))
            continue;

        array_push (&hosts, hostinfo->name);
        DbgPlain (
            BG_DBG_TRACE,
            _T("[%s] integration=%s host=%s type=%d"),
            __FCN__,
            integ->name,
            hostinfo->name,
            hostinfo->platformId);
    }

    CellPdeleteHostList(hostlist);

    RETURN_STRUCT (hosts);

}

/*========================================================================*//**
*
* @retval   array
* @input    cellbind, integType vepa 
* @brief    Will give array of clients with vepa component installed.
*
*//*=========================================================================*/
array_t CellEnumVepaHosts (
    int cellbind,
    int integType )
{
    ERH_FUNCTION(_T("CellEnumVepaHosts"));
    THostList *hostlist, *hostinfo;
    IInteg    *integ = IQuery(integType);
    tchar     *buffer;
    array_t    hosts = array_new(ARR_TYPE_STR);
    DbgFcnIn();

    if (!integ)
    {
        RETURN_STRUCT (hosts);
    }

    DbgPlain (BG_DBG_TRACE, _T("[%s] Integration=%s"), __FCN__, integ->name);

    // cmnPanConfig/cell/cell_info
    if (MCsaGetSingleFile(cellbind, S_CELL, &buffer, READ_ONLY) < 0)
    {
        if (0==ErhErrno())
        {
            ErhMark(ERH_OBE_CELLINFO, __FCN__, ERH_WARNING);
        }
        RETURN_STRUCT (hosts);
    }

    hostlist = CellPparseHosts(buffer);
    FREE (buffer);

    for (hostinfo = hostlist; hostinfo; hostinfo=hostinfo->next)
    {
        if (!VepaInstalled(hostinfo, integType))
            continue;
        if(hostinfo->clus == E_CLUS_INTEGVS)
            continue;
        array_push (&hosts, hostinfo->name);
        DbgPlain (
            BG_DBG_TRACE,
            _T("[%s] integration=%s host=%s type=%d"),
            __FCN__,
            integ->name,
            hostinfo->name,
            hostinfo->platformId);
    }

    CellPdeleteHostList(hostlist);

    RETURN_STRUCT (hosts);

}

// full path to integ object
// GUI uses IntegObj.objectname as slash-separated name to
// determine tree hierarchy
MALLOCED tchar *IntegObjPath (IntegObj *obj)
{
    tchar *objectname = NULL;
    if (!StrIsEmpty(obj->objectname))
        return StrNewCopy(obj->objectname);
    for (; obj; obj=obj->prev)
    {
        tchar *old = StrNewCopy(objectname);
        objectname = REALLOC (objectname, (StrLen(old)+StrLen(obj->name)+2)*sizeof(tchar));
        sprintf (objectname, _T("/%s%s"), obj->name, old);
        FREE (old);
    }
    return objectname;
}

GUI *BarGui_GetContext()
{
    USES_BARGUI_PTD
    return &ThisBarGui->gui;
}


Variant BG_ParseUsage (IN const tchar *usage, IN const CmnVec *args)
{
    Variant options;
    int     argc;
    tchar **argv;
    CmnVecToArgv (args, &argc, &argv);
    options = GetOptParseUsage (usage, argc, argv);
    FREE (argv);
    return options;
}

/* === EOF === */
