/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   sysnfop1s
* @file      recovery/srd/SysNfo/SysNfoP1S/sysnfop1s.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     /
*
* @since     dd.mm.yy	lukaj	Original Coding
*
* @remarks   /
*/
#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/SysNfo/SysNfoP1S/sysnfop1s.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/
#if TARGET_LINUX
/** \include fcntl.h
    \brief File control options
*/
#   include <fcntl.h>

/** \include sys/time.h
    \brief Get process times
*/
#   include <sys/time.h>
#else
#include <clusapi.h>
#define SYSNFOP1S   _T("sysnfop1s.dll")
#endif

#include "lib/cmn/common.h"
#include "recovery/srd/srdcmn.h"
#include "recovery/srd/srdfree.h"
#include "recovery/srd/srdcmnstruct.h"
#include "recovery/srd/srdp1s.h"
#include "sysnfop1s.h"
#include "recovery/drm/inc/drm.h"

#if TARGET_WIN32
#define IMG_NAME            _T("recovery.zip")
#else
#define IMG_NAME            _T("recovery.img")
#endif

#define TMP_EXT             _T(".tmp")
#define SIG_PFX             _T("sig")
#define IMG_PFX             _T("img")
#define P1S_PFX             _T("p1s")
#define INI_NAME            _T("recovery.p1s")
#define SIG_NAME            _T("recovery.sig")
#define DISK_DIR_NAME       L"Disk1"
#define DRSTART_NAME        L"drstart.exe"
#define BS                  L"\\"

/* ---------------------------------------------------------------------------
|   defines
 ---------------------------------------------------------------------------*/
#if TARGET_LINUX
/** \def SYSNFOP1S
    \brief Name of SysNfoP1S library
*/
#define SYSNFOP1S   _T("sysnfop1s.so")
#endif

#if !TARGET_LINUX
BOOL APIENTRY DllMain( HANDLE hModule,
                       R_DWORD  ul_reason_for_call,
                       R_VOID* lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }

    return TRUE;
}
#endif

static R_BOOL
P1SVerifyLocation(R_TCHAR *filePath)
{
    R_BOOL  bRet = FALSE;
    FILEHANDLE hFile = INVALID_HANDLE_VALUE;

    _TRY_
        hFile = OB2_OpenFile(filePath, 1, 1);
        if(hFile == INVALID_HANDLE_VALUE)
            _LEAVE_;
        bRet = TRUE;
    _FINALLY_
        if(hFile != INVALID_HANDLE_VALUE)
            OB2_CloseFile(hFile);
        OB2_DeleteFile(filePath);
    _ENDTRY_

    return bRet;
}

/** \fn P1SGetTempFileName(R_TCHAR *pszPfx,
                            R_TCHAR *pszSfx,
                            R_TCHAR *pszDef,
                            R_TCHAR *pszOut)
    \brief Get temporary file
    \param pszPfx Prefix path
    \param pszSfx Sufix path
    \param pszDef Default path
    \param pszOut Output path
    \return Return pszOut - outut path
*/
static R_TCHAR*
P1SGetTempFileName(R_TCHAR *pszPfx, R_TCHAR *pszSfx, R_TCHAR *pszDef, R_TCHAR *pszOut)
{
    R_TCHAR *pszRet = NULL;
    FILESEARCHHANDLE hFind;
    R_DWORD tickCount = 0;
    R_TCHAR pfxStr[5] = { 0 }, sfxStr[5] = { 0 };
    R_TCHAR tempPath[STRLEN_PATH+1] = { 0 }, tempName[STRLEN_PATH+1] = { 0 };

#if !TARGET_WIN32
    struct  timeval tv;

    gettimeofday(&tv, NULL);
    tickCount = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#else
    tickCount = GetTickCount();
#endif

    _TRY_
        R_BOOL try_again;

        if(NULL == pszOut)
            _LEAVE_;
        if(pszPfx != NULL)
            strncpy(pfxStr, pszPfx, 4);
        if(pszSfx != NULL)
            strncpy(sfxStr, pszSfx, 4);

        do {
            FILESEARCHDATA wfd = { 0 };

            sprintf(tempName, _T("%s%08u%s"), pfxStr, tickCount, sfxStr);
            PathConcat(tempPath, cmnPanTmp, tempName, STRLEN_PATH);

            DbgPlain(81, _T("Allocating temp path: %s."), tempPath);

            switch (OB2_FindFirstFile(&hFind, tempPath, &wfd))
            {
            case -1: /* Path not found */
                try_again = FALSE;
                break;
            case -2: /* An unexpected error occurred */
                try_again = TRUE;
                break;
            default: /* Everything OK */
                tickCount += 1;
                OB2_FindClose(&hFind);
                try_again = TRUE;
                break;
            }
        } while (try_again);

        if (P1SVerifyLocation(tempPath) == FALSE)
            _LEAVE_;

        strcpy(pszOut, tempPath);

        pszRet = pszOut;
    _FINALLY_
        if(
            NULL == pszRet &&
            NULL != pszDef &&
            NULL != pszOut
        )
        {
            PathConcat(pszOut, cmnPanTmp, pszDef, STRLEN_PATH);
            pszRet = pszOut;
        }
    _ENDTRY_

    return pszRet;
}

SYSNFOP1S_API R_RESULT
P1S_GetErrorInfo(R_HANDLE drim, R_INT* outDrmErrCode, R_INT* outWinErrCode, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const char *value;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_get_error_information(di->pDrmHandle, outDrmErrCode, (R_UINT *)outWinErrCode, &value) == DRM_ERR_OK)
    {
#if TARGET_WIN32
        USES_CONVERSION;
        UNREF_CONV;
#endif
        strcpy(outValue, A2T(value));

        Return = SRDERR_SUCCESS;
    }

	return Return;
}

static R_VOID
DRMHandleDrimError(PDRIMINFO pDrim)
{
    ERH_FUNCTION(_T("HandleDrimError"));

    SRD_ENTER_FCN;

    _TRY_
        R_INT  drimError = DRM_ERR_OK;
        R_TCHAR  errorText[STRLEN_STD + 1];
        R_DWORD winError = SRDERR_SUCCESS, errorCount = 0, warningCount = 0;

        if(NULL == pDrim || NULL == pDrim->pDrmHandle)
        {
            DbgPlain(81, _T("Invalid input parameters."));
            _LEAVE_;
        }

        if(P1S_GetErrorInfo(pDrim, &drimError, &winError, errorText) != DRM_ERR_OK)
        {
            DbgPlain(81, _T("P1S_GetErrorInfo() failure."));
            _LEAVE_;
        }

        if(NULL == pDrim->pDrimStatus)
            pDrim->pDrimStatus = MALLOC(sizeof(DRIMSTATUS));

        else if(pDrim->pDrimStatus->errorText != NULL)
            FREE(pDrim->pDrimStatus->errorText);

        if(NULL == pDrim->pDrimStatus)
        {
            DbgPlain(81, _T("Failed to allocate DRIMSTATUS information."));
            _LEAVE_;
        }
        memset(pDrim->pDrimStatus, 0, sizeof(DRIMSTATUS));

        pDrim->pDrimStatus->drimError = drimError;
        pDrim->pDrimStatus->winError = winError;
        pDrim->pDrimStatus->warningCount = warningCount;
        pDrim->pDrimStatus->errorCount = errorCount;
        if(NULL != errorText && errorText[0] != '\0')
        pDrim->pDrimStatus->errorText = StrNewCopy(errorText);

        DbgPlain(81, _T("*** DRIM STATUS ***."));
        DbgPlain(81, _T("  Warning count: %d."), (R_INT)pDrim->pDrimStatus->warningCount);
        DbgPlain(81, _T("  Error count:   %d."), (R_INT)pDrim->pDrimStatus->errorCount);
        DbgPlain(81, _T("  DRIM error:    %ld."), pDrim->pDrimStatus->drimError);
        DbgPlain(81, _T("  Windows error: %d."), (R_INT)pDrim->pDrimStatus->winError);
        if(NULL != pDrim->pDrimStatus->errorText)
            DbgPlain(81, _T("  Error text:    %s."), pDrim->pDrimStatus->errorText);
    _NOFINALLY_

    SRD_RETURN_VOID;
}

/* ---------------------------------------------------------------------------
|
|	Cluster Specific Section
|
 ---------------------------------------------------------------------------*/
#if TARGET_LINUX
/** TODO Clusters*/
#else
#define CLUSAPI_DLL_NAME    _T("clusapi.dll")

typedef HCLUSTER (__stdcall *OpenClusterProc)(LPCWSTR);
typedef BOOL (__stdcall *CloseClusterProc)(HCLUSTER);
typedef HCLUSENUM (__stdcall *ClusterOpenEnumProc)(HCLUSTER, R_DWORD);
typedef R_DWORD (__stdcall *ClusterCloseEnumProc)(HCLUSENUM);
typedef R_DWORD (__stdcall *ClusterEnumProc)(HCLUSENUM, R_DWORD, R_DWORD*, LPWSTR, R_DWORD*);
typedef HRESOURCE (__stdcall *OpenClusterResourceProc)(HCLUSTER, LPCWSTR);
typedef BOOL (__stdcall *CloseClusterResourceProc)(HRESOURCE);
typedef R_DWORD (__stdcall *ClusterResourceControlProc)(HRESOURCE, OPTIONAL HNODE, R_DWORD, R_VOID*, R_DWORD, R_VOID*, R_DWORD, R_DWORD*);

typedef struct _tagClusFcn
{
    HMODULE                     hClusMod;
    OpenClusterProc             fcnOpen;
    CloseClusterProc            fcnClose;
    ClusterOpenEnumProc         fcnOpenEnum;
    ClusterCloseEnumProc        fcnCloseEnum;
    ClusterEnumProc             fcnEnum;
    OpenClusterResourceProc     fcnOpenRes;
    CloseClusterResourceProc    fcnCloseRes;
    ClusterResourceControlProc  fcnControl;
} CLUSFCN, *PCLUSFCN;

typedef struct _tagSigInfo
{
    R_TCHAR *volName;               /* IN:  volume name                                     */
    R_ULONG signature;              /* OUT: disk signature that contains the input volume   */
} SIGINFO, *PSIGINFO;

typedef struct _tagDiskVolInfo
{
    R_TCHAR     volName[MAX_PATH];  /* OUT: volume name                             */
    R_ULONG     signature;          /* IN:  disk signature                          */
    R_ULONG     index;              /* IN:  volume index from the disk to return    */
} DISKVOLINFO, *PDISKVOLINFO;

/* ---------------------------------------------------------------------------
|	Gets the disk signature of the input cluster volume. Must be WINAPI.
 ---------------------------------------------------------------------------*/
static R_RESULT WINAPI
ClusterGetDiskSignature(R_BYTE *buffer, R_DWORD dwSize, PSIGINFO sigInfo)
{
    ERH_FUNCTION(_T("ClusterGetDiskSignature"));

    R_RESULT    retVal = SRDERR_ERROR;
    R_BOOL      foundName = FALSE;
    CLUSPROP_BUFFER_HELPER cbh = { 0 };

    USES_CONVERSION;
    UNREF_CONV;


    SRD_ENTER_FCN;

    cbh.pb = buffer;
    while(foundName == FALSE && cbh.pb < buffer + dwSize && cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        switch(cbh.pSyntax->dw)
        {
            case CLUSPROP_SYNTAX_PARTITION_INFO:
                if(stricmp(sigInfo->volName, W2T(cbh.pPartitionInfoValue->szDeviceName)) == 0)
                {
                    DbgPlain(DBG_SYSNFO, _T("Found device name: %ws."), cbh.pPartitionInfoValue->szDeviceName);
                    foundName = TRUE;
                }
                break;
            default:
                break;
        }

        cbh.pb += (sizeof( CLUSPROP_VALUE ) + ALIGN_CLUSPROP(cbh.pValue->cbLength)); // Syntax and length.
    }

    if(foundName == TRUE)
    {
        cbh.pb = buffer;
        while(retVal != SRDERR_SUCCESS && cbh.pb < buffer + dwSize && cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
        {
            switch(cbh.pSyntax->dw)
            {
                case CLUSPROP_SYNTAX_DISK_SIGNATURE:
                    DbgPlain(DBG_SYSNFO, _T("Signature: %lu."), cbh.pDiskSignatureValue->dw);
                    sigInfo->signature = cbh.pDiskSignatureValue->dw;

                    retVal = SRDERR_SUCCESS;
                    break;
                default:
                    break;
            }

            cbh.pb += (sizeof( CLUSPROP_VALUE ) + ALIGN_CLUSPROP(cbh.pValue->cbLength)); // Syntax and length.
        }
    }

    SRD_RETURN_VAL(retVal);
}

/* ---------------------------------------------------------------------------
|	Gets the volume that resides on the specific disk (signature). Must be
|   WINAPI.
 ---------------------------------------------------------------------------*/
static R_RESULT WINAPI
ClusterGetNextDiskVolume(R_BYTE *buffer, R_DWORD dwSize, PDISKVOLINFO dvInfo)
{
    ERH_FUNCTION(_T("ClusterGetNextDiskVolume"));

    R_RESULT    retVal = SRDERR_ERROR;
    R_BOOL      sigFound = FALSE;
    R_ULONG     currentIndex = 0;
    CLUSPROP_BUFFER_HELPER cbh = { 0 };

    USES_CONVERSION;
    UNREF_CONV;


    SRD_ENTER_FCN;

    cbh.pb = buffer;
    while(sigFound == FALSE && cbh.pb < buffer + dwSize && cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        switch(cbh.pSyntax->dw)
        {
            case CLUSPROP_SYNTAX_DISK_SIGNATURE:
                if(dvInfo->signature == cbh.pDiskSignatureValue->dw)
                {
                    DbgPlain(DBG_SYSNFO, _T("Signature: %lu."), cbh.pDiskSignatureValue->dw);

                    sigFound = TRUE;
                }
                break;
            default:
                break;
        }

        cbh.pb += (sizeof( CLUSPROP_VALUE ) + ALIGN_CLUSPROP(cbh.pValue->cbLength)); // Syntax and length.
    }

    if(sigFound == TRUE)
    {
        cbh.pb = buffer;
        while(retVal != SRDERR_SUCCESS && cbh.pb < buffer + dwSize && cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
        {
            switch(cbh.pSyntax->dw)
            {
                case CLUSPROP_SYNTAX_PARTITION_INFO:
                    if(currentIndex++ == dvInfo->index)
                    {
                        DbgPlain(DBG_SYSNFO, _T("Found device name: %ws."), cbh.pPartitionInfoValue->szDeviceName);
                        strcpy(dvInfo->volName, W2T(cbh.pPartitionInfoValue->szDeviceName));

                        retVal = SRDERR_SUCCESS;
                    }
                    break;
                default:
                    break;
            }

            cbh.pb += (sizeof( CLUSPROP_VALUE ) + ALIGN_CLUSPROP(cbh.pValue->cbLength)); // Syntax and length.
        }
    }

    SRD_RETURN_VAL(retVal);
}

/* ---------------------------------------------------------------------------
|	Performs cluster resource query. Handles only CLUS_RESCLASS_STORAGE storage
|   resource classes. fpAction function performs resource specific action.
 ---------------------------------------------------------------------------*/
static R_RESULT
ClusterQueryResource(HCLUSTER hCluster, PCLUSFCN cf, WCHAR* resourceName, FARPROC fpAction, R_VOID *fpData)
{
    ERH_FUNCTION(_T("ClusterQueryResource"));

    R_RESULT    retVal = SRDERR_ERROR;
    HRESOURCE   hResource = NULL;
    void        *buffer = NULL;
    R_BOOL      newResource = TRUE;
    R_DWORD     dwResult = 0, dwBytesReturned = 0, dwBufferSize = 0;
    CLUSPROP_BUFFER_HELPER      cbh = { 0 };
    CLUS_RESOURCE_CLASS_INFO    resClassInfo = { 0 };

    SRD_ENTER_FCN;

    __try
    {
        hResource = cf->fcnOpenRes(hCluster, resourceName);

        if (hResource == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("HANDLED WIN ERROR: %d ... ClusterOpenResource."), GetLastError()); }
            );

        DbgPlain(DBG_SYSNFO, _T("Opened resource %ws."), resourceName);

        dwResult = cf->fcnControl(
            hResource,
            NULL,
            CLUSCTL_RESOURCE_GET_CLASS_INFO,
            NULL,
            0,
            &resClassInfo,
            sizeof(resClassInfo),
            &dwBytesReturned
        );

        if (dwResult != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("HANDLED WIN ERROR: %d CLUSCTL_RESOURCE_GET_CLASS_INFO."), GetLastError()); }
            );

        switch(resClassInfo.rc)
        {
            case CLUS_RESCLASS_STORAGE:
                DbgPlain(DBG_SYSNFO, _T("The resource is %s storage...."),
                    resClassInfo.SubClass & CLUS_RESSUBCLASS_SHARED ? _T("SHARED") : _T("LOCAL"));
                break;
            default:
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("The resource is not a storage resource.")); }
                );
        }

        dwBytesReturned = 10240;
        do {
            if(buffer != NULL)
                FREE(buffer);

            buffer = MALLOC(dwBytesReturned);
            if(buffer == NULL)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("Memory allocation: no space for buffer.")); }
                );

            dwResult = cf->fcnControl(
                hResource,
                NULL,
                CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                NULL,
                0,
                buffer,
                dwBytesReturned,
                &dwBytesReturned
            );
        } while(dwResult == ERROR_MORE_DATA);

        if(dwResult != ERROR_SUCCESS)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("HANDLED WIN ERROR: %d, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO."), GetLastError()); }
            );

        retVal = (R_RESULT)fpAction(buffer, dwBytesReturned, fpData);
    }
    __finally
    {
        if(buffer != NULL)
            FREE(buffer);
        if(hResource != NULL && cf->fcnCloseRes != NULL)
            cf->fcnCloseRes(hResource);
    }

    SRD_RETURN_VAL(retVal);
}

/* ---------------------------------------------------------------------------
|	Performs cluster resource action. Takes care of loading cluster DLL,
|   opening active cluster and basic resource enumeration. Delegates other
|   work to ClusterResourceQuery() function.
 ---------------------------------------------------------------------------*/
static R_RESULT
ClusterResourceAction(FARPROC fpAction, R_VOID *fpData)
{
    ERH_FUNCTION(_T("ClusterResourceAction"));

    R_RESULT    retVal = SRDERR_ERROR;
    HCLUSTER    hCluster = NULL;
    HCLUSENUM   hEnum = NULL;
    CLUSFCN     cf = { 0 };
    R_ULONG     bufSize = 100;
    WCHAR       *buffer = NULL;

    SRD_ENTER_FCN;

    __try
    {
        R_ULONG ii = 0;
        R_ULONG index = 0, ulResult = ERROR_SUCCESS;

        cf.hClusMod = LoadLibrary(CLUSAPI_DLL_NAME);
        if(NULL == cf.hClusMod)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cluster API library could not be loaded. Possibly unavailable on this system.")); }
            );
        cf.fcnOpen = (OpenClusterProc)GetProcAddress(cf.hClusMod, "OpenCluster");
        cf.fcnClose = (CloseClusterProc)GetProcAddress(cf.hClusMod, "CloseCluster");
        cf.fcnOpenEnum = (ClusterOpenEnumProc)GetProcAddress(cf.hClusMod, "ClusterOpenEnum");
        cf.fcnCloseEnum = (ClusterCloseEnumProc)GetProcAddress(cf.hClusMod, "ClusterCloseEnum");
        cf.fcnEnum = (ClusterEnumProc)GetProcAddress(cf.hClusMod, "ClusterEnum");
        cf.fcnOpenRes = (OpenClusterResourceProc)GetProcAddress(cf.hClusMod, "OpenClusterResource");
        cf.fcnCloseRes = (CloseClusterResourceProc)GetProcAddress(cf.hClusMod, "CloseClusterResource");
        cf.fcnControl = (ClusterResourceControlProc)GetProcAddress(cf.hClusMod, "ClusterResourceControl");

        if(
            NULL == cf.fcnOpen ||
            NULL == cf.fcnClose ||
            NULL == cf.fcnOpenEnum ||
            NULL == cf.fcnCloseEnum ||
            NULL == cf.fcnEnum ||
            NULL == cf.fcnOpenRes ||
            NULL == cf.fcnCloseRes ||
            NULL == cf.fcnControl
        )
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("clusapi.dll functions could not be bound. Wrong dll version ???.")); }
            );

        hCluster = cf.fcnOpen(NULL);
        if(NULL == hCluster)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Cannot open current cluster - error %d."), GetLastError()); }
            );
        hEnum = cf.fcnOpenEnum(hCluster, CLUSTER_ENUM_RESOURCE);
        if (hEnum == NULL)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("ClusterOpenEnum: GetLastError: %d."), GetLastError()); }
            );

        buffer = MALLOC(bufSize * sizeof(WCHAR));
        if(NULL == buffer)
            SRD_W32_LEAVE(
                { DbgPlain(DBG_SYSNFO, _T("Buffer allocation error.")); }
            );

        for (index = 0, ulResult = ERROR_SUCCESS; ulResult != ERROR_NO_MORE_ITEMS && retVal != SRDERR_SUCCESS;)
        {
            R_DWORD dwType = 0;
            R_DWORD dwOldBuff = bufSize;

            if (NULL == buffer)
                SRD_W32_LEAVE(
                    { DbgPlain(DBG_SYSNFO, _T("Buffer (re)allocation error.")); }
                );

            ulResult = cf.fcnEnum(hEnum, index, &dwType, buffer, &bufSize);

            switch (ulResult)
            {
                case ERROR_SUCCESS:
                    DbgPlain(DBG_SYSNFO, _T("Resource found: %ws."), buffer);
                    switch(dwType)
                    {
                        case CLUSTER_ENUM_RESOURCE:
                            retVal = ClusterQueryResource(hCluster, &cf, buffer, fpAction, fpData);
                            break;
                        case CLUSTER_ENUM_NODE:
                        case CLUSTER_ENUM_RESTYPE:
                            break;
                        default:
                            break;
                    }

                    index++;
                    bufSize = dwOldBuff;
                    break;
                case ERROR_MORE_DATA:
                    DbgPlain(DBG_SYSNFO, _T("Need bigger buffer %d."), bufSize);
                    bufSize += 20;
                    buffer = REALLOC(buffer, bufSize * sizeof(WCHAR));
                    break;
                case ERROR_NO_MORE_ITEMS:
                    DbgPlain(DBG_SYSNFO, _T("End of list."));
                    break;
                default:
                    SRD_W32_LEAVE(
                        { DbgPlain(DBG_SYSNFO, _T("ClusterOpenEnum error %d."), ulResult); }
                    );
            }
        }
    }
    __finally
    {
        if(NULL != buffer)
            FREE(buffer);
        if(NULL != hEnum && NULL != cf.fcnCloseEnum)
            cf.fcnCloseEnum(hEnum);
        if(NULL != hCluster && NULL != cf.fcnClose)
            cf.fcnClose(hCluster);
        if(NULL != cf.hClusMod)
            FreeLibrary(cf.hClusMod);
    }

    SRD_RETURN_VAL(retVal);
}

/* ---------------------------------------------------------------------------
|	Adds additional cluster volumes to the INI file. This volumes must be
|   added in order for DRIM to format them during recovery, otherwise the
|   W2K cluster does not come online.
 ---------------------------------------------------------------------------*/
static R_RESULT
ClusterAddRequiredDiskVolumes(PSRDDATA pSrdData, PRESTVOL **restVols, R_ULONG *restCount, R_ULONG ulPurpose)
{
    ERH_FUNCTION(_T("ClusterAddRequiredDiskVolumes"));

    R_RESULT    retVal = SRDERR_ERROR;
    R_ULONG     ii = 0;
    R_ULONG     rC = *restCount, origC = rC;
    PRESTVOL    *rV = *restVols;

    SRD_ENTER_FCN;

    _TRY_
        R_ULONG     jj = 0;
        R_ULONG     quorumVolume = 0;

        DbgPlain(81, _T("Required purpose = %lu."), ulPurpose);

        /* ---------------------------------------------------------------------------
        |   find searched volume among restore objects.
         ---------------------------------------------------------------------------*/
        for(ii = 0; ii < origC; ii++)
        {
            SIGINFO     si = { 0 };
            DISKVOLINFO dvi = { 0 };
            R_TCHAR     volName[MAX_PATH] = { 0 };

            if(
                (rV[ii]->VolumePurpose & ulPurpose) == 0 ||
                strlen(rV[ii]->VolumeName) > 2
            )
                continue;

            quorumVolume = rV[ii]->VolumeName[1];
            DbgPlain(81, _T("Found '%lu' volume with required purpose."), quorumVolume);

            /* ---------------------------------------------------------------------------
            |   Search for the required volume. If the current one is not found, go on
            |   and verify another.
             ---------------------------------------------------------------------------*/
            sprintf(volName, _T("%C:"), (R_TCHAR)quorumVolume);
            si.volName = volName;

            if(ClusterResourceAction((FARPROC)ClusterGetDiskSignature, &si) != SRDERR_SUCCESS)
                continue;

            dvi.signature = si.signature;
            /* ---------------------------------------------------------------------------
            |   if no disk signature is found, search for another volume of the same type.
             ---------------------------------------------------------------------------*/
            if(dvi.signature == 0)
            {
                DbgPlain(81, _T("No valid disk signature for volume '%lu'."), quorumVolume);
                continue;
            }

            while(TRUE)
            {
                R_ULONG kk = 0;
                R_TCHAR pszName[MAX_PATH] = { 0 };

                if(ClusterResourceAction((FARPROC)ClusterGetNextDiskVolume, &dvi) != SRDERR_SUCCESS)
                    break;

                dvi.index++;
                /* ---------------------------------------------------------------------------
                |   verify if the currently examined volume does not already reside
                |   among restore objects. if it does, just proceed to the next volume.
                 ---------------------------------------------------------------------------*/
                sprintf(pszName, _T("/%C"), dvi.volName[0]);
                StrToUpper(pszName);

                for(kk = 0; kk < rC; kk++)
                {
                    if(stricmp(pszName, rV[kk]->VolumeName) == 0)
                    {
                        rV[kk]->VolumePurpose |= ulPurpose;
                        break;
                    }
                }
                if(kk < rC)
                {
                    DbgPlain(81, _T("Volume %s is already among restore objects."), pszName);
                    continue;
                }
                else
                    DbgPlain(81, _T("Volume %s is not yet among restore objects."), pszName);
                /* ---------------------------------------------------------------------------
                |   the volume does not yet exist among restore object. add it.
                 ---------------------------------------------------------------------------*/
                rV = REALLOC(rV, (rC + 1) * sizeof(PRESTVOL));
                if(rV == NULL)
                {
                    DbgPlain(81, _T("FAILURE - reallocating objects' array."));
                    _LEAVE_;
                }
                rV[rC] = MALLOC(sizeof(RESTVOL));
                if(rV[rC] == NULL)
                {
                    DbgPlain(81, _T("FAILURE - allocating new object's memory."));
                    _LEAVE_;
                }
                sprintf(rV[rC]->VolumeName, pszName);
                rV[rC]->VolumePurpose = ulPurpose;
                rV[rC]->VolumeType = OT_WINFS;
                rV[rC]->SrdObject = NULL;

                rC += 1;
                DbgPlain(81, _T("Restore object count's been increased to %d."), rC);
            }
        }

        *restVols = rV;
        *restCount = rC;

        retVal = SRDERR_SUCCESS;
    _FINALLY_
        if(retVal != SRDERR_SUCCESS)
        {
            /* ---------------------------------------------------------------------------
            |   free the difference between the original restore objects and currently
            |   added volumes. the original ones must not be freed.
             ---------------------------------------------------------------------------*/
            for(ii = origC; ii < rC; ii++)
                FREE(rV[ii]);
        }
    _ENDTRY_

    SRD_RETURN_VAL(retVal);
}
#endif /* linux - cluster*/

SYSNFOP1S_API R_RESULT
P1S_StartSession(R_HANDLE drim, R_TCHAR* workDir, DRM_HANDLE *session)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_initialize_ex(T2W(workDir), session) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_StopSession(R_HANDLE drim, R_BOOL keepLogFile)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_finalize(di->pDrmHandle, keepLogFile) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiCreate(R_HANDLE drim, DRM_INFO_HANDLE* ri)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_create(di->pDrmHandle, ri) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiOpen(R_HANDLE drim, R_TCHAR* riFile, DRM_INFO_HANDLE* ri, R_INT *pReturn)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    *pReturn = dll->fn_drm_ri_open(di->pDrmHandle, T2W(riFile), ri);

    if(*pReturn == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;
    else if(*pReturn == DRM_ERR_INVVER)
        *pReturn = SRDERR_INVVER;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClose(R_HANDLE drim, DRM_INFO_HANDLE ri)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_close(di->pDrmHandle, ri) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddUserData(R_HANDLE drim, DRM_INFO_HANDLE ri, R_TCHAR* key, R_TCHAR *value)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_user_data(di->pDrmHandle, ri, T2W(key), T2W(value)) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddUserBootVolume(R_HANDLE drim, R_TCHAR* bootVolume)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_user_boot_volume(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(bootVolume)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClearUserBootVolume(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_clear_user_boot_volume(di->pDrmHandle,
        di->pDrmInfoHandle) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClearUserSystemVolume(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_clear_user_system_volume(di->pDrmHandle,
        di->pDrmInfoHandle) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClearUserInstallVolume(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_clear_user_install_volume(di->pDrmHandle,
        di->pDrmInfoHandle) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClearRestoreSetVolume(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_clear_restore_set_volume(di->pDrmHandle,
        di->pDrmInfoHandle) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddUserSystemVolume(R_HANDLE drim, R_TCHAR* systemVolume)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_user_system_volume(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(systemVolume)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddUserInstallVolume(R_HANDLE drim, R_TCHAR* installVolume)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_user_install_volume(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(installVolume)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddRestoreSetVolume(R_HANDLE drim, R_TCHAR* rsVolume)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_restore_set_volume(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(rsVolume)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetUserData(R_HANDLE drim, DRM_INFO_HANDLE ri, R_TCHAR* key, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_user_data(di->pDrmHandle, ri,
        T2W(key), &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

/* ---------------------------------------------------------------------------
|   This is a wrapper around the DRM's drm_ri_get_system_uuid() API. It simply
|   converts all parameters into / from the format understood by both - DRM as
|   well as DPDR - parts of the code.
---------------------------------------------------------------------------*/
SYSNFOP1S_API R_RESULT
P1S_RiGetSystemUUID(R_HANDLE drim,
                    DRM_INFO_HANDLE ri,
                    const R_TCHAR* separator,
                    R_TCHAR *outValue)
{
    R_RESULT result = SRDERR_ERROR;

    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if (dll->fn_drm_ri_get_system_uuid(di->pDrmHandle, ri,
        T2W(separator), &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        result = SRDERR_SUCCESS;
    }

    return(result);
}

SYSNFOP1S_API R_RESULT
P1S_RiGetMediaType(R_HANDLE drim, int* mtCode, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const char *value;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_get_media_type(di->pDrmHandle, di->pDrmInfoHandle,
        mtCode, &value) == DRM_ERR_OK)
    {
#if TARGET_WIN32
        USES_CONVERSION;
        UNREF_CONV;
#endif
        strcpy(outValue, A2T(value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiSetMediaType(R_HANDLE drim, int mtCode)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_set_media_type(di->pDrmHandle, di->pDrmInfoHandle,
        (int)mtCode) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiStore(R_HANDLE drim, R_TCHAR* riFilePath)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_store(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(riFilePath)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RegisterNotify(R_HANDLE drim, drm_notify_callback callback_fcn, void* data)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if (dll->fn_drm_register_notify(di->pDrmHandle, callback_fcn, data) == DRM_ERR_OK)
        Return  = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RegisterRestoreResult(R_HANDLE drim, drm_int32_t result, drm_bool_t reboot)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if (dll->fn_drm_register_restore_result(di->pDrmHandle, result, reboot) == DRM_ERR_OK)
        Return  = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_GetLogFileName(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_get_log_file_name(di->pDrmHandle, &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetRestoreCommand(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_restore_command(di->pDrmHandle, di->pDrmInfoHandle,
        &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetRestoreCommandParams(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_restore_command_params(di->pDrmHandle, di->pDrmInfoHandle,
        &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiSetRestoreCommand(R_HANDLE drim, R_TCHAR* rCommand, R_TCHAR* rcParams)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_set_restore_command(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(rCommand), T2W(rcParams)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetRecoveryScopeTitle(R_HANDLE drim, R_TCHAR *outTitle)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *title = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_recovery_scope_title(di->pDrmHandle, di->pDrmInfoHandle,
        &title) == DRM_ERR_OK)
    {
        strcpy(outTitle, W2T((wchar_t *)title));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetRecoveryScope(R_HANDLE drim,
                       drm_recovery_scope_param_t scopeType,
                       drm_recovery_scope_t* scope,
                       R_TCHAR *outDescription)
{
    R_RESULT Return = SRDERR_ERROR;
    const char *description = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_get_recovery_scope(di->pDrmHandle, di->pDrmInfoHandle,
        scopeType, scope, &description) == DRM_ERR_OK)
    {
#if TARGET_WIN32
        USES_CONVERSION;
        UNREF_CONV;
#endif
        strcpy(outDescription, A2T(description));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

/* Schedule a backup application file to be encrypted. */
SYSNFOP1S_API R_RESULT
P1S_RiAddEncryptedFile(R_HANDLE drim, const R_TCHAR* encPath)
{
    R_RESULT result = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if (dll->fn_drm_ri_add_encrypted_file(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(encPath)) == DRM_ERR_OK)
    {
        result = SRDERR_SUCCESS;
    }

    return(result);
}

SYSNFOP1S_API R_RESULT
P1S_RsCreate(R_HANDLE drim, R_TCHAR* rsPath, DRM_SET_HANDLE* rs)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rs_create(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(rsPath), rs) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiSetDepot(R_HANDLE drim, R_TCHAR* rDepot)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_set_depot(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(rDepot)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetDepot(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_depot(di->pDrmHandle, di->pDrmInfoHandle,
        &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsOpen(R_HANDLE drim, R_TCHAR* rsFile, DRM_SET_HANDLE* rs)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rs_open(di->pDrmHandle, T2W(rsFile), rs) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsSetRecoveryInfo(R_HANDLE drim, DRM_SET_HANDLE rs, DRM_INFO_HANDLE ri)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rs_set_recovery_info(di->pDrmHandle, rs,
        di->pDrmInfoHandle) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsGetRecoveryInfo(R_HANDLE drim, DRM_SET_HANDLE rs, DRM_INFO_HANDLE *ri)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rs_get_recovery_info(di->pDrmHandle, rs, ri) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsCreateISOImage(R_HANDLE drim, DRM_SET_HANDLE rs, R_TCHAR *isoPath)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rs_create_image(di->pDrmHandle, di->pDrmInfoHandle,
        rs, T2W(isoPath)) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsExportDriverStore(R_HANDLE drim, DRM_SET_HANDLE rs, R_TCHAR *driverStore)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rs_export_driver_store(di->pDrmHandle, rs, T2W(driverStore)) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RsClose(R_HANDLE drim, DRM_SET_HANDLE rs)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rs_close(di->pDrmHandle, rs) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_DRMExportDriverStore(R_HANDLE pDrimInfo, R_TCHAR* driverStore)
{
    ERH_FUNCTION(_T("P1S_DRMExportDriverStore"));

    R_RESULT    Return = SRDERR_ERROR;
    PDRIMINFO   pDrimIn = (PDRIMINFO)pDrimInfo;
    DRM_SET_HANDLE rs = NULL;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = SRDERR_SUCCESS;

        if (NULL == driverStore || *driverStore == _T('\0'))
        {
            DbgPlain(81, _T("Driver store NULL or empty string."));
            _LEAVE_;
        }

        if (NULL == pDrimIn->pszImageFile)
        {
            DbgPlain(81, _T("Recovery set image file NULL."));
            _LEAVE_;
        }

        if(P1S_RsOpen(pDrimIn, pDrimIn->pszImageFile, &rs) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("Failed to open recovery set."));
            _LEAVE_;
        }

        resDrim = P1S_RsExportDriverStore(pDrimIn, rs, driverStore);

        if(resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RsExportDriverStore() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(rs != NULL)
            P1S_RsClose(pDrimIn, rs);
    _ENDTRY_

    SRD_RETURN_VAL(Return);

}

/* ---------------------------------------------------------------------------
|   Copying of scsitab MA configuration file to the <windows>\temp directory,
|   where it can be picked up by DRIM file collecting. temp directory is
|   created if it does not exist.
 ---------------------------------------------------------------------------*/
#define SCSITABTEMPDIRNAME      _T("\\Temp")
#define SCSITABSOURCENAME       _T("\\scsitab")
#define SCSITABDESTNAME         _T("\\scsitab.dev.cfg")

static R_RESULT
CopyScsitabFile(R_VOID)
{
    ERH_FUNCTION(_T("CopyScsitabFile"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

#if TARGET_LINUX
/** TODO*/
#else
    _TRY_
        R_TCHAR     scsitabPath[MAX_PATH] = { 0 };
        R_TCHAR     scsitabSource[MAX_PATH] = { 0 };

        if(GetWindowsDirectory(scsitabPath, MAX_PATH) == 0)
        {
            DbgPlain(81, _T("Cannot get Windows directory [%lu]."), GetLastError());
            _LEAVE_;
        }
        strcat(scsitabPath, SCSITABTEMPDIRNAME);
        if(
            CreateDirectory(scsitabPath, NULL) == FALSE &&
            GetLastError() != ERROR_ALREADY_EXISTS
        )
        {
            DbgPlain(81, _T("Cannot create temporary directory [%lu]."), GetLastError());
            _LEAVE_;
        }
        strcat(scsitabPath, SCSITABDESTNAME);
        sprintf(scsitabSource, _T("%s") SCSITABSOURCENAME, cmnPanBase);

        if(CopyFile(scsitabSource, scsitabPath, FALSE) == FALSE)
        {
            DbgPlain(81, _T("Cannot copy scsitab file [%lu]."), GetLastError());
            _LEAVE_;
        }
        Return = SRDERR_SUCCESS;
    _NOFINALLY_
#endif
    SRD_RETURN_VAL(Return);
}

#define REGSVC_OMNIINET         _T("SYSTEM\\CurrentControlSet\\Services\\OmniInet")
#define REGSVC_CRS              _T("SYSTEM\\CurrentControlSet\\Services\\omniback_crs")
#define REGSVC_VELOCIS          _T("SYSTEM\\CurrentControlSet\\Services\\Velocis (ob2_40)")
#define REGVAL_START            _T("Start")

static R_RESULT
ChangeStartup(R_BOOL restore, R_TCHAR* pszKey, R_DWORD *pValue)
{
    ERH_FUNCTION(_T("ChangeStartup"));

    R_RESULT    Return = SRDERR_ERROR;

#if TARGET_LINUX
    SRD_ENTER_FCN;
#else
    HKEY        hService = NULL;

    SRD_ENTER_FCN;

    DbgPlain(81, _T("Restoring for %s is %d."), pszKey, restore);

    _TRY_
        R_DWORD dwDemand = SERVICE_DEMAND_START;

        R_DWORD dwValue = 0, dwSize = 0;

        if(NULL == pValue)
            _LEAVE_;

        if(FALSE == restore)
            *pValue = (R_DWORD)-1;

        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, KEY_WRITE | KEY_READ, &hService) != ERROR_SUCCESS)
        {
            DbgPlain(81, _T("Cannot open key %s."), pszKey);
            _LEAVE_ERROR_(Return, SRDERR_SUCCESS);
        }
        if(FALSE == restore)
        {
            dwSize = sizeof(R_DWORD);
            if(RegQueryValueEx(hService, REGVAL_START, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
            {
                DbgPlain(81, _T("Cannot query value Start."));
                _LEAVE_;
            }
            if(RegSetValueEx(hService, REGVAL_START, 0, REG_DWORD, (LPBYTE)&dwDemand, sizeof(R_DWORD)) != ERROR_SUCCESS)
            {
                DbgPlain(81, _T("Cannot set value Start."));
                _LEAVE_;
            }

            *pValue = dwValue;
            DbgPlain(81, _T("Value Start set to %d."), dwDemand);
            Return = SRDERR_SUCCESS;
        }
        else
        {
            if(*pValue == (R_DWORD)-1)
            {
                DbgPlain(81, _T("Value is -1 and will not be set."));
                _LEAVE_ERROR_(Return, SRDERR_SUCCESS);
            }
            if(RegSetValueEx(hService, REGVAL_START, 0, REG_DWORD, (LPBYTE)pValue, sizeof(R_DWORD)) != ERROR_SUCCESS)
            {
                DbgPlain(81, _T("Cannot set value Start."));
                _LEAVE_;
            }
            DbgPlain(81, _T("Value Start restored to %d."), *pValue);
            Return = SRDERR_SUCCESS;
        }
    _FINALLY_
        if(hService != NULL)
            RegCloseKey(hService);
    _ENDTRY_
#endif

    SRD_RETURN_VAL(Return);
}

static R_RESULT
ChangeServiceStartup(R_BOOL restore, R_DWORD *pInet, R_DWORD *pCrs, R_DWORD *pRds)
{
    R_RESULT    Return = SRDERR_SUCCESS;

    ChangeStartup(restore, REGSVC_OMNIINET, pInet);
    ChangeStartup(restore, REGSVC_CRS, pCrs);
    ChangeStartup(restore, REGSVC_VELOCIS, pRds);
#if TARGET_LINUX
/** TODO*/
#else
    RegFlushKey(HKEY_LOCAL_MACHINE);
#endif

    return Return;
}

static R_RESULT
PrepareRecoveryInfoHandle(PDRIMINFO pDrim, PSRDDATA pSrdData)
{
    R_RESULT            iRet = SRDERR_ERROR;
    R_ULONG             ii = 0;
    R_ULONG             restoreCount = 0, origCount = 0;
    PRESTVOL            *restoreVols = NULL;

    _TRY_
        R_TCHAR pszOs[32] = { 0 };
        R_TCHAR pszVolName[MAX_PATH] = { 0 };
        R_TCHAR string[STRLEN_STD+1];
#if !TARGET_LINUX
        USES_CONVERSION;
        UNREF_CONV;
#endif

        if (NULL == pDrim)
        {
            DbgPlain(81, _T("DRIM info structure is NULL!"));
            _LEAVE_;
        }
        if (OsStringFromTable(pDrim->Platform, pszOs) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("OsStringFromTable failed!"));
            _LEAVE_;
        }
        if (P1S_RiAddUserData(pDrim, pDrim->pDrmInfoHandle, _T("Os"), pszOs) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RiAddUserData [OS] failed!"));
            _LEAVE_;
        }
#if TARGET_LINUX
        sprintf(string, _T("%ld"), pDrim->Version);
#else
        _stprintf(string, _T("%d"), pDrim->Version);
#endif
        if (P1S_RiAddUserData(pDrim, pDrim->pDrmInfoHandle, _T("Version"), string) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RiAddUserData [Version] failed!"));
            _LEAVE_;
        }
        if (pSrdData->RestoreVolumes->RestVolCount == 0)
        {
            DbgPlain(81, _T("No rest volumes!"));
            _LEAVE_;
        }
        /* ---------------------------------------------------------------------------
        |   allocate a separate array of restore objects to keep the original one
        |   safe. cluster and possibly some other object make me do that.
         ---------------------------------------------------------------------------*/
        restoreVols = MALLOC(pSrdData->RestoreVolumes->RestVolCount * sizeof(PRESTVOL));
        if (NULL == restoreVols)
        {
            DbgPlain(81, _T("Allocate memory failed!"));
            _LEAVE_;
        }
        for (ii = 0L; ii < pSrdData->RestoreVolumes->RestVolCount; ii++)
        {
            restoreVols[ii] = pSrdData->RestoreVolumes->RestoreVolumes[ii];
        }
        restoreCount = pSrdData->RestoreVolumes->RestVolCount;
        origCount = restoreCount;

#if TARGET_LINUX
/** TODO Cluster*/
#else
        if (ClusterAddRequiredDiskVolumes(pSrdData, &restoreVols, &restoreCount, QUORUM_VOLUME) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("Add required disk volumes [Quorum] failed!"));
            _LEAVE_;
        }
        if (ClusterAddRequiredDiskVolumes(pSrdData, &restoreVols, &restoreCount, OB2DB_CONFIG) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("Add required disk volumes [OB2DB] failed!"));
            _LEAVE_;
        }
#endif

        for (ii = 0L; ii < restoreCount; ii++)
        {
            PRESTVOL pRestVol = restoreVols[ii];

            /* ---------------------------------------------------------------------------
            |   skip volumes that should not be added to the OBVolumes section. this goes
            |   for non-volume objects as well as additional data volumes.
             ---------------------------------------------------------------------------*/
#if TARGET_WIN32
            if ((pRestVol->VolumePurpose & CFG_VOLUME) != 0 ||
                (pRestVol->VolumePurpose & OB2DB_FILES) != 0
#if 0 /* TODO: enable this to switch off Full Recovery */
                || pRestVol->VolumePurpose == DATA_VOLUME
#endif
            )
            {
                continue;
            }
#else
            if (((pRestVol->VolumePurpose & CFG_VOLUME) != 0 &&
                (pRestVol->VolumePurpose & DATA_VOLUME) == 0) ||
                (pRestVol->VolumePurpose & OB2DB_FILES) != 0
#if 0 /* TODO: enable this to switch off Full Recovery */
                || pRestVol->VolumePurpose == DATA_VOLUME
#endif
            )
            {
                continue;
            }
#endif

#if TARGET_LINUX
            strcpy(pszVolName, pRestVol->VolumeName);
#else
            if (strlen(pRestVol->VolumeName) > 2)
            {
                /* ---------------------------------------------------------------------------
                |   Windows 7/Windows 2008 R2 hidden boot volume
                ---------------------------------------------------------------------------*/
                if (StrCmp(pRestVol->VolumeName, _T("@")) == 0)
                {
                    strcpy(pszVolName, pRestVol->VolumeName);
                }
                else if (StrStr(pRestVol->VolumeName, _T("Volume{")) != NULL)
                {
                    sprintf(pszVolName, _T("\\%s"), pRestVol->VolumeName);
                }
                else
                    strcpy(pszVolName, pRestVol->VolumeName + 1);
            }
            else
                sprintf(pszVolName, _T("%s:"), pRestVol->VolumeName + 1);

            PathToBackslashes(pszVolName);
#endif
            /* ---------------------------------------------------------------------------
            |   Skip Windows 7/Windows 2008 R2 hidden boot volume
             ---------------------------------------------------------------------------*/
            if (StrCmp(pRestVol->VolumeName, _T("@")) != 0)
            {
                /* ---------------------------------------------------------------------------
                |   this one is only for restore set. currently it is implemented to be equal
                |   to OBVolumes section, but it could be different.
                 ---------------------------------------------------------------------------*/
                if (P1S_RiAddRestoreSetVolume(pDrim, pszVolName) != SRDERR_SUCCESS)
                {
                    DbgPlain(81, _T("P1S_RiAddRestoreSetVolume [%s] failed!"), pszVolName);
                    _LEAVE_;
                }
            }

            /* ---------------------------------------------------------------------------
            |   special treatment for boot volume. its number is added to a special
            |   part of the section. here it is assumed that only one volume can be
            |   assigned BOOT_VOLUME flag.
             ---------------------------------------------------------------------------*/
            if ((pRestVol->VolumePurpose & BOOT_VOLUME) != 0)
            {
                if (P1S_RiAddUserBootVolume(pDrim, pszVolName) != SRDERR_SUCCESS)
                {
                    DbgPlain(81, _T("P1S_RiAddRestoreSetVolume [BOOT_VOLUME] failed!"));
                    _LEAVE_;
                }
            }

            /* ---------------------------------------------------------------------------
            |   special treatment for system volume. its number is added to a special
            |   part of the section. here it is assumed that only one volume can be
            |   assigned SYSTEM_VOLUME flag.
             ---------------------------------------------------------------------------*/
            if ((pRestVol->VolumePurpose & SYSTEM_VOLUME) != 0 ||
                (pRestVol->VolumePurpose & PROFILES_VOLUME) != 0 ||
                (pRestVol->VolumePurpose & ADS_VOLUME) != 0
            )
            {
                if (P1S_RiAddUserSystemVolume(pDrim, pszVolName) != SRDERR_SUCCESS)
                {
                    DbgPlain(81, _T("P1S_RiAddRestoreSetVolume [SYSTEM_VOLUME|PROFILES_VOLUME|ADS_VOLUME] failed!"));
                    _LEAVE_;
                }
            }
            /* ---------------------------------------------------------------------------
            |   special treatment for OmniBack volumes. their numbers are added to a special
            |   part of the section. here it is assumed that more than one volume can be
            |   assigned OmniBack volume flags.
             ---------------------------------------------------------------------------*/
            if ((pRestVol->VolumePurpose & OB2_VOLUME) != 0 ||
                (pRestVol->VolumePurpose & OB2_DATA_VOLUME) != 0 ||
                (pRestVol->VolumePurpose & OB2DB_VOLUME) != 0
            )
            {
                if (P1S_RiAddUserInstallVolume(pDrim, pszVolName) != SRDERR_SUCCESS)
                {
                    DbgPlain(81, _T("P1S_RiAddRestoreSetVolume [OB2_VOLUME|OB2_DATA_VOLUME|OB2DB_VOLUME] failed!"));
                    _LEAVE_;
                }

            }
        }

#if TARGET_LINUX
        if (P1S_RiSetRestoreCommand(pDrim, _T("Disk1/drstart"), _T("")) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RiSetRestoreCommand [Disk1/drstart] failed!"));
            _LEAVE_;
        }
#else
        if (P1S_RiSetRestoreCommand(pDrim, _T("Disk1\\drstart.exe"), _T("")) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RiSetRestoreCommand [Disk1\\drstart.exe] failed!"));
            _LEAVE_;
        }
#endif
        iRet = SRDERR_SUCCESS;
    _FINALLY_
        /* ---------------------------------------------------------------------------
        |   free the difference between the original restore objects and currently
        |   added volumes. the original ones must not be freed.
         ---------------------------------------------------------------------------*/
        for (ii = origCount; ii < restoreCount; ii++)
        {
            FREE(restoreVols[ii]);
        }
        if (restoreVols != NULL)
        {
            FREE(restoreVols);
        }
    _ENDTRY_

    return(iRet);
}

SYSNFOP1S_API R_RESULT
P1S_DRMPrepareRecoverySet(PDRIMINFO pDrim, PSRDDATA pSrdData)
{
    ERH_FUNCTION(_T("P1S_DRMPrepareRecoverySet"));

    R_RESULT            Return = SRDERR_ERROR;
    R_TCHAR             pszIniPath[MAX_PATH] = { 0 };
    R_TCHAR             pszImgPath[MAX_PATH] = { 0 };
    DRM_SET_HANDLE rs = NULL;

    SRD_ENTER_FCN;

    _TRY_
        R_RESULT    resDrim = SRDERR_SUCCESS;
        R_DWORD dwInet = -1, dwCrs = -1, dwRds = -1;

        if(
            NULL == pDrim ||
            NULL == pDrim->pDrmHandle ||
            NULL == pSrdData->RestoreVolumes ||
            NULL == pSrdData->Header
        )
            _LEAVE_;

        pDrim->Platform = pSrdData->Header->Platform;
        pDrim->Version = pSrdData->Header->Version;

       if(PrepareRecoveryInfoHandle(pDrim, pSrdData) != SRDERR_SUCCESS)
       {
            DbgPlain(81, _T("PrepareRecoveryInfoHandle() FAILED."));
            _LEAVE_;
       }

       if (pDrim->callback_function &&
           P1S_RegisterNotify(pDrim, pDrim->callback_function, pDrim->user_data) != SRDERR_SUCCESS)
       {
            DbgPlain(81, _T("P1S_RegisterNotify() FAILED."));
            _LEAVE_;
       }

        P1SGetTempFileName(P1S_PFX, TMP_EXT, INI_NAME, pszIniPath);
        P1SGetTempFileName(IMG_PFX, TMP_EXT, IMG_NAME, pszImgPath);
        DbgPlain(81, _T("Temporary file name: %s."), pszImgPath);

        CopyScsitabFile();

        ChangeServiceStartup(FALSE, &dwInet, &dwCrs, &dwRds);
        resDrim = P1S_RsCreate(pDrim, pszImgPath, &rs);
        DRMHandleDrimError(pDrim);
        ChangeServiceStartup(TRUE, &dwInet, &dwCrs, &dwRds);

        if(resDrim != DRM_ERR_OK)
        {
            DbgPlain(81, _T("P1S_RsCreate() FAILED [error code: %d]."), resDrim);
            _LEAVE_;
        }

        pDrim->pszImageFile = StrNewCopy(pszImgPath);
        if(NULL == pDrim->pszImageFile)
            _LEAVE_;

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(rs)
            P1S_RsClose(pDrim, rs);
        if(Return != SRDERR_SUCCESS)
        {
            if(*pszImgPath != _T('\0'))
                OB2_DeleteFile(pszImgPath);
            FreeDrimData(pDrim);
        }
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

static R_RESULT
UpdateIniFile(PDRIMINFO pInfo)
{
    R_RESULT iRet = SRDERR_ERROR;

    _TRY_
        R_TCHAR path[MAX_PATH] = { 0 };
        R_TCHAR *encFile = NULL;

        if(
            NULL == pInfo ||
            NULL == pInfo->pDrmHandle ||
            NULL == pInfo->pDrmInfoHandle
        )
            _LEAVE_;
        /* On all Windows versions cmnPanBaseAppData and cmnPanBase are set to \Program Files\OmniBack\.
           The only exception is Vista, where cmnPanBase is set to \Program Files\OmniBack\ and
           cmnPanBaseAppData is set to \ProgramData\Omniback\. On all Windows versions DRSetup is
           located in cmnPanBase. The only exception is Vista where DRSetup is in cmnPanBaseAppData. */
        if ((pInfo->Platform == OS_WXP64) || (pInfo->Platform == OS_VISTA_64) || 
            (pInfo->Platform == OS_WIN7_64) || (pInfo->Platform == OS_WIN8_64)||
            (pInfo->Platform == OS_WIN81_64) || (pInfo->Platform == OS_WIN10_64))
        {
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetup64"), cmnPanBaseAppData);
        }
#else
           _stprintf(path, L"%s\\Depot\\DRSetup64", T2W(cmnPanBaseAppData));
        }
#endif
        else if ((pInfo->Platform == OS_WXPAMD64) ||
            (pInfo->Platform == OS_VISTA_AMD64) ||
            (pInfo->Platform == OS_WIN7_AMD64) ||
            (pInfo->Platform == OS_WIN8_AMD64) ||
            (pInfo->Platform == OS_WIN81_AMD64) ||
            (pInfo->Platform == OS_WIN10_AMD64))
        {
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetupx8664"), cmnPanBaseAppData);
        }
#else
           _stprintf(path, L"%s\\Depot\\DRSetupx8664", T2W(cmnPanBaseAppData));
        }
#endif
        else if(pInfo->Platform == OS_LNX)
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetupLNX"), cmnPanBaseAppData);
#else
           _stprintf(path, L"%s\\Depot\\DRSetupLNX", T2W(cmnPanBaseAppData));
#endif
        else if(pInfo->Platform == OS_LNX64)
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetupLNX64"), cmnPanBaseAppData);
#else
           _stprintf(path, L"%s\\Depot\\DRSetupLNX64", T2W(cmnPanBaseAppData));
#endif
        else if(pInfo->Platform == OS_LNXAMD64)
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetupLNXx8664"), cmnPanBaseAppData);
#else
           _stprintf(path, L"%s\\Depot\\DRSetupLNXx8664", T2W(cmnPanBaseAppData));
#endif
        else
#if TARGET_LINUX
           sprintf(path, _T("%s/Depot/DRSetup"), cmnPanBaseAppData);
#else
           _stprintf(path, L"%s\\Depot\\DRSetup", T2W(cmnPanBaseAppData));
#endif

        if(P1S_RiSetDepot(pInfo, path) != SRDERR_SUCCESS)
            _LEAVE_;

#if TARGET_LINUX
        encFile = _T("Disk1/recovery.srd");
#else
        encFile = _T("Disk1\\recovery.srd");
#endif
        if (P1S_RiAddEncryptedFile(pInfo, encFile) != SRDERR_SUCCESS)
        {
            _LEAVE_;
        }

        iRet = SRDERR_SUCCESS;
    _NOFINALLY_

    return iRet;
}

static R_RESULT
P1S_DRMExtractRecoveryInfoInternal(PDRIMINFO pDrim, DRM_SET_HANDLE rs, DRM_INFO_HANDLE *ri)
{
    ERH_FUNCTION(_T("P1S_DRMExtractRecoveryInfoInternal"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = DRM_ERR_OK;
#if !TARGET_LINUX
        USES_CONVERSION;
        UNREF_CONV;
#endif

        if(
            NULL == pDrim ||
            NULL == pDrim->pDrmHandle ||
            NULL == rs
        )
            _LEAVE_;

        resDrim = P1S_RsGetRecoveryInfo(pDrim, rs, ri);

        if(resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RsGetRecoveryInfo() FAILED [error code: %d]."), resDrim);
            _LEAVE_;
        }

        Return = SRDERR_SUCCESS;
    _NOFINALLY_

    SRD_RETURN_VAL(Return);
}

static R_RESULT
P1S_DRMInsertRecoveryInfoInternal(PDRIMINFO pDrim, DRM_INFO_HANDLE ri, DRM_SET_HANDLE rs)
{
    ERH_FUNCTION(_T("P1S_DRMInsertRecoveryInfoInternal"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = DRM_ERR_OK;
#if !TARGET_LINUX
        USES_CONVERSION;
        UNREF_CONV;
#endif

        if(
            NULL == pDrim ||
            NULL == pDrim->pDrmHandle ||
            NULL == pDrim->pDrmInfoHandle ||
            NULL == rs ||
            NULL == ri
        )
            _LEAVE_;

        resDrim = P1S_RsSetRecoveryInfo(pDrim, rs, ri);

        if(resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RsSetRecoveryInfo() FAILED [error code: %d]."), resDrim);
            _LEAVE_;
        }

        Return = SRDERR_SUCCESS;
    _NOFINALLY_

    SRD_RETURN_VAL(Return);
}

SYSNFOP1S_API R_RESULT
P1S_Initialize(tchar *pPostFix, tchar *pRanges, tchar *pSelect)
{
    R_RESULT Return = SRDERR_ERROR;
#if TARGET_WIN32
    if (CmnInit (
            SYSNFOP1S, _T("sysnfop1s"),
            pPostFix, pRanges, pSelect, 0) == 0
        )
    {
        Return = SRDERR_SUCCESS;
    }
#else
    Return = SRDERR_SUCCESS;
#endif
    return Return;
}

SYSNFOP1S_API R_VOID
P1S_Uninitialize(R_VOID)
{
#if TARGET_WIN32
    CmnExit();
#endif
}

SYSNFOP1S_API R_RESULT
P1S_DRMCreateISOImage(R_HANDLE pDrimInfo, R_TCHAR *pszIsoPath, R_BOOL fTape)
{
    ERH_FUNCTION(_T("P1S_DRMCreateISOImage"));

    R_RESULT    Return = SRDERR_ERROR;
    PDRIMINFO   pDrimIn = (PDRIMINFO)pDrimInfo;
    DRM_SET_HANDLE rs = NULL;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = SRDERR_SUCCESS;
        int mediaType = DRM_RMT_CD_ISO_IMAGE;

        if(
            NULL == pszIsoPath ||
            *pszIsoPath == _T('\0')
        )
            _LEAVE_;

        if(NULL == pDrimIn->pszImageFile)
        {
            DbgPlain(81, _T("FAILURE - invalid input options."));
            _LEAVE_;
        }

        if(UpdateIniFile(pDrimIn) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("UpdateIniFile() FAILED."));
            _LEAVE_;
        }

        if(P1S_RsOpen(pDrimIn, pDrimIn->pszImageFile, &rs) != SRDERR_SUCCESS)
            _LEAVE_;

        switch (fTape)
        {
        case MT_TAPE:
            mediaType = DRM_RMT_TAPE_ISO_IMAGE;
            break;
        case MT_NET:
            mediaType = DRM_RMT_WIM_IMAGE;
            break;
        default:
            mediaType = DRM_RMT_CD_ISO_IMAGE;
            break;
        }

        if (P1S_RiSetMediaType(pDrimIn, mediaType) != SRDERR_SUCCESS)
        {
            _LEAVE_;
        }

        DbgPlain(81, _T("Creating ISO image."));

        resDrim = P1S_RsCreateISOImage(pDrimIn, rs, pszIsoPath);
        DRMHandleDrimError(pDrimIn);

        if(resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("FAILED to create CD ISO image [error code: %d]."), resDrim);
            _LEAVE_;
        }

        DbgPlain(81, _T("Created ISO image."));

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(SRDERR_SUCCESS != Return)
        {
            if(
                pszIsoPath != NULL &&
                *pszIsoPath != _T('\0')
            )
#if TARGET_LINUX
                remove(pszIsoPath);
#else
                DeleteFile(pszIsoPath);
#endif
        }

        if(rs != NULL)
            P1S_RsClose(pDrimIn, rs);
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

extern R_RESULT CopyLogFile(PDRIMINFO);

SYSNFOP1S_API R_RESULT
P1S_SignRecoverySet(R_HANDLE pDrimInfo, R_TCHAR *pszSignature)
{
    ERH_FUNCTION(_T("P1S_SignRecoverySet"));

    R_RESULT    Return = SRDERR_ERROR;
    PDRIMINFO   pDrim = (PDRIMINFO)pDrimInfo;
    DRM_SET_HANDLE rs = NULL;
    DRM_INFO_HANDLE ri = NULL;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = DRM_ERR_OK;

       if(
            NULL == pDrim ||
            NULL == pDrim->pDrmHandle ||
            NULL == pDrim->pDrmInfoHandle ||
            NULL == pszSignature ||
            NULL == pDrim->pszImageFile
        )
            _LEAVE_;

        if((resDrim = P1S_RiAddUserData(pDrim, pDrim->pDrmInfoHandle, _T("Signature"), pszSignature)) != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("Error setting KEY_SIGNATURE value %s."), pszSignature);
            _LEAVE_;
        }
        if(pDrim->Signature != NULL)
            FREE(pDrim->Signature);
        pDrim->Signature = StrNewCopy(pszSignature);

        resDrim = P1S_RsOpen(pDrim, pDrim->pszImageFile, &rs);
        if (resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RsOpen() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }
        resDrim = P1S_DRMExtractRecoveryInfoInternal(pDrim, rs, &ri);
        if (resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_DRMExtractRecoveryInfoInternal() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }
        resDrim = P1S_RiAddUserData(pDrim, ri, _T("Signature"), pszSignature);
        if (resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("Error setting KEY_SIGNATURE value %s."), pszSignature);
            _LEAVE_;
        }
        resDrim = P1S_DRMInsertRecoveryInfoInternal(pDrim, ri, rs); /* parasoft-suppress  PB-07 "Coding Guidelines Feedback code suppression see PB-07-3 issue on the page" */
        if (resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_DRMInsertRecoveryInfoInternal() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(ri)
            P1S_RiClose(pDrim, ri);
        if(rs)
            P1S_RsClose(pDrim, rs);
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

SYSNFOP1S_API R_RESULT
P1S_VerifyRecoverySet(R_HANDLE pDrimInfo, R_ULONG *cmpResult, R_TCHAR *imgSig, R_TCHAR *p1sSig)
{
    ERH_FUNCTION(_T("P1S_VerifyRecoverySet"));

    R_RESULT    Return = SRDERR_ERROR;
    PDRIMINFO   pDrim = (PDRIMINFO)pDrimInfo;
    DRM_SET_HANDLE rs = NULL;
    DRM_INFO_HANDLE ri = NULL;

    SRD_ENTER_FCN;

    _TRY_
        LONG    resDrim = DRM_ERR_OK;
        R_TCHAR pszImg[32] = { 0 };
        R_TCHAR pszP1s[32] = { 0 };

        if(
            NULL == pDrim ||
            NULL == cmpResult ||
            NULL == pDrim->pDrmHandle ||
            NULL == pDrim->pDrmInfoHandle
        )
            _LEAVE_;

        resDrim = P1S_RsOpen(pDrim, pDrim->pszImageFile, &rs);
        if (resDrim != SRDERR_SUCCESS)
        {
            DbgPlain(81, _T("P1S_RsOpen() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }

        P1S_RiGetUserData(pDrim, pDrim->pDrmInfoHandle, _T("Signature"), pszP1s);

        resDrim = P1S_DRMExtractRecoveryInfoInternal(pDrim, rs, &ri);
        if (resDrim != DRM_ERR_OK)
        {
            DbgPlain(81, _T("P1S_DRMExtractRecoveryInfoInternal() FAILED [error: %d]."), resDrim);
            _LEAVE_;
        }

        P1S_RiGetUserData(pDrim, ri, _T("Signature"), pszImg);

        DbgPlain(81, _T("Image signature:  %s."), pszImg);
        DbgPlain(81, _T("Ini signature:  %s."), pszP1s);

        if(
            pszImg[0] == _T('\0') ||
            pszP1s[0] == _T('\0')
        )
            *cmpResult = DRIMVERIFY_INCOMPLETE;
        else if(strcmp(pszImg, pszP1s) != 0)
            *cmpResult = DRIMVERIFY_DIFFERENT;
        else
            *cmpResult = DRIMVERIFY_SAME;

        if(imgSig != NULL)
            strcpy(imgSig, pszImg);
        if(p1sSig != NULL)
            strcpy(p1sSig, pszP1s);

        Return = SRDERR_SUCCESS;
    _FINALLY_
        if(ri)
            P1S_RiClose(pDrim, ri);
        if(rs)
            P1S_RsClose(pDrim, rs);
    _ENDTRY_

    SRD_RETURN_VAL(Return);
}

SYSNFOP1S_API R_RESULT
P1S_RiOpenVolumeMapping(R_HANDLE drim, DRM_MAP_HANDLE *rm)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_open_volume_mapping(di->pDrmHandle,
        di->pDrmInfoHandle, rm) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}


SYSNFOP1S_API R_RESULT
P1S_RmGetCount(R_HANDLE drim, int *count)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rm_get_count(di->pDrmMapHandle, count) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RmGetMapping(R_HANDLE drim,
                 int index,
                 R_TCHAR *origVolOut,
                 R_TCHAR *newVolOut,
                 drm_volume_status_t *outFormatStatus,
                 drm_volume_status_t *outChkDskStatus)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *origVolValue, *newVolValue = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rm_get_mapping(di->pDrmMapHandle, index, &origVolValue,
        &newVolValue, outFormatStatus, outChkDskStatus) == DRM_ERR_OK)
    {
        strcpy(origVolOut, W2T((wchar_t *)origVolValue));
        strcpy(newVolOut, W2T((wchar_t *)newVolValue));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RmClose(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rm_close(di->pDrmMapHandle) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiOpenVolumeList(R_HANDLE drim, DRM_VOL_HANDLE *rvl)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_open_volume_list(di->pDrmHandle, di->pDrmInfoHandle,
        rvl) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiSetToolkitDir(R_HANDLE drim, R_TCHAR* toolkitDir)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_set_toolkit_directory(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(toolkitDir)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiAddDriver(R_HANDLE drim, R_TCHAR* infPath)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_add_driver(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(infPath)) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiClearDriver(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_ri_clear_driver(di->pDrmHandle, di->pDrmInfoHandle) == DRM_ERR_OK)
    {
        Return = SRDERR_SUCCESS;
    }

    return Return;
}

/* Set recovery password to be used during the encryption process. */
SYSNFOP1S_API R_RESULT
P1S_RiSetRecoveryPassword(R_HANDLE drim, const R_TCHAR* rPassword)
{
    R_RESULT result = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if (dll->fn_drm_ri_set_recovery_password(di->pDrmHandle, di->pDrmInfoHandle,
        T2W(rPassword)) == DRM_ERR_OK)
    {
        result = SRDERR_SUCCESS;
    }

    return(result);
}

SYSNFOP1S_API R_RESULT
P1S_RvlGetVolumeInfo(R_HANDLE drim, int index, R_TCHAR *outValue, R_BOOL *shared)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    drm_bool_t shrd = 0;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_rvl_get_volume_info(di->pDrmVolHandle,index, &value, &shrd) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));
        *shared = shrd;

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RvlGetCount(R_HANDLE drim, int *count)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rvl_get_count(di->pDrmVolHandle, count) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RvlClose(R_HANDLE drim)
{
    R_RESULT Return = SRDERR_ERROR;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;

    if(dll->fn_drm_rvl_close(di->pDrmVolHandle) == DRM_ERR_OK)
        Return = SRDERR_SUCCESS;

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetBootDrive(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_boot_drive(di->pDrmHandle, di->pDrmInfoHandle,
        &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}

SYSNFOP1S_API R_RESULT
P1S_RiGetBaseDirectory(R_HANDLE drim, R_TCHAR *outValue)
{
    R_RESULT Return = SRDERR_ERROR;
    const wchar_t *value = NULL;
    PDRIMINFO di = (PDRIMINFO)drim;
    PDRIMDLL dll = di->pDrimDll;
    USES_CONVERSION_W;

    if(dll->fn_drm_ri_get_base_directory(di->pDrmHandle, di->pDrmInfoHandle,
        &value) == DRM_ERR_OK)
    {
        strcpy(outValue, W2T((wchar_t *)value));

        Return = SRDERR_SUCCESS;
    }

    return Return;
}
