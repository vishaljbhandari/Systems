/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Instant Restore DataBase API
* @file      lib/smbdb/dbsmis/dbsmis.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Functions dealing with Instant Restore DataBase part of storing and
*            retreiving of information about SMIS devices and sessions.
*            
*            FUNCTIONALITY/FUNCTIONS
*
* @since     Base code taken from VA IRdb interface
*
* @remarks   *            HISTORY
*            Base code taken from VA IRdb interface
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/smbdb/dbsmis/dbsmis.c $Rev$ $Date::                      $:");
#endif

/* ------------------------------------------------------- INCLUDE FILES -- */

#include <stdlib.h>
#include <stdio.h>

#define IRDB_IMPL

#include "lib/cmn/common.h"
#include "lib/smbdb/irdb.h"
#include "lib/smbdb/irdbcmn.h"

#include "cs/csa/csa.h"                     /* Need access to csaInitialized */

#include "dbsmis.h"

/* ------------------------------------------------------------- GLOBALS -- */

/* -------------------------------------------------------------- PROTOS -- */
static int
sessID_sortRoutine (
    const void *str1,
    const void *str2
);

static int
sessID_sortRoutine_struct (
    const void *str1,
    const void *str2
);

/* ----------------------------------------------------------- FUNCTIONS -- */

/*========================================================================*//**
*
* @param     irdbType IRDB_TYPE_SMISLOGIN or IRDB_TYPE_SMISLHLOGIN.
*
* @retval    When connecting is successful it returns 0, otherwise -1.
*
* @brief     Function is used to connect on the specified Cell Server. It
*            calls dbInit() function in irdbcmn.c to stay consistent with
*            the other dbSMIS_xxxx() calls from this file module.
*
*
*            IMPORTANT
*            This function may only be called once during the execution of the
*            program!
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_init_on (
    irdbTyp_t irdbType,
    const tchar *server,
    int    Flag,
    int    FlagLocal
)
{
    ERH_FUNCTION(_T("dbSMIS_init_on_generic"));
    int ret = RET_OKAY;
    const tchar *serverPtr = server;
    tchar* tmpPtr = NULL;

    DbgFcnInEx(__FLND__, _T("Srv=%s, Flg=%d, FlgLoc=%d"), NSD(server), Flag, FlagLocal);

    if ( server == NULL )
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
        serverPtr = tmpPtr;
    }

    if ( irdbInit(tmpPtr, Flag, FlagLocal, irdbType ) != RET_OKAY )
    {
        DbgStamp( DBG_IRDB );
        DbgPlain(DBG_IRDB, _T("Problems calling irdbInit()"));
        ret = RET_ERROR;
    }

    if ( tmpPtr != NULL )
    {
        /* the CliReadCSHost() allocated mem for tmpPtr */
        FREE( tmpPtr );
    }

    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSMIS_init_on */


LIBDBSMIS_API int
dbSMIS_init (
    irdbTyp_t irdbType,
    int Flag,
    int FlagLocal
)
{
    return (dbSMIS_init_on(irdbType, NULL, Flag, FlagLocal));
}


/*========================================================================*//**
*
* @retval    When disconnecting is successful it returns 0, otherwise -1.
*
* @brief     Function is used to disconnect from the specified Cell Server. It
*            calls irdbExit() function in irdbcmn.c to stay consistent with
*            the other dbSMIS_xxxx() calls from this file module.
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_exit (
    void
)
{
    ERH_FUNCTION(_T("dbSMIS_exit"));
    int ret = RET_OKAY;

    DbgFcnIn();

    if ( irdbExit() != RET_OKAY )
    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("Problems calling irdbExit()"));
        ret = RET_ERROR;
    }

    DbgFcnOutEx(__FLND__, _T("ret=%d"), ret);
    return(ret);
}   /* dbSMIS_exit */


/*========================================================================*//**
* @param     IRdb_Table      - IN: filter for IR database entries based on disk array type
* @param     fileList        - I/O: used as a CS file-list pointer
* @param     numElems        - IN: MAX number of elems to be read (-1 4 inf.)
* @param     pattern         - IN: fileglobbing pattern
* @param     outVal          - OUT: (pre)allocated list to store findings
* @param     _should_allocate- IN: flag saying should the function alloc memory
*
* @brief     This function is designed for browsing the keys of the dbSMIS, according
*            to the given fileglobbing pattern (similar to giving sorted output of
*            the 'ls/dir' commands in the '.../db40/smis/replica' directory).
*            The format of the dbSMIS key is given in VAFILENAME_FMT, and it should
*            be: <SessID> <TimeStamp> <Lun> <Datalist>
*
*            The 'outVal' output can be given to the dbSMIS content-retrieval
*            function, dbSMIS_fileScan().
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_fileList (
    irdbTyp_t   IRdb_Table,
    tchar       **fileList,
    int         numElems,
    const tchar *inPattern,
    tchar       ***outVal,
    short       _should_allocate
)
{
    ERH_FUNCTION( _T("dbSMIS_fileListEx") );
    int counter;
    int i;
    UINT _allocated;
    tchar *pname, *pdate;
    tchar *pattern = NULL;

    DbgFcnInEx(__FLND__, _T("#=%d ptrn='%s' alloc=%d"), numElems, NSD(inPattern), _should_allocate );

    ErhAssert( _T("dbSMIS_fileListEx:fileList=NULL"), fileList != NULL );
    ErhAssert( _T("dbSMIS_fileListEx:numElems=0"),    numElems != 0 );
    ErhAssert( _T("dbSMIS_fileListEx:outVal=NULL"),   outVal   != NULL );

    /* We are doing reallocation because the call below FnmFilenameMatch expects a tchar*.
       Even though it does not actually change the pattern this is not ensured by the API
       prototype. Actually, even system functions are not consistent in this. But, at least
       we can try to be...                                                                  */
    if ( NULL != inPattern )
        pattern = StrNewCopy (inPattern);

    /* Open the fileList if needed */
    if ( (*fileList) == NULL )
    {
        if ( irdbListFiles(IRdb_Table, fileList) )
        {
            DbgPlain(DBG_IRDB, _T("[%s] Problem getting file list."), __FCN__);
            counter = RET_ERROR;
            goto _lab_exit;
        }
        else if ( StrParseStart( (*fileList) ) != MSG_GR_LIST )
        {
            DbgPlain(DBG_IRDB, _T("[%s] File list in incorrect format."), __FCN__);
            counter = RET_ERROR;
            goto _lab_exit;
        }
    }

    counter    = 0;
    _allocated = 0;

    /*
     * Note: numElems can hold the negative value, that's why we type-cast it to unsigned.
     *       To get all of the elements, use the numElems = -1 ( -1 becomes MAXUINT )
     */
    while(
           ((UINT )counter < (UINT )numElems) &&
           (StrParseNNext(2, &pname, &pdate) != -1)
         )
    {
        /*
         * Memory (re) allocation part
         */
        if ( _should_allocate )
        {
            if ( _allocated == 0 )
            {
                /* Initial allocation.
                 * Num of elems: MIN( (UINT )numElems, SMISDEV_ALLOC_UNIT) */
                _allocated = ( (UINT )numElems > SMISDEV_ALLOC_UNIT )
                             ? SMISDEV_ALLOC_UNIT
                             : numElems;
                /* 'i' holds the mem. size chunk; alloc 1 more to have the last elem NULL */
                i          = (_allocated+1) * sizeof(tchar *);
                (*outVal)  = (tchar **)MALLOC(i);
                (void) memset( (*outVal), 0, i );
            }
            else if ( _allocated <= (UINT )counter )
            {
                /* Reallocation needed.
                 * Num of elems: incremented by SMISDEV_ALLOC_UNIT */
                _allocated += SMISDEV_ALLOC_UNIT + 1;         /* alloc 1 more to have the last elem NULL */
                i           = sizeof( tchar * );            /* 'i' holds the unit size */
                (*outVal)   = (tchar **)realloc( (*outVal), _allocated * i );
                (void) memset( &(*outVal)[counter], 0, (_allocated-counter) * i );
            }
        }

        if ( pattern && (FnmFilenameMatch(pattern, pname, 0) == 0) )
        {
            DbgPlain(DBG_IRDB, _T("[%s] Skipping '%s' (pattern)."), __FCN__, pname);
        }
        else
        {
            /* Store the value */
            /* Note: This is storing a pointer which points to the 'fileList'
             *       StrMsg's buffer. If you free the buffer, the references
             *       will not be valid any more.
             */
            (*outVal)[counter] = pname;
            counter++;
        }
    }

    /* Sort the list */
	if (counter > 0)	/* [Purify] UMR: Uninitialized memory read in dbSMIS_fileListEx {3 occurrences} */
		qsort((*outVal), counter, sizeof(tchar*), sessID_sortRoutine);

_lab_exit:
    FREE (pattern);
    DbgFcnOutEx(__FLND__, _T("count=%d"), counter);
    return counter;
}



/* use struct with context data instead of **filebuf and *bufOffset */
static int dbSMIS_parseEntry (irdbTyp_t irdbType, const tchar *filename, const tchar *scanFor, smis_evaDev_t *dev, tchar **fileBuff, int *bufOffset)
{
    ERH_FUNCTION (_T("dbSMIS_parseEntry"));
    int retVal = RET_OKAY;
    tchar *tmpPtr = NULL;
    int newOffset = -1;
    int i = 0;

    DbgFcnInEx (__FLND__, _T("filename = %s, scanFor = %s, bufOffset = %d"), NSD(filename), NSD(scanFor), *bufOffset);

    /* if new file has not been received yet, retrieve it from the CS */
    if ( NULL == *fileBuff )
    {
        if ( StrIsEmpty(filename) )
        {
            /* We have no file and also no file was specified. This is illegal use of the function. */
            DbgStamp( DBG_UNEXPECTED );
            DbgPlain( DBG_UNEXPECTED, _T("[%s] Either filename or buffer must be specified in the call."), __FCN__);
            retVal = RET_ERROR;
            goto end;
        }

        if ( (irdbGetFile(irdbType, filename, fileBuff) != 0) || ((*fileBuff)==NULL) )
        {
            DbgStamp( DBG_UNEXPECTED );
            DbgPlain( DBG_UNEXPECTED, _T("[%s] *WARNING* Can not get the content of the file '%s'."), __FCN__, filename);
            retVal = RET_ERROR;
            goto end;
        }
        *bufOffset = 0;
    }

    if ( -1 == *bufOffset )
    {
        DbgStamp( DBG_IRDB );
        DbgPlain( DBG_IRDB, _T("[%s] Reached the end of the buffer."), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }

    /* Check if the entry is multi-line */
    if ( NULL != (tmpPtr = strchr( &(*fileBuff)[*bufOffset], _T('\n'))) )
    {
        /* Found '\n' - set the new offset and cut the buffer */
        newOffset = (UINT)(tmpPtr - (*fileBuff));
        DbgPlain(DBG_IRDB, _T("[%s] Found '\\n' at %u offset. Cutting the string..."), __FCN__, newOffset);
        (*fileBuff)[newOffset++] = _T('\0');
    }
    else
    {
        newOffset = -1;
    }

    /* Parse the entry */
    DbgPlain(DBG_IRDB+100, _T("[%s] Content of the current entry: '%s'"), __FCN__, *fileBuff);

    /* grep file part. Be careful here because strcmp does not get entire file but only one entry */
    if ( scanFor && (strstr(&(*fileBuff)[*bufOffset], scanFor)==NULL) )
    {
        DbgPlain(DBG_IRDB, _T("[%s] Ignoring an entry in the file (grep)"), __FCN__);
        DbgPlain(DBG_IRDB+100, _T("`-> Was looking for '%s' in '%s'"), scanFor, &(*fileBuff)[*bufOffset]);
        retVal = RET_ERROR;
        goto end;
    }
    
    i = sscanf(&(*fileBuff)[*bufOffset], SMIS_REPLICA_ENTRY_READ_FMT,
                    dev->VDiskID,
                    dev->EVAID,
                   &dev->flags,
                   &dev->VDiskType,
                   &dev->purge,
                    dev->LUNWWN,
                   &dev->dateTime,
                   &dev->crc,
                    dev->ParentVDiskID,
                    dev->sessId,
                    dev->dataList,
                    dev->appSystem,
                    dev->bckpSystem,
                    dev->VDiskName,
                    dev->EVAName,
                    dev->diskPath,
                    dev->currentDiskPath
                    );

    StrToUpper(dev->VDiskID);
    StrToUpper(dev->EVAID);
    StrToUpper(dev->LUNWWN);
    StrToUpper(dev->ParentVDiskID);

    DbgPlain(DBG_IRDB, _T("[%s] Scanned %d of at least %d elems."), __FCN__, i, SMIS_REPLICA_ENTRY_CONTENT_ELEMS);

    if (i < SMIS_REPLICA_ENTRY_CONTENT_ELEMS)
    {
        DbgStamp( DBG_UNEXPECTED );
        DbgPlain( DBG_UNEXPECTED, _T("*WARNING* Problem parsing the buffer. Scanned only %d/%d entries. Continuing..."), i, SMIS_REPLICA_ENTRY_CONTENT_ELEMS );
        retVal = RET_ERROR;
        goto end;
    }
    
end:
    *bufOffset = newOffset;
    DbgFcnOutEx (__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}


/*========================================================================*//**
*
* @param     irdbType    - IN:  filter for IR database entries based on disk array type
* @param     fileList    - I/O: used as a CS file-list pointer
*                               You can initially pass it as a pointer to NULL
*                               You could also give it a list of vaDB entries (like
*                                one acquired from dbSMIS_fileList())
* @param     numElems    - IN:  number of requested elements (can be -1 for inf.)
* @param     _isIR       - IN:  IR session filter (can be <0 to ignore)
* @param     scanFor     - IN:  string to search in a CS-file
*                               Works similar to 'grep', fills only matching
* @param     outVal      - I/O: used to present resoults.
*                               Must be a _pointer_ to _pointer_! See (*)
* @param     _should_allocate - IN: flag saying should the function alloc memory. See (**)
*
* @retval    It can will return:
*                >0  - number of elements read and stored in the outVal
*                 0  - no elements to read from the 'fileList' (EOF condition)
*                -1  - an error has occured
*
* @brief     This function will browse the IR database entries, filling either the
*            preallocated structure, or allocating it for you.
*
*            (*) Therefore, as an 'outVal' parameter, one should use a pointer to a
*            pointer.
*
*//*=========================================================================*/

LIBDBSMIS_API int
dbSMIS_fileScanEx (
    irdbTyp_t       irdbType,
    tchar          **fileList,
    int              numElems,
    int              isIR,
    int              isReserved,
    int              isOriginal,
    const tchar     *fnamePattern,
    const tchar     *scanFor,
    smis_evaDev_t **outVal,
    short           _should_allocate,
    tchar         **fileBuff,           /* External buffer save - handles multiline */
    int            *_buff_offs          /* External offset save - handles multiline */
)
{
    ERH_FUNCTION( _T("dbSMIS_fileScanEx") );
    int counter;
    int i;
    UINT     _allocated;
    tchar **fname = NULL;

    DbgFcnInEx(__FLND__, _T("#=%d ir=%d isReserved=%d isOriginal=%d ptrn='%s' grep='%s' alloc=%d"),
                         numElems,
                         isIR,
                         isReserved,
                         isOriginal,
                         NSD(fnamePattern),
                         NSD(scanFor),
                         _should_allocate);

    ErhAssert( _T("dbSMIS_fileScan:fileList=NULL"), fileList != NULL );
    ErhAssert( _T("dbSMIS_fileScan:numElems=0"),    numElems != 0 );
    ErhAssert( _T("dbSMIS_fileScan:outVal=NULL"),   outVal   != NULL );

    counter    = 0;     /* Structure list counter */
    _allocated = 0;     /* Structure list size */

    /*
     * NOTE: numElems can hold the negative value, that's why we type-cast it to unsigned.
     *       To get the max. value of elems, use the numElems = -1
     */

    while ( (UINT )counter < (UINT )numElems )
    {
        if ( -1 == *_buff_offs)
            FREE (*fileBuff);

        /* if new file has not been received yet, get the new filename */
        if ( NULL == *fileBuff )
        {
            int funRetVal;

            FREE (fname);

            /* NOTE: Always just one file name. numElems is not the same as numFilenames */
            funRetVal = dbSMIS_fileList (irdbType, fileList, 1, fnamePattern, &fname, 1);

            if ( 0 > funRetVal )
            {
                DbgPlain(DBG_IRDB, _T("[%s] The dbSMIS_fileList() returned a negative value. Breaking out..."), __FCN__);
                counter = RET_ERROR;
                goto _lab_exit;
            }

            if ( 0 == funRetVal )
            {
                DbgPlain (DBG_IRDB, _T("[%s] No more files left. Can exit."), __FCN__);
                goto _lab_exit;
            }
        }

        /* Allocate memory for the new element */
        if ( _should_allocate )
        {
            if ( _allocated == 0 )
            {
                /* Initial allocation.
                 * Num of elems: MIN( (UINT )numElems, SMISDEV_ALLOC_UNIT) */
                _allocated = ( (UINT )numElems > SMISDEV_ALLOC_UNIT )
                             ? SMISDEV_ALLOC_UNIT
                             : numElems;
                i          = _allocated * sizeof(smis_evaDev_t);  /* 'i' holds the mem. size chunk */
                (*outVal)  = (smis_evaDev_t *)MALLOC(i);
                (void) memset( (*outVal), 0, i );
            }
            else if ( _allocated <= (UINT )counter )
            {
                /* Reallocation needed.
                 * Num of elems: incremented by SMISDEV_ALLOC_UNIT */
                _allocated += SMISDEV_ALLOC_UNIT;
                i           = sizeof(smis_evaDev_t);              /* 'i' holds the unit size */
                (*outVal)   = (smis_evaDev_t *)realloc( (*outVal), _allocated * i );
                (void) memset( &(*outVal)[counter], 0, (_allocated-counter) * i );
            }
        }

        if ( RET_OKAY != dbSMIS_parseEntry (irdbType, fname ? fname[0] : NULL, scanFor, (*outVal)+counter, fileBuff, _buff_offs))
        {
            DbgPlain (DBG_IRDB, _T("[%s] Parse entry failed. Moving on to next entry."), __FCN__);
            continue;
        }

        /* Check whether the entry is filtered out */
        if ( isIR >= 0 )
        {
            if ( isIR != (SMIS_IR_FLAG & (*outVal)[counter].flags) )
            {
                DbgPlain( DBG_IRDB, _T("[%s] Entry skipped because req. IR(%d) is not the same as in entry (%d)"), __FCN__, isIR, (*outVal)[counter].flags );
                continue;
            }
        }

        if ( isReserved >= 0 )
        {
            if ( (isReserved << 1) != (SMIS_RESERVED_FLAG & (*outVal)[counter].flags ) )
            {
                DbgPlain( DBG_IRDB, _T("[%s] Entry skipped because req. reservation flag (%d) is not the same as in entry (%d)"), __FCN__, isReserved, (*outVal)[counter].flags );
                continue;
            }
        }

        if ( isOriginal >= 0 )
        {
            if ( (isOriginal << 2) != (SMIS_ORIGINAL_FLAG & (*outVal)[counter].flags ) )
            {
                DbgPlain( DBG_IRDB, _T("[%s] Entry skipped because req. original flag (%d) is not the same as in entry (%d)"), __FCN__, isOriginal, (*outVal)[counter].flags );
                continue;
            }
        }

        /* CAUTION: From this point on, the counter points to the NEW (empty) element */
        counter++;
    }

_lab_exit:
    if ( 0 < counter )
        qsort( (*outVal), counter, sizeof(smis_evaDev_t), sessID_sortRoutine_struct );
    
    DbgFcnOutEx(__FLND__, _T("count=%d"), counter);
    return counter;
}


LIBDBSMIS_API int
dbSMIS_fileScan (
    irdbTyp_t irdbType,
    tchar          **fileList,
    int              numElems,
    int              _isIR,
    int              _isReserved,
    int              _isOriginal,
    const tchar     *fnamePattern,
    const tchar     *scanFor,
    smis_evaDev_t **outVal,
    short           _should_allocate
)
{
	USES_IRDB_INSTANCE
    return dbSMIS_fileScanEx (
        irdbType, fileList, numElems, _isIR, _isReserved, _isOriginal, fnamePattern, scanFor, outVal, _should_allocate,
        &ThisIrdb->fileBuff, &ThisIrdb->buffOffset);
}


/*========================================================================*//**
*
* @param     irdbTyp_t irdbType - IN: filter for IR database entries based on disk array type
* @param     tchar **list    - OUT: holds allocated list of elems
* @param     int    *Num     - OUT: holds the number of elems
*
* @retval    0       - the list is filled
|      -1       - error condition, failure
*
* @brief     This function is but a compatibility wrapper around the dbSMIS_fileScan()
*            function, and it will try to allocate the memory and read the complete
*            content of the SMIS database.
*
*            Please call dbSMIS_fileScan() function directly.
*            That way you can call it couple of times with the same buffer allocated
*            buffer.
*
*//*=========================================================================*/

LIBDBSMIS_API int
dbSMIS_uniqueList (
    irdbTyp_t irdbType,
    smis_evaDev_t **list,
    int            *Num
)
{
    ERH_FUNCTION( _T("dbSMIS_uniqueList") );
    int retVal;
    tchar   *fileList = NULL;
    tchar   *pattern = _T("[0-9]* *");
    smis_evaDev_t *retList = NULL;

    DbgFcnIn();

    retVal = dbSMIS_fileScan(irdbType,     /* filter for IR database entries based on disk array type */
                            &fileList,     /* ptr 2 NULL */
                            -1,             /* read till end */
                             1,             /* IR flag */
                            -1,             /* Reservation flag */
                             0,             /* Original disks flag */
                             pattern,       /* entry pattern */
                             NULL,          /* content search string */
                            &retList,       /* return list */
                             1 );           /* alloc mem */

    DbgPlain( DBG_IRDB, _T("[%s] The dbSMIS_fileScan() query returned %d elems"), __FCN__, retVal );

    if ( retVal < 0 )
    {
        DbgStamp( DBG_IRDB );
        DbgPlain( DBG_IRDB, _T("[%s] Can't get the list of files, retVal=%d"), __FCN__, retVal );
    }
    else
    {
        DbgPlain( DBG_IRDB, _T("[%s] Got my list of files, %d elems."), __FCN__, retVal );
        *Num = retVal;
        *list= retList;
        retVal = RET_OKAY;
    }

    if ( fileList )
        FREE( fileList );
    /* CAUTION: The 'retList' holds big chunk of allocated memory which should be freed by caller */

    DbgFcnOutEx (__FLND__, _T("ret=%d"), retVal);
    return retVal;
}   /* dbSMIS_uniqueList */


/*========================================================================*//**
*
* @param     irdbType    - IN: filter for IR database entries based on disk array type
* @param     sessID      - IN:  session ID for wich the devices are queried
* @param     appSystem   - OUT: the application sytem used in the session
* @param     bckpSystem  - OUT: the backup sytem used in the session
|
* @param     Note: the 'appSystem' and 'bckpSystem' should be allocated by caller
*
* @retval    0      Success
|       -1      Error / session not found
*
* @brief     Function very similar to dbVA_sessInfo()
*
*//*=========================================================================*/

#define SESS_INFO_ELEMS     2       /* The number of elements dbSMIS_sessInfo() will try to get */

LIBDBSMIS_API int
dbSMIS_sessInfo (
    irdbTyp_t irdbType,
    tchar *sessID,
    tchar *appSystem,
    tchar *bckpSystem
)
{
    ERH_FUNCTION(_T("dbSMIS_sessInfo"));
    int retVal;
    tchar    *fileList = NULL;
    tchar    *tmpPtr;
    tchar     sessPattern[STRLEN_SESSIONNAME+4]={0};
    smis_evaDev_t  retList[SESS_INFO_ELEMS] = {0, 0};
    smis_evaDev_t *pRetList;

    DbgFcnInEx( __FLND__, _T("sessID=%s"), NSD(sessID) );

    ErhAssert( _T("dbSMIS_sessInfo:sessID=NULL"),     sessID     != NULL );
    ErhAssert( _T("dbSMIS_sessInfo:strlen(sessID) < 12"), StrLen(sessID) > 11 );
    ErhAssert( _T("dbSMIS_sessInfo:appSystem=NULL"),  appSystem  != NULL );
    ErhAssert( _T("dbSMIS_sessInfo:bckpSystem=NULL"), bckpSystem != NULL );

    if ( 
        (sessID[10] == _T('-')) &&      /* first chech if it's the 'transformable' format */
        ((tmpPtr = StrFromUserSessionId(sessID)) != NULL)   /* Then try to transform it */
       )
    {
        DbgPlain( DBG_IRDB, _T("[%s] Using SessID '%s'"), __FCN__, tmpPtr );
    }
    else
    {
        /* SessID not in the 'transformable' format. Try using it directly. */
        tmpPtr = sessID;
    }
    /* NOTE: the 'tmpPtr' holds the correct SessID now */

    sessPattern[0] = _T('\0');
    StrCat(sessPattern, tmpPtr,   STRLEN_SESSIONNAME+3);
    StrCat(sessPattern, _T(" *"), STRLEN_SESSIONNAME+3);
    dbSMIS_sessID2fileName(sessPattern);       /* Convert to filesystem format: 2002.02.02... */

    pRetList = (smis_evaDev_t *) &retList;
    (void) memset( pRetList, 0, sizeof(retList) );

    retVal = dbSMIS_fileScan(irdbType,       /* filter for IR database entries based on disk array type */
                              &fileList,     /* ptr 2 NULL */
                              SESS_INFO_ELEMS, /* read count */
                              -1,           /* IR flag */
                              -1,           /* Reservation flag */
                              -1,           /* Is original disk flag 
                                               appropriate sessID supplied by caller - no need to filter additionally */
                              sessPattern,  /* entry pattern */
                              NULL,         /* content search string */
                             &pRetList,     /* return list */
                              0 );          /* alloc mem */

    DbgPlain( DBG_IRDB, _T("[%s] The dbSMIS_fileScan() query returned %d elems"), __FCN__, retVal );

    if ( retVal <= 0 )
    {
        DbgStamp( DBG_IRDB );
        DbgPlain( DBG_IRDB, _T("[%s] Can't get the list of files, retVal=%d"), __FCN__, retVal );
        retVal = RET_ERROR;
    }
    else
    {
        DbgPlain( DBG_IRDB, _T("[%s] Got my list of files, %d elems."), __FCN__, retVal );
        StrCopy( appSystem,  retList[0].appSystem,  STRLEN_STD );
        StrCopy( bckpSystem, retList[0].bckpSystem, STRLEN_STD );
        retVal = RET_OKAY;
    }

    if ( fileList )
        FREE( fileList );

    DbgFcnOutEx(__FLND__, _T("ret=%d"), retVal);
    return retVal;
}   /* dbSMIS_sessInfo */



/*========================================================================*//**
*
* @param     irdbType IRDB_TYPE_SMISLOGIN or IRDB_TYPE_SMISLHLOGIN.
*
* @retval    ?       - ?
*
* @brief     TBD
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_updtCIMOMpwd (
    irdbTyp_t irdbType,
    const tchar *CIMOM_host,
    int    CIMOM_port,
    const tchar *CIMOM_namespace,
    const tchar *CIMOM_user,
    const tchar *CIMOM_passwd,
    int CIMOM_ssl
)
{
    ERH_FUNCTION(_T("dbSMIS_updtCIMOMpwd"));
    smis_CIMOM_t *myCIMOMs;
    int           myElems;
    tchar pwd_name[STRLEN_PATH+1];
    tchar pwd_content[STRLEN_PATH*2+1];
    tchar ssl_ptr[STRLEN_PATH+1];
    int retVal = RET_OKAY;

    DbgFcnInEx(__FLND__, _T("H:%s, P:%d, NS:%s, U:%s S:%d"),
               NSD(CIMOM_host),
                   CIMOM_port,
               NSD(CIMOM_namespace),
               NSD(CIMOM_user),
                   CIMOM_ssl);

    ErhAssert( _T("dbSMIS_updtCIMOMpwd:CIMOM_host=NULL"), CIMOM_host != NULL );
    ErhAssert( _T("dbSMIS_updtCIMOMpwd:CIMOM_port<0"), CIMOM_port >= 0 );
    ErhAssert( _T("dbSMIS_updtCIMOMpwd:CIMOM_namespace=NULL"), CIMOM_namespace != NULL );
    ErhAssert( _T("dbSMIS_updtCIMOMpwd:CIMOM_user=NULL"), CIMOM_user != NULL );
    ErhAssert( _T("dbSMIS_updtCIMOMpwd:CIMOM_passwd=NULL"), CIMOM_passwd != NULL );

    /*
     * CHECKME: Should check for FS illegal chars and whitespaces
     */
    (void) sprintf(pwd_name, _T("%s %d %s"), CIMOM_host, CIMOM_port, CIMOM_user);

    retVal = dbSMIS_getCIMOMs( irdbType, pwd_name, &myCIMOMs, &myElems );
    if ( retVal<0 || myElems<=0 )
    {
        /* No similar elements found - just insert this one */
        (void) sprintf(pwd_content, _T("%s %d %s"), CIMOM_namespace,CIMOM_ssl, CIMOM_passwd);
        retVal = irdbPutFile(irdbType, pwd_name, pwd_content);
    }
    else
    {
        /*
         * Some elements already exist for that host/port/user combination
         * Will have to browse them and see if we should append or overwrite
         */
        int i;
        int _has_overwritten;
        tchar *tmpPtr = NULL;

        _has_overwritten = 0;
        pwd_content[0] = _T('\0');
        for (i=0; i<myElems; i++)
        {
            if ( i )
            {
                StrCatEx(pwd_content, _T("\n"), STRLEN_STD, &tmpPtr);
            }
            if ( strcmp(myCIMOMs[i].CIM_namespace, CIMOM_namespace) == 0 )
            {
                _has_overwritten = 1;
                StrCopy(myCIMOMs[i].passwd, CIMOM_passwd, STRLEN_STD);
                myCIMOMs[i].ssl=CIMOM_ssl;

            }
            StrCatEx(pwd_content, myCIMOMs[i].CIM_namespace, STRLEN_STD, &tmpPtr);
            StrCatEx(pwd_content, _T(" "), STRLEN_STD, &tmpPtr);
            
            sprintf(ssl_ptr,_T("%d"),CIMOM_ssl);
            StrCatEx(pwd_content, ssl_ptr, STRLEN_STD, &tmpPtr);
            StrCatEx(pwd_content, _T(" "), STRLEN_STD, &tmpPtr);
            
            StrCatEx(pwd_content, myCIMOMs[i].passwd, STRLEN_STD, &tmpPtr);
        }

        if ( ! _has_overwritten )
        {
            /* This is a new element that we need to append */
            /* CHECKME: Should we try to realloc the content's size? */

            StrCatEx(pwd_content, _T("\n"), STRLEN_STD, &tmpPtr);
            StrCatEx(pwd_content, CIMOM_namespace, STRLEN_STD, &tmpPtr);

            sprintf(ssl_ptr,_T("%d"),CIMOM_ssl);
            StrCatEx(pwd_content, ssl_ptr, STRLEN_STD, &tmpPtr);
            StrCatEx(pwd_content, _T(" "), STRLEN_STD, &tmpPtr);
            
            StrCatEx(pwd_content, _T(" "), STRLEN_STD, &tmpPtr);
            StrCatEx(pwd_content, CIMOM_passwd, STRLEN_STD, &tmpPtr);
        }

        retVal = irdbPutFile(irdbType, pwd_name, pwd_content);
        FREE(myCIMOMs);
    }

    DbgFcnOutEx(__FLND__, _T("ret=%d"), retVal );
    return(retVal);
}



/*========================================================================*//**
*
* @param     irdbType IRDB_TYPE_SMISLOGIN or IRDB_TYPE_SMISLHLOGIN
*
* @retval    ?       - ?
*
* @brief     TBD
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_listCIMOMs (
    irdbTyp_t irdbType,
    tchar       **fileList,
    int         numElems,
    const tchar *pattern,
    tchar       ***outVal,
    short       _should_allocate
)
{
    return dbSMIS_fileList (irdbType, fileList, numElems, pattern, outVal, _should_allocate);
}


/*========================================================================*//**
*
* @param     irdbType IRDB_TYPE_SMISLOGIN or IRDB_TYPE_SMISLHLOGIN.
*
* @retval    ?       - ?
*
* @brief     TBD
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_getCIMOMs (
    irdbTyp_t irdbType,
    const tchar   *pattern,
    smis_CIMOM_t **outCIMOMs,
    int           *outElems
)
{
    ERH_FUNCTION(_T("dbSMIS_getCIMOMs"));
    tchar  *fileList = NULL;
    tchar **out_fl;
    int retVal;
    int _elem_cnt;

    DbgFcnInEx(__FLND__, _T("pat=%s"), NSD(pattern));

    ErhAssert( _T("dbSMIS_getCIMOMs:outCIMOMs=NULL"), outCIMOMs != NULL );

    retVal = dbSMIS_listCIMOMs(irdbType, &fileList, -1, pattern, &out_fl, 1);
    DbgPlain(DBG_IRDB, _T("[%s] Got %d from dbSMIS_listCIMOMs()"), __FCN__, retVal);

    if ( retVal < 0 )
    {
        _elem_cnt = -1;
    }
    else if ( retVal > 0 )
    {
        int _siz;
        int _ret;
        int _err_cnt;
        int i;
        int ssl_cont;
        tchar *_content;

        ssl_cont = 0;
        _siz = (retVal+1)*sizeof(smis_CIMOM_t);
        *outCIMOMs = MALLOC( _siz );
        if ( (*outCIMOMs) == NULL )
        {
            /* TODO: LOG Can't alloc memory */
            abort();
        }
        (void) memset ( (*outCIMOMs), 0, _siz );

        _err_cnt = _elem_cnt = 0;
        for (i=0; i<retVal; i++)
        {
            _ret = sscanf(out_fl[i], _T("%s %u %s"),
                                     (*outCIMOMs)[_elem_cnt].host,
                                    &(*outCIMOMs)[_elem_cnt].port,
                                     (*outCIMOMs)[_elem_cnt].user );
            if ( _ret != 3 )
            {
                /*
                 * Soft error - can't parse one entry
                 */
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("*WARNING* Can't parse '%s'. Continuing..."), out_fl[i]);
                _err_cnt++;
            }
            else if ((irdbGetFile(irdbType,
                                  out_fl[i],
                                 &_content) != 0) ||
                     (_content==NULL) )
            {
                /*
                 * Soft error - can't read one entry
                 */
                DbgStamp( DBG_UNEXPECTED );
                DbgPlain( DBG_UNEXPECTED, _T("*WARNING* Can not get the content of the passwd file '%s'. Continuing..."), out_fl[i]);
                _err_cnt++;
            }
            else
            {
                /* Content scanned successsfully - let's parse it */
                tchar *tmpPos, *tmpPtr;

                tmpPtr = _content;

                while ( tmpPtr )
                {
                    /* Slice entry's content by delimiter */
                    if ( (tmpPos=strchr(tmpPtr, _T(' '))) == NULL )
                    {
                        /* Can't get entry delimiter!! */
                        DbgStamp( DBG_UNEXPECTED );
                        DbgPlain( DBG_UNEXPECTED, _T("*WARNING* Can not parse the content of the passwd file '%s'. Continuing..."), out_fl[i]);
                        _err_cnt++;
                        break;
                    }
                    *(tmpPos++) = _T('\0');
                    StrCopy((*outCIMOMs)[_elem_cnt].CIM_namespace, tmpPtr, STRLEN_STD);
                    tmpPtr = tmpPos;

                    if (tmpPtr[1] == _T(' '))
                    {
                        /* Note:: Format being parsed is -- root/eva 0 ABCD */
                        if (tmpPtr[0] == _T('0'))
                        {
                            (*outCIMOMs)[_elem_cnt].ssl=0;
                        }
                        else if (tmpPtr[0] == _T('1'))
                        {
                            (*outCIMOMs)[_elem_cnt].ssl=1;
                        }
                        tmpPtr[1] = _T('\0');
                        tmpPtr =  tmpPtr + 2;
                    }
                    else
                    {
                        /* Note:: Format being parsed is -- root/eva ABCD for 5.5*/
                        (*outCIMOMs)[_elem_cnt].ssl=0;
                    }
/*
                    StrAtoi(tmpPtr,&ssl_cont,&_conv);
                    if (!_conv)
                    {
                        DbgStamp( DBG_UNEXPECTED );
                        DbgPlain( DBG_UNEXPECTED, _T("*WARNING* StrAtoi failed"));
                        DbgPlain(DBG_UNEXPECTED, _T("May be not ssl in dp5.5"));
                    }
                    else
                    {
                        if ( (tmpPos=strchr(tmpPtr, _T(' '))) == NULL )
                        {
                        DbgStamp( DBG_UNEXPECTED );
                        DbgPlain( DBG_UNEXPECTED, _T("*WARNING* Can not parse the content of the passwd file '%s'. Continuing..."), out_fl[i]);
                        _err_cnt++;
                        *(tmpPos++) = _T('\0');
                        break;
                        }
                    }
                    (*outCIMOMs)[_elem_cnt].ssl=ssl_cont;
                    tmpPtr = tmpPos;
*/
                    
                    /* Check for newline with the rest of the string */
                    if ( (tmpPos = strstr(tmpPtr, _T("\r\n"))) == NULL )
                    {
                        /*
                         * No newline - close the entry
                         */
                        StrCopy((*outCIMOMs)[_elem_cnt].passwd, tmpPtr, STRLEN_STD);
                        DbgPlain( DBG_IRDB, _T("Successfully scanned elem# %d: host=%s, port=%d, user=%s, namespace=%s, passwd=%s ssl=%d"),
                                  _elem_cnt,
                                  (*outCIMOMs)[_elem_cnt].host,
                                  (*outCIMOMs)[_elem_cnt].port,
                                  (*outCIMOMs)[_elem_cnt].user,
                                  (*outCIMOMs)[_elem_cnt].CIM_namespace,
                                  (*outCIMOMs)[_elem_cnt].passwd,
                                  (*outCIMOMs)[_elem_cnt].ssl);
                        _elem_cnt++;
                        FREE(_content);
                        break;
                    }
                    else
                    {
                        /*
                         * Newline found - more data in one entry
                         */
                        int _new_siz;

                        /* Cut the string on newline position, save the first part */
                        while ( *(tmpPos) == _T('\n') || *(tmpPos) == _T('\r') )
                        {
                            *(tmpPos++) = _T('\0');
                        }
                        StrCopy((*outCIMOMs)[_elem_cnt].passwd, tmpPtr, STRLEN_STD);
                        DbgPlain( DBG_IRDB, _T("Successfully scanned elem# %d: host=%s, port=%d, user=%s, namespace=%s, passwd=%s ssl=%d"),
                                  _elem_cnt,
                                  (*outCIMOMs)[_elem_cnt].host,
                                  (*outCIMOMs)[_elem_cnt].port,
                                  (*outCIMOMs)[_elem_cnt].user,
                                  (*outCIMOMs)[_elem_cnt].CIM_namespace,
                                  (*outCIMOMs)[_elem_cnt].passwd ,
                                  (*outCIMOMs)[_elem_cnt].ssl);
                        _elem_cnt++;

                        /* realloc */
                        _new_siz = _siz + sizeof(smis_CIMOM_t);
                        *outCIMOMs = realloc(*outCIMOMs, _new_siz);
                        (void) memset( ((char *)(*outCIMOMs)) + _siz, 0, sizeof(smis_CIMOM_t) );
                        _siz = _new_siz;

                        /* Copy the key values */
                        StrCopy((*outCIMOMs)[_elem_cnt].host, (*outCIMOMs)[_elem_cnt-1].host, STRLEN_STD);
                        StrCopy((*outCIMOMs)[_elem_cnt].user, (*outCIMOMs)[_elem_cnt-1].user, STRLEN_STD);
                        (*outCIMOMs)[_elem_cnt].port = (*outCIMOMs)[_elem_cnt-1].port;

                        /* reposition, and enter the loop once more */
                        tmpPtr = tmpPos;
                    }


                }   /* WHILE - scanning elem's content */
            }       /* IF-ELSE - elem scanned */
        }           /* FOR - elem */

        FREE(out_fl);
        FREE(fileList);
        retVal = (_err_cnt == 0) ? RET_OKAY : RET_ERROR;
    }
    else
    {
        /* retVal = 0 .. no entries */
        _elem_cnt = 0;
    }

    /* If a pointer has been given, store the number of elements */
    if ( outElems != NULL )
    {
        *outElems = _elem_cnt;
    }

    DbgFcnOutEx(__FLND__, _T("elems=%d, ret=%d"), _elem_cnt, retVal);
    return retVal;
}


/*========================================================================*//**
*
* @param     irdbType IRDB_TYPE_SMISLOGIN or IRDB_TYPE_SMISLHLOGIN.
*
* @retval    ?       - ?
*
* @brief     TBD
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_delCIMOMs (
    irdbTyp_t irdbType,
    tchar *pattern,
    int   *outElems
)
{
    ERH_FUNCTION(_T("dbSMIS_delCIMOMs"));

    tchar  *fileList = NULL;
    tchar **out_fl;
    int retVal;
    int _elem_cnt;


    DbgFcnInEx(__FLND__, _T("pat=%s"), NSD(pattern));

    retVal = dbSMIS_listCIMOMs(irdbType, &fileList, -1, pattern, &out_fl, 1);
    DbgPlain(DBG_IRDB, _T("[%s] Got %d from dbSMIS_listCIMOMs()"), __FCN__, retVal);

    if ( retVal < 0 )
    {
        _elem_cnt = -1;
    }
    else if ( retVal > 0 )
    {
        int i, _err_cnt;

        _err_cnt = _elem_cnt = 0;
        for (i=0; i<retVal; i++)
        {
             if ( irdbRemFile(irdbType, out_fl[i]) == 0 )
             {
                 DbgPlain(DBG_IRDB, _T("[%s] Deleted CIMOM elem '%s'"), __FCN__, out_fl[i]);
                 _elem_cnt++;
             }
             else
             {
                 DbgStamp(DBG_UNEXPECTED);
                 DbgPlain(DBG_UNEXPECTED, _T("[%s] Can't delete CIMOM elem '%s'"), __FCN__, out_fl[i]);
                 _err_cnt++;
             }
        }
        FREE(out_fl);
        FREE(fileList);
        retVal = (_err_cnt == 0) ? RET_OKAY : RET_ERROR;
    }
    else
    {
        /* Nothing to delete */
        _elem_cnt = 0;
    }

    /* If a pointer has been given, store the number of elements */
    if ( outElems != NULL )
    {
        *outElems = _elem_cnt;
    }

    DbgFcnOutEx(__FLND__, _T("elems=%d, ret=%d"), _elem_cnt, retVal);
    return retVal;
}


/*========================================================================*//**
*
* @param     ?
*
* @retval    ?       - ?
*
* @brief     TBD
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_appendReplica (
    smis_evaDev_t  *entry,
    smis_evaDev_t **outVal,
    unsigned int   *elem_count
)
{
    ERH_FUNCTION(_T("dbSMIS_appendReplica"));
    unsigned int _blk;
    int retVal = RET_ERROR;

    DbgFcnInEx(__FLND__, _T("entry=%p, out=%p, cnt=%p"), entry, outVal, elem_count);

    ErhAssert( _T("dbSMIS_appendReplica:entry=NULL"), entry != NULL );
    ErhAssert( _T("dbSMIS_appendReplica:outVal=NULL"), outVal != NULL );
    ErhAssert( _T("dbSMIS_appendReplica:elem_count=NULL"), elem_count != NULL );

    /*
     * memory (re)allocation
     */
    _blk = sizeof(smis_evaDev_t);
    if ( *outVal == NULL )
    {
        unsigned int _siz;

        DbgPlain(DBG_IRDB, _T("[%s] Initial call - allocating first space"), __FCN__);

        _siz = (SMISDEV_ALLOC_UNIT+1)*_blk;

        *outVal = MALLOC(_siz);
        if ( *outVal == NULL )
        {
            ErhMark (ERH_NOMEM, __FCN__, ERH_MAJOR);
            goto _lab_bail_out;
        }
        DbgPlain(DBG_IRDB+100, _T("[%s] Allocated mem segment @ %p, size %u, blk %u"), __FCN__, *outVal, _siz, _blk );
        (void) memset( *outVal, 0, _siz );
    }
    else if ( ( (*elem_count) % SMISDEV_ALLOC_UNIT ) == 0 )
    {
        DbgPlain(DBG_IRDB, _T("[%s] Num of elems=%d - time to reallocate"), __FCN__, *elem_count);

        (*outVal) = REALLOC( *outVal, (*elem_count+SMISDEV_ALLOC_UNIT+1)*_blk );
        DbgPlain(DBG_IRDB+100, _T("[%s] RE-allocated mem segment @ %p, size %u"), __FCN__, *outVal, (*elem_count+SMISDEV_ALLOC_UNIT+1)*_blk );
        if ( *outVal == NULL )
        {
            ErhMark (ERH_NOMEM, __FCN__, ERH_MAJOR);
            goto _lab_bail_out;
        }
        DbgPlain(DBG_IRDB+100, _T("[%s] MEM-set from @ %p, size %u"), __FCN__, &(*outVal)[*elem_count+1], SMISDEV_ALLOC_UNIT*_blk );
        (void) memset( &(*outVal)[*elem_count+1], 0, SMISDEV_ALLOC_UNIT*_blk );
    }

    if ( memcpy( &(*outVal)[*elem_count], entry, _blk ) == NULL )
    {
        ErhMark (ERH_SYSERR, __FCN__, ERH_MAJOR);
        goto _lab_bail_out;
    }
    (*elem_count)++;
    retVal = RET_OKAY;

_lab_bail_out:
    DbgFcnOutEx(__FLND__, _T("ret=%d"), retVal);
    return retVal;
}


/***************************************************************************
 * static int sessID_sortRoutine_struct()
 *      - used internally to for a qsort()
 */
static int sessID_sortRoutine_struct (
    const void *ptr1,
    const void *ptr2
)
{
	smis_evaDev_t *a = (smis_evaDev_t*) ptr1;
	smis_evaDev_t *b = (smis_evaDev_t*) ptr2;
	
    tchar *s1 = StrNewCopy (StrFromUserSessionId (a->sessId));
    tchar *s2 = StrFromUserSessionId (b->sessId);
    int retVal = StrCmp (s1, s2);
    FREE (s1);
    return retVal;
}


/***************************************************************************
 * static int sessID_sortRoutine()
 *      - used internally to for a qsort()
 */
static int
sessID_sortRoutine (
    const void *str1,
    const void *str2
)
{
    return StrCmp( (*(tchar **)str1), (*(tchar **)str2) );
}


/***************************************************************************
 * static void fix_empty_strings()
 *      - will put in 'N/A' for empty strings
 *      - to be used when inserting stuff into the dbVA
 */
static void
fix_empty_strings (
    tchar *entry
)
{
    if ( entry && (entry[0] == _T('\0')) )
    {
        entry[0] = _T('N');
        entry[1] = _T('/');
        entry[2] = _T('A');
        entry[3] = _T('\0');
    }
}


/*========================================================================*//**
*
* @retval    NULL    - failure
* @retval    sessID  - transformed sessID value (replica of original)
*
* @brief     Transfer between 2002.01.28 and 2002/01/28 format (sessIDs)
*
*//*=========================================================================*/

LIBDBSMIS_API
tchar *
dbSMIS_sessID2fileName (
    tchar *sessID
)
{
    ERH_FUNCTION( _T("dbSMIS_sessID2fileName") );
    int _ret;

    DbgFcnInEx(__FLND__, _T("sessID='%s'"), NSD(sessID));

    ErhAssert( _T("dbSMIS_sessID2fileName:sessID=NULL"), sessID != NULL );

    if ( (_ret = (int)StrLen(sessID)) < 10 )
    {
        DbgStamp( DBG_UNEXPECTED );
        DbgPlain( DBG_UNEXPECTED, _T("The SessionID '%s' is not in the expected format"), sessID );
        _ret = -1;
    }
    else if ( sessID[4] == _T('/') && sessID[7] == _T('/') )
    {
        sessID[4] = sessID[7] = _T('.');
        _ret = 0;
    }
    else if ( sessID[4] == _T('.') && sessID[7] == _T('.') )
    {
        sessID[4] = sessID[7] = _T('/');
        _ret = 0;
    }
    else
    {
        DbgStamp( DBG_UNEXPECTED );
        DbgPlain( DBG_UNEXPECTED, _T("The SessionID '%s' is not in the expected format"), sessID );
        _ret = -1;
    }

    DbgFcnOutEx(__FLND__, _T("ret=%d"), _ret);

    return (_ret) ? NULL : sessID;
}


/*========================================================================*//**
*
* @param     irdbType - filter for IR database entries based on disk array type
* @param     curr_fname - filename
* @param     inArry - array of all entries in smisDB
* @param     inArry_cnt - number of elements
*
* @retval    RET_ERROR or RET_OK
*
* @brief     Takes the initial FName. In case the SessID changes, it will calculate
*            a new name entry.
*
*//*=========================================================================*/
LIBDBSMIS_API int
dbSMIS_updateReplicasNamed ( irdbTyp_t irdbType,
                             tchar         *curr_fname,
                             smis_evaDev_t *inArry,
                             unsigned int   inArry_cnt )
{
    ERH_FUNCTION(_T("dbSMIS_updateReplicasNamed"));
    size_t totalBytesWritten = 0;
    int charsWritten = 0;
    int totalCharsWritten = 0;
    size_t totalBytesAllocated = MAX_SM_PACKETSIZE + sizeof(tchar);
    tchar *buffer = MALLOC(totalBytesAllocated);

    tchar *curr_sessID;
    int retVal = RET_ERROR;
    int i = 0;    

    DbgFcnInEx(__FLND__, _T("out=%p, cnt=%u"), inArry, inArry_cnt);

    ErhAssert( _T("dbSMIS_updateReplicasNamed:inArry=NULL"), inArry != NULL );
    ErhAssert( _T("dbSMIS_updateReplicasNamed:inArry_cnt=NULL"), inArry_cnt >= 0 );

    //sort array on session id
    qsort( inArry, inArry_cnt, sizeof(smis_evaDev_t), sessID_sortRoutine_struct ); 

    //iterate through all entries
    while ( i < inArry_cnt )
    {
        /* roll the loop again for new sessionID*/
        curr_sessID = inArry[i].sessId;
        totalCharsWritten = 0;
        totalBytesWritten = 0;

        /* Set filename */
        sprintf(curr_fname, SMIS_FILENAME_FMT, dbSMIS_sessID2fileName(StrFromUserSessionId(curr_sessID)), inArry[i].dataList); 
        DbgPlain(DBG_IRDB+100, _T("Upadating IRDBfile: %s"), NSD(curr_fname));

        // iterate through entries with the same sessionID
        while( i < inArry_cnt && StrCmp(curr_sessID,inArry[i].sessId) == 0 )
        {
            if ( totalBytesWritten + MAX_SM_PACKETSIZE / 2 >= totalBytesAllocated)
            {
                totalBytesAllocated += MAX_SM_PACKETSIZE;
                buffer = (tchar *)REALLOC(buffer, totalBytesAllocated);
                DbgPlain(DBG_IRDB+100, _T("Bytes allocated: %d"), totalBytesAllocated);
            }

            if ( totalCharsWritten )
            {
                /* Continuing entry -> add "\n" */
                (void) StrCat(buffer, _T("\n"), ((totalBytesAllocated / sizeof(tchar)) - 1) );
                totalCharsWritten++;
                totalBytesWritten += 1 * sizeof(tchar);
            }

            fix_empty_strings(inArry[i].VDiskID);
            fix_empty_strings(inArry[i].EVAID);
            fix_empty_strings(inArry[i].LUNWWN);

            /* Sorry about this ugly work-around. Sometimes the CIMOM will return ElementName
             * instead of a WWN for LUNWWN. Currently it seems that the best place to put it
             * is whereever the entries are being created. Please remove this on confirmation
             * that this has been fixed on the CIMOM side.
             */
            StrReplace (inArry[i].LUNWWN, _T(' '), _T('_'));

            fix_empty_strings(inArry[i].ParentVDiskID);
            fix_empty_strings(inArry[i].sessId);
            fix_empty_strings(inArry[i].dataList);
            fix_empty_strings(inArry[i].appSystem);
            fix_empty_strings(inArry[i].bckpSystem);
            fix_empty_strings(inArry[i].VDiskName);
            fix_empty_strings(inArry[i].EVAName);
            fix_empty_strings(inArry[i].diskPath);
            fix_empty_strings(inArry[i].currentDiskPath);

            charsWritten = sprintf(&buffer[totalCharsWritten], SMIS_REPLICA_ENTRY_WRITE_FMT,
                                      inArry[i].VDiskID,
                                      inArry[i].EVAID,
                                      inArry[i].flags,
                                      inArry[i].VDiskType,
                                      inArry[i].purge,
                                      inArry[i].LUNWWN,
                                      inArry[i].dateTime,
                                      inArry[i].crc,
                                      inArry[i].ParentVDiskID,
                                      inArry[i].sessId,
                                      inArry[i].dataList,
                                      inArry[i].appSystem,
                                      inArry[i].bckpSystem,
                                      inArry[i].VDiskName,
                                      inArry[i].EVAName,
                                      inArry[i].diskPath,
                                      inArry[i].currentDiskPath
                                      );

            if ( charsWritten <= 0 )
            {
                ErhMarkSys (ERH_SYSERR, GetLastError(), __FCN__, ERH_MAJOR);
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("Something went wrong with appending entry for SMIS IRdb"));

                retVal = RET_ERROR;
                goto _lab_bail_out;
            }

            totalCharsWritten += charsWritten;
            totalBytesWritten += charsWritten * sizeof(tchar);
            i++;
        }
           
        /* Beginning new sequence - flush buffer */
        buffer[totalCharsWritten] = '\0';

        retVal = irdbPutFile(irdbType, curr_fname, buffer);
        if ( retVal != RET_OKAY )
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_MAJOR);
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("Something went wrong with writing entry to SMIS IRdb"));

            retVal = RET_ERROR;
            goto _lab_bail_out;
        }

         DbgPlain(DBG_IRDB+100, _T("Upadated IRDBfile: %s"), NSD(curr_fname));
    }

_lab_bail_out:
    FREE(buffer);
    buffer = NULL;
    DbgFcnOutEx(__FLND__, _T("ret=%d"), retVal);
    return(retVal);
}


LIBDBSMIS_API int
dbSMIS_updateReplicas (
    irdbTyp_t irdbType,
    smis_evaDev_t *inArry,
    unsigned int   inArry_cnt
)
{
    tchar curr_fname[STRLEN_PATH+1] = {0};

    ErhAssert( _T("dbSMIS_updateReplicas:inArry=NULL"), inArry != NULL );
    ErhAssert( _T("dbSMIS_updateReplicas:inArry.sessId=NULL"), inArry[0].sessId != NULL );

    sprintf(curr_fname, _T("%s %s"),
            dbSMIS_sessID2fileName(StrFromUserSessionId(inArry[0].sessId)),
            inArry[0].dataList);

    return dbSMIS_updateReplicasNamed( irdbType, curr_fname, inArry, inArry_cnt );
}

/*========================================================================*//**
*
* @brief     Changes reservation of dbsmis entry.
*
* @param     irdbTyp_t irdbType - filter for IR database entries based on disk array type
* @param     tchar *sessID - session id 
* @param     BOOL isReserved - state to which entry will be changed
*
* @retval    RET_ERROR - if something went wrong when retrieving from database
* @retval    1 - if no entries where found
* @retval    RET_OKAY - otherwise
*
*//*=========================================================================*/
LIBDBSMIS_API int dbSMIS_changeReservation (irdbTyp_t irdbType, tchar *sessID, BOOL isReserved)
{
    ERH_FUNCTION (_T("dbSMIS_changeReservation"));
    smis_evaDev_t *devs = NULL;
    tchar pattern [STRLEN_PATH+1] = {0};
    tchar *fileList = NULL;
    int retVal = RET_OKAY;
    int numDevs = 0;
    int i = 0;
    const int ret_NoEntries = 1;

    DbgFcnInEx (__FLND__, _T("sessID = %s, isReserved = %d"), NSD(sessID), isReserved);

    sprintf( pattern, SMIS_FILENAME_FMT, sessID, _T("*"));
    dbSMIS_sessID2fileName (pattern);
    numDevs = dbSMIS_fileScan (irdbType, &fileList, -1, -1, -1, -1, pattern, NULL, &devs, 1);

    if ( 0 > numDevs )
    {
        DbgPlain (DBG_UNEXPECTED, _T("[%s] dbSMIS_fileScan failed. Cannot continue."), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }
    else if ( 0 == numDevs )
    {
        DbgPlain (DBG_IRDB, _T("[%s] No entries found."), __FCN__);
        retVal = ret_NoEntries;
        goto end;
    }

    DbgPlain (DBG_IRDB, _T("[%s] Successfully retrieved the replica information"), __FCN__);

    for (i=0; i < numDevs; i++)
    {
        if (isReserved)
            devs[i].flags |= SMIS_RESERVED_FLAG;
        else
            devs[i].flags &= (~SMIS_RESERVED_FLAG);
    }
    
    if ( RET_OKAY != dbSMIS_updateReplicas (irdbType, devs, numDevs) )
    {
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Could not store the updated information in the database"), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }

    DbgPlain (DBG_IRDB, _T("[%s] Successfully updated the replica information"), __FCN__);

end:
    FREE (devs);
    FREE (fileList);
    DbgFcnOutEx (__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}

/* Prerok, TODO: Temporary setting this function to static to remove a warning. We can
   still consider this for reuse outside of the module. In this case, we should expose
   the function prototype through dbsmis.h - currently there is no need for this.      */
static void dbSMIS_createFilePattern (tchar *outPattern, const tchar *query, int queryType)
{
    ERH_FUNCTION (_T("dbSMIS_createFilePattern"));
    ErhAssert( _T("dbSMIS_createFilePattern:outPattern=NULL"), outPattern != NULL );

    if ( queryType == QUERY_TYPE_PURGED_OBJECTS )
    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("[%s] Creating a purge pattern"), __FCN__);

        if (query && (StrLen (query) > 11) && (query[4] == _T('/')) )
        {
            DbgPlain(DBG_IRDB, _T("[%s] Warning: Using session ID in PURGE query. No filtering."), __FCN__);
            sprintf(outPattern, SMIS_PURGE_FILENAME_FMT, _T("*"), _T("*"));
        }
        else
        {
            sprintf(outPattern, SMIS_PURGE_FILENAME_FMT, _T("*"), NS(query));
        }

        goto end;
    }

    if (query && (StrLen (query) > 11) && (query[4] == _T('/')) )
    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("[%s] Creating a pattern based on session ID"), __FCN__);

        if ( query[10] == _T('-') )
        {
            sprintf( outPattern, SMIS_FILENAME_FMT, StrFromUserSessionId ((tchar*) query), _T("*"));
        }
        else
        {
            sprintf( outPattern, SMIS_FILENAME_FMT, query, _T("*") );
        }
        
        dbSMIS_sessID2fileName( outPattern );
        goto end;
    }

    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("[%s] Creating a pattern based on datalist name"), __FCN__);

        if ( QUERY_TYPE_ALL_OBJECTS == queryType )
        {
            sprintf(outPattern, _T("* %s"), NS(query));
        }
        else
        {
            /* ??? is used for files begining with "MC " */
            sprintf(outPattern, _T("???[0-9]* %s"), NS(query));
        }

        goto end;
    }

end:
    DbgPlain(DBG_IRDB, _T("`-> the pattern is: '%s'"), outPattern);
}


int dbSMIS_deleteReplicas (irdbTyp_t irdbType, const tchar *query, tchar ***deletedSessions, tchar ***excludedSessions, tchar ***failedSessions)
{
    tchar pattern[STRLEN_PATH+1];
    tchar *fileList = NULL;
    tchar **fileArry = NULL;
    smis_evaDev_t dev = {0};
    tchar *fileBuffer = NULL;
    int bufOffset = 0;
    int numFiles = 0;
    int numPurgeFiles = 0;
    int i = 0;
    int countDeleted = 0;
    int countExcluded = 0;
    int countFailed = 0;
    int retVal = RET_OKAY;
    int funRetVal = 0;

    ERH_FUNCTION(_T("dbSMIS_deleteReplicas"));
    DbgFcnInEx(__FLND__, _T("query = '%s'"), NSD(query));

    /* First let's try to delete all valid entries that match the specified query */
    dbSMIS_createFilePattern ( pattern, query, QUERY_TYPE_VALID_OBJECTS );

    numFiles = dbSMIS_fileList (irdbType, &fileList, -1, pattern, &fileArry, 1 );

    if ( numFiles < 0 )
    {
        retVal = RET_ERROR;
        goto end;
    }
    else if ( numFiles == 0 )
    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("Could not find valid entries in SMISA IRdb."));
    }
    
    /*
     * Now that we ensured that we have our entries in pmyEntries, proceed to deletion.
     */

    if ( NULL != deletedSessions)
        (*deletedSessions) = (tchar**) CALLOC (numFiles+1, sizeof(tchar*));
    
    if ( NULL != excludedSessions)
        (*excludedSessions) = (tchar**) CALLOC (numFiles+1, sizeof(tchar*));

    if ( NULL != failedSessions)
        (*failedSessions) = (tchar**) CALLOC (numFiles+1, sizeof(tchar*));

    DbgPlain(DBG_IRDB, _T("[%s] Deleting %d valid entries from SMISA IRdb"), __FCN__, numFiles);

    for (i=0; i<numFiles; i++)
    {
        /* Each iteration should get the next file. dbSMIS_parseEntry will only parse the first     */
        /* entry from the file, but this is OK, because the filtering flags are set on session      */
        /* basis, therefore all entries within the file contain the same flags.                     */
        FREE (fileBuffer);

        if ( RET_OKAY != dbSMIS_parseEntry (irdbType, fileArry[i], NULL, &dev, &fileBuffer, &bufOffset))
        {
            DbgPlain(DBG_IRDB, _T("[%s] Could not parse file '%s', continuing to next file"), __FCN__, fileArry[i]);
            continue;
        }

        if ( SMIS_RESERVED_FLAG & dev.flags )
        {
            DbgPlain(DBG_IRDB, _T("[%s] File '%s' contains an excluded session, continuing to next file."), __FCN__, fileArry[i]);
            if ( NULL != excludedSessions )
                (*excludedSessions)[countExcluded++] = StrNewCopy(dev.sessId);
            continue;
        }

        DbgPlain(DBG_IRDB, _T("[%s] Deleting SMISA IRdb entry %d/%d - '%s'"), __FCN__, i+1, numFiles, fileArry[i]);

        funRetVal = irdbRemFile(irdbType, fileArry[i]);
        if ( funRetVal < 0 )
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED, _T("Problems deleting SMIS IRdb file - irdbRemFile(IRDB TYPE %d, '%s') failed with %d"), irdbType, NSD(fileArry[i]), funRetVal);

            if ( NULL != failedSessions )
                (*failedSessions)[countFailed++] = StrNewCopy(dev.sessId);

            continue;
        }

        if ( NULL != deletedSessions )
            (*deletedSessions)[countDeleted++] = StrNewCopy(dev.sessId);
    }

    FREE (fileList);
    FREE (fileArry);

    /* After deleting valid entries, there may also be some data in the purge bucket */

    dbSMIS_createFilePattern ( pattern, query, QUERY_TYPE_PURGED_OBJECTS );

    numPurgeFiles = dbSMIS_fileList (irdbType, &fileList, -1, pattern, &fileArry, 1 );

    if ( numPurgeFiles < 0 )
    {
        retVal = RET_ERROR;
        goto end;
    }
    else if ( numPurgeFiles == 0 )
    {
        DbgStamp(DBG_IRDB);
        DbgPlain(DBG_IRDB, _T("Could not find valid entries in SMISA IRdb."));
    }
    else
    {
        DbgPlain(DBG_IRDB, _T("[%s] Deleting %d purge entries from SMISA IRdb"), __FCN__, numPurgeFiles);

        if ( NULL != deletedSessions)
        {
            (*deletedSessions) = (tchar**) REALLOC ((*deletedSessions), (numFiles+numPurgeFiles+1)*sizeof(tchar*));
            memset ((*deletedSessions)+numFiles, 0, (numPurgeFiles+1)*sizeof(tchar*));
        }
    
        if ( NULL != excludedSessions)
        {
            (*excludedSessions) = (tchar**) REALLOC ((*excludedSessions), (numFiles+numPurgeFiles+1)*sizeof(tchar*));
            memset ((*excludedSessions)+numFiles, 0, (numPurgeFiles+1)*sizeof(tchar*));
        }

        if ( NULL != failedSessions)
        {
            (*failedSessions) = (tchar**) REALLOC ((*failedSessions), (numFiles+numPurgeFiles+1)*sizeof(tchar*));
            memset ((*failedSessions)+numFiles, 0, (numPurgeFiles+1)*sizeof(tchar*));
        }
    }


    for (i=0; i<numPurgeFiles; i++)
    {
        FREE (fileBuffer);

        if ( RET_OKAY != dbSMIS_parseEntry (irdbType, fileArry[i], NULL, &dev, &fileBuffer, &bufOffset))
        {
            DbgPlain(DBG_IRDB, _T("[%s] Could not parse file '%s', continuing to next file"), __FCN__, fileArry[i]);
            continue;
        }

        if ( 0 == StrCmp(dev.dataList, NS(query)) || 0 == StrCmp (dev.sessId, NS(query)) || 0 == StrCmp(dev.sessId, StrToUserSessionId ((tchar*)(NS(query)))))
        {
            DbgPlain(DBG_IRDB, _T("[%s] Deleting SMISA IRdb entry %d/%d - '%s'"), __FCN__, i+1, numPurgeFiles, fileArry[i]);

            funRetVal = irdbRemFile(irdbType, fileArry[i]);
            if ( funRetVal < 0 )
            {
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("Problems deleting SMIS IRdb file - irdbRemFile(IRDB TYPE %d, '%s') failed with %d"), irdbType, NSD(fileArry[i]), funRetVal);

                if ( NULL != failedSessions )
                    (*failedSessions)[countFailed++] = StrNewCopy(dev.sessId);

                continue;
            }

            if ( NULL != deletedSessions )
                (*deletedSessions)[countDeleted++] = StrNewCopy(dev.sessId);
        }
    }

end: 
    FREE( fileList );
    FREE( fileArry );
    DbgFcnOutEx(__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}


/*========================================================================*//**
*
* @brief     Parses the EVA CA configuration settings.
*
*//*=========================================================================*/
static int dbSMIS_CAconf_parse (tchar *inBuffer, smis_CA_t **drGroupList, int *drGroupCount)
{
    ERH_FUNCTION( _T("dbSMIS_CAconf_parse") );
    int retVal = RET_OKAY;
    tchar *identifier = NULL;
    tchar *evaName = NULL;
    tchar *tmpPtr = NULL;
    BOOL gotoNextLine = FALSE;
    
    DbgFcnInEx(__FLND__, _T("inBuffer = %p, drGroupList = %p, drGroupCount = %p"), inBuffer, drGroupList, drGroupCount);

    if ( NULL == inBuffer || NULL == drGroupList || NULL == drGroupCount )
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid input parameters."), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }

    (*drGroupList) = NULL;
    (*drGroupCount) = 0;

    /* This ensures that there is enough space for the sscanfs in the code */
    identifier = (tchar*) CALLOC (StrLen(inBuffer), sizeof(tchar));
    evaName = (tchar*) CALLOC (StrLen(inBuffer), sizeof(tchar));

    for ( tmpPtr = inBuffer; *tmpPtr; tmpPtr++ )
    {
        /* Jump to the first non-white space character */
        if (isspace(*tmpPtr))
            continue;

        DbgPlain (DBG_DETAIL_ACTION, _T("[%s] Still to process: '%s'"), __FCN__, tmpPtr);

        do
        {
            if ( _T('#') == *tmpPtr )
            {
                gotoNextLine = TRUE;
                break;
            }

            if ( _T('[') == *tmpPtr )
            {
                if ( 1 != sscanf(tmpPtr, _T("[%[a-fA-F0-9]]"), evaName) )
                {
                    DbgPlain (DBG_MAIN_ACTION, _T("[%s] Could not parse EVA name. Could not enclose the ']' around valid characters."), __FCN__);
                    evaName[0] = 0;
                    gotoNextLine = TRUE;
                    retVal = RET_ERROR;
                    break;
                }

                if ( 16 != StrLen(evaName) )
                {
                    DbgPlain (DBG_MAIN_ACTION, _T("[%s] EVA name '%s' should be 16 characters long."), __FCN__, evaName);
                    evaName[0] = 0;
                    gotoNextLine = TRUE;
                    retVal = RET_ERROR;
                    break;
                }

                StrToUpper (evaName);
                DbgPlain (DBG_MAIN_ACTION, _T("[%s] Found EVA name: '%s'"), __FCN__, evaName);
                tmpPtr += StrLen (evaName)+1;
                break;
            }

            if ( _T('"') == *tmpPtr )
            {
                if ( 1 != sscanf(tmpPtr, _T("\"%[^\"]\""), identifier) )
                {
                    DbgPlain (DBG_MAIN_ACTION, _T("[%s] Could not parse the identifier in \"."), __FCN__);
                    gotoNextLine = TRUE;
                    retVal = RET_ERROR;
                    break;
                }

                DbgPlain (DBG_MAIN_ACTION, _T("[%s] Got an identifier: '%s'"), __FCN__, identifier);
                tmpPtr += StrLen (identifier)+1;
                break;
            }

            if ( 1 == sscanf (tmpPtr, _T("%[^ \t\n\r,#]"), identifier) )
            {
                DbgPlain (DBG_MAIN_ACTION, _T("[%s] Got an identifier: '%s'"), __FCN__, identifier);
                tmpPtr += StrLen (identifier)-1;
                break;
            }
        }
        while (FALSE);

        if ( 0 < StrLen (identifier) && 16 == StrLen (evaName))
        {
            if ( 0 == *drGroupCount % SMISDEV_ALLOC_UNIT )
            {
                (*drGroupList) = (smis_CA_t*) REALLOC ((*drGroupList), (*drGroupCount + SMISDEV_ALLOC_UNIT) * sizeof(smis_CA_t));
            }

            StrCopy ((*drGroupList)[*drGroupCount].EVAWWN, evaName, STRLEN_STD);
            StrCopy ((*drGroupList)[*drGroupCount].DRGroup, identifier, STRLEN_STD);
            (*drGroupCount)++;
            identifier[0] = 0;
        }
        else if ( 0 < StrLen (identifier) )
        {
            DbgPlain (DBG_MAIN_ACTION, _T("[%s] Found an identifier, but no EVA has been specified. Ignoring this identifier."), __FCN__);
            retVal = RET_ERROR;
            identifier[0] = 0;
        }

        if ( TRUE == gotoNextLine )
        {
            while (_T('\n') != *tmpPtr && _T('\r') != *tmpPtr && *tmpPtr)
                tmpPtr++;

            /* This is to set off the next increment in the for loop. A bit hackish, but better than using a while loop */
            tmpPtr--;

            gotoNextLine = FALSE;
        }
    }
    
end:
    if (RET_OKAY != retVal)
    {
        FREE (*drGroupList);
        *drGroupCount = 0;
    }
    else
    {
        int i;
        DbgPlain (DBG_MAIN_ACTION, _T("[%s] Returning the following information:"), __FCN__);
        for (i=0; i < *drGroupCount; i++)
        {
            DbgPlain (DBG_MAIN_ACTION, _T("[%s] EVA: '%s', DRGroup: '%s'"), __FCN__, (*drGroupList)[i].EVAWWN, (*drGroupList)[i].DRGroup);
        }
    }

    FREE (evaName);
    FREE (identifier);
    DbgFcnOutEx(__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}


int dbSMIS_CAconf_get (smis_CA_t **drGroupList, int *drGroupCount)
{
    ERH_FUNCTION( _T("dbSMIS_CAconf_get") );
    int retVal = RET_OKAY;
    tchar *fileBuf = NULL;

    DbgFcnInEx(__FLND__, _T("drGroupList = %p, drGroupCount = %p"), drGroupList, drGroupCount);

    if ( NULL == drGroupList || NULL == drGroupCount )
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters specified."), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }

    (*drGroupList) = NULL;
    (*drGroupCount) = 0;

    retVal = irdbGetFile( IRDB_TYPE_SMISCACONF, IRDB_CACONF_FILE, &fileBuf );

    if ( RET_OKAY != retVal || NULL == fileBuf )
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Could not retrieve the CA configuration file. ErhErrno() = %d"), __FCN__, ErhErrno());
        retVal = RET_ERROR;
        goto end;
    }

    retVal = dbSMIS_CAconf_parse (fileBuf, drGroupList, drGroupCount);

    if ( RET_OKAY != retVal )
    {
        DbgPlain (DBG_MAIN_ACTION, _T("[%s] Parsing file failed."), __FCN__);
        goto end;
    }

end:
    FREE (fileBuf);
    DbgFcnOutEx(__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}

int dbSMIS_CAconf_put (tchar *fileBuf, smis_CA_t **drGroupList, int *drGroupCount)
{
    ERH_FUNCTION( _T("dbSMIS_CAconf_put") );
    int retVal = RET_OKAY;

    DbgFcnInEx(__FLND__, _T("drGroupList = %p, drGroupCount = %p"), drGroupList, drGroupCount);
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] File contents:\n== START ==\n%s\n== END ==\n"), __FCN__, fileBuf);

    if ( NULL == drGroupList || NULL == drGroupCount )
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters specified."), __FCN__);
        retVal = RET_ERROR;
        goto end;
    }

    (*drGroupList) = NULL;
    (*drGroupCount) = 0;

    retVal = dbSMIS_CAconf_parse (fileBuf, drGroupList, drGroupCount);

    if ( RET_OKAY != retVal )
    {
        DbgPlain (DBG_MAIN_ACTION, _T("[%s] Parsing file failed. Will not put it on the server."), __FCN__);
        goto end;
    }

    retVal = irdbPutFile( IRDB_TYPE_SMISCACONF, IRDB_CACONF_FILE, fileBuf );

    if ( RET_OKAY != retVal )
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Could not upload the CA configuration file. ErhErrno() = %d"), __FCN__, ErhErrno());
        retVal = RET_ERROR;
        goto end;
    }

end:
    DbgFcnOutEx(__FLND__, _T("retVal = %d"), retVal);
    return retVal;
}

/*========================================================================*//**
* 
* @brief    This function is designed to determine whether the replica is 
*           available or not. It browses dbSMIS for the file having replica 
*           information based on the given session id. Replica is available if
*           file exists.
*
* @param    irdbType    - IN: filter for IR database entries based on disk array type
* @param    sessID      - IN: session id
*
* @return   TRUE if replica is avaiable, FALSE otherwise
*
*//*=========================================================================*/
BOOL 
dbSMIS_IsReplicaAvailable (irdbTyp_t irdbType, const tchar *sessID)
{
    ERH_FUNCTION( _T("dbSMIS_IsReplicaAvailable") );
    BOOL isPresent = FALSE;
    tchar *tmpPtr;
    tchar sessPattern[STRLEN_SESSIONNAME+3];
    tchar* fPattern;
    USES_IRDB_INSTANCE
    if (NULL == sessID)
    {
        return FALSE;
    }

    if (IRDB_INVALID == irdbType)
    {
        return FALSE;
    }

    if ((sessID[10] == _T('-')) &&      /* first check if it's the 'transformable' format */
        ((tmpPtr = StrFromUserSessionId(sessID)) != NULL))   /* Then try to transform it */
    {
        DbgPlain (DBG_DETAIL_PROGTRACE, _T("Using session ID: %s"), tmpPtr);
    }
    else
    {
        /* SessID not in the 'transformable' format. Try using it directly. */
        tmpPtr = (tchar*)sessID;
    }

    sessPattern[0] = _T('\0');
    strncat(sessPattern, tmpPtr,   STRLEN_SESSIONNAME);
    strncat(sessPattern, _T(" *"), 2);
    fPattern = dbSMIS_sessID2fileName(sessPattern);  /* Convert to filesystem format: 2002.02.02... */

    if (fPattern != NULL)
    {
        tchar *fileList = NULL;
        smis_evaDev_t *pEntry = NULL;
        int retVal;

        DbgPlain (DBG_DETAIL_PROGTRACE, _T("Fileglobbing pattern: %s"), fPattern);	 
        
        ThisIrdb->buffOffset = -1;
        retVal = dbSMIS_fileScan(irdbType, &fileList, 1, 1, -1, -1, fPattern, NULL, &pEntry, 1);

        if (retVal > 0)
        {
	        DbgPlain (DBG_DETAIL_PROGTRACE, _T("Replica is available"));
	        isPresent = TRUE;
        }

        FREE(fileList);
        FREE(pEntry);
    }

    return isPresent;
}
