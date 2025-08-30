/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   sysnfo
* @file      recovery/srd/SysNfo/sysnfocmn.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     /
*
* @since     dd.mm.yy    lukaj   Original Coding
*
* @remarks   /
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /sysnfocmn.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|   include files
 ---------------------------------------------------------------------------*/
#if TARGET_LINUX
/** \include fcntl.h
    \brief Manupulate file descriptor
*/
#include <fcntl.h>

/** \include sys/param.h
    \brief For MAXHOSTNAMELEN
*/
#include <sys/param.h>

/** \include sys/stat.h
    \brief Get file/directory status
*/
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/** \include SysNfoLNX/xmlparser.h
    \brief XML parser - For parse RedHat Cluster Suite configuration files
*/
#include "SysNfoLNX/ezxml.h"
#else
    #include <clusapi.h>
    #include <lm.h>
#endif

#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "lib/ipc/ipwrap.h"
#include "lib/parse/vec.h"
#include "lib/parse/idbconfig.h"
#include "sysnfocmn.h"
#include "../srd.h"
#include "../srdfree.h"

/* ---------------------------------------------------------------------------
|   defines
 ---------------------------------------------------------------------------*/
#define MAX_DB_PATHS        64
#define REGPATH_OB2COMMON   REGCOMMON
#define REGVAL_CLUSENABLED  _T("ClusteringEnabled")
#define REGVAL_OMNICFGDB    _T("OmniConfigDB")
#define REGVAL_OMNICFGUNC   _T("OmniConfigUNC")
#define CLUSAPI_DLL_NAME    _T("clusapi.dll")
#define UNREF_CONV          {(void)(_converted);(void)(_convert);}
#define RSM_SERVICE_NAME    _T("ntmssvc")
static int clus_Type = 0;
#define VOLUME_GUID_LEN 49 /* parasoft-suppress  CODSTA-37 "C code define suppression" */
/* ---------------------------------------------------------------------------
|   for IA64 plaftorm
 ---------------------------------------------------------------------------*/
#ifndef CLUS_RESTYPE_NAME_NETNAME
#define CLUS_RESTYPE_NAME_NETNAME   L"Network Name"
#endif
#ifndef CLUS_RESTYPE_NAME_HARDDISK
#define CLUS_RESTYPE_NAME_HARDDISK  L"Physical Disk"
#endif

#if TARGET_LINUX
/** \def SLESRELEASE
    \brief Path to SLES release file.
*/
#define SLESRELEASE  (OB2_StatFile(_T("/etc/SuSE-release"), NULL) == 0) ?    \
                                            _T("/etc/SuSE-release") :       \
                    (OB2_StatFile(_T("/etc/SUSE-release"), NULL) == 0) ?    \
                                            _T("/etc/SUSE-release") :       \
                    (OB2_StatFile(_T("/etc/suse-release"), NULL) == 0) ?    \
                                            _T("/etc/suse-release") :       \
                    _T("/etc/SuSE-release")

/** \def RHELRELEASE
    \brief Path to RHEL release file.
*/
#define RHELRELEASE  "/etc/redhat-release"

/** \def PROC_MOUNTS
    \brief Path to mounts file.
*/
#define PROC_MOUNTS  "/proc/mounts"

/** \def MAX_OB_PATH
    \brief Number of default DP paths on linux
*/
#define MAX_OB_PATH 3

/** \def MAX_OB_DATA_PATH
    \brief Number of default Data DP paths on linux
*/
#define MAX_OB_DATA_PATH 2

/** \def MAX_POSIBILE_OB_PATH
    \brief Number of posible HP DP paths on linux
*/
#define MAX_POSIBILE_OB_PATH 10

/** \def MAX_POSIBILE_OB_DATA_PATH
    \brief Number of posible HP Data DP paths on linux
*/
#define MAX_POSIBILE_OB_DATA_PATH 8

/** \def SERVICE_FILE
    \brief Path to services file
*/
#define SERVICE_FILE        _T("/etc/services")

/** \def CELL_SERVER_FILE
    \brief Path to cell server/manager file
*/
#define CELL_SERVER_FILE    _T("/etc/opt/omni/client/cell_server")

/** \def DBLOC_FILE_PATH
    \brief Path to HP DP database file
*/
#define DBLOC_FILE_PATH     DBLOC_DB40 _T("db_files")

/** \def DBLOC_DB40
    \brief This variable is not nesessery for linux
*/
#define DBLOC_DB40          _T("")

/** \def RPM_SUSE
    \brief This is a stupid solution, but in this moment we do not have any other solution
*/
#define RPM_SUSE     _T("rpm -qa --qf '%{RPMTAG_DIRNAMES}:%{VENDOR}\\n' |               \
                    grep -i 'SUSE' |                                                       \
                    cut -d: -f1 |                                                       \
                    uniq |                                                              \
                    sort -u |                                                           \
                    while read D;                                                       \
                    do                                                                  \
                        for DD in `/bin/df -P -t ext4 -t gfs2 -t psfs -t ext2 -t SFS -t ext -t minix -t xiafs -t vmfs -t reiserfs -t ext3 -t vfat -t vxfs -t xfs -t hsmfs -t gfs -t lustre_lite -t ocfs -t ocfs2 -t btrfs 2>/dev/null | sed '1d' | awk '{print $6}'`; \
                        do                                                              \
                            if [ \"$D\" == \"$DD/\" ] || [ \"$D\" == \"$DD\" ] || [ \"$D\" == \"$DD/bin/\" ]; then \
                                echo \"$DD\";                                           \
                            fi                                                          \
                        done;                                                           \
                    done")

/** \def RPM_RHEL
    \brief This is a stupid solution, but in this moment we do not have any other solution
*/
#define RPM_RHEL    _T("rpm -qa --qf '%{RPMTAG_DIRNAMES}:%{VENDOR}\\n' |                \
                    grep -i 'Red Hat' |                                                    \
                    cut -d: -f1 |                                                       \
                    uniq |                                                              \
                    sort -u |                                                           \
                    while read D;                                                       \
                    do                                                                  \
                        for DD in `/bin/df -P -t ext4 -t gfs2 -t psfs -t ext2 -t SFS -t ext -t minix -t xiafs -t vmfs -t reiserfs -t ext3 -t vfat -t vxfs -t xfs -t hsmfs -t gfs -t lustre_lite -t ocfs -t ocfs2 2>/dev/null | sed '1d' | awk '{print $6}'`; \
                        do                                                              \
                            if [ \"$D\" == \"$DD/\" ] || [ \"$D\" == \"$DD\" ] || [ \"$D\" == \"$DD/bin/\" ]; then \
                                echo \"$DD\";                                           \
                            fi                                                          \
                        done;                                                           \
                    done")

/** \def GET_PACKAGES_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get package(s) of ServiceGuard cluster on 11.16 and 11.18
*/
#define GET_PACKAGES_SLES   _T("/opt/cmcluster/bin/cmviewcl -l package |grep -v PACKAGE |sed 's/ (/(/g'")

/** \def GET_NODES_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get nodes wich are member of cluster on ServiceGuard 11.16 and 11.18
*/
#define GET_NODES_SLES      _T("/opt/cmcluster/bin/cmviewcl -l node |grep -v NODE")

/** \def GET_QS_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get hostname of Quorum Server on ServiceGuard
*/
#define GET_QS_SLES         _T("/opt/cmcluster/bin/cmgetconf |grep -i -e '^QS_HOST'")

/** \def DUMP_CONF_18_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get path of run script for package on ServiceGuard 11.16
*/
#define DUMP_CONF_18_SLES   _T("/opt/cmcluster/bin/cmgetconf -p %s |grep -i -e '^RUN_SCRIPT' |grep /")

/** \def GET_LOG_VOLS_16_18_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for share volume(s) on ServiceGuard 11.18
*/
#define GET_LOG_VOLS_16_18_SLES _T("/opt/cmcluster/bin/cmgetconf -p %s |grep -i -e '^fs_name' -e '^fs_directory' -e '^fs_type' -e '^fs_mount_opt' -e '^fs_umount_opt' -e '^fs_fsck_opt'")

/** \def GET_IP_ADDRESS_16_18_SLES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for ip address on ServiceGuard 11.18
*/
#define GET_IP_ADDRESS_16_18_SLES _T("/opt/cmcluster/bin/cmgetconf -p %s |grep -i -e '^ip_address'")

/** \def GET_PACKAGES_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get package(s) of ServiceGuard cluster on 11.16 and 11.18
*/
#define GET_PACKAGES_RHEL   _T("/usr/local/cmcluster/bin/cmviewcl -l package |grep -v PACKAGE |sed 's/ (/(/g'")

/** \def GET_NODES_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get nodes wich are member of cluster on ServiceGuard 11.16 and 11.18
*/
#define GET_NODES_RHEL      _T("/usr/local/cmcluster/bin/cmviewcl -l node |grep -v NODE")

/** \def GET_QS_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get hostname of Quorum Server on ServiceGuard
*/
#define GET_QS_RHEL         _T("/usr/local/cmcluster/bin/cmgetconf |grep -i -e '^QS_HOST'")

/** \def DUMP_CONF_18_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get path of run script for package on ServiceGuard 11.16
*/
#define DUMP_CONF_18_RHEL   _T("/usr/local/cmcluster/bin/cmgetconf -p %s |grep -i -e '^RUN_SCRIPT' |grep /")

/** \def GET_LOG_VOLS_16_18_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for share volume(s) on ServiceGuard 11.18
*/
#define GET_LOG_VOLS_16_18_RHEL _T("/usr/local/cmcluster/bin/cmgetconf -p %s |grep -i -e '^fs_name' -e '^fs_directory' -e '^fs_type' -e '^fs_mount_opt' -e '^fs_umount_opt' -e '^fs_fsck_opt'")

/** \def GET_IP_ADDRESS_16_18_RHEL
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for ip address on ServiceGuard 11.18
*/
#define GET_IP_ADDRESS_16_18_RHEL _T("/usr/local/cmcluster/bin/cmgetconf -p %s |grep -i -e '^ip_address'")

/** \def GET_NODES_HAV1
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get nodes wich are member of cluster on Heartbeat v1
*/
#define GET_NODES_HAV1      _T("cat /etc/ha.d/ha.cf |grep -e  '^node'")

/** \def GET_NODES_HAV1
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get nodes wich are member of cluster on Heartbeat v1
*/
#define GET_RESOURCES       _T("cat /etc/ha.d/haresources")

/** \def PARSE_CONF_18
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for share volume(s) on ServiceGuard 11.16
*/
#define PARSE_CONF_18       _T("cat %s |grep -i -e \"^LV\\[\" -e \"^FS\\[\" -e \"^FS_TYPE\\[\" -e \"^FS_MOUNT_OPT\\[\" -e \"^FS_UMOUNT_OPT\\[\" -e \"^FS_FSCK_OPT\\[\" | sed 's/;/\\n/g' |sed 's/^ //g'")

/** \def PARSE_CONF_FOR_ADDRESS_18
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Parse package.config file for ip addresses of package on ServiceGuard 11.16
*/
#define PARSE_CONF_FOR_ADDRESS_18   _T("cat %s |grep -i -e \"^IP\\[\"")

/** \def GET_NET_NAME
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get virtual hostname of package for DP
*/
#define GET_NET_NAME        _T("cat /etc/opt/omni/server/sg/sg.conf |grep -i -e '^CS_SERVICE_HOSTNAME' |cut -d'=' -f2")

/** \def GET_VERSION_V1
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get version of ServiceGuard
*/
#define GET_VERSION_V1      _T("rpm -qf /usr/bin/cl_status")

/** \def GET_VERSION_V2
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get version of SLES heartbeat v2
*/
#define GET_VERSION_V2      _T("crmadmin --version | sed 's/, /\\n/g' | sed 's/(/\\n/g' | sed 's/)//g'")

/** \def GET_VERSION
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get version of RHEL Cluster Suite
*/
#define GET_VERSION         _T("/sbin/cman_tool -V |grep cman_tool |cut -d' ' -f2")

/** \def GET_SERVICES
    \brief This is a stupid solution, but in this moment we do not have any
            other solution.
            Get services on cluster - RHEL Cluster Suite
*/
#define GET_SERVICES        _T("/usr/sbin/clustat |grep service")

/** \def SUPPORTED_FS_TYPE
    \brief Macro for supported file systems
*/
#define SUPPORTED_FS_TYPE(Fs_type)             \
    (strcmp (Fs_type, "ext2") == 0              \
        || strcmp (Fs_type, "ext3") == 0        \
        || strcmp (Fs_type, "reiserfs") == 0    \
        || strcmp (Fs_type, "vfat") == 0        \
        || strcmp (Fs_type, "vxfs") == 0        \
        || strcmp (Fs_type, "xfs") == 0         \
        || strcmp (Fs_type, "gfs") == 0         \
        || strcmp (Fs_type, "gfs2") == 0        \
        || strcmp (Fs_type, "minix") == 0       \
        || strcmp (Fs_type, "ocfs") == 0        \
        || strcmp (Fs_type, "ocfs2") == 0)
#else
    #define DBLOC_FILE_PATH     DBLOC_DB40 _T("\\datafiles\\catalog\\db_files")
    #define DBLOC_DB40          _T("\\db40")

    static R_RESULT GetRSMServiceInfo(R_ULONG*);
#endif

/** \fn GetOmniBackVolume(R_ULONG *pLetter,
                            R_TCHAR *pszDir,
                            R_ULONG *pData,
                            R_TCHAR *pszDataDir)
    \brief Retreives the location of the OmniBack installation directory, its
            volume's drive letter/mount point or both.
    \param pLetter drive letter
    \param pszDir DP Volumes
    \param pData drive letter
    \param pszDataDir DP Data Volumes
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
#if TARGET_WIN32
R_RESULT
GetOmniBackVolume(R_ULONG *pLetter,
                    R_TCHAR *pszDir,
                    R_ULONG *pData,
                    R_TCHAR *pszDataDir)
#else
R_RESULT
GetOmniBackVolume(R_ULONG *pLetter,
                    R_TCHAR *pszDir,
                    R_ULONG *pData,
                    R_TCHAR *pszDataDir,
                    PCLUSINFO pClusInfo)
#endif
{
    ERH_FUNCTION (_T("GetOmniBackVolume"));
    R_RESULT Return = SRDERR_ERROR;

#if TARGET_LINUX
    R_INT   ii = 0,
            jj = 0,
            kk = 0,
            IsExists = 0;
    R_TCHAR ob_paths[] = _T("/opt/omni/"),
            ob_etc_paths[] = _T("/etc/opt/omni/server/"),
            ob_var_paths[] = _T("/var/opt/omni/server/"),
            *ptr = NULL,
            *ptr1 = NULL,
            SymlinkBuffer[STRLEN_STD] = { _T('\0') },
            TmpVar[STRLEN_STD] = { _T('\0') },
            TmpVar1[STRLEN_STD] = { _T('\0') };


    /**
     * split /opt/omni/ path to three path /, /opt and /opt/omni for detect
     * mount point or symbolic link for cluster
     */
    while ((ptr = strrchr(ob_paths, '/')) != NULL)
    {
        *ptr = _T('\0');

        if (IsSymLink(ob_paths, SymlinkBuffer))
        {
            strcpy(TmpVar, SymlinkBuffer);
            strcat(TmpVar, GEN_ROOT);
        }
        else
        {
            strcpy(TmpVar, ob_paths);
            strcat(TmpVar, GEN_ROOT);
        }

        while ((ptr1 = strrchr(TmpVar, '/')) != NULL)
        {
            *ptr1 = _T('\0');

            if (IsMountPoint(TmpVar))
            {
                if (TmpVar[0] != _T('\0'))
                {
                    if (ii == 0)
                        strcpy(pszDir, TmpVar);
                    else
                    {
                        strcat(pszDir, _T(","));
                        strcat(pszDir, TmpVar);
                    }
                }
                ii++;
            }
            else if (pClusInfo != NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("TmpVar: %s"), TmpVar);
                DbgPlain(DBG_SYSNFO, _T("pClusInfo->ConfigDBPath: %s"), pClusInfo->ConfigDBPath);
                if (StrCmp(TmpVar, pClusInfo->ConfigDBPath) == 0)
                {
                    if (TmpVar[0] != _T('\0'))
                    {
                        if (ii == 0)
                            strcpy(pszDir, TmpVar);
                        else
                        {
                            strcat(pszDir, _T(","));
                            strcat(pszDir, TmpVar);
                        }
                    }
                    ii++;
                }
            }
        }
    }
    if (!pszDir || pszDir[0] == _T('\0'))
        strcpy(pszDir, GEN_ROOT);

    /**
     * split /etc/opt/omni/server/ path to file path /, /etc, /etc/opt,
     * /etc/opt/omni and /etc/opt/omni/server for detect
     * mount point or symbolic link for cluster
     */
    while ((ptr = strrchr(ob_etc_paths, '/')) != NULL)
    {
        *ptr = _T('\0');

        if (IsSymLink(ob_etc_paths, SymlinkBuffer))
        {
            strcpy(TmpVar, SymlinkBuffer);
            strcat(TmpVar, GEN_ROOT);
        }
        else
        {
            strcpy(TmpVar, ob_etc_paths);
            strcat(TmpVar, GEN_ROOT);
        }

        while ((ptr1 = strrchr(TmpVar, '/')) != NULL)
        {
            *ptr1 = _T('\0');

            if (IsMountPoint(TmpVar))
            {
                if (TmpVar[0] != _T('\0'))
                {
                    if (jj == 0)
                        strcpy(TmpVar1, TmpVar);
                    else
                    {
                        strcat(TmpVar1, _T(","));
                        strcat(TmpVar1, TmpVar);
                    }
                }
                jj++;
            }
            else if (pClusInfo != NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("TmpVar: %s"), TmpVar);
                DbgPlain(DBG_SYSNFO, _T("pClusInfo->ConfigDBPath: %s"), pClusInfo->ConfigDBPath);
                if (StrCmp(TmpVar, pClusInfo->ConfigDBPath) == 0)
                {
                    if (TmpVar[0] != _T('\0'))
                    {
                        if (jj == 0)
                            strcpy(TmpVar1, TmpVar);
                        else
                        {
                            strcat(TmpVar1, _T(","));
                            strcat(TmpVar1, TmpVar);
                        }
                    }
                    jj++;
                }
            }
        }
    }

    /**
     * split /var/opt/omni/server/ path to file path /, /var, /var/opt,
     * /var/opt/omni and /var/opt/omni/server for detect
     * mount point or symbolic link for cluster
     */
    while ((ptr = strrchr(ob_var_paths, '/')) != NULL)
    {
        *ptr = _T('\0');

        if (IsSymLink(ob_var_paths, SymlinkBuffer))
        {
            strcpy(TmpVar, SymlinkBuffer);
            strcat(TmpVar, GEN_ROOT);
        }
        else
        {
            strcpy(TmpVar, ob_var_paths);
            strcat(TmpVar, GEN_ROOT);
        }

        while ((ptr1 = strrchr(TmpVar, '/')) != NULL)
        {
            *ptr1 = _T('\0');

            if (IsMountPoint(TmpVar))
            {
                if (TmpVar[0] != _T('\0'))
                {
                    if (kk == 0)
                        strcpy(pszDataDir, TmpVar);
                    else
                    {
                        strcat(pszDataDir, _T(","));
                        strcat(pszDataDir, TmpVar);
                    }
                }
                kk++;
            }
            else if (pClusInfo != NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("TmpVar: %s"), TmpVar);
                DbgPlain(DBG_SYSNFO, _T("pClusInfo->ConfigDBPath: %s"), pClusInfo->ConfigDBPath);
                if (StrCmp(TmpVar, pClusInfo->ConfigDBPath) == 0)
                {
                    if (TmpVar[0] != _T('\0'))
                    {
                        if (kk == 0)
                            strcpy(pszDataDir, TmpVar);
                        else
                        {
                            strcat(pszDataDir, _T(","));
                            strcat(pszDataDir, TmpVar);
                        }
                    }
                    kk++;
                }
            }
        }
    }

    if (pszDataDir && pszDataDir[0] != _T('\0'))
    {
        if (strstr(pszDataDir, _T(",")))
        {
            while ((ptr = strrchr(pszDataDir, ',')) != NULL)
            {
                *ptr = _T('\0');
                if (strcmp(pszDataDir, TmpVar1) == 0)
                    IsExists = 1;
            }
        }
        else
        {
            if (strcmp(pszDataDir, TmpVar1) == 0)
                IsExists = 1;
        }

        if (!IsExists)
        {
            strcat(pszDataDir, _T(","));
            strcat(pszDataDir, TmpVar1);
        }
    }

    if (!pszDataDir || pszDataDir[0] == _T('\0'))
        strcpy(pszDataDir, GEN_ROOT);

    Return = SRDERR_SUCCESS;
#else
    HKEY hOmni = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_OB2COMMON,
        0, KEY_READ, &hOmni) == ERROR_SUCCESS)
    {
        R_ULONG ulSize = MAX_PATH * sizeof(R_TCHAR);
        R_TCHAR pszHDir[MAX_PATH], pszDDir[MAX_PATH];

        if (RegQueryValueEx(hOmni, REGVAL_HOMEDIR, NULL, NULL,
            (R_UCHAR*)pszHDir, &ulSize) == ERROR_SUCCESS)
        {
            if (pLetter != NULL)
                *pLetter = (R_ULONG)pszHDir[0];
            if (pszDir != NULL)
                strcpy(pszDir, pszHDir);

            ulSize = MAX_PATH * sizeof(R_TCHAR);
            if (RegQueryValueEx(hOmni, REGVAL_DATADIR, NULL, NULL,
                (R_UCHAR*)pszDDir, &ulSize) == ERROR_SUCCESS)
            {
                if (pData != NULL)
                    *pData = (R_ULONG)pszDDir[0];
                if (pszDataDir != NULL)
                    strcpy(pszDataDir, pszDDir);

                Return = SRDERR_SUCCESS;
            }
        }

        RegCloseKey(hOmni);
    }
#endif

    return(Return);
}

/** \fn FreeOmniBackDatabaseLocations(R_TCHAR **ppDbLocs, ULONG ulCount)
    \brief Free memory of Database location path
    \param ppDbLocs Database location paths
    \param ulCount Number of database locations
    \return VOID
*/
R_VOID
FreeOmniBackDatabaseLocations(R_TCHAR **ppDbLocs, ULONG ulCount)
{
    if (ppDbLocs != NULL && ulCount > 0)
    {
        R_ULONG ii = 0;

        for(ii = 0; ii < ulCount; ii++)
            FREE(ppDbLocs[ii]);

        FREE(ppDbLocs);
    }
}

/** \fn SkipNewline(R_TCHAR *lpszStart, R_BOOL bFwd)
    \brief Skip new line character form the string
    \param lpszStart String with new line
    \param bFwd forward or backward
    \return
*/
static R_TCHAR*
SkipNewline(R_TCHAR *lpszStart, R_BOOL bFwd)
{
    R_TCHAR *lpszRet = lpszStart;

    while(
        *lpszRet != _T('\0') &&
        (*lpszRet == _T('\n') ||
        *lpszRet == _T('\r'))
    )
        lpszRet = (bFwd ? lpszRet + 1 : lpszRet - 1);
    return lpszRet;
}

static R_TCHAR*
SkipNewlineArr(R_TCHAR *start)
{
    start[strcspn(start, _T("\r\n"))] = 0;
    return start;
}


/** \fn GetLine(R_TCHAR* lpszBuffer, R_TCHAR* lpszOut)
    \brief Get line by line from file
    \param lpszBuffer
    \param lpszOut
    \return
*/
static R_TCHAR*
GetLine(R_TCHAR* lpszBuffer, R_TCHAR* lpszOut)
{
    R_TCHAR *lpszRet = NULL;
    R_TCHAR *lpszNext = NULL;

    lpszNext = strchr(lpszBuffer, _T('\n'));
    if (lpszNext != NULL)
    {
        lpszNext = SkipNewline(lpszNext, FALSE);
        lpszNext += 1;

        strncpy(lpszOut, lpszBuffer, lpszNext - lpszBuffer);
        lpszOut[lpszNext - lpszBuffer] = _T('\0');

        lpszRet = SkipNewline(lpszNext, TRUE);
    }
    else
    {
        strcpy(lpszOut, lpszBuffer);
        lpszRet = lpszBuffer + strlen(lpszOut);
    }

    return lpszRet;
}

/** \fn GetDbFilePath(R_TCHAR *pszDBFiles, R_TCHAR *pszConfigDB)
    \brief Get HP DP database file
    \param pszDBFiles
    \param pszConfigDB
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetDbFilePath(R_TCHAR *pszDBFiles, R_TCHAR *pszConfigDB)
{
    ERH_FUNCTION (_T("GetDbFilePath"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

#if TARGET_LINUX
    _TRY_
    {
#else
    __try
    {
#endif
        R_TCHAR pszDBPath[MAX_PATH] = { 0 };
        R_TCHAR pszUNCPath[MAX_PATH] = { 0 };
#if TARGET_WIN32
        HKEY    hCommon = NULL;
        R_DWORD dwSize = 0;

        if (NULL == pszDBFiles || NULL == pszConfigDB)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Invalid parameters."));}
            );

        __try
        {
            R_TCHAR *pszCfg = NULL;

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_OB2COMMON, 0, KEY_READ, &hCommon) != ERROR_SUCCESS)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("Cannot open OBCommon registry key."));}
                );
            dwSize =    MAX_PATH * sizeof(R_TCHAR);
            if (RegQueryValueEx(hCommon, REGVAL_OMNICFGDB, NULL, NULL, (R_UCHAR*)pszDBPath, &dwSize) != ERROR_SUCCESS)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("Cannot query %s registry value."), REGVAL_OMNICFGDB);}
                );
            dwSize =    MAX_PATH * sizeof(R_TCHAR);
            if (RegQueryValueEx(hCommon, REGVAL_OMNICFGUNC, NULL, NULL, (R_UCHAR*)pszUNCPath, &dwSize) != ERROR_SUCCESS)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("Cannot query %s registry value."), REGVAL_OMNICFGUNC);}
                );
            if ((pszCfg = strrchr(pszDBPath, _T('\\'))) == NULL)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("No backslash in %s path."), pszDBPath);}
                );
            *pszCfg = _T('\0');
            if ((pszCfg = strrchr(pszUNCPath, _T('\\'))) == NULL)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("No backslash in %s path."), pszUNCPath);}
                );
            *pszCfg = _T('\0');

            DbgPlain(DBG_SYSNFO, _T("UNC path: %s."), pszUNCPath);
            DbgPlain(DBG_SYSNFO, _T("DB  path: %s."), pszDBPath);

            Return = SRDERR_SUCCESS;
        }
#if TARGET_LINUX
        _FINALLY_
        {
#else
        __finally
        {
#endif
            if (NULL != hCommon)
                RegCloseKey(hCommon);
        }
#endif

        /**
         * cmnPanBase on linux return /opt/omni, but position on database is
         * /var/opt/omni/...
         * PANDBCATALOG and PANCONFIGDB are defined in /lib/cmn/unix_defs.h
         */
        if (Return != SRDERR_SUCCESS)
        {
#if TARGET_LINUX
            strcpy(pszUNCPath, PANDBCATALOG);
            strcpy(pszDBPath, PANCONFIGDB);
#else
            strcpy(pszUNCPath, cmnPanBase);
            strcpy(pszDBPath, cmnPanBase);
#endif
            Return = SRDERR_SUCCESS;
        }

        if (Return == SRDERR_SUCCESS)
        {
            PathConcat(pszDBFiles, pszUNCPath, DBLOC_FILE_PATH, MAX_PATH);
            PathConcat(pszConfigDB, pszDBPath, DBLOC_DB40, MAX_PATH);

            DbgPlain(DBG_SYSNFO, _T("db_files path: %s."), pszDBFiles);
            DbgPlain(DBG_SYSNFO, _T("DBConfig path: %s."), pszConfigDB);
        }
    }
#if TARGET_LINUX
        _ENDTRY_
#else
        __finally{}
#endif

    SRD_RETURN_VAL(Return);
}

/** \fn GetOmniBackDatabaseLocations(R_TCHAR ***ppDbLocs, R_ULONG *pCount)
    \brief Get HP DP Database location
    \param ppDbLocs Path(s) of database location
    \param pCount Number of location
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
R_RESULT
GetOmniBackDatabaseLocations(R_TCHAR ***ppDbLocs, R_ULONG *pCount)
{
    ERH_FUNCTION (_T("GetOmniBackDatabaseLocations"));

    R_RESULT    Return = SRDERR_ERROR;
    R_TCHAR     *tmpStr = NULL;
    R_TCHAR     **pszPaths = NULL;
    R_ULONG     ulCount = 0;

    SRD_ENTER_FCN;

    _TRY_
    {
        if (pCount == NULL || ppDbLocs == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Invalid parameters."));
            _LEAVE_;
        }

        *pCount = 0;

        pszPaths = MALLOC(sizeof(R_TCHAR*) * MAX_DB_PATHS);
        if (pszPaths == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation FAILURE."));
            _LEAVE_;
        }
        memset(pszPaths, 0, sizeof(R_TCHAR*) * MAX_DB_PATHS);

        tmpStr = IDBGetDatapath();
        if (!tmpStr)
        {
            DbgPlain(DBG_SYSNFO, _T("IDBGetDatapath() FAILED."));
            _LEAVE_;
        }

        pszPaths[ulCount] = MALLOC((strlen(tmpStr) + 1) * sizeof(R_TCHAR));
        if (pszPaths[ulCount] == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation FAILURE."));
            _LEAVE_;
        }

        strcpy(pszPaths[ulCount++], tmpStr);
        FREE(tmpStr);

        tmpStr = IDBGetIDBPath();
        if (!tmpStr)
        {
            DbgPlain(DBG_SYSNFO, _T("IDBGetIDBPath() FAILED."));
            _LEAVE_;
        }

        pszPaths[ulCount] = MALLOC((strlen(tmpStr) + 1) * sizeof(R_TCHAR));
        if (pszPaths[ulCount] == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation FAILURE."));
            _LEAVE_;
        }

        strcpy(pszPaths[ulCount++], tmpStr);
        FREE(tmpStr);

        tmpStr = IDBGetJCEPath();
        if (!tmpStr)
        {
            DbgPlain(DBG_SYSNFO, _T("IDBGetJCEPath() FAILED."));
            _LEAVE_;
        }

        pszPaths[ulCount] = MALLOC((strlen(tmpStr) + 1) * sizeof(R_TCHAR));
        if (pszPaths[ulCount] == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation FAILURE."));
            _LEAVE_;
        }

        strcpy(pszPaths[ulCount++], tmpStr);
        FREE(tmpStr);

        *ppDbLocs = pszPaths;
        *pCount = ulCount;

        DbgPlain(DBG_SYSNFO, _T("Got %ld database location(s)."), ulCount);

        Return = SRDERR_SUCCESS;
    }
    _FINALLY_
    {
        if (Return != SRDERR_SUCCESS)
        {
            FreeOmniBackDatabaseLocations(pszPaths, ulCount);
            FREE(tmpStr);
        }
    }
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

/**
   \fn GetSysDirectories(R_TCHAR *pSysRoot,
                        R_TCHAR *pObHome,
                        R_TCHAR *pObData)
   \brief Check for system directories (Windows: c:\windows;
          Linux:/, /tmp ...)
   \param pSysRoot return All System directories
   \param pObHome return Home directory of Data Protector
   \param pObData return Data directory of Data Protector
   \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSysDirectories(R_TCHAR *pSysRoot,
                    R_TCHAR *pObHome,
                    R_TCHAR *pObData)
{
    ERH_FUNCTION (_T("GetSysDirectories"));
    R_RESULT    Result = SRDERR_SUCCESS;

#if TARGET_LINUX
    const R_TCHAR   *ObPaths[MAX_OB_PATH] = {_T("/etc/opt/omni"),
                                            _T("/opt/omni"),
                                            _T("/var/opt/omni")};
    const R_TCHAR   *ObDataPaths[MAX_OB_DATA_PATH] = {_T("/etc/opt/omni"),
                                                    _T("/var/opt/omni")};

#endif

    if (pSysRoot != NULL)
        *pSysRoot = _T('\0');
    if (pObHome != NULL)
        *pObHome = _T('\0');
    if (pObData != NULL)
        *pObData = _T('\0');

#if TARGET_WIN32
    if (pSysRoot != NULL)
    {
        if (GetWindowsDirectory(pSysRoot, MAX_PATH) == 0)
            Result = SRDERR_ERROR;
    }

    if (
        Result == SRDERR_SUCCESS &&
        pObHome != NULL &&
        pObData != NULL
    )
    {
        HKEY    hOmni = NULL;

        Result = SRDERR_ERROR;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_OB2COMMON, 0, KEY_READ, &hOmni) == ERROR_SUCCESS)
        {
            R_ULONG ulSize = MAX_PATH * sizeof(R_TCHAR);

            if (RegQueryValueEx(hOmni, REGVAL_HOMEDIR, NULL, NULL, (R_UCHAR*)pObHome, &ulSize) == ERROR_SUCCESS)
            {
                ulSize = MAX_PATH * sizeof(R_TCHAR);
                if (RegQueryValueEx(hOmni, REGVAL_DATADIR, NULL, NULL, (R_UCHAR*)pObData, &ulSize) == ERROR_SUCCESS)
                {
                    Result = SRDERR_SUCCESS;
                }
            }

            RegCloseKey(hOmni);
        }
    }
#elif TARGET_LINUX
    /**
     * Linux System Mount Points
     */
    if (pSysRoot != NULL)
    {
        if (GetLnxSysDirectories(pSysRoot) != SRDERR_SUCCESS)
            Result = SRDERR_ERROR;
    }

    /**
     * OBHOME paths
     */
    if (pObHome != NULL)
    {
        R_INT   ii=0;

        for(ii=0; ii<MAX_OB_PATH; ii++)
        {
            if (ii>0)
            {
                strcat(pObHome,_T(","));
                strcat(pObHome,ObPaths[ii]);
            }
            else
                strcpy(pObHome,ObPaths[ii]);
        }
    }

    /**
     * OBDATA paths
     */
    if (ObDataPaths != NULL)
    {
        R_INT   ii=0;

        for(ii=0; ii<MAX_OB_DATA_PATH; ii++)
        {
            if (ii>0)
            {
                strcat(pObData,_T(","));
                strcat(pObData,ObDataPaths[ii]);
            }
            else
                strcpy(pObData,ObDataPaths[ii]);
        }
    }
#else
    Result = SRDERR_SUCCESS;
#endif  /* TARGET_WIN32, TARGET_LINUX */


    return Result;
}

/** \fn GetInetPort(R_ULONG *pInetPort)
    \brief Get Inet port (Default inet port is 5565)
    \param pInetPort Return inet port
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetInetPort(R_ULONG *pInetPort)
{
    ERH_FUNCTION (_T("GetInetPort"));
    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

    if (pInetPort != NULL)
    {
#if TARGET_WIN32
        HKEY        hOmni = NULL;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_OB2COMMON, 0, KEY_READ, &hOmni) == ERROR_SUCCESS)
        {
            R_TCHAR     pszPort[MAX_PATH];
            R_ULONG     ulSize = MAX_PATH * sizeof(R_TCHAR);

            if (RegQueryValueEx(hOmni, REGVAL_INETPORT, NULL, NULL, (R_UCHAR*)pszPort, &ulSize) == ERROR_SUCCESS)
            {
                if (sscanf(pszPort, _T("%d"), pInetPort) == 1)
                    Return = SRDERR_SUCCESS;
            }

            RegCloseKey(hOmni);
        }

        if (Return != SRDERR_SUCCESS)
        {
            *pInetPort = INET_PORT;

            Return = SRDERR_SUCCESS;
        }
#elif TARGET_LINUX /* TARGET_WIN32 */
    R_INT   InetPort = 0;   /**< Temporary variable for Inet Port*/
    R_TCHAR line[100];      /**< line from release file*/
    FILE    *procpt;        /**< File handler for open release file*/

    _TRY_
        if (pInetPort != NULL)
        {
            /**
             * Open /etc/services file
             */
            procpt = fopen(SERVICE_FILE, _T("r"));
            if (!procpt)
            {
                Return = SRDERR_ERROR;
                _LEAVE_;
            }

            /**
             * Read Inet port from /etc/services. Default: 5565
             */
            while (fgets(line,sizeof(line),procpt))
            {
                if (sscanf(line,"omni\t\t%d/tcp",&InetPort) == 1)
                    continue;
            }

            *pInetPort = InetPort;
        }

        Return = SRDERR_SUCCESS;
    _FINALLY_
        /**
         * Destroy file handler
         */
        if (procpt)
            fclose(procpt);
    _ENDTRY_
#else   /* TARGET_WIN32, TARGET_LINUX */
        *pInetPort = INET_PORT;

        Return = SRDERR_SUCCESS;
#endif  /* TARGET_WIN32, TARGET_LINUX */
    }

    SRD_EXIT_FCN;

    return Return;
}

/** \fn GetHeaderInfo(PSRDHEADER pHeader)
    \brief Get header informations of system as: version OS,
           Platform, hostname, cellmanager, auxcellmanager ...
    \param pHeader Return SRD structure with header informations
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetHeaderInfo(PSRDHEADER pHeader)
{
    ERH_FUNCTION (_T("GetHeaderInfo"));

    R_RESULT    Result = SRDERR_ERROR;
    R_ULONG     Version = 0, Platform = OS_ERROR;

    SRD_ENTER_FCN;

    /**
     * Get Version of the SRD file/OmniBack
     *
     * Function implemented in: srdcmn.c
     */
    if (GetVersionInfo(&Version) == SRDERR_SUCCESS)
    {
        /**
         * Get Platform
         *
         * Function implemented in: srdcmn.c
         */
        if (GetPlatformInfo(&Platform, NULL, NULL) == SRDERR_SUCCESS)
        {
#if TARGET_LINUX
            R_TCHAR *cl = getenv(_T("OB2_DR_CLIENT_NAME"));
#else
            R_TCHAR *cl = GetEnv(_T("OB2_DR_CLIENT_NAME"));
#endif
            pHeader->Version = Version;
            pHeader->Platform = Platform;

            if (cl)
            {
                DbgPlain(DBG_SYSNFO,
                    _T("Using environment variable OB2_DR_CLIENT_NAME for header.host."));
                pHeader->HostName = (R_TCHAR*)StrNewCopy(cl);
            }
            else
            {
                DbgPlain(DBG_SYSNFO,
                    _T("Environment variable OB2_DR_CLIENT_NAME not found; using cmnNodename for header.host."));
                pHeader->HostName = (R_TCHAR*)StrNewCopy(cmnNodename);
            }

            /* Try to expand Client host name to long name */
            cl = NULL;
            CmnExpandHostName(pHeader->HostName, &cl);
            if (cl)
            {
                DbgPlain(DBG_SYSNFO,
                    _T("Expanded for header.host from %s to %s."),
                    pHeader->HostName,
                    cl);
                FREE(pHeader->HostName);
                pHeader->HostName = cl;
                cl = NULL;
            }
            else
            {
                DbgPlain(DBG_SYSNFO,
                    _T("Could not expand for header.host from %s."),
                    pHeader->HostName);
            }

            pHeader->CellManager = StrNewCopy(cmnCSHostname);
            pHeader->auxCellManager = StrNewCopy(cmnCSHostname); /* parasoft-suppress  PB-07 "Both types are the same." */

            Result = SRDERR_SUCCESS;
        }
    }

    SRD_EXIT_FCN;

    return Result;
}

/** \fn GetSysInfo(PSYSINFO pSysInfo)
    \brief Get system information
    \param pHeader Return SRD structure with system informations
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSysInfo(PSYSINFO pSysInfo)
{
    ERH_FUNCTION (_T("GetSysInfo"));

    R_RESULT    Result = SRDERR_ERROR;
    R_ULONG     Sp = 0;
    R_TCHAR     pBuild[STRLEN_STD];

    SRD_ENTER_FCN;

    /**
     * Get Build and Service Pack of the Operating System
     *
     * Function implemented in: srdcmn.c
     */
    if (GetPlatformInfo(NULL, pBuild, &Sp) == SRDERR_SUCCESS)
    {
        R_TCHAR pSysRoot[MAX_PATH];
        R_TCHAR pObHome[MAX_PATH];
        R_TCHAR pObData[MAX_PATH];

        /**
         * Get system directories
         *
         * Function implemented in: this file
         */
        if (GetSysDirectories(pSysRoot, pObHome, pObData) == SRDERR_SUCCESS)
        {
            R_ULONG     InetPort = 0;
            R_ULONG     startupType = RST_UNKNOWN;

            /**
             * Get Inet port and startup type of the RSM Service
             *
             * Functions implemented in: srdcmn.c
             */
            if (
                GetInetPort(&InetPort) == SRDERR_SUCCESS
#if TARGET_WIN32
                 &&
                GetRSMServiceInfo(&startupType) == SRDERR_SUCCESS
#endif
            )
            {
#if TARGET_WIN32
                R_TCHAR  pObDataShort[STRLEN_2K];
                R_TCHAR *token = NULL;
                R_INT retValue =  -1;
#endif

                pSysInfo->Build = StrNewCopy(pBuild);
                pSysInfo->ServicePack = Sp;
                pSysInfo->InetPort = InetPort;
                pSysInfo->SystemRoot = StrNewCopy(pSysRoot);
                pSysInfo->ObHome = StrNewCopy(pObHome);
                pSysInfo->ObData = StrNewCopy(pObData);
                pSysInfo->RSMStartupType = startupType;
#if TARGET_WIN32
                PathCutTrailSlashes(pObData, 0);
                retValue = GetShortPathName(pObData, pObDataShort, STRLEN_2K);

                if (retValue != 0 &&
                    strcmp(pObDataShort, pObData) != 0)
                {
                    memcpy(pObDataShort, pObDataShort+3, STRLEN_2K);

                    token = strtok(pObDataShort, _T("\\"));
                    if (token != NULL)
                    {
                        pSysInfo->ObDataShort = StrNewCopy(pObDataShort);
                    }
                }
#endif
                Result = SRDERR_SUCCESS;
            }
        }
    }

    SRD_EXIT_FCN;

    return Result;
}

/** \fn GetHeaderData(PSRDHEADER *ppOut)
    \brief Collecting Header of the SRD file Information
    \param ppOut Return SRD structure with Header System Information
    \return SRDERR_SUCCESS or SRDERR_SUCCESS
*/
R_RESULT
GetHeaderData(PSRDHEADER *ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    PSRDHEADER  pHeader = NULL;

    _TRY_
    {
        pHeader = MALLOC(sizeof(SRDHEADER));
        if (pHeader == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
            _LEAVE_;
        }
        memset(pHeader, 0, sizeof(SRDHEADER));

        /**
         * Get Header information of the system
         *
         * Function implemented in: this file
         */
        if (GetHeaderInfo(pHeader) != SRDERR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot obtain SRD buffer header data."));
            _LEAVE_;
        }
        *ppOut = pHeader;

        Return = SRDERR_SUCCESS;
    }
    _FINALLY_
    {
        if (Return != SRDERR_SUCCESS)
        {
            FreeHeaderInfo(pHeader);
            FREE(pHeader);
        }
    }
    _ENDTRY_

    return Return;
}

/** \fn GetSystemData(PSYSINFO *ppOut)
    \brief Collecting System informations
    \param ppOut Return SRD structure with System Information
    \return SRDERR_SUCCESS or SRDERR_SUCCESS
*/
R_RESULT
GetSystemData(PSYSINFO *ppOut)
{
    R_RESULT Return = SRDERR_ERROR;
    PSYSINFO pSysInfo = NULL;

    _TRY_
    {
        pSysInfo = MALLOC(sizeof(SYSINFO));
        if (pSysInfo == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
            _LEAVE_;
        }
        memset(pSysInfo, 0, sizeof(SYSINFO));

        /**
         * Get System informations
         *
         * Function implemented in: this file
         */
        if (GetSysInfo(pSysInfo) != SRDERR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot obtain SRD buffer system info data."));
            _LEAVE_;
        }
        *ppOut = pSysInfo;

        Return = SRDERR_SUCCESS;
    }
    _FINALLY_
    {
        if (Return != SRDERR_SUCCESS)
        {
            FreeSysInfo(pSysInfo);
            FREE(pSysInfo);
        }
    }
    _ENDTRY_

    return Return;
}

R_RESULT
IsCellManager(PSRDHEADER pHeader, PCLUSINFO pClusInfo)
{
    R_RESULT    Return = SRDERR_ERROR;

    if (
        (
            NULL != pHeader &&
            NULL != pHeader->CellManager && NULL != pHeader->HostName &&
            (NULL == pClusInfo || 0 == pClusInfo->ClusterIsCM) &&
            stricmp(pHeader->CellManager, pHeader->HostName) == 0
        )
        ||
        (
            IsClusterCellManager(pClusInfo) == SRDERR_SUCCESS
        )
    )
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

R_RESULT
IsClusterCellManager(PCLUSINFO pClusInfo)
{
    R_RESULT Return = SRDERR_ERROR;

    if (
        NULL != pClusInfo &&
        0 != pClusInfo->ClusterIsCM
    )
        Return = SRDERR_SUCCESS;

    return Return;
}

#if TARGET_WIN32

typedef HCLUSTER (__stdcall *OpenClusterProc)(LPCWSTR);
typedef BOOL (__stdcall *CloseClusterProc)(HCLUSTER);
typedef R_DWORD (__stdcall *GetClusterInfoProc)(HCLUSTER, LPWSTR, R_DWORD*, LPCLUSTERVERSIONINFO);
typedef R_DWORD (__stdcall *GetClusterQuorumProc)(HCLUSTER, LPWSTR, R_DWORD*, LPWSTR, R_DWORD*, R_DWORD*);
typedef R_DWORD (__stdcall *GetNodeClusterStateProc)(LPCWSTR, R_DWORD*);
typedef HCLUSENUM (__stdcall *ClusterOpenEnumProc)(HCLUSTER, R_DWORD);
typedef R_DWORD (__stdcall *ClusterCloseEnumProc)(HCLUSENUM);
typedef R_DWORD (__stdcall *ClusterEnumProc)(HCLUSENUM, R_DWORD, R_DWORD*, LPWSTR, R_DWORD*);
typedef HRESOURCE (__stdcall *OpenClusterResourceProc)(HCLUSTER, LPCWSTR);
typedef BOOL (__stdcall *CloseClusterResourceProc)(HRESOURCE);
typedef R_DWORD (__stdcall *ClusterResourceControlProc)(HRESOURCE, HNODE, R_DWORD, R_VOID*, R_DWORD, R_VOID*, R_DWORD, R_DWORD*);
typedef CLUSTER_RESOURCE_STATE (__stdcall *GetClusterResourceStateProc)(HRESOURCE, LPWSTR, R_DWORD*, LPWSTR, R_DWORD*);


typedef struct _tagClusFuncInfo
{
    HMODULE hClusMod;
    OpenClusterProc fcnOpen;
    CloseClusterProc fcnClose;
    GetClusterInfoProc fcnInfo;
    GetNodeClusterStateProc fcnState;
    GetClusterQuorumProc fcnQuorum;
    ClusterOpenEnumProc fcnOpenEnum;
    ClusterCloseEnumProc fcnCloseEnum;
    ClusterEnumProc fcnEnum;
    OpenClusterResourceProc fcnOpenResource;
    CloseClusterResourceProc fcnCloseResource;
    ClusterResourceControlProc fcnResourceControl;
    GetClusterResourceStateProc fcnResourceState;
} CLUSFUNCINFO, *PCLUSFUNCINFO;

/* --------------------------------------------------------------------
|   Upcases the volume identifier of the input path. The function assumes
|   that the first character in the input path is the drive letter.
----------------------------------------------------------------------*/
R_VOID
UpcaseVolumeLetter(R_TCHAR *pszPath)
{
    if (
        pszPath != NULL &&
        StrLen(pszPath) > 0
    )
    {
        R_TCHAR chOld = *(pszPath + 1);

        *(pszPath + 1) = _T('\0');
        CharUpper(pszPath);
        *(pszPath + 1) = chOld;
    }
}

/* --------------------------------------------------------------------
|   Upcases a single character
----------------------------------------------------------------------*/
R_TCHAR
UpcaseCharacter(R_ULONG ulChar)
{
    R_TCHAR pszChar[2];

    pszChar[0] = (R_TCHAR)ulChar;
    pszChar[1] = 0;

    UpcaseVolumeLetter(pszChar);

    return *pszChar;
}

/* --------------------------------------------------------------------
|   Retreives the system volume directory path, drive letter only or
|   both (c:\winnt, c:\windows, etc.)
----------------------------------------------------------------------*/
R_RESULT
GetSystemVolume(R_ULONG *pLetter, R_TCHAR *pszDir)
{
    R_RESULT    Return = SRDERR_SUCCESS;
    R_TCHAR     pszWinDir[MAX_PATH];

    GetWindowsDirectory(pszWinDir, MAX_PATH);

    if (pLetter != NULL)
        *pLetter = (R_ULONG)pszWinDir[0];
    if (pszDir != NULL)
        strcpy(pszDir, pszWinDir);

    return Return;
}

R_RESULT
GetClusterQuorumVolume(R_TCHAR *pszVol)
{
    ERH_FUNCTION (_T("GetClusterQuorumVolume"));

    R_RESULT    Return = SRDERR_ERROR;
    HMODULE     hClusMod = NULL;
    HANDLE      hCluster = NULL;
    OpenClusterProc      fcnOpen = NULL;
    CloseClusterProc     fcnClose = NULL;
    GetClusterQuorumProc fcnQuorum = NULL;

    SRD_ENTER_FCN;

    __try
    {
        WCHAR   pszQuorumName[MAX_PATH] = { 0 }, pszLogPath[MAX_PATH] = { 0 };
        R_DWORD dwNameSize = MAX_PATH, dwLogSize = MAX_PATH, dwLogMaxSize = 0;
        USES_CONVERSION;
        UNREF_CONV;

        hClusMod = LoadLibrary(CLUSAPI_DLL_NAME);
        if (NULL == hClusMod)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cluster API library could not be loaded. Possibly unavailable on this system.")); }
            );
        fcnOpen = (OpenClusterProc)GetProcAddress(hClusMod, "OpenCluster");
        fcnClose = (CloseClusterProc)GetProcAddress(hClusMod, "CloseCluster");
        fcnQuorum = (GetClusterQuorumProc)GetProcAddress(hClusMod, "GetClusterQuorumResource");

        if (
            NULL == fcnOpen ||
            NULL == fcnClose ||
            NULL == fcnQuorum
        )
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cluster API library functions could not be bound. Wrong dll version ???.")); }
            );

        hCluster = fcnOpen(NULL);
        if (NULL == hCluster)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cannot open current cluster - error %d."), GetLastError()); }
            );

        if (fcnQuorum(hCluster, pszQuorumName, &dwNameSize, pszLogPath, &dwLogSize, &dwLogMaxSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cannot get the name of quorum resource - error %d."), GetLastError()); }
            );

        /*
         * if Volume GUID
         */
        if (strstr(pszLogPath, _T("Volume{")) != NULL)
        {
            //Quorum volume log path contains the /Cluster that we don't need
            strncpy(pszVol, W2T(pszLogPath), VOLUME_GUID_LEN);
        }
        else
        {
            strcpy(pszVol, W2T(pszLogPath));
        }

        DbgPlain(DBG_SYSNFO, _T("Got QUORUM log path: %s."), pszLogPath);
        DbgPlain(DBG_SYSNFO, _T("Got QUORUM volume: %s."), pszVol);

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if (NULL != hCluster && NULL != fcnClose)
            fcnClose(hCluster);
        if (NULL != hClusMod)
            FreeLibrary(hClusMod);
    }
    SRD_RETURN_VAL(Return);
}

static R_RESULT
GetRSMServiceInfo(R_ULONG *startupType)
{
    R_RESULT    Return = SRDERR_SUCCESS;
    SC_HANDLE   svcMgr = NULL, svcNtms = NULL;
    R_BYTE *buffer = NULL;

    __try
    {
        LPQUERY_SERVICE_CONFIG qsc = NULL;
        R_DWORD bytesNeeded = 0;

        *startupType = RST_UNKNOWN;

        /*---------------------------------------------------------------------
        |   Connect to service manager
         --------------------------------------------------------------------*/
        svcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (svcMgr == NULL)
            SRD_W32_LEAVE(
                {
                    DbgStamp(DBG_SYSNFO);
                    DbgPlain(DBG_SYSNFO, _T("OpenSCManager() -> System Error: %d"),
                        GetLastError());
                }
            );
        /*---------------------------------------------------------------------
        |   Try to open RSM Service
         --------------------------------------------------------------------*/
        svcNtms = OpenService(svcMgr, RSM_SERVICE_NAME, SERVICE_QUERY_CONFIG);
        if (svcNtms == NULL)
            SRD_W32_LEAVE(
                {
                    DbgStamp(DBG_SYSNFO);
                    DbgPlain(DBG_SYSNFO, _T("OpenService(%s) -> System Error: %d"),
                        RSM_SERVICE_NAME, GetLastError());
                }
            );

        *startupType = RST_MANUAL;
        if (!QueryServiceConfig(svcNtms, NULL, 0, &bytesNeeded))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                SRD_W32_LEAVE(
                    {
                        DbgStamp(DBG_SYSNFO);
                        DbgPlain(DBG_SYSNFO, _T("Unexpected QueryServiceConfig() error: %d"),
                            GetLastError());
                    }
                );
            buffer = MALLOC(bytesNeeded);
            if (NULL == buffer)
                SRD_W32_LEAVE(
                    {
                        DbgStamp(DBG_SYSNFO);
                        DbgPlain(DBG_SYSNFO, _T("Unexpected memory allocation error."));
                    }
                );
        }
        qsc = (LPQUERY_SERVICE_CONFIG)buffer;
        if (!QueryServiceConfig(svcNtms, qsc, bytesNeeded, &bytesNeeded))
            SRD_W32_LEAVE(
                {
                    DbgStamp(DBG_SYSNFO);
                    DbgPlain(DBG_SYSNFO, _T("QueryServiceConfig() error: %d"),
                        GetLastError());
                }
            );
        switch(qsc->dwStartType)
        {
            case SERVICE_DISABLED:
                *startupType = RST_DISABLED;
                break;
            case SERVICE_AUTO_START:
                *startupType = RST_AUTOMATIC;
                break;
            default:
                *startupType = RST_MANUAL;
                break;
        }

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if (buffer != NULL)
        {
            FREE(buffer);
        }
        if (svcNtms != NULL)
        {
            CloseServiceHandle(svcNtms);
        }
        if (svcMgr != NULL)
        {
            CloseServiceHandle(svcMgr);
        }
    }

    return Return;
}

static void
ResourceQueryNetworkName(HCLUSTER cluster, PCLUSFUNCINFO cf, R_TCHAR *res_name, R_TCHAR *net_name)
{
    ERH_FUNCTION(_T("ResourceQueryNetworkName"));

    HRESOURCE resource = NULL;

    R_TCHAR *net_name_long = NULL;
    net_name[0] = _T('\0');

    DbgFcnIn();

    __try
    {
        R_DWORD result, bytes_ret;
        wchar_t buffer[STRLEN_1K];
        R_TCHAR resource_type[STRLEN_1K];

        resource = cf->fcnOpenResource(cluster, T2W(res_name));
        if (NULL == resource)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to open cluster resources."));
            __leave;
        }

        DbgPlain(DBG_SYSNFO, _T("Opened resource (%s)."), res_name);

        result = cf->fcnResourceControl(resource, NULL,
            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
            NULL, 0, buffer, STRLEN_1K * sizeof(wchar_t), &bytes_ret);
        if (result != ERROR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to get resource's type information."));
            __leave;
        }

        strcpy(resource_type, W2T(buffer));
        DbgPlain(DBG_SYSNFO, _T("Resource type: %s."), resource_type);
        if(!strcmp(W2T(resource_type),CLUS_STR_MNS))
        {
            clus_Type = 1;
        }
        else
        {
            DbgPlain(DBG_SYSNFO,_T(" Resource type not MNS"));
        }
        if (stricmp(resource_type, _T("Network Name")) != 0)
            __leave;

        result = cf->fcnResourceControl(resource, NULL,
            CLUSCTL_RESOURCE_GET_NETWORK_NAME,
            NULL, 0, buffer, STRLEN_1K * sizeof(wchar_t), &bytes_ret);
        if (result != ERROR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to get resource's network name."));
            __leave;
        }

        strcpy(net_name, W2T(buffer));
        if (CmnExpandHostName(net_name, &net_name_long) != 1)
        {
            DbgPlain(DBG_SYSNFO, _T("Cannot expand cluster host name %s."), net_name);
        }
        else
        {
            strcpy(net_name, net_name_long);
            FREE(net_name_long);
        }

        DbgPlain(DBG_SYSNFO, _T("Network name: %s."), net_name);
    }
    __finally
    {
        if (resource != NULL)
        {
            cf->fcnCloseResource(resource);
        }
    }

    DbgFcnOut();
}

static void
ClusterQueryNetworkNames(HCLUSTER cluster, PCLUSFUNCINFO cf, R_TCHAR *network_names)
{
    ERH_FUNCTION(_T("ClusterQueryNetworkNames"));

    HCLUSENUM clusenum = NULL;
    wchar_t *buffer = NULL;
    R_TCHAR names[STRLEN_16K] = { _T('\0') };

    DbgFcnIn();

    __try
    {
        int index;
        R_DWORD result, bufsize;

        clusenum = cf->fcnOpenEnum(cluster, CLUSTER_ENUM_RESOURCE);
        if (NULL == clusenum)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to enumerate cluster resources."));
            __leave;
        }

        DbgPlain(DBG_SYSNFO, _T("Cluster resource enumeration opened."));

        result = ERROR_SUCCESS;
        bufsize = 512;
        buffer = MALLOC(bufsize * sizeof(wchar_t));
        if (NULL == buffer)
        {
            DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."), bufsize * sizeof(wchar_t));
            __leave;
        }

        for (index = 0; result != ERROR_NO_MORE_ITEMS; )
        {
            R_DWORD type;
            R_DWORD bufcount = bufsize;
            R_TCHAR net_name[STRLEN_1K];

            result = cf->fcnEnum(clusenum, index, &type, buffer, &bufcount);
            switch (result)
            {
            case ERROR_SUCCESS:
                DbgPlain(DBG_SYSNFO, _T("Cluster resource (%s) found."), W2T(buffer));

                if (type == CLUSTER_ENUM_RESOURCE)
                {
                    ResourceQueryNetworkName(cluster, cf, W2T(buffer), net_name);
                    if (net_name[0] != _T('\0'))
                    {
                        if (names[0] != _T('\0'))
                            strcat(names, _T(","));
                        strcat(names, net_name);
                    }
                }
                else
                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                    DbgPlain(DBG_SYSNFO, _T("Strange resource (%s)."), W2T(buffer));
                }
                index++;
                break;
            case ERROR_MORE_DATA:
                DbgPlain(DBG_SYSNFO, _T("Buffer too small, reserving more space."));
                bufsize = bufcount + 128;
                buffer = REALLOC(buffer, bufsize * sizeof(wchar_t));
                if (NULL == buffer)
                {
                    DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."), 
                        bufsize * sizeof(wchar_t));
                    __leave;
                }
               break;
            case ERROR_NO_MORE_ITEMS:
                DbgPlain(DBG_SYSNFO, _T("End of cluster resource list."));
                break;
            default:
                DbgPlain(DBG_SYSNFO, _T("Failed to enumerate a cluster item."));
                __leave;
            }
        }
    }
    __finally
    {
        if (clusenum)
            cf->fcnCloseEnum(clusenum);
        if (buffer)
            FREE(buffer);
    }
    strcpy(network_names, names);

    DbgFcnOut();
}

//When CSV support is added enable this function (delete #if 0 #endif)
//For more details see 
#if 0
/* --------------------------------------------------------------------
|   Get cluster network name
----------------------------------------------------------------------*/
static void
GetClusterNetworkName(HCLUSTER cluster,
                      PCLUSFUNCINFO cf,
                      R_TCHAR *network_name)
{
    ERH_FUNCTION(_T("GetClusterNetworkName"));

    HCLUSENUM clusenum = NULL;
    wchar_t *buffer = NULL;
    R_TCHAR names[STRLEN_4K] = { _T('\0') };

    DbgFcnIn();

    __try
    {
        int index = 0;
        R_DWORD result = 0;
        R_DWORD bufsize = 0;

        clusenum = cf->fcnOpenEnum(cluster, CLUSTER_ENUM_RESOURCE);
        if (NULL == clusenum)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to enumerate cluster resources."));
            __leave;
        }

        DbgPlain(DBG_SYSNFO, _T("Cluster resource enumeration opened."));

        result = ERROR_SUCCESS;
        bufsize = 512; /* parasoft-suppress  CODSTA-29 "Old code" */
        buffer = MALLOC(bufsize * sizeof(wchar_t));
        if (NULL == buffer)
        {
            DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."), bufsize * sizeof(wchar_t));
            __leave;
        }

        index = 0;
        while (result != ERROR_NO_MORE_ITEMS)
        {
            R_DWORD type = 0;
            R_DWORD bufcount = bufsize;
            R_TCHAR net_name[STRLEN_1K] = { _T('\0') };

            result = cf->fcnEnum(clusenum, index, &type, buffer, &bufcount);
            switch (result)
            {
            case ERROR_SUCCESS:
                DbgPlain(DBG_SYSNFO, _T("Cluster resource (%s) found."), W2T(buffer));

                if (type == CLUSTER_ENUM_RESOURCE)
                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                    if (stricmp(W2T(buffer), _T("Cluster Name")) == 0)
                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                        ResourceQueryNetworkName(cluster, cf, W2T(buffer), net_name);
                        if (net_name[0] != _T('\0'))
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            if (names[0] != _T('\0'))
                                strcat(names, _T(","));
                            strcat(names, net_name);
                        }
                    }
                    else
                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                        DbgPlain(DBG_SYSNFO, _T("Strange resource (%s)."), W2T(buffer));
                    }
                }
                index++;
                break;
            case ERROR_MORE_DATA:
                DbgPlain(DBG_SYSNFO, _T("Buffer too small, reserving more space."));
                bufsize = bufcount + 128; /* parasoft-suppress  CODSTA-29 "Old code" */
                buffer = REALLOC(buffer, bufsize * sizeof(wchar_t));
                if (NULL == buffer)
                { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                    DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."),
                        bufsize * sizeof(wchar_t));
                    __leave;
                }
                break;
            case ERROR_NO_MORE_ITEMS:
                DbgPlain(DBG_SYSNFO, _T("End of cluster resource list."));
                break;
            default: /* parasoft-suppress  MISRA2004-15_2 "Coding Guidelines suppression" */
                DbgPlain(DBG_SYSNFO, _T("Failed to enumerate a cluster item."));
                __leave;
            }
        }
    }
    __finally
    {
        if (clusenum != NULL)
        {
            cf->fcnCloseEnum(clusenum);
        }
        if (buffer != NULL)
        {
            FREE(buffer);
        }
    }
    strcpy(network_name, names);

    DbgFcnOut();
}
#endif

/*QCCR2A35378*/
#define MAX_GROUP_NAME   STRLEN_16K+1
#define MAX_NET_NAME     STRLEN_16K+1
#define MAX_UNSUPPORTED  STRLEN_48K+1

typedef struct _cluster_group
{
    R_TCHAR m_name[MAX_GROUP_NAME];
    R_TCHAR m_net_name[MAX_NET_NAME];
    R_TCHAR m_volumes[MAX_UNSUPPORTED];
    R_BOOL m_has_network_name;  /* This is used to determine do we have network name for cluster grup */
} CLUSTERGROUP, *PCLUSTERGROUP;

static void
AddNetworkName(R_TCHAR *group_name, R_TCHAR* net_name, CmnVec *groups)
{
    ERH_FUNCTION(_T("AddNetworkName"));
    int i;

    DbgFcnIn();
    DbgPlain(DBG_SYSNFO, _T("Adding network (%s) name to group (%s)"), net_name, group_name);

    for (i = 0; i < CmnVecH(groups); i++)
    {
        PCLUSTERGROUP g = (PCLUSTERGROUP)CmnVecI(groups, i);
        if (!stricmp(g->m_name, group_name))
            break;
    }

    if (i >= CmnVecH(groups))
    {
        CLUSTERGROUP new_group;

        DbgPlain(DBG_SYSNFO, _T("Adding new group."));

        memset(&new_group, 0, sizeof(CLUSTERGROUP));
        strcpy(new_group.m_name, group_name);
        new_group.m_has_network_name = TRUE;
        strcpy(new_group.m_net_name, net_name);
        CmnVecAdd(groups, &new_group, sizeof(CLUSTERGROUP));
    }
    else
    {
        PCLUSTERGROUP g = NULL;

        DbgPlain(DBG_SYSNFO, _T("Updating existing group."));

        g = (PCLUSTERGROUP)CmnVecI(groups, i);
        g->m_has_network_name = TRUE;
        strcpy(g->m_net_name, net_name);
    }

    DbgFcnOut();
}

static void
AddPhysicalVolume(R_TCHAR *group_name, R_TCHAR *volumeName, CmnVec *groups)
{
    ERH_FUNCTION (_T("AddPhysicalVolume"));

    int i;

    DbgFcnIn();

    DbgPlain(DBG_SYSNFO, _T("Adding physical volume (%s) to group (%s)"), volumeName, group_name);

    for (i = 0; i < CmnVecH(groups); i++)
    {
        PCLUSTERGROUP g = (PCLUSTERGROUP)CmnVecI(groups, i);
        if (!stricmp(g->m_name, group_name))
            break;
    }

    if (i >= CmnVecH(groups))
    {
        CLUSTERGROUP new_group;

        DbgPlain(DBG_SYSNFO, _T("Adding new group."));

        memset(&new_group, 0, sizeof(CLUSTERGROUP));
        strcpy(new_group.m_name, group_name);
        new_group.m_has_network_name = FALSE;
        strcpy(new_group.m_volumes, volumeName);
        CmnVecAdd(groups, &new_group, sizeof(CLUSTERGROUP));
    }
    else
    {
        PCLUSTERGROUP g = NULL;

        DbgPlain(DBG_SYSNFO, _T("Updating existing group."));

        g = (PCLUSTERGROUP)CmnVecI(groups, i);

        if (g->m_volumes[0] != _T('\0'))
            strcat(g->m_volumes, _T(","));
        strcat(g->m_volumes, volumeName);
    }

    DbgFcnOut();
}

static void
ResourceQueryMountPoint(
    PCLUSFUNCINFO cf, HRESOURCE resource, DWORD partition_number, R_TCHAR* vol_name)
{
    ERH_FUNCTION (_T("ResourceQueryMountPoints"));

    unsigned char *buffer = NULL;
    R_DWORD result = 0;
    R_DWORD bytes_ret = 0;
    R_DWORD buff_size = 2048*2;

    DbgFcnIn();

     __try
     {
         buffer = MALLOC(buff_size);
         if (!buffer)
         {
             DbgPlain(DBG_SYSNFO, _T("Memory allocation problem."));
             __leave;
         }

         memset(buffer, 0, buff_size);

         do
         {
             result = cf->fcnResourceControl(resource, NULL,
                 CLUSCTL_RESOURCE_STORAGE_GET_MOUNTPOINTS,
                 &partition_number, sizeof(DWORD), buffer,
                 buff_size, &bytes_ret);

             if (result == ERROR_MORE_DATA)
             {
                 buff_size = bytes_ret + 128;
                 buffer = REALLOC(buffer, buff_size);
                              
                 if (!buffer)
                 {
                     DbgPlain(DBG_SYSNFO, _T("Memory allocation problem."));
                     __leave;
                 }

                 memset(buffer, 0, buff_size);
             }
             else if (result != ERROR_SUCCESS)
             {
                 DbgPlain(DBG_SYSNFO, 
                     _T("Failed to get cluster partition mount points."));
                 __leave;
             }
             else
             {
                 break;
             }
         }
         while (TRUE); 

         if (((R_TCHAR*)buffer)[0] != _T('\0'))
         {
             strcpy(vol_name, (R_TCHAR*)buffer);
             PathToSlashes(vol_name);
             PathCutTrailSlashes(vol_name, 0);

             DbgPlain(DBG_SYSNFO,
                 _T("Found cluster partition mount point: %s"), (R_TCHAR*)buffer);
         }
     }
     __finally
     {
         if (buffer)
         {
             FREE(buffer);
         }
     }

     DbgFcnOut();
}

static void
ResourceQueryClusterGroups(HCLUSTER cluster, PCLUSFUNCINFO cf, R_TCHAR *res_name, CmnVec *groups)
{
    ERH_FUNCTION (_T("ResourceQueryClusterGroups"));

    HRESOURCE resource = NULL;
    unsigned char *disk_buffer = NULL;

    DbgFcnIn();

    __try
    {
        R_DWORD result, bytes_ret;
        wchar_t buffer[STRLEN_1K];
        R_TCHAR resource_type[STRLEN_STD];

        resource = cf->fcnOpenResource(cluster, T2W(res_name));
        if (NULL == resource)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to open cluster resources."));
            __leave;
        }

        DbgPlain(DBG_SYSNFO, _T("Opened resource (%s)."), res_name);

        result = cf->fcnResourceControl(resource, NULL,
            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
            NULL, 0, buffer, STRLEN_1K * sizeof(wchar_t), &bytes_ret);
        if (result != ERROR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to get resource's type information."));
            __leave;
        }

        strcpy(resource_type, W2T(buffer));
        DbgPlain(DBG_SYSNFO, _T("Resource type: %s."), resource_type);
        /*QCCR2A23162:The disaster recovery wizard was unable to find '': object/drive backed up*/
        /*Quorum disk should not be added to SRD file for MNS cluster. To identify a MNS cluster, check the cluster
        resource type for "Majority Node Set"*/
        if(!strcmp(W2T(resource_type),CLUS_STR_MNS))
        {
            clus_Type = 1;
        }
        else
        {
            DbgPlain(DBG_SYSNFO,_T("Resource type not MNS"));
        }
        if (!stricmp(resource_type, CLUS_RESTYPE_NAME_NETNAME) ||
            !stricmp(resource_type, CLUS_RESTYPE_NAME_HARDDISK))
        {
            wchar_t group[MAX_GROUP_NAME];
            R_DWORD group_size = MAX_GROUP_NAME;
            R_TCHAR group_name[MAX_GROUP_NAME];

            if (cf->fcnResourceState(resource, NULL, NULL, group, &group_size) == ClusterResourceStateUnknown)
            {
                DbgPlain(DBG_SYSNFO, _T("Failed to get resource's group name."));
                __leave;
            }

            strcpy(group_name, W2T(group));

            /**
             * resource_type = "Network Name"
            */
            if (!stricmp(resource_type, CLUS_RESTYPE_NAME_NETNAME))
            {
                R_TCHAR *net_name_long = NULL;
                R_TCHAR net_name[STRLEN_1K];

                result = cf->fcnResourceControl(resource, NULL,
                    CLUSCTL_RESOURCE_GET_NETWORK_NAME,
                    NULL, 0, buffer, STRLEN_1K * sizeof(wchar_t), &bytes_ret);
                if (result != ERROR_SUCCESS)
                {
                    DbgPlain(DBG_SYSNFO, _T("Failed to get resource's network name."));
                    __leave;
                }

                strcpy(net_name, W2T(buffer));
                if (CmnExpandHostName(net_name, &net_name_long) != 1)
                {
                    DbgPlain(DBG_SYSNFO, _T("Cannot expand cluster host name %s."), net_name);
                }
                else
                {
                    strcpy(net_name, net_name_long);
                    FREE(net_name_long);
                }

                DbgPlain(DBG_SYSNFO, _T("Network name: %s."), net_name);

                AddNetworkName(group_name, net_name, groups);
            }
            /**
             * resource_type = "Physical Disk"
            */
            else
            {
                CLUSPROP_BUFFER_HELPER cbh;
                R_DWORD buff_size = 2048;
                disk_buffer = MALLOC(buff_size);
                if (!disk_buffer)
                {
                    DbgPlain(DBG_SYSNFO, _T("Memory allocation problem."));
                    __leave;
                }
                do
                {
                    result = 0;
                    bytes_ret = 0;
                    if (IsVistaOrHigher())
                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                        result = cf->fcnResourceControl(resource, NULL,
                            CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO_EX,
                            NULL, 0, disk_buffer, buff_size, &bytes_ret);
                    }
                    else
                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                        result = cf->fcnResourceControl(resource, NULL,
                            CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                            NULL, 0, disk_buffer, buff_size, &bytes_ret);
                    }

                    if (result == ERROR_MORE_DATA)
                    {
                        buff_size = bytes_ret + 128;
                        disk_buffer = REALLOC(disk_buffer, buff_size);
                        if (!disk_buffer)
                        {
                            DbgPlain(DBG_SYSNFO, _T("Memory allocation problem."));
                            __leave;
                        }
                    }
                    else
                    {
                        break;
                    }

                }
                while (TRUE);

                if (result != ERROR_SUCCESS)
                {
                    DbgPlain(DBG_SYSNFO, _T("Failed to get Physical Disk resource info."));
                    __leave;
                }

                cbh.pb = disk_buffer;
                while (cbh.pb < disk_buffer + bytes_ret &&
                    cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
                {
                    R_TCHAR vol_name[MAX_PATH] = { _T('\0') }; /* parasoft-suppress  METRICS-23 "Adoption to the old code" */

                    switch (cbh.pSyntax->dw)
                    {
                        case CLUSPROP_SYNTAX_PARTITION_INFO_EX:
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            if (IsVistaOrHigher())
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */

                                if (strstr(W2T(cbh.pPartitionInfoValueEx->szDeviceName),
                                    _T("Volume{")) != NULL)
                                {
                                    DbgPlain(DBG_SYSNFO, 
                                        _T("Found volume GUID cluster resource (%s), searching for mount points."),
                                        W2T(cbh.pPartitionInfoValueEx->szDeviceName));

                                    ResourceQueryMountPoint(cf, resource,
                                        cbh.pPartitionInfoValueEx->PartitionNumber, vol_name);
                                }

                                if (vol_name[0] == _T('\0'))
                                {
                                    strcpy(vol_name, W2T(cbh.pPartitionInfoValueEx->szDeviceName));
                                }

                                AddPhysicalVolume(group_name, vol_name, groups);
                            }
                            else
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                strcpy(vol_name, W2T(cbh.pPartitionInfoValue->szDeviceName));
                                AddPhysicalVolume(group_name, vol_name, groups);
                            }
                            break;
                        }
                        case CLUSPROP_SYNTAX_PARTITION_INFO:
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            if (IsVistaOrHigher())
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                strcpy(vol_name, W2T(cbh.pPartitionInfoValueEx->szDeviceName));
                                AddPhysicalVolume(group_name, vol_name, groups);
                            }
                            else
                            { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                                strcpy(vol_name, W2T(cbh.pPartitionInfoValue->szDeviceName));
                                AddPhysicalVolume(group_name, vol_name, groups);
                            }
                            break;
                        }
                        default:
                            break;
                    }

                    cbh.pb += (sizeof(CLUSPROP_VALUE) +
                        ALIGN_CLUSPROP(cbh.pValue->cbLength));
                }
            }
        }
        else
            __leave;
    }
    __finally
    {
        if (resource)
        {
            cf->fcnCloseResource(resource);
        }
        if (disk_buffer)
        {
            FREE(disk_buffer);
        }
    }

    DbgFcnOut();
}

/* --------------------------------------------------------------------
|   Query unsupported volumes
----------------------------------------------------------------------*/
static void
ClusterQueryUnsupportedVolumes(HCLUSTER cluster, PCLUSFUNCINFO cf, R_TCHAR *unsupportedVols, R_TCHAR *supportedVols)
{
    ERH_FUNCTION(_T("ClusterQueryUnsupportedVolumes"));

    HCLUSENUM clusenum = NULL;
    wchar_t *buffer = NULL;
    CmnVec groups;

    DbgFcnIn();

    __try
    {
        int index;
        R_DWORD result, bufsize;

        CmnVecInit(&groups, 50, NULL);

        clusenum = cf->fcnOpenEnum(cluster, CLUSTER_ENUM_RESOURCE);
        if (NULL == clusenum)
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to enumerate cluster resources."));
            __leave;
        }

        DbgPlain(DBG_SYSNFO, _T("Cluster resource enumeration opened."));

        result = ERROR_SUCCESS;
        bufsize = 512;
        buffer = MALLOC(bufsize * sizeof(wchar_t));
        if (NULL == buffer)
        {
            DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."), bufsize * sizeof(wchar_t));
            __leave;
        }

        for (index = 0; result != ERROR_NO_MORE_ITEMS; )
        {
            R_DWORD type;
            R_DWORD bufcount = bufsize;

            result = cf->fcnEnum(clusenum, index, &type, buffer, &bufcount);
            switch (result)
            {
            case ERROR_SUCCESS:
                DbgPlain(DBG_SYSNFO, _T("Cluster resource (%s) found."), W2T(buffer));

                switch(type)
                {
                case CLUSTER_ENUM_RESOURCE:
                    ResourceQueryClusterGroups(cluster, cf, W2T(buffer), &groups);
                   break;
                default:
                    DbgPlain(DBG_SYSNFO, _T("Strange resource (%s)."), W2T(buffer));
                    break;
                }
                index++;
                break;
            case ERROR_MORE_DATA:
                DbgPlain(DBG_SYSNFO, _T("Buffer too small, reserving more space."));
                bufsize = bufcount + 128;
                buffer = REALLOC(buffer, bufsize * sizeof(wchar_t));
                if (NULL == buffer)
                {
                    DbgPlain(DBG_SYSNFO, _T("Cannot allocate resource buffer of size (%d)."), 
                        bufsize * sizeof(wchar_t));
                    __leave;
                }
                break;
            case ERROR_NO_MORE_ITEMS:
                DbgPlain(DBG_SYSNFO, _T("End of cluster resource list."));
                break;
            default:
                DbgPlain(DBG_SYSNFO, _T("Failed to enumerate a cluster item."));
                __leave;
            }
        }

        for (index = 0; index < CmnVecH(&groups); index++)
        {
            R_TCHAR *vols = NULL;
            R_TCHAR net_name[MAX_PATH] = { _T('\0') };
            PCLUSTERGROUP group = (PCLUSTERGROUP)CmnVecI(&groups, index);

            DbgPlain(DBG_SYSNFO, _T("Group network name: %d"), group->m_has_network_name);
            DbgPlain(DBG_SYSNFO, _T("  Group name:        %s"), group->m_name);
            DbgPlain(DBG_SYSNFO, _T("  Group net name:    %s"), group->m_net_name);
            DbgPlain(DBG_SYSNFO, _T("  Group volumes:     %s"), group->m_volumes);

#if 0 /**< Enable this, if use Cluster Shared Volumes*/
            /**
             * For all share volumes that do not belonging net name
             * we 'force' add cluster net name
            */
            if (group->m_net_name[0] == _T('\0'))
            {
                //TODO: When support is added enable the function GetClusterNetworkName it is under
                //#if 0 #endif
                GetClusterNetworkName(cluster, cf, net_name);

                group->m_has_network_name = TRUE;
                strcpy(group->m_net_name, net_name);
            }
#endif

            vols = (group->m_has_network_name ? supportedVols : unsupportedVols);
            if (group->m_volumes[0] != _T('\0'))
            {
                if (vols == supportedVols)
                {
                    R_TCHAR* token = NULL;
                    R_TCHAR volBuf[MAX_UNSUPPORTED] = { _T('\0') }; /* parasoft-suppress  METRICS-23 "Adoption to the old code" */

                    strcpy(volBuf, group->m_volumes);
                    token = strtok(volBuf, _T(","));
                    while (token)
                    {
                        if (vols[0] != _T('\0'))
                        { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                            strcat(vols, _T(","));
                        }
                        strcat(vols, token);
                        strcat(vols, _T(","));
                        strcat(vols, group->m_net_name);

                        token = strtok(NULL, _T(","));
                    }
                }
                else
                {
                    if (vols[0] != _T('\0'))
                    { /* parasoft-suppress  METRICS-23 "Adoption to the old code" */
                        strcat(vols, _T(","));
                    }
                    strcat(vols, group->m_volumes);
                }
            }
        }

        DbgPlain(DBG_SYSNFO, _T("Unsupported volumes: %s"), unsupportedVols);
        DbgPlain(DBG_SYSNFO, _T("Supported volumes:   %s"), supportedVols);
    }
    __finally
    {
        if (clusenum)
        {
            cf->fcnCloseEnum(clusenum);
        }
        if (buffer)
        {
            FREE(buffer);
        }
        CmnVecFree(&groups);
    }

    DbgFcnOut();
}

static R_RESULT
GetClusInfo(PCLUSINFO pClusInfo)
{
    ERH_FUNCTION(_T("GetClusInfo"));

    R_RESULT    Result = SRDERR_ERROR;
    HKEY        hKey = NULL;
    HANDLE      hCluster = NULL;
    CLUSFUNCINFO cf;

    memset(&cf, 0, sizeof(CLUSFUNCINFO));

    DbgFcnIn();

    __try
    {
        R_TCHAR network_names[STRLEN_16K] = { 0 };
        R_TCHAR unsupported_vols[STRLEN_48K] = { 0 };
        R_TCHAR supported_vols[STRLEN_48K] = { 0 };
        R_DWORD dwWinErr = ERROR_SUCCESS, dwState = 0;
        R_TCHAR *pszHostName = NULL, *pszExpName = NULL, pszClusReg[MAX_PATH] = { 0 };
        WCHAR   pszClusterName[MAX_PATH] = { 0 }, pszConfigDBPath[MAX_PATH] = { 0 };
        WCHAR   pszQuorumName[MAX_PATH] = { 0 }, pszLogPath[MAX_PATH] = { 0 };
        R_DWORD dwNameSize = MAX_PATH, dwLogSize = MAX_PATH, dwLogMaxSize = 0;
        R_ULONG ulSize = 0, ulClusAware = 0;
        USES_CONVERSION;
        UNREF_CONV;

        /* ---------------------------------------------------------------------------
        |   load clusapi.dll library. if it cannot be loaded report that the cluster
        |   is not supported on this configuration.
         ---------------------------------------------------------------------------*/
        cf.hClusMod = LoadLibrary(CLUSAPI_DLL_NAME);
        if (NULL == cf.hClusMod)
            SRD_W32_LEAVE(
                {
                    Result = SRDERR_NOT_SUPPORTED;
                    DbgPlain(DBG_SYSNFO, _T("Cluster API library could not be loaded. Possibly unavailable on this system."));
                }
            );
        /* ---------------------------------------------------------------------------
        |   bind to required functions. if this cannot be accomplished report error -
        |   this should be an unexpected state with clusapi.dll available.
         ---------------------------------------------------------------------------*/
        cf.fcnOpen = (OpenClusterProc)GetProcAddress(cf.hClusMod, "OpenCluster");
        cf.fcnClose = (CloseClusterProc)GetProcAddress(cf.hClusMod, "CloseCluster");
        cf.fcnInfo = (GetClusterInfoProc)GetProcAddress(cf.hClusMod, "GetClusterInformation");
        cf.fcnState = (GetNodeClusterStateProc)GetProcAddress(cf.hClusMod, "GetNodeClusterState");
        cf.fcnQuorum = (GetClusterQuorumProc)GetProcAddress(cf.hClusMod, "GetClusterQuorumResource");
        cf.fcnOpenEnum = (ClusterOpenEnumProc)GetProcAddress(cf.hClusMod, "ClusterOpenEnum");
        cf.fcnCloseEnum = (ClusterCloseEnumProc)GetProcAddress(cf.hClusMod, "ClusterCloseEnum");
        cf.fcnEnum = (ClusterEnumProc)GetProcAddress(cf.hClusMod, "ClusterEnum");
        cf.fcnOpenResource = (OpenClusterResourceProc)GetProcAddress(cf.hClusMod, "OpenClusterResource");
        cf.fcnCloseResource = (CloseClusterResourceProc)GetProcAddress(cf.hClusMod, "CloseClusterResource");
        cf.fcnResourceControl = (ClusterResourceControlProc)GetProcAddress(cf.hClusMod, "ClusterResourceControl");
        cf.fcnResourceState = (GetClusterResourceStateProc)GetProcAddress(cf.hClusMod, "GetClusterResourceState");
        if (
            NULL == cf.fcnOpen ||
            NULL == cf.fcnClose ||
            NULL == cf.fcnInfo ||
            NULL == cf.fcnQuorum ||
            NULL == cf.fcnOpenEnum ||
            NULL == cf.fcnCloseEnum ||
            NULL == cf.fcnEnum ||
            NULL == cf.fcnOpenResource ||
            NULL == cf.fcnCloseResource ||
            NULL == cf.fcnResourceControl ||
            NULL == cf.fcnResourceState
        )
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cluster API library functions could not be bound. Wrong dll version ???.")); }
            );
        if (NULL == cf.fcnState)
        {
            DbgPlain(DBG_SYSNFO, _T("GetClusterNodeState() cannot be bound. Testing using OpenCluster()."));
            hCluster = cf.fcnOpen(NULL);
            if (NULL == hCluster)
                SRD_W32_LEAVE(
                    {
                        Result = SRDERR_NOT_SUPPORTED;
                        DbgPlain(DBG_SYSNFO, _T("OpenCluster() FAILED [error: %d]."), GetLastError());
                    }
                );
            if (NULL != cf.fcnClose)
                cf.fcnClose(hCluster);
            hCluster = NULL;
        }
        else
        {
            DbgPlain(DBG_SYSNFO, _T("GetClusterNodeState() successfully bound."));
            /* ---------------------------------------------------------------------------
            |   get current cluster node state. if this cannot be obtained assume that the
            |   cluster node (if at all) is in an unsupported state.
             ---------------------------------------------------------------------------*/
            dwWinErr = cf.fcnState(NULL, &dwState);
            if (dwWinErr != ERROR_SUCCESS)
                SRD_W32_LEAVE(
                    {
                        Result = SRDERR_NOT_SUPPORTED;
                        DbgPlain(DBG_SYSNFO, _T("GetNodeClusterState() FAILED [error: %d]."), dwWinErr);
                    }
                );
            switch(dwState)
            {
                case ClusterStateNotRunning:
                case ClusterStateRunning:
                    break;
                default:
                    SRD_W32_LEAVE(
                        {
                            Result = SRDERR_NOT_SUPPORTED;
                            DbgPlain(DBG_SYSNFO, _T("Unsupported cluster state: %d."), dwWinErr);
                        }
                    );
                    break;
            }
        }
        /* ---------------------------------------------------------------------------
        |   if we come here, the cluster node is in a supported state, so the information
        |   is going to be collected and stored as SRD info.
         ---------------------------------------------------------------------------*/
        sprintf(pszClusReg, _T("%s\\Cluster"), REGBASE);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszClusReg, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            /* ---------------------------------------------------------------------------
            |   if Cluster registry key is present in the local registry that we assume
            |   that this is cluster aware CM.
             ---------------------------------------------------------------------------*/
            RegCloseKey(hKey); hKey = NULL;
            pClusInfo->ClusterIsCM = 1;
        }
        else
            DbgPlain(DBG_SYSNFO, _T("NO Cluster key in registry. I am not clustered CM."));
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_OB2COMMON, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot access 'Common' registry path.")); }
            );
        /* ---------------------------------------------------------------------------
        |   verify if OmniBack enabled clustering in the registry.
         ---------------------------------------------------------------------------*/
        ulSize =  sizeof(R_ULONG);
        if (RegQueryValueEx(hKey, REGVAL_CLUSENABLED, NULL, NULL, (R_UCHAR*)&ulClusAware, &ulSize) != ERROR_SUCCESS)
            pClusInfo->ClusterAware = 0;
        else
            pClusInfo->ClusterAware = ulClusAware;
        /* ---------------------------------------------------------------------------
        |   get omniback DB config path
         ---------------------------------------------------------------------------*/
        ulSize =  sizeof(R_TCHAR) * MAX_PATH;
        if (RegQueryValueEx(hKey, REGVAL_OMNICFGDB, NULL, NULL, (R_UCHAR*)W2T(pszConfigDBPath), &ulSize) == ERROR_SUCCESS)
        {
            R_TCHAR *pszSlsh = NULL;

            pszSlsh = strrchr(W2T(pszConfigDBPath), _T('\\'));
            if (NULL != pszSlsh)
                *pszSlsh = _T('\0');
            pClusInfo->ConfigDBPath = StrNewCopy(W2T(pszConfigDBPath));
        }
        /* ---------------------------------------------------------------------------
        |   get cluster administrative name - quorum volume is backed up under this name.
         ---------------------------------------------------------------------------*/
        Result = SRDERR_SUCCESS;

        hCluster = cf.fcnOpen(NULL);
        if (NULL == hCluster)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cannot open current cluster - error %d."), GetLastError()); }
            );
        if (cf.fcnInfo(hCluster, pszClusterName, &dwNameSize, NULL) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cannot get the name of the cluster - error %d."), GetLastError()); }
            );
        pszHostName = W2T(pszClusterName);
        if (CmnExpandHostName(pszHostName, &pszExpName) != 1)
        {
            DbgPlain(DBG_SYSNFO, _T("Cannot expand cluster host name %s."), pszHostName);
            pszExpName = pszHostName;
        }
        pClusInfo->AdminVS = StrNewCopy(pszExpName);


        /* ---------------------------------------------------------------------------
        |   Added to populate the cluster resource type.
         ---------------------------------------------------------------------------*/
        memset(pszQuorumName,0,MAX_PATH*sizeof(WCHAR));
        memset(pszLogPath,0,MAX_PATH*sizeof(WCHAR));
        dwLogSize = MAX_PATH, dwLogMaxSize = MAX_PATH;
        dwNameSize = MAX_PATH;
        if (cf.fcnQuorum(hCluster, pszQuorumName, &dwNameSize, pszLogPath, &dwLogSize, &dwLogMaxSize) != ERROR_SUCCESS)
            SRD_W32_LEAVE(
            { DbgPlain(DBG_SYSNFO, _T("Cannot get the name of quorum resource - error %d."), GetLastError()); }
        );

        /* On Windows 2008 pszQuorumName will be empty in case of Majority Node Set Cluster. */
        DbgPlain(DBG_SYSNFO, _T("GetClusInfo: pszQuorumName = %s"), pszQuorumName);
        if (!strcmp(W2T(pszQuorumName),CLUS_STR_MNS) || !strcmp(W2T(pszQuorumName), _T("")))
        {
            pClusInfo->clusterType = CLUS_TYPE_MNS;
            DbgPlain(DBG_SYSNFO, _T("Majority Node Set Cluster."));
        }
        else if ((StrStrNS(pszQuorumName, CLUS_STR_MNFS)) != NULL)
        {
            pClusInfo->clusterType = CLUS_TYPE_MNFS;
            DbgPlain(DBG_SYSNFO, _T("File Share Witness detected."));
        }

        ClusterQueryNetworkNames(hCluster, &cf, network_names);
        if (network_names[0] != _T('\0'))
            pClusInfo->NetworkNames = StrNewCopy(network_names);

        ClusterQueryUnsupportedVolumes(hCluster, &cf, unsupported_vols, supported_vols);
        if(clus_Type == 1)
        {
            DbgPlain(DBG_SYSNFO,_T("setting type to MNS"));
            pClusInfo->clusterType = CLUS_TYPE_MNS;
        }
        else
        {
            DbgPlain(DBG_SYSNFO,_T("cluster type not set to MNS"));
        }
        if (unsupported_vols[0] != _T('\0'))
            pClusInfo->UnsupportedVols = StrNewCopy(unsupported_vols);
        if (supported_vols[0] != _T('\0'))
        {
            DbgPlain(DBG_SYSNFO,_T("len : %d\n"),StrLen(supported_vols));
            pClusInfo->SupportedVols = StrNewCopy(supported_vols);
        }
    }
    __finally
    {
        if (NULL != hCluster && NULL != cf.fcnClose)
            cf.fcnClose(hCluster);
        if (NULL != cf.hClusMod)
            FreeLibrary(cf.hClusMod);
        if (NULL  != hKey)
            RegCloseKey(hKey);
    }

    DbgFcnOut();

    return Result;
}

R_TCHAR*
AllocateRegistryString(HKEY hkKey, R_TCHAR *lpszValue, R_BOOL bMultiSz)
{
    R_TCHAR *pszRet = NULL;
    R_TCHAR *lpszWalk = NULL;
    R_TCHAR lpszString[1024] = { 0 };
    R_DWORD dwSize = 0;
    size_t  iLen = 0;

    dwSize = sizeof(R_TCHAR) * 1024;
    if (RegQueryValueEx(hkKey, lpszValue, NULL, NULL,
        (LPBYTE)lpszString, &dwSize) == ERROR_SUCCESS)
    {
        R_TCHAR     *pszStr = MALLOC(sizeof(R_TCHAR) * 1024);

        if (NULL != pszStr)
        {
            pszStr[0] = _T('\0');

            if (TRUE == bMultiSz)
            {
                iLen = strlen(lpszString);
                lpszWalk = lpszString;

                while(*lpszWalk != _T('\0'))
                {
                    strcat(pszStr, lpszWalk);
                    lpszWalk += iLen + 1;
                    if (*lpszWalk != _T('\0'))
                        strcat(pszStr, _T(","));
                }
            }
            else
                strcat(pszStr, lpszString);

            pszRet = pszStr;
        }
    }

    return pszRet;
}

typedef NET_API_STATUS (NET_API_FUNCTION *NetWkstaGetInfoProc)(LPWSTR, R_DWORD, LPBYTE*);
typedef NET_API_STATUS (NET_API_FUNCTION *NetApiBufferFreeProc)(R_VOID*);
#define NETINFONAME     "NetWkstaGetInfo"
#define NETFREENAME     "NetApiBufferFree"

R_RESULT
GetMachineDomain(R_TCHAR *pszOut)
{
    ERH_FUNCTION (_T("GetMachineDomain"));

    R_RESULT    retVal = SRDERR_ERROR;
    HANDLE      hModule = NULL;

    SRD_ENTER_FCN;

    __try
    {
        LPWKSTA_INFO_100        lpInfo = NULL;
        NetWkstaGetInfoProc     fnInfo = NULL;
        NetApiBufferFreeProc    fnFree = NULL;
        USES_CONVERSION;
        UNREF_CONV;

        hModule = LoadLibrary(_T("netapi32.dll"));
        if (NULL == hModule)
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot load Windows NET API library module."));
                }
            );

        fnInfo = (NetWkstaGetInfoProc)GetProcAddress(hModule, NETINFONAME);
        fnFree = (NetApiBufferFreeProc)GetProcAddress(hModule, NETFREENAME);
        if (
            NULL == fnInfo ||
            NULL == fnFree
        )
            SRD_W32_LEAVE(
                {
                    DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot bind to Windows NET API library functions."));
                }
            );

        if (fnInfo(NULL, 100, (LPBYTE*)&lpInfo) == NERR_Success)
        {
            WCHAR   lanGroup[128];

            if (lpInfo->wki100_langroup != NULL)
                wcscpy(lanGroup, lpInfo->wki100_langroup);
            else
                lanGroup[0] = L'\0';

            fnFree(lpInfo);

            if (lanGroup[0] == L'\0')
                SRD_W32_LEAVE(
                    {
                        DbgPlain(DBG_SYSNFO, _T("FAILURE - domain name cannot be obtained."));
                    }
                );
            strcpy(pszOut, W2T(lanGroup));
        }

        retVal = SRDERR_SUCCESS;
    }
    __finally
    {
        if (NULL != hModule)
            FreeLibrary(hModule);
    }

    SRD_RETURN_VAL(retVal);
}
#else /* if TARGET_LINUX*/
/** \fn strnstr(const R_TCHAR *s, const R_TCHAR *find, size_t slen)
    \brief locates the first occurrence of the null-terminated string needle
            in the string haystack, where not more than length characters
            are searched.
    \param s haystack
    \param find needle
    \param slen length characters for searched
    \return If needle is an empty string, haystack is returned;
            if needle occurs nowhere in haystack, NULL is returned;
            otherwise a pointer to the first character of the first occurrence
            of needle is returned.
*/
R_TCHAR *
strnstr(const R_TCHAR *s, const R_TCHAR *find, size_t slen)
{
    R_TCHAR c,
            sc;
    size_t len;

    if ((c = *find++) != '\0')
    {
        len = strlen(find);
        do
        {
            do
            {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);

            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((R_TCHAR *)s);
}

/** \fn IsMountPoint(R_TCHAR *pRPMPath)
    \brief Check path for mountpoint
    \param pRPMPath Path from RPM package or any other path
    \return If path is mountpoint return TRUE,
            otherwise return FALSE
*/
R_INT
IsMountPoint(R_TCHAR *pRPMPath)
{
    ERH_FUNCTION (_T("IsMountPoint"));
    struct stat st,
                st2;
    R_TCHAR     tBuf[STRLEN_STD] = { _T('\0') },
                strBuf[STRLEN_STD] = { _T('\0') };
    R_INT       Return=0;

    SRD_ENTER_FCN;

    _TRY_
        if (!pRPMPath || pRPMPath[0] == _T('\0'))
        {
            DbgPlain(DBG_SYSNFO, _T("   IsMountPoint() - Empty path"));
            _LEAVE_;
        }

        if (stat(pRPMPath, &st) != 0)
        {
            DbgPlain(DBG_SYSNFO, _T("   stat() failed for path %s."), pRPMPath);
            _LEAVE_;
        }

        if (!S_ISDIR(st.st_mode))
        {
            DbgPlain(DBG_SYSNFO, _T("   Path %s is not directory."), pRPMPath);
            _LEAVE_;
        }

        if (S_ISLNK(st.st_mode) ||
            IsSymLink(pRPMPath, strBuf))
        {
            DbgPlain(DBG_SYSNFO, _T("   Path %s is link."), pRPMPath);
            _LEAVE_;
        }

        memset(tBuf, 0, sizeof(tBuf));
        strncpy(tBuf, pRPMPath, sizeof(tBuf) - 4);
        strcat(tBuf, "/..");
        if (stat(tBuf, &st2) != 0)
        {
            DbgPlain(DBG_SYSNFO, _T("   stat() failed for path %s."), tBuf);
            _LEAVE_;
        }

        if ((st.st_dev != st2.st_dev) ||
            (st.st_dev == st2.st_dev && st.st_ino == st2.st_ino)
        )
            Return = 1;

        /**
         * mpfs is supported by the Linux kernel from version 2.4 and up
         * udev is not realy mount point
         */
        if (strcmp(pRPMPath, _T("/dev")) == 0 ||
            strcmp(pRPMPath, _T("/dev/")) == 0)
            Return = 0;

        DbgPlain(DBG_SYSNFO, _T("   Path %s is%s mountpoint."),
                                pRPMPath, ((Return == 1) ? _T("") : _T(" NOT")));
    _FINALLY_
    _ENDTRY_

    SRD_EXIT_FCN;

    return Return;
}

/** \fn IsSymLink(R_TCHAR *pPath, R_TCHAR *targetPath)
    \brief Check path for symbolic link
    \param pPath Path wich we check
    \param targetPath return target path of symbolic link
    \return If path is symbolic link return TRUE and target path,
            otherwise return FALSE
*/
R_INT
IsSymLink(R_TCHAR *pPath, R_TCHAR *targetPath)
{
    ERH_FUNCTION (_T("IsSymLink"));

    R_INT       Return = 0,
                len = 0;
    R_TCHAR     tgtPath[STRLEN_STD]={_T('\0')};

    SRD_ENTER_FCN;

    _TRY_
        if (!pPath || pPath[0] == _T('\0'))
            _LEAVE_;

        len = readlink(pPath, tgtPath, STRLEN_STD-1);
        if (len == -1)
        {
            if (errno == EINVAL)
            {
                DbgPlain(DBG_SYSNFO, _T("%s is not symbolic link."), pPath);
            }
            else
            {
                DbgPlain(DBG_SYSNFO, _T("readlink(): Cannot read symbolic"
                                        " link for %s [%d]."), pPath, errno);
                _LEAVE_;
            }
        }
        else
        {
            /**
             * NUL-terminate the target path.
             */
            if (strcmp(&tgtPath[len - 1], _T("/")) == 0)
            {
                /* If user created the link with the / at the end we
                need to remove this because GUI does that too */
                len -= 1;
            }
            tgtPath[len] = _T('\0');
            strcpy(targetPath, tgtPath);
            DbgPlain(DBG_SYSNFO, _T("%s->%s"), pPath, targetPath);

            Return = 1;
        }
    _FINALLY_
    _ENDTRY_

    SRD_EXIT_FCN;

    return Return;
}

/** \fn GetLnxSysDirectories(R_TCHAR *pSysRoot)
    \brief Collect linux system directories
    \param pSysRoot Return all system directories with delimiter ','
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
R_RESULT
GetLnxSysDirectories(R_TCHAR *pSysRoot)
{
    ERH_FUNCTION (_T("GetLnxSysDirectories"));
    R_RESULT            Result = SRDERR_ERROR;

    R_TCHAR             *Cmd = NULL;            /**< Command line*/
    R_TCHAR             tBuf[BUFSIZ];           /**< Buffer*/
    FILE                *pFile = NULL;          /**< File handler*/
    R_INT               ii=0,                   /**< Counter*/
                        fd=0;                   /**< File handler*/

    SRD_ENTER_FCN;

    _TRY_
        /**
         * Open SLES release file if exists
         */
        if ((fd = open(SLESRELEASE, O_RDONLY)) != SRDERR_ERROR)
        {
            Cmd = RPM_SUSE;
        }

        /**
         * Open RHEL release file if exists
         */
        else if ((fd = open(RHELRELEASE, O_RDONLY)) != SRDERR_ERROR)
        {
            Cmd = RPM_RHEL;
        }

        /**
         * Check dependencies program for RM query
         */
        if (OB2_StatFile(_T("/bin/grep"), NULL) != 0 &&
            OB2_StatFile(_T("/usr/bin/grep"), NULL) != 0 )
        {
            DbgPlain(DBG_SYSNFO, _T("   grep: command not found"));
            _LEAVE_;
        }

        if (OB2_StatFile(_T("/bin/cat"), NULL) != 0)
        {
            DbgPlain(DBG_SYSNFO, _T("   cat: command not found"));
            _LEAVE_;
        }

        if (OB2_StatFile(_T("/usr/bin/uniq"), NULL) != 0)
        {
            DbgPlain(DBG_SYSNFO, _T("   uniq: command not found"));
            _LEAVE_;
        }

        if (OB2_StatFile(_T("/bin/sort"), NULL) != 0 &&
            OB2_StatFile(_T("/usr/bin/sort"), NULL) != 0 )
        {
            DbgPlain(DBG_SYSNFO, _T("   sort: command not found"));
            _LEAVE_;
        }

        if (OB2_StatFile(_T("/bin/sed"), NULL) != 0 &&
            OB2_StatFile(_T("/usr/bin/sed"), NULL) != 0 )
        {
            DbgPlain(DBG_SYSNFO, _T("   sed: command not found"));
            _LEAVE_;
        }

        if (OB2_StatFile(_T("/bin/awk"), NULL) != 0 &&
            OB2_StatFile(_T("/usr/bin/awk"), NULL) != 0 )
        {
            DbgPlain(DBG_SYSNFO, _T("   awk: command not found"));
            _LEAVE_;
        }

        if ((pFile = popen(Cmd, _T("r"))) == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("   popen(): RPM query failed."));
            _LEAVE_;
        }

        /**
         * Read command output
         */
        while (fgets(tBuf, BUFSIZ, pFile) != NULL)
        {
            if (ii>0)
            {
                strcat(pSysRoot,_T(","));
                strcat(pSysRoot,SkipNewlineArr(tBuf));
            }
            else
                strcpy(pSysRoot,SkipNewlineArr(tBuf));
            ii++;
        }

        if (ii == 0)
            strcpy(pSysRoot, _T("/"));

        Result = SRDERR_SUCCESS;

    _FINALLY_
        /**
         * Destroy file handler
         */
        if (fd)
            close(fd);

        /**
         * Destroy file handler
         */
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    SRD_EXIT_FCN;

    return Result;
}

/** \fn GetHostnameByAddress(R_TCHAR *IpAddress, R_TCHAR *HostName)
    \brief Get Hostname by ip address
    \param IpAddress Ip address
    \param HostName Return hostname
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
R_RESULT
GetHostnameByAddress(R_TCHAR *IpAddress, R_TCHAR *HostName)
{
    ERH_FUNCTION (_T("GetHostnameByAddress"));

    R_RESULT Result = SRDERR_ERROR;
    BOOL ipc = FALSE;

    SRD_ENTER_FCN;

    _TRY_
        tchar hostname[STRLEN_HOSTNAME + 1];

        DbgPlain(DBG_SYSNFO, _T("Try to get a hostname for IP address: %s"),
                                IpAddress);

        if (!IpcIsInitialized())
        {
            IpcInit();
            ipc = TRUE;
        }

        IpcGetHostname(IpAddress, hostname);
        if (StrIsEmpty(hostname))
        {
            DbgPlain(DBG_SYSNFO, _T("Failed to convert IP address %s to hostname."), IpAddress);
            _LEAVE_;
        }

        strcpy(HostName, hostname);
        
        Result = SRDERR_SUCCESS;
    _FINALLY_
        if (Result == SRDERR_SUCCESS)
        {
            DbgPlain(DBG_SYSNFO, _T("Successfully converted IP address %s to hostname %s."), IpAddress, HostName);
        }
        if (ipc)
        {
            IpcExit();
        }
    _ENDTRY_

    SRD_EXIT_FCN;

    return(Result);
}

/** \fn GetClusterSuite(R_VOID)
    \brief Check Cluster suite
    \return Cluster suite type
*/
clusterType
GetClusterSuite(R_VOID)
{
    /**
     * Check for serviceguard cluster system
     */
    if (OB2_StatFile(_T("/opt/cmcluster/bin/cmviewcl"), NULL) == 0 &&
        OB2_StatFile(_T("/opt/cmcluster/bin/cmgetconf"), NULL) == 0
    )
        return CLUS_TYPE_HP_SG_SLES;
    else if (OB2_StatFile(_T("/usr/local/cmcluster/bin/cmviewcl"), NULL) == 0 &&
        OB2_StatFile(_T("/usr/local/cmcluster/bin/cmgetconf"), NULL) == 0
    )
        return CLUS_TYPE_HP_SG_RHEL;

/**
 * In this moment DP support only HP Service Guard Cluster System
 */
#if 0
    /**
     * Check for heartbeat v2 cluster system
     */
    else if (OB2_StatFile(_T("/usr/sbin/crmadmin"), NULL) == 0 &&
            OB2_StatFile(_T("/usr/bin/cl_status"), NULL) == 0 &&
            OB2_StatFile(_T("/var/lib/heartbeat/crm/cib.xml"), NULL) == 0
    )
        return CLUS_TYPE_SLES_HA_V2;
    /**
     * Check for heartbeat cluster system
     */
    else if (OB2_StatFile(_T("/usr/bin/cl_status"), NULL) == 0 &&
            OB2_StatFile(_T("/etc/ha.d/ha.cf"), NULL) == 0 &&
            OB2_StatFile(_T("/etc/ha.d/haresources"), NULL) == 0
    )
        return CLUS_TYPE_SLES_HA_V1;
    /**
     * Check for RHEL Cluster Suite cluster system
     */
    else if ((OB2_StatFile(_T("/sbin/cman_tool"), NULL) == 0 ||
            OB2_StatFile(_T("/usr/sbin/cman_tool"), NULL) == 0) &&
            OB2_StatFile(_T("/usr/sbin/clustat"), NULL) == 0 &&
            OB2_StatFile(_T("/etc/cluster/cluster.conf"), NULL) == 0
    )
        return CLUS_TYPE_RHEL_CS;
#endif
    else
        return CLUS_TYPE_UNKNOWN;
}

/** \fn GetShareVolumesSG(PSERVICEGUARD *ppServiceGuard, R_INT clType)
    \brief Get all share volumes on HP ServiceGuard Cluster Suite
    \param pServiceGuard SRD Structure with SG Cluster Information
    \param clType Type Cluster Suite
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetShareVolumesSG(PSERVICEGUARD *ppServiceGuard, R_INT clType)
{
    ERH_FUNCTION (_T("GetShareVolumesSG"));
    R_RESULT Result = SRDERR_ERROR;

    FILE    *pFile1 = NULL,     /**< File handler for get packages*/
            *pFile2 = NULL,     /**< File handler for get conf file*/
            *pFile3 = NULL,     /**< File handler for parse conf file*/
            *pFile4 = NULL;     /**< File handler for parse share vol*/

    SRD_ENTER_FCN;

    _TRY_
        R_TCHAR line[BUFSIZ] = { _T('\0') },                    /**< line from buffer*/
                Package[STRLEN_STD] = { _T('\0') },             /**< package name*/
                Status[STRLEN_STD] = { _T('\0') },              /**< status of package*/
                State[STRLEN_STD] = { _T('\0') },               /**< state of package*/
                AutoRun[STRLEN_STD] = { _T('\0') },             /**< autorun parameter*/
                Node[STRLEN_STD] = { _T('\0') },                /**< package owner*/
                *NodeLong = NULL,                               /**< hostname.domain for package owner*/
                HostName[STRLEN_STD] = { _T('\0') },            /**< hostname.domain for package owner*/
                GetConfCMD[STRLEN_STD] = { _T('\0') },          /**< conf file*/
                PkgScriptPath[STRLEN_STD] = { _T('\0') },       /**< pkg script path*/
                ParseScriptPathCMD[STRLEN_STD] = { _T('\0') },  /**< parse pkg script file*/
                ParseScriptAddressCMD[STRLEN_STD] = { _T('\0') },/**< parse pkg script file for ip address*/
                ParseLogVolsCMD[STRLEN_STD] = { _T('\0') },     /**< parse pkg script for share vol*/
                Var[STRLEN_STD] = { _T('\0') },                 /**< variable for share vols*/
                Val[STRLEN_STD] = { _T('\0') },                 /**< value for share vols*/
                Count[STRLEN_STD] = { _T('\0') },               /**< number of share vol in one package*/
                TmpVar[STRLEN_STD] = { _T('\0') },              /**< temp variable*/
                TmpVar1[STRLEN_STD] = { _T('\0') };             /**< temp variable*/
        R_INT   ii=0,       /**< counter for packages*/
                jj=0,       /**< counter for share vols*/
                kk=0,       /**< counter for share vols*/
                ll=0;       /**< counter for ip addresses*/

        /**
         * Get package(s)
         */
        if (clType == CLUS_TYPE_HP_SG_SLES)
        {
            DbgPlain(DBG_SYSNFO, _T("Running command: %s"), GET_PACKAGES_SLES);
            if ((pFile1 = popen(GET_PACKAGES_SLES, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_PACKAGES_SLES);
                _LEAVE_;
            }
        }
        else
        {
            DbgPlain(DBG_SYSNFO, _T("Running command: %s"), GET_PACKAGES_RHEL);
            if ((pFile1 = popen(GET_PACKAGES_RHEL, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_PACKAGES_RHEL);
                _LEAVE_;
            }
        }

        /**
         * Parse command output
         */
        (*ppServiceGuard)->PackageCount = 0;
        while (fgets(line, sizeof(line), pFile1))
        {
            DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
            if (sscanf(line, " %s %s %s %s %[^\n ]", Package,
                                                    Status,
                                                    State,
                                                    AutoRun,
                                                    Node) != 5)
                continue;

            /**
             * Reset IP Addres counter for package
             * Each package has its own addresses
             */
            ll=0;

            (*ppServiceGuard)->SGPackages[ii] = MALLOC(sizeof(SGPACKAGES));
            if (NULL == (*ppServiceGuard)->SGPackages[ii])
            {
                DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                Result = SRDERR_MEMORY_ALLOCATION;
                _LEAVE_;
            }

            /**
             * If Multinode package
             */
            if (strcmp(Node, _T("no")) == 0 ||
                strcmp(Node, _T("yes")) == 0)
                strcpy(Node, StrNewCopy(cmnNodename));

            if (CmnExpandHostName(Node, &NodeLong) != 1)
            {
                strcpy(Node, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand node host"
                                        " name %s."), StrNewCopy(Node));
            }
            else
            {
                strcpy(Node, NodeLong);
                FREE(NodeLong);
            }

            (*ppServiceGuard)->SGPackages[ii]->Package = StrNewCopy(Package);
            (*ppServiceGuard)->SGPackages[ii]->PkgStatus = StrNewCopy(Status);
            (*ppServiceGuard)->SGPackages[ii]->State = StrNewCopy(State);
            (*ppServiceGuard)->SGPackages[ii]->AutoRun = StrNewCopy(AutoRun);
            (*ppServiceGuard)->SGPackages[ii]->Node = StrNewCopy(Node);

            /**
             * Parse output for RUN_SCRIPT path
             */
            if (clType == CLUS_TYPE_HP_SG_SLES)
                sprintf(GetConfCMD, DUMP_CONF_18_SLES,
                                (*ppServiceGuard)->SGPackages[ii]->Package);
            else
                sprintf(GetConfCMD, DUMP_CONF_18_RHEL,
                                (*ppServiceGuard)->SGPackages[ii]->Package);

            DbgPlain(DBG_SYSNFO, _T("Running command: %s"), GetConfCMD);
            if ((pFile2 = popen(GetConfCMD, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GetConfCMD);
                Result = SRDERR_ERROR;
                _LEAVE_;
            }

            /**
             * Parse output
             */
            strcpy(PkgScriptPath, _T(""));
            while (fgets(line, sizeof(line), pFile2))
            {
                DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
                if (sscanf(line, "%s %[^\n ]", TmpVar,
                                                PkgScriptPath) != 2)
                    continue;
            }

            /**
             * Get Share volume(s) from config file
             * Check for exists package run script file
             */
            if (OB2_StatFile(PkgScriptPath, NULL) == 0)
            {
                sprintf(ParseScriptPathCMD, PARSE_CONF_18, PkgScriptPath);
                DbgPlain(DBG_SYSNFO, _T("Running command: %s"), ParseScriptPathCMD);
                if ((pFile3 = popen(ParseScriptPathCMD, _T("r"))) == NULL)
                {
                    DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                            "failed."), ParseScriptPathCMD);
                    Result = SRDERR_ERROR;
                    _LEAVE_;
                }

                /**
                 * Parse output
                 */
                (*ppServiceGuard)->SGPackages[ii]->ShareVolCount = 0;
                kk = 0;
                jj=0;
                while (fgets(line, sizeof(line), pFile3))
                {
                    DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
                    if (sscanf(line, "%[^'['][%[0-9]%[^'=']=%[^\n]",
                                        Var, Count, TmpVar1, Val) != 4)
                        continue;

                    DbgPlain(DBG_SYSNFO, _T("After parsing: %s[%s]=%s"), Var, Count, Val);

                    /**
                     * Each volume have 6 parameters
                     */
                    if (kk%6 == 0)
                    {
                        if (kk != 0)
                            jj++;

                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj] =
                                        MALLOC(sizeof(SGSHAREVOLUMES));
                        if (NULL == (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj])
                        {
                            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                            Result = SRDERR_MEMORY_ALLOCATION;
                            _LEAVE_;
                        }

                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->LogicalVolume = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->MountPoint = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsType = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsMountOpt = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsUMountOpt = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsFSCKOpt = NULL;

                        (*ppServiceGuard)->SGPackages[ii]->ShareVolCount = jj+1;
                    }

                    if (strcasecmp(Var, _T("LV")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->LogicalVolume =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("FS")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->MountPoint =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("FS_TYPE")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsType =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("FS_MOUNT_OPT")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsMountOpt =
                                        StrNewCopy(Val);
                    if (strcasecmp(Var, _T("FS_UMOUNT_OPT")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsUMountOpt =
                                        StrNewCopy(Val);
                    if (strcasecmp(Var, _T("FS_FSCK_OPT")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsFSCKOpt =
                                        StrNewCopy(Val);

                    kk++;
                }

                /**
                 * Collect IP addresses of packages
                 */
                sprintf(ParseScriptAddressCMD, PARSE_CONF_FOR_ADDRESS_18, PkgScriptPath);
                DbgPlain(DBG_SYSNFO, _T("Running command: %s"), ParseScriptAddressCMD);
                if ((pFile3 = popen(ParseScriptAddressCMD, _T("r"))) == NULL)
                {
                    DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                            "failed."), ParseScriptAddressCMD);
                    Result = SRDERR_ERROR;
                    _LEAVE_;
                }

                /**
                 * Parse output
                 */
                (*ppServiceGuard)->SGPackages[ii]->AddressCount = 0;
                while (fgets(line, sizeof(line), pFile3))
                {
                    DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
                    if (sscanf(line, "%[^'['][%[0-9]%[^'=\"']=\"%[^\"\n ]",
                                        Var, Count, TmpVar1, Val) != 4)
                        continue;

                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll] =
                                    MALLOC(sizeof(SGPACKAGEADDRES));
                    if (NULL == (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll])
                    {
                        DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                        Result = SRDERR_MEMORY_ALLOCATION;
                        _LEAVE_;
                    }

                    if (GetHostnameByAddress(StrTrimQuotes(Val), HostName) != SRDERR_SUCCESS)
                    {
                        DbgPlain(DBG_SYSNFO, _T("GetHostnameByAddress error."));
                        _LEAVE_;
                    }

                    DbgPlain(DBG_SYSNFO, _T("IpAddress: %s"), StrTrimQuotes(Val));
                    DbgPlain(DBG_SYSNFO, _T("HostName: %s"),HostName);

                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll]->IpAddress =
                                        StrNewCopy(StrTrimQuotes(Val));
                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll]->HostName =
                                        StrNewCopy(HostName);

                    ll++;
                }
                (*ppServiceGuard)->SGPackages[ii]->AddressCount = ll;
            }
            /**
             * Configuration package only in one file.
             */
            else
            {
                if (clType == CLUS_TYPE_HP_SG_SLES)
                    sprintf(ParseLogVolsCMD, GET_LOG_VOLS_16_18_SLES,
                            (*ppServiceGuard)->SGPackages[ii]->Package);
                else
                    sprintf(ParseLogVolsCMD, GET_LOG_VOLS_16_18_RHEL,
                            (*ppServiceGuard)->SGPackages[ii]->Package);

                DbgPlain(DBG_SYSNFO, _T("Running command: %s"), ParseLogVolsCMD);
                if ((pFile4 = popen(ParseLogVolsCMD, _T("r"))) == NULL)
                {
                    DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                            "failed."), ParseLogVolsCMD);
                    Result = SRDERR_ERROR;
                    _LEAVE_;
                }

                jj=0;
                kk=0;
                /**
                 * Parse output
                 */
                while (fgets(line, sizeof(line), pFile4))
                {
                    DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
                    if (sscanf(line, " %s %[^\n ]", Var, Val) != 2)
                        continue;

                    /**
                     * Each volume have 6 parameters
                     */
                    if (kk%6 == 0)
                    {
                        if (kk != 0)
                            jj++;

                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj] =
                                        MALLOC(sizeof(SGSHAREVOLUMES));
                        if (NULL == (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj])
                        {
                            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                            Result = SRDERR_MEMORY_ALLOCATION;
                            _LEAVE_;
                        }

                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->LogicalVolume = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->MountPoint = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsType = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsMountOpt = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsUMountOpt = NULL;
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsFSCKOpt = NULL;

                        (*ppServiceGuard)->SGPackages[ii]->ShareVolCount = jj+1;
                    }

                    if (strcasecmp(Var, _T("fs_name")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->LogicalVolume =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("fs_directory")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->MountPoint =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("fs_type")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsType =
                                        StrNewCopy(StrTrimQuotes(Val));
                    if (strcasecmp(Var, _T("fs_mount_opt")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsMountOpt =
                                        StrNewCopy(Val);
                    if (strcasecmp(Var, _T("fs_umount_opt")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsUMountOpt =
                                        StrNewCopy(Val);
                    if (strcasecmp(Var, _T("fs_fsck_opt")) == 0)
                        (*ppServiceGuard)->SGPackages[ii]->SGShareVolumes[jj]->FsFSCKOpt =
                                        StrNewCopy(Val);

                    kk++;
                }

                /**
                 * Collect IP addresses of packages
                 */
                if (clType == CLUS_TYPE_HP_SG_SLES)
                    sprintf(ParseScriptAddressCMD, GET_IP_ADDRESS_16_18_SLES,
                            (*ppServiceGuard)->SGPackages[ii]->Package);
                else
                    sprintf(ParseScriptAddressCMD, GET_IP_ADDRESS_16_18_RHEL,
                            (*ppServiceGuard)->SGPackages[ii]->Package);

                DbgPlain(DBG_SYSNFO, _T("Running command: %s"), ParseScriptAddressCMD);
                if ((pFile4 = popen(ParseScriptAddressCMD, _T("r"))) == NULL)
                {
                    DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                            "failed."), ParseScriptAddressCMD);
                    Result = SRDERR_ERROR;
                    _LEAVE_;
                }

                /**
                 * Parse output
                 */
                (*ppServiceGuard)->SGPackages[ii]->AddressCount = 0;
                while (fgets(line, sizeof(line), pFile4))
                {
                    DbgPlain(DBG_SYSNFO, _T("Parse line: %s"), line);
                    if (sscanf(line, " %s %[^\n ]", Var, Val) != 2)
                        continue;

                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll] =
                                    MALLOC(sizeof(SGPACKAGEADDRES));
                    if (NULL == (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll])
                    {
                        DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                        Result = SRDERR_MEMORY_ALLOCATION;
                        _LEAVE_;
                    }

                    if (GetHostnameByAddress(StrTrimQuotes(Val), HostName) != SRDERR_SUCCESS)
                    {
                        DbgPlain(DBG_SYSNFO, _T("GetHostnameByAddress error."));
                        _LEAVE_;
                    }

                    DbgPlain(DBG_SYSNFO, _T("IpAddress: %s"), StrTrimQuotes(Val));
                    DbgPlain(DBG_SYSNFO, _T("HostName: %s"),HostName);

                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll]->IpAddress =
                                        StrNewCopy(StrTrimQuotes(Val));
                    (*ppServiceGuard)->SGPackages[ii]->SGPackageAddress[ll]->HostName =
                                        StrNewCopy(HostName);

                    ll++;
                }
                (*ppServiceGuard)->SGPackages[ii]->AddressCount = ll;
            }
            ii++;
        }

        (*ppServiceGuard)->PackageCount = ii;

        Result = SRDERR_SUCCESS;
    _FINALLY_
        /**
         * Destroy file handler
         */
        if (pFile1)
            pclose(pFile1);
        if (pFile2)
            pclose(pFile2);
        if (pFile3)
            pclose(pFile3);
        if (pFile4)
            pclose(pFile4);
    _ENDTRY_

    SRD_EXIT_FCN;

    return Result;
}

/** \fn GetSupUnsupVolumesSG(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
    \brief Collect supported/unsupported share volumes
    \param pServiceGuard SRD Structure with SG Cluster information
    \param pClusInfo SRD Structure with Cluster Information
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSupUnsupVolumesSG(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
{
    ERH_FUNCTION (_T("GetSupUnsupVolumesSG"));

    R_RESULT Result = SRDERR_ERROR;

    R_INT   ii=0,   /**< Package counter*/
            jj=0,   /**< Share volume counter*/
            kk=0,   /**< Adress counter*/
            ll=0,   /**< Supported volume counter*/
            mm=0;   /**< Unsupported volume counter*/
    R_TCHAR SupportedVols[STRLEN_48K] = { _T('\0') },   /**< list supported vols*/
            UnsupportedVols[STRLEN_48K] = { _T('\0') }; /**< list unsupported vols*/

    SRD_ENTER_FCN;

    for (ii=0; ii<pServiceGuard->PackageCount; ii++)
    {
        for (jj=0; jj<pServiceGuard->SGPackages[ii]->ShareVolCount; jj++)
        {
            for (kk=0; kk<pServiceGuard->SGPackages[ii]->AddressCount; kk++)
            {
                if (SUPPORTED_FS_TYPE(pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->FsType) &&
                    StrCmp(pServiceGuard->SGPackages[ii]->Node, _T("unowned")) != 0)
                {
                    if (ll > 0)
                    {
                        strcat(SupportedVols, _T(","));
                        strcat(SupportedVols,
                                pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint);
                    }
                    else
                    {
                        StrCpy(SupportedVols,
                                pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint);
                    }

                    strcat(SupportedVols, _T(","));
                    if (pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName != NULL &&
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName[0] != _T('\0'))
                        strcat(SupportedVols,
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName);
                    else
                        strcat(SupportedVols,
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->IpAddress);

                    DbgPlain(DBG_SYSNFO, _T("Supported volume: %s"), SupportedVols);

                    ll++;
                }
                else
                {
                    if (mm > 0)
                    {
                        strcat(UnsupportedVols, _T(","));
                        strcat(UnsupportedVols,
                                pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint);
                    }
                    else
                    {
                        StrCpy(UnsupportedVols,
                                pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint);
                    }

                    strcat(UnsupportedVols, _T(","));
                    if (pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName != NULL &&
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName[0] != _T('\0'))
                        strcat(UnsupportedVols,
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName);
                    else
                        strcat(UnsupportedVols,
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->IpAddress);

                    DbgPlain(DBG_SYSNFO, _T("Unsupported volume: %s"), UnsupportedVols);

                    mm++;
                }
            }
        }
    }

    pClusInfo->SupportedVols = StrNewCopy(SupportedVols);
    pClusInfo->UnsupportedVols = StrNewCopy(UnsupportedVols);

    Result = SRDERR_SUCCESS;

    SRD_EXIT_FCN;

    return Result;
}

/** \fn IsClusterAware(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
    \brief Check for cluster aware
    \param pServiceGuard SRD Structure with SG Cluster information
    \param pClusInfo SRD Structure with Cluster Information
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
IsClusterAwareSG(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
{
    ERH_FUNCTION (_T("IsClusterAwareSG"));

    R_RESULT Result = SRDERR_ERROR;

    R_TCHAR TmpVar[STRLEN_STD] = { _T('\0') },              /**< Temp var*/
            VarSymlinkBuffer[STRLEN_STD] = { _T('\0') },    /**< /var symlink*/
            EtcSymlinkBuffer[STRLEN_STD] = { _T('\0') },    /**< /etc symlink*/
            *ptr = NULL;                                    /**< tmp pointer*/

    R_INT   ii=0,       /**< package counter*/
            jj=0;       /**< share vols counter*/

    SRD_ENTER_FCN;

    pClusInfo->ClusterAware = 0;

    /**
     * Check if DP is cluster aware
     * /var/opt/omni/server and /etc/opt/omni/server must be symbolic
     * link on directory on the share volume of ServiceGuard
     */
    if (IsSymLink(_T("/var/opt/omni/server"), VarSymlinkBuffer) &&
        IsSymLink(_T("/etc/opt/omni/server"), EtcSymlinkBuffer)
    )
    {
        strcpy(TmpVar, VarSymlinkBuffer);
        if (strstr(TmpVar, _T("/")))
        {
            while ((ptr = strrchr(TmpVar, '/')) != NULL)
            {
                *ptr = _T('\0');
                for (ii=0; ii<pServiceGuard->PackageCount; ii++)
                {
                    for (jj=0; jj<pServiceGuard->SGPackages[ii]->ShareVolCount; jj++)
                    {
                        if (strcmp(pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint,
                                    TmpVar) == 0)
                        {
                            pClusInfo->ConfigDBPath = StrNewCopy(TmpVar);
                            pClusInfo->ClusterAware = 1;
                        }
                    }
                }
            }
        }
    }

    Result = SRDERR_SUCCESS;

    SRD_EXIT_FCN;

    return Result;
}

#if 0
/** \fn GetNodesSG(PCLUSINFO pClusInfo, R_INT clType)
    \brief Collect nodes hostname
    \param pClusInfo Return SRD structure with cluster informations
    \param clType Type Cluster Suite
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetNodesSG(PCLUSINFO pClusInfo, R_INT clType)
{
    ERH_FUNCTION (_T("GetNodesSG"));

    R_RESULT    Result = SRDERR_ERROR;

    FILE    *pFile = NULL;                      /**< File handler*/
    R_INT   ii=0;                               /**< Counter*/
    R_TCHAR line[BUFSIZ] = { _T('\0') },        /**< line by line from buffer*/
            Status[STRLEN_STD] = { _T('\0') },  /**< Status of node*/
            State[STRLEN_STD] = { _T('\0') },   /**< State of node*/
            Node[STRLEN_STD] = { _T('\0') },    /**< hostname of node*/
            *NodeLong = NULL,                   /**< hostname with domain*/
            TmpNetNames[STRLEN_16K] = { _T('\0') };

    SRD_ENTER_FCN;

    _TRY_
        if (clType == CLUS_TYPE_HP_SG_SLES)
        {
            if ((pFile = popen(GET_NODES_SLES, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_NODES_SLES);
                _LEAVE_;
            }
        }
        else
        {
            if ((pFile = popen(GET_NODES_RHEL, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_NODES_RHEL);
                _LEAVE_;
            }
        }

        while (fgets(line, sizeof(line), pFile))
        {
            if (sscanf(line, " %s %s %[^\n ]", Node,
                                                Status,
                                                State) != 3)
                continue;

            if (CmnExpandHostName(Node, &NodeLong) != 1)
            {
                strcpy(Node, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand node hostname %s."),
                                        Node);
            }
            else
            {
                strcpy(Node, NodeLong);
                FREE(NodeLong);
            }

            if (ii > 0)
            {
                strcat(TmpNetNames, _T(","));
                strcat(TmpNetNames, Node);
            }
            else
                strcpy(TmpNetNames, StrNewCopy(Node));
            ii++;
        }

        pClusInfo->NetworkNames = StrNewCopy(TmpNetNames);

        Result = SRDERR_SUCCESS;

    _FINALLY_
        /** Destroy file handler*/
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    SRD_EXIT_FCN;

    return Result;
}
#endif

/** \fn GetVirtualHostnameSG(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
    \brief Collect virtual hostname for all packages
    \param pServiceGuard SRD Structure with SG Cluster information
    \param pClusInfo Return SRD structure with cluster informations
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetVirtualHostnameSG(PSERVICEGUARD pServiceGuard, PCLUSINFO pClusInfo)
{
    ERH_FUNCTION (_T("GetVirtualHostnameSG"));

    R_RESULT    Result = SRDERR_ERROR;

    R_INT   ii=0,   /**< Package counter*/
            jj=0,   /**< Address counter*/
            kk=0;   /**< Hostname/IpAddress of the packages counter*/
    R_TCHAR TmpVirtualHostNames[STRLEN_16K] = { _T('\0') };

    SRD_ENTER_FCN;

    for (ii=0; ii<pServiceGuard->PackageCount; ii++)
    {
        for (jj=0; jj<pServiceGuard->SGPackages[ii]->AddressCount; jj++)
        {
            if (kk > 0)
            {
                strcat(TmpVirtualHostNames,_T(","));
                if (pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName != NULL &&
                    pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName[0] != _T('\0'))
                    strcat(TmpVirtualHostNames,
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName);
                else
                    strcat(TmpVirtualHostNames,
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->IpAddress);
            }
            else
            {
                if (pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName != NULL &&
                    pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName[0] != _T('\0'))
                    strcpy(TmpVirtualHostNames,
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->HostName);
                else
                    strcpy(TmpVirtualHostNames,
                        pServiceGuard->SGPackages[ii]->SGPackageAddress[jj]->IpAddress);
            }

            kk++;
        }
    }

    pClusInfo->NetworkNames = StrNewCopy(TmpVirtualHostNames);

    Result = SRDERR_SUCCESS;

    SRD_EXIT_FCN;

    return Result;
}

#if 0
/** \fn GetQuorumSG(PCLUSINFO pClusInfo, R_INT clType)
    \brief Get Quorum hostname
    \param pClusInfo Return SRD structure with cluster informations
    \param clType Type Cluster Suite
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetQuorumSG(PCLUSINFO pClusInfo, R_INT clType)
{
    ERH_FUNCTION (_T("GetQuorumSG"));

    R_RESULT    Result = SRDERR_ERROR;

    FILE    *pFile = NULL;                      /**< File handler*/
    R_TCHAR line[BUFSIZ] = { _T('\0') },        /**< line by line from buffer*/
            TmpVar[STRLEN_STD] = { _T('\0') },  /**< Status of node*/
            Quorum[STRLEN_STD] = { _T('\0') },  /**< Quorum hostname*/
            *QuorumLong = NULL,                 /**< hostname with domain*/
            TmpNetNames[STRLEN_16K] = { _T('\0') };

    SRD_ENTER_FCN;

    _TRY_
        if (clType == CLUS_TYPE_HP_SG_SLES)
        {
            if ((pFile = popen(GET_QS_SLES, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_QS_SLES);
                _LEAVE_;
            }
        }
        else
        {
            if ((pFile = popen(GET_QS_RHEL, _T("r"))) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                        "failed."), GET_QS_RHEL);
                _LEAVE_;
            }
        }

        strcpy(TmpNetNames, pClusInfo->NetworkNames);

        while (fgets(line, sizeof(line), pFile))
        {
            if (sscanf(line, " %s %[^\n ]", TmpVar,
                                            Quorum) != 2)
                continue;

            if (CmnExpandHostName(Quorum, &QuorumLong) != 1)
            {
                strcpy(Quorum, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand quorum host name %s."),
                                        Quorum);
            }
            else
            {
                strcpy(Quorum, QuorumLong);
                FREE(QuorumLong);
            }

            strcat(TmpNetNames, _T(","));
            strcat(TmpNetNames, Quorum);
        }

        pClusInfo->NetworkNames = StrNewCopy(TmpNetNames);

        Result = SRDERR_SUCCESS;

    _FINALLY_
        /** Destroy file handler*/
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    SRD_EXIT_FCN;

    return Result;
}
#endif

/** \fn GetNodesHAV2(PCLUSINFO pClusInfo, ezxml_t ezXml)
    \brief Collect nodes hostname
    \param pClusInfo Return SRD structure with cluster informations
    \param ezXml XML file structure handler
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetNodesHAV2(PCLUSINFO pClusInfo, ezxml_t ezXml)
{
    R_RESULT    Result = SRDERR_ERROR;

    ezxml_t Nodes,                              /**< 'nodes' child*/
            Node;                               /**< 'node' child*/
    R_TCHAR TmpNetNames[BUFSIZ] = { _T('\0') }, /**< temp net names*/
            NodeName[BUFSIZ] = { _T('\0') },    /**< node hostname*/
            *NodeLong = NULL;                   /**< hostname with domain*/
    R_INT   ii=0;                               /**< Counter*/

    /** Get Nodes tree from XML file*/
    Nodes = ezxml_get(ezXml, "configuration", 0, "nodes", -1);
    for (Node = ezxml_child(Nodes, _T("node"));
            Node; Node = Node->next)
    {
        /** Expand node hostname to hostname with domain*/
        if (CmnExpandHostName(ezxml_attr(Node, _T("uname")), &NodeLong) != 1)
        {
            strcpy(NodeName, cmnNodename);
            DbgPlain(DBG_SYSNFO, _T("Cannot expand node host name %s."),
                                    NodeName);
        }
        else
        {
            strcpy(NodeName, NodeLong);
            FREE(NodeLong);
        }

        if (ii>0)
        {
            strcat(TmpNetNames, _T(","));
            strcat(TmpNetNames, NodeName);
        }
        else
            strcpy(TmpNetNames, NodeName);
        ii++;
    }

    pClusInfo->NetworkNames = StrNewCopy(TmpNetNames);

    Result = SRDERR_SUCCESS;

    return Result;
}


/** \fn GetShareVolumesHAV2(PHARESOURCES *ppHAResources)
    \brief Collect all share volumes
    \param pHAResources Return SRD structure with share volumes
    \param ezXml XML file structure handler
    \return SRDERR_SUCCESS OR SRDERR_ERROR
*/
static R_RESULT
GetShareVolumesHAV2(PHARESOURCES *ppHAResources, ezxml_t ezXml)
{
    R_RESULT Result = SRDERR_ERROR;

    ezxml_t Resources,                          /**< 'resources' child*/
            Clone,                              /**< 'clone' child*/
            Attributes,                         /**< 'attributes' child*/
            Nvpair;                             /**< 'nvpair' child*/
    R_INT   ii=0;                               /**< Counter*/

    _TRY_
        /** Get resources - clone*/
        Resources = ezxml_get(ezXml, "configuration", 0, "resources", -1);
        for (Clone = ezxml_child(Resources, _T("clone"));
                Clone; Clone = Clone->next)
        {
            (*ppHAResources)->HAShareVolumes[ii] =
                            MALLOC(sizeof(HASHAREVOLUMES));
            if (NULL == (*ppHAResources)->HAShareVolumes[ii])
            {
                DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                Result = SRDERR_MEMORY_ALLOCATION;
                _LEAVE_;
            }

            /** Get attributes of the resource*/
            Attributes = ezxml_get(Clone, "primitive", 0,
                            "instance_attributes", 0, "attributes", -1);

            /** Get share volume(s)*/
            for (Nvpair = ezxml_child(Attributes, _T("nvpair"));
                    Nvpair; Nvpair = Nvpair->next)
            {
                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("device")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->LogicalVolume =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));

                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("directory")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->MountPoint =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));

                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("fstype")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->FsType =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));
            }

            if ((*ppHAResources)->HAShareVolumes[ii]->LogicalVolume != NULL &&
                (*ppHAResources)->HAShareVolumes[ii]->MountPoint != NULL &&
                (*ppHAResources)->HAShareVolumes[ii]->FsType != NULL
                )
                ii++;
        }

        /** Get resource - group*/
        for (Clone = ezxml_child(Resources, _T("group"));
                Clone; Clone = Clone->next)
        {
            (*ppHAResources)->HAShareVolumes[ii] =
                            MALLOC(sizeof(HASHAREVOLUMES));
            if (NULL == (*ppHAResources)->HAShareVolumes[ii])
            {
                DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                Result = SRDERR_MEMORY_ALLOCATION;
                _LEAVE_;
            }

            /** Get attributes of the resource*/
            Attributes = ezxml_get(Clone, "primitive", 0,
                            "instance_attributes", 0, "attributes", -1);

            /** Get share volume(s)*/
            for (Nvpair = ezxml_child(Attributes, _T("nvpair"));
                    Nvpair; Nvpair = Nvpair->next)
            {
                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("device")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->LogicalVolume =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));

                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("directory")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->MountPoint =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));

                if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("fstype")) == 0)
                    (*ppHAResources)->HAShareVolumes[ii]->FsType =
                        StrNewCopy(ezxml_attr(Nvpair, _T("value")));
            }

            if ((*ppHAResources)->HAShareVolumes[ii]->LogicalVolume != NULL &&
                (*ppHAResources)->HAShareVolumes[ii]->MountPoint != NULL &&
                (*ppHAResources)->HAShareVolumes[ii]->FsType != NULL
                )
                ii++;
        }

        /** Get resource*/
        (*ppHAResources)->HAShareVolumes[ii] =
                        MALLOC(sizeof(HASHAREVOLUMES));
        if (NULL == (*ppHAResources)->HAShareVolumes[ii])
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
            Result = SRDERR_MEMORY_ALLOCATION;
            _LEAVE_;
        }

        /** Get attributes of the resource*/
        Attributes = ezxml_get(Resources, "primitive", 0,
                        "instance_attributes", 0, "attributes", -1);

        /** Get share volume(s)*/
        for (Nvpair = ezxml_child(Attributes, _T("nvpair"));
                Nvpair; Nvpair = Nvpair->next)
        {
            if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("device")) == 0)
                (*ppHAResources)->HAShareVolumes[ii]->LogicalVolume =
                    StrNewCopy(ezxml_attr(Nvpair, _T("value")));

            if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("directory")) == 0)
                (*ppHAResources)->HAShareVolumes[ii]->MountPoint =
                    StrNewCopy(ezxml_attr(Nvpair, _T("value")));

            if (strcmp(ezxml_attr(Nvpair, _T("name")), _T("fstype")) == 0)
                (*ppHAResources)->HAShareVolumes[ii]->FsType =
                    StrNewCopy(ezxml_attr(Nvpair, _T("value")));
        }

        if ((*ppHAResources)->HAShareVolumes[ii]->LogicalVolume != NULL &&
            (*ppHAResources)->HAShareVolumes[ii]->MountPoint != NULL &&
            (*ppHAResources)->HAShareVolumes[ii]->FsType != NULL
            )
            ii++;

        (*ppHAResources)->ShareVolCount = ii;

        Result = SRDERR_SUCCESS;
    _FINALLY_
    _ENDTRY_

    return Result;
}

/** \fn GetSupUnsupVolumesHAV2(PHARESOURCES pHAResources, PCLUSINFO pClusInfo)
    \brief Collect supported/unsupported share volumes
    \param pHAResources SRD Structure with HA v2 Cluster information
    \param pClusInfo SRD Structure with Cluster Information
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSupUnsupVolumesHAV2(PHARESOURCES pHAResources, PCLUSINFO pClusInfo)
{
    R_RESULT Result = SRDERR_ERROR;

    R_TCHAR SuppVols[BUFSIZ] = { _T('\0') },    /**< supported vols*/
            UnsupVols[BUFSIZ] = { _T('\0') };   /**< unsupported vols*/
    R_INT   ii=0,                               /**< Counter*/
            jj=0,                               /**< Counter*/
            kk=0;                               /**< Counter*/

    /** Parse share volume(s) for supported/unsupported volumes*/
    for (ii=0; ii<pHAResources->ShareVolCount; ii++)
    {
        if (SUPPORTED_FS_TYPE(pHAResources->HAShareVolumes[ii]->FsType) &&
            IsMountPoint(pHAResources->HAShareVolumes[ii]->MountPoint)
        )
        {
            if (jj > 0)
            {
                strcat(SuppVols, _T(","));
                strcat(SuppVols, pHAResources->HAShareVolumes[ii]->MountPoint);
            }
            else
                strcpy(SuppVols, pHAResources->HAShareVolumes[ii]->MountPoint);

            strcat(SuppVols, _T(","));
            strcat(SuppVols, cmnNodename);
            jj++;
        }
        else
        {
            if (kk > 0)
            {
                strcat(UnsupVols, _T(","));
                strcat(UnsupVols, pHAResources->HAShareVolumes[ii]->MountPoint);
            }
            else
                strcpy(UnsupVols, pHAResources->HAShareVolumes[ii]->MountPoint);

            strcat(UnsupVols, _T(","));
            strcat(UnsupVols, cmnNodename);
            kk++;
        }
    }

    /** Add share volumes to SRD structure*/
    pClusInfo->SupportedVols = StrNewCopy(SuppVols);
    pClusInfo->UnsupportedVols = StrNewCopy(UnsupVols);

    Result = SRDERR_SUCCESS;

    return Result;
}

/** \fn GetNodesHAV1(PCLUSINFO pClusInfo)
    \brief Collect all nodes in list with ',' delimiter
    \param pClusInfo Return SRD structure with cluster informations
    \return SRDERR_SUCCESS,or SRDERR_ERROR
*/
static R_RESULT
GetNodesHAV1(PCLUSINFO pClusInfo)
{
    R_RESULT Result = SRDERR_ERROR;

    FILE    *pFile = NULL;                      /**< File handler*/
    R_TCHAR line[BUFSIZ] = { _T('\0') },        /**< line by line from buffer*/
            TmpVar[STRLEN_STD] = { _T('\0') },  /**< Status of node*/
            Node[STRLEN_STD] = { _T('\0') },    /**< node hostname*/
            *NodeLong = NULL,                   /**< hostname with domain*/
            TmpNetNames[STRLEN_STD] = { _T('\0') };
    R_INT   ii=0;                               /**< Counter*/

    _TRY_
        /** Get package(s)*/
        if ((pFile = popen(GET_NODES_HAV1, _T("r"))) == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                    "failed."), GET_NODES_HAV1);
            _LEAVE_;
        }

        /** Parse output*/
        while (fgets(line, sizeof(line), pFile))
        {
            if (sscanf(line, "%s %[^\n ]", TmpVar, Node) != 2)
                continue;

            if (CmnExpandHostName(Node, &NodeLong) != 1)
            {
                strcpy(Node, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand node hostname %s."),
                                        Node);
            }
            else
            {
                strcpy(Node, NodeLong);
                FREE(NodeLong);
            }

            if (ii > 0)
            {
                strcat(TmpNetNames, _T(","));
                strcat(TmpNetNames, Node);
            }
            else
                strcpy(TmpNetNames, StrNewCopy(Node));
            ii++;
        }

        pClusInfo->NetworkNames = StrNewCopy(TmpNetNames);

        Result = SRDERR_SUCCESS;
    _FINALLY_
        /** Destroy file handler*/
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    return Result;
}

/** \fn GetSupUnsupVolumesHAV1(PCLUSINFO pClusInfo)
    \brief Collect supported/unsupported share volumes
    \param pClusInfo SRD Structure with Cluster Information
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSupUnsupVolumesHAV1(PCLUSINFO pClusInfo)
{
    R_RESULT Result = SRDERR_ERROR;

    FILE    *pFile = NULL;                      /**< File handler*/
    R_TCHAR SuppVols[BUFSIZ] = { _T('\0') },    /**< supported vols*/
            UnsupVols[BUFSIZ] = { _T('\0') },   /**< unsupported vols*/
            line[BUFSIZ] = { _T('\0') },        /**< line by line from buffer*/
            Node[STRLEN_STD] = { _T('\0') },    /**< node hostname*/
            NodeIP[STRLEN_STD] = { _T('\0') },  /**< node ip address*/
            Resource[STRLEN_STD] = { _T('\0') },/**< resource*/
            Filesystem[STRLEN_STD] = { _T('\0') },/**< tmp var for parse file*/
            Device[STRLEN_STD] = { _T('\0') },  /**< device of share volume*/
            MountPoint[STRLEN_STD] = { _T('\0') },/**< mountpoint of share volume*/
            FsType[STRLEN_STD] = { _T('\0') },  /**< filesystem type of share volume*/
            *NodeLong = NULL;                   /**< hostname with domain*/
    R_INT   ii=0,                               /**< Counter*/
            jj=0;                               /**< Counter*/

    _TRY_
        /** Get package(s)*/
        if ((pFile = popen(GET_RESOURCES, _T("r"))) == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                    "failed."), GET_RESOURCES);
            _LEAVE_;
        }

        /** Parse output*/
        while (fgets(line, sizeof(line), pFile))
        {
            if (sscanf(line, "%s %s %[^\n ]", Node, NodeIP, Resource) != 3)
                continue;

            if (CmnExpandHostName(Node, &NodeLong) != 1)
            {
                strcpy(Node, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand node hostname %s."),
                                        Node);
            }
            else
            {
                strcpy(Node, NodeLong);
                FREE(NodeLong);
            }

            if (sscanf(Resource, "%[^::]::%[^::]::%[^::]::%s", Filesystem,
                                                Device,
                                                MountPoint,
                                                FsType) == 4
            )
            {
                if (SUPPORTED_FS_TYPE(FsType) &&
                    IsMountPoint(MountPoint)
                )
                {
                    if (ii > 0)
                    {
                        strcat(SuppVols, _T(","));
                        strcat(SuppVols, MountPoint);
                    }
                    else
                        strcpy(SuppVols, MountPoint);

                    strcat(SuppVols, _T(","));
                    strcat(SuppVols, Node);
                    ii++;
                }
                else
                {
                    if (jj > 0)
                    {
                        strcat(UnsupVols, _T(","));
                        strcat(UnsupVols, MountPoint);
                    }
                    else
                        strcpy(UnsupVols, MountPoint);

                    strcat(UnsupVols, _T(","));
                    strcat(UnsupVols, Node);
                    jj++;
                }
            }
        }

        /** Add share volumes to SRD structure*/
        pClusInfo->SupportedVols = StrNewCopy(SuppVols);
        pClusInfo->UnsupportedVols = StrNewCopy(UnsupVols);

        Result = SRDERR_SUCCESS;
    _FINALLY_
        /** Destroy file handler*/
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    Result = SRDERR_SUCCESS;

    return Result;
}

/** \fn GetNodesRHELCS(PCLUSINFO pClusInfo, ezxml_t ezXml)
    \brief Collect nodes hostname
    \param pClusInfo Return SRD structure with cluster informations
    \param ezXml XML file structure handler
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetNodesRHELCS(PCLUSINFO pClusInfo, ezxml_t ezXml)
{
    R_RESULT    Result = SRDERR_ERROR;

    ezxml_t Nodes,                              /**< 'nodes' child*/
            Node;                               /**< 'node' child*/
    R_TCHAR TmpNetNames[BUFSIZ] = { _T('\0') }, /**< temp net names*/
            NodeName[BUFSIZ] = { _T('\0') },    /**< node hostname*/
            *NodeLong = NULL;                   /**< hostname with domain*/
    R_INT   ii=0;                               /**< Counter*/

    /** Get Nodes tree from XML file*/
    Nodes = ezxml_get(ezXml, "clusternodes", -1);
    for (Node = ezxml_child(Nodes, _T("clusternode"));
            Node; Node = Node->next)
    {
        /** Expand node hostname to hostname with domain*/
        if (CmnExpandHostName(ezxml_attr(Node, _T("name")), &NodeLong) != 1)
        {
            strcpy(NodeName, cmnNodename);
            DbgPlain(DBG_SYSNFO, _T("Cannot expand node host name %s."),
                                    NodeName);
        }
        else
        {
            strcpy(NodeName, NodeLong);
            FREE(NodeLong);
        }

        if (ii>0)
        {
            strcat(TmpNetNames, _T(","));
            strcat(TmpNetNames, NodeName);
        }
        else
            strcpy(TmpNetNames, NodeName);
        ii++;
    }

    pClusInfo->NetworkNames = StrNewCopy(TmpNetNames);

    Result = SRDERR_SUCCESS;

    return Result;
}

/** \fn GetSupUnsupVolumesRHELCS(PCLUSINFO pClusInfo, ezxml_t ezXml)
    \brief Collect supported/unsupported share volumes
    \param pClusInfo SRD Structure with Cluster Information
    \param ezXml XML file handler
    \return SRDERR_SUCCESS or SRDERR_ERROR
*/
static R_RESULT
GetSupUnsupVolumesRHELCS(PCLUSINFO pClusInfo, ezxml_t ezXml)
{
    R_RESULT Result = SRDERR_ERROR;

    FILE    *pFile = NULL;                      /**< File handler*/
    ezxml_t Attributes,                         /**< 'attributes' child*/
            Service,                            /**< 'service' child*/
            Resource,                           /**< 'resource' child*/
            Nvpair,                             /**< 'nvpair' child*/
            Rm;                                 /**< 'rm' child*/
    R_TCHAR line[BUFSIZ] = { _T('\0') },        /**< buffer from output*/
            SuppVols[BUFSIZ] = { _T('\0') },    /**< supported volumes*/
            UnsupVols[BUFSIZ] = { _T('\0') },   /**< unsupported volumes*/
            NodeName[BUFSIZ] = { _T('\0') },    /**< node hostname*/
            *NodeLong = NULL,                   /**< hostname with domain*/
            TmpVar1[STRLEN_STD] = { _T('\0') }, /**< temp variable*/
            TmpVar[STRLEN_STD] = { _T('\0') },  /**< temp variable*/
            TmpMnt[STRLEN_STD] = { _T('\0') },  /**< temp variable*/
            ServiceName[STRLEN_STD] = { _T('\0') }; /**< cluster service name*/
    R_INT   jj=0,                               /**< Counter*/
            kk=0;                               /**< Counter*/

    _TRY_
        /** Get service(s)*/
        if ((pFile = popen(GET_SERVICES, _T("r"))) == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("   popen(): %s "
                                    "failed."), GET_SERVICES);
            _LEAVE_;
        }

        /** Parse output*/
        while (fgets(line, BUFSIZ, pFile) != NULL)
        {
            if (sscanf(line, " %[^:]:%s %s %[^\n ]", TmpVar,
                                            ServiceName,
                                            NodeName,
                                            TmpVar1) != 4)
                continue;

            if (CmnExpandHostName(NodeName, &NodeLong) != 1)
            {
                strcpy(NodeName, cmnNodename);
                DbgPlain(DBG_SYSNFO, _T("Cannot expand node host name %s."),
                                        NodeName);
            }
            else
            {
                strcpy(NodeName, NodeLong);
                FREE(NodeLong);
            }

            Rm = ezxml_get(ezXml, "rm", -1);
            /** Get services*/
            for (Service = ezxml_child(Rm, _T("service"));
                Service; Service = Service->next)
            {
                /** Get all resources from specific service*/
                if (strcmp(ezxml_attr(Service, _T("name")), ServiceName) == 0)
                {
                    DbgPlain(DBG_SYSNFO, _T("   Get resource for service: %s."),
                                            ServiceName);

                    for (Resource = ezxml_child(Service, _T("fs"));
                            Resource; Resource = Resource->next)
                    {
                        DbgPlain(DBG_SYSNFO, _T("   Reference name for resource: %s."),
                                                ezxml_attr(Resource, _T("ref")));

                        /** Get resources*/
                        Attributes = ezxml_get(ezXml, "rm", 0, "resources", -1);

                        /** Get share volume(s)*/
                        for (Nvpair = ezxml_child(Attributes, _T("fs"));
                                Nvpair; Nvpair = Nvpair->next)
                        {
                            DbgPlain(DBG_SYSNFO, _T("   Reference name for resource: %s."),
                                                    ezxml_attr(Resource, _T("ref")));
                            DbgPlain(DBG_SYSNFO, _T("   Resource name: %s."),
                                                    ezxml_attr(Nvpair, _T("name")));

                            if (strcmp(ezxml_attr(Nvpair, _T("name")),
                                        ezxml_attr(Resource, _T("ref"))) == 0)
                            {
                                DbgPlain(DBG_SYSNFO, _T("   Mountpoint: %s FsType: %s."),
                                                        ezxml_attr(Nvpair, _T("mountpoint")),
                                                        ezxml_attr(Nvpair, _T("fstype")));

                                strcpy(TmpMnt, ezxml_attr(Nvpair, _T("mountpoint")));
                                if (SUPPORTED_FS_TYPE(ezxml_attr(Nvpair, _T("fstype"))) &&
                                    IsMountPoint(TmpMnt)
                                )
                                {
                                    if (jj > 0)
                                    {
                                        strcat(SuppVols, _T(","));
                                        strcat(SuppVols, TmpMnt);
                                    }
                                    else
                                        strcpy(SuppVols, TmpMnt);

                                    strcat(SuppVols, _T(","));
                                    strcat(SuppVols, NodeName);
                                    jj++;
                                }
                                else
                                {
                                    if (kk>0)
                                    {
                                        strcat(UnsupVols, _T(","));
                                        strcat(UnsupVols, TmpMnt);
                                    }
                                    else
                                        strcpy(UnsupVols, TmpMnt);

                                    strcat(UnsupVols, _T(","));
                                    strcat(UnsupVols, NodeName);
                                    kk++;
                                }
                            }
                        }
                    }

                    for (Resource = ezxml_child(Service, _T("clusterfs"));
                            Resource; Resource = Resource->next)
                    {
                        DbgPlain(DBG_SYSNFO, _T("   Reference name for resource: %s."),
                                                ezxml_attr(Resource, _T("ref")));

                        /** Get resources*/
                        Attributes = ezxml_get(ezXml, "rm", 0, "resources", -1);

                        /** Get share volume(s)*/
                        for (Nvpair = ezxml_child(Attributes, _T("clusterfs"));
                                Nvpair; Nvpair = Nvpair->next)
                        {
                            DbgPlain(DBG_SYSNFO, _T("   Reference name for resource: %s."),
                                                    ezxml_attr(Resource, _T("ref")));
                            DbgPlain(DBG_SYSNFO, _T("   Resource name: %s."),
                                                    ezxml_attr(Nvpair, _T("name")));

                            if (strcmp(ezxml_attr(Nvpair, _T("name")),
                                        ezxml_attr(Resource, _T("ref"))) == 0)
                            {
                                DbgPlain(DBG_SYSNFO, _T("   Mountpoint: %s FsType: %s."),
                                                        ezxml_attr(Nvpair, _T("mountpoint")),
                                                        ezxml_attr(Nvpair, _T("fstype")));

                                strcpy(TmpMnt, ezxml_attr(Nvpair, _T("mountpoint")));
                                if (SUPPORTED_FS_TYPE(ezxml_attr(Nvpair, _T("fstype"))) &&
                                    IsMountPoint(TmpMnt)
                                )
                                {
                                    if (jj > 0)
                                    {
                                        strcat(SuppVols, _T(","));
                                        strcat(SuppVols, TmpMnt);
                                    }
                                    else
                                        strcpy(SuppVols, TmpMnt);

                                    strcat(SuppVols, _T(","));
                                    strcat(SuppVols, NodeName);
                                    jj++;
                                }
                                else
                                {
                                    if (kk>0)
                                    {
                                        strcat(UnsupVols, _T(","));
                                        strcat(UnsupVols, TmpMnt);
                                    }
                                    else
                                        strcpy(UnsupVols, TmpMnt);

                                    strcat(UnsupVols, _T(","));
                                    strcat(UnsupVols, NodeName);
                                    kk++;
                                }
                            }
                        }
                    }
                }
            }
        }

        /** Add share volumes to SRD structure*/
        pClusInfo->SupportedVols = StrNewCopy(SuppVols);
        pClusInfo->UnsupportedVols = StrNewCopy(UnsupVols);

        Result = SRDERR_SUCCESS;
    _FINALLY_
        /** Destroy file handler*/
        if (pFile)
            pclose(pFile);
    _ENDTRY_

    return Result;
}

/** \fn GetClusInfo(PCLUSINFO pClusInfo)
    \brief Collect Cluster information
    \param pClusInfo Return SRD structure with cluster informations
    \return SRDERR_SUCCESS, SRDERR_ERROR or SRDERR_NOT_SUPPORTED
*/
static R_RESULT
GetClusInfo(PCLUSINFO pClusInfo)
{
    R_RESULT    Result = SRDERR_ERROR;

    R_TCHAR     omnisvPath[STRLEN_STD] = { _T('\0') };  /**< Path to omnisv*/
    ezxml_t     ezXml = NULL;                           /**< xml file handler*/
    clusterType clType = CLUS_TYPE_UNKNOWN;
    R_INT       ii=0,
                jj=0,
                kk=0;

    _TRY_
        /**
         * Get Cluster Suite type
         */
        clType = GetClusterSuite();

        /**
         * if HP ServiceGuard
         */
        if (clType == CLUS_TYPE_HP_SG_SLES ||
            clType == CLUS_TYPE_HP_SG_RHEL)
        {
            PSERVICEGUARD   pServiceGuard = NULL;

            pServiceGuard = MALLOC(sizeof(SERVICEGUARD));
            if (pServiceGuard == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                Result = SRDERR_MEMORY_ALLOCATION;
                _LEAVE_;
            }
            memset(pServiceGuard , 0, sizeof(SERVICEGUARD));

            /**
             * Get all share volumes in all packages
             */
            if (GetShareVolumesSG(&pServiceGuard, clType) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect share volumes failed."));
                _LEAVE_;
            }

            /**
             * Collect supported/unsupported share volumes
             */
            if (GetSupUnsupVolumesSG(pServiceGuard, pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect supported/unsupported "
                                        "share volumes failed."));
                _LEAVE_;
            }

            /**
             * Check if DP is cluster aware
             */
            if (IsClusterAwareSG(pServiceGuard, pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Check for DP cluster aware failed."));
                _LEAVE_;
            }

            /**
             * Get AdminVS hostname
             * If DP installed successfully on cluster. AdminVS is
             * cell server hostname
             */
            if (pClusInfo->ClusterAware == 1)
            {
                pClusInfo->AdminVS = StrNewCopy(cmnCSHostname);
            }

            /**
             * Get virtual hostname in list with delimiter ','
             */
            if (GetVirtualHostnameSG(pServiceGuard, pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect virtual hostname failed."));
                _LEAVE_;
            }

#if 0
            /**
             * Get quorum hostname
             */
            if (GetQuorumSG(pClusInfo, clType) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Get qourum hostname failed."));
                _LEAVE_;
            }
#endif

            for (ii=0; ii<pServiceGuard->PackageCount; ii++)
            {
                DbgPlain(DBG_SYSNFO, _T("Package: %s"),
                            pServiceGuard->SGPackages[ii]->Package);
                for (jj=0; jj<pServiceGuard->SGPackages[ii]->ShareVolCount; jj++)
                {
                    DbgPlain(DBG_SYSNFO, _T("   LogicalVolume: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->LogicalVolume);
                    DbgPlain(DBG_SYSNFO, _T("   MountPoint: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->MountPoint);
                    DbgPlain(DBG_SYSNFO, _T("   FsType: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->FsType);
                    DbgPlain(DBG_SYSNFO, _T("   FsMountOpt: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->FsMountOpt);
                    DbgPlain(DBG_SYSNFO, _T("   FsUMountOpt: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->FsUMountOpt);
                    DbgPlain(DBG_SYSNFO, _T("   FsFSCKOpt: %s"),
                            pServiceGuard->SGPackages[ii]->SGShareVolumes[jj]->FsFSCKOpt);

                    for (kk=0; kk<pServiceGuard->SGPackages[ii]->AddressCount; kk++)
                    {
                        DbgPlain(DBG_SYSNFO, _T("   IpAddress: %s"),
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->IpAddress);
                        DbgPlain(DBG_SYSNFO, _T("   HostName: %s"),
                                pServiceGuard->SGPackages[ii]->SGPackageAddress[kk]->HostName);
                    }
                }
            }

            /**
             * Free memory
             */
            if (pServiceGuard)
            {
                FreeServiceGuard(pServiceGuard);
                FREE(pServiceGuard);
            }
        } /** if HP ServiceGuard */
        /**
         * if SLES Heartbeat v2
         */
        else if (clType == CLUS_TYPE_SLES_HA_V2)
        {
            R_TCHAR *CibFile = _T("/var/lib/heartbeat/crm/cib.xml");

            PHARESOURCES   pHAResources = NULL;
            pHAResources = MALLOC(sizeof(HARESOURCES));
            if (pHAResources == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
                Result = SRDERR_MEMORY_ALLOCATION;
                _LEAVE_;
            }
            memset(pHAResources , 0, sizeof(HARESOURCES));

            /**
             * Prepare xml file for parsing
             */
            if ((ezXml = ezxml_parse_file(CibFile)) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   ezxml_parse_file(): failed."));
                _LEAVE_;
            }

            /**
             * Get nodes in list with delimiter ','
             */
            if (GetNodesHAV2(pClusInfo, ezXml) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect nodes hostname failed."));
                _LEAVE_;
            }

            /**
             * Get all share volumes in all packages
             */
            if (GetShareVolumesHAV2(&pHAResources, ezXml) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect share volumes failed."));
                _LEAVE_;
            }

            /**
             * Collect supported/unsupported share volumes
             */
            if (GetSupUnsupVolumesHAV2(pHAResources, pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect supported/unsupported "
                                        "share volumes failed."));
                _LEAVE_;
            }

            /**
             * Free memory
             */
            if (pHAResources)
            {
                FreeHeartbeat(pHAResources);
                FREE(pHAResources);
            }
        } /** if SLES Heartbeat v2*/

        /**
         * if SLES Heartbeat v1
         */
        else if (clType == CLUS_TYPE_SLES_HA_V1)
        {
            /**
             * Get nodes in list with delimiter ','
             */
            if (GetNodesHAV1(pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect nodes hostname failed."));
                _LEAVE_;
            }

            /**
             * Collect supported/unsupported share volumes
             */
            if (GetSupUnsupVolumesHAV1(pClusInfo) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect supported/unsupported "
                                        "share volumes failed."));
                _LEAVE_;
            }
        } /** if SLES Heartbeat v1*/

        /**
         * if RHEL Cluster Suite
         */
        else if (clType == CLUS_TYPE_RHEL_CS)
        {
            R_TCHAR *ClusterFile = _T("/etc/cluster/cluster.conf");

            ezxml_t ezXml;  /**< xml file handler*/

            /**
             * Prepare xml file for parsing
             */
            if ((ezXml = ezxml_parse_file(ClusterFile)) == NULL)
            {
                DbgPlain(DBG_SYSNFO, _T("   ezxml_parse_file(): failed."));
                _LEAVE_;
            }

            /**
             * Get nodes in list with delimiter ','
             */
            if (GetNodesRHELCS(pClusInfo, ezXml) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect nodes hostname failed."));
                _LEAVE_;
            }

            /**
             * Collect supported/unsupported share volumes
             */
            if (GetSupUnsupVolumesRHELCS(pClusInfo, ezXml) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_SYSNFO, _T("Collect supported/unsupported "
                                        "share volumes failed."));
                _LEAVE_;
            }
        }
        /**
         * An unknown cluster type or not cluster at all
         */
        else
            Result = SRDERR_NOT_SUPPORTED;

        if (clType != CLUS_TYPE_UNKNOWN)
        {
            /**
             * Check if cluster is cell server
             */
            strcpy(omnisvPath, cmnPanSBin);
            strcat(omnisvPath, _T("/omnisv"));
            if (OB2_StatFile(omnisvPath, NULL) ==0)
                pClusInfo->ClusterIsCM = 1;
            else
                pClusInfo->ClusterIsCM = 0;

            /**
             * Get cluster type
             * Windows specific
             */
            pClusInfo->clusterType = 0;

            /**
             * Print to debug log
             */
            if (clType == CLUS_TYPE_HP_SG_SLES)
                DbgPlain(DBG_SYSNFO, _T("   Cluster: ServiceGuard on SLES"));
            else if (clType == CLUS_TYPE_HP_SG_RHEL)
                DbgPlain(DBG_SYSNFO, _T("   Cluster: ServiceGuard on RHEL"));
            else if (clType == CLUS_TYPE_SLES_HA_V1)
                DbgPlain(DBG_SYSNFO, _T("   Cluster: SLES Heartbeat v1"));
            else if (clType == CLUS_TYPE_SLES_HA_V2)
                DbgPlain(DBG_SYSNFO, _T("   Cluster: SLES Heartbeat v2"));
            else
                DbgPlain(DBG_SYSNFO, _T("   Cluster: RHEL Cluster Suite"));

            DbgPlain(DBG_SYSNFO, _T("      AdminVS: %s"), pClusInfo->AdminVS);
            DbgPlain(DBG_SYSNFO, _T("      ConfigDBPath: %s"), pClusInfo->ConfigDBPath);
            DbgPlain(DBG_SYSNFO, _T("      NetworkNames: %s"), pClusInfo->NetworkNames);
            DbgPlain(DBG_SYSNFO, _T("      UnsupportedVols: %s"), pClusInfo->UnsupportedVols);
            DbgPlain(DBG_SYSNFO, _T("      SupportedVols: %s"), pClusInfo->SupportedVols);
            DbgPlain(DBG_SYSNFO, _T("      ClusterIsCM: %s"),
                        (pClusInfo->ClusterIsCM == 1) ? _T("Yes") : _T("No"));
            DbgPlain(DBG_SYSNFO, _T("      ClusterAware: %s"),
                        (pClusInfo->ClusterAware == 1) ? _T("Yes") : _T("No"));

            Result = SRDERR_SUCCESS;
        }

    _FINALLY_
        /**
         * Free xml structure
         */
        if (ezXml)
            ezxml_free(ezXml);
    _ENDTRY_

    return Result;
}
#endif

R_RESULT
GetClusterData(PCLUSINFO *ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    PCLUSINFO   pClusInfo = NULL;

    *ppOut = NULL;

    _TRY_
        pClusInfo = MALLOC(sizeof(CLUSINFO));
        if (pClusInfo == NULL)
        {
            DbgPlain(DBG_SYSNFO, _T("Memory allocation error."));
            Return = SRDERR_MEMORY_ALLOCATION;
            _LEAVE_;
        }
        memset(pClusInfo , 0, sizeof(CLUSINFO));

        Return = GetClusInfo(pClusInfo);
        if (
            Return != SRDERR_SUCCESS &&
            Return != SRDERR_NOT_SUPPORTED
        )
        {
            DbgPlain(DBG_SYSNFO, _T("FAILURE - cannot obtain SRD buffer cluster info data."));
        }

        if (Return == SRDERR_SUCCESS)
            *ppOut = pClusInfo;

    _FINALLY_
        if (Return != SRDERR_SUCCESS)
        {
            FreeClusInfo(pClusInfo);
            FREE(pClusInfo);

            if (Return == SRDERR_NOT_SUPPORTED)
                Return = SRDERR_SUCCESS;
        }
    _ENDTRY_

    return Return;
}
