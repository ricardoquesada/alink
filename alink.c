#include "alink.h"

char case_sensitive=1;
char padsegments=0;
char mapfile=0;
PCHAR mapname=0;
unsigned short maxalloc=0xffff;
int output_type=OUTPUT_EXE;
PCHAR outname=0;

FILE *afile=0;
UINT filepos=0;
long reclength=0;
unsigned char rectype=0;
char li_le=0;
UINT prevofs=0;
long prevseg=0;
long gotstart=0;
RELOC startaddr;
UINT imageBase=0;
UINT fileAlign=1;
UINT objectAlign=1;
UINT stackSize;
UINT stackCommitSize;
UINT heapSize;
UINT heapCommitSize;
unsigned char osMajor,osMinor;
unsigned char subsysMajor,subsysMinor;
unsigned int subSystem;
int buildDll=FALSE;
PUCHAR stubName=NULL;

long errcount=0;

unsigned char buf[65536];
PDATABLOCK lidata;

PPCHAR namelist=NULL;
PPSEG seglist=NULL;
PPSEG outlist=NULL;
PPGRP grplist=NULL;
PSORTENTRY publics=NULL;
PEXTREC externs=NULL;
PPCOMREC comdefs=NULL;
PPRELOC relocs=NULL;
PIMPREC impdefs=NULL;
PEXPREC expdefs=NULL;
PLIBFILE libfiles=NULL;
PRESOURCE resource=NULL;
PSORTENTRY comdats=NULL;
PPCHAR modname;
PPCHAR filename;
UINT namecount=0,namemin=0,
    pubcount=0,pubmin=0,
    segcount=0,segmin=0,outcount=0,
    grpcount=0,grpmin=0,
    extcount=0,extmin=0,
    comcount=0,commin=0,
    fixcount=0,fixmin=0,
    impcount=0,impmin=0,impsreq=0,
    expcount=0,expmin=0,
    nummods=0,
    filecount=0,
    libcount=0,
    rescount=0;
UINT libPathCount=0;
PCHAR *libPath=NULL;
char *entryPoint=NULL;

void processArgs(int argc,char **argv)
{
    long i,j;
    int helpRequested=FALSE;
    UINT setbase,setfalign,setoalign;
    UINT setstack,setstackcommit,setheap,setheapcommit;
    int setsubsysmajor,setsubsysminor,setosmajor,setosminor;
    unsigned char setsubsys;
    int gotbase=FALSE,gotfalign=FALSE,gotoalign=FALSE,gotsubsys=FALSE;
    int gotstack=FALSE,gotstackcommit=FALSE,gotheap=FALSE,gotheapcommit=FALSE;
    int gotsubsysver=FALSE,gotosver=FALSE;
    char *p,*q;
    int c;
    char **newargs;
    FILE *argFile;

    for(i=1;i<argc;i++)
    {
	/* cater for response files */
	if(argv[i][0]=='@')
	{
	    argFile=fopen(argv[i]+1,"rt");
	    if(!argFile)
	    {
		printf("Unable to open response file \"%s\"\n",argv[i]+1);
		exit(1);
	    }
	    newargs=(char**)checkMalloc(argc*sizeof(char*));
	    for(j=0;j<argc;j++)
	    {
		newargs[j]=argv[j];
	    }
	    p=NULL;
	    j=0;
	    while((c=fgetc(argFile))!=EOF)
	    {
		if(c==';') /* allow comments, starting with ; */
		{
		    while(((c=fgetc(argFile))!=EOF) && (c!='\n')); /* loop until end of line */
		    /* continue main loop */
		    continue;
		}
		if(isspace(c))
		{
		    if(p) /* if we've got an argument, add to list */
		    {
			newargs=(char**)checkRealloc(newargs,(argc+1)*sizeof(char*));
			newargs[argc]=p;
			argc++;
			/* clear pointer and length indicator */
			p=NULL;
			j=0;
		    }
		    /* and continue */
		    continue;
		}
		if(c=='"')
		{
		    /* quoted strings */
		    while(((c=fgetc(argFile))!=EOF) && (c!='"')) /* loop until end of string */
		    {
			if(c=='\\')
			{
			    c=fgetc(argFile);
			    if(c==EOF)
			    {
				printf("Missing character to escape in quoted string, unexpected end of file found\n");
				exit(1);
			    }
			}
			    
			p=(char*)checkRealloc(p,j+2);
			p[j]=c;
			j++;
			p[j]=0;
		    }
		    if(c==EOF)
		    {
			printf("Unexpected end of file encountered in quoted string\n");
			exit(1);
		    }
		    
		    /* continue main loop */
		    continue;
		}
		/* if no special case, then add to string */
		p=(char*)checkRealloc(p,j+2);
		p[j]=c;
		j++;
		p[j]=0;
	    }
	    if(p)
	    {
		newargs=(char**)checkRealloc(newargs,(argc+1)*sizeof(char*));
		newargs[argc]=p;
		argc++;
	    }
	    fclose(argFile);
	    argv=newargs;
	}
	else if(argv[i][0]==SWITCHCHAR)
	{
	    if(strlen(argv[i])<2)
	    {
		printf("Invalid argument \"%s\"\n",argv[i]);
		exit(1);
	    }
	    switch(argv[i][1])
	    {
	    case 'c':
		if(strlen(argv[i])==2)
		{
		    case_sensitive=1;
		    break;
		}
		else if(strlen(argv[i])==3)
		{
		    if(argv[i][2]=='+')
		    {
			case_sensitive=1;
			break;
		    }
		    else if(argv[i][2]=='-')
		    {
			case_sensitive=0;
			break;
		    }
		}
		printf("Invalid switch %s\n",argv[i]);
		exit(1);
		break;
	    case 'p':
		switch(strlen(argv[i]))
		{
		case 2:
		    padsegments=1;
		    break;
		case 3:
		    if(argv[i][2]=='+')
		    {
			padsegments=1;
			break;
		    }
		    else if(argv[i][2]=='-')
		    {
			padsegments=0;
			break;
		    }
		default:
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 'm':
		switch(strlen(argv[i]))
		{
		case 2:
		    mapfile=1;
		    break;
		case 3:
		    if(argv[i][2]=='+')
		    {
			mapfile=1;
			break;
		    }
		    else if(argv[i][2]=='-')
		    {
			mapfile=0;
			break;
		    }
		default:
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 'o':
		switch(strlen(argv[i]))
		{
		case 2:
		    if(i<(argc-1))
		    {
			i++;
			if(!outname)
			{
			    outname=checkMalloc(strlen(argv[i])+1+4); /* space for added .EXT if none given */
			    strcpy(outname,argv[i]);
			}
			else
			{
			    printf("Can't specify two output names\n");
			    exit(1);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		default:
		    if(!strcmp(argv[i]+2,"EXE"))
		    {
			output_type=OUTPUT_EXE;
			imageBase=0;
			fileAlign=1;
			objectAlign=1;
			stackSize=0;
			stackCommitSize=0;
			heapSize=0;
			heapCommitSize=0;
		    }
		    else if(!strcmp(argv[i]+2,"COM"))
		    {
			output_type=OUTPUT_COM;
			imageBase=0;
			fileAlign=1;
			objectAlign=1;
			stackSize=0;
			stackCommitSize=0;
			heapSize=0;
			heapCommitSize=0;
		    }
		    else if(!strcmp(argv[i]+2,"PE"))
		    {
			output_type=OUTPUT_PE;
			imageBase=WIN32_DEFAULT_BASE;
			fileAlign=WIN32_DEFAULT_FILEALIGN;
			objectAlign=WIN32_DEFAULT_OBJECTALIGN;
			stackSize=WIN32_DEFAULT_STACKSIZE;
			stackCommitSize=WIN32_DEFAULT_STACKCOMMITSIZE;
			heapSize=WIN32_DEFAULT_HEAPSIZE;
			heapCommitSize=WIN32_DEFAULT_HEAPCOMMITSIZE;
			subSystem=WIN32_DEFAULT_SUBSYS;
			subsysMajor=WIN32_DEFAULT_SUBSYSMAJOR;
			subsysMinor=WIN32_DEFAULT_SUBSYSMINOR;
			osMajor=WIN32_DEFAULT_OSMAJOR;
			osMinor=WIN32_DEFAULT_OSMINOR;
		    }
		    else if(!strcmp(argv[i]+1,"objectalign"))
		    {
			if(i<(argc-1))
			{
			    i++;
			    setoalign=strtoul(argv[i],&p,0);
			    if(p[0]) /* if not at end of arg */
			    {
				printf("Bad object alignment\n");
				exit(1);
			    }
			    if((setoalign<512)|| (setoalign>(256*1048576))
			       || (getBitCount(setoalign)>1))
			    {
				printf("Bad object alignment\n");
				exit(1);
			    }
			    gotoalign=TRUE;
			}
			else
			{
			    printf("Invalid switch %s\n",argv[i]);
			    exit(1);
			}
		    }
		    else if(!strcmp(argv[i]+1,"osver"))
		    {
			if(i<(argc-1))
			{
			    i++;
			    if(sscanf(argv[i],"%d.%d%n",&setosmajor,&setosminor,&j)!=2)
			    {
				printf("Invalid version number %s\n",argv[i]);
				exit(1);
			    }
			    if((j!=strlen(argv[i])) || (setosmajor<0) || (setosminor<0)
			       || (setosmajor>65535) || (setosminor>65535))
			    {
				printf("Invalid version number %s\n",argv[i]);
				exit(1);
			    }
			    gotosver=TRUE;
			}
			else
			{
			    printf("Invalid switch %s\n",argv[i]);
			    exit(1);
			}
			break;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		break;
	    case 'L':
		if(strlen(argv[i])==2)
		{
		    if(i<(argc-1))
		    {
			i++;
			libPathCount++;
			libPath=(PCHAR*)checkRealloc(libPath,libPathCount*sizeof(PCHAR));
			j=strlen(argv[i]);
			if(argv[i][j-1]!=PATH_CHAR)
			{
				/* append a path separator if not present */
			    libPath[libPathCount-1]=(char*)checkMalloc(j+2);
			    strcpy(libPath[libPathCount-1],argv[i]);
			    libPath[libPathCount-1][j]=PATH_CHAR;
			    libPath[libPathCount-1][j+1]=0;
			}
			else
			{
			    libPath[libPathCount-1]=checkStrdup(argv[i]);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		printf("Invalid switch %s\n",argv[i]);
		exit(1);
		break;
	    case 'h':
	    case 'H':
	    case '?':
		if(strlen(argv[i])==2)
		{
		    helpRequested=TRUE;
		}
		else if(!strcmp(argv[i]+1,"heapsize"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setheap=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad heap size\n");
			    exit(1);
			}
			gotheap=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(argv[i]+1,"heapcommitsize"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setheapcommit=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad heap commit size\n");
			    exit(1);
			}
			gotheapcommit=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		break;
	    case 'b':
		if(!strcmp(argv[i]+1,"base"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setbase=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad image base\n");
			    exit(1);
			}
			if(setbase&0xffff)
			{
			    printf("Bad image base\n");
			    exit(1);
			}
			gotbase=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else
		{
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 's':
		if(!strcmp(argv[i]+1,"subsys"))
		{
		    if(i<(argc-1))
		    {
			i++;
			if(!strcmp(argv[i],"gui")
			   || !strcmp(argv[i],"windows")
			   || !strcmp(argv[i],"win"))
			{
			    setsubsys=PE_SUBSYS_WINDOWS;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(argv[i],"char")
				|| !strcmp(argv[i],"console")
				|| !strcmp(argv[i],"con"))
			{
			    setsubsys=PE_SUBSYS_CONSOLE;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(argv[i],"native"))
			{
			    setsubsys=PE_SUBSYS_NATIVE;
			    gotsubsys=TRUE;
			}
			else if(!strcmp(argv[i],"posix"))
			{
			    setsubsys=PE_SUBSYS_POSIX;
			    gotsubsys=TRUE;
			}
			else
			{
			    printf("Invalid subsystem id %s\n",argv[i]);
			    exit(1);
			}
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(argv[i]+1,"subsysver"))
		{
		    if(i<(argc-1))
		    {
			i++;
			if(sscanf(argv[i],"%d.%d%n",&setsubsysmajor,&setsubsysminor,&j)!=2)
			{
			    printf("Invalid version number %s\n",argv[i]);
			    exit(1);
			}
			if((j!=strlen(argv[i])) || (setsubsysmajor<0) || (setsubsysminor<0)
			   || (setsubsysmajor>65535) || (setsubsysminor>65535))
			{
			    printf("Invalid version number %s\n",argv[i]);
			    exit(1);
			}
			gotsubsysver=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(argv[i]+1,"stacksize"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setstack=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad stack size\n");
			    exit(1);
			}
			gotstack=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(argv[i]+1,"stackcommitsize"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setstackcommit=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad stack commit size\n");
			    exit(1);
			}
			gotstackcommit=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else if(!strcmp(argv[i]+1,"stub"))
		{
		    if(i<(argc-1))
		    {
			i++;
			stubName=argv[i];
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		    break;
		}
		else
		{
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 'f':
		if(!strcmp(argv[i]+1,"filealign"))
		{
		    if(i<(argc-1))
		    {
			i++;
			setfalign=strtoul(argv[i],&p,0);
			if(p[0]) /* if not at end of arg */
			{
			    printf("Bad file alignment\n");
			    exit(1);
			}
			if((setfalign<512)|| (setfalign>65536)
			   || (getBitCount(setfalign)>1))
			{
			    printf("Bad file alignment\n");
			    exit(1);
			}
			gotfalign=TRUE;
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		}
		else
		{
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 'd':
		if(!strcmp(argv[i]+1,"dll"))
		{
		    buildDll=TRUE;
		}
		else
		{
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
	    case 'e':
		if(!strcmp(argv[i]+1,"entry"))
		{
		    if(i<(argc-1))
		    {
			i++;
			entryPoint=argv[i];
		    }
		    else
		    {
			printf("Invalid switch %s\n",argv[i]);
			exit(1);
		    }
		}
		else
		{
		    printf("Invalid switch %s\n",argv[i]);
		    exit(1);
		}
		break;
		
	    default:
		printf("Invalid switch %s\n",argv[i]);
		exit(1);
	    }
	}
	else
	{
	    filename=checkRealloc(filename,(filecount+1)*sizeof(PCHAR));
	    filename[filecount]=checkMalloc(strlen(argv[i])+1);
	    memcpy(filename[filecount],argv[i],strlen(argv[i])+1);
	    for(j=strlen(filename[filecount]);
		j&&(filename[filecount][j]!='.')&&
		    (filename[filecount][j]!=PATH_CHAR);
		j--);
	    if((j<0) || (filename[filecount][j]!='.'))
	    {
		j=strlen(filename[filecount]);
		/* add default extension if none specified */
		filename[filecount]=checkRealloc(filename[filecount],strlen(argv[i])+5);
		strcpy(filename[filecount]+j,DEFAULT_EXTENSION);
	    }
	    filecount++;
	}
    }
    if(helpRequested || !filecount)
    {
	printf("Usage: ALINK [file [file [...]]] [options]\n");
	printf("\n");
	printf("    Each file may be an object file, a library, or a Win32 resource\n");
	printf("    file. If no extension is specified, .obj is assumed. Modules are\n");
	printf("    only loaded from library files if they are required to match an\n");
	printf("    external reference.\n");
	printf("    Options and files may be listed in any order, all mixed together.\n");
	printf("\n");
	printf("The following options are permitted:\n");
	printf("\n");
	printf("    @name   Load additional options from response file name\n");
	printf("    -c      Enable Case sensitivity\n");
	printf("    -c+     Enable Case sensitivity\n");
	printf("    -c-     Disable Case sensitivity\n");
	printf("    -p      Enable segment padding\n");
	printf("    -p+     Enable segment padding\n");
	printf("    -p-     Disable segment padding\n");
	printf("    -m      Enable map file\n");
	printf("    -m+     Enable map file\n");
	printf("    -m-     Disable map file\n");
	printf("----Press Enter to continue---");
	while(((c=getchar())!='\n') && (c!=EOF));
	printf("\n");
	printf("    -h      Display this help list\n");
	printf("    -H      \"\n");
	printf("    -?      \"\n");
	printf("    -L ddd  Add directory ddd to search list\n");
	printf("    -o name Choose output file name\n");
	printf("    -oXXX   Choose output format XXX\n");
	printf("        Available options are:\n");
	printf("            COM - MSDOS COM file\n");
	printf("            EXE - MSDOS EXE file\n");
	printf("            PE  - Win32 PE Executable\n");
	printf("    -entry name   Use public symbol name as the entry point\n");
	printf("----Press Enter to continue---");
	while(((c=getchar())!='\n') && (c!=EOF));
	printf("\nOptions for PE files:\n");
	printf("    -base addr        Set base address of image\n");
	printf("    -filealign addr   Set section alignment in file\n");
	printf("    -objectalign addr Set section alignment in memory\n");
	printf("    -subsys xxx       Set subsystem used\n");
	printf("        Available options are:\n");
	printf("            console   Select character mode\n");
	printf("            con       \"\n");
	printf("            char      \"\n");
	printf("            windows   Select windowing mode\n");
	printf("            win       \"\n");
	printf("            gui       \"\n");
	printf("            native    Select native mode\n");
	printf("            posix     Select POSIX mode\n");
	printf("    -subsysver x.y    Select subsystem version x.y\n");
	printf("    -osver x.y        Select OS version x.y\n");
	printf("    -stub xxx         Use xxx as the MSDOS stub\n");
	printf("    -dll              Build DLL instead of EXE\n");
	printf("    -stacksize xxx    Set stack size to xxx\n");
	printf("    -stackcommitsize xxx Set stack commit size to xxx\n");
	printf("    -heapsize xxx     Set heap size to xxx\n");
	printf("    -heapcommitsize xxx Set heap commit size to xxx\n");
	exit(0);
    }
    if((output_type!=OUTPUT_PE) &&
       (gotoalign || gotfalign || gotbase || gotsubsys || gotstack ||
	gotstackcommit || gotheap || gotheapcommit || buildDll || stubName || 
	gotsubsysver || gotosver))
    {
	printf("Option not supported for non-PE output formats\n");
	exit(1);
    }
    if(gotstack)
    {
	stackSize=setstack;
    }
    if(gotstackcommit)
    {
	stackCommitSize=setstackcommit;
    }
    if(stackCommitSize>stackSize)
    {
	printf("Stack commit size is greater than stack size, committing whole stack\n");
	stackCommitSize=stackSize;
    }
    if(gotheap)
    {
	heapSize=setheap;
    }
    if(gotheapcommit)
    {
	heapCommitSize=setheapcommit;
    }
    if(heapCommitSize>heapSize)
    {
	printf("Heap commit size is greater than heap size, committing whole heap\n");
	heapCommitSize=heapSize;
    }
    if(gotoalign)
    {
	objectAlign=setoalign;
    }
    if(gotfalign)
    {
	fileAlign=setfalign;
    }
    if(gotbase)
    {
	imageBase=setbase;
    }
    if(gotsubsys)
    {
	subSystem=setsubsys;
    }
    if(gotsubsysver)
    {
	subsysMajor=setsubsysmajor;
	subsysMinor=setsubsysminor;
    }
    if(gotosver)
    {
	osMajor=setosmajor;
	osMinor=setosminor;
    }
}

void matchExterns()
{
    long i,j,k,old_nummods;
    int n;
    PSORTENTRY listnode;
    PCHAR name;
    PPUBLIC pubdef;

    do
    {
	for(i=0;i<expcount;i++)
	{
	    if(expdefs[i].pubdef) continue;
	    if(listnode=binarySearch(publics,pubcount,expdefs[i].int_name))
	    {
		for(k=0;k<listnode->count;k++)
		{
		    /* exports can only match global publics */
		    if(((PPUBLIC)listnode->object[k])->modnum==0)
		    {
			expdefs[i].pubdef=(PPUBLIC)listnode->object[k];
			break;
		    }
		}
	    }
	}
	for(i=0;i<extcount;i++)
	{
	    /* skip if we've already matched a public symbol */
	    /* as they override all others */
	    if(externs[i].flags==EXT_MATCHEDPUBLIC) continue;
	    externs[i].flags=EXT_NOMATCH;
	    if(listnode=binarySearch(publics,pubcount,externs[i].name))
	    {
		for(k=0;k<listnode->count;k++)
		{
		    /* local publics can only match externs in same module */
		    /* and global publics can only match global externs */
		    if(((PPUBLIC)listnode->object[k])->modnum==externs[i].modnum)
		    {
			externs[i].pubdef=(PPUBLIC)listnode->object[k];
			externs[i].flags=EXT_MATCHEDPUBLIC;
			break;
		    }
		}
	    }
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(j=0;j<impcount;j++)
		{
		    if(!strcmp(externs[i].name,impdefs[j].int_name)
		       || ((case_sensitive==0) &&
			   !strcasecmp(externs[i].name,impdefs[j].int_name)))
		    {
			externs[i].flags=EXT_MATCHEDIMPORT;
			externs[i].impnum=j;
			impsreq++;
		    }
		}
	    }
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(j=0;j<expcount;j++)
		{
		    if(!expdefs[j].pubdef) continue;
		    if(!strcmp(externs[i].name,expdefs[j].exp_name)
		       || ((case_sensitive==0) &&
			   !strcasecmp(externs[i].name,expdefs[j].exp_name)))
		    {
			externs[i].pubdef=expdefs[j].pubdef;
			externs[i].flags=EXT_MATCHEDPUBLIC;
		    }
		}
	    }
	}

	old_nummods=nummods;
	for(i=0;(i<expcount)&&(nummods==old_nummods);i++)
	{
	    if(!expdefs[i].pubdef)
	    {
		for(k=0;k<libcount;++k)
		{
		    name=checkStrdup(expdefs[i].int_name);
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if(listnode=binarySearch(libfiles[k].symbols,libfiles[k].numsyms,name))
		    {
			loadlibmod(k,listnode->count);
			break;
		    }
		    free(name);
		}
	    }
	}
	for(i=0;(i<extcount)&&(nummods==old_nummods);i++)
	{
	    if(externs[i].flags==EXT_NOMATCH)
	    {
		for(k=0;k<libcount;++k)
		{
		    name=checkStrdup(externs[i].name);
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if(listnode=binarySearch(libfiles[k].symbols,libfiles[k].numsyms,name))
		    {
			loadlibmod(k,listnode->count);
			break;
		    }
		    free(name);
		}
	    }
	}
	for(i=0;(i<pubcount)&&(nummods==old_nummods);++i)
	{
	    for(k=0;k<publics[i].count;++k)
	    {
		pubdef=(PPUBLIC)publics[i].object[k];
		if(!pubdef->aliasName) continue;
		if(listnode=binarySearch(publics,pubcount,pubdef->aliasName))
		{
		    for(j=0;j<listnode->count;j++)
		    {
			if((((PPUBLIC)listnode->object[j])->modnum==pubdef->modnum)
			   && !((PPUBLIC)listnode->object[j])->aliasName)
			{
			    /* if we've found a match for the alias, then kill the alias */
			    free(pubdef->aliasName);
			    (*pubdef)=(*((PPUBLIC)listnode->object[j]));
			    break;
			}
		    }
		}
		if(!pubdef->aliasName) continue;
		for(k=0;k<libcount;++k)
		{
		    name=checkStrdup(pubdef->aliasName);
		    if(!(libfiles[k].flags&LIBF_CASESENSITIVE))
		    {
			strupr(name);
		    }
		
		    if(listnode=binarySearch(libfiles[k].symbols,libfiles[k].numsyms,name))
		    {
			loadlibmod(k,listnode->count);
			break;
		    }
		    free(name);
		}
		
	    }
	}
	
    } while (old_nummods!=nummods);
}

void matchComDefs()
{
    int i,j,k;
    int comseg;
    int comfarseg;
    PSORTENTRY listnode;
    PPUBLIC pubdef;

    if(!comcount) return;

    for(i=0;i<comcount;i++)
    {
	if(!comdefs[i]) continue;
	for(j=0;j<i;j++)
	{
	    if(!comdefs[j]) continue;
	    if(comdefs[i]->modnum!=comdefs[j]->modnum) continue;
	    if(strcmp(comdefs[i]->name,comdefs[j]->name)==0)
	    {
		if(comdefs[i]->isFar!=comdefs[j]->isFar)
		{
		    printf("Mismatched near/far type for COMDEF %s\n",comdefs[i]->name);
		    exit(1);
		}
		if(comdefs[i]->length>comdefs[j]->length)
		    comdefs[j]->length=comdefs[i]->length;
		free(comdefs[i]->name);
		free(comdefs[i]);
		comdefs[i]=0;
		break;
	    }
	}
    }

    for(i=0;i<comcount;i++)
    {
	if(!comdefs[i]) continue;
	if(listnode=binarySearch(publics,pubcount,comdefs[i]->name))
	{
	    for(j=0;j<listnode->count;j++)
	    {
		/* local publics can only match externs in same module */
		/* and global publics can only match global externs */
		if((((PPUBLIC)listnode->object[j])->modnum==comdefs[i]->modnum)
		   && !((PPUBLIC)listnode->object[j])->aliasName)
		{
		    free(comdefs[i]->name);
		    free(comdefs[i]);
		    comdefs[i]=0;
		    break;
		}
	    }
	}
    }

    seglist=(PPSEG)checkRealloc(seglist,(segcount+1)*sizeof(PSEG));
    seglist[segcount]=(PSEG)checkMalloc(sizeof(SEG));
    namelist=(PPCHAR)checkRealloc(namelist,(namecount+1)*sizeof(PCHAR));
    namelist[namecount]=checkStrdup("COMDEFS");
    seglist[segcount]->nameindex=namecount;
    seglist[segcount]->classindex=-1;
    seglist[segcount]->overlayindex=-1;
    seglist[segcount]->length=0;
    seglist[segcount]->data=NULL;
    seglist[segcount]->datmask=NULL;
    seglist[segcount]->attr=SEG_PRIVATE | SEG_PARA;
    seglist[segcount]->winFlags=WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
    comseg=segcount;
    segcount++;
    namecount++;


    for(i=0;i<grpcount;i++)
    {
	if(!grplist[i]) continue;
	if(grplist[i]->nameindex<0) continue;
	if(!strcmp("DGROUP",namelist[grplist[i]->nameindex]))
	{
	    if(grplist[i]->numsegs==0) continue; /* don't add to an emtpy group */
	    /* because empty groups are special */
	    /* else add to group */
	    grplist[i]->segindex[grplist[i]->numsegs]=comseg;
	    grplist[i]->numsegs++;
	    break;
	}
    }

    seglist=(PPSEG)checkRealloc(seglist,(segcount+1)*sizeof(PSEG));
    seglist[segcount]=(PSEG)checkMalloc(sizeof(SEG));
    namelist=(PPCHAR)checkRealloc(namelist,(namecount+1)*sizeof(PCHAR));
    namelist[namecount]=checkStrdup("FARCOMDEFS");
    seglist[segcount]->nameindex=namecount;
    seglist[segcount]->classindex=-1;
    seglist[segcount]->overlayindex=-1;
    seglist[segcount]->length=0;
    seglist[segcount]->data=NULL;
    seglist[segcount]->datmask=NULL;
    seglist[segcount]->attr=SEG_PRIVATE | SEG_PARA;
    seglist[segcount]->winFlags=WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
    namecount++;
    comfarseg=segcount;
    segcount++;

    for(i=0;i<comcount;i++)
    {
	if(!comdefs[i]) continue;
	pubdef=(PPUBLIC)checkMalloc(sizeof(PUBLIC));
	if(comdefs[i]->isFar)
	{
	    if(comdefs[i]->length>65536)
	    {
		seglist=(PPSEG)checkRealloc(seglist,(segcount+1)*sizeof(PSEG));
		seglist[segcount]=(PSEG)checkMalloc(sizeof(SEG));
		namelist=(PPCHAR)checkRealloc(namelist,(namecount+1)*sizeof(PCHAR));
		namelist[namecount]=checkStrdup("FARCOMDEFS");
		seglist[segcount]->nameindex=namecount;
		seglist[segcount]->classindex=-1;
		seglist[segcount]->overlayindex=-1;
		seglist[segcount]->length=comdefs[i]->length;
		seglist[segcount]->data=NULL;
		seglist[segcount]->datmask=
		    (PUCHAR)checkMalloc((comdefs[i]->length+7)/8);
		for(j=0;j<(comdefs[i]->length+7)/8;j++)
		    seglist[segcount]->datmask[j]=0;
		seglist[segcount]->attr=SEG_PRIVATE | SEG_PARA;
		seglist[segcount]->winFlags=WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
		namecount++;
		pubdef->segnum=segcount;
		segcount++;
		pubdef->ofs=0;
	    }
	    else if((comdefs[i]->length+seglist[comfarseg]->length)>65536)
	    {
		seglist[comfarseg]->datmask=
		    (PUCHAR)checkMalloc((seglist[comfarseg]->length+7)/8);
		for(j=0;j<(seglist[comfarseg]->length+7)/8;j++)
		    seglist[comfarseg]->datmask[j]=0;

		seglist=(PPSEG)checkRealloc(seglist,(segcount+1)*sizeof(PSEG));
		seglist[segcount]=(PSEG)checkMalloc(sizeof(SEG));
		namelist=(PPCHAR)checkRealloc(namelist,(namecount+1)*sizeof(PCHAR));
		namelist[namecount]=checkStrdup("FARCOMDEFS");
		seglist[segcount]->nameindex=namecount;
		seglist[segcount]->classindex=-1;
		seglist[segcount]->overlayindex=-1;
		seglist[segcount]->length=comdefs[i]->length;
		seglist[segcount]->data=NULL;
		seglist[segcount]->datmask=NULL;
		seglist[segcount]->attr=SEG_PRIVATE | SEG_PARA;
		seglist[segcount]->winFlags=WINF_READABLE |WINF_WRITEABLE | WINF_NEG_FLAGS;
		comfarseg=segcount;
		segcount++;
		namecount++;
		pubdef->segnum=comfarseg;
		pubdef->ofs=0;
	    }
	    else
	    {
		pubdef->segnum=comfarseg;
		pubdef->ofs=seglist[comfarseg]->length;
		seglist[comfarseg]->length+=comdefs[i]->length;
	    }
	}
	else
	{
	    pubdef->segnum=comseg;
	    pubdef->ofs=seglist[comseg]->length;
	    seglist[comseg]->length+=comdefs[i]->length;
	}
	pubdef->modnum=comdefs[i]->modnum;
	pubdef->grpnum=-1;
	pubdef->typenum=0;
	pubdef->aliasName=NULL;
	if(listnode=binarySearch(publics,pubcount,comdefs[i]->name))
	{
	    for(j=0;j<listnode->count;++j)
	    {
		if(((PPUBLIC)listnode->object[j])->modnum==pubdef->modnum)
		{
		    if(!((PPUBLIC)listnode->object[j])->aliasName)
		    {
			printf("Duplicate public symbol %s\n",comdefs[i]->name);
			exit(1);
		    }
		    free(((PPUBLIC)listnode->object[j])->aliasName);
		    (*((PPUBLIC)listnode->object[j]))=(*pubdef);
		    pubdef=NULL;
		    break;
		}
	    }
	}
	if(pubdef)
	{
	    sortedInsert(&publics,&pubcount,comdefs[i]->name,pubdef);
	}
    }
    seglist[comfarseg]->datmask=
	(PUCHAR)checkMalloc((seglist[comfarseg]->length+7)/8);
    for(j=0;j<(seglist[comfarseg]->length+7)/8;j++)
	seglist[comfarseg]->datmask[j]=0;


    seglist[comseg]->datmask=
	(PUCHAR)checkMalloc((seglist[comseg]->length+7)/8);
    for(j=0;j<(seglist[comseg]->length+7)/8;j++)
	seglist[comseg]->datmask[j]=0;


    for(i=0;i<expcount;i++)
    {
	if(expdefs[i].pubdef) continue;
	if(listnode=binarySearch(publics,pubcount,expdefs[i].int_name))
	{
	    for(j=0;j<listnode->count;j++)
	    {
		/* global publics only can match exports */
		if(((PPUBLIC)listnode->object[j])->modnum==0)
		{
		    expdefs[i].pubdef=(PPUBLIC)listnode->object[j];
		    break;
		}
	    }
	}
    }
    for(i=0;i<extcount;i++)
    {
	if(externs[i].flags!=EXT_NOMATCH) continue;
	if(listnode=binarySearch(publics,pubcount,externs[i].name))
	{
	    for(j=0;j<listnode->count;j++)
	    {
		/* global publics only can match exports */
		if(((PPUBLIC)(listnode->object[j]))->modnum==externs[i].modnum)
		{
		    externs[i].pubdef=(PPUBLIC)(listnode->object[j]);
		    externs[i].flags=EXT_MATCHEDPUBLIC;
		    break;
		}
	    }
	}
    }
}

void sortSegments()
{
    long i,j,k;
    UINT base,align;
    long baseSeg;

    for(i=0;i<segcount;i++)
    {
	if(seglist[i])
	{
	    if((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS)
	    {
		seglist[i]->absframe=0;
	    }
	}
    }

    outcount=0;
    base=0;
    outlist=checkMalloc(sizeof(PSEG)*segcount);
    for(i=0;i<grpcount;i++)
    {
	if(grplist[i])
	{
	    grplist[i]->segnum=-1;
	    for(j=0;j<grplist[i]->numsegs;j++)
	    {
		k=grplist[i]->segindex[j];
		if(!seglist[k])
		{
		    printf("Error - group %s contains non-existent segment\n",namelist[grplist[i]->nameindex]);
		    exit(1);
		}
		/* don't add removed sections */
		if(seglist[k]->winFlags & WINF_REMOVE)
		{
		    continue;
		}
		/* add non-absolute segment */
		if((seglist[k]->attr&SEG_ALIGN)!=SEG_ABS)
		{
		    switch(seglist[k]->attr&SEG_ALIGN)
		    {
		    case SEG_WORD:
			align=2;
			break;
		    case SEG_DWORD:
			align=4;
			break;
		    case SEG_8BYTE:
			align=0x8;
			break;
		    case SEG_PARA:
			align=0x10;
			break;
		    case SEG_32BYTE:
			align=0x20;
			break;
		    case SEG_64BYTE:
			align=0x40;
			break;
		    case SEG_PAGE:
			align=0x100;
			break;
		    case SEG_MEMPAGE:
			align=0x1000;
			break;
		    case SEG_BYTE:
		    default:
			align=1;
			break;
		    }
		    if(align<objectAlign)
		    {
			align=objectAlign;
		    }
		    base=(base+align-1)&(0xffffffff-(align-1));
		    seglist[k]->base=base;
		    if(seglist[k]->length>0)
		    {
			base+=seglist[k]->length;
			if(seglist[k]->absframe!=0)
			{
			    printf("Error - Segment %s part of more than one group\n",namelist[seglist[k]->nameindex]);
			    exit(1);
			}
			seglist[k]->absframe=1;
			seglist[k]->absofs=i+1;
			if(grplist[i]->segnum<0)
			{
			    grplist[i]->segnum=k;
			}
			if(outcount==0)
			{
			    baseSeg=k;
			}
			else
			{
			    outlist[outcount-1]->virtualSize=seglist[k]->base-
				outlist[outcount-1]->base;
			}
			outlist[outcount]=seglist[k];
			outcount++;
		    }
		}
	    }
	}
    }
    for(i=0;i<segcount;i++)
    {
	if(seglist[i])
	{
	    /* don't add removed sections */
	    if(seglist[i]->winFlags & WINF_REMOVE)
	    {
		continue;
	    }
	    /* add non-absolute segment, not already dealt with */
	    if(((seglist[i]->attr&SEG_ALIGN)!=SEG_ABS) &&
	       !seglist[i]->absframe)
	    {
		switch(seglist[i]->attr&SEG_ALIGN)
		{
		case SEG_WORD:
		case SEG_BYTE:
		case SEG_DWORD:
		case SEG_PARA:
		    align=0x10;
		    break;
		case SEG_PAGE:
		    align=0x100;
		    break;
		case SEG_MEMPAGE:
		    align=0x1000;
		    break;
		default:
		    align=1;
		    break;
		}
		if(align<objectAlign)
		{
		    align=objectAlign;
		}
		base=(base+align-1)&(0xffffffff-(align-1));
		seglist[i]->base=base;
		if(seglist[i]->length>0)
		{
		    base+=seglist[i]->length;
		    seglist[i]->absframe=1;
		    seglist[i]->absofs=0;
		    if(outcount==0)
		    {
			baseSeg=i;
		    }
		    else
		    {
			outlist[outcount-1]->virtualSize=seglist[i]->base-
			    outlist[outcount-1]->base;
		    }
		    outlist[outcount]=seglist[i];
		    outcount++;
		}
	    }
	    else if((seglist[i]->attr&SEG_ALIGN)==SEG_ABS)
	    {
		seglist[i]->base=(seglist[i]->absframe<<4)+seglist[i]->absofs;
	    }
	}
    }
    /* build size of last segment in output list */
    if(outcount)
    {
	outlist[outcount-1]->virtualSize=
	    (outlist[outcount-1]->length+objectAlign-1)&
	    (0xffffffff-(objectAlign-1));
    }
    for(i=0;i<grpcount;i++)
    {
	if(grplist[i] && (grplist[i]->segnum<0)) grplist[i]->segnum=baseSeg;
    }
}

void loadFiles()
{
    long i,j,k;
    char *name;

    for(i=0;i<filecount;i++)
    {
	afile=fopen(filename[i],"rb");
	if(!strchr(filename[i],PATH_CHAR))
	{
	    /* if no path specified, search library path list */
	    for(j=0;!afile && j<libPathCount;j++)
	    {
		name=(char*)checkMalloc(strlen(libPath[j])+strlen(filename[i])+1);
		strcpy(name,libPath[j]);
		strcat(name,filename[i]);
		afile=fopen(name,"rb");
		if(afile)
		{
		    free(filename[i]);
		    filename[i]=name;
		    name=NULL;
		}
		else
		{
		    free(name);
		    name=NULL;
		}
	    }
	}
	if(!afile)
	{
	    printf("Error opening file %s\n",filename[i]);
	    exit(1);
	}
	for(k=0;k<i;++k)
	{
	    if(!strcmp(filename[i],filename[k])) break;
	}
	if(k!=i)
	{
	    fclose(afile);
	    continue;
	}
	
	filepos=0;
	printf("Loading file %s\n",filename[i]);
	j=fgetc(afile);
	fseek(afile,0,SEEK_SET);
	switch(j)
	{
	case LIBHDR:
	    loadlib(afile,filename[i]);
	    break;
	case THEADR:
	case LHEADR:
	    loadmod(afile);
	    break;
	case 0:
	    loadres(afile);
	    break;
	case 0x4c:
	case 0x4d:
	case 0x4e:
	    loadcoff(afile);
	    break;
	case 0x21:
	    loadCoffLib(afile,filename[i]);
	    break;
	default:
	    printf("Unknown file type\n");
	    fclose(afile);
	    exit(1);
	}
	fclose(afile);
    }
}

void generateMap()
{
    long i,j;
    PPUBLIC q;

    afile=fopen(mapname,"wt");
    if(!afile)
    {
	printf("Error opening map file %s\n",mapname);
	exit(1);
    }
    printf("Generating map file %s\n",mapname);

    for(i=0;i<segcount;i++)
    {
	if(seglist[i])
	{
	    fprintf(afile,"SEGMENT %s ",
		    (seglist[i]->nameindex>=0)?namelist[seglist[i]->nameindex]:"");
	    switch(seglist[i]->attr&SEG_COMBINE)
	    {
	    case SEG_PRIVATE:
		fprintf(afile,"PRIVATE ");
		break;
	    case SEG_PUBLIC:
		fprintf(afile,"PUBLIC ");
		break;
	    case SEG_PUBLIC2:
		fprintf(afile,"PUBLIC(2) ");
		break;
	    case SEG_STACK:
		fprintf(afile,"STACK ");
		break;
	    case SEG_COMMON:
		fprintf(afile,"COMMON ");
		break;
	    case SEG_PUBLIC3:
		fprintf(afile,"PUBLIC(3) ");
		break;
	    default:
		fprintf(afile,"unknown ");
		break;
	    }
	    if(seglist[i]->attr&SEG_USE32)
	    {
		fprintf(afile,"USE32 ");
	    }
	    else
	    {
		fprintf(afile,"USE16 ");
	    }
	    switch(seglist[i]->attr&SEG_ALIGN)
	    {
	    case SEG_ABS:
		fprintf(afile,"AT 0%04lXh ",seglist[i]->absframe);
		break;
	    case SEG_BYTE:
		fprintf(afile,"BYTE ");
		break;
	    case SEG_WORD:
		fprintf(afile,"WORD ");
		break;
	    case SEG_PARA:
		fprintf(afile,"PARA ");
		break;
	    case SEG_PAGE:
		fprintf(afile,"PAGE ");
		break;
	    case SEG_DWORD:
		fprintf(afile,"DWORD ");
		break;
	    case SEG_MEMPAGE:
		fprintf(afile,"MEMPAGE ");
		break;
	    default:
		fprintf(afile,"unknown ");
	    }
	    if(seglist[i]->classindex>=0)
		fprintf(afile,"'%s'\n",namelist[seglist[i]->classindex]);
	    else
		fprintf(afile,"\n");
	    fprintf(afile,"  at %08lX, length %08lX\n",seglist[i]->base,seglist[i]->length);
	}
    }
    for(i=0;i<grpcount;i++)
    {
	if(!grplist[i]) continue;
	fprintf(afile,"\nGroup %s:\n",namelist[grplist[i]->nameindex]);
	for(j=0;j<grplist[i]->numsegs;j++)
	{
	    fprintf(afile,"    %s\n",namelist[seglist[grplist[i]->segindex[j]]->nameindex]);
	}
    }

    if(pubcount)
    {
	fprintf(afile,"\npublics:\n");
    }
    for(i=0;i<pubcount;++i)
    {
	for(j=0;j<publics[i].count;++j)
	{
	    q=(PPUBLIC)publics[i].object[j];
	    if(q->modnum) continue;
	    fprintf(afile,"%s at %s:%08lX\n",
		    publics[i].id,
		    (q->segnum>=0) ? namelist[seglist[q->segnum]->nameindex] : "Absolute",
		    q->ofs);
	}
    }
    
    if(expcount)
    {
	fprintf(afile,"\n %li exports:\n",expcount);
	for(i=0;i<expcount;i++)
	{
	    fprintf(afile,"%s(%i)=%s\n",expdefs[i].exp_name,expdefs[i].ordinal,expdefs[i].int_name);
	}
    }
    if(impcount)
    {
	fprintf(afile,"\n %li imports:\n",impcount);
	for(i=0;i<impcount;i++)
	{
	    fprintf(afile,"%s=%s:%s(%i)\n",impdefs[i].int_name,impdefs[i].mod_name,impdefs[i].flags==0?impdefs[i].imp_name:"",
		    impdefs[i].flags==0?0:impdefs[i].ordinal);
	}
    }
    fclose(afile);
}

int main(int argc,char *argv[])
{
    long i,j;
    int isend;
    char *libList;
    PPUBLIC q;

    printf("ALINK v1.6 (C) Copyright 1998-9 Anthony A.J. Williams.\n");
    printf("All Rights Reserved\n\n");

    libList=getenv("LIB");
    if(libList)
    {
	for(i=0,j=0;;i++)
	{
	    isend=(!libList[i]);
	    if(libList[i]==';' || !libList[i])
	    {
		if(i-j)
		{
		    libPath=(PCHAR*)checkRealloc(libPath,(libPathCount+1)*sizeof(PCHAR));
		    libList[i]=0;
		    if(libList[i-1]==PATH_CHAR)
		    {
			libPath[libPathCount]=checkStrdup(libList+j);
		    }
		    else
		    {
			libPath[libPathCount]=(PCHAR)checkMalloc(i-j+2);
			strcpy(libPath[libPathCount],libList+j);
			libPath[libPathCount][i-j]=PATH_CHAR;
			libPath[libPathCount][i-j+1]=0;
		    }
		    libPathCount++;
		}
		j=i+1;
	    }
	    if(isend) break;
	}
    }

    processArgs(argc,argv);

    if(!filecount)
    {
	printf("No files specified\n");
	exit(1);
    }

    if(!outname)
    {
	outname=checkMalloc(strlen(filename[0])+1+4);
	strcpy(outname,filename[0]);
	i=strlen(outname);
	while((i>=0)&&(outname[i]!='.')&&(outname[i]!=PATH_CHAR)&&(outname[i]!=':'))
	{
	    i--;
	}
	if(outname[i]=='.')
	{
	    outname[i]=0;
	}
    }
    i=strlen(outname);
    while((i>=0)&&(outname[i]!='.')&&(outname[i]!=PATH_CHAR)&&(outname[i]!=':'))
    {
	i--;
    }
    if(outname[i]!='.')
    {
	switch(output_type)
	{
	case OUTPUT_EXE:
	case OUTPUT_PE:
	    if(!buildDll)
	    {
		strcat(outname,".exe");
	    }
	    else
	    {
		strcat(outname,".dll");
	    }
	    break;
	case OUTPUT_COM:
	    strcat(outname,".com");
	    break;
	default:
	    break;
	}
    }

    if(mapfile)
    {
	if(!mapname)
	{
	    mapname=checkMalloc(strlen(outname)+1+4);
	    strcpy(mapname,outname);
	    i=strlen(mapname);
	    while((i>=0)&&(mapname[i]!='.')&&(mapname[i]!=PATH_CHAR)&&(mapname[i]!=':'))
	    {
		i--;
	    }
	    if(mapname[i]!='.')
	    {
		i=strlen(mapname);
	    }
	    strcpy(mapname+i,".map");
	}
    }
    else
    {
	if(mapname)
	{
	    free(mapname);
	    mapname=0;
	}
    }

    loadFiles();

    if(!nummods)
    {
	printf("No required modules specified\n");
	exit(1);
    }

    if(rescount && (output_type!=OUTPUT_PE))
    {
	printf("Cannot link resources into a non-PE application\n");
	exit(1);
    }

    if(entryPoint)
    {
	if(!case_sensitive)
	{
	    strupr(entryPoint);
	}
	
	if(gotstart)
	{
	    printf("Warning, overriding entry point from Command Line\n");
	}
	/* define an external reference for entry point */
	externs=checkRealloc(externs,(extcount+1)*sizeof(EXTREC));
	externs[extcount].name=entryPoint;
	externs[extcount].typenum=-1;
	externs[extcount].pubdef=NULL;
	externs[extcount].flags=EXT_NOMATCH;
	externs[extcount].modnum=0;

	/* point start address to this external */
	startaddr.ftype=REL_EXTDISP;
	startaddr.frame=extcount;
	startaddr.ttype=REL_EXTONLY;
	startaddr.target=extcount;

	extcount++;
	gotstart=TRUE;
    }

    matchExterns();
    printf("matched Externs\n");
    
    matchComDefs();
    printf("matched ComDefs\n");

    for(i=0;i<expcount;i++)
    {
	if(!expdefs[i].pubdef)
	{
	    printf("Unresolved export %s=%s\n",expdefs[i].exp_name,expdefs[i].int_name);
	    errcount++;
	}
	else if(expdefs[i].pubdef->aliasName)
	{
	    printf("Unresolved export %s=%s, with alias %s\n",expdefs[i].exp_name,expdefs[i].int_name,expdefs[i].pubdef->aliasName);
	    errcount++;
	}
	
    }

    for(i=0;i<extcount;i++)
    {
	if(externs[i].flags==EXT_NOMATCH)
	{
	    printf("Unresolved external %s\n",externs[i].name);
	    errcount++;
	}
	else if(externs[i].flags==EXT_MATCHEDPUBLIC)
	{
	    if(externs[i].pubdef->aliasName)
	    {
		printf("Unresolved external %s with alias %s\n",externs[i].name,externs[i].pubdef->aliasName);
		errcount++;
	    }
	}
    }

    if(errcount!=0)
    {
	exit(1);
    }

    combineBlocks();
    sortSegments();

    if(mapfile) generateMap();
    switch(output_type)
    {
    case OUTPUT_COM:
	OutputCOMfile(outname);
	break;
    case OUTPUT_EXE:
	OutputEXEfile(outname);
	break;
    case OUTPUT_PE:
	OutputWin32file(outname);
	break;
    default:
	printf("Invalid output type\n");
	exit(1);
	break;
    }
    return 0;
}
