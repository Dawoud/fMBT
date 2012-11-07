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
#include "log.hh"
#include <cstdio>
#include <cstdlib>
#include "helper.hh"

#ifndef DROI
#include <glib.h>
#endif

void Log::push(std::string&s) {
  print("<%s>\n",s.c_str());
  element.push(s);
}

void Log::push(const char*s)
{
  std::string ss(s);
  push(ss);
}


void Log::pop() {
  std::string s = element.top();
  element.pop();
  print("</%s>\n",s.c_str());
}

#include <stdarg.h>
void Log::vprint(const char* format,va_list ap)
{
  char* msg=NULL;

  if (vasprintf(&msg,format,ap)>0) {
    write(msg);
    std::free(msg);
  }
}

void Log::vuprint(const char* format,va_list ap)
{
  char* msg=NULL;

  if (vasprintf(&msg,format,ap)>0) {
    char* m=escape_string(msg);
    write(m);
    escape_free(m);
    std::free(msg);
  }
}

void Log::print(const char* format,va_list ap)
{
  // indentation level
  for (unsigned int i=1; i<element.size(); i++) fprintf(out, "    ");

  vprint(format,ap);   
  fflush(out);
}
void Log::print(const char* format,...)
{ 
  va_list ap;
  va_start(ap, format);
  print(format,ap);   
  va_end(ap);
}

void Log::write(int action,const char *name,const char *msg)
{
  fprintf(out,"Action %i, name \"%s\", msg \"%s\"\n",
	 action,name,msg);
}

void Log::write(const char* msg)
{
  fprintf(out,"%s",msg);
}

void Log::write(std::string& msg)
{
  write(msg.c_str());
}

void Log::error(const char** format,...)
{
    va_list ap;
    char* msg=NULL;

    va_start(ap, format);

    print(format[0],ap);

    if (vasprintf(&msg,format[1],ap)>0) {
      char* m=unescape_string(msg);
      fprintf(stderr,"%s",m);
      std::free(msg);
    }
    va_end(ap);
}
void Log::debug(const char* msg,...)
{
  if (debug_enabled) {
    va_list ap;

    va_start(ap, msg);
    write("<debug msg=\"");
    vuprint(msg,ap);
    va_end(ap);
    write("\"/>\n");
  }
}
