/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   Generic Parser
* @file      lib/parse/gen_parser.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Multifunctional Parser.
*            
*            19.12.2002 aleksander belov:  PLEncode, PLDecode rewritten, small bug corrected
*/

#include "lib/cmn/target.h"

__rcsId("$Header: /lib/parse/gen_parser.c $Rev$ $Date::                      $:");

#include "lib/cmn/common.h"
#include "lib/cmn/type_array.h"
#include "lib/cmn/type_variant.h"
#include "lib/cmn/token.h"
#include "gen_parser.h"
#include "parser_ptd.h"

#if TARGET_HPUX
#   include <iconv.h>
#endif

#define LOWER(c)    (CharIsUpper(c) ? tolower(c) : (c) )

ParserTable defOmniInfo[] = {
    {PLDOI_KEY      }, 
    {PLDOI_VERSION  }, 
    {PLDOI_DESC     },
    {PLDOI_NLSSET   },
    {PLDOI_NLSID    },
    {PLDOI_NTPATH   },
    {PLDOI_UXPATH   },
    {PLDOI_FLAGS    },
    { 0 }
};


tchar*
StrFindToken(const tchar *str, const tchar *delim, OUT tchar ** tend)
{
    str += strspn(str, delim); // skip leading <delim> chars
    if (!str[0]) 
        return NULL;

    *tend = (tchar*)str + strcspn(str, delim); // skip until first <delim> char
    return (tchar*)str;
}


static tchar*
GetNextWord(const tchar *p, OUT tchar ** next)  
{
    const tchar *word;

    if (StrIsEmpty(p)) 
        return NULL;

    word = StrFindToken(p, UWORDDELIM, next);

    if (word==NULL) return NULL;

    if (word[0] == UCQUOTE)
    {
        word = TCSINC(word);                /* skip quote                   */

        if (word[0] == UCQUOTE)             /* token is "" empty we cannot  */
        {                                   /* return NULL, set the *len=0  */
            *next = (tchar*)TCSINC(word);   /* to indicate empty string     */
            return (*next);   
        }
        word = StrFindToken(word, UQUOTE, next);  /* find next quote  */
    }

    return (tchar*)word;
}


/* ===========================================================================
|   FUNCTION
|       
|   DESCRIPTION
|       
|   ARGUMENTS
|       
|   RETURNS
|       
 ========================================================================== */
static tchar *StrBaseWord(MODIFIED tchar *s)
{
    tchar *d = s;
    while (*s && *s != '[')
    {
        s = TCSINC(s);
    }

    if (*s == '[')
        *s = '\0';

    return d;
}

/*========================================================================*//**
*
* @retval    1 if matched,
* @retval    0 otherwise
*
* @brief     Matches two keywords
*
*//*=========================================================================*/
int KwMatch (const tchar *s, const tchar *pat)
{
    int optional = 0;
    int optionalS = 0;

    if(!pat && !s) return 1;
    if(!pat || !s) return 0;
    
    while (1)
    {
        while (*s && (*pat != _T('[')) 
            && LOWER(*pat) == LOWER (*s))
        {
            s = TCSINC(s);
            pat = TCSINC(pat);
        }

        if (*pat == _T('['))
        {
            /* The remainder of the pattern is optional */
            optional = 1;
            pat = TCSINC(pat);
        }
        if (*s == _T('['))
        {
            /* The remainder of the pattern is optional */
            optionalS = 1;
            s = TCSINC(s);
        }

        if (optionalS && optional)
            return 1;

        if (*s == UCZERO)
        {
            /* Done with keyword string, decide if we match */
            return  (*pat == _T('\0') || optional);

        }
        else if (*pat == _T('\0'))
        {
            /* End of pattern, but not keyword string */
            return 0;
        }
        else if (!(optional && LOWER(*pat) == LOWER (*s)))
        {
            /* Not the end of either and still no match */
            return 0;
        }
    }
}


/* ===========================================================================
   ==================                         ================================
   ================== Parser Table Functions  ================================
   ==================                         ================================
   =========================================================================== */


/*========================================================================*//**
*
* @brief     Creates new ParserTable item
*
*//*=========================================================================*/
ParserTable * PTNewItem()
{
    ParserTable * pt = CALLOC(1, sizeof(ParserTable));

    pt->nlsSet = pt->nlsId = -1;

    return pt;
}


/*========================================================================*//**
*
* @param     pt(I/O) pointer to parser table list
*
* @brief     Frees ParserTable list
*
*//*=========================================================================*/
int PTFree(ParserTable * pt)
{
    int i;

    for(i=0; pt[i].key; i++)
    {
        FREE(pt[i].desc); 
        FREE(pt[i].key); 
        FREE(pt[i].param2);
        FREE(pt[i].param1);
    }

    FREE(pt);

    return 1;
}

/*========================================================================*//**
*
* @brief     Appends one ParserTable to another
*
*//*=========================================================================*/
int PTAppend(ParserTable ** to, ParserTable * from)
{
    int i,j;

    for(i=0; (*to) && ((*to)[i].key); i++);
    for(j=0; from[j].key; j++);

    *to = REALLOC(*to, sizeof(ParserTable)*(i+j+1));
    if(*to==NULL) return 0;

    for(j=0; from[j].key; j++)
    {
        (*to)[i+j].key      = StrNewCopy(from[j].key);
        (*to)[i+j].value    = StrNewCopy(from[j].value);
        (*to)[i+j].desc     = StrNewCopy(from[j].desc);
        (*to)[i+j].nlsId    = from[j].nlsId;
        (*to)[i+j].nlsSet   = from[j].nlsSet;
        (*to)[i+j].param1   = StrNewCopy(from[j].param1);
        (*to)[i+j].param2   = StrNewCopy(from[j].param2);
        (*to)[i+j].flags    = from[j].flags;
    }
    (*to)[i+j].key = NULL;
    return 1;
}

/*========================================================================*//**
*
* @param     word(I) - keyword
* @param     pt(I)   - pointer to ParserTable List
*
* @retval    -1 - cannot find string in ParserTable List
|      >0  - index in ParserTableList
*
* @brief     Finds a ParserTable item by the specified key
*
*//*=========================================================================*/
int
PTFind(const tchar *word, const ParserTable *pt)
{
    int i;

    if (*word == UCMINUS) 
        word = TCSINC(word);  

    if (StrIsEmpty(word)) 
        return -1;

    if (!pt)
        return -1;

    for(i=0; pt[i].key; i++) 
    {
        if (KwMatch(word,pt[i].key)) 
        {
            return i;
        }
    }

    return -1;
}


/*========================================================================*//**
*
* @param     pt(I)   - pointer to ParserTable List
*
* @retval    number of items in ParserTable List
*
*//*=========================================================================*/
int
PTGetItemCount(const ParserTable * pt)
{
    int i = 0;

    if (!pt)
        return 0;

    for (i = 0; pt[i].key; i++);

    return i;
}

/*========================================================================*//**
*
* @param     pt(I)   - pointer to ParserTable List
* @param     ptSize(I)-number of items in ParserTable List
*
* @retval    1 - OK
* @retval    0 - FALSE
*
* @brief     check if the given index is correct for ParserTable List
*
*//*=========================================================================*/
static int PTCheckIndex(const ParserTable *pt, int ptSize, int index)
{
    if (pt && (index < ptSize) && (index >= 0))
        return 1;

    return 0;
}


static LONG PTGetFlags(const ParserTable *pt, const tchar* key)
{
    LONG i = PTFind(key, pt);
    if (i == -1) 
        return CIF_DEFAULT;
    return pt[i].flags;
}


/* ===========================================================================
   ==================                         ================================
   ================== Parser Object Functions ================================ 
   ==================                         ================================
   =========================================================================== */

/*========================================================================*//**
*
* @retval    pointer to a newly created PObj
*
* @brief     Creates and initializes a new PObj - ParserObject
*
*//*=========================================================================*/
PObj* PONewItem()
{
    PObj *po = CALLOC(1, sizeof *po);
    po->index = -1;
    return po;
}


/*========================================================================*//**
*
* @param     key(I)  - pointer to key
* @param     value(I)- pointer to value
* @param     index(I)- index in ParserTable List
*
* @retval    pointer to newly created PObj
*
* @brief     creates new PObj with specified key, value and index
*
*//*=========================================================================*/
PObj*
PONew(const tchar *const key,
      const tchar *const value,
      const int index)
{
    PObj * p;

    if(key == NULL) 
        return NULL;  /* key can't be empty */

    p = PONewItem();

    p->key   = StrNewCopy(key);
    p->value = StrNullCopy(value);
    p->index = index;
    return p;
}


/*========================================================================*//**
*
* @param     po(I/O) - pointer to the PObj
*
* @retval    NULL
*
* @brief     Destroys the specified PObj
*
*//*=========================================================================*/
PObj* POFree(PObj *po)
{
    FREE(po->key);
    FREE(po->value);
    
    FREE(po);

    return NULL;
}

/*========================================================================*//**
*
* @param     po1(I)  - pointer to first object
* @param     po2(I)  - pointer to second object
*
* @retval    1 - objects are different
* @retval    0 - objects are identical
*
* @brief     Compares two objects
*
*//*=========================================================================*/
int POCmp(const PObj *po1, const PObj *po2)
{
    if (!po1 && !po2)
        return 1;

    if (!po1 || !po2)
        return 0;

    if (!StrICmp(po1->key, po2->key)) 
        return 1;

    if (!StrICmp(po1->value, po2->value)) 
        return 1;

    if (po1->index != po2->index)
        return 1;


    return 0;
}

/*========================================================================*//**
*
* @param     src - pointer to existing PObj to be copied
*
* @retval    pointer to the copied PObj or NULL if error
*
* @brief     Allocates memory and copies there existing PObj
*
*//*=========================================================================*/
PObj * PONewCopy(const PObj *src)
{
    PObj * p1 = PONewItem();
    if (POCopy(p1, src))
        return p1;

    FREE(p1);

    return NULL;
}

/*========================================================================*//**
*
* @param     src(I) - pointer to source PObj
* @param     dest(I/O) - pointer to existing of destination PObj
*
* @retval    0 FAILED
* @retval    1 successfully finished
*
* @brief     copies PObj to given destination
*
*//*=========================================================================*/
int POCopy(PObj *dest, const PObj *src)
{
    if (!dest || !src) 
        return 0;

    dest->key   = StrNewCopy(src->key);
    dest->value = StrNullCopy(src->value);
    dest->index = src->index;

    return 1;
}


/**
 * @brief Determines whether the host is a virtual host entry.
 * @param fHost  The host to inspect
 * @return TRUE if this is a virtual host entry.
 *
 * A virtual host entry holds configuration data about a host, which cannot
 * be reached by the CRS, because it lacks a core component and thus an INET.
 *
 * For historic reasons the CRS assumes all entries in cell_info are reachable
 * by INET, i.e. have the core component installed. Therefore the cell_info file
 * has no separate entry for core.
 *
 */
int
POIsVirtualHost(const PObj* po)
{
    /*
     * Integrations may use the cell_info file to record information about hosts,
     * which not necessarily have the core package installed. This does not mean
     * you cannot install other components, which will install core. Therefore
     * testing of the presence of such a component is not a reliable means to
     * determine whether a core package is installed or not.
     *
     * This implementation relies on the behavior that the core package will set the
     * value of the -os field to a valid OS, while it has the value PLCIK_VMWAREHOST
     * for virtual hosts.
     */
    return (StrCmp(PLGetValue(po, PLCIK_OS), PLCIK_VMWAREHOST) == 0) ||
           (PLGetValue(po, PLCIK_VIRTUAL_HOST) != NULL);
}


/* ===========================================================================
   ==================                         ================================
   ================== Parser List Functions   ================================ 
   ==================                         ================================
   =========================================================================== */


/* ===========================================================================
|   FUNCTION: 
|   DESCRIPTION
|       
|   ARGUMENTS
|       
|   RETURNS
|       
 ========================================================================== */
ParserList * PLNew()
{
    return CALLOC(1, sizeof(ParserList));
}


/*========================================================================*//**
*
* @param     pl - pointer to ParserList
*
* @retval    0 - FAILED
* @retval    1 - OK
*
* @brief     Frees ParserList and all PObj objects linked to it
*
*//*=========================================================================*/
int PLFree(ParserList * pl)
{
    PObj * t, *t2, *po;

    if (pl==NULL) 
    {
        return 0;
    }

    if (pl->flags & PL_PTAUTOCREATED)
        PTFree(pl->pt);

    po = pl->start;

    while(po)
    {
        t=po;
        po=po->child;

        while(t)
        {
            t2 = t;
            t = t->next;

            POFree(t2);
        }
    }

    FREE(pl);
    return 1;
}

/*========================================================================*//**
*
* @param     pl(I) - Pointer to ParserList structure
*
* @retval    pointer to newly created ParserList or NULL if failed
*
* @brief     Creates a new copy of ParserList structure and all tree of PObj objects
*
*//*=========================================================================*/
ParserList * PLNewCopy(const ParserList * pl)
{
    ParserList * pnew;
    PObj *row, *col, *tmp, *por=NULL, *poc=NULL;
    int stcol, strow;
    
    if (pl==NULL) return NULL;

    pnew = PLNew();

    if (pl->flags & PL_PTAUTOCREATED)
        PTAppend(&(pnew->pt), pl->pt);
    else
        pnew->pt = pl->pt;

    pnew->ptSize = pl->ptSize;

    strow=1;
    for(row=pl->start; row; row=row->child)
    {
        stcol=1;
        for(col=row; col; col=col->next)
        {
            tmp = PONewCopy(col);

            if (stcol)
            {
                stcol=0;
                if (!strow)
                {
                    tmp->parent=por;
                    por->child=tmp;
                }
                poc=por=tmp;
                pnew->end=tmp;

                if (strow)
                {
                    strow=0;
                    pnew->start=tmp;
                }
            }
            else
            {
                tmp->prev=poc;
                poc->next=tmp;
                poc=tmp;
            }

        }
    }
    return pnew;
}

/* ===========================================================================
|   FUNCTION: 
|   DESCRIPTION
|       
|   ARGUMENTS
|       
|   RETURNS
|       
 ========================================================================== */
PObj*
PLCopyLine(const PObj *pObj)
{
    const PObj *p = pObj;
    PObj *newCopyStart = NULL, *newCopyEnd = NULL;

    while (p)
    {
        PObj *newObj = PONewCopy(p);
        if (!newCopyStart)
        {
            newCopyStart = newCopyEnd = newObj;
        }
        else
        {
            newObj->prev = newCopyEnd;
            newCopyEnd->next = newObj;
            newCopyEnd = newObj;
        }

        p = p->next;
    }

    return newCopyStart;
}

/*========================================================================*//**
*
* @param     buff(I) - string to be parsed
* @param     keys(I) - string which defines keys for parsing the buffer
*
* @retval    pointer to the ParserList (parsed buffer)
*
* @brief     And here finally goes the function which takes only two strings - one
*            is rules for parsing (in defOmniInfo format,
*            i.e. "-key debug -flags 3\n-key in\n...") another is a string to be
*            parsed using these rules (ex. "-debug 1-99 -in input.txt ...")
*
*//*=========================================================================*/
ParserList * PLParse(ctchar *keys, ctchar *buff)
{
    ERH_FUNCTION(_T("PLParse"));
    ParserList * pl;
    ParserTable * pt;

    DbgFcnIn();

    if(!keys || !buff) RETURN(NULL);

    pl = PLGetListFromBuffer(defOmniInfo, keys);
    if (!pl) RETURN(NULL);

    pt = PLtoPTAll(pl,      defOmniInfo[0].key, defOmniInfo[1].key, 
        defOmniInfo[2].key, defOmniInfo[3].key, defOmniInfo[4].key, 
        defOmniInfo[5].key, defOmniInfo[6].key, defOmniInfo[7].key);

    PLFree(pl);

    if(!pt) RETURN(NULL);

    pl = PLGetListFromBuffer(pt, buff);

    if (!pl)
    {
        PTFree(pt);
        RETURN(NULL);
    }

    pl->flags |= PL_PTAUTOCREATED;
    RETURN(pl);
}

/*========================================================================*//**
*
* @param     buff(I) - pointer to the start of a buffer to parse
* @param     pt(I)   - pointer to the ParserTable array
*
* @retval    pointer to a created ParserList structure
*
* @brief     Parses the input buffer into a ParserList structure which points to
*            linked list of PObj objects.  To do so the function must know the list
*            of "right" keys which are passed to it as a ParserTable array which can
*            be either loaded from file or it can be a predefined structure (like
*            defCellInfo - the ParserTable structure filled with all information
*            needed to parse a cell_info file). If the buffer has BOM at the beginning
*            function skips it automatically, remembering that the buffer was in
*            unicode format by setting PL_UNICODE flag in pl->flag
*
*            All of the created objects (ParserList, PObj, strings pointed by PObj)
*            must be freed after use.  This can be done by calling PLFree function
*
*//*=========================================================================*/
ParserList * PLGetListFromBuffer(_In_ const ParserTable *pt, _In_opt_ const tchar* buff)
{
    ERH_FUNCTION(_T("PLGetListFromBuffer"));
    const tchar *ptr, *line;
    tchar *next;
    tchar eol;

    ParserList *pl;

    DbgFcnIn();

    if (StrIsEmpty(buff)) 
        RETURN(NULL);

    pl = PLNew();
    if (pl == NULL)
        RETURN(NULL);

    if (IsStringMarkedUnicode(buff))
    {
        buff = TCSINC(buff);
        pl->flags |= PL_UNICODE;
        DbgPlain(20, _T("This is Unicode string!"));
    }

    pl->pt = (ParserTable*)pt;
    pl->ptSize = PTGetItemCount(pt);

    ptr = line = buff;

    while(*ptr)
    {
        line = StrFindToken(ptr, UNEWLINEDELIM, &next);

        if (line == NULL) 
            break;

        eol = next[0];
        next[0] = UCZERO;

        PLParseLine(pl, line);
        next[0] = eol;

        ptr = next;
    }

    PLDbgPlain(299, _T("ParserList parsed"), pl);

    pl->current = pl->start;
    RETURN(pl);
}


/*========================================================================*//**
*
* @param     pl(I)   - ParserList with linked ParserTable
* @param     line(I) - line of buffer to parse
*
* @retval    pointer to the last created object PObj
*
* @brief     Parses the given buffer (must be one line, i.e without newline chars)
*            and appends the data to the end of ParserList pl structure.
*
*//*=========================================================================*/
PObj * PLParseLine(ParserList * pl, const tchar * line)
{
    tchar *word = NULL;
    tchar tc;

    PObj *tmp = NULL;

    tchar *next    = NULL;
    tchar *value   = NULL;
    tchar *key     = NULL;
    int ptf = -1, oldPtf = -1;


    pl->current = NULL; /* for adding new line */
    while(word = GetNextWord(line, &next), word)
    {
        tc = next[0];       /* break line to get word */
        next[0] = UCZERO;   /* insert \0 in string   */
        
        ptf = (*word == UCMINUS) ? PTFind(word, pl->pt) : -1;
        if (ptf != -1)
        {
            if (value)
            {
                if (tmp)
                {
                    if ((oldPtf != -1) && pl->pt[oldPtf].flags & CIF_ENCODE)
                    {
                        tmp->value = PLDecode(value);
                        FREE(value);
                    }
                    else
                        tmp->value = value;
                }
                else 
                    FREE(value);

                value = NULL;
            }

            key = TCSINC(word);

            tmp = PONew(key, value, ptf);
            PLInsertObj(pl, tmp);

            oldPtf = ptf; /* save last found table index - in case that we need to encode value */
        }  
        else
        {  /* a value */
            if(value)
                StrAppend(value, USPACE);

            StrAppend(value, word);
        }
        next[0] = tc; /* return line back */
        line = next;

        if (tc==UCQUOTE) 
            line = TCSINC(line);
    }
    
    if (value && tmp) 
    {
        if ((oldPtf != -1) && pl->pt[oldPtf].flags & CIF_ENCODE)
        {
            tmp->value = PLDecode(value);
            FREE(value);
        }
        else
            tmp->value = value;
    }

    return tmp;
}

/*========================================================================*//**
*
* @param     po    - pointer to the first PObj in the line from which the
*                    conversion should start
* @param     pl    - ParserList, needed for ParserTable flags information
*                    (used for quotation)
*
* @retval    1 - operation completed successfully
* @retval    0 - error encountered
*
* @retval    buff  - pointer to the beginning of the allocated buffer
*
* @brief     converts PL line to one string text format (cell_info format)
*
*//*=========================================================================*/
static int 
PLGet(Variant *out, const ParserList *pl, const PObj *po)
{
    const PObj *p = po;
    LONG flags = 0;

    while(p)
    {
        VarPuts(out, UMINUS);
        VarPuts(out, StrBaseWord(p->key));
        VarPuts(out, USPACE);
        
        if ((p->index == -1) || !pl->pt)
            flags = PTGetFlags(pl->pt, p->key);
        else
            flags = pl->pt[p->index].flags;

        if (!(flags & CIF_NOTQUOTED) && !(flags & CIF_QUOTED))
            flags |= StrChr(p->value, UCSPACE) ? CIF_QUOTED : CIF_NOTQUOTED;

        if (flags & CIF_QUOTED)
            VarPuts(out, UQUOTE);

        if(!StrIsEmpty(p->value))
        {
            if (flags & CIF_ENCODE)
            {
                VarPuts(out, PLEncodex(p->value));
            }
            else
                VarPuts(out, p->value);
        }

        if (flags & CIF_QUOTED)
            VarPuts(out, UQUOTE);

        p = p->next;

        if (p)
            VarPuts(out, USPACE);
    }

    return 1;
}


int 
PLGetBufferFromLine(OUT MALLOCED tchar **buff, const ParserList *pl, const PObj *po)
{
    Variant out = {0};
    int r = PLGet(&out, pl, po);
    *buff = V2T(&out);
    return r;
}


/*========================================================================*//**
*
* @param     pl(I) - pointer to the ParserList structure to be converted to cell
*                    info format.  ParserTable must be linked to ParserList structure
*                    (if the pl variable is a result of work of PLGetListFromBuffer
*                    function then it's allright - ParserTable is already linked)
*                    This is needed to know which values should be quoted.  If
*                    ParserTable was not specified then values will always be quoted
*                    (ex.  -da "A.03.50") if pl->flag PL_UNICODE was set function will
*                    write BOM mark at the beginning of the buffer.
* @param     buff(O) - pointer to the allocated buffer
*
* @retval    1 - operation completed successfully
* @retval    0 - error encountered
*
* @brief     The opposite to PLGetListFromBuffer.  Prepares the buffer using the
*            ParserList structure, made by PLGetListFromBuffer
*
*//*=========================================================================*/
int PLGetBufferFromList(tchar **buff, ParserList * pl)
{
    ERH_FUNCTION(_T("PLGetBufferFromList"));
    Variant out = {0};

    DbgFcnIn();

    if (pl == NULL) 
        RETURN(0);

    if (pl->start == NULL)
        RETURN(0);

    #ifdef UNICODE
    if (pl->flags & PL_UNICODE)
    {
        tchar bom[] = {BOM, UCZERO};
        VarPuts(&out, bom);
    }
    #endif

    pl->current = pl->start;
    while (pl->current)
    {
        PLGet(&out, pl, pl->current); 
        pl->current = pl->current->child;
        
        if (pl->current)
            VarPuts(&out, UNEWLINE);
    }

    *buff = V2T(&out);

    RETURN(1);
}

/*========================================================================*//**
*
* @param     pl(I/O) - any element in the line where the element should be added
* @param     p(I)    - PObj element to be linked as a last PObj in the line
*
* @retval    p if success, NULL if one of the arguments missing
*
* @brief     Adds a new element at the end of specified line
*
*//*=========================================================================*/
PObj * PLAdd(PObj * pl, PObj * p)
{
    if (!pl || !p) 
        return NULL;

    for(; pl->next; pl=pl->next);
    pl->next=p; p->prev=pl;
    return p;
}

/*========================================================================*//**
*
* @param     pl(I/O) - parser list where the element should be added
* @param     p(I)    - PObj element to be insertid in the line
*
* @retval    p if success, NULL if one of the arguments missing
*
* @brief     Insert a new element (sorted by index) in pl->current line
*
*//*=========================================================================*/
PObj * PLInsertObj(ParserList * pl, PObj * po)
{
    PObj *old = NULL;
    PObj *pc  = NULL;

    if (!pl || !po) 
        return NULL;

    if (po->index == -1)
        po->index = PTFind(po->key, pl->pt);

    if (!pl->current)
        return PLAddLine(pl, po);

    pc = pl->current;
    while (pc)
    {
        if (po->index < pc->index)
            break;

        old = pc;
        pc = pc ->next;
    }

    po->next = pc;
    po->prev = old;
    if (pc)
        pc->prev = po;

    if (old) 
    {/* add object somewhere in the line not at the beginning */
        old->next = po;
    }
    else
    {/* add object at the beginning of line*/
        if (pc)
        {
            po->parent = pc->parent;
            po->child = pc->child;
            if (pc->parent) pc->parent->child = po;
            if (pc->child) pc->child->parent = po;
        }
        
        if (!pl->start || (pl->start == pc))
            pl->start = po;

        if (!pl->current || (pl->current == pc))
            pl->current = po;

        if (!pl->end || (pl->end == pc))
            pl->end = po;
    }

    return po;
}

/*========================================================================*//**
*
* @param     pl(I/O) - pointer to ParserList structure
* @param     p(I)    - PObj element to be linked as a last row
*
* @retval    p if success, NULL if one of the arguments missing
* @retval    pl->current will be set to p
*
* @brief     Adds a new row at the end of the parserList structure
*
*//*=========================================================================*/
PObj * PLAddLine(ParserList * pl, PObj * p)
{
    if (!pl || !p) return NULL;

    if (pl->end)
    {
        p->parent = pl->end;
        pl->end->child = p;
        pl->current = pl->end = p;
    }
    else
    {
        pl->current = pl->end = pl->start = p;
    }
    
    return p;
}

/*========================================================================*//**
*
* @param     pl(I/O) - pointer to ParserList structure
* @param     p(O)    - PObj element to be linked as a last row
*
* @retval    p if success, NULL if one of the arguments missing
* @retval    pl->current will be set to p
*
* @brief     In case that new line already inserted remove it and add a new line
*            at the end of the parserList structure
*
*//*=========================================================================*/
/* array of values to preserve */
ctchar * const PLPreservedKeys[] = 
{
    PLCIK_EMAIL,                   /* -email */
    PLCIK_ENCRYPTION,              /* -encryption */
    PLCIK_APPSERVER_SUBTYPE,       /* -appserver_subtype */
    PLCIK_APPSERVER_TYPE,          /* -appserver_type */
    PLCIK_APPSERVER_INT_SECURITY,  /* -appserver_integrated_security */
    PLCIK_APPSERVER_CERT,          /* -appserver_cert */
    PLCIK_APPSERVER_PASS,          /* -appserver_pass */
    PLCIK_APPSERVER_PORT,          /* -appserver_port */
    PLCIK_APPSERVER_USER,          /* -appserver_user */
    PLCIK_APPSERVER_WEB_ROOT,      /* -appserver_web_root */
    PLCIK_APPSERVER_ORGANIZATION,  /* -appserver_organization */
    PLCIK_APPSERVER_VEPAGREPLUGIN, /* -appserver_vepagre_plugin */
    NULL
};



PObj*
PLReplaceLine(ParserList *pl, PObj *p)
{
    PObj *pObj = NULL;
    PObj *pObjItem = NULL;
    
    int i = -1;
    
    if (!pl || !p) 
        return NULL;
    

    /* preserve options (e.g.: -email, -encryption) if they exists in old line and does not in new */
    if ((pObj = PLFindLine(pl, p->value)) != NULL)
    {
        for (i=0; PLPreservedKeys[i]; ++i)
        {
            ctchar *preservedKey = PLPreservedKeys[i];
            
            if ((pObjItem = PLFindObj(pObj, preservedKey)) != NULL && 
                PLFindObj(p, preservedKey) == NULL) 
            {
                PLAdd(p, PONew(preservedKey, pObjItem->value, -1));
            }
        }    
    }

    PLRemoveLineByKey(pl, p->value);
    PLAddLine(pl, p);

    return pl->current;
}


/*========================================================================*//**
*
* @param     po - PObj to be removed from ParserList structure
*
* @retval    NULL - error
* @retval    PObj* - pointer to the next object (or child if there is no next)
*
* @brief     Removes the specified PObj
*
*//*=========================================================================*/
PObj * PLRemoveObj(PObj * po)
{
    PObj * p;

    if(po==NULL) return 0;

    p = po->next;

    if(po->parent) 
        po->parent->child = po->child;

    if (po->child)
        po->child->parent = po->parent;

    if(po->prev) 
        po->prev->next = po->next;

    if(po->next)
        po->next->prev = po->prev;

    POFree(po);

    return p;
}

/*========================================================================*//**
*
* @param     po(I/O) - pointer to any object in the line
* @param     key(I)  - key name of the object to be removed
*
* @retval    1 found and removed
* @retval    0 not found
|      -1 error
*
* @brief     Searches and removes the element in the given ParserList line.
*
*//*=========================================================================*/
int
PLRemoveObjByKey(PObj * po, const tchar * key)
{
    po=PLFindObj(po, key);

    if(!po) return 0;

    PLRemoveObj(po);

    return 1;
}

/*========================================================================*//**
*
* @param     po(I/O) - pointer to any object in the line
* @param     key(I)  - key name of the object to be removed
* @param     value(I)- value of the object to be removed
*
* @retval    1 found and removed
* @retval    0 not found
|      -1 error
*
* @brief     Searches and removes the element in the given ParserList line.
*
*//*=========================================================================*/
int PLRemoveObjByValue(PObj * po, const tchar * key, const tchar *value)  
{
    po=PLFindObjByValue(po, key, value);

    if(!po) return 0;

    PLRemoveObj(po);

    return 1;
}

/*========================================================================*//**
*
* @param     pl(I/O) - pointer to ParserList structure
* @param     po(I)   - pointer to any object of the line to be removed
*
* @retval    1 found and removed
* @retval    0 not found
|      -1 error
*
* @brief     Removes the selected line from the ParserList structure
*
*//*=========================================================================*/
int PLRemoveLine(ParserList *pl, PObj * po)
{
    if (!po)
        return -1;

    if (pl)
    {
        if (pl->start == po)
            pl->start = po->child;

        if (pl->end == po)
            pl->end = po->parent;
    }
    
    while(po)
    {
        po = PLRemoveObj(po);
    }

    if (pl)
        pl->current = pl->start;

    return 1;
}

/*========================================================================*//**
*
* @param     pl(I/O) - pointer to ParserList structure
* @param     key(I)  - key name of the first object in line to be removed
*
* @retval    1 found and removed
* @retval    0 not found
|      -1 error
*
* @brief     Removes the selected line from the ParserList structure
*
*//*=========================================================================*/
int PLRemoveLineByKey(ParserList *pl, const tchar * key)
{
    PObj *pObj = PLFindLine(pl, key);
    if (!pObj)
        return 0;

    return PLRemoveLine(pl, pObj);
}

/*========================================================================*//**
*
* @param     po      - pointer to any item in the line of PObjs
* @param     key     - pointer to a key to be found in line
*
* @retval    1 success
* @retval    0 cannot find object
*
* @retval    value - pointer to a value in the found PObj
*
* @brief     Searches for the key in the given line in Parserlist. The input parameter
*            is a pointer to the start of linked list of PObj items.  It is built by the
*            PLGetListFromBuffer function
*
*//*=========================================================================*/
int
PLFindValue(const PObj *po, const tchar *key, OUT tchar ** value)
{ 
    po = PLFindObj(po, key);

    if(po)
    {
        *value = (tchar*)po->value;
        return 1;
    }

    *value = NULL;
    return 0;
}


/*========================================================================*//**
*
* @param     pl      - pointer to ParserList
* @param     key     - the string to be found in the current line
*
* @retval    1 success (found the key)
* @retval    0 error or not found
*
* @retval    value   - the address of the value is written
*
* @brief     Searches for the specified key in the pl->current PL line
*
*//*=========================================================================*/
int PLFindCurrentValue(ParserList * pl, tchar * key, tchar ** value)
{
    return PLFindValue(pl->current, key, value);
}

/*========================================================================*//**
*
* @param     po     - pointer to any item in the line of PObjs
* @param     key    - pointer to a key to be found in line
*
* @retval    pointer to PObj or NULL if not found
*
* @brief     Searches specified PL line, for PObj that has the specified key
*
*//*=========================================================================*/
PObj*
PLFindObj(const PObj * po, const tchar *key)
{
    if (!po || !key) return NULL;
    for(; po->prev; po=po->prev);
    for(; po && !KwMatch(po->key, key); po=po->next);

    return (PObj*)po;
}

/*========================================================================*//**
*
* @param     po      - pointer to any item in the line of PObjs
* @param     key,value - pointers to the key and value of PObj to be found
*
* @retval    pointer to PObj or NULL if not found
*
* @brief     Searches for the PObj with the specified key and value.
*
*//*=========================================================================*/
PObj*
PLFindObjByValue(PObj * po, const tchar * key, const tchar *value)
{
    if (!po || !key) return NULL;
    for(; po->prev; po=po->prev);
    for(; po; po=po->next)
    {
        if (KwMatch(po->key, key) && !StrICmp(po->value, value))
            break;
    }

    return po;
}

/* ===========================================================================
|   FUNCTION: PLFindLine
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
PObj*
PLFindLine(ParserList *pl, const tchar *value)
{
    ParserList *p = pl;
    
    if (!pl)
        return NULL;

    p->current = pl->start;

    while (p->current)
    {
        if ( KwMatch(PLGetValue(pl->current, p->pt[0].key), value) )
            return p->current;

        p->current = p->current->child;
    }

    return NULL;
}

/* ===========================================================================
|   FUNCTION:
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
PObj* PLFindLineByKey(ParserList *pl, const tchar *key)
{
    ParserList *p = pl;
    
    if (!pl)
        return NULL;

    p->current = pl->start;

    while (p->current)
    {
        if (PLFindObj(p->current, key))
            return p->current;

        p->current = p->current->child;
    }

    return NULL;
}

/* ===========================================================================
|   FUNCTION:
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
PObj*
PLSetValue(PObj * po, const tchar * key, const tchar *value)
{
    PObj *p;
    p = PLFindObj(po, key);

    if (!p)
        return(PLAdd(po, PONew(key, value, -1)));
    else
    {
        FREE(p->value);
        p->value = StrNewCopy(value);
        return(p);
    }
}

/* ===========================================================================
|   FUNCTION:
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
PObj*
PLSetValueI(PObj *po, const tchar * key, LONG i)
{
    tchar num[STRLEN_INT + 1];
    sprintf(num, _T("%d"), i);
    return(PLSetValue(po, key, num));
}

/* ===========================================================================
|   FUNCTION:
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
tchar*
PLGetValue(const PObj * po, const tchar * key)
{
    tchar *t = NULL;

    PLFindValue(po, key, &t);
    return t;
}

/*========================================================================*//**
*
* @param     v(I)    - pointer to string
* @param     ok(O)   - returned value are correct
*
* @retval    converted value
*
* @brief     convert value to LONG and return it.
*            In case that string starts with '0x' use function for Hex conversion;
*            In case that string not start with '0x' and start with '0' use function
*            for Binary conversion otherwise use fonction standard conversion
*
*//*=========================================================================*/
static LONG
PLStrToLong(const tchar *val, int *ok)
{
    LONG r = 0;

    tchar *pt = NULL;
    if (!val)
    {
        *ok = 0;
        return 0;
    }

    if (val[0] != _T('0'))
    {
        r = strtoul(val, &pt, 10);
        *ok = StrIsEmpty(pt);
        return r;
    }

    if ((val[1] == _T('x')) || (val[1] == _T('X')))
    {
        r = strtoul(val, &pt, 16);
        *ok = StrIsEmpty(pt);
        return r;
    }

    r = strtoul(val, &pt, 2);
    *ok = StrIsEmpty(pt);
    return r;
}


/*========================================================================*//**
*
* @param     po(I)   - pointer to the object
* @param     key(I)  - keyword
*
* @retval    converted value
*
* @brief     find object with keyword (key) convert value to LONG and return it
*
*//*=========================================================================*/
LONG
PLGetValueI(const PObj * po, const tchar * key)
{
    int ok = 0;
    tchar * t = PLGetValue(po, key);

    return PLStrToLong(t, &ok);
}


/*========================================================================*//**
*
* @param     pl(I)   - ParserList structure
*
* @retval    number of lines in pl
*
* @brief     Counts how many lines there are in the ParserList structure
*
*//*=========================================================================*/
int
PLGetNumOfLines(const ParserList* pl)
{
    int count = 0;
    const PObj *p = NULL;

    if (!pl) return 0;

    p = pl->start;
    while (p)
    {
        count++;
        p = p->child;
    }

    return count;
}

/*========================================================================*//**
*
* @param     pl(I)   - ParserList structure
* @param     objKey(I)- pointer to the key to be found in line
*
* @retval    number of rows in pl which cantains object wit key obKey
*
* @brief     Counts how many lines/rows there are in the ParserList structure
*            which contains object with the same key as objKey
*
*//*=========================================================================*/
int
PLGetNumOfLinesByKey(const ParserList* pl, const tchar* objKey)
{
    int count = 0;
    const PObj *p = NULL;

    if (!pl) return 0;

    p = pl->start;
    while (p)
    {
        if (PLFindObj(p, objKey))
            count++;
        p = p->child;
    }

    return count;
}

/*========================================================================*//**
*
* @param     pl - parserlist
* @param     key, flags - names of keywords in ParserList who's values
*                         are going to become the corresponding keys in ParserTable
*
* @retval    pointer to ParserTable
*
* @brief     Short version of the PLtoPTAll function Uses defOmniInfo keywords
*            (-key, -flags - are most useful)
*
*//*=========================================================================*/
ParserTable * PLtoPT(ParserList * pl)
{
    return PLtoPTAll(pl,    defOmniInfo[0].key, defOmniInfo[1].key, 
        defOmniInfo[2].key, defOmniInfo[3].key, defOmniInfo[4].key, 
        defOmniInfo[5].key, defOmniInfo[6].key, defOmniInfo[7].key);
}

/*========================================================================*//**
*
* @param     pl - parserlist
* @param     key, desc, nlsSet, nlsId, param1, param2, flags -
*                names of keywords in ParserList who's values
* @param     are going to become the corresponding keys in ParserTable
*
* @retval    pointer to ParserTable
*
* @brief     Converts ParserList to ParserTable. This is used to parse some file1
*            based on rules defined in another file2.  For example we parse OmniInfo
*            file in ParserList structure.  Then we use this function to convert
*            it to ParserTable and use this parserTable to parse cell_info file.
*            The reason why we do this is that cell_info file can have some additional
*            keywords (when we add some new components, and it'll look like -lotusnotes,
*            which originally is not in default defCellInfo parserTable) As input this
*            function must have strings which represent name of corresponding field in
*            ParserTable, for example "-name" will go to the pt->key field, "-text"
*            will go to pt->desc field. For the numeric fields like flags or nlsId
*            their values will be automatically converted from text to LONG
*            note: see PLtoPT function below for common use
*
*//*=========================================================================*/
ParserTable * PLtoPTAll(ParserList * pl, tchar * key, tchar * version,
                        tchar * desc, tchar *nlsSet, tchar *nlsId, 
                        tchar * param1, tchar * param2, tchar * flags)
{

    PObj * col, * row;
    ParserTable * pt;
    int valid;
    LONG result;

    int i=0;

    pt = PTNewItem();
    if (!pt) return NULL;
 
    if (!pl) return NULL;
    if (!pl->start) return NULL;

    for(row = pl->start; row; row = row->child)
    {
        for(col = row; col; col = col->next)
        {
            if (KwMatch(col->key, key))
                pt[i].key = StrNewCopy(col->value);
            else if (KwMatch(col->key, version))
                pt[i].value = StrNewCopy(col->value);
            else if (KwMatch(col->key, flags))
            {
                result = PLStrToLong(col->value, &valid);
                if (valid)
                    pt[i].flags = result;
            }
            else if (KwMatch(col->key, desc))
                pt[i].desc = StrNewCopy(col->value);
            else if (KwMatch(col->key, nlsSet))
            {
                result = PLStrToLong(col->value, &valid);
                if (valid)
                    pt[i].nlsSet = result;
            }
            else if (KwMatch(col->key, nlsId))
            {
                result = PLStrToLong(col->value, &valid);
                if (valid)
                    pt[i].nlsId = result;
            }
            else if (KwMatch(col->key, param1))
                pt[i].param1 = StrNewCopy(col->value);
            else if (KwMatch(col->key, param2))
                pt[i].param2 = StrNewCopy(col->value);

        }
        if (pt[i].key)
        {
            i++;
            pt = REALLOC(pt, sizeof(ParserTable) * (i+1));
            if (!pt) return NULL;
            pt[i].key = NULL; pt[i].desc = NULL; pt[i].nlsSet = 0; pt[i].nlsId = 0;
            pt[i].param2 = NULL; pt[i].param1 = NULL; pt[i].flags = 0; 
            pt[i].value = NULL;
        }

    }
    return pt;
}



/* ===========================================================================
|   FUNCTION: 
|       
|   DESCRIPTION
|       
|       
|   ARGUMENTS
|       
|       
|   RETURNS
|       
|       
 ========================================================================== */
LONG
PLGetFlags(const ParserList *pl, int index)
{
    if (!pl)
        return CIF_DEFAULT;

    if (!PTCheckIndex(pl->pt, pl->ptSize, index))
        return CIF_DEFAULT;

    return pl->pt[index].flags;
}

/*========================================================================*//**
*
* @param     pl     - pointer to ParserList
* @param     index  - index of element in parser tablel
*
* @retval    pointer to strinf
*
* @brief     return string for item. In case that cannot load string from
*            OB catalogs return description form tabel
*
*//*=========================================================================*/
tchar*
PLGetDisplayString(const ParserList* pl, int index)
{
    if (!pl)
        return NULL;

    if (!PTCheckIndex(pl->pt, pl->ptSize, index))
        return NULL;

    if ((pl->pt[index].nlsSet > 0) && (pl->pt[index].nlsId > 0))
    {
        tchar *desc = NlsGetMessage(pl->pt[index].nlsSet, pl->pt[index].nlsId);
        if (desc != NULL && !StrStr(desc, _T("Bad catalog access for message"))) /* invisible string */
            return desc;
    }

    return (tchar*)pl->pt[index].desc;
}


void
PLDbgPlain(int level, const tchar *title, MODIFIED ParserList *pl)
{
    Variant line = {0};
    PObj *po, *p;

    if (!pl || !DbgMatch(level))
        return;

    DbgPlain(level, _T("[%s] ptSize:%d flags:0x%08X"), NSD(title), pl->ptSize, pl->flags);
    
    for (p = pl->start; p; p = p->child)
    {
        VarSetLen(&line, 0);
        for (po = p; po; po = po->next)
        {
            if (po->index==-1)
                po->index = PTFind(po->key, pl->pt);

            VarCat(&line, _T("key:%s val:%s "), NSD(po->key), 
                PLGetFlags(pl, po->index) & CIF_ENCODE? _T("ENCODED") : NSD(po->value));
        }
        DbgPlain(level, _T("\t%s"), V2T(&line));
    }
    VarFree(&line);
}


/*========================================================================*//**
*
* @param     oldFile - pointer to ParserList
* @param     newFile - index of element in parser tablel
* @param     pl(I/O) - pointer to new merged ParserList
*
* @retval    -1 cannot merge
* @retval    0 one of oldFile and newFile is empty and return other
* @retval    > 0 merge successfully finish
*
* @brief     return merged ParserTable list.
*
*//*=========================================================================*/
int PLMergeFormatFiles(tchar* oldFile, tchar* newFile, tchar **merged)
{
    ERH_FUNCTION(_T("PLMergeFormatFiles"));
    ParserList *plOld = NULL;
    ParserList *plNew = NULL;
    int added = 0;

    DbgFcnIn();

    plOld = PLGetListFromBuffer(defOmniInfo, oldFile);
    plNew = PLGetListFromBuffer(defOmniInfo, newFile);

    if (!plOld)
    {
        DbgPlain(10, _T("Cannot parse oldFile file !!!"));
        *merged = StrNewCopy(newFile);
        added++;
    }

    if (!plNew)
    {
        DbgPlain(10, _T("Cannot parse newFile file !!!"));
        *merged = StrNewCopy(oldFile);
        added++;
    }
    if (added)
    {
        PLFREE(plOld);
        PLFREE(plNew);
        RETURN_INT(added == 2 ? -1 : 0);
    }
    
    while (plNew->current)
    {
        PObj *line = PLFindLine(plOld, plNew->current->value);
        if (line)
        {/* merge line */
            PObj *co = plNew->current;
            while (co)
            {
                PObj *fo = PLFindObj(line, co->key);
                if (fo)
                {
                    if (StrICmp(fo->value, co->value))
                    { /* object founded but value diff */
                        FREE(fo->value);
                        fo->value = StrNewCopy(co->value);
                        added++;
                    }
                }
                else
                {/* object is not in line add it */
                    PLAdd(line, PONewCopy(co));
                }
                co = co->next;
            }
        }
        else
        { /* add all line */
            if (PLAddLine(plOld, PLCopyLine(plNew->current)))
            {
                DbgPlain(140, _T("Add in cell_format new item form omni_format <%s>"), 
                    plNew->current->value?plNew->current->value:_T("<NULL>"));
                added++;
            }
        }
        plNew->current = plNew->current->child;
    }

    PLFREE(plNew);
    if (!PLGetBufferFromList(merged, plOld))
    {
        added = -1;
    }
    PLFREE(plOld);

    RETURN(added);
}

/*========================================================================*//**
* @brief    encode (obfuscate) string to ASCII range 35(#) ... 126(~) thus avoiding quotes in output
* @param    plain       - string to encode.
*
* @retval   NULL        - if plain is NULL
*           ""          - malloc-ed empty string if plain is empty string
*           otherwise   - pointer to malloc-ed encoded string
*
* @bugs     when compiled with UNICODE, PLEncode works only for wchars < 256
* @bugs     PLEncode produces different results for same input depending on UNICODE
*           settings. Thus decode must be done by same UNICODE settings as encode.
*           This is issue when text is encoded on windows and decoded on unix.
*
* @note     use PLEncodeUTF8 instead of PLEncode.
*//*=========================================================================*/

#define PLENCODE_UTF8 1U

static tchar*
PLEncodeTo(Variant *out, const tchar *plain, unsigned flags)
{
    int *buffer;
    int  i, len;

    if (StrIsEmpty(plain))
        return StrNullCopy(plain);

    if (flags & PLENCODE_UTF8)
    {
        USES_CONVERSION_T2A
        const char *utf8 = T2S(plain);

        len = strlenA(utf8);
        buffer = alloca((len+1) * sizeof(int));

        for(i = 0; i < len; ++i)
            buffer[i] = (unsigned char)utf8[i];
    }
    else
    {
        len = strlen(plain);
        buffer = alloca((len+1) * sizeof(int));
    
        for(i = 0; i < len; ++i)
            buffer[i] = (unsigned char)plain[i];
    }

    buffer[len] = 0;

    /*  Encrypt the sequence */
    for(i = 0; buffer[i+1]; ++i)
    {
        buffer[i] ^= buffer[i+1] ^ 0x20;
    }

    for(i = len-1; i > 0; --i)
    {
        buffer[i] ^= buffer[i-1] ^ 0x40;
    }


    /*  Not all of the encoded integers are printable ASCII. 
        Convert encrypted stuff into printable range */
    out->length = 0;
    for(i = 0; i < len; ++i)
    {
        tchar t;

        for (; buffer[i] > 90; buffer[i] -= 91)
            VarPutc(out, '#' + 91);

        t = buffer[i] + '#';
        VarPutc(out, ((flags & PLENCODE_UTF8) && t == '\'') ? '!' : t);
    }

    return V2T(out);
}


MALLOCED tchar*
PLEncode(const tchar *plain)
{
    Variant out = VarUndef;
    return PLEncodeTo(&out, plain, 0);
}


/* @brief   same as PLEncode, just caller must not free returned string */
const tchar*
PLEncodex(const tchar *plain)
{
    Variant *out = CmnGetTmpVar(NULL);
    return PLEncodeTo(out, plain, 0);
}


const tchar*
PLEncodeUTF8(const tchar *plain)
{
    Variant *out = CmnGetTmpVar(NULL);
    return PLEncodeTo(out, plain, PLENCODE_UTF8);
}


/*========================================================================*//**
*
* @brief    opposite to PLEncode
* @param    crypt       - string to decode.
*
* @retval   NULL        - if crypt is NULL
*           NULL        - if crypt was not encoded by PLEncode. Function marks ERH_FFORMAT.
*           ""          - malloc-ed empty string if crypt is empty string
*           otherwise   - pointer to malloc-ed decoded string
*//*=========================================================================*/
static tchar*
PLDecodeTo(Variant *out, const tchar *crypt, unsigned flags)
{
    ERH_FUNCTION(_T("PLDecodeTo"));

    int *buffer, *ebuffer;
    int i, clen, len, sz;
    
    if (StrIsEmpty(crypt))
        return StrNullCopy(crypt);

    clen = strlen(crypt);
    sz = (clen + 1) * sizeof(int);
    buffer  = alloca(sz);
    ebuffer = alloca(sz); /* could be less */

    for(i = 0; i < clen; ++i)
        ebuffer[i] = (unsigned char)(crypt[i] == _T('!') ? _T('\'') : crypt[i]);

    ebuffer[clen] = 0;

    /*  First convert back the ASCII string into encrypted integer sequence to prepare for decoding */
    len = 0;
    buffer[0] = 0;
    for(i = 0; i < clen; ++i)
    {
        int code = ebuffer[i];
        if (code < 35 || code > 126) /* invalid encoded string */
        {
            tchar info[STRLEN_PATH+1];
            StrNPrintf(info, STRLEN_PATH, _T("Invalid character %d(%c) at position %d. Input:%s"), code, code, i, crypt);
            ErhSetErrorInfo(info);
            ErhMark(ERH_FFORMAT, __FCN__, ERH_MAJOR);
            return NULL;
        }

        buffer[len] += code - 35;

        if (code != 126)
            buffer[++len] = 0;
    }


    /*  decrypt the sequence */
    for(i = 1; i < len; i++)
    {
        buffer[i] ^= buffer[i-1] ^ 0x40;
    }

    for(i = len-2; i >= 0; --i)
    {
        buffer[i] ^= buffer[i+1] ^ 0x20;
    }

    out->length = 0;

    #ifdef UNICODE
    if (flags & PLENCODE_UTF8)
    {
        char *utf8 = alloca(len+1);
        for(i = 0; i < len; ++i)
            utf8[i] = (char)buffer[i];
        utf8[len] = 0;

        VarCopyS2T(out, utf8);
        return V2T(out);
    }
    #endif

    for(i = 0; i < len; ++i)
        VarPutc(out, buffer[i]);

    return V2T(out);
}


MALLOCED tchar*
PLDecode(const tchar *crypt)
{
    Variant out = VarUndef;
    return PLDecodeTo(&out, crypt, 0);
}


const tchar*
PLDecodeUTF8(const tchar *crypt)
{
    Variant *out = CmnGetTmpVar(NULL);
    return PLDecodeTo(out, crypt, PLENCODE_UTF8);
}


int
ParGetErrorLine(void)
{
    USES_PARSER_PTD;
    return ThisParser->error.line;
}


int
ParGetErrorCol(void)
{
    USES_PARSER_PTD;
    return ThisParser->error.col;
}


/* ===========================================================================
| @section  atomic file update
 =========================================================================== */
 
CfChange
CfChangeCreate(_In_ int fileID)
{
    CfChange change = {0};
    change.fileID = fileID;
    return change;
}


static int
VarFindKey(const Variant *list, ctchar *key)
{
    int i, len = VarLength(list);
    for (i = 0; i < len; ++i)
    {
        ctchar *k = VarKey(list, i);
        if (0 == StrCmp(k, key))
            return i;
    }
    return -1;
}
        

ctchar*
CfGetPrimaryKeyName(int fileID)
{
    switch (fileID)
    {
    case S_CELL:
    case S_BMASTERS:
        return PLCIK_HOSTNAME;
        break;

    case S_CELLFORMAT:
    case S_OMNIINFO:
        return PLDOI_KEY;

    default:
        return NULL;
    }
}


// takes ownership of <data> - will be deleted after commit.
int
CfChangeAdd(_Inout_ CfChange *change, _In_ ctchar *key, _In_ unsigned flags, _In_ Variant data)
{
    Variant v;
    int primaryKeyPos = VarFindKey(&data, CfGetPrimaryKeyName(change->fileID));
    
    VarListMove(&data, primaryKeyPos, 0);

    v = VarList(_T("key"), T2V(key), _T("flags"), I2V(flags), _T("data"), data, NULL);
    VarPush(&change->data, v);
    return 0;
}

/* ===========================================================================
| @note     we quote values when needed, regardless of the key: if value has
|           blanks, quotes or other special characters.
|           libparse explicitly quotes only keys: "desc", "host", "uxpath", "os" and "ntpath"
|
| @note     caller is responsible to encode passwords; in difference from libparse,
|           this API does not automatically encode/decode password fields:
|           a) that would require all files to come in pairs (like cell_info and cell_format do)
|           b) passwords should anyway stay encoded until needed, which is only
|              when being passed to the 3rd party interface (e.g. MA passes password to NDMP server)
|           Thus the new rule is: PLEncode after user enters password, and decode
|           only when plain password is needed.
 =========================================================================== */
static Variant
CfUnpackLineImpl(_In_ ctchar *line, int lineno)
{
    ERH_FUNCTION(_T("CfUnpackLineImpl"));
    Variant rec = {0}, *val = NULL;
    tok_t tok = tok_init(line);

    while (tok_get_token(&tok, TOK_MODE_RESTORE))
    {
        ctchar *t;

        // key
        if (!tok.quote && tok.value[0] == '-') 
        {
            ctchar *key = tok.value + 1;
            val = VarListAdd(&rec, key, VarUndef);
            continue;
        }

        // value
        if (!val)
        {
            DbgPlain(DBG_UNEXPECTED, _T("[%s] missing key. line:%d, token:%s"), __FCN__, lineno, tok.value);
            VarFree(&rec);
            tok_free(&tok);
            return VarUndef;
        }

        switch (TypeOf(val))
        {
        case var_undef:
            *val = VarText(tok.value);
            break;
                
        case var_string:
            t = V2T(val);
            *val = VarUndef;
            VarPush(val, T2V(t));
                
        case var_array:
            VarPush(val, T2V(tok.value));
            break;
                
        default:
            tok_free(&tok);
            ErhAbort();
        }
    }
    return rec;
}


Variant
CfUnpackLine(_In_ ctchar *buf)
{
    tok_t line = tok_init(buf);
    Variant ret = CfUnpackLineImpl(tok_get_line(&line, TOK_MODE_RESTORE), 1);
    tok_free(&line);
    return ret;
}


Variant
CfUnpack(_In_ ctchar *buf, _In_ int fileID)
{
    ERH_FUNCTION(_T("CfUnpack"));
    Variant out = {0};
    int     lineno = 0;
    tok_t   l = tok_init(buf);
    ctchar *primaryKey = CfGetPrimaryKeyName(fileID);

    ErhAssert(__FCN__, primaryKey);

    while (tok_get_line(&l, TOK_MODE_RESTORE))
    {
        Variant rec;
        ctchar *k;
        tchar *line = l.value;
        lineno++;

        PtrSkipSpaces(line);
        if (!line[0])
            continue;

        rec = CfUnpackLineImpl(line, lineno);
        if (!VarDefined(&rec))
        {
            VarFree(&out);
            return VarUndef;
        }

        k = VarGetText(&rec, primaryKey);
        if (StrIsEmpty(k))
        {
            DbgPlain(DBG_UNEXPECTED, _T("[%s] missing primary key. line:%d, key:%s, record:%s"), 
                __FCN__, lineno, primaryKey, VarPack(&rec));
            VarFree(&out);
            return VarUndef;
        }
        
        VarListAdd(&out, k, rec);
    }
    return out;
}


static void
CfPackString(_Inout_ Variant *out, _In_ ctchar *str)
{
    BOOL needQuote = StrIsEmpty(str) || strpbrk(str, _T(" ~$|{}[]<>\t\r\v\n"));
    VarCat(out, needQuote? _T("\"%s\" ") : _T("%s "), str);
}


MALLOCED tchar*
CfPack(_In_ const Variant *doc)
{
    ERH_FUNCTION(_T("CfPack"));
    Variant out = {0};
    Iterator i, j, k;
    VarIterate(doc, i)
    {
        Variant *line = IterVal(&i);
        VarIterate(line, j)
        {
            ctchar  *key = IterKey(&j);
            Variant *val = IterVal(&j);
            VarCat(&out, _T("-%s "), key);

            switch (TypeOf(val))
            {
            case var_undef:
                break;

            case var_string:
            case var_int:
                CfPackString(&out, V2T(val));
                break;

            case var_array:
                VarIterate(val, k)
                    CfPackString(&out, V2T(IterVal(&k)));
                break;

            default:
                DbgStamp(DBG_UNEXPECTED);
                DbgPlain(DBG_UNEXPECTED, _T("invalid value type. key:'%s', val:%s"), key, VarPack(val));
                VarFree(&out);
                return NULL;
            }
        }
        VarPutc(&out, '\n');
    }

    VarTrimChars(&out, _T("\r\n"), VAR_MATCH_END);

    return V2T(&out);
}


static void
cf_assign(Variant *blk, ctchar *key, Variant *val, Variant *newval)
{ 
    if (StrCmp(V2T(newval), CF_T_DELETE) == 0)
    {
        VarListDel(blk, key);
    }
    else if (val)
    {
        VarFree(val);
        *val = VarCopy(newval);
    }
    else
    {
        VarListAdd(blk, key, VarCopy(newval));
    }
}


int
CfPatch(_Inout_ Variant *file, _In_ const Variant *change)
{
    int nchanges = 0;
    Iterator i, j;

    // array [ { key=>"record key", flags=>CF_XXX, data=>{changelist} } ]
    VarIterate(change, i)
    {
        Variant  copy;
        Variant *v = IterVal(&i);
        ctchar  *key = VarGetText(v, _T("key"));
        int      flags = VarGetInt(v, _T("flags"));
        Variant *data  = VarGet(v, _T("data"));
        Variant *oldrec = VarGet(file, key);

        if (oldrec == NULL && (flags & CF_REC_OLD))
            continue;

        if (oldrec != NULL && (flags & CF_REC_NEW))
            continue;

        if (flags & CF_REC) // replace whole record
        {
            nchanges++;

            if ((flags & CF_REC_KEEP) && oldrec && VarDefined(data))
            {
                int k;

                copy = VarCopy(data);

                for (k = 0; PLPreservedKeys[k]; ++k)
                {
                    ctchar  *p = PLPreservedKeys[k];
                    Variant *oldval = VarGet(oldrec, p);

                    if (oldval && !VarGet(&copy, p))
                    {
                        VarSet(&copy, p, *oldval);
                        *oldval = VarUndef;
                    }
                }
                VarFree(oldrec);
                *oldrec = copy;
                continue;
            }

            cf_assign(file, key, oldrec, data);

            continue;
        }

        // changelist: { key1=>val1, key2, val2, ... }
        VarIterate(data, j)
        {
            ctchar  *k = IterKey(&j);
            Variant *v = IterVal(&j);

            Variant *oldval = VarGet(oldrec, k);
            if (oldval == NULL && (flags & CF_VAL_OLD))
                continue;

            if (oldval != NULL && (flags & CF_VAL_NEW))
                continue;

            // @xxx     having same key > 1 times, e.g. -clust node -clust integ
            // @todo    if value is unchanged (e.g. change "foo=>12" applied to record that already has foo=>12), dont increment
            //          we may update file less times
            nchanges++;
            if (!oldrec)
                oldrec = CmnVarListAdd(file, key, VarUndef);

            cf_assign(oldrec, k, oldval, v);
        }
    }

    return nchanges;
}


static const unsigned DEAD = 0xDEADBEEF;

static Variant
CfDiffRecord(Variant *a, Variant *b)
{
    Variant d = {0};
    Iterator i;

    VarIterate(a, i)
    {
        ctchar  *key = IterKey(&i);
        Variant *aval = IterVal(&i);
        Variant *bval = VarGet(b, key);

        if (!bval)
        {
            VarListAdd(&d, key, CF_DELETE);
            continue;
        }

        if (VarCmp(aval, bval) != 0)
        {
            VarListAdd(&d, key, *bval);
            *bval = VarUndef;
        }

        VarFree(bval);
        *bval = I2V(DEAD);
    }

    VarIterate(b, i)
    {
        ctchar  *key = IterKey(&i);
        Variant *bval = IterVal(&i);
        
        if (TypeOf(bval) == var_int && V2I(bval) == DEAD)
            continue;

        VarListAdd(&d, key, *bval);
        *bval = VarUndef;
    }

    return d;
}


CfChange
CfDiff(int fileID, tchar *tbase, tchar *tnow)
{
    Variant a = CfUnpack(tbase, fileID), b = CfUnpack(tnow, S_CELL);

    Variant *aval, *bval;
    Iterator i;

    CfChange change = CfChangeCreate(fileID);

    // find added/changed records
    VarIterate(&b, i)
    {
        ctchar *key = IterKey(&i);
        Variant d;

        bval = IterVal(&i);
        aval = VarGet(&a, key);
        if (!aval) // not found -> added record
        {
            CfChangeAdd(&change, key, CF_REC, *bval);
            *bval = I2V(DEAD);
            continue;
        }

        // @xxx     issues with this system (system being: remember cell_info we downloaded, and, when user changes and 
        //          clicks "apply", calculate diff = now - downloaded, and send it to be applied as a patch):
        // a)       whole record (host) could've been deleted meanwhile (e.g. host exported and we changed some password)
        // b)       user changed original attribute X0 -> X1, and, before clicking "apply" changed again X1 -> X0.
        //          when user clicks "apply" we will detect no changes, but why did the customer click "apply" then?
        //          we do not distinguish between "same value as initially, do not do anything despite me clicking apply" and
        //          "re-apply inserted values, although you think they are unchanged".
        //          to be 100% sure, we'd have to track attributes that user sees.

        d = CfDiffRecord(aval, bval);
        if (VarDefined(&d))
            CfChangeAdd(&change, key, CF_REC_OLD|CF_VAL_ANY, d);
    }

    // find deleted records: present in base but not in new doc
    VarIterate(&a, i)
    {
        ctchar *key = IterKey(&i);
        bval = VarGet(&b, key);

        if (!bval) // deleted record
            CfChangeAdd(&change, key, CF_REC, CF_DELETE);
    }

    VarFree(&a); VarFree(&b);
    return change;
}

