/**
*
* (c) Copyright 1993-2009, 2019 Micro Focus or one of its affiliates
*
* @ingroup   String Utilities (Str)
* @file      lib/cmn/strutil.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     Implementation of product-wide common string utilities
*
* @since     09.09.93   Caka          Initial Coding
*/

#include "lib/cmn/target.h"

__rcsId("$Header: /lib/cmn/strutil.c $Rev$ $Date::                      $:");


#include <time.h>
#include <ctype.h>

#include "lib/cmn/common.h"
#include "type_variant.h"

/* ---------------------------------------------------------------------------
|   Globals
 -------------------------------------------------------------------------- */
/* has to be non-const for legacy code; but do not modify */
tchar strEmpty[1] = _T("");  /* Pointer to empty string */



/*========================================================================*//**
* @brief    strnlen port. see man strnlen()
* @note     windows MBCS strnlen returns "the number of multibyte characters" (ref: msdn strnlen)
*//*=========================================================================*/
#if TARGET_WIN32 && !defined(_MBCS)
#   define str_strnlen _tcsnlen
#   define str_strnlenA strnlen
#elif TARGET_LINUX
#   define str_strnlenA strnlen
#   define str_strnlen  strnlen
    extern size_t strnlen(const char *s, size_t maxlen);
#else
    INLINED size_t
    str_strnlen(const char *str, size_t maxsize)
    {
        size_t n;
        for (n = 0; n < maxsize && *str; n++, str++);
        return n;
    }
#   define str_strnlenA str_strnlen
#endif

/*========================================================================*//**
* @brief    copy <len> tchars from <source> to <dest> and nul-terminate dest.
*           <len> must be valid, i.e. len <= strlen(source)
*//*=========================================================================*/
INLINED void
str_memcpy(tchar dest[], const tchar *source, size_t len)
{
    if (len) memmove(dest, source, len * sizeof(tchar));
    dest[len] = 0;
}

#ifdef UNICODE
INLINED void
str_memcpyA(char dest[], const char *source, size_t len)
{
    if (len) memmove(dest, source, len);
    dest[len] = 0;
}
#endif

INLINED MALLOCED tchar*
str_memdup(const tchar *str, size_t len)
{
    tchar *copy = MALLOC((len+2)*sizeof(tchar));
    if (copy)
    {
        if (len && str) memcpy (copy, str, len*sizeof(tchar));
        copy[len] = copy[len+1] = 0;
    }
    return (copy);
}


MALLOCED tchar*
CmnStrNDup (const tchar *src, size_t len)
{
    return str_memdup(src, len);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     *item            OB2_Version_Struct memory area
* @param     *Version_String  Version string for decomposition
*
* @retval    Nothing (void)
*
* @brief     This will break a typical string like A.03.51%B2 down into
*            single components such as 3, 51, B and 2
*
*//*=========================================================================*/
void
Decompose_OB2_Version_String(OB2_Cmn_Version *item, const tchar * Version_String)
{
    const tchar *p = Version_String;

    item->maj = 0;
    item->min = 0;
    item->bet = 'Z';
    item->betval = 0;

    if (!p)
        return;

    while(*p && !CharIsDigit(*p))
    {
        p++;
    }

    item->maj = atoi(p);

    while(*p && CharIsDigit(*p))
    {
        p++;
    }

    if(*p)
    {
        p++;
    }
    else
    {
        return;
    }

    item->min= atoi(p);

    while(*p && CharIsDigit(*p))
    {
        p++;
    }

    if(*p)
    {
        p++;
    }
    else
    {
        return;
    }

    while(*p && !CharIsAlpha(*p))
    {
        p++;
    }

    if(!*p)
    {
        return;
    }

    item->bet= p[0];

    while(*p && CharIsAlpha(*p))
    {
        p++;
    }

    if(!*p)
    {
        return;
    }

    item->betval = atoi(p);

    return;
}

/*========================================================================*//**
*
* @brief    compare ver1 and ver2 as two DP version strings
*
* @param    ver1, ver2  - DP version strings, e.g. A.03.51%B2
*
* @retval   1   ver1 is the higher version
*          -1   ver2 is the higher version
* @retval   0   ver1 and ver2 denote same version
*//*=========================================================================*/
int
OB2_higher_version(ctchar *ver1, ctchar *ver2)
{
    OB2_Cmn_Version a, b;

    Decompose_OB2_Version_String(&a, ver1);
    Decompose_OB2_Version_String(&b, ver2);

    return
        a.maj > b.maj?  1 :
        a.maj < b.maj? -1 :
        a.min > b.min?  1 :
        a.min < b.min? -1 :
        a.bet > b.bet?  1 :
        a.bet < b.bet? -1 :
        a.betval > b.betval?  1 :
        a.betval < b.betval? -1 :
        0;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str            Initialization string.
*
* @retval    Pointer to the allocated duplicated string.
*
* @brief     This  function  duplicates  the given  string  with  MALLOC and
*            returns  pointer  to the  duplicate.  When used, the  returned
*            string must be FREEd.
*
*//*=========================================================================*/
MALLOCED tchar *
StrNewCopy(const tchar *source)
{
    size_t len = source? strlen(source) : 0;
    return str_memdup(source, len);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str            Initialization string.
* @param     max            Maximum length
*
* @retval    Pointer to the allocated duplicated string.
*
* @brief     This  function  duplicates  the given  string  with  MALLOC and
*            returns  pointer  to the  duplicate.  The length of the duplicated
*            string can be less or equal maximum length - max. When used, the
*            returned string must be FREEd.
*
*//*=========================================================================*/
MALLOCED tchar *
StrNNewCopy (const tchar *str, size_t max)
{
    size_t  l = StrnLen(str, max);
    return str_memdup(str, l);
}


/*========================================================================*//**
* @brief    Do not use
*//*=========================================================================*/
MALLOCED tchar*
StrNewCopyUni(const void *str, BOOL isUnicode)
{
    return isUnicode? StrNewCopyW2T(str) : StrNewCopyS2T(str);
}


/*========================================================================*//**
* @retval   Return string length in tchars, zero for NULL or empty string
*//*=========================================================================*/
size_t
StrLen(const tchar *str)
{
    return str? strlen(str) : 0;
}


/*========================================================================*//**
* @retval   Return smallest of string length in tchars and maxlen, zero for NULL or empty string
* @note     Rationale: does not need to iterate to the end of (potentially) large string <str>
*//*=========================================================================*/
size_t
StrnLen(_In_opt_ const tchar *str, size_t maxlen)
{
    return str? str_strnlen(str, maxlen) : 0;
}


BOOL
StrIsLonger(_In_opt_ const tchar *str, size_t maxlen)
{
    size_t n = str? str_strnlen(str, maxlen) : 0;
    return n==maxlen && str && str[n];
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     dest            Destination string.
* @param     source          Source string.
* @param     destsize        Size of <dest> buffer in tchars, minus 1
*
* @retval    number of copied tchars w/o NUL tchar.
*
* @brief    Copy string <source> to <dest>, but not more than <destsize> characters.
*           Destination is always null terminated.
*
*//*=========================================================================*/
int
StrCopy (_Out_z_cap_(destsize+1) tchar *dest, const tchar *source, size_t destsize)
{
    size_t len;
    if (NULL == dest)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCopy] WARNING: got NULL pointer for destination!"));
        DbgDumpStack(DBG_UNEXPECTED);
        return -1;
    }

    len = source? str_strnlen(source, destsize) : 0;

    str_memcpy(dest, source, len);

    return (int)len;
}


#ifdef UNICODE
int
StrCopyA(_Out_z_cap_(destsize+1) char *dest, const char *source, size_t destsize)
{
    size_t len;

    if (NULL == dest)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCopy] WARNING: got NULL pointer for destination!"));
        DbgDumpStack(DBG_UNEXPECTED);
        return -1;
    }

    len = source? str_strnlenA(source, destsize) : 0;

    str_memcpyA(dest, source, len);

    return (int)len;
}
#endif


int
StrNCopy(tchar *dest, const tchar *source, size_t len, size_t destsize)
{
    if (NULL == dest)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrNCopy] WARNING: got NULL pointer for destination!"));
        DbgDumpStack(DBG_UNEXPECTED);
        return -1;
    }

    if (NULL == source) source = strEmpty;

    if (len > destsize) len = destsize;

    str_memcpy(dest, source, len);

    return (int)len;
}


/*========================================================================*//**
*
* @ingroup  String Utilities
*
* @brief    The NULLSAFE wrappers for some libc functions.
*
* @param    same as the matching libc function
* @retval   same as the matching libc function, unless stated otherwise
*
*//*=========================================================================*/

// @retval  new end of <dest>
tchar*
StrCpyNS(tchar *dest, const tchar *source)
{
    if (NULL == dest)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCpyNS] WARNING: got NULL pointer for destination!"));
        DbgDumpStack(DBG_UNEXPECTED);
        return NULL;
    }
    if (!source)
        source = _T("");

    while ((*dest++ = *source++) != 0);
    return dest - 1;
}


tchar *
StrTokNS(tchar *str, const tchar *sep)
{
    return sep? strtok(str, sep) : NULL;
}

tchar *
StrChrNS(const tchar *string, tchar_int c)
{
    return string? strchr(string, c) : NULL;
}

tchar *
StrrChrNS(const tchar *string, tchar_int c)
{
    return string? strrchr(string, c) : NULL;
}

tchar *
StrStrNS(const tchar *string, const tchar *pattern)
{
    return string && pattern ? strstr(string, pattern) : NULL;
}


/*========================================================================*//**
* @brief    Append <source> to <dest>, given tchar dest[destSize+1]
* @param    dest        Destination string.
* @param    source      Source string.
* @param    destSize    Size of <dest> buffer, in tchars, not counting EOS
*//*=========================================================================*/
void
StrCatNS(
    _Inout_z_cap_(destsize+1) tchar *dest,
    _In_ ctchar *source,
    _In_ size_t  destsize)
{
    StrCatEx(dest, source, destsize, NULL);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     dest            Destination string buffer.
* @param     source          Source string.
* @param     maxdestlen      Size of destination buffer, in tchars, not counting space for EOS.
* @param     pend            Optional context to speed-up. See USAGE.
*
* @retval    Number of tchars still free in destination (may be negative!)
*
* @brief     Append <source> to <dest> string. The <dest> buffer is <maxdestlen> tchars long, not counting EOS.
*
* @details   USAGE
*            tchar a[STRLEN_STD+1];
*            StrCopy(a, b, STRLEN_STD);
*            StrCatEx(a, c, STRLEN_STD, NULL);
*
*            or:
*
*            tchar a[STRLEN_STD+1];
*            tchar *ptr=NULL;
*            StrCopy(a, b, STRLEN_STD);
*            StrCatEx(a, c, STRLEN_STD, &ptr);
*            StrCatEx(a, d, STRLEN_STD, &ptr);
*
* @note     Use Variants for strings w/o size limit
*//*=========================================================================*/
SSIZE_T
StrCatEx(_Inout_z_cap_(destsize+1) tchar *dest, _In_ ctchar *source, _In_ SSIZE_T destsize, tchar **optptr)
{
    SSIZE_T destlen;
    SSIZE_T srclen;
    tchar *myend = NULL;

    if (dest == NULL)
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCatEx] WARNING: got NULL pointer for destination!"));
        DbgDumpStack(DBG_UNEXPECTED);
        return -1;
    }

    if (source == NULL) source = strEmpty;

    /* no optimization pointer, supply one internally */
    if (optptr == NULL)
    {
        optptr = &myend;
    }

    /* first call, initialize - set tchar pass end of dest  */
    if (*optptr == NULL)
    {
        *optptr = dest + strlen(dest);
    }

    srclen  = strlen(source);
    destlen = *optptr - dest; /* current dest length */

    if (destlen >= destsize) /* buffer already full */
    {
        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCatEx] WARNING: destination buffer is FULL. Ignored string='%s'!"), source);
        DbgDumpStack(DBG_UNEXPECTED);
    }
    else if (destlen + srclen > destsize) /* truncate */
    {
        size_t s = destsize - destlen;
        str_memcpy(*optptr, source, s);

        DbgStamp(DBG_UNEXPECTED);
        DbgPlain(DBG_UNEXPECTED, _T("[StrCatEx] WARNING: destination buffer does not have enough space. Source string ='%s' will be truncated at %lld!"),
            source, (dpint64_t)s);
        DbgDumpStack(DBG_UNEXPECTED);
    }
    else
        str_memcpy(*optptr, source, srclen);

    *optptr += srclen;

    return destsize - destlen - srclen;
}


/*========================================================================*//**
* @brief    write string list to target string. last string must be NULL.
*           dest = token1 + token2 + ... tokenN
*
* @retval   >= 0    number of appended tchars
* @retval   <  0    number of tchars that didn't fit into <dest>
*
*//*=========================================================================*/
SSIZE_T
StrCatV(_Out_z_cap_(STRLEN_PATH+1) tchar *dest, const tchar *token, ...)
{
    va_list va;
    SSIZE_T r = 0;
    tchar  *ctx = dest;

    if (!dest)
        return 0;

    dest[0] = 0;

    if (!token)
        return 0;

    va_start(va, token);
    for (; token; token = va_arg(va, const tchar*))
    {
        r = StrCatEx(dest, token, STRLEN_PATH, &ctx);
    }

    va_end(va);

    return r >= 0? STRLEN_PATH - r : r;
}



/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     dest           String allocated with MALLOC or StrNewCopy.
* @param     source         String to be appended.
*
* @retval    Pointer to the string concatenated string (dest*source).
*
* @brief     This function appends the source to the dest and returns
*            (possibly new) dest.
*            When used, the returned string must be FREEd.
*
*//*=========================================================================*/

MALLOCED tchar *
StrAppendM (MALLOCED tchar *dest, const tchar *source)
{
    if (source)
    {
        size_t destLen    = StrLen(dest);
        size_t sourceLen  = strlen(source);
        dest = REALLOC(dest, (destLen + sourceLen + 1)*sizeof(tchar));
        memcpy (dest+destLen, source, (sourceLen+1)*sizeof(tchar));
    }
    return dest;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to string.
*
* @brief     Function changes all characters in the string into upper case.
*
* @remarks   This function changes the existing string.
*
*//*=========================================================================*/
tchar *
StrToUpper (tchar *str)
{
    tchar    *tmp;

    if (str == NULL)
        return NULL;

    for (tmp=str; *tmp; tmp++)
    {
#ifdef _UNICODE
        *tmp = toupper(*tmp);
#else
        int val;

        /* (unsigned char) cast crucial, value domain is -1 .. 255 */
        val = toupper((unsigned char)(*tmp));
        if (val != 0)
        {
            /* We can't get 0 here since loop exits before end of string.  */
            /* We got 0 on HP-UX for "invalid" input values (possibly only */
            /* in case of MBCS). This is fixed by the cast above, but same */
            /* thing could happen for some other reason, so we double      */
            /* check. */
            /* We're still not processing MBCS strings correctly (it can't */
            /* be done in-place anyway!), but we're at least somewhat      */
            /* compatible by leaving the old value if 0 is returned.       */
            *tmp = val;
        }
#endif
    }

    return (str);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to string.
*
* @brief     Function changes all characters in the string into lower case.
*
* @remarks   <outBuffer> and <str> can be the same, in which case function changes existing string.
*
*//*=========================================================================*/
tchar*
StrToLowerR(tchar *outBuffer, const tchar *str)
{
    tchar *out = outBuffer;

    if (str == NULL)
        return NULL;

    for (; *str; str++, out++)
    {
#ifdef _UNICODE
        *out = tolower(*str);
#else
        /* (unsigned char) cast crucial, value domain is -1 .. 255 */
        int val = tolower((unsigned char)(*str));
        if (val != 0)
        {
            /* We can't get 0 here since loop exits before end of string.  */
            /* We got 0 on HP-UX for "invalid" input values (possibly only */
            /* in case of MBCS). This is fixed by the cast above, but same */
            /* thing could happen for some other reason, so we double      */
            /* check. */
            /* We're still not processing MBCS strings correctly (it can't */
            /* be done in-place anyway!), but we're at least somewhat      */
            /* compatible by leaving the old value if 0 is returned.       */
            *out = val;
        }
        else
        {
            *out = *str;
        }
#endif
    }

    *out = 0;

    return (outBuffer);
}



tchar *
StrToLower(MODIFIED tchar *str)
{
    return StrToLowerR(str, str);
}


ctchar*
StrToLowert(ctchar *str)
{
    tchar *out = CmnGetTmpStr(NULL, StrLen(str));
    return StrToLowerR(out, str);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to the first non-space character in the string.
*
* @remarks   If you already have a pointer, simply use the PtrSkipSpaces() macro.
*
*//*=========================================================================*/
tchar*
StrSkipSpaces(const tchar *str)
{
    PtrSkipSpaces(str);
    return (tchar*)str;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    No return value. Trailing \n and \r are cut in-place.
*
* @remarks   Modifies the source string!
*
*//*=========================================================================*/
void
StrCutNewLine(tchar *str)
{
    tchar *pos = StrEnd(str);

    while (pos != str)
    {
        pos--;

        if (*pos == '\n' || *pos == '\r')
        {
            *pos = 0;
            continue;
        }
        return;
    }
}


/* @return  last non-whitespace+1, or NULL if whole string is whitespace */
INLINED tchar*
str_lastw(NULLSAFE const tchar *str)
{
    const tchar *pos = NULL;

    if (str) for (pos = StrEnd(str)-1; pos >= str && CharIsSpace(*pos); pos--);

    return pos && pos >= str? (tchar*)pos+1 : NULL;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param    str      Input string.
*
* @brief    remove trailing whitespace (truncates input <str>).
*
* @returns  input string or, if truncate removed complete string, NULL
*
*//*=========================================================================*/
tchar*
StrCutTrailingSpacesInPlace(NULLSAFE tchar *str)
{
    tchar *w = str_lastw(str);
    if (!w)
        return NULL;

    *w = 0;
    return str;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to the first non-space character in the string.
* @retval    Trailing whitespace is cut in-place.
*
* @remarks   Modifies the source string!
*
*//*=========================================================================*/
tchar*
StrCutSpacesInPlace(tchar *str)
{
    PtrSkipSpaces(str);
    return StrCutTrailingSpacesInPlace(str);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to the new string.
*
* @brief     This  function  returns  the  string  identical  to  the input
*            string  parameters  without the leading spaces. If there are only
*            spaces in the string or string is empty, NULL pointer is returned.
*//*=========================================================================*/
MALLOCED tchar*
StrCutLeadingSpaces(const tchar *str)
{
    PtrSkipSpaces(str);
    return str && *str? StrNullCopy(str) : NULL;
}

/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to the new string.
*
* @brief     This function returns the string identical to the str parameters
*            without the trailing substring made of spaces. If there are only
*            spaces in the string or string is empty, NULL pointer is returned.
*//*=========================================================================*/
MALLOCED tchar*
StrCutTrailingSpaces(const tchar *str)
{
    tchar *w = str_lastw(str);
    return w? str_memdup(str, w - str) : NULL;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      Input string.
*
* @retval    Pointer to the new string.
*
* @brief     This function returns the string identical to the input parameter
*            without the leading and trailing made of spaces.If there are only
*            spaces in the str or str is empty, NULL pointer is returned.
*//*=========================================================================*/
MALLOCED tchar*
StrCutSpaces(const tchar *str)
{
    PtrSkipSpaces(str);
    return StrCutTrailingSpaces(str);
}


/*========================================================================*//**
*
* @ingroup   Hostname utils
*
* @param     dest        may be same as source (but better set source to NULL)
* @param     source      hostname (may be NULL for in-place)
* @param     maxlen      may use -1 when dest==source
*
* @retval    0 when maxlen truncated the string
* @retval    otherwise length of dest
*
* @brief     Copies the hostname string or modifies it in-place.
*            Uppercase A-Z are converted to lowercase. This is not a true,
*            (locale aware) "to lower" functionality - it is tailored
*            specifically for DNS behavior.
*
*            ATTENTION
*            Make sure source is not unintentionally equal to NULL!
*
*//*=========================================================================*/

size_t
HostToLower(_Out_z_cap_(destsize+1) tchar *dest, const tchar *source, size_t destsize)
{
    tchar *t = dest;

    if (!source)
        source = dest;


    /* in-place */
    for (; destsize > 0 && *source != 0; destsize--, source++, t++)
    {
        if (*source > 'Z' || *source < 'A')
        {
            *t = *source;
        }
        else
        {
            *t = *source + 'a' - 'A';
        }
    }

    *t = 0;

    return (destsize == 0 ? 0 : t - dest);
}



/*========================================================================*//**
*
* @ingroup   Hostname utils
*
* @param     host1       hostname
* @param     host2       hostname
*
* @retval    0 for match (same as strcmp!)
*
* @brief     Replaces strcmp for compare of two hostnames.
*            Respects environment settings regarding case sensitivity and such.
*            This function is primarily intended for security related checks.
*            It does not guarantee proper recognition of aliases, 'is local' type
*            checks and so on. IpcIPIsEqual() (or its successor) is intended for
*            such purposes.
*
*//*=========================================================================*/

int
HostStrCmp(const tchar *host1, const tchar *host2)
{
    static int first_time = 1;
    static int case_insensitive_mode = 2;  /* Note: 1 is quite insecure! */
    tchar hostbuf1[STRLEN_HOSTNAME+1];
    tchar hostbuf2[STRLEN_HOSTNAME+1];


    if (first_time)
    {
        first_time = 0;

        EnvReadInt(_T("OB2_HOSTCMPINSENSITIVE"), &case_insensitive_mode);
        /* EnvReadInt(_T("OB2_HOSTCMPSHORTNAMES"), &short_names_mode); */
        /* EnvReadInt(_T("OB2_HOSTCMPALIASES"), &aliases_mode); */  /* NOT recomended, may be sloooow */
    }

    if (host1 == NULL)
    {
        host1 = strEmpty;
    }

    if (host2 == NULL)
    {
        host2 = strEmpty;
    }

    if (case_insensitive_mode == 0)
    {
        return strcmp(host1, host2);
    }
    else if (case_insensitive_mode == 1)
    {
        return stricmp(host1, host2);
    }

    /* (case_insensitive_mode == 2) */
    if (HostToLower(hostbuf1, host1, STRLEN_HOSTNAME) != 0)
    {
        host1 = hostbuf1;
    }
    if (HostToLower(hostbuf2, host2, STRLEN_HOSTNAME) != 0)
    {
        host2 = hostbuf2;
    }

    return strcmp(host1, host2);
}

/*========================================================================*//**
*
* @ingroup   Hostname utils
*
* @copydoc HostStrCmp
*
* @brief     HostStrCmp assumes hostnames are in long format and might give
*            surprising results when used with mixed names. This implementation
*            will expand hostnames before comparison. Since this is a slow
*            operation do not use this function if hostnames are already
*            expanded, but use HostStrCmp instead.
*
*//*=========================================================================*/
int
HostStrCmpExpanded(const tchar* host1, const tchar* host2)
{
    tchar hostbuf1[STRLEN_HOSTNAME+1] = {0};
    tchar hostbuf2[STRLEN_HOSTNAME+1] = {0};
    StrCopy(hostbuf1, CmnExpandHostname(host1), STRLEN_HOSTNAME);
    StrCopy(hostbuf2, CmnExpandHostname(host2), STRLEN_HOSTNAME);
    return HostStrCmp(hostbuf1, hostbuf2);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      String.
* @param     result   Returned integer.
* @param     valid    Returned status of conversion.
*
* @retval    Status of conversion.
*
* @brief     This function converts the string str into the integer
*            result and sets valid to 1 if succeeded.
*            Leading whitespaces and trailing anything are ignored.
*
*//*=========================================================================*/

int
StrAtoi (const tchar *str, int *result, int *valid)
{
    int n = sscanf(NS(str),_T("%d"),result);
    if (valid) *valid = n;
    return n;
}

/* ===========================================================================
| @brief    convert input string to unsigned int, ignore leading whitespace and trailing non-digits.
|
| @param    IN  str    - string to parse
| @param    OUT result - result
| @retval   TRUE if converted ok, FALSE otherwise
 ========================================================================== */
BOOL
StrAtou(const tchar *str, unsigned int *result)
{
    return sscanf(NS(str), _T("%u"),result) == 1;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      String.
* @param     result   Returned ULONG.
* @param     valid    Returned status of conversion.
*
* @retval    Status of conversion.
*
* @brief     This function converts the string str into the ULONG
*            result and sets valid to 1 if succeeded.
*            Leading whitespaces and trailing anything are ignored.
*
*//*=========================================================================*/
int
StrAtoULONG (const tchar *str, ULONG *result, int *valid)
{
    int n = sscanf(NS(str),_T("%u"),result);
    if (valid) *valid = n;
    return n;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      String.
*
* @retval    pointer to address from string_
*
*//*=========================================================================*/

void *
StrAtoPtr (const tchar *str, BOOL *valid)
{
    void *p = NULL;

    BOOL ok = (1 == sscanf(NS(str),_T("%p"), &p));

    if (valid) *valid = ok;

    return ok ? p : NULL;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      String.
* @param     result   Returned int.
* @param     valid    Returned status of conversion.
*
* @retval    Status of conversion.
*
* @brief     This function converts the string str into the integer
*            result and sets valid to 1 if succeeded.
*            Also recognises keywords TRUE, FALSE, YES and NO (stricmp).
*            If string is empty, it is interpreted to be true (env. var. defined)
*
*//*=========================================================================*/

int
StrAtoBool (const tchar *str, int *result, int *valid)
{
    int n = 0;
    int r = 0;

    if (!str)
        goto out;

    n = sscanf(str,_T("%d"), &r);
    if (n)
        goto out;

    if (
        stricmp(str, _T(""))     == 0 ||
        stricmp(str, _T("true")) == 0 ||
        stricmp(str, _T("yes"))  == 0 ||
        stricmp(str, _T("on"))   == 0)
    {
        r = 1;
        n = 1;
    }
    else if (
        stricmp(str, _T("false")) == 0 ||
        stricmp(str, _T("no"))    == 0 ||
        stricmp(str, _T("off"))   == 0)
    {
        r = 0;
        n = 1;
    }

out:
    if (valid)
        *valid = n;

    if (result && n)
        *result = r;

    return n;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str      String.
* @param     result   Returned OFF_T64.
* @param     valid    Returned status of conversion.
*
* @retval    Status of conversion.
*
* @brief     This function converts the string str into the OFF_T64
*            result and sets valid to 1 if succeeded.
*            Leading whitespace and trailing anything are ignored.
*
*//*=========================================================================*/

int
StrAtoOFF_T64 (const tchar *str, OFF_T64 *result, int *valid)
{
    int n = sscanf(str, OFF_T64F ,result);
    if (valid) *valid = n;
    return n;
}


int
StrAtoi64(const tchar *str, dpint64_t *result, int *valid)
{
    int n = sscanf(str, _T("%lld"), result);
    if (valid) *valid = n;
    return n;
}

/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     low      string containing the low part
* @param     high     string containing the high part (may be NULL)
* @param     *result  returned OFF_T64
* @param     *valid   returned status of conversion
*
*//*=========================================================================*/

void
StrLowHighToOFF_T64 (const tchar *low, NULLSAFE const tchar *high, OUT OFF_T64 *result, OUT int *valid)
{
    int myValid = 0;
    ULONG valueHigh, valueLow;

    if(!valid)
        valid = &myValid;

    if (high == NULL)
    {
        StrAtoULONG(low, &valueLow, valid);
        if (!*valid) return;

        *result = valueLow;
        /* valid is already true */
        return;
    }
    else
    {
        StrAtoLONG(low, (LONG *) &valueLow, valid);
        if (!*valid) return;
        StrAtoLONG(high, (LONG *) &valueHigh, valid);
        if (!*valid) return;  /* if we got a non-NULL high, we insist it must be valid! */

        *result = IntsToLL(valueHigh, valueLow);
        /* valid is already true */
    }
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     value          number (64bit)
*
* @retval    64 bit number in string format (tchar)
*             Threadlocal static buffer is used (TLS)
*
*//*=========================================================================*/

tchar *
StrFromOFF_T64 (OFF_T64 value)
{
    USES_CMN_PTD
    tchar *strBuf = ThisCmn->strFromOFF_T64;

    sprintf(strBuf, OFF_T64F, value);

    return strBuf;
}



/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     low      string containing the low part
* @param     high     string containing the high part (may be NULL)
*
* @retval    64 bit number in string format (tchar)
* @retval    Threadlocal static buffer is used (TLS)
* @retval    CAUTION: TLS buffer is the same as used by StrFromOFF_T64()
*
*//*=========================================================================*/

tchar *
StrFromLowHighStr(const tchar *low, NULLSAFE const tchar *high)
{
    const int base = 10;

    /* We're simply calling StrFromOFF_T64() and reusing its TLS buffer */
    /* BTW: atoi(NS(NULL)) yields 0 */
    /* atoi has been replaced by strtoul because it handles 2Gb+ strings wrong */
    return StrFromOFF_T64(IntsToLL(strtoul(NS(high), NULL, base), strtoul(low, NULL, base)));
}


/* ===========================================================================
|
|   FUNCTION    CmnScaleDoubleSize
|
|       Converts object sizes into the appropriate measurement unit.
|
|       * If mode = -1, values are converted dynamically into an appropriate
|       measurement unit.
|       * If mode >= 0  values are converted into the measurement unit
|       determined by mode.
|       * Parameter min_unit can be used to prevent small units from being
|       used where this would be inappropriate (for example for tapes).
|
 ========================================================================== */

void
CmnScaleDoubleSize (IN OUT double *value, IN OUT int *unit, int mode, int min_unit)
{
    if (min_unit == -1) min_unit = *unit;

    if (mode >= 0)  /* requested unit */
    {
       if (mode < min_unit) mode = min_unit;  /* but respect min_unit */
       while (*unit < mode)
       {
          *value /= 1024.0;
          (*unit)++;
       }
    }
    else if (mode == -1)  /* dynamic mode */
    {
       while ((*unit < min_unit) || (*value >= 999.995))  /* printf("%6.2f", 999.995) produces "1000.00" */
       {
          *value /= 1024.0;
          (*unit)++;
       }
    }

    /* 0 and only 0 should output 0 */
    if ((*value != 0.0) && (*value < 0.01)) *value = (*unit == 0) ? 1.0 : 0.01;
}


/*============== Time String Functions =====================================*/


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     t        Time as number of seconds since 1/1/1970.
*
* @retval    Time as number of seconds since 1/1/1970.
*
* @brief     This function converts UTC (Coordinated Universal Time - the time
*            standard) to broken local time, strips the seconds, minutes and hours,
*            and then converts it back to time_t.
*
*//*=========================================================================*/
time_t StrSetToLocalMidnight(time_t t)
{
    struct tm *T = localtime(&t);
    T->tm_sec=T->tm_min=T->tm_hour = 0;
    return(mktime(T));
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     t        Time as number of seconds since 1/1/1970.
*
* @retval    pointer to the formatted string.
*
* @brief     This function converts time from the time_t type into
*            the FULL DATE string with respect to LANG. Time will
*            be presented as LOCAL TIME.
*
*            Example:Wed Nov 24 14:59:11 MET 1993
*
*//*=========================================================================*/
tchar *
StrFromTime (time_t t)
{
    USES_CMN_PTD
    tchar *buf = ThisCmn->strTimeStr;

    StrFromTimeEx(buf, STRLEN_STD, OB2_TIMEFMT_LONG, t);

    return buf;
}

tchar*
StrFromTimeFormat (time_t t, int format)
{
    USES_CMN_PTD
    tchar *buf = ThisCmn->strTimeStr;
    buf[0] = 0;
    StrFromTimeEx(buf, STRLEN_STD, format, t);

    return buf;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     t        Time as number of seconds since 1/1/1970.
*
* @retval    pointer to the formated string.
*
* @brief     This function converts time from the time_t type into
*            the FULL DATE string using fixed C locale. Time will
*            be presented as LOCAL TIME.
*
*            Example: 09/04/96 15:50:06
*
*//*=========================================================================*/
tchar *
StrFromTime1 (time_t t)
{
    USES_CMN_PTD
    tchar *buf = ThisCmn->strTimeStr;

    StrFromTimeEx(buf, STRLEN_STD, OB2_TIMEFMT_FIXED2, t);

    return buf;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     t        Time as number of seconds since 1/1/1970.
*
* @retval    pointer to the formated string.
*
* @brief     This function converts time from the time_t type into
*            the SHORT DATE string with respect to LANG. Time will
*            be presented as LOCAL TIME.
*
*            Example: 11/24/93 14:51:24
*
*//*=========================================================================*/
tchar *
StrShortFromTime (time_t t)
{
    USES_CMN_PTD
    tchar *buf = ThisCmn->strTimeStr;

    StrFromTimeEx(buf, STRLEN_STD, OB2_TIMEFMT_SHORT, t);

    return buf;
}


/* ---------------------------------------------------------------------------
|   NOTE: implementation of StrFromTimeEx() in strtime.c
 -------------------------------------------------------------------------- */


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     str     Time string
*
* @retval    Returns time_t if format was OK or 0 if format was bad.
*
* @brief     This  function  accepts the input string for date and time in
*            the  form:
*
*            yyyy/mm/dd.hh:mm:ss
*
*            As of A.05.50, the following formats are also allowed:
*            yyyy/mm/dd hh:mm:ss
*            yyyy-mm-dd hh:mm:ss
*            yyyy-mm-ddThh:mm:ss
*
*            The time component is optional and a default  (12:00:00) will
*            be used.  The year may be  specified  as two or four  digits.
*            It will return the unix-time  representation of the specified
*            input i.e it will return the Seconds since Jan 1 1970.
*
*            The time entered is considered LOCAL.
*
*//*=========================================================================*/
/* Original return value for failure was 0, and was checked as such by the
   callers. Times near 1.1.1970 are tricky anyway because of the time zones,
   so it is not a big deal and we can keep this value for now. */
#define OB2_TIME_T_OUT_OF_RANGE  ((time_t)0)

dptime_t  StrToTime (NULLSAFE const tchar *str)
{
    int year, month, day, hour, min, sec, days_since_70;
    time_t seconds;
    int n;
    struct tm *timeptr;

    if (!str) str = EmptyString;
    year = month = day = hour = min = sec = 0;
    n = sscanf (
        str,
        /* _T("%ld/%ld/%ld.%ld:%ld:%ld"), */
        _T("%d%*[-/]%d%*[-/]%d%*[ .T]%d%*[:.]%d%*[:.]%d"),
        &year,
        &month,
        &day,
        &hour,
        &min,
        &sec
    );

    /*-------- if not short or long form reject -------------------------*/

    if ( (n!=3) && (n!=6) ) return OB2_TIME_T_OUT_OF_RANGE;


    /*-------- expand year number if necessary --------------------------*/

    if (year >= 0 && year < 69)
    {
        year += 2000;
    }
    else if (year >= 70 && year < 100)
    {
        year += 1900;
    }

    if (year < 1970) return OB2_TIME_T_OUT_OF_RANGE;
    if (year > 2037) return OB2_TIME_T_OUT_OF_RANGE;

    /* --------------------------------------------------------------------
    |   We will consider March  to be the first month so that our formula
    |   remains simple for leap and non-leap years ( Feb being the odd month
    |   out).
     -------------------------------------------------------------------- */

    year = year - 1971 + (month + 9)/12;
    month = (month + 9) % 12;


    /* --------------------------------------------------------------------
    |
    |  The MAGIC formula.
    |
     -------------------------------------------------------------------- */

    days_since_70 = 365*year + (year-2)/4 +
                    30*month + ((month%5)+1)/2 + 3*(month/5) + 59 + day;


    /* --------------------------------------------------------------------
    |   Correction for 1970 thru Feb. 1972.  This is here because
    |   T is the number of days since the beginning of time, not
    |   including the present day.  The formula adds the present day
    |   though, but after 1972 this is compensated for by the correction
    |   for leap years being one less than it should be.
     -------------------------------------------------------------------- */

    if (year < 2)
    {
        days_since_70 -= 1;
    }


    /*----------------- convert to seconds and we are done ---------------*/

    seconds = ((24*days_since_70 + hour) * 60 + min)*60 + sec;


    timeptr=gmtime(&seconds);
    if (timeptr==NULL) return OB2_TIME_T_OUT_OF_RANGE;
    timeptr->tm_isdst=-1;
    return (mktime(timeptr));

/*
CHECKME: Converting to seconds and then to tm using gmtime() is unnecessary.
We should just declare a tm struct, fill it in and proceed with mktime().
*/
    /*
    seconds = mktime(&tm_struct);
    if (seconds == -1) return OB2_TIME_T_OUT_OF_RANGE;
    return (seconds);
    */
}





/*========================================================================*//**
* @ingroup  String Utilities
* @param    str     file path
* @retval   Return path basename. Recognize both / and \\ as path separators.
*//*=========================================================================*/
tchar *
StrBasename(const tchar *str)
{
    ctchar *t;
    if (str) for (t = str; *t; t++) if (*t == '/' || *t == '\\') str=t+1;
    return (tchar*)str;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     path     file path
* @param     dir      Output buffer of minimum STRLEN_PATH+1 size
*
*//*=========================================================================*/
tchar*
StrDirnameR(IN const tchar *path, _Out_z_cap_(STRLEN_PATH+1) tchar *dir)
{
    tchar *t, *sep = NULL;
    StrCopy (dir, path, STRLEN_PATH);
    for (t = dir; *t; t++) if (*t == '/' || *t == '\\') sep = t;

    if (!sep)
    {
        dir[0] = 0;
        return NULL;
    }

    sep[sep==dir] = 0;
    return dir;
}


tchar *StrDirname (IN const tchar *path)
{
    USES_CMN_PTD
    return StrDirnameR(path, ThisCmn->strDirname);
}


tchar *StrExtname (const tchar *path)
{
    const tchar *basename = StrBasename(path);
    tchar *dot = basename? strrchr(basename, _T('.')) : NULL;
    return dot? dot+1 : NULL;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param    path        IN path to clean
* @param    flags       IN or-ed STRCLEAN_XXX flags. See FLAGS.
* @param    output      OUT clean path (MUST be at least STRLEN_PATH+1 tchars in size)
*
* @retval   Clean path = output.
*
* @brief    If no flag is set, join / characters and remove tail / (unless that produces empty output).
*           Irrelevant of <flags>, / is always treated as path separator.
*
* @xxx      Previous impl ignored STRCLEAN_INBS if STRCLEAN_INAUTO was set.
*
* @note     FLAGS
*           -------------------------------------------------------------------
*           STRCLEAN_UNC            Leave leading double identical path separators, if present
*           STRCLEAN_INBS           Treat both \ and / as a input separators
*           STRCLEAN_INAUTO         Separator is determined based on the first occurrence of \ or /.
*           STRCLEAN_OUTBS          Output path separator is \.
*                                   Implies STRCLEAN_INBS.
*           STRCLEAN_OUTGEN         Use GEN_SLASH as output path separator
*           STRCLEAN_KEEPLASTDEL    Do not remove tail path separator
*
* @note     Backslash may be second byte in multibyte sequence (e.g. in shift-jis), but not the first.
*           Hence we use tcsinc/tcscpy to handle multibyte sequences.
*
*//*=========================================================================*/
tchar*
StrCleanPathnameExR(IN const tchar *path, IN int flags, _Out_z_cap_(STRLEN_PATH+1) tchar *output)
{
    tchar *end = output == path? (void*)(size_t)-1 : output + STRLEN_PATH;
    tchar *out = output;
    const tchar *in = path;
    tchar insep  = flags & (STRCLEAN_INBS|STRCLEAN_OUTBS) ? '\\' : flags & STRCLEAN_INAUTO? 0 : '/';
    tchar outsep = flags & STRCLEAN_OUTGEN? GEN_SLASH : flags & STRCLEAN_OUTBS ? '\\' : '/';
    BOOL sep = FALSE;

    if (StrIsEmpty(in))
        goto done;

    #define issep(ch) ((ch) == insep || (ch) == '/')
    #define isanysep(ch) ((ch) == '\\' || (ch) == '/')

    if ((flags & STRCLEAN_UNC) && isanysep(in[0]))
    {
        if (insep == 0)
            insep = in[0];

        if (in[1] == in[0])
        {
            *out++ = outsep; *out++ = outsep; in += 2;
            while(issep(*in)) in++;
        }
    }

    for(; out < end && *in; tcsinc(in))
    {
        switch (*in)
        {
        case '\\':
            if (insep == 0) insep = '\\'; else if (insep != '\\') goto default_label;
            sep = TRUE;
            continue;

        case '/':
            if (insep == 0) insep = '/';
            sep = TRUE;
            continue;

        default:
        default_label:
            if (sep) *out++ = outsep;
            sep = FALSE;
            out += tccpy(out, in);
        }
    }

    if (out < end && sep && ((flags & STRCLEAN_KEEPLASTDEL) || out == output))
        *out++ = outsep;

done:
    *out = 0;
    return(output);
}


/* @brief   wrappers over StrCleanPathnameExR */
tchar*
StrCleanPathnameEx(const tchar *path, int flags)
{
    USES_CMN_PTD
    tchar *output = ThisCmn->strCleanPathnameEx;

    return (StrCleanPathnameExR (path, flags, output));
}


tchar*
StrCleanPathnameR(IN const tchar *path, IN int ignored_dirDelimiter, _Out_z_cap_(STRLEN_PATH+1) tchar *output)
{
    return StrCleanPathnameExR(path, STRCLEAN_INBS, output);
}


tchar*
StrCleanPathname(const tchar *path, int ignored_dirDelimiter)
{
    USES_CMN_PTD
    tchar *output = ThisCmn->strCleanPathname;
    return StrCleanPathnameExR(path, STRCLEAN_INBS, output);
}


#if !TARGET_OPENVMS
/*========================================================================*//**
*
* @param     str                Path name to convert
*            maxLength          Max length of the full path name buffer
*            convertToLong      Expand short path names if any?
*            fullPath           Buffer that will contain the full path name
*
* @retval    NULL               Errors: - string is empty
*                                       - full path name is greater than maxLength
*                                       - invalid path
*                                       - convertToLong = TRUE, but short path name does not exist
*
*            pointer to the full path name buffer
*
* @brief     Convert path <str> to full path name.
*            *NOTE* If <convertToLong> option is TRUE, path must exist on the host
*            where this function is called
*
*//*=========================================================================*/

tchar *
StrFullPathName(
    _In_ ctchar *str,
    _In_ int    maxLength,
    _In_ BOOL   convertToLong,
    _Out_z_cap_(maxLength+1) tchar *fullPath)
{
    ERH_FUNCTION(_T("StrFullPathName"));
    if (fullPath)
        fullPath[0] = 0;

    if (!str)
        return (NULL);

    #if TARGET_WIN32
    {
        tchar longPath[STRLEN_PATH+1] = {0};

        if (NULL == _tfullpath(fullPath, str, maxLength))
        {
            OSLOGW(_tfullpath, _T("len:%d, path:%s"), maxLength, str);
            return (NULL);
        }
        if (convertToLong)
        {
            DWORD ret = GetLongPathName(fullPath, longPath, maxLength+1);
            if (!ret || ret > maxLength+1)
            {
                OSLOGW(GetLongPathName, _T("len:%d, path:%s"), maxLength+1, fullPath);
                return (NULL);
            }
            StrCopy(fullPath, longPath, maxLength);
        }
        return (fullPath);
    }
    #else
    {
        return (realpath(str, fullPath));
    }
    #endif
}
#endif

/*========================================================================*//**
*
* @retval    PATH_TYPE_ERROR
*            PATH_TYPE_ABSOLUTE
*            PATH_TYPE_UNC
*            PATH_TYPE_RELATIVE
*            PATH_TYPE_DRIVE_RELATIVE
*            PATH_TYPE_DEVICE_NAMESPACE     \\?\D:\very_long_path
*            PATH_TYPE_BASENAME
*            PATH_TYPE_DEVICE                windows: CON, PRN, AUX, NUL, COM[1-9], LPT[1-9]
*
* @brief     classify path. ref: msdn Naming Files, Paths, and Namespaces
*
*//*=========================================================================*/

#ifdef BACKSLASHES
#   define CharIsPathSep(ch) ((ch) == _T('/') || (ch) == _T('\\'))
    static const tchar PathSepList[] = _T("\\/");
#else
#   define CharIsPathSep(ch) ((ch) == _T('/'))
    static const tchar PathSepList[] = _T("/");
#endif

int
StrClassifyPath(const tchar *path)
{
    if (StrIsEmpty(path))
        return PATH_TYPE_ERROR;

    #if TARGET_WIN32
    {
        tchar drive, eos, any, sep;
        int   n;
        size_t len = strlen(path);

        if (path[0] == _T('.') && (!path[1] || (path[1] == _T('.') && !path[2])))
            return PATH_TYPE_RELATIVE;

        if (strpbrk(path, FORBIDDEN_PATH_CHARS_WINDOWS))
            return PATH_TYPE_ERROR;

        if (1 == sscanf(path, _T("\\\\?\\%c"), &any))
            return PATH_TYPE_DEVICE_NAMESPACE;

        if (1 == sscanf(path, _T("\\\\.\\%c"), &any))
            return PATH_TYPE_DEVICE_NAMESPACE;

        if (len == 3 && (
            0 == StrICmp(path, _T("CON")) ||
            0 == StrICmp(path, _T("PRN")) ||
            0 == StrICmp(path, _T("AUX")) ||
            0 == StrICmp(path, _T("NUL"))))
        {
            return PATH_TYPE_DEVICE;
        }

        if (len > 3 && (
            (1 == sscanf(path, _T("COM%d%c"), &n, &eos) && n>0 && n<10) ||
            (1 == sscanf(path, _T("LPT%d%c"), &n, &eos) && n>0 && n<10)))
        {
            return PATH_TYPE_DEVICE;
        }

        if (1 == sscanf(path, _T("\\\\%*[A-Za-z0-9-]%c"), &sep))
            return CharIsPathSep(sep)? PATH_TYPE_UNC : PATH_TYPE_ERROR;

        if (2 == sscanf(path, _T("%c:%c"), &drive, &sep))
        {
            return !isalpha(drive)    ? PATH_TYPE_ERROR :
                    CharIsPathSep(sep)? PATH_TYPE_ABSOLUTE :
                                        PATH_TYPE_DRIVE_RELATIVE;
        }

        if (strchr(PathSepList, path[0]))
            return PATH_TYPE_DRIVE_RELATIVE;
    }
    #else
    {
        if (path[0] == _T('/'))
            return PATH_TYPE_ABSOLUTE;
    }
    #endif

    return strpbrk(path, PathSepList) ? PATH_TYPE_RELATIVE :
                                        PATH_TYPE_BASENAME;
}


/* NT specific defines - on NT this can be found in winnt.h */
#if !defined(FILE_ATTRIBUTE_READONLY)
#   define FILE_ATTRIBUTE_READONLY              0x00000001
#   define FILE_ATTRIBUTE_HIDDEN                0x00000002
#   define FILE_ATTRIBUTE_SYSTEM                0x00000004
#   define FILE_ATTRIBUTE_DIRECTORY             0x00000010
#   define FILE_ATTRIBUTE_ARCHIVE               0x00000020
#   define FILE_ATTRIBUTE_DEVICE                0x00000040
#   define FILE_ATTRIBUTE_NORMAL                0x00000080
#   define FILE_ATTRIBUTE_TEMPORARY             0x00000100
#   define FILE_ATTRIBUTE_SPARSE_FILE           0x00000200
#   define FILE_ATTRIBUTE_REPARSE_POINT         0x00000400
#   define FILE_ATTRIBUTE_COMPRESSED            0x00000800
#   define FILE_ATTRIBUTE_OFFLINE               0x00001000
#   define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED   0x00002000
#   define FILE_ATTRIBUTE_ENCRYPTED             0x00004000
#   define FILE_ATTRIBUTE_INTEGRITY_STREAM      0x00008000
#   define FILE_ATTRIBUTE_VIRTUAL               0x00010000
#   define FILE_ATTRIBUTE_NO_SCRUB_DATA         0x00020000
#   define FILE_ATTRIBUTE_EA                    0x00040000
#endif

const tchar*
StrRWX(int objType, int fflags, unsigned mode)
{
    USES_CMN_PTD
    tchar *strBuf = ThisCmn->strRWX;

    if (fflags & FF_NT_ATTR)  /* NT file or directory */
    {
        tchar *t = strBuf;

        if (mode & FILE_ATTRIBUTE_NORMAL)
            mode = 0;

        #define check(mode,flag,tag) { if (t - strBuf < STRLEN_RWX && (mode & flag)) *t++ = tag; }
        check(mode, FILE_ATTRIBUTE_READONLY,       'R');
        check(mode, FILE_ATTRIBUTE_HIDDEN,         'H');
        check(mode, FILE_ATTRIBUTE_SYSTEM,         'S');
        check(mode, FILE_ATTRIBUTE_DIRECTORY,      'D');
        check(mode, FILE_ATTRIBUTE_ARCHIVE,        'A');
        check(mode, FILE_ATTRIBUTE_TEMPORARY,      'T');
        check(mode, FILE_ATTRIBUTE_SPARSE_FILE,    'P');
        check(mode, FILE_ATTRIBUTE_REPARSE_POINT,  'L');
        check(mode, FILE_ATTRIBUTE_COMPRESSED,     'C');
        check(mode, FILE_ATTRIBUTE_OFFLINE,        'O');
        check(mode, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, 'I');
        check(mode, FILE_ATTRIBUTE_ENCRYPTED,      'E');

        while (t < strBuf + STRLEN_RWX) *t++ = ' ';
        strBuf[STRLEN_RWX] = 0;
    }
    else  /* POSIX attributes/permissions */
    {
        switch (objType)
        {
        case OBJ_FILE:    strBuf[0] = _T('-');  break;
        case OBJ_DIR:     strBuf[0] = _T('d');  break;
        case OBJ_FIFO:    strBuf[0] = _T('p');  break;
        case OBJ_BDEV:    strBuf[0] = _T('b');  break;
        case OBJ_CDEV:    strBuf[0] = _T('c');  break;
        case OBJ_SLINK:   strBuf[0] = _T('l');  break;
        case OBJ_SOCKET:  strBuf[0] = _T('s');  break;
        case OBJ_NETSPEC: strBuf[0] = _T('n');  break;
        case OBJ_CDF:     strBuf[0] = _T('H');  break;
        case OBJ_XNAM:    strBuf[0] = _T('x');  break;

        default:  /* unknown!!! */
            strBuf[0] = _T('U');
        }

        #define SUID(mode)  (mode & 04000)
        #define SGID(mode)  (mode & 02000)
        #define SBIT(mode)  (mode & 01000)

        #define USER(mode)  ((mode >> 6) & 0007)
        #define GROUP(mode) ((mode >> 3) & 0007)
        #define OTHER(mode) (mode & 0007)

        #define R(mode)     (mode & 4 ? _T('r'): _T('-'))
        #define W(mode)     (mode & 2 ? _T('w'): _T('-'))
        #define X(mode)     (mode & 1 ? _T('x'): _T('-'))

        /* user permissions */
        strBuf[1] = R(USER(mode));
        strBuf[2] = W(USER(mode));
        strBuf[3] = X(USER(mode));
        if (SUID(mode))
            strBuf[3] = (strBuf[3] == _T('x')? _T('s'): _T('S'));

        /* group permissions */
        strBuf[4] = R(GROUP(mode));
        strBuf[5] = W(GROUP(mode));
        strBuf[6] = X(GROUP(mode));
        if (SGID(mode))
            strBuf[6] = (strBuf[6] == _T('x')? _T('s'): _T('S'));

        /* other permissions */
        strBuf[7] = R(OTHER(mode));
        strBuf[8] = W(OTHER(mode));
        strBuf[9] = X(OTHER(mode));
        if (SBIT(mode))
            strBuf[9] = (strBuf[9] == _T('x')? _T('t'): _T('T'));

        /* append null character */
        strBuf[10] = _T('\0');
    }

    return(strBuf);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     szProgramName   Program name
* @param     szOptions       Program options
* @param     argc            Number of program arguments
* @param     argv            Input arguments returned in an array
*
* @retval    0       ok
* @retval    -1      error
*
* @brief     Function transforms the parameters from string to an array.
*
* @remarks   argc is always set at least to 1
*            argv[0] s a duplicate string of szProgramName (unless szProgramName
*            is NULL)
*
*//*=========================================================================*/
#define NO_QUOTE    _T(' ')
#define D_QUOTE     _T('\"')
#define S_QUOTE     _T('\'')

int StrToArg(_In_opt_ ctchar *szProgramName, _In_ ctchar *szOptions, _Out_ int *argc, _Out_ tchar **argv[])
{
    return StrToArgEx(szProgramName, szOptions, argc, argv, 0);
}

int StrToArgEx(_In_opt_ ctchar *szProgramName, _In_ ctchar *szOptions, _Out_ int *argc, _Out_ tchar **argv[], _In_ int flags)
{
    int     iCount = 0;
    tchar   **szA = NULL;
    tchar   *szOneOpt;
    ctchar  *pOpt = szOptions;
    tchar   *pOpt1;
    tchar   lQuote;
    int     retainQuotes = 0;

    if ( StrICmp(szProgramName,_T("mbx_bar.exe"))==0 || StrICmp(szProgramName,_T("ese_bar.exe"))==0  ||
         StrICmp(szProgramName,_T("sql_bar.exe"))==0 || StrICmp(szProgramName,_T("eseutil.exe"))==0)
        retainQuotes = 0;
    else
        EnvReadInt(_T("OB2_RETAIN_QUOTES_IN_SCRIPTS"), &retainQuotes);

    /* add program name */
    szA = (tchar **) REALLOC( szA, (++iCount) * sizeof(tchar *) );
    if (flags & STRTOARGEX_FLAG_SKIPFIRSTARG)
    {
        /* will skip the first element in argv from being NULL if szProgramName == NULL */
        iCount--;
    }
    else if (szProgramName)
    {
        szA[iCount-1] = StrNewCopy(szProgramName);
    }
    else
    {
        szA[iCount-1] = NULL;
    }

    while ( (pOpt) && (*pOpt) )
    {
        lQuote = NO_QUOTE;

        while ( (*pOpt) && ( (*pOpt) == _T(' ')) ) pOpt++;
        if (!(*pOpt)) break;

        if ( (*pOpt) == D_QUOTE )
        {
            if (!(*(++pOpt))) break;
            lQuote = D_QUOTE;
        }
        else if ( (*pOpt) == S_QUOTE )
        {
            if (!(*(++pOpt))) break;
            lQuote = S_QUOTE;
        }


        pOpt1 = strchr(pOpt, lQuote);

        while ( (pOpt1!=NULL) && (lQuote!=NO_QUOTE) && (*(pOpt1+1)==lQuote) )
        {
            pOpt1 = strchr(pOpt1+2, lQuote);
        }


        if (pOpt1)
        {
            if( retainQuotes != 0 )
            {
                if (lQuote != NO_QUOTE)
                    szOneOpt = StrNNewCopy(pOpt-1, ( (pOpt1++) - pOpt + 2) );
                else
                    szOneOpt = StrNNewCopy(pOpt, (pOpt1++) - pOpt);
            }
            else
                szOneOpt = StrNNewCopy(pOpt, (pOpt1++) - pOpt);
        }
        else
            szOneOpt = StrNewCopy(pOpt);




        szA = (tchar **) REALLOC( szA, (++iCount) * sizeof(tchar *) );
        szA[iCount-1] = szOneOpt;

        pOpt = pOpt1;
    }

    /* add sentinel at the end, but don't increment the counter */
    szA = REALLOC( szA, (iCount+1) * sizeof(tchar *) );
    szA[iCount] = NULL;

    *argc = iCount;
    *argv = szA;


    if (!(flags & STRTOARGEX_FLAG_NODBGOUTPUT))
    {
        int i;
        DbgPlain(10, _T("\tNumber of arguments = %d"), *argc);
        DbgPlain(10, _T("\tArgument list:"));
        for(i=0; i < *argc; i++)
            DbgPlain(10, _T("\t%s"), NSD((*argv)[i]) );
    }

    return(0);
}

/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     argc            Number of program arguments
* @param     argv            Array of input arguments
*
* @retval    0       ok
* @retval    -1      error
*
* @brief     Function frees the environment allocated by StrToArg function.
*
*//*=========================================================================*/
int FreeStrToArg(int *argc, tchar **argv[])
{
    int i;

    for (i = 0; i < *argc; i++)
        FREE( (*argv)[i] );

    FREE(*argv);

    *argc = 0;

    return 0;
}



/*========================================================================*//**
*
* @ingroup  String Utilities
*
* @brief    Return malloc-ed string, same as input, except that every char 'ch' is doubled
*
*//*=========================================================================*/
MALLOCED tchar *
StrDuplicateChar(const tchar *in, tchar ch)
{
    Variant out = VarUndef;

    if (!in)
        return NULL;

    while (*in)
    {
        VarPutc(&out, *in);
        if (*in == ch)
            VarPutc(&out, *in);
    }

    return V2T(&out);
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @brief     Function will remove first and last quote in str if they exist
*
*//*=========================================================================*/
MALLOCED tchar *
StrTrimQuotes(const tchar *str)
{
    size_t len = StrLen(str);
    if (!str)
        return NULL;

    if (len > 1)
    {
        if ( ( str[0] == '"'  && str[len-1] == '"'  ) ||
             ( str[0] == '\'' && str[len-1] == '\'' )
           )
        {
            return str_memdup(str+1, len-2);
        }
    }

    return (str_memdup(str, len));
}


tchar*
StrTrimQuotesInPlace(MODIFIED tchar *str)
{
    size_t len = StrLen(str);
    if (len < 2)
        return str;

    if ( ( str[0] == '"'  && str[len-1] == '"' ) ||
         ( str[0] == '\'' && str[len-1] == '\'') )
    {
        str[len-1] = 0;
        return str+1;
    }

    return str;
}


/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @brief     Attempts to recognize any special characters that can be used to
*            abuse popen calls to execute scripts other than those in cmnPanLBin.
*
*//*=========================================================================*/
int
StrExecIsNotSecure(const tchar *cmd)
{
    if (cmd) for(; *cmd; cmd++)
    {
        switch(*cmd)
        {
        case ';':   /* UNIX cmd concat */
        case '`':   /* UNIX (ksh) */
        case '&':   /* NT cmd concat (&& implied) */
        case '|':   /* piping */
        case '>':   /* > redirection (>> implied) */
        case '<':   /* < redirection */
            return 1;
        case '$':
            if (cmd[1] == '(') return 1;    /* UNIX (ksh) */
            break;
        case '.':
            if (cmd[1] == '.') return 1;    /* parent dir */
            break;
        default:
            ;/* if you can think of anything else... */
        }
    }

    return 0;
}

/**
* @brief Prohibit unsafe binaries
*
* @param exe - executable name
*
* @return TRUE if executable is blacklisted
*         FALSE otherwise
*
* @remark Prohibit unsafe binaries (e.g. 3rd party binaries
*         in DP distribution that may execute other binaries
*         via input parameters - perl.exe -esystem 'cmd')
*/
int
StrIsExeBlacklisted(const tchar *exe)
{
    return
        0 == StrICmp(exe, _T("perl.exe")) ||
        0 == StrICmp(exe, _T("perl")) ||
        0 == StrICmp(exe, _T("openssl.exe")) ||
        0 == StrICmp(exe, _T("openssl"));
}



/* ===========================================================================
| @remark   cmnBinWhiteList - list of all DP binaries shipped to cmnPanLBin.
|           Remove any binary if you are 100% sure we are not executing it via inet
 =========================================================================== */
static const tchar *cmnBinWhiteList[] = {
#if TARGET_WIN32
    _T("40comupd.exe"),
    _T("backint.exe"),
    _T("bm.exe"),
    _T("bma.exe"),
    _T("cjutil.exe"),
    _T("cma.exe"),
    _T("create_db_clus.pl"),
    _T("cswrapper.exe"),
    _T("D2DRestartBackup.cmd"),
    _T("D2DRestartBackup.pl"),
    _T("D2DRestartBackupBg.cmd"),
    _T("db2arch.exe"),
    _T("db2bar.exe"),
    _T("dbpop.exe"),
    _T("dbsm.exe"),
    _T("devbra.exe"),
    _T("dma.exe"),
    _T("DPComServer.exe"),
    _T("dra.exe"),
    _T("drstart.exe"),
    _T("e2010_bar.exe"),
    _T("ese_bar.exe"),
    _T("ExchangeGre.CLI.exe"),
    _T("ExchangeGre.GUI.exe"),
    _T("grm_check.ps1"),
    _T("inimerger.exe"),
    _T("ldbar.exe"),
    _T("ldbar32.exe"),
    _T("Manager.exe"),
    _T("mbx_bar.exe"),
    _T("mma.exe"),
    _T("Mom.exe"),
    _T("mrgcfg.exe"),
    _T("msutil.exe"),
    _T("mysqlbar.pl"),
    _T("nfsServiceCheck.ps1"),
    _T("ob2onbar.pl"),
    _T("ob2rman.pl"),
    _T("ob2smbsplit.exe"),
    _T("ob2sybase.exe"),
    _T("omniabort.exe"),
    _T("omniamo.exe"),
    _T("omniasutil.pl"),
    _T("omnib2dinfo.exe"),
    _T("OmniBack.msc"),
    _T("omnibidol.bat"),
    _T("omnicab.exe"),
    _T("omnicc.exe"),
    _T("omnicellinfo.exe"),
    _T("omnicheck.exe"),
    _T("omnicjutil.exe"),
    _T("omniclus.exe"),
    _T("omnicreatedl.exe"),
    _T("omnidbcheck.exe"),
    _T("omnidbp4000.exe"),
    _T("omnidbsmis.exe"),
    _T("omnidbutil.exe"),
    _T("omnidbvss.exe"),
    _T("omnidbzdb.exe"),
    _T("omnidbrestore.pl"),
    _T("omnidbutil.exe"),
    _T("omnidbxp.exe"),
    _T("omnidd.exe"),
    _T("omnidlc.exe"),
    _T("omnidls.exe"),
    _T("omnidownload.exe"),
    _T("omnidr.exe"),
    _T("omniex2000SM.bat"),
    _T("omniex2000.exe"),
    _T("omnigetmsg.exe"),
    _T("omnigencert.pl"),
    _T("omnihealthcheck.exe"),
    _T("omniiaputil.exe"),
    _T("omniiconv.exe"),
    _T("omniidol.bat"),
    _T("omniidol.pl"),
    _T("omniindex.bat"),
    _T("omniinetpasswd.exe"),
    _T("omniiso.exe"),
    _T("omnikeymigrate.exe"),
    _T("omnikeytool.exe"),
    _T("omnimcopy.exe"),
    _T("omniminit.exe"),
    _T("omnimigrate.pl"),
    _T("omnimlist.exe"),
    _T("omnimm.exe"),
    _T("omnimnt.exe"),
    _T("omnimver.exe"),
    _T("omninotifupg.exe"),
    _T("omniobjconsolidate.exe"),
    _T("omniobjcopy.exe"),
    _T("omniobjverify.exe"),
    _T("omniofflr.exe"),
    _T("omnipm.exe"),
    _T("omniresolve.exe"),
    _T("omnirpt.exe"),
    _T("omnirsh.exe"),
    _T("omnisap.exe"),
    _T("omnisnapmgr.pl"),
    _T("omnisnmp.exe"),
    _T("omnisrdupdate.exe"),
    _T("omnistoreapputil.exe"),
    _T("omnistat.exe"),
    _T("omnisv.exe"),
    _T("omnitrig.exe"),
    _T("omniupload.exe"),
    _T("omniusb.exe"),
    _T("omniwl.pl"),
    _T("pgsqlbar.exe"),
    _T("postgres_bar.exe"),
	_T("postgresql_bar.pl"),
    _T("rma.exe"),
    _T("sanconf.exe"),
    _T("sap_bl_util.exe"),
    _T("sapback.exe"),
    _T("sapdb_backint.exe"),
    _T("sapdbbar.exe"),
    _T("saprest.exe"),
    _T("saproll.exe"),
    _T("SharePoint_VSS_createbarlists.pl"),
    _T("SharePoint_VSS_backup.ps1"),
    _T("sharepoint_bar.exe"),
    _T("smisa.exe"),
    _T("smisasysdbupgrade.exe"),
    _T("SPUtil.exe"),
    _T("SPUtil_2013.exe"),
    _T("sql_bar.exe"),
    _T("sql_smb.exe"),
    _T("ssea.exe"),
    _T("sseasysdbupgrade.exe"),
    _T("storeonceinfo.exe"),
    _T("StoreOnceSoftware.exe"),
    _T("stutil.exe"),
    _T("syb_tool.exe"),
    _T("syma.exe"),
    _T("testbar2.exe"),
    _T("uma.exe"),
    _T("upgrade_cfg.exe"),
    _T("upgrade_cfg_from_evaa.exe"),
    _T("upgrade_cm_from_evaa.exe"),
    _T("util_cmd.exe"),
    _T("util_db2.exe"),
    _T("util_informix.pl"),
    _T("util_mysqlbinlog.exe"),
    _T("util_mysql.pl"),
    _T("util_notes.exe"),
    _T("util_notes32.exe"),
    _T("util_orarest.exe"),
    _T("util_oracle8.pl"),
    _T("util_pgsql.exe"),
    _T("util_sapdb.exe"),
    _T("util_sybase.pl"),
    _T("util_sap.exe"),
    _T("util_vmware.exe"),
    _T("vdsa.exe"),
    _T("vepa_bar.exe"),
    _T("vepa_util.exe"),
    _T("vmware_bar.exe"),
    _T("vmwaregre-agent.exe"),
    _T("vssbar.exe")
#elif TARGET_OPENVMS
    _T("BDA-NET.EXE"),
    _T("BMA-NET.EXE"),
    _T("BMA.EXE"),
    _T("CAT_D.EXE"),
    _T("CAT_E.EXE"),
    _T("CMA-NET.EXE"),
    _T("CMA.EXE"),
    _T("DEVBRA.EXE"),
    _T("DMA.EXE"),
    _T("ECHO_D.EXE"),
    _T("ECHO_E.EXE"),
    _T("FSBRDA.EXE"),
    _T("INET.EXE"),
    _T("MMA.EXE"),
    _T("OMNIABORT.EXE"),
    _T("OMNIAMO.EXE"),
    _T("OMNIB.EXE"),
    _T("OMNICC.EXE"),
    _T("OMNICELLINFO.EXE"),
    _T("OMNICHECK.EXE"),
    _T("OMNICREATEDL.EXE"),
    _T("OMNIDB.EXE"),
    _T("OMNIDBUPGRADE.EXE"),
    _T("OMNIDLS.EXE"),
    _T("OMNIDOWNLOAD.EXE"),
    _T("OMNIGETMSG.EXE"),
    _T("OMNIHEALTHCHECK.EXE"),
    _T("OMNIMCOPY.EXE"),
    _T("OMNIMINIT.EXE"),
    _T("OMNIMLIST.EXE"),
    _T("OMNIMM.EXE"),
    _T("OMNIMNT.EXE"),
    _T("OMNIMVER.EXE"),
    _T("OMNINOTIFUPG.EXE"),
    _T("OMNIOBJCONSOLIDATE.EXE"),
    _T("OMNIOBJCOPY.EXE"),
    _T("OMNIOBJVERIFY.EXE"),
    _T("OMNIR.EXE"),
    _T("OMNIRPT.EXE"),
    _T("OMNISTAT.EXE"),
    _T("OMNIUPLOAD.EXE"),
    _T("RDA-NET.EXE"),
    _T("RMA-NET.EXE"),
    _T("RMA.EXE"),
    _T("TAPEANALYSER.EXE"),
    _T("TESTBAR2.EXE"),
    _T("UMA.EXE"),
    _T("UTIL_CMD.EXE"),
    _T("UTIL_ORAREST.EXE"),
    _T("VBDA.EXE"),
    _T("VRDA.EXE"),
    _T("OMNI$DIAGNOSE.COM"),
    _T("UTIL.COM"),
    _T("OB2RMAN.PL"),
    _T("UTIL_ORACLE8.PL")
#else
    _T(".util"),
    _T("D2DRestartBackup.pl"),
    _T("D2DRestartBackup.sh"),
    _T("D2DRestartBackupBg.sh"),
    _T("IDBsetup.sh"),
    _T("SharePoint_VSS_createbarlists.pl"),
    _T("StoreOnceSoftware"),
    _T("StoreOnceSoftwared"),
    _T("backint"),
    _T("balance_disk.pl"),
    _T("bma"),
    _T("bmsetup"),
    _T("cma"),
    _T("csfailover.ksh"),
    _T("db2arch"),
    _T("db2bar"),
    _T("db2bar_32"),
    _T("dbsm"),
    _T("devbra"),
    _T("dma"),
    _T("dma.exe"),
    _T("dpfs"),
    _T("dpsvcsetup.sh"),
    _T("dra"),
    _T("drstart"),
    _T("echo_d"),
    _T("echo_e"),
    _T("get_info"),
    _T("hostlookup"),
    _T("hplogin"),
    _T("inimerger"),
    _T("ipcck"),
    _T("ldapinit.cli"),
    _T("ldbar.exe"),
    _T("ldbar32.exe"),
    _T("miniel"),
    _T("mma"),
    _T("mrgcfg"),
    _T("mysqlbar.pl"),
    _T("nfsServiceCheck.sh"),
    _T("ob2hana.pl"),
    _T("ob2install"),
    _T("ob2onbar.pl"),
    _T("ob2rman.pl"),
    _T("ob2smbsplit"),
    _T("ob2sybase.pl"),
    _T("oblookup"),
    _T("omniabort"),
    _T("omniamo"),
    _T("omniasutil.pl"),
    _T("omnib2dinfo"),
    _T("omnicab"),
    _T("omnicc"),
    _T("omnicellinfo"),
    _T("omnicheck"),
    _T("omnicjutil"),
    _T("omniclus"),
    _T("omnicreatedl"),
    _T("omnidas.sh"),
    _T("omnidbcheck"),
    _T("omnidbrestore.pl"),
    _T("omnidbsmis"),
    _T("omnidbutil"),
    _T("omnidbvss"),
    _T("omnidbxp"),
    _T("omnidbzdb"),
    _T("omnidl"),
    _T("omnidlc"),
    _T("omnidls"),
    _T("omnidownload"),
    _T("omnidr"),
    _T("omniex2000SM.sh"),
    _T("omnigencert.pl"),
    _T("omnigetmsg"),
    _T("omnihealthcheck"),
    _T("omniiconv"),
    _T("omniiso"),
    _T("omnikeymigrate"),
    _T("omnikeytool"),
    _T("omnimcopy"),
    _T("omnimigrate.pl"),
    _T("omniminit"),
    _T("omnimlist"),
    _T("omnimm"),
    _T("omnimnt"),
    _T("omnimver"),
    _T("omninotifupg"),
    _T("omniobjconsolidate"),
    _T("omniobjcopy"),
    _T("omniobjverify"),
    _T("omniofflr"),
    _T("omniresolve"),
    _T("omnirpt"),
    _T("omnirsh"),
    _T("omnisap.exe"),
    _T("omniscp.sh"),
    _T("omnisftp.sh"),
    _T("omnisrdupdate"),
    _T("omnisrdx"),
    _T("omnissh.sh"),
    _T("omnistat"),
    _T("omnistoreapputil"),
    _T("omnistream"),
    _T("omnisv"),
    _T("omnitrig"),
    _T("omniupload"),
    _T("omniusers"),
    _T("omniwl.pl"),
    _T("pgsqlbar.exe"),
    _T("postgres_bar.exe"),
	_T("postgresql_bar.pl"),
    _T("rma"),
    _T("sanconf"),
    _T("sap_bl_util"),
    _T("sap_bl_util.exe"),
    _T("sapback"),
    _T("sapdb_backint"),
    _T("sapdbbar.exe"),
    _T("saphana_backint"),
    _T("saprest"),
    _T("saproll"),
    _T("smisa"),
    _T("smisasysdbupgrade"),
    _T("ssea"),
    _T("ssi"),
    _T("ssi.sh"),
    _T("storeonceinfo"),
    _T("syb_tool"),
    _T("syma"),
    _T("t_parent"),
    _T("testbar2"),
    _T("uma"),
    _T("upgrade_cfg_from_evaa"),
    _T("upgrade_cm_from_evaa"),
    _T("util_cmd"),
    _T("util_cmd.exe"),
    _T("util_db2"),
    _T("util_db2_32"),
    _T("util_hana.pl"),
    _T("util_informix.pl"),
    _T("util_mysql.pl"),
    _T("util_mysqlbinlog"),
    _T("util_notes.exe"),
    _T("util_notes32.exe"),
    _T("util_oracle8.pl"),
    _T("util_orarest.exe"),
    _T("util_orarest_64.exe"),
    _T("util_pgsql.exe"),
	_T("util_postgreSQL.pl"),
    _T("util_sap.exe"),
    _T("util_sapdb"),
    _T("util_sybase.pl"),
    _T("util_vmware.exe"),
    _T("vcsfailover.ksh"),
    _T("vepa_bar.exe"),
    _T("vepa_util.exe"),
    _T("vix-vmwaregre-agent.exe"),
    _T("vmware_bar.exe"),
    _T("vmwaregre-agent.exe"),
#endif
};


BOOL
StrIsExeWhitelisted(const tchar *exe)
{
    int i;
    for (i = 0; i < _countof(cmnBinWhiteList); i++)
    {
        if (0 == PathCmp(exe, cmnBinWhiteList[i]))
        {
            return TRUE;
        }
    }

    return FALSE;
}


// @brief   check if user-supplied script name is safe to be executed as pre/post-exec script
BOOL
StrScriptIsSecure(ctchar *exe)
{
    int type = StrClassifyPath(exe);
    BOOL typeOK = type == PATH_TYPE_BASENAME || type == PATH_TYPE_RELATIVE;
    return typeOK && !StrExecIsNotSecure(exe) && !StrIsExeBlacklisted(StrBasename(exe));
}





/* ============================================================================
| @brief    parse next line from omnirc file
| @returns   1  ok, got line
|            0  no more lines
 ============================================================================== */
void
OmnircInit(omnirc_t *omnirc, tchar *buffer)
{
    memset (omnirc, 0, sizeof *omnirc);
    omnirc->start = buffer;
    omnirc->line  = buffer;
}


/* @brief   parse single next line from <omnirc> previously initialized by OmnircInit */
BOOL
OmnircReadLine(omnirc_t *omnirc)
{
    tchar *s  = NULL;
    tchar *e  = NULL;
    tchar *eq = NULL;
    tchar *x  = NULL;

    if (!omnirc || !omnirc->start)
    {
        return (FALSE);
    }

    /* restore previous line */
    if (!omnirc->leaveEos && omnirc->nameEndChar)
    {
        omnirc->nameEnd[0] = omnirc->nameEndChar;
    }

    if (!omnirc->leaveEos && omnirc->valEndChar)
    {
        omnirc->valEnd[0] = omnirc->valEndChar;
    }

    omnirc->valEndChar  = 0;
    omnirc->nameEndChar = 0;

nextLine: /* process next line */
    s = omnirc->line;
    if (!s)
    {
        return FALSE;
    }

    while (*s && CharIsSpace(*s)) s++;

    e = strchr(s, _T('\n'));
    omnirc->line = e + 1;

    /* last line w/o eol */
    if (!e)
    {
        e = s + StrLen(s);
        omnirc->line = NULL;
    }

    /* trim value (and line) trailing blanks */
    while (e > s && CharIsSpace(*e)) e--;

    /* skip empty lines, comment lines, or lines without name (e.g. "= value") */
    if (e == s || *s == _T('#') || *s == 0 || *s == _T('='))
        goto nextLine;

    eq = strchr(s, _T('='));

    /* ignore invalid line format */
    if (!eq)
        goto nextLine;

    /* trim name trailing blanks */
    for (x = eq - 1; CharIsSpace(*x); --x);
    omnirc->name        = s;
    omnirc->nameEndChar = x[1];
    omnirc->nameEnd     = x+1;
    x[1] = 0; /* terminate name */

    /* trim value leading blanks */
    for (x = eq + 1; CharIsSpace(*x); ++x);
    omnirc->value      = x;
    omnirc->valEndChar = e[1];
    omnirc->valEnd     = e+1;
    e[1] = 0; /* terminate value */

    return (TRUE);
}

/* ============================================================================
| @brief    parse next line from global file
| @returns   1  ok, got line
|            0  no more lines
 ============================================================================== */
void
GlobalInit(global_t *global, tchar *buffer)
{
    memset(global, 0, sizeof *global);
    global->start = buffer;
    global->line = buffer;
}


/* @brief   parse single next line from <global> previously initialized by GlobalInit */
BOOL
GlobalReadLine(global_t *global)
{
    tchar *s = NULL;
    tchar *e = NULL;
    tchar *eq = NULL;
    tchar *x = NULL;

    if (!global || !global->start)
    {
        return (FALSE);
    }

    /* restore previous line */
    if (!global->leaveEos && global->nameEndChar)
    {
        global->nameEnd[0] = global->nameEndChar;
    }

    if (!global->leaveEos && global->valEndChar)
    {
        global->valEnd[0] = global->valEndChar;
    }

    global->valEndChar = 0;
    global->nameEndChar = 0;

nextLine: /* process next line */
    s = global->line;
    if (!s)
    {
        return FALSE;
    }

    while (*s && CharIsSpace(*s)) s++;

    e = strchr(s, _T('\n'));
    global->line = e + 1;

    /* last line w/o eol */
    if (!e)
    {
        e = s + StrLen(s);
        global->line = NULL;
    }

    /* trim value (and line) trailing blanks */
    while (e > s && CharIsSpace(*e)) e--;

    /* skip empty lines, comment lines, or lines without name (e.g. "= value") */
    if (e == s || *s == _T('#') || *s == 0 || *s == _T('='))
        goto nextLine;

    eq = strchr(s, _T('='));

    /* ignore invalid line format */
    if (!eq)
        goto nextLine;

    /* trim name trailing blanks */
    for (x = eq - 1; CharIsSpace(*x); --x);
    global->name = s;
    global->nameEndChar = x[1];
    global->nameEnd = x + 1;
    x[1] = 0; /* terminate name */

    /* trim value leading blanks */
    for (x = eq + 1; CharIsSpace(*x); ++x);
    global->value = x;
    global->valEndChar = e[1];
    global->valEnd = e + 1;
    e[1] = 0; /* terminate value */

    return (TRUE);
}

/*========================================================================*//**
*
* @retval    0       OK
*           -1       invalid input arguments
*
* @brief     The functions convert hex strings to binary data and vice versa
*
*//*=========================================================================*/

int
StrBytesToHex(IN const void *ptr, IN size_t len, _Out_z_cap_(maxLen+1) tchar *str, IN size_t maxLen)
{
    const UCHAR *data = ptr;
    maxLen /= 2;

    str[0] = 0;

    if (len == 0)
    {
        return -1;  /* produce error on 0 length */
    }

    for (;;)
    {
        if (len == 0) break;
        if (maxLen == 0) return -1;

        sprintf(str, _T("%02X"), *data);

        str += 2;
        data++;
        maxLen--;
        len--;
    }

    return 0;
}


int
StrHexToBytes(IN const tchar *str, OUT void *out, IN size_t maxLen, OUT int *len)
{
    UCHAR *data = out;

    int temp1, temp2;
    int outLen = 0;

    if (str == NULL) goto error_lbl;
    if (*str == 0) goto error_lbl;  /* produce error on empty strings */

    for (;;)
    {
        if (*str == 0) break;
        if (maxLen == 0) break;

        if (sscanf(str++, _T("%1X"), &temp1) != 1) goto error_lbl;
        if (sscanf(str++, _T("%1X"), &temp2) != 1) goto error_lbl;

        *data = (UCHAR)((temp1<<4) + temp2);

        data++;
        maxLen--;
        outLen++;
    }

    if (len)
        *len = outLen;

    return 0;

error_lbl:
    if (len)
        *len = outLen;

    return -1;
}


const tchar*
StrBufferToHex(const void* ptr, size_t ndata)
{
    static const tchar hexdigit[] = _T("0123456789ABCDEF");
    const UCHAR *data = ptr;

    tchar *buf = CmnGetTmpStr(NULL, ndata * 2 + 1);
    size_t pos, i;

    for (i = pos = 0; i < ndata; ++i)
    {
        buf[pos++] = hexdigit[data[i] >> 4];
        buf[pos++] = hexdigit[data[i] & 0xF];
    }

    buf[pos] = 0;

    return buf;
}

#if TARGET_WIN32 && defined(UNICODE)
#   define Unmap(unmap,ch) ((DPWCHAR)(ch) > 255? -1 : unmap[(UCHAR)ch])
#else
#   define Unmap(unmap,ch) unmap[(UCHAR)ch]
#endif

void*
StrHexToBuffer(const tchar *hex, size_t *ndata)
{
    static const signed char unmap[256] =
    {
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
         0,   1,   2,   3,   4,   5,   6,   7,    8,   9,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  10,  11,  12,  13,  14,  15,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  10,  11,  12,  13,  14,  15,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    };

    size_t len = StrLen(hex);
    char  *buf = (char*)CmnGetTmpStr(NULL, len/2 + 1);
    const tchar *t;
    size_t pos = 0;

    if (len == 0)
        return NULL;

    if (len & 0x1) /* odd number of tchars => format error */
        return NULL;

    for (t = hex; *t; t += 2, pos++)
    {
        char upper = Unmap(unmap,t[0]);
        char lower = Unmap(unmap,t[1]);

        if (upper == -1 || lower == -1)
            return NULL;

        buf[pos] = (upper << 4) | lower;
    }

    buf[pos]   = 0;
    buf[pos+1] = 0;

    if (ndata)
        *ndata = pos;

    return buf;
}




/* ===========================================================================
| @brief    resize CmnBuffer storage to <size> bytes
| @returns  internal buffer
| @note     growth strategy:
|           - by chunk size if CmnBuffer::chunk is set to non-zero
|           - twice previous size until CMNBUFFER_MAX_EXP_GROWTH is reached, after that:
|           - in CMNBUFFER_CHUNK_GROWTH increments
 ========================================================================== */
#define CMNBUFFER_MAX_EXP_GROWTH (10* CAPACITY_1MB)
#define CMNBUFFER_CHUNK_GROWTH   (CAPACITY_1MB)
void*
CmnBufferRealloc(CmnBuffer *buf, size_t size)
{
    if (!buf)
        return NULL;

    if (size > buf->size)
    {
        size_t newsize = buf->chunk?
                         (size/buf->chunk + 1) * buf->chunk :
                         buf->size < CMNBUFFER_MAX_EXP_GROWTH?
                         buf->size * 2 :
                         (size/CMNBUFFER_CHUNK_GROWTH + 1) * CMNBUFFER_CHUNK_GROWTH;

        newsize = Maximum(newsize, size);

        if (!REALLOCT (&buf->ptr, newsize + sizeof(DPWCHAR)))
            return NULL;

        if (!buf->nomemzero)
            memset (buf->ptr + buf->size, 0, newsize - buf->size + sizeof(DPWCHAR));

        buf->size = newsize;
    }

    return buf->ptr;
}


/* ===========================================================================
| @brief    resize CmnBuffer storage to accommodate additional <size> bytes
|           and copy given data at added space
| @returns  position in internal CmnBuffer storage where data is copied
========================================================================== */
void*
CmnBufferGrow(CmnBuffer *buf, size_t size)
{
    if (!buf)
        return (NULL);

    CmnBufferRealloc(buf, buf->high + size);
    return buf->ptr + buf->high;
}


void*
CmnBufferPush(CmnBuffer *buf, const void *data, size_t size)
{
    void *pos = CmnBufferGrow(buf, size);
    if (!pos)
        return (NULL);

    if (data && size)
    {
        memcpy (pos, data, size);
    }
    buf->high += size;
    return pos;
}


void*
CmnBufferSet(CmnBuffer *buf, const void *data, size_t size)
{
    if (!buf)
        return NULL;

    if (buf->ptr == data && buf->high >= size)
        return buf->ptr;

    buf->high = 0;
    return CmnBufferPush(buf, data, size);
}


/* @brief   truncate buffer length and return old length */
size_t
CmnBufferSetLen(CmnBuffer *buf, size_t newlen)
{
    size_t oldlen;

    if (!buf)
        return 0;

    CmnBufferRealloc(buf, newlen);

    oldlen    = buf->high;
    buf->high = newlen;
    buf->pos  = 0;
    return oldlen;
}


void
CmnBufferFree(CmnBuffer *buf)
{
    if (buf)
    {
        FREE (buf->ptr);
        memset (buf, 0, sizeof(*buf));
    }
}


/*========================================================================*//**
*
* @brief    Replace variables in given <text> with their values returned by callback <map>
*           Variables inside <text> must be in following format:
*           $VARIABLE_NAME or ${VARIABLE_NAME} or %VARIABLE_NAME%
*           VARIABLE_NAME = [_0-9A-Za-z]+
*
*           Callback must return MALLOCED string which will be free-d in StrExpandString
*
*//*=========================================================================*/
MALLOCED tchar *StrExpandString (const tchar *text, StrMapCallback map, void *userptr)
{
    #define str_realloc(_buf,_bufsz,_pos) \
    if ((_pos) >= (_bufsz))               \
        _buf = REALLOC(_buf, ((_bufsz) = (_pos)+16) * sizeof(tchar));

    #define ch_isvalid(_ch) (CharIsAlnum(_ch) || ((_ch)==_T('_')))

    tchar *buf;
    size_t bufsz;
    size_t pos = 0;
    tchar *copy, *in, *val, *e;
    tchar  save;

    if (!text)
        return NULL;

    /* rather than to think of concurrent threads etc. */
    copy  = StrNewCopy(text);
    buf   = StrNewCopy(text);
    bufsz = StrLen(buf);

    for (in=copy; in[0]; )
    {
        e = in+1;
        switch (in[0])
        {
        case _T('$'):
            /* format: "abc ${VARIABLE}xyz */
            if (e[0]==_T('{'))
            {
                for (e++; ch_isvalid(e[0]); ++e);
                if (e == in+2 || e[0] != _T('}'))
                    goto _default_;

                e[0] = _T('\0'); e++; in++;
            }

            /* format: "abc $VARIABLE xyz" */
            else
            {
                for (; ch_isvalid(e[0]); ++e);
                if (e == in+1)
                    goto _default_;
            }
            break;

        /* format: "abc %VARIABLE%xyz */
        case _T('%'):
            for (; ch_isvalid(e[0]); ++e);
            if (e == in+1 || e[0] != _T('%'))
                goto _default_;

            e[0] = _T('\0'); e++;
            break;

        default:
        _default_:
            str_realloc(buf, bufsz, pos+1);
            buf[pos++] = *in++;
            continue;
        }

        in++; save = e[0]; e[0] = _T('\0');
        val = map(in, userptr);
        e[0] = save;
        if (val)
        {
            size_t len = strlen(val);
            str_realloc (buf, bufsz, pos+len+1);
            strcpy (buf+pos, val);
            pos += len;
            FREE (val);
        }
        in = e;
    }

    str_realloc (buf, bufsz, pos+1);
    buf[pos] = _T('\0');
    FREE (copy);
    return buf;

#undef str_realloc
#undef ch_isvalid
}


/* ===========================================================================
|   FUNCTION    StrExpandEnvironmentStrings
|       Replace variables in given <text> with their values from environment
 ========================================================================== */
MALLOCED tchar *StrExpandEnvironmentStrings (const tchar *text)
{
    return StrExpandString(text, (StrMapCallback)EnvGetString, NULL);
}



/* ===========================================================================
|   FUNCTION    StrTokR
|       portable strtok_r implementation
 ========================================================================== */
tchar *StrTokR (tchar *text, const tchar *sep, tchar **ctx)
{
    tchar *t;

    /* check arguments */
    if (!sep || !ctx)
        return NULL;

    /* successive calls: get current position from context */
    if (!text && (NULL == (text = *ctx)))
        return NULL;

    /* skip leading separators */
    for (; text[0] && strchr(sep, text[0]); text++);

    for (t=text; t[0] && !strchr(sep, t[0]); t++);
    if (!t[0])
    {
        *ctx = NULL;
        return text;
    }

    t[0] = _T('\0');
    *ctx = t+1;
    return text;
}

#if UNICODE
char *StrTokRA (char *text, const char *sep, tchar **ctx)
{
    char *t;

    /* check arguments */
    if (!sep || !ctx)
        return NULL;

    /* successive calls: get current position from context */
    if (!text && (NULL == (text = (char*)*ctx)))
        return NULL;

    /* skip leading separators */
    for (; text[0] && strchrA(sep, text[0]); text++);

    for (t=text; t[0] && !strchrA(sep, t[0]); t++);
    if (!t[0])
    {
        *ctx = NULL;
        return text;
    }

    t[0] = 0;
    *ctx = (tchar*)(t+1);
    return text;
}
#endif


tchar*
StrTokCsv(tchar **ppos, tchar sep, tchar quote)
{
    tchar *start = ppos? *ppos : NULL;
    tchar *a = start, *b = start;

    if (!a)
        return NULL;

    if (a[0] == 0)
        return *ppos = NULL;

    if (!sep)   sep   = ',';
    if (!quote) quote = '"';

    if (a[0] == quote)
    {
        for (++a, ++b; *a; ++a, ++b)
        {
            if (a[0] != quote) { *b = *a; continue; }
            if (a[1] == quote) { *b = *a; a++; continue; }
            a++;
            break;
        }
        *b = 0;
        *ppos = a + (*a == sep);
        return start+1;
    }

    b = strchr(a, sep);
    if (b) { *b = 0; *ppos = b+1; } else { *ppos = 0; }
    return start;
}


const tchar*
StrTail(const tchar *text, size_t n)
{
    size_t len = StrLen(text);
    return !text ? NULL : len>n? text + len-n : text;
}



/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     fname             string to check
* @param     flags             0x00 default, check all known and deprecated
*                              0x01 to check UNIX only
*                              0x02 to check Windows only
*
* @retval    Pointer to the forbidden character or NULL if none is found
*
* @remarks   Behaves like strchr, meaning it returns a pointer to non-const
*            although it accepts a pointer to const!
*
*//*=========================================================================*/

tchar *
FindForbiddenFnmChar (const tchar *fname, unsigned flags)
{
    return
        flags & 0x01 ? strchr(fname, '/') :
        flags & 0x02 ? strpbrk(fname, FORBIDDEN_FNM_CHARS_WINDOWS) :
                       strpbrk(fname, FORBIDDEN_FNM_CHARS_ALL);
}


void
ReplaceForbiddenFnmChars (tchar *string, tchar replacement_char)
{
    tchar *pos = string;

    if (string == NULL) return;
    if (replacement_char == 0) replacement_char = '_';

    for (; (pos = FindForbiddenFnmChar(pos, 0)) != NULL; pos++)
    {
        *pos = replacement_char;
    }
}



/*========================================================================*//**
*
* @brief     Characters /\:*"?*<> are converted to % followed by character hex code
*            (e.g. / is %2F).
*            Percent sign is doubled (% is %%)
*            Useful for filenames.
*
*//*=========================================================================*/
tchar*
StrToUrl(IN const tchar *t)
{
    Variant *out    = CmnGetTmpVar(NULL);
    tchar   *e      = NULL;
    tchar   *outbuf = NULL;

    if (!t)
        return NULL;

    /* maximum encoded buffer size <= 3 * decoded buffer size */
    VarSetLen(out, StrLen(t) * 3);

    e = outbuf = V2T(out);

    for (; *t; ++t)
    {
        switch (*t)
        {
        #define _encode(_tgt, _enc) { \
            const tchar *_text = _enc;\
            _tgt[0]=_T('%');          \
            _tgt[1]=_text[0];         \
            _tgt[2]=_text[1];         \
            _tgt+=3;                  \
        }
        case _T('%') :  e[0] = _T('%'); e[1] = _T('%'); e+=2; break;
        case _T('/') :  _encode(e, _T("2F")); break;
        case _T('\\'):  _encode(e, _T("5C")); break;
        case _T(':') :  _encode(e, _T("3A")); break;
        case _T('*') :  _encode(e, _T("2A")); break;
        case _T('"') :  _encode(e, _T("22")); break;
        case _T('?') :  _encode(e, _T("3F")); break;
        case _T('|') :  _encode(e, _T("7C")); break;
        case _T('<') :  _encode(e, _T("3C")); break;
        case _T('>') :  _encode(e, _T("3E")); break;

        default:
            *e++ = *t;
        }
    }

    VarSetLen(out, e - outbuf);

    return outbuf;
}

/*========================================================================*//**
*
* @brief     Spaces are converted to _
*            (e.g. ' ' is '_').
*
*//*=========================================================================*/
tchar*
StrSpaces2Underscore(MODIFIED tchar *str)
{
    tchar *t = str;
    if (!t || !t[0])
        return NULL;

    for (; *t; t++)
    {
        if (CharIsSpace(*t))
        {
            *t = '_';
        }
    }
    return str;
}

tchar*
StrFromUrl(IN const tchar *t)
{
    Variant *out    = CmnGetTmpVar(NULL);
    tchar   *e      = NULL;
    tchar   *outbuf = NULL;

    if (!t)
        return NULL;

    /* maximum decoded buffer size <= encoded buffer size */
    VarSetLen(out, StrLen(t));

    e = outbuf = V2T(out);

    for (; *t; ++t)
    {
        if (t[0] != _T('%'))
        {
            *e++ = *t;
        }
        else if (t[1] == _T('%'))
        {
            *e++ = _T('%');
            t++;
        }
        else if (t[1] && t[2])
        {
            UCHAR c;
            if (0 != StrHexToBytes (t+1, &c, 1, NULL))
                return NULL;

            *e++ = c;
            t+=2;
        }
        else
        {
            *e++ = *t;
        }
    }

    VarSetLen(out, e - outbuf);

    return outbuf;
}


/*========================================================================*//**
* @brief    Escape given string using percent encoding (see RFC3986)
* @returns  Internal buffer containing escaped string
*
* @details  Could not use StrToUrl() since it escapes minimal set of characters
*           and escapes % as %%.
*//*=========================================================================*/
const tchar*
StrEscapeUrl(IN const tchar *str)
{
    static const char is_unreserved[256] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 0, 0, 0, 1
    };

    static const char hexdigit[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    USES_CONVERSION_T2A

    Variant *out    = CmnGetTmpVar(NULL);
    const char *t   = T2S(str);
    tchar   *e      = NULL;
    tchar   *outbuf = NULL;

    if (!t)
        return NULL;

    /* maximum encoded buffer size <= 3 * decoded buffer size */
    VarSetLen(out, strlenA(t)*3);

    outbuf = V2T(out);

    for (e = outbuf; *t; ++t, ++e)
    {
        UCHAR code = (UCHAR)*t;

        if (is_unreserved[code])
        {
            *e = *t;
        }
        else
        {
            int hi = code >> 4;
            int lo = code & 0XF;
            *e++ = _T('%');
            *e++ = hexdigit[hi];
            *e   = hexdigit[lo];
        }
    }

    VarSetLen(out, e - outbuf);

    return outbuf;

}

/*========================================================================*//**
*
* @ingroup   String Utilities
*
* @param     peerType - one of PEER_XXX #defines
* @param     outbuf - output buffer, at least STRLEN_STD+1 in size
*
* @retval    outbuf is returned
*
* @brief     retrieves the peer type description from message catalog
*
*//*=========================================================================*/
tchar *
StrPeerType(int peerType, tchar outbuf[STRLEN_STD+1])
{
    if (peerType == -1)
    {
        strcpy(outbuf, _T("<invalid peer type -1>"));
    }
    else
    {
        StrCopy(outbuf, NlsGetLiteral(PEER_TYPE_BASE+peerType), STRLEN_STD);
    }

    return(outbuf);
}


/*========================================================================*//**
*
* @param     string            string to cconunt characters in
* @param     find_char         character to count
*
* @retval    Number of times character is found in a string
*
*//*=========================================================================*/
int
StrCountChar(IN const tchar *string, IN tchar find_char)
{
    int count = 0;

    if (string == NULL || find_char == 0)
    {
        return 0;
    }

    for (; *string; ++string)
    {
        count += *string == find_char;
    }
    return count;
}


/* @section integer <-> string support */
/* @brief   return TRUE iff entire given string can be converted to 64-bit signed integer */
BOOL
StrIsInt64(NULLSAFE const tchar *str)
{
    dpint64_t i = 0;
    tchar eos[2];
    return sscanf(NS(str), _T("%lld%1s"), &i, eos) == 1;
}


BOOL
StrIsInt(NULLSAFE const tchar *str)
{
    dpint64_t i = 0;
    tchar eos[2];
    return sscanf(NS(str), _T("%lld%1s"), &i, eos) == 1 && i <= INT_MAX && i >= INT_MIN;
}


dpint64_t
StrToInt64(NULLSAFE const tchar *t)
{
    dpint64_t i = {0};
    return 1==sscanf (NS(t), INT64_FMT, &i)? i : 0;
}


BOOL
StrToInt64m(NULLSAFE const tchar *str, dpint64_t *pval)
{
    ERH_FUNCTION(_T("StrToInt64m"));
    tchar eos[2];
    unsigned  lo = 0, hi = 0;
    int       r;
    dpint64_t val = -1;

    if (!pval) pval = &val;

    r = sscanf(NS(str), _T("%lld%1s"), &val, eos);
    if(r == 1 && val == -1)
    {
        /* special case used for failed object status from MA */
        *pval = val;
        return TRUE;
    }
    #if TARGET_LINUX
    if (r == 1 && val == INT64_MAX) /* overflow for signed */
        r = sscanf(NS(str), _T("%llu%1s"), &val, eos);
    #endif

    if (r == 1)
    {
        if (val < INT_MIN)
            goto overflow;

        if (val < -1)
        {
            val = (unsigned)(int)val;
            DbgPlain(DBG_DETAIL_PROGTRACE, _T("[%s] message '%s': negative value as overflow:%lld"), __FCN__, NS(str), val);
        }
        *pval = val;
        return TRUE;
    }

    r = sscanf(NS(str), _T("%u%u%1s"), &lo, &hi, eos);
    if (r < 1 || r > 2)
        return FALSE;

    val = IntsToLL(hi, lo);
    if (val < 0)
        goto overflow;

    *pval = val;
    return TRUE;

overflow:
    DbgStamp(DBG_UNEXPECTED);
    DbgDumpStack(DBG_UNEXPECTED);
    DbgPlain(DBG_UNEXPECTED, _T("[%s] exception when parsing message '%s': value:%lld is out of range."), __FCN__, NS(str), val);
    if (*pval) *pval = -1;
    return FALSE;
}


int
StrToIntEx(NULLSAFE const tchar *t, int defaultValue)
{
    dpint64_t i = {0};
    tchar eos[2];
    return 1==sscanf (NS(t), _T("%lld%1s"), &i, eos) && i <= INT_MAX && i >= INT_MIN? (int)i : defaultValue;
}


tchar*
StrFromInt64(dpint64_t i)
{
    USES_CMN_PTD
    tchar *buf = ThisCmn->strFromOFF_T64;
    sprintf(buf, INT64_FMT, i);
    return buf;
}


tchar*
StrFromInt(int d)
{
    USES_CMN_PTD
    tchar *strBuf = ThisCmn->strFromInt[ThisCmn->strFromIntPos++];
    ThisCmn->strFromIntPos %= LIBCMN_TMPVAR_COUNT;
    sprintf (strBuf, _T("%d"), d);
    return strBuf;
}

tchar*
   StrFromUInt(unsigned d)
{
   USES_CMN_PTD
   tchar *strBuf = ThisCmn->strFromInt[ThisCmn->strFromIntPos++];
   ThisCmn->strFromIntPos %= LIBCMN_TMPVAR_COUNT;
   sprintf (strBuf, _T("%u"), d);
   return strBuf;
}


/*========================================================================*//**
*
* @param     str                String to extract from
*            defaultUnit        Default multiplier
*            acceptUnits        Support postfix?
*
* @retval    -1                 Errors: - string is empty
*                                       - acceptUnits = false, but string has a postfix
*                                       - postfix is an unsupported unit
*                                       - size < defaultUnit
*                                       - size < 0
*            >0 size in bytes
*
* @brief     Convert string <str> to size in bytes.
*            The <str> format must be: <number><postfix>, or <number> (in this case, default <postfix> is used)
*            where <postfix> is one of K, M, G.
*
*//*=========================================================================*/

dpint64_t
StrToSize(const tchar *str,
          dpint64_t defaultUnit,
          BOOL acceptUnits)
{
    dpint64_t value;
    tchar     unit, eos;
    int       r;
    dpint64_t m = defaultUnit;

    if (!str)
    {
        return -1;
    }

    r = sscanf(str, _T("%lld%c%c"), &value, &unit, &eos);

    switch (r)
    {
        case 1: break;
        case 2:
            if(!acceptUnits)
            {
                return -1;
            }
            switch (toupper(unit))
            {
            case _T('K'): m = CAPACITY_1KB; break;
            case _T('M'): m = CAPACITY_1MB; break;
            case _T('G'): m = CAPACITY_1GB; break;
            default: return -1;
            }
            break;
        default: return -1;
    }

    value = value * m;

    return value < m ? -1 : value;
}

/*========================================================================*//**
*
* @brief     maos for converting:
*            int-2-int
*            int-2-string
*            string-2-string
*
*//*=========================================================================*/

/*========================================================================*//**
* @brief     int-2-int search
*//*=========================================================================*/
int
Int2Int(const Int2IntMap *intMap, int searchVal)
{
    int i;
    for (i=0; intMap[i].leftVal != -1; i++)
    {
        if (intMap[i].leftVal == searchVal)
            return (intMap[i].rightVal);
    }

    return (-1);
}

/*========================================================================*//**
* @brief     int-2-int reverse search
*//*=========================================================================*/
int
Int2Int_Reverse(const Int2IntMap *intMap, int searchVal)
{
    int i;
    for (i=0; intMap[i].rightVal != -1; i++)
    {
        if (intMap[i].rightVal == searchVal)
            break;
    }

    return (intMap[i].leftVal);
}


/*========================================================================*//**
* @brief     int-2-string search
*//*=========================================================================*/
const tchar*
Int2String(const Int2StringMap *intMap, int searchVal)
{
    int i;
    for (i=0; intMap[i].leftVal != -1; i++)
    {
        if (intMap[i].leftVal == searchVal)
            break;
    }

    return (intMap[i].rightVal);
}

/*========================================================================*//**
* @brief     int-2-string reverse search
*//*=========================================================================*/
int
Int2String_Reverse(const Int2StringMap *intMap, const tchar *searchVal)
{
    int i;
    for (i=0; intMap[i].leftVal != -1; i++)
    {
        if (StrCmp(intMap[i].rightVal, searchVal) == 0)
            break;
    }

    return (intMap[i].leftVal);
}

/*========================================================================*//**
* @brief     string-2-string search
*//*=========================================================================*/
const tchar*
String2String(const String2StringMap *intMap, const tchar *searchVal)
{
    int i;
    for (i=0; intMap[i].leftVal; i++)
    {
        if (StrCmp(intMap[i].leftVal, searchVal) == 0)
            break;
    }

    return (intMap[i].rightVal);
}

/*========================================================================*//**
* @brief     string-2-string reverse search
*//*=========================================================================*/
const tchar*
String2String_Reverse(const String2StringMap *intMap, const tchar *searchVal)
{
    int i;
    for (i=0; intMap[i].leftVal; i++)
    {
        if (!StrCmp(intMap[i].rightVal, searchVal))
            break;
    }

    return (intMap[i].leftVal);
}


int
StrMapCharsEx(MODIFIED tchar *str, const tchar *from, const tchar *to, OPTIONAL int *newlen)
{
    int    nchanged = 0;
    size_t nto = StrLen(to);
    tchar *out;
    tchar *t;

    if (StrIsEmpty(from))
        return -1;

    if (!str)
        return 0;

    for (t = out = str; *t; t++)
    {
        ctchar *match = strchr(from, *t);
        size_t pos;
        if (!match)
        {
            *out++ = *t;
            continue;
        }

        nchanged++;
        if (!nto) /* trim: delete (overwrite) matching chars */
            continue;

        pos = match - from;
        pos = Minimum(pos, nto - 1);

        *out++ = to[pos];
    }
    *out = 0;
    if (newlen)
        *newlen = out - str;

    return nchanged;
}

int
StrMapChars(MODIFIED tchar *str, const tchar *from, const tchar *to)
{
    return StrMapCharsEx(str, from, to, NULL);
}


void
StrTrimChars(_Inout_ tchar *str, _In_ ctchar *chars, int flags)
{
    Variant v = CT2V(str);
    VarTrimChars(&v, chars, flags);
}


/* @brief   find if string <str> contains any of <what> strings
 *
 * @param   src     the string that is being searched
 * @param   what    NULL terminated list of strings to search for
 *
 * @retval  TRUE    match is found
 *          FALSE   no match
 */
BOOL
StrContainsAny(const tchar *src, const tchar *what, ...)
{
    va_list va;

    va_start(va, what);
    for (; what; what = va_arg(va,ctchar*))
    {
        if (StrStr(src, what))
        {
            va_end(va);
            return TRUE;
        }
    }

    va_end(va);
    return FALSE;
}


BOOL
StrMatchAny(const tchar *src, const tchar *what, ...)
{
    va_list va;

    va_start(va, what);
    for (; what; what = va_arg(va,ctchar*))
    {
        if (StrCmp(src, what) == 0)
        {
            va_end(va);
            return TRUE;
        }
    }

    va_end(va);
    return FALSE;
}

#define calloca(s) memset(alloca(s), 0, (s))
#if TARGET_WIN32
    typedef SSIZE_T ssize_t;
#endif

DP_ASSERT_EXPR(sizeof(size_t) == sizeof(ssize_t));


/* @brief   return length of longest substring common to <a> and <b> */
size_t
StrLongestCommonLength(const tchar *a, const tchar *b)
{
    size_t m = a? strlen(a) : 0;
    size_t n = b? strlen(b) : 0;
    int   *x = m? calloca(m*sizeof(int)) : 0;
    int   *y = n? calloca(n*sizeof(int)) : 0;
    ssize_t i, j;

    if (!x || !y)
        return 0;

    for (i = m; i >= 0; i--)
    {
        for (j = n; j >= 0; j--)
        {
            if (a[i] == 0 || b[j] == 0)
                x[j] = 0;
            else if (a[i] == b[j])
                x[j] = 1 + y[j+1];
            else
                x[j] = Maximum(y[j], x[j+1]);
        }
        y = x;
    }
    return x[0];
}


int
StrVFormatTo(_Out_z_cap_(count+1) tchar *buffer,
             unsigned flags,
             size_t count,
             _Printf_format_string_ const tchar *format,
             va_list va)
{
    int r;
    if (buffer) buffer[0] = 0;

    if (!buffer || !count) return flags&1? -1 : 0;

    #if TARGET_WIN32
        r = _vsntprintf(buffer, count, format, va);
        buffer[r >= 0 && r <= count? r : count] = 0;
        if (r == -1)
        {
            return (flags & 1) || errno == EINVAL? -1 : (int)count;
        }
        if (r > count)
        {
            return flags & 1 ? -1 : (int)count;
        }
    #else
        r = vsnprintf(buffer, count+1, format, va);
        #if TARGET_HPUX
        /* libc on some hpux systems will return -1 on overflow - like windows */
            if (r == -1) r = count + 1;
        #endif
        if (r > count) r = flags&1? -1 : count;

    #endif

    return r;
}


int
StrNPrintf(_Out_z_cap_(count+1) tchar *buffer, size_t count, _Printf_format_string_ const tchar *format, ...)
{
    int r;
    va_list va;
    va_start (va, format);
    r = StrVFormatTo(buffer, 0x0, count, format, va);
    va_end(va);
    return r;
}


int
StrFormatTo(_Out_z_cap_(count+1) tchar *buffer, size_t count, _Printf_format_string_ const tchar *format, ...)
{
    int r;
    va_list va;
    va_start (va, format);
    r = StrVFormatTo(buffer, 0x1, count, format, va);
    va_end(va);
    return r;
}

#if TARGET_OPENVMS || (TARGET_WIN32 && !defined(UNICODE))
#   define CmnBrandMapKeyword(x) -1
#endif

tchar *StrBcExpand(_Inout_ tchar *fmt, _Out_z_cap_(STRLEN_MESSAGE+1) tchar *outbuf)
{
    int id;
    Variant out = VarFtext((outbuf[0]=0, outbuf), STRLEN_MESSAGE);
    tchar *a = fmt, *start = fmt;
    tchar *sep, *e;

    while (sep = strchr(a, '<'), sep)
    {
        if (sep[1] != '<') { a = sep+1; continue; } 
        if (sep[2] != '<') { a = sep+2; continue; }
        while (sep[3] == '<') sep++; // could be > 3 '<' chars
        e = strchr(sep+3, '>');
        if (!e || e[1] != '>' || e[2] != '>') break;

        e[0] = 0;
        id = sep[3] == 'B' && sep[4] == 'C' && sep[5] == '_'? CmnBrandMapKeyword(sep+6) : -1;

        VarNPuts(&out, start, sep - start);
        VarPuts(&out, id == -1? sep+6 : GetBrandString(id));
        a = start = e+3;
    }
    if (start == fmt) // no need to copy. most catalog strings don't have <<<>>> anyway
        return (tchar*)fmt;

    VarPuts(&out, start);
    return out.data.string;
}

/*========================================================================*//**
* @brief    Convert raw nls catalog messages to display format:
*           - Truncate msg at first EOL.
*           - Replace: \\t -> \t, \\n -> \n, \\ -> \ and \<other_char> -> <other_char>
*           - On windows, remove existing \r and \t
*           - Dynamic branding: expand branding strings - <<<BC_XXX>>> to values
*//*=========================================================================*/
tchar*
NlsTransform(MODIFIED tchar *msg)
{
    USES_CMN_PTD
    tchar *c, *t;
    int  n;

    for (c = t = msg; *c;)
    {
        switch (*c)
        {
        #if TARGET_WIN32
        case '\r':
        case '\t':
            c++; continue;
        #endif
        case '\n':
            goto done;
        case '\\':
            switch (c[1])
            {
            case 't': *t++ = '\t'; c+=2; continue;
            case 'n': *t++ = '\n'; c+=2; continue;
            case  0 : c++; continue;
            #if TARGET_WIN32
            case '\r': c+=2; continue;
            #endif
            }
            c++;
        default:
            n = tccpy(t, c);
            t += n, c += n;
        }
    }
    done:
    *t = 0;

    return StrBcExpand(msg, ThisCmn->nlsBcExpand);
}

/* === EOF === */
