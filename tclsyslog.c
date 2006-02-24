/* Syslog interface for tcl

*/
#include <tcl.h>
#include <syslog.h>
#include <string.h>
typedef struct {
                int logOpened;
                int facility,options;
                char ident[32];
                Tcl_HashTable *priorities;
                Tcl_HashTable *facilities;
		Tcl_HashTable *option_names;
               } SyslogInfo;

void Syslog_ListHash(Tcl_Interp *interp,Tcl_HashTable *table);	       
/* SyslogHelp - puts usage message into interp->result
 * 
 *
 */

void SyslogHelp(Tcl_Interp *interp,char *cmdname)
{  Tcl_AppendResult(interp,"Wrong # of args. should be ",cmdname, 
           " ?option value? priority message",NULL);
}

/* Syslog_Log -
 * implements syslog tcl command. General format: syslog ?options? level text
 * options -facility -ident -options
 * 
 */


int Syslog_Log(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST  objv[])
{    SyslogInfo *info=(SyslogInfo *)data;
    Tcl_DString *message = NULL;
    int priority;
    int i=1;
    if (objc<=1) {
        SyslogHelp(interp,Tcl_GetString(objv[0]));
        return TCL_ERROR;
    }
  while (i<objc-1) {
    if (!strncmp(Tcl_GetString(objv[i]),"-facility",10)) {
	char *facility_name = Tcl_GetString(objv[i+1]);
        Tcl_HashEntry * entry=Tcl_FindHashEntry(info->facilities,facility_name);
        if (!entry) {
           Tcl_AppendResult(interp,"Invalid facility name: \"",Tcl_GetString(objv[i+1]),"\"",
                " available facilities: ",
		   NULL);
	   Syslog_ListHash(interp,info->facilities);
           return TCL_ERROR;
        }
        info->facility=(int)Tcl_GetHashValue(entry);
        if (info-> logOpened) {
            closelog();
            info-> logOpened=0;
        }
     } else if (!strncmp(Tcl_GetString(objv[i]),"-options",9)) {
            int tmp;
            int j,n;
	    Tcl_Obj *elem;
	    Tcl_HashEntry *entry;
	    Tcl_ResetResult(interp);
	    tmp=0;
	    if (Tcl_ListObjLength(interp,objv[i+1],&n)==TCL_ERROR) {
		return TCL_ERROR;
            }		
	    for (j=0;j<n;j++) {
	        Tcl_ListObjIndex(interp,objv[i+1],j,&elem);
	        entry=Tcl_FindHashEntry(info->option_names,Tcl_GetString(elem));
	        if (!entry) {
		    if (n!=1 || Tcl_GetIntFromObj(interp,elem,&tmp)!=TCL_OK) {
			Tcl_AppendResult(interp,"Invalid option '",
				Tcl_GetString(elem),"' valid ones are:",NULL);
			Syslog_ListHash(interp,info->option_names);
			return TCL_ERROR;
	            }
		} else {   	 
		    tmp |= (int) Tcl_GetHashValue(entry);
		}    
	    }	
	    
        info->options=tmp;
        if (info->logOpened) {
            closelog();
            info->logOpened=0;
        }
     } else if (!strncmp(Tcl_GetStringFromObj(objv[i],NULL),"-ident",7)) {
        char *ident_name=Tcl_GetString(objv[i+1]);
	Tcl_DString *dstring=(Tcl_DString *)Tcl_Alloc(sizeof(Tcl_DString));
	Tcl_DStringInit(dstring);
	Tcl_UtfToExternalDString(NULL,ident_name,strlen(ident_name),
          dstring);		 
	 strncpy(info->ident,Tcl_DStringValue(dstring),32);
	 Tcl_DStringFree(dstring);
	 Tcl_Free((char *)dstring);
        info->ident[31]=0;
        if (info->logOpened) {
            closelog();
            info->logOpened=0;
        }
     } else {
	 char *messageutf;
       Tcl_HashEntry *entry=Tcl_FindHashEntry(info->priorities,Tcl_GetString(objv[i]));
       if (!entry) {
          Tcl_AppendResult(interp,"Invalid syslog level \"",Tcl_GetString(objv[i]),"\"",
		  " available levels: ",
               NULL);
	  Syslog_ListHash(interp,info->priorities); 
          return TCL_ERROR;
       }
       priority=(int)Tcl_GetHashValue(entry);
       message=(Tcl_DString *)Tcl_Alloc(sizeof(Tcl_DString));
       Tcl_DStringInit(message);
       messageutf=Tcl_GetString(objv[i+1]);
       Tcl_UtfToExternalDString(NULL,messageutf,strlen(messageutf),
	        message);
      
       i+=2;
       if (i<objc-1) {
           SyslogHelp(interp,Tcl_GetString(objv[0]));
           return TCL_ERROR;
       }
     }
     i+=2;
  }
  if (i<objc-1) {
     SyslogHelp(interp,Tcl_GetString(objv[0]));
     return TCL_ERROR;
  }
  if (message) {
      if (!info->logOpened) {
	  openlog(info->ident,info->options,info->facility);
	  info->logOpened=1;
      }
      syslog(priority,"%s",Tcl_DStringValue(message));
      Tcl_DStringFree(message);
      Tcl_Free((char *)message);
  }
  return TCL_OK;
}
/* 
 *  Syslog_Delete - Tcl_CmdDeleteProc for syslog command.
 *  Frees all hash tables and closes log if it was opened.
 */
void Syslog_Delete(ClientData data)
{ SyslogInfo *info=(SyslogInfo *)data;
  Tcl_DeleteHashTable(info->facilities);
  Tcl_Free((char *)info->facilities);
  Tcl_DeleteHashTable(info->priorities);
  Tcl_Free((char *)info->priorities);
  if (info->logOpened) {
     closelog();
  }
  Tcl_Free((char *)info);
}

/*
 * Syslog_ListHash - appends to interp result all the values of given
 * hash table
 */
void Syslog_ListHash(Tcl_Interp *interp,Tcl_HashTable *table) 
{
    Tcl_HashSearch *searchPtr=(Tcl_HashSearch *)
	  Tcl_Alloc(sizeof(Tcl_HashSearch));
    Tcl_HashEntry *entry;
    char separator[3]={' ',' ',0};   
    entry=Tcl_FirstHashEntry(table,searchPtr);
    while (entry) {
        Tcl_AppendResult(interp,separator,Tcl_GetHashKey(table,entry),NULL);
        separator[0]=',';
        entry=Tcl_NextHashEntry(searchPtr);
    }   
    Tcl_Free((char *)searchPtr);
} 
/*
 * My simplified wrapper for add values into hash
 *
 */
void AddEntry(Tcl_HashTable *table,char *key,int value)
{ int new;
  Tcl_HashEntry *entry=Tcl_CreateHashEntry(table,key,&new);
  Tcl_SetHashValue(entry,(ClientData)value);
}
/*
 * Syslog_Init 
 * Package initialization procedure for Syslog package. 
 * Creates command 'syslog', fills hash tables to map symbolic prioriry 
 * and facility names to system constants.
 */
int Syslog_Init(Tcl_Interp *interp)
{  char *argv0;
    SyslogInfo *info;
   if (Tcl_InitStubs(interp,"8.1",0)==NULL) {
      return TCL_ERROR;
   }     
   info=(SyslogInfo *)Tcl_Alloc(sizeof(SyslogInfo));
   info->logOpened=0;
   info->options=0;
   info->facility=LOG_USER;
   argv0=Tcl_GetVar(interp,"argv0",TCL_GLOBAL_ONLY);
   if (argv0) {
       strncpy(info->ident,argv0,32);
   } else {
       strcpy(info->ident,"Tcl script");
   }
   info->ident[31]=0;
   info->facilities =(Tcl_HashTable *) Tcl_Alloc(sizeof(Tcl_HashTable));
   Tcl_InitHashTable(info->facilities,TCL_STRING_KEYS);
   AddEntry(info->facilities,"auth",LOG_AUTH);  
#ifndef LOG_AUTHPRIV
# define LOG_AUTHPRIV LOG_AUTH
#endif
   AddEntry(info->facilities,"authpriv",LOG_AUTHPRIV);  
   AddEntry(info->facilities,"cron",LOG_CRON);  
   AddEntry(info->facilities,"daemon",LOG_DAEMON);  
   AddEntry(info->facilities,"kernel",LOG_KERN);
   AddEntry(info->facilities,"lpr",LOG_LPR);
   AddEntry(info->facilities,"mail",LOG_MAIL);
   AddEntry(info->facilities,"news",LOG_NEWS);
   AddEntry(info->facilities,"syslog",LOG_SYSLOG);
   AddEntry(info->facilities,"user",LOG_USER);
   AddEntry(info->facilities,"uucp",LOG_UUCP);
   AddEntry(info->facilities,"local0",LOG_LOCAL0);
   AddEntry(info->facilities,"local1",LOG_LOCAL1);
   AddEntry(info->facilities,"local2",LOG_LOCAL2);
   AddEntry(info->facilities,"local3",LOG_LOCAL3);
   AddEntry(info->facilities,"local4",LOG_LOCAL4);
   AddEntry(info->facilities,"local5",LOG_LOCAL5);
   AddEntry(info->facilities,"local6",LOG_LOCAL6);
   AddEntry(info->facilities,"local7",LOG_LOCAL7);
   info->priorities = (Tcl_HashTable *) Tcl_Alloc(sizeof(Tcl_HashTable));
   Tcl_InitHashTable(info->priorities,TCL_STRING_KEYS);
   AddEntry(info->priorities,"emerg",LOG_EMERG);
   AddEntry(info->priorities,"alert",LOG_ALERT);
   AddEntry(info->priorities,"crit",LOG_CRIT);
   AddEntry(info->priorities,"err",LOG_ERR);
   AddEntry(info->priorities,"error",LOG_ERR);
   AddEntry(info->priorities,"warning",LOG_WARNING);
   AddEntry(info->priorities,"notice",LOG_NOTICE);
   AddEntry(info->priorities,"info",LOG_INFO);
   AddEntry(info->priorities,"debug",LOG_DEBUG);
   info->option_names=(Tcl_HashTable *) Tcl_Alloc(sizeof(Tcl_HashTable));
   Tcl_InitHashTable(info->option_names,TCL_STRING_KEYS);
   AddEntry(info->option_names,"CONS",LOG_CONS);
   AddEntry(info->option_names,"NDELAY",LOG_NDELAY);
   AddEntry(info->option_names,"PERROR",LOG_PERROR);
   AddEntry(info->option_names,"PID",LOG_PID);
   AddEntry(info->option_names,"ODELAY",LOG_ODELAY);
   AddEntry(info->option_names,"NOWAIT",LOG_NOWAIT);
   Tcl_CreateObjCommand(interp,"syslog",Syslog_Log,(ClientData) info,
            Syslog_Delete); 
   return Tcl_PkgProvide(interp,"Syslog",VERSION);
}
