/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   BARGUI - Single Mailbox portion
* @file      lib/bargui/mbx.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @since     02.09.02 Klemen Care        Initial Coding
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/bargui/mbx.c $Rev$ $Date::                      $:");
#endif


#include <stdio.h>

#include "lib/cmn/common.h"
#include "cs/csa/csa.h"
#include "bargui.h"
#include "barguicommon.h"
#include "mbx.h"
#include "msese.h"
#include "integ/exchange/Mailbox/mbx_util.h"

static void MBX_FOLDERS_FREE (MBX_FOLDERS_STRUCT *dir)
{
    FREE (dir->name);
    FREE (dir->path);
}


static tchar *MBX_DecodePassword(tchar *encoded)
{
    size_t len;
    tchar *plain;
    
    if (!encoded)
        return NULL;

    len = StrLen(encoded);
    plain = CALLOC (len+1, sizeof(tchar));

    CmnUnScrambleBytes (encoded, plain, len*sizeof(tchar));
    return plain;
}


static int MBX_UnpackConfig (BP_Opt file, IntegConfig *config)
{
    ERH_FUNCTION (_T("MBX_UnpackConfig"));
    MBXCONFIG *c = &config->MBXConfig;
    tchar *password;

    DbgFcnIn();

    password = BP_GetStrCopy (&file, _T("Password"));
    c->admin  = BP_GetStrCopy (&file, _T("Login"));
    c->domain = BP_GetStrCopy (&file, _T("Domain"));

    if ((StrIsEmpty(c->admin) && StrIsEmpty(c->domain) && StrIsEmpty(password)) ||
        (StrIsEmpty(password)))
        RETURN_INT (ERH_OBE_CONFIG_READ);

    c->password = MBX_DecodePassword (password);
    FREE(password);
    RETURN_INT (OBE_OK);
}

static tchar *MBX_UnpackMount (IN tchar *mountPoint, OUT IntegMountPoint *mp)
{ 
    tchar *s, *l = NULL;

    memset (&mp->mbx, 0, sizeof(mp->mbx));
    /* mountPoint = /Single Mailbox/dbName/streamId */
    if (StrIsEmpty(mountPoint)) 
        return NULL;

    /* 1st must be '/' */
    if ( mountPoint[0] != _T('/') )
        return NULL;

    /* get the 2nd '/' */
    s = strchr(mountPoint + 1, _T('/'));

    /* get the last '/' */
    l = strrchr(mountPoint, _T('/'));

    /* 2nd must not be last */
    if (s == l)
        return NULL;

    strncpy(mp->mbx.appName, mountPoint+1, s-mountPoint-1);
    mp->mbx.appName[s-mountPoint-1] = '\0';

    strncpy(mp->mbx.dbName, s+1, l-s-1);
    mp->mbx.dbName[l-s-1] = '\0';

    sscanf(l+1, _T("%d"), &mp->mbx.streamId);

    DbgPlain(10, _T("Exchange Object: %s, %s, %d\n"), mp->mbx.appName, mp->mbx.dbName, mp->mbx.streamId);

    mp->mbx.type = StrCmp(mp->mbx.dbName, MBX_PUBLIC_OBJ) ? MBX_TYPE_PRIVAT : MBX_TYPE_PUBLIC;

    return StrIsEmpty(mp->mbx.appName) ? NULL : mp->mbx.appName;
}

/*========================================================================*//**
*
* @brief     Calls mbx_bar for checking the configuration
*
*//*=========================================================================*/
static int MBX_CheckConfig(GUI *context, Integ *integ)
{
    ERH_FUNCTION(_T("CheckConfigMBX"));
    int   retval=OBE_OK;
    tchar szAppSrv[STRLEN_ITEMNAME+1] = _T("");
    tchar hostname[STRLEN_ITEMNAME+1] = _T("");

    DbgFcnIn();

    if (integ->options.MBXGlobal.restore.restoreTo)
    {
        /* restore to another host */
        sprintf(szAppSrv, _T("%s%s"), BG_MBX_APPSRV, integ->options.MBXGlobal.restore.restoreTo);
        strcpy(hostname, integ->options.MBXGlobal.restore.restoreTo);
    }
    else
    {
        /* creating backup specification */
        sprintf(szAppSrv, _T("%s%s"), BG_MBX_APPSRV, integ->hostname);
        strcpy(hostname, integ->hostname);
    }

    retval = UtilExecute (context->dbsmhandle, NULL, hostname,
        MBX_AGENT, MBX_CHKCONF, szAppSrv, NULL);

    RETURN_INT(retval);
}

/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Calling mbx_bar for configuring the integration
*
*//*=========================================================================*/
static int MBX_Config (
    IN GUI         *context,
    IN Integ       *integ,
    IN IntegConfig *config )
{
    ERH_FUNCTION(_T("ConfigMBX"));
    int   retval = ERH_OBE_NORETVAL;
    tchar szAppSrv[STRLEN_ITEMNAME+1] = _T("");
    tchar hostname[STRLEN_ITEMNAME+1] = _T("");

    tchar *encodedPwd = NULL;

    DbgFcnIn();

    encodedPwd = EncodePassword(config->MBXConfig.password);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Encoded password=%s"), __FCN__, encodedPwd);

    if (integ->options.MBXGlobal.restore.restoreTo)
    {
        /* restore to another host */
        /* CI by Slava: Why do we need it in backup context during the configuration?*/
        sprintf(szAppSrv, _T("%s%s"), BG_MBX_APPSRV, integ->options.MBXGlobal.restore.restoreTo);
        strcpy(hostname, integ->options.MBXGlobal.restore.restoreTo);
    }
    else
    {
        sprintf(szAppSrv, _T("%s%s"), BG_MBX_APPSRV, integ->hostname);
        strcpy(hostname, integ->hostname);
    }
    
    retval = UtilExecute (context->dbsmhandle,
        NULL,
        hostname,
        MBX_AGENT,
        MBX_ECONFIG,
        MBX_ADMIN,
        config->MBXConfig.admin,
        MBX_PASSWORD,
        encodedPwd,
        MBX_DOMAIN,
        config->MBXConfig.domain,
        szAppSrv,
        NULL);

    FREE(encodedPwd);
    RETURN_INT (retval);
}

/*========================================================================*//**
*
* @brief     Retrieve versions of object (get also -trees option)
*
*//*=========================================================================*/
static int MBX_GetVerObj (
    int       cellbind,
    int       dbsmHandle,
    IntegObj *object,
    array_t  *versions )
{
    ERH_FUNCTION(_T("GetVerObjMBX"));
    USES_BARGUI_PTD

    IInteg *integ = object? IQuery(object->apptype) : NULL;
    tchar   objectname[STRLEN_OBJECTNAME+1]={0};
    array_t objectnames = array_new(ARR_TYPE_STR);
    int     i;
    int     retval;

    DbgFcnIn();
    StrToObjectNameEx(OT_OB2BAR,
        object->hostname,
        object->objectname,
        integ->name,
        objectname);

    array_push (&objectnames, objectname);

    for (i=0; i<object->addObjCount; ++i)
    {
        StrToObjectNameEx(OT_OB2BAR,
            object->hostname,
            object->objectname,
            integ->name,
            objectname);

        array_push (&objectnames, objectname);
    }

    ThisBarGui->queryObjectType = FUN_OBJECTQUERYFLAGS_NORMAL|FUN_OBJECTQUERYFLAGS_TREES;

    retval = QueryVersionsOfObjects (
        dbsmHandle,
        FilterVersionCompleted,
        NULL,
        0,
        objectnames,
        versions );

    array_free (&objectnames);
    RETURN_INT (retval);
}

static void MBX_IntegConfigFree (IntegConfig *cfg)
{
    FREE(cfg->MBXConfig.admin);
    FREE(cfg->MBXConfig.password);
    FREE(cfg->MBXConfig.domain);
}


static int comp (const void * a, const void * b)
{
    IntegObj *a1=*(IntegObj **)a;
    IntegObj *b1=*(IntegObj **)b;
    return ( StrCmp (a1->objectname,b1->objectname));
}

/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Create args string for mbx_bar for backup
*
*//*=========================================================================*/
int MBX_PackBarlist (
    WlObject  *pWLOBJECT,
    Integ     *integration,
    IntegObj **integObjArray,
    int        IntegObjCount )
{
    ERH_FUNCTION(_T("MBX_PackBarlist"));
    CmnVec *args = &pWLOBJECT->data.cl.args;
    IntegObj **ioArray=NULL;
    int     i;

    BG_DBGENTRY;

    /* hostname is always added to barlist: for clusters mostly */
    CmnVecAddOpt (args, BG_MBX_APPSRV, integration->hostname);
    ioArray=(IntegObj **)MALLOC(sizeof(IntegObj *)*IntegObjCount);
    for(i=0;i<IntegObjCount;i++) ioArray[i]=integObjArray[i];
    qsort(ioArray, IntegObjCount, sizeof(IntegObj *), comp);
    if (!IntegObjCount)
    {
        /*All or nothing is marked?*/
        if ( integration->options.MBXGlobal.backup.backupAll )
            CmnVecAddStr (args, BG_MBX_BACKUP_ALL);
    }
    else
    {
        tchar *prevMailbox=StrNewCopy(_T(""));
        BOOL prevIsPublic=FALSE;
        /* PERFORM BACKUP TOKEN */
        CmnVecAddStr (args, BG_MBX_BACKUP_PERFORM);

        for (i=0; i<IntegObjCount; ++i)
        {
            IntegObj *integObject = ioArray[i];
            tchar   *stType=NULL;
            tchar   *grpStart=NULL;
            int     bkpType=-1;
            int     storageType=-1;
            tchar   *mbStart=NULL;
            tchar   *mbEnd=NULL;
            tchar   *group=NULL;
            tchar   *mailbox=NULL;
            tchar   *folder=NULL;
            tchar   *objName=integObject->objectname+1;
            grpStart=StrChr(objName,MBX_FOLDER_SEP_CHAR);
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Object name:%s"), __FCN__, NS(integObject->objectname));
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] name:%s"), __FCN__, NS(integObject->name));
            if(!grpStart)
            {
                grpStart=StrChr(objName,_T('\0'));
                bkpType=0;
            }
            stType=StrNNewCopy(objName, grpStart-objName);
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] StorageType:\"%s\""),__FCN__, stType);

            if(StrNCmp(stType, MBX_TYPE_PRIVAT_STR, StrLen(MBX_TYPE_PRIVAT_STR)))
            {
                storageType=MBX_TYPE_PUBLIC;
            }
            else
            {
                storageType=MBX_TYPE_PRIVAT;
            }
            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Numeric Storage Type=%d"), __FCN__, storageType);
            if(storageType==MBX_TYPE_PRIVAT)
            {
                if(bkpType==-1)
                {
                    tchar *grpEnd=StrChr(grpStart+1, MBX_FOLDER_SEP_CHAR);
                    if(!grpEnd)
                    {
                        bkpType=1;
                        grpEnd=StrChr(grpStart, _T('\0'));

                    }
                    else
                        if(bkpType==-1)
                        {
                            mbStart=grpEnd+1;
                            mbEnd=StrChr(mbStart, MBX_FOLDER_SEP_CHAR);
                            if(!mbEnd)
                            {
                                mbEnd=StrChr(mbStart, _T('\0'));
                                bkpType=2;
                            }
                            else
                            {
                                if(bkpType==-1)
                                {
                                    folder=StrNewCopy(mbEnd+1);
                                    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Folder:%s"), __FCN__, folder);
                                    bkpType=3;
                                }
                            }
                            mailbox=StrNNewCopy(mbStart, mbEnd-mbStart);
                            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Mailbox:%s"), __FCN__, mailbox);
                        }
                        group=StrNNewCopy(grpStart+1,1);
                        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Group:%s"), __FCN__, group);
                }
                switch(bkpType)
                {
                case 0:
                  CmnVecAddStr (args, BG_MBX_BACKUP_ALL_MBX);
                  break;
                case 1:
                  CmnVecAddOpt (args, BG_MBX_GROUP, BU_EncodePath(integObject->name));
                  break;
                case 2:
                  CmnVecAddOpt (args, BG_MBX_MAILBOX, BU_EncodePath(integObject->name));
                  break;
                case 3:
                  DbgPlain(DBG_MAIN_ACTION, _T("[%s] Folder:%s"), __FCN__, folder);
                  if(StrCmp(prevMailbox,mailbox))
                  {
                      CmnVecAddOpt (args, BG_MBX_MAILBOX, mailbox);
                  }
                  if(integObject->options.MBXLocal.backup.exclude)
                  {
                        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Exclude this folder"), __FCN__);
                        CmnVecAddOpt (args, BG_MBX_EXCLUDE, folder);
                  }
                  else
                  {
                        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Include this folder"), __FCN__);
                        CmnVecAddOpt (args, BG_MBX_FOLDER, folder);
                  }
                }
                FREE(prevMailbox);
                prevMailbox=StrNewCopy(mailbox);
            }
            else
            {
                if(!prevIsPublic)
                {
                    CmnVecAddStr (args, BG_MBX_PUBLIC);
                    prevIsPublic=TRUE;
                }
                if(StrCmp(integObject->name, MBX_TYPE_PUBLIC_STR))
                {
                    folder=StrNewCopy(objName+StrLen(MBX_TYPE_PUBLIC_STR)+1);
                    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Folder:%s"), __FCN__, folder);
                    if(integObject->options.MBXLocal.backup.exclude)
                    {
                        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Exclude this folder"), __FCN__);
                        CmnVecAddOpt (args, BG_MBX_EXCLUDE, folder);
                    }
                    else
                    {
                        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Include this folder"), __FCN__);
                        CmnVecAddOpt (args, BG_MBX_FOLDER, folder);
                    }
                }
            }

        }
    }
    FREE(ioArray);

    CmnVecAddIntOpt (args, BG_MBX_N,         integration->options.MBXGlobal.backup.paralelism);
    CmnVecAddOpt    (args, BG_MBX_PRE_EXEC,  integration->options.preexec);
    CmnVecAddOpt    (args, BG_MBX_POST_EXEC, integration->options.postexec);

    BG_RETURN(OBE_OK);
}


static MALLOCED tchar * ConvertWlToObjFolder(ctchar *folder)
{
    tchar *tmp=StrrChr(folder, MBX_FOLDER_SEP_CHAR);
    tchar *newFolder=StrNewCopy(BU_DecodePath(tmp?tmp+1:folder));
    if (newFolder==NULL)
        DbgPlain(DBG_MAIN_ACTION, _T("Get folder %s from Path %s"), NS(newFolder), NS(folder));
    return(newFolder);
}


/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Puts data from args section of barlist into IntegObj structure
*
*//*=========================================================================*/
int MBX_UnpackBarlist (
    Integ       *integration, 
    IntegObj  ***IntegObjArray,
    int         *IntegObjCount,
    WlObject    *pWLOBJECT)
{
    ERH_FUNCTION(_T("MBX_UnpackBarlist"));
    IntegObj    *IntegPointer, *integObj;
    tchar       *lpszFirstToken=NULL, szBuffer[STRLEN_MESSAGE+1];
    int         i=0;
    int         iAllocCount=0;
    int         tokensCount=CmnVecH(&(pWLOBJECT->data.cl.args));

    BG_DBGENTRY;

    integObj= NULL;
    while (i<tokensCount)
    {
        lpszFirstToken= CmnVecI(&(pWLOBJECT->data.cl.args), i);
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proceeding %s"), __FCN__, lpszFirstToken);

        if (!StrNICmp(lpszFirstToken,BG_MBX_BACKUP_ALL, StrLen(BG_MBX_BACKUP_ALL)) )
        {
        /* BACKUP_ALL TOKEN */
            integration->options.MBXGlobal.backup.backupAll = TRUE;
            i++;
        }
        else if (!StrNICmp(lpszFirstToken,BG_MBX_PRE_EXEC, StrLen(BG_MBX_PRE_EXEC)))
        {
        /* PREEXEC TOKEN */
            if (integration->options.preexec) FREE(integration->options.preexec);
            integration->options.preexec= StrNewCopy( StrChr(lpszFirstToken,_T(':'))+1 );
            i++;
        }
        else if  (!StrNICmp(lpszFirstToken,BG_MBX_POST_EXEC, StrLen(BG_MBX_POST_EXEC)))
        {
        /* POSTEXEC TOKEN */
            if (integration->options.postexec) FREE(integration->options.postexec);
            integration->options.postexec= StrNewCopy( StrChr(lpszFirstToken,_T(':'))+1 );
            i++;
        }
        else if  (!StrNICmp(lpszFirstToken,BG_MBX_BACKUP_ALL_MBX, StrLen(BG_MBX_BACKUP_ALL_MBX)))
        {
        /* ALL_MBX TOKEN */
            if ((*IntegObjCount+1)>iAllocCount)
            {
                integObj=(IntegObj*)REALLOC(integObj,
                (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
            }

            IntegPointer= &(integObj[*IntegObjCount]);
            (*IntegObjCount)++;

            IntegObjInit(IntegPointer, integration->apptype);

            IntegPointer->apptype= integration->apptype;
            IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
            IntegPointer->hostname= StrNewCopy(integration->hostname);
            IntegPointer->name= StrNewCopy(MBX_TYPE_PRIVAT_STR);

            sprintf(szBuffer, _T("%c%s"), MBX_FOLDER_SEP_CHAR, MBX_TYPE_PRIVAT_STR);
            IntegPointer->objectname= StrNewCopy(szBuffer);
            IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;

            IntegPointer->description= StrNewCopy(_T(""));
            IntegPointer->level=0;
            i++;
        }
        else if  (!StrNICmp(lpszFirstToken,BG_MBX_GROUP, StrLen(BG_MBX_GROUP)) )
        {
        /* GROUP TOKEN */
            if ((*IntegObjCount+1)>iAllocCount)
            {
                integObj=(IntegObj*)REALLOC(integObj,
                (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
            }

            IntegPointer= &(integObj[*IntegObjCount]);
            (*IntegObjCount)++;

            IntegObjInit(IntegPointer, integration->apptype);

            IntegPointer->apptype= integration->apptype;
            IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
            IntegPointer->hostname= StrNewCopy(integration->hostname);
            IntegPointer->name= StrNewCopy( BU_DecodePath( StrChr(lpszFirstToken,_T(':')) +1 ) );

            sprintf(szBuffer, _T("%c%s%c%s"),
                MBX_FOLDER_SEP_CHAR,
                MBX_TYPE_PRIVAT_STR,
                MBX_FOLDER_SEP_CHAR,
                BU_EncodePath(IntegPointer->name));
            IntegPointer->objectname= StrNewCopy(szBuffer);
            IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;

            IntegPointer->description= StrNewCopy(_T(""));
            IntegPointer->level=1;
            i++;
        }
        else if  (!StrNICmp(lpszFirstToken,BG_MBX_MAILBOX, StrLen(BG_MBX_MAILBOX)) )
        {
        /* MAILBOX TOKEN */
            tchar *mailbox_decoded=StrNewCopy(BU_DecodePath(lpszFirstToken+StrLen(BG_MBX_MAILBOX)+1));
            tchar *mailbox_encoded=StrNewCopy(lpszFirstToken+StrLen(BG_MBX_MAILBOX)+1);

            tchar firstChar_encoded[4];     /*encoded chars are made out of 2 chars + we need NUL char*/
            firstChar_encoded[0]=mailbox_encoded[0];
            if (firstChar_encoded[0]==BU_ENCODE_ESCAPE_CHAR)
            {
            /*encoded char*/
                firstChar_encoded[1]=mailbox_encoded[1];
                firstChar_encoded[2]=0;
            }
            else
            {
            /*normal char*/
                firstChar_encoded[0]=toupper(firstChar_encoded[0]);
                firstChar_encoded[1]=0;
            }

            DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proccedeing with new mailbox %s"), __FCN__, mailbox_decoded);
            /*next token*/
            i++;
            lpszFirstToken= CmnVecI(&(pWLOBJECT->data.cl.args), i);

            /*include Mailbox in the list if next token is not a mailbox folder*/
            if ( 0 != StrNICmp(lpszFirstToken,BG_MBX_FOLDER, StrLen(BG_MBX_FOLDER)) )
            {
                /*include Mailbox */
                if ((*IntegObjCount+1)>iAllocCount)
                {
                    integObj=(IntegObj*)REALLOC(integObj,
                        (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
                }

                IntegPointer= &(integObj[*IntegObjCount]);
                (*IntegObjCount)++;

                IntegObjInit(IntegPointer, integration->apptype);

                IntegPointer->apptype= integration->apptype;
                IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
                IntegPointer->hostname= StrNewCopy(integration->hostname);
                IntegPointer->description= StrNewCopy(_T(""));
                IntegPointer->name= StrNewCopy(mailbox_decoded);

                sprintf(szBuffer, _T("%c%s%c%s%c%s"),
                    MBX_FOLDER_SEP_CHAR,
                    MBX_TYPE_PRIVAT_STR,
                    MBX_FOLDER_SEP_CHAR,
                    firstChar_encoded,
                    MBX_FOLDER_SEP_CHAR,
                    mailbox_encoded);
                IntegPointer->objectname= StrNewCopy(szBuffer);
                IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;

                IntegPointer->level=2;
            }

            /*Include FOLDERs and EXCLUDEs in the list.*/
            while (
              ( !StrNICmp(lpszFirstToken,BG_MBX_FOLDER, StrLen(BG_MBX_FOLDER)) )
              ||
              ( !StrNICmp(lpszFirstToken,BG_MBX_EXCLUDE, StrLen(BG_MBX_EXCLUDE)) )
            )
            {
///             tchar *folder=ConvertWlToObjPath(StrChr(lpszFirstToken, _T(':'))+1);
                tchar *folder_decoded=StrNewCopy(BU_DecodePath(StrChr(lpszFirstToken, _T(':'))+1));
                tchar *folder_encoded=StrNewCopy(StrChr(lpszFirstToken, _T(':'))+1);
                tchar *tmp=folder_encoded;
                int level=3;
                BOOL exclude=FALSE;
                if (!StrNICmp(lpszFirstToken,BG_MBX_EXCLUDE, StrLen(BG_MBX_EXCLUDE)))
                {
                    exclude=TRUE;
                }
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proceeding %s"), __FCN__, lpszFirstToken);
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proccedeing with new folder %s"), __FCN__, folder_decoded);

                if ((*IntegObjCount+1)>iAllocCount)
                {
                    integObj=(IntegObj*)REALLOC(integObj,
                        (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
                }

                IntegPointer= &(integObj[*IntegObjCount]);
                (*IntegObjCount)++;

                IntegObjInit(IntegPointer, integration->apptype);

                IntegPointer->apptype= integration->apptype;
                IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
                IntegPointer->hostname= StrNewCopy(integration->hostname);
                IntegPointer->description= StrNewCopy(_T(""));

                while (tmp=StrChr(tmp+1, MBX_FOLDER_SEP_CHAR), tmp)
                {
                      level++;
                }
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Level=%d"), __FCN__, level);

                IntegPointer->name= ConvertWlToObjFolder(folder_encoded);
                sprintf(szBuffer, _T("%c%s%c%s%c%s%c%s"),
                    MBX_FOLDER_SEP_CHAR,
                    MBX_TYPE_PRIVAT_STR,
                    MBX_FOLDER_SEP_CHAR,
                    firstChar_encoded,
                    MBX_FOLDER_SEP_CHAR,
                    mailbox_encoded,
                    MBX_FOLDER_SEP_CHAR,
                    folder_encoded);
                IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;
                IntegPointer->objectname= StrNewCopy(szBuffer);
                IntegPointer->level=level;
                IntegPointer->options.MBXLocal.backup.exclude=exclude;
                FREE(folder_decoded);
                FREE(folder_encoded);
                i++;
                lpszFirstToken= CmnVecI(&(pWLOBJECT->data.cl.args), i);
            }

            FREE(mailbox_decoded);
            FREE(mailbox_encoded);
        }
        else if  (!StrNICmp(lpszFirstToken,BG_MBX_PUBLIC, StrLen(BG_MBX_PUBLIC)) )
        {
        /* PUBLIC TOKEN */
            BOOL haveFolders=FALSE;
            i++;
            lpszFirstToken= CmnVecI(&(pWLOBJECT->data.cl.args), i);
            /* FOLDER/EXCLUDE TOKEN */
            while (
              ( !StrNICmp(lpszFirstToken,BG_MBX_FOLDER, StrLen(BG_MBX_FOLDER)) )
              ||
              ( !StrNICmp(lpszFirstToken,BG_MBX_EXCLUDE, StrLen(BG_MBX_EXCLUDE)) )
            )
            {
///             tchar *folder=ConvertWlToObjPath(StrChr(lpszFirstToken, _T(':'))+1);
                tchar *folder_decoded=StrNewCopy(BU_DecodePath(StrChr(lpszFirstToken, _T(':'))+1));
                tchar *folder_encoded=StrNewCopy(StrChr(lpszFirstToken, _T(':'))+1);
                tchar *tmp=folder_encoded;
                int level=1;
                BOOL exclude=FALSE;
                haveFolders=TRUE;
                if (!StrNICmp(lpszFirstToken,BG_MBX_EXCLUDE, StrLen(BG_MBX_EXCLUDE)))
                {
                    exclude=TRUE;
                }
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proceeding %s"), __FCN__, lpszFirstToken);
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Proccedeing with new folder %s"), __FCN__, folder_decoded);

                if ((*IntegObjCount+1)>iAllocCount)
                {
                    integObj=(IntegObj*)REALLOC(integObj,
                        (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
                }

                IntegPointer= &(integObj[*IntegObjCount]);
                (*IntegObjCount)++;

                IntegObjInit(IntegPointer, integration->apptype);

                IntegPointer->apptype= integration->apptype;
                IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
                IntegPointer->hostname= StrNewCopy(integration->hostname);
                IntegPointer->description= StrNewCopy(_T(""));

                while (tmp=StrChr(tmp+1, MBX_FOLDER_SEP_CHAR), tmp)
                    level++;

                IntegPointer->name = ConvertWlToObjFolder(folder_encoded);

                sprintf(szBuffer, _T("%c%s%c%s"),
                    MBX_FOLDER_SEP_CHAR,
                    MBX_TYPE_PUBLIC_STR,
                    MBX_FOLDER_SEP_CHAR,
                    folder_encoded);
                IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;
                IntegPointer->objectname= StrNewCopy(szBuffer);
                IntegPointer->level=level;
                IntegPointer->options.MBXLocal.backup.exclude=exclude;
                FREE(folder_decoded);
                FREE(folder_encoded);
                i++;
                lpszFirstToken= CmnVecI(&(pWLOBJECT->data.cl.args), i);
                DbgPlain(DBG_MAIN_ACTION, _T("Next Token:%s"), lpszFirstToken);
            }

            if(!haveFolders)
            {
                if ((*IntegObjCount+1)>iAllocCount)
                {
                    integObj=(IntegObj*)REALLOC(integObj,
                        (iAllocCount+=BG_ALLOC_COUNT)*sizeof(IntegObj) );
                }

                IntegPointer= &(integObj[*IntegObjCount]);
                (*IntegObjCount)++;

                IntegObjInit(IntegPointer, integration->apptype);

                IntegPointer->apptype= integration->apptype;
                IntegPointer->appname= StrNewCopy(BG_NO_INSTANCES);
                IntegPointer->hostname= StrNewCopy(integration->hostname);
                IntegPointer->description= StrNewCopy(_T(""));
                IntegPointer->name= StrNewCopy(MBX_TYPE_PUBLIC_STR);
                sprintf(szBuffer, _T("%c%s"),MBX_FOLDER_SEP_CHAR,MBX_TYPE_PUBLIC_STR);
                IntegPointer->objectname= StrNewCopy(szBuffer);
                IntegPointer->delimeter=MBX_FOLDER_SEP_CHAR;

                IntegPointer->level=0;
            }

        }
        else
        {
            i++;
        }
    }

    *IntegObjArray= MALLOC(*IntegObjCount*sizeof(IntegObj*));

    IntegPointer= integObj;
    for (i=0;i<*IntegObjCount;i++) (*IntegObjArray)[i]= IntegPointer++;

    BG_RETURN(OBE_OK);
}

/*========================================================================*//**
*
* @retval    Variant          document describing the restore command
*
* @brief     Fill restore document (for restore and chain calculation)
*
*//*=========================================================================*/
static Variant GetRestoreDoc (
    IN Integ     *integ,
    IN int        nObjects, 
    IN IntegObj **pobjects,
    IN IntegObj  *objects,
    IN IntegVer **pversions,
    IN IntegVer  *versions,
    IN BOOL       quotePath,
	IN BOOL       isBrowseOnly
    )
{
    Variant  doc = VarUndef;
    tchar   *prevMailbox = NULL;
    int      i;

    #define PATHARG quotePath ? _T("%s \"%s\"") : _T("%s %s")
    MBXGLOBALRestore *global = &(integ->options.MBXGlobal.restore);

    VarPush(&doc, VarFormat(NULL, _T("%s%s"), BG_MBX_APPSRV, 
        StrIsEmpty(global->restoreTo) ? integ->hostname : global->restoreTo
    ));

    VarPush(&doc, VarFormat(NULL, _T("%s %s"), BG_MBX_RESTOREFROM, integ->hostname));
    
    for (i=0; i<nObjects; ++i)
    {
        IntegObj    *obj = pobjects ? pobjects[i] : objects ? &objects[i] : NULL;
        IntegVer    *ver = pversions ? pversions[i] : versions ? &versions[i] : NULL;

        MBXLOCALRestore *local = &(obj->options.MBXLocal.restore);

        /* if integObjArray is type of folder local->mbxName contains mailbox name */
        if ( ((obj->level == 2) && (local->mbxType==MBX_TYPE_PRIVAT))
            || ((obj->level == 0) && (local->mbxType==MBX_TYPE_PUBLIC))
            || (StrCmp(prevMailbox, local->mbxName) != 0)
            )
        {
            tchar *mbx = StrIsEmpty(local->mbxName) ? obj->name : local->mbxName;
            if (!StrIsEmpty(local->restoreInto))
            {
                VarPush(&doc, VarFormat(NULL, PATHARG, BG_MBX_MAILBOX, BU_EncodePath(local->restoreInto)));
                if (MBX_TYPE_PUBLIC != local->mbxType)
                {
                    VarPush(&doc, VarFormat(NULL, PATHARG, BG_MBX_RESTORE_FROM_MBX, BU_EncodePath(mbx)));
                }
                else
                {
                    VarPush(&doc, VarFormat(NULL, PATHARG, BG_MBX_RESTORE_FROM_MBX, BU_EncodePath(MBX_PUBLIC_OBJ)));
                }
            }
            else
            {
                if (MBX_TYPE_PUBLIC != local->mbxType)
                {
                    VarPush(&doc, VarFormat(NULL, PATHARG, BG_MBX_MAILBOX, BU_EncodePath(mbx)));
                }
                else
                {
                    VarPush(&doc, VarFormat(NULL, BG_MBX_PUBLIC));
                }
            }

            VarPush(&doc, VarFormat(NULL, _T("%s %s"), BG_MBX_RESTORE_MAILBOX_VER, CliSessionIDToStr(ver->sessionname)));

            if (local->chain)
                VarPush(&doc, T2V(BG_MBX_RESTORE_CHAIN));
            /* FIX : QCCR2A31118 - GUI doesn't send the parameter 'MBX_RESTORE_ORIG_FOLDER' during browse operation*/
			if (!isBrowseOnly)
			{
            if (LOCATION_ORIG_OVERWRITE == local->restoreLocation)
                VarPush(&doc, VarFormat(NULL, _T("%s %s"), BG_MBX_RESTORE_ORIG_FOLDER, BG_MBX_RESTORE_OVERWRITE));
            
            if (LOCATION_ORIG_KEEP == local ->restoreLocation)
                VarPush(&doc, VarFormat(NULL, _T("%s %s"), BG_MBX_RESTORE_ORIG_FOLDER, BG_MBX_RESTORE_KEEP));
			}

            prevMailbox = local->mbxName == NULL ? obj->name : local->mbxName;
        }

        if (((obj->level > 2 && local->mbxType == MBX_TYPE_PRIVAT) || 
             (obj->level > 0 && local->mbxType == MBX_TYPE_PUBLIC))
            && local->restoreFolder != FOLDER_NO_RESTORE)
        {
            VarPush(&doc, VarFormat(NULL, PATHARG, 
                local->restoreFolder == FOLDER_INCLUDE ? BG_MBX_RESTORE_MAILBOX_FOLDER : BG_MBX_RESTORE_MAILBOX_EXCLUDE,
                obj->description
            ));
        }
    }

    return doc;
}

/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Fill Command for restore
*
*//*=========================================================================*/
int MBX_GetRestoreCommand(
    Integ     *integ, 
    IntegObj **objects,
    int        nObjects,
    IntegVer **versions,
    tchar    **args
    )
{
    ERH_FUNCTION(_T("MBX_GetRestoreCommand"));

    Variant doc, msg = VarUndef;
    int     i;

    DbgFcnIn();

	/* FIX : QCCR2A31118 */
    doc = GetRestoreDoc(integ, nObjects, objects, NULL, versions, NULL, TRUE, FALSE);

    integ->nosu = TRUE;

    if (nObjects)
        VarCat(&msg, _T("%s "), BG_MBX_RESTORE_PERFORM);

    for (i=0;i<VarLength(&doc);++i)
        VarCat(&msg, _T("%s "), VarTextAt(&doc, i));

    *args = V2T(&msg);

    DbgPlain(DBG_MAIN_ACTION, _T("Command Line is \n%s"), *args);

    RETURN_INT(OBE_OK);
}

int MBX_EnumRestoreChain (
    IN  GUI      *gui,
    IN  Integ    *integ,
    IN  IntegObj *objects,
    IN  int       objCount,
    IN  IntegVer *versions,
    OUT CmnVec   *ovids,
    OUT CmnVec   *devices )
{
    ERH_FUNCTION(_T("MBX_EnumRestoreChain"));

    Variant  doc, *devlist, *ovidlist;
    Variant  out = VarUndef;
    tchar    msg[MAX_STRING_SIZE+1]={0};
    int      i, retval;

    MBXGLOBALRestore *global = &(integ->options.MBXGlobal.restore);

    DbgFcnIn();

	/* FIX : QCCR2A31118 */
    doc = GetRestoreDoc(integ, objCount, NULL, objects, NULL, versions, FALSE, TRUE);

    StrMsgMake (msg, MAX_STRING_SIZE, FUN_EXECINTEGUTIL,
        StrIsEmpty(global->restoreTo) ? integ->hostname : global->restoreTo,
        MBX_AGENT,
        _T("-listchain"),
        NULL
    );

    for (i=0;i<VarLength(&doc);++i)
        StrMsgAppend(VarTextAt(&doc, i));

    retval = UtilExecuteX (gui->dbsmhandle, msg, UTIL_EXECUTE_VARIANT, &out);

    if (0 != retval)
    {
        ErhMark(retval, __FCN__, ERH_WARNING);
        VarFree (&out);
        RETURN_INT(retval);
    }

    devlist  = VarGet(&out, _T("devices"));
    ovidlist = VarGet(&out, _T("ovids"));

    for (i=0; i<VarLength(ovidlist);++i)
    {
        CmnVecAddStr(ovids, VarTextAt(ovidlist, i));
    }

    for (i=0; i<VarLength(devlist); ++i)
        CmnVecAddStr(devices, VarTextAt(devlist, i));

    VarFree(&out);

    ERHCLEARERROR
    RETURN_INT(retval);
}

/*========================================================================*//**
*
* @brief     Retrieves the instances for specific integration on specific hostname
*
*//*=========================================================================*/
static int MBX_EnumInstances  (
    GUI       *gui,
    int        appType,
    THostList *hostinfo,
    array_t   *integs )
{
    ERH_FUNCTION(_T("MBX_EnumInstances"));
    Integ *integ;

    DbgFcnIn();

    if (!hostinfo->MBX)
        RETURN_INT (OBE_OK);

    integ = array_push (integs, NULL);
    IntegInit(integ, appType);
    integ->hostname   = StrNewCopy(hostinfo->name);
    integ->appname    = StrNewCopy(MBX_APP_NAME);
    integ->platformId = hostinfo->platformId;

    RETURN_INT (OBE_OK);
}




/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Browse groups, mailboxes, folders, public folders
*
*//*=========================================================================*/
int MBX_EnumObjects (
    GUI      *gui,
    Integ    *integration,
    int       level,
    IntegObj *prevObj,
    array_t  *objs )
{
    ERH_FUNCTION(_T("MBX_EnumObjects"));
    int       retval=OBE_OK;
    array_t   lines = array_new(ARR_TYPE_STR);
    IntegObj *io;
    tchar     szAppSrv[STRLEN_1K];
    int       i;

    DbgFcnIn();
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Level=%d"), __FCN__, level);

    if(level==0)
    {
        /* if level==0 then we just display two elements: Private and Public */
        io = array_push (objs, NULL);
        IntegObjInit(io, integration->apptype);
        io->name = StrNewCopy(MBX_TYPE_PRIVAT_STR);
        io->delimeter=MBX_FOLDER_SEP_CHAR;
        io->objectname=StrNewCopy(MBX_FOLDER_SEP);
        StrAppend(io->objectname,MBX_TYPE_PRIVAT_STR);
        io->appname = StrNewCopy(integration->appname);
        io->hostname = StrNewCopy(integration->hostname);
        io->prev = NULL;
        io->level = level;
        io->flag |= ( integration->isTemplate ) ? INTEGOBJ_LEAF : 0;

        io = array_push (objs, NULL);
        IntegObjInit(io, integration->apptype);
        io->name = StrNewCopy(MBX_TYPE_PUBLIC_STR);
        io->delimeter=MBX_FOLDER_SEP_CHAR;
        io->objectname=StrNewCopy(MBX_FOLDER_SEP);
        StrAppend(io->objectname,MBX_TYPE_PUBLIC_STR);
        io->appname = StrNewCopy(integration->appname);
        io->hostname = StrNewCopy(integration->hostname);
        io->prev = NULL;
        io->level = level;
        io->flag |= ( integration->isTemplate ) ? INTEGOBJ_LEAF : 0;
        goto end;
    }
    else if ( integration->isTemplate == TRUE )
    {
        /* templates only support level = 0 */
        RETURN_INT(OBE_OK);
    }

// Show public folder from root or from folder
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Browse subobjects of %s level %d"), __FCN__, NS(prevObj->name), level);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Previous Object Name is %s"), __FCN__, NS(prevObj->objectname));

    sprintf(szAppSrv, _T("%s%s"), BG_MBX_APPSRV, integration->hostname);

    if(!StrNCmp(prevObj->objectname+1, MBX_TYPE_PUBLIC_STR, StrLen(MBX_TYPE_PUBLIC_STR)))
    {
        if(level==1)
        {
            retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                MBX_AGENT,
                MBX_BROWSE,
                _T("-public"),
                szAppSrv,
                NULL);
        }
        else
        {
            tchar *folderName=StrChr(prevObj->objectname+1, MBX_FOLDER_SEP_CHAR)+1;
            retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                MBX_AGENT,
                MBX_BROWSE,
                _T("-public"),
                _T("-folder"),
                folderName,
                szAppSrv,
                NULL);
        }
    }
//depending on level show groups, mailboxes or mailbox folders
    else
    {
        switch(level)
        {
        case 0:
            {
                break;
            }
        case 1:
            {
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Browse Groups"), __FCN__);
                retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                    MBX_AGENT,
                    MBX_BROWSE,
                    szAppSrv,
                    NULL);
                break;
            }
        case 2:
            {
                tchar szGroup[STRLEN_OBJNAME+1] = {0};
                sprintf(szGroup, _T("%s %s"), MBX_GROUP, prevObj->name);
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Browse Group:%s"), __FCN__, prevObj->name);
                retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                    MBX_AGENT,
                    MBX_BROWSE,
                    MBX_GROUP,
                    BU_EncodePath(prevObj->name),
                    szAppSrv,
                    NULL);
                break;
            }
        case 3:
            {
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Browse Mailbox:%s"), __FCN__, prevObj->name);
                retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                    MBX_AGENT,
                    MBX_BROWSE,
                    _T("-mailbox"),
                    BU_EncodePath(prevObj->name),
                    szAppSrv,
                    NULL);
                break;
            }
        default:
            {
                tchar *grpStart=StrChr(prevObj->objectname+1, MBX_FOLDER_SEP_CHAR)+1;
                tchar *mbxStart=StrChr(grpStart, MBX_FOLDER_SEP_CHAR)+1;
                tchar *mbxEnd=StrChr(mbxStart, MBX_FOLDER_SEP_CHAR);
                tchar *mailbox=StrNNewCopy(mbxStart,mbxEnd-mbxStart);
                tchar *folder=mbxEnd+1;
                DbgPlain(DBG_MAIN_ACTION, _T("[%s]\nOriginal folder:%s."), __FCN__, folder);
                DbgPlain(DBG_MAIN_ACTION, _T("[%s] Browse Mailbox:%s, folder:%s"), __FCN__, mailbox, folder);
                retval = UtilExecute (gui->dbsmhandle, &lines, integration->hostname,
                    MBX_AGENT,
                    MBX_BROWSE,
                    _T("-mailbox"),
                    mailbox,
                    _T("-folder"),
                    folder,
                    szAppSrv,
                    NULL);

                FREE (mailbox);
            }
        }
    }

    for (i=0; i<lines.count; ++i)
    {
        tchar *newName=NULL;
        tchar *msg = array_get(&lines, i);

        io = array_push (objs, NULL);
        IntegObjInit(io, integration->apptype);
        io->name = StrNewCopy(msg);
        if ( level == 2 && StrStr(io->name, MBX_DUPLICATED_TAG) )
            io->flag = INTEGOBJ_NO_BACKUP_SELECT | INTEGOBJ_LEAF;
        io->delimeter=MBX_FOLDER_SEP_CHAR;
        newName = StrNewCopy(BU_EncodePath(io->name));
        io->objectname = StrNewCopy(prevObj->objectname);
        StrAppend(io->objectname,MBX_FOLDER_SEP);
        StrAppend(io->objectname, newName);
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Object name=%s"), __FCN__, io->objectname);
        io->appname = StrNewCopy(integration->appname);
        io->hostname = StrNewCopy(integration->hostname);
        io->prev = prevObj;
        io->level = level;
        FREE(newName);
    }

    array_free (&lines);
end:
    RETURN_INT(retval);
}


/*
A
+---Aobject
    Abala
B
+---BmailBox
    Bibi
C
    ...
*/
/*========================================================================*//**
*
* @brief     Browse objects for restore
*
*//*=========================================================================*/
int MBX_EnumRestoreObjects (
    int        level,
    tchar     *mountpoint,
    Integ     *integration,
    IntegObj  *prevObj,
    array_t   *objs)
{
    ERH_FUNCTION(_T("MBX_EnumRestoreObjects"));
    IntegMountPoint mp = {0};
    IntegObj *io        = NULL;
    tchar    *name      = NULL;
    tchar     group[10] = {0};
    DbgFcnIn();


    if (NULL == MBX_UnpackMount(mountpoint, &mp))
        RETURN_INT (OBE_OK);

    /* Create groups */
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Mailbox Name = %s"), __FCN__, mp.mbx.dbName);
    if(mp.mbx.type == MBX_TYPE_PRIVAT)
    {
        sprintf (group, _T("[ %c ]"), toupper(mp.mbx.dbName[0]));
        name = 
            level == 0? MBX_TYPE_PRIVAT_STR : 
            level == 1? group :
            0==StrCmp(prevObj->name, group)? mp.mbx.dbName :
            NULL;
    }
    else if (level == 0)
    {
        name = MBX_TYPE_PUBLIC_STR;
    }
    
    if (!name)
        RETURN_INT (OBE_OK);

    if (array_findstr(objs, IntegObjGetName, name))
        RETURN_INT (OBE_OK);

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Display Name: %s"), __FCN__, name);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Mountpoint: %s"), __FCN__, NSD(mountpoint));
    io = array_push (objs, NULL);
    IntegObjInitEx (io, integration);
    io->name       = StrNewCopy( BU_DecodePath(name) );
    io->objectname = StrNewCopy(mountpoint);    /*mountpoint is already encoded*/
    // io->realname   = StrNewCopy(objectName);
    io->prev       = prevObj;
    io->level      = level;
    io->options.MBXLocal.restore.mbxType = mp.mbx.type;
    io->options.MBXLocal.restore.mbxName = StrNewCopy(mp.mbx.dbName);

    array_sort (objs, IntegObjCmpName);
    RETURN_INT (OBE_OK);
}

/*========================================================================*//**
*
* @brief     Retrieves the versions of a specified integration object.
*
*//*=========================================================================*/
int MBX_EnumObjectVersions (
    GUI      *gui,
    IntegObj *integObj,
    array_t  *versions )
{
    ERH_FUNCTION(_T("MBX_EnumObjectVersions"));
    int iRet;
    DbgFcnIn();
    iRet = MBX_GetVerObj(gui->cellbind, gui->dbsmhandle, integObj, versions);
    RETURN_INT (iRet);
}


/*========================================================================*//**
*
* @param     folderOpts - NUM_OF_FOLDERS \tNUM_OF_SUBFOLDERS Folder\t[NUM_OF_SUBFOLDERS Folder\t...]
*
* @retval    tchar *          pointer to non parsed string
*
*//*=========================================================================*/
static tchar *MBX_ParseFoldersR(
        tchar           *folderOpts, 
    OUT tchar           *fullPath,
    OUT array_t         *folders,
    int                  level)
{
    MBX_FOLDERS_STRUCT *folder;

    tchar *pos;
    tchar *posName;
    tchar *newFullPath;
    int    numSubfolders, i;

    pos = StrChr(folderOpts, _T(' '));
    if (pos == NULL)
    {
        return NULL;
    }

    *pos = _T('\0');
    pos++;
    posName = pos;

    pos = StrChr(pos, _T('\t'));
    if (pos == NULL)
    {
        return NULL;
    }

    *pos = _T('\0');
    pos++;

    numSubfolders = atoi(folderOpts);
    newFullPath = StrNewCopy(fullPath);

    if (level > 1)
        StrAppend(newFullPath, MBX_FOLDER_SEP_OLD);

    if (level > 0)
    {
        StrAppend(newFullPath, BU_EncodePath(posName));

        /* Store folder to the struct */
        folder = array_push(folders,NULL);
        folder->name = StrNewCopy(posName);
        folder->path = StrNewCopy(newFullPath);
        folder->level = level;
    }

    for (i = 0; i < numSubfolders; i++)
    {
        pos = MBX_ParseFoldersR(pos, newFullPath, folders, level+1);
        if (pos == NULL)
        {
            break;
        }
    }

    FREE(newFullPath);

    return pos;
}

/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Function parses folder names from -baropt string and returns them in a form of
*            MBX_FOLDERS_STRUCT.
*
* @remarks   MBX_FOLDERS_STRUCT struct should be freed by the caller.
*
*//*=========================================================================*/
static int MBX_ExtractFolders(
    tchar    *ovopt,
    array_t  *folders,
    IntegObj *prevObj)
{
    ERH_FUNCTION(_T("MBX_ExtractFolders"));

    int iRet = OBE_OK;
    int lvl  = 0;

    tchar *pos, *pos2;
    tchar *tmpFolders = NULL;
    tchar *token      = NULL;
    tchar *fullpath   = NULL;
    tchar  folder[STRLEN_1K + 1] = {0};

    BG_DBGENTRY;

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE, _T("%s:: -baropt string: %s"), __FCN__, ovopt);

    if (StrIsEmpty(ovopt))
        BG_RETURN(iRet);

    /* Replace \v (double quote replacement) by double quotes */
    StrReplace (ovopt, _T('\v'), _T('"'));


    /* Extract folder names from -baropt string: -baropt "extrenal = {<folder names>};" */
    pos = StrStr(ovopt, _T("{"));
    if (pos == NULL)
    {
        DbgStamp(BG_DBG_TRACE);
        DbgPlain(BG_DBG_TRACE, _T("%s:: '{' not found in %s."), __FCN__, ovopt);
        BG_RETURN(iRet);
    }

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE, _T("%s:: '{' found in %s"), __FCN__, ovopt);

    pos2 = StrrChr(ovopt, _T('}'));
    if (pos2 == NULL)
    {
        DbgStamp(BG_DBG_TRACE);
        DbgPlain(BG_DBG_TRACE, _T("%s:: '}' not found in %s."),    __FCN__, ovopt);
        BG_RETURN(iRet);
    }

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE, _T("%s:: '}' found in %s"), __FCN__, ovopt);

    /* Copy <folder names> */
    tmpFolders = StrNNewCopy(pos + 1, pos2 - pos -1);

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE, _T("%s:: Extracted folder names from -baropt: %s"), __FCN__, tmpFolders);

    token = StrStr(tmpFolders, MBX_EXTERNAL_OPTS_FORMAT_VER);
    if (token != tmpFolders)
    {
        /* Parse folder names from string */
        token = StrTok(tmpFolders, MBX_FOLDER_SEP_OLD);

        while (token != NULL)
        { 
            /* Check if double separator:
            Calendar\t\tContats
            ^ <- token
            */
            if (tmpFolders[token-tmpFolders-1] == MBX_FOLDER_SEP_CHAR_OLD)
            {
                /* New root folder */
                MBX_FOLDERS_STRUCT *dir = array_push(folders,NULL);

                /* Store folder to the struct */
                dir->name = StrNewCopy(folder);
                dir->path = StrNewCopy(fullpath);
                dir->level = lvl;

                FREE(fullpath);
                lvl=0;
            }

            /* Store folder's path in a form folder1\tfolder2\tfolder3... */
            if (fullpath != NULL)
            {
                StrAppend(fullpath, MBX_FOLDER_SEP_OLD);
            }

            StrAppend(fullpath, BU_EncodePath(token));
            lvl++;

            DbgStamp(BG_DBG_TRACE);
            DbgPlain(BG_DBG_TRACE, _T("%s:: Folder: %s, Level: %d, Full path: %s"), __FCN__, token, lvl, fullpath);

            StrCpy(folder, token);

            token = StrTok(NULL, MBX_FOLDER_SEP_OLD);
        }

        if (!StrIsEmpty(folder))
        {
            /* Add the last folder to the struct */
            MBX_FOLDERS_STRUCT *dir = array_push(folders,NULL);
            dir->name  = StrNewCopy(folder);
            dir->path  = StrNewCopy(fullpath);
            dir->level = lvl;
        }
    }
    else
    {
        DbgStamp(BG_DBG_TRACE);
        DbgPlain(BG_DBG_TRACE, _T("%s:: Parse folders using: %s"), __FCN__, MBX_EXTERNAL_OPTS_FORMAT_VER);
        token += StrLen(MBX_EXTERNAL_OPTS_FORMAT_VER) + 1;
        token = MBX_ParseFoldersR(token, NULL, folders, 0);
        DbgStamp(BG_DBG_TRACE);
        DbgPlain(BG_DBG_TRACE, _T("%s:: Remainder: %s"), __FCN__, token ? token : _T("NULL"));
    }

    FREE(tmpFolders);

    BG_RETURN(iRet);
}

/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @remarks   MBX_FOLDERS_STRUCT struct should be freed by the caller.
*
*//*=========================================================================*/
#define cached (ThisBarGui->MBX_ExtractFolders_cached)

static int MBX_ExtractFolders2(
    int       dbsm,
    tchar    *sessName,
    array_t  *folders,
    IntegObj *prevObj)
{
    USES_BARGUI_PTD
    MBX_FOLDERS_STRUCT *dir;
    int         iRet    = OBE_OK;
    query_t     query   = {0};
    int isPrivate=-1;

    ERH_FUNCTION(_T("MBX_ExtractFolders2"));

    DbgFcnIn();

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE, 
        _T("[%s] About to FUN_LISTOVEROFAPPBACKUP for %s, hostname=%s, appname=%s"), 
        __FCN__, sessName, prevObj->hostname, OB2_APP_MBX);

    query_open(&query, dbsm, FUN_LISTOVEROFAPPBACKUP, _T("%s%s%s"),
        sessName,
        prevObj->hostname,
        OB2_APP_MBX
    );

    iRet = query.status;

    if (0 != iRet)
    {
        DbgPlain(BG_DBG_TRACE, _T("[%s] FUN_LISTOVEROFAPPBACKUP failed with %d."),
            __FCN__, iRet);
        goto exit;
    }

    if (!cached)
    {
        tchar * idStringCopy = NULL;

        tchar *prevMbxName=StrChr(prevObj->objectname+1 ,_T('/'));
        tchar *prevMbxNameEnd=StrChr(++prevMbxName, _T('/'));
        prevMbxName=StrNNewCopy(prevMbxName, prevMbxNameEnd-prevMbxName);            

        while (query_read(&query))
        {
            tchar *mbxName=StrChr(*(query.record), _T('/'));
            tchar *mbxNameEnd=NULL;
            if(!mbxName)continue;
            mbxName=StrChr(++mbxName, _T('/'));
            if(!mbxName)continue;
            mbxNameEnd=StrChr(++mbxName, _T('/'));
            if(!mbxNameEnd)continue;
            mbxName=StrNNewCopy(mbxName, mbxNameEnd-mbxName);
            if(!StrCmp(mbxName, prevMbxName))
            {
                const tchar *idString = query.record[FLD_OV_ID + NR_OBJ];
                idStringCopy = StrNewCopy(idString);
                if(StrCmp(mbxName,MBX_PUBLIC_OBJ))isPrivate=1;
                FREE(mbxName);
                break;
            }
            FREE(mbxName);
        }

        query_close(&query);

        /// now run FUN_LISTCATALOG, to see what was backed up
        query_open(&query, dbsm, FUN_LISTCATALOG, _T("%s"), idStringCopy);
        FREE(idStringCopy);
        //query_open(&query, dbsm, FUN_FBLISTDIR, _T("%d"), id);
        iRet = query.status;
        if (0 != iRet)
            goto exit;
    }

    while (cached ?  0 : query_read(&query))
    {
        tchar  guid[STRLEN_STD];
        tchar  mbxName[STRLEN_STD];
        int    i               = 0;

        tchar *pos, *folder, *folderDec, *fullPath  = NULL;
        IntegObjItem    item            = {0};

        int             neededL         = prevObj->level - isPrivate;   /// 2 is root; we need root+1
        int             cLvl            = 0;

        IntegObjItemInit(&item);
        IntegObjItemUnpack(&item, query.record);

        if (item.type != OBJ_DIR)
            goto next;

        if (2 != sscanf(item.name, _T("/%[^/]/%[^/]/"), guid, mbxName))
        {
            DbgStamp(DBG_UNEXPECTED);
            DbgPlain(DBG_UNEXPECTED,_T("[%s] Not enough data in item name: %s"),
                __FCN__, item.name);
            iRet=OBE_ERR;
            goto exit;
        }
        pos = StrChr(item.name + 1, MBX_FOLDER_SEP_CHAR) + 1;
        pos = StrChr(pos + 1, MBX_FOLDER_SEP_CHAR);

        /// pos now points to root folder 
        folder = StrTok(pos, MBX_FOLDER_SEP);
        cLvl++;

        fullPath = StrNewCopy(folder);

        while (folder && (cLvl < neededL))
        {
            if (NULL == (folder = StrTok(NULL, MBX_FOLDER_SEP)))
                goto next;
            cLvl++;
            StrAppend(fullPath, MBX_FOLDER_SEP);
            StrAppend(fullPath, folder);
        }

        folderDec = StrNewCopy(BU_DecodePath(folder));

        /* has this already been added? */
        array_iterate (folders, i, dir)
        {
            if ((0 == StrCmp(dir->name, folderDec)) &&
                (0 == StrCmp(dir->path, fullPath))  &&
                (cLvl == dir->level))
            {
                goto next;
            }
        }

        /* does the parent folder match?; root folders have no parent.. anything goes */
        if (neededL > 1)
        {
            tchar       *p      = NULL;

            p = StrrChr(fullPath, MBX_FOLDER_SEP_CHAR);
            if (p)
            {
                int ret = 0;

                *p = _T('\0');
                ret = StrCmp(fullPath, prevObj->description);
                *p = MBX_FOLDER_SEP_CHAR;
                if (ret != 0)
                    goto next;
            }
        }


        dir = array_push(folders,NULL);
        dir->name    = folderDec;
        dir->path    = fullPath;
        dir->level   = cLvl;
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Added %s to list. Num of is now %d."),
            __FCN__, NS(dir->path), array_length(folders));


next:
        ;
    }

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] %d Folders are extracted"), __FCN__, array_length(folders));
exit:

    BG_RETURN(iRet);
}


/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Based on selSessionID parameter function creates restore chain and returns
*            version indexes for versions that need to be restored
*
* @remarks   idx should be freed by the caller.
*
*//*=========================================================================*/
static int MBX_CreateRestoreChain (
    IntegVer **versions, 
    int        versionsCount, 
    tchar     *selSessionID, 
    int       *number, 
    int      **idx)
{

    ERH_FUNCTION(_T("MBX_CreateRestoreChain"));
    int iRet = OBE_OK;
    int n;
    char isINCR1 = 0;

    BG_DBGENTRY;
    *number = 0;
    *idx = NULL;

    DbgPlain(BG_DBG_TRACE,_T("[%s] Selected sessionID: %s. Searching from %d versions"), __FCN__, selSessionID, versionsCount);

    /* Find position of selected version in a list of versions */
    for (n = 0; n < versionsCount; n++)
    {
        if(StrCmp(versions[n]->sessionname, selSessionID) != 0)
        {
            DbgPlain(BG_DBG_TRACE, _T("%s:: Session %s from list of versions doesn't match selected sessionID."),
                __FCN__, versions[n]->sessionname
            );
        }
        else
        {
            (*number)++;
            *idx = REALLOC(*idx, sizeof(int) * (*number));
            (*idx)[(*number) - 1] = n;

            DbgStamp(BG_DBG_TRACE);
            DbgPlain(BG_DBG_TRACE, _T("%s:: Session %s added to the list."), __FCN__, versions[n]->sessionname);
            break;
        }
    }

    if (*idx != NULL && versions[(*idx)[0]]->backuptype != BT_FULL)
    {
        /// n is already set to the idx of selected version; it can't be less then that
        for (n++; n < versionsCount; n++)
        {
            switch (versions[n]->backuptype)
            {
                case BT_INCR1:
                    isINCR1 = 1;
                    break;

                case BT_FULL:
                    isINCR1 = 0;
            }           

            if (!isINCR1)
            {
                (*number)++;
                *idx = REALLOC(*idx, sizeof(int) * (*number));
                (*idx)[(*number) - 1] = n;
            }

            if (BT_FULL == versions[n]->backuptype)
                break;
        } /// for
    } /// if

#if 0
    {
        /* Create restore chain from list of versions; start with selected version */
        for (n = (*idx)[0] + 1; n < versionsCount - 1; n++)
        {
            if(prevVersion == BT_FULL)
            {
                /* Full backup type found, restore chain created */
                DbgStamp(BG_DBG_TRACE);
                DbgPlain(BG_DBG_TRACE,_T("%s:: Backup type for version %s is FULL"), __FCN__, versions[n]->sessionname);
                break;
            }

            if (prevVersion == BT_INCR1)
            {
                /* Skip all versions between Incr1 and Full */
                DbgStamp(BG_DBG_TRACE);
                DbgPlain(BG_DBG_TRACE, _T("%s:: Skipping %s (Backup type: %d)"),
                    __FCN__, versions[n]->sessionname, versions[n]->backuptype);
                continue;
            }

            /* Add the version to the list, selected version has beeen added already */
            if (n != 0)
            {
                (*number)++;
                *idx = REALLOC(*idx, sizeof(int) * (*number));
                (*idx)[(*number) - 1] = n;
            }

            prevVersion = versions[n]->backuptype;
        }
    }
#endif

    DbgStamp(BG_DBG_TRACE);
    DbgPlain(BG_DBG_TRACE,_T("%s:: Restore chain:"), __FCN__);
    for (n = 0; n < *number; n++)
    {
        DbgPlain(BG_DBG_TRACE, _T("%s:: Session: %s, Backup type: %ld"),
            __FCN__, versions[(*idx)[n]]->sessionname, versions[(*idx)[n]]->backuptype
        );
    }

    BG_RETURN(iRet);
}


/*========================================================================*//**
*
* @retval    OBE_OK          everything is okay
*
* @brief     Return list of folders for particular level (prevObj->level),
*            selected version and restore chain
*
*//*=========================================================================*/
int MBX_BrowseFolders (
    int         dbsm,
    Integ      *integration,
    IntegObj   *prevObj,
    IntegObj ***IntegObjArray,
    int        *integObjCount,
    IntegVer  **versions,
    int         versionsCount,
    IntegVer   *selectedVersion,
    BOOL        restoreChain)
{

    ERH_FUNCTION(_T("MBX_BrowseFolders"));
    array_t folders = array_new(ARR_TYPE_COPY, sizeof(MBX_FOLDERS_STRUCT), MBX_FOLDERS_FREE);
    MBX_FOLDERS_STRUCT *dir;

    array_t objects = array_new(ARR_TYPE_COPY, sizeof(IntegObj), NULL);

    int iRet = OBE_OK;

    int *idxVer = NULL;
    int countVer = 0;

    IntegObj *io = NULL;
    int i;
    int stType=-1;
    int neededLevel=-1;

    BG_DBGENTRY;

    BG_CheckArgs( selectedVersion!=NULL );

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] OvOpts is: %s"), __FCN__, selectedVersion->ovopt);

    if (NULL == StrStr(selectedVersion->ovopt, GUID_INTEG_MBX))
    {
        /// old way of doing things
        MBX_ExtractFolders(selectedVersion->ovopt, &folders, prevObj);
    }
    else
    {
        /// new way of doing things
        MBX_ExtractFolders2(dbsm, selectedVersion->sessionname, &folders, prevObj);
    }

    DbgPlain(DBG_MAIN_ACTION, _T("prevObj->objectName=%s"), NS(prevObj->objectname));

    //JGUI (Commented by bz): fflush locks up, because of BBC library 
    //fflush(NULL);
    if(!StrNCmp(prevObj->name, MBX_TYPE_PRIVAT_STR, StrLen(MBX_TYPE_PRIVAT_STR)))
    {
        stType=MBX_TYPE_PRIVAT;
    }
    else if (!StrNCmp(prevObj->name, MBX_TYPE_PUBLIC_STR, StrLen(MBX_TYPE_PUBLIC_STR)))
    {
        stType=MBX_TYPE_PUBLIC;
    }
    else
        stType=prevObj->options.MBXLocal.restore.mbxType;

    neededLevel=(stType==MBX_TYPE_PRIVAT)?3:1;

    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Storage type = %d"), __FCN__, stType);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Previous object name = %s"), __FCN__, prevObj->name);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] mailbox name name = %s"), __FCN__, prevObj->options.MBXLocal.restore.mbxName);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Previous object type = %d"), __FCN__, prevObj->options.MBXLocal.restore.mbxType);
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] Add folders of level %d"), __FCN__, prevObj->level-neededLevel+2);

    if (restoreChain == TRUE)
    {
        int i = 0;

        MBX_CreateRestoreChain(versions, versionsCount, selectedVersion->sessionname, &countVer, &idxVer);
        for (i = 0; i < countVer; i++)
        {
            array_t verFolders = array_new(ARR_TYPE_COPY, sizeof(MBX_FOLDERS_STRUCT), MBX_FOLDERS_FREE);
            int j;
            MBX_FOLDERS_STRUCT *verFolder;

            if (NULL == StrStr(selectedVersion->ovopt, GUID_INTEG_MBX))
            {
                /// old way of doing things
                MBX_ExtractFolders(versions[idxVer[i]]->ovopt, &verFolders, prevObj);
            }
            else 
            {
                /// new way of doing things
                MBX_ExtractFolders2(dbsm, versions[idxVer[i]]->sessionname, &verFolders, prevObj);
            }

            /// add folder from restore chain to selected version's folders in not already there
            array_iterate (&verFolders, j, verFolder)
            {
                int n;
                MBX_FOLDERS_STRUCT *me;
                BOOL isThere = FALSE;

                array_iterate (&folders, n, me)
                {
                    if (0 == StrCmp(verFolder->path, me->path))
                    {
                        isThere = TRUE;
                        break;
                    }
                }
                if (!isThere)
                {
                    DbgPlain(DBG_MAIN_ACTION, _T("%s] Adding %s to folder list."),
                        __FCN__, NS(verFolder->path));

                    me = array_push (&folders,NULL);
                    me->level = verFolder->level;
                    me->name  = StrNewCopy(verFolder->name);
                    me->path  = StrNewCopy(verFolder->path);
                }
            }

            array_free (&verFolders);
        }
        FREE(idxVer);
    }

    /* copy data from MBX_FOLDERS_STRUCT array to IntegObj */
    array_iterate(&folders, i, dir)
    {
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Check %d Folder=%s, type=%d, level:%d"), __FCN__, i, dir->name, stType, dir->level);
        if (dir->level != prevObj->level-neededLevel+2)
            continue;


        /* Add folder to the list either:
            - if its path corresponds to the previous object
            - previous object is level 2 (mailbox) 0(public) */
        DbgPlain(DBG_MAIN_ACTION, _T("[%s] Object name=%s, type=%d"), __FCN__, dir->name, stType);

        if (prevObj->level!=neededLevel-1 &&
            StrNICmp(dir->path, prevObj->description, StrLen(prevObj->description)) != 0)
        {
            continue;
        }

        io = array_push(&objects, NULL);

        /* fill the IntegObj structure */
        IntegObjInit(io, integration->apptype);

        io->appname  = StrNewCopy(integration->appname);
        io->hostname = StrNewCopy(integration->hostname);
        /* Folder name */
        io->name = StrNewCopy(dir->name);
        /*We do not need real object name. This part will be enouh to get mailbox name*/
        io->objectname = StrNewCopy(prevObj->objectname);
        /* Folder's full path */
        io->description = StrNewCopy(dir->path);
        /* -baropt string */
        io->realname = StrNewCopy(selectedVersion->ovopt);
        /* Remember mailbox name that folder belongs to */
        io->options.MBXLocal.restore.mbxType=stType;
        if (prevObj->level == neededLevel-1)
        {
            io->options.MBXLocal.restore.mbxName = prevObj->name;
        }
        else
        {
            io->options.MBXLocal.restore.mbxName = prevObj->options.MBXLocal.restore.mbxName;
        }

        io->prev = prevObj;
        io->level= prevObj->level + 1;

        DbgPlain(BG_DBG_TRACE,_T("%s:: (Level %d) Folder %s type %d is added to the object list. ."),
            __FCN__, io->level, io->name, io->options.MBXLocal.restore.mbxType);

    }

    array_free (&folders);

    *integObjCount = array_length(&objects);
    *IntegObjArray = (void*)objects.data;
    DbgPlain(DBG_MAIN_ACTION, _T("[%s] IntegObjCount=%d"), __FCN__, *integObjCount);
    BG_RETURN(iRet);
}



/* ===========================================================================
|   Describe integration
 =========================================================================== */
IInteg *MBX_GetParams()
{
    static IInteg i = {0};
    i.type = OB2_BAR_MBX;
    i.name = OB2_APP_MBX;
    i.mount.unpack   = MBX_UnpackMount;
    i.config.execute = MBX_Config;
    i.config.unpack  = MBX_UnpackConfig;
    i.config.check   = (bargui_checkconfig_f)MBX_CheckConfig;
    i.config.appname = _T("SingleMailbox");
    i.config.dtor    = MBX_IntegConfigFree;
    i.schInfo        = schConfigType[SchMBX];
    i.enumInstances  = MBX_EnumInstances;
    i.treeDepth      = 999;

    return &i;
}

/* === EOF === */
