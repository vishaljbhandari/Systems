/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   LibXtra (WIN32)
* @file      lib/xtra/win32_suid.c
*
* @par Last Change:
* $Rev: 1981$
* $Date$
*
* @brief     User impersonation module for Windows 2008
*
* @since     16.10.2007  Nikolai        Initial coding
*/

#include "lib/cmn/target.h"

__rcsId("$Header: /lib/xtra/win32_suid.c $Rev$Date:: 2009-09-11 19:11:14 #$");

#include <Userenv.h>
#include <Ntsecapi.h>

#include "lib/cmn/common.h"
#include "lib/cmn/type_variant.h"
#include "lib/xtra/crypt_kms.h"
#include "win32_registryNT.h"
#include "win32_suid.h"


BOOL 
CheckUserInInetConfig(const tchar *user, const tchar *domain, tchar **password)
{
    ERH_FUNCTION(_T("CheckUserInInetConfig"));

    tchar  *buf = NULL;
    tchar  *entityName = NULL;
    ctchar *account = user;                     /* user[@domain] */
    tchar   pass[STRLEN_OB2USER+1] = {0};       /* decoded password */
    tchar  *epass;
    HANDLE  keyhandle;
    BOOL    success;
    
    DbgFcnIn();

    if (StrIsValid(domain))
    {
        account = buf = StrFormat(_T("%s@%s"), user, domain);
    }

    /* Open Key for Inet configuration */
    keyhandle = NTRegistryOpenKey(KEY_INET, TRUE, KEY_INET_SECURITY);
    if (!keyhandle) 
    {
        RETURN_BOOL(FALSE);
    }

    /* get the password from registry */
    epass = NTRegistryReadStringValue(KEY_INET, account);

    if (!epass)
        RETURN_BOOL(FALSE);

    // Decrypt password using KMS
    entityName = StrFormat(_T("%s:%s"), buf, cmnHostname);
    StrToUpper(entityName);
 
    success = EncryptDecryptText(entityName, epass, password, DECRYPT);

    HandleDbSession(DISCONNECT);
    FREE(epass);
    FREE(buf);
    FREE(entityName);
    RETURN_BOOL(success);
}


/*========================================================================*//**
*
* @ingroup   LibXtra
*
* @param     tchar   *user       (IN)    user name
* @param     tchar   *domain     (IN)    domain name
* @param     BOOL    *existed    (IN/OUT) set by add priv., and used by remove priv.
* @param     BOOL    addRights   (IN)    TRUE will add privileges, FALSE will remove priv.
*
* @brief     Add or remove 'logon as a service' privilege to specified account
*
*//*=========================================================================*/
#define DbgError(_func, _error) { \
    int error = _error; \
    DbgPlain (DBG_MAIN_ACTION, _T("[%s] OS call %s failed with %s"), \
        __FCN__, _T(#_func), ErhSysErrnoToText(error)); \
    retval = error; \
}

#define DbgOSError(_func) DbgError(_func, GetLastError())
#define DbgLsaError(_func,_status) { SetLastError(LsaNtStatusToWinError(_status)); DbgError(_func, GetLastError()); }

#pragma comment (lib, "Userenv")

static int Win32_ChangeAccountRights (
    IN     const tchar *user, 
    IN     const tchar *domain,
    IN     tchar       *privName, 
    IN OUT BOOL        *existed, 
    IN     BOOL        addRights)
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

    priv.Buffer        = privName;
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
* @brief     Impersonates specified user
*
*//*=========================================================================*/
int OB2_ImpersonateUser(
    IN  const tchar *user,
    IN  const tchar *domain,
    IN  const tchar *pass,
    OUT HANDLE      *token)
{
    ERH_FUNCTION(_T("ImpersonateUser"));
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
        user,
        domain,
        pass,
        LOGON32_LOGON_SERVICE,
        LOGON32_PROVIDER_DEFAULT, 
        token);

    if (!ok)
    {
        DbgOSError (LogonUser);
        goto exit;
    }

    profile.dwSize     = sizeof(PROFILEINFO);
    profile.lpUserName = user;
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
                __FCN__);
    }
exit:
    RETURN_INT (retval);
}