/*
 * fMBT, free Model Based Testing tool
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "dparse.h"
#include "helper.hh"
#include <getopt.h>
#include <glib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>

#include "aalang.hh"
#include "config.h"
#ifndef DROI
#include <error.h>
#else
void error(int exitval, int dontcare, const char* format, ...)
{
  va_list ap;
  fprintf(stderr, "fmbt-aalc error: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(exitval);
}
#endif

extern "C" {
extern D_ParserTables parser_tables_lang;
}

void print_usage()
{
  std::printf(
    "Usage: fmbt-aalc [options] inputfile\n"
    "Options:\n"
    "    -h     print usage\n"
    "    -o     output to a file (defaults to stdout)\n"
    "    -c     compile (needs to have output file)\n"
    "    -D     define preprocessor flag\n"
    );
}

std::vector<std::string> prep;
std::string pstr;
extern std::string result;
extern aalang* obj;

std::string compile_command("g++ -fPIC -shared -x c++  - -I " 
			    INCDIR
			    " -o ");
std::string prep_command("fmbt-aalp");

/*
 * Copy-paste and some editing from d/parse.c:1875 syntax_error_report_fn
 **/
#include "d/d.h"
static void
parse_syntax_error_report(struct D_Parser *ap) {
  Parser *p = (Parser *)ap;
 
  char *fn = d_dup_pathname_str(p->user.loc.pathname);

  p->user.loc.ws++;
  while (p->user.loc.ws && isspace(*p->user.loc.ws)) {
    if (*p->user.loc.ws=='\n') {
      p->user.loc.line++;
    }
    p->user.loc.ws++;
  }

  fprintf(stderr,"%s:%d: syntac error '%.8s'\n",fn,p->user.loc.line,
	  p->user.loc.ws);

  FREE(fn);
}

int main(int argc,char** argv) {
  int c;
  bool lib=false;
  FILE* outputfile=stdout;
  static struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

  while ((c = getopt_long (argc, argv, "B:b:hco:D:v", long_opts, NULL)) != -1) {
    switch (c)
      {
      case 'v':
	printf("Version: "VERSION"\n");
	return 0;
	break;
      case 'B':
	prep_command=optarg;
	break;
      case 'b':
	compile_command=optarg;
	compile_command+=" ";
	break;
      case 'c':
	lib=true;
	break;
      case 'D':
	prep.push_back(optarg);
	pstr=pstr+" -D"+optarg+" ";
	break;
      case 'o':
	outputfile=fopen(optarg,"w");
	compile_command=compile_command+optarg;
	if (!outputfile)
          error(1,0,"cannot open output file \"%s\".",optarg);
	break;
      case 'h':
	print_usage();
	return 0;
      default:
	return 2;
      }
  }

  if (optind == argc || (lib && outputfile==stdout)) {
    print_usage();
    return -1;
  }

  char *s;
  D_Parser *p = new_D_Parser(&parser_tables_lang, 5120);

  p->loc.pathname = argv[optind];

  if (prep_command=="") {
    s=readfile(argv[optind]);
  } else {
    s=readfile(argv[optind],prep_command.c_str());
  }
  if (!s)
    error(1,0,"cannot read input file \"%s\".",argv[optind]);

  p->syntax_error_fn=parse_syntax_error_report;

  bool ret=dparse(p,s,std::strlen(s));
  ret=p->syntax_errors==0 && ret;
  free_D_Parser(p);
  p=NULL;
  g_free(s);
  s=NULL;

  if (!ret||!obj)
    error(1,0,"parsing AAL failed.");

  result=obj->stringify();
  delete obj;

  if (lib) {
    int _stdin;
    GPid pid;
    int argc;
    gchar **argv=NULL;
    GError *gerr=NULL;

    compile_command+=pstr;

    if (!g_shell_parse_argv(compile_command.c_str(),
                            &argc,&argv,&gerr) || argv==NULL)
      error(1,0,"failed to parse compiler parameters in command\n    %s",
            compile_command.c_str());

    g_spawn_async_with_pipes(
      NULL,argv,NULL,
      (GSpawnFlags)(G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD),
      NULL,NULL,&pid,&_stdin,NULL,NULL,&gerr);

    for(int i=0;i<argc;i++) {
      if (argv[i]) {
	free(argv[i]);
      }
    }
    free(argv);

    unsigned int pos=0;
    unsigned int wrote=0;
    do {
      wrote=TEMP_FAILURE_RETRY(write(_stdin,result.c_str()+pos,result.length()-pos));
      pos+=wrote;
    } while (wrote>0 && pos<result.length());
    close(_stdin);
    
    {
      int status;
      if (waitpid(pid, &status, 0)==-1)
        error(1,0,"could not read compiler status.");
      if (!WIFEXITED(status) || WEXITSTATUS(status)!=0)
        error(1,0,"compiling failed.");
    }
  } else {
    fprintf(outputfile,"%s",result.c_str());
  }
  if (outputfile!=stdout) {
    fclose(outputfile);
  }
  result="";

  return 0;
}
