/**
*
* (c) Copyright 2019 Micro Focus or one of its affiliates
*
* @ingroup   Common Library
* @file      lib/cmn/win32.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     NT specific functions needed by common library
*
* @since     23.01.97   luka     Original coding (out of common.c)
*/

#include "lib/cmn/target.h"

__rcsId("$Header: /lib/cmn/win32.c $Rev$ $Date::                      $:");

#include <winsock.h>
#include <crtdbg.h>
#include <lmcons.h>

#include "lib/cmn/common.h"
#include "lib/cmn/type_array.h"
#include "win32.h"

/* due to default old SDK */

#ifndef VER_SUITE_EMBEDDED_RESTRICTED
#define VER_SUITE_EMBEDDED_RESTRICTED       0x00000800
#define VER_SUITE_SECURITY_APPLIANCE        0x00001000
#endif
#ifndef VER_SUITE_STORAGE_SERVER
#define VER_SUITE_STORAGE_SERVER            0x00002000
#define VER_SUITE_COMPUTE_SERVER            0x00004000
#endif
#ifndef VER_SUITE_WH_SERVER
#define VER_SUITE_WH_SERVER                 0x00008000
#endif

#ifndef PRODUCT_UNDEFINED
#define PRODUCT_ULTIMATE                            0x00000001
#define PRODUCT_HOME_BASIC                          0x00000002
#define PRODUCT_HOME_PREMIUM                        0x00000003
#define PRODUCT_ENTERPRISE                          0x00000004
#define PRODUCT_HOME_BASIC_N                        0x00000005
#define PRODUCT_BUSINESS                            0x00000006
#define PRODUCT_STANDARD_SERVER                     0x00000007
#define PRODUCT_DATACENTER_SERVER                   0x00000008
#define PRODUCT_SMALLBUSINESS_SERVER                0x00000009
#define PRODUCT_ENTERPRISE_SERVER                   0x0000000A
#define PRODUCT_STARTER                             0x0000000B
#define PRODUCT_DATACENTER_SERVER_CORE              0x0000000C
#define PRODUCT_STANDARD_SERVER_CORE                0x0000000D
#define PRODUCT_ENTERPRISE_SERVER_CORE              0x0000000E
#define PRODUCT_ENTERPRISE_SERVER_IA64              0x0000000F
#define PRODUCT_BUSINESS_N                          0x00000010
#define PRODUCT_WEB_SERVER                          0x00000011
#define PRODUCT_CLUSTER_SERVER                      0x00000012
#define PRODUCT_HOME_SERVER                         0x00000013
#define PRODUCT_STORAGE_EXPRESS_SERVER              0x00000014
#define PRODUCT_STORAGE_STANDARD_SERVER             0x00000015
#define PRODUCT_STORAGE_WORKGROUP_SERVER            0x00000016
#define PRODUCT_STORAGE_ENTERPRISE_SERVER           0x00000017
#define PRODUCT_SERVER_FOR_SMALLBUSINESS            0x00000018
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM        0x00000019
#define PRODUCT_HOME_PREMIUM_N                      0x0000001A
#define PRODUCT_ENTERPRISE_N                        0x0000001B
#define PRODUCT_ULTIMATE_N                          0x0000001C
#define PRODUCT_WEB_SERVER_CORE                     0x0000001D
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT    0x0000001E
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY      0x0000001F
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING     0x00000020
#define PRODUCT_SMALLBUSINESS_SERVER_PRIME          0x00000021
#define PRODUCT_HOME_PREMIUM_SERVER                 0x00000022
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V          0x00000023
#define PRODUCT_STANDARD_SERVER_V                   0x00000024
#define PRODUCT_DATACENTER_SERVER_V                 0x00000025
#define PRODUCT_ENTERPRISE_SERVER_V                 0x00000026
#define PRODUCT_DATACENTER_SERVER_CORE_V            0x00000027
#define PRODUCT_STANDARD_SERVER_CORE_V              0x00000028
#define PRODUCT_ENTERPRISE_SERVER_CORE_V            0x00000029
#define PRODUCT_HYPERV                              0x0000002A
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2                                 89
#endif

#ifndef STRLEN_TO_BYTES
#define STRLEN_TO_BYTES(len)  ((len+1)*sizeof(TCHAR))
#endif

static int
CmnCleanupClustering(void)
{
    if (!cmnClus) 
        return(0);

    cmnClus = 0;

    return 1;
}

/*========================================================================*//**
*
* @ingroup   Common Library
*
* @retval    0 if OK, -1 on error
*
* @brief     Function initializes the clustering environment (if any).
*            Execution must be serialized.
*
*//*=========================================================================*/
static int
CmnInitializeClustering(void)
{
    OSVERSIONINFOEX osVer;
    int r, enabled = 0;

    osVer.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
    CmnGetVersionEx(&osVer);

    if (osVer.dwPlatformId != VER_PLATFORM_WIN32_NT || osVer.dwMajorVersion < 4)
    {
        return 0;
    }

    r = RegReadDWORD(HKEY_LOCAL_MACHINE, REGCOMMON, _T("ClusteringEnabled"), &enabled);
    if (r == 0 && enabled)
        cmnClus = 1;

    if (!cmnClus)
        CmnCleanupClustering();

    return 0;
}



/*========================================================================*//**
*
* @ingroup   Common Library
*
* @retval    0 if OK, -1 on error
*
* @brief     Function initialize running environemnt fo use with OmniBack II
*
* @remarks   No ErhMark() is done on error
*
*//*=========================================================================*/
int
CmnInitializeEnvironment (void)
{
    ERH_FUNCTION(_T("CmnInitializeEnvironment"));
    WORD wVersionRequested;
    WSADATA wsaData;
    BOOL ok = TRUE;
    int err;

    /* initialize only once per process */
    if (cmnProcessInited)
        return 0;

    /* signal we are up and running */
    if (!CmnDefined(ORACLE8))
    {
        tchar *mutexName = getenv(_T("OB2CMNINITMUTEX"));
        if (!StrIsEmpty(mutexName))
        {
            HANDLE mutex = OpenMutex(SYNCHRONIZE, FALSE, mutexName);
            if (mutex)
            {
                ReleaseMutex(mutex);
                Win32_CloseHandle(mutex);
            }
            putenv(_T("OB2CMNINITMUTEX="));
        }
    }

    /* set std handles as un-inheritable */
    Win32_InheritStdHandle(STD_OUTPUT_HANDLE, FALSE);

    Win32_InheritStdHandle(STD_INPUT_HANDLE, FALSE);

    Win32_InheritStdHandle(STD_ERROR_HANDLE, FALSE);


    /*-----------------------------------------------------------------
    | Note that KEY_WOW64_64KEY does not well work on 32-bit systems.
    +----------------------------------------------------------------*/
    if (XP64P())
    {
        RegSetFlag(KEY_WOW64_64KEY);
    }

    #if TARGET_WIN64
    /* On Itanium (Windows) platform by default alignment exceptions are thrown in case of datatype misalignment problem.
     This can be prevented by setting OB2_ALIGNNMENTEXCEPTION to 0. */
    {
        int value=1;
        EnvReadInt(_T("OB2_ALIGNNMENTEXCEPTION"), &value);
        if (value==0)
        {
            SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT); 
        }
    }
    #endif 

    SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    #ifdef _DEBUG
    /* For debug builds, enable memory leak reporting */
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
    #endif

    if (!CmnDefined(DP_AGNOSTIC))
    {
        /* This call is not really necessary, it is done implicitly by */
        /* the first GetBrandString() call.                            */
        /* We only do this to see if it fails so we can report it.     */
        ok &= CmnLoadBrandingDLL() == 0;
        if (!ok && !CmnDefined(NOINSTALL))
        {
            ErhMarkSys(ERH_BRANDCHGFAIL, GetLastError(), __FCN__, ERH_CRITICAL);
            return (-1);
        }
    }

    /* Initialize WSA library (needed for tcp/ip functions) */
    wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup (wVersionRequested, &wsaData);
    if (err != 0)
    {
        DbgLogFull(
            __FLNPT__, NULL,
            _T("CmnInitializeEnvironment: WSAStartup() failed, errcode=%d"),
            err
        );
        fprintf (stderr, _T("WSAStartup(): Sockets error: %d\n"), err);
        fflush (stderr);
        return (-1);
    }

    /* get installation paths */
    ok &= CmnGetInstallInfo() == 0;
    if (!ok && !CmnDefined(NOINSTALL))
        return -1;

    /* initialize clustering environment */
    ok &= CmnInitializeClustering() == 0;

    return ok? 0 : -1;
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @retval    Function is void
*
* @brief     Function cleans up running environment
*
*//*=========================================================================*/
int
CmnCleanupEnvironment(void)
{
    ERH_FUNCTION(_T("CmnCleanupEnvironment"));

    if (cmnUsageCount > 0) return (0);  /* still in use by some threads... */

    /* last CmnExit, cleanup everything: */

    if (cmnClus) CmnCleanupClustering();
    
    CmnUnloadBrandingDLL();

    if (WSACleanup() == SOCKET_ERROR && WSAGetLastError() != WSANOTINITIALISED)
    {
        SetLastError(WSAGetLastError());
        OSLOG(WSACleanup, _T(""));
    }

    cmnProcessInited = 0;

    return (0);
}



/*========================================================================*//**
*
* @ingroup   Common Library
*
* @param     tchar *user   output for user name (of this process)
* @param     tchar *domain output for domain name (of this process)
*
* @retval    -1     error
* @retval    0      user account
* @retval    1      system account
*
* @brief     This function queries for username and domain of the account starting
*            this process. It does the check if process is running under local system
*            account (regardless of the locale!). In that case CMN_USER_SYSTEM and
*            CMN_DOMAIN_SYSTEM are used. Otherwise, queried names of the owner
*            of the process are used.
*
* @remarks   o If it is impossible to get username\domain (very unlikely situation also
*              on a non networked computer) function returns NT_Domain\<user>.
*              If no information can be retrieved, then the function returns
*              WinNT\NT_Domain and the return code is -1;
*            o This function works if there is no network. It needs more time to execute
*              but returns cached info if network is unavailable.
*            o Function can be used with null parameters just to check if process
*              runs under Local System account.
*            o Supports Win NT.
*//*=========================================================================*/
int 
CmnGetUserAndDomain(
    _Out_z_cap_(STRLEN_USER+1)  tchar *user, 
    _Out_z_cap_(STRLEN_GROUP+1) tchar *domain)
{
    ERH_FUNCTION(_T("CmnGetUserAndDomain"));

    SID                        *queriedSid; /* SID queried from process token */
    SID                         localSystem = { 0 }; /* local system account SID */
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    HANDLE                      procToken = INVALID_HANDLE_VALUE; /* process token */
    TOKEN_USER                 *tokenUser = NULL;
    DWORD                       size = 0;
    SID_NAME_USE                snu;
    BOOL                        ok;
    int                         retval = -1;
    tchar   userbuf[STRLEN_USER+1];
    tchar   domainbuf[STRLEN_GROUP+1];
    DWORD   usrSize = STRLEN_USER; // UNLEN
    DWORD   domainSize = STRLEN_GROUP;

    if (!user) user = userbuf;
    if (!domain) domain = domainbuf;

    user[0] = domain[0] = 0;

/* ----------------------------------------------------- */
/* Initializing local system RID to compare with queried */

    InitializeSid(&localSystem, &siaNtAuthority, 1);
    *(GetSidSubAuthority(&localSystem, 0)) = SECURITY_LOCAL_SYSTEM_RID;


/* ----------------------------------------------------- */
/* Get SID from current procedure token                  */

    ok = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &procToken);
    if (!ok)
    {
        OSLOG(OpenProcessToken, _T("self, TOKEN_QUERY"));
        goto exit;
    }
    
    while (ok = GetTokenInformation(procToken, TokenUser, tokenUser, size, &size), !ok && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        tokenUser = REALLOC(tokenUser, size);
    }

    if (!ok)
    {
        OSLOG(GetTokenInformation, _T("TokenUser"));
        goto exit;
    }

    /* We have all info about user running this process */
    queriedSid = tokenUser->User.Sid;

/* ----------------------------------------------------- */
/* Validating and comparing SIDs                         */

    if (!IsValidSid(queriedSid))
    {
        OSLOG(IsValidSid, _T("queriedSid"));
        goto exit;
    }
    if (!IsValidSid(&localSystem))
    {
        OSLOG(IsValidSid, _T("localSystem"));
        goto exit;
    }
    if (EqualSid(queriedSid, &localSystem))
    {
        DbgPlain(1, _T("[%s] System account."), __FCN__);
        StrCopy(user, CMN_USER_SYSTEM, STRLEN_USER);
        StrCopy(domain, CMN_DOMAIN_SYSTEM, STRLEN_GROUP);
        retval = 1;
        goto exit;
    }

    DbgPlain(1, _T("[%s] Non-System account."), __FCN__);
    retval = 0;

/* ----------------------------------------------------- */
/* Getting username and domain name from queried SID     */
    ok = LookupAccountSid(
        NULL, queriedSid, user, &usrSize, domain, &domainSize,
        &snu
    );
    if (ok)
        goto exit;

    OSLOG(LookupAccountSid, _T("queriedSid"));
    StrCopy(domain, _T("NT_Domain"), STRLEN_GROUP); /* default domain */

    usrSize = STRLEN_USER;
    ok = GetUserName(user, &usrSize);
    if (!ok)
    {
        OSLOG(GetUserName, _T(""));
        StrCopy(user, _T("WinNT"), STRLEN_USER);  /* default username */
        retval = -1;
    }

exit:
    Win32_CloseHandle(procToken);
    FREE(tokenUser);

    return retval;
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @retval    0 if OK, -1 on error
*
* @brief     Function gets user and group info.
*
* @remarks   No ErhMark() is done on error
*
*//*=========================================================================*/
int
CmnGetUserInfo(void)
{
    if (cmnProcessInited)
        return (0);

    /* The rest of the function is executed only once per process! */
    /* All variables that are set should be global (no TREADLOCALs !!!) */

    CmnGetUserAndDomain(cmnEun, cmnEgn);
    /* Get username (function respects OB2 size limits)
     CmnGetUser(cmnEun); */
    _tcsupr(cmnEun);  /* make it upper case */

    /* Primary group is useless, just get domain and use it for (UNIX) group
     CmnGetPrimGroupAndDomain(NULL, cmnEgn); */
    _tcsupr(cmnEgn);  /* make it upper case */

    return (0);
}


static int
CmnGetBaseInstallInfo(void)
{
    ERH_FUNCTION(_T("CmnGetBaseInstallInfo"));

    if (CmnDefined(DP_AGNOSTIC) && cmnInitializationInfo.cmnGetBaseInstallInfo != NULL)
    {
        if (cmnInitializationInfo.cmnGetBaseInstallInfo(cmnPanBase, cmnPanBaseAppData, cmnPanLog, cmnPanTmp) != 0 ||
            StrIsEmpty(cmnPanBase) || StrIsEmpty(cmnPanBaseAppData))
        {
            ErhMark(ERH_MPROTO, __FCN__, ERH_CRITICAL);
            return(-1);
        }

        PathCutTrailSlashes(cmnPanBase, 0);
        PathCutTrailSlashes(cmnPanBaseAppData, 0);

        if (StrIsEmpty(cmnPanLog))
            PathConcat(cmnPanLog, cmnPanBaseAppData, _T("log\\"), STRLEN_PATH);

        if (StrIsEmpty(cmnPanTmp))
            PathConcat(cmnPanTmp, cmnPanBaseAppData, _T("tmp\\"), STRLEN_PATH);
    }
    else
    {
        if (!CmnGetDir(_T("HomeDir"), cmnPanBase))
        {
            ErhMarkSys(ERH_REGISTRYFAIL, GetLastError(), __FCN__, ERH_CRITICAL);
            return (-1);
        }

        if (!CmnGetDir(_T("DataDir"), cmnPanBaseAppData))
            StrCopy(cmnPanBaseAppData, cmnPanBase, STRLEN_PATH);

        if (!CmnGetDir(_T("OmniLog"), cmnPanLog))
            PathConcat(cmnPanLog, cmnPanBaseAppData, _T("\\log\\"), STRLEN_PATH);

        if (!CmnGetDir(_T("OmniTmp"), cmnPanTmp))
            PathConcat(cmnPanTmp, cmnPanBaseAppData, _T("\\tmp\\"), STRLEN_PATH);

        if (!CmnGetClusterSharedDir(cmnPanBaseSAppData))
            StrCopy(cmnPanBaseSAppData, cmnPanBaseAppData, STRLEN_PATH);
    }

    return(0);
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @retval    0 if OK, -1 on error
*
* @brief     Get installation paths
*
* @remarks   No ErhMark() is done on error
*
*//*=========================================================================*/
int
CmnGetInstallInfo (void)
{
    ERH_FUNCTION(_T("CmnGetInstallInfo"));
    tchar logdir[STRLEN_PATH+1];

    if (cmnProcessInited && cmnPanBase[0])
        return (0);

    Win32ApiInit();
    if (Win32.GetNativeSystemInfo)
    {
        SYSTEM_INFO systemInfo = {0};
        Win32.GetNativeSystemInfo(&systemInfo);
        cmnSystemInfo.machineArch = systemInfo.wProcessorArchitecture;
    }

    /* The rest of the function is executed only once per process! */
    /* All variables that are set should be global (no TREADLOCALs !!!) */
    if (CmnGetBaseInstallInfo() != 0)
    {
        if (CmnDefined(NOINSTALL))
        {
            if (GetTempPath(STRLEN_PATH, cmnPanTmp) == 0)
            {
                tchar drive[MAX_PATH+1] = _T("C:");
                GetSystemDirectory(drive, MAX_PATH);
                drive[2] = 0;
                StrNPrintf(cmnPanTmp, STRLEN_PATH, _T("%s\\temp"), drive);
            }

            StrCopy(cmnPanLog, cmnPanTmp, STRLEN_PATH);
            OB2_CreateDirectory(cmnPanTmp, 0);
        }

        return(-1);
    }


    if (!CmnGetDir(_T("OracleDBDir"), cmnPanOracleBase))
    {
        StrClear(cmnPanOracleBase);
        StrClear(cmnPanOracleBin);
    }
    else
    {
        PathConcat(cmnPanOracleBin, cmnPanOracleBase, _T("\\bin\\"), STRLEN_PATH);
    }

    if (CmnGetDir(_T("OmniConfigUNC"), cmnPanConfigBase))
    {
        /* Removed checking for OmniConfigUNC path existence. When installing CM as cluster-aware there are some cases
           where OmniConfigUNC path does not exist yet (i.e.:Config files were not yet copied on active node) and cmnPanConfigBase 
           is reset to cmnPanBaseAppData\... But this path does not include server configuration files on cluster-aware CM.
           The consequences are: path wrongly initialized in inet ->failing check installation, 
           inet is unable to provide cell_info files -> push inst is not working....

        if (OB2_StatFile(cmnPanConfigBase, NULL) < 0)
            PathConcat(cmnPanConfigBase, cmnPanBaseAppData, _T("\\config"), STRLEN_PATH);*/
        ;

    } 
    else 
    {
        PathConcat(cmnPanConfigBase, cmnPanBaseAppData, _T("config"), STRLEN_PATH);
    }

    PathConcat(cmnPanConfig,       cmnPanConfigBase,  _T("server\\"), STRLEN_PATH);
    PathConcat(cmnPanConfigClient, cmnPanBaseAppData, _T("config\\client\\"), STRLEN_PATH);
    PathConcat(cmnPanConfigIS,     cmnPanConfig,      _T("install\\"), STRLEN_PATH);


    /* CHECKME: is this ever set in registry? */
    if (!CmnGetDir(_T("OmniNewConfig"), cmnPanNewConfig))
        PathConcat(cmnPanNewConfig, cmnPanBaseAppData, _T("NewConfig\\"), STRLEN_PATH);

    StrCopy(logdir, cmnPanConfigBase, STRLEN_PATH);
    PathCutLeaf(logdir);

    /* support for DP8.0 Postgres(PG) and JobControlEngine(JCE) log paths */
    PathConcat(cmnPanLogCM_PG,  logdir, _T("server\\db80\\pg\\pg_log"), STRLEN_PATH);
    PathConcat(cmnPanLogCM_JCE, logdir, _T("log\\AppServer"), STRLEN_PATH);
    PathConcat(cmnPanLogCM,     logdir, _T("log\\server"), STRLEN_PATH);

    PathConcat(cmnPanEnhincrDB, cmnPanBaseAppData, _T("enhincrdb"), STRLEN_PATH);

    PathConcat(cmnPanBin          ,cmnPanBase,  _T("bin\\"), STRLEN_PATH);
    PathConcat(cmnPanSBin         ,cmnPanBase,  _T("bin\\"), STRLEN_PATH);
    PathConcat(cmnPanLBin         ,cmnPanBase,  _T("bin\\"), STRLEN_PATH);
    PathConcat(cmnPanScripts      ,cmnPanBase,  _T("bin\\"), STRLEN_PATH);
    PathConcat(cmnPanSessions     ,cmnPanConfig,_T("sessions\\"), STRLEN_PATH);
    PathConcat(cmnPanConfigEmc,    cmnPanConfigClient, _T("emc\\"), STRLEN_PATH);
    PathConcat(cmnPanConfigTmpEmc, cmnPanTmp,   _T("emc\\"), STRLEN_PATH);
    PathConcat(cmnPanKeyExport    ,cmnPanConfig,_T("export\\keys\\"), STRLEN_PATH);
    PathConcat(cmnPanKeyImport    ,cmnPanConfig,_T("import\\keys\\"), STRLEN_PATH);
    PathConcat(cmnPanMCFExport    ,cmnPanConfig,_T("export\\mcf"), STRLEN_PATH);
    PathConcat(cmnPanMCFImport    ,cmnPanConfig,_T("import\\mcf"), STRLEN_PATH);
    
    /* CHECKME: Are these used on NT? */
    PathConcat(cmnPanNls          ,cmnPanBase,  _T("nls\\"), STRLEN_PATH);
    PathConcat(cmnPanInst         ,cmnPanBase,  _T("tmp\\"), STRLEN_PATH);
    PathConcat(cmnPanHelp         ,cmnPanBase,  _T("tmp\\"), STRLEN_PATH);
    PathConcat(cmnPanIcons        ,cmnPanBase,  _T("tmp\\"), STRLEN_PATH);
    PathConcat(cmnPanOpC          ,cmnPanBase,  _T("tmp\\"), STRLEN_PATH);
    PathConcat(cmnPanLib          ,cmnPanBase,  _T("lib\\"), STRLEN_PATH);

    /* 32 bit libraries locations */
#if defined(_M_IX86)
    PathConcat(cmnPanLib32, cmnPanBase, _T("lib\\i386\\"), STRLEN_PATH);
#else /* fallback */
    PathConcat(cmnPanLib32, cmnPanBase, _T("bin\\"), STRLEN_PATH);
#endif

    /* 64 bit libraries locations */
#if defined(_M_IA64)
    PathConcat(cmnPanLib64, cmnPanBase, _T("\\lib\\ia64\\"), STRLEN_PATH);
#elif defined(_M_X64) || defined (_M_AMD64)
    PathConcat(cmnPanLib64, cmnPanBase, _T("\\lib\\x8664\\"), STRLEN_PATH);
#else /* fallback */
    PathConcat(cmnPanLib64, cmnPanBase, _T("\\bin\\"), STRLEN_PATH);
#endif
 
    PathConcat(cmnPanMan                 ,cmnPanBase,         _T("\\tmp\\"), STRLEN_PATH);
    PathConcat(cmnPanServerCertificates  ,cmnPanConfig,       _T("\\certificates\\"), STRLEN_PATH);
    PathConcat(cmnPanClientCertificates  ,cmnPanConfigClient, _T("\\certificates\\"), STRLEN_PATH);
    
    PathConcat(cmnPanClusCMSSCertificates,  cmnPanConfig,     _T("\\sscertificates"), STRLEN_PATH);
    PathConcat(cmnPanSSCertificates,        cmnPanConfigClient,       _T("\\sscertificates"), STRLEN_PATH);

    /* Directories for OB database. */
    if (!CmnGetDir(_T("OmniConfigDB"),  cmnPanConfigDB))
        PathConcat(cmnPanConfigDB, cmnPanBaseAppData, _T("server\\db80"), STRLEN_PATH);
    
    PathConcat(cmnPanConfigDB6x,   cmnPanBaseAppData,   _T("db40"),                 STRLEN_PATH);
    PathConcat(cmnPanDBdatafiles,  cmnPanConfigDB,      _T("datafiles\\"),          STRLEN_PATH);
    PathConcat(cmnPanDBcatalog,    cmnPanConfigDB,      _T("datafiles\\catalog\\"), STRLEN_PATH);
    PathConcat(cmnPanConfigIDB,    cmnPanConfig,        _T("idb\\"),                STRLEN_PATH);
    PathConcat(cmnPanDBcdb,        cmnPanConfig,        _T("idb\\"),                STRLEN_PATH);
    
    PathConcat(cmnPanDBmmdb,       cmnPanConfigDB,      _T("datafiles\\mmdb\\"),    STRLEN_PATH);
    PathConcat(cmnPanDBkeystore,   cmnPanConfigDB,      _T("keystore\\"),           STRLEN_PATH);
    PathConcat(cmnPanDBkeyidcat,   cmnPanConfigDB,      _T("keystore\\catalog\\"),  STRLEN_PATH);
    PathConcat(cmnPanDBlogfiles,   cmnPanConfigDB,      _T("logfiles\\"),           STRLEN_PATH);
    PathConcat(cmnPanDBsyslog,     cmnPanConfigDB,      _T("logfiles\\syslog\\"),   STRLEN_PATH);
    PathConcat(cmnPanDBrlog,       cmnPanConfigDB,      _T("logfiles\\rlog"),       STRLEN_PATH);
    PathConcat(cmnPanSessMsg,      cmnPanConfigDB,      _T("msg\\"),                STRLEN_PATH);
    PathConcat(cmnPanSmMeta,       cmnPanConfigDB,      _T("meta\\"),               STRLEN_PATH);
    PathConcat(cmnPanDBxp,         cmnPanConfigDB,      _T("xpdb\\ldev"),           STRLEN_PATH);
    PathConcat(cmnPanDBxpexcl,     cmnPanConfigDB,      _T("xpdb\\exclude"),        STRLEN_PATH);
    PathConcat(cmnPanDBxpcm,       cmnPanConfigDB,      _T("xpdb\\cm"),             STRLEN_PATH);
    PathConcat(cmnPanDBva,         cmnPanConfigDB,      _T("vadb\\ldev"),           STRLEN_PATH);
    PathConcat(cmnPanDBvaexcl,     cmnPanConfigDB,      _T("vadb\\exclude"),        STRLEN_PATH);
    PathConcat(cmnPanDBevalogin,   cmnPanConfigDB,      _T("evadb\\login"),         STRLEN_PATH);
    PathConcat(cmnPanDBevasnap,    cmnPanConfigDB,      _T("evadb\\snap"),          STRLEN_PATH);
    PathConcat(cmnPanDBevadg,      cmnPanConfigDB,      _T("evadb\\dgrules"),       STRLEN_PATH);
    PathConcat(cmnPanDBevaport,    cmnPanConfigDB,      _T("evadb\\port"),          STRLEN_PATH);
    PathConcat(cmnPanDBSQL,        cmnPanConfigDB,      _T("sqldb"),                STRLEN_PATH);
    PathConcat(cmnPanDBsa,         cmnPanConfigDB,      _T("sadb\\snapshots"),      STRLEN_PATH);
    PathConcat(cmnPanPostgresBase, cmnPanBase,          _T("idb"),                  STRLEN_PATH);
    PathConcat(cmnPanPostgresBin,  cmnPanPostgresBase,  _T("bin"),                  STRLEN_PATH);

    /* restore reporting */
    PathConcat(cmnPanDBReports,    cmnPanConfigDB,      _T("reportdb\\"),           STRLEN_PATH);
    PathConcat(cmnPanDBReportsRestore, cmnPanConfigDB,  _T("reportdb\\restore\\"),  STRLEN_PATH);

    StrReplace(cmnPanConfig,   '\\', '/');
    StrReplace(cmnPanConfigDB, '\\', '/');

    CmnGetCShostname(_T("CellServer"), cmnCSHostname);

    return (0);
}


/*========================================================================*//**
*
* @ingroup   Common Library (WIN32)
*
* @param     (none)
*
* @retval    int      350, 351, 400 for NTs, 95 or 98 for Windows 95 and 98
*                     500 for Windows 2000, 490 for Windows ME
*                     501 for Windows XP
*                     600 for Vista and Longhorn
*
* @brief     Function returns a proprietary identification for Windows version.
*
* @remarks   DA hint: (winver >= 400) | (winver < 100) have User Profiles
*
*//*=========================================================================*/
int
Win32Version()
{
    ERH_FUNCTION(_T("Win32Version"));
    OSVERSIONINFOEX osVer;
    static int retval = -1;  /* Non-threadlocal static without critical section */
                             /* Guess it'll work just fine, though. (Lars) */

    if (retval != -1)
    {
        /* we've been here before, just return it! */
        return retval;
    }

    if (!CmnGetVersionEx(&osVer))
    {
        return -1;  /* retval unchanged */
    }

    switch (osVer.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        /* Windows NT */
        retval = (osVer.dwMajorVersion*100+osVer.dwMinorVersion);
        break;

    case VER_PLATFORM_WIN32_WINDOWS:
        /* Windows 95 or Windows 98. */
        if (osVer.dwMajorVersion != 4) goto unexpected_lbl;
        if (osVer.dwMinorVersion == 0)
        {
            retval = 95;
            break;
        }
        if (osVer.dwMinorVersion == 10)
        {
            retval = 98;
            break;
        }
        if (osVer.dwMinorVersion == 90)
        {
            retval = 490;
            break;
        }
        goto unexpected_lbl;

    case VER_PLATFORM_WIN32s:
        /* Win32s on Windows 3.1 */
        retval = 311;
        break;

    default:
        goto unexpected_lbl;
    }

    return retval;


unexpected_lbl:

    DbgStamp(DBG_UNEXPECTED);
    DbgPlain(DBG_UNEXPECTED, _T("%s: unexpected values:")
        _T("\n\tdwPlatformId   = %lu")
        _T("\n\tdwMajorVersion = %lu")
        _T("\n\tdwMinorVersion = %lu"),
        __FCN__,
        osVer.dwPlatformId,
        osVer.dwMajorVersion,
        osVer.dwMinorVersion
    );

    retval = 0;  /* unknown */
    return retval;
}

/*========================================================================*//**
*
* @brief     Predicate determines if we are running on 64-bit XP. If so, other
*            package must be run.
*
* @since     07.may 2002 jozem       XP64P(): FreeLibrary(), not CloseHandle()
*
*//*=========================================================================*/
BOOL
XP64P()
{
    ERH_FUNCTION(_T("XP64P"));
    BOOL bRet = FALSE;
    tchar szSystem[MAX_PATH+1];
    UINT nResult = 0;

    DbgFcnIn();
    
    Win32ApiInit();

    if (!Win32.kernel32)
    {
        /*-------------------------------------------------------------
        | Failure here probably indicates Win9x
        +------------------------------------------------------------*/
        OSLOGW(LoadLibrary, _T("Kernel Module Library"));
        goto exit;
    }

    /*-----------------------------------------------------------------
    | GetSystemWow64Directory procedure is available on XP, but not
    | on NT/2000
    +----------------------------------------------------------------*/
    if (!Win32.GetSystemWow64Directory)
    {
        DbgPlain(97, _T("GetSystemWow64Directory not found in Kernel Module Library => not an XP"));
        goto exit;
    }

    /*-----------------------------------------------------------------
    | We've got the procedure address, so we are running XP now.
    | On 32bit-platforms, this function fails (ret==0) with extended
    |   error being ERROR_CALL_NOT_IMPLEMENTED
    | On 64-bit platforms, it returns the length of the name of the
    |   Wow64 directory (>0).
    +----------------------------------------------------------------*/
    nResult = Win32.GetSystemWow64Directory(szSystem, MAX_PATH);
    if (nResult)
    {
        DbgPlain(97, _T("System dir found at %s"), szSystem);
        DbgPlain(97, _T("So, we can conclude this is Windows 64bit"));
        bRet = TRUE;
        goto exit;
    }
    else
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            DbgPlain(97, _T("Call not implemented error... 32 bit Windows"));
        }
        else
        {
            OSLOG(GetSystemWow64Directory, _T(""));
        }
    }


exit:
    RETURN_BOOL(bRet);
} /* XP64P */


static const tchar*
GetWindowsSuiteName(DWORD suite)
{
    return
        suite & VER_SUITE_DATACENTER    ? _T("Datacenter") :
        suite & VER_SUITE_ENTERPRISE    ? _T("Enterprise") :
        suite & VER_SUITE_COMPUTE_SERVER? _T("Compute Cluster") :
        suite & VER_SUITE_BLADE         ? _T("Web Edition") :
                                          _T("Standard");
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @param     Returns productName, the build number, the os major version number,
*            the os minor version number, the service pack and its build number
*
*
* @retval    1 if successful, 0 on failure 
*
* @brief     Gets OS information from the registry 
*
* @remarks   Limited to STRLEN_STD.
*
*//*=========================================================================*/
static BOOL
RegQueryString(_In_ HKEY hkey, _In_ ctchar *valueName, _Out_z_cap_(maxlen+1) tchar *buf, _In_ size_t maxlen)
{
    ERH_FUNCTION(_T("RegQueryString"));
    DWORD type = 0;
    DWORD size = (maxlen+1)*sizeof(tchar);
    DWORD n;
    int r = RegQueryValueEx(hkey, valueName, NULL, &type, (void*)buf, &size);
    if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size < sizeof(tchar))
    {
        if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND)
        {
            SetLastError(r);
            OSLOG(RegQueryValueEx, _T("valueName:%s"), valueName);
        }
        buf[0] = 0;
        return FALSE;
    }

    n = size/sizeof(tchar);
    if (buf[n-1])
        buf[n - (n>maxlen)] = 0;

    return TRUE;
}

_Success_(return) static BOOL
RegQueryDWORD(_In_ HKEY hkey, _In_ ctchar *valueName, _Out_ DWORD *value)
{
    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    DWORD v;
    if (RegQueryValueEx(hkey, valueName, NULL, &type, (LPBYTE)&v, &size) != ERROR_SUCCESS || type != REG_DWORD)
        return FALSE;
    *value = v;
    return TRUE;
}

static void
CmnGetRegProductInfo(_Inout_ OSVERSIONINFOEX *info)
{
    ERH_FUNCTION(_T("CmnRegGetProductInfo"));
    tchar s[STRLEN_PATH+2] = {0};
    HKEY  hKey = NULL;
    DWORD r;

    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_WINNT_CURRENTVERSION, 0, KEY_EXECUTE|WOW64_REG_VIEW|RegGetFlags(), &hKey);
    if (r != ERROR_SUCCESS)
    {
        SetLastError(r);
        OSLOG(RegOpenKeyEx, _T("key:%s"), REG_WINNT_CURRENTVERSION);
        ErhMark(ERH_NOHOSTFILE, __FCN__, ERH_CRITICAL);
        return;
    }

    if (RegQueryDWORD(hKey, _T("CurrentMajorVersionNumber"), &info->dwMajorVersion) &&
        RegQueryDWORD(hKey, _T("CurrentMinorVersionNumber"), &info->dwMinorVersion))
    {
        ;
    }
    else if (RegQueryString(hKey, _T("CurrentVersion"), s, STRLEN_PATH))
    {
        sscanf(s, _T("%d.%d"), &info->dwMajorVersion, &info->dwMinorVersion);
    }

    // Get current build
    if (RegQueryString(hKey, _T("CurrentBuild"), s, STRLEN_PATH))
    {
        sscanf(s, _T("%d"), &info->dwBuildNumber);
    }

    /* Get Service Pack build number
    if (RegQueryString(hKey, _T("CSDBuildNumber"),s, STRLEN_PATH))
    {
        sscanf(s, _T("%d"), buildSP );
    }

    Get Product Name
    if (RegQueryString(hKey, _T("ProductName"), s, STRLEN_PATH))
    {
        StrCopy(productName, s, STRLEN_STD);
    }
    */

    // Get service pack version
    if (RegQueryString(hKey, _T("CSDVersion"), s, STRLEN_PATH))
    {
        StrCopy(info->szCSDVersion, s, ARRAYSIZE(info->szCSDVersion)-1);
    }

    r = RegCloseKey(hKey);
    if (r != ERROR_SUCCESS)
    {
        SetLastError(r);
        OSLOGW(RegCloseKey, _T("key:%s"), REG_WINNT_CURRENTVERSION);
    }
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @param     OSVERSIONINFO   
*
* @retval    1 if successful, 0 otherwise 
*
* @brief     replaces GetVersionEx to correct wrong version numbers
*
* QUOTE MSDN:
* With the release of Windows 8.1, (-> >= 6.2)
* the behavior of the GetVersionEx API has changed in the value 
* it will return for the operating system version. 
* The value returned by the GetVersionEx function now depends on
* how the application is manifested.
*
*//*=========================================================================*/
BOOL
CmnGetVersionEx(_Out_ OSVERSIONINFOEX *info)
{
    ERH_FUNCTION(_T("CmnGetVersionEx"));
    info->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((void*)info))
    {
        OSLOGW(GetVersionEx, _T(""));
        return FALSE;
    }

    if (info->dwMajorVersion >= 6 && info->dwMinorVersion >= 2)
    {
        CmnGetRegProductInfo(info);
    }
    return TRUE;
}


/*========================================================================*//**
*
* @ingroup   Common Library
*
* @param     machineInfo   Pointer where to put machine info string
*
* @retval    Void function.
*
* @brief     Gets machine information and construct machine info string
*
*//*=========================================================================*/
void
CmnGetMachineInfoString(_Out_z_cap_(STRLEN_STD+1) tchar *machineInfo)
{
    Variant         info = VarUndef;
    OSVERSIONINFOEX ver  = {0};
    SYSTEM_INFO     sys  = {0};

    DWORD cpuType;
    DWORDLONG conditionMask = 0;

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    machineInfo[0] = 0;

    if (!CmnGetVersionEx(&ver))
        return;

    Win32ApiInit();

    /* Call GetNativeSystemInfo if supported or GetSystemInfo otherwise. */
    if (Win32.GetNativeSystemInfo)
    {
        Win32.GetNativeSystemInfo(&sys);
    }
    else 
    {
        GetSystemInfo(&sys);
    }

    cpuType = sys.wProcessorArchitecture;

    if (VER_PLATFORM_WIN32_NT == ver.dwPlatformId && ver.dwMajorVersion > 4)
    {
        DWORD productType = 0;
        const tchar *cpuText = NULL;

        VarPuts(&info, _T("Microsoft "));

        switch (ver.dwMajorVersion * 100 + ver.dwMinorVersion)
        {
        case 1000:
            VarPuts(&info, VER_NT_WORKSTATION == ver.wProductType?
                _T("Windows 10"):
                _T("Windows Server 2016"));
            break;
        case 603:
            VarPuts(&info, VER_NT_WORKSTATION == ver.wProductType?
                    _T("Windows 8.1"):
                    _T("Windows Server 2012 R2"));
            break;
        case 602:
            VarPuts(&info, VER_NT_WORKSTATION == ver.wProductType?
                    _T("Windows 8 ") :
                    _T("Windows Server 2012"));
            break;
        case 601:
            VarPuts(&info, VER_NT_WORKSTATION == ver.wProductType? 
                   _T("Windows 7 ") : 
                   _T("Windows Server 2008 R2 "));
            break;

        case 600:
            VarPuts(&info, VER_NT_WORKSTATION == ver.wProductType?
                   _T("Windows Vista ") : 
                   _T("Windows Server 2008 "));

            if (Win32.GetProductInfo)
                Win32.GetProductInfo( 6, 0, 0, 0, &productType);

            #define CASE(type_,text_) case type_ : VarPuts(&info, _T(text_)); break
            switch (productType)
            {
            CASE(PRODUCT_ULTIMATE,                      "Ultimate Edition");
            CASE(PRODUCT_HOME_PREMIUM,                  "Home Premium Edition");
            CASE(PRODUCT_HOME_BASIC,                    "Home Basic Edition");
            CASE(PRODUCT_ENTERPRISE,                    "Enterprise Edition");
            CASE(PRODUCT_BUSINESS,                      "Business Edition");
            CASE(PRODUCT_STARTER,                       "Starter Edition");
            CASE(PRODUCT_CLUSTER_SERVER,                "Cluster Server Edition");
            CASE(PRODUCT_DATACENTER_SERVER,             "Datacenter Edition");
            CASE(PRODUCT_DATACENTER_SERVER_CORE,        "Datacenter Edition (core installation)");
            CASE(PRODUCT_ENTERPRISE_SERVER,             "Enterprise Edition");
            CASE(PRODUCT_ENTERPRISE_SERVER_CORE,        "Enterprise Edition (core installation)");
            CASE(PRODUCT_ENTERPRISE_SERVER_IA64,        "Enterprise Edition for Itanium-based Systems");
            CASE(PRODUCT_SMALLBUSINESS_SERVER,          "Small Business Server");
            CASE(PRODUCT_SMALLBUSINESS_SERVER_PREMIUM,  "Small Business Server Premium Edition");
            CASE(PRODUCT_STANDARD_SERVER,               "Standard Edition");
            CASE(PRODUCT_STANDARD_SERVER_CORE,          "Standard Edition (core installation)");
            CASE(PRODUCT_WEB_SERVER,                    "Web Server Edition");
            }
            #undef CASE

            if (PROCESSOR_ARCHITECTURE_AMD64 == cpuType)
                VarPuts(&info, _T( ", 64-bit"));

            else if (PROCESSOR_ARCHITECTURE_INTEL == cpuType)
                VarPuts(&info, _T(", 32-bit"));
            break;

        case 502:
            if (VER_SUITE_STORAGE_SERVER == ver.wSuiteMask)
                VarPuts(&info, _T("Windows Storage Server 2003"));
            else if (VER_SUITE_WH_SERVER == ver.wSuiteMask)
                VarPuts(&info, _T("Windows Home Server"));
            else if (VER_NT_WORKSTATION == ver.wProductType && PROCESSOR_ARCHITECTURE_AMD64 == cpuType)
                VarPuts(&info, _T("Windows XP Professional x64 Edition"));
            else 
                VarPuts(&info, _T("Windows Server 2003, "));

            /* Test for the server type. */
            if (VER_NT_WORKSTATION != ver.wProductType)
            {
                switch (cpuType)
                {
                case PROCESSOR_ARCHITECTURE_IA64:
                    cpuText = _T("Edition for Itanium-based Systems");
                    break;
                case PROCESSOR_ARCHITECTURE_AMD64:
                    cpuText = _T("x64 Edition");
                    break;
                default:
                    cpuText = _T("Edition");
                }
                VarCat(&info, _T("%s %s"), GetWindowsSuiteName(ver.wSuiteMask), cpuText);
            }
            break;

        case 501:
            VarPuts(&info, _T("Windows XP "));
            if (ver.wSuiteMask & VER_SUITE_PERSONAL)
                VarPuts(&info, _T("Home Edition"));
            else 
                VarPuts(&info, _T("Professional"));
            break;

        case 500:
            VarPuts(&info, _T("Windows 2000 "));
            if (ver.wProductType == VER_NT_WORKSTATION)
            {
                VarPuts(&info, _T("Professional"));
            }
            else 
            {
                if (ver.wSuiteMask & VER_SUITE_DATACENTER)
                    VarPuts(&info, _T("Datacenter Server"));
                else if (ver.wSuiteMask & VER_SUITE_ENTERPRISE)
                    VarPuts(&info, _T("Advanced Server"));
                else 
                    VarPuts(&info, _T("Server"));
            }
            break;
        }

        /* Include service pack (if any) and build number. */
        if (!StrIsEmpty(ver.szCSDVersion))
        {
            VarCat(&info, _T(" %s"), ver.szCSDVersion);
        }

        VarCat(&info, _T(" (build %d)"), ver.dwBuildNumber);
    }
    else
    {
        VarCat(&info, _T("Microsoft Windows %d:%d.%d"), ver.dwPlatformId, ver.dwMajorVersion, ver.dwMinorVersion);
    }

    /* testing function Win32Version():  (comment/uncomment this as needed!) */
    VarCat(&info, _T(" (ver %d)"), Win32Version());

    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMAJOR, VER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMINOR, VER_EQUAL);
    if (!VerifyVersionInfo(&ver, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR, conditionMask)
        &&  GetLastError() == ERROR_OLD_WIN_VERSION )
    {
        /* application compatibility mode */
        VarPuts(&info, _T(" (app. compat.)"));
    }

    /* information about build settings */
#if defined(_M_ALPHA)
    VarPuts(&info, _T(" [ALPHA]"));
#elif defined(_M_IX86)
    /* _T("") / _T("  [Intel]") */
#elif defined(_M_IA64)
    VarPuts(&info, _T(" [IA64]"));
#elif defined(_M_MRX000)
    VarPuts(&info, _T(" [MIPS]"));
#elif defined(_M_MPPC)
    VarPuts(&info, _T(" [PowerPC]"));
#elif defined(_M_PPC)
    VarPuts(&info, _T(" [Power Macintosh]"));
#elif defined(_M_X64) || defined (_M_AMD64)
    VarPuts(&info, _T(" [x64]"));
#endif

#ifndef _UNICODE
#   ifdef _MBCS
    VarPuts(&info, _T(" MBCS"));
#   else
    VarPuts(&info, _T(" ASCII"));
#   endif
#endif

#ifdef _DEBUG
#   if !defined(_MT)
    VarPuts(&info, _T(" <Debug Single Threaded>"));
#   else
    VarPuts(&info, _T(" <Debug>"));
#   endif
#   if defined(_OB2_CMN_DLL)
    VarPuts(&info, _T(" library in DLL"));
#   endif
#else
#   if !defined(_MT)
    VarPuts(&info, _T(" <Single Threaded>"));
#   else
    /* Release, multithreaded (OK) */
#   endif
#endif
#   if !defined(_DLL)
    VarPuts(&info, _T(" CRT linked statically"));
#   endif

    StrCopy(machineInfo, V2T(&info), STRLEN_STD);
    VarFree(&info);
}

void
CmnSetProcessTitle(_Printf_format_string_ const tchar *fmt, ...)
{
}


/*========================================================================*//**
*
* @ingroup   Command Line Interface
*
* @brief     Reads host of the local cell server (CS)
*
*//*=========================================================================*/
int
CliReadCSHost (tchar **host)
{
    ERH_FUNCTION(_T("CliReadCSHost"));
    
    tchar *val = RegGetString(HKEY_LOCAL_MACHINE, REGSITE, _T("CellServer"));
    if (!val)
    {
        OSLOG(RegGetString, _T("keyName:%s, valueName:CellServer"), REGSITE);
        ErhMark(ERH_NOHOSTFILE, __FCN__, ERH_CRITICAL);
        return(-1);
    }

    CmnExpandHostName(val, host);
    FREE(val);

    return 0;
}


static BOOL WINAPI
CliCommandModeIntr(DWORD dwType)
{
    DbgStamp(10);
    DbgPlain(10, _T("CTRL-C interrupt\n"));

    switch (dwType)
    {
        case CTRL_C_EVENT:
            commandModeSet = 1;
            return (TRUE);
        default:
            return (FALSE);
    }
}

void
CliSetCommandModeIntr()
{
    ERH_FUNCTION(_T("CliSetCommandModeIntr"));
    HANDLE hCon;
    DWORD  dwMode;

    hCon = GetStdHandle(STD_INPUT_HANDLE);
    if(hCon == INVALID_HANDLE_VALUE)
    {
        OSLOG(GetStdHandle, _T("STD_INPUT_HANDLE"));
    }

    if (!GetConsoleMode(hCon, &dwMode) && GetLastError() != ERROR_INVALID_HANDLE)
    {
        OSLOG(GetConsoleMode, _T("STD_INPUT_HANDLE"));
    }
    
    if (!SetConsoleMode(hCon, (dwMode | ENABLE_PROCESSED_INPUT)) && GetLastError() != ERROR_INVALID_HANDLE)
    {
        OSLOG(SetConsoleMode, _T("STD_INPUT_HANDLE, ENABLE_PROCESSED_INPUT"));
    }

    if (!SetConsoleCtrlHandler(CliCommandModeIntr, TRUE))
    {
        OSLOG(SetConsoleCtrlHandler, _T("CliCommandModeIntr"));
    }
}


/*========================================================================*//**
*
* @param     None
*
* @retval    Void
*
* @brief     Signals to the system that this process is not to be interrupted
*            by means of ACPI power management functions.
*
*            SetThreadExecutionState() is only present with Windows 98 or
*            Windows 2000 (or above). Hence, we link dynamically to handle the
*            cases of Windows NT 4.0 (or below) and Windows 95 with the same
*            binary.
*
* @since     23.10.00   jozem    Initial version.
*
*//*=========================================================================*/
#if defined(_M_ALPHA)

void OB2_Win32PowerManagement() {}

#else

void OB2_Win32PowerManagement()
{
    ERH_FUNCTION(_T("OB2_Win32PowerManagement"));

    DbgFcnIn();

    /*-----------------------------------------------------------------
    | Consult environment variable OB2_NTALLOWSUSPEND.
    | This one allows user to have Omniback II to honor power messages
    | regardless of activity.
    +----------------------------------------------------------------*/
    if(EnvGetBool(_T("OB2_NTALLOWSUSPEND"))) 
    {
        DbgPlain(97, _T("Power Management honored as-is due to OB2_NTALLOWSUSPEND."));
        goto exit;
    }

    /*-----------------------------------------------------------------
    | Get handle to kernel32.dll. Note that kernel32.dll is expected
    | to be present in memory already. We do not load it nor are we
    | to release it.
    +----------------------------------------------------------------*/
    Win32ApiInit();
    if (Win32.kernel32 == NULL) 
    {
        goto exit;
    }

    /*-----------------------------------------------------------------
    | Locate SetThreadExecutionState() in kernel32.dll.
    +----------------------------------------------------------------*/
    if (Win32.SetThreadExecutionState == NULL)
    {
        DbgPlain(97, _T("Cannot locate SetThreadExecutionState() in Kernel Module Library"));
        goto exit;
    }

    /*-----------------------------------------------------------------
    | OK. Set the flags now. Ignore previous execution state returned
    | by the function since we arene't really thinking about resetting
    | these flags back.
    +----------------------------------------------------------------*/
    Win32.SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    DbgPlain(97, _T("Power Management adjusted to ")
                 _T("ES_SYSTEM_REQUIRED | ES_CONTINUOUS"));

exit:
    DbgFcnOut();
} /* OB2_Win32PowerManagement */

#endif



BOOL 
Win32_InheritHandleM(_Inout_ HANDLE *handle, _In_ BOOL inherit, _In_ ctchar *handleName, _In_ ctchar *__FCN__)
{
    DWORD newflag = inherit? HANDLE_FLAG_INHERIT : 0;
    DWORD flags = 0;
    BOOL ok;

    if ((handle == NULL) || (*handle == NULL) || (*handle == INVALID_HANDLE_VALUE))
        return(TRUE);

    ok = GetHandleInformation(*handle, &flags);
    if (!ok)
    {
        OSLOGW(GetHandleInformation, _T("%s=%p"), handleName, *handle);
        return(FALSE);
    }

    if ((flags & HANDLE_FLAG_INHERIT) == newflag)
        return(ok);

    ok = SetHandleInformation(*handle, HANDLE_FLAG_INHERIT, newflag);
    if (!ok)
        OSLOGW(SetHandleInformation, _T("%s=%p,flags:%x,type:%d"), handleName, *handle,newflag, GetFileType(*handle));

    return ok;
}


BOOL
Win32_InheritStdHandleM(_In_ DWORD handleID, _In_ BOOL inherit, _In_ ctchar *handleName, _In_ ctchar *fcn)
{
    HANDLE h = GetStdHandle(handleID);
    if (!h)
        return TRUE;

    if (GetFileType(h) == FILE_TYPE_CHAR) // cant un-inherit console handles
        return TRUE;

    return Win32_InheritHandleM(&h, inherit, handleName, fcn);
}


BOOL
Win32_CloseHandleM(_Inout_ HANDLE *handle, _In_ ctchar *handleName, _In_ ctchar *__FCN__)
{
    BOOL ok = TRUE;

    if (!handle || *handle == INVALID_HANDLE_VALUE || *handle == NULL)
        return TRUE;

    ok = CloseHandle(*handle);
    if (!ok)
        OSLOGW(CloseHandle, _T("%s=%p"), handleName, *handle);

    *handle = INVALID_HANDLE_VALUE;

    return ok;
}


// Exception handling
int OB2_ExceptionHandler (struct _EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *e = info->ExceptionRecord;
    #define IF(TYPE) e->ExceptionCode==EXCEPTION_##TYPE? _T(#TYPE) :
    tchar *name = 
        IF (ACCESS_VIOLATION)          IF (ARRAY_BOUNDS_EXCEEDED)
        IF (BREAKPOINT)                IF (DATATYPE_MISALIGNMENT)
        IF (FLT_DENORMAL_OPERAND)      IF (FLT_DIVIDE_BY_ZERO)
        IF (FLT_INEXACT_RESULT)        IF (FLT_INVALID_OPERATION)
        IF (FLT_OVERFLOW)              IF (FLT_STACK_CHECK)
        IF (FLT_UNDERFLOW)             IF (ILLEGAL_INSTRUCTION)
        IF (IN_PAGE_ERROR)             IF (INT_DIVIDE_BY_ZERO)
        IF (INT_OVERFLOW)              IF (INVALID_DISPOSITION)
        IF (NONCONTINUABLE_EXCEPTION)  IF (PRIV_INSTRUCTION)
        IF (SINGLE_STEP)               IF (STACK_OVERFLOW)
        _T("Unknown!");
    #undef IF

    DbgPlain (DBG_UNEXPECTED, _T("Win32 Exception!\n")
        _T("\t|Type            %s (0x%0X)\n")
        _T("\t|Location        %p\n")
        _T("\t|Can continue    %s\n")
        _T("\t|Details         %s\n"),
        name, e->ExceptionCode,
        e->ExceptionAddress,
        e->ExceptionFlags==0? _T("Yes") : _T("No"),
        e->ExceptionCode != EXCEPTION_ACCESS_VIOLATION? _T("None") : 
            e->ExceptionInformation[0]==0? _T("While reading") : _T("While writing")
    );
    DbgDumpStack (DBG_UNEXPECTED);
    return EXCEPTION_EXECUTE_HANDLER;
}


/* ===========================================================================
| @section  Win32 services
 =========================================================================== */

/**----------------------------------------------------------------------------
@brief  check if the service state is one of the pending states
@retval TRUE if service state is a pending state; FALSE otherwise

@param  serviceState  state of the service, e.g. SERVICE_STOPPED or SERVICE_START_PENDING
*/
static BOOL
isServiceStatePending (_In_ int serviceState)
{
    switch (serviceState)
    {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_PAUSE_PENDING:
        return TRUE;
    default:
        return FALSE;
    }
}

ctchar*
Win32_ServiceStateString(DWORD state)
{
    switch (state)
    {
    #define _case(_name) case _name: return _T(#_name)
    _case(SERVICE_CONTINUE_PENDING);
    _case(SERVICE_PAUSE_PENDING);
    _case(SERVICE_PAUSED);
    _case(SERVICE_RUNNING);
    _case(SERVICE_START_PENDING);
    _case(SERVICE_STOP_PENDING);
    _case(SERVICE_STOPPED);
    default: return StrFromInt(state);
    }
}

DWORD
Win32_ServiceOpen(
    _Out_ WIN32_SERVICE *srv, 
    _In_ SC_HANDLE scm, 
    _In_ ctchar *serviceName, 
    _In_ DWORD access)
{
    ERH_FUNCTION(_T("Win32_ServiceOpen"));
    memzero(srv);

    srv->handle = OpenService(scm, serviceName, access);
    if (!srv->handle)
    {
        OSLOGW(OpenService, _T("name:%s, access:%d"), serviceName, access);
        return GetLastError();
    }

    srv->scm = scm;
    srv->name = StrNullCopy(serviceName);
    srv->displayName = NULL;
    srv->userData = NULL;
    return ERROR_SUCCESS;
}


void
Win32_ServiceClose(_Inout_opt_ WIN32_SERVICE *srv)
{
    ERH_FUNCTION(_T("Win32_ServiceClose"));
    if (!srv)
        return;

    if (srv->handle && !CloseServiceHandle(srv->handle))
        OSLOGW(CloseServiceHandle, _T("name:%s"), NSD(srv->name));

    FREE(srv->name);
    FREE(srv->displayName);
    srv->handle = srv->scm = NULL;
    return;
}


_Success_(return == 0) static DWORD
Win32_WaitPendingService(_In_ WIN32_SERVICE *srv, _In_ dpint64_t endTime, _Out_ DWORD *state)
{
    ERH_FUNCTION(_T("Win32_WaitPendingService"));
    BOOL ok;
    SERVICE_STATUS status = {0};

    DbgFcnIn();

    /* @todo use SERVICE_CONTROL_INTERROGATE instead?
       msdn: The QueryServiceStatus() function returns the most recent service status information reported to the service control manager. 
       If the service just changed its status, it may not have updated the service control manager yet. 
       Applications can find out the current service status by interrogating the service directly using the ControlService()
       function with the SERVICE_CONTROL_INTERROGATE control code. */

    while (ok = QueryServiceStatus(srv->handle, &status), 
           ok && isServiceStatePending(status.dwCurrentState) && CmnGetSystemTimeAsFileTime() <= endTime)
    {
        Sleep(Minimum(status.dwWaitHint, 500));
    }

    if (!ok)
    {
        OSLOG(QueryServiceStatus, _T("name:%s"), srv->name);
        RETURN_UINT(GetLastError());
    }

    *state = status.dwCurrentState;

    DbgPlain(DBG_DETAIL_PROGTRACE,_T("[%s] name:%s, state:%s"), __FCN__, srv->name, Win32_ServiceStateString(*state));

    if (isServiceStatePending(*state))
    {
        RETURN(ERROR_SERVICE_REQUEST_TIMEOUT);
    }

    RETURN(ERROR_SUCCESS);
}

    
/**----------------------------------------------------------------------------
@brief  Try to start or stop a service, without checking for dependencies,
        and wait for a limited time for service to react.
@retval Windows error code, ERROR_SUCCESS on success
        otherwise e.g. ERROR_SERVICE_REQUEST_TIMEOUT, ERROR_DEPENDENT_SERVICES_RUNNING, ...

@param  hService    handle to the service, with enough access rights
@param  timeout     seconds to wait for the service to start or stop
@param  serviceArgs parameters for starting the service; can be NULL
@param  finalState  SERVICE_STOPPED or SERVICE_RUNNING
                    finalState determines the request to service, either starting or stopping
*/

static DWORD 
Win32_ControlService(
    _In_     WIN32_SERVICE *srv, 
    _In_     int            timeout,
    _In_opt_ const argvRec *serviceArgs,
    _In_     DWORD          finalState)
{
    ERH_FUNCTION(_T("Win32_ControlService"));
    DWORD     state, lastError;
    dpint64_t end = CmnGetSystemTimeAsFileTime() + TIME_TO_IFILETIME(timeout > 0? timeout : 0);
    BOOL ok;

    DbgFcnInEx(__FLND__, _T("name:%s, finalState:%s"), srv->name, Win32_ServiceStateString(finalState));

    lastError = Win32_WaitPendingService(srv, end, &state);
    if (lastError)
        RETURN_INT(lastError);
    
    if (state == finalState)
        RETURN(ERROR_SUCCESS);

    /* start or stop the service */
    if (finalState == SERVICE_STOPPED)
    {
        SERVICE_STATUS status;
        ok = ControlService(srv->handle, SERVICE_CONTROL_STOP, &status);
        if (!ok)
        {
            /* not logging to debug.log as ERROR_DEPENDENT_SERVICES_RUNNING is common */
            OSLOGW(ControlService, _T("name:%s, SERVICE_CONTROL_STOP"), srv->name);
            RETURN_UINT(GetLastError());
        }
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] name:%s, state:%s"), __FCN__, srv->name, Win32_ServiceStateString(status.dwCurrentState));
        /* ... but the service can still run, just the call was successful */
    } 
    else 
    {
        DWORD dwCount = serviceArgs? serviceArgs->argc : 0;
        ok = StartService(srv->handle, dwCount, (dwCount ? serviceArgs->argv : NULL));
        if (!ok)
        {
            OSLOG(StartService, _T("name:%s"), srv->name);
            RETURN_UINT(GetLastError());
        }
    }

    lastError = Win32_WaitPendingService(srv, end, &state);
    if (lastError)
        RETURN_INT(lastError);
    
    if (state == finalState)
        RETURN(ERROR_SUCCESS);
    
    /* service accepted request, but didn't reach the final state */
    DbgPlain(DBG_MAIN_ACTION,_T("[%s] name:%s, state:%s"), __FCN__, srv->name, Win32_ServiceStateString(state));
    RETURN(ERROR_SERVICE_REQUEST_TIMEOUT);
}


DWORD 
Win32_ServiceStop(
    _In_  WIN32_SERVICE *srv, 
    _In_ int waitTimeOutSeconds,
    _In_opt_ void (*callback)(WIN32_SERVICE_CALLBACK_DATA*))
{
    WIN32_SERVICE_CALLBACK_DATA callbackData;
    DWORD result;
    if (callback)
    {
        callbackData.serviceAction = SERVICE_STOPPED;
        callbackData.serviceName = srv->name;
        callbackData.displayName = srv->displayName;
        callbackData.userData = NULL;
        callbackData.serviceFinished = FALSE;
        callbackData.serviceResult = ERROR_SUCCESS;
        callback (&callbackData);
    }
    result = Win32_ControlService(srv, waitTimeOutSeconds, NULL, SERVICE_STOPPED);
    if (callback)
    {
        callbackData.serviceFinished = TRUE;
        callbackData.serviceResult = result;
        callback (&callbackData);
    }
    return result;
}


DWORD 
Win32_ServiceStart(
    _In_     WIN32_SERVICE *srv,
    _In_     int timeoutSeconds,
    _In_opt_ const argvRec *serviceArgs,
    _In_opt_ void (*callback)(WIN32_SERVICE_CALLBACK_DATA*))
{
    WIN32_SERVICE_CALLBACK_DATA callbackData;
    DWORD result;
    if (callback)
    {
        callbackData.serviceAction = SERVICE_RUNNING;
        callbackData.serviceName = srv->name;
        callbackData.displayName = srv->displayName;
        callbackData.userData = NULL;
        callbackData.serviceFinished = FALSE;
        callbackData.serviceResult = ERROR_SUCCESS;
        callback (&callbackData);
    }
    result = Win32_ControlService(srv, timeoutSeconds, serviceArgs, SERVICE_RUNNING);
    if (callback)
    {
        callbackData.serviceFinished = TRUE;
        callbackData.serviceResult = result;
        callback (&callbackData);
    }
    return result;
}


DWORD
Win32_EnumDependentServices(_In_ WIN32_SERVICE *srv, _In_ DWORD withState, _Out_ array_t *deps)
{
    ERH_FUNCTION(_T("Win32_EnumDependentServices"));
    ENUM_SERVICE_STATUS *buf = NULL;
    DWORD     i, bufsz = 0, count = 0, need;
    BOOL      ok;

    DbgFcnIn();

    *deps = array_new(ARR_TYPE_CONTIG, sizeof(ENUM_SERVICE_STATUS), NULL);

    while (ok = EnumDependentServices(srv->handle, withState, buf, bufsz, &need, &count), !ok && 
           GetLastError() == ERROR_MORE_DATA)
    {
        buf = REALLOC(buf, bufsz = need);
    }

    if (!ok)
    {
        OSLOG(EnumDependentServices, _T("name:%s"), srv->name);
        RETURN_UINT(GetLastError());
    }

    deps->count = count;
    deps->ndata = bufsz;
    deps->data  = buf;

    for (i = 0; i < count; ++i)
        DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] state:%s, serviceName:%s, displayName:%s"), __FCN__, 
            Win32_ServiceStateString(buf[i].ServiceStatus.dwCurrentState), 
            buf[i].lpServiceName,
            buf[i].lpDisplayName);

    RETURN(ERROR_SUCCESS);
}


DWORD
Win32_ServiceStopWithDependents(
    _In_  WIN32_SERVICE *srv,
    _In_  int timeoutSeconds,
    _Out_ array_t *deps, /* of ENUM_SERVICE_STATUS */
    _In_opt_ void (*callback)(WIN32_SERVICE_CALLBACK_DATA*))
{
    ERH_FUNCTION(_T("Win32_ServiceStopWithDependents"));
    DWORD lastError;
    int   i;
    const ENUM_SERVICE_STATUS *st;

    DbgFcnIn();

    /* let's stop all dependent services */
    lastError = Win32_EnumDependentServices(srv, SERVICE_ACTIVE, deps);
    if (lastError)
        RETURN_UINT(lastError);

    /* the first service in array is the first to be stopped and the last to be started */
    array_iterate(deps, i, st)
    {
        WIN32_SERVICE s;
        lastError = Win32_ServiceOpen(&s, srv->scm, st->lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (lastError != 0)
        {
            array_free(deps);
            RETURN_UINT(lastError);
        }
        s.userData = srv->userData;
        s.displayName = StrNullCopy(st->lpDisplayName);
        lastError = Win32_ServiceStop(&s, timeoutSeconds, callback);
        Win32_ServiceClose(&s);

        if (lastError != ERROR_SUCCESS)
        {
            array_free(deps);
            RETURN_UINT(lastError);
        }
    }

    lastError = Win32_ServiceStop(srv, timeoutSeconds, callback);

    /* leave the dependent-service data allocated another function can then start these services again */
    RETURN_UINT (lastError);
}


DWORD
Win32_ServiceStartWithDependents(
    _In_     WIN32_SERVICE *srv,
    _In_     int timeoutSeconds,
    _In_opt_ const argvRec *primaryArgs,
    _In_opt_ const array_t *deps, /* of ENUM_SERVICE_STATUS */
    _In_opt_ void (*callback)(WIN32_SERVICE_CALLBACK_DATA*))
{
    ERH_FUNCTION(_T("Win32_ServiceStartWithDependents"));
    DWORD lastError;
    int i;
    const ENUM_SERVICE_STATUS *st;

    DbgFcnIn();

    lastError = Win32_ServiceStart(srv, timeoutSeconds, primaryArgs, callback);
    if (lastError)
        RETURN_UINT(lastError);

    /* the first service in array is the first to be stopped and the last to be started */
    array_riterate(deps, i, st)
    {
        WIN32_SERVICE s;
        // Open the service
        lastError = Win32_ServiceOpen(&s, srv->scm, st->lpServiceName, SERVICE_START | SERVICE_QUERY_STATUS);
        if (lastError != 0)
        {
            RETURN_UINT (GetLastError());
        }
        s.userData = srv->userData;
        s.displayName = StrNullCopy(st->lpDisplayName);
        lastError = Win32_ServiceStart(&s, timeoutSeconds, NULL, callback);
        Win32_ServiceClose(&s);

        if (lastError != ERROR_SUCCESS)
        {
            RETURN_UINT (lastError);
        }
    }

    RETURN_INT(lastError);
}


DWORD 
Win32_ServiceSetRestart(
    _In_ SC_HANDLE hManager,
    _In_ int serviceSetLength,
    _In_ ctchar *serviceNameSet[], 
    _In_ ctchar *displayNameSet[],
    _In_ int timeoutSeconds,
    _In_opt_ void *userData,
    _In_opt_ void (*callback)(WIN32_SERVICE_CALLBACK_DATA*))
{
    ERH_FUNCTION(_T("Win32_ServiceSetRestart"));
    DWORD lastError = ERROR_SUCCESS;
    WIN32_SERVICE *primarySet = NULL;
    array_t *dependents=NULL;
    int i;
    
    DbgFcnIn();
    primarySet = CALLOC(serviceSetLength, sizeof(WIN32_SERVICE));
    dependents = CALLOC(serviceSetLength, sizeof(array_t));
    for (i=0; i<serviceSetLength; i++)
    {
        if (Win32_ServiceOpen(&primarySet[i], hManager, serviceNameSet[i], SERVICE_ALL_ACCESS) != 0)
        {
            goto exit;
        }
        primarySet[i].userData = userData;
        primarySet[i].displayName = StrNullCopy(displayNameSet[i]);
        lastError = Win32_ServiceStopWithDependents(&primarySet[i],timeoutSeconds,&dependents[i], callback);
        if (lastError != ERROR_SUCCESS)
        {
            goto exit;
        }
    }
    for (i=serviceSetLength-1; i>=0; i--)
    {
        lastError = Win32_ServiceStartWithDependents(&primarySet[i],timeoutSeconds,NULL, &dependents[i], callback);
        if (lastError != ERROR_SUCCESS)
        {
            goto exit;
        }
    }

exit:
    for (i=0; i<serviceSetLength; i++)
    {
        array_free(&dependents[i]);
        Win32_ServiceClose(&primarySet[i]);
    }
    FREE(dependents);
    RETURN_UINT (lastError);
}


DWORD
Win32_ServiceDelete(_In_ WIN32_SERVICE *srv)
{
    ERH_FUNCTION(_T("Win32_ServiceDelete"));
    if (!DeleteService(srv->handle))
    {
        OSLOGW(DeleteService, _T("name:%s"), srv->name);
        return GetLastError();
    }
    return 0;
}

DWORD 
Win32_ServiceDeleteByName(_In_ SC_HANDLE scm, _In_ ctchar *serviceName)
{
    WIN32_SERVICE srv = {0};
    DWORD lastError = Win32_ServiceOpen(&srv, scm, serviceName, SERVICE_ALL_ACCESS);
    if (lastError == ERROR_SUCCESS)
        lastError = Win32_ServiceStop(&srv, 0, NULL);

    if (lastError == ERROR_SUCCESS)
        lastError = Win32_ServiceDelete(&srv);

    Win32_ServiceClose(&srv);
    return lastError;
}


/* ---------------------------------------------------------------------------
*
*   @brief Encrypts the input string and returns base64 encoded string of encrypted 
*   data using Windows "Data Protection" APIs
*   @param[in]  tchar*  plainData      - Input string for encryption
*   @param[out] tchar** encryptedData  - Pointer to Address of malloced base64 encoded 
*                                        string of encrypted data
*   @return int length of encrypted data on success, a negative value on failure
*
 -------------------------------------------------------------------------- */
int CmnEncryptTStrToBase64(const tchar *plainData, tchar **encryptedString)
{
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    BYTE *encryptedData    = NULL;
    DWORD nDestinationSize = -1;

    if (!plainData)
        return -1;

    DataIn.pbData = (BYTE *)plainData;

    DataIn.cbData = (DWORD)STRLEN_TO_BYTES(strlen(plainData));

    if (CryptProtectData(
        &DataIn,
        NULL,                               // A description string.
        NULL,                               // Optional entropy
        NULL,                               // Reserved.
        NULL,                               // Pass a PromptStruct.
        0,                                  //user-based encryption.
                                            //Only the user who ran the encryption will be able to decrypt
        &DataOut))
    {

        encryptedData = (BYTE *)MALLOC(DataOut.cbData);
        if (!encryptedData)
            return -2;

        memcpy(encryptedData, DataOut.pbData, (size_t)DataOut.cbData);

        if (CryptBinaryToString(encryptedData, DataOut.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &nDestinationSize))
        {
            //Allocate output array
            *encryptedString = (TCHAR *)MALLOC( nDestinationSize * sizeof(TCHAR));
            if (*encryptedString)
            {
                //Do base64 encoding using Windows crypt API
                if (!CryptBinaryToString(encryptedData, DataOut.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, *encryptedString, &nDestinationSize))
                    nDestinationSize = -1;
            }
            else
                nDestinationSize = -1;
        }
        else
            nDestinationSize = -1;

        FREE(encryptedData);

        LocalFree(DataOut.pbData);
        return nDestinationSize;
    }
    else
    {
        return -3;
    }
}


/* ---------------------------------------------------------------------------
*
*   @brief Encrypts char array to a byte array followed by base 64 encoding using Windows "Data Protection" APIs
*
*   P.S. - This is method has been exclusively written for Java compatibility and should only be used elsewhere after
*   thorough testing. This method is not Unicode compliant and only works on single byte char arrays returning single
*   byte base 64 encoded char arrays.
*
*   @param[in] char* plainData Input string for encryption
*   @param[out] char** pszDest Pointer to address of heap allocated base 64 encoded data
*   @return int length of base 64 encoded data on success, a negative value on failure
*
 -------------------------------------------------------------------------- */

int CmnEncryptStrToBase64(const char *plainData, char **pszDest)
{
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DWORD nDestinationSize;
	BYTE *encryptedData = NULL;

	if (!plainData)
		return -1;

	DataIn.pbData = (BYTE*)plainData;
	DataIn.cbData = (DWORD)strlenA(plainData);

	if (CryptProtectData(
		&DataIn,
		NULL,                               // A description string.
		NULL,                               // Optional entropy
		NULL,                               // Reserved.
		NULL,                               // Pass a PromptStruct.
		0,                                  //user-based encryption.
											//Only the user who ran the encryption will be able to decrypt
		&DataOut))
	{
		//Allocate output array
		encryptedData = MALLOC(DataOut.cbData);
		if (!encryptedData)
			return -2;

		//copy to output array
		memcpy(encryptedData, DataOut.pbData, DataOut.cbData);
		
		if (CryptBinaryToStringA(encryptedData, DataOut.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &nDestinationSize))
		{
			*pszDest = (char *)HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, nDestinationSize * sizeof(CHAR));
			if (*pszDest)
			{
				if (CryptBinaryToStringA(encryptedData, DataOut.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, *pszDest, &nDestinationSize))
				{
					//Encryption successful
				}
			}
			else
			{
				nDestinationSize = 0;
			}
		}
		else
		{
			nDestinationSize = 0;
		}

		if (encryptedData)
			FREE(encryptedData);

		LocalFree(DataOut.pbData);
		return nDestinationSize;
	}
	else
	{
		return -3;
	}
}


 /* ---------------------------------------------------------------------------
 *
 *   @brief Decrypts the input string using Windows "Data Protection" APIs
 *   @param[in]  tchar*  plainData      - Base64 encoded string of encrypted data
 *   @param[out] tchar** encryptedData  - Pointer to Address of decrypted string
 *   @return int length of decrypted data on success, a negative value on failure
 *
  -------------------------------------------------------------------------- */
int CmnDecryptBase64ToTStr(const tchar *encodedData, tchar **decryptedData)
{
    DATA_BLOB encryptedDataBlob;
    DATA_BLOB decryptedDataBlob;
    BYTE *pbDataInput   = NULL;
    BYTE *encryptedData = NULL;
    DWORD nDestinationSize = 0;

    if (!encodedData)
        return -1;

    if (CryptStringToBinary(
        encodedData,
        StrLen(encodedData),
        CRYPT_STRING_BASE64_ANY,
        NULL,
        &nDestinationSize,
        NULL,
        NULL))
    {

        encryptedData = (BYTE *)MALLOC(nDestinationSize);
        if (encryptedData)
        {
            CryptStringToBinary(
                encodedData,
                StrLen(encodedData),
                CRYPT_STRING_BASE64_ANY,
                encryptedData,
                &nDestinationSize,
                NULL,
                NULL);
        }
    }

    encryptedDataBlob.cbData = nDestinationSize;
    encryptedDataBlob.pbData = encryptedData;
    
    if (CryptUnprotectData(
        &encryptedDataBlob,
        NULL,
        NULL,                 // Optional entropy
        NULL,                 // Reserved
        NULL,                 // Optional PromptStruct
        0,                    //user-based encryption.
                              //Only the user who ran the encryption will be able to decrypt
        &decryptedDataBlob))
    {
        //Allocate output array
        *decryptedData = (TCHAR *)MALLOC( decryptedDataBlob.cbData * sizeof(TCHAR));
        memcpy(*decryptedData, decryptedDataBlob.pbData, decryptedDataBlob.cbData);
        LocalFree(decryptedDataBlob.pbData);

        FREE(encryptedData);
        return (decryptedDataBlob.cbData);
    }
    else
    {
        FREE(encryptedData);
        return -3;
    }
}
