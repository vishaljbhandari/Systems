/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Auditing / API
* @file      db/auditing/auditcmn.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Auditing API
*/

#include "lib/cmn/target.h"

static tchar rcsId[] = _T("$Header: /db/auditing/auditcmn.c $Rev$ $Date::                      $:");

#include "lib/cmn/common.h"
#include "lib/parse/vec.h"

#include "db/dbcmn.h"
#include "db/dberrmac.h"

#include "db/auditing/auditing.h"



/* ============ utility ================= */

int cmnGenerateFilename(const tchar *sessionName, OUT tchar *filename)
{
    int i;

    /* delimiter ' ' and '-' */
    for (i=0; sessionName[i]!=_T(' ') && sessionName[i]!=_T('-') ; i++)
    {
        if (sessionName[i]==_T('/'))
        {
            filename[i]=_T('_');
        }
	    else
        {
            filename[i]=sessionName[i];
        }
    }

    filename[i]=_T('\0');
    filename[i+4]=_T('\0'); /*hmm index*/

    return 0;
}


int cmnGenerateFilenameExtensions(tchar *fileName, slogFileNames *allFileNames, int32 logChangedProtection)
{
    tchar  cmnPath[STRLEN_PATH+1],
           auditDir[STRLEN_PATH+1];


    strcpy(cmnPath, cmnPanLogCM);

    /* QCCR26323 This if is added because auditing not creating entries 
                 in auditing logs on MS cluster shared disk */
    if (*cmnPath != _T('\\'))
    {
       StrCleanPathnameR (cmnPath, 0, auditDir);
    }
    else
    {
        StrCopy(auditDir, cmnPath, STRLEN_PATH);
    }
    strcat(auditDir, AUDIT_WDIR);
    

    strcpy(allFileNames->auditDir, auditDir);
    allFileNames->auditDir[strlen(auditDir)-1]=_T('\0'); /* trailing ... */

    if(logChangedProtection)
    {
        strcpy(allFileNames->protLog, auditDir);
        strcat(allFileNames->protLog, fileName);
        strcat(allFileNames->protLog, PROTECTION_LOG); 
    }
    else
    {
        strcpy(allFileNames->sessionLog, auditDir);
        strcat(allFileNames->sessionLog, fileName);
        strcat(allFileNames->sessionLog, SESSION_LOG);
    
        strcpy(allFileNames->objectsLog, auditDir);
        strcat(allFileNames->objectsLog, fileName);
        strcat(allFileNames->objectsLog, OBJECTS_LOG);
     
        strcpy(allFileNames->mediaLog, auditDir);
        strcat(allFileNames->mediaLog, fileName);
        strcat(allFileNames->mediaLog, MEDIA_LOG);
    }

    return 0;
}
/*============== utility end ==================== */



/* pack string data in a linear buffer to be later stored */
tchar *cmnAuditPackData(DbGlobals *dbGl, tchar *buffer, dpint64_t *blockSize, ...)
{
    tchar    pNAstring[]=_T("N/A"); 
    va_list  paramList;
    size_t     allocSize;
    ERH_FUNCTION (_T("cmnAuditPackData"));
    

  
    va_start(paramList, blockSize);     
               
    allocSize = *blockSize;

    /* pack all parameters */
    while (1)
    {
        tchar   *curString=NULL;
        size_t   strLen;

        if ( (curString = va_arg( paramList, tchar*)) == NULL )
        {
            /* no more parameters */
            break;
        }

        strLen = strlen(curString);
        if (!strLen)
        {
            /*empty string*/
            curString = pNAstring;
            strLen = strlen(pNAstring);
        }
        if ( NULL == (buffer = REALLOC(buffer, sizeof(tchar) * (allocSize += strLen+1))))
        {
            va_end(paramList);
            return(buffer);
        }
        
        DbgPlain (DBG_AUDIT_LOW, _T("[%s] buffer=%p curString='%s' strLen=%lld"),
            __FCN__, (void *)buffer, curString, (dpint64_t)strLen);

        strcpy (buffer+(*blockSize), curString);
        *blockSize=allocSize;
    } /* while (1) */

    
    DbgPlain(DBG_AUDIT_LOW, _T("[%s] *blockSize=%lld buffer='%s'"), __FCN__, *blockSize, buffer);
    va_end(paramList);

    return (buffer);
}


/* merge source string into target string inside the subbuffer - with source removal */
int cmnAuditMergeData(
    DbGlobals *dbGl,
    tchar     **buffer,
    size_t    *startBlockSize,
    size_t    *allocSize,
    int       targetPos,
    int       sourcePos,
    int       noStartParams
)
{
    int     i;
    tchar   *pTraverse,
            *newBuffer,
            *pTargetBuf;

    tchar   *pToSource = NULL;
  
    size_t  targetSize = 0,
            sourceSize = 0; 
  
    size_t  newAllocSize;
    ERH_FUNCTION (_T("cmnAuditMergeData"));



    /* first traverse to find source and target positions and sizes */
    pTraverse = *buffer;
    for (i=0; pTraverse < (*buffer + *allocSize); i++)
    {
        if ( i == targetPos ) 
        { 
            targetSize = strlen(pTraverse);
        }
        else if ( i == sourcePos)
        {
            /* copy source string to temporary */
            sourceSize = strlen(pTraverse);
            pToSource  = pTraverse;
        }

        DbgPlain(DBG_AUDIT_LOW, _T("[%s] merge source buffer: %s"), __FCN__, pTraverse);
        pTraverse += strlen(pTraverse)+1; 
    } /* for... */


    /* allocate new buffer for copy */
    newAllocSize = *allocSize - (targetSize+1);
    newBuffer = MALLOC (newAllocSize * sizeof(tchar));
    if (newBuffer == NULL)
    {
        return -1;          
    }


    /* retraverse and copy correct strings */
    pTraverse  = *buffer;
    pTargetBuf = newBuffer;
    for (i=0; pTraverse < (*buffer + newAllocSize); i++)
    {
         if ( i == targetPos )
         {
             /* overwrite with source */
             memcpy(pTargetBuf, pToSource, (sourceSize+1) * sizeof(tchar) ); 
         }
         else if ( i != sourcePos ) 
         {
             /* source is ignored, all the rest is copyed */
             memcpy(pTargetBuf, pTraverse, (strlen(pTraverse)+1) * sizeof(tchar) );
         }

         if ( i == noStartParams )
         {
             /* correct position of start block size marker */
             DbgPlain(DBG_AUDIT_LOW, _T("[%s] merge starting block size=%lu new=%lu"),
                __FCN__, (unsigned long)*startBlockSize, (unsigned long)(pTargetBuf-newBuffer));
             *startBlockSize = pTargetBuf - newBuffer;
         }

        DbgPlain(DBG_AUDIT_LOW, _T("[%s] merge target buffer: %s "), __FCN__, pTargetBuf);
        pTargetBuf += strlen(pTargetBuf)+1;
        pTraverse  += strlen(pTraverse)+1;
    } /* for... */


    FREE(*buffer);
    *allocSize = newAllocSize;
    *buffer    = newBuffer;

    /* all OK */
    return 0;  
}



/* write data into appropriate log */
int cmnAuditWriteLog(DbGlobals *dbGl, tchar *logFileName, tchar *header, tchar *buffer)
{
    FILEHANDLE fHandle;
    size_t     fileOffset = 0;
    int        ret=0;
    ERH_FUNCTION (_T("cmnAuditWriteLog"));


    /* check buffers */
    if (!header || !buffer)
    {
        return -1;
    }
     
    /*open auditing log file for writing */
    fHandle = OB2_OpenFile(logFileName, 1 /*GENERIC_WRITE*/ , 0);
    if (fHandle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    if (OB2_LockFile (fHandle, F_LOCK, 0, 0)< 0)
    {
        DbgPlain(DBG_AUDIT, _T("[%s] error could not lock %s "), __FCN__, logFileName);
        OB2_CloseFile(fHandle);
        return -1;
    }

    DbgPlain(DBG_AUDIT, _T("[%s] Writing to log '%s', block size=%lu, headersize=%lu"),
                __FCN__,
                logFileName,
                (unsigned long)*(((size_t *)header)+1),
                (unsigned long)AUDIT_HEADER_SIZE 
            );

    /* write header and buffer */
    fileOffset = OB2_SeekFile(fHandle, 0, SEEK_END);
    if (OB2_WriteFile(fHandle, header, AUDIT_HEADER_SIZE)==-1 ||
        OB2_WriteFile(fHandle, buffer, *(((size_t *)header)+1) * sizeof(tchar) )==-1)
    {
        ret = -1;
    }

    OB2_CloseFile(fHandle);

    /* FIXME: fileOffset returned is always >= 0 even if OB2_WriteFile() fails! */

    return fileOffset;
}  /* cmnAuditWriteLog */




/* find current file offset */
size_t cmnAuditFindOffset(DbGlobals   *dbGl, tchar *logFileName)
{
    FILEHANDLE fHandle;
    size_t     fileOffset = 0;

    /* FIXME: why are we opening a file? OB2_StatFile()? */

    /*open auditing log file for writing */
    fHandle = OB2_OpenFile(logFileName, 1 /*GENERIC_WRITE*/ , 0);
    if (fHandle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
       
    fileOffset = OB2_SeekFile(fHandle, 0, SEEK_END);
    OB2_CloseFile(fHandle);

    return fileOffset;
}  /* cmnAuditWriteLog */





/* check and create logs */
int cmnAuditTouchLogs(DbGlobals   *dbGl, slogFileNames *allFileNames)
{
    int         ret=0,
                enablePurge=0;
    FILEHANDLE  fHandle;
    ERH_FUNCTION (_T("cmnAuditTouchLogs"));
    


    /* FIXME: non-existant error handling! */
    /* FIXME: if something fails, an error should be returned not enablePurge */

    /* test if auditing dir is already present */  
    ret = OB2_StatFile(allFileNames->auditDir, NULL);
    if (ret<0)
    {
        /*create auditing dir*/
        DbgPlain(DBG_AUDIT, _T("[%s] Creating directory '%s'"), __FCN__, allFileNames->auditDir);
        ret = OB2_CreateDirectory(allFileNames->auditDir, 0700);
    }

    if(glLogChangedProtection) /* BE:  protection time enhancement */
    {
        ret = OB2_StatFile(allFileNames->protLog, NULL);
        if (ret<0)
        {
            DbgPlain(DBG_AUDIT, _T("[%s] Creating protection log '%s'"), __FCN__, allFileNames->protLog);

            fHandle = OB2_OpenFile(allFileNames->protLog, 1 /*GENERIC_WRITE*/ , 1 /*create it*/);
            OB2_CloseFile(fHandle);
        }
        return 0;
    }

    DbgPlain(DBG_AUDIT, _T(" Test logs! \n %s \n %s \n %s \n"), 
             allFileNames->sessionLog,
             allFileNames->objectsLog,
             allFileNames->mediaLog);


    /* test the existance of log files for the current date and eventualy create them */  


    /*
        FIXME: OB2_StatFile() returns -1 if not found, -2 on other error and
        that should be handled differently (don't create files if -2!)
    */

    /* ses */
    ret = OB2_StatFile(allFileNames->sessionLog, NULL);
    if (ret<0)
    {
        DbgPlain(DBG_AUDIT, _T("[%s] Creating session log '%s'"), __FCN__, allFileNames->sessionLog);

        /*create auditing file*/
        fHandle = OB2_OpenFile(allFileNames->sessionLog, 1 /*GENERIC_WRITE*/ , 1 /*create it*/);
        OB2_CloseFile(fHandle);
        enablePurge=1;
    }

    /* obj */  
    ret = OB2_StatFile(allFileNames->objectsLog, NULL);
    if (ret<0)
    {
        DbgPlain(DBG_AUDIT, _T("[%s] Creating object log '%s'"), __FCN__, allFileNames->objectsLog);
        /*create auditing file*/
        fHandle = OB2_OpenFile(allFileNames->objectsLog, 1 /*GENERIC_WRITE*/ , 1 /*create it*/);
        OB2_CloseFile(fHandle);
        enablePurge=1;
    }

    /* med */
    ret = OB2_StatFile(allFileNames->mediaLog, NULL);
    if (ret<0)
    {
        DbgPlain(DBG_AUDIT, _T("[%s] Creating media log '%s'"), __FCN__, allFileNames->mediaLog);
        /*create auditing file*/
        fHandle = OB2_OpenFile(allFileNames->mediaLog, 1 /*GENERIC_WRITE*/ , 1 /*create it*/);
        OB2_CloseFile(fHandle);
        enablePurge=1;
    }

	return enablePurge=1;
}


