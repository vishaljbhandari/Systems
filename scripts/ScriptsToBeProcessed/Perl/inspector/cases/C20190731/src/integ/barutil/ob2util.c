/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   OB2 BAR Services
* @file      integ/barutil/ob2util.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Utility functions for integrations.
*
* @since     14.03.2000    Matej Kenda     Initial version.
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /integ/barutil/ob2util.c $Rev$ $Date::                      $:") ;
#endif

#define HAVE_VARIANT

#if TARGET_WIN32
    #include <Userenv.h>
    #include <Ntsecapi.h>
    #pragma comment (lib, "Userenv")
#endif

#include "lib/cmn/common.h"
#include "lib/cmn/containers.h"
#include "lib/cmn/argvmgr.h"
#include "cs/csa/csa.h"
#include "ob2util.h"
#include "integ/bar2/ob2expdefines.h"

void
BP_ErhMarkM(int bpretval, const tchar *function, int level, const tchar *file, int line)
{
    if (ErhErrno() != 0 || bpretval == BP_NOERROR)
    {
        return;
    }

    switch (bpretval)
    {
    case BPERR_INVTOKTYPE:
    case BPERR_INVTOK:
    case BPERR_UNEXPEOF:
    case BP_EOF:
        ErhMarkM (ERH_FFORMAT, 0, function, level, file, line);
        break;

    case BPERR_FILEIO:
        ErhMarkM (ERH_SYSERR, GetLastError(), function, level, file, line);
        break;

    case BPERR_OUTOFMEM:
        ErhMarkM (ERH_NOMEM, 0, function, level, file, line);
    }
}

int
BU_CheckUser(const tchar *cellServer, IN int check_for_privateobj)
{
    ERH_FUNCTION(_T("BU_CheckUser"));
    int    handle = -1;
    int    status;
    ULONG  userRights;

    DbgFcnIn();

    handle = MCsaBindServer(cellServer);
    if (handle < 0)
    {
        RETURN_INT(OBE_ERR);
    }

    status = MCsaGetAcl(handle, &userRights);
    if (status == -1)
    {
        ErhPromoteError (EBSM_NOUSER);
        RETURN_INT (OBE_NOPERM);
    }

    if (check_for_privateobj == USER_CAN_SEE_PRIVATE_OBJ)
    {
        if (!CanSeeBarPrivateObjects(userRights))
        {
            ErhMarkMajor (ERH_DENIED);
            RETURN_INT (OBE_NOPERM);
        }
    }

    RETURN_INT (OBE_OK);
}

/* ===========================================================================
|   CSA wrappers
 ========================================================================== */

INLINE int BU_CsaRead (const csa_t *csa, const tchar *filename, tchar **contents)
{
    int status = MCsaGetGroupItem(csa->handle, G_TOP_INTEG_CNF, filename, contents, MODIFY);
    return status==0? 0 : BPERR_ERRNO;
}


INLINE int BU_CsaWrite (const csa_t *csa, const tchar *filename, const tchar *contents)
{
    int status = MCsaPutGroupItem(csa->handle, G_TOP_INTEG_CNF, filename, contents, UNLOCK);
    return status==0? 0 : BPERR_ERRNO;
}
 
void 
BU_CsaExit (IN OUT csa_t *ctx)
{
    if (ctx->init)
        CsaExit ();
}

int 
BU_CsaInit (OUT csa_t *ctx, IN const tchar *cell)
{
    if (!cell)
        cell = OB2_CellServer();

    if (!CsaIsInitialized())
    {
        if (MCsaInit(cell)!=0)
            return BPERR_ERRNO;
        ctx->init = 1;
    }

    ctx->handle = MCsaBindServer(cell);
    if (ctx->handle < 0)
    {
        BU_CsaExit (ctx);
        return BPERR_ERRNO;
    }

    return 0;
}

/*========================================================================*//**
*
* @ingroup   BARUTIL
*
* @param     tchar   *user       (IN)    user name
* @param     tchar   *domain     (IN)    domain name
* @param     BOOL    *existed    (IN/OUT) set by add priv., and used by remove priv.
* @param     BOOL    addRights   (IN)    TRUE will add privileges, FALSE will remove priv.
*
* @brief     Add or remove 'logon as a service' privilege to specified account
*
*//*=========================================================================*/
#if TARGET_WIN32

#include <Userenv.h>
#include <Ntsecapi.h>

#define DbgError(_func, _error) { \
    int error = _error; \
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] OS call %s failed with %s"), \
        __FCN__, _T(#_func), ErhSysErrnoToText(error)); \
    retval = error; \
}

#define DbgOSError(_func) DbgError(_func, GetLastError())
#define DbgLsaError(_func,_status) { SetLastError(LsaNtStatusToWinError(_status)); DbgError(_func, GetLastError()); }

static int Win32_ChangeAccountRights (
    IN     const tchar *user, 
    IN     const tchar *domain,
    IN     const tchar *privName, 
    IN OUT BOOL        *existed, 
    IN     BOOL         addRights)
{
    ERH_FUNCTION(_T("Win32_ChangeAccountRights"));
    LSA_OBJECT_ATTRIBUTES objAttributes = {0};
    LSA_UNICODE_STRING    priv;
    LSA_HANDLE            lsaHandle = NULL;
    BOOL         ok;
    PSID         userSid = NULL;
    DWORD        sidSize = 128;
    DWORD        refDomainLen = 64;
    tchar       *account   = NULL;
    LPTSTR       refDomain = NULL;
    SID_NAME_USE sidUse;
    int          status;
    int          retval = 0;

    DbgFcnInEx(__FLND__, _T("user:%s domain:%s addRights:%d"), NS(user), NS(domain), addRights);

    if (StrIsEmpty(user) || StrIsEmpty(domain) || !existed)
        RETURN_INT (ERROR_INVALID_PARAMETER);

    /* if in remove mode and privilege existed(not granted by us) than skip */
    if (!addRights && *existed)
        RETURN_INT (ERROR_SUCCESS);

    status = LsaOpenPolicy(
        NULL,           /* open policy on local machine */
        &objAttributes, /* connection att. members not used */
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, /* LsaAddAccountRights needs these rights */
        &lsaHandle      /* handle to policy object */
    );

    if (status != ERROR_SUCCESS) 
    {
        DbgLsaError (LsaOpenPolicy, status);
        goto exit;
    }

    account = StrNewCopy(domain);
    StrAppend(account, _T("\\"));
    StrAppend(account, user);
    DbgPlain(DBG_MAIN_ACTION, _T("Get SID of %s"), account);

    do {
        userSid = REALLOC (userSid, sidSize);
        if (!userSid)
        {
            retval = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        refDomain = REALLOC (refDomain, refDomainLen*sizeof(tchar));
        if (!refDomain)
        {
            retval = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
 
        ok = LookupAccountName(NULL, account, userSid, &sidSize, refDomain, &refDomainLen, &sidUse);
    } while (!ok && (GetLastError() == ERROR_INSUFFICIENT_BUFFER));

    if (!ok)
    {
        DbgOSError (LookupAccountName);
        goto exit;
    }

    priv.Buffer        = (tchar*)privName;
    priv.Length        = (USHORT) (StrLen(privName) * sizeof(tchar));
    priv.MaximumLength = (USHORT) (priv.Length + sizeof(tchar));

    /* grant 'logon as a service' privileges to account with userSid */
    if (addRights == TRUE)
    {
        ULONG i, cnt = 0;
        PLSA_UNICODE_STRING privStrEnum = NULL;
        *existed = FALSE;

        /* check if userSid already has 'logon as a service' privileges */
        status = LsaEnumerateAccountRights(lsaHandle, userSid, &privStrEnum, &cnt);
        if (status != ERROR_SUCCESS)
        {
            DbgLsaError (LsaEnumerateAccountRights, status);
            /* in case user is not mentioned in policy enumerate call will return ERROR_FILE_NOT_FOUND...skip */
            if (LsaNtStatusToWinError(status) == ERROR_FILE_NOT_FOUND)
                retval = ERROR_SUCCESS;
        }

        for (i=0; i<cnt; i++)
        {
            if (StrCmp(privStrEnum[i].Buffer, privName) == 0)
            {
                DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] This user has %s rights already in the policy."), 
                    __FCN__, privName);
                *existed = TRUE;
                retval = ERROR_SUCCESS;
                goto exit;
            }
        }

        status = LsaAddAccountRights(lsaHandle, userSid, &priv, 1);
        if (status != ERROR_SUCCESS)
        {
            DbgLsaError(LsaAddAccountRights, status);
            goto exit;
        }

        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] %s rights granted."), __FCN__, privName);
    }


    /* remove 'logon as a service' privileges to account with userSid */
    else if (!addRights && !*existed)
    {
        status = LsaRemoveAccountRights(lsaHandle, userSid, FALSE, &priv, 1);
        if (status != ERROR_SUCCESS)
        {
            DbgLsaError (LsaRemoveAccountRights, status);
            goto exit;
        }

        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] %s rights removed."), __FCN__, privName);
    }

exit:
    if (lsaHandle != NULL)
        status = LsaClose(lsaHandle);

    FREE (account);
    FREE (userSid);
    FREE (refDomain);

    RETURN_INT (retval);
}



/*========================================================================*//**
*
* @retval    0         - OK
* @retval    otherwise - OS error code
*
*//*=========================================================================*/
int BU_ImpersonateUser(
    IN  const tchar *user, 
    IN  const tchar *domain,
    IN  const tchar *pass, 
    OUT HANDLE      *token)
{
    ERH_FUNCTION(_T("BU_ImpersonateUser"));
    int              retval  = 0;
    HANDLE           process = NULL;
    BOOL             ok;
    LUID             luid;
    PROFILEINFO      profile = {0};
    TOKEN_PRIVILEGES privileges;
    BOOL             existed = FALSE;

    DbgFcnInEx(__FLND__, _T("user:%s domain:%s"), NS(user), NS(domain));
    
    if (StrIsEmpty(user) || StrIsEmpty(domain))
        RETURN_INT (ERROR_INVALID_PARAMETER);

    /* grant 'logon as a service' privileges to domain\user, if not already granted */
    if (Win32_ChangeAccountRights(user, domain, SE_SERVICE_LOGON_NAME, &existed, TRUE) != ERROR_SUCCESS)
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Error adding accnt rights."), __FCN__);
    }

    /* Open access token of current process */
    ok = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY|TOKEN_DUPLICATE, &process);
    if (!ok)
    {
        DbgOSError (OpenProcessToken);
        goto exit;
    }

    /* Retrieves the locally unique identifier */
    ok = LookupPrivilegeValue(NULL, SE_TCB_NAME, &luid);
    if (!ok)
    {
        DbgOSError (LookupPrivilegeValue);
        goto exit;
    }

    privileges.PrivilegeCount           = 1;
    privileges.Privileges[0].Luid       = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    /* Enable privileges TokenPriv to access token */
    ok = AdjustTokenPrivileges(process, FALSE, &privileges, 0, NULL, NULL);
    if (!ok)
    {
        DbgOSError (AdjustTokenPrivileges);
        goto exit;
    }

    /* Logon as user to local computer */
    ok = LogonUser(
        (tchar*)user,
        (tchar*)domain,
        (tchar*)pass,
        LOGON32_LOGON_SERVICE,
        LOGON32_PROVIDER_DEFAULT, 
        token);

    if (!ok)
    {
        DbgOSError (LogonUser);
        goto exit;
    }

    profile.dwSize     = sizeof(PROFILEINFO);
    profile.lpUserName = (tchar*)user;
    profile.dwFlags    = PI_NOUI;
    /* Load user's profile */
    retval = LoadUserProfile(*token, &profile);
    if ((retval == ERROR_INVALID_PARAMETER) || !retval) 
    {
        DbgOSError (LoadUserProfile);
        goto exit;
    }

    /* Impersonate security context of a logged user */
    if (!ImpersonateLoggedOnUser(*token))
    {
        DbgOSError (ImpersonateLoggedOnUser);
        goto exit;
    }

    /* Close registry so NT will 'refresh' hives using newly impersonated user ID. */
    if (RegCloseKey(HKEY_CURRENT_USER) != ERROR_SUCCESS)
    {
        DbgOSError (RegCloseKey);
        goto exit;
    }

    retval = ERROR_SUCCESS;
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] I'm completely different user now."), __FCN__);

    /* disable 'logon as a service' privileges to domain\user only if granted by us */
    if (Win32_ChangeAccountRights(user, domain, SE_SERVICE_LOGON_NAME, &existed, FALSE) != ERROR_SUCCESS)
    {
        DbgPlain(DBG_MAIN_ACTION, 
            _T("[%s] Error deleting accnt rights...probably not in builtin\administrators group."), 
            __FCN__
        );
    }
exit:
    RETURN_INT (retval);
}

#endif



/*========================================================================*//**
*
* @brief     Reads and parses options from specified file.
*
*//*=========================================================================*/
BP_Result BU_GetLocalFile(IN const tchar *filename, OUT BP_Opt *opts)
{
    *opts = CmnVarGetFile(filename);
    return 
        CmnVarDefined(opts)? BP_NOERROR   :
        ErhErrno() != 0    ? BPERR_FILEIO :
        BP_NOERROR;
}


/*========================================================================*//**
*
* @brief     Packs and writes options to a specified file.
*
*//*=========================================================================*/
BP_Result BU_PutLocalFile(const tchar *filename, const BP_Opt opts)
{
    int  result = 0;

    tchar *packed = BP_Pack (opts, VAR_PACK_BOM);
    if (!packed)
        return BPERR_INVPARAM;

    if (CmnPutAsciiFile(filename,packed) != 0)
        result = BPERR_FILEIO;

    FREE(packed);
    return result;
}


/*========================================================================*//**
*
* @brief     Construct path to config file relative to cmnPanConfig/integ as:
*            config/<apptype>/<hostname>%%<appname>
*            o if <hostname> is empty use cmnHostname
*            o if <appname> is empty, construct path as: config/<apptype>/<hostname>
*            does not expand <hostname> in this case
*
*            Converts forbidden characters in appname using BU_EncodeFilename
*
*            TODO
*            fix code that used old convention - empty host meant <appname> path.
*            (oracle8_common.c)
*
*//*=========================================================================*/
static tchar *BU_GetConfigPath (
    OUT tchar        path[STRLEN_PATH+1],
    IN  const tchar *apptype,
    IN  const tchar *hostname,
    IN  const tchar *appname )
{
    ERH_FUNCTION(_T("BU_GetConfigPath"));
    DbgFcnIn();

    if (StrIsEmpty(apptype) || (StrIsEmpty(hostname) && StrIsEmpty(appname)))
        RETURN_STR (NULL);

    if (StrIsEmpty(hostname))
    {    
        hostname = cmnHostname;
    }
    else if (!StrIsEmpty(appname))
    {
        hostname = OB2_ExpandHostname(hostname);
    }

    if (StrIsEmpty(appname))
    {
        CreatePathFromParams(path, _T("config/%s/%s"), apptype, StrToUrl(hostname));
    }
    else
    {
        CreatePathFromParams(path, _T("config/%s/%s%%%s"), apptype, hostname, StrToUrl(appname));
    }

    RETURN_STR (path);
}




/* ===========================================================================
| FUNCTION     BU_GetConfig
|    Reads and unpacks config file from CS.
 ========================================================================== */
BP_Result BU_GetConfig(
    const tchar  *cellname,
    const tchar  *hostname,
    const tchar  *apptype,
    const tchar  *appname,
    OUT   BP_Opt *config )
{
    ERH_FUNCTION(_T("BU_GetConfig"));
    csa_t   csa = {0};
    int     result = 0;
    tchar  *buffer = NULL;
    tchar   name[STRLEN_PATH+1];

    DbgFcnInEx (__FLND__, _T("apptype:%s hostname:%s appname:%s"), NS(apptype), NS(hostname), NS(appname));
    if (!BU_GetConfigPath (name, apptype, hostname, appname))
        RETURN_INT (BPERR_INVPARAM);

    memset(config, 0, sizeof(BP_Opt));

    if (BU_CsaInit(&csa, cellname) != 0)
        RETURN_INT (BPERR_ERRNO);

    result = BU_CsaRead(&csa, name, &buffer);
    BU_CsaExit (&csa);

    if (result != BP_NOERROR)
        RETURN_INT (result);

    result = BP_ParseBuffer(buffer, name, config);
    FREE(buffer);

    if (result != BP_NOERROR)
        CmnVarFree (config);

    RETURN_INT(result);
}

BP_Result BU_GetConfigRaw(
    const tchar  *cellname,
    const tchar  *hostname,
    const tchar  *apptype,
    const tchar  *appname,
    OUT MALLOCED  tchar **config)
{
    ERH_FUNCTION(_T("BU_GetConfigRaw"));
    
    csa_t   csa = {0};
    int     result = 0;

    tchar   name[STRLEN_PATH+1];

    DbgFcnInEx (__FLND__, _T("apptype:%s hostname:%s appname:%s"), NS(apptype), NS(hostname), NS(appname));

    if (*config == NULL)
    {
        *config = MALLOC(sizeof(tchar));
    }
    
    if (!BU_GetConfigPath (name, apptype, hostname, appname))
    {
        RETURN_INT (BPERR_INVPARAM);
    }

    if (BU_CsaInit(&csa, cellname) != 0)
    {
        RETURN_INT (BPERR_ERRNO);
    }

    result = BU_CsaRead(&csa, name, config);

    BU_CsaExit (&csa);
 
    RETURN_INT(result);
}


BP_Result BU_GetLicType(
    const tchar  *cellname,
    int *licType,
    int *licquantity)
{
    ERH_FUNCTION(_T("BU_GetLicType"));
    
    csa_t   csa = {0};
    int     result = 0;

    DbgFcnInEx (__FLND__, _T("cellname:%s "), NS(cellname));

    if (BU_CsaInit(&csa, cellname) != 0)
    {
        RETURN_INT (BPERR_ERRNO);
    }

    result = MCsaGetLicType(csa.handle, licType, licquantity);
    BU_CsaExit (&csa);

    RETURN_INT(result);
}


/*========================================================================*//**
*
* @brief     Returns list of files from integ/config/<apptype> directory
*
*//*=========================================================================*/
BP_Result BU_EnumConfig(
    const tchar   *cellname,
    const tchar   *apptype,
    OUT   Variant *configNames)
{
    ERH_FUNCTION(_T("BU_EnumConfig"));
    csa_t   csa = {0};
    int     result = 0;
    int     msgno = -1;
    tchar  *buffer = NULL;
    tchar  *name = NULL; 
    tchar  *mtime = NULL;
    tchar   dir[STRLEN_PATH+1];
    StrMsgStruct msg = {0};

    DbgFcnInEx (__FLND__, _T("apptype:%s"), NS(apptype));

    if (NULL != apptype)
    {
        CreatePathFromParams(dir, _T("config/%s"), apptype);
    }
    else
    {
        CreatePathFromParams(dir, _T("config"));
    }

    if (BU_CsaInit(&csa, cellname) != 0)
    {
        RETURN_INT (BPERR_ERRNO);
    }
    
    result = MCsaGetGroupListEx (csa.handle, G_TOP_INTEG_CNF, dir, &buffer);
    BU_CsaExit (&csa);

    if (result != 0)
    {
        RETURN_INT (BPERR_ERRNO);
    }

    msgno = StrParseStartL (&msg, buffer);
    if (MSG_GR_LIST != msgno)
    {
        ErhMark (ERH_MFORMAT, __FCN__, ERH_MAJOR);
        RETURN_INT(BPERR_ERRNO);
    }

    while (0 == StrParseNNextL(&msg, 2, &name, &mtime))
    {
        CmnVarPush (configNames, T2V(name));
    }
    FREE(buffer);

    RETURN_INT(result);
}


/* ===========================================================================
| FUNCTION     BU_PutConfig
 ========================================================================== */
BP_Result BU_PutConfig (
    const tchar *cellname,
    const tchar *hostname,
    const tchar *apptype,
    const tchar *appname,
    IN    BP_Opt config )
{
    ERH_FUNCTION(_T("BU_PutConfigRaw"));

    csa_t   csa    = {0};
    int     result = 0;
    tchar  *buffer = NULL;
    tchar   name[STRLEN_PATH+1];

    DbgFcnInEx (__FLND__, _T("cellname:%s apptype:%s hostname:%s appname:%s"), NS(cellname), NS(apptype), NS(hostname), NS(appname));

    if (!BU_GetConfigPath (name, apptype, hostname, appname))
        RETURN_INT (BPERR_INVPARAM);

    if (BU_CsaInit(&csa, cellname) != 0)
        RETURN_INT (BPERR_ERRNO);

    buffer = BP_Pack(config, VAR_PACK_BOM);
    result = BU_CsaWrite(&csa, name, buffer);
    FREE (buffer);
    BU_CsaExit(&csa);

    RETURN_INT (result);
}

BP_Result BU_PutConfigRaw (
    const tchar *cellname,
    const tchar *hostname,
    const tchar *apptype,
    const tchar *appname,
    IN    tchar  *config )

{
    ERH_FUNCTION(_T("BU_PutConfig"));
    csa_t   csa    = {0};
    int     result = 0;
    tchar   name[STRLEN_PATH+1];

    DbgFcnInEx (__FLND__, _T("cellname:%s apptype:%s hostname:%s appname:%s"), NS(cellname), NS(apptype), NS(hostname), NS(appname));

    if (StrIsEmpty(config) || !BU_GetConfigPath (name, apptype, hostname, appname))
        RETURN_INT (BPERR_INVPARAM);

    if (BU_CsaInit(&csa, cellname) != 0)
        RETURN_INT (BPERR_ERRNO);

    result = BU_CsaWrite(&csa, name, config);

    BU_CsaExit(&csa);

    RETURN_INT (result);
}



BP_Result BU_UpdateConfig(BP_Opt *config, const tchar *OptName, const tchar *OptVal)
{
    ERH_FUNCTION(_T("UpdateConfig"));

    DbgStamp(DBG_MAIN_PROGTRACE);
    DbgPlain(DBG_MAIN_PROGTRACE,
        _T("[%s] Updating configuration:")
        _T("Option name  - %s")
        _T("Option Value - %s"),
        __FCN__,
        OptName,
        OptVal
    );

    if (OptVal == NULL)
    {
        return BP_RemoveOptByName(config, OptName);
    }
    else if (NULL != BP_GetOptByName(*config, OptName))
    {
        return BP_ChangeStr(config, OptName, OptVal );
    }
    else
    {
        return BP_AddStr(config, OptName, OptVal);
    }
}

BP_Result
BU_GetBarlist(const tchar         *cellServer,
              const tchar         *appType,
              const tchar         *barList,
              OUT MALLOCED tchar **contents)
{
    tchar path[STRLEN_PATH+1];
    int status;
    int b;

    status = BU_ComposeBarlistPath(path, appType, barList);
    if (status != 0)
        return status;

    b = MCsaBindServer(cellServer);
    if (b == -1)
    {
        return BPERR_ERRNO;
    }

    status = MCsaGetCustomFile(b, CMN_PAN_CONFIG, path, contents, READ_ONLY);
    return status==0? 0 : BPERR_ERRNO;
}

BP_Result
BU_PutBarlist(const tchar *cellServer,
              const tchar *appType,
              const tchar *barList,
              const tchar *contents)
{
    tchar path[STRLEN_PATH+1];
    int status;
    int b;

    status = BU_ComposeBarlistPath(path, appType, barList);
    if (status != 0)
        return status;

    b = MCsaBindServer(cellServer);
    if (b == -1)
    {
        return BPERR_ERRNO;
    }

    status = MCsaPutCustomFile(b, CMN_PAN_CONFIG, path, contents, UNLOCK);
    return status==0? 0 : BPERR_ERRNO;
}


/*=========================================================================*//**
*  FUNCTION   BU_GetBarlistPath
*  @param     tchar  *path     (OUT)  barlistPath
*  @param     tchar  *specname (IN)   specName
*
*  DESCRIPTION
*       Construct path to barlist file relative to cmnPanConfig/integ as:
*
*//*========================================================================== */
int
BU_ComposeBarlistPath(OUT tchar        path[STRLEN_PATH+1],
                      IN  const tchar *appType,
                      IN  const tchar *barList)
{
    ERH_FUNCTION(_T("BU_GetBarlistPath"));

    const OB2IntegType *integ;
    DbgFcnIn();

    integ = CmnGetIntegType(appType);
    if (!integ)
        RETURN_INT(BPERR_INVPARAM);

    CreatePathFromParams(path, _T("%s/%s"), integ->dir.datalistDir, barList);
    DbgPlain (DBG_MAIN_ACTION, _T("BarlistPath: %s"), path);

    RETURN_INT(BP_NOERROR);
}

/* ===========================================================================
| FUNCTION     BU_GetCellInfoFile
 ========================================================================== */
BP_Result BU_GetCellInfoFile(const tchar *cellserver, tchar **buffer)
{
    ERH_FUNCTION(_T("BU_GetCellInfoFile"));
    csa_t   csa = {0};
    int     result = 0;

    DbgFcnInEx (__FLND__, _T("cellserver:%s"), NS(cellserver));

    if (BU_CsaInit(&csa, cellserver) != 0)
        RETURN_INT (BPERR_ERRNO);

    result = MCsaGetSingleFile(csa.handle, S_CELL, buffer, READ_ONLY);
    BU_CsaExit (&csa);

    RETURN_INT(result==0? BP_NOERROR : BPERR_ERRNO);
}

/* ===========================================================================
| FUNCTION     BU_GetCellInfoHostList
 ========================================================================== */
BP_Result BU_GetCellInfoHostList(const tchar *cellserver, THostList **hostlist)
{
    ERH_FUNCTION(_T("BU_GetCellInfoHostList"));
    tchar   *buffer = NULL;
    int     result = 0;

    DbgFcnInEx (__FLND__, _T("cellserver:%s"), NS(cellserver));

    if (BU_GetCellInfoFile(cellserver, &buffer) != BP_NOERROR)
    {
        RETURN_INT(ERH_OBE_CELLINFO);
    }

    /* start parsing the file */
    *hostlist = CellPparseHosts(buffer);
    FREE(buffer);

    RETURN_INT(result);
}

/* ===========================================================================
| FUNCTION     BU_GetHostFromCellInfo
 ========================================================================== */
BP_Result BU_GetHostFromCellInfo(const tchar *cellserver, const tchar *hostname, THostList **host)
{
    ERH_FUNCTION(_T("BU_GetHostFromCellInfo"));
    THostList *hostlist = NULL;
    int     result = 0;

    DbgFcnInEx (__FLND__, _T("cellserver:%s, hostname:%s"), NS(cellserver), NS(hostname));

    result = BU_GetCellInfoHostList(cellserver, &hostlist);

    /* find host info from list */
    if (hostlist)
    {
        *host = CellPfindHost(hostlist, hostname);

        if (!(*host))
        {
            result = ERH_OBE_CELLINFO_HOST_NOT_FOUND;
        }
    }
/*    FREE(hostlist); */

    RETURN_INT(result);
}


/* ===========================================================================
|   FUNCTION    OB2_SetHostname
|       Override cmnHostName with given <hostname>
 ========================================================================== */
void OB2_SetHostname(const tchar *hostname)
{
    if (StrIsEmpty(hostname))
        return;

    hostname = OB2_ExpandHostname(hostname);
    DbgPlain(DBG_MAIN_ACTION, _T("[OB2_SetHostname] cmnHostname=%s"), hostname);
    StrCopy(cmnHostname, hostname, STRLEN_HOSTNAME);
}


tchar *OB2_CellServer (void)
{
    ERH_FUNCTION (_T("OB2_CellServer"));

    static tchar cellserver[STRLEN_HOSTNAME+1]={0};
    tchar *host;

    if (!StrIsEmpty(cellserver))
        return cellserver;
    
    DbgFcnIn();
    
    host = GetEnv(_T("OB2_CELL_SERVER_HOSTNAME"));
    if (!StrIsEmpty(host))
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] *Caution*: override cell server to %s"), __FCN__, host);
        StrCopy (cellserver, OB2_ExpandHostname(host), STRLEN_HOSTNAME);
    }
    
    else if (0==CliReadCSHost (&host))
    {
        StrCopy (cellserver, OB2_ExpandHostname(host), STRLEN_HOSTNAME);
        FREE (host);
    }

    RETURN_STR(cellserver);
}


NONREENTRANT ctchar *OB2_ExpandHostname (ctchar *shortName)
{
    ctchar *longName;
    if (StrIsEmpty(shortName))
        return NULL;

    longName = CmnExpandHostname(shortName);
    return longName? longName : shortName;
}

/* ===========================================================================
|   FUNCTION    OB2_GetTempFileName
|       Get unique temp file name
 ========================================================================== */
MALLOCED tchar*
OB2_GetTempFileName(const tchar *dir,
    const tchar *prefix,
    const tchar *postfix)
{
    tchar *path = NULL;
    static int count = 0;

    if (StrIsEmpty(dir))
        dir = cmnPanTmp;

    path = StrFormat(_T("%s/%s%d%d%d%d.%s"),
        PathClean(dir),
        prefix ? prefix : _T("ob2"),
        (int)time(NULL),
        GetPid(),
        GetTid(),
        count,
        postfix ? postfix : _T("tmp")
    );

    count = count + 1;
    return (path);
}


/*========================================================================*//**
*
* @brief     Map BP_ and BU_ api error codes to appropriate ErhErrno
*
*//*=========================================================================*/
DLLEXPORT int BU_ErhErrno(IN BP_Result bpretval)
{
    switch (bpretval)
    {
    case BPERR_INVTOKTYPE:
    case BPERR_INVTOK:
    case BPERR_UNEXPEOF:
    case BP_EOF:
        return ERH_FFORMAT;
        break;

    case BPERR_FILEIO:
        return ERH_SYSERR;
        break;

    case BPERR_OUTOFMEM:
        return ERH_NOMEM;
    
    case BPERR_INVPARAM:
    case BPERR_INVVARTYPE:
        return ERH_INTERNAL;

    case BPERR_ERRNO:
        return ErhErrno();

    default:
        return 0;
    }
}


/*========================================================================*//**
*
* @brief     Windows: Same as appropriate printf-like functions but with controlled
*            conversion UCS-2 --> output (printf always does W2A implicitly)
*
*//*=========================================================================*/
#if TARGET_WIN32 && defined(UNICODE)
static int
BU_Vfprintf (FILE *file, const tchar *fmt, va_list va)
{
    Variant *v = CmnGetTmpVar(NULL);
    Variant *s = CmnGetTmpVar(NULL);
    int len;
    CmnVarVFormat(v, fmt, va);
    len = CmnVarStrLen(v);
    
    CmnVarCopyW2S(s, V2T(v));
    #undef fputs
    return fputs ((void*)V2T(s), file) == EOF? -1 : len;
}


int
BU_Fprintf(FILE *file, const tchar *fmt, ...)
{
    int status;
    va_list va;
    va_start (va, fmt);
    status = BU_Vfprintf (file, fmt, va);
    va_end (va);
    return status;
}


int
BU_Printf(const tchar *fmt, ...)
{
    int status;
    va_list va;
    va_start (va, fmt);
    status = BU_Vfprintf (stdout, fmt, va);
    va_end(va);
    return status;
}

#endif /* TARGET_WIN32 */


/*========================================================================*//**
*
* @brief     Reads <name> environment variable as string, if it does not exists
*            it returns copy of default value.
*            Returns allocated string or NULL if default value is NULL.
*
*//*=========================================================================*/
MALLOCED tchar *BU_EnvGetStrValue (IN const tchar name[], IN tchar default_val[])
{
    tchar *env = EnvGetString(name);
    if (env)
        return env;

    DbgPlain(DBG_DETAIL_PROGTRACE, _T("Using default value \'%s\'"), NSD(default_val));
    return StrDup(default_val);
}



/*========================================================================*//**
*
* @brief     Reads <name> environment variable as int, if it does not exists
*            or value is not an integer the default value is returned.
*
*//*=========================================================================*/
int BU_EnvGetIntValue (IN const tchar name[], IN OUT int *value)
{
    int    retval = 0;
    int    default_value = *value;
    tchar *env = BU_GetEnv(name);
    if ( env )
    {
        int valid;
        StrAtoBool(env, value, &valid);
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("Environment variable %s found"), env);
        if ( valid )
        {
            DbgPlain(DBG_DETAIL_PROGTRACE, _T("Valid value = %d"), *value);
            retval = 1;
        }
        else
        {
            *value = default_value;
            DbgPlain(DBG_DETAIL_PROGTRACE, _T("Invalid value, keeping default value = %d"), *value);
        }
    }
    else
    {
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("Environment variable %s not found"), name);
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("Keeping default value = %d"), *value);
    }

    FREE(env);
    return (retval);
}


BP_Result BU_ExportToEnv (IN const BP_Opt *opt)
{
    Iterator it = IterNew(opt);

    if (TypeOf(opt) != var_list)
        return BPERR_INVPARAM;
        
    while (IterNext(&it))
    {
        tchar   *key  = IterKey(&it);
        Variant *val  = IterVal(&it);
        int      type = TypeOf(val);
        if (var_string == type)
            BU_PutEnv (key, V2T(val));
        else if (var_int == type)
            BU_PutEnv (key, StrFromInt(V2I(val)));
    }
    return BP_NOERROR;
}




#if BARUTIL_UNUSED_CODE
/* ---------------------------------------------------------------------------
|   This function is not used currently. It would read a list of integrations that
|   are installed in a cell.
 -------------------------------------------------------------------------- */
BP_Result BU_GetIntegNames(const tchar *cellname, OUT BP_Opt *namelist)
{
    BP_Result result = 0;
    csa_t     csa    = {0};
    tchar    *buffer = NULL;

    memset(namelist, 0, sizeof(BP_Opt));

    if (0 != BU_CsaInit(&csa, cellname))
        return BPERR_ERRNO;

    result = BP_CsaRead(csa, BUC_INTEG_NAMES, &buffer);

    BU_CsaExit(&csa);

    if (result != BP_NOERROR)
        return result;

    result = BP_ParseBuffer(buffer, BUC_INTEG_NAMES, namelist);
    FREE(buffer);

    return result;
}

#endif


int StrLookupText (IN const PairTextInt *table, IN const tchar *val)
{
    if (!table || StrIsEmpty(val))
        return -1;
    
    for (; table->val; table++)
        if (0==StrICmp(table->val, val))
            return table->id;
    
    return -1;
}


/*========================================================================*//**
*
* @brief    PathCleanR does not clean UNC names on windows, so be careful when
*            part1 is omitted and part2 has leading slash.
*
*//*=========================================================================*/
tchar*
PathCatR(OUT tchar *out, IN const tchar *part1, IN const tchar *part2)
{
    if (StrIsEmpty(part1))
    {
        return PathCleanR(out, part2);
    }
    else
    {
        tchar buf[STRLEN_PATH+1];
        CreatePathFromParams(buf, _T("%s%c%s"), NS(part1), GEN_SLASH, NS(part2));
        return PathCleanR(out, buf);
    }
}


/*========================================================================*//**
*
* @param    path - path to traverse
* @param    ctx  - context for temporary data
*
* @brief    Path /a/b/c is traversed as: /a, /a/b, /a/b/c .
* @todo     works only for absolute paths, i.e. paths starting with '/'
*//*=========================================================================*/
tchar*
PathWalkFirstEx(pathwalk_t *ctx, const tchar *path, unsigned flags)
{
    if (!ctx)
        return NULL;

    memset (ctx, 0, sizeof(*ctx));
    if (!path)
        return NULL;

    ctx->pos   = ctx->path;
    ctx->flags = flags;
    ctx->first = 1;

    StrCopy(ctx->path, path, STRLEN_PATH);
    return PathWalkNext(ctx);
}


tchar *PathWalkNext(pathwalk_t *ctx)
{    
    if (ctx->done)
        return NULL;

    ctx->ncomp++;

    if (!ctx->pos)
    {
        ctx->done = TRUE;
        return ctx->path;
    }

    ctx->value = ctx->path;

    /* first component */
    if ((ctx->flags & 0x1) && ctx->first && ctx->path[0]==_T('/'))
    {
        ctx->first = 0;
        return ctx->value = _T("/");
    }

    ctx->pos[0] = _T('/');
    ctx->pos    = strchr(ctx->pos+1, _T('/'));
    if (!ctx->pos)
        return ctx->path;

    ctx->pos[0] = _T('\0');
    if (!ctx->pos[1])
        ctx->pos = NULL;

    return ctx->path;
}


/* ===========================================================================
|   FUNCTION     BU_Encode/DecodePath
|       Encode string so it can be used in GUI object paths
 ========================================================================== */
tchar *BU_EncodePathEx (IN const tchar *t, ULONG flags)
{
    return StrEscape(t, flags);
}


tchar *BU_DecodePath (IN const tchar *t)
{
    return StrUnescape(t, NULL);
}


BOOL BU_StrAtoi(const tchar *str, int *result)
{
    BOOL   valid  = TRUE;
    size_t strLen = StrLen(str);

    if (strLen < 10)
    {
        valid = sscanf(str, _T("%d"), result);
    }
    else if (strLen > 10)
    {
        valid = FALSE;
    }
    else
    {
        double d = atof(str);
        valid = d > INT_MAX? FALSE : sscanf(str, _T("%d"), result);
    }

    return valid;
}




const tchar*
OB2_StatusToText(OB2_STATUS status)
{
    #define CASE(e) case OBE_##e: return _T("OBE_") _T(#e);
    switch (status)
    {
    CASE(OK);
    CASE(ERR);
    CASE(OB2ERR);
    CASE(NOTSUPP);
    CASE(INVARG);
    CASE(INVMODE);
    CASE(NODATA);
    CASE(SYSERR);
    CASE(NOPERM);
    CASE(BARLIST);
    CASE(DISABLE);
    CASE(RESOLVE);
    CASE(PRE_EXEC);
    CASE(ENDOBJ);
    CASE(SPLIT);
    CASE(PREPARE_R2);
    CASE(MOREDATA);
    CASE(OB2ABORT);
    CASE(QUERY);
    CASE(TMOUT);
    CASE(PROTO);
    CASE(OB2WRONGCTX);
    CASE(OB2CTXUSED);
    CASE(RETRY);
    CASE(SM_NEGOTIATION);
    default: return _T("OBE_UNKNOWN");
    }
}


void
ErhMarkDlerror(const tchar *fcn, const tchar *info)
{
    int    error     = GetLastError();
    tchar *errortext = dlerror();
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] info:%s error:[%d] %s"), fcn, NS(info), error, NS(errortext));
    ErhMarkSys (ERH_UNSUPP, error, fcn, ERH_MAJOR);
    ErhSetSysErrorStr(info);
}


/*========================================================================*//**
*
* @param     cmdline    script and argument to execute. must reside in cmnPanBin
* @param     isPreexec  true/false
* @param     user       to start script (can be null)
* @param     password   user's password
* @param     timeout    script execution timeout (0 means infinitive)
*
* @brief     Function adds cmnPanBin prefix to the passed command line string.
*
*            If preexec script fails then Major error is reported.
*            If postexec script fails then Minor error is reported.
*            Otherwise Normal message is reported.
*
*//*=========================================================================*/
int
OB2_ExecuteScriptM(const tchar *cmdline,
                  BOOL         isPreexec,
                  const tchar *user,
                  const tchar *password,
                  int          timeout)
{
    ERH_FUNCTION(_T("OB2_ExecuteScript"));

    int     nlsID = isPreexec? NLS_BAR_START_PREEXEC : NLS_BAR_START_POSTEXEC;
    int     result;
    CmnRunStruct run = {0};
    argvRec args = {0};
    ctchar *nlsMsg;

    DbgFcnIn();

    DbgPlain(DBG_MAIN_PROGTRACE,
             _T("[%s] Input parameters:\n")
             _T("cmdline   : %s\n")
             _T("isPreexec : %s\n")
             _T("user      : %s\n")
             _T("timeout   : %d"),
             __FCN__,
            NSD(cmdline), StrFromBOOL(isPreexec), NSD(user), timeout);

    /* Run and forget can not be used for integration pre-post exec */
    if (timeout < 0) timeout = 0;

    if (StrIsEmpty(cmdline))
    {
        RETURN_INT(0);
    }

    if (GetEnvInt(_T("OB2OEXECOFF")) == 1)
    {
        DbgLogFull (__FLNPT__, S_INETLOG_FILE, _T("OB2OEXECOFF=1, script \"%s\" specified but not executed."),
            cmdline
        );
        ErhFullReport(__FCN__, ERH_WARNING, NlsGetMessage(NLS_SET_DA, NLS_DA_OEXEC_DISABLED));
        RETURN(0);
    }

    ArgvReadFromBuffer(&args, cmdline);
    if (!StrScriptIsSecure(args.argv[0]))
    {
        ErhFullReport(__FCN__, ERH_MAJOR, NlsGetMessage(NLS_SET_DA, NLS_DA_OEXEC_RESTRICTED, cmdline));
        DbgLogFull ( __FLNPT__, S_INETLOG_FILE, _T("Security error ! Execution of %s denied."), cmdline);
        result = RUN_SCRIPT_ERROR;
    }
    else
    {
        ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage(NLS_SET_LIBBAR, nlsID, cmdline));
        run.user = user; run.password = password; run.dir = cmnPanLBin; run.timeout = timeout;
        result = CmnRunScript(args.argc, args.argv, &run);
    }

    ArgvFree(&args);

    switch (result)
    {
    case RUN_SCRIPT_ERROR:
        nlsMsg = NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_SCRIPT_ERROR, run.exitcode);
        break;
    case RUN_SCRIPT_TIMEOUT:
        nlsMsg = NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_SCRIPT_TIMEOUT);
        break;
    case RUN_SCRIPT_INVALID_ARG:
        nlsMsg = NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_SCRIPT_INVARG);
        break;
    case RUN_SCRIPT_OK:
        if (run.exitcode == 0)
        {
            ErhFullReport(__FCN__, ERH_NORMAL, NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_SCRIPT_EXECUTED));
            RETURN(0);
         }

    default:
        nlsMsg = NlsGetMessage(NLS_SET_LIBBAR, NLS_BAR_SCRIPT_START_ERROR, run.exitcode);
    }
    
    ErhFullReport(__FCN__, isPreexec? ERH_MAJOR : ERH_MINOR, nlsMsg);
    
    RETURN_INT(result == RUN_SCRIPT_OK? 0 : -1);
}
