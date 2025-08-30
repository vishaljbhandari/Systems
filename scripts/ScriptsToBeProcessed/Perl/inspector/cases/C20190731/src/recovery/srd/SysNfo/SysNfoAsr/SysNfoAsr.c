/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   sysnfowxp
* @file      recovery/srd/SysNfo/SysNfoAsr/SysNfoAsr.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     /
*
* @since     August 2001	lukaj	Original Coding
*
* @remarks   /
*/
#include "lib\cmn\target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /recovery/srd/SysNfo/SysNfoAsr/SysNfoAsr.c $Rev$ $Date::                      $:");
#endif

/* ---------------------------------------------------------------------------
|	include files
 ---------------------------------------------------------------------------*/

#include "lib\cmn\common.h"

#include "../SysNfo.h"
#include "../../srdfree.h"
#include "../../srd.h"

/* --------------------------------------------------------------------
|	SYSSETUP.DLL function typedefs and defines
----------------------------------------------------------------------*/
#define ASR_SIF_FILE                _T("asr.sif")
#define ASRPNP_SIF_FILE             _T("asrpnp.sif")
#define CONFIG_NT_FILE              _T("config.nt")
#define AUTOEXEC_NT_FILE            _T("autoexec.nt")
#define SETUP_LOG_FILE              _T("setup.log")
#define OB_SETUP_SEQ_NUMBER         4000
#define OB_SETUP_NAME               _T("drstart.exe")
#define OB_SETUP_INI_NAME           _T("omnicab.ini")
#define OB_SRD_NAME                 _T("recovery.srd")
#define OB_SCSITAB_NAME             _T("scsitab")
#define OB_SCSITAB_DEST_NAME        _T("scsitab.dev.cfg")
#define OB_VENDOR_NAME              _T("HP OmniBack II 5.10")
#define SYSSETUP_DLLNAME            _T("syssetup.dll")

typedef BOOL (_stdcall *AsrCreateStateFileProcA)(PCSTR, PCSTR, CONST BOOL, PCSTR, DWORD_PTR*);
typedef BOOL (_stdcall *AsrCreateStateFileProcW)(PCWSTR, PCWSTR, CONST BOOL, PCWSTR, DWORD_PTR*);
typedef BOOL (_stdcall *AsrAddSifEntryProcA)(DWORD_PTR, PCSTR, PCSTR);
typedef BOOL (_stdcall *AsrAddSifEntryProcW)(DWORD_PTR, PCWSTR, PCWSTR);
typedef BOOL (_stdcall *AsrFreeContextProc)(DWORD_PTR*);

#if defined(UNICODE) || defined(_UNICODE)
    #define AsrCreateStateFileProc          AsrCreateStateFileProcW
    #define AsrCreateStateFileName          "AsrCreateStateFileW"
    #define AsrAddSifEntryProc              AsrAddSifEntryProcW
    #define AsrAddSifEntryName              "AsrAddSifEntryW"
#else
    #define AsrCreateStateFileProc          AsrCreateStateFileProcA
    #define AsrCreateStateFileName          "AsrCreateStateFileA"
    #define AsrAddSifEntryProc              AsrAddSifEntryProcA
    #define AsrAddSifEntryName              "AsrAddSifEntryA"
#endif

#define AsrFreeContextName                  "AsrFreeContext"

#define ASR_INSTALLFILES_SECTION_NAME                       _T("[INSTALLFILES]")
#define ASR_COMMANDS_SECTION_NAME                           _T("[COMMANDS]")
#define ASR_SIF_INSTALLFILES_MEDIA_PROMPT_ALWAYS            0x00000001
#define ASR_SIF_INSTALLFILES_MEDIA_PROMPT_ON_ERROR          0x00000002
#define ASR_SIF_INSTALLFILES_REQUIRED_FILE                  0x00000006
#define ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS       0x00000010
#define ASR_SIF_INSTALLFILES_PROMPT_IF_FILE_EXISTS          0x00000020

#define BUFFER_MOUNTPOINT_LENGTH                            64 /* parasoft-suppress  CODSTA-37 "C define issue" */

typedef struct _tagSysSetupModule
{
    HMODULE                 hSysSetupMod;
    AsrCreateStateFileProc  fnCreate;
    AsrFreeContextProc      fnFree;
    AsrAddSifEntryProc      fnAdd;
} SYSSETUPMODULE, *PSYSSETUPMODULE;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
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

SYSNFO_API R_RESULT
InitSysNfo(tchar *pPostFix, tchar *pRanges, tchar *pSelect)
{
	R_RESULT Return = SRDERR_ERROR;

	if (CmnInit (
			_T("SysNfoAsr.dll"), _T("SysNfoAsr"),
			pPostFix, pRanges, pSelect, 0) == 0
		)
	{
		Return = SRDERR_SUCCESS;
	}

	return Return;
}

SYSNFO_API R_VOID
FreeSysNfo(R_VOID)
{
	CmnExit();
}

static R_RESULT
SetProcessPrivilege(R_TCHAR *pszPrivilege, R_BOOL bOn)
{
	ERH_FUNCTION (_T("SetProcessPrivilege"));

	R_RESULT            rRet = SRDERR_ERROR;
	HANDLE				hToken = NULL;
    TOKEN_PRIVILEGES	tpRec = { 0 };

	tpRec.PrivilegeCount = 1;
	tpRec.Privileges[0].Attributes = bOn ? SE_PRIVILEGE_ENABLED : 0;

	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		DbgStamp (DBG_SYSNFOASR);
		DbgPlain (DBG_SYSNFOASR, _T("Cannot open process token [%lu] !\n"), GetLastError());
	}
	else if (!LookupPrivilegeValue (NULL, pszPrivilege, &tpRec.Privileges[0].Luid))
	{
		DbgStamp (DBG_SYSNFOASR);
		DbgPlain (DBG_SYSNFOASR, _T("Cannot lookup privilege value [%lu] !\n"), GetLastError ());
	}
	else if (!AdjustTokenPrivileges (hToken, FALSE, &tpRec, 0, (PTOKEN_PRIVILEGES) NULL, 0))
	{
		DbgStamp (DBG_SYSNFOASR);
		DbgPlain (DBG_SYSNFOASR, _T("Cannot adjust token privilege [%lu]!\n%s"), GetLastError ());
	}
	else if (!CloseHandle (hToken))
	{
		DbgStamp (DBG_SYSNFOASR);
		DbgPlain (DBG_SYSNFOASR, _T("Cannot close token handle [%lu]!\n"), GetLastError ());
	}
	else
	{
		rRet = SRDERR_SUCCESS;
	}

	return rRet;
}

static R_RESULT
ValidateSrdData(PSRDDATA pSrdData)
{
    R_RESULT Return = SRDERR_ERROR;

    __try
    {
        if(pSrdData == NULL)
            __leave;
        if(
            IsBadReadPtr(pSrdData, sizeof(SRDDATA)) ||
            IsBadWritePtr(pSrdData, sizeof(SRDDATA))
        )
            __leave;
        if(pSrdData->Header == NULL)
            __leave;
        if(
            IsBadReadPtr(pSrdData->Header, sizeof(SRDHEADER)) ||
            IsBadWritePtr(pSrdData->Header, sizeof(SRDHEADER))
        )
            __leave;

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
    }

    return Return;
}

static R_VOID
FreeSysSetupModule(PSYSSETUPMODULE pIn)
{
    if(pIn != NULL)
    {
        if(pIn->hSysSetupMod != NULL)
            FreeLibrary(pIn->hSysSetupMod);

        memset(pIn, 0, sizeof(SYSSETUPMODULE));
    }
}

static R_RESULT
LoadSysSetupModule(PSYSSETUPMODULE pOut)
{
    ERH_FUNCTION(_T("LoadSysSetupModule"));

    R_RESULT rRet = SRDERR_ERROR;

    if(pOut != NULL)
    {
        TCHAR path[STRLEN_1K + 1] = _T("");
        TCHAR dllPath[STRLEN_1K + 1] = _T("");

        GetSystemDirectory(path, STRLEN_1K);
        CreatePathFromParams(dllPath, _T("%s\\%s"), path, _T("syssetup.dll"));
        pOut->hSysSetupMod = DPWrapperLoadDLL(dllPath, 0, 0);
        if(pOut->hSysSetupMod != NULL)
        {
            pOut->fnCreate = (AsrCreateStateFileProc)GetProcAddress(pOut->hSysSetupMod, AsrCreateStateFileName);
            pOut->fnFree = (AsrFreeContextProc)GetProcAddress(pOut->hSysSetupMod, AsrFreeContextName);
            pOut->fnAdd = (AsrAddSifEntryProc)GetProcAddress(pOut->hSysSetupMod, AsrAddSifEntryName);

            if(
                pOut->fnCreate != NULL && 
                pOut->fnFree != NULL &&
                pOut->fnAdd != NULL
            )
            {
                rRet = SRDERR_SUCCESS;
            }
            else
            {
                FreeSysSetupModule(pOut);

                DbgStamp(DBG_SYSNFOASR);
                DbgPlain(DBG_SYSNFOASR, _T("[%s] FAILURE - GetProcAddress() on one or more of system setup API library functions."), __FCN__);
            }
        }
        else
        {
            DbgStamp(DBG_SYSNFOASR);
            DbgPlain(DBG_SYSNFOASR, _T("[%s] FAILURE - LoadLibrary() for module system setup API library."), __FCN__);
        }
    }

    return rRet;
}

static R_RESULT
VolumesToGUIDs(PSRDDATA pSrdData, R_TCHAR **ppOut)
{
    R_RESULT    Return = SRDERR_ERROR;
    R_TCHAR     *pGUIDs = NULL;
    R_ULONG     ulSizeInChars = 0;

    __try
    {
        R_ULONG ii = 0;

        for(ii = 0; ii < pSrdData->RestoreVolumes->RestVolCount; ii++)
        {
            R_ULONG     ulLen = 0;
            R_TCHAR     pszVolMtPoint[MAX_PATH];
            R_TCHAR     pszVolGUID[64];
            PRESTVOL    pVol = pSrdData->RestoreVolumes->RestoreVolumes[ii];

            /* ---------------------------------------------------------------------------
            |   skip hidden boot volume (windows 7/windows 2008 r2)
             ---------------------------------------------------------------------------*/
/** \TODO
 * Check this solution
 */
            if (StrCmp(pVol->VolumeName, _T("@")) == 0)
            {
                DbgPlain(DBG_SYSNFOASR, _T("Skipping hidden boot volume %s."), pVol->VolumeName);
                continue;
            }

            /* ---------------------------------------------------------------------------
            |   skip volumes that are not directly linked to volume drive letter mappings.
             ---------------------------------------------------------------------------*/
            if(
                (pVol->VolumePurpose & ~CFG_VOLUME) == 0 ||
                (pVol->VolumePurpose & ~OB2DB_FILES) == 0
            )
                continue;

            /* ---------------------------------------------------------------------------
            |	skip volumes that are on  a cluster shared storage.
             ---------------------------------------------------------------------------*/
            if(
                (pVol->VolumePurpose & QUORUM_VOLUME) != 0 ||
                (pVol->VolumePurpose & OB2DB_CONFIG) != 0
            )
            {
                DbgPlain(DBG_SYSNFOASR, _T("Skipping cluster volume %s."), pVol->VolumeName);
                continue;
            }

            if (pVol->VolumePurpose == DATA_VOLUME)
            {
                DbgPlain(DBG_SYSNFOASR, _T("Skipping data volume %s."), pVol->VolumeName);
                continue;
            }

            /* ---------------------------------------------------------------------------
            |	generate appropriate volume name (backslash format, without trailing backslash).
             ---------------------------------------------------------------------------*/
            if (StrStr(pVol->VolumeName, _T("Volume{")) != NULL)
            {
                strcpy(pszVolGUID, pVol->VolumeName);
            }
            else
            {
                sprintf(pszVolMtPoint, _T("%s"), pVol->VolumeName + 1);
                PathToBackslashes(pszVolMtPoint);
                PathCutTrailSlashes(pszVolMtPoint, 0);

                /* ---------------------------------------------------------------------------
                |   if volume mountpoint add only backslash, otherwise append colon first.
                 ---------------------------------------------------------------------------*/
                if (strlen(pszVolMtPoint) <= 1)
                    strcat(pszVolMtPoint, _T(":"));
                strcat(pszVolMtPoint, _T("\\"));

                /* ---------------------------------------------------------------------------
                |   win32 API call to obtain GUID version of the mountpoints name.
                 ---------------------------------------------------------------------------*/
                if (GetVolumeNameForVolumeMountPoint(pszVolMtPoint, pszVolGUID, BUFFER_MOUNTPOINT_LENGTH) == FALSE)
                    SRD_W32_LEAVE(
                        {
                            DbgPlain(DBG_SYSNFOASR, _T("GetVolumeNameForVolumeMountPoint(%s,,) failed [%lu]."), 
                                pszVolMtPoint, GetLastError());
                        }
                    );
            }
            /* ---------------------------------------------------------------------------
            |	important to remove the trailing backslash that is appended to pszVolGUID
            |   by GetVolumeNameForVolumeMountPoint() API. AsrCreateStateFile() is not
            |   happy to see such a backslash at the end.
             ---------------------------------------------------------------------------*/
            PathCutTrailSlashes(pszVolGUID, 0);
            /* ---------------------------------------------------------------------------
            |	allocation of double NULL terminated multi-sz string of volume GUIDs.
             ---------------------------------------------------------------------------*/
            if(pGUIDs == NULL)
            {
                ulLen = (R_ULONG)(strlen(pszVolGUID) + 2) * sizeof(R_TCHAR);

                pGUIDs = MALLOC(ulLen);
                if(pGUIDs == NULL)
                    __leave;
                strcpy(pGUIDs, pszVolGUID);
                ulSizeInChars = ulLen / sizeof(R_TCHAR);
            }
            else
            {
                ulLen = ulSizeInChars * sizeof(R_TCHAR) + (R_ULONG)(strlen(pszVolGUID) + 1) * sizeof(R_TCHAR);
                pGUIDs = REALLOC(pGUIDs, ulLen);
                if(pGUIDs == NULL)
                    __leave;
                strcat(&pGUIDs[ulSizeInChars - 1], pszVolGUID);
                ulSizeInChars = (ulLen / sizeof(R_TCHAR));
            }
            /* ---------------------------------------------------------------------------
            |	always double null terminate the string.
             ---------------------------------------------------------------------------*/
            pGUIDs[ulSizeInChars - 1] = _T('\0');
        }

        *ppOut = pGUIDs;

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(
            Return != SRDERR_SUCCESS &&
            pGUIDs != NULL
        )
            FREE(pGUIDs);
    }

    return Return;
}

static R_RESULT
AddAsrFile(PASRDATA *ppInOut, R_ULONG ulType)
{
	ERH_FUNCTION (_T("AddAsrFile"));

    R_RESULT    Return = SRDERR_ERROR;
    R_TCHAR     pszFilePath[MAX_PATH];
    R_TCHAR     pszWinDir[MAX_PATH];
    R_UCHAR     *pFileBuf = NULL;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    PASRDATA    pAsrData = NULL;

    SRD_ENTER_FCN;

    __try
    {
        R_ULONG ulBytes = 0;
        R_ULONG ulFileSize = 0;

        if(ppInOut == NULL || *ppInOut == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - invalid parameters.")); });

        pAsrData = *ppInOut;

        GetWindowsDirectory(pszWinDir, MAX_PATH);

        switch(ulType)
        {
            case ASRFILE_ASR_SIF:
                sprintf(pszFilePath, _T("%sconfig\\")ASR_SIF_FILE, cmnPanTmp); break;
            case ASRFILE_ASRPNP_SIF:
                sprintf(pszFilePath, _T("%sconfig\\")ASRPNP_SIF_FILE, cmnPanTmp); break;
            case ASRFILE_CONFIG_NT:
                sprintf(pszFilePath, _T("%s\\repair\\")CONFIG_NT_FILE, pszWinDir); break;
            case ASRFILE_AUTOEXEC_NT:
                sprintf(pszFilePath, _T("%s\\repair\\")AUTOEXEC_NT_FILE, pszWinDir); break;
            case ASRFILE_SETUP_LOG:
                sprintf(pszFilePath, _T("%s\\repair\\")SETUP_LOG_FILE, pszWinDir); break;
            default:
                SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - unknown file type -> %d."), ulType); });
                break;
        }

        hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - file open -> %s."), pszFilePath); });

        ulFileSize = GetFileSize(hFile, NULL);
        if(ulFileSize == INVALID_FILE_SIZE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - file size.")); });

        pFileBuf = MALLOC(ulFileSize);
        if(pFileBuf == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - memory allocation.")); });

        if(ReadFile(hFile, pFileBuf, ulFileSize, &ulBytes, NULL) != TRUE || ulBytes != ulFileSize)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - cannot read from file -> %s."), pszFilePath); });

        if(pAsrData->Count > 0)
        {
            R_ULONG ulSize = sizeof(ASRDATA) + pAsrData->Count * sizeof(PASRFILEBUF);

            pAsrData = REALLOC(pAsrData, ulSize);
            if(pAsrData == NULL)
                SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - memory allocation 2.")); });
            pAsrData->AsrFiles[pAsrData->Count] = NULL;
        }

        pAsrData->AsrFiles[pAsrData->Count] = MALLOC(sizeof(ASRFILEBUF));
        if(pAsrData->AsrFiles[pAsrData->Count] == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILURE - file buffer allocation.")); });

        pAsrData->AsrFiles[pAsrData->Count]->Size = ulFileSize;
        pAsrData->AsrFiles[pAsrData->Count]->Data = pFileBuf;
        pAsrData->AsrFiles[pAsrData->Count]->Type = ulType;

        pAsrData->Count += 1;

        *ppInOut = pAsrData;

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(Return != SRDERR_SUCCESS)
        {
            if(pFileBuf != NULL)
                FREE(pFileBuf);
        }
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
    }

    SRD_RETURN_VAL(Return);
}

static R_RESULT
AddInstallSection(PSYSSETUPMODULE pSysSetup, DWORD_PTR dwContext)
{
	ERH_FUNCTION (_T("AddInstallSection"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

    __try
    {
        R_TCHAR     pszString[MAX_PATH * 2];

        _stprintf(
            pszString,
            _T("1,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",0x%08x"),
            ASR_DISK_LABEL,
            _T("%FLOPPY%"),
            OB_SETUP_NAME,
            _T("%TEMP%\\")OB_SETUP_NAME,
            GetBrandString(BC_FULL_PRODUCT_NAME_VER),
            ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS | ASR_SIF_INSTALLFILES_REQUIRED_FILE
        );
        if(pSysSetup->fnAdd(dwContext, ASR_INSTALLFILES_SECTION_NAME, pszString) == FALSE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AsrAddSifEntry() for ")
                _T("[INSTALLFILES] section FAILED -> %s [%d]."), pszString, GetLastError()); });

        _stprintf(
            pszString,
            _T("1,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",0x%08x"),
            ASR_DISK_LABEL,
            _T("%FLOPPY%"),
            OB_SETUP_INI_NAME,
            _T("%TEMP%\\")OB_SETUP_INI_NAME,
            GetBrandString(BC_FULL_PRODUCT_NAME_VER),
            ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS | ASR_SIF_INSTALLFILES_REQUIRED_FILE
        );
        if(pSysSetup->fnAdd(dwContext, ASR_INSTALLFILES_SECTION_NAME, pszString) == FALSE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AsrAddSifEntry() for ")
                _T("[INSTALLFILES] section FAILED -> %s [%d]."), pszString, GetLastError()); });

        _stprintf(
            pszString,
            _T("1,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",0x%08x"),
            ASR_DISK_LABEL,
            _T("%FLOPPY%"),
            OB_SRD_NAME,
            _T("%TEMP%\\")OB_SRD_NAME,
            GetBrandString(BC_FULL_PRODUCT_NAME_VER),
            ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS
        );
        if(pSysSetup->fnAdd(dwContext, ASR_INSTALLFILES_SECTION_NAME, pszString) == FALSE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AsrAddSifEntry() for ")
                _T("[INSTALLFILES] section FAILED -> %s [%d]."), pszString, GetLastError()); });

        _stprintf(
            pszString,
            _T("1,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",0x%08x"),
            ASR_DISK_LABEL,
            _T("%FLOPPY%"),
            OB_SCSITAB_NAME,
            _T("%SYSTEMROOT%\\TEMP\\")OB_SCSITAB_DEST_NAME,
            GetBrandString(BC_FULL_PRODUCT_NAME_VER),
            ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS
        );
        if(pSysSetup->fnAdd(dwContext, ASR_INSTALLFILES_SECTION_NAME, pszString) == FALSE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AsrAddSifEntry() for ")
                _T("[INSTALLFILES] section FAILED -> %s [%d]."), pszString, GetLastError()); });

        Return = SRDERR_SUCCESS;
    }
    __finally {}

    SRD_RETURN_VAL(Return);
}

static R_RESULT
AddCommandSection(PSYSSETUPMODULE pSysSetup, DWORD_PTR dwContext)
{
	ERH_FUNCTION (_T("AddCommandSection"));

    R_RESULT    Return = SRDERR_ERROR;

    SRD_ENTER_FCN;

    __try
    {
        R_TCHAR     pszString[MAX_PATH * 2];
    
        _stprintf(
            pszString,
            _T("1,%u,0,\"%s\",\"\""),
            OB_SETUP_SEQ_NUMBER,
            _T("%TEMP%\\")OB_SETUP_NAME
        );
        if(pSysSetup->fnAdd(dwContext, ASR_COMMANDS_SECTION_NAME, pszString) == FALSE)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AsrAddSifEntry() for [COMMANDS] section FAILED -> %s [%08x]."), pszString, GetLastError()); });

        Return = SRDERR_SUCCESS;
    }
    __finally {}

    SRD_RETURN_VAL(Return);
}

static R_RESULT
CollectAsrInfo(R_VOID *pAsrInfo)
{
	ERH_FUNCTION (_T("CollectAsrInfo"));

    R_RESULT    Return = SRDERR_ERROR;
    PSRDDATA    pSrdData = NULL;
    R_TCHAR     *pGUIDs = NULL;
    DWORD_PTR   dwContext = 0;
    SYSSETUPMODULE SysSetup = { 0 };
    R_TCHAR     pszTmpDir[MAX_PATH];
    PASRDATA    pAsrData = NULL;

    SRD_ENTER_FCN;

    __try
    {
        /* ---------------------------------------------------------------------------
        |	there must be a valid pointer in which to return the information.
         ---------------------------------------------------------------------------*/
        if(pAsrInfo == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Output pointer is NULL.")); });
        /* ---------------------------------------------------------------------------
        |	validate the SRDDATA pointer that can be passed as an input parameter.
         ---------------------------------------------------------------------------*/
        pSrdData = *(PSRDDATA*)pAsrInfo;
        if(
            pSrdData == NULL ||
            ValidateSrdData(pSrdData) != SRDERR_SUCCESS
        )
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Invalid SRD data.")); });
        /* ---------------------------------------------------------------------------
        |	verify if everything went well.
         ---------------------------------------------------------------------------*/
        if(
            pSrdData == NULL ||
            pSrdData->RestoreVolumes == NULL ||
            pSrdData->RestoreVolumes->RestVolCount == 0
        )
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Wrong RestoreVolumes.")); });
        /* ---------------------------------------------------------------------------
        |	based on the input/collected information, collect the critical volume
        |   GUIDs to be passed to the AsrCreateStateFile() function.
         ---------------------------------------------------------------------------*/
        if(VolumesToGUIDs(pSrdData, &pGUIDs) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("VolumesToGUIDs FAILED.")); });
        /* ---------------------------------------------------------------------------
        |	dynamic loading of the system module that provides ASR backup capabilities.
         ---------------------------------------------------------------------------*/
        if(LoadSysSetupModule(&SysSetup) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Couldn't load system setup API library.")); });
        /* ---------------------------------------------------------------------------
        |	backup privilege must be set otherwise AsrCreateStateFile() complains.
         ---------------------------------------------------------------------------*/
        SetProcessPrivilege(SE_BACKUP_NAME, TRUE);
        /* ---------------------------------------------------------------------------
        |	manufacture sif files using XP AsrCreateStateFile() API.
         ---------------------------------------------------------------------------*/
        sprintf(pszTmpDir, _T("%sconfig\\")ASR_SIF_FILE, cmnPanTmp);
        if(SysSetup.fnCreate(pszTmpDir, GetBrandString(BC_FULL_PRODUCT_NAME_VER), FALSE, pGUIDs, &dwContext) == FALSE)
            SRD_W32_LEAVE(
                {
                    DWORD   dwError = GetLastError();

                    if(dwError == ERROR_CALL_NOT_IMPLEMENTED)
                        Return = SRDERR_NOT_SUPPORTED;
                    DbgPlain(DBG_SYSNFOASR, _T("AsrCreateStateFile() FAILED [%d]."), dwError);
                 }
            );
        /* ---------------------------------------------------------------------------
        |	Add [COMMANDS] section.
         ---------------------------------------------------------------------------*/
        if(AddCommandSection(&SysSetup, dwContext) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AddCommandSection FAILED.")); });
        /* ---------------------------------------------------------------------------
        |	Add [INSTALLFILES] section.
         ---------------------------------------------------------------------------*/
        if(AddInstallSection(&SysSetup, dwContext) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("AddInstallSection() FAILED.")); });

        SysSetup.fnFree(&dwContext);
        dwContext = 0;

        /* ---------------------------------------------------------------------------
        |	allocation of the ASRDATA structure.
         ---------------------------------------------------------------------------*/
        pAsrData = MALLOC(sizeof(ASRDATA));
        if(pAsrData == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Couldn't allocate ASRDATA structure.")); });
        memset(pAsrData, 0, sizeof(ASRDATA));
        /* ---------------------------------------------------------------------------
        |	allocation of the corresponding header structure.
         ---------------------------------------------------------------------------*/
        pAsrData->Header = MALLOC(sizeof(ASRHEADER));
        if(pAsrData->Header == NULL)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("Couldn't allocate ASRDATA header structure.")); });
        memset(pAsrData->Header, 0, sizeof(ASRHEADER));
        /* ---------------------------------------------------------------------------
        |	the initialization of the header structure.
         ---------------------------------------------------------------------------*/
        pAsrData->Header->Magic = ASR_MAGIC;
        pAsrData->Header->Platform = pSrdData->Header->Platform;
        pAsrData->Header->Version = pSrdData->Header->Version;

        /* ---------------------------------------------------------------------------
        |	the addition of the 5 key files to the ASRDATA buffer.
         ---------------------------------------------------------------------------*/
        if(AddAsrFile(&pAsrData, ASRFILE_ASR_SIF) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILED to add asr.sif to archive.")); });
        if(AddAsrFile(&pAsrData, ASRFILE_ASRPNP_SIF) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILED to add asrpnp.sif to archive.")); });
#if !TARGET_WIN64
        if(AddAsrFile(&pAsrData, ASRFILE_CONFIG_NT) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILED to add config.nt to archive.")); });
        if(AddAsrFile(&pAsrData, ASRFILE_AUTOEXEC_NT) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILED to add autoexec.nt to archive.")); });
#endif
        if(AddAsrFile(&pAsrData, ASRFILE_SETUP_LOG) != SRDERR_SUCCESS)
            SRD_W32_LEAVE( { DbgPlain(DBG_SYSNFOASR, _T("FAILED to add setup.log to archive.")); });

        *(PASRDATA*)pAsrInfo = pAsrData;

        Return = SRDERR_SUCCESS;
    }
    __finally
    {
        if(
            Return != SRDERR_SUCCESS &&
            pAsrData != NULL
        )
        {
            /* ---------------------------------------------------------------------------
            |	only free on error.
             ---------------------------------------------------------------------------*/
            FreeAsrData(pAsrData);
            FREE(pAsrData);
        }
        /* ---------------------------------------------------------------------------
        |	always free.
         ---------------------------------------------------------------------------*/
        if(pGUIDs != NULL)
            FREE(pGUIDs);
    
        if(dwContext != 0)
            SysSetup.fnFree(&dwContext);

        FreeSysSetupModule(&SysSetup);
    }

    SRD_RETURN_VAL(Return);
}

SYSNFO_API R_RESULT
CollectSystemInformation(R_VOID* pSrdData, R_ULONG ulType)
{
    R_RESULT    Return = SRDERR_ERROR;

    switch(ulType)
    {
        case SRDHT_ASRINFO:
            Return = CollectAsrInfo(pSrdData); break;
        default:
            DbgPlain(DBG_SYSNFOASR, _T("FAILURE - unsupported type [%lu]."), ulType);
            Return = SRDERR_NOT_SUPPORTED;

            break;
    }

    return Return;
}
