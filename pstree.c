/*	This is pstree V1.9 written by Fred Hucht (c) 1993/96	*
 *	EMail: fred@thp.Uni-Duisburg.DE				*
 *	Feel free to copy and redistribute in terms of the	*
 * 	GNU public license. 					*/

static char *WhatString[2]= {
  "@(#)pstree V1.9.1 by Fred Hucht (C) 1993-95",
  "@(#)EMail:fred@thp.Uni-Duisburg.DE"
  };

#define MAXLINE 256

#if defined(_AIX) || defined(___AIX)	/* AIX >= 3.1 */
/* Under AIX, we directly read the process table from the kernel */
#  define HAS_TERMDEF
#  define UID2USER
#  define _ALL_SOURCE
#  include <procinfo.h>
/*
 * #define PSCMD 	"ps -ekf"
 * #define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"
 * #define PSVARS	p[i].name, &p[i].pid, &p[i].ppid, p[i].cmd
 */
#elif defined(__linux)	/* Linux */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*d %ld %ld %ld %*35c %*s %[^\n]"
#elif defined(sparc)	/* SunOS */
/* contributed by L. Mark Larsen <mlarsen@ptdcs2.intel.com> */
#  define UID2USER
#  define PSCMD 	"ps jaxw"
#  define PSFORMAT 	"%ld %ld %*d %*d %*s %*d %*s %ld %*s %[^\n]"
#  define PSVARS 	&p[i].ppid, &p[i].pid, &p[i].uid, p[i].cmd
#elif defined(bsdi)
/* contributed by Dean Gaudet <dgaudet@hotwired.com> */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%ld %ld %ld %*d %*d %*d %*d %*d %*s %*s %*s %*s %[^\n]"
#elif defined(_BSD)	/* Untested */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*d %*c %ld %ld %ld %*d %*d %*d %*x %*d %d %*15c %*s %[^\n]"
#elif defined(__convex)	/* ConvexOS */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*s %ld %ld %ld %*d %*g %*d %*d %*21c %*s %[^\n]"
#else			/* HP-UX, A/UX etc. */
#  define PSCMD 	"ps -ef"
#  define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"
#endif

#ifndef PSVARS 		/* Set default */
# ifdef UID2USER
#  define PSVARS	&p[i].uid, &p[i].pid, &p[i].ppid, p[i].cmd
# else
#  define PSVARS	p[i].name, &p[i].pid, &p[i].ppid, p[i].cmd
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* For str...() */
#include <unistd.h>		/* For getopt */

#ifdef UID2USER
#include <pwd.h>
struct passwd *pw;
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

struct TreeChars {
  char *s1, 		/* String to intend */
       *s2, 		/* String between header and pid */
       *p, 		/* dito, when parent of printed childs */
        pgl,		/* Process group leader */
        npgl,		/* No process group leader */
        c, 		/* Last char of s1 for line with child */
        e;		/* Last char of s1 for last child */
};

static struct TreeChars
  Ascii = { " |",    "--",       "-+",       '=',    '-',    '|',    '\\' },
  Pc850 = { " \263", "\304\304", "\304\302", '\315', '\304', '\303', '\300' },
  *c;

int MyPid, NProc, Ns1, Maxlevel = 0, Alignlen, Columns;
short align = FALSE, showall = TRUE, soption = FALSE, Uoption = FALSE;
char *name = "", *str = NULL, *Progname;
long ipid = -1;

#ifdef DEBUG
int debug = FALSE;
#endif


struct {
  long uid, pid, ppid;
  int pgl;
  char name[9], cmd[MAXLINE];
  int  parent, child, pchilds, level;
} *p;

#if defined(_AIX) || defined(___AIX)	/* AIX 3.x */

#define NPROCS 10000
int getprocs() {
  struct procinfo proc[NPROCS];
  struct userinfo user;
  int i;
  int nproc = getproc(proc,NPROCS,sizeof(struct procinfo));
  
  if( NULL == (p = malloc( (nproc+1) * sizeof(*p) ))) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  for(i = 0; i < nproc; i++) {
    getuser(&proc[i],sizeof(struct procinfo),
	    &user,   sizeof(struct userinfo));
    
    p[i].uid  = proc[i].pi_uid;
    p[i].pid  = proc[i].pi_pid;
    p[i].ppid = proc[i].pi_ppid;
    p[i].pgl  = proc[i].pi_pid == proc[i].pi_pgrp;
    
    pw = getpwuid(p[i].uid);
    strcpy(p[i].name, pw->pw_name);
    
    if(proc[i].pi_stat == SZOMB) {
      strcpy(p[i].cmd, "<defunct>");
    } else {
      char *c = p[i].cmd;
      int ci = 0;
      getargs(&proc[i], sizeof(struct procinfo), c, MAXLINE - 2);
      c[MAXLINE-2] = c[MAXLINE-1] = '\0';

      /* Collect args. Stop when we encounter two '\0' */
      while(c[ci] != '\0' && (ci += strlen(&c[ci])) < MAXLINE - 2)
	c[ci++] = ' ';
      
      /* Drop trailing blanks */
      ci = strlen(c);
      while(ci > 0 && c[ci-1] == ' ') ci--;
      c[ci] = '\0';
      
      /* Insert [ui_comm] when getargs returns nothing */
      if(c[0] == '\0') {
	int l = strlen(user.ui_comm);
	c[0] = '[';
	strcpy(c+1, user.ui_comm);
	c[l+1] = ']';
	c[l+2] = '\0';
      }
    }
#ifdef DEBUG
    if(debug) fprintf(stderr,
		      "%d: uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, pgl=%d, cmd[%d]='%s'\n",
		      i, p[i].uid, p[i].name, p[i].pid, p[i].ppid,
		      p[i].pgl, strlen(p[i].cmd),p[i].cmd);
#endif
    p[i].parent = p[i].child = FALSE;
    p[i].pchilds = 0;
  }
  return nproc;
}

#else /* _AIX */

int getprocs() {
  FILE *tn;
  int len, i = 0;  extern int errno; /* For popen() */
#ifdef UID2USER
  int xx; /* For testing... */
#endif
  char line[MAXLINE], command[] = PSCMD;
  
  if(NULL == (tn = (FILE*)popen(command,"r"))) {
    fprintf(stderr, "Problems with pipe, errno = %d\n",errno);
    exit(1);
  }
#ifdef DEBUG
  if(debug) fprintf(stderr, "popen:errno = %d\n", errno);
#endif
  
  fgets(line, MAXLINE, tn); /* Throw away header line */
#ifdef DEBUG
  if(debug) fputs(line, stderr);
#endif
  
  if( NULL == (p = malloc( sizeof(*p) ))) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  while(NULL != fgets(line, MAXLINE, tn) && 10 < (len = strlen(line))) {
#ifdef DEBUG
    if(debug) {fprintf(stderr, "len=%3d ", len); fputs(line, stderr);}
#endif
    
    if( NULL == (p = realloc(p, (i+1) * sizeof(*p) ))) {
      fprintf(stderr, "Problems with realloc.\n");
      exit(1);
    }
    
#ifdef sparc
    { /* SunOS allows columns to run together.  With the -j option, the CPU
       * time used can run into the numeric user id, so make sure there is
       * space between these two columns.  Also, the order of the desired
       * items is different. */
      char buf1[45], buf2[MAXLINE];
      buf1[44] = '\0';
      sscanf(line, "%44c%[^\n]", buf1, buf2);
      sprintf(line, "%s %s", buf1, buf2);
    }
#endif
    
    sscanf(line, PSFORMAT, PSVARS);
    
#ifdef UID2USER	/* get username */
    pw = getpwuid(p[i].uid);
    strcpy(p[i].name, pw->pw_name);
#endif

#ifdef DEBUG
    if(debug) fprintf(stderr,
		      "uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, cmd='%s'\n",
		      p[i].uid, p[i].name, p[i].pid, p[i].ppid, p[i].cmd);
#endif
    p[i].parent = p[i].child = p[i].pgl = FALSE;
    p[i].pchilds = 0;
    i++;
  }
  pclose(tn);
  return i;
}
#endif /* _AIX */

#define IS_CHILD(i, pid) (p[i].ppid == pid && p[i].pid != pid)

int get_pid_index(long pid) {
  int i = 0;
  while(i < NProc && p[i].pid != pid) i++; /* Search process */
  return i;
}

int pstree1(long pid, int level) {
  int i, j;
  i = get_pid_index(pid);
  if(i == NProc) return 1; /* Return if pid does not exist */
  
  if(0 == strcmp(p[i].name, name) 		/* for -u */
     || (Uoption &&
	 0 != strcmp(p[i].name, "root"))	/* for -U */
     || p[i].pid == ipid			/* for -p */
     || (soption
	 && NULL != strstr(p[i].cmd, str)
	 && p[i].pid != MyPid)			/* for -s */
     ) p[i].parent = p[i].child = TRUE;

  p[i].level = level;
  
  if(p[i].pid == MyPid) return 0; /* Ignore my children (sh, ps)... */
  
  for(j=0; j<NProc; j++) if(IS_CHILD(j, pid)) {
    p[j].child   = p[i].child;  /* set if parent user matches */
    if(pstree1(p[j].pid, level + 1))  /* call me */
      fprintf(stderr, "This should not happen. pid=%ld\n",p[j].pid);
    p[i].parent |= p[j].parent; /* set if child user matches */
  }
  return 0;
}

#define IS_PRINTED(i) (showall || p[i].child || p[i].parent)

void pstree2(long pid, const char *head) {
  int i, j, headL, nheadL;
  char nhead[MAXLINE], out[4*MAXLINE], *Format;
  
  i = get_pid_index(pid);
  if(i == NProc) {
    fprintf(stderr, "This should not happen (2). pid=%ld\n", pid);
    exit(1);
  }
  
  if(!IS_PRINTED(i)) return; /* No need to process */
  
  for(j = 0; j < NProc; j++) if(IS_CHILD(j, pid) && IS_PRINTED(j))
    p[i].pchilds++; /* Count printed childs of i */
  
  if(p[i].pid == MyPid) p[i].pchilds = 0; /* Ignore my childs */
  
  headL = Ns1 * p[i].level - 1; /* = strlen(head)-1; */
  
#ifdef DEBUG
  if(headL != strlen(head) - 1)
    fprintf(stderr, "Ohoh, headL error. %d != %d.\n", headL, strlen(head) - 1);
#endif
  
  if(align) {
    for(j = 0; j < Alignlen - headL; j++) nhead[j] = c->s2[0];
    nhead[j] = '\0';
    Format = "%s%s%s%c %05d %-8s %s";
  }
  else {
    nhead[0] = '\0';
    Format = "%s%s%s%c %05d %s %s";
  }
  
  j = sprintf(out, Format,
	      head,
	      p[i].pchilds ? c->p : c->s2,
	      nhead,
	      p[i].pgl ? c->pgl : c->npgl,
	      pid, p[i].name, p[i].cmd);
  
  out[headL] = (out[headL] == c->e) ? c->e : c->c;
  out[Columns] = '\0';
  puts(out);
  
  if(p[i].pid == MyPid) return; /* Don't show my childs... */
#ifdef DEBUG
  fprintf(stderr, "head='%s' nhead='%s' outlen=%d\n", head, nhead, j);
#endif
  sprintf(nhead, "%s%s", head, c->s1);
  
  if(nhead[headL] == c->e) nhead[headL] = ' ';
  
  nheadL = headL + Ns1; /* = strlen(nhead)-1; */
  
  for(j = 0; j < NProc; j++) if(IS_CHILD(j, pid)) {
    if(IS_PRINTED(j) && 1 == p[i].pchilds--)
      nhead[nheadL] = c->e; /* last child gets c->e */
#ifdef DEBUG
    fprintf(stderr, "%ld '%s'\n", p[j].pid, nhead);
#endif
    pstree2(p[j].pid, nhead);
  }
}

void Pstree(long pid) {
  int i, r;
  
  r = pstree1(pid, 0); /* Pass 1 */
  
  if(r) {
    fprintf(stderr, "Process %ld does not exist!\n",pid);
    return;
  }
  
  if(align) { /* Shall output be aligned? */
    for(i = 0; i < NProc; i++)
      if(IS_PRINTED(i) && Maxlevel < p[i].level)
	Maxlevel = p[i].level;
    Alignlen = Maxlevel * Ns1 - 1;
#ifdef DEBUG
    if(debug) fprintf(stderr, "Maxlevel = %d, Alignlen = %d\n", Maxlevel, Alignlen);
#endif
  }
  
  pstree2(pid, ""); /* Pass 2 */
}

void Usage(void) {
  fprintf(stderr,
	  "%s, %s\n\n"
	  "Usage: %s [-a] "
#ifdef DEBUG
	  "[-d] "
#endif
	  "[-g] [-u user] [-U] [-s string] [-p pid] [-w] [pid ...]\n"
	  "   -a        align output\n"
#ifdef DEBUG
	  "   -d        print debugging info\n"
#endif
	  "   -g        use IBM-850 graphics chars for tree\n"
	  "   -u user   show only parts containing processes of <user>\n"
	  "   -U        don't show parts containing only root processes\n"
          "   -s string show only parts containing process with <string> in commandline\n"
          "   -p pid    show only parts containing process <pid>\n"
	  "   -w        wide output, not truncated to window width\n"
	  "   pid ...   process ids to start from, default is 1 (init)\n"
	  "             use 0 to also show kernel processes\n"
	  , WhatString[0]+4, WhatString[1]+4, Progname);
#if defined(_AIX) || defined(___AIX)	/* AIX 3.x */
  fprintf(stderr, "\nProcess group leaders are marked with '='.\n");
#endif
  exit(1);
}

int main(int argc, char **argv) {
  extern int optind;
  extern char *optarg;
  int ch;
  long pid;
  int graph = FALSE, wide = FALSE;
  Progname = strrchr(argv[0],'/');
  Progname = (NULL == Progname) ? argv[0] : Progname+1;
  
  while((ch = getopt(argc, argv, "adghp:s:u:Uw?")) != EOF)
    switch(ch) {
    case 'a':
      align   = TRUE;
      break;;
#ifdef DEBUG
    case 'd':
      debug   = TRUE;
      break;;
#endif
    case 'g':
      graph   = TRUE;
      break;;
    case 'p':
      showall = FALSE;
      ipid    = atoi(optarg);
      break;;
    case 's':
      showall = FALSE;
      soption = TRUE;
      str     = optarg;
      break;;
    case 'u':
      showall = FALSE;
      name    = optarg;
      if(NULL == getpwnam(name)) {
	fprintf(stderr, "%s: User '%s' does not exist.\n",
		Progname, name);
	exit(1);
      }
      break;;
    case 'U':
      showall = FALSE;
      Uoption = TRUE;
      break;;
    case 'w':
      wide    = TRUE;
      break;;
    case 'h':
    case '?':
    default :
      Usage();
      break;;
    }
  
  NProc = getprocs();
  MyPid = getpid();
  if(wide)
    Columns = MAXLINE-1;
  else {
#ifdef HAS_TERMDEF
    Columns = atoi((char*)termdef(fileno(stdout),'c'));
#else
    Columns = 80;
#endif
  }
  c = graph ? &Pc850 : &Ascii;

  Ns1 = strlen(c->s1);
  
#ifdef DEBUG
  if(debug) fprintf(stderr, "Columns = %d\n", Columns);
#endif
  if(Columns == 0 || Columns >= MAXLINE) Columns = MAXLINE-1;
  if(argc == optind) /* No pids */
    Pstree(1);
  else while(optind < argc) {
    pid = (long)atoi(argv[optind]);
    Pstree(pid);
    optind++;
  }
  free(p);
  return 0;
}
