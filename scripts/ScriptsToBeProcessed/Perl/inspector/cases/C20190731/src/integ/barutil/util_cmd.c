/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Option Parser (Opt)
* @file      integ/barutil/util_cmd.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Perform various CM-related tasks on behalf of scripts.
*            All functions return TRUE/FALSE and set ErhErrno.
*
*
* @since     22.9.2000   antonk          Initial Coding
*            xx.xx.2003  milosm          Rewrite
*/
#include "lib/cmn/target.h"
__rcsId("$Header: /integ/barutil/util_cmd.c $Rev$ $Date::                      $:") ;

#include "lib/cmn/common.h"

#include "lib/parse/gen_parser.h"
#include "lib/parse/wlparse.h"
#include "lib/parse/wllist.h"
#include "lib/parse/cellinfo.h"

#include "lib/xtra/scramble.h"
#include "lib/ipc/ipc.h"
#include "lib/os/libos.h"
#include "cs/csa/csa.h"
#include "ob2util.h"
#include "ob2rexec.h"
#include "integ/bar2/libbar.h"
#include "xtra/query.h"
#include "xtra/ob2com.h"
#include "xtra/ob2com_server.h"
#include "xtra/task.h"
#include "optmgr.h"


#if TARGET_WIN32
#   pragma warning(disable: 4100)
#endif

/* Return codes are similar to the ones from grep command on unix */
#define RESULT_SUCCESS   0
#define RESULT_FAILURE   2

#define MSG_PEEK_ERROR   -1
#define MSG_PEEK_NO_DATA -2

/* for user check: should the user be just user or more */
#define USER_EXISTS                 0
#define USER_CAN_SEE_PRIVATE_OBJ    1

/* Default output separator between fields in message */
#define QUERY_DEFAULT_COLSEP _T("|")

#define UTIL_CMD_EXE  _T("util_cmd.exe")
#define RCP_TIMEOUT 60
#define MSG_METADATA     2000

#ifdef DPCOM_EMULATE_DLSYM
    static void ExportStubs(void);
#endif

static BOOL EncodeString (IN const tchar *plaintext)
{
    tchar *encoded = NULL;
    ERH_FUNCTION(_T("EncodeString"));

    DbgFcnIn();

    if (NULL==(encoded=EncodePassword(plaintext)))
        RETURN_BOOL (FALSE);

    BU_Printf(_T("%s\n"), encoded);
    FREE (encoded);
    RETURN_BOOL (TRUE);
}


static BOOL DecodeString (IN ctchar *encoded)
{
    tchar *decoded = NULL;
    ERH_FUNCTION(_T("DecodeString"));

    DbgFcnIn();

    if (NULL==(decoded=DecodePassword(encoded)))
        RETURN_BOOL(FALSE);

    BU_Printf(_T("%s\n"), decoded);
    FREE (decoded);
    RETURN_BOOL(TRUE);
}


static const tchar*
GetConfigFilename (IN const tchar *integtype, IN const tchar *basename)
{
    THREADLOCAL static tchar filename[STRLEN_PATH+1];
    CreatePathFromParams(filename, _T("config/%s/%s"), integtype, basename);
    return filename;
}


static const tchar*
GetIntegConfigname(IN const tchar *hostname, IN const tchar *instance)
{
    THREADLOCAL static tchar filename[STRLEN_PATH+1];
    instance?
        CreatePathFromParams(filename, _T("%s%%%s"), hostname, instance) :
        CreatePathFromParams(filename, _T("%s"),     hostname);
    return filename;
}


/* ===========================================================================
| @brief    Construct datalist/barlist path relative to cmnPanConfig
 ========================================================================== */
static const tchar*
GetDatalistName(IN OPTIONAL const tchar *apptype, const tchar *name)
{
    THREADLOCAL static tchar path[STRLEN_PATH+1];
    StrIsValid(apptype) && 0 != StrICmp(apptype, OB2_APP_OB2)?
        PathFormat(path,_T("%s/%s/%s"), G_TOP_INTEGLIST_PATH, apptype, name) :
        PathFormat(path,_T("%s/%s"), G_WORKLIST_PATH, name);
    return path;
}


static BOOL CellServer_FileWrite(IN const tchar *integtype, IN const tchar *filename, const tchar *buffer)
{
    ERH_FUNCTION (_T("CellServer_FileWrite"));
    int handle;
    DbgFcnIn();

    handle = MCsaBindServer(opt.cellserver);
    if (handle >= 0)
    {
        MCsaPutGroupItem(handle, G_TOP_INTEG_CNF, GetConfigFilename(integtype, filename),
            buffer, MODIFY);
        MCsaUnbindServer (handle,0);
    }
    RETURN_BOOL (ErhErrno() == 0);
}


static MALLOCED tchar *CellServer_FileRead (IN const tchar *integtype, IN const tchar *filename)
{
    ERH_FUNCTION (_T("CellServer_FileRead"));
    tchar *buffer = NULL;
    int    handle;
    DbgFcnIn();

    handle = MCsaBindServer(opt.cellserver);
    if (handle >= 0)
    {
        MCsaGetGroupItem (handle, G_TOP_INTEG_CNF, GetConfigFilename(integtype, filename),
            &buffer, MODIFY);
        MCsaUnbindServer (handle, 0);
    }
    RETURN_PTR (buffer);
}


static MALLOCED tchar *Local_FileRead (const tchar *filename)
{
    return CmnGetAsciiFile(filename);
}


static BOOL Local_FileWrite (IN const tchar *filename, IN const tchar *buffer)
{
    ERH_FUNCTION (_T("Local_FileWrite"));
    DbgFcnIn();
    if (filename)
    {
        int retval = CmnPutAsciiFile (filename, buffer);
        RETURN_BOOL (retval == 0);
    }

    if (-1 == BU_Printf (_T("%s\n"), NS(buffer)))
    {
        ErhMarkSysMajor ();
        RETURN_BOOL (FALSE);
    }

    RETURN_BOOL (TRUE);
}


/*--------------------------------------------------------------------------
| @section  QUERIES
| @todo     wrap GetObjectsOfSession around RunQuery
 --------------------------------------------------------------------------*/
static BOOL GetObjectsOfSession(IN const tchar *session)
{
    IpcHandle dbsm = INVALID_IPC_HANDLE;
    query_t   query;

    ERH_FUNCTION(_T("GetObjectsOfSession"));

    DbgFcnIn();

    session = StrFromUserSessionId(session);
    if (StrIsEmpty(session))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL(FALSE);
    }

    StrMsgMake(NULL, 0, MSG_OPTIONS, NULL);

    if ((dbsm=CsaStartSession (DB, NULL))<0)
    {
        RETURN_BOOL(FALSE);
    }

    query_open (&query, dbsm, FUN_LISTOVEROFAPPBACKUP,
        _T("%s%s%s"), 
        session,    
        opt.hostname,
        opt.integtype
    );
    query.flags |= QUERY_BREAK_ON_UNKNOWN_MESSAGE;

    if (0 != query.status)
    {
        goto on_exit;
    }

    while (query_read(&query))
    {
        int lType=-1;
        tchar pszHost[STRLEN_HOSTNAME+1]={0};
        tchar pszMpoint[STRLEN_BAROBJNAME+1]={0};
        tchar pszLabel[STRLEN_APPTYPE+1]={0};

        StrFromObjectName(
            &lType, pszHost, pszMpoint, pszLabel,
            query.record[FLD_OBJ_OBJECTNAME]
        );
         BU_Printf(_T("%s:%s\t%s\n"), pszHost, pszMpoint, pszLabel);
    }

    query_close (&query);

on_exit:

    if (dbsm != INVALID_IPC_HANDLE)
    {
        CsaDisconnectSession(dbsm);
    }

    RETURN_BOOL (ErhErrno()==0);
}


/*========================================================================*//**
*
* @retval    TRUE      no error
* @retval    FALSE     error occurred and is marked as:
*                      ERH_OBE_MSG_INV  got invalid message id
*                      ERH_OBE_INVARG   invalid arguments for message
*                      *                marked by CsaWrite, CsaRead, or received
*                                       with MSG_RESULT
*
* @brief     Executes single query and outputs results to the stdout.
*            See query.c for query types and parameters.
*
*//*=========================================================================*/
static void QueryPrint (IN query_t query, IN array_t *selected_columns, BOOL rawOutput)
{
    int i,j;
    Variant rec = VarArray(VarUndef);

    if (selected_columns->count)
    {
        for (i=0; i<selected_columns->count; ++i)
        {
            j = *((int *)array_get(selected_columns, i)) - 1;
            if (j>=0 && j<query.length)
                VarPush (&rec, T2V(query.record[j]));
        }
    }
    else
    {
        for (j=0; j<query.length; ++j)
        {
            VarPush (&rec, T2V(query.record[j]));
        }
    }

    if (rawOutput)
    {
        for (i=0; i<VarLength(&rec); ++i)
            ConPrintf (_T("%s\n"), VarTextAt(&rec,i));
    }
    else
    {
        tchar *packed = VarPack(&rec);
        if (packed)
            ConPrintf (_T("%s\n"), packed);

        ConPrint (_T("*RESULT*0\n"));

        FREE (packed);
    }

    VarFree (&rec);
}


static BOOL
QueryRun(MODIFIED IpcHandle *dbsmHandle, IN const tchar *id, IN const argvRec *args, IN const tchar *select)
{
    ERH_FUNCTION (_T("QueryRun"));
    BOOL            ok;
    int             i;
    int             funId;
    query_t         query = {0};
    array_t         columns = array_new(ARR_TYPE_CONTIG, sizeof(int), NULL);
    StrMsg          msg = {0};
    BOOL            rawOutput;

    const query_format_t *query_format = query_get_description_by_name (id);

    DbgFcnIn();
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] id='%s', columns='%s'"),
        __FCN__,
        NSD(id),
        NSD(select));


    if (!query_format)
    {
        ErhMarkMajor (ERH_OBE_MSG_INV);
        RETURN_BOOL (FALSE);
    }

    funId = query_format->funId;

    /* Validate query parameters */
    if (args->argc < query_format->numParams)
    {
        DbgPlain(DBG_MAIN_ACTION, _T("%s: Missing parameters. Required %d, received %d."),
            __FCN__, query_format->numParams, args->argc);
        ErhMarkMajor (ERH_OBE_INVARG);
        ErhSetErrorInfo(NS(query_format->description));
        RETURN_BOOL (FALSE);
    }

    /* Create array of selected columns */
    if (select)
    {
        tchar *t;
        tchar *sel = StrNewCopy (select);

        for (t=strtok(sel,_T(",")); t; t=strtok(NULL, _T(",")))
        {
            int id = atoi(t);
            array_push(&columns, &id);
        }
        FREE (sel);
    }

    /* Start DBSM unless already started */
    if (*dbsmHandle == INVALID_IPC_HANDLE)
    {
        *dbsmHandle = CsaStartSessionDB();
        if (*dbsmHandle < 1)
        {
            RETURN_BOOL (FALSE);
        }
    }

    /* Compose and send message */
    StrMsgStartL(&msg, NULL, 0, funId);
    for (i=0; i<args->argc; ++i)
    {
        StrMsgAppendL (&msg, args->argv[i]);
    }
    query_opent (&query, *dbsmHandle, &msg);
    StrMsgFreeL(&msg);

    if (0 != query.status)
    {
        RETURN_INT (FALSE);
    }

    rawOutput = FUN_EXECINTEGUTIL==funId || FUN_EXECINTEGUTILEX==funId;

    while (query_read(&query))
    {
        QueryPrint (query, &columns, rawOutput);
    }

    if (!query_empty(&query))
    {
        QueryPrint (query, &columns, rawOutput);
    }

    ok = query.status == 0;

    query_close (&query);

    RETURN_BOOL (ok);
}


static BOOL Query (
    IN const tchar   *id,
    IN const argvRec *args,
    IN const tchar   *columns,
    IN const tchar   *commandfile)
{
    ERH_FUNCTION (_T("Query"));
    IpcHandle dbsmHandle = INVALID_IPC_HANDLE;
    tchar  *file = NULL, *line;
    BOOL    ok = TRUE;
    DbgFcnIn();

    if (id)
    {
        QueryRun (&dbsmHandle, id, args, columns);
        goto on_return;
    }

    file = Local_FileRead(commandfile);
    if (!file)
        goto on_return;

    for (line=StrTok(file, _T("\r\n")); line && ok; line=StrTok(NULL, _T("\r\n")))
    {
        argvRec cmdline = {0}, args = {0};

        ArgvReadFromBuffer (&cmdline, line);
        if (cmdline.argc <= 0)
            continue;

        id = cmdline.argv[0];
        args.argc = cmdline.argc-1;
        args.argv = cmdline.argv+1;

        ok = QueryRun(&dbsmHandle, id, &args, columns);
        BU_Fprintf (stdout, _T("*RETVAL*%d\n"), ErhErrno());
        ArgvFree(&cmdline);
    }

on_return:
    if (dbsmHandle != INVALID_IPC_HANDLE)
        CsaDisconnectSession(dbsmHandle);

    FREE(file);
    RETURN_BOOL (ErhErrno()==0);
}


/**
* @param host host name where command should be executed
* @param mountPoint to be listed
* @param dir directory on selected mount point
*
* @brief This function will execute FSBRDA on remote host and list provided directory
*        -listremotedir hostname.deu.hp.com c: /tmp
 */
static BOOL
ListRemoteDir(IN const tchar *host, IN const tchar *mountPoint, IN const tchar *dir, IN tchar *columns)
{
    ERH_FUNCTION(_T("ListRemoteDir"));

    IpcHandle dbsm = INVALID_IPC_HANDLE;
    argvRec   args = {0};

    DbgFcnIn();

    if (StrIsEmpty(host) || StrIsEmpty(mountPoint) || StrIsEmpty(dir))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL(FALSE);
    }

    StrMsgMake(NULL, 0, MSG_OPTIONS, NULL);

    if ((dbsm=CsaStartSession (DB, NULL))<0)
    {
        RETURN_BOOL(FALSE);
    }
    ArgvAdd(&args, host);
    ArgvAdd(&args, mountPoint);
    QueryRun(&dbsm, _T("FUN_FSBROWSE"), &args, columns);

    ArgvFree(&args);
    ArgvAdd(&args, dir);
    QueryRun(&dbsm, _T("FUN_FSLISTDIR"), &args, columns);
    ArgvFree(&args);

    if (dbsm != INVALID_IPC_HANDLE)
    {
        CsaDisconnectSession(dbsm);
    }

    RETURN_BOOL (ErhErrno()==0);
}


/*========================================================================*//**
*
* @brief    Download integ config file from CS and write it to <outputfile>
*
* @note     When writing to file UCS-2 is used; when writing to stdout UTF8 is used.
*           Config file is parsed for proper format. 
*           <hostname> is expanded.
*           BU/BP API does not ErhMark parsing errors, so the caller must mark
*           ERH_FFORMAT (or similar)
*
*//*=========================================================================*/
static BOOL GetConfig (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *outputfile )
{
    ERH_FUNCTION(_T("GetConfig"));
    BP_Opt  config = {0};
    int     retval;
    tchar  *buffer;

    DbgFcnIn();

    DbgPlain (DBG_MAIN_ACTION, _T("[%s] Read %s/%s%%%s"), __FCN__, integtype, hostname, instance);
    retval = BU_GetConfig (opt.cellserver, hostname, integtype, instance, &config);

    if (BP_NOERROR != retval)
    {
        ErhMarkBP (retval);
        RETURN_BOOL (FALSE);
    }

    buffer = BP_Dump (config);
    Local_FileWrite (outputfile, buffer);
    FREE (buffer);
    BP_Free (&config);

    RETURN_BOOL (ErhErrno()==0);
}


static int PutConfig (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *inputfile )
{
    ERH_FUNCTION(_T("PutConfig"));
    BP_Opt config = {0};
    tchar *buffer = NULL;
    int    retval;
    DbgFcnIn();

    buffer = Local_FileRead (inputfile);
    if (!buffer)
        RETURN_BOOL (FALSE);

    BP_InitAsList  (&config,_T("top"));
    retval = BP_ParseBuffer (buffer, NULL, &config);
    if (BP_NOERROR == retval)
        retval  = BU_PutConfig (opt.cellserver, hostname, integtype, instance, config);
    BP_Free (&config);
    FREE (buffer);

    ErhMarkBP (retval);
    RETURN_INT(ErhErrno() == 0);
}


/*--------------------------------------------------------------------------
|   FUNCTION
|       GetOption/PutOption
 --------------------------------------------------------------------------*/
static BOOL GetOption (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *inputfile,
    IN const tchar *sublist,
    IN const tchar *option )
{
    ERH_FUNCTION(_T("GetOption"));
    BP_Opt  config = {0};
    BP_Opt *list, *elem;
    int     retval = BP_NOERROR;

    DbgFcnIn();

    DbgPlain (DBG_MAIN_ACTION, _T("[%s] Read %s/%s%%%s"), __FCN__, integtype, hostname, instance);
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] Find %s/%s"), __FCN__, NS(sublist), NS(option));
    
    if (StrIsEmpty(option))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL  (FALSE);
    }

    retval = inputfile?
        BU_GetLocalFile (inputfile, &config) :
        BU_GetConfig    (opt.cellserver, hostname, integtype, instance, &config);

    if (BP_NOERROR != retval)
    {
        ErhMarkBP (retval);
        RETURN_BOOL (FALSE);
    }

    list = sublist? BP_GetOptByPath (config, sublist) : &config;
    elem = list   ? BP_GetOptByName (*list,  option ) : NULL;
    if (elem)
    {
        tchar *buffer = BP_Dump (*elem);
        Local_FileWrite (NULL, buffer);
        FREE (buffer);
    }
    BP_Free (&config);
    RETURN_BOOL (ErhErrno()==0);
}


static BOOL PutOption (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *localfile,
    IN const tchar *sublist,
    IN const tchar *option,
    IN const tchar *value )
{
    ERH_FUNCTION(_T("PutOption"));
    BP_Opt  config = {0};
    BP_Opt *list;
    int     r;

    DbgFcnIn();

    if (StrIsEmpty(option) || (StrIsEmpty(localfile) && 
        (StrIsEmpty(hostname) || StrIsEmpty(integtype) || StrIsEmpty(instance))))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL  (FALSE);
    }

    r = localfile?
        BU_GetLocalFile (localfile, &config) :
        BU_GetConfig    (opt.cellserver, hostname, integtype, instance, &config);

    if (BP_NOERROR != r)
    {
        ErhMarkBP (r);
        RETURN_BOOL (FALSE);
    }

    list = sublist? BP_GetOptByPath (config, sublist) : &config;
    if (!list)
    {
        BP_AddListByPath(&config, sublist, &list);
    }
    if (list)
    {
        BP_RemoveOptByName (list, option);
        if (StrIsEmpty(value))
            BP_AddStr(list, option, strEmpty);
        else if (StrIsInt(value))
            BP_AddInt(list, option, StrToInt(value));
        else
            BP_AddStr(list, option, StrTrimQuotes(value));
    }

    localfile?
        BU_PutLocalFile (localfile, config) :
        BU_PutConfig    (opt.cellserver, hostname, integtype, instance, config);

    BP_Free (&config);

    RETURN_BOOL (ErhErrno()==0);
}


/*========================================================================*//**
*
* @brief     Gets/puts file <integtype>/<methodfile> w/o any checks for file
*            format.
*
*//*=========================================================================*/
static int GetMethodFile (
    IN const tchar *integtype,
    IN const tchar *methodfile,
    IN const tchar *outputfile )
{
    tchar *buffer = NULL;

    ERH_FUNCTION(_T("GetMethodFile"));
    DbgFcnIn();

    if (StrIsEmpty(integtype) || StrIsEmpty(methodfile))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL  (FALSE);
    }

    buffer = CellServer_FileRead (integtype, methodfile);
    if (buffer)
        Local_FileWrite (outputfile, buffer);

    FREE(buffer);
    
    RETURN_BOOL (ErhErrno() == 0);
}


static BOOL PutMethodFile (
    IN const tchar *integtype,
    IN const tchar *methodfile,
    IN const tchar *method,
    IN const tchar *inputfile )
{
    ERH_FUNCTION(_T("PutMethodFile"));
    tchar *buffer;

    DbgFcnIn();

    if (StrIsEmpty(integtype) || StrIsEmpty(methodfile))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL (FALSE);
    }

    if (StrIsEmpty(method))
    {
        buffer = Local_FileRead (inputfile);
        if (!buffer) 
            RETURN_INT (FALSE);
    }
    else
    {
        buffer = StrFormat(_T("BACKUP_METHOD='%s';\n"), NS(method));
    }

    CellServer_FileWrite (integtype, methodfile, buffer);
    FREE (buffer);
    RETURN_BOOL (ErhErrno() == 0);
}


/*========================================================================*//**
*
* @brief     Gets/puts file <integtype>/<hostname>%%<instance> w/o file format check
*
*//*=========================================================================*/
static int GetFile (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *outputfile )
{
    ERH_FUNCTION(_T("GetFile"));
    tchar *buffer;
    DbgFcnIn();

    if (StrIsEmpty(integtype) || StrIsEmpty(hostname) || StrIsEmpty(instance))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL  (FALSE);
    }

    buffer = CellServer_FileRead (integtype, GetIntegConfigname(hostname, instance));
    if (buffer)
        Local_FileWrite (outputfile, buffer);
    
    FREE(buffer);

    RETURN_BOOL (ErhErrno() == 0);
}


static int PutFile (
    IN const tchar *integtype,
    IN const tchar *hostname,
    IN const tchar *instance,
    IN const tchar *inputfile )
{
    ERH_FUNCTION(_T("PutFile"));
    tchar *buffer = NULL;

    DbgFcnIn();
    if (StrIsEmpty(integtype) || StrIsEmpty(hostname) || StrIsEmpty(instance))
    {
        ErhMarkMajor (ERH_BAD_CMDLINE);
        RETURN_BOOL  (FALSE);
    }

    buffer = Local_FileRead (inputfile);
    if (buffer)
    {
        CellServer_FileWrite (integtype, GetIntegConfigname(hostname,instance), buffer);
    }
    FREE(buffer);
    RETURN_BOOL (ErhErrno() == 0);
}




/* ===========================================================================
| @brief    upload barlist file <inputfile> to cs
 ========================================================================== */
static void 
PutBarList(IN const tchar *integtype,
           IN const tchar *barlist,
           IN const tchar *inputfile)
{
    ERH_FUNCTION(_T("PutBarList"));
    tchar *buffer = NULL;

    if (StrIsEmpty(integtype) || StrIsEmpty(barlist) || StrIsEmpty(inputfile))
    {
        ErhMarkMajor(ERH_BAD_CMDLINE);
        return;
    }

    buffer = Local_FileRead (inputfile);
    if (buffer)
    {
        const tchar *barlistName = GetDatalistName(integtype, barlist);
        csa_t csa = {0};

        if (BU_CsaInit(&csa, opt.cellserver) != BP_NOERROR)
        {
            ErhMarkMajor (ERH_INTERNAL);
            return;
        }

        if (MCsaPutCustomFile(csa.handle, CMN_PAN_CONFIG, barlistName, buffer, UNLOCK) != BP_NOERROR)
        {
            BU_CsaExit (&csa);
            ErhMarkMajor (ERH_INTERNAL);
            return;
        }

        BU_CsaExit (&csa);
        FREE(buffer);
    }
    return;
}


static BOOL ParseCatalogFile (const tchar *filename)
{
    size_t size = 0;
    tchar *t;
    tchar *buf = CmnGetFile(filename, &size);

    if (!buf)
        return FALSE;

    for (t=buf; t && t[0]; t += StrLen(t) + 1)
    {
        BU_Printf (_T("%s\n"), t);
    }
    FREE (buf);
    return TRUE;
}


/* ===========================================================================
| @note supported formats for -dpcom option
|
|1. -dpcom <host> <exe>
|   Be a proxy to dpcom server <exe> on <host>.
|
|2. -dpcom <host>:<port>
|   Be a dpcom server. Back-connect to client at <host>:<port>.
|   If <host> is empty, use localhost. Port must be numeric value.
|
|3. -dpcom :pipe[,options] or :stdio[,options] or :fifo:<name>[,options]
|   Be local dpcom server, communicate via stdio or over named pipe. Used by perl/python etc.
|
|4. -dpcom
|   Same as -dpcom :pipe
|
| Summary: -dpcom <addr>[,options], where addr is: <host>:<port> or :<port> or :pipe or :stdio or :fifo:<fifoName>
| If format [1] is given, initiate remote dpcom server exe@host and proxy to it
|
 =========================================================================== */
static int
CreateInstance(_Out_ DPCOM *com, _In_opt_ ctchar *hostname, _In_opt_ ctchar *arglist)
{
    if (!opt.remote && (StrIsEmpty(hostname) || IpcIPIsEqual(hostname, cmnNodename)))
    {
        return DPComCreateInstance(com, NULL);
    }
    else
    {
        return DPComCreate(com, hostname, StrIsEmpty(arglist)? UTIL_CMD_EXE : arglist, NULL);
    }
}


static int OB2_DPComProxy (ctchar *constr, ctchar *server, ctchar *arglist)
{
    ERH_FUNCTION(_T("OB2_DPComProxy"));
    DPCOM com = {0};
    OB2_STATUS status;

    DbgFcnInEx(__FLND__, _T("constr:%s, server:%s, arglist:%s"), NS(constr), NS(server), NS(arglist));

    if (!StrChr(constr, ':'))  // proxy mode
    {
        status = CreateInstance(&com, constr, arglist);
        constr = NULL;
    }
    else
    {
        status = CreateInstance(&com, server, arglist);
    }

    if (status != 0)
        RETURN_BOOL(FALSE);

    {
        struct DPCOM_SERVER *srv = DPComServerNew(constr);
        if (!srv)
            RETURN_BOOL(FALSE);

        DPComServerSetTarget(srv, &com);
        status = DPComServerRun(srv);
        DPComServerEnd(srv=CmnGetContext(DpComPtd));
    }

    RETURN_BOOL(status == 0)
}


static tchar agCurrentPath[STRLEN_PATH + 1] = {0};

static int 
AgnosticGetBaseInstallInfo(tchar *homedir, tchar *datadir, tchar *logdir, tchar *tmpdir)
{
    StrCopy (homedir, agCurrentPath, STRLEN_PATH);
    StrCopy (datadir, agCurrentPath, STRLEN_PATH);
    StrCopy (logdir,  agCurrentPath, STRLEN_PATH);
    StrCopy (tmpdir,  agCurrentPath, STRLEN_PATH);

    return 0;
}

static int
DpAgnostic(int *pargc, tchar *argv[])
{
    int i = 0;

    for (i = *pargc - 1; i > 0; i-- )
    {
        if (0 == StrICmp(argv[i], _T("-agnostic")))
            break;
    }

    if (i > 0)
    {
        /* Path fixup */
        size_t nPath = 0;

        StrCopy(agCurrentPath, argv[0], STRLEN_PATH);
        PathCutLeaf(agCurrentPath);
        nPath = StrLen(agCurrentPath);

        cmnInitializationInfo.cmnGetBaseInstallInfo = AgnosticGetBaseInstallInfo;
        CmnDefine(DP_AGNOSTIC);

        StrCopy(cmnPanBin, agCurrentPath, STRLEN_PATH);
        cmnPanBin[nPath] = GEN_SLASH;

        SetConType(CON_UTF8);

        /* Version fixup */
        if (i != *pargc - 1)
        {
            StrCopy(productID.version, argv[i+1], STRLEN_STD);
        }

        /* Open catalog */
        NlsOpenCatalog();

        /* Hide additional params from cli parsers */
        *pargc = i;
    }

    return 0;
}


/* ===========================================================================
*
* @brief    Set file attributes/permissions received as part of
*           MSG_METADATA message.
*
*//*=========================================================================*/
#if !TARGET_WIN32
#   include <pwd.h>
#   include <grp.h>
#endif

static int RcpSetFileAttr(ctchar *file, const Variant *attr)
{
    ERH_FUNCTION(_T("RcpSetFileAttr"));
    int      r = 0;
    unsigned mode  = VarGetInt(attr, _T("mode"));
    unsigned posix = VarGetInt(attr, _T("posix"));
    unsigned uid   = VarGetInt(attr, _T("uid"));
    unsigned gid   = VarGetInt(attr, _T("gid"));
    ctchar  *user  = VarGetText(attr, _T("user"));
    ctchar  *group = VarGetText(attr, _T("group"));

    DbgFcnInEx(__FLND__, _T("posix:%d, mode:%x, uid:%u, gid:%u, user:%s, group:%s"), posix, mode, uid, gid, user, group);

    #if TARGET_WIN32
    {
        if (posix)
            RETURN_INT(0);

        if (!SetFileAttributes(file, mode))
        {
            OSLOGW(SetFileAttributes, _T("%s"), file);
            RETURN_INT(-1);
        }
    }
    #else
    {
        struct passwd *pw = NULL;
        struct group  *gr = NULL;
        if (!posix)
            RETURN_INT(0);

        if (chmod(file, mode) != 0)
        {
            OSLOGW(chmod, "%s", file);
            r = -1;
        }

        if (user  && !(pw = getpwnam(user)))  OSLOGW(getpwnam, "user:%s", user);
        if (group && !(gr = getgrnam(group))) OSLOGW(getgrnam, "group:%s", group);

        if (pw && pw->pw_uid != uid) 
        {
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] override uid. user:%s, clientUid:%u, serverUid:%u"), __FCN__, user, uid, pw->pw_uid);
            uid = pw->pw_uid;
        }

        if (gr && gr->gr_gid != gid)
        {
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] override gid. group:%s, clientGid:%u, serverGid:%u"), __FCN__, group, gid, gr->gr_gid);
            gid = gr->gr_gid;
        }

        if (chown(file, uid, gid) != 0)
        {
            OSLOGW(chown, "uid:%d, gid:%d, path:%s", uid, gid, file);
            r = -1;
        }
    }
    #endif

    RETURN_INT(r);
}

/*========================================================================*//**
* @brief     Read file attributes/permissions and return Variant packed string
*            e.g.: uid=123;guid=234;mode=777;posix=1
*            "posix" is set to 1 if file has posix attributes/permissions
*//*=========================================================================*/
#if !TARGET_WIN32
#   include <pwd.h>
#   include <grp.h>
#endif

static Variant RcpGetFileAttr(ctchar *file)
{
    ERH_FUNCTION(_T("RcpGetFileAttr"));
    int r = 0;
    Variant metaData = VarUndef;
    OB2_Stat stat;

    DbgFcnIn();

    // Read file attributes and permissions
    r = OB2_StatFile(file, &stat);
    if (r != 0)
    {
        ErhMarkSys(ERH_SYSERR, GetLastError(), _T("OB2_StatFile"), ERH_MAJOR);
        RETURN(VarUndef);
    }

    if (stat.type != OBJ_FILE)
    {
        ErhMark(ERH_UNSUPP, __FCN__, ERH_MAJOR); // only files supported
        RETURN(VarUndef);
    }

    VarSet(&metaData, _T("mode"), I2V(stat.mode));

    #if !TARGET_WIN32
    {
        struct group  *gr;
        struct passwd *pw;

        VarSet(&metaData, _T("uid"), I2V(stat.uid));
        VarSet(&metaData, _T("gid"), I2V(stat.gid));

        pw = getpwuid(stat.uid);
        if (!pw) OSLOGW(getpwuid, "uid:%d", stat.uid);

        gr = getgrgid(stat.gid);
        if (!gr) OSLOGW(getgrgid, "uid:%d", stat.gid);

        VarSet(&metaData, _T("posix"), I2V(1));
        if (pw && pw->pw_name) VarSetText(&metaData, "user",  pw->pw_name);
        if (gr && gr->gr_name) VarSetText(&metaData, "group", gr->gr_name);
    }
    #endif

    RETURN(metaData);
}


/*========================================================================*//**
* @brief     Verify if RcpServer host belong to the same cell as RcpClient host
*            (i.e.: this host) and that both have oracle8 installed.
*
* @retval    TRUE      Target and source host belongs to same Cell Manager and 
*                      have oracle8 component installed
*//*=========================================================================*/
static BOOL
RcpVerifyHosts(ctchar *targetHost)
{
    ERH_FUNCTION (_T("RcpVerifyHosts"));
    int handle;
    int retVal = FALSE;
    tchar  omni_info[STRLEN_PATH+1];
    tchar *bufCellInfo  = NULL;
    tchar *bufOmniInfo = NULL;
    THostList *hostlist = NULL;
    THostList *host;
    ParserList *pl = NULL;

    DbgFcnIn();

    /* Parse local omni_info configuration file */
    PathConcat(omni_info, cmnPanConfigClient, _T("omni_info"), STRLEN_PATH);
    bufOmniInfo = CmnGetAsciiFile(omni_info);
    if (!bufOmniInfo)
        goto end;

    pl = PLGetListFromBuffer(defOmniInfo, bufOmniInfo);
    if (!pl)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] omni_info, parsing failed."), __FCN__);
        goto end;
    }

    if (!PLFindLine(pl, PLCIK_ORACLE8))
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] sourceHost:%s, oracle8 not installed."), __FCN__, cmnNodename);
        goto end;
    }

    /* Get my cell_info and verify targetHost (RcpServer) exist in it */
    handle = MCsaBindServer(cmnCSHostname);
    if (handle < 0)
        goto end;

    if (MCsaGetSingleFile(handle, S_CELL, &bufCellInfo, READ_ONLY) < 0)
        goto end;

    /* start parsing the file */
    hostlist = CellPparseHosts(bufCellInfo);
    if (!hostlist)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] cell_info, parsing failed."), __FCN__);
        goto end;
    }

    /* find host info from list */
    host = CellPfindHost(hostlist, targetHost);
    if (!host)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] targetHost:%s, not in cell."), __FCN__, targetHost);
        goto end;
    }

    /* verify that oracle is installed */
    if (!host->Oracle8)
    {
        DbgPlain(DBG_UNEXPECTED, _T("[%s] targetHost:%s, oracle8 not installed."), __FCN__, targetHost);
        goto end;
    }

    retVal = TRUE;
end:
    PLFree(pl);
    CellPdeleteHostList(hostlist);
    FREE(bufCellInfo);
    FREE(bufOmniInfo);
    RETURN_BOOL(retVal);
}


/*========================================================================*//**
* @brief     Verify if rcp transfer is allowed
*//*=========================================================================*/
static BOOL
RcpIsAllowed(ctchar *file, ctchar *targetHost)
{
    ERH_FUNCTION(_T("RcpIsAllowed"));
    BOOL   ok;
    tchar *fileL;

    DbgFcnIn();

    fileL = StrNewCopy(file);
    StrToLower(fileL);
    ok = StrEndsWith(fileL, _T("_bckp.ora")) || StrEndsWith(fileL, _T(".ctl"));

    if (ok) ok = RcpVerifyHosts(targetHost);

    FREE(fileL);
    RETURN_BOOL(ok);
}


#if HAVE_RCP
/*========================================================================*//**
*
* @brief     Connect to RCP server and transfer file to it
*            Opt parameter contains arguments needed for establishing transfers, e.g.:
*            server=foo.hermes.si;port=12323234;path=/opt/oracle/foobar.dat
*
*//*=========================================================================*/
static int RcpClient(ctchar *opt)
{
    ERH_FUNCTION(_T("RcpClient"));
    Variant args   = VarUnpackit(opt, VAR_PACK_NOTOP|VAR_PACK_TIGHT);
    ctchar *server = VarGetText(&args, _T("server"));
    ctchar *path   = VarGetText(&args, _T("path"));
    int     port   = VarGetInt(&args,  _T("port"));
    int     retval = -1;

    IpcHandle   h;
    FILEHANDLE  fh = INVALID_HANDLE_VALUE;
    StrMsg      msg = {0};
    char        buf[MAX_SM_PACKETSIZE]; // @todo Check optimal size of buffer
    Variant     metaData;

    DbgFcnIn();

    // connect to server:port
    h = IpcConnectToProcess(server, port, RCP_TIMEOUT);  // @todo: Do we need use timeout also for some other (send/receive functions)
    if (h == -1)
    {
        if (ErhErrno() != 0)
            ErhMarkMajor(ERH_INTERNAL);
        goto end;
    }

    if (!RcpIsAllowed(path, server))
    {
        ErhMarkMajor(ERH_DENIED);
        goto end;
    }

    // Reads file attr/permissions and send them to server
    metaData = RcpGetFileAttr(path);
    if (!VarDefined(&metaData))
        goto end;

    StrMsgStartL(&msg, NULL, 0, MSG_METADATA);
    StrMsgAppendVariantL(&msg, &metaData);
    VarFree(&metaData);

    if (IpcSendStrMsgL(h, &msg) < 0)
        goto end;

    fh = OB2_OpenFile(path, 0, OB2_OPENFILE_EXISTING); // @todo windows: phase 2: BackupRead/BackupWrite
    if (fh == INVALID_HANDLE_VALUE)
    {
        ErhMarkSys(ERH_INTERNAL, GetLastError(), __FCN__, ERH_MAJOR);
        goto end;
    }

    while_forever()
    {
        int last;

        // read MAX_SM_PACKETSIZE bytes, not tchars, as we may be unicode build and other side may not
        int bytes = OB2_ReadFile(fh, buf, MAX_SM_PACKETSIZE);
        if (bytes == -1)
        {
            ErhMarkSys(ERH_INTERNAL, GetLastError(), __FCN__, ERH_MAJOR);
            goto end;
        }

        if (bytes == 0)
        {
            retval = 0; // end of file
            goto end;
        }
        
        last = bytes < MAX_SM_PACKETSIZE;

        StrMsgStartL(&msg, NULL, 0, MSG_DATA);
        StrMsgFAppendL(&msg, _T("%d%d"), last, bytes);

        if (IpcSendStrMsgL(h, &msg) < 0)
            goto end;

        if (IpcSendData (h, buf, bytes) < 0)
            goto end;
    }

end:
    if (retval == 0) ErhClearError();

    if (h != INVALID_IPC_HANDLE)
    {
        StrMsgStartL(&msg, NULL, 0, MSG_RESULT);
        ErhPackMsgL(&msg);
        IpcSendStrMsgL(h, &msg);
    }

    OB2_CloseFile(fh);
    IpcCloseConnection(h);

    VarFree(&args);
    StrMsgFreeL(&msg);
    RETURN_INT(retval);
}

/*========================================================================*//**
*
* @brief    Listen for RCP client connection, initiate RCP client startup on
*           remote host, and accept file transfer from it.
*
* @param    opt
*           Variant formated string with remote host name, remote file location
*           and local (target) file location, e.g.:
*           server=foo.hermes.si;port=12323234;path=/opt/oracle/foobar.dat
*
*//*=========================================================================*/
static int RcpServer(ctchar *opt)
{
    ERH_FUNCTION(_T("RcpServer"));
    Variant     serverOpts = VarUnpackit(opt, VAR_PACK_NOTOP|VAR_PACK_TIGHT);
    ctchar     *sourceHost = VarGetText(&serverOpts, _T("sourceHost"));
    ctchar     *sourcePath = VarGetText(&serverOpts, _T("sourcePath"));
    ctchar     *targetPath = VarGetText(&serverOpts, _T("targetPath"));
    int         overwrite  = VarGetInt(&serverOpts, _T("overwrite"));
    int         retval = -1;
    int         serverPort, event, msgno;
    BOOL        changed = FALSE;
    FILEHANDLE  fh = INVALID_HANDLE_VALUE;
    IpcHandle   h  = INVALID_IPC_HANDLE;

    int         bufsize = MAX_SM_PACKETSIZE;
    tchar      *buf = MALLOC(bufsize); // bytes
    tchar      *packed = NULL;
    Variant     opts;
    Variant     metaData;
    INET_EXEC   inet = {0};

    DbgFcnIn();

    // startup: we open listen port
    if (IpcAcceptConnections(IPC_ANY_PORT) == -1)
        goto end;

    serverPort = IpcGetListenPort();

    opts   = VarValidList(_T("server"), T2V(cmnNodename), _T("path"), T2V(sourcePath), _T("port"), I2V(serverPort), NULL);
    packed = VarPackit(&opts, VAR_PACK_NOTOP|VAR_PACK_TIGHT);
    VarFree(&opts);

    // Start util_cmd with -rcp_client server=<ourhost>;port=<ourport>;path=<srcpath> option
    inet.rexec.forget = 1;
    inet.execID = EXEC_INTEGUTIL;
    if (CsaInetExecle(&inet, sourceHost, UTIL_CMD_EXE, _T("-rcp_client"), packed , NULL) == -1)
        goto end;

    // Wait for rcp client connection
    h = IpcWaitForIpcEvent(RCP_TIMEOUT);

    event = IpcGetLastIpcEvent();
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] IpcGetLastIpcEvent(%d)=%d (%s)"), __FCN__, h, event, IpcGetEventName(event));

    if (INVALID_IPC_HANDLE == h || IPC_CONNECT_EVENT != event)
    {
        ErhMark(EIPCCLOSED, __FCN__, ERH_MAJOR);
        goto end;
    }

    fh = OB2_OpenFile(targetPath, OB2_OPENFILE_RDWR, overwrite? OB2_OPENFILE_CREATE_ALWAYS : OB2_OPENFILE_CREATE_NEW);
    if (fh == INVALID_HANDLE_VALUE)
    {
        ErhMarkSys(ERH_SYSERR, GetLastError(), __FCN__, ERH_MAJOR);
        goto end;
    }

    /* ---------------------------------------------------------------------------
    |   Get chunks of transfered file as part OF MSG_DATA messages
     -------------------------------------------------------------------------- */
    while_forever()
    {
        int last = 0;
        int size = IpcReceiveData(h, buf, bufsize);
        if (size < 0)
            goto end;

        msgno = StrParseStart(buf);
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] msgno:%d"), __FCN__, msgno);

        switch (msgno)
        {
        case MSG_METADATA:
            metaData = VarUnpack(StrParseNext());
            if (RcpSetFileAttr(targetPath, &metaData) != 0)
            {
                ErhFullReport(__FCN__, ERH_WARNING,
                    NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_OSERROR, targetPath, ErhSysErrnoToText(GetLastError())));
            }
            VarFree(&metaData);

            continue;
        
        case MSG_RESULT:
            ErhUnpackMsg();
            retval = ErhErrno() == 0? 0 : -1;
            goto end;

        case MSG_DATA:
            break;

        default:
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Invalid message:%d (%s)"), __FCN__, msgno, CmnMsgIdToDbg(msgno));
            ErhMark(ERH_MPROTO, __FCN__, ERH_MAJOR);
            goto end;
        }

        // Parse initial msg with size of data package to follow
        if (StrParseMessageF(_T("%d%d"), &last, &size) == -1 || size <= 0)
        {
            DbgPlain(DBG_UNEXPECTED, _T("[%s] Bad MSG_DATA format"), __FCN__);
            ErhMark(ERH_MPROTO, __FCN__, ERH_MAJOR);
            goto end;
        }
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] size:%d"), __FCN__, size);

        // Receive data
        if (size > bufsize)
            buf = REALLOC(buf, bufsize = size*2);

        if (IpcReceiveData(h, buf, size) != size)
            goto end;

        if (OB2_WriteFile(fh, buf, size) != size)
        {
            ErhMarkSys(ERH_SYSERR, GetLastError(), __FCN__, ERH_MAJOR);
            goto end;
        }

        changed = TRUE;
    }

end:
    IpcCloseConnection(h);

    if ( retval == -1 &&
        (changed || (fh != INVALID_HANDLE_VALUE && OS_GetFileSizeByHandle(fh) == 0)) )
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Transfer failed, remove file. Path:%s"), __FCN__, targetPath);
        OB2_CloseFile(fh);
        OB2_DeleteFile(targetPath);
    }
    else
    {
        OB2_CloseFile(fh);
    }

    VarFree(&serverOpts);
    FREE(packed);
    FREE(buf);
    RETURN_INT(retval);
}
#endif


int main (int argc, tchar *argv[], tchar **envp)
{
    ERH_FUNCTION(_T("main"));

    int retval = 0;
    CmnSetProgname(_T("UTIL_CMD"));

    OB2_DebugOptsFromEnv (&opt.debug);
    OB2_DebugOptsFromArg (&opt.debug, argc, argv);
    OB2_DebugOptsToEnv   (&opt.debug);

    DPComSetExports (ExportStubs);

    DpAgnostic(&argc, argv);

    if (CmnInitd(_T("util_cmd"), &opt.debug) < 0)
    {
        ErhMarkMajor (ERH_OBE_INTERNAL);
        goto at_exit;
    }

    OB2_DumpArgs (DBG_MAIN_ACTION, argc, argv, envp);

    retval = OptParseOptions(argc-1, argv+1);

    switch (retval)
    {
    case -1:
    case -2:
        ErhMarkMajor (ERH_BAD_CMDLINE);
        goto at_exit;

    case -3:
        /* handled by optmgr: help, version */
        CmnExit();
        CmnExitProcess (RESULT_SUCCESS);
        break;

    default:
        break;
    }

    IpcInit();

    if (StrIsEmpty(opt.cellserver))
    {
        opt.cellserver = cmnCSHostname;
        if (StrIsEmpty(opt.cellserver))
        {
            DbgStamp(DBG_UNEXPECTED);
            goto at_exit;
        }
    }

    CsaSetEmbedded(1);
    
    OB2_SetHostname (GetEnv(_T("OB2BARHOSTNAME")));

    retval = MCsaInit (opt.cellserver);
    if (retval < 0)
    {
        goto at_exit;
    }

    /* Check user */
    retval = BU_CheckUser(
        opt.cellserver,
        opt.action==OPT_ECHOD || opt.action==OPT_ECHOE ? USER_CAN_SEE_PRIVATE_OBJ : USER_EXISTS
    );
    if (retval != OBE_OK)
    {
        goto at_exit;
    }

    if (StrIsEmpty(opt.hostname))
    {
        opt.hostname = cmnHostname;
    }
    opt.hostname = StrNullCopy(OB2_ExpandHostname(opt.hostname));

    switch (opt.action)
    {
    case OPT_GETCONFIG:
        GetConfig (opt.integtype, opt.hostname, opt.instance, opt.localfile );
        break;
    
    case OPT_PUTCONFIG:
        PutConfig (opt.integtype, opt.hostname, opt.instance, opt.localfile );
        break;
        
    case OPT_GETOPTION:
        GetOption (opt.integtype, opt.hostname, opt.instance, opt.localfile,
            opt.sublist, opt.optionname );
        break;

     case OPT_PUTOPTION:
        PutOption (opt.integtype, opt.hostname, opt.instance, opt.localfile,
            opt.sublist, opt.optionname, opt.optionvalue );
        break;

    case OPT_GETFILE:
        GetFile (opt.integtype, cmnHostname, opt.instance, opt.localfile );
        break;

    case OPT_PUTFILE:
        PutFile (opt.integtype, cmnHostname, opt.instance, opt.localfile );
        break;

    case OPT_GETMETHOD:
        GetMethodFile (opt.integtype, opt.filename, opt.localfile );
        break;

    case OPT_PUTMETHOD:
        PutMethodFile (opt.integtype, opt.filename, opt.method, opt.localfile );
        break;

    case OPT_GETOBJOFSESS:
        GetObjectsOfSession(opt.session);
        break;

    case OPT_LISTREMOTEDIR:
        ListRemoteDir (opt.hostname, opt.mountpoint, opt.directory, opt.query_columns);
        break;

    case OPT_QUERY:
        Query (opt.query_id, &opt.args, opt.query_columns, opt.localfile);
        break;
 
    case OPT_ECHOE:
        EncodeString(opt.plaintext);
        break;

    case OPT_ECHOD:
        DecodeString(opt.crypttext);
        break;
        
    case OPT_CATALOGFILE:
        ParseCatalogFile(opt.filename);
        break;

    case OPT_REXEC:
        CsaInetExec(&opt.exec, opt.hostname, &opt.args);
        break;

    case OPT_DPCOM:
        OB2_DPComProxy (opt.dpcom_constr, opt.dpcom_server, opt.arglist);
        break;

    case OPT_PUTBARLIST:
        PutBarList (opt.integtype, opt.filename, opt.localfile);
        break;

    case OPT_GETOMNIRC:
        {
            tchar *val = OB2_OmnircGet(opt.hostname, opt.omnirc.name);
            if (val) ConPrintf(_T("%s\n"), val);
        }
        break;

    case OPT_SETOMNIRC:
        OB2_OmnircSet(opt.hostname, opt.omnirc.name, opt.omnirc.value);
        break;

    #if HAVE_RCP
    case OPT_RCP:
        RcpServer(opt.rcp);
        break;

    case OPT_RCP_CLIENT:
        RcpClient(opt.rcp_client);
        break;
    #endif

    case OPT_GETSTAT:
        {
            Variant v = VarUndef;
            MCsaStat(0, 0, &v);
            if (VarDefined(&v)) ConPrintf(_T("%s\n"), VarPackit(&v, VAR_PACK_JSON));
        }
        break;

    default:
        ErhMarkMajor (ERH_BAD_CMDLINE);
        break;
    }

at_exit:
    retval = ErhErrno();
    BU_Fprintf (stdout, _T("*RETVAL*%d\n"), retval);
    if (retval != 0)
    {
        if (!StrIsEmpty(ErhSysErrorStr()))
            BU_Fprintf (stdout, _T("%s\n"), ErhSysErrorStr());

        if (!StrIsEmpty(ErhGetErrorInfo()))
            BU_Fprintf(stdout, _T("%s\n"), ErhGetErrorInfo());
    }

    CsaExit();
    IpcExit();
    CmnExit();

    CmnExitProcess (retval==0? RESULT_SUCCESS : RESULT_FAILURE);
}




/* ===========================================================================
| @section  DPCOM stubs
|
| @todo     Move to separate file
 ========================================================================== */
#define k_host       _T("host")
#define k_user       _T("user")
#define k_group      _T("group")
#define k_type       _T("type")
#define k_name       _T("name")
#define k_instance   _T("instance")
#define k_cellserver _T("cellserver")
#define k_userclass  _T("userclass")
#define k_dir        _T("dir")
#define k_json       _T("json")

#define DPCOM_STRING(_name)                             \
    if (TypeOf(&(_name))!=var_string) {                 \
        ErhMark(ERH_OBE_INVARG, _T(#_name), ERH_MAJOR); \
        return VarUndef;                                \
    }


#define DPCOM_GETSTRING(_opt, _name) {                  \
    Variant *_elem = VarGet(&(_opt), k_##_name);        \
    if (TypeOf(_elem) != var_string) {                  \
        ErhMark(ERH_OBE_INVARG, _T(#_name), ERH_MAJOR); \
        return VarUndef;                                \
    }                                                   \
    _name = V2T(_elem);                                 \
}

#if TARGET_SCO || TARGET_MACOSX
#   define DISABLE_DPCOM 1
#endif

#if !DISABLE_DPCOM

DPCOM_EXPORT(OB2_EncodePassword, (Variant plaintext))
{
    DPCOM_STRING (plaintext);
    return CT2V(EncodePassword (V2T(&plaintext)));
}


DPCOM_EXPORT(OB2_DecodePassword, (Variant crypt))
{
    DPCOM_STRING (crypt);
    return CT2V(DecodePassword (V2T(&crypt)));
}


DPCOM_EXPORT(OB2_ExpandHostname, (Variant shortName))
{
    DPCOM_STRING (shortName);
    return T2V(CmnExpandHostname (V2T(&shortName)));
}


DPCOM_EXPORT(OB2_CellServer, (void))
{
    return T2V(OB2_CellServer());
}


DPCOM_EXPORT(OB2_NlsGetCatalogString, (Variant msgSet, Variant msgNo))
{
    tchar *text = NlsGetCatalogString(V2I(&msgSet), V2I(&msgNo));
    return T2V(text);
}


static int RemoteScriptRead (void *userptr, const tchar *line)
{
    Variant *array = userptr;
    DbgPlain(DBG_MAIN_ACTION, _T("line> %s"), line);
    if (array)
    {
        VarPush (array, T2V(line));
        return 0;
    }
    
    return DPComSendInfo(CT2V(line));
}


// @section     connect to (BAR|RBAR) session
typedef struct {
    tchar barlist[STRLEN_PATH+1];
    int   stype;
} SM;


static taskreturn_t TASKAPI
SessionMonitorThread(void *arg)
{
    ERH_FUNCTION(_T("SessionMonitorThread"));
    StrMsg msg = {0};
    SM    *sm = arg;
    int    handle;
    int    r;
    BOOL   abort = FALSE;
    dpint64_t scriptPid = StrToInt64(GetEnv(_T("OB2_DPCOM_CLIENT_PID")));

    if (CmnInitd(_T("util_cmd"), &opt.debug) < 0)
        goto done;

    if (MCsaInit(opt.cellserver) != 0)
        goto done;

    StrMsgMakeL(&msg, NULL, 0, MSG_OPTIONS, _T("-bar"), sm->barlist, NULL);

    handle = CsaStartSession(sm->stype, StrMsgGetBufL(&msg));
    if (handle == -1)
        goto done;

    StrMsgMakeL(&msg, NULL, 0, MSG_MONITOR, StrFromInt(MON_STATUSONCE), cmnEun, cmnEgn, cmnHostname, NULL);
    if (IpcSendStrMsgL(handle, &msg) == -1)
        goto done;

    do
    {
        int h = IpcWaitForIpcEvent(-1);
        if (h != handle)
            goto done;

        if (IpcReceiveStrMsgL(h, &msg) <= 0)
            goto done;

        StrParseMessageToDebugsL(&msg, NULL, DBG_MAIN_ACTION, _T("IN"), _T("\t|"), TRUE);
        r = StrParseStartL(&msg, NULL);
    } while (r != MSG_ABORT && r != MSG_STOP);

    // send signal to script; repeat until script is dead
    abort = r == MSG_ABORT && StrToInt(StrParseNextL(&msg)) == EAB_USERABORT;
    while (abort)
    {
        OSPROC_INFO proc = {0};

        #if TARGET_WIN32
            SetConsoleCtrlHandler(NULL, TRUE); 
            if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)) OSLOGW(GenerateConsoleCtrlEvent, _T("CTRL_C_EVENT"));
        #else
            if (scriptPid > 0 && kill(scriptPid, SIGINT) == -1) OSLOGW(kill, "pid:%lld", scriptPid);
        #endif

        CmnSleep(200);
        abort = CmnGetProcInfo(scriptPid, OSPROC_FTIME, &proc) == 0;
    }

done:
    CsaExitEx(CSA_EXIT_SM);
    CmnExit();
    return -1;
}


DPCOM_EXPORT(StartSessionMonitor, (Variant args))
{
    static BOOL connected = FALSE;
    static SM sm = {0};
    Task    task = {0};
    int     stype   = VarGetInt (&args, _T("stype"));
    ctchar *barlist = VarGetText(&args, _T("barlist"));

    if (connected)
        return I2V(TRUE);

    if ((stype != BAR && stype != RBAR) || StrIsEmpty(barlist))
    {
        ErhMark (ERH_MFORMAT, _T("StartSessionMonitor"), ERH_WARNING);
        return I2V(FALSE);
    }

    StrCopy(sm.barlist, barlist, STRLEN_PATH);
    sm.stype = stype;

    return I2V(TaskRun(&task, SessionMonitorThread, &sm) == TASK_OK);
}


DPCOM_EXPORT(CmnRunScript, (Variant args, Variant options))
{
    CmnRunStruct run = {0};
    argvRec a = {0};
    int i, r;
    Variant  output = VarArray(VarUndef);
    Variant *env = VarGet(&options, _T("env"));
    Variant  v = VarUndef;
    Iterator it;
    BOOL monitor = VarGetInt(&options, _T("monitor"));

    VarIterate(env, it)
        CmnEnvSet(&run.env, IterKey(&it), V2T(IterVal(&it)));

    run.dir     = VarGetText(&options, _T("dir"));
    run.execDir = VarGetText(&options, _T("execDir"));
    run.input   = VarGetText(&options, _T("input"));
    run.timeout = VarGetInt (&options, _T("timeout"));
    run.flags   = VarGetInt (&options, _T("flags"));
    run.user    = VarGetText(&options, _T("user"));
    run.group   = VarGetText(&options, _T("group"));
    run.password= VarGetText(&options, _T("password"));
    run.lang    = VarGetText(&options, _T("lang"));

    run.readlineCtx = monitor? NULL : &output;
    run.readlineHook= RemoteScriptRead;

    for (i = 0; i < VarLength(&args); ++i)
        ArgvAdd(&a, VarTextAt(&args, i));

    r = CmnRunScript(a.argc, a.argv, &run);

    ArgvFree(&a);

    CmnEnvFree(&run.env);

    VarSet(&v, _T("status"),   I2V(r));
    VarSet(&v, _T("exitcode"), I2V(run.exitcode));
    VarSet(&v, _T("pid"),      I64V(run.pid));

    if (VarLength(&output))
        VarSet(&v, _T("output"), output);
    
    return v;
}

DPCOM_EXPORT(OB2_RunScript, (Variant options))
{
    CmnRunStruct params = {0};
    argvRec      args   = {0};
    Variant      output = VarArray(VarUndef);
    Variant     *vargs;
    Iterator     i;

    tchar *host = VarGetText(&options, k_host);
    if (!host) host = cmnHostname;

    params.user  = VarGetText(&options, k_user);
    params.group = VarGetText(&options, k_group);

    vargs = VarGet(&options, _T("args"));
    i = IterNew(vargs);
    while (IterNext(&i))
        ArgvAdd(&args, V2T(IterVal(&i)));

    params.readlineCtx  = &output;
    params.readlineHook = RemoteScriptRead;

    OB2_RunRemoteScript (host, args, &params);
    ArgvFree (&args);

    return output;
}


static NONREENTRANT ctchar*
OB2_GetConfigName(const Variant *options)
{
    ERH_FUNCTION(_T("OB2_GetConfigName"));
    THREADLOCAL static tchar out[STRLEN_PATH+1];
    tchar namebuf[STRLEN_PATH+1];

    ctchar *type     = VarGetText(options, k_type);
    ctchar *host     = VarGetText(options, k_host);
    ctchar *instance = VarGetText(options, k_instance);
    ctchar *name     = VarGetText(options, k_name);

    DbgFcnIn();

    if (StrIsEmpty(type))
        RETURN_STR(NULL);
    
    if (StrIsEmpty(name))
    {
        if (StrIsEmpty(instance))
            RETURN_STR(NULL);

        host = OB2_ExpandHostname(StrIsEmpty(host)? cmnHostname : host);
        PathFormat(namebuf, _T("%s%%%s"), OB2_ExpandHostname(host), instance);
        name = namebuf;
    }

    PathFormat(out, _T("config/%s/%s"), type, name);
    RETURN_STR(out);
}


DPCOM_EXPORT(OB2_GetConfig, (Variant options))
{
    Variant   y        = VarUndef;
    tchar    *contents = NULL;
    IpcHandle crs;
    int       status;

    const tchar *cell = VarGetText(&options, k_cellserver);

    ctchar *name = OB2_GetConfigName(&options);

    if (StrIsEmpty(name))
    {
        ErhMark (ERH_MFORMAT, _T("GetConfig"), ERH_WARNING);
        return VarUndef;
    }

    crs = MCsaBindServer(cell? cell : OB2_CellServer());
    if (crs < 0)
    {
        return VarUndef;
    }

    status = MCsaGetGroupItem(crs, G_TOP_INTEG_CNF, name, &contents, MODIFY);
    if (status < 0)
        return VarUndef;

    y = VarUnpackit(contents, VAR_PACK_NOTOP);
    FREE (contents);

    return y;
}



DPCOM_EXPORT(OB2_PutConfig, (Variant options, Variant config))
{
    IpcHandle crs;
    Variant  *c;

    ctchar *cell = VarGetText(&options, k_cellserver);

    ctchar *name = OB2_GetConfigName(&options);

    c = VarDefined(&config)? &config : VarGet(&options, _T("config"));

    if (StrIsEmpty(name) || (TypeOf(c) != var_list))
    {
        ErhMark (ERH_MFORMAT, _T("GetConfig"), ERH_WARNING);
        return VarUndef;
    }

    crs = MCsaBindServer(cell? cell : OB2_CellServer());
    if (crs < 0)
        return VarUndef;
    
    {
        tchar *contents = VarPackit(c, VAR_PACK_NOTOP);
        int result = MCsaPutGroupItem(crs, G_TOP_INTEG_CNF, name, contents, UNLOCK);
        FREE (contents);
        return result == 0? I2V(1) : VarUndef;
    }
}


DPCOM_EXPORT(OB2_UserAdd, (Variant options))
{
    int cellbind, status;
    tchar *name, *group, *host, *userclass;
    
    tchar *cell = VarGetText(&options, k_cellserver);

    DPCOM_GETSTRING (options, name);
    DPCOM_GETSTRING (options, group);
    DPCOM_GETSTRING (options, host);
    DPCOM_GETSTRING (options, userclass);

    cellbind = MCsaBindServer(cell? cell : OB2_CellServer());
    if (cellbind<0)
        return VarUndef;

    status = MCsaUserAdd (cellbind, name, group, host, userclass);

    return I2V(status==0);
}


DPCOM_EXPORT(OB2_UserDel, (Variant options))
{
    int cellbind, status;
    tchar *name, *group, *host;

    tchar *cell = VarGetText(&options, k_cellserver);

    DPCOM_GETSTRING (options, name);
    DPCOM_GETSTRING (options, group);
    DPCOM_GETSTRING (options, host);

    cellbind = MCsaBindServer(cell? cell : OB2_CellServer());
    if (cellbind<0)
        return VarUndef;
    
    status = MCsaUserDel (cellbind, name, group, host);
    
    return I2V(status==0);
}


DPCOM_EXPORT(OB2_UserCommit, (Variant vcell))
{
    int    status;
    tchar *cell = V2T(&vcell);
    
    if (StrIsEmpty(cell))
    {
        status = MCsaUserCommit(-1);
    }
    else
    {
        int cellbind = MCsaBindServer(cell);
        if (cellbind<0)
            return VarUndef;
    
        status = MCsaUserCommit (cellbind);
    }

    return I2V(status==0);
}

/* this sounds awful; however, there can be only one query at once since there is only one */
/* DBSM per CSA context (thread) */    
static query_t query  = {0};


DPCOM_EXPORT(QueryInit, (Variant vcell))
{
    ctchar   *cell = V2T(&vcell);
    int       cellbind;
    IpcHandle dbsm;
    tchar     msgoptions[STRLEN_MESSAGE+1];
    
    cell = OB2_ExpandHostname(cell? cell : OB2_CellServer());
    cellbind = MCsaBindServer(cell);

    StrMsgMake (msgoptions, STRLEN_MESSAGE, MSG_OPTIONS, NULL);
    dbsm = MCsaStartSession (cellbind, DB, msgoptions);
    return I2V(dbsm);
}


DPCOM_EXPORT(Query, (Variant dbsm, Variant fun, Variant args, Variant opts))
{
    BOOL            ok;
    StrMsgStruct    msg = {0};
    Iterator        i;
    tchar          *id  = V2T(&fun);
    const query_format_t *fmt = query_get_description_by_name(id);
    BOOL            json = VarGetInt(&opts, k_json);

    if (!fmt || VarLength(&args) < fmt->numParams)
    {
        ErhMark (ERH_MFORMAT, _T("OB2_Query"), ERH_WARNING);
        return VarUndef;
    }
    
    StrMsgStartL (&msg, NULL, 0, fmt->funId);
    i = IterNew(&args);
    while (IterNext(&i))
        StrMsgAppendL (&msg, V2T(IterVal(&i)));

    ok = query_opent (&query, V2I(&dbsm), &msg);
    StrMsgFreeL (&msg);
    if (!ok)
        I2V(ok);
    
    if (VarGetInt(&opts, _T("all")))
    {
        Variant all = VarArray(VarUndef);
        int i;
        while (query_read (&query))
        {
            Variant rec = VarArray(VarUndef);
            for (i=0; i<query.length; ++i)
                VarPush (&rec, json ? VarUnpack(query.record[i]) : T2V(query.record[i]));
            VarPush(&all, rec);
        }
        return query.status != 0 && VarLength(&all) == 0? VarUndef : all;
    }

    return I2V(ok);
}


DPCOM_EXPORT(QueryFetch, (void))
{
    BOOL ok;
    int  i;
    Variant rec = VarArray(VarUndef);
    ok = query_read (&query);
    if (!ok && query_empty(&query))
    {
        query_close(&query);
        return VarUndef;
    }

    for (i=0; i<query.length; ++i)
        VarPush (&rec, T2V(query.record[i]));

    return rec;
}


DPCOM_EXPORT(GetInstallInfo, (void))
{
    /* Add additional Pan variables if needed */
    Variant vars = VarList(
        _T("PanBase"), T2V(cmnPanBase),
        _T("PanBin"), T2V(cmnPanBin),
        _T("PanTmp"), T2V(cmnPanTmp),
        NULL );

    return vars;
}


DPCOM_EXPORT(PutToFile, (Variant filename, Variant text))
{
    /* mode set explicitly to override restrictive umask if dpcom server
     * cannot be run under setuid - see QCCR2A72510. */
    return I2V(CmnVarPutFileEx(&text, V2T(&filename), 0644 | PUT_FILE_CHMOD) == 0);
}

#undef DeleteFile
DPCOM_EXPORT(DeleteFile, (Variant filename))
{ 
    return I2V(OB2_DeleteFile(V2T(&filename)) == 0);
}


// @note   needed by backup navigator
DPCOM_EXPORT(RunningSessions, (void))
{
    tchar *buf = NULL;
    tchar *a[10];
    Variant retval = VarArray(VarUndef);

    if (-1 == CsaGetSessionOverview(&buf))
    {
        return retval;
    }

    if (StrParseStart (buf) == -1)
    {
        FREE(buf);
        return retval;
    }

    while (-1 != StrParseMessage (7, a))
    {
        Variant rec = VarUndef;
        int i;
        for (i=0; i < 7; ++i) VarPush(&rec, T2V(a[i]));
        VarPush (&retval, rec);
    }
    FREE(buf);

    return retval;
}


DPCOM_EXPORT(FormatPackedMessage, (Variant vPackedMessage))
{
    Variant rec = VarArray(VarUndef);

    VarPush (&rec, T2V(NlsExtractFromSM(V2T(&vPackedMessage))));

    return  rec;
}

#define TypeIsInteg(type) (StrIsValid(type) && 0 != StrCmp(type, OB2_APP_OB2))

DPCOM_EXPORT(GetDatalist, (Variant opts))
{
    Variant  v = {0};
#if !TARGET_OPENVMS
    int      r;
    Worklist wl = {0};
    ctchar  *cell = VarGetText(&opts, k_cellserver);
    tchar   *type = VarGetText(&opts, k_type);
    ctchar  *name = GetDatalistName(type, VarGetText(&opts, k_name));
    tchar   *buf = NULL;

    int bind = MCsaBindServer(StrIsValid(cell)? cell : OB2_CellServer());

    if (bind == -1)
        return VarUndef;

    if (MCsaGetCustomFile(bind, CMN_PAN_CONFIG, name, &buf, READ_ONLY) != 0)
        return VarUndef;

    WlSetNoExpand(TRUE);
    WlSetNoGlobals(TRUE);
    WlInitWorklist(&wl);
    r = WlParse(&wl, buf, TypeIsInteg(type)? PS_INTEG : 0);

    FREE(buf);

    if (r != 0)
        return VarUndef;

    WlListJSON(&v, &wl, FALSE);
    WlFreeWorklist(&wl);
#endif
    return v;
}

DPCOM_EXPORT(GetDatalists, (Variant opts))
{
    Variant  out  = {0};
    int      r;
    ctchar  *cell = VarGetText(&opts, k_cellserver);
    tchar   *buf  = NULL;
    StrMsg   msg  = {0};
    MsgDatalist dl;

    int bind = MCsaBindServer(StrIsValid(cell)? cell : OB2_CellServer());

    if (bind == -1)
        return VarUndef;

    r = MCsaGetFilteredDatalists(bind, &buf, CSA_GET_DATALISTS_NO_SCHEDULE);
    if (r == -1)
        return VarUndef;

    StrParseStartL(&msg, buf);
    while (StrParseDatalist(&msg, &dl) != -1)
    {
        VarPush(&out, CmnVarValidList(
            _T("type"),     T2V(dl.type),
            _T("filename"), T2V(dl.filename),
            _T("mtime"),    T2V(dl.mtime),
            NULL)
        );
    }

    StrMsgFreeL(&msg);
    FREE(buf);

    return out;
}

DPCOM_EXPORT(PutDatalist, (Variant opts, Variant doc))
{
    #if TARGET_OPENVMS
    return I2V( 0 );
    
    #else
    int      r, parseErr;
    Worklist wl   = {0};
    tchar   *buf;
    ctchar  *type = VarGetText(&opts, k_type);
    ctchar  *base = VarGetText(&opts, k_name);
    ctchar  *cell = VarGetText(&opts, k_cellserver);
    ctchar  *dir  = VarGetText(&opts, k_dir);
    int      bind = MCsaBindServer(StrIsValid(cell)? cell : OB2_CellServer());

    if (bind == -1)
        return VarUndef;

    WlInitWorklist(&wl);
    parseErr = WlParseJSON(&wl, &doc);

    if (parseErr == -1)
        return I2V(-1);

    buf = WlList(&wl, TypeIsInteg(type)? PS_INTEG : 0);

    if (StrIsEmpty(dir))
    {
        ctchar *name = GetDatalistName(type, base);
        r = MCsaPutCustomFile(bind, CMN_PAN_CONFIG, name, buf, UNLOCK);
    }
    else
    {
        tchar fileName[STRLEN_PATH+1];
        PathFormat(fileName, _T("%s/%s"), dir, base);
        r = CmnPutFileEx(fileName, buf, 0, PUT_FILE_UTF8|PUT_FILE_CRLF);
    }

    FREE(buf);
    WlFreeWorklist(&wl);
    
    return I2V(r == 0 && parseErr == 0? 0 : -2);
    #endif
}


DPCOM_EXPORT(GetGroupList, (Variant opts))
{
    Variant  out  = {0};
    int      r;
    int      group = VarGetInt(&opts, k_group);
    ctchar  *name  = VarGetText(&opts, k_name);
    ctchar  *cell  = VarGetText(&opts, k_cellserver);
    tchar   *buf  = NULL;
    tchar   *filename, *mtime;
    StrMsg   msg  = {0};

    int bind = MCsaBindServer(StrIsValid(cell)? cell : OB2_CellServer());

    if (bind == -1)
        return VarUndef;

    r = MCsaGetGroupListEx(bind, group, name, &buf);
    if (r == -1)
        return VarUndef;

    StrParseStartL(&msg, buf);
    while (StrParseNNextL(&msg, 2, &filename, &mtime) != -1)
    {
        VarPush(&out, CmnVarValidList(
            _T("filename"), T2V(filename),
            _T("mtime"),    T2V(mtime),
            NULL)
        );
    }

    StrMsgFreeL(&msg);
    FREE(buf);

    return out;
}

#include "lib/parse/objectcopy.h"

DPCOM_EXPORT(GetGroupItem, (Variant opts))
{
    int      group = VarGetInt(&opts, k_group);
    ctchar  *name  = VarGetText(&opts, k_name);
    ctchar  *cell  = VarGetText(&opts, k_cellserver);
    BOOL     json  = VarGetInt(&opts, k_json);
    tchar   *buf;
    int      r;

    int bind = MCsaBindServer(StrIsValid(cell)? cell : OB2_CellServer());
    if (bind == -1)
        return VarUndef;

    r = MCsaGetGroupItem(bind, group, name, &buf, READ_ONLY);
    if (r == -1)
        return VarUndef;

    #if !TARGET_OPENVMS
    if (json)
    {
        Variant out = VarUndef;
        OCC_SpecificationT oc;
        OCC_Init(&oc);
        WlSetNoExpand(TRUE);
        WlSetNoGlobals(TRUE);

        switch (group)
        {
        case G_CONSOLIDATIONSPEC_POSTBACKUP:
        case G_CONSOLIDATIONSPEC_SCHEDULED:
            r = ObjectConsolidationParse(&oc, buf);
            break;
        case G_COPYSPEC_POSTBACKUP:
        case G_COPYSPEC_SCHEDULED:
        case G_REPLICATION_POSTBACKUP:
        case G_REPLICATION_SCHEDULED:
        case G_VERIFICATIONSPEC_POSTBACKUP:
        case G_VERIFICATIONSPEC_SCHEDULED:
            r = ObjectCopyParse(&oc, buf);
            break;
        default:
            r = -1;
            break;
        }

        if (r != -1) ObjectCopyListJSON(&out, &oc);
        OCC_Free(&oc);
        FREE(buf);
        return out;
    }
    #endif

    return CT2V(buf);
}


DPCOM_EXPORT(GetCellInfo, (Variant opts))
{
    Variant     out = VarUndef;
    THostList  *hostlist = NULL;
    ctchar     *cell = VarGetText(&opts, k_cellserver);

    if (BU_GetCellInfoHostList(cell, &hostlist) != 0)
        return VarUndef;
    
    out = CellPackHostList(hostlist, 0);

    CellPdeleteHostList(hostlist);

    return out;
}


DPCOM_EXPORT(GetCsvFile, (Variant path, Variant opts))
{
    ERH_FUNCTION(_T("GetCsvFile"));
    ctchar *tsep   = VarGetText(&opts, _T("sep"));
    ctchar *tquote = VarGetText(&opts, _T("quote"));
    tchar   sep    = tsep && tsep[0]? tsep[0] : ';';
    tchar   quote  = tquote && tquote[0]? tquote[0] : '"';
    Variant out    = VarUndef;

    tchar *line, *ctx, *e;
    tchar *file = CmnGetAsciiFile(V2T(&path));
    if (!file)
    {
        ErhMarkSysMajor();
        return VarUndef;
    }

    for (line = StrTok(file, _T("\r\n")); line; line = StrTok(NULL, _T("\r\n")))
    {
        Variant *vline = VarPush(&out, VarUndef);
        for (ctx = line; (e = StrTokCsv(&ctx, sep, quote)) != NULL; )
        {
            VarPush(vline, T2V(e));
        }
    }

    FREE(file);
    
    return out;
}

#ifdef DPCOM_EMULATE_DLSYM
static void ExportStubs()
{
    DPComExport (OB2_EncodePassword);
    DPComExport (OB2_DecodePassword);
    DPComExport (OB2_ExpandHostname);
    DPComExport (OB2_CellServer);
    DPComExport (OB2_NlsGetCatalogString);
    DPComExport (OB2_RunScript);
    DPComExport (OB2_GetConfig);
    DPComExport (OB2_PutConfig);
    DPComExport (OB2_UserAdd);
    DPComExport (OB2_UserDel);
    DPComExport (OB2_UserCommit);
    DPComExport (QueryInit);
    DPComExport (Query);
    DPComExport (QueryFetch);
    DPComExport (GetInstallInfo);
    DPComExport (PutToFile);
    DPComExport (DeleteFile);
    DPComExport (RunningSessions);
    DPComExport (FormatPackedMessage);
    DPComExport (GetDatalist);
    DPComExport (GetDatalists);
    DPComExport (GetGroupList);
    DPComExport (GetGroupItem);
}
#endif // DPCOM_EMULATE_DLSYM

#endif // DISABLE_DPCOM
