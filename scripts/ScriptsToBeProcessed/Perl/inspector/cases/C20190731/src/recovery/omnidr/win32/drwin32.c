/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   HP OpenView OmniBack II Disaster Recovery
* @file      recovery/omnidr/win32/drwin32.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Disaster Recovery Restore Process Windows Specifics.
*            Implementation.
*
* @since     August     2001    Tadej Mali      Original Coding.
*
* @remarks   /
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/omnidr/win32/drwin32.c $Rev$ $Date::                      $:") ;
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/
#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "lib/ipc/ipwrap.h"

#include <shlobj.h>

#include <shlobj.h>
#include <ntmsapi.h>

#pragma warning (4: 4032)  // function promoting - Lars' advice
#include <conio.h>
#pragma warning (3: 4032)  // function promoting

#include "../omnidr.h"
#include "../drcmn.h"
#include "../optgen.h"
#include "../../srd/SysNfo/sysnfonet.h"
#include "drim.h"
#include "drwin32.h"

/*--- Active Directory API --------------------------------------------------*/

typedef HRESULT (_stdcall *DsIsNTDSOnlineProcA)(LPCSTR, BOOL*);
typedef HRESULT (_stdcall *DsIsNTDSOnlineProcW)(LPCWSTR, BOOL*);

#if defined(UNICODE) || defined(_UNICODE)
    #define DsIsNTDSOnlineProc          DsIsNTDSOnlineProcW
    #define DsIsNTDSOnlineName          "DsIsNTDSOnlineW"
#else
    #define DsIsNTDSOnlineProc          DsIsNTDSOnlineProcA
    #define DsIsNTDSOnlineName          "DsIsNTDSOnlineA"
#endif

/*--- Certificate Services API ----------------------------------------------*/

typedef HRESULT (_stdcall *CsIsCSOnlineProc)(LPCWSTR, BOOL*);
#define CsIsCSOnlineName                "CertSrvIsServerOnline"


/*--- NTMS Services API -----------------------------------------------------*/

typedef HANDLE (_stdcall *OpenNtmsSessionProcA)(LPCSTR, LPCSTR, DWORD);
typedef HANDLE (_stdcall *OpenNtmsSessionProcW)(LPCWSTR, LPCWSTR, DWORD);

typedef DWORD  (_stdcall *CloseNtmsSessionProc)(HANDLE);

typedef DWORD  (_stdcall *EnumerateNtmsObjectProc)(HANDLE, LPNTMS_GUID, LPNTMS_GUID, LPDWORD, DWORD, DWORD);

typedef DWORD  (_stdcall *GetNtmsObjectInformationProcA)(HANDLE, LPNTMS_GUID, LPNTMS_OBJECTINFORMATIONA);
typedef DWORD  (_stdcall *GetNtmsObjectInformationProcW)(HANDLE, LPNTMS_GUID, LPNTMS_OBJECTINFORMATIONW);

typedef DWORD  (_stdcall *DisableNtmsObjectProc)(HANDLE, DWORD, LPNTMS_GUID);

#if defined(UNICODE) || defined(_UNICODE)
    #define OpenNtmsSessionProc    OpenNtmsSessionProcW
    #define OpenNtmsSessionName    "OpenNtmsSessionW"
#else
    #define OpenNtmsSessionProc    OpenNtmsSessionProcA
    #define OpenNtmsSessionName    "OpenNtmsSessionA"
#endif

#define CloseNtmsSessionName       "CloseNtmsSession"
#define EnumerateNtmsObjectName    "EnumerateNtmsObject"

#if defined(UNICODE) || defined(_UNICODE)
    #define GetNtmsObjectInformationProc    GetNtmsObjectInformationProcW
    #define GetNtmsObjectInformationName    "GetNtmsObjectInformationW"
#else
    #define GetNtmsObjectInformationProc    GetNtmsObjectInformationProcA
    #define GetNtmsObjectInformationName    "GetNtmsObjectInformationA"
#endif

#define DisableNtmsObjectName       "DisableNtmsObject"


/*--- Dynamic DLLs ----------------------------------------------------------*/

#define NTDS_DLLNAME    _T("ntdsbcli.dll")
#define CS_DLLNAME      _T("certadm.dll")
#define NTMS_DLLNAME    _T("ntmsapi.dll")


/*--- Various defines -------------------------------------------------------*/
#define ADSMAP_OPT      _T("adsmap")
#define CSMAP_OPT       _T("csmap")
#define SERVER_OPT      _T("adsserver")

/*-----------------------------------------------------------------------------
|   static functions
 ----------------------------------------------------------------------------*/
static int DRWin32PrepareRegistry(PDRContext pDRCtxt);

static HWND GetConsoleHwnd(void);
static int CleanUpTempOS(PDRContext pDRCtxt);
static int CleanUpActiveOS(PDRContext pDRCtxt);
static int DRWin32SetProcessPrivilege (tchar *lpszPrivilege, BOOL on);

static int DRWin32ClusterSkipVolumes(PDRContext pDRCtxt);

static int DRWin32CheckInetSafeMode(PDRContext pDRCtxt);

static int 
DRWin32CreateJetDBOptions(
    PDRContext pDRCtxt,
    argvRec *pAvOpt,
    tchar *pszItem,
    int iPurpose,
    tchar *pszVarOptName
    );

/*-----------------------------------------------------------------------------
|   Registry defines
 ----------------------------------------------------------------------------*/
#define ALLOW_PROTECTED_RENAMES_VALUE   _T("AllowProtectedRenames")
#define SESSION_MANAGER_KEY             _T("System\\CurrentControlSet\\Control\\Session Manager")

/*-----------------------------------------------------------------------------
|   Implementation
 ----------------------------------------------------------------------------*/

/*========================================================================*//**
*
* @param     hRoot    -  handle to opened registry key
* @param     lpszSub  -  name of a subkey to delete from the hRoot
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Recursively delete lpszSub from hRoot.
*
*//*=========================================================================*/
int DRWin32DeleteRegKey(HKEY hRoot, LPTSTR lpszSub)
{
	INT		iRet = PANDR_RET_ERROR;
	HKEY	hKeySrc = NULL;
	LONG	lRes = ERROR_SUCCESS;
	ULONG	ulKeyNameLen = 128;
	TCHAR	lpszKeyName[128];
	ULONG	ulIdx = 0;

	lRes = RegOpenKeyEx(hRoot, lpszSub, 0, KEY_WRITE | KEY_READ, &hKeySrc);
	if(lRes == ERROR_SUCCESS)
	{
		do {
			ulKeyNameLen = 128;

			lRes = RegEnumKeyEx(hKeySrc, ulIdx, lpszKeyName, &ulKeyNameLen, NULL, NULL, NULL, NULL);
			if(lRes == ERROR_SUCCESS)
			{
				TCHAR	lpszBase[MAX_PATH];

				sprintf(lpszBase, _T("%s\\%s"), lpszSub, lpszKeyName);

				iRet = DRWin32DeleteRegKey(hRoot, lpszBase);
				if(iRet !=PANDR_RET_SUCCESS)
                {
					lRes = ERROR_GEN_FAILURE;
                }
			}
		} while(lRes == ERROR_SUCCESS);

		if(lRes == ERROR_NO_MORE_ITEMS)
        {
			lRes = ERROR_SUCCESS;
        }

		RegCloseKey(hKeySrc);
	}
    
	if(
       (
	    lRes == ERROR_SUCCESS &&
		RegDeleteKey(hRoot, lpszSub) == ERROR_SUCCESS
	   )
       ||
       (
        lRes==ERROR_FILE_NOT_FOUND
       )
      )
    {
		iRet = PANDR_RET_SUCCESS;
    }
	return iRet;
}


/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. Checks the registry for OmniBack entries.
*            2. Sets the InetPort as given in SRD file.
*
*//*=========================================================================*/

static
int DRWin32PrepareRegistry(PDRContext pDRCtxt)
{
    int iRet=PANDR_RET_ERROR;    
    R_ULONG ulInetPort=5565;
    tchar pszStrBuff[STRLEN_PATH+1];
    tchar pszCMName[STRLEN_HOSTNAME+1];
    HKEY hkTest=0;
    LONG error;
    
    USES_SRDQUERY;

    ERH_FUNCTION(_T("DRWin32PrepareRegistry"));

    DBGFCNIN();
    __try
    {       
        error=RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGCOMMON, 0, KEY_EXECUTE, &hkTest);
        
        switch (error)
        {
        case ERROR_SUCCESS:
            DBGPRINT(_T("Disaster Recovery already registered.\n"));
            PUT_TRACE(pDRCtxt, _T("Disaster Recovery already registered."));
            iRet=PANDR_OMNIBACK_FOUND;
            __leave;
            break;

        case ERROR_FILE_NOT_FOUND:
            DBGPRINT(_T("Disaster Recovery is not registered. Proceeding.\n"));
            PUT_TRACE(pDRCtxt, _T("Disaster Recovery is not registered. Proceeding."));
            break;

        default:
            DBGPRINT(_T("Unknown Error. Aborting."));
            PUT_TRACE(pDRCtxt, _T("Unknown Error. Aborting."));
            __leave;
            break;
        }

        if (pDRCtxt->srdH==NULL)
        {            
            DBGPRINT(_T("SRD file was not found. It is not possible to register DR.\n"));
            PUT_TRACE(pDRCtxt, _T("SRD file was not found. It is not possible to register DR."));
            __leave;
        }

        PREPARE_SRDQUERY(&ulInetPort, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_INET_PORT, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("Error in SRDACT_GET_INET_PORT."));
            __leave;
        }

        PREPARE_SRDQUERY(pszCMName, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_AUX_CELL_MANAGER, SRDHT_SYSINFO
                ) != SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("Error in SRDACT_GET_CELL_MANAGER."));
            __leave;
        }
        StrToLower(pszCMName);

        /* Set Inet Port value */
        sprintf(pszStrBuff, _T("%d"), ulInetPort);
        RegWriteString(HKEY_LOCAL_MACHINE, REGCOMMON, REGVAL_INETPORT, pszStrBuff);
        RegWriteDWORD(HKEY_LOCAL_MACHINE, REGPARAMETERS, REGVAL_INETPORT, ulInetPort);
        
        /* Set Cell Manager name */
        RegWriteString(HKEY_LOCAL_MACHINE, REGSITE, REGVAL_CELLSERVER, pszCMName);
        
        /* Set Home Directory */
        GetModuleFileName(NULL, pszStrBuff, STRLEN_PATH+1);
        PathCutLeaf(pszStrBuff);
        if (STRCMP(PathGetLeaf(pszStrBuff), BIN_PATH)==0)
        {
            PathCutLeaf(pszStrBuff);
        }
        strcat(pszStrBuff, _T("\\"));
        RegWriteString(HKEY_LOCAL_MACHINE, REGCOMMON, REGVAL_HOMEDIR, pszStrBuff);

        PUT_TRACE(pDRCtxt, _T("Disaster Recovery successfully registered."));
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        RegCloseKey(hkTest);
    }

    DBGFCNOUT();
    return (iRet);
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Calls DRWin32PrepareRegistry(). Reports on error.
*
*//*=========================================================================*/
int DRWin32Register(PDRContext pDRCtxt)
{
    int iRet=PANDR_RET_ERROR;
    int iTempRet=iRet;
    tchar pszSCSITabFrom[STRLEN_PATH+1], pszSCSITabTo[STRLEN_PATH+1];
    ERH_FUNCTION(_T("DRWin32Register"));

    DBGFCNIN();

    __try
    {
        iTempRet=DRWin32PrepareRegistry(pDRCtxt);
        if ((iTempRet!=PANDR_RET_SUCCESS) && (iTempRet!=PANDR_OMNIBACK_FOUND))
        {
            DBGPRINT(_T("It was not possible to register Disaster Recovery. Aborting.\n"));
            PUT_TRACE(pDRCtxt, _T("It was not possible to register Disaster Recovery. Aborting."));
            __leave;
        }
        if (iTempRet==PANDR_OMNIBACK_FOUND)
        {
            pDRCtxt->bWasInstalled=TRUE;
            PUT_TRACE(pDRCtxt, _T("The Disaster Recovery was already registered."));
        }

        DRGetSystemRoot(pszSCSITabFrom, STRLEN_PATH+1);
        strcat(pszSCSITabFrom, _T("\\temp\\scsitab.dev.cfg"));

        GetModuleFileName(NULL, pszSCSITabTo, STRLEN_PATH+1);
        PathCutLeaf(pszSCSITabTo);
        if (STRCMP(PathGetLeaf(pszSCSITabTo), BIN_PATH)==0)
        {
            PathCutLeaf(pszSCSITabTo);
        }
        strcat(pszSCSITabTo, _T("\\scsitab"));

        CopyFile(pszSCSITabFrom, pszSCSITabTo, FALSE);

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Always free. */
    }

    DBGFCNOUT();
    return (iRet);
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Registers MiniOS. Only for EADR.
*
*//*=========================================================================*/
int DRWin32RegisterMiniOS(PDRContext pDRCtxt)
{
    int iRet=PANDR_RET_ERROR;

    if (IS_DRIM(pDRCtxt))
        iRet = RegWriteDWORD(HKEY_LOCAL_MACHINE, REGCOMMON, REGVAL_MINIOS, 1);
    else
        iRet = PANDR_RET_SUCCESS;

    return (iRet);
}

enum OsVersions
{
    VISTA_OR_WIN7_MAJOR_NUMBER  = 6,
    VISTA_MINOR_NUMBER          = 0,
    WIN7_MINOR_NUMBER           = 1
};

/*========================================================================*//**
*
* @param     major    -  On success, holds major OS version.
* @param     minor    -  On success, holds minor OS version.
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Verifies, if omnidr is running inside Windows PE.
*
*//*=========================================================================*/
static int
GetOSVersion(DWORD* major, DWORD* minor)
{
    OSVERSIONINFOEX ovi;

    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!CmnGetOsVersionInfo(&ovi))
    {
        return(PANDR_RET_ERROR);
    }

    *major = ovi.dwMajorVersion;
    *minor = ovi.dwMinorVersion;

    return(PANDR_RET_SUCCESS);
}

/*========================================================================*//**
*
* @retval    1 if Windows Vista/Server 2K8, 0 otherwise.
*
* @brief     Verifies, if omnidr is running inside Windows Vista/Server 2K8.
*
*//*=========================================================================*/
int
DRWin32IsVista(void)
{
    DWORD major = 0;
    DWORD minor = 0;

    if (GetOSVersion(&major, &minor))
    {
        return(0);
    }
    else
    {
        if (major == VISTA_OR_WIN7_MAJOR_NUMBER && minor == VISTA_MINOR_NUMBER)
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }
}

/*========================================================================*//**
*
* @retval    1 if Windows Vista/Server 2K8 or later OS, 0 otherwise.
*
* @brief     Verifies, if omnidr is running inside Windows Vista/Server 2K8
*            or later OS.
*
*//*=========================================================================*/
int
DRWin32IsVistaOrHigher(void)
{
    DWORD major = 0;
    DWORD minor = 0;

    if (GetOSVersion(&major, &minor))
    {
        return(0);
    }
    else
    {
        if (major >= VISTA_OR_WIN7_MAJOR_NUMBER)
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }
}

/*========================================================================*//**
*
* @retval    1 if Windows 7/Server 2K8 R2, 0 otherwise.
*
* @brief     Verifies, if omnidr is running inside Windows 7/Server 2K8 R2.
*
*//*=========================================================================*/
int
DRWin32IsWin7(void)
{
    DWORD major = 0;
    DWORD minor = 0;

    if (GetOSVersion(&major, &minor))
    {
        return(0);
    }
    else
    {
        if (major == VISTA_OR_WIN7_MAJOR_NUMBER && minor == WIN7_MINOR_NUMBER)
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }
}

/*========================================================================*//**
*
* @retval    1 if Windows 7/Server 2K8 R2 or later OS, 0 otherwise.
*
* @brief     Verifies, if omnidr is running inside Windows 7/Server 2K8 R2.
*            or later OS.
*
*//*=========================================================================*/
int
DRWin32IsWin7OrHigher(void)
{
    DWORD major = 0;
    DWORD minor = 0;

    if (GetOSVersion(&major, &minor))
    {
        return(0);
    }
    else
    {
        if (major > VISTA_OR_WIN7_MAJOR_NUMBER || (major == VISTA_OR_WIN7_MAJOR_NUMBER && minor >= WIN7_MINOR_NUMBER))
        {
            return(1);
        }
        else
        {
            return(0);
        }
    }
}

/*========================================================================*//**
*
* @retval    1 if WinPE, 0 otherwise.
*
* @brief     Verifies, if omnidr is running inside Windows PE.
*
*//*=========================================================================*/
int
DRWin32IsWinPE(void)
{
    int iRet = 0;

    DWORD val = 0;
    if (!RegReadDWORD(HKEY_LOCAL_MACHINE, REGCOMMON, REGVAL_MINIOS, &val))
    {
        if (val > 0)
        {
            if (DRWin32IsVistaOrHigher())
            {
                iRet = 1;
            }
        }
    }

    return(iRet);
}

/*========================================================================*//**
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Key: SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore
*            Value: Plug & Play
*
*            Remove "CurrentControlSet\\Enum\\"
*
*//*=========================================================================*/
void DRWin32TamperKeysNotToRestore()
{
    LONG lResult=0;
    HKEY hKey=NULL;
    DWORD dwSize=0, dwType=0;
    tchar *pszNewData=NULL, *pszSrcString=NULL, *pszDstString=NULL;

    tchar *pszOldData=
        RegGetMULTI_SZ(
            HKEY_LOCAL_MACHINE, 
            _T("SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore"), 
            _T("Plug & Play"), _T('\0')
            );

    pszSrcString=pszOldData;

    lResult=
        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, 
            _T("SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore"), 
            0, KEY_ALL_ACCESS, &hKey
            );
    __try
    {
        if (lResult!=ERROR_SUCCESS)
        {
            __leave;
        }

        if (RegQueryValueEx(hKey, _T("Plug & Play"), NULL, &dwType, NULL, &dwSize)!=ERROR_SUCCESS)
        {
            __leave;
        }
        pszNewData=MALLOC(dwSize);

        pszDstString=pszNewData;
        *pszDstString=NULL_TCHAR;
        dwSize=0;
        while (strlen(pszSrcString)>0)
        {
            if (strcmp(pszSrcString, _T("CurrentControlSet\\Enum\\"))!=0)
            {
                strcat(pszDstString, pszSrcString);
                dwSize+=((DWORD)strlen(pszSrcString)+1);
                pszDstString=pszNewData+dwSize;
                *pszDstString=NULL_TCHAR;
            }
            pszSrcString+=(strlen(pszSrcString)+1);
        }

        dwSize++;
        dwSize*=sizeof(tchar);

        RegSetValueEx(hKey, _T("Plug & Play"), 0, REG_MULTI_SZ, (BYTE*)pszNewData, dwSize);
    }
    __finally
    {
        if (hKey!=NULL)
        {
            RegCloseKey(hKey);
        }
        FREE(pszNewData);
        FREE(pszOldData);
    }
  
    return;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Check all volumes if they are to be restored.
*
*            If (iMSCSPass==MSCS_JOIN_CLUS)
*            skip DR_OB2DB_FILES   ( i.e. -omnidb cmname:/ [Database]   )
*            skip DR_OB2DB_VOLUME  ( i.e. -winfs  cmname:/DB_VOLUME     )
*            skip DR_OB2DB_CONFIG  ( i.e. -winfs  cmname:/DB_CONFIG     )
*            skip DR_QUORUM_VOLUME ( i.e. -winfs  clusvs:/QUORUM_VOLUME )
*            If (iMSCSPass==MSCS_FORM_CLUS)
*            do not skip anything
*            If (iMSCSPass==MSCS_CLUSDB)
*            skip all except:
*            DR_OB2DB_FILES, DR_OB2DB_VOLUME, DR_OB2DB_CONFIG,
*            DR_CFG_VOLUME     ( i.e. -winfs  nodename:/CONFIGURATION )
*
*//*=========================================================================*/
static int DRWin32ClusterSkipVolumes(PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32ClusterSkipVolumes"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx=0, ulPurpose=0;

    DbgFcnIn();
    
    for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
    {
        ulPurpose=((PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVolIdx]))->iPurpose;
        
        if (pDRCtxt->iMSCSMode==MSCS_JOIN_CLUS)
        {
            PVolRec volRec = (PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVolIdx]);

            if(ulPurpose & (DR_OB2DB_FILES|DR_OB2DB_VOLUME|DR_OB2DB_CONFIG|DR_QUORUM_VOLUME))
            {
                /*-----------------------------------------------------------------
                |   We are restoring a cluster node of a working cluster.
                |   We will skip CM database and Quorum Volume.
                ----------------------------------------------------------------*/
                DbgPlain(DBG_DR_INIT, _T("Skipping volume %s"), volRec->pszVolMnt);
                volRec->state=VOL_SKIP;
            }

            if (ulPurpose & (DR_DATA_VOLUME))
            {
                /*-----------------------------------------------------------------
                |   We should also skip any shared data volumes. Shared cluster
                |   data volumes can only be restored if Full Recovery With Shared
                |   Volumes is selected during a DRIM recovery or if primary node
                |   restore is under way during an ASR (or another method) restore.
                ----------------------------------------------------------------*/
                USES_SRDQUERY;
                R_ULONG isClusVol = 0;
                
                PREPARE_SRDQUERY(volRec->pszVolMnt, &isClusVol, NULL, NULL, NULL);
                if (DO_SRDQUERY(
                        pDRCtxt->srdH, SRDACT_IS_CLUSTER_VOLUME, SRDHT_SYSINFO
                        )!=SRDERR_SUCCESS)
                {
                    DbgPlain(
                        DBG_DR_SRDQUERY,
                        _T("RecoveryInformationAction(SRDACT_IS_CLUSTER_VOLUME) failure. ")
                        _T("Assuming volume %s is not a cluster volume."),
                        volRec->pszVolMnt);
                }

                if (isClusVol)
                {
                    DbgPlain(
                        DBG_DR_INIT,
                        _T("Skipping volume %s. Not full cluster recovery scope selected."),
                        volRec->pszVolMnt);
                    volRec->state=VOL_SKIP;
                }
            }
        }
        else if (
                 (pDRCtxt->iMSCSMode==MSCS_FORM_CLUS) &&
                 (ulPurpose & ~(DR_OB2DB_FILES|DR_OB2DB_VOLUME|DR_OB2DB_CONFIG|DR_QUORUM_VOLUME))
                )
        {
            /*-----------------------------------------------------------------
            |   We are restoring the first cluster node. 
            |   There is no need to skip CM database nor Quorum Volume
             ----------------------------------------------------------------*/
        }                
        else if (
                 (pDRCtxt->iMSCSMode==MSCS_DB_CLUS) &&
                 (ulPurpose & ~(DR_OB2DB_FILES|DR_OB2DB_VOLUME|DR_OB2DB_CONFIG|DR_CFG_VOLUME))
                )
        {
            /*-----------------------------------------------------------------
            |   We are restoring a cluster database of a working cluster.
            |   We will skip everything except CM database (because this is 
            |   not the appropriate place for the decision) and /CONFIGURATION.
             ----------------------------------------------------------------*/
            DbgPlain(DBG_DR_INIT, _T("Skipping volume %s"), ((PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVolIdx]))->pszVolMnt);
            ((PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVolIdx]))->state=VOL_SKIP;
        }
    }

    iRet=PANDR_RET_SUCCESS;
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Checks if MS Cluster Service is to be restored.
*
*//*=========================================================================*/
int DRWin32ClusterInit(PDRContext pDRCtxt)
{    
    int iRet=PANDR_RET_ERROR;
    USES_SRDQUERY;

    ERH_FUNCTION(_T("DRWin32ClusterInit"));

    DbgFcnIn();
    __try
    {
        PREPARE_SRDQUERY(GCI_CLUSAWARE, &pDRCtxt->bClusterAware, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_CLUSTER_INFO, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("ClusterAware is missing in SRD."));
            pDRCtxt->bClusterAware=FALSE;
        }

        if (!pDRCtxt->bClusterAware)
        {
            DbgPlain(DBG_DR_INIT, _T("I am not cluster aware!"));
            iRet=PANDR_RET_SUCCESS;
            __leave;
        }
        pDRCtxt->bMSCluster=TRUE;
        pDRCtxt->iMSCSMode=MSCS_JOIN_CLUS;
        DbgPlain(DBG_DR_INIT, _T("I am cluster aware!"));

        PREPARE_SRDQUERY(GCI_ADMINVS, pDRCtxt->pszAdminVS, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_CLUSTER_INFO, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("GCI_ADMINVS failed."));
            __leave;
        }

        PREPARE_SRDQUERY(GCI_CLUSCM, &pDRCtxt->bIamClusCM, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_CLUSTER_INFO, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("GCI_CLUSCM failed."));
            pDRCtxt->bIamClusCM=FALSE;
            __leave;
        }

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Composes default SRD filename. The SRD file should by default be
*            located in the same directory as the executable.
*
*//*=========================================================================*/
void DRWin32SetDefaultSRDName(PDRContext pDRCtxt)
{
    tchar pszSRD[STRLEN_PATH+1];
    ERH_FUNCTION(_T("DRWin32SetDefaultSRDName"));    
    
    if (pDRCtxt->ptrOpt->srdFile==NULL)
    {    
        GetModuleFileName(NULL, pszSRD, STRLEN_PATH+1);
        PathCutLeaf(pszSRD);
        strcat(pszSRD, BACKSLASH SRD_FILENAME);
        pDRCtxt->ptrOpt->srdFile=StrNewCopy(pszSRD);
    }

    return;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Composes OmniDR Log filename. It is located in the same directory as
*            the executable.
*
*//*=========================================================================*/
void DRWin32GetOmniDRLogName(tchar **pszFileName)
{
    tchar pszLOG[STRLEN_PATH+1];
    ERH_FUNCTION(_T("DRWin32GetOmniDRLogName"));    
    
    GetModuleFileName(NULL, pszLOG, STRLEN_PATH+1);
    PathCutLeaf(pszLOG);
    strcat(pszLOG, BACKSLASH PANDR_PRECMN_LOG);
    *pszFileName=StrNewCopy(pszLOG);

    return;
}

/*========================================================================*//**
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Gets system root directory.
*
*//*=========================================================================*/
int DRWin32GetSystemRoot(TCHAR *systemRoot, int buffSize)
{
    int iRet=PANDR_RET_ERROR;

    if (GetEnvironmentVariable(_T("windir"), systemRoot, buffSize))
        iRet = PANDR_RET_SUCCESS;
    else if (GetEnvironmentVariable(_T("systemroot"), systemRoot, buffSize))
        iRet = PANDR_RET_SUCCESS;
    else if(GetWindowsDirectory(systemRoot, buffSize))
        iRet = PANDR_RET_SUCCESS;

    return iRet;
}

/*========================================================================*//**
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Copies a file.
*
*//*=========================================================================*/
int DRWin32CopyFile(TCHAR *sourceFile, TCHAR *targetFile, BOOL fail)
{
    int iRet=PANDR_RET_ERROR;

    if(CopyFile(sourceFile, targetFile, fail))
        iRet = PANDR_RET_SUCCESS;

    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Inet Running -  PANDR_INET_FOUND
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     When running in safe mode, it enables Inet installation.
*
*//*=========================================================================*/
#define REG_SAFEMODE_W2K    _T("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\")
#define REG_SAFEMODE_WNT    _T("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\")
#define STR_SERVICE         _T("Service")

static int DRWin32CheckInetSafeMode(PDRContext pDRCtxt)
{
    tchar *pszSafeMode=NULL;
    int iRet=PANDR_RET_ERROR;
    ERH_FUNCTION(_T("DRWin32CheckInetSafeMode"));

    DbgFcnIn();

    __try
    {
        pszSafeMode=GetEnv(_T("SAFEBOOT_OPTION"));
        if (pszSafeMode==NULL)
        {
            DbgPlain(DBG_DR_REGISTER, _T("This IS NOT Safe Mode..."));
            iRet=PANDR_RET_SUCCESS;
            __leave;
        }
        DbgPlain(DBG_DR_REGISTER, _T("This IS Safe Mode..."));

        switch(pDRCtxt->ulTargetPlatform)
        {
            case OS_NT4:
                DbgPlain(DBG_DR_REGISTER, _T("Win NT4... Nothing to put into registry..."));
                break;
            case OS_WXP64:
            case OS_WXPAMD64:
            case OS_WXP:
            case OS_VISTA:
            case OS_VISTA_64:
            case OS_VISTA_AMD64:
            case OS_W2K:
            case OS_WIN7:
            case OS_WIN7_64:
            case OS_WIN7_AMD64:
            case OS_WIN8:
            case OS_WIN8_64:
            case OS_WIN8_AMD64:
            case OS_WIN81:
            case OS_WIN81_64:
            case OS_WIN81_AMD64:
            case OS_WIN10:
            case OS_WIN10_64:
            case OS_WIN10_AMD64:

                DbgPlain(DBG_DR_REGISTER, _T("Win>NT4... Enable Inet in Safe Mode..."));
                if (RegWriteString(HKEY_LOCAL_MACHINE, REG_SAFEMODE_W2K INET_SERVICE_NAME, _T(""), STR_SERVICE)!=0)
                {
                    DbgPlain(DBG_DR_REGISTER, _T("Win>NT4... Enable Inet in Safe Mode FAILURE !!!"));
                    __leave;
                }
                break;
        }
        DbgPlain(DBG_DR_REGISTER, _T("Win>NT4... Enable Inet in Safe Mode SUCCESS !!!"));
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /*Empty*/
    }

    DbgFcnOut();
    return iRet;
}


/*========================================================================*//**
*
* @param     None
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Stops and disable RSM (Removable Storage) service.
*
*//*=========================================================================*/
#define NTMS_SERVICE_NAME   _T("ntmssvc")

int DRWin32DisableRemovableStorage(void)
{
    int retVal = PANDR_RET_ERROR;
    SERVICE_STATUS ssNtms;
    SC_HANDLE svcMgr = NULL, svcNtms = NULL;

    __try
    {
        DWORD startTime = 0;
        BOOL stoppedSuccessfully = FALSE;

        DbgStamp(DBG_DR_REGISTER);
        DbgPlain(DBG_DR_REGISTER, _T("Starting the stopping of NTMS service."));

        svcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if(svcMgr == NULL)
        {
            DbgPlain(DBG_DR_REGISTER, _T("Cannot connect to service database [%d]."),
                GetLastError());
            __leave;
        }

        svcNtms = OpenService(svcMgr, NTMS_SERVICE_NAME, SERVICE_QUERY_STATUS | 
            SERVICE_STOP | SERVICE_CHANGE_CONFIG);
        if(svcNtms == NULL)
        {
            _tprintf(_T("Error opening NTMS service [%d].\n"),
                GetLastError());
            __leave;
        }

        if(!QueryServiceStatus(svcNtms, &ssNtms))
        {
            DbgPlain(DBG_DR_REGISTER, _T("Error querying for service status [%d]."),
                GetLastError());
            __leave;
        }

        if(
            ssNtms.dwCurrentState != SERVICE_STOP_PENDING &&
            ssNtms.dwCurrentState != SERVICE_STOPPED
        )
        {
            if(!ControlService(svcNtms, SERVICE_CONTROL_STOP, &ssNtms))
            {
                DbgPlain(DBG_DR_REGISTER, _T("Cannot stop NTMS service [%d]."),
                    GetLastError());
                __leave;
            }
        }
        else
        {
            DbgPlain(DBG_DR_REGISTER, _T("NTMS service is already stopped or is about to be."));
        }

        startTime = GetTickCount();
        do {
            if(!QueryServiceStatus(svcNtms, &ssNtms))
            {
                DbgPlain(DBG_DR_REGISTER, _T("Error querying for service status [%d]."),
                    GetLastError());
            }
            else if(ssNtms.dwCurrentState == SERVICE_STOPPED)
            {
                stoppedSuccessfully = TRUE;
                break;
            }

            Sleep(1000);
        } while((GetTickCount() - startTime) < 60000);

        if(!stoppedSuccessfully)
        {
            DbgPlain(DBG_DR_REGISTER, _T("Error stopping NTMS service in 1 minute."));
            __leave;
        }
        else
        {
            DbgPlain(DBG_DR_REGISTER, _T("NTMS service stopped within 1 minute timeframe."));
        }

        if(!ChangeServiceConfig(
            svcNtms,
            SERVICE_NO_CHANGE,      /* service type */
            SERVICE_DISABLED,       /* start type */
            SERVICE_NO_CHANGE,      /* error control */
            NULL,                   /* path name */
            NULL,                   /* load group */
            NULL,                   /* tag ID */
            NULL,                   /* dependencies */
            NULL,                   /* service start name */
            NULL,                   /* password */
            NULL                    /* display name*/
            )
        )
        {
            DbgPlain(DBG_DR_REGISTER, _T("Error changing NTMS service start type [%d]."),
                GetLastError());
            __leave;
        }

        DbgStamp(DBG_DR_REGISTER);
        DbgPlain(DBG_DR_REGISTER, _T("NTMS service successfully stopped and disabled."));

        retVal = PANDR_RET_SUCCESS;
    }
    __finally
    {
        if(svcNtms != NULL)
        {
            CloseServiceHandle(svcNtms);
        }
        if(svcMgr != NULL)
        {
            CloseServiceHandle(svcMgr);
        }
    }

    return retVal;
}

/*========================================================================*//**
*
* @param     startupType  - one of the MS predefined service start values
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Sets RSM (Removable Storage) service startup type.
*
*//*=========================================================================*/
int DRWin32ConfigureRemovableStorage(DWORD startupType)
{
    int retVal = PANDR_RET_ERROR;
    SC_HANDLE svcMgr = NULL, svcNtms = NULL;

    __try
    {
        DbgStamp(DBG_DR_REGISTER);
        DbgPlain(DBG_DR_REGISTER, _T("Starting the configuration of NTMS service."));

        svcMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if(svcMgr == NULL)
        {
            DbgPlain(DBG_DR_REGISTER, _T("Cannot connect to service database [%d]."),
                GetLastError());
            __leave;
        }

        svcNtms = OpenService(svcMgr, NTMS_SERVICE_NAME, SERVICE_CHANGE_CONFIG);
        if(svcNtms == NULL)
        {
            DbgPlain(DBG_DR_REGISTER, _T("Error opening NTMS service [%d]."),
                GetLastError());
            __leave;
        }

        if(!ChangeServiceConfig(
            svcNtms,
            SERVICE_NO_CHANGE,      /* service type */
            startupType,            /* start type */
            SERVICE_NO_CHANGE,      /* error control */
            NULL,                   /* path name */
            NULL,                   /* load group */
            NULL,                   /* tag ID */
            NULL,                   /* dependencies */
            NULL,                   /* service start name */
            NULL,                   /* password */
            NULL                    /* display name*/
            )
        )
        {
            DbgPlain(DBG_DR_REGISTER, _T("Error changing NTMS service start type [%d]."),
                GetLastError());
            __leave;
        }

        DbgStamp(DBG_DR_REGISTER);
        DbgPlain(DBG_DR_REGISTER, _T("NTMS service successfully configured."));

        retVal = PANDR_RET_SUCCESS;
    }
    __finally
    {
        if(svcNtms != NULL)
        {
            CloseServiceHandle(svcNtms);
        }
        if(svcMgr != NULL)
        {
            CloseServiceHandle(svcMgr);
        }
    }

    return retVal;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Inet Running -  PANDR_INET_FOUND
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Starts Inet service if necessary.
*            1. Open Service Manager
*            2. Try to open Inet Service. If it does not exist, proceed.
*            3. When running in safe mode, make sure that Inet can be installed.
*            4. Create service, and wait up to 60 seconds to see if running properly.
*
*//*=========================================================================*/
#define INET_ARGS       4
#define INET_DEBUGS     {INET_SERVICE_NAME, _T("-debug"), _T("1-200"), _T("inet.txt")}

int DRWin32StartInet(PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32StartInet"));
    int iRet=PANDR_RET_ERROR, iIter;
    SC_HANDLE schService=NULL;
    SC_HANDLE schSCManager=NULL;
    SERVICE_STATUS ssOmniInet={0};
    tchar pszInetImage[STRLEN_PATH+1];
    DWORD dwError;
    tchar *pszInetDebugs[INET_ARGS]=INET_DEBUGS;    

    DbgFcnIn();

    __try
    {
        /*---------------------------------------------------------------------
        |   Open Service Manager
         --------------------------------------------------------------------*/
        schSCManager=
            OpenSCManager(
                NULL,                       /* machine (NULL == local)    */
                NULL,                       /* database (NULL == default) */
                SC_MANAGER_ALL_ACCESS       /* access required            */
                );
        if (schSCManager==NULL)
        {            
            dwError=GetLastError();            
            DbgStamp(DBG_DR_REGISTER);
            DbgPlain(DBG_DR_REGISTER, _T("OpenSCManager() -> System Error: %d"), dwError);
            ErhFullReport(
                __FCN__, ERH_CRITICAL,
                NlsGetMessage(
                    NLS_SET_DSTRECOVER, NLS_PANDR_SYSTEM_ERROR,
                    dwError, ErhSysErrnoToText(dwError)
                    )            
                );
            __leave;
        }
        DbgPlain(DBG_DR_REGISTER,_T("OpenSCManager() ... Success."));        

        /*---------------------------------------------------------------------
        |   Try to open Inet Service
         --------------------------------------------------------------------*/
        schService=OpenService(schSCManager, INET_SERVICE_NAME, SERVICE_QUERY_STATUS);        

        /*---------------------------------------------------------------------
        |   Service aleardy exists
         --------------------------------------------------------------------*/
        if (schService!=NULL)
        {
            DbgPlain(DBG_DR_REGISTER,_T("Service already exists. Is it running?."));
            dwError=QueryServiceStatus(schService,&ssOmniInet);
            
            if (dwError==0)
            {
                dwError=GetLastError();
                DbgStamp(DBG_DR_REGISTER);
                DbgPlain(
                    DBG_DR_REGISTER, _T("QueryServiceStatus->System Error: %d"), dwError
                    );

                ErhFullReport(
                    __FCN__, ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_DSTRECOVER, NLS_PANDR_SYSTEM_ERROR, dwError, ErhSysErrnoToText(dwError)
                    )            
                );
                __leave;
            }
                        
            if (ssOmniInet.dwCurrentState==SERVICE_RUNNING)
            {
                DbgPlain(
                    DBG_DR_REGISTER,
                    _T("Service already exists and is running. Nothing else to do.")
                    );
                iRet=PANDR_INET_FOUND;
                __leave;
            } 
            
            DbgPlain(DBG_DR_REGISTER, _T("OmniInet service status=%d"), ssOmniInet.dwCurrentState);
    
            /*--- ATTENTION ---------------------------------------------------
            |   Sometimes during DRIM recovery happens, that OmniInet is  reported to be successfully 
            |   uninstalled by DRWin32UninstallInet(). 
            |
            |   However, later on, when the DR Inet is being installed here, the Service Manger reports 
            |   that Inet is installed, but it is not running, hence we can not install it.
            |
            |   Therefore the system will reboot, and after the boot the Inet will not be installed 
            |   at all and we will be able to install it here. 
            |   
            |   Oh yes, and do it only once (instance==0).
             ----------------------------------------------------------------*/
            if ((IS_DRIM(pDRCtxt))&&(pDRCtxt->ptrOpt->instance==0))
            {
                if (DRWin32DrimPrepareForReboot(pDRCtxt)==PANDR_RET_SUCCESS)
                {
                    ErhFullReport(
                        __FCN__, ERH_NORMAL,
                        NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_INETREBOOT)
                    );            
                    Sleep(5000);

                    iRet=PANDR_DRIM_INETFAILED;
                    __leave;
                }
            }

            ErhFullReport(
                __FCN__, ERH_CRITICAL,
                NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_INET_NOTRUNNING)           
                );            

            __leave;
        }        

        /*---------------------------------------------------------------------
        |   Check for Safe Mode.
         --------------------------------------------------------------------*/
        if (DRWin32CheckInetSafeMode(pDRCtxt)!=PANDR_RET_SUCCESS)
        {
            __leave;
        }

        /*---------------------------------------------------------------------
        |   Try to create service.
         --------------------------------------------------------------------*/
        RegReadString(HKEY_LOCAL_MACHINE, REGCOMMON, REGVAL_HOMEDIR, pszInetImage, STRLEN_PATH);
        strcat(pszInetImage, BIN_PATH _T("\\") INET_SERVICE_EXE);

        DbgPlain(DBG_DR_REGISTER, _T("OmniInet image: %s"), pszInetImage);
 
        schService=CreateService(
                       schSCManager,              /* SCManager database      */
                       INET_SERVICE_NAME,         /* name of service         */
                       INET_SERVICE_DISPNAME,     /* name to display         */
                       SERVICE_ALL_ACCESS,        /* desired access          */
                       SERVICE_WIN32_OWN_PROCESS, /* service type            */
                       SERVICE_AUTO_START,        /* start type              */
                       SERVICE_ERROR_NORMAL,      /* error control type      */
                       pszInetImage,              /* service's binary        */
                       NULL,                      /* no load ordering group  */
                       NULL,                      /* no tag identifier       */
                       NULL,                      /* dependencies            */
                       NULL,                      /* LocalSystem account     */
                       NULL                       /* no password             */
                       );
        
        /*---------------------------------------------------------------------
        |   Service Creation Failed.
         --------------------------------------------------------------------*/
        if (schService==NULL)
        {
            dwError=GetLastError();
            DbgStamp(DBG_DR_REGISTER);
            DbgPlain(DBG_DR_REGISTER,_T("CreateService -> System Error: %d"), dwError);
            __leave;
        }
        DbgPlain(DBG_DR_REGISTER,_T("Inet service created."));

        /*---------------------------------------------------------------------
        |   Try to run service.
         --------------------------------------------------------------------*/
        if (pDRCtxt->ptrOpt->debugRanges==NULL)
        {
            dwError=StartService(schService, 0, NULL);
        }
        else
        {
            dwError=StartService(schService, INET_ARGS, pszInetDebugs);
        }        
        DbgPlain(DBG_DR_REGISTER,_T("Service was started. Is it running properly?."));

        ErhFullReport(
            __FCN__, ERH_NORMAL,
            NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_STARTING_INET)            
            );
        
        /*--- ATTENTION -------------------------------------------------------
        |
        |   A long timeout of 60s was chosen, because in certain circumstances
        |   (e.g. network cable unplugged, which is equivalent of networking
        |   unavailable due to disaster) CmnExpandHostname() takes up to 30s 
        |   before it returns. Since every OmniBack module (including Inet) 
        |   tries to expand CellManager's name, they will wait at least for 30s
        |   before start.
        |
        |   Query for service status, until it is SERVICE_RUNNING or 60s 
        |   timeout occurs.
         --------------------------------------------------------------------*/
        ssOmniInet.dwCurrentState=SERVICE_STOPPED;
        for (iIter=0; iIter<60; iIter++)
        {          
            Sleep(1000);
            dwError=QueryServiceStatus(schService, &ssOmniInet);

            if (dwError==0)
            {
                dwError=GetLastError();            
                DbgStamp(DBG_DR_REGISTER);
                DbgPlain(
                    DBG_DR_REGISTER, _T("QueryServiceStatus -> System Error: %d"), dwError
                    );

                ErhFullReport(
                    __FCN__, ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_DSTRECOVER, NLS_PANDR_SYSTEM_ERROR,
                        dwError, ErhSysErrnoToText(dwError)
                        )            
                    );
                __leave;
            }
            if (ssOmniInet.dwCurrentState==SERVICE_RUNNING)
            {
                break;
            }
        }

        if (ssOmniInet.dwCurrentState!=SERVICE_RUNNING)
        {
            DbgPlain(DBG_DR_REGISTER, _T("Service is not running properly."));
            ErhFullReport(
                __FCN__, ERH_CRITICAL,
                NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_INET_FAILED)            
                );
            __leave;
        }

        ErhFullReport(
            __FCN__, ERH_NORMAL,
            NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_INET_STARTED)            
            );

        DbgPlain(DBG_DR_REGISTER,_T("Inet service started successfully."));
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        if (schService!=NULL)
        {
            CloseServiceHandle(schService);
        }
        if (schSCManager!=NULL)
        {
            CloseServiceHandle(schSCManager);
        }
    }

    DbgFcnOut();
    return(iRet);
}


/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Inet Running -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Uninstalls Inet service if necessary.
*            1. Open Service Manager.
*            2. Try to open Inet Service. On failure, there is nothing to do.
*            3. If service is not running, delete it.
*
*//*=========================================================================*/
int DRWin32UninstallInet(PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32UninstallInet"));
    tchar pszBuffer[1024];
    int iRet=PANDR_RET_ERROR;
    SC_HANDLE schService=NULL;
    SC_HANDLE schSCManager=NULL;
    SERVICE_STATUS ssOmniInet;
    DWORD dwError;
    
    DbgFcnIn();    
    PUT_TRACE(pDRCtxt, _T("Function Entered."))

    __try
    {
        /*---------------------------------------------------------------------
        |   Open Service Manager
         --------------------------------------------------------------------*/
        schSCManager=
            OpenSCManager(
                NULL,                       /* machine (NULL == local)    */
                NULL,                       /* database (NULL == default) */
                SC_MANAGER_ALL_ACCESS       /* access required            */
                );
        if (schSCManager==NULL)
        {            
            dwError=GetLastError();                        
            sprintf(pszBuffer, _T("%d"), dwError);
            START_TRACE(pDRCtxt, _T("OpenSCManager() -> System Error: "));
            APPEND_TO_TRACE(pDRCtxt, pszBuffer);
            APPEND_TO_TRACE(pDRCtxt, _T("\n"));
            __leave;
        }
        PUT_TRACE(pDRCtxt, _T("OpenSCManager() ... Success."));

        /*---------------------------------------------------------------------
        |   Try to open Inet Service
         --------------------------------------------------------------------*/
        schService=OpenService(schSCManager, INET_SERVICE_NAME, SERVICE_QUERY_STATUS|DELETE);
        /*---------------------------------------------------------------------
        |   Service does not exist.
         --------------------------------------------------------------------*/
        if (schService==NULL)
        {
            DbgStamp(DBG_DR_REGISTER);
            PUT_TRACE(pDRCtxt, _T("OmniInet is not present. Nothing to uninstall."));
            iRet=PANDR_RET_SUCCESS;
            __leave;
        }
        PUT_TRACE(pDRCtxt, _T("OpenService() ... Success."));

        dwError=QueryServiceStatus(schService, &ssOmniInet);
        if (dwError==0)
        {
            dwError=GetLastError();
            sprintf(pszBuffer, _T("%d"), dwError);
            START_TRACE(pDRCtxt, _T("QueryServiceStatus->System Error: "));
            APPEND_TO_TRACE(pDRCtxt, pszBuffer);
            APPEND_TO_TRACE(pDRCtxt, _T("\n"));           
            __leave;
        }

        if (ssOmniInet.dwCurrentState==SERVICE_RUNNING)
        {
            PUT_TRACE(pDRCtxt, _T("OmniInet service already exists and is running. Nothing else to do."));
            iRet=PANDR_RET_SUCCESS;
            __leave;
        } 
        PUT_TRACE(pDRCtxt, _T("Trying to delete OmniInet."));

        /*---------------------------------------------------------------------
        |   Try to delete service.
         --------------------------------------------------------------------*/
        dwError=DeleteService(schService);
        if (dwError==0)
        {            
            dwError=GetLastError();
            sprintf(pszBuffer, _T("%d"), dwError);
            START_TRACE(pDRCtxt, _T("DeleteService->System Error: "));
            APPEND_TO_TRACE(pDRCtxt, pszBuffer);
            APPEND_TO_TRACE(pDRCtxt, _T("\n"));
            __leave;
        }
        PUT_TRACE(pDRCtxt, _T("DeleteService() suceedded... Going to Sleep() for 3 s..."));
        Sleep(3000);
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        if (schService!=NULL)
        {
            if (!CloseServiceHandle(schService))
            {
                dwError=GetLastError();
                sprintf(pszBuffer, _T("%d"), dwError);
                START_TRACE(pDRCtxt, _T("CloseServiceHandle->System Error: "));
                APPEND_TO_TRACE(pDRCtxt, pszBuffer);
                APPEND_TO_TRACE(pDRCtxt, _T("\n")); 
            }
            else
            {
                PUT_TRACE(pDRCtxt, _T("OmniInet handle closed!"));
            }
        }
        if (schSCManager!=NULL)
        {
            if (!CloseServiceHandle(schSCManager))
            {
                dwError=GetLastError();
                sprintf(pszBuffer, _T("%d"), dwError);
                START_TRACE(pDRCtxt, _T("CloseServiceHandle->System Error: "));
                APPEND_TO_TRACE(pDRCtxt, pszBuffer);
                APPEND_TO_TRACE(pDRCtxt, _T("\n"));
            }
            else
            {
                PUT_TRACE(pDRCtxt, _T("Service Manager handle closed!"));
            }
        }
    }

    DbgFcnOut();

    sprintf(pszBuffer, _T("%d"), iRet);
    START_TRACE(pDRCtxt, _T("Function Exited. iRet="));
    APPEND_TO_TRACE(pDRCtxt, pszBuffer);
    APPEND_TO_TRACE(pDRCtxt, _T("\n")); 
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt    -   pointer to DR Context
*
* @retval    Success      -  PANDR_RET_SUCCESS
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Stop services, which can cause trouble during amnual recovery of W2K.
*            Results as a fix of HSLco33636. Also related to HSLco29277.
*
*            1. Open Service Manager.
*            2. Loop through Services list
*            3. If service is running, try to stop it.
*
*//*=========================================================================*/
int DRWin32StopServices(PDRContext pDRCtxt)
{
    return 0;
}

/*========================================================================*//**
*
* @param     pszServiceName    -   Service name
*
* @retval    Success      -  Service State or PANDR_SRV_NOTINSTALLED
* @retval    Error        -  PANDR_RET_ERROR
*
* @brief     Get current service status.
*
*            1. Open Service Manager.
*            2. Query for service status.
*            3. Close handles.
*
*//*=========================================================================*/
int DRWin32GetServiceState(tchar *pszServiceName)
{
    ERH_FUNCTION(_T("DRWin32GetServiceState"));
    int iRet=PANDR_RET_ERROR;
    SC_HANDLE scSrvManager=NULL;
    SC_HANDLE scService=NULL;
    SERVICE_STATUS ssService={0};
    DWORD dwError;
    
    DbgFcnIn();
    __try
    {
        /*---------------------------------------------------------------------
        |   Open Service Manager
         --------------------------------------------------------------------*/
        scSrvManager=
            OpenSCManager(
                NULL,                       /* machine (NULL == local)    */
                NULL,                       /* database (NULL == default) */
                SC_MANAGER_ALL_ACCESS       /* access required            */
                );

        if (scSrvManager==NULL)
        {            
            dwError=GetLastError();            
            DbgStamp(DBG_DR_REGISTER);
            DbgPlain(
                DBG_DR_REGISTER, 
                _T("OpenSCManager() -> System Error: %d: %s"), 
                dwError, ErhSysErrnoToText(dwError)
                );            
            __leave;
        }
        DbgPlain(DBG_DR_REGISTER,_T("OpenSCManager() ... Success."));        

        /*---------------------------------------------------------------------
        |   Try to open Service.
         --------------------------------------------------------------------*/
        scService=OpenService(scSrvManager, pszServiceName, SERVICE_QUERY_STATUS);
        if (scService==NULL)
        {
            if (GetLastError()==ERROR_SERVICE_DOES_NOT_EXIST)
            {
                iRet=PANDR_SRV_NOTINSTALLED;
            }
            __leave;
        }        

        /*---------------------------------------------------------------------
        |   Query Service's status.
         --------------------------------------------------------------------*/

        DbgPlain(DBG_DR_REGISTER,_T("Service exists. What is its status?."));
        dwError=QueryServiceStatus(scService, &ssService);
            
        if (dwError==0)
        {
            dwError=GetLastError();
            DbgStamp(DBG_DR_REGISTER);
            DbgPlain(
                DBG_DR_REGISTER,
                _T("QueryServiceStatus->System Error: %d: %s"), 
                dwError, ErhSysErrnoToText(dwError)
                );
            __leave;
        }
                        
        iRet=ssService.dwCurrentState;
    }
    __finally
    {
        if (scService!=NULL)
        {
            CloseServiceHandle(scService);
        }
        if (scSrvManager!=NULL)
        {
            CloseServiceHandle(scSrvManager);
        }
    }           
    DbgFcnOut();
    return iRet;
}


/*========================================================================*//**
*
* @param     pDRCtxt   -  pointer to DR context
*
* @retval    PANDR_RET_SUCCESS - if scope can be fulfilled.
* @retval    PANDR_RET_ERROR   - if scope can not be fulfilled.
*
* @brief     Based on CLI options or DRIM scope it sets the VOL_SKIP flag as
*            necessary.
*            If any of required volumes is not writable, returns PANDR_RET_ERROR.
*
*//*=========================================================================*/
int DRWin32AssertScope(PDRContext pDRCtxt)
{
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVol;
    OptRestoreMode drOldMode;

    ERH_FUNCTION (_T("DRWin32AssertScope"));

    DbgFcnIn();

    drOldMode=pDRCtxt->ptrOpt->mode;

    switch (DRWin32DRIMSetScope(pDRCtxt))
    {
        case PANDR_RET_SUCCESS:
            break;
        case PANDR_NO_DRIM:
            break;
        case PANDR_RET_ERROR:
            pDRCtxt->ptrOpt->mode=drOldMode;
            goto end;
            break;
    }

    if (pDRCtxt->bMSCluster)
    {
        PVolRec pQourum=NULL;
        
        DbgPlain(DBG_DR_INIT, _T("OK, I was a part of a cluster..."));

        for (ulVol=0; ulVol<pDRCtxt->pVolRecTab->nEl; ulVol++)
        {
            pQourum=(PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVol]);
            if (pQourum->iPurpose & DR_QUORUM_VOLUME)
            {                
                DbgPlain(DBG_DR_INIT, _T("The volume %s is a cluster qourum resource."), pQourum->pszVolMnt);
                break;
            }
        }
        
        if (!(pQourum->iPurpose & DR_QUORUM_VOLUME))
        {            
            pQourum=NULL;
            DbgPlain(DBG_DR_INIT, _T("The volume is either an MNS Quorum resource or not at all a qourum resource. Resetting pQourum to NULL..."));
        }

        if (pDRCtxt->ptrOpt->msclusdb)
        {
            pDRCtxt->iMSCSMode=MSCS_DB_CLUS;
            DbgPlain(DBG_DR_INIT, _T("Cluster mode: MSCS_DB_CLUS"));
        }
        else if (pDRCtxt->ptrOpt->mode==MODE_FULL_CLUS)
        {
            pDRCtxt->iMSCSMode=MSCS_FORM_CLUS;
            DbgPlain(DBG_DR_INIT, _T("Cluster mode: MSCS_FORM_CLUS"));
        } 
        else if (DRWin32IsASR() && (pQourum!=NULL) && (pQourum->bIsWritable))
        {
            pDRCtxt->iMSCSMode=MSCS_FORM_CLUS;
            DbgPlain(DBG_DR_INIT, _T("Cluster mode: MSCS_FORM_CLUS"));
        }
        else
        {            
            pDRCtxt->ptrOpt->cmdb=FALSE;
            pDRCtxt->iMSCSMode=MSCS_JOIN_CLUS;
            DbgPlain(DBG_DR_INIT, _T("Cluster mode: MSCS_JOIN_CLUS"));

            if (pDRCtxt->bIamClusCM)
            {
                DbgPlain(DBG_DR_INIT, _T("CM database will not be restored."));
                ErhFullReport(
                    __FCN__, ERH_NORMAL,
                    NlsGetMessage(NLS_SET_DSTRECOVER, NLS_PANDR_MSCS_NO_CMDB)            
                    );
            }
        }
         
        if(DRWin32ClusterSkipVolumes(pDRCtxt)!=PANDR_RET_SUCCESS)
        {
            goto end;
        }
    }

    for (ulVol=0; ulVol<pDRCtxt->pVolRecTab->nEl; ulVol++)
    {
        PVolRec pVol=(PVolRec)(pDRCtxt->pVolRecTab->ptrEl[ulVol]);

        /*---------------------------------------------------------------------
        |   We always restore at least BOOT, SYSTEM (including ADS, Profiles) 
        |   and CONFIGURATION volume.
         --------------------------------------------------------------------*/
        if (pVol->iPurpose & (DR_BOOT_VOLUME|DR_SYSTEM_VOLUME|DR_CFG_VOLUME|DR_ADS_VOLUME|DR_PROFILES_VOLUME))
        {            
            if (!pVol->bIsWritable)
            {
                DbgStamp(DBG_DR_INIT);
                DbgPlain(DBG_DR_INIT, _T("Volume %s is not writable."), pVol->pszVolMnt);
                ErhFullReport(
                    __FCN__, ERH_CRITICAL,
                    NlsGetMessage(
                        NLS_SET_DSTRECOVER, NLS_PANDR_INACCESSIBLE_VOLUME, pVol->pszVolMnt
                        )            
                    );
                goto end;
            }
            continue;
        }

        /*---------------------------------------------------------------------
        |   Additionally, we restore all OmniBack related volumes 
        |   (CM DB and installation dir)
         --------------------------------------------------------------------*/
        if (pVol->iPurpose & (DR_OB2_VOLUME|DR_OB2_DATA_VOLUME|DR_OB2DB_VOLUME|DR_OB2DB_FILES|DR_OB2DB_CONFIG))
        {
            if ((pDRCtxt->ptrOpt->mode<MODE_DEFAULT) || (!pVol->bIsWritable))
            {                
                DbgStamp(DBG_DR_INIT);
                if (!pVol->bIsWritable)
                {
                    DbgPlain(
                        DBG_DR_INIT, _T("Volume %s is not writable."), pVol->pszVolMnt
                        );
                }
                DbgPlain(
                    DBG_DR_INIT, _T("Scope=%d, volume %s is skipped."), 
                    pDRCtxt->ptrOpt->mode, pVol->pszVolMnt
                    );
                pVol->state=VOL_SKIP;
                continue;
            }

            /*-----------------------------------------------------------------
            |   If CM DB is explicitely excluded...
             ----------------------------------------------------------------*/
            if ((!pDRCtxt->ptrOpt->cmdb)&&(pVol->iPurpose & (DR_OB2DB_FILES)))
            {                
                DbgPlain(
                    DBG_DR_INIT, 
                    _T("-no_cmdb results in skipping the volume %s."), pVol->pszVolMnt
                    );
                pVol->state=VOL_SKIP;
            }

            continue;
        }

        if (pDRCtxt->ptrOpt->mode<MODE_FULL)
        {
            DbgPlain(
                DBG_DR_INIT, _T("Scope=%d, volume %s is skipped."), 
                pDRCtxt->ptrOpt->mode, pVol->pszVolMnt
                );
            pVol->state=VOL_SKIP;
        }

        /**
         * Skip VHD volume
         */
        if (pVol->iPurpose & DR_VHD_VOLUME)
        {
            DbgPlain(
                DBG_DR_INIT, _T("Skip VHD volume: %s"), 
                pVol->pszVolMnt
                );
            pVol->state = VOL_SKIP;
        }
    }

    if (pDRCtxt->ptrOpt->mode<MODE_DEFAULT)
    {
        DbgStamp(DBG_DR_INIT);
        DbgPlain(
            DBG_DR_INIT, _T("Mode %d implies -no_clean!"), pDRCtxt->ptrOpt->mode
            );
        pDRCtxt->ptrOpt->clean=FALSE;
    }

    iRet=PANDR_RET_SUCCESS;
end:
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -  pointer to DR context
*
* @retval    PANDR_RET_SUCCESS - if scope was saved.
* @retval    PANDR_RET_ERROR   - if scope can not be saved.
*
* @brief     Save recovery scope when in monitor mode.
*
*//*=========================================================================*/
int DRWin32SaveRecoveryScope(PDRContext pDRCtxt)
{
    ERH_FUNCTION (_T("DRWin32SaveRecoveryScope"));

    int iRet=PANDR_RET_ERROR;
    int iwriteSize = 0;
    PVolRec pVol = NULL;
    R_ULONG ulVolIdx = 0;
    R_ULONG ulSessIdx = 0;
    R_UINT64 copyId=(R_UINT64)0;
    TCHAR pszOutBuffer[STRLEN_PATH + 1];
    TCHAR *res_obj = NULL;
    FILEHANDLE  hfile = INVALID_HANDLE_VALUE;
    TCHAR scope_file[STRLEN_PATH + 1];
    TCHAR sessionid[MAX_PATH];
    R_BOOL bResumed = FALSE;

    DbgFcnIn();

    __try
    {
        USES_SRDQUERY;

        strcpy(pszOutBuffer, pDRCtxt->ptrOpt->drimINI);
        PathCutLeaf(pszOutBuffer);
        sprintf(scope_file, _T("%s\\%s"), pszOutBuffer, _T("restore_scope.txt"));

        hfile = OB2_OpenFile(scope_file, 1, 1);
        if (INVALID_HANDLE_VALUE == hfile)
        {
            DbgPlain(
                DBG_DR_SRDQUERY,
                _T("Failed to create output file.")
                );
            __leave;
        }

        for (ulVolIdx = 0; ulVolIdx < pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            pVol = (PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
            if (pVol->state != VOL_PENDING)
            {
                continue;
            }

            for (ulSessIdx=0; ulSessIdx < pVol->avSessions.argc; ulSessIdx++)
            {
                if ((pVol->iPurpose & (DR_CFG_VOLUME)) &&
                    ((ulSessIdx + 1) < pVol->avSessions.argc))
                {
                    continue;
                }

                res_obj = StrNewCopy(_T("-volume "));
                StrAppend(res_obj, pVol->pszVolMnt);

                StrAppend(res_obj, _T(" -type "));
                switch (pVol->ulType)
                {
                    case OT_WINFS:
                        StrAppend(res_obj, _T("WinFS"));
                        break;
                    case OT_FILESYSTEM:
                        StrAppend(res_obj, _T("Filesystem"));
                        break;
                    case OT_RAWDISK:
                        StrAppend(res_obj, _T("RAWDisk"));
                        break;
                    case OT_OB2BAR:
                        StrAppend(res_obj, _T("IDB"));
                        break;
                    default:
                        StrAppend(res_obj, _T("<UNKNOWN>"));
                        break;
                }

                StrAppend(res_obj, _T(" -label "));
                StrAppend(res_obj, pVol->pszLabel);

                StrAppend(res_obj, _T(" -session "));
                TrimResumedSessionIdx(pVol->avSessions.argv[ulSessIdx], sessionid, &bResumed);
                StrAppend(res_obj, sessionid);

                if (ulSessIdx)
                {
                    if (bResumed)
                        StrAppend(res_obj, _T("  (resumed)"));
                    else
                        StrAppend(res_obj, _T("  (inc)"));
                }
                else
                {
                    StrAppend(res_obj, _T("  (full)"));
                }

                PREPARE_SRDQUERY(ulVolIdx, ulSessIdx, GOSI_COPYID, &copyId, NULL);
                if (DO_SRDQUERY(
                    pDRCtxt->srdH, SRDACT_GET_OBJECT_SESSION_INFO, SRDHT_SYSINFO
                    )!=SRDERR_SUCCESS)
                {
                    DbgPlain(
                        DBG_DR_SRDQUERY,
                        _T("RecoveryInformationAction(GOSI_COPYID) failure.")
                        );
                    __leave;
                }

                if (copyId)
                {
                    StrAppend(res_obj, _T(" (copy)"));
                }

                memset(pszOutBuffer, 0, MAX_PATH);
                PREPARE_SRDQUERY(ulVolIdx, ulSessIdx, GOSI_DAID, pszOutBuffer, NULL);
                if (DO_SRDQUERY(
                    pDRCtxt->srdH, SRDACT_GET_OBJECT_SESSION_INFO, SRDHT_SYSINFO
                    )!=SRDERR_SUCCESS)
                {
                    DbgPlain(
                        DBG_DR_SRDQUERY,
                        _T("RecoveryInformationAction(GOSI_DAID) failure.")
                        );
                    __leave;
                }

                StrAppend(res_obj, _T(" -daid "));
                StrAppend(res_obj, pszOutBuffer);

                StrAppend(res_obj, _T("\r\n"));

                iwriteSize = (int)(strlen(res_obj) * sizeof(TCHAR));
                if (OB2_WriteFile(hfile, res_obj, iwriteSize) != iwriteSize)
                {
                    DbgPlain(
                        DBG_DR_SRDQUERY,
                        _T("Failed to write to out file.")
                        );
                    __leave;
                }
                FREE(res_obj);
            }
        }

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        FREE(res_obj);
        if (hfile != INVALID_HANDLE_VALUE)
        {            
            OB2_CloseFile(hfile);
        }
    }

    DbgFcnOut();

    return iRet;
}

/*========================================================================*//**
*
* @param     pszMountPoint   -   Volume Mountpoint
*
* @retval    Volume is Write Accessible      -  TRUE
* @retval    Volume is NOT Write Accessible  -  FALSE
*
* @brief     Tries to write and delete a file to a volume.
*
*//*=========================================================================*/
int DRWin32IsVolumeWritable(tchar *pszMountPoint)
{
	int bIsWritable=FALSE;
    HANDLE hTest=NULL;
    tchar pszFileName[STRLEN_PATH+1];
    DWORD dwBytes;
   
    ERH_FUNCTION (_T("DRWin32IsVolumeWritable"));

    pszFileName[0]=NULL_TCHAR;
    __try
    {
        if ((wcsstr(pszMountPoint, _T("\\\\?\\Volume{"))!=NULL) || (wcsstr(pszMountPoint, _T("//?/Volume{"))!=NULL))
        {
            strcat(pszFileName, pszMountPoint+1);
        }
        else
        {
            strcat(pszFileName, pszMountPoint+1);
            if (pszFileName[1]!=_T(':'))
            {
                strcat(pszFileName, _T(":"));
            }
        }
        
        strcat(pszFileName, _T("\\WriteMe.tst"));        
        PathToBackslashes(pszFileName);

        hTest=CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                  FILE_ATTRIBUTE_NORMAL, NULL
                  );
        if (hTest==INVALID_HANDLE_VALUE)
        {
            dwBytes=GetLastError();
            __leave;
        }

        if (!WriteFile(hTest, (char*)pszFileName,
                 (DWORD)strlen(pszFileName)*sizeof(tchar), &dwBytes, NULL
                 )
           )
        {
            __leave;
        }
         
        bIsWritable=TRUE;
    }
    __finally
    {        
        if (hTest!=INVALID_HANDLE_VALUE)
        {            
            CloseHandle(hTest);
        }
        DeleteFile(pszFileName);
    }
   
    return bIsWritable;
}


/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Sets required privileges.
*
*//*=========================================================================*/
static int DRWin32SetProcessPrivilege (tchar *lpszPrivilege, BOOL on)
{
	ERH_FUNCTION (_T("DRWin32SetProcessPrivilege"));

	int					iRet = PANDR_RET_ERROR;
	HANDLE				hToken=NULL;
	TOKEN_PRIVILEGES	tpRec = {0};

	DbgFcnIn();

	tpRec.PrivilegeCount = 1;
	tpRec.Privileges[0].Attributes = on ? SE_PRIVILEGE_ENABLED : 0;

	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		DbgStamp (DBG_DR_ACTION);
		DbgPlain (DBG_DR_ACTION, _T("Cannot open process token! Error: %d\n"), GetLastError ());
	}
	else if (!LookupPrivilegeValue (NULL, lpszPrivilege, &tpRec.Privileges[0].Luid))
	{
		DbgStamp (DBG_DR_ACTION);
		DbgPlain (DBG_DR_ACTION, _T("Cannot lookup privilege value! Error: %d\n"), GetLastError ());
	}
	else if (!AdjustTokenPrivileges (hToken, FALSE, &tpRec, 0, (PTOKEN_PRIVILEGES) NULL, 0))
	{
		DbgStamp (DBG_DR_ACTION);
		DbgPlain (DBG_DR_ACTION, _T("Cannot adjust token privilege! Error: %d\n"), GetLastError ());
	}
	else if (!CloseHandle (hToken))
	{
		DbgStamp (DBG_DR_ACTION);
		DbgPlain (DBG_DR_ACTION, _T("Cannot close token handle! Error: %d\n"), GetLastError ());
	}
	else
	{
		iRet = PANDR_RET_SUCCESS;
	}

    DbgFcnOut();
	return (iRet);
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. Load restored SOFTWARE hive into registry.
*            2. Write cleanup commands into RunOnce Key.
*            3. Save and unload the hive.
*
*//*=========================================================================*/
static int CleanUpTempOS(PDRContext pDRCtxt)
{
    tchar pszSoftwareHive[STRLEN_PATH+1];
    tchar pszCleanDir[STRLEN_PATH+1];
    tchar pszCleanupCommand[2048];

    HKEY hSoftKey=NULL;
    int bLoadSuccess=FALSE;

    int iRet=PANDR_RET_ERROR;
    LONG lLastError=0;

    /*Locate the restored registry hive*/
    pszSoftwareHive[0]=NULL_TCHAR;
    strcat(pszSoftwareHive, pDRCtxt->pszTargetSysDir);
    strcat(pszSoftwareHive, SYSTEM32_DIR _T("/config/SOFTWARE"));
    PathToBackslashes(pszSoftwareHive);

    __try
    {
         /*Load it into the active registry*/
        DRWin32SetProcessPrivilege(SE_RESTORE_NAME, TRUE);

        lLastError=RegLoadKey(HKEY_LOCAL_MACHINE, RESTORED_HKLM_SOFTWARE, pszSoftwareHive);

        if (lLastError!=ERROR_SUCCESS)
        {
            DbgPlain(10, _T("Cannot load registry hive %s (error = %d)."), pszSoftwareHive, lLastError);
            __leave;
        }
        bLoadSuccess=TRUE;

        /*Create RunOnce Key*/
        lLastError=
            RegCreateKeyEx(
                HKEY_LOCAL_MACHINE, RESTORED_RUNONCE, 0, _T("\0"),
                REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSoftKey, NULL
                );
        if (lLastError!=ERROR_SUCCESS)
        {
            DbgPlain(10, _T("Cannot create key %s (error = %d)."), RESTORED_RUNONCE, lLastError);
            __leave;
        }

        /*Call DRIM Cleanup function*/
        DRWin32DRIMCleanUp(pDRCtxt, hSoftKey);

        *pszCleanupCommand=NULL_TCHAR;
        DRGetSystemRoot(pszCleanDir, STRLEN_PATH);
        sprintf(pszCleanupCommand, TEMPOS_CLEANUP_CMD, pszCleanDir);
        lLastError=
            RegSetValueEx(
                hSoftKey, _T("DR OS Cleanup"), 0, REG_SZ, (char*)pszCleanupCommand,
                (1+(DWORD)strlen(pszCleanupCommand))*sizeof(tchar)
                );
       
        if (lLastError!=ERROR_SUCCESS)
        {
            DbgPlain(10, _T("Cannot set value \"DR OS Cleanup\" = %s (error = %d)."), pszCleanupCommand, lLastError);
            __leave;
        }                

        iRet=PANDR_RET_SUCCESS;

    }
    __finally
    {
        /*On success save and unload*/
        if (hSoftKey!=NULL)
        {
            RegCloseKey(hSoftKey);
        }
        if (bLoadSuccess)
        {
            lLastError=RegUnLoadKey(HKEY_LOCAL_MACHINE, RESTORED_HKLM_SOFTWARE);
            if (lLastError != ERROR_SUCCESS)
                DbgPlain(10, _T("Cannot load registry hive %s (error = %d)."), pszSoftwareHive, lLastError);
        }        
    }

    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. Locate the "All Users\\Start Menu\\Programs\\Startup" directory
*            2. Prepare the cleanup script.
*            3. Save the cleanup script.
*
*//*=========================================================================*/
static int CleanUpActiveOs(PDRContext pDRCtxt)
{        
    HANDLE hCleanup = INVALID_HANDLE_VALUE;
    tchar pszCleanupScript[2048];
    tchar pszStartupPath[MAX_PATH];
    tchar *pszSystemRoot=NULL;
    tchar *pszInstDir=NULL;
    char *pszAsciiScript=NULL;

    LPMALLOC pMalloc = NULL;
	LPITEMIDLIST pidlStartup = NULL;
    int bGotPath = FALSE;

    DWORD dwWritten=0;
    USES_CONVERSION;

	__try
	{
		if(!SUCCEEDED(SHGetMalloc(&pMalloc)))
        {
			__leave;
        }
		if(!SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTUP, &pidlStartup)))
        {
			__leave;
        }
		if(!SUCCEEDED(SHGetPathFromIDList(pidlStartup, pszStartupPath)))
        {
			__leave;
        }
        bGotPath=TRUE;
	}
	__finally
	{
		if(pidlStartup != NULL)
			pMalloc->lpVtbl->Free(pMalloc, pidlStartup);
		if(pMalloc != NULL)
			pMalloc->lpVtbl->Release(pMalloc);

		if(!bGotPath)
		{
			DRGetSystemRoot(pszStartupPath, MAX_PATH);                

			strcpy(
                pszStartupPath+3,
                _T("Documents and Settings\\All Users\\Start Menu\\Programs\\Startup")
                );
		}
	}
    
    strcat(pszStartupPath, _T("\\CleanDR.cmd"));

    __try
    {
        hCleanup=CreateFile(
            pszStartupPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL
            );

        if (hCleanup==INVALID_HANDLE_VALUE)
        {
            __leave;
        }
        
        pszSystemRoot=GetEnv(_T("SYSTEMROOT"));
        if ((pszSystemRoot!=NULL) && (strstr(cmnPanBase, pszSystemRoot)!=NULL))
        {
            pszInstDir=StrNewCopy(_T("%SYSTEMROOT%"));
            StrAppend(pszInstDir, cmnPanBase+strlen(pszSystemRoot));
        }
        else
        {
            pszInstDir=StrNewCopy(cmnPanBase);
        }        
        
        sprintf(pszCleanupScript,_T("rd /q /s \"%s\"\ndel %%0\n"),pszInstDir);
        pszAsciiScript=T2A(pszCleanupScript);
        WriteFile(hCleanup, pszAsciiScript, (DWORD)strlenA(pszAsciiScript), &dwWritten, NULL);
    }
    __finally
    {
        if (hCleanup!=INVALID_HANDLE_VALUE)
        {
            CloseHandle(hCleanup);
        }               
        FREE(pszInstDir);
    }
    return PANDR_RET_SUCCESS;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. If TempOS, copy restored .DEFAULT into DEFAULT registry hive
*            Call TempOS cleanup routine.
*            2. If ActiveOS, call ActiveOS cleanup routine.
*
*//*=========================================================================*/
int DRWin32CleanUp(PDRContext pDRCtxt)
{
    int iRet=PANDR_RET_ERROR;
    ERH_FUNCTION(_T("DRWin32CleanUp"));
    
    DbgFcnIn();
    /*-------------------------------------------------------------------------
    |   If Temp DR OS, copy registry hive .default to default
     ------------------------------------------------------------------------*/
    if (pDRCtxt->tempOS)
    {
        /*---------------------------------------------------------------------
        |   If Temp DR OS, first copy registry hive .default to default
         --------------------------------------------------------------------*/
        tchar pszSource[STRLEN_PATH+1];
        tchar pszDstntn[STRLEN_PATH+1];
        tchar pszWinDir[STRLEN_PATH+1];

        *pszSource=NULL_TCHAR;
        strcat(pszSource, pDRCtxt->pszTargetSysDir);
        strcat(pszSource, SYSTEM32_DIR);
        strcpy(pszDstntn, pszSource);
        
        strcat(pszSource, _T("/config/.default"));
        strcat(pszDstntn,  _T("/config/default"));

        DbgPlain(DBG_DR_ACTION, _T("Copying %s to %s."), pszSource, pszDstntn);
        CopyFile(pszSource, pszDstntn, FALSE);

        DRGetSystemRoot(pszWinDir, STRLEN_PATH+1);

        if (!IsWinVistaOrHigher(pDRCtxt->ulTargetPlatform) &&
            (strnicmp(cmnPanBase, pszWinDir, strlen(pszWinDir)) == 0) && 
            (pDRCtxt->ptrOpt->clean) && (!pDRCtxt->bWasInstalled)
           )
        {      
            iRet=CleanUpTempOS(pDRCtxt);
        }
    }
    else if ((!pDRCtxt->bWasInstalled) && (pDRCtxt->ptrOpt->clean))
    {
        iRet=CleanUpActiveOs(pDRCtxt);
    }
    
    if(DRIsASR())
    {
        __try
        {
            R_ULONG startupType = RST_UNKNOWN;
            USES_SRDQUERY;
            /*---------------------------------------------------------------------
            |   Query SRD for RSM startup type.
             --------------------------------------------------------------------*/
            PREPARE_SRDQUERY(&startupType, NULL, NULL, NULL, NULL);
            if (DO_SRDQUERY(
                    pDRCtxt->srdH, SRDACT_GET_RSM_STARTUP_TYPE, SRDHT_SYSINFO
                    ) != SRDERR_SUCCESS)
            {
                DbgPlain(DBG_DR_SRDQUERY, _T("SRD query error: SRDACT_GET_RSM_STARTUP_TYPE."));
                __leave;
            }

            DbgPlain(DBG_DR_SRDQUERY, _T("SRDACT_GET_RSM_STARTUP_TYPE: %d."), startupType);
            switch(startupType)
            {
                case RST_DISABLED:
                    startupType = SERVICE_DISABLED;
                    break;
                case RST_AUTOMATIC:
                    startupType = SERVICE_AUTO_START;
                    break;
                default:
                    startupType = SERVICE_DEMAND_START;
                    break;
            }

            DRWin32ConfigureRemovableStorage(startupType);
        }
        __finally {}
    }
    else
    {
        DbgPlain(DBG_DR_SRDQUERY, _T("ASR is not running. No NTMS sevice configuration"));
    }

    DbgFcnOut();
    return (iRet);
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     avCmdOpt  -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  Exit Code of finished program.
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Runs a process.
*
*//*=========================================================================*/

int DRWin32RunProcess(PDRContext pDRCtxt, argvRec *avCmdOpt)
{
    ERH_FUNCTION(_T("DRWin32RunRestoreProcess"));
    int iRet=PANDR_RET_ERROR;
    tchar *pszCmd;
    int i;

    DbgFcnIn();

    /*-------------------------------------------------------------------------
    |   Glue directory and executable name
     ------------------------------------------------------------------------*/
    pszCmd=StrNewCopy(_T("\""));
    StrAppend(pszCmd, avCmdOpt->argv[0]);
    StrAppend(pszCmd, avCmdOpt->argv[1]);
    StrAppend(pszCmd, _T("\" "));

    DbgPlain(DBG_DR_PROCSTART, _T("Preparing to run:"));
    DbgPlain(DBG_DR_PROCSTART, _T("%s"), pszCmd);    

    for (i=2; i<avCmdOpt->argc; i++)
    {
        StrAppend(pszCmd, avCmdOpt->argv[i]);
        StrAppend(pszCmd, _T(" "));
        DbgPlain(DBG_DR_PROCSTART, _T("\t%s"), avCmdOpt->argv[i]);
    }        

    if ((!pDRCtxt->bIsVisible) && (pDRCtxt->bIsOnlineRestore) && (!pDRCtxt->ptrOpt->monitor))
    {
        DbgPlain(DBG_DR_PROCSTART, _T("Adding -silent to omnir"));
        StrAppend(pszCmd, _T("-silent "));
    }

    iRet = DRWin32Run(pszCmd);

	if(iRet != PANDR_RET_SUCCESS)
		DbgPlain(DBG_DR_PROCSTART, _T("DRWin32Run() resulted in error: %d"), iRet);

	FREE(pszCmd);
    
    DbgFcnOut();
    return (iRet);
}



int DRWin32Run(tchar *pszCmd)
{
    ERH_FUNCTION(_T("DRWin32Run"));

    int iRet = PANDR_RET_ERROR;

    DbgFcnIn();

    __try
    {
        DWORD dwError = ERROR_SUCCESS;
        STARTUPINFO si = {0};
        PROCESS_INFORMATION pi = {0};

        // Enable STD_XXX_HANDLE inheritance (QCCR2A40426).
        SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE),
            HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE),
            HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE),
            HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

        si.cb = sizeof(STARTUPINFO);
        if (CreateProcess(
                NULL, pszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi
                )==FALSE)
        {
            dwError=GetLastError();
            DbgPlain(
                DBG_DR_PROCSTART,
                _T("CreateProcess() resulted in error: %d"), dwError
                );
            __leave;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
    
        if (GetExitCodeProcess(pi.hProcess, &iRet)==FALSE)
        {
            dwError=GetLastError();
            DbgPlain(
                DBG_DR_PROCSTART,
                _T("GetExitCodeProcess() resulted in error: %d"), dwError
                );
            __leave;
        }
        else
        {
            DbgPlain(DBG_DR_PROCSTART, _T("Process exit code was: %d"), iRet);
        }
    }
    __finally {}

    DbgFcnOut();

    return(iRet);
}

#if defined(TARGET_WIN64) && !defined(_AMD64_)

int CleanNvram(void)
{
   ERH_FUNCTION(_T("CleanNvram"));

   int iRet=PANDR_RET_ERROR;

   DbgFcnIn();

   __try
    {
        tchar command[STRLEN_STD];
        strcpy(command, _T("nvram --clean"));

		iRet = DRWin32Run(command);

		if(iRet != PANDR_RET_SUCCESS)
		{
			DbgPlain(DBG_DR_PROCSTART, _T("DRWin32Run() resulted in error: %d"), iRet);
			__leave;
		}
        else
            DbgPlain(DBG_DR_PROCSTART, _T("DRWin32Run() succeeded."));

		iRet = PANDR_RET_SUCCESS;
    }
    __finally {}

   DbgFcnOut();

   return (iRet);
}
#endif /* defined(TARGET_WIN64) && !defined(_AMD64_) */

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    No EUP   -  PANDR_NO_EXCLUDE
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for EISA utility Partition. If there was none, return
*            PANDR_NO_EXCLUDE, otherwise create EXCLUDE options.
*
*//*=========================================================================*/
int DRWin32CreateEUPOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateEUPOptions"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulBuffer=0;

    USES_SRDQUERY;

    DbgFcnIn();

    __try
    {
        /*---------------------------------------------------------------------
        |   Query SRD for existence of EISA Utility partition.
         --------------------------------------------------------------------*/
        PREPARE_SRDQUERY(&ulBuffer, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_QUERY_EUP_EXISTS, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_SRDQUERY, _T("Error in SRDACT_QUERY_EUP_EXISTS."));
            __leave;
        }

        if (ulBuffer==0)
        {
            /*-----------------------------------------------------------------
            |   There was no EISA Utility partition, so, don't exclude it.
             ----------------------------------------------------------------*/
            iRet=PANDR_NO_EXCLUDE;
        }
        else
        {
            /*-----------------------------------------------------------------
            |   We do not restore EUP now, but just mark it for exclude.
             ----------------------------------------------------------------*/
            iRet=PANDR_RET_SUCCESS;
        }    
    }
    __finally
    {
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt         -   pointer to DR Context
* @param     pAvOpt          -   pointer to argvRec for options
* @param     pszItem         -   JetDBase item
* @param     iPurpose        -   volume purpose
* @param     pszVarOptName   -   user option name
*
* @retval    Success  -  PANDR_NO_EXCLUDE
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Adds user option pszVarOptName for CONFIGURATION item pszItem.
*            Returns PANDR_NO_EXCLUDE, because VRDA automaticaly treats
*            affected JetDBase objects in different way as -tree /.
*
*            Example of ActiveDirectoryService: -var adsmap "<Orig><Target>[{;<Orig><Target>}]"
*
*//*=========================================================================*/
static int 
DRWin32CreateJetDBOptions(
    PDRContext pDRCtxt,
    argvRec *pAvOpt,
    tchar *pszItem,
    int iPurpose,
    tchar *pszVarOptName
    )
{
    ERH_FUNCTION(_T("DRWin32CreateJetDBOptions"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx;
    
    tchar *tcTmp=NULL;
    PVolRec pJDBVol=NULL;

    tchar pszJDBMnt[STRLEN_PATH+1], pszJDBMapMnt[STRLEN_PATH+1], pszJDBMap[STRLEN_PATH+1];

    DbgFcnIn();
    
    ArgvAdd(pAvOpt, TREE_OPT);
    ArgvAdd(pAvOpt, pszItem);

    DbgPlain(DBG_DR_OPTIONS, VAR_OPT);
    DbgPlain(DBG_DR_OPTIONS, _T("%s"), pszVarOptName);

    ArgvAdd(pAvOpt, VAR_OPT);
    ArgvAdd(pAvOpt, pszVarOptName);

    *pszJDBMap=NULL_TCHAR;
    _Q(pszJDBMap);
    
    for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
    {
        /*---------------------------------------------------------------------
        |   Look for iPurpose Volumes
         --------------------------------------------------------------------*/
        pJDBVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
        if (!(pJDBVol->iPurpose & iPurpose))
        {
            continue;
        }
        
        DbgPlain(
            DBG_DR_OPTIONS,
            _T("iPurpose: %d, Volume: %d, VolumePurpose: %d"),
            iPurpose, ulVolIdx, pJDBVol->iPurpose
            );

        /*---------------------------------------------------------------------
        |   Append original mount and replace ':' with '$'
         --------------------------------------------------------------------*/
        *pszJDBMnt=NULL_TCHAR;
        strcat(pszJDBMnt, pJDBVol->pszVolMnt+1);
        COLON_CHECK(pszJDBMnt);
        tcTmp=NULL;
        tcTmp=strchr(pszJDBMnt, COLON_TCHAR);
        if (tcTmp!=NULL)
        {
            *tcTmp=_T('$');
        }
        DbgPlain(DBG_DR_OPTIONS, _T("pszJDBMnt=%s"), pszJDBMnt);
        
        /*---------------------------------------------------------------------
        |   If mapped, create map string. Replace ':' with '$'
         --------------------------------------------------------------------*/
        if (pJDBVol->bIsMapped)
        {
            *pszJDBMapMnt=NULL_TCHAR;
            strcat(pszJDBMapMnt, pJDBVol->pszMapVolMnt+1);
            COLON_CHECK(pszJDBMapMnt);
            tcTmp=NULL;
            tcTmp=strchr(pszJDBMapMnt, COLON_TCHAR);
            if (tcTmp!=NULL)
            {
                *tcTmp=_T('$');
            }
            DbgPlain(DBG_DR_OPTIONS, _T("pszJDBMapMnt=%s"), pszJDBMapMnt);
        }

        /*---------------------------------------------------------------------
        |   Create <Original>,<Target>;
         --------------------------------------------------------------------*/
        strcat(pszJDBMap, pszJDBMnt);
        strcat(pszJDBMap, _T(","));
        strcat(
            pszJDBMap,
            pJDBVol->bIsMapped ? pszJDBMapMnt : pszJDBMnt
            );
        strcat(pszJDBMap, _T(";"));
        DbgPlain(DBG_DR_OPTIONS, _T("pszJDBMap=%s"), pszJDBMap);
    }                
    
    /*-------------------------------------------------------------------------
    |   Remove trailing ';'
     ------------------------------------------------------------------------*/
    tcTmp=strrchr(pszJDBMap, _T(';'));
    if (tcTmp!=NULL)
    {
        *tcTmp=NULL_TCHAR;
    }
    _Q(pszJDBMap);
    DbgPlain(DBG_DR_OPTIONS, _T("pszJDBMap=%s"), pszJDBMap);
    PathToSlashes(pszJDBMap);

    DbgPlain(DBG_DR_OPTIONS, _T("%s %s"), pszVarOptName, pszJDBMap);
    ArgvAdd(pAvOpt, pszJDBMap);

    iRet=PANDR_RET_SUCCESS;    

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_NO_EXCLUDE
* @retval    No ADS   -  PANDR_NO_EXCLUDE
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for ADS Volume. If there was none, return PANDR_NO_EXCLUDE.
*            Checks if ADS is online. If yes, allows restore only with TempOs.
*            Calls JetDBase options generator.
*
*//*=========================================================================*/
int DRWin32CreateADSOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateADSOptions"));
    int iRet=PANDR_RET_ERROR;  
    R_ULONG ulVolIdx;    
    PVolRec pADSVol=NULL;

    tchar *tcTmp=NULL;
    tchar pszSrv[STRLEN_HOSTNAME+3]; /* Additional space for \\ */
    BOOL bIsNTDSOnL=FALSE;
    DsIsNTDSOnlineProc fnOnline=NULL;
    HMODULE hNtds=NULL;
    HRESULT hRes;

    DbgFcnIn();
    __try
    {
        /*---------------------------------------------------------------------
        |   Search for ActiveDirectoryService volumes.
         --------------------------------------------------------------------*/
        for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            pADSVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
            if (pADSVol->iPurpose & DR_ADS_VOLUME)
            {
                break;
            }        
        }  
        if (ulVolIdx==pDRCtxt->pVolRecTab->nEl)
        {
            /*-----------------------------------------------------------------
            |   No ADS volumes found. Leaving.
            |   This happens only, when they are not specified in SRD file.
             ----------------------------------------------------------------*/
            iRet=PANDR_NO_EXCLUDE;
            __leave;
        }

        /*---------------------------------------------------------------------
        |   Check if ActiveDirectoryService is ONLINE.
         --------------------------------------------------------------------*/
        __try
        {
            hNtds = LoadLibrary(NTDS_DLLNAME);
            if(hNtds == NULL)
            {
                DbgPlain(DBG_DR_CREATEOPTIONS, _T("Cannot load ntdsbcli library."));
                __leave;
            }

            fnOnline = 
                (DsIsNTDSOnlineProc)GetProcAddress(hNtds, DsIsNTDSOnlineName);
            if(fnOnline == NULL)
            {
                DbgPlain(
                    DBG_DR_CREATEOPTIONS,
                    _T("Cannot get DsIsNTDSOnline() function pointer.")
                    );
                __leave;
            }
                
            sprintf(pszSrv, _T("\\\\%s"), cmnNodename);
            tcTmp=strchr(pszSrv, _T('.'));
            if (tcTmp!=NULL)
            {
                *tcTmp=NULL_TCHAR;
            }
   
            hRes=fnOnline(pszSrv, &bIsNTDSOnL);

            if (hRes!=ERROR_SUCCESS)
            {
                DbgPlain(DBG_DR_CREATEOPTIONS, _T("DsIsNTDSOnline() failed."));
            }
        }

        __finally
        {
            if ((bIsNTDSOnL) && (!pDRCtxt->tempOS))
            {
                /*-------------------------------------------------------------
                |   Do not perform the Disaster Restore of ADS, when it is 
                |   online and restoring active OS.
                 ------------------------------------------------------------*/
                DbgPlain(
                    DBG_DR_CREATEOPTIONS, 
                    _T("It is not possible to perform Disaster Recovery of online NTDS.")
                    );
                iRet=PANDR_JETDB_ONLINE;
            }
            else
            {
                strcpy(pszSrv, pDRCtxt->origTargetName);
                tcTmp=NULL;
                tcTmp=strchr(pszSrv, _T('.'));
                if (tcTmp!=NULL)
                {
                    *tcTmp=NULL_TCHAR;
                }

                ArgvAdd(pAvOpt, VAR_OPT);
                ArgvAdd(pAvOpt, SERVER_OPT);
                ArgvAdd(pAvOpt, pszSrv);
                iRet=DRWin32CreateJetDBOptions(pDRCtxt, pAvOpt, ADS_ITEM, DR_ADS_VOLUME, ADSMAP_OPT);
            }
        }
    }
    __finally
    {
        if (hNtds!=NULL)
        {
            FreeLibrary(hNtds);
        }
    }
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_NO_EXCLUDE
* @retval    No  CS   -  PANDR_NO_EXCLUDE
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for CS Volume. If there was none, return PANDR_NO_EXCLUDE.
*            Calls JetDBase options generator.
*
*//*=========================================================================*/
int DRWin32CreateCSOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateCSOptions"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx;
    PVolRec pCSVol=NULL;

    /*tchar pszHost[STRLEN_HOSTNAME+3]; *//* Additional space for \\ */
    HMODULE hCs=NULL;

    DbgFcnIn();
    __try
    {
        /*---------------------------------------------------------------------
        |   Search for Certificate Server volumes.
         --------------------------------------------------------------------*/
        for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            pCSVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
            if (pCSVol->iPurpose & DR_CS_VOLUME)
            {
                break;
            }        
        }  
        if (ulVolIdx==pDRCtxt->pVolRecTab->nEl)
        {
            /*-----------------------------------------------------------------
            |   No CS volumes found. There is nothing to exclude.
            |   This happens only, when they are not listed in the SRD file.
             ----------------------------------------------------------------*/
            iRet=PANDR_NO_EXCLUDE;
            __leave;
        }
        
        if (pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS, 
                _T("Certificate Server Database is not recovered during TempOS based DR.")
                );
            iRet=PANDR_RET_SUCCESS;
            __leave;
        }


        /*--- FIXME ------------------------------------------------------------
        |   At the moment we ALWAYS exclude Certificate Server DB, because   
        |   scenario 1 results in VRDA crash, while scenario 2 produces the same
        |   erorr as unconditional exclusion.
         ---------------------------------------------------------------------*/
        iRet=PANDR_RET_SUCCESS;
        __leave;        
        
        /*---------------------------------------------------------------------
        |   Scenario 1
        |   In the case of active OS, we leave it to the VRDA
         --------------------------------------------------------------------*/
        iRet=PANDR_NO_EXCLUDE;
    }
    __finally
    {
        /*NOP*/
    }

#if 0
        /*This did not result in satisfactory restore, hence #if 0*/
        BOOL bIsCSOnL=FALSE;
        CsIsCSOnlineProc fnOnline=NULL;
        HRESULT hRes=0;
        tchar *tcTmp=NULL;

        /*---------------------------------------------------------------------
        |   Scenario 2
        |   We try to be smart.
         --------------------------------------------------------------------*/        
        
        /*---------------------------------------------------------------------
        | From here on, we exclude the Certificate server, unless online and 
        | running.
         --------------------------------------------------------------------*/
        iRet=PANDR_RET_SUCCESS;

        /*---------------------------------------------------------------------
        |   Check if Certificate Server is ONLINE.
         --------------------------------------------------------------------*/
        hCs = LoadLibrary(CS_DLLNAME);
        if(hCs == NULL)
        {
            DbgPlain(DBG_DR_CREATEOPTIONS, _T("Cannot load Certadm.dll."));
            __leave;
        }

        fnOnline = (CsIsCSOnlineProc)GetProcAddress(hCs, CsIsCSOnlineName);
        if(fnOnline == NULL)
        {
            DbgPlain(DBG_DR_CREATEOPTIONS, _T("Cannot get CertSrvIsServerOnline() function pointer."));
            __leave;
        }
            
        sprintf(pszHost, _T("\\\\%s"), cmnNodename);
        tcTmp=strchr(pszHost, _T('.'));
        if (tcTmp!=NULL)
        {
            *tcTmp=NULL_TCHAR;
        }

        hRes=fnOnline(pszHost, &bIsCSOnL);  
        if (hRes!=ERROR_SUCCESS)
        {
            DbgPlain(DBG_DR_CREATEOPTIONS, _T("CertSrvIsServerOnline() failed."));
        }

        if (!bIsCSOnL)
        {
            DbgPlain(DBG_DR_CREATEOPTIONS, _T("CS is not online..."));
            __leave;
        }

        DbgPlain(DBG_DR_CREATEOPTIONS, _T("CS is online..."));

        if ((strcmp(pDRCtxt->origTargetName, pDRCtxt->hostName)==0))
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Original system [%s] equals active host [%s]\n"),
                _T("Proceeding with hot restore..."),
                pDRCtxt->origTargetName, pDRCtxt->hostName
                );
            iRet=PANDR_NO_EXCLUDE;
        }                

    }    
    __finally
    {
        if (hCs!=NULL)
        {
            FreeLibrary(hCs);
        }
    }
#endif
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     The CONFIGURATION volume is always restored after filesystem restore.
*            Therefore the procedure searches the target system directory for COM+
*            files and replaces the highest number COM+ file with the restored
*            file.
*            If there are no such files, the COM+ file is restored with name
*            ComPlusDatabase.
*
*//*=========================================================================*/
int DRWin32CreateCOMOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateCOMOptions"));
    int iRet=PANDR_RET_ERROR;
    tchar pszTargetFile[STRLEN_PATH+STRLEN_FILE+1];
    tchar pszFindPattern[STRLEN_PATH+STRLEN_FILE+1];
    FILESEARCHHANDLE fhHandle={0};
    FILESEARCHDATA  fdFileData;
    ULONG ulHighCom=0, ulThisCom=0;

    DbgFcnIn();
    *pszTargetFile=NULL_TCHAR;
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS, 
                _T("Trying to run Cold Restore of COM+ Database on the active system.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS, 
                _T("Should never come here. A major error! Exiting...")
                );
            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }

        /*---------------------------------------------------------------------
        |   Search pszTargetSysDir for \\Registration\\*.clb
         --------------------------------------------------------------------*/
        *pszFindPattern=NULL_TCHAR;
        strcat(pszFindPattern, pDRCtxt->pszTargetSysDir);
        strcat(pszFindPattern, COMPLUS_PATTERN);        
        PathToBackslashes(pszFindPattern);        
        
        if (OB2_FindFirstFile(&fhHandle, pszFindPattern, &fdFileData)!=0)
        {
            _Q(pszTargetFile);
            strcat(pszTargetFile, pDRCtxt->pszTargetSysDir);
            strcat(pszTargetFile, COMPLUS_DIR);
            strcat(pszTargetFile, COMPLUS_DEF_FILE);
            _Q(pszTargetFile);
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Search for COM+ objects - %s - failed. Restoring as: %s"),
                pszFindPattern, pszTargetFile
                );
        }
        else
        {            
            tchar pszHighCom[STRLEN_PATH+STRLEN_FILE+1];
            strcpy(pszHighCom, fdFileData.szFileName);

            do
            {
                if (strcmp(fdFileData.szFileName, pszHighCom)>0)
                {
                    strcpy(pszHighCom, fdFileData.szFileName);
                }

            } while (OB2_FindNextFile(&fhHandle, &fdFileData)==0);
        
            _Q(pszTargetFile);
            strcat(pszTargetFile, pDRCtxt->pszTargetSysDir);
            strcat(pszTargetFile, COMPLUS_DIR);
            strcat(pszTargetFile, _T("\\"));
            strcat(pszTargetFile, pszHighCom);
            _Q(pszTargetFile);
            
            DbgPlain(DBG_DR_CREATEOPTIONS, _T("Restoring COM+ DB as: %s"), pszTargetFile);
        }

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, COMPLUS_ITEM);
        ArgvAdd(pAvOpt, AS_OPT);
        PathToBackslashes(pszTargetFile);
        ArgvAdd(pAvOpt, pszTargetFile);

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        OB2_FindClose(&fhHandle);
    }
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containing cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCES
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for Profiles Volume.
*            Composes new Profiles Directory.
*
*//*=========================================================================*/
int DRWin32CreateProfOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx;
    tchar pszBuffer[STRLEN_PATH+1];
    tchar pszProfilePath[STRLEN_PATH+1];
    tchar pszTargetProfilePath[STRLEN_PATH+1];
    tchar *tcTmp=NULL;

    PVolRec pProfVol;

    USES_SRDQUERY;

    ERH_FUNCTION(_T("DRWin32CreateProfOptions"));

    DbgFcnIn();
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Trying to run Cold restore of Profiles on Active OS.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Should never come here. A major error! Exiting...")
                );
            
            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }

        /*---------------------------------------------------------------------
        |   Query SRD for original Profiles Directory
         --------------------------------------------------------------------*/
        PREPARE_SRDQUERY(pszBuffer, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_PROFILES_PATH, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(
                DBG_DR_SRDQUERY,
                _T("Error in SRDACT_GET_PROFILES_PATH failed.")
                );
            __leave;
        }
        
        DRConvertPathToDBFormat(pszBuffer, pszProfilePath); 

        /*---------------------------------------------------------------------
        |   Find Profiles Volume
         --------------------------------------------------------------------*/
        for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            if (
                ((PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx])->iPurpose
                                                     & DR_PROFILES_VOLUME
               )
            {
                break;
            }
        }
        if (ulVolIdx==pDRCtxt->pVolRecTab->nEl)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("No Profiles volume found. Exiting...")
                );
            __leave;
        }
        pProfVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];

        /*---------------------------------------------------------------------
        |   Compose new Profiles directory
         --------------------------------------------------------------------*/        
        if (pProfVol->bIsMapped)            
        {
            DRComposePath(
                pProfVol->pszMapVolMnt,
                DRStripMount(pszProfilePath, pProfVol->pszVolMnt),
                pszTargetProfilePath
                );
        }         
        
        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(
            pszBuffer,
            pProfVol->bIsMapped ? pszTargetProfilePath+1 : pszProfilePath+1
            );
        _Q(pszBuffer);        

        tcTmp=strchr(pszBuffer, _T('$'));
        if (tcTmp!=NULL)
        {
            *tcTmp=_T(':');
        }

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, PROFILES_ITEM);
        ArgvAdd(pAvOpt, AS_OPT);
        ArgvAdd(pAvOpt, pszBuffer);

        DbgPlain(
            DBG_DR_CREATEOPTIONS,
            _T("New Profiles Directory is:"),
            pszBuffer
            );
        iRet=PANDR_RET_SUCCESS;
    }

    __finally
    {
        /* Empty */
    }
    
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containing cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCES
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for Profiles Volume.
*            Composes new Profiles Directory.
*
*//*=========================================================================*/
int DRWin32CreateProfOptionsEx (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    int iRet=PANDR_RET_ERROR;

    ERH_FUNCTION(_T("DRWin32CreateProfOptionsEx"));

    DbgFcnIn();
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Trying to run Cold restore of Profiles on Active OS.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Should never come here. A major error! Exiting...")
                );

            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }
        iRet = PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containing cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCES
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks SRD for Event Logs.
*            Composes new Event Logs options.
*
*//*=========================================================================*/
int DRWin32CreateEventOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    int iRet=PANDR_RET_ERROR;
    tchar pszBuffer[STRLEN_PATH+1];

    ERH_FUNCTION(_T("DRWin32CreateEventOptions"));

    DbgFcnIn();
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Trying to run Cold restore of Profiles on Active OS.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Should never come here. A major error! Exiting...")
                );

            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, EVENT_ITEM);

        ArgvAdd(pAvOpt, AS_OPT);

        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(pszBuffer, pDRCtxt->pszTargetSysDir);
        strcat(pszBuffer, SYSTEM32_DIR);
        strcat(pszBuffer, EVTLOG_DIR);
        _Q(pszBuffer);
        PathToBackslashes(pszBuffer);
        ArgvAdd(pAvOpt, pszBuffer);

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Restore NTMS into System Directory.
*
*//*=========================================================================*/
int DRWin32CreateNTMSOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateNTMSOptions"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx;
    tchar pszBuffer[STRLEN_PATH+1];
    PVolRec pSysVol=NULL;
    tchar *tcTmp=NULL;

    DbgFcnIn();
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Trying to run Cold restore of NTMS on Active OS.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Should never come here. A major error! Exiting...")
                );            
            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }

        /*---------------------------------------------------------------------
        |   Find System Volume
         --------------------------------------------------------------------*/
        for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            if (
                ((PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx])->iPurpose
                                                       & DR_SYSTEM_VOLUME
               )
            {
                pSysVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
                break;
            }
        }
        if (pSysVol==NULL)
        {
           DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("No System volume found. Exiting...")
                );
            __leave;
        }

        ArgvAdd(pAvOpt, TREE_OPT);
        
        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(pszBuffer, NTMS_ITEM);
        strcat(pszBuffer, pSysVol->pszVolMnt);
        COLON_CHECK(pszBuffer);
        _Q(pszBuffer);
        tcTmp=strchr(pszBuffer, _T(':'));
        if (tcTmp!=NULL)
        {
            *tcTmp=_T('$');
        }        
        ArgvAdd(pAvOpt, pszBuffer);
        
        DbgPlain(DBG_DR_CREATEOPTIONS, _T("-tree %s"), pszBuffer);

        ArgvAdd(pAvOpt, AS_OPT);
        
        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(
            pszBuffer, 
            pSysVol->bIsMapped?pSysVol->pszMapVolMnt+1:pSysVol->pszVolMnt+1
            );
        COLON_CHECK(pszBuffer);
        _Q(pszBuffer);
        ArgvAdd(pAvOpt, pszBuffer);

        DbgPlain(DBG_DR_CREATEOPTIONS, _T("-as %s"), pszBuffer);
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }           
    
    DbgFcnOut();
    return iRet;
}


/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Restore DNS into System Directory.
*
*//*=========================================================================*/
int DRWin32CreateDNSOptions(PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateDNSOptions"));
    int iRet=PANDR_RET_ERROR;
    tchar pszBuffer[STRLEN_PATH+1];

    DbgFcnIn();
    __try
    {
        if (!pDRCtxt->tempOS)
        {
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Trying to run Cold restore of DNS on Active OS.")
                );
            DbgPlain(
                DBG_DR_CREATEOPTIONS,
                _T("Should never come here. A major error! Exiting...")
                );            
            iRet=PANDR_ILLEGAL_ACTOSACT;
            __leave;
        }

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, DNS_ITEM);

        ArgvAdd(pAvOpt, AS_OPT);

        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(pszBuffer, pDRCtxt->pszTargetSysDir);
        strcat(pszBuffer, SYSTEM32_DIR);
        strcat(pszBuffer, _T("/DNS"));
        _Q(pszBuffer);
        PathToBackslashes(pszBuffer);
        ArgvAdd(pAvOpt, pszBuffer);

        DbgPlain(DBG_DR_CREATEOPTIONS, _T("-tree %s -as %s"), DNS_ITEM, pszBuffer);
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }           

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     IIS is not restored during Phase 2 of DR.
*
*//*=========================================================================*/
int DRWin32CreateIISOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateIISOptions"));
    int iRet=PANDR_RET_ERROR;
    tchar pszIISDBPath[STRLEN_PATH+1];

    DbgFcnIn();         
            
    __try
    {

#if 0
        if (IS_ACT_OS(pDRCtxt))
        {
            if (DRWin32GetServiceState(IISADMIN_SRV_NAME)==SERVICE_RUNNING)
            {
                iRet=PANDR_NO_EXCLUDE;
                __leave;
            }
        }
#endif

        sprintf(pszIISDBPath, _T("%s/system32/inetsrv/MetaBack/DisasterRecovery.MD0"), pDRCtxt->pszTargetSysDir);

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, IIS_ITEM);
        ArgvAdd(pAvOpt, AS_OPT);
        ArgvAdd(pAvOpt, pszIISDBPath);
        
        DbgPlain(DBG_DR_CREATEOPTIONS, _T("-tree %s -as %s"), IIS_ITEM, pszIISDBPath);

        iRet=PANDR_RET_SUCCESS;        
    }    
    __finally
    {
        /* Empty */
    }
    
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Restore SYSVOL into "SYSVOL/domain" Directory.
*
*//*=========================================================================*/
int DRWin32CreateSYSVOLOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateSYSVOLOptions"));
    int iRet=PANDR_RET_ERROR;
    R_ULONG ulVolIdx;    
    PVolRec pVolSelect=NULL;
    tchar pszSysVol[STRLEN_PATH+1], pszSysVolAS[STRLEN_PATH+1];    
    USES_SRDQUERY;

    DbgFcnIn();

    __try
    {
        /* Get SYSVOL location */

        PREPARE_SRDQUERY(pszSysVol, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_SYSVOL_PATH, SRDHT_SYSINFO
                )!=SRDERR_SUCCESS)
        {
            DbgPlain(DBG_DR_INIT, _T("Error in SRDACT_GET_SYSVOL_PATH."));
            __leave;
        }

        if (pszSysVol[0]==NULL_TCHAR)
        {
            DbgPlain(DBG_DR_INIT, _T("There was no SYSVOL in SRD."));
            iRet=PANDR_NO_EXCLUDE;
            __leave;
        }                        
        PathToSlashes(pszSysVol);
        _tcslwr(pszSysVol);
        
        /* Find SYSVOL object */
        for (ulVolIdx=0; ulVolIdx<pDRCtxt->pVolRecTab->nEl; ulVolIdx++)
        {
            PVolRec pVol=(PVolRec)pDRCtxt->pVolRecTab->ptrEl[ulVolIdx];
            tchar pszVolMnt[STRLEN_PATH+1];

            if (!(pVol->iPurpose & DR_ADS_VOLUME))
            {
                /* Wrong purpose */
                continue;
            }

            strcpy(pszVolMnt, pVol->pszVolMnt);
            _tcslwr(pszVolMnt);
            if (strstr(pszSysVol, pszVolMnt+1)==NULL)
            {
                /* Wrong path */
                continue;
            }

            if (pVolSelect==NULL)
            {
                /* This is the first possible match */
                pVolSelect=pVol;
                continue;
            }

            if (strlen(pVol->pszVolMnt)>strlen(pVolSelect->pszVolMnt))
            {
                /* If by any miracle the SYSVOL is on a Volume Mount Point, we catch it here */
                pVolSelect=pVol;
            }
        }
                
        if (pVolSelect==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("SRD info: SYSVOL = %s"), pszSysVol);
            DbgPlain(DBG_DR_INIT, _T("However, no matching volumes were found. Skipping SYSVOL options..."));
            iRet=PANDR_NO_EXCLUDE;
            __leave;
        }

        /* Now pVolSelect points to the volume, where the SYSVOL originally resided */
        pszSysVolAS[0]=NULL_TCHAR;
        if (pVolSelect->bIsMapped)
        {
            tchar *ptcMatch=NULL;

            DbgPlain(DBG_DR_INIT, _T("The volume %s where SYSVOL resided IS remapped to %s. Constructing new location..."), pVolSelect->pszVolMnt, pVolSelect->pszMapVolMnt);
            _Q(pszSysVolAS);
            strcat(pszSysVolAS, pVolSelect->pszMapVolMnt+1);
            COLON_CHECK(pszSysVolAS);
            
            ptcMatch=pszSysVol+strlen(pVolSelect->pszVolMnt+1);
            if (*ptcMatch==COLON_TCHAR)
            {
                *ptcMatch++;
            }
            strcat(pszSysVolAS, ptcMatch);
            _Q(pszSysVolAS);

            DbgPlain(DBG_DR_INIT, _T("New location for SYSVOL is: %s"), pszSysVolAS);
        }
        else
        {
            DbgPlain(DBG_DR_INIT, _T("The volume %s where SYSVOL resided IS NOT remapped. Restoring the SYSVOL to original location..."), pVolSelect->pszVolMnt);
            
            STRCATQ(pszSysVolAS, pszSysVol);                        
        }

        PathToBackslashes(pszSysVolAS);

        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, SYSVOL_ITEM);
        ArgvAdd(pAvOpt, AS_OPT);
        ArgvAdd(pAvOpt, pszSysVolAS);

        DbgPlain(DBG_DR_CREATEOPTIONS, _T("-tree %s -as %s"), SYSVOL_ITEM, pszSysVolAS);
        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /* Empty */
    }           

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. Prepare the following:
*            -tree /Registry -exclude /Registry/Cluster
*            2. If TempOs
*            -into <TargetWinNT>
*            3. If MSCluster
*            -tree /Registry/Cluster -into <TargetWinNT>\cluster\clusdb
*
*//*=========================================================================*/
int DRWin32CreateREGOptions (PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateREGOptions"));
    int iRet=PANDR_RET_ERROR;

    tchar pszBuffer[STRLEN_PATH+1];

    DbgFcnIn();

    ArgvAdd(pAvOpt, TREE_OPT);
    ArgvAdd(pAvOpt, REGISTRY_ITEM);

    ArgvAdd(pAvOpt, EXCLUDE_OPT);
    ArgvAdd(pAvOpt, REGISTRY_CLUS);
    
    ArgvAdd(pAvOpt, EXCLUDE_OPT);
    ArgvAdd(pAvOpt, REGISTRY_BCD);
    
    if (pDRCtxt->tempOS)
    {
        ArgvAdd(pAvOpt, INTO_OPT);

        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(pszBuffer, pDRCtxt->pszTargetSysDir);
        strcat(pszBuffer, SYSTEM32_DIR);
        _Q(pszBuffer);
        PathToBackslashes(pszBuffer);
        ArgvAdd(pAvOpt, pszBuffer);
    }    

    if (pDRCtxt->bMSCluster)
    {
        ArgvAdd(pAvOpt, TREE_OPT);
        ArgvAdd(pAvOpt, REGISTRY_CLUS);
        ArgvAdd(pAvOpt, AS_OPT);
    
        *pszBuffer=NULL_TCHAR;
        _Q(pszBuffer);
        strcat(pszBuffer, pDRCtxt->pszTargetSysDir);
        strcat(pszBuffer, CLUSTER_DB_FILE);
        _Q(pszBuffer);
        PathToBackslashes(pszBuffer);
        ArgvAdd(pAvOpt, pszBuffer);
    }        

    iRet=PANDR_RET_SUCCESS;
    DbgFcnOut();
    return iRet;    

}

/*========================================================================*//**
*
* @retval    ASR in progress         -  TRUE
* @retval    ASR is not in progress  -  FALSE
*
* @brief     Checks if ASR is in progress
*
*//*=========================================================================*/

int DRWin32IsASR(void)
{
    int iRet=FALSE;
    tchar *pszAsrInProgress=NULL;

    pszAsrInProgress=RegGetString(HKEY_LOCAL_MACHINE, REG_KEY_ASR_IN_PROGRESS, REG_VAL_ASR_IN_PROGRESS);

    if (pszAsrInProgress!=NULL)
    {
        iRet=TRUE;
    }
    
    FREE(pszAsrInProgress);
    return iRet;
}

/*========================================================================*//**
*
* @retval    EADR in progress         -  TRUE
* @retval    EADR is not in progress  -  FALSE
*
* @brief     Checks if EADR is in progress
*
*//*=========================================================================*/

int DRWin32IsEADR(PDRContext pDRCtxt)
{
    int iRet=FALSE;

    if (pDRCtxt->ptrOpt->drimINI)
        iRet = TRUE;

    return iRet;
}

/*========================================================================*//**
*
* @param     message - Message to be displayed to the user
* @param     time    - Timeout value (in milliseconds)
* @param     key     - Key to wait for
*
* @retval    -2 - An error occured
* @retval    -1 - Wait aborted (ESC pressed)
* @retval    0  - Wait timed out
* @retval    1  - Input key pressed
*
* @brief     Waits for a certain key to be pressed or a timeout to occur.
*
*//*=========================================================================*/
static int 
DRWin32InteractiveWait(const tchar *message,
                       int time,
                       int key)
{
    ERH_FUNCTION (_T("DRWin32InteractiveWait"));

    int          retVal = -2; /* error occured */
    INPUT_RECORD irInBuf[1];
    HANDLE       hStdin = INVALID_HANDLE_VALUE; 
    DWORD        oldMode = 0; 

    DbgFcnIn();

    _TRY_
        DWORD millis = GetTickCount();

        printf(message);
        fflush(stdout);

        hStdin = GetStdHandle(STD_INPUT_HANDLE);

        if(!GetConsoleMode(hStdin, &oldMode))
            _LEAVE_;

        if(!SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT))
            _LEAVE_;

        FlushConsoleInputBuffer(hStdin);

        while(TRUE)
        {
            DWORD numRead = 0;

            if(GetNumberOfConsoleInputEvents(hStdin, &numRead))
            {
                if(numRead > 0)
                {
                    numRead = 0;
                    memset(irInBuf, 0, sizeof(INPUT_RECORD));

                    if(ReadConsoleInput(hStdin, irInBuf, 1, &numRead))
                    {
                        DWORD ii = 0;

                        for (ii = 0; ii < numRead; ii++)
                        {
                            switch(irInBuf[ii].EventType)
                            {
                                case KEY_EVENT:
                                    if(irInBuf[ii].Event.KeyEvent.
                                        wVirtualKeyCode == key) 
                                    {
                                        retVal = 1; /* Input key pressed */
                                        _LEAVE_;
                                    }
                                    else if(irInBuf[ii].Event.KeyEvent.
                                        wVirtualKeyCode == VK_ESCAPE)
                                    {
                                        retVal = -1; /* aborted */
                                        _LEAVE_;
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            Sleep(50);

            if (GetTickCount() - millis > (DWORD)time)
            {
                retVal = 0; /* timed out */
                _LEAVE_;
            }
        }
    _FINALLY_
        if(oldMode)
            SetConsoleMode(hStdin, oldMode);
    _ENDTRY_

    DbgFcnOut();

    return(retVal);
}

/*========================================================================*//**
*
* @retval    Network set successfully -  TRUE
* @retval    Failed to set network    -  FALSE
*
* @brief     Sets original network configuration for ASR (only).
*
*//*=========================================================================*/
void DebugCallback(int level, tchar *message, void *data)
{
    PDRContext pDRCtxt = (PDRContext)data;

    APPEND_TO_TRACE(pDRCtxt, message);
    APPEND_TO_TRACE(pDRCtxt, NEW_LINE_STRING)
}

enum NetCfg
{
    NWCFG_NONE        = -1,
    NWCFG_ASR         = 0,
    NWCFG_LEGACY_EADR = 1,
    NWCFG_WINPE       = 2
};

enum ElapsedTime
{
    TIME_TO_WAIT_ASR    = 5000,
    TIME_TO_WAIT_LEGACY = 10000,
    MINUTE_IN_MILLIS    = 60000
};

/* user's decision, return 0 to skip network configuration */
typedef int (*InputProc)(PDRContext, PNETWORKINFO, PNETWORKINFO);
/* preprocess data if needed, return > 0 if success, 0 to skip configuration, < 0 on error */
typedef int (*PreprocessProc)(PDRContext, PNETWORKINFO, PNETWORKINFO);
/* wait following the network configuration procedure */
typedef void (*WaitProc)(PDRContext);

/* The default data preprocessing function */
static int
PreprocessDefault(PDRContext pDRCtxt, /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named
                                         unless they are void or an ellipsis (...). Or C2055 is reported." */
                  PNETWORKINFO srd, /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named
                                       unless they are void or an ellipsis (...). Or C2055 is reported." */
                  PNETWORKINFO cur) /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named
                                       unless they are void or an ellipsis (...). Or C2055 is reported." */
{
    return(1);
}
/* The default wait-for-completion function */
static void
WaitDefault(PDRContext pDRCtxt) /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named unless
                                   they are void or an ellipsis (...). Or C2055 is reported." */
{}

/* Network configuration method structure */
typedef struct SNwcfgMethods
{
    InputProc input;            /* User input procedure */
    PreprocessProc preprocess;  /* Data preprocessing procedure */
    WaitProc wait;              /* Wait at the end of the task procedure */
} NWCFG_METHOD;

/* ASR DR method input handling function */
static int
InputAsr(PDRContext pDRCtxt,
         PNETWORKINFO srd, /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named unless
                              they are void or an ellipsis (...). Or C2055 is reported." */
         PNETWORKINFO cur) /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named unless
                              they are void or an ellipsis (...). Or C2055 is reported." */
{
    ERH_FUNCTION(_T("InputAsr"));

    if (DRWin32InteractiveWait(
        _T("Press F8 in the next 5 seconds to skip network configuration...\n"),
        TIME_TO_WAIT_ASR, VK_F8) > 0
    )
    {
        /* user pressed F8 */
        printf(_T("Network configuration skipped by user.\n"));
        fflush(stdout);
        PUT_TRACE(pDRCtxt, _T("ASR: network configuration skipped by user."));
        return(0);
    }
    else
    {
        return(1);
    }
}

/*
* Retrieve network question from a configuration file (valid only inside WinPE)
* The file in question should be named net.cfg and should be located in the
* same directory where omnidr resides.
*
* The configuration string consists of 2 parts:
* 1.) Message to be displayed followed by a comma
* 2.) Boolean value about what should be done if F8 is pressed (1 to switch to
*     original network setting, 0 to keep DHCP)
*
* Examples: Press F8 in the next 10 seconds to keep current (DHCP) network settings...,0
*           Press F8 in the next 10 seconds to switch to original network settings...,1
*/
static int
GetQuestionFromFile(PDRContext pDRCtxt,
                    tchar *output_message,
                    int *answer)
{
    ERH_FUNCTION(_T("GetQuestionFromFile"));

    tchar path[STRLEN_PATH + 1] = _T("");
    tchar *fileBuffer = NULL;

    tchar message[STRLEN_STD + 1] = _T("");
    int switch_to_original = -1;

    GetModuleFileName(NULL, path, STRLEN_PATH);
    PathCutLeaf(path);
    PathAppendComponent(path, _T("net.cfg"));

    fileBuffer = CmnGetAsciiFile(path);
    if (fileBuffer)
    {
        tchar *comma = strchr(fileBuffer, _T(','));
        if (comma)
        {
            tchar *trimmed = NULL;
            tchar number[STRLEN_STD + 1];

            *comma = _T('\0');

            strcpy(message, fileBuffer);
            trimmed = StrCutSpaces(message);
            if (trimmed)
            {
                strcpy(message, trimmed);
                FREE(trimmed);
            }
            strcat(message, _T("\n"));

            strcpy(number, comma + 1);
            trimmed = StrCutSpaces(number);
            if (trimmed)
            {
                strcpy(number, trimmed);
                FREE(trimmed);
            }

            if (!StrAtoi(number, &switch_to_original, NULL))
            {
                message[0] = _T('\0');
                switch_to_original = -1;
            }
            if (switch_to_original > 1)
            {
                switch_to_original = 1;
            }
        }
        FREE(fileBuffer);
    }
    if (switch_to_original < 0)
    {
        return(0);
    }
    else
    {
        strcpy(output_message, message);
        *answer = switch_to_original;
        return(1);
    }
}

/* The Windows PE based EADR input handling function */
static int
InputWinpe(PDRContext pDRCtxt,
           PNETWORKINFO srd,
           PNETWORKINFO cur)
{
    ERH_FUNCTION(_T("InputWinpe"));

    tchar message[STRLEN_STD + 1] = _T("");
    int file_answer = -1;
    int switch_to_original = -1;

    if (!GetQuestionFromFile(pDRCtxt, message, &file_answer))
    {
        strcpy(message, _T("Press F8 in the next 10 seconds to switch to ")
            _T("original network settings...\n"));
        file_answer = 1;
    }

    if (DRWin32InteractiveWait(message, TIME_TO_WAIT_LEGACY, VK_F8) > 0)
    {
        /* user pressed F8 */
        switch_to_original = file_answer;
    }
    else
    {
        switch_to_original = !file_answer;
    }

    if (!switch_to_original)
    {
        printf(_T("Skipped network reconfiguration.\n\n"));
        fflush(stdout);
        PUT_TRACE(pDRCtxt, _T("WINPE: network configuration skipped by default."));
    }
    return(switch_to_original);
}

/* The legacy EADR input handling function */
static int
InputLegacy(PDRContext pDRCtxt,
            PNETWORKINFO srd, /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named unless
                                 they are void or an ellipsis (...). Or C2055 is reported." */
            PNETWORKINFO cur)
{
    ERH_FUNCTION(_T("InputLegacy"));

    if (DRWin32InteractiveWait(
        _T("Press F8 in the next 10 seconds to switch network to DHCP...\n"),
        TIME_TO_WAIT_LEGACY, VK_F8) > 0
    )
    {
        /* user pressed F8 */
        return(1);
    }
    else
    {
        /*
            this one is a bit tricky. if user decides not to switch to DHCP and
            DHCP already exist on at least one of the adapters, than we should
            preferably force the use of IP addresses for -target option. if we
            switched to DHCP in one of previous passes, DNS may not have
            the restoring host connected to the proper IP address.
        */
        R_DWORD ii = 0;
        for (ii = 0; ii < cur->adapterCount; ++ii)
        {
            if (cur->adapters[ii]->ipData.dhcpEnabled)
            {
                pDRCtxt->bForceIp = 1;
            }
        }

        printf(_T("Skipping DHCP configuration.\n"));
        fflush(stdout);
        PUT_TRACE(pDRCtxt, _T("Legacy EADR: skipping DHCP configuration."));

        return(0);
    }
}

/* The legacy EADR input data preprocessing function */
static int
PreprocessLegacy(PDRContext pDRCtxt,
                 PNETWORKINFO srd,
                 PNETWORKINFO cur) /* parasoft-suppress  OPT-03 "ANSI C requires formal parameters to be named unless
                                      they are void or an ellipsis (...). Or C2055 is reported." */
{
    R_DWORD ii = 0;
    for (ii = 0; ii < srd->adapterCount; ++ii)
    {
        if (!srd->adapters[ii]->ipData.dhcpEnabled)
        {
            srd->adapters[ii]->ipData.dhcpEnabled = 1;
        }
    }

    pDRCtxt->bForceIp = 1;

    return(1);
}

/* Wait-a-minute function */
static void
WaitAMinute(PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("WaitAMinute"));

    PUT_TRACE(pDRCtxt, _T("DRWin32SetNetwork() succeeded. Finalizing network configuration."));
    printf(_T("\nFinalizing network configuration. This may take a few minutes...\n"));
    fflush(stdout);
    Sleep(MINUTE_IN_MILLIS);
}

/* Determine what network configuration method to use */
static int
GetNetworkConfigurationMethod(PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("GetNetworkConfigurationMethod"));

    USES_SRDQUERY;
    R_ULONG platform = 0;

    if (DRWin32IsASR())
    {
        return(NWCFG_ASR);
    }

    PREPARE_SRDQUERY(&platform, NULL, NULL, NULL, NULL);
    if (DO_SRDQUERY(
            pDRCtxt->srdH, SRDACT_GET_PLATFORM, SRDHT_SYSINFO
            ) != SRDERR_SUCCESS)
    {
        PUT_TRACE(pDRCtxt, _T("Error in SRDACT_GET_PLATFORM."));
        return(NWCFG_NONE);
    }

    if (IsLegacy(platform) && DRWin32IsEADR(pDRCtxt))
    {
        return(NWCFG_LEGACY_EADR);
    }

    if (DRWin32IsWinPE())
    {
        return(NWCFG_WINPE);
    }

    return(NWCFG_NONE);
}

/* Reconfigure network */
int
DRWin32SetNetwork(PDRContext pDRCtxt) /* parasoft-suppress  METRICS-28 "Old code.
                                         This legacy function was only slightly enhanced." */
{
    ERH_FUNCTION(_T("DRWin32SetNetwork"));

    int iRet=FALSE;
    NETWORKINFO curInfo;

    /* 
     * Network restore functionality for WinPE EADR is
     * now done by the DRM module, so we skip network restore. 
     */
    if (DRWin32IsWinPE())
    {
        return (TRUE);
    }

    DBGFCNIN();

    _TRY_
        int proceed = 0;
        PNETWORKINFO srdInfo = NULL;
        USES_SRDQUERY;

        int method = NWCFG_NONE;
        NWCFG_METHOD methods[] = {
            { InputAsr,     PreprocessDefault, WaitAMinute},
            { InputLegacy,  PreprocessLegacy,  WaitAMinute},
            { InputWinpe,   PreprocessDefault, WaitAMinute},
        };

        /*
            the following line must be in front of every _LEAVE_. otherwise
            FreeNetworkInfo() fails.
        */
        memset(&curInfo, 0, sizeof(NETWORKINFO));

        if (!InitNetworkModule(DebugCallback, pDRCtxt))
        {
            PUT_TRACE(pDRCtxt, _T("Cannot load network helper dlls."));
            _LEAVE_;
        }
        
        PREPARE_SRDQUERY(&srdInfo, NULL, NULL, NULL, NULL);
        if (DO_SRDQUERY(
                pDRCtxt->srdH, SRDACT_GET_NETWORK_INFO, SRDHT_SYSINFO
                ) != SRDERR_SUCCESS)
        {
            if (DRWin32IsWinPE())
            {
                PUT_TRACE(pDRCtxt, _T("Invalid/No network information inside SRD file."));
                PUT_TRACE(pDRCtxt, _T("Network left at the WinPE default."));
                iRet = TRUE;
            }
            else
            {
                PUT_TRACE(pDRCtxt, _T("Error in SRDACT_GET_NETWORK_INFO."));
            }
            _LEAVE_;
        }

        if (!GetNetworkInfo(&curInfo))
        {
            PUT_TRACE(pDRCtxt, _T("Cannot obtain current ")
                _T("system's network information."));
            _LEAVE_;
        }

        method = GetNetworkConfigurationMethod(pDRCtxt);
        if (method == NWCFG_NONE)
        {
            PUT_TRACE(pDRCtxt, _T("Not running in ASR/Legacy EADR mode."));
            iRet = TRUE;
            _LEAVE_;
        }
        
        if (!methods[method].input(pDRCtxt, srdInfo, &curInfo))
        {
            iRet = TRUE;
            _LEAVE_;
        }

        switch (methods[method].preprocess(pDRCtxt, srdInfo, &curInfo))
        {
        case 0:
            /* no reconfiguration necessary */
            PUT_TRACE(pDRCtxt, _T("Preprocessing requested the skipping ")
                _T("of the reconfiguration."));
            iRet = TRUE;
            break;
        case 1:
            /* Continue the reconfiguration */
            proceed = 1;
            break;
        default:
            /* an error occurred during the preprocessing */
            PUT_TRACE(pDRCtxt, _T("Error preprocessing network information."));
            break;
        }
        if (proceed == 0)
        {
            _LEAVE_;
        }

        printf(_T("Starting network configuration...\n"));
        fflush(stdout);

        if (!SetNetworkInfo(&curInfo, srdInfo))
        {
            PUT_TRACE(pDRCtxt, _T("Failed to set current ")
                _T("system's network information."));
            _LEAVE_;
        }

        /*QXCM1000217795*/
        methods[method].wait(pDRCtxt);

        iRet=TRUE;
    _FINALLY_
        FreeNetworkInfo(&curInfo);
        FreeNetworkModule();
    _ENDTRY_

    DBGFCNOUT();

    return(iRet);
}


/*========================================================================*//**
*
* @retval    Success - PANDR_RET_SUCCESS
* @retval    Error   - PANDR_RET_ERROR
*
* @brief     Disables Tape Driver
*
*//*=========================================================================*/
int DRWin32DisableLibraries(void)
{
    ERH_FUNCTION(_T("DRWin32DisableLibraries"));
    int iRet=PANDR_RET_ERROR;

    HMODULE hNtmsLib=NULL;

    OpenNtmsSessionProc fpOpen=NULL;
    CloseNtmsSessionProc fpClose=NULL;
    EnumerateNtmsObjectProc fpEnumerate=NULL;
    GetNtmsObjectInformationProc fpGetInfo=NULL;
    DisableNtmsObjectProc fpDisable=NULL;

    HANDLE h=NULL;
	NTMS_GUID *pArrNtmsObjs = NULL;
	NTMS_OBJECTINFORMATION ntmsObjInfo = {0};
	DWORD dwArrSize = 8;
	DWORD dwError = 0;
	DWORD dwIdx = 0;

    DbgFcnIn();

    __try
    {
        hNtmsLib=LoadLibrary(NTMS_DLLNAME);

        if (hNtmsLib==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("Loading of %s library failed. System Error: %d"), NTMS_DLLNAME, GetLastError());
            __leave;
        }

        fpOpen=(OpenNtmsSessionProc)GetProcAddress(hNtmsLib, OpenNtmsSessionName);
        if (fpOpen==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("fpOpen: GetProcAddress failed. System Error: %d"), GetLastError());
            __leave;
        }

        fpClose=(CloseNtmsSessionProc)GetProcAddress(hNtmsLib, CloseNtmsSessionName);
        if (fpClose==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("fpClose: GetProcAddress failed. System Error: %d"), GetLastError());
            __leave;
        }

        fpEnumerate=(EnumerateNtmsObjectProc)GetProcAddress(hNtmsLib, EnumerateNtmsObjectName);
        if (fpEnumerate==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("fpEnumerate: GetProcAddress failed. System Error: %d"), GetLastError());
            __leave;
        }
        
        fpGetInfo=(GetNtmsObjectInformationProc)GetProcAddress(hNtmsLib, GetNtmsObjectInformationName);
        if (fpGetInfo==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("fpGetInfo: GetProcAddress failed. System Error: %d"), GetLastError());
            __leave;
        }    

        fpDisable=(DisableNtmsObjectProc)GetProcAddress(hNtmsLib, DisableNtmsObjectName);
        if (fpDisable==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("fpDisable: GetProcAddress failed. System Error: %d"), GetLastError());
            __leave;
        }
    

        h=fpOpen(NULL, NULL, 0);
        if (h==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("OpenNtmsSession failed. System Error: %d"), GetLastError());
            __leave;
        }

	    pArrNtmsObjs=(NTMS_GUID *)MALLOC(sizeof(NTMS_GUID)*dwArrSize);
        if (pArrNtmsObjs==NULL)
        {
            DbgPlain(DBG_DR_INIT, _T("MALLOC returned NULL."));
            __leave;
        }
        
        for(dwError=ERROR_INSUFFICIENT_BUFFER; dwError!=ERROR_SUCCESS; )
	    {
		    dwError=fpEnumerate(h, NULL, pArrNtmsObjs, &dwArrSize, NTMS_LIBRARY, 0);
		    if (dwError==ERROR_INSUFFICIENT_BUFFER)
		    {
			    pArrNtmsObjs=(NTMS_GUID *)REALLOC(pArrNtmsObjs, sizeof(NTMS_GUID)*dwArrSize);
                if (pArrNtmsObjs==NULL)
                {
                    DbgPlain(DBG_DR_INIT, _T("REALLOC returned NULL."));
                    __leave;
                }
                DbgPlain(DBG_DR_INIT, _T("REALLOCed to %d."), dwArrSize);
			    continue;
		    }		
	    }

        ntmsObjInfo.dwType=NTMS_LIBRARY;        
	    ntmsObjInfo.dwSize=sizeof(NTMS_OBJECTINFORMATION);        

	    for (dwIdx = 0; dwIdx<dwArrSize; dwIdx++)
	    {
		    dwError=fpGetInfo(h, pArrNtmsObjs+dwIdx, &ntmsObjInfo);

            if (dwError!=ERROR_SUCCESS)
            {
                DbgPlain(DBG_DR_INIT, _T("GetNtmsObjectInformation failed at dwIdx=%d of %d. System Error: %d"), dwIdx, dwArrSize, dwError);
                __leave;
            }

    		if (ntmsObjInfo.Info.Library.LibraryType!=NTMS_LIBRARYTYPE_ONLINE)
		    {
			    DbgPlain(DBG_DR_INIT, _T("Library %s is not NTMS_LIBRARYTYPE_ONLINE. Proceeding..."), ntmsObjInfo.szName);
                continue;
		    }

		    if (ntmsObjInfo.Enabled)
		    {
                DbgPlain(DBG_DR_INIT, _T("Disabling NtmsObject %s..."), ntmsObjInfo.szName);
			    
                dwError=fpDisable(h, NTMS_LIBRARY, pArrNtmsObjs+dwIdx);
                if (dwError==ERROR_SUCCESS)
                {
                    DbgPlain(DBG_DR_INIT, _T("Done !!!"));
                    continue;
                }
                
                DbgPlain(DBG_DR_INIT, _T("DisableNtmsObject failed for %s. System Error: %d"), ntmsObjInfo.szName, dwError);
                 __leave;                
		    }
            DbgPlain(DBG_DR_INIT, _T("Object %s was not enabled. Proceeding..."), ntmsObjInfo.szName);
    	}

        iRet=PANDR_RET_SUCCESS;
    }

    __finally
    {
        if ((h!=NULL) && (fpClose!=NULL))
        {
            fpClose(h);
        }

        if (hNtmsLib!=NULL)
        {
            FreeLibrary(hNtmsLib);
        }
        
        FREE(pArrNtmsObjs);
    }

    DbgFcnOut();

    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
* @param     pAvOpt    -   pointer argvRec structure containining cmd line and
*                          parameters
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     1. If no clustering, bye.
*            2. If joining existing cluster or TempOS
*            -exlude /ClusterDatabase
*            3. Else
*            -tree /ClusterDatabase
*
*//*=========================================================================*/
int DRWin32CreateMSCSOptions(PDRContext pDRCtxt, argvRec *pAvOpt)
{
    ERH_FUNCTION(_T("DRWin32CreateMSCSOptions"));
    int iRet=PANDR_RET_ERROR;

    DbgFcnIn();

    __try
    {
        if (!pDRCtxt->bMSCluster)
        {
            iRet=PANDR_NO_EXCLUDE;
            __leave;
        }

        if((pDRCtxt->iMSCSMode==MSCS_DB_CLUS) && (!pDRCtxt->tempOS))
        {
            ArgvAdd(pAvOpt, TREE_OPT);
            ArgvAdd(pAvOpt, MSCLUSDB_ITEM);
        }
        else if((pDRCtxt->iMSCSMode==MSCS_FORM_CLUS) && (!pDRCtxt->tempOS))
        {
            tchar pszTempPath[STRLEN_PATH+1];

            GetSystemDirectory(pszTempPath, STRLEN_PATH);
            strcpy(pszTempPath+3, _T("TEMP\\"));
            pszTempPath[strlen(_T("C:\\TEMP\\"))]=NULL_TCHAR;

            ArgvAdd(pAvOpt, TREE_OPT);
            ArgvAdd(pAvOpt, MSCLUSDB_ITEM);
            ArgvAdd(pAvOpt, INTO_OPT);
            ArgvAdd(pAvOpt, pszTempPath);
        }

        iRet=PANDR_RET_SUCCESS;
    }
    __finally
    {
        /*Empty*/
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Set appropriate registry key and remember their old values.
*
*//*=========================================================================*/
int DRWin32DisableSystemFileProtection (PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32DisableSystemFileProtection"));
    int iRet=PANDR_RET_ERROR;

    /*-------------------------------------------------------------------------
    |   If the value AllowProtectedNames exists, it is stored and later 
    |   restored.
    |   Otherwise it is set to -1 and the value should be removed.
     ------------------------------------------------------------------------*/        
    iRet=RegReadDWORD(
        HKEY_LOCAL_MACHINE, SESSION_MANAGER_KEY, ALLOW_PROTECTED_RENAMES_VALUE,
        (DWORD *)&(pDRCtxt->iAllowProtectedRenames)
        );

    if (iRet==1)
    {
        pDRCtxt->iAllowProtectedRenames=-1;
    }
    
    iRet=RegWriteDWORD(HKEY_LOCAL_MACHINE, SESSION_MANAGER_KEY, ALLOW_PROTECTED_RENAMES_VALUE, 1);

    if (iRet==ERROR_SUCCESS)
    {
        iRet=PANDR_RET_SUCCESS;
    }

    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Reset appropriate registry key.
*
*//*=========================================================================*/
int DRWin32EnableSystemFileProtection (PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32EnableSystemFileProtection"));
    int iRet=PANDR_RET_ERROR;
    DWORD dwValue=0;

    if (pDRCtxt->iAllowProtectedRenames!=-1)
    {
        iRet=RegWriteDWORD(
                HKEY_LOCAL_MACHINE, SESSION_MANAGER_KEY, ALLOW_PROTECTED_RENAMES_VALUE,
                pDRCtxt->iAllowProtectedRenames
                );
        
        if (iRet==ERROR_SUCCESS)
        {
            iRet=PANDR_RET_SUCCESS;
        }
    }
    else
    {
        HKEY hKey=NULL;

        iRet=RegOpenKeyEx(HKEY_LOCAL_MACHINE, SESSION_MANAGER_KEY, 0, KEY_ALL_ACCESS, &hKey);                
        iRet=RegDeleteValue(hKey, ALLOW_PROTECTED_RENAMES_VALUE);
        iRet=RegCloseKey(hKey);
        
        if (iRet==ERROR_SUCCESS)
        {
            iRet=PANDR_RET_SUCCESS;
        }
    }        
    return iRet;
}

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Checks if application runs in a visible window.
*
*//*=========================================================================*/
int DRWin32IsAppVisible()
{
    ERH_FUNCTION(_T("DRWin32IsAppVisible"));
    int iRet=FALSE;    
    HWND hWindow=NULL;
    
    DbgFcnIn();
    hWindow=GetConsoleHwnd();

    if (hWindow!=NULL)
    {
        iRet=IsWindowVisible(hWindow);
    }

    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Get a handle to console window of a process.
*
*//*=========================================================================*/
static HWND GetConsoleHwnd(void)
{
    // Adapted from from MSDN Q124103

#define MY_BUFSIZE 1024                  // Buffer size for console window titles.

    HWND hwndFound=NULL;                 // This is what is returned to the caller.
    tchar pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated WindowTitle.
    tchar pszOldWindowTitle[MY_BUFSIZE]; // Contains original WindowTitle.
    int iRet=-1;

    __try
    {
        // Fetch current window title.
        if (GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE)==0)
        {
            DbgPlain(
                DBG_DR_ACTION, 
                _T("GetConsoleTitle failed. Error=%d"), GetLastError()
                );
            __leave;
        }

        // Format a "unique" NewWindowTitle.
        wsprintf(
            pszNewWindowTitle,
            _T("%d/%d"), GetTickCount(), GetCurrentProcessId()
            );

        // Change current window title.
        SetConsoleTitle(pszNewWindowTitle);

        // Ensure window title has been updated.
        Sleep(10);

        // Look for NewWindowTitle.
        hwndFound=FindWindow(NULL, pszNewWindowTitle);

        // Restore original window title.
        SetConsoleTitle(pszOldWindowTitle);
    }
    __finally
    {
    }

    return(hwndFound);
} 

/*========================================================================*//**
*
* @param     pDRCtxt   -   pointer to DR Context
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Make application's window window.
*
*//*=========================================================================*/
int DRWin32MakeMeVisible (PDRContext pDRCtxt)
{
    ERH_FUNCTION(_T("DRWin32MakeMeVisible"));
    int iRet=PANDR_RET_SUCCESS;    
    WINDOWPLACEMENT sWinPlac={0};
    HWND hMyWindow=NULL;

    DbgFcnIn();

    __try
    {
        if (pDRCtxt->bIsVisible)
        {
            DbgStamp(DBG_DR_PROCGLOB);
            DbgPlain(DBG_DR_PROCGLOB, _T("I am ALREADY visible."));           
            __leave;
        }

        sWinPlac.length=sizeof(WINDOWPLACEMENT);
        sWinPlac.showCmd=SW_SHOWNORMAL;

        DbgStamp(DBG_DR_PROCGLOB);
        DbgPlain(DBG_DR_PROCGLOB, _T("I was NOT visible."));

        hMyWindow=GetConsoleHwnd();

        if (hMyWindow==NULL)
        {
            DbgPlain(DBG_DR_PROCGLOB, _T("I am still NOT visible."));
            iRet=PANDR_RET_ERROR;
            __leave;
        }

        ShowWindow(hMyWindow, SW_SHOWMAXIMIZED);
        ShowWindow(hMyWindow, SW_SHOWMAXIMIZED);
        /*--- CHECME & ATTENTION --------------------------------------------
        |
        |   If ShowWindow is called for the first time it does not always 
        |   display the window. So, call it again and then it works.
        |
         -------------------------------------------------------------------*/
        pDRCtxt->bIsVisible=TRUE;
        DbgPlain(DBG_DR_PROCGLOB, _T("I am visible NOW."));
    }
    __finally
    {
    }
            
    DbgFcnOut();
    return iRet;
}

/*========================================================================*//**
*
* @param     iTimeout   -
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     !!!!!!!!!!! CHECME !!!!!!!!!!
*            This function is never used. It most likely even does not work.
*            Do not use it without a thorough inspection.
*
*//*=========================================================================*/
int DRWin32PickMenu(int iTimeout)
{
    ERH_FUNCTION(_T("DRWin32PickMenu"));
    int iRet=PANDR_RET_ERROR, iGetC=PANDR_RET_ERROR;
    int bKbHit=FALSE, bExtCode=FALSE;

    DbgFcnIn();

    while (!bKbHit && (!iTimeout>0))
    {
        Sleep(50);
        bKbHit=_kbhit();
        iTimeout--;        
    }

    if (bKbHit)
    {
        iGetC=_getch();
        if ((iGetC==0) || (iRet==0xE0))
        {
            bExtCode=TRUE;
            iGetC=_getch();
        }

        if (!bExtCode)
        {
            switch(iGetC)
            {
            case 27:
                iRet=PANDR_ESC_KEY;
                break;

            case 82:
            case 114:
                iRet=PANDR_R_KEY;
                break;

            case 67:
            case 99:
                iRet=PANDR_C_KEY;
                break;               
            }            
        }
    }
    else
    {
        iRet=PANDR_KEYBOARD_TIMEOUT;
    }


    DbgFcnOut();
    return iRet;
}


/*========================================================================*//**
*
* @retval    Success  -  PANDR_RET_SUCCESS
* @retval    Error    -  PANDR_RET_ERROR
*
* @brief     Waits for a key stroke
*
*//*=========================================================================*/
#define KEYSTROKE_TIMEOUT_CYCLES           20*900
/* 20 * 50 ms = 1s, 900 * 1 s = 15 min */
void DRWin32WaitForKeystroke()
{
    int iWaitCycles=0;

    while ((!_kbhit()) && (iWaitCycles<KEYSTROKE_TIMEOUT_CYCLES))
    {
        Sleep(50);
        iWaitCycles++;
    }
    return;
}


/*========================================================================*//**
*
* @brief     Reboots the system
*
*//*=========================================================================*/
void DRWin32Reboot()
{
    DRWin32SetProcessPrivilege(SE_SHUTDOWN_NAME , TRUE);
    ExitWindowsEx(EWX_REBOOT, 0);
    return;
}

/*========================================================================*//**
*
* @param     pszName     -   name of a host
* @param     pazAddress  -   corresponding IP address
*
*//*=========================================================================*/
void DRWin32GetIpAddress(tchar *pszName, tchar *pszAddress)
{
    ERH_FUNCTION(_T("GetIPAddress"));

    struct addrinfo *ai, *ailist;
    tchar buf[STRLEN_IPADDRESS+1] = _T("");
    BOOL bRet = FALSE;
    USES_CONVERSION;

    DbgFcnIn();

    __try
    {
        ailist = IpcGetHostInfo(pszName);
        if(ailist == NULL)
        {
            DbgPlain(DBG_DR_ACTION, _T("IpcGetHostInfo() FAILED."));
            __leave;
        }

        for (ai=ailist; ai != NULL; ai=ai->ai_next)
        {
            ipw_ntop(ai->ai_addr, (socklen_t)ai->ai_addrlen, buf);
            if (!StrIsEmpty(buf))
            {
                break;
            }
        }
        if (StrIsEmpty(buf))
        {
            DbgPlain(DBG_DR_ACTION, _T("Failed to convert sockaddr to string form."));
            __leave;
        }

        StrCopy(pszAddress, buf, STRLEN_HOSTNAME);
        bRet = TRUE;
    }
    __finally
    {    
        if(bRet==TRUE)
        {
            DbgPlain(DBG_DR_ACTION, _T("Successfully converted host name %s to IP %s."), pszName, pszAddress);
        }
        else 
        {
            strcpy(pszAddress, pszName);
        }
    }
    DbgFcnOut();
    return;
}
