/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   SSEA_Messages
* @file      integ/smb/sse/smbmsg.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     SMB Message interface functions
*
* @since     27.05.99  Aleksander Skrlj   Initial coding
*/

#include "ssermlib.h" /* This should be included before target.h */
#include "lib/cmn/target.h"
#include "lib/parse/vec.h"


#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/smb/sse/smbmsg.c $Rev$ $Date::                      $:") ;
#endif


/* Application specific includes */

/* OB common includes */
#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "integ/smb/libsmb.h"
#include "integ/smb/smb.h"
#include "integ/smb/smbcmn.h"
#include "lib/smbdb/irdbcmn.h"
#include "lib/smbdb/dbxp/libdbxp.h"
#include "integ/smb/libsmbdbsys2.h"

/* Module includes */
#include "ssedef.h"
#include "sse.h"
#include "ssedbxp.h"
#include "smbmsg.h"
#include "smbmsgbckp.h"
#include "smbmsgrest.h"
#include "smbmsgir.h"
#include "ssegen.h"
#include "ssebackup.h"
#include "sserestore.h"
#include "sseir.h"
#include "sseapi.h"
#include "optmgr.h"
#include "nls.h"

#include "debug.h"

/*added by ashi.. */
#include "optmgr.h" 

/* Globals */

extern PSSE_ARRAY_T pSseArray;  /* HP SureStoreE Database */

/* WARNING! THIS SHOULD BE REMOVED */
extern PSMB_DB_T pSmbDb;
extern int DmpMode;

int    skipReport     =  0;  /* Set if last message should be sent to CON   */
static tchar   *runScriptOutput; /* output of script */

int    ConIpcHandle   = -1;  /* IPC handle for Smb-r1 <--> CON channel     */
int    SmbIpcHandle  = -1;  /* IPC handle for Smb-r1 <--  Smb-r2 channel */
int    ConDA          =  0;  /* Number of already connected DA to Smb-r1   */
int    SmbState      = STATE_IDLE;
/*
#define MAX_SMB_AGENTS 1
*/
#define CLOSE_EVENT          0   /* Unexpected close on socket */
#define READ_ERROR           1   /* Read error on socket       */
#define PARSE_ERROR          2   /* Message parsing error      */
#define INVALID_HANDLE      -1   /* ipc invalid handle         */

/*
struct datable {
    tchar     *Host;
    int       Port;
    int       Used;
    int       Closed;
    int       SmbID;
    IpcHandle Handle;
    int       Type;
    int       WaitForAckFromSMB;
    int       WaitForAckFromCON;

} SMBtable[MAX_SMB_AGENTS];
*/
#define StrMsgAdjustOutbound(_buf)
static int      alrm_errno;


/* Functions */

extern const tchar *
MsgPrintMSGConstant(
    int iMsgID
)
{
    ERH_FUNCTION (_T("MsgPrintConstant"));    
    switch (iMsgID) {
        case MSG_STOP:                  
            return (_T("MSG_STOP"));
        case MSG_ABORT:                     
            return (_T("MSG_ABORT"));

        case MSG_SMB_RESOLVE:               
            return (_T("MSG_SMB_RESOLVE"));
        case MSG_SMB_BACKUP_SPLIT:          
            return (_T("MSG_SMB_BACKUP_SPLIT"));
        case MSG_SMB_PREPARE_R2:            
            return (_T("MSG_SMB_PREPARE_R2"));
        case MSG_SMB_BACKUP_RESUME:         
            return (_T("MSG_SMB_BACKUP_RESUME"));

        case MSG_SMB_RESTORE_RESOLVE:       
            return (_T("MSG_SMB_RESTORE_RESOLVE"));
        case MSG_SMB_RESTORE_SPLIT:         
            return (_T("MSG_SMB_RESTORE_SPLIT"));
        case MSG_SMB_RESTORE_RESUME:        
            return (_T("MSG_SMB_RESTORE_RESUME"));
        case MSG_SMB_RESTORE_PREPARE_R2:    
            return (_T("MSG_SMB_RESTORE_PREPARE_R2"));

        case REP_SMB_RESOLVE:               
            return (_T("REP_SMB_RESOLVE"));
        case REP_SMB_BACKUP_SPLIT:          
            return (_T("REP_SMB_BACKUP_SPLIT"));
        case REP_SMB_PREPARE_R2:            
            return (_T("REP_SMB_PREPARE_R2"));
        case REP_SMB_BACKUP_RESUME:         
            return (_T("REP_SMB_BACKUP_RESUME"));

        case REP_SMB_RESTORE_RESOLVE:       
            return (_T("REP_SMB_RESTORE_RESOLVE"));
        case REP_SMB_RESTORE_SPLIT:         
            return (_T("REP_SMB_RESTORE_SPLIT"));
        case REP_SMB_RESTORE_RESUME:        
            return (_T("REP_SMB_RESTORE_RESUME"));
        case REP_SMB_RESTORE_PREPARE_R2:    
            return (_T("REP_SMB_RESTORE_PREPARE_R2"));

        case MSG_RESLOCK_REQUEST_STATUS:
            return (_T("MSG_RESLOCK_REQUEST_STATUS"));
            
        default: 
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Unknown message ID %d!"),
                __FCN__, iMsgID);
            return (_T("UNKNOWN"));
    }
}

extern const tchar *
MsgPrintFUNRESConstant(
    int iMsgID
)
{
    ERH_FUNCTION (_T("MsgPrintFUNRESConstant"));    
    switch (iMsgID) {
        case MSG_STOP:          return (_T("MSG_STOP"));
        case MSG_ABORT:         return (_T("MSG_ABORT"));

        case SMB_FUN_RESOLVE:           return (_T("SMB_FUN_RESOLVE"));
        case SMB_FUN_SYNC:              return (_T("SMB_FUN_SYNC"));
        case SMB_FUN_SPLIT:             return (_T("SMB_FUN_SPLIT"));
        case SMB_FUN_RESUME:            return (_T("SMB_FUN_RESUME"));
        case SMB_FUN_PREEXEC:           return (_T("SMB_FUN_PREEXEC"));
        case SMB_FUN_POSTEXEC:          return (_T("SMB_FUN_POSTEXEC"));

        case SMB_FUN_RESTORE_RESOLVE:   return (_T("SMB_FUN_RESTORE_RESOLVE"));
        case SMB_FUN_RESTORE_SYNC:      return (_T("SMB_FUN_RESTORE_SYNC"));
        case SMB_FUN_RESTORE_SPLIT:     return (_T("SMB_FUN_RESTORE_SPLIT"));
        case SMB_FUN_RESTORE_PREEXEC:   return (_T("SMB_FUN_RESTORE_PREEXEC"));
        case SMB_FUN_RESTORE_POSTEXEC:  return (_T("SMB_FUN_RESTORE_POSTEXEC"));
        case SMB_FUN_RESTORE_RESUME:    return (_T("SMB_FUN_RESTORE_RESUME"));

        case SMB_RES_RESOLVE:           return (_T("SMB_RES_RESOLVE"));
        case SMB_RES_SYNC:              return (_T("SMB_RES_SYNC"));
        case SMB_RES_SPLIT:             return (_T("SMB_RES_SPLIT"));
        case SMB_RES_RESUME:            return (_T("SMB_RES_RESUME"));
        case SMB_RES_PREEXEC:           return (_T("SMB_RES_PREEXEC"));
        case SMB_RES_POSTEXEC:          return (_T("SMB_RES_POSTEXEC"));

        case SMB_RES_RESTORE_RESOLVE:   return (_T("SMB_RES_RESTORE_RESOLVE"));
        case SMB_RES_RESTORE_SYNC:      return (_T("SMB_RES_RESTORE_SYNC"));
        case SMB_RES_RESTORE_SPLIT:     return (_T("SMB_RES_RESTORE_SPLIT"));
        case SMB_RES_RESTORE_PREEXEC:   return (_T("SMB_RES_RESTORE_PREEXEC"));
        case SMB_RES_RESTORE_POSTEXEC:  return (_T("SMB_RES_RESTORE_POSTEXEC"));
        case SMB_RES_RESTORE_RESUME:    return (_T("SMB_RES_RESTORE_RESUME"));

        default: 
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] Unknown message ID %d!"),
                __FCN__, iMsgID);
            return (_T("UNKNOWN"));
    }
}

int
ParseSmMsg (
    int msgType
)
{
    int retVal = RETURN_OK;
    tchar *buffer [SMB_MSIZE_SM];

    ERH_FUNCTION (_T("ParseSmMsg"));
    DbgFcnIn();

    {
        StrParseStart (USE_GLOBAL_BUFFER);
    
        while (StrParseMessage (SMB_MSIZE_SM, buffer)!=-1)
        {
            int objStat, valid;
            
            DbgPlain (DBG_SSE_MSG, 
                _T("[%s] %s, %s, %s"), __FCN__, 
                buffer[0], buffer[1], buffer[2]);

            StrAtoi (buffer[2], &objStat, &valid);

            if (!valid)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid status %s"), 
                    __FCN__, buffer[2]);
                /*
                ErhFullReport (__FCN__, ERH_MAJOR,
                    NlsGetMessage (NLS_SET_SMB,
                        NLS_MSG_UNEXPECTED_OBJECT_STATUS,
                        buffer[2], buffer[0]));
                        */

                objStat = SMB_OS_ERROR;
                retVal = RETURN_ER;
            }
            switch (msgType)
            {
                case MSG_SMB_RESOLVE:
                case MSG_SMB_RESTORE_RESOLVE:
                    retVal = AddBckpObject (
                        pSmbDb, buffer[0], buffer[1], SMB_OS_OK);
                    break;

                default:
                    if (objStat != SMB_OS_OK)
                    {
                        DbgPlain (DBG_SSE_MSG, 
                            _T("[%s] buffer[0] = '%s'   objStat = '%d'"), 
                            __FCN__, buffer[0], objStat);
                    
                        SmbR1ChangeStatus(buffer[0], objStat);
                    }
                    break;
            }
        }
    }

    /*SmbPrintDb(pSmbDb);*/
    if (retVal != RETURN_OK)
        pSmbDb->iStatus = retVal;
    /*SmbUpdateStatus(pSmbDb);*/

    DbgFcnOutRet(pSmbDb->iStatus);
    return (pSmbDb->iStatus);

} /* ParseSmMsg () */

int
ParseR1Msg (
    int msgType
)
{
    int retVal = RETURN_OK;
    tchar **buffer = NULL;

    ERH_FUNCTION (_T("ParseR1Msg"));

    DbgFcnInEx(__FLND__, _T("Msg: %s (%d)"), 
       MsgPrintFUNRESConstant (msgType), msgType);

    switch (msgType)
    {
        case SMB_FUN_RESOLVE:
        case SMB_FUN_RESTORE_RESOLVE: 
            SmbGenR2AddObj (pSmbDb);
            break;
        case SMB_FUN_RESUME:
            {
                int msg_id = StrParseStart (USE_GLOBAL_BUFFER);
                tchar *sessCount = StrParseNext();
                tchar **sessIds = NULL;
                int i;
                
                DbgPlain(DBG_SSE_MSG, _T("msg_id:%d, sessCount: %s"), msg_id, NS(sessCount));   
                
                for (i = 0; i<atoi(sessCount); i++)
                {
                    tchar *sessID=NULL;
                    DbgPlain(DBG_SSE_MSG, _T("i: %d"), i);   
                    if ((sessID=StrParseNext ()) == NULL)
                    {
                        ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED, 
                            _T("[%s] Expected message is empty!"), __FCN__);

                        retVal = RETURN_ER;
                        goto end;
                    }
                    else
                    {
                        sessIds = (tchar**) REALLOC (sessIds, (i+1)*sizeof(tchar *) );
                        sessIds[i] = StrNewCopy(sessID);
                        DbgPlain(DBG_SSE_MSG, _T("sessIds[%d]: %s"), i, NS(sessIds[i]));   
                    }
                }
                for (i = 0; i<atoi(sessCount); i++)
                {
                    DbgPlain(DBG_SSE_MSG, _T("Disable sessID: %s"), NS(sessIds[i]));   
                    retVal = SSEA_DisableBs (sessIds[i]);
                    if ( RETURN_OK == retVal )
                        retVal = SmbDbSys_Remove (sessIds[i]);
                    
                    if (retVal != RETURN_OK)
                    {
                        int j=0;

                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                              _T("[%s] SSEA_DisableBs failed!"), __FCN__);
                        
                        for (j=i; j<atoi(sessCount); j++)
                        {
                            FREE(sessIds[j]);
                        }

                        FREE(sessIds);
                        
                        goto end;
                    }
                    FREE(sessIds[i]);
                }

                FREE(sessIds);
               
            }
            break;
        case SMB_FUN_SPLIT:
        case SMB_FUN_RESTORE_SPLIT:
        case SMB_FUN_PREEXEC:
        case SMB_FUN_RESTORE_PREEXEC:
        case SMB_FUN_POSTEXEC:
        case SMB_FUN_RESTORE_POSTEXEC:
        case SMB_FUN_RESTORE_RESUME:
            if ((buffer = (tchar **) MALLOC (SMB_MSIZE_FUN * sizeof (tchar *)))
                != NULL) {
                while (StrParseMessage (SMB_MSIZE_FUN,buffer)!=-1)
                    SmbR2UpdStatus (buffer);
                FREE (buffer);
            }
            break;
        default:
            DbgStamp (DBG_SSE_MSG);
            break;
    }

end:

    if (retVal != RETURN_OK)
        pSmbDb->iStatus = retVal;
    /*SmbUpdateStatus(pSmbDb);*/

    DbgFcnOutRet(pSmbDb->iStatus);
    return (pSmbDb->iStatus);

} /* ParseR1Msg () */

int
ParseR2Msg (
    int msgType
)
{
    int retVal = RETURN_OK;
    tchar *tsBackHost;
    tchar *tsToken;
    int i;

    ERH_FUNCTION (_T("ParseR2Msg"));
    DbgFcnInEx(__FLND__, _T("Msg: %s (%d)"), 
       MsgPrintFUNRESConstant (msgType), msgType);

    StrParseStart (USE_GLOBAL_BUFFER);

    /* parseR2Msg for ir session */
    if (opt.irsession)
    {
        switch (msgType)
        {
            case SMB_RES_RESTORE_RESOLVE:
                if ((tsBackHost=StrParseNext ()) == NULL)
                { 
                    ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                       _T("[%s] Expected message is empty!"), __FCN__);

                    retVal = RETURN_ER;
                    goto end;
                }
                
                strncpy (pSmbDb->tsBackHost, tsBackHost, STRLEN_PATH);
                opt.bckpHost = StrNewCopy (tsBackHost);

                if ((tsToken=StrParseNext ()) == NULL)
                {
                    ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] Expected message is empty!"), __FCN__);

                    retVal = RETURN_ER;
                    goto end;
                } 

                DbgPlain (DBG_SSE_ACTION, 
                  _T("[%s] pSmbDb->iStatus = %s"), 
                  __FCN__, 
                  pSmbDb->iStatus == RETURN_OK ? T_RETURN_OK : T_RETURN_ER);
                  pSmbDb->iStatus = atoi (tsToken);
                  DbgPlain (DBG_SSE_ACTION, 
                     _T("[%s] pSmbDb->iStatus = %s"), 
                     __FCN__, 
                     pSmbDb->iStatus == RETURN_OK ? T_RETURN_OK : T_RETURN_ER);

                  if ((retVal = R1MsgParseSysObjects (pSmbDb->pSysObjects)) 
                      != RETURN_OK)
                  {
                      DbgStamp (DBG_UNEXPECTED);
                      DbgPlain (DBG_UNEXPECTED,
                       _T("[%s] R1MsgParseSysObjects failed!"), __FCN__);
                      retVal = RETURN_ER;
                      goto end;
                  }
                  
                  if ((retVal = R1UpdateObjects (pSmbDb->pObjects, 
                              pSmbDb->iObjectCnt)) != RETURN_OK)
                  {
                      DbgStamp (DBG_UNEXPECTED);
                      DbgPlain (DBG_UNEXPECTED,
                         _T("[%s] R1UpdateObjects failed!"), __FCN__);
                      retVal = RETURN_ER;
                      goto end;
                  }
                  if ((retVal = SmbUpdateStatus(pSmbDb)) != RETURN_OK)
                  {
                      DbgStamp (DBG_UNEXPECTED);
                      DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] R1UpdateStatus failed!"), __FCN__);
                      retVal = RETURN_ER;
                      goto end;
                  }

                  break;
            case SMB_RES_RESTORE_SYNC:
            case SMB_RES_RESTORE_SPLIT:
            case SMB_RES_RESTORE_RESUME:
            case SMB_RES_RESTORE_PREEXEC:
            case SMB_RES_RESTORE_POSTEXEC:
                if ((tsBackHost=StrParseNext ()) == NULL)
                {
                    ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] Expected message is empty!"), __FCN__);

                    retVal = RETURN_ER;
                    goto end;
                } 
                
                if ((tsToken=StrParseNext ()) == NULL)
                {
                    ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                      _T("[%s] Expected message is empty!"), __FCN__);

                    retVal = RETURN_ER;
                    goto end;
                }

                pSmbDb->iStatus = atoi (tsToken);

                if ((retVal = R1MsgParseSysObjectsStatus (pSmbDb->pSysObjects)) 
                    != RETURN_OK)
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                      _T("[%s] R1MsgParseSysObjects failed!"), __FCN__);
                    retVal = RETURN_ER;
                    goto end;
                }
        
                if ((retVal = R1MsgParseRdskAll (pSmbDb->pSysObjects->pRdsk, 
                                           pSmbDb->pSysObjects->iRdskCount)) 
                    != RETURN_OK)
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                              _T("[%s] R1MsgParseRdskAll failed!"), 
                              __FCN__);
                    goto end;
                }
        
                for (i=0; i<pSmbDb->pSysObjects->iRdskCount; i++)
                {
                    DbgStamp (DBG_SSE_ACTION);
                    DbgPlain (DBG_SSE_ACTION,
                      _T("[%s]  crc = %x\n!"), 
                      __FCN__,pSmbDb->pSysObjects->pRdsk[i]->crc);
                }
        
                /*SmbUpdateStatus(pSmbDb);*/

                if ((retVal = SmbUpdateStatus(pSmbDb)) != RETURN_OK)
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                      _T("[%s] R1UpdateStatus failed!"), __FCN__);
                    retVal = RETURN_ER;
                    goto end;
                }

                break;
            default:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] Unexpected message %s (%d)"), __FCN__, 
                MsgPrintFUNRESConstant (msgType), msgType);
                break;
        }
        goto end;
    }
 
    /* parseR2Msg for backup or split mirror restore session */
    switch (msgType)
    {
        case SMB_RES_RESTORE_PREEXEC:
        {
            if ((tsBackHost=StrParseNext ()) == NULL)
            {
                ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Expected message is empty!"), __FCN__);
    
                retVal = RETURN_ER;
                goto end;
            }
    
            strncpy (pSmbDb->tsBackHost, tsBackHost, STRLEN_PATH);
            opt.bckpHost = StrNewCopy (tsBackHost);
    
            if ((tsToken=StrParseNext ()) == NULL)
            {
                ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, 
                    _T("[%s] Expected message is empty!"), __FCN__);
    
                retVal = RETURN_ER;
                goto end;
            }
    
            DbgPlain (DBG_SSE_ACTION, 
                _T("[%s] pSmbDb->iStatus = %s"), __FCN__, pSmbDb->iStatus == RETURN_OK ?
                T_RETURN_OK : T_RETURN_ER);
            pSmbDb->iStatus = atoi (tsToken);
            DbgPlain (DBG_SSE_ACTION, 
                _T("[%s] pSmbDb->iStatus = %s"), __FCN__, pSmbDb->iStatus == RETURN_OK ?
                T_RETURN_OK : T_RETURN_ER);
    
            if ((retVal = R1MsgParseSysObjects (pSmbDb->pSysObjects)) != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                _T("[%s] R1MsgParseSysObjects failed!"), __FCN__);
                retVal = RETURN_ER;
                goto end;
            }
            if ((retVal = R1UpdateObjects (pSmbDb->pObjects, pSmbDb->iObjectCnt))
                != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                _T("[%s] R1UpdateObjects failed!"), __FCN__);
                retVal = RETURN_ER;
                goto end;
            }
            if ((retVal = SmbUpdateStatus(pSmbDb)) != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                _T("[%s] R1UpdateStatus failed!"), __FCN__);
                retVal = RETURN_ER;
                goto end;
            }
            break;
      }
      case SMB_RES_RESOLVE:
      case SMB_RES_RESTORE_RESOLVE:
        break;

      case SMB_RES_SYNC:
      case SMB_RES_RESTORE_SYNC:
      case SMB_RES_SPLIT:
      case SMB_RES_RESTORE_SPLIT:
      case SMB_RES_RESUME:
      case SMB_RES_RESTORE_RESUME:
      case SMB_RES_PREEXEC:
      case SMB_RES_POSTEXEC:
      case SMB_RES_RESTORE_POSTEXEC:
      {
        if ((tsBackHost=StrParseNext ()) == NULL)
        {
            ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Expected message is empty!"), __FCN__);

            retVal = RETURN_ER;
            goto end;
        }

        if ((tsToken=StrParseNext ()) == NULL)
        {
            ErhMark (ERH_INVALID_MSG, __FCN__, ERH_INTERNAL);
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, 
                _T("[%s] Expected message is empty!"), __FCN__);

            retVal = RETURN_ER;
            goto end;
        }

        pSmbDb->iStatus = atoi (tsToken);
        
        if (msgType == SMB_RES_PREEXEC)
        {
            if ((retVal = R1MsgParseSysObjects (pSmbDb->pSysObjects)) 
            != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] R1MsgParseSysObjects failed!"), __FCN__);
                retVal = RETURN_ER;
                goto end;
            }
        }
        

        if ((retVal = R1MsgParseSysObjectsStatus (pSmbDb->pSysObjects)) 
            != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] R1MsgParseSysObjectsStatus failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }

        if (msgType == SMB_RES_PREEXEC)
        {
            strncpy (pSmbDb->tsBackHost, tsBackHost, STRLEN_PATH);
            opt.bckpHost = StrNewCopy (tsBackHost);
            
            if ((retVal = R1UpdateObjects (pSmbDb->pObjects, pSmbDb->iObjectCnt)) != RETURN_OK)
            {
               DbgStamp (DBG_UNEXPECTED);
               DbgPlain (DBG_UNEXPECTED,
               _T("[%s] R1UpdateObjects failed!"), __FCN__);
               retVal = RETURN_ER;
               goto end;
            }
        }
        
        
        if ((retVal = R1MsgParseRdskAll (pSmbDb->pSysObjects->pRdsk, 
                                         pSmbDb->pSysObjects->iRdskCount)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] R1MsgParseRdskAll failed!"), 
                      __FCN__);
            goto end;
        }
        for (i=0; i<pSmbDb->pSysObjects->iRdskCount; i++)
        {
            DbgStamp (DBG_SSE_ACTION);
            DbgPlain (DBG_SSE_ACTION,
                      _T("[%s]  crc = %x\n!"), __FCN__,pSmbDb->pSysObjects->pRdsk[i]->crc);
        }

        /*SmbUpdateStatus(pSmbDb);*/

        if ((retVal = SmbUpdateStatus(pSmbDb)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] R1UpdateStatus failed!"), __FCN__);
            retVal = RETURN_ER;
            goto end;
        }

        /* 
            QXCR1000781475: if Rdsks are marked as error because they belong to a VxVM group (not supported at HP-UX 11.31 with 
            legacy DSF disabled) mark their corresponding XP disks as invalid so they will not be resynced.
        */
        for (i=0; i<pSmbDb->pSysObjects->iRdskCount; i++)
        {
            if (RDSK_ER == pSmbDb->pSysObjects->pRdsk[i]->status)
            {
                DbgPlain(DBG_SSE_ACTION, _T("[%s] Status pRdsk[%d] is error, setting status of XP disk to invalid"), __FCN__, i);
                pSseArray->pSrcDevices[i]->iStatus = LDEV_INVALID;
            }
        }

#if TARGET_UNIX
        if (opt.instant_restore && pSmbDb->pSysObjects->iDgCount>0)
        {
            for (i=0; i<pSmbDb->pSysObjects->iDgCount; i++)
            {
                if (SMB_OS_ERROR == pSmbDb->pSysObjects->pDg[i]->iStatus)
                {
                    DbgPlain(DBG_SSE_ACTION, _T("[%s] Status of DG[%d] is not OK"), __FCN__, i);
                    retVal = RETURN_ER;
                    goto end;
                }
            }
        }
#endif

        break;
      }
      default:
      {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] Unexpected message %s (%d)"), __FCN__, 
            MsgPrintFUNRESConstant (msgType), msgType);
        break;
      }
    }
end:
    DbgPlain (DBG_SSE_ACTION, 
        _T("[%s] pSmbDb->iStatus = %s"), __FCN__, pSmbDb->iStatus == RETURN_OK ?
        T_RETURN_OK : T_RETURN_ER);
    if (pSmbDb->iStatus == RETURN_OK)
        pSmbDb->iStatus = retVal;
    DbgPlain (DBG_SSE_ACTION, 
        _T("[%s] pSmbDb->iStatus = %s"), __FCN__, pSmbDb->iStatus == RETURN_OK ?
        T_RETURN_OK : T_RETURN_ER);

    /*SmbUpdateStatus(pSmbDb);*/

    DbgFcnOutRet (retVal);
    return (retVal);

} /* ParseR2Msg () */

int
MsgR1ToR2 (
    int  msgType
)
{
    int retVal = RETURN_OK;
    int bufOverflow = RETURN_OK;

    ERH_FUNCTION (_T("MsgR1ToR2"));
    DbgFcnInEx(__FLND__, _T("Msg: %s (%d)"), 
         MsgPrintFUNRESConstant (msgType), msgType);

    StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, msgType, 0);

    if (msgType==SMB_FUN_RESOLVE || msgType==SMB_FUN_RESTORE_RESOLVE) 
    {
        if ((retVal = SmbGenMsgAppend ()) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] SMB_MSGAPPEND failed!"), __FCN__);
            ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }

        if ((retVal = R1MsgAppendSysObjects (pSmbDb->pSysObjects)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }
    }
    else if (msgType == SMB_FUN_RESUME)
    {
        /* fill outBuffer with sessId's */
        tchar   **sessIds = NULL;
        int     sessIdcntr = 0;
        int     p = 0;
                       
        /* find sessIDs of Ldevs for resync */
        if (GetLdevSessionIds(pSseArray->pNextTgtDevices,  
                                pSseArray->iDevCount,
                                &sessIds, 
                                &sessIdcntr) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] GetLdevSessionIds failed!"), __FCN__);
            
            retVal = RETURN_ER;
            goto end;
        }
        
        /* append number of sessionIDs */
        if (StrMsgFAppend (_T("%d"), sessIdcntr) == -1)
        {
            ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
            bufOverflow = RETURN_ER;
            goto end;
        }

        DbgPlain (DBG_SSE_MSG, 
                  _T("[%s] sessIdcntr = %d"),
                  __FCN__, sessIdcntr);

        /* append sessionIDs */
        
        for (p = 0; p<sessIdcntr ; p++)
        {
            DbgPlain(DBG_SSE_MSG, _T("sessIds[%d]: %s"), p, sessIds[p]);   
            
            if (StrMsgFAppend (_T("%s"), sessIds[p]) == -1)
            {
                ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
                bufOverflow = RETURN_ER;
            }
            FREE(sessIds[p]);
        }
        FREE(sessIds);

    }
    else
        if (StrMsgFAppend (_T("%d"), -1) == -1)
        {
            ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
            bufOverflow = RETURN_ER;
        }

end:
    if (bufOverflow != RETURN_OK)
    {
        ErhFullReport ( __FCN__, 
                        ERH_CRITICAL,
                        NlsGetMessage(
                            NLS_SET_SSE,
                            NLS_MSG_MESSAGE_TO_LONG,
                            ErhErrnoToText ()
                            )
                      );
    }
    DbgFcnOutRet(retVal);
    return (retVal);

} /* MsgR1ToR2 () */


int
MsgR1ToSm (
    int  msgType
)
{
    int retVal = RETURN_OK;
    int bufOverflow = RETURN_OK;

    ERH_FUNCTION (_T("MsgR1ToSm"));
    DbgFcnInEx(__FLND__, _T("Msg: %s (%d)"), 
        MsgPrintMSGConstant (msgType), msgType);

   {
        int i;

        StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, msgType, 0);

        if (StrMsgFAppend (_T("%d"), pSmbDb->iStatus) == -1)
        {
            ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }

        DbgPlain (DBG_SSE_MSG,
            _T("%d"), pSmbDb->iStatus);

        for (i=0; i<pSmbDb->iObjectCnt; i++)
        {
            if (StrMsgFAppend (
                _T("%s%d%s%s"),
                pSmbDb->pObjects[i]->pBckpObject->tsObjectName,
                pSmbDb->pObjects[i]->iStatus,
                pSmbDb->tsBackHost,
	            PathToSlashes(pSmbDb->pObjects[i]->pBckpObject->tsBackName) ) == -1)
            {
                ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }

            DbgPlain (
                DBG_SSE_MSG,
                _T("%s+%d+%s+%s"),
                pSmbDb->pObjects[i]->pBckpObject->tsObjectName,
                pSmbDb->pObjects[i]->iStatus,
                pSmbDb->tsBackHost,
				PathToSlashes(pSmbDb->pObjects[i]->pBckpObject->tsBackName)
            );
        }
    }

end:
    if (bufOverflow != RETURN_OK)
    {
        ErhFullReport ( __FCN__, 
                        ERH_CRITICAL,
                        NlsGetMessage(
                            NLS_SET_SYMA,
                            NLS_MSG_MESSAGE_TO_LONG,
                            ErhErrnoToText ()
                            )
                      );
    }

    DbgFcnOutRet(retVal);
    return (retVal);

} /* MsgR1ToSm () */

int
MsgR2ToR1 (
    int  msgType
)
{
    int retVal = RETURN_OK;
    int bufOverflow = RETURN_OK; 

    ERH_FUNCTION (_T("MsgR2ToR1"));

    DbgFcnInEx(__FLND__, _T("Msg: %s (%d)"),
        MsgPrintFUNRESConstant (msgType), msgType);

    StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, msgType, 0);

    if (StrMsgFAppend (_T("%s"), opt.hostname) == -1)
    {
        ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
        bufOverflow = RETURN_ER;
        retVal = RETURN_ER;
        goto end;
    }

    if (StrMsgFAppend (_T("%d"), pSmbDb->iStatus) == -1)
    {
        ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
        bufOverflow = RETURN_ER;
        retVal = RETURN_ER;
        goto end;
    }

    /* R2MsgAppend for ir sesions */
    if (opt.irsession)
    {
        if (msgType == SMB_RES_RESTORE_RESOLVE)
        {
            if ((retVal = R2MsgAppendSysObjects (pSmbDb->pSysObjects)) != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }
        }
        else
        {
            if ((retVal = R2MsgAppendSysObjectsStatus (pSmbDb->pSysObjects)) 
                       != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                  _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }
        
            if ((retVal = R2MsgAppendRdskAll (pSmbDb->pSysObjects->pRdsk, 
                           pSmbDb->pSysObjects->iRdskCount)) != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED, _T("[%s] MsgAppendRdskAll failed!"), 
                          __FCN__);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }
        
        }

        goto end;
    }
    /* R2MsgAppend for backup or split mirror restore sessions */
    if (msgType == SMB_RES_RESTORE_PREEXEC)
    {
        if ((retVal = R2MsgAppendSysObjects (pSmbDb->pSysObjects)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }
    }
    else
    {
        if (msgType == SMB_RES_PREEXEC)
        {
            if ((retVal = R2MsgAppendSysObjects (pSmbDb->pSysObjects)) != RETURN_OK)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }
        }
        
        if ((retVal = R2MsgAppendSysObjectsStatus (pSmbDb->pSysObjects)) 
            != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
               _T("[%s] MsgAppendSysObjects failed!"), __FCN__);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }
        if ((retVal = R2MsgAppendRdskAll (pSmbDb->pSysObjects->pRdsk, 
                                          pSmbDb->pSysObjects->iRdskCount)) != RETURN_OK)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED, _T("[%s] MsgAppendRdskAll failed!"), 
                      __FCN__);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }
        
    }

end:
    if (bufOverflow != RETURN_OK)
    {
        ErhFullReport ( __FCN__, 
                        ERH_CRITICAL,
                        NlsGetMessage(
                            NLS_SET_SYMA,
                            NLS_MSG_MESSAGE_TO_LONG,
                            ErhErrnoToText ()
                            )
                      );
    }
    DbgFcnOutRet(retVal);
    return (retVal);

} /* MsgR2ToR1 () */


/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     msg
*
* @brief     Sends message to CON
*
*//*=========================================================================*/
int SmbReportHook (msg)
    tchar      *msg;
{
    struct StrMsg LOCAL = {0};
    ERH_FUNCTION (_T("SmbReportHook"));

    StrMsgInitL(&LOCAL);
    StrConvertCharToNull(msg);

#ifndef NLS_PACKED
    StrMsgMakeL(&LOCAL, NULL, 0, MSG_MSGFMT, msg, NULL);
#else
    StrMsgUseL(&LOCAL, msg, 0);
#endif

    if (IpcSendStrMsgL(ConIpcHandle, &LOCAL) == -1)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
            _T("[%s] IpcSendStrMsgL failed ERROR[%d]"), __FCN__, ErhErrno());
        ErhClearError ();
        skipReport = 1;
    }

    StrMsgFreeL(&LOCAL);
    return (1) ;
} /* SmbReportHook */


extern int 
SmbConsoleHook (tchar *msg)
{
    struct StrMsg LOCAL = {0};
    ERH_FUNCTION (_T("SmbConsoleHook"));

    StrMsgInitL(&LOCAL);
    StrMsgMakeL (&LOCAL, NULL, 0, MSG_MSGFMT, msg, NULL);

    IpcSendStrMsgL(ConIpcHandle, &LOCAL);
    StrMsgFreeL(&LOCAL);

    return (1);
}

extern int
SmbLogHook (msg)
    tchar      *msg;
{
    return (1);
}

extern int
SmbRunScriptHook (msg)
    tchar      *msg;
{
    StrAppend(runScriptOutput, _T("\t"));
    StrAppend(runScriptOutput, msg);

    return(1);
}

/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @retval    -none-
*
* @brief     Sends a data to all syma-R2 agents, which are
*            connected to syma-R1
*
*//*=========================================================================*/
void
SMB_SEND_TO_ALL_SMBR2 ()
{

    int sndsize,i;
    int snd_msg = 0;

    ERH_FUNCTION (_T("SMB_SEND_TO_ALL_SMBR2"));

    DbgStamp (DBG_SSE_MSG);

    for (i=0;i < MAX_SMB_AGENTS;i++)
    {

        if ( SMBtable[i].SmbID < 0 )
            continue;

        DbgPlain(
            DBG_SSE_MSG,
            _T("%s(); Sending data, SMBtable[i].Handle=%d\n"),
            __FCN__, SMBtable[i].Handle
        );

        sndsize = IpcSendData ( 
                      SMBtable[i].Handle,
                      USE_GLOBAL_BUFFER,
                      AUTO_SIZE
                  );

        if ( sndsize <= 0 )
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (
                DBG_UNEXPECTED, 
                _T("%s(); from Smb(r1)==>Smb(r2) write %d ERROR[%d]"),
                __FCN__, sndsize, ErhErrno()
            );

            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage ( 
                NLS_SET_SSE, NLS_SMB_R1_IPC_SMBR2_WRITE, ErhErrnoToText()));
                

            IpcCloseConnection (SMBtable[i].Handle);
            ErhClearError ();

            SMBtable[i].Handle             = -1 ;
            SMBtable[i].SmbID             = -1 ;
            SMBtable[i].Port               = -1;
            SMBtable[i].Used               = 0;
            SMBtable[i].Closed             = 0;
            SMBtable[i].Type               = SMB_PEER_IDLE;
            SMBtable[i].WaitForAckFromSMB = 0;
            SMBtable[i].WaitForAckFromCON  = 0;

            /*SmbExit (EX_ERR_INTERNAL);*/
        }

/* ---------------------------------------------------------------------------
|       successfully sent
 --------------------------------------------------------------------------- */ 
        snd_msg++;

        DbgPlain (
            DBG_SSE_MSG, 
            _T("%s(); Sending message from Smb(r1)==>Smb(r2) Handle: %d sent OK !"),
            __FCN__, SMBtable[i].Handle
        );

        SMBtable[i].WaitForAckFromSMB = 1;

    } /* for */

    if (snd_msg==0)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (
            DBG_UNEXPECTED,
            _T("%s(); No IPC connections found to SMB-R2 agent(s)!!\n"),
            __FCN__
        );

        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R1_NO_PEER_TO_SMBR2));
            

        /*SmbExit (EX_ERR_INTERNAL);*/
    }

} /* SMB_SEND_TO_ALL_SMBR2() */

/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @retval    -none-
*
* @brief     SMB-R2 sends a data to SMB-R1
*
*//*=========================================================================*/
void
SMB_SEND_TO_SMBR1 ()
{
    int sndsize;
    
    ERH_FUNCTION (_T("SMB_SEND_TO_SMBR1"));
    
    DbgPlain (DBG_SSE_MSG, 
        _T("[%s] MsgBuffer = %p, StrMsgSize(MsgBuffer) = %d"), 
        __FCN__, StrMsgGetBuf(), StrMsgSize(StrMsgGetBuf()));

    sndsize = IpcSendData (
                  SmbIpcHandle, 
                  USE_GLOBAL_BUFFER, 
                  AUTO_SIZE
              );
    if (sndsize <= 0)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (
            DBG_UNEXPECTED, 
            _T("%s(); from Smb(r2)==>Smb(r1) write %d ERROR[%d]"),
            __FCN__, sndsize, ErhErrno()
        );

        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R2_IPC_SMBR1_WRITE, ErhErrnoToText()));
            

        IpcCloseConnection (SmbIpcHandle);
        SmbIpcHandle = -1;

        ErhClearError ();
        SmbExit (EX_ERR_INTERNAL);
    }

    DbgPlain (
        DBG_SSE_MSG,
        _T("%s(); Sending message from SMB-R2 to SMB-R1 OK!\n"),
        __FCN__
    );
}

/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @retval    -none-
*
* @brief     Sends a data to CON/BSM
*
*//*=========================================================================*/
void
SMB_SEND_TO_CON_UNCOND ()
{
    int sndsize;

    ERH_FUNCTION (_T("SMB_SEND_TO_CON_UNCOND"));

    sndsize=IpcSendData (
                ConIpcHandle,
                USE_GLOBAL_BUFFER,
                AUTO_SIZE
        );

    if ( sndsize <= 0 )
    {
        skipReport = 1;
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (
            DBG_UNEXPECTED,
            _T("%s(); From SMB-Rx: write <= 0 ERROR[%d]\n"),
            __FCN__, ErhErrno()
        );
        /* SmbExit (EX_ERR_INTERNAL);*/
    } 

    DbgPlain(DBG_SSE_MSG, _T("%s(); Sending message to ConIpcHandle OK!\n"), __FCN__);
}
/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @retval    -none-
*
* @brief     Sends a data to CON/BSM
*
*//*=========================================================================*/
void
SMB_SEND_TO_CON ()
{
    ERH_FUNCTION (_T("SMB_SEND_TO_CON"));

    if (DmpMode == 0) /* Not started in DMP mode */
        SMB_SEND_TO_CON_UNCOND ();
    else
        DbgPlain(DBG_SSE_MSG, _T("%s(); Skipped sending to CON => DMP MODE!\n"), __FCN__);
}

/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     -none-
*
* @brief     This funtion handles SMB actions due to incoming SM,DBSM,
*            and SMB messages.
*
*//*=========================================================================*/
int
SmbEventLoop ()
{
    int retVal = RETURN_OK;
    int exitVal = EX_OK;

    int i=0;
    int readMessage=0;

    ERH_FUNCTION (_T("SmbEventLoop"));

    DbgFcnIn ();
/* ---------------------------------------------------------------------------
|   Now prepare for async event loop ...
 -------------------------------------------------------------------------- */

    DbgPlain (DBG_SSE_MSG,
        _T("[%s] Waiting IPC request: CON(SM) <==> SSEA. ConIpcHandle=%d"),
        __FCN__, ConIpcHandle);

    while (1)   /* loop until expected event happens */
    {
        int ipc=-1;
        int ipce=-1;
        int rcvsize;

/* ---------------------------------------------------------------------------
|       Waiting for IPC request  
 -------------------------------------------------------------------------- */
        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG, _T("[%s] Waiting for event ..."), __FCN__);

        /* Time to wait for IPC event: -1 = Infinite */
        ipc = IpcWaitForIpcEventDPI (-1); 
        ipce = IpcGetLastIpcEvent();

/* ---------------------------------------------------------------------------
|       We have an event, now first check for error & timeout conditions
 -------------------------------------------------------------------------- */

        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG, _T("[%s] IPC_EVENT"), __FCN__);

        if (ipc < 0)
        {
            if (IpcGetLastIpcEvent() == IPC_ERROR_EVENT)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] IPC_ERROR_EVENT (ErhErrno:%d)"),
                   __FCN__, ErhErrno());

                /* TBD
                ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
                        NLS_SET_SMB, NLS_SMB_IPC_EVENT, ErhErrnoToText ()));
                        */
            }
            else
            {
                /* Should Never Be */
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] Should never be!!!\n"), __FCN__);
            } 

            exitVal = EX_ERR_INTERNAL;
            goto end;
        }

/* ---------------------------------------------------------------------------
|       Is it a CON message ?
|       In XCOPY backup mode, CON and R1 handles are the same, but CON does not
|       recognize SMB_FUN_... msgs. We have to skip this and go to SmbIpcHandle
 -------------------------------------------------------------------------- */

        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG,
            _T("[%s] IPC EVENT: opt.R1host=%d, ipc(handle)=%d, opt.R2dmpstarted=%d"), 
            __FCN__, opt.R1host, ipc, opt.R2dmpstarted);
            
        if (ipc == ConIpcHandle && !opt.R2dmpstarted)
        {
            int msg_id;

            DbgStamp (DBG_SSE_MSG);
            DbgPlain (DBG_SSE_MSG,
               _T("[%s] IPC EVENT: CON: ConIpcHandle=%d"), 
               __FCN__, ConIpcHandle);

            switch ( IpcGetLastIpcEvent() )
            {

              case IPC_READ_EVENT:
                DbgPlain (DBG_SSE_MSG, 
                    _T("[%s] IPC_READ_EVENT"), __FCN__);
                         
                DbgPlain (DBG_SSE_MSG, 
                    _T("[%s] MsgBuffer = %p, sizeof(MsgBuffer) = %d"), 
                    __FCN__, StrMsgGetBuf(), StrMsgSize(StrMsgGetBuf()));

                rcvsize = IpcReceiveData (ipc /*ConIpcHandle*/, USE_GLOBAL_BUFFER,
                    AUTO_SIZE);

                if (rcvsize <= 0)
                {
                    skipReport = 1;

                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] From CON: read %d; ERROR[%d]"),
                        __FCN__, rcvsize, ErhErrno());

                    exitVal = EX_ERR_USER;
                    goto end;
                }

                readMessage = StrParseStart (USE_GLOBAL_BUFFER);

                DbgPlain (DBG_SSE_MSG,
                     _T("[%s] IPC_READ_EVENT from SM: %s (%d)"),
                     __FCN__, MsgPrintMSGConstant(readMessage), readMessage);

                msg_id = readMessage;

                /* Map restore messages */
                if (opt.restore)
                    switch ( readMessage )
                    {
                        case MSG_SMB_RESOLVE:
                            msg_id = MSG_SMB_RESTORE_RESOLVE; break;

                        case MSG_SMB_PREPARE_R2:
                            msg_id = MSG_SMB_RESTORE_PREPARE_R2; break;
                    }

                DbgPlain (DBG_SSE_MSG,
                     _T("[%s] Mapped message id from SM: %s (%d)"),
                     __FCN__, MsgPrintMSGConstant(msg_id), msg_id);

                switch ( msg_id )
                {
                  case -1:
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] from CON: parse -1"), __FCN__);

                    /* 
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                    ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
                            NLS_SET_SMB, NLS_SMB_IPC_CON_PARSE,
                            ErhErrnoToText ()));
                    ErhClearError ();
                    */

                    exitVal = EX_ERR_INTERNAL;
                    goto end;


                  case MSG_STOP: 
/* --------------------------------------------------------------------------
CR    ACTION:
CR    Check this
CR
CR    REASON:
CR    On MSG_STOP
CR      R1 should wait for MSG_STOP from R2
CR      R2 should go down.
CR    Why? Because R1 raid manager has to work if we want
CR    to resync disks.
CR
CR    PRIORITY:
CR    Very High!!!
--------------------------------------------------------------------------- */
                    if ( opt.R1host == 1 )
                    {
                        int br, connectedR2 = 0;

                        for (br = 0; br < MAX_SMB_AGENTS; br++)
                            if ( SMBtable[br].SmbID >= 0 )
                                connectedR2++;
                        
                        DbgStamp (DBG_SSE_MSG);
                        DbgPlain (DBG_SSE_MSG, _T("[%s] Connected R2 agents = %d"), 
                            __FCN__, connectedR2);

                        if (connectedR2 == 0)
                        {
                            DbgStamp (DBG_SSE_MSG);
                            exitVal = EX_OK;
                            goto end;
                        }
/* ---------------------------------------------------------------------------
|                       Ignore shutdown from CON; R2 is still alive 
--------------------------------------------------------------------------- */ 
                        DbgStamp (DBG_SSE_MSG);
                        DbgPlain (DBG_SSE_MSG, _T("[%s] SMB-R2 is still alive ..."), 
                            __FCN__);
                        break;
                    }
                    else
                    {
                        exitVal = EX_OK;  /* R2 should go down now */
                        goto end;
                    }
                    break;
    
                  case MSG_ABORT:
/* --------------------------------------------------------------------------
CR    ACTION:
CR    Check this
CR
CR    REASON:
CR    On MSG_ABORT
CR      R1 should wait for MSG_ABORT from R2
CR      R2 should go down.
CR    Why? Because R1 raid manager has to work if we want
CR    to resync disks.
CR
CR    PRIORITY:
CR    Very High!!!
--------------------------------------------------------------------------- */
                    if ( opt.R1host == 1 )
                    {
                        int br, connectedR2 = 0;

                        for (br = 0; br < MAX_SMB_AGENTS; br++)
                            if ( SMBtable[br].SmbID >= 0 )
                                connectedR2++;

                        DbgStamp (DBG_SSE_MSG);
                        DbgPlain (DBG_SSE_MSG, _T("[%s] Connected R2 agents = %d"), 
                            __FCN__, connectedR2);

                        if (connectedR2 == 0)
                        {
                            DbgStamp (DBG_SSE_MSG);
                            exitVal = EX_ERR_USER;
                            goto end;
                        }
                        DbgStamp (DBG_SSE_MSG);
                        DbgPlain (DBG_SSE_MSG, _T("[%s] SMB-R2 is still alive ..."), 
                            __FCN__);
/* ---------------------------------------------------------------------------
|                           Ignore shutdown from CON; R1 is still alive 
--------------------------------------------------------------------------- */ 
                        break;
                    }
                    else
                        exitVal = EX_ERR_USER;  
                        goto end;
                    break;

                  case MSG_SMB_RESOLVE:
                    retVal = MsgSmbResolve (); break;

                  case MSG_SMB_RESTORE_RESOLVE:
                    if (opt.irsession)
                        retVal = MsgSmbIRResolve ();
                    else
                        retVal = MsgSmbRestoreResolve ();
                    break;
       
                  case MSG_SMB_BACKUP_SPLIT:
                    retVal = MsgSmbBackupSplit (); break;

                  case MSG_SMB_RESTORE_SPLIT:
                    if (opt.irsession)
                        retVal = MsgSmbIRSplit ();
                    else
                        retVal = MsgSmbRestoreSplit ();
                    break;

                  case MSG_SMB_BACKUP_RESUME:
                    retVal = MsgSmbBackupResume (); break;
    
                  case MSG_SMB_RESTORE_RESUME:
                    if (opt.irsession)
                        retVal = MsgSmbIRResume ();
                    else
                        retVal = MsgSmbRestoreResume ();
                    break;

                  case MSG_SMB_PREPARE_R2:
                    retVal = MsgSmbPrepare (); break;

                  case MSG_SMB_RESTORE_PREPARE_R2:
                    if (opt.irsession)
                        retVal = MsgSmbIRPrepare ();
                    else
                        retVal = MsgSmbRestorePrepare ();
                    break;

                  default: 
/* ---------------------------------------------------------------------------
|                   PROTOCOL ERROR; IGNORE AND CONTINUE  
 --------------------------------------------------------------------------- */ 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] PROTOCOL ERROR: SMBtable[0].Handle=%d"),
                        __FCN__, SMBtable[0].Handle);
                    break;
                } /* switch */
                break;

              default: 
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] PROTOCOL ERROR: ConIpcError"), __FCN__);

                break;

            } /* switch  IpcGetLastIpcEvent()  */

        } /* if (ipc == ConIpcHandle) */

/* ---------------------------------------------------------------------------
|       Is it a R1 message ?
|
| !!!   SmbIpcHandle is checking at the beggining, becouse in DMP mode (R2)
|       Con and Smb() are the same - all mesages are coming from R1
|
 -------------------------------------------------------------------------- */
 
        else if ( ipc == SmbIpcHandle /*|| opt.R1host == 0*/)
        {
            DbgStamp (DBG_SSE_MSG);
            DbgPlain (DBG_SSE_MSG,
               _T("[%s] IPC EVENT: SmbIpcHandle=%d (opt.R1host=%d, ipc=%d)"), 
               __FCN__, SmbIpcHandle, opt.R1host, ipc);

            switch( IpcGetLastIpcEvent() )
            {
              case IPC_CLOSE_EVENT:
/* ---------------------------------------------------------------------------
|               Got unexpected close event from ssea-r1 
 --------------------------------------------------------------------------- */ 
                ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] Got CLOSE event r2<==r1,handle=%d"),
                   __FCN__, SmbIpcHandle
                );
    
                XSM_IPC_ERROR_R2_FROM_R1 (CLOSE_EVENT);

                ErhClearError ();

                DbgPlain (DBG_UNEXPECTED,
                   _T("[%s] Current SMB_STATE==%d, STATE_PREPARE_R2_DONE==%d"),
                   __FCN__, SmbState, STATE_PREPARE_R2_DONE);

                if (SmbState<STATE_PREPARE_R2_DONE) {
                    exitVal = EX_ERR_INTERNAL;
                    goto end;
                }
                
                break;

              case IPC_READ_EVENT:
                DbgPlain (DBG_SSE_MSG,_T("[%s] IPC_READ_EVENT"), __FCN__);

                rcvsize = IpcReceiveData (ipc/*SmbIpcHandle*/, USE_GLOBAL_BUFFER,
                    AUTO_SIZE);

                if ( rcvsize == 0 )
                {
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] From SMB(r2)<==SMB(r1): read 0 ERROR[%d]"),
                        __FCN__, ErhErrno());

                    XSM_IPC_ERROR_R2_FROM_R1 (CLOSE_EVENT);
                    ErhClearError ();
                    
                    if (SmbState<STATE_PREPARE_R2_DONE)
                    {
                       exitVal = EX_ERR_INTERNAL;
                       goto end;
                    }
                    break;
                }
                else if ( rcvsize < 0 )
                {
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] From SMB(r2)<==SMB(r1): read -1 ERROR[%d]"),
                        __FCN__, ErhErrno());
                    XSM_IPC_ERROR_R2_FROM_R1 (READ_ERROR);
                    ErhClearError ();
                    
                    if (SmbState<STATE_PREPARE_R2_DONE)
                    {
                       exitVal = EX_ERR_INTERNAL;
                       goto end;
                    }
                    break;
                }

                readMessage = StrParseStart (USE_GLOBAL_BUFFER);
                DbgPlain (DBG_SSE_MSG,
                    _T("[%s] IPC_READ_EVENT from R1: message = %s (%d)"),
                    __FCN__, MsgPrintFUNRESConstant(readMessage), readMessage
                );
                switch ( readMessage )
                {
                  case -1:
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] from SMB(r2)<==SMB(r1): parse -1"), __FCN__);

                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
                    XSM_IPC_ERROR_R2_FROM_R1 (PARSE_ERROR);

                    ErhClearError ();

                    if (SmbState<STATE_PREPARE_R2_DONE)
                    {
                       exitVal = EX_ERR_INTERNAL;
                       goto end;
                    }
                    break;

                  case MSG_STOP:
                    exitVal = EX_OK;
                    goto end;
                    break;

                  case MSG_ABORT:
                    exitVal = EX_ERR_USER;
                    goto end;
                    break;


                  case SMB_FUN_RESUME:
                    retVal = MsgFunResume (); break;

                  case SMB_FUN_RESTORE_RESUME:
                    retVal = MsgFunRestoreResume (); break;

                  case SMB_FUN_RESOLVE:
                    retVal = MsgFunResolve (); break;

                  case SMB_FUN_RESTORE_RESOLVE:
                    if (opt.irsession)
                        retVal = MsgFunIRResolve ();
                    else
                        retVal = MsgFunRestoreResolve ();
                    break;

                  case SMB_FUN_SYNC:
                    retVal = MsgFunSync (); break;

                  case SMB_FUN_RESTORE_SYNC:
                    retVal = MsgFunRestoreSync (); break;

                  case SMB_FUN_SPLIT: 
                    retVal = MsgFunSplit (); break;

                  case SMB_FUN_RESTORE_SPLIT: 
                    retVal = MsgFunRestoreSplit (); break;

                  case SMB_FUN_PREEXEC:
                    retVal = MsgFunPrepare (); break;

                  case SMB_FUN_RESTORE_PREEXEC:
                    retVal = MsgFunRestorePrepare (); break;
#if 0 /* obsolete */
                  case SMB_FUN_POSTEXEC:
                    retVal = MsgFunPostExec (); break;
#endif
                  case SMB_FUN_RESTORE_POSTEXEC:
                    retVal = MsgFunRestorePostExec (); break;

#if TARGET_WIN32
                  case SMB_FUN_CHANGESIG:
                  {
                      /************************************************************
                        Sending of this message and receiving result is handled 
                        by function SmbIRClusterPrepare (sseir.c) that has its
                        own msg loop just for it. Dont look for SMB_RES_CHANGESIG
                        inside this message loop *sigh*. This SmbEventLoop() 
                        function is screaming for rewrite...
                       ************************************************************/
                      retVal = MsgFunIRChangeSignature(); 
                      break;
                  }
#endif  /*TARGET_WIN32*/
                  default:
/* ---------------------------------------------------------------------------
|                   PROTOCOL ERROR; IGNORE AND CONTINUE
 --------------------------------------------------------------------------- */ 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] PROTOCOL ERROR: SmbIpcHandle"), __FCN__);
                    break;

                } /* switch ( readMessage ) */
                break;

              default:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] PROTOCOL ERROR: SmbIpcHandle"), __FCN__);
                break;
            } /* switch (IpcGetLastIpcEvent()) */

        } /* if ( ipc == SmbIpcHandle ) */


/* ---------------------------------------------------------------------------
|       Is it the SMB(r1) <=== SMB(r2) channel : SMBtable[i].Handle
 -------------------------------------------------------------------------- */

        else 
        {
            DbgPlain(DBG_SSE_MSG, 
                _T("[%s] Entered else, not Con not Smb...IPC event:%d"), 
                __FCN__, ipce );

            switch(ipce)
            {
              case IPC_CONNECT_EVENT:
                DbgPlain(DBG_SSE_MSG, _T("IPC Connect event found ...\n"));

/* ---------------------------------------------------------------------------
|             Got a connect event om SMB-R1 from SMB-R2 or DA
 --------------------------------------------------------------------------- */ 

                if ( SmbState == STATE_IDLE )
                    DbgPlain (DBG_SSE_MSG,
                        _T("[%s] IPC_CONNECT_EVENT: R1<--R2, handle=%d"), 
                        __FCN__, ipc);
                else
                    DbgPlain (DBG_SSE_MSG,
                        _T("[%s] IPC_CONNECT_EVENT: R1<--SM, handle=%d"), 
                        __FCN__, ipc);

	            DbgPlain(DBG_SSE_MSG, _T("Modifying SMBtable .... ver 1\n"));
                for (i=0;i<MAX_SMB_AGENTS;i++)
                    if ( SMBtable[i].SmbID < 0 )
                        break;

                if ( i == MAX_SMB_AGENTS )
                {
/* ---------------------------------------------------------------------------
|                   Internal error; ssea-R2 handle not found
 --------------------------------------------------------------------------- */ 

                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED, 
                        _T("[%s] r1<=r2(SM); not found syma-r2 handle!!!!"), 
                        __FCN__);
                    break;
                }
	            DbgPlain(DBG_SSE_MSG, _T("Modifying SMBtable .... ver 2\n"));

                SMBtable[i].SmbID = i;
                SMBtable[i].Handle = ipc;
                SMBtable[i].Type   = ( SmbState == STATE_IDLE ) ? 
                    SMB_PEER_R2 : SMB_PEER_DA;

/* ---------------------------------------------------------------------------
|               Create packet with buffer and send it to the SMB-R2.
 -------------------------------------------------------------------------- */

                StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_LISTEN, 0);

                {
                    int sndSize;

                    sndSize = IpcSendData (SMBtable[i].Handle, USE_GLOBAL_BUFFER,
                        AUTO_SIZE);

                    if (sndSize<=0)
                    {
                        DbgStamp (DBG_UNEXPECTED);
                        DbgPlain (DBG_UNEXPECTED,
                            _T("[%s] To SMB-R2: write <= 0 ERROR[%d]"),
                            __FCN__, ErhErrno());
                        exitVal = EX_ERR_INTERNAL;
                        goto end;
                    }
                }
                DbgPlain (DBG_SSE_MSG,
                   _T("[%s] SmbIpcHandle: Send message MSG_LISTEN to peer=%x"),
                   __FCN__, SMBtable[i].Type
                );
                break;

              case IPC_ERROR_EVENT:
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] IPC_ERROR_EVENT in conn r1 <== r2"), 
                    __FCN__);

                exitVal = EX_ERR_INTERNAL;
                goto end;
                break;

              case IPC_CLOSE_EVENT:
/* ---------------------------------------------------------------------------
|               Got unexpected close event from syma-r2 
 --------------------------------------------------------------------------- */ 
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (
                    DBG_UNEXPECTED,
                    _T("%s(); Got CLOSE event r1<==r2,handle=%d,port=%d\n"),
                    __FCN__, SMBtable[i].Handle,SMBtable[i].Port
                );
                IpcCloseConnection (SMBtable[i].Handle);
                ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
    
                XSM_IPC_ERROR_R1_FROM_R2 (i, CLOSE_EVENT);

                ErhClearError ();
                DbgPlain (
                   DBG_UNEXPECTED,
                   _T("%s(); Current SMB_STATE==%d, STATE_PREPARE_R2_DONE==%d\n"),
                   __FCN__, SmbState, STATE_PREPARE_R2_DONE
                );
                if (SmbState<STATE_PREPARE_R2_DONE)
                {
                   exitVal = EX_ERR_INTERNAL;
                   goto end;
                }
                
                break;

              case IPC_READ_EVENT:
                for (i=0;i<MAX_SMB_AGENTS;i++)
                {
                    if (SMBtable[i].Handle == ipc)
                       break;
                }
                if (i == MAX_SMB_AGENTS)
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (
                        DBG_UNEXPECTED, 
                        _T("%s(); r1<=r2; not found syma-r2 handle!!!!\n"), 
                        __FCN__
                    );
                    continue;
                }
        
                DbgStamp (DBG_SSE_MSG);
                DbgPlain(
                    DBG_SSE_MSG,
                    _T("%s(); IPC EVENT: SMBtable[%d].Handle=%d, Peer=%x\n"),
                    __FCN__, i, SMBtable[i].Handle,SMBtable[i].Type
                );

                rcvsize = IpcReceiveData (
                              ipc /*SMBtable[i].Handle*/,
                              USE_GLOBAL_BUFFER,
                              AUTO_SIZE
                          );

                if ( rcvsize == 0 )
                {
/* ---------------------------------------------------------------------------
|                   close connection 
 --------------------------------------------------------------------------- */ 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain(
                        DBG_UNEXPECTED,
                        _T("%s(); SMB(r1)<==SMB(r2): read 0 ERROR[%d], Peer=%x\n"),
                        __FCN__, ErhErrno(), SMBtable[i].Type
                    );
                    IpcCloseConnection (SMBtable[i].Handle);
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

                    XSM_IPC_ERROR_R1_FROM_R2 (i, CLOSE_EVENT);

                    ErhClearError();
                    /*
                    DbgPlain (
                       DBG_UNEXPECTED,
                       _T("%s(); Current SMB_STATE==%d, STATE_PREPARE_R2_DONE==%d\n"),
                       __FCN__, SmbState, STATE_PREPARE_R2_DONE
                    );
                    if (SmbState<STATE_PREPARE_R2_DONE)
                    {
                    */
                       exitVal = EX_ERR_INTERNAL;
                       goto end;
                       /*
                    }
                    */
                          
                    break;
                }
                else if ( rcvsize < 0 )
                {
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (
                        DBG_UNEXPECTED,
                        _T("%s(); SMB(r1)<==SMB(r2): read -1 ERROR[%d], Peer=%x\n"),
                        __FCN__, ErhErrno(), SMBtable[i].Type
                    );

                    IpcCloseConnection (SMBtable[i].Handle);
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

                    XSM_IPC_ERROR_R1_FROM_R2 (i, READ_ERROR);

                    ErhClearError();
                    /*
                    DbgPlain (
                       DBG_UNEXPECTED,
                       _T("%s(); Current SMB_STATE==%d, STATE_PREPARE_R2_DONE==%d\n"),
                       __FCN__, SmbState, STATE_PREPARE_R2_DONE
                    );
                    if (SmbState<STATE_PREPARE_R2_DONE)
                    {
                    */
                       exitVal = EX_ERR_INTERNAL;
                       goto end;
                       /*
                    }
                    */

                    break;
                }
                readMessage=StrParseStart(USE_GLOBAL_BUFFER);
                DbgPlain (DBG_SSE_MSG,
                    _T("%s(); IPC_READ_EVENT from R2: message %s (%d)"),
                    __FCN__, MsgPrintFUNRESConstant(readMessage), readMessage
                );
                switch ( readMessage )
                {
                  case -1:
/* ---------------------------------------------------------------------------
|                   Message parsing error  
 --------------------------------------------------------------------------- */ 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (
                        DBG_UNEXPECTED, 
                        _T("%s(); from SMB(r1)<==SMB(r2): parse -1"), __FCN__
                    );
                    ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

                    XSM_IPC_ERROR_R1_FROM_R2 (i, PARSE_ERROR);

                    ErhClearError ();
                    exitVal = EX_ERR_INTERNAL;
                    goto end;

                  case SMB_RES_RESOLVE:
                    retVal = MsgResResolve (); break;

                  case SMB_RES_RESTORE_RESOLVE:
                    if (opt.irsession)
                        retVal = MsgResIRResolve ();
                    else
                        retVal = MsgResRestoreResolve ();
                    break;
                    
                  case SMB_RES_SYNC:
                    retVal = MsgResSync (); break;

                  case SMB_RES_RESTORE_SYNC:
                    retVal = MsgResRestoreSync (); break;

                  case SMB_RES_PREEXEC:
                    retVal = MsgResPrepare (); break;
    
                  case SMB_RES_RESTORE_PREEXEC:
                    retVal = MsgResRestorePrepare (); break;
#if 0 /* obsolete */   
                  case SMB_RES_POSTEXEC:
                    retVal = MsgResPostExec (); break;
#endif    
                  case SMB_RES_RESTORE_POSTEXEC:
                    retVal = MsgResRestorePostExec (); break;
    
                  case SMB_RES_RESUME:
                    retVal = MsgResResume (); break;
                        
                  case SMB_RES_RESTORE_RESUME:
                    retVal = MsgResRestoreResume (); break;
    
                  case SMB_RES_SPLIT:
                    retVal = MsgResSplit (); break;

                  case SMB_RES_RESTORE_SPLIT:
                    retVal = MsgResRestoreSplit (); break;

                  case MSG_STOP:
                    SmbCloseR2Agents(MSG_STOP);
                    exitVal = EX_OK;
                    goto end;
                  break;

                  case MSG_ABORT:
                    SmbCloseR2Agents(MSG_STOP);
                    exitVal = EX_ERR_USER;
                    goto end;
                  break;

                  default:
/* ---------------------------------------------------------------------------
|                   PROTOCOL ERROR; IGNORE AND CONTINUE 
 --------------------------------------------------------------------------- */ 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (
                        DBG_UNEXPECTED,
                        _T("%s(); 1. PROTOCOL ERROR! SMBtable[i].Handle=%d\n"),
                        __FCN__, SMBtable[i].Handle
                    );
                    SmbCloseR2Agents(MSG_STOP);
                    exitVal = EX_ERR_INTERNAL;
                    goto end;
                    break;

                } /* switch ( readMessage ) */
                     break;

              default:
/* ---------------------------------------------------------------------------
|               PROTOCOL ERROR; IGNORE AND CONTINUE 
 -------------------------------------------------------------------------- */ 
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (
                    DBG_UNEXPECTED,
                    _T("%s(); 1. PROTOCOL ERROR! SMBtable[i].Handle=%d\n"),
                    __FCN__, SMBtable[i].Handle
                );
                break;

            } /* switch( IpcGetLastIpcEvent() ) */

        } /* else Incoming SMB-R2 channel */

    } /* while */

end:
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Retval is set but unsed: %d"), __FCN__, retVal);
    /* Cleanup */
    DbgFcnOutExit(exitVal);
    return exitVal;

} /* SmbEventLoop */


/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     cc            exit condition code
*
* @retval    never.
*
*//*=========================================================================*/
void
SmbExit (cc)
    long cc;
{
    int retVal = RETURN_OK;
    ERH_FUNCTION (_T("SmbExit"));

    DbgFcnIn ();

    DbgStamp (DBG_SSE_INIT);
    DbgPlain (
        DBG_SSE_INIT, 
        _T("%s (cc=%ld), skipReport=%d TEST opt.R1host:%d\n"), 
        __FCN__, cc, skipReport, opt.R1host
    );

/* --------------------------------------------------------------------------
CR    ACTION:
CR    Do we need to start cleand up is SmbExit is called with status
CR    EX_OK? YES, BECAUSE WE NEED TO STOP RM.
CR    
CR    REASON:
CR    Possible bug.
CR
CR    PRIORITY: 
CR    Very high!!
--------------------------------------------------------------------------- */
/*    if (cc != EX_OK) */
    {
#if 0
        if (opt.restore)
            if (opt.R1host == 1)
                retVal = SmbGenR1CleanupRestore (pSmbDb);
            else
                retVal = SmbGenR2CleanupRestore (pSmbDb);
        else
            if (opt.R1host == 1)
                retVal = SmbGenR1Cleanup (pSmbDb);
            else
                retVal = SmbGenR2Cleanup (pSmbDb);
#endif
        
        if (opt.R1host == 1)
            retVal = SmbGenR1Cleanup (pSmbDb);
        else
            retVal = SmbGenR2Cleanup (pSmbDb);
        
    }


    DbgStamp (DBG_SSE_INIT);
    if (skipReport == 0)
    {

        DbgStamp (DBG_SSE_INIT);
    /* TBD */
        ErhFullReport (
            __FCN__,
            ERH_NORMAL,
            NlsGetMessage (NLS_SET_SSE,
                ( cc == EX_OK ? NLS_SMB_COMPLETED : NLS_SMB_ABORTED),
                ( opt.R1host == 1 ? _T("SSEA-Application") : _T("SSEA-Backup")),
                cmnHostname));
                

        DbgStamp (DBG_SSE_INIT);
        StrMsgStartEx (
            USE_GLOBAL_BUFFER,
            AUTO_SIZE,
            (cc == EX_OK ? MSG_STOP : MSG_ABORT),
            0
        );

        if (cc != EX_OK) StrMsgFAppend (_T("%ld"), cc);

        DbgPlain (
            DBG_SSE_MSG,
            _T("[%s]\tSending MSG_%s to CON .."),
            __FCN__, (cc == EX_OK ? _T("STOP") : _T("ABORT"))
        );
        
        if (IpcSendData (ConIpcHandle, USE_GLOBAL_BUFFER, AUTO_SIZE)<=0)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (
                DBG_UNEXPECTED, 
                _T("IpcSendData(%s) message %s ERROR[%d] to BSM\n"),
                (opt.R1host == 1 ? _T("SSEA-R1") : _T("SSEA-R2")),
                (cc == EX_OK ? _T("STOP") : _T("ABORT")),
                ErhErrno()
            );
        }

        if (opt.R1host==0)
        {
            if (IpcSendData (SmbIpcHandle, USE_GLOBAL_BUFFER, AUTO_SIZE)<=0)
            {
                DbgStamp (DBG_UNEXPECTED);
                DbgPlain (
                    DBG_UNEXPECTED,
                    _T("IpcSendData(%s) message %s ERROR[%d] to SMB-R1\n"),
                    (opt.R1host == 1 ? _T("SSEA-R1") : _T("SSEA-R2")),
                    (cc == EX_OK ? _T("STOP") : _T("ABORT")),
                    ErhErrno()
                );
            }
        }
    }

    ErhClearError ();

    IpcShutdownAndCloseConnection (ConIpcHandle);

    ErhClearError ();

    if (opt.R1host==0)
    {
        DbgPlain (DBG_SSE_MSG, _T("Exiting SMB-R2"));
        if (SmbIpcHandle >= 0)
           IpcShutdownAndCloseConnection (SmbIpcHandle);
    }   

    DbgFcnOutExit(retVal);
}


/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @retval    0 if SMB-R2 connect successfuly to SMB-R1 agent
* @retval    1 otherwise
*
* @brief     Establish connection between SMB process running on application
*            host (R1) and SMB-R2 process running on backup host (R2).
*
*//*=========================================================================*/

int
SmbConnectAgent ()
{
    int  retCode;
    int  ackTimeout=(30);

    ERH_FUNCTION (_T("SmbConnectAgent"));
    
    DbgPlain (DBG_SSE_MSG, _T("[%s] Enter"), __FCN__);

    DbgPlain (DBG_SSE_MSG, _T("Establishing connection SMB(R2) ==> SMB(R1)"));
    DbgPlain (DBG_SSE_MSG, _T("---------------------------------------------"));
    DbgPlain (DBG_SSE_MSG, _T("Application host : %s, port %d"), opt.appHost, opt.port);
    
    /* In DMP mode, CON is representing R1 */
    if (opt.R2dmpstarted)
    {
        DbgPlain (DBG_SSE_MSG, 
                  _T("[%s] SmbIpcHandle switched to ConIpcHandle = [%d]"),
                  __FCN__, ConIpcHandle);
        SmbIpcHandle = ConIpcHandle; 
        
        DbgPlain (DBG_SSE_MSG, _T("[%s] Exit"), __FCN__);
        return(RETURN_OK);
    }


    if (( SmbIpcHandle = IpcConnectToProcessDPI (opt.appHost,opt.port,0)) < 0)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain(
            DBG_UNEXPECTED,
            _T("%s(): Problem with IpcConnectToProcessDPI; ERROR[%d]\n"), 
            __FCN__, ErhErrno()
        );
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (
            NLS_SET_SSE, NLS_SMB_R2_CANNOT_CONNECT, cmnHostname,
            opt.appHost, opt.port));
            
        return (RETURN_ER);
    }  
    else 
    {
        DbgPlain (
            DBG_SSE_MSG,
            _T("%s:Handle=%d:IpcConnectToProcessDPI(): OK !"),
            opt.appHost,
            SmbIpcHandle
        );

#if !TARGET_WIN32
        signal (SIGALRM, alarmHandler);
        alarm (ackTimeout);
        alrm_errno = 0;
#else
        /* TODO */
#endif

        DbgPlain (
            DBG_SSE_MSG,
            _T("%s(); Waiting for ACK. Timeout is %d sec."),
            __FCN__, ackTimeout
        );

        retCode = IpcReceiveData ( 
                      SmbIpcHandle,
                      USE_GLOBAL_BUFFER,
                      AUTO_SIZE
                  );
#if !TARGET_WIN32
        alarm (0);
#endif

        if ( retCode <= 0 && alrm_errno == EINTR )
        { 
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (
                DBG_UNEXPECTED,
                _T("%s(); %s: IpcReceiveData: ERROR[%d]\n"),
                __FCN__, opt.appHost, ErhErrno()
            );

            /* TBD */
            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage ( 
                NLS_SET_SSE, NLS_SMB_R2_CANNOT_GET_INFO, cmnHostname,
                opt.appHost, ErhErrnoToText()));
            ErhClearError ();

            DbgPlain (DBG_SSE_MSG, _T("[%s] Exit"), __FCN__);
            return (RETURN_ER);
        }
            
        DbgPlain (
            DBG_SSE_MSG,
            _T("%s(); %s:Handle=%d:IpcReceiveData(): OK !"),
            __FCN__, opt.appHost, SmbIpcHandle
        );
       
        if ( StrParseStart( USE_GLOBAL_BUFFER ) == MSG_ABORT )
        {
            return (RETURN_ER);
        }

        DbgPlain (
            DBG_SSE_MSG, 
            _T("%s(); SMB-R2 connected, host=%s, port=%d, handle=%d\n"),
            __FCN__,
            opt.appHost,
            opt.port,
            SmbIpcHandle
        );
    } 
    
    DbgPlain (DBG_SSE_MSG, _T("[%s] Exit"), __FCN__);
    return (RETURN_OK);
}



/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     -none-
*
* @retval    1 - if syma-r1 got all replys from syma-r2
* @retval    0 - otherwise
*
* @brief     Scans through established connections syma-r1 <--> syma-r2 if their
*            SMBtable[i].WaitForAckFromSMB value equals to 1 and returns 0
*            if it finds any. Otherwise returns 0. Objective is to find if syma-R1
*            is still waiting for response message from any syma-R2 agent
*
*//*=========================================================================*/

int
SmbFinalRep (void)
{
    int i;

    ERH_FUNCTION (_T("SmbFinalRep"));

    for (i=0;i < MAX_SMB_AGENTS;i++)
    {
        if ( SMBtable[i].SmbID < 0 )
        {
            continue;
        }
        if ( SMBtable[i].WaitForAckFromSMB == 1 )
        {
           return (0);
        }
    }

    return (1);

} /* SmbFinalRep() */



/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     handle   Ipc handle of connection
* @param     type     Type of error event
*
* @retval    - none -
*
* @brief     Handle unexpected ipc events in syma-r1 from syma-r2. Mainly it
*            sends report to xSM and updates internal structure that records
*            connection information.
*
*//*=========================================================================*/

void
XSM_IPC_ERROR_R1_FROM_R2 (handle_id,type)
    int handle_id;
    int type;
{

    ERH_FUNCTION (_T("SmbEventLoop")); /* Do not change function name !! */

    switch (type)
    {
      case CLOSE_EVENT:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R1_IPC_SMBR2_CLOSE, ErhErrnoToText ()));
        break;

      case READ_ERROR:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R1_IPC_SMBR2_READ, ErhErrnoToText ()));
        break;

      case PARSE_ERROR:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R1_IPC_SMBR2_PARSE, ErhErrnoToText ()));
        break;
    }

    ErhClearError ();
    
     
    SMBtable[handle_id].Handle             = INVALID_HANDLE;
    SMBtable[handle_id].SmbID             = -1;
    SMBtable[handle_id].Port               = -1;
    SMBtable[handle_id].Used               = 0;
    SMBtable[handle_id].Closed             = 0;
    SMBtable[handle_id].Type               = SMB_PEER_IDLE;
    SMBtable[handle_id].WaitForAckFromSMB = 0;
    SMBtable[handle_id].WaitForAckFromCON  = 0;

    return;
}

/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     type     Type of error event
*
* @retval    - none -
*
* @brief     Handle unexpected ipc events in syma-r2 from syma-r1. Mainly it
*            sends report to xSM and resets ipc handle.
*
*//*=========================================================================*/
void
XSM_IPC_ERROR_R2_FROM_R1 (type)
    int type;
{

    ERH_FUNCTION (_T("SmbEventLoop")); /* Do not change function name !! */

    switch (type)
    {
      case CLOSE_EVENT:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R2_IPC_SMBR1_CLOSE, ErhErrnoToText ()));
        break;

      case READ_ERROR:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R2_IPC_SMBR1_READ, ErhErrnoToText ()));
        break;

      case PARSE_ERROR:
        ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
            NLS_SMB_R2_IPC_SMBR1_PARSE, ErhErrnoToText ()));
        break;
    }
    

    IpcCloseConnection (SmbIpcHandle);
    ErhClearError ();
     
    SmbIpcHandle = INVALID_HANDLE;  

    return;
}


/*========================================================================*//**
*
* @ingroup   SSEA_Messages
*
* @param     mess_id Message ID
*
* @retval    -none-
*
* @brief     Smb-r1 sends a STOP or ABORT message to all symar-r2 agent and
*            closes ipc connection
*
*//*=========================================================================*/
void
SmbCloseR2Agents (mess_id)
    int mess_id;
{

    int i,sndsize;

    ERH_FUNCTION (_T("SmbCloseR2Agents"));

    StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, mess_id, 0);


    for (i=0;i<MAX_SMB_AGENTS;i++)
    {
        if ( SMBtable[i].Handle < 0 )
            continue;

        DbgPlain (DBG_SSE_MSG,
            _T("[%s] Sending data, SMBtable[i].Handle=%d\n"),
            __FCN__, SMBtable[i].Handle);
    
        sndsize = IpcSendData (SMBtable[i].Handle, USE_GLOBAL_BUFFER,
            AUTO_SIZE);

        DbgPlain (DBG_SSE_MSG, 
            _T("[%s] Sent message from R1 to R2 Handle: %d write = %d!"),
            __FCN__, SMBtable[i].Handle, sndsize);

        {
            IpcCloseConnection (SMBtable[i].Handle);

            SMBtable[i].Handle             = -1;
            SMBtable[i].SmbID             = -1;
            SMBtable[i].Port               = -1;
            SMBtable[i].Used               = 0;
            SMBtable[i].Closed             = 0;
            SMBtable[i].Type               = SMB_PEER_IDLE;
            SMBtable[i].WaitForAckFromSMB = 0;
            SMBtable[i].WaitForAckFromCON  = 0;

            DbgPlain (DBG_SSE_MSG, 
                _T("[%s] Handle for R2 is closed !!!"), __FCN__);
        } 
    } /* for */

    return;

} /* SmbCloseR2Agents () */

void
alarmHandler (sig)
        int             sig;
{
        /* NOOP */
        alrm_errno = EINTR;
       
        DbgPlain (DBG_SSE_GLOBAL, _T("*** ALARM TIMEOUT ***"));

}       /*  alarmHandler  */

int
SmbConnect(ac, av)
    int     ac;
    tchar   **av;
{
    int     retVal = RETURN_OK;
    tchar    verString[STRLEN_VERSION+1];
    int i;
    unsigned ipc=0;


    ERH_FUNCTION (_T("SmbConnect"));

    DbgFcnIn ();

    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG, _T("[%s] Trying IpcAttachConnection"), __FCN__);
    
    DbgPlain (DBG_SSE_MSG, _T("[%s] Trying IpcAttachConnection (opt.inet = %d)"), 
              __FCN__, opt.inet);
#if TARGET_WIN32
    ConIpcHandle = IpcAttachConnection (opt.inet, opt.inet);
#else
    ConIpcHandle = IpcAttachConnection (0, 1);
#endif

    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG, 
        _T("[%s] IpcAttachConnection done! ConIpcHandle = %d"), 
        __FCN__, ConIpcHandle);

    MakeShortVersionString (STRLEN_VERSION, verString);

    StrMsgMake (USE_GLOBAL_BUFFER, 
                AUTO_SIZE, 
                MSG_ID, 
                PROG_SSEA, 
                verString, 
                NULL);

    {
        int sndSize;
            
        sndSize = IpcSendData (ConIpcHandle, 
                               USE_GLOBAL_BUFFER, 
                               AUTO_SIZE);

        if (sndSize <= 0)
        {
            skipReport = 1;
    
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                    _T("[%s] To CON write <= 0; ERROR:[%d]"), 
                    __FCN__, ErhErrno());

            retVal = RETURN_ER;
            goto end;
        }
    }

    if(AgentIdentificationLoop (ac, av) < 0)
    {
        skipReport = 1;

        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG,
            _T("[%s] Agent identification failure."), 
            __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    if (RUNMODE_AUTH_CHECK == opt.runmode)
    {
        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG, _T("[%s] Authentication check. Skipping further connections"), __FCN__);
        goto end;
    }
/* ---------------------------------------------------------------------------
|  Initialize globals for SMB-R1 <==> SMB-R2  
 -------------------------------------------------------------------------- */

    if ( opt.R1host == 1 )
    {
      for ( i=0;i<MAX_SMB_AGENTS;i++)
      {
          SMBtable[i].Handle             = -1;
          SMBtable[i].SmbID             = -1 ;
          SMBtable[i].Port               = -1;
          SMBtable[i].Used               = 0;
          SMBtable[i].Closed             = 0;
          SMBtable[i].Type               = SMB_PEER_IDLE;
          SMBtable[i].WaitForAckFromSMB = 0;
          SMBtable[i].WaitForAckFromCON  = 0;
          SMBtable[i].Host               = _T("");
      }
    }

/* ---------------------------------------------------------------------------
|   SMB-R1: Allow incoming connections to SMB-R1.
 -------------------------------------------------------------------------- */

    if (opt.R1host == 1) {
        if (IpcAcceptConnectionsDPI ( IPC_ANY_PORT ) < 0)
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                _T("[%s] IpcAcceptConnection < 0, ERROR[%d]"),
                __FCN__, ErhErrno());

            ErhFullReport (__FCN__, ERH_CRITICAL, NlsGetMessage (NLS_SET_SSE,
                    NLS_SMB_R1_CANNOT_LISTEN));
                    

            retVal = RETURN_ER;
            goto end;
        }
        ipc=IpcGetListenPort ();
    }
    else
    { 

/* ---------------------------------------------------------------------------
|   SMB-R2: Make connection SMB-R2 to SMB-R1
 -------------------------------------------------------------------------- */

        if (SmbConnectAgent () == RETURN_ER)
        {
            StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_ABORT, 0);
            {
                int sndSize;
                //QXCM1000369015-Invalid format of message sent

                StrMsgFAppend(_T("%d"), RETURN_ER);
                sndSize = IpcSendData (ConIpcHandle, USE_GLOBAL_BUFFER, 
                    AUTO_SIZE);

                if (sndSize<=0)
                { 
                    DbgStamp (DBG_UNEXPECTED);
                    DbgPlain (DBG_UNEXPECTED,
                        _T("[%s] To CON write <= 0 ; ERROR[%d]"), 
                        __FCN__, ErhErrno());

                    skipReport = 1;
                }
            }
            retVal = RETURN_ER;
            goto end;
        }
    }

/* ---------------------------------------------------------------------------
|    Send a successful startup message to our MAIN process ...
 -------------------------------------------------------------------------- */
    
    StrMsgStartEx (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_LISTEN, 0);
    StrMsgFAppend (_T("%d"),(opt.R1host == 1) ? ipc : 0);

    {
        int sndSize;
        
        sndSize = IpcSendData (ConIpcHandle, 
                               USE_GLOBAL_BUFFER,
                               AUTO_SIZE);

        if (sndSize<=0)
        {
           DbgStamp (DBG_UNEXPECTED);
           DbgPlain (DBG_UNEXPECTED,
                _T("[%s] To CON write <= 0; ERROR[%d]"),
                __FCN__, ErhErrno());
           skipReport = 1;

           retVal = RETURN_ER;
           goto end;
        }
    }

    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG,
         _T("[%s] MSG_LISTEN: port=%d"), __FCN__, (opt.R1host == 1) ? ipc : 0);

end:
    /* Cleanup */
    DbgPlain (DBG_SSE_MSG, _T("[%s] ConIpcHandle = %d"), __FCN__, ConIpcHandle);
    DbgFcnOutRet (retVal);
    return retVal;
}

int
MsgMirrorLock (targdev *tgtDev,
               int   iDevCount,
               int     *locked)
{
    int status = -1;
    int retVal = RETURN_OK;
    int i = 0;
    int n = 0;
    int p = 0;
    tchar mirror_id[STRLEN_PATH + 1];
    int rcvsize = 0;
    int nr_locked = 0;
    tchar **msgArg=NULL;
    int readMessage = 0;
    
    ERH_FUNCTION (_T("MsgMirrorLock"));
    DbgFcnIn ();

    /* Check input parameter */
    if (locked == NULL)
    {
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    /* Initialise */
    *locked = 0;
    
    /* find a number of disks for locking */
    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION,_T("iDevCount = %d\n"), iDevCount);
    for (i=0; i<iDevCount; i++)
    {
        DbgPlain (DBG_SSE_ACTION,_T("tgtDev[%d]->serial = %d\n"),i, tgtDev[i].serial);

        if ( tgtDev[i].serial != 0)
            p++;
    }

    
    if (!p)
    {
        DbgStamp (DBG_SSE_ACTION);
        DbgPlain (DBG_SSE_ACTION,_T("There are no disks for locking"));
        goto end;
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION,_T("Number of disks for locking is %d\n"), p);
    
    /* Pack message */
    StrMsgStart (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_RESLOCK_REQUEST);
    
    StrMsgFAppend (_T("%d%d%d"), 0, 0, 0);
    StrMsgFAppend (_T("%d"), p);
    for (i=0; i<iDevCount; i++)
    {
        if (tgtDev[i].serial == 0)
            continue;
        
         sprintf (mirror_id, _T("%d_%d"), tgtDev[i].serial, 
                 tgtDev[i].ldevno);

         DbgStamp (DBG_SSE_ACTION);
         DbgPlain (DBG_SSE_ACTION,_T("mirror_id = %s!!\n"), mirror_id);
               
         StrMsgFAppend (_T("%d"), RESLOCK_RESTYPE_HPSURESTOREXP_LDEV);
         StrMsgFAppend (_T("%s"), mirror_id);
    }

    DbgStamp (DBG_SSE_ACTION);
    DbgPlain (DBG_SSE_ACTION,_T("MsgBuffer = %s\n"), StrMsgGetBuf());

    /* Send message to SM */
       
    SMB_SEND_TO_CON_UNCOND ();

    
    /* Waiting for ever */
    rcvsize = IpcReceiveData (ConIpcHandle, USE_GLOBAL_BUFFER,
                              AUTO_SIZE);

    if (rcvsize <= 0)
    {
        ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
                  _T("[%s] From CON: read %d; ERROR[%d]"),
                  __FCN__, rcvsize, ErhErrno());

        IpcCloseConnection (ConIpcHandle);
        ConIpcHandle = -1;
        
        retVal = RETURN_ER;
        goto end;
    }

    readMessage = StrParseStart (USE_GLOBAL_BUFFER);

    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG, _T("[%s] Got reply %s"),
              __FCN__, MsgPrintMSGConstant (readMessage));

    /* Did we get a LOCK reply or something else?!  */
    if (readMessage != MSG_RESLOCK_REQUEST_STATUS)
    {
        /* FIXME: ErhMark */
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Unexpected reply -> ABORTING"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    /* Unpack the ErhStat structure from the message */
    if (ErhUnpackMsg () < 0)
    {
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Cannot unpack msg"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    /* n = FLD_RESLOCK_REQUEST_STATUS_NR + iDevCount; */
    n = FLD_RESLOCK_REQUEST_STATUS_NR + p;
    msgArg = MALLOC(n * sizeof (tchar *));

    if (StrParseMessage (n, msgArg) != 0)
    {
        ErhMark (ERH_MFORMAT, __FCN__, ERH_CRITICAL);
        retVal = RETURN_ER;
        goto end;
    }

    if (p != atoi (msgArg[FLD_RESLOCK_REQUEST_STATUS_N]))
    {
        ErhMark (ERH_MFORMAT, __FCN__, ERH_CRITICAL);
        retVal = RETURN_ER;
        goto end;
    }

    for (i=0; i < p; i++)
    {
        status = atoi (msgArg[FLD_RESLOCK_REQUEST_STATUS_NR + i]);

        if (status == RESLOCK_STATUS_LOCKED)
            nr_locked++;
    }

   

    *locked = (nr_locked == p);
    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG, _T("[%s] locked = %d\n"),
              __FCN__, *locked);

    
end:
    if (msgArg)
        FREE(msgArg);
    
    DbgFcnOutRet (retVal);
    return retVal;
}

int
MsgSessionRemove (
    PSSE_DEVICE_T *pDev,
    int           iCount)
{
    tchar *p = NULL;
    tchar *list = NULL;
    int retVal = RETURN_OK;
    
    ERH_FUNCTION (_T("MsgSessionRemove"));
    DbgFcnIn ();

    /* Check input parameter */
    if (*pDev == NULL)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }

    /* allocate session list */
#if 0
    list = (tchar *) MALLOC (STRLEN_STD*4);
    list[0] = _T('\0');
#endif
    
    /* List sessions */
    if (SseDbXP_SessionList (pDev, iCount, &list) != RETURN_OK)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] SseDbXP_SessionList failed!"), __FCN__);

        retVal = RETURN_ER;
        goto end;
    }


    if (irdbInit_Integrations(1,0) != RETURN_OK)
    {
        goto end;
    }

    p = strtok (list, _T(","));
    while (p != NULL)
    {
        DbgPlain (DBG_SSE_MSG, 
                  _T("[%s] Removing session [%s]"), __FCN__, p);

        irdbRemIntegSessions(p);

        p = strtok (NULL, _T(","));
    }
    
    irdbExit_Integrations();

end:
    if (list != NULL)
        FREE (list);
    
    DbgFcnOutRet (retVal);
    return retVal;
}

/** \todo
        STRLEN_ORIGUNIT_ID has to be defined in a header file, included by BSM, RSM and all ZDB agents */
#ifndef STRLEN_ORIGUNIT_ID
#define STRLEN_ORIGUNIT_ID 255
#endif

int
MsgCapacityToSm (
    void
)
{
    int retVal = RETURN_OK;
    int bufOverflow = RETURN_OK;

    ERH_FUNCTION (_T("MsgCapacityToSm"));
    DbgFcnIn ();

    {
        int i;

        StrMsgStart (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_SMB_CAPACITY);

        if (StrMsgFAppend (_T("%d"), BT_SUBTYPE_SURESTOREEXP) == -1)
        {
            ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
            bufOverflow = RETURN_ER;
            retVal = RETURN_ER;
            goto end;
        }


        DbgPlain (DBG_SSE_MSG, _T("%d"), BT_SUBTYPE_SURESTOREEXP);

        for (i=0; i<pSmbDb->pSysObjects->iRdskCount; i++)
        {
            ULONG64 capacity;
            tchar id[STRLEN_ORIGUNIT_ID+1];

            SysDiskSize (pSmbDb->pSysObjects->pRdsk[i]->tsFullPath, &capacity);

            snprintf (id, STRLEN_ORIGUNIT_ID, _T("%d:%d"), 
                pSseArray->pSrcDevices[i]->Dev.serial,
                pSseArray->pSrcDevices[i]->Dev.ldevno);
            id[STRLEN_ORIGUNIT_ID] = _T('\0');

            if (StrMsgFAppend (
                _T("%s%d"),
                id,
                (int)(capacity>>20)
                ) == -1)
            {
                ErhMark(ERH_BUFFER_OVERFLOW, __FCN__, ERH_CRITICAL);
                bufOverflow = RETURN_ER;
                retVal = RETURN_ER;
                goto end;
            }

            DbgPlain (
                DBG_SSE_MSG,
                _T("%d:%d,%d"),
                pSseArray->pSrcDevices[i]->Dev.serial,
                pSseArray->pSrcDevices[i]->Dev.ldevno,
                (int)(capacity>>20)
            );
        }
    }

end:
    if (bufOverflow != RETURN_OK)
    {
        ErhFullReport ( __FCN__, 
                        ERH_CRITICAL,
                        NlsGetMessage(
                            NLS_SET_SSE,
                            NLS_MSG_MESSAGE_TO_LONG,
                            ErhErrnoToText ()
                            )
                      );
    }

    DbgFcnOutRet(retVal);
    return (retVal);

} /* MsgCapacityToSm */


//11189-not used
#if 0
/* BEGIN    QUIX ID:QXCR10000288643 Added By : Umesh (Emp Id : 20130476) */
int LockBckpHost( int *status, int *flagAbort)
{
    int       retVal = RETURN_OK;
    int       retryCount = 0;
    int       retryOfLock = MAXIMUM_HOST_LOCKING_RETRY; 
    tchar     *var = NULL;

	
    ERH_FUNCTION (_T("LockBckpHost"));

    DbgFcnIn();
	
    /* Input parameter varification */
    if (status == NULL || flagAbort == NULL)
    {
        /* if invalid parameter(s) report to debug log */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters (retStatus = %p, retFlagAbort = %p)!"), 
                 __FCN__, status, flagAbort);
        retVal = RETURN_ER;
        goto end;
    }


    if ((var=(tchar*)GetEnv(_T("MAXIMUM_HOST_LOCKING_RETRY")))!=NULL)
    {
        retryOfLock = atoi (var);
    }


    for (retryCount=0; retryCount<retryOfLock; retryCount++)
    {

	/* try to lock the backup host */
        if (RETURN_OK != SseMsgHostLockUnLock( opt.hostname, 
    					 SSE_LOCK_COMMAND_EXEC,
       				         SSE_HOST_LOCK,
                            		 status,
                            		 flagAbort))
        {
            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
              _T("[%s] MsgHostLockUnLock failed! on %s"), 
              __FCN__, opt.hostname);

            /* If any abort msg received then abort the session */
            if ( (*flagAbort) != 0)
            {
                retVal = RETURN_ER;
            }
            /* lock failed */	
            else
            {            
                ErhFullReport(__FCN__, ERH_CRITICAL, NlsGetMessage (
                                           NLS_SET_SSE, 
					   NLS_SSE_APP_HOST_OBJ_LOCKING_FAILED, 
                                           ErhErrnoToText()
										   )
				); 
                ErhClearError();
                retVal = RETURN_ER;
            }
            goto end;
        }
        /* successful lock */
        if ((*status) == TRUE)
        { 
            break;
        } 
       /* Wait for some other session  to release lock and report to CON only once */      
        if (0 == retryCount)
        {
            ErhFullReport(__FCN__, ERH_NORMAL, 
                          NlsGetMessage ( NLS_SET_SSE,
                                          NLS_SSE_WAIT_FOR_LOCKING)
            				);

        }
        /* Delay of 10 sec between each lock retry*/
        sleep(DELAY_BEFORE_HOST_LOCKING_RETRY);


    } /* end of for */

    /* If we hit the max number of retry  then endup the session */
    if (retryOfLock == retryCount)
    {
        ErhFullReport(__FCN__, ERH_MAJOR, 
                      NlsGetMessage (
                                 NLS_SET_SSE,
                                 NLS_SSE_WAIT_FOR_LOCKING_FAILED
                      )
        );

        retVal = RETURN_ER;
        goto end;
    }
end:
    DbgPlain(DBG_SSE_MSG,_T("[%s] retFlagAbort: %d, retStatus: %d"), __FCN__, (*flagAbort), (*status));
    DbgFcnOutRet(retVal);    
    return(retVal);
}/* LockDriveScanOnBckpHost */



int SseMsgHostLockUnLock(
    tchar *hostName,
    tchar *sufix,
    int   operation, /* unlock = 1, lock = 0*/
    int   *status,
    int   *flagAbort
)
{
    CmnVec deviceList = {0};

    int   retVal = RETURN_OK;
    tchar *host_id = NULL;

    ERH_FUNCTION (_T("SseMsgHostLockUnLock"));
    
    DbgFcnInEx(__FLND__,_T("operation = %d"), operation);

    DbgStamp (DBG_SSE_ACTION);

    /* Check input parameter */
    if ( (hostName == NULL) || (status == NULL) || (flagAbort == NULL))
    {
        ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    /* Just for debug logs */
    if (SSE_HOST_LOCK == operation)
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] Mode = LOCKING!"), __FCN__);
    }
    else if ( SSE_HOST_UNLOCK == operation)
    {
        DbgPlain (DBG_SSE_ACTION, _T("[%s] Mode = UNLOCKING!"), __FCN__);
    }
    

    DbgPlain (DBG_SSE_ACTION, _T("[%s] hostName = %s!"), 
             __FCN__, NS(hostName));


    /* Initialization */
    CmnVecInit(&deviceList, 16, NULL);

    /* create host_id */
    host_id = StrNewCopy(hostName);

    /* append string for locking/unlocking */
    host_id = StrAppend(host_id, sufix);
    
    /* Add host-id to device list */
    CmnVecAddStr(&deviceList, host_id);

    retVal = SseMsgLockUnLock(&deviceList, operation, status, flagAbort, NULL);

    CmnVecFree(&deviceList);
end:
    FREE(host_id);
    DbgFcnOut();

    return retVal;
} /* MsgHostLockUnLock */



int SseMsgLockUnLock(
    CmnVec  *deviceIdentifierList,
    int     operation,
    int     *status,
    int     *flagAbort,
    CmnVec  *unLockedDeviceIdentifierList
)
{
    int   retVal = RETURN_OK;
    int   number = 0; /* number of devices for locking */
    tchar *host_id=NULL;
    int   sndsize = 0;
    int   readMessage = 0;
    int   rcvsize = 0;
    tchar **msgArg=NULL;
    int   msgArgCount = 0;  
    int   i = 0;
    int   nr_successfull = 0;
    int   j=0;
    

    ERH_FUNCTION (_T("MsgLockUnLock"));
    
    DbgFcnInEx(__FLND__, 
        _T("operation = %d"), operation);

    DbgStamp (DBG_SSE_ACTION);

    
    /* Check input parameter */
    if ( (deviceIdentifierList == NULL) ||   (status == NULL)   ||  (flagAbort == NULL))
    {
        ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters!"), __FCN__);
        retVal = RETURN_ER;
        goto end;
    }

    /* Initialization */
    *status = 0;
    *flagAbort = 0;
    /* Get the number of device */
    number = CmnVecH(deviceIdentifierList);
 
    /* Start constructing the msg */
    StrMsgStart (USE_GLOBAL_BUFFER, AUTO_SIZE, MSG_RESLOCK_REQUEST);
    StrMsgFAppend (_T("%d%d%d"), operation, 0, 0);
    StrMsgFAppend (_T("%d"), number);

    /* For each device append the device identifier element and Resource lock type */
    for (i=0; i<number; i++)
    {
        tchar *deviceIdentifierElement = (tchar *)CmnVecI(deviceIdentifierList, i);
        DbgPlain (DBG_SSE_ACTION, _T("[%s] deviceIdentifierElement = %s!"), 
                    __FCN__, NS(deviceIdentifierElement));
     
        StrMsgFAppend (_T("%d"), RESLOCK_RESTYPE_HPSURESTOREXP_LDEV);
        StrMsgFAppend (_T("%s"), deviceIdentifierElement);
    }

    /* Send message to Session Manager */
    SMB_SEND_TO_CON_UNCOND ();
    
    /* Wait for  LOCK/UNLOCK reply from Session Manager */
 
    for (j=0; j<2; j++)
    {
	/* Receive the buffer */
        rcvsize = IpcReceiveData (ConIpcHandle, 
                                  USE_GLOBAL_BUFFER,
                                  AUTO_SIZE);
    
        if (rcvsize <= 0)
        {
            ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);

            DbgStamp (DBG_UNEXPECTED);
            DbgPlain (DBG_UNEXPECTED,
                      _T("[%s] From CON: read %d; ERROR[%d]"),
                      __FCN__, rcvsize, ErhErrno());
            
            IpcCloseConnection (ConIpcHandle);
            ConIpcHandle = -1;

            retVal = RETURN_ER;
            goto end;
        }
        /* parse the msg */
        readMessage = StrParseStart (USE_GLOBAL_BUFFER);

        
        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG, _T("[%s] Got reply %s"),
                  __FCN__, MsgPrintMSGConstant (readMessage));

        /* Did we get a LOCK reply or something else  */
        if (readMessage == MSG_RESLOCK_REQUEST_STATUS)
        {
            break;
        }

        /* Wait for LOCK/UNLOCK reply from SM. 
           Because abort has happened try ones again to 
           get lock/unlock reply */

        DbgPlain (DBG_SSE_MSG, _T("[%s] Unexpected reply %s"), __FCN__, MsgPrintMSGConstant (readMessage));
        
        (*flagAbort)++;

    }

    if (readMessage != MSG_RESLOCK_REQUEST_STATUS)
    {
        DbgPlain (DBG_UNEXPECTED, 
                  _T("[%s] didn't get lock/unlock reply"), 
                  __FCN__);
        goto end;

    }
   

    /* Unpack the ErhStat structure from the message */
    if (ErhUnpackMsg () < 0)
    {
        ErhMark (ERH_INTERNAL, __FCN__, ERH_CRITICAL);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Cannot unpack msg"), __FCN__);
        retVal = RETURN_ER;;
        goto end;
    }

    msgArgCount = FLD_RESLOCK_REQUEST_STATUS_NR + number;
    msgArg = (tchar **) MALLOC (msgArgCount * sizeof (tchar *));


    if (StrParseMessage (msgArgCount, msgArg) != 0)
    {
        ErhMark (ERH_MFORMAT, __FCN__, ERH_CRITICAL);
        retVal = RETURN_ER;
        goto end;
    }
    
    
    if (number != atoi (msgArg[FLD_RESLOCK_REQUEST_STATUS_N]))
    {
        ErhMark (ERH_MFORMAT, __FCN__, ERH_CRITICAL);
        retVal = RETURN_ER;
        goto end;
    }

    /* We receive only SUCCESS/FAILED status for each object */
    for (i=0; i < number; i++)
    {
       if (RESLOCK_STATUS_SUCCESS == atoi (msgArg[FLD_RESLOCK_REQUEST_STATUS_NR + i]))
       {
                nr_successfull++;
       }
       else
       {
           if (unLockedDeviceIdentifierList != NULL)
           {
                tchar *deviceIdentifierElement = (tchar *)CmnVecI(deviceIdentifierList, i);
                DbgPlain (DBG_SSE_ACTION, _T("[%s] Cannot lock device %s!"), 
                            __FCN__, NS(deviceIdentifierElement)); 
                CmnVecAddStr(unLockedDeviceIdentifierList, deviceIdentifierElement);
           }
       }
    }
           
    *status = (nr_successfull == number);
    DbgStamp (DBG_SSE_MSG);
    DbgPlain (DBG_SSE_MSG, _T("[%s] lock/unlock status = %d, number of dev in req: %d, number of successful:%d  !"),
            __FCN__, *status, number, nr_successfull);
 
    /* Check if lock/unlock failed*/
    if (nr_successfull != number)
    {
        DbgStamp (DBG_SSE_MSG);
        DbgPlain (DBG_SSE_MSG, _T("[%s] Operation (lock/unlock) failed!"),__FCN__);
    }

end:

  
    if (*flagAbort != 0)
    {
        DbgPlain (DBG_SSE_MSG, 
                  _T("[%s] Unexpected reply -> ABORTING"), 
                  __FCN__);
        
         retVal = RETURN_ER; 
    }

    FREE(host_id);

    FREE(msgArg);

    DbgFcnOut();
    return retVal;

} /* MsgLockUnLock */


int UnLockBckpHost(
    int *retStatus,
    int *retFlagAbort
)
{
    int         retVal = RETURN_OK;
    int status = 0;
    int flagAbort=0;

    ERH_FUNCTION (_T("UnLockBckpHost"));

    DbgFcnIn();
    if (retStatus == NULL || retFlagAbort == NULL)
    {
        /* Invalid parameter(s) */
        ErhMark (ERH_SSE_INVPARAM, __FCN__, ERH_MAJOR);
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED, _T("[%s] Invalid parameters (retStatus = %p, retFlagAbort = %p)!"), 
                                        __FCN__, retStatus, retFlagAbort);
        retVal = RETURN_ER;
        goto end;
    }

    if (SseMsgHostLockUnLock(opt.hostname,
                        SSE_LOCK_COMMAND_EXEC,
                        SSE_HOST_UNLOCK,
                        &status,
                        &flagAbort) != RETURN_OK)
    { 
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] MsgHostLockUnLock failed!  on %s"), 
            __FCN__, opt.hostname);
        
        if ( flagAbort != 0)
        {
            retVal = RETURN_ER;
        }
        else
        {
            ErhFullReport(__FCN__, ERH_CRITICAL, 
                          NlsGetMessage (
                                     NLS_SET_SSE, 
                                     NLS_SSE_HOST_UNLOCKING_FAILED,
                                     ErhErrnoToText()
                          )
			); 

            ErhClearError();
            retVal = RETURN_ER;
        }

        goto end;
    }
    else if (status == 0)
    {
        DbgStamp (DBG_UNEXPECTED);
        DbgPlain (DBG_UNEXPECTED,
            _T("[%s] Can't unlock backup host!"), 
            __FCN__);

        ErhFullReport(__FCN__, ERH_WARNING, 
                      NlsGetMessage (
                             NLS_SET_SSE,
                             NLS_SSE_HOST_NOT_UNLOCKED
                      )
        );
    }

end:
    *retStatus = status;
    *retFlagAbort = flagAbort;
    
    DbgFcnOutRet(retVal);    
    return(retVal);
}/* UnLockDriveScanOnBckpHost */
/* END    QUIX ID:QXCR10000288643 Added By : Umesh (Emp Id : 20130476) */
#endif
