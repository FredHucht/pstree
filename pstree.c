/*	This is pstree written by Fred Hucht (c) 1993-2003	*
 *	EMail: fred AT thp.Uni-Duisburg.de			*
 *	Feel free to copy and redistribute in terms of the	*
 * 	GNU public license. 					*
 *
 * $Id: pstree.c,v 2.21 2003-10-06 13:55:47+02 fred Exp fred $
 */
static char *WhatString[]= {
  "@(#)pstree $Revision: 2.21 $ by Fred Hucht (C) 1993-2003",
  "@(#)EMail: fred AT thp.Uni-Duisburg.de",
  "$Id: pstree.c,v 2.21 2003-10-06 13:55:47+02 fred Exp fred $"
};

#define MAXLINE 512

#if defined(_AIX) || defined(___AIX)	/* AIX >= 3.1 */
/* Under AIX, we directly read the process table from the kernel */
# ifndef _AIX50
/* problems with getprocs() under AIX 5L
 * workaround contributed by Chris Benesch <chris AT fdbs.com> */
#  define USE_GetProcessesDirect
# endif /*_AIX50*/
#  define HAS_TERMDEF
extern char *termdef(int, char);
#  define _ALL_SOURCE
#  include <procinfo.h>
#  define USE_GETPROCS

#  ifdef USE_GETPROCS
#    define IFNEW(a,b) a
#    define ProcInfo procsinfo
extern getprocs(struct procsinfo *, int, struct fdsinfo *, int, pid_t *, int);
#  else /*USE_GETPROCS*/
#    define IFNEW(a,b) b
#    define ProcInfo procinfo
extern getproc(struct procinfo *, int, int);
extern getuser(struct procinfo *, int, void *, int);
#  endif /*USE_GETPROCS*/

extern getargs(struct ProcInfo *, int, char *, int);

/*#  define PSCMD 	"ps -ekf"
  #  define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"*/
#  define HAS_PGID
#  define UID2USER
#  define PSCMD 	"ps -eko uid,pid,ppid,pgid,thcount,args"
#  define PSFORMAT 	"%ld %ld %ld %ld %ld %[^\n]"
#  define PSVARS	&P[i].uid, &P[i].pid, &P[i].ppid, &P[i].pgid, &P[i].thcount, P[i].cmd
/************************************************************************/
#elif defined(__linux)	/* Linux */
#  define USE_GetProcessesDirect
#  define HAS_PGID
#  include <glob.h>
#  include <sys/stat.h>
#  define UID2USER
#  define PSCMD 	"ps -eo uid,pid,ppid,pgid,args"
#  define PSFORMAT 	"%ld %ld %ld %ld %[^\n]"
#  define PSVARS	&P[i].uid, &P[i].pid, &P[i].ppid, &P[i].pgid, P[i].cmd
/************************************************************************/
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || (defined __alpha && defined(_SYSTYPE_BSD))
/* NetBSD contributed by Gary D. Duzan <gary AT wheel.tiac.net>
 * FreeBSD contributed by Randall Hopper <rhh AT ct.picker.com> 
 * Darwin / Mac OS X patch by Yuji Yamano <yyamano AT kt.rim.or.jp>
 * wide output format fix for NetBSD by Jeff Brown <jabrown AT caida.org>
 * (Net|Open|Free)BSD & Darwin merged by Ralf Meyer <ralf AT thp.Uni-Duisburg.DE>
 * TRU64 contributed by Frank Parkin <fparki AT acxiom.co.uk>
 */
#  define HAS_PGID
#  define PSCMD 	"ps -axwwo user,pid,ppid,pgid,command"
#  define PSFORMAT 	"%s %ld %ld %ld %[^\n]"
#  define PSVARS	P[i].name, &P[i].pid, &P[i].ppid, &P[i].pgid, P[i].cmd
/************************************************************************/
#elif defined(sun) && (!defined(__SVR4)) /* Solaris 1.x */
/* contributed by L. Mark Larsen <mlarsen AT ptdcs2.intel.com> */
/* new cpp criteria by Pierre Belanger <belanger AT risq.qc.ca> */
#  define solaris1x
#  define UID2USER
#  ifdef mc68000
/* contributed by Paul Kern <pkern AT utcc.utoronto.ca> */
#    define PSCMD 	"ps laxw"
#    define PSFORMAT 	"%*7c%ld %ld %ld %*d %*d %*d %*x %*d %*d %*x %*14c %[^\n]"
#    define uid_t	int
#    define NEED_STRSTR
#  else
#    define PSCMD 	"ps jaxw"
#    define PSFORMAT 	"%ld %ld %*d %*d %*s %*d %*s %ld %*s %[^\n]"
#    define PSVARS 	&P[i].ppid, &P[i].pid, &P[i].uid, P[i].cmd
#  endif
/************************************************************************/
#elif defined(sun) && (defined(__SVR4)) /* Solaris 2.x */
/* contributed by Pierre Belanger <belanger AT risq.qc.ca> */
#  define solaris2x
#  define PSCMD         "ps -ef"
#  define PSFORMAT      "%s %ld %ld %*d %*s %*s %*s %[^\n]"
/************************************************************************/
#elif defined(bsdi)
/* contributed by Dean Gaudet <dgaudet AT hotwired.com> */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%ld %ld %ld %*d %*d %*d %*d %*d %*s %*s %*s %*s %[^\n]"
/************************************************************************/
#elif defined(_BSD)	/* Untested */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*d %*c %ld %ld %ld %*d %*d %*d %*x %*d %d %*15c %*s %[^\n]"
/************************************************************************/
#elif defined(__convex)	/* ConvexOS */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*s %ld %ld %ld %*d %*g %*d %*d %*21c %*s %[^\n]"
/************************************************************************/
#else			/* HP-UX, A/UX etc. */
#  define PSCMD 	"ps -ef"
#  define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"
#endif
/*********************** end of configurable part ***********************/

#ifndef PSVARS 		/* Set default */
# ifdef UID2USER
#  define PSVARS	&P[i].uid, &P[i].pid, &P[i].ppid, P[i].cmd
# else
#  define PSVARS	P[i].name, &P[i].pid, &P[i].ppid, P[i].cmd
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* For str...() */
#include <unistd.h>		/* For getopt() */
#include <pwd.h>		/* For getpwnam() */

#ifdef DEBUG
# include <errno.h>
#endif

#ifdef NEED_STRSTR
static char *strstr(char *, char *);
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

struct TreeChars {
  char *s2, 		/* SS String between header and pid */
    *p, 		/* PP dito, when parent of printed childs */
    *pgl,		/* G  Process group leader */
    *npgl,		/* N  No process group leader */
    *barc, 		/* C  bar for line with child */
    *bar, 		/* B  bar for line without child */
    *barl,		/* L  bar for last child */
    *sg,		/*    Start graphics (alt char set) */
    *eg,		/*    End graphics (alt char set) */
    *init;		/*    Init string sent at the beginning */
};

/* Example:
 * |-+- 01111 ...        CPPN 01111 ...
 * | \-+=   01112 ...    B LPPG 01112 ...
 * |   |--= 01113 ...    B   CSSG 01113 ...
 * |   \--= 01114 ...    B   LSSG 01114 ...
 * \-+- 01115 ...        LSSN 01115 ...
 */

enum { G_ASCII = 0, G_PC850 = 1, G_VT100 = 2, G_LAST };

/* VT sequences contributed by Randall Hopper <rhh AT ct.picker.com> */
static struct TreeChars TreeChars[] = {
  /* SS          PP          G       N       C       B       L      sg      eg      init */
  { "--",       "-+",       "=",    "-",    "|",    "|",    "\\",   "",     "",     ""             }, /*Ascii*/
  { "\304\304", "\304\302", "\372", "\304", "\303", "\263", "\300", "",     "",     ""             }, /*Pc850*/
  { "qq",       "qw",       "`",    "q",    "t",    "x",    "m",    "\016", "\017", "\033(B\033)0" }  /*Vt100*/
}, *C;

int MyPid, NProc, Columns, RootPid;
short showall = TRUE, soption = FALSE, Uoption = FALSE;
char *name = "", *str = NULL, *Progname;
long ipid = -1;
char *input = NULL;

#ifdef DEBUG
int debug = FALSE;
#endif

struct Proc {
  long uid, pid, ppid, pgid;
  char name[9], cmd[MAXLINE];
  int  print;
  long parent, child, sister;
  unsigned long thcount;
} *P;

#ifdef UID2USER
void uid2user(uid_t uid, char *name, int len) {
#define NUMUN 128
  static struct un_ {
    uid_t uid;
    char name[9];
  } un[NUMUN];
  static short n = 0;
  short i;
  char *found;
#ifdef DEBUG
  if (name == NULL) {
    for (i = 0; i < n; i++)
      fprintf(stderr, "uid = %3d, name = %s\n", un[i].uid, un[i].name);
    return;
  }
#endif
  for (i = n - 1; i >= 0 && un[i].uid != uid; i--);
  if (i >= 0) { /* found locally */
    found = un[i].name;
  } else {
    struct passwd *pw = getpwuid(uid);
    found = pw->pw_name;
    if (n < NUMUN) {
      un[n].uid = uid;
      strncpy(un[n].name, found, 9);
      un[n].name[8] = '\0';
      n++;
    }
  }
  strncpy(name, found, len);
  name[len-1] = '\0';
}
#endif

#if defined(_AIX) || defined(___AIX)	/* AIX 3.x / 4.x */
int GetProcessesDirect(void) {
  int i, nproc, maxnproc = 1024;
  
  struct ProcInfo *proc;
  int idx;
#ifndef USE_GETPROCS
  struct userinfo user;
#endif
  
  do {
    proc = malloc(maxnproc * sizeof(struct ProcInfo));
    if (proc == NULL) {
      fprintf(stderr, "Problems with malloc.\n");
      exit(1);
    }
    
    /* Get process table */
    idx = 0;
    nproc = IFNEW(getprocs(proc, sizeof(struct procsinfo), NULL, 0,
			   &idx, maxnproc),
		  getproc(proc, maxnproc, sizeof(struct procinfo))
		  );
#ifdef DEBUG
    idx = errno; /* Don't ask... */
    if (debug)
      fprintf(stderr,
	      "nproc = %d maxnproc = %d" IFNEW(" idx = %d ","") "\n",
	      nproc, maxnproc, idx);
    errno = idx;
#endif
#ifdef USE_GETPROCS
    if (nproc == -1) {
      perror("getprocs");
      exit(1);
    } else if (nproc == maxnproc) {
      nproc = -1;
    }
#endif
    if (nproc == -1) {
      free(proc);
      maxnproc *= 2;
    } 
  } while (nproc == -1);
  
  P = malloc((nproc+1) * sizeof(struct Proc));
  if (P == NULL) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  for (i = 0; i < nproc; i++) {
#ifndef USE_GETPROCS
    getuser(&proc[i],sizeof(struct procinfo),
	    &user,   sizeof(struct userinfo));
#endif
    P[i].uid     = proc[i].pi_uid;
    P[i].pid     = proc[i].pi_pid;
    P[i].ppid    = proc[i].pi_ppid;
    P[i].pgid    = proc[i].pi_pgrp;
    P[i].thcount = IFNEW(proc[i].pi_thcount, 1);
    
    uid2user(P[i].uid, P[i].name, sizeof(P[i].name));
    
    if (IFNEW(proc[i].pi_state,proc[i].pi_stat) == SZOMB) {
      strcpy(P[i].cmd, "<defunct>");
    } else {
      char *c = P[i].cmd;
      int ci = 0;
      getargs(&proc[i], sizeof(struct procinfo), c, MAXLINE - 2);
      c[MAXLINE-2] = c[MAXLINE-1] = '\0';

      /* Collect args. Stop when we encounter two '\0' */
      while (c[ci] != '\0' && (ci += strlen(&c[ci])) < MAXLINE - 2)
	c[ci++] = ' ';
      
      /* Drop trailing blanks */
      ci = strlen(c);
      while (ci > 0 && c[ci-1] == ' ') ci--;
      c[ci] = '\0';
      
      /* Replace some unprintables with '?' */
      for (ci = 0; c[ci] != '\0'; ci++)
	if (c[ci] == '\n' || c[ci] == '\t') c[ci] = '?';
      
      /* Insert [ui_comm] when getargs returns nothing */
      if (c[0] == '\0') {
	int l = strlen(IFNEW(proc[i].pi_comm,user.ui_comm));
	c[0] = '[';
	strcpy(c+1, IFNEW(proc[i].pi_comm,user.ui_comm));
	c[l+1] = ']';
	c[l+2] = '\0';
      }
    }
#ifdef DEBUG
    if (debug)
      fprintf(stderr,
	      "%d: uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, pgid=%5ld, tsize=%7u, dvm=%4u, "
	      "thcount=%2d, cmd[%d]='%s'\n",
	      i, P[i].uid, P[i].name, P[i].pid, P[i].ppid, P[i].pgid,
	      IFNEW(proc[i].pi_tsize,user.ui_tsize),
	      IFNEW(proc[i].pi_dvm,user.ui_dvm),
	      proc[i].pi_thcount,
	      strlen(P[i].cmd),P[i].cmd);
#endif
    P[i].parent = P[i].child = P[i].sister = -1;
    P[i].print = FALSE;
  }
  free(proc);
  return nproc;
}

#endif /* _AIX */

#ifdef __linux
int GetProcessesDirect(void) {
  glob_t globbuf;
  int i, j;
  
  glob("/proc/[0-9]*", GLOB_NOSORT, NULL, &globbuf);
  
  P = calloc(globbuf.gl_pathc, sizeof(struct Proc));
  if (P == NULL) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  for (i = j = 0; i < globbuf.gl_pathc; i++) {
    char name[32], c;
    FILE *tn;
    struct stat stat;
    int k = 0;
    
    sprintf(name, "%s%s",
	    globbuf.gl_pathv[globbuf.gl_pathc - i - 1], "/stat");
    tn = fopen(name, "r");
    if (tn == NULL) continue; /* process vanished since glob() */
    fscanf(tn, "%ld %s %*c %ld %ld",
	   &P[j].pid, P[j].cmd, &P[j].ppid, &P[j].pgid);
    fstat(fileno(tn), &stat);
    P[j].uid = stat.st_uid;
    fclose(tn);
    P[j].thcount = 1;
    
    sprintf(name, "%s%s",
	    globbuf.gl_pathv[globbuf.gl_pathc - i - 1], "/cmdline");
    tn = fopen(name, "r");
    if (tn == NULL) continue;
    while (k < MAXLINE - 1 && EOF != (c = fgetc(tn))) {
      P[j].cmd[k++] = c == '\0' ? ' ' : c;
    }
    if (k > 0) P[j].cmd[k] = '\0';
    fclose(tn);
    
    uid2user(P[j].uid, P[j].name, sizeof(P[j].name));
    
#ifdef DEBUG
    if (debug) fprintf(stderr,
		       "uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, pgid=%5ld, thcount=%ld, cmd='%s'\n",
		       P[j].uid, P[j].name, P[j].pid, P[j].ppid, P[j].pgid, P[j].thcount, P[j].cmd);
#endif
    P[j].parent = P[j].child = P[j].sister = -1;
    P[j].print  = FALSE;
    j++;
  }
  globfree(&globbuf);
  return j;
}
#endif /* __linux */

int GetProcesses(void) {
  FILE *tn;
  int len, i = 0;
  char line[MAXLINE], command[] = PSCMD;
  
  /* file read code contributed by Paul Kern <pkern AT utcc.utoronto.ca> */
  if (input != NULL) {
    if (strcmp(input, "-") == 0)
      tn = stdin;
    else if (NULL == (tn = fopen(input,"r"))) {
      perror(input);
      exit(1);
    }
  } else {
#ifdef DEBUG
    if (debug) fprintf(stderr, "calling '%s'\n", command);
#endif
    if (NULL == (tn = (FILE*)popen(command,"r"))) {
      perror("Problems with pipe");
      exit(1);
    }
  }
#ifdef DEBUG
  if (debug) fprintf(stderr, "popen:errno = %d\n", errno);
#endif
  
  if (NULL == fgets(line, MAXLINE, tn)) { /* Throw away header line */
    fprintf(stderr, "No input.\n");
    exit(1);
  }
  
#ifdef DEBUG
  if (debug) fputs(line, stderr);
#endif
  
  P = malloc(sizeof(struct Proc));
  if (P == NULL) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  while (NULL != fgets(line, MAXLINE, tn) && 10 < (len = strlen(line))) {
#ifdef DEBUG
    if (debug) {
      fprintf(stderr, "len=%3d ", len);
      fputs(line, stderr);
    }
#endif
    
    if (len == MAXLINE - 1) { /* line too long, drop remaining stuff */
      char tmp[MAXLINE];
      while (MAXLINE - 1 == strlen(fgets(tmp, MAXLINE, tn)));
    }      
    
    P = realloc(P, (i+1) * sizeof(struct Proc));
    if (P == NULL) {
      fprintf(stderr, "Problems with realloc.\n");
      exit(1);
    }
    
    memset(&P[i], 0, sizeof(*P));
    
#ifdef solaris1x
    { /* SunOS allows columns to run together.  With the -j option, the CPU
       * time used can run into the numeric user id, so make sure there is
       * space between these two columns.  Also, the order of the desired
       * items is different. (L. Mark Larsen <mlarsen AT ptdcs2.intel.com>)
       */
      char buf1[45], buf2[MAXLINE];
      buf1[44] = '\0';
      sscanf(line, "%44c%[^\n]", buf1, buf2);
      sprintf(line, "%s %s", buf1, buf2);
    }
#endif
    
    sscanf(line, PSFORMAT, PSVARS);
    
#ifdef UID2USER	/* get username */
    uid2user(P[i].uid, P[i].name, sizeof(P[i].name));
#endif

#ifdef DEBUG
    if (debug) fprintf(stderr,
		      "uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, pgid=%5ld, thcount=%ld, cmd='%s'\n",
		      P[i].uid, P[i].name, P[i].pid, P[i].ppid, P[i].pgid, P[i].thcount, P[i].cmd);
#endif
    P[i].parent = P[i].child = P[i].sister = -1;
    P[i].print  = FALSE;
    i++;
  }
  if (input != NULL)
    fclose(tn);
  else
    pclose(tn);
  return i;
}

int GetRootPid(void) {
  int i;
  for (i = 0; i < NProc; i++) {
    if (P[i].pid == -1) return P[i].pid;
  }
  /* PID == 1 not found, so we'll take process with PPID == 0
   * Fix for TRU64 TruCluster with uniq PIDs
   * reported by Frank Parkin <fparki AT acxiom.co.uk> */
  for (i = 0; i < NProc; i++) {
    if (P[i].ppid == 0) return P[i].pid;
  }
  /* Should not happen */
  fprintf(stderr,
	  "%s: No process found with PID == 1 || PPID == 0, contact author.\n",
	  Progname);
  exit(1);
}

int get_pid_index(long pid) {
  int i;
  for (i = NProc - 1;i >= 0 && P[i].pid != pid; i--); /* Search process */
  return i;
}

#define EXIST(idx) ((idx) != -1)

void MakeTree(void) {
  /* Build the process hierarchy. Every process marks itself as first child
   * of it's parent or as sister of first child of it's parent */
  int me;  
  for (me = 0; me < NProc; me++) {
    int parent;
    parent = get_pid_index(P[me].ppid);
    if (parent != me && parent != -1) { /* valid process, not me */
      P[me].parent = parent;
      if (P[parent].child == -1) /* first child */
	P[parent].child = me;
      else {
	int sister;
	for (sister = P[parent].child; EXIST(P[sister].sister); sister = P[sister].sister);
	P[sister].sister = me;
      }
    }
  }
}

void MarkChildren(int i) {
  int child;
  P[i].print = TRUE;
  for (child = P[i].child; EXIST(child); child = P[child].sister)
    MarkChildren(child);
}

void MarkProcs(void) {
  int me;
  for (me = 0; me < NProc; me++) {
    if (showall) {
      P[me].print = TRUE;
    } else {
      int parent;
      if (0 == strcmp(P[me].name, name)		/* for -u */
	 || (Uoption &&
	     0 != strcmp(P[me].name, "root"))	/* for -U */
	 || P[me].pid == ipid			/* for -p */
	 || (soption
	     && NULL != strstr(P[me].cmd, str)
	     && P[me].pid != MyPid)		/* for -s */
	 ) {
	/* Mark parents */
	for (parent = P[me].parent; EXIST(parent); parent = P[parent].parent) {
	  P[parent].print = TRUE;
	}
	/* Mark children */
	MarkChildren(me);
      }
    }
#if 0 /* experimental thread compression */
    {
      int parent = P[me].parent;
      int ancestor; /* oldest parent with same cmd */
      if (0 == strcmp(P[me].cmd, P[parent].cmd)) {
	P[me].print = FALSE;
	for (parent = P[me].parent;
	     EXIST(parent) && (0 == strcmp(P[me].cmd, P[parent].cmd));
	     parent = P[parent].parent) {
	  ancestor = parent;
	}
	fprintf(stderr, "%d: %d\n",
		P[me].pid,
		P[ancestor].pid);
	P[ancestor].thcount++;
      }
    }
#endif
  }
}

void DropProcs(void) {
  int me;
  for (me = 0; me < NProc; me++) if (P[me].print) {
    int child, sister;
    /* Drop children that won't print */
    for (child = P[me].child;
	 EXIST(child) && !P[child].print; child = P[child].sister);
    P[me].child = child;
    /* Drop sisters that won't print */
    for (sister = P[me].sister;
	 EXIST(sister) && !P[sister].print; sister = P[sister].sister);
    P[me].sister = sister;
  }
}

void PrintTree(int idx, const char *head) {
  char nhead[MAXLINE], out[4 * MAXLINE], thread[16] = {'\0'};
  int child;
  
  if (head[0] == '\0' && !P[idx].print) return;
  
  if (P[idx].thcount > 1) sprintf(thread, "[%ld]", P[idx].thcount);
  
  sprintf(out,
	  "%s%s%s%s%s%s %05ld %s %s%s" /*" (ch=%d, si=%d, pr=%d)"*/,
	  C->sg,
	  head,
	  head[0] == '\0' ? "" : EXIST(P[idx].sister) ? C->barc : C->barl,
	  EXIST(P[idx].child)       ? C->p   : C->s2,
	  P[idx].pid == P[idx].pgid ? C->pgl : C->npgl,
	  C->eg,
	  P[idx].pid, P[idx].name,
	  thread,
	  P[idx].cmd
	  /*,P[idx].child,P[idx].sister,P[idx].print*/);
  
  out[Columns-1] = '\0';
  puts(out);
  
  /* Process children */
  sprintf(nhead, "%s%s ", head,
	  head[0] == '\0' ? "" : EXIST(P[idx].sister) ? C->bar : " ");
  
  for (child = P[idx].child; EXIST(child); child = P[child].sister)
    PrintTree(child, nhead);
}

void Usage(void) {
  fprintf(stderr,
	  "%s\n"
	  "%s\n\n"
	  "Usage: %s "
#ifdef DEBUG
	  "[-d] "
#endif
	  "[-f file] [-g] [-u user] [-U] [-s string] [-p pid] [-w] [pid ...]\n"
	  /*"   -a        align output\n"*/
#ifdef DEBUG
	  "   -d        print debugging info to stderr\n"
#endif
	  "   -f file   read input from <file> (- is stdin) instead of running\n"
	  "             \"%s\"\n"
	  "   -g n      use graphics chars for tree. n=1: IBM-850, n=2: VT100\n"
	  "   -u user   show only branches containing processes of <user>\n"
	  "   -U        don't show branches containing only root processes\n"
          "   -s string show only branches containing process with <string> in commandline\n"
          "   -p pid    show only branches containing process <pid>\n"
	  "   -w        wide output, not truncated to window width\n"
	  "   pid ...   process ids to start from, default is 1 (init)\n"
	  "             use 0 to also show kernel processes\n"
	  , WhatString[0] + 4, WhatString[1] + 4, Progname, PSCMD);
#ifdef HAS_PGID
  fprintf(stderr, "\n%sProcess group leaders are marked with '%s%s%s'.\n",
	  C->init, C->sg, C->pgl, C->eg);
#endif
  exit(1);
}

int main(int argc, char **argv) {
  extern int optind;
  extern char *optarg;
  int ch;
  long pid;
  int graph = G_ASCII, wide = FALSE;
  
  C = &TreeChars[graph];
  
  Progname = strrchr(argv[0],'/');
  Progname = (NULL == Progname) ? argv[0] : Progname + 1;
  
  while ((ch = getopt(argc, argv, "df:g:hp:s:u:Uw?")) != EOF)
    switch(ch) {
      /*case 'a':
	align   = TRUE;
	break;*/
#ifdef DEBUG
    case 'd':
      debug   = TRUE;
      break;
#endif
    case 'f':
      input   = optarg;
      break;
    case 'g':
      graph   = atoi(optarg);
      if (graph < 0 || graph >= G_LAST) {
	fprintf(stderr, "%s: Invalid graph parameter.\n",
		Progname);
	exit(1);
      }
      C = &TreeChars[graph];
      break;
    case 'p':
      showall = FALSE;
      ipid    = atoi(optarg);
      break;
    case 's':
      showall = FALSE;
      soption = TRUE;
      str     = optarg;
      break;
    case 'u':
      showall = FALSE;
      name    = optarg;
      if (
#ifdef solaris2x
	 (int)
#endif
	 NULL == getpwnam(name)) {
	fprintf(stderr, "%s: User '%s' does not exist.\n",
		Progname, name);
	exit(1);
      }
      break;
    case 'U':
      showall = FALSE;
      Uoption = TRUE;
      break;
    case 'w':
      wide    = TRUE;
      break;
    case 'h':
    case '?':
    default :
      Usage();
      break;
    }
  
#ifdef USE_GetProcessesDirect
  NProc = input == NULL ? GetProcessesDirect() : GetProcesses();
#else
  NProc = GetProcesses();
#endif
  
  if (NProc == 0) {
    fprintf(stderr, "%s: No processes read.\n", Progname);
    exit(1);
  }

#ifdef DEBUG
  if (debug) fprintf(stderr, "NProc = %d processes found.\n", NProc);
#endif
  
  RootPid = GetRootPid();

#ifdef DEBUG
  if (debug) fprintf(stderr, "RootPid = %d.\n", RootPid);
#endif

#if defined(UID2USER) && defined(DEBUG)
  if (debug) uid2user(0,NULL,0);
#endif
  MyPid = getpid();
  
  if (wide)
    Columns = MAXLINE - 1;
  else {
#ifdef HAS_TERMDEF
    Columns = atoi((char*)termdef(fileno(stdout),'c'));
#else
    char *env = getenv("COLUMNS");
    Columns = env ? atoi(env) : 80;
#endif
  }
  if (Columns == 0) Columns = MAXLINE - 1;
  
  printf("%s", C->init);
  
  Columns += strlen(C->sg) + strlen(C->eg); /* Don't count hidden chars */

  if (Columns >= MAXLINE) Columns = MAXLINE - 1;
  
  MakeTree();
  MarkProcs();
  DropProcs();
  
  if (argc == optind) { /* No pids */
    PrintTree(get_pid_index(RootPid), "");
  } else while (optind < argc) {
    int idx;
    pid = (long)atoi(argv[optind]);
    idx = get_pid_index(pid);
    if (idx > -1) PrintTree(idx, "");
    optind++;
  }
  free(P);
  return 0;
}

#ifdef NEED_STRSTR
/* Contributed by Paul Kern <pkern AT utcc.utoronto.ca> */
static char * strstr(s1, s2)
     register char *s1, *s2;
{
  register int n1, n2;
  
  if (n2 = strlen(s2))
    for (n1 = strlen(s1); n1 >= n2; s1++, n1--)
      if (strncmp(s1, s2, n2) == 0)
	return s1;
  return NULL;
}
#endif /* NEED_STRSTR */

/*
 * $Log: pstree.c,v $
 * Revision 2.21  2003-10-06 13:55:47+02  fred
 * Fixed SEGV under Linux when process table changes during run
 *
 * Revision 2.20  2003-07-09 20:07:29+02  fred
 * cosmetic
 *
 * Revision 2.19  2003/05/26 15:33:35  fred
 * Merged FreeBSD, (Open|Net)BSD; added Darwin (APPLE), fixed wide output
 * in FreeBSD
 *
 * Revision 2.18  2003/03/13 18:53:22  fred
 * Added getenv("COLUMNS"), cosmetic changes
 *
 * Revision 2.17  2001/12/17 12:18:02  fred
 * Changed ps call to something like ps -eo uid,pid,ppid,pgid,args under
 * AIX and Linux, workaround for AIX 5L.
 *
 * Revision 2.17  2001-12-13 08:27:00+08  chris
 * Added workaround for AIX Version >= 5
 *
 * Revision 2.16  2000-03-01 10:42:22+01  fred
 * Added support for thread count (thcount) in other OSs than AIX
 *
 * Revision 2.15  2000-03-01 10:18:56+01  fred
 * Added process group support for {Net|Open}BSD following a suggestion
 * by Ralf Meyer <ralf AT thp.Uni-Duisburg.de>
 *
 * Revision 2.14  1999-03-22 20:45:02+01  fred
 * Fixed bug when line longer than MAXLINE, set MAXLINE=512
 *
 * Revision 2.13  1998-12-17 19:31:53+01  fred
 * Fixed problem with option -f when input file is empty
 *
 * Revision 2.12  1998-12-07 17:08:59+01  fred
 * Added -f option and sun 68000 support by Paul Kern
 * <pkern AT utcc.utoronto.ca>
 *
 * Revision 2.11  1998-05-23 13:30:28+02  fred
 * Added vt100 sequences, NetBSD support
 *
 * Revision 2.10  1998-02-02 15:04:57+01  fred
 * Fixed bug in MakeTree()/get_pid_index() when parent doesn't
 * exist. Thanks to Igor Schein <igor AT andrew.air-boston.com> for the bug
 * report.
 *
 * Revision 2.9  1998-01-07 16:55:26+01  fred
 * Added support for getprocs()
 *
 * Revision 2.9  1998-01-06 17:13:19+01  fred
 * Added support for getprocs() under AIX
 *
 * Revision 2.8  1997-10-22 15:09:39+02  fred
 * Cosmetic
 *
 * Revision 2.7  1997-10-22 15:01:40+02  fred
 * Minor changes in getprocs for AIX
 *
 * Revision 2.6  1997/10/16 16:35:19  fred
 * Added uid2name() caching username lookup, added patch for Solaris 2.x
 *
 * Revision 2.5  1997-02-05 14:24:53+01  fred
 * return PrintTree when nothing to do.
 *
 * Revision 2.4  1997/02/05 09:54:08  fred
 * Fixed bug when P[i].cmd is empty
 *
 * Revision 2.3  1997-02-04 18:40:54+01  fred
 * Cosmetic
 *
 * Revision 2.2  1997-02-04 14:11:17+01  fred
 * *** empty log message ***
 *
 * Revision 2.1  1997-02-04 13:55:14+01  fred
 * Rewritten
 *
 * Revision 1.13  1997-02-04 09:01:59+01  fred
 * Start of rewrite
 *
 * Revision 1.12  1996-09-17 21:54:05+02  fred
 * *** empty log message ***
 *
 * Revision 1.11  1996-09-17 21:52:52+02  fred
 * revision added
 *
 * Revision 1.10  1996-09-17 21:45:35+02  fred
 * replace \n and \t with ? in output
 *
 * Revision 1.4  1996-09-17 21:43:14+02  fred
 * Moved under RCS, replace \n and \t with ?
 */
