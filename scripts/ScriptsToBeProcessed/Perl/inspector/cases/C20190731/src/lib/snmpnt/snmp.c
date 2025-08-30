/**
*
* (c) Copyright 1993-2009 Hewlett-Packard Development Company, L.P.
*
* @ingroup   ModuleName OPC  
* @file      lib/snmpnt/snmp.c
*
* @par Last Change:
* $Rev$
* $Date$
*
* @brief     OmniBackII integration to OpenView
*
* @since     
*
* @remarks   ...
*/

#include "lib/cmn/target.h"

#ifndef lint
static tchar rcsId[] = _T("$Header: /lib/snmpnt/snmp.c $Rev$ $Date::                      $:") ;
#endif

#include <snmp.h>
#include <mgmtapi.h>

#include "lib/cmn/common.h"

#define DBG_SNMP				141
#define STRLEN_NFTRAP           1200

#define COMMUNITY               "public"
#define VARIABLE_BINDINGS_LEN   7
#define TIMEOUT                 6000 //miliseconds
#define RETRIES                 5

#define OVFILTER_FILE   _T("snmp/OVfilter")

#define OV_NORMAL     _T("Normal")          /* tokens in OVfilter file */
#define OV_WARNING    _T("Warning")         /* just lower case please */
#define OV_MINOR      _T("Minor")
#define OV_MAJOR      _T("Major")
#define OV_CRITICAL   _T("Critical")

/* internal filter representation */

#define ov_ifilter1       (1)
#define ov_ifilter2       (2)
#define ov_ifilter3       (4)
#define ov_ifilter4       (8)
#define ov_ifilter5      (16)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// MGMT API prototypes                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef LPSNMP_MGR_SESSION
(SNMP_FUNC_TYPE 
*SNMPMGROPEN)(
     LPSTR ,              // Name/address of target agent
     LPSTR ,              // Community for target agent
     INT   ,              // Comm time-out in milliseconds
     INT                  // Comm time-out/retry count
);         

typedef BOOL
(SNMP_FUNC_TYPE 
*SNMPMGRCLOSE)(
    LPSNMP_MGR_SESSION    // SNMP session pointer
); 

typedef SNMPAPI
(SNMP_FUNC_TYPE 
*SNMPMGRREQUEST)(                                 
     LPSNMP_MGR_SESSION,   // SNMP session pointer
     BYTE               ,  // Get, GetNext, or Set
     RFC1157VarBindList *, // Varible bindings
     AsnInteger         *, // Result error status
     AsnInteger         *  // Result error index
);      

SNMPMGROPEN    lpfnSnmpMgrOpen;
SNMPMGRCLOSE   lpfnSnmpMgrClose;
SNMPMGRREQUEST lpfnSnmpMgrRequest;

tchar     myLog[STRLEN_STD+1];
tchar     OVFilter[STRLEN_STD+1];

tchar     sCommunity[STRLEN_STD+1];
tchar	  *lpszRegistry;

tchar     ov_filter1[STRLEN_STD+1];
tchar     ov_filter2[STRLEN_STD+1];
tchar     ov_filter3[STRLEN_STD+1];
tchar     ov_filter4[STRLEN_STD+1];
tchar     ov_filter5[STRLEN_STD+1];

/* ---------------------------------------------------------------------------
|         PDU header for the trap 
 -------------------------------------------------------------------------- */
//static ObjectID enterprise[]   = {1,3,6,1,4,1,11,2,17,1};
//static int      enterprise_len = sizeof(enterprise) / sizeof(ObjectID);
//static int      generic_trap   = 6;
//static int      specific_trap  = 59047936;


/* ---------------------------------------------------------------------------
|         object IDs for variables 
 -------------------------------------------------------------------------- */
static UINT oid1[]         = {1,3,6,1,4,1,11,2,17,2,1,0};
static UINT oid2[]         = {1,3,6,1,4,1,11,2,17,2,2,0};
static UINT oid3[]         = {1,3,6,1,4,1,11,2,17,2,3,0};
static UINT oid4[]         = {1,3,6,1,4,1,11,2,17,2,4,0};
static UINT oid5[]         = {1,3,6,1,4,1,11,2,17,2,5,0};
static UINT oid6[]         = {1,3,6,1,4,1,11,2,17,2,6,0};
static UINT oid7[]         = {1,3,6,1,4,1,11,2,17,2,7,0};

static int      oid1_len       = sizeof(oid1) / sizeof(UINT);
static int      oid2_len       = sizeof(oid2) / sizeof(UINT);
static int      oid3_len       = sizeof(oid3) / sizeof(UINT);
static int      oid4_len       = sizeof(oid4) / sizeof(UINT);
static int      oid5_len       = sizeof(oid5) / sizeof(UINT);
static int      oid6_len       = sizeof(oid6) / sizeof(UINT);
static int      oid7_len       = sizeof(oid7) / sizeof(UINT);


/* ---------------------------------------------------------------------------
|    file pointer of our log file - opened once - closed never 
 -------------------------------------------------------------------------- */

//static FILE       *log_fp = FALSE;

#define SERVICE_WAIT_TIME   50 //sec

int SvcEnable(tchar *hname, tchar *svcName, DWORD *dwErrno)
{
	int i=0;
	SC_HANDLE scm, svc;
	SERVICE_STATUS svcStatus;    

	DbgStamp(DBG_SNMP);
	if ((scm = OpenSCManager(
            NULL/*hname*/,
            NULL,
            SC_MANAGER_CONNECT)) == NULL)
	{
          *dwErrno= GetLastError();
          DbgPlain(DBG_SNMP,_T("Error opening SCManager: %d\n"), *dwErrno);
	      return -1;
	}

	  if ((svc = OpenService( scm, 
	    	                  svcName, 
	                          SERVICE_START| GENERIC_READ )) == NULL)
	  {
		    *dwErrno = GetLastError();
            DbgPlain(DBG_SNMP,_T("Error opening Service: %d\n"), *dwErrno);
		    CloseServiceHandle(scm);
		    return -1; 
	  }

	    if (!QueryServiceStatus(svc, &svcStatus))
	    {
	          *dwErrno= GetLastError();
              DbgPlain(DBG_SNMP,_T("Error querying service %s: %d\n"), svcName, *dwErrno);
		      CloseServiceHandle(svc);
	          CloseServiceHandle(scm);
	          return -1;
	    }

         if (svcStatus.dwCurrentState == SERVICE_RUNNING)
         {
            DbgPlain(DBG_SNMP,_T("Service %s already running.\n"), svcName);
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return 0;
         }

		   if( !StartService(svc, 0, NULL) )
		   {
	             *dwErrno = GetLastError();
                     DbgPlain(DBG_SNMP,_T("Error when starting service %s: %d\n"), svcName, *dwErrno);
			     CloseServiceHandle(svc);
	             CloseServiceHandle(scm);
			     return -1;
		   }

			 while ((svcStatus.dwCurrentState != SERVICE_RUNNING)&&(i++<SERVICE_WAIT_TIME) )
		     {
				Sleep(1000);
				if (!QueryServiceStatus(svc, &svcStatus))
				{
					*dwErrno= GetLastError();
					DbgPlain(DBG_SNMP,_T("Error querying service %s: %d\n"), svcName, *dwErrno);
					CloseServiceHandle(svc);
					CloseServiceHandle(scm);
					return -1;
				}
		     }		

	if (i<SERVICE_WAIT_TIME)
	  DbgPlain(DBG_SNMP,_T("Service %s has been successfully started after %d queries.\n"),
                svcName, i+1);
	else
	{
                DbgPlain(DBG_SNMP,
                _T("Service %s cannot be started in %d seconds.Aborting.\n"), svcName, i+1);
                       
	    CloseServiceHandle(svc);
	    CloseServiceHandle(scm);
	    return -1;
	}  	
	
	CloseServiceHandle(svc);
	CloseServiceHandle(scm);

	return 0;
}


void PrintVarBind( RFC1157VarBind *variableBind)
{
    UINT i;

    tchar *szTemp,
          *szLog;

    szLog = (tchar*)MALLOC((STRLEN_MESSAGE + 1)*sizeof(tchar));
    szTemp =(tchar*)MALLOC((STRLEN_STD + 1)*sizeof(tchar));


    strcpy(szLog, _T("\tOID: "));

    for(i=0; i< variableBind->name.idLength; i++)
    {
        sprintf(szTemp, _T("%d."), variableBind->name.ids[i]);
        strcat(szLog, szTemp);
    }

    strcat(szLog, _T("\n\tValue: "));

    if ( variableBind->value.asnType == ASN_INTEGER )
    {
        sprintf(szTemp, _T("%d\n"), variableBind->value.asnValue.number);
    }
    else if ( variableBind->value.asnType == ASN_OCTETSTRING )
    {
        char *tmpString;
        unsigned int ii;

        tmpString=(char*)MALLOC(variableBind->value.asnValue.string.length+3);
        szTemp=(tchar*)REALLOC(szTemp,((variableBind->value.asnValue.string.length+3)*sizeof(tchar)));
        
        for (ii=0; ii<variableBind->value.asnValue.string.length; ii++)
        {
            tmpString[ii] = variableBind->value.asnValue.string.stream[ii]; 
        }
        tmpString[ii] = 0;

        sprintf(szTemp, _T("%hs\n"), (char *) tmpString);

        szLog = (tchar*) REALLOC(szLog, (strlen(szLog) + strlen(szTemp) + 2)*sizeof(tchar));
        FREE(tmpString);
    }

    strcat(szLog, szTemp);

    //WriteLog(szLog);
    DbgPlain(DBG_SNMP, _T("%s"), szLog);

    FREE(szLog);
    FREE(szTemp);
}


void PrintVarBindList( RFC1157VarBindList *variableBindings)
{
    UINT i;

    DbgStamp(DBG_SNMP);
    for (i=0;i< variableBindings->len;i++)
    {
        PrintVarBind( &(variableBindings->list[i]) );
    }
}

/*========================================================================*//**
*
* @ingroup   ModuleName OPC
*
* @param     msg               Message to be sent
* @param     msg_num           Message number
* @param     classnmb          Message class
* @param     param             Parameters (in "keyword=value" style)
*
*//*=========================================================================*/
int 
snmp_trap_send_params(const tchar *msg, const tchar *msg_num, int classnmb, const tchar *param)
{
    USES_CONVERSION_T2A

    int                 i,
                        StrLength,
                        err= 0; // DEFAULT no error
    DWORD               dwError;
    tchar               line[STRLEN_2K+1],
                        severity[STRLEN_STD+1];
    static    int       initialized=0, filter=0;
    
    HINSTANCE           hLibrary;

    RFC1157VarBindList  variableBindings;
    AsnObjectIdentifier asnOID;
    AsnOctetString      asnString;
    tchar               *szBuffer;
    LPSNMP_MGR_SESSION  sessionSNMP= NULL;
    AsnInteger          errorStatus,
                        errorIndex;
    tchar               *filebuf = NULL;
    int                 rptFilter = 1; /* Filter the message */
    char                *msgA;

  
/* ---------------------------------------------------------------------------
|         try to open log file if it is not yet opened 
 -------------------------------------------------------------------------- */

  /* If trap originates from Reporting & Notifications, don't filter the message */
  if (StrCmp(msg_num, _T("NOTIFICATION")) == 0)
  {
      rptFilter = 0;
  }

  if (!initialized)
  {

	  StrCopy(ov_filter1, OV_NORMAL, STRLEN_STD);   StrToLower(ov_filter1);
	  StrCopy(ov_filter2, OV_WARNING, STRLEN_STD);  StrToLower(ov_filter2);
	  StrCopy(ov_filter3, OV_MINOR, STRLEN_STD);    StrToLower(ov_filter3);
	  StrCopy(ov_filter4, OV_MAJOR, STRLEN_STD);    StrToLower(ov_filter4);
	  StrCopy(ov_filter5, OV_CRITICAL, STRLEN_STD); StrToLower(ov_filter5);
    
/* ---------------------------------------------------------------------------
|         read filter file if it exists and set the filters 
 -------------------------------------------------------------------------- */


      sprintf(OVFilter ,_T("%s/%s"), cmnPanConfig, OVFILTER_FILE);
      filebuf = CmnGetAsciiFile(OVFilter);
      if (filebuf != NULL)
      {
          StrCopy(line, filebuf, STRLEN_STD);
          FREE(filebuf);
          DbgStamp(DBG_SNMP);
          DbgPlain(DBG_SNMP, _T("Opening SNMP OVfilter file\n"));

              DbgPlain(DBG_SNMP, _T("File contents:\n%s"), line);

             /* --------------------------------------------------------------
             |   convert to lower case 
              --------------------------------------------------------------- */

              for (i = 0; line[i]; i++) line[i] = tolower(line[i]);

             /* --------------------------------------------------------------
             |   try to find level 1 token 
              --------------------------------------------------------------- */

              if (strstr(line, ov_filter1)) filter |= ov_ifilter1;

             /* --------------------------------------------------------------
             |   try to find level 2 token 
              --------------------------------------------------------------- */

              if (strstr(line, ov_filter2)) filter |= ov_ifilter2;

              /* -------------------------------------------------------------
              |   try to find level 3 token 
               -------------------------------------------------------------- */

              if (strstr(line, ov_filter3)) filter |= ov_ifilter3;
              
			  /* -------------------------------------------------------------
              |   try to find level 4 token 
               -------------------------------------------------------------- */

              if (strstr(line, ov_filter4)) filter |= ov_ifilter4;

			  /* -------------------------------------------------------------
              |   try to find level 5 token 
               -------------------------------------------------------------- */

              if (strstr(line, ov_filter5)) filter |= ov_ifilter5;
      }
      else
      {
          ErhClearError();
      }

      DbgStamp(DBG_SNMP);
      DbgPlain(DBG_SNMP, _T("SNMP Initialized OVfilter %d\n"), filter);

      initialized = 1;
  }

  /* --------------------------------------------------------------------
  |   if received message is filtered there is nothing to send 
   -------------------------------------------------------------------- */

  StrCopy(line, msg, STRLEN_2K);

  DbgStamp(DBG_SNMP);
  DbgPlain(DBG_SNMP, _T("SNMP msg: %s\n"), msg);

  DbgStamp(DBG_SNMP);
  DbgPlain(DBG_SNMP, _T("SNMP param: %s\n"), param);

  StrToLower(line);
  
  DbgStamp(DBG_SNMP);
  DbgPlain(DBG_SNMP, _T("SNMP OVfilter %d\n"), filter);

  StrCopy(ov_filter1, StrToLower(NlsGetLiteral(ERH_LEVEL_BASE+4)), STRLEN_STD);
  StrCopy(ov_filter2, StrToLower(NlsGetLiteral(ERH_LEVEL_BASE)), STRLEN_STD);
  StrCopy(ov_filter3, StrToLower(NlsGetLiteral(ERH_LEVEL_BASE+1)), STRLEN_STD);
  StrCopy(ov_filter4, StrToLower(NlsGetLiteral(ERH_LEVEL_BASE+2)), STRLEN_STD);
  StrCopy(ov_filter5, StrToLower(NlsGetLiteral(ERH_LEVEL_BASE+3)), STRLEN_STD);

  StrCopy(severity, OV_NORMAL, STRLEN_STD);

  if (strstr(line, ov_filter1))
  {
	  if (((filter & ov_ifilter1) == ov_ifilter1) && (rptFilter == 1)) return 0;
	  StrCopy(severity, OV_NORMAL, STRLEN_STD);
  } 
  else
  if (strstr(line, ov_filter2))
  {    
	  if (((filter & ov_ifilter2) == ov_ifilter2)  && (rptFilter == 1)) return 0;
	  StrCopy(severity, OV_WARNING, STRLEN_STD);
  }
  else
  if (strstr(line, ov_filter3))
  {
	  if (((filter & ov_ifilter3) == ov_ifilter3)  && (rptFilter == 1)) return 0;
	  StrCopy(severity, OV_MINOR, STRLEN_STD);
  }
  else    
  if (strstr(line, ov_filter4)) 
  {
	  if (((filter & ov_ifilter4) == ov_ifilter4)  && (rptFilter == 1)) return 0;
	  StrCopy(severity, OV_MAJOR, STRLEN_STD);
  }
  else   
  if (strstr(line, ov_filter5)) 
  {
	  if (((filter & ov_ifilter5) == ov_ifilter5)  && (rptFilter == 1)) return 0;
	  StrCopy(severity, OV_CRITICAL, STRLEN_STD);
  }
  else
  {
        if (rptFilter == 1) return 0;
  }

  /* CHECKME: TBD: Why start the service if the user stopped it?! */

  /* Try to start up SNMP service if not started yet */
  if (SvcEnable(NULL, _T("SNMP"), &dwError)<0)
  {
      return -1;
  }
       
  DbgStamp(DBG_SNMP);
  DbgPlain(DBG_SNMP,_T("Start to set up variable bindings\n") );

  variableBindings.len= VARIABLE_BINDINGS_LEN;
  variableBindings.list= ( RFC1157VarBind * ) 
	                     SNMP_malloc( sizeof( RFC1157VarBind ) * VARIABLE_BINDINGS_LEN );

  /* ------------------------------------------------------------------------
  |   set the value and length of variable 1 - application type
   -------------------------------------------------------------------------- */
  
  asnOID.idLength= oid1_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid1_len );
  memcpy( asnOID.ids, oid1, sizeof(UINT) * oid1_len );

  variableBindings.list[0].name= asnOID;  // NOTE structure copy
  variableBindings.list[0].value.asnType= ASN_INTEGER;
  variableBindings.list[0].value.asnValue.number= 1L;     /* 1 = agent */

  /* ------------------------------------------------------------------------
  |   set the value and length of variable 2 - host    
   -------------------------------------------------------------------------- */
  
  // BE CAREFULL THESE FUNCTIONS DOES NOT TAKE CARE OF UNICODE!!!

  asnOID.idLength= oid2_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid2_len );
  memcpy( asnOID.ids, oid2, sizeof(UINT) * oid2_len );
  

  StrLength= (int)strlen(cmnHostname);
  asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
  memcpy( asnString.stream, T2A(cmnHostname), sizeof(BYTE) * StrLength );
  asnString.length= StrLength;
  asnString.dynamic= FALSE;

  variableBindings.list[1].name= asnOID;
  variableBindings.list[1].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[1].value.asnValue.string= asnString;    
  
  /* ------------------------------------------------------------------------
  |   set the value and length of variable 3 - msg_num 
   -------------------------------------------------------------------------- */

  asnOID.idLength= oid3_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid3_len );
  memcpy( asnOID.ids, oid3, sizeof(UINT) * oid3_len );
  

  StrLength= (int)strlen(msg_num);
  asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
  memcpy( asnString.stream, T2A(msg_num), sizeof(BYTE) * StrLength );
  asnString.length= StrLength;
  asnString.dynamic= FALSE;

  variableBindings.list[2].name= asnOID;
  variableBindings.list[2].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[2].value.asnValue.string= asnString;    

  /* ------------------------------------------------------------------------
  |   set the value and length of variable 4 - application 
   -------------------------------------------------------------------------- */

#define OMNIBACK        GetBrandString(BC_SNMP_APPNAME)


  asnOID.idLength= oid4_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid4_len );
  memcpy( asnOID.ids, oid4, sizeof(UINT) * oid4_len );

  {
      tchar *lpszOmniBack= NULL;
      
      lpszOmniBack= StrNewCopy(OMNIBACK);

      StrLength= (int)strlen(lpszOmniBack);
      asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
      memcpy( asnString.stream, T2A(lpszOmniBack), sizeof(BYTE) * StrLength );
      asnString.length= StrLength;
      asnString.dynamic= FALSE;

      if (lpszOmniBack) FREE(lpszOmniBack);
  }

  variableBindings.list[3].name= asnOID;
  variableBindings.list[3].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[3].value.asnValue.string= asnString;  
  
  /* ------------------------------------------------------------------------
  |   set the value and length of variable 5 - severity
   -------------------------------------------------------------------------- */

  asnOID.idLength= oid5_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid5_len );
  memcpy( asnOID.ids, oid5, sizeof(UINT) * oid5_len );
  

  StrLength= (int)strlen(severity);
  asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
  memcpy( asnString.stream, T2A(severity), sizeof(BYTE) * StrLength );
  asnString.length= StrLength;
  asnString.dynamic= FALSE;

  variableBindings.list[4].name= asnOID;
  variableBindings.list[4].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[4].value.asnValue.string= asnString;    
  
  /* ------------------------------------------------------------------------
  |   set the value and length of variable 6 - msg 
   -------------------------------------------------------------------------- */

  asnOID.idLength= oid6_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid6_len );
  memcpy( asnOID.ids, oid6, sizeof(UINT) * oid6_len );
  
  /*Traps bigger than 1.5k will be fragmented (ethernet), 
  so we will limit the message size to 1200 bytes to prevent fragmentaion  */
  if (StrLen(msg) > STRLEN_NFTRAP)
  {
    DbgPlain(DBG_SNMP, _T("Message will be trunicated to %d bytes to avoid fragmentation."), STRLEN_NFTRAP);
    
  }
    
  szBuffer = StrNNewCopy(msg, STRLEN_NFTRAP);
  DbgPlain(DBG_SNMP,_T("Message length: %d"),StrLen(szBuffer));
    
  for (i = 0; szBuffer[i]; i++)
  {
      if ((szBuffer[i] == '\n') /*|| !isprint(szBuffer[i])*/)
      {
          szBuffer[i] = ' ';
      }
  }

  msgA=T2A(szBuffer);
  StrLength= (int)strlenA(msgA);
  
  /* SNMP strings NOT zero-terminated */
  /* allocate and copy only StrLength, not StrLength+1 */
  asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
  memcpy(asnString.stream, msgA, StrLength);
  asnString.length= StrLength;
  asnString.dynamic= FALSE;

  variableBindings.list[5].name= asnOID;
  variableBindings.list[5].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[5].value.asnValue.string= asnString;

  FREE(szBuffer);
  
  /* ------------------------------------------------------------------------
  |   set the value and length of variable 7 - param
   -------------------------------------------------------------------------- */

  asnOID.idLength= oid7_len;
  asnOID.ids= (UINT *) SNMP_malloc( sizeof( UINT ) * oid7_len );
  memcpy( asnOID.ids, oid7, sizeof(UINT) * oid7_len );
  
  StrLength= (int)strlen(param);
  asnString.stream= (BYTE *) SNMP_malloc( sizeof( BYTE ) * StrLength );
  memcpy( asnString.stream, T2A(param), sizeof(BYTE) * StrLength );
  asnString.length= StrLength;
  asnString.dynamic= FALSE;

  variableBindings.list[6].name= asnOID;
  variableBindings.list[6].value.asnType= ASN_OCTETSTRING;
  variableBindings.list[6].value.asnValue.string= asnString;  

  
  /* ------------------------------------------------------------------------
  |   p now points to the destination - hostname or IP address
  |   try to establish an active SNMP session     
   -------------------------------------------------------------------------- */

  PrintVarBindList(&variableBindings);
  
  tchar path[STRLEN_1K + 1] = _T("");
  tchar dllPath[STRLEN_1K + 1] = _T("");
  GetSystemDirectory(path, STRLEN_1K);
  CreatePathFromParams(dllPath, _T("%s\\%s"), path, _T("mgmtapi.dll"));

 if ((hLibrary = DPWrapperLoadDLL(dllPath, 0, 0)) == NULL)
 {
	DbgPlain(DBG_SNMP,_T("Error %d in LoadLibrary().\n\n"), hLibrary);
    return(-2);
 }

 if ((lpfnSnmpMgrOpen = (SNMPMGROPEN)GetProcAddress(hLibrary,"SnmpMgrOpen")) == NULL)   /* NOT UNICODE */
 {
	DbgPlain(DBG_SNMP,_T("Error %d in GetProcAddress(SnmpMgrOpen).\n\n"), hLibrary);
    return(-2);
 }
 if ((lpfnSnmpMgrRequest = (SNMPMGRREQUEST)GetProcAddress(hLibrary,"SnmpMgrRequest")) == NULL)   /* NOT UNICODE */
 {
	DbgPlain(DBG_SNMP,_T("Error %d in GetProcAddress(SnmpMgrRequest).\n\n"), hLibrary);
    return(-2);
 }
 if ((lpfnSnmpMgrClose = (SNMPMGRCLOSE)GetProcAddress(hLibrary,"SnmpMgrClose")) == NULL)   /* NOT UNICODE */
 {
	DbgPlain(DBG_SNMP,_T("Error %d in GetProcAddress(SnmpMgrClose).\n\n"), hLibrary);
    return(-2);
 }

 //read Community name from the registry
 
    if ((lpszRegistry=RegGetString(
             HKEY_LOCAL_MACHINE,
             GetBrandString(BC_REG_SNMPTRAP),
			 _T("Community")
             )) == NULL)
	{
	    DbgStamp(DBG_SNMP);
	    DbgPlain(DBG_SNMP, _T("Community info not found in the registry, err=%d"),
				    GetLastError());
	    strcpy(sCommunity, _T(COMMUNITY));
    }
    else
    {
			strcpy(sCommunity, lpszRegistry);
    }
 

  DbgPlain(DBG_SNMP,_T("\nCommunity name: %s \n"), sCommunity);
  SetLastError(0); errorStatus = errorIndex = -1;
  if (!(sessionSNMP= lpfnSnmpMgrOpen( 
	                    T2A(cmnHostname),    //agent
	                    T2A(sCommunity),     //community name
						TIMEOUT,             //timeout
						RETRIES		         //retries
                        ))
	 )
  {
      DbgStamp(DBG_SNMP);
      DbgPlain(DBG_SNMP,_T("Cannot open SNMP Manager. ERROR: %d\n"), GetLastError() );
	  err= GetLastError();
  }
  else if ( !lpfnSnmpMgrRequest( 
	                   sessionSNMP,
	                   ASN_RFC1157_SETREQUEST,
					   &variableBindings,
					   &errorStatus,
					   &errorIndex
                       ) 
		 )
  {
      DbgStamp(DBG_SNMP);
      DbgPlain(DBG_SNMP,_T("Failed to send SNMP trap. ERROR: %d\n"), GetLastError() ); 
	  err= GetLastError();	  

      /* These errors cause delays, log them so the user can deduct the problem */
      DbgStamp(DBG_UNEXPECTED);
      DbgPlain(DBG_UNEXPECTED,_T("Failed to send SNMP trap. Check SNMP service configuration.\n"));
  }
  
  DbgStamp(DBG_SNMP);
  DbgPlain(DBG_SNMP,_T("ERROR STATUS: %d ERROR INDEX: %d.\n"), errorStatus, errorIndex );

  SnmpUtilVarBindListFree( &variableBindings );

  if (sessionSNMP) 
	  if (lpfnSnmpMgrClose( sessionSNMP ))
  {
    DbgStamp(DBG_SNMP);
    DbgPlain(DBG_SNMP,_T("SNMP Session successfully closed.\n") );	  
  }
  else
  {
    DbgStamp(DBG_SNMP);
    DbgPlain(DBG_SNMP,_T("SNMP Session NOT closed. ERROR: %d\n"), GetLastError() );	  
  }

  
  return err;
}

int 
snmp_trap_send(const tchar *msg, const tchar *msg_num, int classnmb)
{
    return (snmp_trap_send_params(msg, msg_num, classnmb, strEmpty));
}

