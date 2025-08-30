/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Common Library
* @file      lib/cmn/gen_env.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Generic functions for access to environment
*
* @since     22.12.1997  Lars        Initial coding
*/

#include "lib/cmn/target.h"

__rcsId("$Header: /lib/cmn/gen_env.c $Rev$ $Date::                      $:");

#include "common.h"
#include "gen_env.h"


#define DBG_ENV  18


/* ---------------------------------------------------------------------------
|
|   Environment handling helpers
|
 -------------------------------------------------------------------------- */

int
EnvGetBool(_In_ ctchar *name)
{
    int value = 0;

    EnvReadInt(name, &value);

    /* Caller just needs to know if it exists */
    /* Returned value is 1 if it does (empty, TRUE, YES, ON) */
    /* Returned value is any int if it contains a numeric value */
    /* If it doesn't exist (or contains "garbage"), return value is 0 */

    return value;
}


int
EnvReadInt(_In_ ctchar *name, _Inout_ int *value)
{
    ERH_FUNCTION(_T("EnvReadInt"));
    int r;

    ctchar *strval = GetEnv(name);

    if (strval)
    {
        StrAtoBool(strval, value, &r);
        if (r)
        {
            DbgPlain (DBG_ENV, _T("[%s] %s=%d"), __FCN__, name, *value);
            return 1;
        }
    }

    #if TARGET_WIN32
    /* Try registry */
    r = RegReadDWORD(HKEY_LOCAL_MACHINE, REGPARAMETERS, name, value);
    if (r == 0)
    {
        DbgPlain (DBG_ENV, _T("[%s] %s=%d (registry)"), __FCN__, name, *value);
        return 1;
    }
    #endif

    DbgPlain (DBG_ENV, _T("[%s] %s=n/a (default:%d)"), __FCN__, name, *value);
    return 0;
}

static MALLOCED tchar*
OSGetEnvCopy(IN const tchar *valueName)
{
    #if !TARGET_WIN32
        return StrNullCopy(OSGetEnv(valueName));

    #else
        DWORD  buflen = 0;
        DWORD  len;
        tchar *envString = NULL;
        while ((len = GetEnvironmentVariable(valueName, envString, buflen)) > buflen)
        {
            envString = REALLOC(envString, (buflen = len + 16)*sizeof(tchar));
        }

        if (len)
            return envString;

        FREE(envString);
        return RegGetString(HKEY_LOCAL_MACHINE, REGPARAMETERS, valueName);
    #endif
}


MALLOCED tchar *
EnvGetString(_In_ ctchar *name)
{
    ERH_FUNCTION(_T("EnvGetString"));

    tchar *env;

    if (CmnDefined(CONTEXT_ENV))
    {
        env = StrNullCopy(CmnEnvGet(NULL, name));
    }
    else
    {
        env = OSGetEnvCopy(name);
    }

    if (env)
    {
        DbgPlain (DBG_ENV, _T("[%s] ENV found %s = %s"), __FCN__, name, env);
        return env;
    }

    DbgPlain (DBG_ENV, _T("[%s] ENV not found. %s = NULL"), __FCN__, name);
    return NULL;
}


/*========================================================================*//**
*
* @section  per-context env variables
* @note     env variables names are case-insensitive on windows
*
*//*=========================================================================*/
INLINED CmnEnv*
CmnEnvInit(_Inout_opt_ CmnEnv *env)
{
    if (!env)
    {
        USES_CMN_PTD
        env = &ThisCmn->env;
    }

    if (!env->size)
        *env = hash_filenames();
    
    return env;
}

#define ENV_USED ((void*)(size_t)1)

void
CmnEnvSet(_Inout_opt_ CmnEnv *env, _In_ ctchar *key, _In_opt_ ctchar *value)
{
    hash_iter_t it = hash_find(CmnEnvInit(env), key);

    if (!it.found)
    {
        hash_iter_insert(it, StrNewCopy(key), StrNullCopy(value));
        return;
    }

    /* found existing value */
    {
        hnode_t *node     = *it.pnode;
        tchar   *prev     = node->val;
        size_t   refcount = (size_t)node->userptr;

        /* unchanged value: NOP */
        if (0 == StrCmp(prev, value))
            return;

        /* nobody has reference to old value: free & reuse */
        if (refcount == 0)
        {
            FREE(node->val);
            node->val = StrNewCopy(value);
            return;
        }

        /* otherwise COW */
        hash_iter_insert(it, node->key, StrNewCopy(value));
        node->key = NULL;
    }
}


static void
CmnEnvUnset(_Inout_opt_ CmnEnv *env, _In_ ctchar *key)
{
    hash_iter_t it = hash_find(CmnEnvInit(env), key);
    
    if (!it.found)
    {
        return;
    }

    /* found existing value */
    {
        hnode_t *node     = *it.pnode;
        size_t   refcount = (size_t)node->userptr;

        /* nobody has reference to old value: free */
        if (refcount == 0)
        {
            hash_iter_delete(it);
            return;
        }

        /* otherwise hide old value */
        node->key = NULL;
    }
}


void
CmnEnvFree(_Inout_ CmnEnv *env)
{
    hash_free_with(env, free, free);
}


void
CmnEnvPut(_Inout_opt_ CmnEnv *env, _In_ ctchar *envstr)
{
    tchar *key;
    const tchar *eq = StrChr(envstr, '=');
    if (!eq)
        return;

    key = StrNNewCopy(envstr, eq - envstr);
    CmnEnvSet(env, key, eq+1);
    FREE(key);
}


tchar*
CmnEnvGet(_Inout_opt_ CmnEnv *env, _In_ ctchar *key)
{
    hash_iter_t it = hash_find(CmnEnvInit(env), key);

    if (it.found)
    {
        hnode_t *node = *it.pnode;
        node->userptr = ENV_USED;
        return node->val;
    }

    // @note    we dont want to get system env when using own CmnEnv (i.e. not global thread variable)
    if (env)
        return NULL;

    /* populate from process env */
    /* @todo    once read, real process env will not be asked again; later changes of existing env vars will not get caught */ 
    /* @note    this may as well be a feature */
    {
        tchar *val = OSGetEnvCopy(key);
        if (!val)
            return NULL;

        hash_iter_insert(it, StrNewCopy(key), val);
        (*it.pnode)->userptr = ENV_USED;
        return val;
    }
}


tchar*
GetEnv(_In_ ctchar *name)
{
    if (CmnDefined(CONTEXT_ENV))
    {
        tchar *value = CmnEnvGet(NULL, name);
        if (value) DbgPlain(DBG_ENV, _T("[GetEnv] %s=%s"), name, value);
        return value;
    }
    return OSGetEnv(name);
}


int
PutEnv(_In_ tchar *envStr)
{
    ERH_FUNCTION(_T("PutEnv"));
    if (CmnDefined(CONTEXT_ENV))
    {
        CmnEnvPut(NULL, envStr);
    }
    else if (OSPutEnv(envStr) == -1)
    {
        return -1;
    }

    DbgPlain(DBG_ENV, _T("[PutEnv] %s"), envStr);
    return 0;
}


static int
OSSetEnv(_In_ ctchar *name, _In_opt_ ctchar *value)
{
    ERH_FUNCTION(_T("OSSetEnv"));

    value = NS(value);

    #if TARGET_LINUX
        return setenv(name, value, TRUE);
    #elif TARGET_WIN32
        return _tputenv_s(name, value);
    #else
        return putenv(StrFormat("%s=%s", name, value));
    #endif
}


int
SetEnv(_In_ ctchar *name, _In_opt_ ctchar *value)
{
    ERH_FUNCTION(_T("SetEnv"));
    if (CmnDefined(CONTEXT_ENV))
    {
        CmnEnvSet(NULL, name, value);
    }
    else if (OSSetEnv(name, value) == -1)
    {
        #ifndef TARGET_OPENVMS
        OSLOG2(setenv, _T("name:%s, value:%s"), name, NS(value));
        #endif
        return -1;
    }

    DbgPlain(DBG_ENV, _T("[SetEnv] %s=%s"), name, NS(value));
    return 0;
}



/*========================================================================*//**
*
* @ingroup   Common Library
*
* @param     env      Environment variable name
*
* @retval    0 on success, or -1 on error
*
* @brief     OmniBack wrappers for unsetenv() system calls
*
* @remarks   unsetenv in not available on all platforms
*            there a direct **environ handling is required
*            The unsetenv() function deletes the variable name from the environment.
*            If name does not exist in the environment, then the function succeeds,
*            and the environment is unchanged.
*
*//*=========================================================================*/
int
OSUnsetEnv(_In_ ctchar *name)
{
    ERH_FUNCTION(_T("OSUnsetEnv"));

    int r = 0;

    #if TARGET_LINUX
        r = unsetenv(name);
    #elif TARGET_MACOSX
        unsetenv(name);
    #elif TARGET_AIX
        /* unistd.h: extern char **environ; */
        char **e = environ;
        size_t len = strlen(name);
        for (; *e; e++) 
        {
            if (strncmp(name,*e,len) == 0 && ((*e)[len] == '=' || (*e)[len] == 0))
            {
                break;
            }
        }
        for (; *e; e++)
        {
            e[0] = e[1];
        }
    #else
        r = putenv(StrFormat(_T("%s="), name));
    #endif

    if (r == -1)
        OSLOG(unsetenv, _T("name:%s"), name);

    return r;
}


void 
UnsetEnv(_In_ ctchar *name)
{
    if (CmnDefined(CONTEXT_ENV))
    {
        CmnEnvUnset(NULL, name);
    }
    else
    {
        OSUnsetEnv(name);
    }

    DbgPlain(DBG_ENV, _T("[UnsetEnv] %s"), name);
}


void
CmnEnvExport (_In_opt_ const CmnEnv *env)
{
    hash_iter_t it;
    hash_iterate(env, it)
        OSSetEnv(hash_iter_key(it), hash_iter_value(it));
}
