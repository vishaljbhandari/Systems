/**
*
* (c) Copyright 1993-2009, 2019 Micro Focus or one of its affiliates.
*
* @ingroup   Catalog Manager (Cat)
* @file      ma/xma/catalog.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Catalog Manager
*
*/


#include "lib/cmn/target.h"

#if !(TARGET_NETWARE)
#include <time.h>
#include <string.h>
#endif


#define HPUX_VER 1020

#include "catalog.h"
#include "catalog_write.h"

#include "lib/cmn/common.h"
#include "lib/ipc/ipc.h"
#include "lib/parse/wlstruct.h"
#include "lib/parse/wlparse.h"
#include "lib/parse/wllist.h"

#include "ma/util/mautil.h"
#include "ma/dev/maxfm.h"
#include "ma/dev/filemark.h"

#include "ma.h"

#if TARGET_HW_TAPE_ENCRYPTION

#include "ma/dev/devencr.h"
#include "ma/xma/mahdr.h"

#endif

#if DA_ENCRYPT_KEY_CATALOG
#include "keycat.h"
#endif

#if XMA_NDMP
#if JSONIZER_SUPPORTED_PLATFORM
#include "lib/jsonizer/libjsonizer/jsonizer.h"
#endif
#endif

#ifndef lint
static tchar rcsId[] = _T("$Header: /ma/xma/catalog.c $Rev$ $Date::                      $:") ;
#endif

#if XMA_NDMP
    NdmpCatChunkLst *chunkList = NULL;
    int ndmpCatChunk = 0;
    UCHAR   *tmpBuf = NULL;
    MArecDA *tmpDArec = NULL;
    MArecSess *tmpSessRec = NULL;
    int firstChunk = 1;
    static void ProcessChunk(const int firstChunk);

    static struct {
        tchar *sessID;
        tchar *sessOwner;
        tchar *sessDesc;

        tchar *boName; // 4
        tchar *boDesc;
        tchar *appName;
        tchar *appDesc;
    } HEAD = {0};
    
#endif


/* ---------------------------------------------------------------------------
|
|    Module Globals
|
 -------------------------------------------------------------------------- */
static Catalog m_catalog = {NULL, NULL, 0, 0, 0};
Catalog *g_cat = &m_catalog;


/* -- Static func declarations ------------------------------------------ */

static void CatalogList_CatSendRecSessToCON(const MArecSess *rec);

#ifdef UNICODE
    static const int UNICODE_BUILD = 1;
#else
    static const int UNICODE_BUILD = 0;
#endif

/* ===========================================================================
| @brief    convert string stored in record to tchar, while respecting string length
| @param    nflags - record flags in network byte order
| @param    len    - string length, in host byte order. string need not be nul-terminated
|
| @note     Conversion:
|           a) non unicode record: StrNewCopyS2T
|           b) unicode record: StrNewCopyW2T_MA which is:
|              - no conversion for unicode build
|              - W2T = StrCopyW2A = W->ACP on windows, W->utf8 elsewhere
|              non-win used to be W->ACP until DP5.5 (trunk rev 70)
|              if we asume windows build will always be unicode, rule is W->S everywhere
|
|
| @note     Length attributes (e.g. boNameLength) didnt always mean the same:
|           - NDMP on DP10x          : len = (string len+2)*sizeof(tchar)
|           - other records on DP10x : len = (string len+1)*sizeof(tchar) aka StrSize
|           - very old records (from DP2x) have len=string len
|           Thus we use strnlen approach - respect NUL char or length, whichever comes first
 =========================================================================== */
static ctchar*
StrnR2T(const void *in, ULONG nflags, USHORT len)
{
    BOOL u = (ntohl(nflags) & FLAG_UNICODE) != 0;
    int dsize;
    tchar *out;

    if (u) // windows records came later in DP history and were always nul terminated
    {
        if (UNICODE_BUILD) return in;
        len /= sizeof(USHORT);
        if (len) len--;
        out = CmnGetTmpStr(NULL, dsize = len*3);
        StrCopyW2TS(out, in, dsize);
    }
    else
    {
        // recent DP versions have len=strsize(name) and NUL char
        // ancient versions have len=strlen(name) and no NUL char
        const char *c = (const char*)in;
        int haveNul = len == 0 || c[len-1] == 0;
        out = CmnGetTmpStr(NULL, dsize = len);

        haveNul? StrCopyS2T(out, in, dsize) : StrNCopyS2T(out, in, dsize, len);
    }
    return out;
}


#define rec2t(rec,name) StrnR2T(\
    (char*)rec + STD2HOST_S(rec->name ## Offset), \
    rec->recHeader.recFlags, \
    STD2HOST_S(rec->name ## Length))


#if XMA_NDMP
static void CatalogList_AppendNDMPCatRecToCatBuff(const MArecNDMPCat *rec, const ULONG nrecType);
static void CatalogList_SendEnvBuffNDMP(const MArecDA *rec);
#endif

static int CatalogVerify_CloseDA(MArecDA *rec, DAInfoControlType **daInfoHead);

/****************************************************************************
|
****************************************************************************/
/****************************************************************************
| Catalog
****************************************************************************/
/****************************************************************************
|
****************************************************************************/
void Catalog_Init()
{
    g_cat->catBuf = NULL;
    g_cat->catNext = 0UL;
    g_cat->catSize = 0UL;
    g_cat->bkpType = 0; /* incorrect size of integration objects */
    g_cat->catSeqNo = 0;

    return;
}

/****************************************************************************
|
****************************************************************************/
void Catalog_Restart()
{
    ERH_FUNCTION(_T("Catalog_Restart"));

    g_cat->daHead = CatDump_UpdateDaRecs(g_cat->daHead);
    g_cat->catNext = 0UL; /*** pointer to start of catalog buffer ***/
    g_cat->catSeqNo++;
}

/****************************************************************************
|
****************************************************************************/
void Catalog_Free()
{
    ERH_FUNCTION(_T("Catalog_Free"));

    DbgPlain(DBG_MA_CATALOG, __FCN_FMT__ _T("Freeing Catalog... "), __FCN__);

    g_cat->catNext = 0UL; /*** pointer to end of catalog buffer ***/
    g_cat->catSize = 0UL; /*** allocated size of catalog ***/
    g_cat->catSeqNo = 0; /** DO NOT REMOVE THIS !!! merge from older branch does not apply here (Drago) ***/

    if (g_cat->catBuf != NULL)
    {
        FREE(g_cat->catBuf);
        g_cat->catBuf = NULL;
    }
}

/****************************************************************************
|
****************************************************************************/
int Catalog_Grow(Catalog *in_cat)
{
    ERH_FUNCTION(_T("Catalog_Grow"));

    if (in_cat->catBuf == NULL)
    {
        in_cat->catSize  = MAX_CAT_SIZE;
        in_cat->catBuf   = (UCHAR *)MALLOC(in_cat->catSize);
        if (in_cat->catBuf == NULL)
        {
            DBGSTAMP(DBG_MA_CATALOG);
            DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("MALLOC failed error==%d"), __FCN__, errno);
            return 0;
        }
        in_cat->catNext  = 0L;
    }
    else
    {
        in_cat->catSize += MAX_CAT_SIZE;
        DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("reallocating catalog space to %d bytes"), __FCN__, in_cat->catSize);
        in_cat->catBuf = (UCHAR *)REALLOC(in_cat->catBuf, in_cat->catSize);
    }

    DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("catSize = %d bytes"), __FCN__, in_cat->catSize);
    devCatalogSize(in_cat->catSize);
    return (1);
}


/****************************************************************************
|
****************************************************************************/
ULONG Catalog_GetSize()
{
    return g_cat->catNext;
}

/****************************************************************************
|
****************************************************************************/
void Catalog_Debug(const Catalog *in_cat, int level)
{
    ERH_FUNCTIONA("Catalog_Debug");
    const daRec  *da = NULL;
    const UCHAR *rec;
    
    if (!DbgMatch(level))
        return;

    DbgPlainA(level, __FCNA_FMT__ "******* CATALOG DEBUG *********** in_cat=" PTR_A_FMT, __FCNA__, in_cat);

    if (!in_cat)
    {
        DBGPLAIN(level, __FCN_FMT__ _T("cat = NULL"), __FCN__);
        return;
    }


    /*** debug DA's ***/
    DbgPlainA(level, __FCNA_FMT__ "*** OPEN DA's: daHead=" PTR_A_FMT, __FCNA__, g_cat->daHead);
    for (da = g_cat->daHead; da; da = da->next)
    {
        DbgPlainA(level, __FCNA_FMT__
            "daID=" ULONG_A_FMT "; "
            "daCC=" ULONG_A_FMT "; "
            "loc=[" ULONG_A_FMT ":" ULONG_A_FMT "]" "; "
            "overFlags=" HEX_A_FMT "; "
            "size=[" ULONG_A_FMT ":" ULONG_A_FMT "]",
            __FCNA__,
            STD2HOST_L(da->daID),
            STD2HOST_L(da->daCC),
            STD2HOST_L(da->daSegment),
            STD2HOST_L(da->daOffset),
            STD2HOST_L(da->overFlags),
            STD2HOST_L(da->boSizeHi),
            STD2HOST_L(da->boSizeLo)
        );
    }

    /*** debug content ***/
    DbgPlainA(level, __FCNA_FMT__
        "*** CATALOG CONTENT: catBuf=" PTR_A_FMT "; "
        "size=" ULONG_A_FMT  "; "
        "sequence=" INT_A_FMT, __FCNA__,
        in_cat->catBuf, in_cat->catNext, in_cat->catSeqNo
    );
        
    for (rec = in_cat->catBuf; rec - in_cat->catBuf < in_cat->catNext; rec += ROUND_UP(STD2HOST_L(((catRec*)rec)->recSize)))
    {
        const catRec *cat = (const catRec*)rec;

        switch (STD2HOST_L(cat->recType))
        {
        case CAT_REC_POSIX:
            DbgPlainA (level, "%s: [%u:%u] daID=%u; UID/GID=[%u/%u] size=[%u:%u] %s",
                __FCNA__,
                STD2HOST_L(cat->objSegment),
                STD2HOST_L(cat->objOffset),
                STD2HOST_L(cat->daID),
                STD2HOST_L(cat->objUID),
                STD2HOST_L(cat->objGID),
                STD2HOST_L(cat->objSizeHi),
                STD2HOST_L(cat->objSize),
                cat->objName
            );
            continue;

        case CAT_REC_NT:
            {
                const NTcatRec *nt  = (const NTcatRec *)rec;
                DbgPlainA (level, "%s: [%u:%u] daID=%u; size=[%u:%u] %s",
                    __FCNA__,
                    STD2HOST_L(nt->objSegment),
                    STD2HOST_L(nt->objOffset),
                    STD2HOST_L(nt->daID),
                    STD2HOST_L(nt->objSizeHi),
                    STD2HOST_L(nt->objSizeLo),
                    rec + STD2HOST_S(nt->objAsciiNameOffset)
                );
                continue;
            }

        case CAT_REC_NDMP:
            {
                const NDMPcatRec *ndmp = (const NDMPcatRec*)rec;
                const dpuint64_t offset  = IntsToLL(STD2HOST_L(ndmp->objOffsetHi), STD2HOST_L(ndmp->objOffset));
                const dpuint64_t objSize = IntsToLL(STD2HOST_L(ndmp->objSizeHi), STD2HOST_L(ndmp->objSize));

                DbgPlainA (level, "%s: [%u:%llu] daID=%u; UID/GID=[%u/%u] size=[%llu] %s",
                    __FCNA__,
                    STD2HOST_L(ndmp->objSegment),
                    offset,
                    STD2HOST_L(ndmp->daID),
                    STD2HOST_L(ndmp->objUID),
                    STD2HOST_L(ndmp->objGID),
                    objSize,
                    ndmp->objName
                );
                continue;
            }


        case CAT_REC_NDMP2:
            {
                NDMPcatRec2 *catNDMP = (NDMPcatRec2 *)rec;

                const dpuint64_t offset  = IntsToLL(STD2HOST_L(catNDMP->objOffsetHi), STD2HOST_L(catNDMP->objOffset)); 
                const dpuint64_t inode   = IntsToLL(STD2HOST_L(catNDMP->inodeHi), STD2HOST_L(catNDMP->inode));
                const dpuint64_t objSize = IntsToLL(STD2HOST_L(catNDMP->objSizeHi), STD2HOST_L(catNDMP->objSize));

                DbgPlainA (level, "%s: [%u:%llu] inode=[%llu] daID=%u; UID/GID=[%u/%u] size=[%llu] %s",
                    __FCNA__,
                    STD2HOST_L(catNDMP->objSegment),
                    offset,
                    inode,
                    STD2HOST_L(catNDMP->daID),
                    STD2HOST_L(catNDMP->objUID),
                    STD2HOST_L(catNDMP->objGID),
                    objSize,
                    catNDMP->objName
                    );
                continue;
            }   /* case CAT_REC_NDMP2 */
        default:
            DbgPlainA(level, __FCNA_FMT__ "Got invalid record type (%d)", __FCNA__, STD2HOST_L(cat->recType) );
        }
    }
}

/*****************************************************************************
|
*****************************************************************************/
/*****************************************************************************
| daRec handler
*****************************************************************************/
/*****************************************************************************
|
*****************************************************************************/
daRec *daRec_Push(daRec *in_daHead, daRec *io_daEntry)
{
    io_daEntry->next = in_daHead;
    return io_daEntry;
}

/*****************************************************************************
|
*****************************************************************************/
daRec *daRec_Remove(daRec *in_daHead, daRec *io_daEntry)
{
    ERH_FUNCTION(_T("daRec_Remove"));
    daRec *daTop = in_daHead;

    if (io_daEntry == daTop)
    {
        daTop  = io_daEntry->next;
        DBGPLAIN(DBG_MA_CATALOG_100, __FCN_FMT__ _T("entry removed from top"), __FCN__);
    }
    else
    {
        daRec   *repairQueue = NULL;

        for (repairQueue = daTop; repairQueue->next != io_daEntry; repairQueue = repairQueue->next)
        {
        }
        if (repairQueue->next == io_daEntry)
        {
            DBGPLAIN(DBG_MA_CATALOG_100, __FCN_FMT__ _T("relinking queue"), __FCN__);
            repairQueue->next = repairQueue->next->next;
        }
        else
        {
            DBGPLAIN(DBG_MA_CATALOG_100, __FCN_FMT__ _T("entry not found!"), __FCN__);
        }
    }

    FREE (io_daEntry->boName);
    FREE (io_daEntry->boDesc);
    FREE (io_daEntry->appName);
    FREE (io_daEntry->appDesc);
    FREE (io_daEntry);

    return (daTop);
}

/*****************************************************************************
|
*****************************************************************************/
daRec *daRec_Search(daRec *in_daHead, ULONG daID)
{
    daRec *p = NULL;
    for (p = in_daHead; p; p = p->next)
    {
        if (STD2HOST_L(p->daID) == daID)
            return (p);
    }
    return NULL;
}

/*****************************************************************************
|
*****************************************************************************/
void daRec_Debug(daRec *in_daHead, int level)
{
    ERH_FUNCTION(_T("daRec_Debug"));
    daRec *p = NULL;

    for (p = in_daHead; p; p = p->next)
    {
        DBGPLAIN(level, __FCN_FMT__
            _T("p=")PTR_FMT _T("; ")
            _T("p->daID=")ULONG_FMT _T("; ")
            _T("p->daCC=")ULONG_FMT _T("; ")
            _T("p->daSegment=")ULONG_FMT _T("; ")
            _T("p->daOffset=")ULONG_FMT _T("; ")
            _T("p->next=")PTR_FMT,
            __FCN__,
            p,
            STD2HOST_L(p->daID),
            STD2HOST_L(p->daCC),
            STD2HOST_L(p->daSegment),
            STD2HOST_L(p->daOffset),
            p->next
        );
    }
}

/*****************************************************************************
|
*****************************************************************************/
/*****************************************************************************
| Catalog list
*****************************************************************************/
/*****************************************************************************
|
*****************************************************************************/

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec           pointer to ma record session structure
*
* @brief     This function prints MA record session to debug
*
*//*=========================================================================*/
static void
CatPrintRecSess(const MArecSess *rec)
{
    ERH_FUNCTION(_T("CatPrintRecSess"));
    DBGPLAIN(DBG_MA_CATALOG,
        __FCN_FMT__ _T("SESS: cc=%d seq=%d time=%d:%d sess=%s own=%s desc=%s\n"), __FCN__,
        STD2HOST_L(rec->maCC),
        STD2HOST_L(rec->maCatSeqNo),
        STD2HOST_L(rec->maStartTime),
        STD2HOST_L(rec->maCurrentTime),
        rec2t(rec, sessID),
        rec2t(rec, sessOwner),
        rec2t(rec, sessDesc)
    );
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec           pointer to ma record DA structure
*
* @brief     This function prints MA record DA to debug
*
*//*=========================================================================*/
static void
CatPrintRecDA(const MArecDA *rec)
{
    ERH_FUNCTION(_T("PrintRecDA"));

    DBGPLAIN(DBG_MA_CATALOG,
        _T("DA:\tid=")ULONG_FMT _T(" cc=")ULONG_FMT _T(" pos=")ULONG_FMT _T(":")ULONG_FMT _T(" type=")ULONG_FMT _T(" lvl=")ULONG_FMT _T("\n")
        _T("\tacl=")ULONG_FMT _T(":")ULONG_FMT _T(" prot=")ULONG_FMT _T(":")ULONG_FMT _T(" time=")ULONG_FMT _T(":")ULONG_FMT _T("\n")
        _T("\tboName=%s boNameLen=%u\n")
        _T("\tboDesc=%s boDescLen=%u\n")
        _T("\tappName=%s appNameLen=%u\n")
        _T("\tappDesc=%s appDescLen=%u\n"),
        STD2HOST_L(rec->daID),
        STD2HOST_L(rec->daCC),
        STD2HOST_L(rec->daSegment),
        STD2HOST_L(rec->daOffset),
        STD2HOST_L(rec->boType),
        STD2HOST_L(rec->boLevel),
        STD2HOST_L(rec->boAccsType),
        STD2HOST_L(rec->boAccsValue),
        STD2HOST_L(rec->boProtType),
        STD2HOST_L(rec->boProtValue),
        STD2HOST_L(rec->boStartTime),
        STD2HOST_L(rec->boSinceTime),
        rec2t(rec, boName),  STD2HOST_L(rec->boNameLength),
        rec2t(rec, boDesc),  STD2HOST_L(rec->boDescLength),
        rec2t(rec, appName), STD2HOST_L(rec->appNameLength),
        rec2t(rec, appDesc), STD2HOST_L(rec->appDescLength)
    );
}


/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec           pointer to ma record DA structure
* @param     daInfoHead    pointer to first DA
*
* @retval    0             no error in closing DA
* @retval    N             number of errors in catalog block
*
* @brief     This function close the DA info if DA is completed or aborted.
*
*//*=========================================================================*/
static int CatalogVerify_CloseDA
(
    MArecDA           *rec,
    DAInfoControlType **daInfoHead
)
{

    ERH_FUNCTION(_T("CatCloseDA"));

    DAInfoControlType *p = NULL;
    DAInfoControlType *q = NULL;
    int warns = 0;

    if ((STD2HOST_L(rec->daCC) & CC_COMPLETED) || (STD2HOST_L(rec->daCC) & CC_ABORTED))
    {
        p = FindDAInfo(STD2HOST_L(rec->daID), *daInfoHead);
        if ( NULL == p)
        {
            DBGPLAIN(
                DBG_MA_WORK,
                __FCN_FMT__ _T("Can't find daID=%d in the table."), __FCN__,
                STD2HOST_L(rec->daID)
                );

            /*---------------------------------------------------------------
            NSMex05213
            Some DA(s) did not send any data into this data segment - maybe
            they hanged or did something else. Definitely this is not a
            situation where an error should be reported. There is no data
            lost, and the medium should not be marked poor. The situation
            should be ignored.

            errs += 1;
            ---------------------------------------------------------------*/

            return(warns);
        }

        if (STD2HOST_L(rec->daCC) & CC_COMPLETED)
        {
            if (p->valid == DAINFO_FINI_STATE)
            {

                DBGPLAIN(DBG_MA_WORK,
                    __FCN_FMT__ _T("DA %d has been correctlly ended. Remove it form list."), __FCN__,
                    p->daID
                    );

            }
            else
            {
                DBGPLAIN(DBG_MA_WORK,
                    __FCN_FMT__ _T("ERROR. DA %d has not been ended with DA_REC_END."), __FCN__,
                    p->daID
                    );

                ErhFullReport (
                    _T("MaVerifyMedium"),
                    ERH_WARNING,
                    NlsGetMessage (
                        NLS_SET_MA,
                        NLS_MA_VERIFY_CATALOG_DA
                        )
                    );

                warns += 1;
            }
        }
        else
        {
            DBGPLAIN(DBG_MA_WORK,
                __FCN_FMT__ _T("DA %d has been aborted."), __FCN__,
                p->daID
                );
        }

        /* Remove this DA from the list. */
        DBGPLAIN(DBG_MA_WORK, _T("List of DA:"));
        for (q = *daInfoHead; q != NULL; q = q->next)
        {
            DBGPLAIN(DBG_MA_WORK, _T("\tdaID=%d"), q->daID);
        }

        for (q = *daInfoHead; q != NULL && q != p && q->next != p; q = q->next)
        {
        }

        if (q == NULL)
        {
            DBGSTAMP(DBG_MA_WORK);
            DBGPLAIN(DBG_MA_WORK,
                __FCN_FMT__ _T("ERROR. Can't find appropriate DA list reference."), __FCN__);

            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage (
                NLS_SET_MA,
                NLS_MA_VERIFY_CATALOG_DA_LIST
                )
                );

            warns += 1;
            return(warns);
        }

        if (q == p)
        {
            /* First DA in the list. */
            *daInfoHead = p->next;

            DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("Removed daID=%d"), __FCN__, p->daID);
            FREE(p);
        }
        else
        {
            /* Overbridbge the DA in the list. */
            q->next = p->next;

            DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("Removed daID=%d"), __FCN__, p->daID);
            FREE(p);
        }
    }
    else
    {
        p = FindDAInfo(STD2HOST_L(rec->daID), *daInfoHead);
        if ( NULL == p)
        {
            DBGPLAIN(
                DBG_MA_WORK,
                __FCN_FMT__ _T("ERROR. Can't find daID=%d in the table.\n"), __FCN__,
                STD2HOST_L(rec->daCC)
                );

            ErhFullReport (
                __FCN__,
                ERH_WARNING,
                NlsGetMessage (
                    NLS_SET_MA,
                    NLS_MA_VERIFY_CATALOG_DA_TABLE,
                    STD2HOST_L(rec->daCC)
                    )
                );

            warns += 1;
            return(warns);
        }

        DBGPLAIN(DBG_MA_WORK,
            __FCN_FMT__ _T("Updated DA %d in catalog.\n"), __FCN__,
                                                  p->daID
                                                                        );
        p->valid = DAINFO_CATALOG_STATE;    /* CATALOG PROCESS state */
    }

    return(warns);
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     curSeg        current segment
* @param     curBlk        current block
* @param     bufPtr        pointer to buffer block
* @param     daInfoHead    pointer to first DA info
* @param     warns         pointer to number of warnings
*
* @retval    0             catalog block is ok
* @retval    N             number of errors in catalog block
*
* @brief     This function checks if the catalog block is correct
*
*//*=========================================================================*/
int
CatalogVerify(
    ULONG   curSeg,
    ULONG   curBlk,
    UCHAR   *bufPtr,
    DAInfoControlType **daInfoHead,
    int     *warns)
{
    ERH_FUNCTION(_T("CatalogVerify"));

    ULONG   nrecType = 0;
    int     errs = 0;
    UCHAR   *recPtr = bufPtr + ROUND_UP(sizeof(MAblkHdr));
    *warns      = 0 ;

    DBGFCNIN();

    while ((ULONG)(recPtr - bufPtr) < STD2HOST_L(((MAblkUniv *)bufPtr)->blkHeader.blkLength))
    {
        if (((MArecUniv *)recPtr)->recHeader.magicCookie
            != HOST2STD_L(MA_REC_MAGIC_COOKIE))
        {
            ErhFullReport (
                _T("CatalogVerify"),
                ERH_MAJOR,
                NlsGetMessage (
                    NLS_SET_MA,
                    NLS_MA_INV_REC_HEADER,
                    curSeg, curBlk
                )
            );

            DBGFCNOUT_DEC(errs + 1);
            return (errs + 1);
        }

        nrecType = STD2HOST_L(((MArecUniv *)recPtr)->recHeader.recType);

        switch (nrecType)
        {
            case MA_REC_SESS:
            {
                MArecSess       *rec = (MArecSess *)recPtr;

                CatPrintRecSess(rec);
                break;
            }
            /* case MA_REC_SESS */

            case MA_REC_DA:
            {
                MArecDA     *rec = (MArecDA *)recPtr;

                CatPrintRecDA(rec);

                /* Close the DA info if this DA is completed or aborted. */
                *warns = CatalogVerify_CloseDA(rec, daInfoHead);

                break;
            }
            /* case MA_REC_DA */

#if   DA_ENCRYPT_KEY_CATALOG
            case MA_REC_DA_ENCR:
            {
                /* No need to verify anything for MA_REC_DA_ENCR record */
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("CAT: (MA_REC_DA_ENCR)"), __FCN__);
            }
            break;

            case MA_REC_ENCR :
            {
                /* No need to verify anything for MA_REC_ENCR record */
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("CAT: (MA_REC_ENCR)"), __FCN__);
            }
            break;
#endif

            case MA_REC_CAT:
            {
                MArecCat    *rec = (MArecCat *)recPtr;
                DBGPLAINA(DBG_MA_CATALOG,
                    "%d [%02d:%04d] %d/%d\t%d:%07d %s",
                    STD2HOST_L(rec->daID),
                    STD2HOST_L(rec->objSegment),
                    STD2HOST_L(rec->objOffset),
                    STD2HOST_L(rec->objUID),
                    STD2HOST_L(rec->objGID),
                    STD2HOST_L(rec->objSizeHi),
                    STD2HOST_L(rec->objSize),
                    (char *)rec + STD2HOST_S(rec->objNameOffset)
                );
                break;
            }
            /* case MA_REC_CAT */

            case MA_REC_CAT_DATA:
            {
                MArecCatData *rec = (MArecCatData *)recPtr;

                DBGPLAIN(DBG_MA_CATALOG,
                    _T("%d [XX:XX] %d 0x%X %d"),
                    STD2HOST_L(rec->daID),
                    STD2HOST_L(rec->dataSeq),
                    STD2HOST_L(rec->flags),
                    STD2HOST_L(rec->dataLength)
                );
                break;
            }
            /* case MA_REC_CAT_DATA */

#if XMA_NDMP
            case MA_REC_NDMP_CAT:
            case MA_REC_NDMP2_CAT:
            {
                MArecNDMPCat    *rec = (MArecNDMPCat *)recPtr;
                char *objName = (char *)rec + STD2HOST_S(rec->objNameOffset);

                if(nrecType == MA_REC_NDMP_CAT)
                {
                    DBGPLAINA(DBG_MA_CATALOG,
                        "%d [%02d:%u:%u] %d/%d\t%d:%07d %s",
                        STD2HOST_L(rec->daID),
                        STD2HOST_L(rec->objSegment),
                        STD2HOST_L(rec->objOffsetHi),
                        STD2HOST_L(rec->objOffset),
                        STD2HOST_L(rec->objUID),
                        STD2HOST_L(rec->objGID),
                        STD2HOST_L(rec->objSizeHi),
                        STD2HOST_L(rec->objSize),
                        objName
                    );
                }
                else
                {
                    DBGPLAINA(DBG_MA_CATALOG,
                        "%d [%02d:%u:%u] %d/%d\t%d:%07d\t%u:%u %s",
                        STD2HOST_L(rec->daID),
                        STD2HOST_L(rec->objSegment),
                        STD2HOST_L(rec->objOffsetHi),
                        STD2HOST_L(rec->objOffset),
                        STD2HOST_L(rec->objUID),
                        STD2HOST_L(rec->objGID),
                        STD2HOST_L(rec->objSizeHi),
                        STD2HOST_L(rec->objSize),
                        STD2HOST_L(rec->inodeHi),
                        STD2HOST_L(rec->inode),
                        objName
                    );
                }
                break;
            }
#endif

            case MA_REC_NT_CAT:
            {
                MArecNTCat  *rec = (MArecNTCat *)recPtr;
                DBGPLAINA(DBG_MA_CATALOG,
                    "%d [%02d:%04d] %d:%07d\t%s",
                    STD2HOST_L(rec->daID),
                    STD2HOST_L(rec->objSegment),
                    STD2HOST_L(rec->objOffset),
                    STD2HOST_L(rec->objSizeHi),
                    STD2HOST_L(rec->objSizeLo),
                    (char *)rec + STD2HOST_S(rec->objAsciiNameOffset)
                );
                break;
            }
            /* case MA_REC_NT_CAT */

            case MA_REC_NULL:
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("CAT: (null)"), __FCN__);
                break;

            default:
                ErhFullReport (
                    _T("CatalogVerify"),
                    ERH_MAJOR,
                    NlsGetMessage (
                        NLS_SET_MA,
                        NLS_MA_INV_REC_HEADER,
                        curSeg, curBlk
                    )
                );

                DBGFCNOUT_DEC(errs + 1);
                return (errs + 1);
                /* default */
        }   /* switch (STD2HOST_L(((MArecUniv *)recPtr) ... */

        recPtr += ROUND_UP(STD2HOST_L(((MArecUniv *)recPtr)->recHeader.recSize));
    }

    DBGFCNOUT_DEC(errs);
    return (errs);
}   /* CatalogVerify */

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param
*
* @retval
*
* @brief
*
* @remarks
*
* @note
*
*//*=========================================================================*/

static int
Catalog_Create_MArecSess(
    MArecSess *rec,
    ULONG    recSize,
    ULONG    catSeqNo,
    time_t   startTime,
    ctchar  *sessID,
    ctchar  *sessOwner,
    ctchar  *sessDesc,
    int      sessType)
{
    ERH_FUNCTION (_T("Catalog_Create_MArecSess"));
    UCHAR       *ptr = NULL;
    ULONG       reqSize = 0UL;

    DBGFCNINLOW();

    reqSize = (ULONG) ( sizeof(MArecSess)
        + (strlen(sessID)   + 1)*sizeof(tchar)    /* +1 for ASCIIZ */
        + (strlen(sessOwner)+ 1)*sizeof(tchar)    /* +1 for ASCIIZ */
        + (strlen(sessDesc) + 1)*sizeof(tchar) ); /* +1 for ASCIIZ */

    if (recSize < reqSize)
    {
        DBGFCNOUTLOW();
        return (0);
    }

    rec->recHeader.magicCookie  = HOST2STD_L(MA_REC_MAGIC_COOKIE);
    rec->recHeader.recType      = HOST2STD_L(MA_REC_SESS);
    rec->recHeader.recSubType   = HOST2STD_L(MA_REC_NULL);
    rec->recHeader.recVersion   = HOST2STD_L(MA_REC_VERSION);
    rec->recHeader.recSeqNo     = HOST2STD_L(0L);       /* Not Needed ? */
    rec->recHeader.recSize      = HOST2STD_L(recSize);
#if (TARGET_WIN32) && defined(UNICODE)                  /* UNICODE flag !!! */
    rec->recHeader.recFlags     = HOST2STD_L(1L);
#else
    rec->recHeader.recFlags     = HOST2STD_L(0L);
#endif

    rec->maCC                   = HOST2STD_L(CC_COMPLETED);       /* Not Needed ? */
    rec->maCatSeqNo             = HOST2STD_L(catSeqNo);
    rec->maStartTime            = HOST2STD_L(time_t_2_ULONG(startTime));
    rec->maCurrentTime          = HOST2STD_L(time_t_2_ULONG(time(NULL)));
    rec->sessType               = HOST2STD_L(sessType);

    ptr = (UCHAR *)rec + sizeof(MArecSess);

    /*  sprintf ((char *)ptr, "%s", sessID); */
    memcpy ( (UCHAR *)ptr, sessID, strlen(sessID)*sizeof(tchar));
    rec->sessIDLength           = HOST2STD_S((USHORT)((strlen(sessID)+1)*sizeof(tchar)));
    rec->sessIDOffset           = HOST2STD_S((USHORT)(ptr - (UCHAR *)rec));
    /* we don't write '\0' because we assume that blkBuffer is memset to 0 */
    ptr += STD2HOST_S(rec->sessIDLength);

    /*  sprintf ((char *)ptr, "%s", sessOwner);*/
    memcpy ( (UCHAR *)ptr, sessOwner, strlen(sessOwner)*sizeof(tchar));
    rec->sessOwnerLength        = HOST2STD_S((USHORT)((strlen(sessOwner)+1)*sizeof(tchar)));
    rec->sessOwnerOffset        = HOST2STD_S((USHORT)(ptr - (UCHAR *)rec));
    /* we don't write '\0' because we assume that blkBuffer is memset to 0 */
    ptr += STD2HOST_S(rec->sessOwnerLength);

    /*  sprintf ((char *)ptr, "%s", sessDesc);*/
    memcpy ( (UCHAR *)ptr, sessDesc, strlen(sessDesc)*sizeof(tchar));
    rec->sessDescLength         = HOST2STD_S((USHORT)((strlen(sessDesc)+1)*sizeof(tchar)));
    rec->sessDescOffset         = HOST2STD_S((USHORT)(ptr - (UCHAR *)rec));
    /* we don't write '\0' because we assume that blkBuffer is memset to 0 */
    ptr += STD2HOST_S(rec->sessDescLength);

    ErhAssert (__FCN__, (ULONG)(ptr - (UCHAR *)rec) <= recSize + 1);

    DBGFCNOUTLOW();
    return (1);
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param
*
* @retval
*
* @brief
*
* @remarks
*
* @note
*
*//*=========================================================================*/
int
CatSendSessRec(time_t  startTime,
               ctchar *sessID,
               ctchar *sessOwner,
               ctchar *sessDesc,
               int     sessType)
{
    ERH_FUNCTION (_T("CatSendSessRec"));
    MArecSess   *rec = NULL;
    ULONG       reqSize = 0UL;

    DBGFCNINLOW();
    reqSize = (ULONG) ( sizeof(MArecSess)
        + (strlen(sessID)   + 1)*sizeof(tchar)    /* +1 for ASCIIZ */
        + (strlen(sessOwner)+ 1)*sizeof(tchar)    /* +1 for ASCIIZ */
        + (strlen(sessDesc) + 1)*sizeof(tchar) ); /* +1 for ASCIIZ */

    rec = (MArecSess   *)MALLOC(reqSize);

    memset(rec, 0, reqSize);

    if (Catalog_Create_MArecSess(
        rec,
        reqSize,
        g_cat->catSeqNo,
        startTime,
        sessID,
        sessOwner,
        sessDesc,
        sessType)
     )
    {
        CatalogList_CatSendRecSessToCON (rec);
    }

    FREE(rec);

    DBGFCNOUTLOW();
    return (1);
}   /* CatBlkWriteSessRec */


/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec        pointer to session record struct
*
* @brief     This function sends the sess rec to CON
*
*//*=========================================================================*/
static void
CatalogList_CatSendRecSessToCON(const MArecSess *rec)
{
    ERH_FUNCTION (_T("CatalogList_CatSendRecSessToCON"));

    StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_SESS);

    StrMsgFAppend (
        _T("%d%d%d%d%s%s%s%d"),
        STD2HOST_L(rec->maCC),
        STD2HOST_L(rec->maCatSeqNo),
        STD2HOST_L(rec->maStartTime),
        STD2HOST_L(rec->maCurrentTime),
        rec2t(rec,sessID),
        rec2t(rec,sessOwner),
        rec2t(rec,sessDesc),
        STD2HOST_L(rec->sessType)
    );

    IpcSendData(conIpcHandle, conIpcBuffer, StrMsgSize(conIpcBuffer));
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec            pointer to DaCC record struct
* @param     unicode        flag if unicode is used
*
* @brief     This function send the daCC's to SM
*
*//*=========================================================================*/
int
CatSendDaccToCON(const daRec *rec, int unicode)
{
    ERH_FUNCTION (_T("CatSendDaccToCON"));
    unsigned boType = STD2HOST_L(rec->boType);
    unsigned boLevel = STD2HOST_L(rec->boLevel);
    OB2_SIZE_T objSizeKb = ((IntsToLL(rec->boSizeHi, rec->boSizeLo)) + 1023) / 1024; /*** round up ***/
    ULONG nrecFlags = unicode? htonl(FLAG_UNICODE) : 0;

    ctchar *boName  = StrnR2T(rec->boName,  nrecFlags, rec->boNameLength);
    ctchar *boDesc  = StrnR2T(rec->boDesc,  nrecFlags, rec->boDescLength);
    ctchar *appName = StrnR2T(rec->appName, nrecFlags, rec->appNameLength);
    ctchar *appDesc = StrnR2T(rec->appDesc, nrecFlags, rec->appDescLength);

    DBGFCNINLOWLOW();

    StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_DACC);

    DBGPLAIN(
        DBG_MA_CATALOG_100, __FCN_FMT__
        _T("DaID=") ULONG_FMT _T("; ")
        _T("daCC=") ULONG_FMT _T("; ")
        _T("daSegment=") ULONG_FMT _T("; ")
        _T("daOffset=") ULONG_FMT _T("; ")
        _T("objSizeKb=") OB2_SIZE_T_FMT _T("; ")
        _T("unicode=") INT_FMT,
        __FCN__,
        STD2HOST_L(rec->daID),
        STD2HOST_L(rec->daCC),
        STD2HOST_L(rec->daSegment),
        STD2HOST_L(rec->daOffset),
        objSizeKb,
        unicode
    );

    StrMsgFAppend (
        _T("%d%d%d%d%d%d%d%d%d%d%d%d%s%s%s%s%d%d%d%d%d%d"),
        STD2HOST_L(rec->daID),
        STD2HOST_L(rec->daCC),
        STD2HOST_L(rec->daSegment),
        STD2HOST_L(rec->daOffset),
        boType,
        boType == OT_OB2BAR && boLevel > 100 ? 0 : boLevel,
        STD2HOST_L(rec->boAccsType),
        STD2HOST_L(rec->boAccsValue),
        STD2HOST_L(rec->boProtType),
        STD2HOST_L(rec->boProtValue),
        STD2HOST_L(rec->boStartTime),
        STD2HOST_L(rec->boSinceTime),
        boName,
        boDesc,
        appName,
        appDesc,
        (ULONG)DF_TYPE_NORMAL,
        (ULONG)DF_SUBTYPE_NORMAL,
        STD2HOST_L(rec->generation),
        STD2HOST_L(rec->overFlags),
        GetLowerInt(objSizeKb),
        GetUpperInt(objSizeKb)
    );

    DBGPLAIN(
        DBG_MA_CATALOG, __FCN_FMT__
        _T("\nboName=     %s,\nboDesc=  %s,\nappName=   %s\nappDesc= %s\n"), __FCN__,
        boName,
        boDesc,
        appName,
        appDesc
    );

    IpcSendData (
        conIpcHandle,
        conIpcBuffer,
        StrMsgSize(conIpcBuffer)
        );

    DBGFCNOUTLOWLOW_DEC(1);
    return (1);
}


// @brief   sent MSG_DACC for given record
static void
CatalogList_SendDaccToCON(const MArecDA *rec)
{
    ERH_FUNCTION(_T("CatalogList_SendDaccToCON"));
    unsigned boType  = STD2HOST_L(rec->boType);
    unsigned boLevel = STD2HOST_L(rec->boLevel);
    ctchar *boName, *boDesc, *appName, *appDesc;

    StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_DACC);

    StrMsgFAppend (
        _T("%d%d%d%d%d%d%d%d%d%d%d%d%s%s%s%s%d%d%d%d%d%d"),
        STD2HOST_L(rec->daID),
        STD2HOST_L(rec->daCC),
        STD2HOST_L(rec->daSegment),
        STD2HOST_L(rec->daOffset),
        boType,
        boType == OT_OB2BAR && boLevel > 100 ? 0 : boLevel,
        STD2HOST_L(rec->boAccsType),
        STD2HOST_L(rec->boAccsValue),
        STD2HOST_L(rec->boProtType),
        STD2HOST_L(rec->boProtValue),
        STD2HOST_L(rec->boStartTime),
        STD2HOST_L(rec->boSinceTime),

        boName = rec2t(rec, boName),
        boDesc = rec2t(rec, boDesc),
        appName = rec2t(rec, appName),
        appDesc = rec2t(rec, appDesc),

        (ULONG)STD2HOST_S(rec->boDFType),
        (ULONG)STD2HOST_S(rec->boDFSubType),
        STD2HOST_L(rec->generation),
        STD2HOST_L(rec->overFlags),
        STD2HOST_L(rec->boSizeLo),
        STD2HOST_L(rec->boSizeHi)
    );

    DBGPLAIN(
        DBG_MA_CATALOG,
        __FCN_FMT__ _T("boName=     %s,\nboDesc=  %s,\nappName=   %s\nappDesc= %s\n"), __FCN__,
        boName, boDesc, appName, appDesc);
    
    IpcSendData (conIpcHandle, conIpcBuffer, StrMsgSize(conIpcBuffer));
}

#if XMA_NDMP
/*========================================================================*//**
*
* @ingroup   Catalog Manager NDMP (Cat)
*
* @param     rec            pointer to DaCC record struct
*
* @brief     This function will send environment buffer if not empty
*
*//*=========================================================================*/
static void
CatalogList_SendEnvBuffNDMP(const MArecDA *rec)
{
    ERH_FUNCTION(_T("NDMP_SenEnvBuff"));
    ULONG recFlags = rec->recHeader.recFlags;

    if (MAOPT->dfType == DF_TYPE_NDMP && STD2HOST_S(rec->boFBinLength) != 0)
    {
        StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_FBIN);
        StrMsgFAppend ( _T("%d%d%d%d"),
            FBIN_NDMP_ENV,
            STD2HOST_S(rec->boFBinLength),
            1,                          /*** means the number of parameters that follows ***/
            STD2HOST_L(rec->daID)
            );
        IpcSendData (conIpcHandle, conIpcBuffer, StrMsgSize(conIpcBuffer));
        DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("to CON: MSG_FBIN daID=%d fvtype=%d size=%d bytes "), __FCN__,
            STD2HOST_L(rec->daID),
            1,
            STD2HOST_S(rec->boFBinLength)
            );
        IpcSendData (conIpcHandle, (char*)rec + STD2HOST_S(rec->boFBinOffset), STD2HOST_S(rec->boFBinLength));
    }

    HEAD.boName  = StrNewCopyR2T(rec, boNameOffset, recFlags);
    HEAD.boDesc  = StrNewCopyR2T(rec, boDescOffset, recFlags);
    HEAD.appName = StrNewCopyR2T(rec, appNameOffset, recFlags);
    HEAD.appDesc = StrNewCopyR2T(rec, appDescOffset, recFlags);

    tmpDArec = MALLOC(sizeof(MArecDA));
    memcpy(tmpDArec, rec, sizeof(MArecDA));

    if (MAOPT->dfType == DF_TYPE_NDMP)
        firstChunk = 0;
}


/*========================================================================*//**
*
* @ingroup   BMA Catalog Manager (Cat)
*
* @param     ndmpSeg       ndmp pointer to segment num
* @param     freeDA        ndmp pointer to free DA flag
* @param     catSeqNo      pointer to catalog seq. number
* @param     dev           pointer to devRec structure
*
* @retval    STATE_WORK    OK, continue work ...
* @retval    STATE_NEXT    EOM, error etc.
* @retval    STATE_ABORT   No ACK, busReset
*
* @brief     Writes the first chunk to media. First the file mark is written.
*
*//*=========================================================================*/
void CatalogList_Sess_DaCC(void)
{
    ERH_FUNCTION(_T("CatalogList_Sess_DaCC"));

    DBGFCNIN();

    DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("\tMSG_SESS to CON:"), __FCN__);
    StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_SESS);

    StrMsgFAppend (
        _T("%d%d%d%d%s%s%s%d"),
        STD2HOST_L(tmpSessRec->maCC),
        STD2HOST_L(tmpSessRec->maCatSeqNo),
        STD2HOST_L(tmpSessRec->maStartTime),
        STD2HOST_L(tmpSessRec->maCurrentTime),
        HEAD.sessID,
        HEAD.sessOwner,
        HEAD.sessDesc,
        STD2HOST_L(tmpSessRec->sessType)
    );

    DBGPLAIN(
        DBG_MA_CATALOG,
        __FCN_FMT__ _T("sessID=     %s,\nsessOwner=  %s,\nsessDesc=   %s\n"), __FCN__,
        HEAD.sessID,
        HEAD.sessOwner,
        HEAD.sessDesc
    );

    IpcSendData (
        conIpcHandle,
        conIpcBuffer,
        StrMsgSize(conIpcBuffer)
    );
    DBGPLAIN(
        DBG_MA_CATALOG,
        __FCN_FMT__ _T("maCC=       %d,\nSeqNo=      %d,\nStartTime=  %d,\nCurrentTime=%d,"), __FCN__,
        STD2HOST_L(tmpSessRec->maCC),
        STD2HOST_L(tmpSessRec->maCatSeqNo),
        STD2HOST_L(tmpSessRec->maStartTime),
        STD2HOST_L(tmpSessRec->maCurrentTime)
    );

    /*
    DACC
    */

    StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_DACC);

    StrMsgFAppend (
        _T("%d%d%d%d%d%d%d%d%d%d%d%d%s%s%s%s%d%d%d%d%d%d"),
        STD2HOST_L(tmpDArec->daID),
        STD2HOST_L(tmpDArec->daCC),
        STD2HOST_L(tmpDArec->daSegment),
        STD2HOST_L(tmpDArec->daOffset),
        STD2HOST_L(tmpDArec->boType),
        ((STD2HOST_L(tmpDArec->boType) == 5) && (STD2HOST_L(tmpDArec->boLevel) > 100) ? 0 : STD2HOST_L(tmpDArec->boLevel)),
        STD2HOST_L(tmpDArec->boAccsType),
        STD2HOST_L(tmpDArec->boAccsValue),
        STD2HOST_L(tmpDArec->boProtType),
        STD2HOST_L(tmpDArec->boProtValue),
        STD2HOST_L(tmpDArec->boStartTime),
        STD2HOST_L(tmpDArec->boSinceTime),

        HEAD.boName,
        HEAD.boDesc,
        HEAD.appName,
        HEAD.appDesc,

        (ULONG)STD2HOST_S(tmpDArec->boDFType),
        (ULONG)STD2HOST_S(tmpDArec->boDFSubType),
        STD2HOST_L (tmpDArec->generation),
        STD2HOST_L (tmpDArec->overFlags),
        STD2HOST_L(tmpDArec->boSizeHi),
        STD2HOST_L(tmpDArec->boSizeLo)
    );

    DBGPLAIN(
        DBG_MA_CATALOG,
        __FCN_FMT__ _T("boName=     %s,\nboDesc=  %s,\nappName=   %s\nappDesc= %s\n"), __FCN__,
        HEAD.boName,
        HEAD.boDesc,
        HEAD.appName,
        HEAD.appDesc
    );

    IpcSendData (
        conIpcHandle,
        conIpcBuffer,
        StrMsgSize(conIpcBuffer)
    );

    DBGPLAIN(DBG_MA_CATALOG,
        __FCN_FMT__ _T("\tMSG_DACC(daID=%d) to CON:"), __FCN__,
        STD2HOST_L(tmpDArec->daID)
    );

    /* FBIN - send only once */
#if 0
{
    static int sended=0;

    if (!sended)
        if (STD2HOST_S(tmpDArec->boFBinLength) != 0)
        {
            StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_FBIN);
            StrMsgFAppend ( _T("%d%d%d%d%d"),
                            FBIN_NDMP_ENV,
                            STD2HOST_S(tmpDArec->boFBinLength),
                            1,                          /*** means the number of parameters that follows ***/
                            STD2HOST_L(tmpDArec->daID)
            );
            IpcSendData (conIpcHandle, conIpcBuffer, StrMsgSize(conIpcBuffer));
            DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("to CON: MSG_FBIN daID=%d type=%d size=%d bytes "), __FCN__,
                STD2HOST_L(tmpDArec->daID),
                1,
                STD2HOST_S(tmpDArec->boFBinLength)
            );
            IpcSendData (conIpcHandle, (char*)tmpDArec + STD2HOST_S(tmpDArec->boFBinOffset), STD2HOST_S(tmpDArec->boFBinLength));
            sended=1;
        }
}
#endif
    DBGFCNOUT();
    return;
}


/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec            pointer to record catalog struct
*
* @brief     This function will append catalog record to catv buffer.
*
*//*=========================================================================*/
static void
CatalogList_AppendNDMPCatRecToCatBuff(const MArecNDMPCat *rec, const ULONG nrecType)
{
    ERH_FUNCTIONA("CatalogList_AppendNDMPCatRecToCatBuff");


    /* import data new + old clients */
    NDMPcatRec *catv = NULL;
    NDMPcatRec2 *catNDMP = NULL;

    USHORT nObjLength     = STD2HOST_S(rec->objNameLength);
    USHORT nObjNameOffset = STD2HOST_S(rec->objNameOffset);

    if(nrecType == MA_REC_NDMP_CAT)
    {

        /* recType = MA_REC_NDMP_CAT !!! */
        if ((g_cat->catNext + sizeof(NDMPcatRec) + nObjLength) > g_cat->catSize)
        {
            ErhAssert(__FCN__, Catalog_Grow(g_cat));
        }

        catv = (NDMPcatRec *)(g_cat->catBuf + g_cat->catNext);
        catv->recType    = HOST2STD_L(CAT_REC_NDMP);
        catv->recSize    = HOST2STD_L(0L);

        catv->daID       = rec->daID;
        catv->objSegment = rec->objSegment;
        catv->objOffset  = rec->objOffset;
        catv->objOffsetHi= rec->objOffsetHi;
        catv->objType    = rec->objType;
        catv->objUID     = rec->objUID;
        catv->objGID     = rec->objGID;
        catv->objRWX     = rec->objRWX;
        catv->objDTM     = rec->objDTM;
        catv->objSize    = rec->objSize;
        catv->objSizeHi  = rec->objSizeHi;

        catv->recSize = HOST2STD_L(ROUND_UP(sizeof(NDMPcatRec) + nObjLength));

        /* DA guys said that objNameLength includes '\0' !!!*/
        memcpy( (UCHAR *)&(catv->objName), (UCHAR *)rec + nObjNameOffset, nObjLength);
        /* ------------------------------------------------------
        |   SM actualy doesn't need those parameters, but we'll
        |   put them here for completion - Samo
        ------------------------------------------------------ */
        catv->objNameLength = rec->objNameLength;
        catv->objNameOffset = HOST2STD_S((USHORT)((UCHAR *)catv->objName - (UCHAR *)catv));

        if (DbgMatch(DBG_MA_CATALOG))
        {
            const dpuint64_t offset  = IntsToLL(STD2HOST_L(catv->objOffsetHi), STD2HOST_L(catv->objOffset));
            const dpuint64_t objSize = IntsToLL(STD2HOST_L(catv->objSizeHi), STD2HOST_L(catv->objSize));

            DbgPlainA(DBG_MA_CATALOG, 
                    "%s:%d [%02d:%llu] %d/%d %llu %s",
                    __FCNA__,
                    STD2HOST_L(catv->daID),
                    STD2HOST_L(catv->objSegment),
                    offset,
                    STD2HOST_L(catv->objUID),
                    STD2HOST_L(catv->objGID),
                    objSize,
                    catv->objName
            );
        }

        g_cat->catNext += ROUND_UP(STD2HOST_L(catv->recSize));
    }
    else if(nrecType == MA_REC_NDMP2_CAT)
    {

        /* recType = MA_REC_NDMP2_CAT !!! */
        if ((g_cat->catNext + sizeof(NDMPcatRec2) + nObjLength) > g_cat->catSize)
        {
            ErhAssert(__FCN__, Catalog_Grow(g_cat));
        }

        catNDMP = (NDMPcatRec2 *)(g_cat->catBuf + g_cat->catNext);
        catNDMP->recType    = HOST2STD_L(CAT_REC_NDMP2);
        catNDMP->recSize    = HOST2STD_L(0L);

        catNDMP->daID       = rec->daID;
        catNDMP->objSegment = rec->objSegment;
        catNDMP->objOffset  = rec->objOffset;
        catNDMP->objOffsetHi= rec->objOffsetHi;
        catNDMP->objType    = rec->objType;
        catNDMP->objUID     = rec->objUID;
        catNDMP->objGID     = rec->objGID;
        catNDMP->objRWX     = rec->objRWX;
        catNDMP->objDTM     = rec->objDTM;
        catNDMP->objSize    = rec->objSize;
        catNDMP->objSizeHi  = rec->objSizeHi;
        catNDMP->inode      = rec->inode;
        catNDMP->inodeHi    = rec->inodeHi;

        catNDMP->recSize = HOST2STD_L(ROUND_UP(sizeof(NDMPcatRec2) + nObjLength));
        
        /* DA guys said that objNameLength includes '\0' !!!*/
        memcpy( (UCHAR *)&(catNDMP->objName), (UCHAR *)rec + nObjNameOffset, nObjLength);
        catNDMP->objNameLength = rec->objNameLength;
        catNDMP->objNameOffset = HOST2STD_S((USHORT)((UCHAR *)catNDMP->objName - (UCHAR *)catNDMP));

        if (DbgMatch(DBG_MA_CATALOG))
        {   /* prepare for debug */
            dpuint64_t offset  = IntsToLL(STD2HOST_L(catNDMP->objOffsetHi), STD2HOST_L(catNDMP->objOffset));
            dpuint64_t objSize = IntsToLL(STD2HOST_L(catNDMP->objSizeHi), STD2HOST_L(catNDMP->objSize));
            dpuint64_t inode   = IntsToLL(STD2HOST_L(catNDMP->inodeHi), STD2HOST_L(catNDMP->inode));

            DbgPlainA(DBG_MA_CATALOG,
                "%s:%d [%02d:%llu] %d/%d\t%llu\t%llu %s",
                __FCN__,
                STD2HOST_L(catNDMP->daID),
                STD2HOST_L(catNDMP->objSegment),
                offset,
                STD2HOST_L(catNDMP->objUID),
                STD2HOST_L(catNDMP->objGID),
                objSize,
                inode,
                catNDMP->objName
                );
        }

        g_cat->catNext += ROUND_UP(STD2HOST_L(catNDMP->recSize));
    }
}

#endif /* #if XMA_NDMP */

/*========================================================================*//**
* @brief    Append catRec catalog record to g_cat->catBuf
*
* @note     rec->objNameLength includes space for NUL
*//*=========================================================================*/
static void
CatalogList_AppendCatRecToCatBuff(const MArecCat *rec)
{
    ERH_FUNCTIONA("CatalogList_AppendCatRecToCatBuff");

    catRec  *catv;
    const char *objName = (const char*)rec + STD2HOST_S(rec->objNameOffset);
    USHORT objNameLength = STD2HOST_S(rec->objNameLength);

    // unsigned needNul = objNameLength && objName[objNameLength-1] != 0;
    unsigned recSize = ROUND_UP(sizeof(catRec) + objNameLength);

    if (g_cat->catNext + recSize > g_cat->catSize)
    {
        ErhAssert(__FCN__, Catalog_Grow(g_cat));
    }

    catv = (catRec *)(g_cat->catBuf + g_cat->catNext);
    catv->recType    = HOST2STD_L(CAT_REC_POSIX);
    catv->recSize    = HOST2STD_L(0);

    catv->daID       = rec->daID;
    catv->objSegment = rec->objSegment;
    catv->objOffset  = rec->objOffset;
    catv->objType    = rec->objType;
    catv->objUID     = rec->objUID;
    catv->objGID     = rec->objGID;
    catv->objRWX     = rec->objRWX;
    catv->objDTM     = rec->objDTM;
    catv->objSize    = rec->objSize;
    catv->objSizeHi  = rec->objSizeHi;

    catv->recSize = HOST2STD_L(recSize);

    // objNameLength : includes space for NUL char when sent by the DA
    //                 may not include it when read from medium (during import)
    memcpy(catv->objName, objName, objNameLength);
    catv->objName[objNameLength-1] = 0;

    catv->objNameLength = HOST2STD_S(objNameLength);
    catv->objNameOffset = HOST2STD_S((USHORT)((UCHAR *)catv->objName - (UCHAR *)catv));

    DBGPLAINA(DBG_MA_CATALOG,
        __FCNA_FMT__ "%d [%02d:%04d] %d/%d\t%d:%07d %s", __FCNA__,
        STD2HOST_L(catv->daID),
        STD2HOST_L(catv->objSegment),
        STD2HOST_L(catv->objOffset),
        STD2HOST_L(catv->objUID),
        STD2HOST_L(catv->objGID),
        STD2HOST_L(catv->objSizeHi),
        STD2HOST_L(catv->objSize),
        catv->objName
    );

    g_cat->catNext += recSize;
}


/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     rec            pointer to NT record catalog struct
*
* @brief     This function will append catalog record to catv buffer.
*
*//*=========================================================================*/
static void
CatalogList_AppendNTCatRecToCatBuff(const MArecNTCat *rec)
{
    ERH_FUNCTIONA("CatalogList_AppendNTCatRecToCatBuff");

    NTcatRec *catv = NULL;
    UCHAR *ptr = NULL;

    if ((g_cat->catNext
        + sizeof(NTcatRec)
        + STD2HOST_S(rec->objAsciiNameLength)
        + (STD2HOST_S(rec->objAsciiNameLength) & 1)
        + STD2HOST_S(rec->objUnicodeNameLength)) > g_cat->catSize)
    {
        ErhAssert(__FCN__, Catalog_Grow(g_cat));
    }

    catv = (NTcatRec *)(g_cat->catBuf + g_cat->catNext);

    catv->recType            = HOST2STD_L(CAT_REC_NT);
    catv->recSize            = HOST2STD_L(0L);

    catv->daID               = rec->daID;
    catv->objSegment         = rec->objSegment;
    catv->objOffset          = rec->objOffset;
    catv->objType            = rec->objType;
    catv->objFileAttributes  = rec->objFileAttributes;
    catv->objDtmLo           = rec->objDtmLo;
    catv->objDtmHi           = rec->objDtmHi;
    catv->objSizeLo          = rec->objSizeLo;
    catv->objSizeHi          = rec->objSizeHi;
    catv->objDTM             = rec->objDTM;

    ptr = (UCHAR *)&(catv->objName[0]);

    memcpy (ptr,
        (UCHAR *)rec + STD2HOST_S(rec->objAsciiNameOffset),
        STD2HOST_S(rec->objAsciiNameLength)
        );
    catv->objAsciiNameLength = rec->objAsciiNameLength;
    catv->objAsciiNameOffset = HOST2STD_S((USHORT)(ptr - (UCHAR *)catv));
    ptr += STD2HOST_S(rec->objAsciiNameLength)
        + (STD2HOST_S(rec->objAsciiNameLength) & 1);

    memcpy (ptr,
        (UCHAR *)rec + STD2HOST_S(rec->objUnicodeNameOffset),
        STD2HOST_S(rec->objUnicodeNameLength)
        );
    catv->objUnicodeNameLength = rec->objUnicodeNameLength;
    catv->objUnicodeNameOffset = HOST2STD_S((USHORT)(ptr - (UCHAR *)catv));
    ptr += STD2HOST_S(rec->objUnicodeNameLength);

    catv->recSize = HOST2STD_L((ULONG)(ptr - (UCHAR *)catv));

    DBGPLAINA(DBG_MA_CATALOG,
        __FCNA_FMT__ "%d [%02d:%04d] %d:%07d\t%s", __FCNA__,
        STD2HOST_L(catv->daID),
        STD2HOST_L(catv->objSegment),
        STD2HOST_L(catv->objOffset),
        STD2HOST_L(catv->objSizeHi),
        STD2HOST_L(catv->objSizeLo),
        (char *)catv + STD2HOST_S(catv->objAsciiNameOffset)
    );

    g_cat->catNext += ROUND_UP(STD2HOST_L(catv->recSize));
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     bufPtr        pointer to buffer block
*
* @brief     This function lists the catalog block
*
*//*=========================================================================*/
void CatalogList_SendCatBuf()
{
    ERH_FUNCTION(_T("CatalogList_SendCatBuf"));

    if( g_cat->catNext > 0 ){
        StrMsgStart (conIpcBuffer, MAX_SM_PACKETSIZE, MSG_DATA);
        StrMsgFAppend (MSG_ULONG_FMT, g_cat->catNext);
        IpcSendData(conIpcHandle, conIpcBuffer, StrMsgSize(conIpcBuffer) );
        IpcSendData(conIpcHandle, g_cat->catBuf, g_cat->catNext);
        DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("\tMSG_DATA(")ULONG_FMT _T(") to CON"), __FCN__, g_cat->catNext);
    }
    else
    {
        DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("No catalog to send, length=")ULONG_FMT _T(""), __FCN__, g_cat->catNext);
    }
    return;
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     bufPtr        pointer to buffer block
*
* @brief     This function lists the catalog block
*
*//*=========================================================================*/

void
CatalogList(devRec * dev,
            UCHAR  *bufPtr)
{
    ERH_FUNCTION (_T("CatalogList"));
    UCHAR *recPtr  = bufPtr + ROUND_UP(sizeof(MAblkHdr));
    catRecData    *catData = NULL;
    ULONG          catDataExpectedSeq = 0;
    ULONG          nrecType           = 0;
    static boolean stillProcSameCatBlk = false;


#if XMA_NDMP
#if JSONIZER_SUPPORTED_PLATFORM
    BackupSessionContextStart* sessCtx = (BackupSessionContextStart *)(dev->jsonRec.bckSesCtxStartHandle);
#endif
    static int segment = 1;
#endif

    DBGFCNIN();

    while ((ULONG)(recPtr - bufPtr) < STD2HOST_L(((MAblkUniv *)bufPtr)->blkHeader.blkLength))
    {
        ErhAssert (
            _T("CatalogList"),
            ((MArecUniv *)recPtr)->recHeader.magicCookie
            == HOST2STD_L(MA_REC_MAGIC_COOKIE)
            );

        nrecType = STD2HOST_L(((MArecUniv *)recPtr)->recHeader.recType);
        if (MAENV->skipDuplCatBlkDuringImport != -1)
        {
            static ULONG     lastCatalogSeqNum = -1;

            if ((lastCatalogSeqNum == STD2HOST_L(((MArecSess *)recPtr)->maCatSeqNo)) && (!stillProcSameCatBlk))
            {
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("OB2IMPORTWORKAROUND: Skiping duplicated catalog blok!"), __FCN__);
                break;    /* break while loop */
            }
            else if (lastCatalogSeqNum == -1)
            {
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("OB2IMPORTWORKAROUND: Initialisation..."), __FCN__);
                lastCatalogSeqNum = STD2HOST_L(((MArecSess *)recPtr)->maCatSeqNo);
            }
            else if (!stillProcSameCatBlk)
            {
                DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("OB2IMPORTWORKAROUND: All ok, proceeding to next catalog block..."), __FCN__);
                lastCatalogSeqNum++;
            }
            stillProcSameCatBlk = true;
        }

        switch (nrecType)
        {
        case MA_REC_SESS:
            {
                MArecSess *rec = (MArecSess *)recPtr;

#if XMA_NDMP
                /* --------------------------------------------------------
                Send SESS only with first chunk and copy to tmp buffer
                for later sending (if any).
                ---------------------------------------------------- */
                if (firstChunk)
                {
#endif
                    CatPrintRecSess(rec);

                    DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("\tMSG_SESS to CON:"), __FCN__);
#if XMA_NDMP
/* ----------------------------------------------------------------------------
| QCCR2A4357 - NDMP backups did not write correct sequence numbers for the first
| few months after 6.21's release and also 7.0/7.01's release. This makes the 
| import of backups spanning multiple tapes impossible. The following code 
| tries to simulate the required sequence number by keeping a counter on a per-
| session basis, and saves it in a file. This allows the MMA to handle multiple
| objects across multiple tapes. Other scenarios may need to be handled.
  ---------------------------------------------------------------------------*/
#if TARGET_WIN32
#define SEQFILE _T("\\seqfile")
#else
#define SEQFILE _T("/seqfile")
#endif
                    if(MAENV->genCatSeqNo && STD2HOST_L(rec->maCatSeqNo)==0)
                    {
                        tchar tmpPath[STRLEN_STD];
                        static tchar fileSessId[STRLEN_STD];
                        ctchar *curSessId = NULL;
                        static int fakecatSeqNo = 0;
                        int ret = 0;
                        FILE *fh = NULL;
                        tchar tmpStr[STRLEN_STD];
                        tchar *s1 = NULL, *s2 = NULL;
                        
                        DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("CatSeqNo compensation being attempted"),
                            __FCN__); 
                        sprintf(tmpPath, _T("%s%s"), cmnPanTmp, SEQFILE);

                        ret = OB2_StatFile(tmpPath, NULL);;
                        if(ret == 0)
                        {
                            /* Open file and read contents */
                            DBGPLAIN(DBG_MA_CATALOG, _T("Opening existing file "));  
                            fh = fopen(tmpPath, _T("r+"));
                            if(fh != NULL)
                            {
                                if(fgets(tmpStr, STRLEN_STD, fh) != NULL)
                                {
                                    DBGPLAIN(DBG_MA_CATALOG, __FCN_FMT__ _T("read line \"%s\"."), __FCN__, tmpStr);

                                    s1 = strtok(tmpStr, _T("\t"));
                                    StrCpyNS(fileSessId, s1);

                                    s2 = strtok(NULL, _T("\n\0"));      
                                    sscanf(s2, _T("%d"), &fakecatSeqNo);
                                    
                                    DBGPLAIN(DBG_MA_CATALOG, _T("Contents of file %s %d"), fileSessId, fakecatSeqNo);  
                                    
                                    curSessId = R2T(rec, sessIDOffset, rec->recHeader.recFlags);
                                    
                                    if(!StrCmp(fileSessId, curSessId))
                                    {
                                        /*Session match found - fake seqno */
                                        rec->maCatSeqNo = HOST2STD_L(++fakecatSeqNo);
                                    }
                                    else
                                    {
                                        /*New/different session - overwrite last seqno */
                                        fakecatSeqNo = 0;
                                        StrCopy(fileSessId, curSessId, STRLEN_STD);
                                        DBGPLAIN(DBG_MA_CATALOG, _T("New contents of file %s %d"), fileSessId, fakecatSeqNo);  
                                    }
                                    fseek(fh, 0, SEEK_SET);
                                    fprintf(fh, _T("%s\t%d"), fileSessId, fakecatSeqNo);
                                }
                                else
                                {
                                    DBGPLAIN(DBG_MA_CATALOG, _T("Unexpected file error"));                                     
                                }
                            }
                            else
                            {
                                DBGPLAIN(DBG_MA_CATALOG, _T("Unusual file error - ignoring due to lack of options"));                                
                            }
                        }
                        else
                        {                            
                            /*Create file and write variables */
                            DBGPLAIN(DBG_MA_CATALOG, _T("Creating file "));  
                            fh = fopen(tmpPath, _T("w"));
                         
                            if(fh != NULL)
                            {
                                fakecatSeqNo = 0;
                                fileSessId[0] = _T('\0');
                                curSessId = R2T(rec, sessIDOffset, rec->recHeader.recFlags);

                                DBGPLAIN(DBG_MA_CATALOG, _T("Session id for consideration %s"),curSessId);                                                              
                                StrCopy(fileSessId, curSessId, STRLEN_STD);
                                fprintf(fh, _T("%s\t%d"), fileSessId, fakecatSeqNo);
                                DBGPLAIN(DBG_MA_CATALOG, _T("New contents of file %s %d"), fileSessId, fakecatSeqNo);  
                            }
                            else
                            {
                                DBGPLAIN(DBG_MA_CATALOG, _T("Unusual file error - ignoring due to lack of options"));                                
                            }
                        }
                        if(fh != NULL)
                            fclose(fh);
                    }
#endif
#if XMA_NDMP
#if JSONIZER_SUPPORTED_PLATFORM
                    if(MAOPT->dfSubType == DF_SUBTYPE_OBJSTORE_NDMP)
                    {
                        tchar *sessionID = MALLOC(STRLEN_PATH + 1);
                        tchar *sessOwner = MALLOC(STRLEN_PATH + 1);
                        tchar *sessDesc = MALLOC(STRLEN_PATH + 1);
                        MArecSess   *recObjStoreNDMP = NULL;
                        ULONG       reqSize = 0UL;

                        StrCopyA2T(sessionID, sessCtx->sessionID, STRLEN_PATH);
                        StrCopyA2T(sessOwner, sessCtx->sessOwner, STRLEN_PATH);
                        StrCopyA2T(sessDesc, sessCtx->sessDesc, STRLEN_PATH);

                        reqSize = (ULONG) ( sizeof(MArecSess)
                            + (strlen(sessionID)   + 1)*sizeof(tchar)    /* +1 for ASCIIZ */
                            + (strlen(sessOwner)+ 1)*sizeof(tchar)    /* +1 for ASCIIZ */
                            + (strlen(sessDesc) + 1)*sizeof(tchar) ); /* +1 for ASCIIZ */

                        recObjStoreNDMP = (MArecSess   *)MALLOC(reqSize);

                        memset(recObjStoreNDMP, 0, reqSize);

                        if (Catalog_Create_MArecSess(
                            recObjStoreNDMP,
                            reqSize,
                            g_cat->catSeqNo,
                            STD2HOST_L(sessCtx->maStartTime),
                            sessionID,
                            sessOwner,
                            sessDesc,
                            STD2HOST_L(sessCtx->sessType))
                            )
                        {
                            CatalogList_CatSendRecSessToCON (recObjStoreNDMP);
                        }

                        /* ------------------------------------------------
                        Copy session data to tmp buffers for later use
                        ------------------------------------------ */
                        HEAD.sessID    = StrNewCopyR2T(rec, sessIDOffset,    rec->recHeader.recFlags);
                        HEAD.sessOwner = StrNewCopyR2T(rec, sessOwnerOffset, rec->recHeader.recFlags);
                        HEAD.sessDesc  = StrNewCopyR2T(rec, sessDescOffset,  rec->recHeader.recFlags);

                        tmpSessRec = (MArecSess *)MALLOC(sizeof(MArecSess));
                        memcpy(tmpSessRec, recObjStoreNDMP, sizeof(MArecSess));

                        FREE(recObjStoreNDMP);
                        FREE(sessionID);
                        FREE(sessOwner);
                        FREE(sessDesc);
                    }
                    else
                    {
#endif
#endif
                        CatalogList_CatSendRecSessToCON(rec);
#if XMA_NDMP
#if JSONIZER_SUPPORTED_PLATFORM
                    }
#endif
#endif

#if XMA_NDMP
                    if(MAOPT->dfSubType != DF_SUBTYPE_OBJSTORE_NDMP)
                    {
                        /* ------------------------------------------------
                        Copy session data to tmp buffers for later use
                        ------------------------------------------ */
                        HEAD.sessID    = StrNewCopyR2T(rec, sessIDOffset,    rec->recHeader.recFlags);
                        HEAD.sessOwner = StrNewCopyR2T(rec, sessOwnerOffset, rec->recHeader.recFlags);
                        HEAD.sessDesc  = StrNewCopyR2T(rec, sessDescOffset,  rec->recHeader.recFlags);
                        tmpSessRec = (MArecSess *)MALLOC(sizeof(MArecSess));
                        memcpy(tmpSessRec, rec, sizeof(MArecSess));
                    }
                } /* endif (firstChunk) */
#endif
                break;
            }
            /* end case MA_REC_DA */

        case MA_REC_DA:
            {
                MArecDA *rec = (MArecDA *)recPtr;

#if XMA_NDMP
#define TOLERANCE 100
                /* --------------------------------------------------------
                Send DACC only with firstchunk and copy to tmp buffer
                for later sending (if any).
                ---------------------------------------------------- */
                if (firstChunk)
                {
                     /* ----------------------------------------------------------------------------
                     | QCCR2A40200 - try to correct bad segment numbers caused by unpatched 6.21 or
                     | 7.0 NDMP backups (QCCR2A37160) during import. Keep track of expected segment 
                     | number and if the difference exceeds a limit, override. Limit is arbitrary
                       -------------------------------------------------------------------------- */ 
                    if(MAENV->overrideDaSeg)
                    {
                        DBGPLAIN(DBG_MA_CATALOG,__FCN_FMT__ _T("current seg val %d, diff %d"),__FCN__,STD2HOST_L(rec->daSegment),
                            STD2HOST_L(rec->daSegment) - MAENV->NDMPSegment);
                        /*------------------------------------------------------------------------------------
                        |QCCR2A58063 - Restore is failing on NDMP-Celerra after the export and import of media
                        |Cast to long type to make sure it can minus segment and then compare with TOLERANCE correctly
                         -------------------------------------------------------------------------------------*/
                        if ((LONG)(STD2HOST_L(rec->daSegment) - MAENV->NDMPSegment) > TOLERANCE)
                        {
                            rec->daSegment = HOST2STD_L(MAENV->NDMPSegment);
                        }
                        MAENV->NDMPSegment += 2;
                    }
#undef TOLERANCE
#endif
                    CatPrintRecDA(rec);

                    DBGPLAIN(DBG_MA_CATALOG,
                        __FCN_FMT__ _T("\tMSG_DACC to CON [daID=%d, daCC=%d daSegment=%u daOffset=%u overFlags=%#x boSize=%d:%d]"),
                        __FCN__,
                        STD2HOST_L(rec->daID),
                        STD2HOST_L(rec->daCC),
                        STD2HOST_L(rec->daSegment),
                        STD2HOST_L(rec->daOffset),
                        STD2HOST_L(rec->overFlags),
                        STD2HOST_L(rec->boSizeHi),
                        STD2HOST_L(rec->boSizeLo));

                    /* ---------------------------------------------------------------------------
                    |    Send current DA description record
                    -------------------------------------------------------------------------- */

                    CatalogList_SendDaccToCON(rec);

#if XMA_NDMP
                    /***  send environment buffer if not empty  **/
                    CatalogList_SendEnvBuffNDMP(rec);

                } /* endif (firstChunk) */
#endif
                break;
            }
            /* end case MA_REC_DA */

#if DA_ENCRYPT_KEY_CATALOG
        case MA_REC_DA_ENCR:
        case MA_REC_ENCR :
            {
                /* Process DA/MA  key catalog record */
                if (KeyCatProcessEncrData(STD2HOST_L(((MArecUniv *)recPtr)->recHeader.recType), recPtr)
                    == MA_KEY_CATALOG_FAILURE)
                {
                    return;
                }
            }
            break;
#endif /*  DA_ENCRYPT_KEY_CATALOG */

        case MA_REC_CAT:
            {
                MArecCat *rec = (MArecCat *)recPtr;
                USHORT objNameLength = STD2HOST_S(rec->objNameLength);

/* -------------------------------------------------------------------
|   NSMbb18544
|   Recalculate NameLength. Because if name doesn't include NUL
|   character in Length add one to correctly memcpy the string.
------------------------------------------------------------------ */

                if (objNameLength == 0)
                {
                    rec->objNameLength = HOST2STD_S(1);
                }
                else if (*((UCHAR *)rec + STD2HOST_S(rec->objNameOffset) + objNameLength - 1) != 0)
                {
                    rec->objNameLength = HOST2STD_S(rec->objNameLength + 1);
                }

#if XMA_NDMP
                /* for old NDMP backups this is necessary and chunking is a must */
                if ( ndmpCatChunk &&
                   (g_cat->catNext + sizeof(catRec) + STD2HOST_S(rec->objNameLength) + 1) > chunkSize(chunkList) )
                {
                    /* ---------------------------------------------
                        1. SEND SESS/DACC for any chunk other than first
                        2. Send FBIN (for first chunk only)
                        3. Send Catalog chunk (free buffer)

                        for session that spawns over two tapes
                        fbin will be null for first tape
                        ----------------------------------------------*/
                    DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("Sending Chunk (size=") ULONG_FMT _T(")"), __FCN__, g_cat->catNext);
                    if (!firstChunk)
                    {
                        CatalogList_Sess_DaCC(); /* send messages */
                        /* fake sequence number */
                        tmpSessRec->maCatSeqNo = HOST2STD_L(STD2HOST_L(tmpSessRec->maCatSeqNo) + 1);
                    }

                    CatalogList_SendCatBuf();   /* was duplicate code */
                    Catalog_Restart();

                }
#endif

                /* -------------------------------------------------------------------------
                |    Append catalog record to catv buffer. Will be sent later ...
                -------------------------------------------------------------------------- */
                CatalogList_AppendCatRecToCatBuff(rec);
                
                break;
            } /* case MA_REC_CAT */

#if XMA_NDMP
            case MA_REC_NDMP_CAT:
            case MA_REC_NDMP2_CAT:
            {
                MArecNDMPCat *rec = (MArecNDMPCat *)recPtr;

                /* -------------------------------------------------------------------
                |   NSMbb18544
                |   Recalculate NameLength. Because if name doesn't include NUL
                |   character in Length add one to correctly memcpy the string.
                ------------------------------------------------------------------ */
                if (STD2HOST_S(rec->objNameLength) == 0 ||
                    (*((UCHAR *)rec + STD2HOST_S(rec->objNameOffset) + STD2HOST_S(rec->objNameLength) - 1) != '\0'))
                {
                    rec->objNameLength = HOST2STD_S((USHORT)(STD2HOST_S(rec->objNameLength) + 1));
                }

                if(nrecType == MA_REC_NDMP_CAT)
                {
                /* Chunk Check TODO check because of append backups and parallel backups */
                    if (ndmpCatChunk && (g_cat->catNext + sizeof(NDMPcatRec) + STD2HOST_S(rec->objNameLength) + 1) > chunkSize(chunkList))
                {
                        ProcessChunk(firstChunk);
                    }
                }
                else
                    {
                    /* Chunk Check TODO check because of append backups and parallel backups */
                    if (ndmpCatChunk && (g_cat->catNext + sizeof(NDMPcatRec2) + STD2HOST_S(rec->objNameLength) + 1) > chunkSize(chunkList))
                    {
                        ProcessChunk(firstChunk);   
                    }
                }


                /* ---------------------------------------------------------------------------
                |    Append catalog record to catv buffer. Will be sent later ...
                -------------------------------------------------------------------------- */
                CatalogList_AppendNDMPCatRecToCatBuff(rec, nrecType);

                break;
            } /* case MA_REC_NDMP_CAT,  
                 case MA_REC_NDMP2_CAT */
#endif

        case MA_REC_NT_CAT:
            {
                MArecNTCat *rec = (MArecNTCat *)recPtr;

                /* -------------------------------------------------------------------
                |   NSMbb18544
                |   Recalculate NameLength. Because if name doesn't include NUL
                |   character in Length add one to correctly memcpy the string.
                |   Add two for UniCode string.
                ------------------------------------------------------------------ */

                if (STD2HOST_S(rec->objAsciiNameLength) == 0)
                {

                    rec->objAsciiNameLength = HOST2STD_S((USHORT)
                        (STD2HOST_S(rec->objAsciiNameLength) + 1)
                        );
                }
                else if (*((UCHAR *)rec + STD2HOST_S(rec->objAsciiNameOffset)
                    + STD2HOST_S(rec->objAsciiNameLength) - 1) != '\0'
                    )
                {
                    rec->objAsciiNameLength = HOST2STD_S((USHORT)
                        (STD2HOST_S(rec->objAsciiNameLength) + 1)
                        );
                }

                if (STD2HOST_S(rec->objUnicodeNameLength) == 0)
                {

                    rec->objUnicodeNameLength = HOST2STD_S((USHORT)
                        (STD2HOST_S(rec->objUnicodeNameLength) + 2)
                        );
                }
                else if (*((UCHAR *)rec + STD2HOST_S(rec->objUnicodeNameOffset)
                    + STD2HOST_S(rec->objUnicodeNameLength) - 2) != '\0'
                    || ((UCHAR *)rec + STD2HOST_S(rec->objUnicodeNameOffset)
                    + STD2HOST_S(rec->objUnicodeNameLength) - 1) != '\0'
                    )
                {
                    rec->objUnicodeNameLength = HOST2STD_S((USHORT)
                        (STD2HOST_S(rec->objUnicodeNameLength) + 2)
                        );
                }

                /* ---------------------------------------------------------------------------
                |    Append catalog record to catv buffer. Will be sent later ...
                -------------------------------------------------------------------------- */

                CatalogList_AppendNTCatRecToCatBuff(rec);

                break;
            }
            /* case MA_REC_NT_CAT */

            case MA_REC_CAT_DATA:
            {
                MArecCatData    *rec = (MArecCatData *)recPtr;
                UCHAR           *ptr;

                /* NOTE: there can be multiple MA_REC_CAT_DATA records for ONE catalog data entry
                         if there are more than 1,  we have to merge
                */
                
                if (STD2HOST_L(rec->dataSeq == 0))
                {
                    /* this is first record, there should be no allocated memory yet */
                    ErhAssert (
                        _T("MaListCatalogBlock"),
                        catData == NULL
                    );

                    DBGPLAIN(DBG_MA_CATALOG,
                        __FCN_FMT__ _T("[XX:XXXX] OCD for DA ID %d: chunk 0"), __FCN__,
                        STD2HOST_L(rec->daID)
                    );

                    catData = (catRecData*)MALLOC(sizeof(catRecData) + STD2HOST_L(rec->dataLength));

                    catData->recType    = HOST2STD_L(CAT_REC_DATA);
                    catData->recSize    = HOST2STD_L(0L);
                    catData->daID       = rec->daID;
                    catData->dataLength = HOST2STD_L(0L);
                    catData->dataOffset = HOST2STD_S(0L);
                    catData->dummy1     = HOST2STD_S(0L);
    
                    ptr = (UCHAR *)(&catData->data[0]);

                    memcpy (
                        ptr,
                        ((UCHAR *)rec) + STD2HOST_S(rec->dataOffset),
                        STD2HOST_L(rec->dataLength)
                    );
    
                    catData->dataLength = rec->dataLength;
                    catData->dataOffset = HOST2STD_S((USHORT)(ptr - (UCHAR *)catData));

                    catData->recSize = HOST2STD_L(sizeof(catRecData) + STD2HOST_L(catData->dataLength));

                    catDataExpectedSeq++;
                }
                else
                {
                    /* this is not the first record, there should be allocated memory */
                    ErhAssert (
                        _T("MaListCatalogBlock"),
                        catData != NULL
                    );

                    DBGPLAIN(DBG_MA_CATALOG,
                        __FCN_FMT__ _T("[XX:XXXX] OCD for DA ID %d: chunk %d"), __FCN__,
                        STD2HOST_L(rec->daID),
                        STD2HOST_L(rec->dataSeq)
                    );

                    /* check if sequence number matches */
                    ErhAssert (
                        _T("MaListCatalogBlock"),
                        catDataExpectedSeq == STD2HOST_L(rec->dataSeq)
                    );

                    catData = REALLOC(catData, sizeof(catRecData) + STD2HOST_L(catData->dataLength) + STD2HOST_L(rec->dataLength));

                    ptr = (UCHAR *)(&catData->data[0]) + STD2HOST_L(catData->dataLength);

                    memcpy (
                        ptr,
                        ((UCHAR *)rec) + STD2HOST_S(rec->dataOffset),
                        STD2HOST_L(rec->dataLength)
                    );
    
                    catData->dataLength = HOST2STD_L(STD2HOST_L(catData->dataLength) + STD2HOST_L(rec->dataLength));
                    catData->recSize = HOST2STD_L(sizeof(catRecData) + STD2HOST_L(catData->dataLength));

                    catDataExpectedSeq++;
                }

                if (STD2HOST_L(rec->flags) & MA_REC_CAT_DATA_FLAG_LAST)
                {
                    /* this was last chunk of catalog data, dump it to catalog */

                    if (g_cat->catBuf == NULL)
                    {
                        g_cat->catSize  = MAX_CAT_SIZE;
                        g_cat->catBuf   = (UCHAR *)MALLOC(g_cat->catSize);
#if TARGET_NETWARE
                        if (catBuf == NULL)
                        {
                            DBGSTAMP(DBG_MA_CATALOG);
                            DBGPLAIN(DBG_MA_CATALOG,__FCN_FMT__ _T("MALLOC failed error==%d"), __FCN__, errno);
                            DBGFCNOUT();
                            return;
                        }
#endif
                        g_cat->catNext  = 0L;
                    }
                    else if ((g_cat->catNext
                            + sizeof(catRecData)
                            + STD2HOST_L(catData->dataLength)) > g_cat->catSize)
                    {
                        g_cat->catSize += MAX_CAT_SIZE;
                        g_cat->catBuf   = (UCHAR *)REALLOC(g_cat->catBuf, g_cat->catSize);
                    }

                    DBGPLAIN(DBG_MA_CATALOG,
                        __FCN_FMT__ _T("[XX:XXXX] Appending OCD for DA ID %d"), __FCN__,
                        STD2HOST_L(rec->daID)
                    );

                    memcpy(g_cat->catBuf + g_cat->catNext, (UCHAR *)catData, STD2HOST_L(catData->recSize));
                    g_cat->catNext += ROUND_UP(STD2HOST_L(catData->recSize));

                    FREE(catData);
                    catDataExpectedSeq = 0;
                }
                
                break;
            }
            /* case MA_REC_CAT_DATA */

        default:
            break;
        }   /* switch (STD2HOST_L(((MArecUniv *)recPtr) ... */

        recPtr += ROUND_UP(STD2HOST_L(((MArecUniv *)recPtr)->recHeader.recSize));
    }   /* while ((recPtr - bufPtr) < ... */
    stillProcSameCatBlk = false;

    DBGFCNOUT();
    return;
}   /* CatalogList */

#if !TARGET_NETWARE
/******************************************************************************
|
******************************************************************************/
/******************************************************************************
| Catalog TapeAnalyser
******************************************************************************/
/******************************************************************************
|
******************************************************************************/
/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     buffer         Buffer of read data
*
* @brief     Write out catalog records
*
*//*=========================================================================*/
void
CatalogTA_WriteCatalogRecords(UCHAR *buffer,
                    ULONG dataLength)
{
    ERH_FUNCTION(_T("CatalogTA_WriteCatalogRecords"));
    ULONG           rtype = 0;
    ULONG           recSize = 0;
    UCHAR           *nextRec = NULL;
    MArecUniv       *curRec = NULL;

    DBGFCNIN();

    nextRec = buffer + ROUND_UP(sizeof(MAblkHdr));
    dataLength -= ROUND_UP(sizeof(MAblkHdr));

    do
    {
        if (nextRec == NULL)
        {
            /* end of block */
            curRec = NULL;
            return;
        }
        else
        {
            curRec = (MArecUniv *) nextRec;
        }

        /* --- Calculate offset of next record ---------------------------- */

        recSize = ROUND_UP(STD2HOST_L(curRec->recHeader.recSize));
        nextRec += recSize;

        /* --- If done with block, set nextRecord to NULL ----------------- */
        if (STD2HOST_L(curRec->recHeader.recSize) >= dataLength)
        {
            /* If not precisely done with block, there must be error */
            if (STD2HOST_L(curRec->recHeader.recSize) > dataLength)
            {
                fprintf(stdout, _T("Corrupted record detected, size too big\n"));
                curRec = NULL;
                return;
            }
            nextRec = NULL;

        }
        if (STD2HOST_L(curRec->recHeader.recSize) == 0)
        {
            fprintf(stdout, _T("Corrupted record detected, size = 0\n"));
            return;
        }

        dataLength -= recSize;

        rtype = STD2HOST_L(curRec->recHeader.recType);
        DbgPlain (100, __FCN_FMT__ _T("RecordType(")ULONG_FMT _T(")"), __FCN__, rtype);


        if (MAOPT->rechdr)
        {
            printf(_T("\n%5d   "), STD2HOST_L(curRec->recHeader.recSeqNo));
            PrintRecType (         STD2HOST_L(curRec->recHeader.recType));
            PrintRecType (         STD2HOST_L(curRec->recHeader.recSubType));
            printf(_T("%8d"),      STD2HOST_L(curRec->recHeader.recSize));
            printf(_T("%8d"),      STD2HOST_L(curRec->recHeader.recVersion));
            printf(_T("%10x"),     STD2HOST_L(curRec->recHeader.magicCookie));
            printf(_T("%8x\n"),    STD2HOST_L(curRec->recHeader.recFlags));
        }

        if (rtype == MA_REC_CAT)
        {
            CatalogList_AppendCatRecToCatBuff((MArecCat *) curRec);
        }

        if (rtype == MA_REC_NT_CAT)
        {
            CatalogList_AppendNTCatRecToCatBuff((MArecNTCat *) curRec);
        }

    }
    while ( (curRec != NULL) && (dataLength > 0) );

    DBGFCNOUT();
    return;
}

/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     buffer         Buffer of readen data
*
* @retval    0              Failed
* @retval    1              Successed
*
* @brief     Write out information of the data and catalog blocks
*
*//*=========================================================================*/

int
CatalogTA_UnivBlock(UCHAR *buffer)
{

    ERH_FUNCTION(_T("CatalogTA_UnivBlock"));

    MAblkUniv   *univBlock = NULL;
    univBlock = (MAblkUniv *) buffer;

    DBGFCNIN();


    if (STD2HOST_L(univBlock->blkHeader.magicCookie) != MA_BLK_MAGIC_COOKIE)
    {
        fprintf(stderr, _T("Block not in %s format\n"),
        GetBrandString(BC_BRIEF_PRODUCT_NAME2));

        DbgPlain (100, __FCN_FMT__ _T("FUNCTION ended: != MA_BLK_MAGIC_COOKIE"), __FCN__);
        DBGFCNOUT();
        return (0);
    }

    if (STD2HOST_L(univBlock->blkHeader.blkType) != MA_BLK_CATALOG)
    {
        DbgPlain (100, __FCN_FMT__ _T("FUNCTION ended: != MA_BLK_CATALOG"), __FCN__);
        DBGFCNOUT();
        return (0);
    }

    if (MAOPT->blkhdr)
    {
        WriteOutBlockHeader (univBlock->blkHeader, printf);
    }

    if (STD2HOST_L(univBlock->blkHeader.blkType) == MA_BLK_CATALOG)
    {
        CatalogTA_WriteCatalogRecords (buffer, STD2HOST_L(univBlock->blkHeader.blkLength));
    }

    DBGFCNOUT();
    return (1);
}


/*========================================================================*//**
*
* @ingroup   Catalog Manager (Cat)
*
* @param     buffer         Buffer of readen data
* @param     dataLength     lenght of buffer
* @param     writeInfo      flag if the DA and session info will be written
* @param     recError       OUT pointer to number of errors
*
* @retval    0              Successed
* @retval    1              Failed
*
* @brief     Verifies catalog records (magic cookie, sequence)
*
*//*=========================================================================*/
int
CatalogTA_VerifyCatRec(UCHAR *buffer,
             ULONG dataLength,
             int writeInfo,
             ULONG *recError)
{

    ULONG           rtype = 0UL;
    ULONG           recSize = 0UL;
    UCHAR           *nextRec = NULL;
    MArecUniv       *curRec = NULL;

    static ULONG recSeqNo = 0;


    nextRec = buffer + ROUND_UP(sizeof(MAblkHdr));
    dataLength -= ROUND_UP(sizeof(MAblkHdr));

    do
    {
        if (nextRec == NULL)
        {
            /* end of block */
            curRec = NULL;
            return (1);
        }
        else
        {
            curRec = (MArecUniv *) nextRec;
        }

        /* --- Calculate offset of next record ---------------------------- */

        recSize = ROUND_UP(STD2HOST_L(curRec->recHeader.recSize));
        nextRec += recSize;

        /* --- If done with block, set nextRecord to NULL ----------------- */
        if (STD2HOST_L(curRec->recHeader.recSize) >= dataLength)
        {
            /* If not precisely done with block, there must be error */
            if (STD2HOST_L(curRec->recHeader.recSize) > dataLength)
            {
                printf(_T("Error: Catalog record, size too big\n"));
                curRec = NULL;
    return (1);
            }
            nextRec = NULL;

        }
        if (STD2HOST_L(curRec->recHeader.recSize) == 0)
        {
            printf(_T("Error: Catalog record, size = 0\n"));
            return (0);
        }

        dataLength -= recSize;

        rtype = STD2HOST_L(curRec->recHeader.recType);
        if (STD2HOST_L(curRec->recHeader.magicCookie) != MA_REC_MAGIC_COOKIE)
        {
            printf(_T("Error: Invalid catalog record magic cookie\n"));
            (*recError) += 1;
            recSeqNo = STD2HOST_L(curRec->recHeader.recSeqNo);
            return (0);
        }

        if (STD2HOST_L(curRec->recHeader.recSeqNo) != recSeqNo)
        {
            (*recError) += 1;
            printf(_T("Error: Invalid catalog record sequence number\n"));
            recSeqNo = STD2HOST_L(curRec->recHeader.recSeqNo);
            return (0);
        }

        if (rtype == MA_REC_SESS &&
            writeInfo == 1)
        {
            WriteSessionInfo ((MArecSess *)curRec);
        }

        if (rtype == MA_REC_DA &&
            writeInfo == 1)
        {
            WriteDAInfo ((MArecDA *)curRec, 1);
}

    }
    while ( (curRec != NULL) && (dataLength > 0) );

    return (0);
}
#endif /* TARGET_NETWARE */

#if XMA_NDMP
int copyCatBuf(UCHAR *cat_buffer, ULONG size)
{
    ERH_FUNCTION(_T("CatalogTA_UnivBlock"));

    if ( (g_cat->catNext + size) > g_cat->catSize)
    {
        ULONG newSize = g_cat->catNext + size;
        DbgPlain (100, __FCN_FMT__ _T("Resizing catalog, current size: ") ULONG_FMT _T(", buffer size: ") ULONG_FMT _T(", new size: ") ULONG_FMT,
                                     __FCN__,
                                     g_cat->catNext,
                                     size,
                                     newSize);

        g_cat->catBuf   = (UCHAR *)REALLOC(g_cat->catBuf, newSize);
        g_cat->catSize  = newSize;
    }
    memcpy(g_cat->catBuf + g_cat->catNext, cat_buffer, size);
    g_cat->catNext += size;
    return 0;

    return 1; /* OK */
}

static void ProcessChunk(const int firstChunk)
{
    ERH_FUNCTION(_T("ProcessChunk"));
    /* ---------------------------------------------
        1. SEND SESS/DACC for any chunk other than first
        2. Send FBIN (for first chunk only)
        3. Send Catalog chunk (free buffer)

        for session that spawns over two tapes
        fbin will be null for first tape
    ----------------------------------------------*/
    DBGPLAIN(DBG_MA_WORK, __FCN_FMT__ _T("Sending Chunk (size=")ULONG_FMT _T(")"), __FCN__, g_cat->catNext);

    if (!firstChunk)
    {
        CatalogList_Sess_DaCC(); /* send messages */
        /* fake sequence number */
        tmpSessRec->maCatSeqNo = HOST2STD_L(STD2HOST_L(tmpSessRec->maCatSeqNo) + 1);
    }

    CatalogList_SendCatBuf();   /* was duplicate code */
    Catalog_Restart();
}

#endif
