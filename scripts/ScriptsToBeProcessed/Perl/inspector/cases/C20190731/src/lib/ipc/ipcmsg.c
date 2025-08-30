/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Interprocess Communications Module (ipc)
* @file      lib/ipc/ipcmsg.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     This is an additional layer for ipc module. It contains functions
*            which simplify and potentially optimize sending of OmniBack messages
*            (created by StrMsg module).
*
* @since     26.06.1998  Lars        Initial Coding
*/

#include "lib/cmn/target.h"
#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "lib/ipc/ipc_impl.h"

__rcsId("$Header: /lib/ipc/ipcmsg.c $Rev$ $Date::                      $:");


static int SecIpcHandleReconnectStrMsg(int size,
    IpcHandle Handle,
    struct StrMsg *strMsg,
    int timeout);

static int SecIpcHandleReconnectMsgData(int size,
    IpcHandle Handle,
    void *buffer,
    int buffer_size,
    int timeout);


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @retval    same as IpcSendData
*
* @brief     - uses members of StrMsg structure to determine buffer and size
*            - message is adjusted for outbound if platform requires it
*
*//*=========================================================================*/
int
IpcSendStrMsg (
    IpcHandle       Handle
)
{
#ifdef ADJUST_OUTBOUND_MESSAGES
    StrMsgAdjustOutbound((IpcGetStrMsg(Handle))->msgBuf);
#endif
    return IpcSendData(Handle, NULL, 0);
}


int
IpcSendStrMsgL(IpcHandle Handle, const StrMsg *strMsg)
{
    int size;
    
    if (!strMsg) strMsg = StrMsgGetGlobal();

#ifdef ADJUST_OUTBOUND_MESSAGES
    StrMsgAdjustOutbound(strMsg->msgBuf);
#endif

    size = (strMsg->msgPos + 1) * sizeof(tchar);

    return IpcSendData(Handle, strMsg->msgBuf, size);
}


int
IpcSendStrMsgAsync (
    IpcHandle       Handle
)
{
#ifdef ADJUST_OUTBOUND_MESSAGES
    StrMsgAdjustOutbound((IpcGetStrMsg(Handle))->msgBuf);
#endif
    return IpcSendAsync(Handle, NULL, 0);
}


int
IpcSendStrMsgAsyncL (
    IpcHandle            Handle,
    const struct StrMsg *strMsg
)
{
    int size;

#ifdef ADJUST_OUTBOUND_MESSAGES
    StrMsgAdjustOutbound(strMsg->msgBuf);
#endif

    size = (strMsg->msgPos + 1) * sizeof(tchar);

    return IpcSendAsync(Handle, strMsg->msgBuf, size);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @retval    same as IpcReceiveDataBase
*
* @brief     - There is no Async version (so far no need for it)
*
*//*=========================================================================*/

static int
ipcI_ReceiveStrMsgExL(IpcHandle Handle,
    struct StrMsg   *strMsg,
    int             timeout
)
{
    ERH_FUNCTION(_T("ipcI_ReceiveStrMsgExL"));
    IpcEntry *TH = IpcGetEntry(Handle, __FCN__);
    int retval;
    DbgFcnInEx(FCN_IPCLL, _T(""));

    if (TH == NULL) RETURN_INT(-1);

    /* tricky, we need to temporarily set ipc handle to use this StrMsg */
    /* (NOTE: we only 'treat' the receive-specific StrMsg) */
    {
        StrMsg *oldStrMsg = TH->strMsg4Recv;  /* store */
        TH->strMsg4Recv = strMsg;     /* set */
        retval = ipc_receiveDataExBase(Handle, NULL, 0, timeout);
        TH->strMsg4Recv = oldStrMsg;  /* restore */
    }
    RETURN_INT(retval);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     - Uses StrMsg
*            - There is no Async version (so far no need for it)
*
* @remarks   handles secure reconnect
*
*//*=========================================================================*/

int
IpcReceiveStrMsgExL(IpcHandle Handle,
    struct StrMsg   *strMsg,
    int             timeout)
{
    ERH_FUNCTION(_T("IpcReceiveStrMsgExL"));
    int size = 0;

    DbgFcnInEx(FCN_IPCLL, _T(""));
    size = ipcI_ReceiveStrMsgExL (Handle, strMsg, timeout);
    /* check for secure reconnect */
    if (size > 0)
    {
        size = SecIpcHandleReconnectStrMsg(size, Handle, strMsg, timeout);
    }
    RETURN_INT(size);
}

/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     strMsg
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     wrapper for IpcReceiveStrMsgExL
*
*//*=========================================================================*/
int
IpcReceiveStrMsgL(IpcHandle Handle, StrMsg *strMsg)
{
    return(IpcReceiveStrMsgExL(Handle, strMsg, 0));
}

/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Receive StrMsg to internal buffer and handle reconnect
*
*//*=========================================================================*/

int
IpcReceiveStrMsg(IpcHandle Handle)
{
    ERH_FUNCTION(_T("IpcReceiveStrMsg"));
    int size = 0;
    DbgFcnInEx(FCN_IPCLL, _T(""));
    size = IpcReceiveDataBase(Handle, NULL, 0);
    /* check for secure reconnect */
    if (size > 0)
    {
        size = SecIpcHandleReconnectStrMsg(size, Handle, NULL, 0);
    }
    RETURN_INT(size);
}



/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     size
* @param     Handle
* @param     strMsg
* @param     timeout
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Handle reconnect
*
* @remarks   When a socket descriptor is passed from one process to
*            another the security context has to be re-established.
*
*//*=========================================================================*/

int
SecIpcHandleReconnectStrMsg(int        size,
                            IpcHandle  Handle,
                            StrMsg    *strMsg,
                            int        timeout)
{
    ERH_FUNCTION(_T("SecIpcHandleReconnectStrMsg"));
    IpcEntry *th = IpcGetEntry(Handle, __FCN__);
    if (!th)
        return -1;
    
    if (!th->ssl)
        return size;
    
    DbgFcnInEx(FCN_IPCLL, _T(""));
    while (size > 0)
    {
        StrMsg *msg = strMsg? strMsg : IpcGetReceiveStrMsg(Handle);
        int msgId = StrParseMessageID(ipcI_GetStrMsgBuf(msg));
        int ret = 0;

        DBGPLAIN(DBG_IPCLL, _T("[%s] received: %d"), __FCN__, msgId);
        if (msgId != MSG_SSL_RECONNECT)
            RETURN_INT(size);

        DBGPLAIN(DBG_IPCLL, _T("[%s] received reconnect"), __FCN__);
        ret = SecIpcConnect(Handle);
        if (ret != 0)
            RETURN_INT(-1);

        /* receive again */
        if (strMsg)
        {
            size = ipcI_ReceiveStrMsgExL(Handle, strMsg, timeout);
        }
        else
        {
            size = ipc_receiveDataExBase(Handle, NULL, 0, timeout);
        }
    }

    RETURN_INT(size);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
*
* @retval    same as IpcSendData
*
* @brief     Send a reconnect message
*
* @remarks   When a socket descriptor is passed from one process to
*            another the security context has to be re-established.
*
*//*=========================================================================*/

int
SecIpcSendReconnect(IpcHandle Handle)
{
    ERH_FUNCTION(_T("SecIpcSendReconnect"));
    int ret = 0;
    unsigned noack;
    StrMsg strMsg = { 0 };
    IpcEntry *th = IpcGetEntry(Handle, __FCN__);

    if (!th)
        return -1;

    if (!th->ssl)
        return(0);

    DbgFcnInEx(FCN_IPCLL, _T(""));

    StrMsgStartL(&strMsg, NULL, 0, MSG_SSL_RECONNECT);
    noack = th->recon.noack;
    th->recon.noack = 1;
    ret = IpcSendStrMsgL(Handle, &strMsg);
    th->recon.noack = noack;

    RETURN_INT(ret);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     buffer
* @param     buffer_size
* @param     timeout
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Receive Data and handle reconnect
*
*//*=========================================================================*/

int
IpcReceiveMsgDataEx(IpcHandle Handle,
                    void *buffer,
                    int buffer_size,
                    int timeout)
{
    int ret = 0;
    ret = ipc_receiveDataExBase(Handle, buffer, buffer_size, timeout);
    if (ret > 0)
    {
        ret = SecIpcHandleReconnectMsgData(ret, Handle, buffer, buffer_size, timeout);
    }
    return(ret);
}

/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     buffer
* @param     buffer_size
* @param     timeout
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Wrapper for IpcReceiveMsgDataEx
*
*//*=========================================================================*/

int
ipc_receiveDataEx(IpcHandle Handle,
                  void *buffer,
                  int buffer_size,
                  int timeout)
{
    return(IpcReceiveMsgDataEx(Handle, buffer, buffer_size, timeout));
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     buffer
* @param     buffer_size
*
* @retval    same as IpcReceiveMsgDataEx
*
* @brief     Wrapper for IpcReceiveMsgDataEx
*
*//*=========================================================================*/
int
IpcReceiveData(IpcHandle Handle,
               void *buffer,
               int buffer_size)
{
    return IpcReceiveMsgDataEx(Handle, buffer, buffer_size, 0);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     size
* @param     Handle
* @param     buffer
* @param     buffer_size
* @param     timeout
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Handle reconnect
*
* @remarks   When a socket descriptor is passed from one process to
*            another the security context has to be re-established.
*
*//*=========================================================================*/
int
SecIpcHandleReconnectMsgData(int        size,
                             IpcHandle  Handle,
                             void      *buffer,
                             int        buffer_size,
                             int        timeout)
{
    ERH_FUNCTION(_T("SecIpcHandleReconnectMsgData"));
    IpcEntry *th = IpcGetEntry(Handle, __FCN__);

    if (!th)
        return(-1);
    
    if (!th->ssl)
        return size;

    DbgFcnInEx(FCN_IPCLL, _T("handle:%d"), Handle);

    while (size > 0)
    {
        tchar *msgbuf = buffer;
        int msgId = 0;
        int ret = 0;
        if (msgbuf == NULL)
        {
            msgbuf = ipcI_GetStrMsgBuf(IpcGetReceiveStrMsg(Handle));
        }
        msgId = StrParseMessageID(msgbuf);
        if (msgId != MSG_SSL_RECONNECT)
            RETURN_INT(size);

        DBGPLAIN(DBG_IPCLL, _T("[%s] received reconnect"), __FCN__);
        ret = SecIpcConnect(Handle);
        if (ret != 0)
            RETURN_INT(-1);

        /* receive again */
        size = ipc_receiveDataExBase(Handle, buffer, buffer_size, timeout);
    }
    RETURN_INT(size);
}

/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Handle reconnect
*
* @remarks   When a socket descriptor is passed from one process to
*            another the security context has to be re-established.
*
*//*=========================================================================*/
int 
SecIpcExpectReconnectEx(IpcHandle Handle)
{
    ERH_FUNCTION(_T("SecIpcExpectReconnectEx"));

    /* OB2INETRECONNECTTIMEOUT is by default set to 60 seconds.
       In very slow processing clients to start a process and pass the socket and
       to the newly started process to connect back, we need this time.*/

    int timeout = 60;

    EnvReadInt(_T("OB2INETRECONNECTTIMEOUT"), &timeout);

    return SecIpcExpectReconnect(Handle, timeout);
}

/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     timeout
*
* @retval    same as ipc_receiveDataExBase
*
* @brief     Handle reconnect
*
* @remarks   When a socket descriptor is passed from one process to
*            another the security context has to be re-established.
*
*//*=========================================================================*/
int 
SecIpcExpectReconnect(IpcHandle Handle, int timeout)
{
    ERH_FUNCTION(_T("SecIpcExpectReconnect"));
    struct StrMsg *msgRecv = NULL;
    int msgId = 0;
    int hold = 0;
    void *holdBuf = NULL;
    int holdSize = 0;
    IpcEntry *th = IpcGetEntry(Handle, __FCN__);
    
    DbgFcnInEx(FCN_IPC, _T("handle:%d"), Handle);
    if (!th)
        RETURN_INT(-1);
    
    if (!th->ssl)
        RETURN_INT(0);

    while (1)
    {
        /* peek */
        int ret = ipc_peekDataEx(Handle, NULL, 0, timeout);
        if (ret <= 0)
        {
            FREE(holdBuf);
            RETURN_INT(-1);
        }
        msgRecv = IpcGetReceiveStrMsg(Handle);
        /* check if MSG_SSL_RECONNECT */
        msgId = StrParseStartL(msgRecv, NULL);
        if (msgId != MSG_SSL_RECONNECT)
        {
            DBGPLAIN(DBG_IPC, _T("[%s] WARNING: expected reconnect, received: %d"), __FCN__, msgId);
            if (hold)
            {
                /* error: can't hold more than 1 message */
                FREE(holdBuf);
                RETURN_INT(-1);
            }
            /* hold message and try again */
            holdSize = ret;
            holdBuf = REALLOC(holdBuf, holdSize);
            IpcReceiveDataBase(Handle, holdBuf, holdSize);
            hold = 1;
            continue;
        }
        DBGPLAIN(DBG_IPCLL, _T("[%s] received reconnect"), __FCN__);
        ret = SecIpcConnect(Handle);
        if (ret != 0)
        {
            FREE(holdBuf);
            RETURN_INT(-1);
        }
        if (hold)
        {
            /* push hold message to peek buffer */
            IpcPeekDataPush(Handle, holdBuf, holdSize);
        }
        else
        {
            /* clear peek data */
            IpcReceiveDataBase(Handle, NULL, 0);
        }
        break;
    } /* while */

    FREE(holdBuf);
    RETURN_INT(0);
}


/*========================================================================*//**
*
* @ingroup   Ipc
*
* @param     Handle
* @param     timeout
*
* @retval    2 reconnect handled
*            1 connection not ssl or message received is not MSG_SSL_RECONNECT
*            0 no data / connection closed
*            -1 error
*
* @brief     Peek and handle reconnect
*
* @remarks   A message is peek-ed and if it is reconnect
*            then reconnect is made, peek buffer cleared and return is 2
*            else the message stays in the peek buffer and return is 1
*
*//*=========================================================================*/
int 
SecIpcCheckReconnect(IpcHandle Handle, int timeout)
{
    ERH_FUNCTION(_T("SecIpcCheckReconnect"));
    int msgId;
    int ret;
    IpcEntry *th = IpcGetEntry(Handle, __FCN__);

    DbgFcnInEx(FCN_IPC, _T(""));
    if (!th)
        RETURN_INT(-1);

    if (!th->ssl)
        RETURN_INT(1);

    /* peek */
    ret = ipc_peekDataEx(Handle, NULL, 0, timeout);
    if (ret <= 0)
    {
        RETURN_INT(ret);
    }

    /* check if MSG_SSL_RECONNECT */
    msgId = StrParseMessageID(StrMsgGetRBufL(IpcGetReceiveStrMsg(Handle)));
    if (msgId != MSG_SSL_RECONNECT)
    {
        DBGPLAIN(DBG_IPC, _T("[%s] received: %d"), __FCN__, msgId);
        RETURN_INT(1);
    }
    DBGPLAIN(DBG_IPCLL, _T("[%s] received MSG_SSL_RECONNECT"), __FCN__);
    ret = SecIpcConnect(Handle);
    if (ret != 0)
        RETURN_INT(-1);

    /* clear peek data */
    IpcReceiveDataBase(Handle, NULL, 0);

    RETURN_INT(2);
}



/*========================================================================*//**
 *
 * @ingroup   Ipc
 *
 * @param     handle    - handle where message came from/went to
 * @param     msgName   - name of the message (can be NULL)
 * @param     outgoing  - indicates whether a message is outgoing or incoming
 * @param     ...       - names of message fields, last must be NULL.
 *
 * @retval    none (void)
 *
 * @brief     Dump the DP message with details into debugs
 *
 *//*=========================================================================*/
void
IpcParseMessageToDebugsL(
    IpcHandle     handle,
    const StrMsg *strMsg,
    ctchar       *msgName,
    BOOL          outgoing,
    ...)
{
    ERH_FUNCTION(_T("IpcParseMessageToDebugsL"));
    StrMsg  m = {0};
    int     msgNo = -1;
    int     i;
    tchar  *msgbuf = NULL;
    tchar  *fieldNames[256] = {0};

    va_list va;
    IpcEntry *entry = IpcGetEntry(handle, __FCN__);

    if (entry == NULL) return;

    /* skip if debugging not active or level doesn't match the ranges */
    if (!DBGACTIVE) return;
    if (!DbgMatch(DBG_IPCLOW)) return;

    va_start(va, outgoing);
    for (i=0; (fieldNames[i] = va_arg(va,tchar*)) != NULL; ++i);
    va_end(va);

    msgbuf = strMsg->rcvBuf;
    if (msgbuf == NULL)
    {
        msgbuf = strMsg->msgBuf;
        if (msgbuf == NULL)
        {
            DbgPlain(DBG_IPCLOW, _T("StrParseMessageToDebugs: strMsg==NULL"));
            return;
        }
    }


    msgNo = StrParseStartL(&m, msgbuf);
    if (msgNo != -1)
    {
        int fld;
        tchar peerName[STRLEN_STD+1];

        StrPeerType(IpcGetPeerType(handle), peerName);

        if (StrIsEmpty(msgName))
        {
            DbgPlain(DBG_IPCLOW,
                _T("===============================\n")
                _T("%s[%d]: MSG no. %d %s %s on %s\n")                /* IN/OUT[handle]: <msg ID> from/to <peer> on <host> */
                _T("==============================="),
                outgoing ? _T("OUT") : _T("IN"),
                handle,
                msgNo,
                outgoing ? _T("to") : _T("from"),
                StrIsEmpty(peerName) ? _T("<unknown peer>") : peerName,
                StrIsEmpty(entry->peerHost) ? _T("<unknown host>") : entry->peerHost);
        }
        else
        {
            DbgPlain(DBG_IPCLOW,
                _T("===============================\n")
                _T("%s[%d]: %s(%d) %s %s on %s\n")                  /* IN/OUT[handle]: <msgname + ID> from/to <peer> on <host> */
                _T("==============================="),
                outgoing ? _T("OUT") : _T("IN"),
                handle,
                msgName,
                msgNo,
                outgoing ? _T("to") : _T("from"),
                StrIsEmpty(peerName) ? _T("<unknown peer>") : peerName,
                StrIsEmpty(entry->peerHost) ? _T("<unknown host>") : entry->peerHost);
        }


        for (i=fld=0; ;i++)
        {
            ctchar *token = StrParseNextL(&m);
            if (token == NULL)
                break;

            if (fieldNames[fld])
            {
                DbgPlain(DBG_IPCLOW, _T("| %s\t= %s"), fieldNames[fld], token);
                fld++;
            }
            else
            {
                DbgPlain(DBG_IPCLOW, _T("| <FLD %2d>  =  %s"), i, token);
            }
        }

        DbgPlain(DBG_IPCLOW, _T("------------------------------"));
    }
    else
    {
        DbgLogFull(IPCLOG, _T("[%s] INVALID MESSAGE (-1)"), __FCN__);
        DbgDumpStack(DBG_UNEXPECTED);
    }

    StrMsgFreeL(&m);
}


