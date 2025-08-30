/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   DBSA
* @file      lib/smbdb/dbsa/dbsa.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     This is an extension of DP database, specific to Smart Array integration.
*            It is implemented with ASCII files. Access and manipulation is
*            implemented through library function. Library relies on LIBSMBDB
*            library, which provides core functionality, LIBCSA, which provides
*            remote access mechanism, and LIBIPC, which provides remote communication.
*
* @since     matic   2002-10-18  Copy, paste, check-in
*/
/* CI: GENERAL COMMENTS */
/* CI: ret is initialized to something else than the value assigned to it later on (RET_ERROR) */

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/smbdb/dbsa/dbsa.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/

#include "lib/cmn/common.h"
#include "lib/smbdb/irdb.h"
#include "lib/smbdb/irdbcmn.h"
#include "libdbsa_defs.h"

#include <stdlib.h>
#include <stdio.h>

#include "libdbsa.h"
#include "cs/csa/csa.h"                     /* Need access to csaInitialized */

#if TARGET_WIN32
#ifdef _UNICODE
#define snprintf    _sntprintf
#else
#define snprintf    _snprintf
#endif
#endif

/*========================================================================*//**
*
* @param     IN  initCSA        - The CSAembedded flag. Set to 1 when library shouldn't
*                              initialize IPC
*                                - the caller should use the following
*                                  two values:
*                                    #define DONT_INITIALIZE_IPC         1
*                                    #define INITIALIZE_IPC              0
* @param     IN  localConnection- Set to 1 if you want to bypass the CSA and
*                                 access the database directly. Works only
*                                 with local database.
*                                - the caller should use the following
*                                  two values:
*                                    #define LOCAL_CONNECTION_TO_SADB    1
*                                    #define CSA_CONNECTION_TO_SADB      0
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed; error is marked
*
* @brief     Function calls the irdbInit() function in irdbcmn.c to connect to the
*            Cell Server and initialize irdbcmn library. dbSa_init() must be called
*            prior to any other dbSa_xxxx() calls from this file module.
*
*
*            IMPORTANT
*            This function may only be called once during the execution of the
*            program!
*
*//*=========================================================================*/
/* CI: Function header says that error is marked, but it actually isn't, at least not in all paths */

LIBDBSA_API DBSA_RETURN dbSa_init_on(tchar *server, int initCSA, int localConnection)
{    
    ERH_FUNCTION(_T("dbSa_init_on"));
    int ret = DBSA_OK;
	tchar *tmpPtr = server;

    DbgFcnInEx(__FLND__, _T("initCSA=%d, localConnection=%d"),
        initCSA, localConnection
    );

    if(server == NULL)
	{
        DbgPlain(DBG_IRDB, _T("[%s] Does not have a server name. Will get myself one..."), __FCN__ );
        CliReadCSHost( &tmpPtr );

        /*
         * Backdrop: if CSA insterface is initialized, backdrop to some dummy
         *           hostname.
         */
        if (
             CsaIsInitialized() &&
             ( (tmpPtr == NULL) || (tmpPtr[0] == _T('\0')) )
           )
        {
            const tchar *__dummy_hostname = _T("localhost");

            /* We'll clear the error, the session might already be initialized */
            DbgStamp( DBG_IRDB );
            DbgPlain( DBG_IRDB, _T("*WARNING* Can't get a proper hostname to connect to, but CSA is already initialized. Backdropping to '%s'"),
                      __dummy_hostname );
            ErhClearError();
            tmpPtr = StrNewCopy( __dummy_hostname );
        }
	}

    DbgStamp(DBG_IRDB);
	DbgPlain(DBG_IRDB, _T("Received the cell server name..") ) ;

    /*  Here i have to define a new IRDB_TYPE_**   */
    PROBE_EXIT(
        irdbInit( tmpPtr, initCSA, localConnection, IRDB_TYPE_SA)!= RET_OKAY,
        RET_ERROR,
        DBG_IRDB,
        {
            DbgPlain(DBG_IRDB, _T("Can not connect to the Cell Server %s."), 
                server);
        }
    );

exit:
    if(server == NULL)
	{
		FREE(tmpPtr);
	}
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSa_init */

LIBDBSA_API DBSA_RETURN dbSa_init(int initCSA, int localConnection)
{
	return dbSa_init_on(NULL, initCSA, localConnection);
}

/*========================================================================*//**
*
* @retval    RET_OKAY    success
* @retval    RET_ERROR   failed; error is marked
*
* @brief     Function calls irdbExit() function in irdbcmn.c to cleanup irdbcmn
*            library and disconnect from the Cell Server.
*
*//*=========================================================================*/

LIBDBSA_API DBSA_RETURN dbSa_exit(void)
{
    ERH_FUNCTION(_T("dbSa_exit"));
    int ret = DBSA_OK;

    DbgFcnIn();

    PROBE_EXIT(irdbExit() != RET_OKAY,
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            DbgPlain(DBG_UNEXPECTED, _T("Problem disconnecting from Cell Server."));
        }
    );

exit:
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);

    return(ret);
}   /* dbSa_exit */

/*========================================================================*//**
*
* @param     IN  sessInf  - session info
* @param     IN  devList  - pointer to the list of devices used in the session
* @param     IN  devNum   - number of devices in the list
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed; error is marked
*
* @brief     Stores information about snapshots to the DBSA database.
*            A request is sent to CS for each item in the devList to create/update
*            a corresponding snapshot file. Not all the record fields of devList
*            items need to be defined. The required data fields are: volumeId,
*            instanceNum, crc, and dataList. The rest of data is common to all
*            session, therefore it should be passed in the sesInf parameter.
*            The folowing fields should be defined: sessId, ir, appSystem and
*            bckpSystem.
*
*//*=========================================================================*/
/* CI: Update function header (DESCRIPTION) */

LIBDBSA_API DBSA_RETURN
dbSa_updtSess(sessInfo_t *sesInf, saDev_t *devList, int devNum)
{
    ERH_FUNCTION(_T("dbSa_updtSess"));
    int         ret = DBSA_OK;
    saContext_t *saBuff = NULL; /* CI: More descriptive name for variable should be used */
    time_t      t = 0;
    
    int i;
    
    DbgFcnIn();

    PROBE_EXIT(sesInf == NULL,
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("Session info record not specified."));
        }
    );
    
    PROBE_EXIT( sesInf->sessId[0] == _T('\0'), 
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("Session info data is missing."));
        }
    );
    
    PROBE_EXIT( sesInf->appSystem[0] == _T('\0'), 
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("Application system name is missing."));
        }
    );

    PROBE_EXIT( sesInf->bckpSystem[0] == _T('\0'), 
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("Backup system name is missing."));
        }
    );

    PROBE_EXIT( devList == NULL, 
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("devList is NULL."));
        }
    );

    PROBE_EXIT( devNum < 1, 
        DBSA_ER,
        DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("devNum < 1."));
        }
    );

   /* Prepare buffers with contexts and filenames for each device. */
    saBuff = (saContext_t *)MALLOC(sizeof(saContext_t) * devNum);

    /*get time*/
    t = time(NULL);

    /* Fill buffers. */
    for (i = 0; i < devNum; i++)
    {
        snprintf(saBuff[i].fname,
                STRLEN_PATH,
                SAFILENAME_FMT,
                devList[i].volumeId,
                devList[i].instanceNum );

        snprintf(saBuff[i].data,
                STRLEN_1K,
                SACONTEXT_FMT,
                sesInf->sessId,
                t,
                sesInf->ir, 
                devList[i].crc,
                sesInf->dataList,
                sesInf->appSystem,
                sesInf->bckpSystem);

        saBuff[i].lockFlag = FALSE;
        DbgPlain( DBG_IRDB,
                  _T("update information: file: %s\n contents: %s\n"),
                  saBuff[i].fname, saBuff[i].data);
        DbgPlain( DBG_IRDB,
                  _T("\tSession Id: %s \n\tTime: %ul\n\tir: %d\n\tcrc: %ul\n\tdatalist: %s\n\tappSystem: %s\n\tbckpSystem: %s\n"),
                  sesInf->sessId,
                  t,
                  sesInf->ir, 
                  devList[i].crc,
                  sesInf->dataList,
                  sesInf->appSystem,
                  sesInf->bckpSystem);
    }

    /* Send request(s) for storing the file(s) on the Cell Server for which
     * context was prepared earlier. */
    for (i = 0; i < devNum; i++)
    {
        PROBE_EXIT( irdbPutFile(IRDB_TYPE_SA,
                                saBuff[i].fname,
                                saBuff[i].data) != RET_OKAY,
            DBSA_ER,
            DBG_IRDB,
            {
                DbgPlain(DBG_IRDB,
                         _T("Can not complete storing file context [%s]."),
                         saBuff[i].fname );
            }
        );
    }
    
exit:

    
    if ( saBuff != NULL )
    
    {
        /* CI: Looks like lockFlag is always set to FALSE, so there is no need for 'for' loop */
        for(i = 0; i < devNum; i++)
        {
            if ( saBuff[i].lockFlag )
            {
                ret = irdbUnlockFile(IRDB_TYPE_SA, saBuff[i].fname);
                DbgPlain (DBG_IRDB, 
                          _T("Can not unlock file %s."), 
                          saBuff[i].fname);
            }
        }

        FREE(saBuff);
    }

    DbgFcnOutEx( __FLND__, _T("ret=%d"), ret );
    return( ret );
}   /* dbSa_updtSess */

/*========================================================================*//**
*
* @param     IN   sess  - session which is to be deleted from database
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed; error is marked
*
* @brief     All snapshot files with this session ID i are removed from the
*            database.
*
*//*=========================================================================*/
LIBDBSA_API DBSA_RETURN 
dbSa_rmSess( tchar *sessId )
{
     ERH_FUNCTION( _T("dbSa_rmSess") );
    int ret = DBSA_OK;

    int errorLog = 0;

    int r;
    tchar *fileList = NULL;
    tchar *fileBuff=NULL;
    tchar *pname   =NULL;
    tchar *pdate   =NULL;

    tchar sessId2[ STRLEN_STD+1];
    ULONG time;
    int ir;
    ULONG crc;
    tchar dataList[ STRLEN_1K+1 ];
    tchar appSystem[ STRLEN_HOSTNAME+1];
    tchar bckpSystem[ STRLEN_HOSTNAME+1];

    PROBE_EXIT( sessId == NULL,
                DBSA_ER,
                DBG_IRDB,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain( DBG_IRDB, _T("Session ID is NULL.") );
        }
    );

    DbgFcnInEx(__FLND__, _T("sessId=%s"),
               sessId );


/*--------------------------------------------------------------------------
| get a list of DBSA files
 --------------------------------------------------------------------------*/
    PROBE_EXIT( irdbListFiles( IRDB_TYPE_SA, &fileList ) != 0,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            DbgPlain( DBG_UNEXPECTED, _T("Problem getting file list.") );
        }
        );
    
    PROBE_EXIT( fileList == NULL,
                DBSA_ER,
                DBG_IRDB,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain( DBG_IRDB, _T("File list is empty.") );
        }
        );

        /* CI: Parsing could easily be done inside irdbListFiles function, 
        because this sequence of commands is now used everywhere */
    PROBE_EXIT( StrParseStart(fileList) != MSG_GR_LIST,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            ErhMark (ERH_MFORMAT, __FCN__, ERH_MINOR);
            DbgPlain( DBG_UNEXPECTED, _T("File list in incorrect format.") );
        }
        );

    
/*----------------------------------------------------------------------------
|  Load file one by one and check for sessionIds.
  ---------------------------------------------------------------------------*/
    DbgStamp( DBG_IRDB );
    DbgPlain ( DBG_IRDB, _T("Starting the file check..") );
    while( StrParseNNext(2, &pname, &pdate) != -1)
    {
        ERHCLEARERROR; /* CI: If there was an error reading file, 
        this is critical condition and we should fail the function 
        with error condition. Calling function should probably clear it. */
        DbgStamp( DBG_IRDB );
        if (irdbGetFile( IRDB_TYPE_SA, pname, &fileBuff ) != 0)
        {
            if (errorLog == 0) errorLog = ErhErrno(); /* CI: Two lines *//* CI: This should go out */
            DbgStamp( DBG_IRDB );
            DbgPlain( DBG_IRDB, _T("Can not get file %s context."), pname );
            FREE ( fileBuff );
            ret =  DBSA_ER;
            continue; /* CI: Maybe we should break here */
        }
        if ( fileBuff != NULL )
        {
/*----------------------------------------------------------------------------
|  Parse the file context.
 ---------------------------------------------------------------------------*/
            r = sscanf( fileBuff, SACONTEXT_PARSE_FMT,
                        sessId2,
                        &time,
                        &ir,
                        &crc,
                        dataList,
                        appSystem,
                        bckpSystem);

            if ( r < SACONTEXT_PARSE_ENT )
            {
                if (errorLog == 0) errorLog = ERH_FFORMAT; /* CI: This one should be marked */
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB,_T("Problem parsing the file %s. Ignoring."), pname);
                DbgPlain( DBG_IRDB,_T("Buffer content: %s."), fileBuff);
                    
                FREE( fileBuff );
                continue;
            }

            DbgStamp(DBG_IRDB);
            DbgPlain(DBG_IRDB,
                _T("found sessID: '%s' versus original sessID %s\n"),
                sessId2,sessId );

            FREE( fileBuff );
/*----------------------------------------------------------------------------
| if session IDs match remove the file from DBSA... :-)
 ---------------------------------------------------------------------------*/
            if ( StrCmp( sessId2, sessId ) == 0 )
            {
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB, _T("Removing file %s."), pname );

                /* REMOVING FILE */
                if ( irdbRemFile( IRDB_TYPE_SA, pname ) != 0 )
                { 
                    if (errorLog == 0) errorLog = ErhErrno(); /* CI: This should go out */
                    DbgStamp( DBG_UNEXPECTED );
                    DbgPlain( DBG_UNEXPECTED,_T("Error removing file %s..."),pname);
                    ret = RET_ERROR;
                }
                else
                {
                    DbgStamp( DBG_IRDB );
                    DbgPlain( DBG_IRDB, _T("Removed file %s."), pname );
                }
            }
        }
        else
        {
            DbgStamp( DBG_IRDB );
            DbgPlain( DBG_IRDB, _T("File %s is empty. Ignoring."), pname );
            continue;
        }
    }

exit:

    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) /* CI: This is wrong. Error should be marked where the problem is. It is not here. */
        ErhMark (errorLog, __FCN__, ERH_MINOR);

    if (ErhErrno()!=0)
        ret = DBSA_ER;

    FREE(fileList);

    DbgFcnOutEx( __FLND__, _T("ret=%d"), ret );
    return ret;
}

/*========================================================================*//**
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed or database inconsistency found; error is marked
*
* @brief     Searches through all snapshot files for the specified session ID
*            (sessId). When snapshot file with matching session ID is found the
*            stored application system name and backup system name are copied
*            to return buffers appSystem and bckpSystem. If multiple snapshot files
*            with matching session ID are found and they report different
*            application system or backup system, an error is returned.
*
*//*=========================================================================*/

LIBDBSA_API DBSA_RETURN
dbSa_sessInfo(tchar *sessId, tchar *appSystem, tchar *bckpSystem)
{
    int ret = DBSA_ER;

    tchar *fileList = NULL;
    tchar *fileBuff = NULL;
    tchar *pname    = NULL;
    tchar *pdate    = NULL;

    tchar sessId2[ STRLEN_STD+1 ];
    ULONG time;
    int ir = 1;
    ULONG crc = 0;
    int r = 0;
    int errorLog = 0;
    
    tchar dataList[ STRLEN_1K+1 ];
    tchar tmpAppSystem[ STRLEN_STD+1 ];
    tchar tmpBckpSystem[ STRLEN_STD+1 ];

    int sessInfoFound = 0;
    
    ERH_FUNCTION(_T("dbSa_sessInfo"));

    if (sessId == NULL || appSystem == NULL || bckpSystem == NULL)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("Wrong input parameters"));    
        DbgPlain(DBG_UNEXPECTED, _T("sessId = %s, appSystem = %s, bckpSystem = %s"), 
            NS(sessId), NS(appSystem), NS(bckpSystem));
    }

    DbgFcnInEx(__FLND__, _T("sesInf=%s, *appSystem=0x%X, *bckpSystem=0x%X"), /* CI: Use %p for appSystem, bckpSystem */
               sessId, appSystem, bckpSystem
        );

    /* CI: No need for initialization of parameters. */
    appSystem[0]=_T('\0');
    bckpSystem[0]=_T('\0');
    
    PROBE_EXIT(sessId == NULL,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("SessionId not specified."));
        }
        );

    /* CI: Check CI comments in rmSess */
    /* Get the list of all device files. */
    PROBE_EXIT(irdbListFiles(IRDB_TYPE_SA, &fileList) != 0,
               DBSA_ER,
               DBG_IRDB,
        {
            DbgPlain(DBG_IRDB, _T("Problem getting file list."));
        }
    );

    PROBE_EXIT(fileList == NULL,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MINOR);
            DbgPlain(DBG_UNEXPECTED, _T("File list is empty."));
        }
        );
    
    PROBE_EXIT(StrParseStart(fileList) != MSG_GR_LIST,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_MFORMAT, __FCN__, ERH_MINOR);
            DbgPlain(DBG_UNEXPECTED, _T("File list in incorrect format."));
        }
        );

    /* Load file one by one and check for matching sessionId. */
    while(StrParseNNext(2, &pname, &pdate) != -1)
    {
        ERHCLEARERROR;
        if (irdbGetFile(IRDB_TYPE_SA, pname, &fileBuff) != RET_OKAY)
        {
            if (errorLog == 0) errorLog = ErhErrno();
            DbgStamp(DBG_IRDB);
            DbgPlain(DBG_IRDB, _T("Can not get file %s context."), pname);
            continue;
        };
        if (fileBuff != NULL)
        {
            /* Parse the file context. */
            r = sscanf( fileBuff, SACONTEXT_PARSE_FMT,
                        sessId2,
                        &time,
                        &ir,
                        &crc,
                        dataList,
                        tmpAppSystem,
                        tmpBckpSystem);
            if ( r < SACONTEXT_PARSE_ENT )
            {
                if (errorLog == 0) errorLog = ERH_FFORMAT;
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB,
                          _T("Problem parsing the file %s. Ignoring."), pname );
                
            }
            /* Search for matching session. */
            if (StrCmp(sessId, sessId2) == 0)
            {
                if (sessInfoFound) 
                {
                /* Matching session already found -> check for consistency. */
                    PROBE_EXIT(StrCmp(appSystem, tmpAppSystem) ||
                               StrCmp(bckpSystem, tmpBckpSystem),
                               DBSA_ER,
                               DBG_UNEXPECTED,
                {
                    ErhMark (ERH_SMB_INCONSISTENT_DB, __FCN__, ERH_MINOR);
                    DbgPlain(DBG_UNEXPECTED, 
                             _T("Inconsistent session info detected in file %s. Exiting!"),
                             pname
                        );
                }
                        );
                }
                else
                {
                    /*---------------------------------------------------------
                    | Found matching session for the first time
                     --------------------------------------------------------*/
                    StrCpy( appSystem, tmpAppSystem);
                    StrCpy( bckpSystem, tmpBckpSystem);
                    sessInfoFound = 1;
                    ret = DBSA_OK;
                }
            }
            FREE(fileBuff);
        }
        else
        {
            DbgPlain(DBG_IRDB, _T("File %s is empty. Ignoring."), pname);
        }  
    }

exit:
        
    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) 
        ErhMark (errorLog, __FCN__, ERH_MINOR);
        
    if (ErhErrno()!=0)
        ret =  DBSA_ER;

    if (fileList != NULL)
        FREE(fileList);

    if (fileBuff != NULL)
        FREE(fileBuff);    
    
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSa_sessInfo */

/*========================================================================*//**
*
* @param     IN    ir   - the Instant Restore flag
*                          values (defined in libdbsa_defs.h):
*                              #define IR_SET     1
*                              #define IR_NOT_SET 0
* @param     OUT   list - pointer to the list of devices
* @param     OUT   count  - number of devices in the list
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed; error is marked
*
* @brief     Searches through all snapshots files and returns a list of all
*            unique session IDs (list).  Number of returned items is passed in
*            the Num parameter. Only the sessId is set. Reported are only
*            sessions, whose ir flag setting matches the ir input parameter.
*            This checking can be disabled by setting ir=-1.
*
*//*=========================================================================*/


LIBDBSA_API DBSA_RETURN 
dbSa_sessionList( int ir, tchar ***list, int *count)
{
    int ret = DBSA_OK;
  
    tchar *fileList = NULL;
    tchar *fileBuff = NULL;
    int   r = 0;
    
    ULONG time;
    ULONG crc = 0;
	tchar dataList[ STRLEN_1K+1 ];
    tchar appSystem[ STRLEN_STD+1 ];
    tchar bckpSystem[ STRLEN_STD+1 ];

    tchar **uniqueList = NULL;
	int size = 0;
	tchar *sessID;

    int i;
    int errorLog = 0;
    int found = 0;

    tchar *pname = NULL;
    tchar *pdate = NULL;

    int volumeId = 0;
    int instanceNum = 0;
    
    int newIR = 0;
    
    ERH_FUNCTION(_T("dbSa_sessionList"));

    DbgFcnInEx(__FLND__, _T("ir=%d, *list=0x%X, *Num=0x%X"),
               ir, list, count 
        );
    

/*----------------------------------------------------------------------------
|  Get the list of all device files.
---------------------------------------------------------------------------*/
    PROBE_EXIT( irdbListFiles( IRDB_TYPE_SA, &fileList ) != 0,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            DbgPlain( DBG_UNEXPECTED, _T("Problem getting file list.") );
        }
        );
    
    PROBE_EXIT( fileList == NULL,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MINOR);
            DbgPlain( DBG_UNEXPECTED, _T("File list is empty.") );
        }
        );

    PROBE_EXIT( StrParseStart(fileList) != MSG_GR_LIST,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            ErhMark (ERH_MFORMAT, __FCN__, ERH_MINOR);
            DbgPlain( DBG_UNEXPECTED, _T("File list in incorrect format.") );
        }
        );

/*----------------------------------------------------------------------------
|  Load file one by one and check for sessionIds.
 ---------------------------------------------------------------------------*/
    uniqueList = NULL;

    while( StrParseNNext(2, &pname, &pdate) != -1)
    {
        ERHCLEARERROR;
        if (irdbGetFile( IRDB_TYPE_SA, pname, &fileBuff ) != 0)
        {
            if (errorLog == 0) errorLog = ErhErrno();
            DbgStamp( DBG_IRDB )
            DbgPlain( DBG_IRDB, _T("Can not get file %s context."), pname);
            continue;
        };
        
        if ( fileBuff != NULL )
        {
/*----------------------------------------------------------------------------
|  Parse the file context.
  ---------------------------------------------------------------------------*/
		   sessID=(tchar*)MALLOC( (STRLEN_STD+1)*sizeof(tchar) );


		   r = sscanf( fileBuff, SACONTEXT_PARSE_FMT,
                        sessID,
                        &time,
                        &newIR,
                        &crc,
                        dataList,
                        appSystem,
                        bckpSystem);
  
          if ( r <  SACONTEXT_PARSE_ENT)
            {
                if (errorLog == 0) errorLog = ERH_FFORMAT;
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB,
                          _T("Problem parsing the file. Ignoring.") );
                FREE( fileBuff );
                continue;
            }

          /* look for a matching sessId already in the list */
          for ( i=0; i<size; i++ )
          {
              if ( StrCmp( sessID,
                           uniqueList[ i ] ) == 0 )
                  found = 1;
          }
            
          if ( found == 0 )
          {
              if (( ir == IR_NOT_SET ) || ( ir == newIR ))
              {
                  uniqueList = (tchar**)REALLOC( uniqueList, ( size+1 )*sizeof(tchar*) );
                  uniqueList[i] = (tchar*)MALLOC( ( STRLEN_STD+1 )*sizeof(tchar) );
                  StrCpy( uniqueList[ size ], sessID) ;
                  size++;
              }
            }
          else
          {
              found = 0;
          }
          FREE( fileBuff );
        }
        else
        {
            DbgStamp( DBG_IRDB );
            DbgPlain( DBG_IRDB, _T("File %s is empty. Ignoring."), pname );
        }
    }

    DbgStamp( DBG_IRDB );
    DbgPlain( DBG_IRDB, _T("Finished creating UniqueList") );

    *list = uniqueList; 
    *count = size;

exit:
    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) 
        ErhMark (errorLog, __FCN__, ERH_MINOR);
        
    if (ErhErrno()!=0)
        ret = DBSA_ER;

    if ( fileList != NULL )
        FREE( fileList );

    if ( fileBuff != NULL )
        FREE( fileBuff );
    
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);

    return(ret);
}   /* dbSa_sessionList */

/*========================================================================*//**
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed or database inconsistency found; error is marked
*
* @brief     Searches through all snapshot files for the specified session ID
*            (sessId). When snapshot file with matching session ID is found the
*            information stored in that file is stored in saDev_t list which is
*            later returned. It fill in information on: volumeID, instanceNum,
*            sessId, dateTime, crc, ir, dataList, appSystem and bckpSystem.
*            If multiple snapshot files with matching session IDs
*            are found and they report different application system or backup
*            system, an error is returned.
*
*//*=========================================================================*/

LIBDBSA_API DBSA_RETURN
dbSa_sessInfoEX(tchar *sessId, saDev_t **list, int *count)
{
    int ret = DBSA_ER;

    tchar *fileList = NULL;
    tchar *fileBuff = NULL;
    tchar *pname    = NULL;
    tchar *pdate    = NULL;

    tchar sessId2[ STRLEN_STD+1 ];
    ULONG time;
    int ir = 1;
    ULONG crc = 0;
    int r = 0;
    int errorLog = 0;

    tchar appSystem[ STRLEN_STD+1 ];
    tchar bckpSystem[ STRLEN_STD+1 ];
    
    tchar dataList[ STRLEN_1K+1 ];

    int sessInfoFound = 0;

    saDev_t *devs=NULL;
    int size = 0;
    
    ERH_FUNCTION(_T("dbSa_sessInfoEX"));


    DbgFcnInEx(__FLND__, _T("sesInf=%s, **list=0x%X, *count=0x%X"),
               sessId, list, count
        );


    appSystem[0]='\0';
    bckpSystem[0]='\0';
    
    PROBE_EXIT(sessId == NULL,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("SessionId not specified."));
        }
        );


    /* Get the list of all device files. */
    PROBE_EXIT(irdbListFiles(IRDB_TYPE_SA, &fileList) != 0,
               DBSA_ER,
               DBG_IRDB,
        {
            DbgPlain(DBG_IRDB, _T("Problem getting file list."));
        }
    );

    PROBE_EXIT(fileList == NULL,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MINOR);
            DbgPlain(DBG_UNEXPECTED, _T("File list is empty."));
        }
        );


    PROBE_EXIT(StrParseStart(fileList) != MSG_GR_LIST,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_MFORMAT, __FCN__, ERH_MINOR);
            DbgPlain(DBG_UNEXPECTED, _T("File list in incorrect format."));
        }
        );


    /* Load file one by one and check for matching sessionId. */

    size = 0;
    while(StrParseNNext(2, &pname, &pdate) != -1)
    {
        ERHCLEARERROR;

        if (irdbGetFile(IRDB_TYPE_SA, pname, &fileBuff) != RET_OKAY)
        {
            if (errorLog == 0) errorLog = ErhErrno();
            DbgStamp(DBG_IRDB);
            DbgPlain(DBG_IRDB, _T("Can not get file %s context."), pname);
            continue;
        };

        if (fileBuff != NULL)
        {
            /* Parse the file context. */
            r = sscanf( fileBuff, SACONTEXT_PARSE_FMT,
                        sessId2,
                        &time,
                        &ir,
                        &crc,
                        dataList,
                        appSystem,
                        bckpSystem);
            
            if ( r < SACONTEXT_PARSE_ENT )
            {
                if (errorLog == 0) errorLog = ERH_FFORMAT;
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB, _T("Problem parsing the file %s. Ignoring."), pname );
            }

            /* Search for matching session. */
            if (StrCmp(sessId, sessId2) == 0)
            {
                if (sessInfoFound) 
                {
                /* Matching session already found -> check for consistency. */
                    PROBE_EXIT(StrCmp(appSystem, devs[0].appSystem) ||
                               StrCmp(bckpSystem, devs[0].bckpSystem),
                               DBSA_ER,
                               DBG_UNEXPECTED,
                {
                    ErhMark (ERH_SMB_INCONSISTENT_DB, __FCN__, ERH_MINOR);
                    DbgPlain(DBG_UNEXPECTED, 
                             _T("Inconsistent session info detected in file %s. Exiting!"),
                             pname
                        );
                }
                        );
                }
                else
                {
                    /*---------------------------------------------------------
                    | Found sessionId for the first time
                     --------------------------------------------------------*/
                    sessInfoFound = 1;
                    ret = DBSA_OK;
                }
                
                devs = (saDev_t*)REALLOC(devs, (size+1)*sizeof(saDev_t));

                /* fill in the information from file */
                StrCopy( devs[ size ].sessId, sessId, STRLEN_SESSIONNAME);
                
                r = sscanf( pname, _T("%32s_%d"),
                            devs[ size ].volumeId,
                            &(devs[ size ].instanceNum) );
                
                if ( r < 2 )
                {
                    if (errorLog == 0) errorLog = ERH_FFORMAT;
                    DbgStamp( DBG_IRDB );
                    DbgPlain( DBG_IRDB, _T("Problem parsing the filename (%s).Ignoring."),
                              pname );
                    FREE( fileBuff );
                    continue;
                }

                devs[ size ].crc = crc;
                devs[ size ].ir = ir;
                devs[ size ].dateTime = time ;
                StrCpy( devs[ size ].dataList, dataList );
                StrCpy( devs[ size ].appSystem, appSystem );
                StrCpy( devs[ size ].bckpSystem, bckpSystem );

                size++;
            }
            
            FREE(fileBuff);
        }
        else
        {
            DbgPlain(DBG_IRDB, _T("File %s is empty. Ignoring."), pname);
        }  
    }

       
exit:

    (*list) = devs;
    (*count) = size;

    DbgPlain(DBG_IRDB, _T("sesInf=%s, *list=0x%X, count=%d"),
             sessId, *list, *count );

    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) 
        ErhMark (errorLog, __FCN__, ERH_MINOR);
        
    if (ErhErrno()!=0)
        ret = DBSA_ER;

    if (fileList != NULL)
        FREE(fileList);

    if (fileBuff != NULL)
        FREE(fileBuff);    
    
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSa_sessInfo */


/*========================================================================*//**
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed or database inconsistency found; error is marked
*
* @brief     Searches through all snapshot files for the specified session ID
*            (sessId). When snapshot file with matching session ID is found the
*            information stored in that file is stored in saDev_t list which is
*            later returned. It fill in information on: volumeID, instanceNum,
*            sessId, dateTime, crc, ir, dataList, appSystem and bckpSystem.
*            If multiple snapshot files with matching session IDs
*            are found and they report different application system or backup
*            system, an error is returned.
*
*//*=========================================================================*/

LIBDBSA_API DBSA_RETURN
dbSa_snapshotInfo(const tchar volId[], const int instanceNum, saDev_t *snapInfo)
{
    tchar *fileBuff = NULL;
    tchar name[STRLEN_1K];
    int ret           = DBSA_OK,
        errorLog      = 0,
        r             = 0,
        sessInfoFound = 0;


    ERH_FUNCTION(_T("dbSa_snapshotInfo"));

    DbgFcnInEx(__FLND__, _T("volId=%s, instanceNum=0x%X, *snapinfo=0x%X"),
               volId, instanceNum, snapInfo
        );

    PROBE_EXIT(volId == NULL,
               DBSA_ER,
               DBG_UNEXPECTED,
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgPlain(DBG_UNEXPECTED, _T("volId not specified."));
        }
        );

    sprintf(name,
            SAFILENAME_FMT,
            volId,
            instanceNum );

    if (irdbGetFile(IRDB_TYPE_SA, name, &fileBuff) != DBSA_OK)
    {
        int retVal = RET_OKAY;

        ERHCLEARERROR;
        if ((retVal = irdbFileExistInFileList(IRDB_TYPE_SA, name)) != DBSA_OK)
        {
            ERHCLEARERROR;
            
            if (retVal == ERH_LIBDBSA_RECORD_NOT_FOUND)
            {
                ErhMark (ERH_LIBDBSA_RECORD_NOT_FOUND, __FCN__, ERH_MAJOR);
                DbgStamp(DBG_IRDB);
                DbgPlain(DBG_IRDB, _T("There is no file: %s"), name);
            }
            

            DbgFcnOutEx(__FLND__, _T("retVal=%d"), retVal);            
            return DBSA_ER;
        }
/* MATIC 20021213
        if (errorLog == 0) errorLog = ErhErrno();
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("Can not get file %s context."), name);
        ErhMark (ERH_LIBDBSA_RECORD_NOT_FOUND, __FCN__, ERH_MAJOR);
*/
    };
/* CI: If irdbGetFile fails, it can be because there is no record or something
   CI: else has failed. I think that the only way to determine this is to call 
   CI: irdbListFiles and try to find file name in the list. If it's not there
   CI: mark error with code I defined in oberrors.h, ERH_LIBDBSA_RECORD_NOT_FOUND.
   CI: Otherwise, error should be marked already. Don't forget to update description 
   CI: and IRS. */
    if (fileBuff != NULL)
    {
        DbgStamp(DBG_IRDB); 
        DbgPlain(DBG_UNEXPECTED, _T("[%s] fileBuff = [%s]"), __FCN__, fileBuff);

        r = sscanf( fileBuff, SACONTEXT_PARSE_FMT, 
                    snapInfo->sessId,
                    &(snapInfo->dateTime),
                    &(snapInfo->ir),
                    &(snapInfo->crc),
                    snapInfo->dataList,
                    snapInfo->appSystem,
                    snapInfo->bckpSystem);
                    
        
        if ( r < SACONTEXT_PARSE_ENT )
        {
            if (errorLog == 0) errorLog = ERH_FFORMAT;
            DbgStamp( DBG_IRDB );
            DbgPlain( DBG_IRDB,
                      _T("Problem parsing the file %s. Ignoring."), name );
        }

        snapInfo->instanceNum = instanceNum;
        strcpy(snapInfo->volumeId, volId);
        
    }
exit:

    DbgStamp(DBG_IRDB); 

    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) 
        ErhMark (errorLog, __FCN__, ERH_MINOR);

    if (ErhErrno()!=0)
        ret = DBSA_ER;

    if (fileBuff != NULL)
        FREE(fileBuff);    


  /*  DbgStamp (DBG_IRDB);
    DbgPlain (DBG_IRDB, 
        _T("[%s]\
              \n\t snapInfo->volumeId      = %s\
              \n\t snapInfo->instanceNum   = %d\
              \n\t snapInfo->sessId        = %s\
              \n\t snapInfo->dateTime      = %ld\
              \n\t snapInfo->crc           = %ld\
              \n\t snapInfo->ir            = %d\
              \n\t snapInfo->datalist      = %s\
              \n\t snapInfo->appSystem     = %s\
              \n\t snapInfo->bckSystem     = %s\n\n"), 
              __FCN__, 
              snapInfo->volumeId, 
              snapInfo->instanceNum,
              snapInfo->sessId,   
              snapInfo->dateTime, 
              snapInfo->crc,      
              snapInfo->ir,  
              snapInfo->dataList, 
              snapInfo->appSystem,
              snapInfo->bckpSystem);*/


        DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
        return(ret);
}   /* dbSa_sessInfo */


/*========================================================================*//**
*
* @param     OUT   list - pointer to the list of devices
* @param     OUT   count  - number of devices in the list
*
* @retval    DBSA_OK    success
* @retval    DBSA_ER   failed; error is marked
*
* @brief     Returns a list of snapshot files.
*
*//*=========================================================================*/


LIBDBSA_API DBSA_RETURN 
dbSa_snapshotList (saDev_t **list, int *count)
{
    int ret = DBSA_OK;

    tchar *fileList = NULL;
    tchar *fileBuff = NULL;
    int   r = 0;
    
    saDev_t *devices = NULL;
    
    int size = 0;
    tchar *pname    = NULL;
    tchar *pdate    = NULL;

    int errorLog = 0;
    
    ERH_FUNCTION(_T("dbSa_snapshotList"));

    
    DbgFcnInEx(__FLND__, _T("**list=0x%X, *Num=0x%X"),list, count 
        );

    
    *list  = NULL;
    *count = 0;

/*----------------------------------------------------------------------------
|  Get the list of all device files.
---------------------------------------------------------------------------*/
    PROBE_EXIT( irdbListFiles( IRDB_TYPE_SA, &fileList ) != 0,
                DBSA_ER,
                DBG_UNEXPECTED,
        {
            DbgPlain( DBG_UNEXPECTED, _T("Problem getting file list.") );
        }
        );
    if ( fileList == NULL )
    {
        DbgPlain( DBG_UNEXPECTED, _T("File list is empty.") );
    }
    else
    {
        PROBE_EXIT( StrParseStart(fileList) != MSG_GR_LIST,
                    DBSA_ER,
                    DBG_UNEXPECTED,
            {
                ErhMark (ERH_MFORMAT, __FCN__, ERH_MINOR);
                DbgPlain( DBG_UNEXPECTED,
                          _T("File list in incorrect format.") );
            }
            );
/*----------------------------------------------------------------------------
|  Load files to see if they contain any info.
 ---------------------------------------------------------------------------*/
        devices = (saDev_t *)MALLOC( size * sizeof( saDev_t ) );
        while( StrParseNNext(2, &pname, &pdate) != -1)
        {
            ERHCLEARERROR;
            if (irdbGetFile( IRDB_TYPE_SA, pname, &fileBuff ) != 0)
            {
                if (errorLog == 0) errorLog = ErhErrno();
                DbgStamp( DBG_IRDB )
                    DbgPlain( DBG_IRDB,
                              _T("Can not get file %s context."), pname);
                continue;
            };

            if ( fileBuff != NULL )
            {
                devices = (saDev_t *)REALLOC( devices,
                                              ( size+1 )*
                                              sizeof( saDev_t ) );
                r = sscanf (pname, _T("%32s_%d"),
                            devices[size].volumeId,
                            &devices[size].instanceNum );
                if ( r < 2)
                {
                    if (errorLog == 0) errorLog = ERH_FFORMAT;
                    DbgStamp( DBG_IRDB );
                    DbgPlain( DBG_IRDB,
                            _T("Problem parsing the filename (%s). Ignoring."),
                              pname );
                    FREE( fileBuff );
                    continue;
                }

                r = sscanf (fileBuff, SACONTEXT_PARSE_FMT,
                        devices[size].sessId,
                        &(devices[size].dateTime),
                        &(devices[size].ir),
                        &(devices[size].crc),
                        devices[size].dataList,
                        devices[size].appSystem,
                        devices[size].bckpSystem);

                if ( r < SACONTEXT_PARSE_ENT )
                {
                    if (errorLog == 0) errorLog = ERH_FFORMAT;
                    DbgStamp( DBG_IRDB );
                    DbgPlain( DBG_IRDB,
                            _T("Problem parsing the filename (%s). Ignoring."),
                              pname );
                    FREE( fileBuff );
                    continue;
                }
                
				size++;
                
				FREE( fileBuff );
            }
            else
            {
                DbgStamp( DBG_IRDB );
                DbgPlain( DBG_IRDB, _T("File %s is empty. Ignoring."), pname );
            }
        }

        DbgStamp( DBG_IRDB );
        DbgPlain( DBG_IRDB, _T("Finished creating devices.") );
        
        *list = devices; 
        *count = size;
    }
exit:
    /* mark first cleared error */
    if ((errorLog != 0) && (ErhErrno()==0)) 
        ErhMark (errorLog, __FCN__, ERH_MINOR);
        
    if (ErhErrno()!=0)
        ret = DBSA_ER;

    if ( fileList != NULL )
        FREE( fileList );

    if ( fileBuff != NULL )
        FREE( fileBuff );
    
    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSa_snapshotList */
